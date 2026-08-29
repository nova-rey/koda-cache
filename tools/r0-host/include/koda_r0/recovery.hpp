#pragma once

#include "koda_r0/volume_ownership.hpp"

namespace koda::r0 {

[[nodiscard]] Result recover_exact_device(VolumeOwnership& ownership,
                                          const Allowlist& allowlist,
                                          std::string_view confirmation);
[[nodiscard]] Result recover_exact_device(VolumeOwnership& ownership,
                                          const Allowlist& allowlist,
                                          const DeviceIdentity& observed,
                                          std::string_view confirmation);

}  // namespace koda::r0
