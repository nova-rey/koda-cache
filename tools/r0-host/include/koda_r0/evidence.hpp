#pragma once

#include "koda_r0/types.hpp"
#include <filesystem>
#include <fstream>
#include <mutex>

namespace koda::r0 {

class EvidenceLog {
 public:
  explicit EvidenceLog(const std::filesystem::path& path,
                       std::string run_id = "r0-run");
  ~EvidenceLog();
  EvidenceLog(const EvidenceLog&) = delete;
  EvidenceLog& operator=(const EvidenceLog&) = delete;

  [[nodiscard]] Result record(State state, std::string_view operation,
                              std::string_view result,
                              std::uint32_t error_code = 0,
                              std::string_view detail = {});
  [[nodiscard]] std::uint64_t sequence() const noexcept { return sequence_; }

 private:
  std::ofstream output_;
  std::string run_id_;
  std::uint64_t sequence_{0};
  mutable std::mutex mutex_;
};

}  // namespace koda::r0
