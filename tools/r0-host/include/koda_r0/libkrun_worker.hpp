#pragma once

#include "koda_r0/types.hpp"
#include <filesystem>
#include <memory>

namespace koda::r0 {

struct LibkrunConfig {
  std::filesystem::path disk_path;
  std::string block_id{"KODA-R0-DISK"};
  bool raw{true};
  bool direct_io{false};
  bool sync_full{true};
};

class LibkrunWorker {
 public:
  virtual ~LibkrunWorker() = default;
  [[nodiscard]] virtual Result start(const LibkrunConfig& config) = 0;
  [[nodiscard]] virtual Result wait(GuestReport& report) = 0;
  virtual void terminate() noexcept = 0;
};

// The production experiment supplies a Windows implementation which loads a
// pinned krun.dll and runs this object in a kill-on-close Job Object. Keeping
// the seam abstract makes safety/state tests runnable without Windows or DLLs.
[[nodiscard]] std::unique_ptr<LibkrunWorker> make_libkrun_worker();

}  // namespace koda::r0
