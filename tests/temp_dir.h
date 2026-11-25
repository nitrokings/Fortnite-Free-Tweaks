#pragma once

#include <filesystem>
#include <system_error>
#include <utility>

#include "core/journal.h"

namespace vnm::test {

class TempDir {
public:
    TempDir() {
        path = std::filesystem::temp_directory_path() /
               ("vnm-test-" + new_entry_id());
        std::error_code ec;
        std::filesystem::create_directories(path, ec);
    }

    ~TempDir() {
        std::error_code ec;
        std::filesystem::remove_all(path, ec);
    }

    TempDir(const TempDir&) = delete;
    TempDir& operator=(const TempDir&) = delete;

    std::filesystem::path file(const std::string& name) const { return path / name; }

    std::filesystem::path path;
};

}  // namespace vnm::test
