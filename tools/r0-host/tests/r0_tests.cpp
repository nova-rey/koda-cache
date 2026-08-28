#include "koda_r0/device_identity.hpp"
#include "koda_r0/evidence.hpp"
#include "koda_r0/libkrun_worker.hpp"

#include <cassert>
#include <filesystem>
#include <fstream>
#include <iostream>

using namespace koda::r0;

static Allowlist disposable() {
  Allowlist result;
  result.device.volume_guid = "\\\\?\\Volume{r0}";
  result.device.volume_serial = "VOL-R0";
  result.device.disk_guid = "DISK-R0";
  result.device.storage_duid = "DUID-R0";
  result.device.model = "disposable";
  result.device.device_serial = "SERIAL-R0";
  result.device.bus = "test";
  result.device.path = "\\\\.\\PhysicalDrive99";
  result.device.length_bytes = 64 * 1024 * 1024;
  result.device.logical_sector_bytes = 512;
  result.device.physical_sector_bytes = 4096;
  result.device.label = "KODA-R0-DISPOSABLE";
  result.reserved.offset_bytes = 1024 * 1024;
  result.reserved.length_bytes = 32 * 1024 * 1024;
  result.forbidden_device_fingerprints.push_back("OS-DISK-FINGERPRINT");
  return result;
}

int main() {
  auto allowlist = disposable();
  assert(validate_allowlist(allowlist, allowlist.device, "KODA-R0-DISPOSABLE").ok);
  assert(!validate_allowlist(allowlist, allowlist.device, "").ok);
  auto altered = allowlist.device;
  altered.disk_guid = "OTHER";
  assert(!validate_allowlist(allowlist, altered, "KODA-R0-DISPOSABLE").ok);
  assert(validate_region(allowlist, 1024 * 1024, 4096).ok);
  assert(!validate_region(allowlist, 0, 4096).ok);
  assert(!validate_region(allowlist, 1024 * 1024 + 1, 4096).ok);
  assert(std::string(state_name(State::WindowsOffline)) == "WINDOWS_OFFLINE");

  const auto path = std::filesystem::temp_directory_path() / "koda-r0-events.jsonl";
  std::filesystem::remove(path);
  {
    EvidenceLog log(path, "test-run");
    assert(log.record(State::WindowsReady, "test", "ok").ok);
    assert(log.record(State::Failed, "test", "failed", 5, "quoted \"detail\"").ok);
    assert(log.sequence() == 2);
  }
  std::ifstream input(path);
  const std::string contents((std::istreambuf_iterator<char>(input)), {});
  assert(contents.find("koda.r0.event.v1") != std::string::npos);
  assert(contents.find("quoted \\\"detail\\\"") != std::string::npos);
  std::filesystem::remove(path);

  auto worker = make_libkrun_worker();
  const auto unsupported = worker->start(LibkrunConfig{});
  assert(!unsupported.ok);
  std::cout << "r0 safety and evidence tests passed\n";
}
