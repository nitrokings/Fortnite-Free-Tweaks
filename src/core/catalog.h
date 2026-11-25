#pragma once

// The compiled-in catalogue is the allowlist. VNMHelper refuses any plan whose
// item id, registry path, value name or type does not match an entry here, so a
// bug (or a tampered plan file) cannot make the privileged helper write
// somewhere else.

#include <string_view>
#include <vector>

#include "core/tweak.h"

namespace vnm {

struct TweakSpec {
    std::string_view id;
    std::string_view module;
    std::string_view title;
    std::string_view target;
    std::string_view valueName;
    std::string_view valueType;
    Risk risk;
    Revert revert;
};

const std::vector<TweakSpec>& catalog();
const TweakSpec* find_spec(std::string_view id);

}  // namespace vnm
