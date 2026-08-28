#include "koda_r0/evidence.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>

namespace koda::r0 {
namespace {
std::string json_escape(std::string_view value) {
  std::string out;
  out.reserve(value.size() + 8);
  for (const char c : value) {
    switch (c) {
      case '\\': out += "\\\\"; break;
      case '"': out += "\\\""; break;
      case '\n': out += "\\n"; break;
      case '\r': out += "\\r"; break;
      case '\t': out += "\\t"; break;
      default: out += c;
    }
  }
  return out;
}

std::string utc_now() {
  const auto now = std::chrono::system_clock::now();
  const auto time = std::chrono::system_clock::to_time_t(now);
  std::tm tm{};
#ifdef _WIN32
  gmtime_s(&tm, &time);
#else
  gmtime_r(&time, &tm);
#endif
  std::ostringstream result;
  result << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
  return result.str();
}
}  // namespace

EvidenceLog::EvidenceLog(const std::filesystem::path& path, std::string run_id)
    : output_(path, std::ios::out | std::ios::app), run_id_(std::move(run_id)) {}

EvidenceLog::~EvidenceLog() { output_.flush(); }

Result EvidenceLog::record(State state, std::string_view operation,
                           std::string_view result, std::uint32_t error_code,
                           std::string_view detail) {
  std::lock_guard guard(mutex_);
  if (!output_.is_open()) return Result{false, "cannot open evidence log", 0};
  ++sequence_;
  output_ << "{\"schema\":\"koda.r0.event.v1\",\"run_id\":\""
          << json_escape(run_id_) << "\",\"sequence\":" << sequence_
          << ",\"timestamp\":\"" << utc_now() << "\",\"state\":\""
          << state_name(state) << "\",\"operation\":\""
          << json_escape(operation) << "\",\"result\":\""
          << json_escape(result) << "\",\"error_code\":" << error_code
          << ",\"detail\":\"" << json_escape(detail) << "\"}\n";
  output_.flush();
  if (!output_) return Result{false, "failed writing evidence log", 0};
  return Result{true, "evidence recorded", 0};
}

}  // namespace koda::r0
