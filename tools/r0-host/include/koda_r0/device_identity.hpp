#pragma once

#include "koda_r0/types.hpp"
#include <string_view>
#include <vector>

namespace koda::r0 {

[[nodiscard]] std::string fingerprint(const DeviceIdentity& identity);
// Inventory is descriptive only. Returned entries with missing stable fields
// are never eligible for destructive operations.
[[nodiscard]] std::vector<DeviceIdentity> enumerate_volume_candidates();
[[nodiscard]] Result validate_allowlist(const Allowlist& allowlist,
                                         const DeviceIdentity& observed,
                                         std::string_view confirmation);
[[nodiscard]] Result validate_region(const Allowlist& allowlist,
                                     std::uint64_t offset,
                                     std::uint64_t length);

}  // namespace koda::r0
