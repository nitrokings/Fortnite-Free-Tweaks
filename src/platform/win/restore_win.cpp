#include "platform/win/restore_win.h"

#ifdef _WIN32
#include <windows.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <random>
#include <system_error>
#endif

#include <algorithm>
#include <cctype>

namespace vnm {

std::string_view to_string(RestorePointStatus status) {
    switch (status) {
        case RestorePointStatus::ExistsToday: return "exists-today";
        case RestorePointStatus::Created: return "created";
        case RestorePointStatus::ProtectionDisabled: return "protection-disabled";
        case RestorePointStatus::AccessDenied: return "access-denied";
        case RestorePointStatus::Failed: return "failed";
        case RestorePointStatus::Unsupported: return "unsupported";
    }
    return "failed";
}

#ifdef _WIN32
namespace {

std::string lowered(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return text;
}

bool mentions(const std::string& haystack, std::string_view needle) {
    return lowered(haystack).find(lowered(std::string(needle))) != std::string::npos;
}

struct PowerShellResult {
    int exit_code = -1;
    std::string output;
};

// Writes the script to a temp file and runs it with -File, which avoids all of
// cmd.exe's quoting rules for inline -Command strings.
PowerShellResult run_script(const std::string& script) {
    PowerShellResult result;
    std::error_code ec;
    const auto directory = std::filesystem::temp_directory_path(ec);
    if (ec) {
        result.output = "no temporary directory available";
        return result;
    }

    std::random_device device;
    const std::string name = "vnm-" + std::to_string(device()) + ".ps1";
    const auto path = directory / name;

    {
        std::ofstream file(path, std::ios::binary);
        if (!file) {
            result.output = "could not write the temporary PowerShell script";
            return result;
        }
        // UTF-8 BOM so PowerShell 5.1 reads non-ASCII descriptions correctly.
        file << "\xEF\xBB\xBF" << script;
    }

    const std::string command =
        "powershell -NoProfile -NonInteractive -ExecutionPolicy Bypass -File \"" +
        path.string() + "\" 2>&1";

    FILE* pipe = _popen(command.c_str(), "r");
    if (pipe == nullptr) {
        std::filesystem::remove(path, ec);
        result.output = "could not start PowerShell";
        return result;
    }

    char buffer[512];
    while (std::fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        result.output += buffer;
    }
    result.exit_code = _pclose(pipe);
    std::filesystem::remove(path, ec);
    return result;
}

}  // namespace
#endif  // _WIN32

bool is_process_elevated() {
#ifdef _WIN32
    HANDLE token = nullptr;
    if (OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &token) == 0) {
        return false;
    }
    TOKEN_ELEVATION elevation{};
    DWORD size = sizeof(elevation);
    const bool elevated =
        GetTokenInformation(token, TokenElevation, &elevation, sizeof(elevation), &size) != 0 &&
        elevation.TokenIsElevated != 0;
    CloseHandle(token);
    return elevated;
#else
    return false;
#endif
}

RestorePointResult ensure_restore_point_today() {
#ifdef _WIN32
    if (!is_process_elevated()) {
        return {RestorePointStatus::AccessDenied,
                "VNMHelper needs administrator rights to create a restore point."};
    }

    const auto existing = run_script(
        "$today = (Get-Date).ToString('yyyyMMdd')\n"
        "$points = Get-ComputerRestorePoint -ErrorAction SilentlyContinue\n"
        "if ($points) {\n"
        "  $last = $points | Sort-Object SequenceNumber | Select-Object -Last 1\n"
        "  if ($last.CreationTime.Substring(0, 8) -eq $today) { 'VNM_STATUS=EXISTS' }\n"
        "  else { 'VNM_STATUS=NONE' }\n"
        "} else { 'VNM_STATUS=NONE' }\n");

    if (mentions(existing.output, "VNM_STATUS=EXISTS")) {
        return {RestorePointStatus::ExistsToday,
                "A restore point was already created today, which satisfies the gate."};
    }

    const auto created = run_script(
        "try {\n"
        "  Checkpoint-Computer -Description 'VNM TWEAKS' -RestorePointType 'MODIFY_SETTINGS' "
        "-ErrorAction Stop\n"
        "  'VNM_STATUS=CREATED'\n"
        "} catch { 'VNM_ERROR=' + $_.Exception.Message }\n");

    if (mentions(created.output, "VNM_STATUS=CREATED")) {
        return {RestorePointStatus::Created, "A new restore point was created."};
    }
    // Order matters: the 24-hour message also contains the words "system restore".
    if (mentions(created.output, "24 hour") || mentions(created.output, "24-hour")) {
        return {RestorePointStatus::ExistsToday,
                "Windows reports a restore point was already created in the last 24 hours."};
    }
    if (mentions(created.output, "0x80042302") || mentions(created.output, "0x80070422") ||
        mentions(created.output, "is disabled") || mentions(created.output, "not enabled")) {
        return {RestorePointStatus::ProtectionDisabled,
                "System Protection is switched off, so Windows cannot create a restore point. "
                "VNM TWEAKS can switch it on for the system drive and try again."};
    }
    if (mentions(created.output, "denied") || mentions(created.output, "administrator")) {
        return {RestorePointStatus::AccessDenied,
                "Windows refused the request because the helper is not elevated."};
    }
    return {RestorePointStatus::Failed, created.output};
#else
    return {RestorePointStatus::Unsupported,
            "System restore points are a Windows feature; this build cannot create one."};
#endif
}

bool enable_system_protection(std::string& detail) {
#ifdef _WIN32
    if (!is_process_elevated()) {
        detail = "Administrator rights are required.";
        return false;
    }
    const auto result = run_script(
        "try {\n"
        "  Enable-ComputerRestore -Drive \"$env:SystemDrive\\\" -ErrorAction Stop\n"
        "  'VNM_STATUS=ENABLED'\n"
        "} catch { 'VNM_ERROR=' + $_.Exception.Message }\n");
    if (mentions(result.output, "VNM_STATUS=ENABLED")) {
        detail = "System Protection was enabled for the system drive.";
        return true;
    }
    detail = result.output;
    return false;
#else
    detail = "System Protection is a Windows feature.";
    return false;
#endif
}

}  // namespace vnm
