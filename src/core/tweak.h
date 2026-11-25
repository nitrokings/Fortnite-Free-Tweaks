#pragma once

// The tweak contract. Every item in VNM TWEAKS must be able to:
//   detect()  - read the machine's current state
//   preview() - describe the exact target and before/after values
//   apply()   - make the change and write a journal entry
//   undo()    - restore the exact previous state
// An item that cannot do all four is not a tweak; it is a guide and must not
// have an Apply button.

#include <cstdint>
#include <string>
#include <string_view>

namespace vnm {

enum class Risk { Safe, Moderate, Advanced };
enum class Revert { Instant, NeedsReboot };
enum class TweakState { On, Off, Unknown };

inline std::string_view to_string(Risk risk) {
    switch (risk) {
        case Risk::Safe: return "Safe";
        case Risk::Moderate: return "Moderate";
        case Risk::Advanced: return "Advanced";
    }
    return "Unknown";
}

inline std::string_view to_string(TweakState state) {
    switch (state) {
        case TweakState::On: return "On";
        case TweakState::Off: return "Off";
        case TweakState::Unknown: return "Unknown";
    }
    return "Unknown";
}

struct RegistryValue {
    bool exists = false;
    std::string data;
    std::string type;
};

// Implemented for real on Windows (win/registry_win.cpp) and in memory for
// tests. Paths are written as "HKCU\\Software\\..." - release 0.1 only allows
// HKEY_CURRENT_USER, which needs no elevation.
class IRegistry {
public:
    virtual ~IRegistry() = default;
    virtual RegistryValue read(std::string_view path, std::string_view name) const = 0;
    virtual bool write_dword(std::string_view path, std::string_view name,
                             std::uint32_t value, std::string& error) = 0;
    virtual bool delete_value(std::string_view path, std::string_view name,
                              std::string& error) = 0;
};

struct DetectResult {
    TweakState state = TweakState::Unknown;
    std::string detail;  // human-readable reason, always filled in
};

struct ChangePlan {
    std::string itemId;
    std::string title;
    std::string target;      // registry path, e.g. HKCU\Software\...
    std::string valueName;
    std::string valueType;   // "REG_DWORD"
    std::string before;      // "" means the value did not exist
    std::string after;
    std::string session;
    std::string restorePoint;  // timestamp of the restore point used, or ""
    Risk risk = Risk::Safe;
    Revert revert = Revert::Instant;
};

}  // namespace vnm
