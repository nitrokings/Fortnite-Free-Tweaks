#pragma once

#include <string>
#include <string_view>

namespace vnm {

enum class RestorePointStatus {
    ExistsToday,
    Created,
    ProtectionDisabled,
    AccessDenied,
    Failed,
    Unsupported,
};

struct RestorePointResult {
    RestorePointStatus status = RestorePointStatus::Failed;
    std::string detail;
};

std::string_view to_string(RestorePointStatus status);

bool is_process_elevated();

// Creates a restore point unless one was already made today (Windows allows one
// per 24 hours, so an existing point satisfies the gate).
RestorePointResult ensure_restore_point_today();

bool enable_system_protection(std::string& detail);

}  // namespace vnm
