#include "koda_r0/device_identity.hpp"

#include <limits>

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>
#endif

namespace koda::r0 {

const char* state_name(State state) noexcept {
  switch (state) {
    case State::WindowsReady: return "WINDOWS_READY";
    case State::WindowsFlushed: return "WINDOWS_FLUSHED";
    case State::WindowsLocked: return "WINDOWS_LOCKED";
    case State::WindowsDismounted: return "WINDOWS_DISMOUNTED";
    case State::WindowsOffline: return "WINDOWS_OFFLINE";
    case State::HostFilesystemInaccessible: return "HOST_FILESYSTEM_INACCESSIBLE";
    case State::GuestAttached: return "GUEST_ATTACHED";
    case State::GuestIoComplete: return "GUEST_IO_COMPLETE";
    case State::GuestFlushed: return "GUEST_FLUSHED";
    case State::GuestReleased: return "GUEST_RELEASED";
    case State::WindowsReopen: return "WINDOWS_REOPEN";
    case State::WindowsOnline: return "WINDOWS_ONLINE";
    case State::WindowsMounted: return "WINDOWS_MOUNTED";
    case State::NtfsVerified: return "NTFS_VERIFIED";
    case State::Failed: return "FAILED";
  }
  return "UNKNOWN";
}

std::string fingerprint(const DeviceIdentity& identity) {
  return identity.disk_guid + "|" + identity.device_serial + "|" +
         identity.storage_duid + "|" + std::to_string(identity.length_bytes);
}

std::vector<DeviceIdentity> enumerate_volume_candidates() {
  std::vector<DeviceIdentity> result;
#ifdef _WIN32
  wchar_t volume_name[MAX_PATH]{};
  HANDLE find = FindFirstVolumeW(volume_name, MAX_PATH);
  if (find == INVALID_HANDLE_VALUE) return result;
  do {
    wchar_t label[MAX_PATH]{};
    DWORD serial = 0, max_component = 0, flags = 0;
    wchar_t filesystem[32]{};
    DeviceIdentity candidate;
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS,
                                         volume_name, -1, nullptr, 0, nullptr, nullptr);
    if (size > 1) {
      std::string utf8(static_cast<std::size_t>(size - 1), '\0');
      WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, volume_name, -1,
                          utf8.data(), size, nullptr, nullptr);
      candidate.volume_guid = std::move(utf8);
    }
    if (GetVolumeInformationW(volume_name, label, MAX_PATH, &serial,
                              &max_component, &flags, filesystem, 32)) {
      const int label_size = WideCharToMultiByte(CP_UTF8, 0, label, -1, nullptr, 0, nullptr, nullptr);
      if (label_size > 1) {
        candidate.label.resize(static_cast<std::size_t>(label_size - 1));
        WideCharToMultiByte(CP_UTF8, 0, label, -1, candidate.label.data(), label_size, nullptr, nullptr);
      }
      candidate.volume_serial = std::to_string(serial);
    }
    // Disk/GPT/storage identity is intentionally left blank here and must be
    // populated by the enrollment probe before this candidate can be used.
    result.push_back(std::move(candidate));
  } while (FindNextVolumeW(find, volume_name, MAX_PATH));
  FindVolumeClose(find);
#endif
  return result;
}

static Result reject(std::string message, std::uint32_t code = 0) {
  return Result{false, std::move(message), code};
}

Result validate_allowlist(const Allowlist& allowlist,
                          const DeviceIdentity& observed,
                          std::string_view confirmation) {
  if (allowlist.schema != "koda.r0.device-allowlist.v1")
    return reject("unsupported allowlist schema");
  if (confirmation != "KODA-R0-DISPOSABLE")
    return reject("destructive confirmation token is required");
  if (allowlist.device.label != "KODA-R0-DISPOSABLE")
    return reject("allowlist label is not the disposable test label");
  const auto& expected = allowlist.device;
  if (expected.volume_guid.empty() || expected.disk_guid.empty() ||
      expected.device_serial.empty() || expected.storage_duid.empty() ||
      expected.length_bytes == 0 || expected.path.empty())
    return reject("allowlist is missing a stable device identity");
  if (observed.volume_guid != expected.volume_guid ||
      observed.volume_serial != expected.volume_serial ||
      observed.disk_guid != expected.disk_guid ||
      observed.storage_duid != expected.storage_duid ||
      observed.device_serial != expected.device_serial ||
      observed.length_bytes != expected.length_bytes ||
      observed.path != expected.path || observed.label != expected.label)
    return reject("observed device does not exactly match allowlist");
  const auto observed_fp = fingerprint(observed);
  for (const auto& forbidden : allowlist.forbidden_device_fingerprints)
    if (forbidden.empty() || forbidden == observed_fp)
      return reject("device matches a forbidden device fingerprint");
  return Result{true, "allowlist match", 0};
}

Result validate_region(const Allowlist& allowlist, std::uint64_t offset,
                       std::uint64_t length) {
  if (length == 0 || offset > std::numeric_limits<std::uint64_t>::max() - length)
    return reject("raw region overflows device address space");
  const auto end = offset + length;
  const auto reserved_end = allowlist.reserved.offset_bytes +
                            allowlist.reserved.length_bytes;
  if (allowlist.reserved.length_bytes == 0 ||
      allowlist.reserved.offset_bytes > allowlist.device.length_bytes ||
      reserved_end < allowlist.reserved.offset_bytes || reserved_end > allowlist.device.length_bytes ||
      offset < allowlist.reserved.offset_bytes || end > reserved_end)
    return reject("raw I/O is outside the enrolled reserved region");
  if (offset % 4096 != 0 || length % 4096 != 0)
    return reject("raw I/O is not 4096-byte aligned");
  return Result{true, "region is allowed", 0};
}

}  // namespace koda::r0
