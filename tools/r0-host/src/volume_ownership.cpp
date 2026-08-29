#include "koda_r0/volume_ownership.hpp"

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <winioctl.h>

#include <string>

namespace {
std::wstring wide(std::string_view value) {
  if (value.empty()) return {};
  const int count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                        value.data(), static_cast<int>(value.size()),
                                        nullptr, 0);
  if (count <= 0) return {};
  std::wstring result(static_cast<std::size_t>(count), L'\0');
  MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, value.data(),
                      static_cast<int>(value.size()), result.data(), count);
  return result;
}
}
#endif

namespace koda::r0 {

VolumeOwnership::~VolumeOwnership() {
#ifdef _WIN32
  if (volume_handle_ != nullptr) CloseHandle(volume_handle_);
#endif
}

Result VolumeOwnership::flush_lock_dismount_offline(
    const Allowlist& allowlist, std::string_view confirmation) {
  if (confirmation != "KODA-R0-DISPOSABLE")
    return Result{false, "destructive confirmation token is required", 0};
  if (allowlist.device.volume_guid.empty() ||
      allowlist.device.label != "KODA-R0-DISPOSABLE")
    return Result{false, "volume identity is not an enrolled disposable volume", 0};
#ifdef _WIN32
  if (volume_handle_ != nullptr)
    return Result{false, "volume is already owned by this harness", 0};
  const auto path = wide(allowlist.device.volume_guid);
  HANDLE handle = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                              OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (handle == INVALID_HANDLE_VALUE)
    return Result{false, "CreateFileW(volume) failed", GetLastError()};
  if (!FlushFileBuffers(handle)) {
    const auto error = GetLastError(); CloseHandle(handle);
    return Result{false, "FlushFileBuffers failed", error};
  }
  evidence_.record(State::WindowsFlushed, "FlushFileBuffers", "ok");
  DWORD returned = 0;
  if (!DeviceIoControl(handle, FSCTL_LOCK_VOLUME, nullptr, 0, nullptr, 0,
                       &returned, nullptr)) {
    const auto error = GetLastError(); CloseHandle(handle);
    return Result{false, "FSCTL_LOCK_VOLUME failed; refusing force dismount", error};
  }
  locked_ = true;
  evidence_.record(State::WindowsLocked, "FSCTL_LOCK_VOLUME", "ok");
  if (!DeviceIoControl(handle, FSCTL_DISMOUNT_VOLUME, nullptr, 0, nullptr, 0,
                       &returned, nullptr)) {
    const auto error = GetLastError(); CloseHandle(handle); locked_ = false;
    return Result{false, "FSCTL_DISMOUNT_VOLUME failed", error};
  }
  evidence_.record(State::WindowsDismounted, "FSCTL_DISMOUNT_VOLUME", "ok");
  // After the explicit dismount, a mounted query must fail with NOT_READY.
  SetLastError(ERROR_SUCCESS);
  if (DeviceIoControl(handle, FSCTL_IS_VOLUME_MOUNTED, nullptr, 0, nullptr, 0,
                      &returned, nullptr) || GetLastError() != ERROR_NOT_READY) {
    const auto error = GetLastError(); CloseHandle(handle); locked_ = false;
    return Result{false, "could not prove volume was dismounted", error};
  }
  if (!DeviceIoControl(handle, IOCTL_VOLUME_OFFLINE, nullptr, 0, nullptr, 0,
                       &returned, nullptr)) {
    const auto error = GetLastError(); CloseHandle(handle); locked_ = false;
    return Result{false, "IOCTL_VOLUME_OFFLINE failed", error};
  }
  volume_handle_ = handle;
  offline_ = true;
  evidence_.record(State::WindowsOffline, "IOCTL_VOLUME_OFFLINE", "ok");
  HANDLE probe = CreateFileW(path.c_str(), GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr,
                             OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (probe != INVALID_HANDLE_VALUE) {
    CloseHandle(probe);
    DeviceIoControl(handle, IOCTL_VOLUME_ONLINE, nullptr, 0, nullptr, 0,
                    &returned, nullptr);
    CloseHandle(handle); volume_handle_ = nullptr; offline_ = false; locked_ = false;
    return Result{false, "offline volume remained independently writable", 0};
  }
  const auto probe_error = GetLastError();
  if (probe_error != ERROR_NOT_READY && probe_error != ERROR_INVALID_FUNCTION) {
    DeviceIoControl(handle, IOCTL_VOLUME_ONLINE, nullptr, 0, nullptr, 0,
                    &returned, nullptr);
    CloseHandle(handle); volume_handle_ = nullptr; offline_ = false; locked_ = false;
    return Result{false, "offline access probe was inconclusive", probe_error};
  }
  evidence_.record(State::HostFilesystemInaccessible, "volume access probe", "ERROR_NOT_READY");
  return Result{true, "volume flushed, locked, dismounted, and offline", 0};
#else
  (void)allowlist;
  return Result{false, "Windows ownership operations require Windows", 0};
#endif
}

Result VolumeOwnership::mark_guest_attached() {
  if (!offline_) return Result{false, "guest cannot attach before volume is offline", 0};
  const auto result = evidence_.record(State::GuestAttached, "libkrun attach", "ok");
  return result.ok ? Result{true, "guest may attach", 0} : result;
}

Result VolumeOwnership::return_to_windows(const Allowlist& allowlist,
                                          const GuestReport& guest) {
  if (!offline_ || !locked_)
    return Result{false, "Windows return requires an offline locked volume", 0};
  if (!guest.io_complete || !guest.flushed || !guest.released)
    return Result{false, "guest did not prove I/O completion, flush, and release", 0};
#ifdef _WIN32
  evidence_.record(State::GuestIoComplete, "guest report", "ok");
  evidence_.record(State::GuestFlushed, "guest fsync", "ok");
  evidence_.record(State::GuestReleased, "guest close", "ok");
  // Before changing the volume state, require an exclusive reopen of the
  // allowlisted raw namespace. This is the positive host-side evidence that
  // no guest/VMM handle remains. A failed or ambiguous probe fails closed.
  const auto raw_path = wide(allowlist.device.path);
  HANDLE reopen = CreateFileW(raw_path.c_str(), GENERIC_READ | GENERIC_WRITE,
                              0, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                              nullptr);
  if (reopen == INVALID_HANDLE_VALUE)
    return Result{false, "exclusive raw reopen failed; refusing Windows online", GetLastError()};
  evidence_.record(State::WindowsReopen, "exclusive raw reopen", "ok");
  CloseHandle(reopen);
  DWORD returned = 0;
  if (!DeviceIoControl(static_cast<HANDLE>(volume_handle_), IOCTL_VOLUME_ONLINE,
                       nullptr, 0, nullptr, 0, &returned, nullptr))
    return Result{false, "IOCTL_VOLUME_ONLINE failed", GetLastError()};
  offline_ = false;
  evidence_.record(State::WindowsOnline, "IOCTL_VOLUME_ONLINE", "ok");
  if (!CloseHandle(static_cast<HANDLE>(volume_handle_)))
    return Result{false, "closing volume handle failed", GetLastError()};
  volume_handle_ = nullptr;
  locked_ = false;
  // Mount-manager/NTFS validation is deliberately a required external step;
  // this method never reports it as successful without observed evidence.
  return Result{false, "volume online; remount and NTFS validation are required", 0};
#else
  (void)allowlist;
  return Result{false, "Windows ownership operations require Windows", 0};
#endif
}

Result VolumeOwnership::recover(const Allowlist& allowlist) {
#ifdef _WIN32
  if (!offline_) return Result{true, "device is not held offline", 0};
  DWORD returned = 0;
  if (!DeviceIoControl(static_cast<HANDLE>(volume_handle_), IOCTL_VOLUME_ONLINE,
                       nullptr, 0, nullptr, 0, &returned, nullptr))
    return Result{false, "recovery IOCTL_VOLUME_ONLINE failed", GetLastError()};
  offline_ = false;
  CloseHandle(static_cast<HANDLE>(volume_handle_)); volume_handle_ = nullptr;
  locked_ = false;
  evidence_.record(State::WindowsOnline, "recovery", "ok");
  (void)allowlist;
  return Result{true, "exact held volume returned online", 0};
#else
  (void)allowlist;
  return Result{false, "Windows ownership operations require Windows", 0};
#endif
}

}  // namespace koda::r0
