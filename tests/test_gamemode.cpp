#include "test_framework.h"

#include <string>

#include "core/gamemode.h"
#include "core/journal.h"
#include "fake_registry.h"
#include "temp_dir.h"

using namespace vnm;
using vnm::test::FakeRegistry;
using vnm::test::TempDir;

namespace {
constexpr const char* kPath = "HKCU\\Software\\Microsoft\\GameBar";
constexpr const char* kName = "AutoGameModeEnabled";
}  // namespace

TEST_CASE("gamemode detects an unconfigured machine as off") {
    FakeRegistry registry;
    TempDir temp;
    Journal journal(temp.file("j.jsonl"));
    GameModeTweak tweak(registry, journal);

    const auto detection = tweak.detect();
    CHECK(detection.state == TweakState::Off);
    CHECK(!detection.detail.empty());
}

TEST_CASE("gamemode preview captures the current value and the target") {
    FakeRegistry registry;
    TempDir temp;
    Journal journal(temp.file("j.jsonl"));
    GameModeTweak tweak(registry, journal);

    const auto plan = tweak.preview(true);
    CHECK_EQ(plan.itemId, std::string("win.gamemode"));
    CHECK_EQ(plan.target, std::string(kPath));
    CHECK_EQ(plan.valueName, std::string(kName));
    CHECK_EQ(plan.before, std::string());
    CHECK_EQ(plan.after, std::string("1"));
}

TEST_CASE("gamemode apply writes the value and journals it") {
    FakeRegistry registry;
    TempDir temp;
    Journal journal(temp.file("j.jsonl"));
    GameModeTweak tweak(registry, journal);

    auto plan = tweak.preview(true);
    std::string error;
    CHECK(tweak.apply(plan, error));
    CHECK(error.empty());

    const auto stored = registry.read(kPath, kName);
    CHECK(stored.exists);
    CHECK_EQ(stored.data, std::string("1"));

    const auto entries = journal.load();
    CHECK_EQ(entries.size(), static_cast<std::size_t>(1));
    if (!entries.empty()) {
        CHECK_EQ(entries[0].existedBefore, std::string("0"));
        CHECK_EQ(entries[0].after, std::string("1"));
        CHECK_EQ(entries[0].target, std::string(kPath));
    }
}

TEST_CASE("gamemode undo removes a value that did not exist before") {
    FakeRegistry registry;
    TempDir temp;
    Journal journal(temp.file("j.jsonl"));
    GameModeTweak tweak(registry, journal);

    auto plan = tweak.preview(true);
    std::string error;
    CHECK(tweak.apply(plan, error));
    const auto entries = journal.load();
    CHECK_EQ(entries.size(), static_cast<std::size_t>(1));

    CHECK(tweak.undo(entries[0], error));
    CHECK(error.empty());
    CHECK(!registry.read(kPath, kName).exists);

    const auto after = journal.load();
    CHECK_EQ(after.size(), static_cast<std::size_t>(2));
    if (after.size() == 2) {
        CHECK_EQ(after[1].undoOf, entries[0].id);
        CHECK_EQ(after[1].after, std::string());
    }
}

TEST_CASE("gamemode undo restores the exact previous value") {
    FakeRegistry registry;
    registry.values[FakeRegistry::Key{kPath, kName}] = "1";
    TempDir temp;
    Journal journal(temp.file("j.jsonl"));
    GameModeTweak tweak(registry, journal);

    auto plan = tweak.preview(false);
    CHECK_EQ(plan.before, std::string("1"));
    std::string error;
    CHECK(tweak.apply(plan, error));
    CHECK_EQ(registry.read(kPath, kName).data, std::string("0"));

    const auto entries = journal.load();
    CHECK(tweak.undo(entries[0], error));
    CHECK_EQ(registry.read(kPath, kName).data, std::string("1"));
}

TEST_CASE("gamemode refuses a plan that targets a different registry value") {
    FakeRegistry registry;
    TempDir temp;
    Journal journal(temp.file("j.jsonl"));
    GameModeTweak tweak(registry, journal);

    auto plan = tweak.preview(true);
    plan.target = "HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run";
    std::string error;
    CHECK(!tweak.apply(plan, error));
    CHECK(!error.empty());
    CHECK(!registry.read("HKCU\\Software\\Microsoft\\Windows\\CurrentVersion\\Run", kName).exists);
}

TEST_CASE("gamemode refuses a plan with an unknown item id") {
    FakeRegistry registry;
    TempDir temp;
    Journal journal(temp.file("j.jsonl"));
    GameModeTweak tweak(registry, journal);

    auto plan = tweak.preview(true);
    plan.itemId = "win.something-else";
    std::string error;
    CHECK(!tweak.apply(plan, error));
    CHECK(!error.empty());
}

TEST_CASE("gamemode rolls the registry back when the journal cannot be written") {
    FakeRegistry registry;
    TempDir temp;
    // A directory where the journal file should be: every append must fail.
    Journal journal(temp.path);
    GameModeTweak tweak(registry, journal);

    auto plan = tweak.preview(true);
    std::string error;
    CHECK(!tweak.apply(plan, error));
    CHECK(!error.empty());
    CHECK(!registry.read(kPath, kName).exists);
}

TEST_CASE("gamemode apply reports when there is nothing to do") {
    FakeRegistry registry;
    registry.values[FakeRegistry::Key{kPath, kName}] = "1";
    TempDir temp;
    Journal journal(temp.file("j.jsonl"));
    GameModeTweak tweak(registry, journal);

    auto plan = tweak.preview(true);
    std::string error;
    CHECK(!tweak.apply(plan, error));
    CHECK(!error.empty());
    CHECK(registry.writeLog.empty());
}
