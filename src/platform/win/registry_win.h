#pragma once

#include "core/tweak.h"

namespace vnm {

// HKEY_CURRENT_USER only in release 0.1. Paths are written as
// "HKCU\\Software\\Microsoft\\GameBar"; anything else is refused.
class WinRegistry : public IRegistry {
public:
    RegistryValue read(std::string_view path, std::string_view name) const override;
    bool write_dword(std::string_view path, std::string_view name, std::uint32_t value,
                     std::string& error) override;
    bool delete_value(std::string_view path, std::string_view name, std::string& error) override;
};

}  // namespace vnm
