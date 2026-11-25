#include "test_framework.h"

#include <cstdint>
#include <string>

#include "core/catalog.h"
#include "core/plan.h"

using namespace vnm;

namespace {

ChangePlan game_mode_plan() {
    ChangePlan plan;
    plan.itemId = "win.gamemode";
    plan.title = "Game Mode";
    plan.target = "HKCU\\Software\\Microsoft\\GameBar";
    plan.valueName = "AutoGameModeEnabled";
    plan.valueType = "REG_DWORD";
    plan.before = "0";
    plan.after = "1";
    plan.session = "session-1";
    plan.restorePoint = "2026-11-25T10:00:00Z";
    return plan;
}

}  // namespace

TEST_CASE("plan round-trips through the file format") {
    const auto original = game_mode_plan();
    const std::string text = serialize_plan(original);

    std::string error;
    const auto parsed = parse_plan(text, error);
    CHECK(parsed.has_value());
    CHECK(error.empty());
    if (parsed) {
        CHECK_EQ(parsed->itemId, original.itemId);
        CHECK_EQ(parsed->target, original.target);
        CHECK_EQ(parsed->valueName, original.valueName);
        CHECK_EQ(parsed->before, original.before);
        CHECK_EQ(parsed->after, original.after);
        CHECK_EQ(parsed->restorePoint, original.restorePoint);
    }
}

TEST_CASE("plan parsing refuses a missing or unsupported version") {
    std::string error;
    CHECK(!parse_plan("{\"itemId\":\"win.gamemode\",\"after\":\"1\"}", error).has_value());
    CHECK(!error.empty());

    error.clear();
    CHECK(!parse_plan("{\"version\":\"2\",\"itemId\":\"win.gamemode\",\"after\":\"1\"}", error)
               .has_value());
}

TEST_CASE("plan validation accepts the catalogue definition") {
    const TweakSpec* spec = find_spec("win.gamemode");
    CHECK(spec != nullptr);
    if (spec != nullptr) {
        std::string error;
        CHECK(validate_plan(game_mode_plan(), *spec, error));
        CHECK(error.empty());
    }
}

TEST_CASE("plan validation rejects any field the catalogue does not allow") {
    const TweakSpec* spec = find_spec("win.gamemode");
    CHECK(spec != nullptr);
    if (spec == nullptr) {
        return;
    }

    std::string error;

    auto wrongTarget = game_mode_plan();
    wrongTarget.target = "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    CHECK(!validate_plan(wrongTarget, *spec, error));

    error.clear();
    auto wrongName = game_mode_plan();
    wrongName.valueName = "SomethingElse";
    CHECK(!validate_plan(wrongName, *spec, error));

    error.clear();
    auto wrongType = game_mode_plan();
    wrongType.valueType = "REG_SZ";
    CHECK(!validate_plan(wrongType, *spec, error));

    error.clear();
    auto hklm = game_mode_plan();
    hklm.target = "HKLM\\Software\\Microsoft\\GameBar";
    CHECK(!validate_plan(hklm, *spec, error));

    error.clear();
    auto injection = game_mode_plan();
    injection.after = "1; rm -rf /";
    CHECK(!validate_plan(injection, *spec, error));

    error.clear();
    auto tooBig = game_mode_plan();
    tooBig.after = "4294967296";
    CHECK(!validate_plan(tooBig, *spec, error));
}

TEST_CASE("dword parsing only accepts plain decimal values") {
    std::uint32_t value = 0;
    CHECK(parse_dword("1", value));
    CHECK_EQ(value, 1U);

    CHECK(parse_dword("0", value));
    CHECK_EQ(value, 0U);

    CHECK(parse_dword("4294967295", value));
    CHECK_EQ(value, 4294967295U);

    CHECK(!parse_dword("", value));
    CHECK(!parse_dword("0x1", value));
    CHECK(!parse_dword("12a", value));
    CHECK(!parse_dword("-1", value));
    CHECK(!parse_dword(" 1", value));
    CHECK(!parse_dword("4294967296", value));
}
