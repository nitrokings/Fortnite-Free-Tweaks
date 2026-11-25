#pragma once

#include <filesystem>
#include <string>

namespace vnm {

// %LOCALAPPDATA%\VNMTweaks on Windows, $XDG_DATA_HOME/VNMTweaks elsewhere.
// VNMTWEAKS_DATA_DIR overrides both (used by tests and by the release checks).
std::filesystem::path app_data_dir();
std::filesystem::path journal_dir();

// Journal file for the current month.
std::filesystem::path current_journal_file();

// "0.1.0" in CI builds, "dev" when the macro is missing.
std::string app_version();

}  // namespace vnm
