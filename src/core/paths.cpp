#include "core/paths.h"

#include <chrono>
#include <cstdlib>
#include <ctime>

#include "core/journal.h"

namespace vnm {
namespace {

std::filesystem::path env_path(const char* name) {
    const char* value = std::getenv(name);
    if (value == nullptr || *value == '\0') {
        return {};
    }
    return std::filesystem::path(value);
}

}  // namespace

std::filesystem::path app_data_dir() {
    if (const auto override = env_path("VNMTWEAKS_DATA_DIR"); !override.empty()) {
        return override;
    }
#ifdef _WIN32
    if (const auto local = env_path("LOCALAPPDATA"); !local.empty()) {
        return local / "VNMTweaks";
    }
#else
    if (const auto xdg = env_path("XDG_DATA_HOME"); !xdg.empty()) {
        return xdg / "VNMTweaks";
    }
    if (const auto home = env_path("HOME"); !home.empty()) {
        return home / ".local" / "share" / "VNMTweaks";
    }
#endif
    return std::filesystem::current_path() / "VNMTweaksData";
}

std::filesystem::path journal_dir() { return app_data_dir() / "journal"; }

std::filesystem::path current_journal_file() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &seconds);
#else
    localtime_r(&seconds, &tm);
#endif
    return monthly_journal_path(journal_dir(), tm.tm_year + 1900, tm.tm_mon + 1);
}

std::string app_version() {
#ifdef VNM_VERSION
    return VNM_VERSION;
#else
    return "dev";
#endif
}

}  // namespace vnm
