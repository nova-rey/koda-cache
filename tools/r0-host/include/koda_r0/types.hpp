#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace koda::r0 {

enum class State {
  WindowsReady,
  WindowsFlushed,
  WindowsLocked,
  WindowsDismounted,
  WindowsOffline,
  HostFilesystemInaccessible,
  GuestAttached,
  GuestIoComplete,
  GuestFlushed,
  GuestReleased,
  WindowsReopen,
  WindowsOnline,
  WindowsMounted,
  NtfsVerified,
  Failed,
};

[[nodiscard]] const char* state_name(State state) noexcept;

struct DeviceIdentity {
  std::string volume_guid;
  std::string volume_serial;
  std::string disk_guid;
  std::string storage_duid;
  std::string model;
  std::string device_serial;
  std::string bus;
  std::string path;
  std::uint64_t length_bytes{};
  std::uint32_t logical_sector_bytes{};
  std::uint32_t physical_sector_bytes{};
  std::optional<std::uint32_t> azure_lun;
  std::string label;
};

struct ReservedRegion {
  std::uint64_t offset_bytes{};
  std::uint64_t length_bytes{};
  std::uint64_t record_bytes{128 * 1024};
};

struct Allowlist {
  std::string schema{"koda.r0.device-allowlist.v1"};
  DeviceIdentity device;
  ReservedRegion reserved;
  std::vector<std::string> forbidden_device_fingerprints;
};

struct GuestReport {
  bool io_complete{false};
  bool flushed{false};
  bool released{false};
  std::uint64_t capacity_bytes{};
  std::uint32_t logical_sector_bytes{};
  std::string challenge_sha256;
  std::string response_sha256;
  std::string message;
};

struct Result {
  bool ok{false};
  std::string message;
  std::uint32_t error_code{};
};

}  // namespace koda::r0
