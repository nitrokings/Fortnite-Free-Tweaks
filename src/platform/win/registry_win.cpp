#include "platform/win/registry_win.h"

#ifdef _WIN32
#include <windows.h>
#endif

#include <string>

namespace vnm {
namespace {

#ifdef _WIN32

std::wstring utf8_to_wide(std::string_view text) {
    if (text.empty()) {
        return {};
    }
    const int length = MultiByteToWideChar(CP_UTF8, 0, text.data(),
                                           static_cast<int>(text.size()), nullptr, 0);
    if (length <= 0) {
        return {};
    }
    std::wstring out(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), out.data(), length);
    return out;
}

struct RootKey {
    HKEY key = nullptr;
    std::wstring subkey;
    bool ok = false;
};

RootKey split_root(std::string_view path) {
    constexpr std::string_view prefix = "HKCU\\";
    if (!path.starts_with(prefix)) {
        return {};
    }
    return {HKEY_CURRENT_USER, utf8_to_wide(path.substr(prefix.size())), true};
}

std::string win32_error(LSTATUS status) {
    return "Windows error " + std::to_string(static_cast<long>(status));
}

#endif  // _WIN32

}  // namespace

RegistryValue WinRegistry::read(std::string_view path, std::string_view name) const {
    RegistryValue result;
#ifdef _WIN32
    const RootKey root = split_root(path);
    if (!root.ok) {
        return result;
    }

    HKEY key = nullptr;
    if (RegOpenKeyExW(root.key, root.subkey.c_str(), 0, KEY_QUERY_VALUE, &key) != ERROR_SUCCESS) {
        return result;
    }

    const std::wstring value_name = utf8_to_wide(name);
    DWORD type = 0;
    DWORD size = 0;
    LSTATUS status =
        RegQueryValueExW(key, value_name.c_str(), nullptr, &type, nullptr, &size);
    if (status == ERROR_SUCCESS && type == REG_DWORD && size == sizeof(DWORD)) {
        DWORD data = 0;
        status = RegQueryValueExW(key, value_name.c_str(), nullptr, &type,
                                  reinterpret_cast<LPBYTE>(&data), &size);
        if (status == ERROR_SUCCESS) {
            result.exists = true;
            result.type = "REG_DWORD";
            result.data = std::to_string(static_cast<unsigned long>(data));
        }
    }
    RegCloseKey(key);
#else
    (void)path;
    (void)name;
#endif
    return result;
}

bool WinRegistry::write_dword(std::string_view path, std::string_view name, std::uint32_t value,
                              std::string& error) {
    error.clear();
#ifdef _WIN32
    const RootKey root = split_root(path);
    if (!root.ok) {
        error = "only HKEY_CURRENT_USER paths are supported in release 0.1";
        return false;
    }

    HKEY key = nullptr;
    DWORD disposition = 0;
    LSTATUS status = RegCreateKeyExW(root.key, root.subkey.c_str(), 0, nullptr, 0, KEY_SET_VALUE,
                                     nullptr, &key, &disposition);
    if (status != ERROR_SUCCESS) {
        error = "could not open the key (" + win32_error(status) + ")";
        return false;
    }

    const std::wstring value_name = utf8_to_wide(name);
    const DWORD data = value;
    status = RegSetValueExW(key, value_name.c_str(), 0, REG_DWORD,
                            reinterpret_cast<const BYTE*>(&data), sizeof(data));
    RegCloseKey(key);
    if (status != ERROR_SUCCESS) {
        error = "could not write the value (" + win32_error(status) + ")";
        return false;
    }
    return true;
#else
    (void)path;
    (void)name;
    (void)value;
    error = "the registry is only available on Windows";
    return false;
#endif
}

bool WinRegistry::delete_value(std::string_view path, std::string_view name, std::string& error) {
    error.clear();
#ifdef _WIN32
    const RootKey root = split_root(path);
    if (!root.ok) {
        error = "only HKEY_CURRENT_USER paths are supported in release 0.1";
        return false;
    }

    HKEY key = nullptr;
    if (RegOpenKeyExW(root.key, root.subkey.c_str(), 0, KEY_SET_VALUE, &key) != ERROR_SUCCESS) {
        // Key missing means the value is already absent.
        return true;
    }
    const std::wstring value_name = utf8_to_wide(name);
    const LSTATUS status = RegDeleteValueW(key, value_name.c_str());
    RegCloseKey(key);
    if (status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND) {
        return true;
    }
    error = "could not remove the value (" + win32_error(status) + ")";
    return false;
#else
    (void)path;
    (void)name;
    error = "the registry is only available on Windows";
    return false;
#endif
}

}  // namespace vnm
