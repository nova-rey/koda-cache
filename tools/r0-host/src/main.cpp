#include "koda_r0/device_identity.hpp"
#include "koda_r0/evidence.hpp"
#include "koda_r0/libkrun_worker.hpp"

#include <iostream>

int main(int argc, char** argv) {
  if (argc < 2 || std::string_view(argv[1]) == "help" ||
      std::string_view(argv[1]) == "--help") {
    std::cout << "koda-r0 (disposable feasibility harness)\n"
                 "commands: inventory enroll baseline image-proof device-probe "
                 "cycle failure-test recover vmm-worker\n";
    return argc < 2 ? 2 : 0;
  }
  std::cerr << "koda-r0: command is not available in this non-Windows build: "
            << argv[1] << "\n";
  return 3;
}
