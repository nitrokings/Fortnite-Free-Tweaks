#include "core/plan.h"

#include <limits>

#include "core/json_lite.h"

namespace vnm {

std::string serialize_plan(const ChangePlan& plan) {
    return json::write_object({
        {"version", "1"},
        {"itemId", plan.itemId},
        {"title", plan.title},
        {"target", plan.target},
        {"valueName", plan.valueName},
        {"valueType", plan.valueType},
        {"before", plan.before},
        {"after", plan.after},
        {"session", plan.session},
        {"restorePoint", plan.restorePoint},
    });
}

std::optional<ChangePlan> parse_plan(std::string_view text, std::string& error) {
    const auto fields = json::parse_object(text, error);
    if (!fields) {
        return std::nullopt;
    }

    auto take = [&](std::string_view key) -> std::string {
        const std::string* value = json::find(*fields, key);
        return value != nullptr ? *value : std::string();
    };

    const std::string version = take("version");
    if (version != "1") {
        error = "unsupported plan version: '" + version + "'";
        return std::nullopt;
    }

    ChangePlan plan;
    plan.itemId = take("itemId");
    plan.title = take("title");
    plan.target = take("target");
    plan.valueName = take("valueName");
    plan.valueType = take("valueType");
    plan.before = take("before");
    plan.after = take("after");
    plan.session = take("session");
    plan.restorePoint = take("restorePoint");

    if (plan.itemId.empty()) {
        error = "plan has no itemId";
        return std::nullopt;
    }
    if (plan.after.empty()) {
        error = "plan has no target value";
        return std::nullopt;
    }
    return plan;
}

bool parse_dword(std::string_view text, std::uint32_t& out) {
    if (text.empty() || text.size() > 10) {
        return false;
    }
    std::uint64_t value = 0;
    for (const char c : text) {
        if (c < '0' || c > '9') {
            return false;
        }
        value = value * 10U + static_cast<std::uint64_t>(c - '0');
        if (value > std::numeric_limits<std::uint32_t>::max()) {
            return false;
        }
    }
    out = static_cast<std::uint32_t>(value);
    return true;
}

bool validate_plan(const ChangePlan& plan, const TweakSpec& spec, std::string& error) {
    error.clear();
    if (plan.itemId != spec.id) {
        error = "item id does not match the catalogue entry";
        return false;
    }
    if (plan.target != spec.target) {
        error = "target does not match the catalogue entry (refusing to write elsewhere)";
        return false;
    }
    if (plan.valueName != spec.valueName) {
        error = "value name does not match the catalogue entry";
        return false;
    }
    if (plan.valueType != spec.valueType) {
        error = "value type does not match the catalogue entry";
        return false;
    }
    if (!plan.target.starts_with("HKCU\\")) {
        error = "release 0.1 only writes to HKEY_CURRENT_USER";
        return false;
    }

    std::uint32_t after = 0;
    if (!parse_dword(plan.after, after)) {
        error = "target value is not a decimal DWORD";
        return false;
    }
    if (!plan.before.empty()) {
        std::uint32_t before = 0;
        if (!parse_dword(plan.before, before)) {
            error = "current value in the plan is not a decimal DWORD";
            return false;
        }
    }
    return true;
}

}  // namespace vnm
