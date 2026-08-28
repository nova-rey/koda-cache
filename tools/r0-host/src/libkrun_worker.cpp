#include "koda_r0/libkrun_worker.hpp"

namespace koda::r0 {
namespace {
class UnsupportedWorker final : public LibkrunWorker {
 public:
  Result start(const LibkrunConfig&) override {
    return Result{false, "libkrun worker is only available in the Windows R0 build", 0};
  }
  Result wait(GuestReport&) override {
    return Result{false, "libkrun worker is only available in the Windows R0 build", 0};
  }
  void terminate() noexcept override {}
};
}  // namespace

std::unique_ptr<LibkrunWorker> make_libkrun_worker() {
  // The Windows implementation is intentionally a seam for the pinned
  // krun.dll child process. No fake guest success is returned on other hosts.
  return std::make_unique<UnsupportedWorker>();
}
}  // namespace koda::r0
