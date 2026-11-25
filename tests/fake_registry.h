#pragma once

#include <map>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

#include "core/tweak.h"

namespace vnm::test {

class FakeRegistry : public IRegistry {
public:
    struct Key {
        std::string path;
        std::string name;
        bool operator<(const Key& other) const {
            return std::tie(path, name) < std::tie(other.path, other.name);
        }
    };

    std::map<Key, std::string> values;
    std::vector<std::string> writeLog;
    bool failWrites = false;

    RegistryValue read(std::string_view path, std::string_view name) const override {
        RegistryValue result;
        const auto it = values.find(Key{std::string(path), std::string(name)});
        if (it != values.end()) {
            result.exists = true;
            result.type = "REG_DWORD";
            result.data = it->second;
        }
        return result;
    }

    bool write_dword(std::string_view path, std::string_view name, std::uint32_t value,
                     std::string& error) override {
        if (failWrites) {
            error = "simulated write failure";
            return false;
        }
        values[Key{std::string(path), std::string(name)}] = std::to_string(value);
        writeLog.push_back(std::string(path) + "|" + std::string(name) + "=" +
                           std::to_string(value));
        return true;
    }

    bool delete_value(std::string_view path, std::string_view name,
                      std::string& error) override {
        if (failWrites) {
            error = "simulated delete failure";
            return false;
        }
        values.erase(Key{std::string(path), std::string(name)});
        return true;
    }
};

}  // namespace vnm::test
