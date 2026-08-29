#pragma once

#include "koda_r0/evidence.hpp"
#include "koda_r0/types.hpp"

namespace koda::r0 {

class VolumeOwnership {
 public:
  explicit VolumeOwnership(EvidenceLog& evidence) : evidence_(evidence) {}
  ~VolumeOwnership();
  VolumeOwnership(const VolumeOwnership&) = delete;
  VolumeOwnership& operator=(const VolumeOwnership&) = delete;

  [[nodiscard]] Result flush_lock_dismount_offline(
      const Allowlist& allowlist, std::string_view confirmation);
  [[nodiscard]] Result mark_guest_attached();
  [[nodiscard]] Result return_to_windows(const Allowlist& allowlist,
                                         const GuestReport& guest);
  [[nodiscard]] Result recover(const Allowlist& allowlist);
  [[nodiscard]] bool guest_may_own() const noexcept { return offline_; }

 private:
  EvidenceLog& evidence_;
  bool locked_{false};
  bool offline_{false};
#ifdef _WIN32
  void* volume_handle_{nullptr};
#endif
};

}  // namespace koda::r0
