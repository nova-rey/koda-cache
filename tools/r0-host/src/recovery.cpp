#include "koda_r0/recovery.hpp"
#include "koda_r0/device_identity.hpp"

namespace koda::r0 {

Result recover_exact_device(VolumeOwnership& ownership,
                            const Allowlist& allowlist,
                            std::string_view confirmation) {
  if (confirmation != "KODA-R0-DISPOSABLE")
    return Result{false, "recovery confirmation token is required", 0};
  if (allowlist.schema != "koda.r0.device-allowlist.v1" ||
      allowlist.device.label != "KODA-R0-DISPOSABLE")
    return Result{false, "recovery allowlist is not an exact disposable-device record", 0};
  return ownership.recover(allowlist);
}

Result recover_exact_device(VolumeOwnership& ownership,
                            const Allowlist& allowlist,
                            const DeviceIdentity& observed,
                            std::string_view confirmation) {
  const auto identity = validate_allowlist(allowlist, observed, confirmation);
  if (!identity.ok) return identity;
  return ownership.recover(allowlist);
}

}  // namespace koda::r0
