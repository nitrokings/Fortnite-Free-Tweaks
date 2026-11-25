#include "test_framework.h"

#include <filesystem>
#include <fstream>
#include <string>

#include "core/journal.h"
#include "temp_dir.h"

using namespace vnm;
using vnm::test::TempDir;

namespace {

JournalEntry make_entry(const std::string& id, const std::string& after) {
    JournalEntry entry;
    entry.id = id;
    entry.timestamp = "2026-11-25T10:00:00Z";
    entry.itemId = "win.gamemode";
    entry.title = "Game Mode";
    entry.target = "HKCU\\Software\\Microsoft\\GameBar";
    entry.valueName = "AutoGameModeEnabled";
    entry.valueType = "REG_DWORD";
    entry.before = "0";
    entry.after = after;
    entry.existedBefore = "1";
    entry.risk = "Safe";
    entry.revert = "Instant";
    entry.appVersion = "0.1.0";
    return entry;
}

}  // namespace

TEST_CASE("journal appends entries and links the hash chain") {
    TempDir temp;
    Journal journal(temp.file("2026-11.jsonl"));

    JournalEntry first = make_entry("a1", "1");
    JournalEntry second = make_entry("a2", "0");
    std::string error;

    CHECK(journal.append(first, error));
    CHECK(error.empty());
    CHECK(journal.append(second, error));

    CHECK_EQ(first.prevHash, std::string("0"));
    CHECK_EQ(second.prevHash, first.hash);

    const auto entries = journal.load(&error);
    CHECK_EQ(entries.size(), static_cast<std::size_t>(2));
    CHECK(error.empty());

    std::string chainError;
    CHECK(Journal::verify_chain(entries, chainError));
    CHECK(chainError.empty());

    const auto found = journal.find("a2");
    CHECK(found.has_value());
    if (found) {
        CHECK_EQ(found->after, std::string("0"));
    }
}

TEST_CASE("journal detects a modified entry") {
    TempDir temp;
    Journal journal(temp.file("2026-11.jsonl"));
    std::string error;
    JournalEntry entry = make_entry("a1", "1");
    CHECK(journal.append(entry, error));

    auto entries = journal.load(&error);
    CHECK_EQ(entries.size(), static_cast<std::size_t>(1));
    entries[0].after = "0";  // pretend someone edited the file

    std::string chainError;
    CHECK(!Journal::verify_chain(entries, chainError));
    CHECK(!chainError.empty());
}

TEST_CASE("journal refuses to append to a damaged file") {
    TempDir temp;
    const auto path = temp.file("2026-11.jsonl");
    {
        std::ofstream file(path, std::ios::binary);
        file << "this is not json\n";
    }

    Journal journal(path);
    std::string error;
    JournalEntry entry = make_entry("a1", "1");
    CHECK(!journal.append(entry, error));
    CHECK(!error.empty());
}

TEST_CASE("journal round-trips the undoOf link") {
    TempDir temp;
    Journal journal(temp.file("2026-11.jsonl"));
    JournalEntry entry = make_entry("a1", "1");
    entry.undoOf = "original-id";
    std::string error;
    CHECK(journal.append(entry, error));

    const auto entries = journal.load(&error);
    CHECK_EQ(entries.size(), static_cast<std::size_t>(1));
    if (!entries.empty()) {
        CHECK_EQ(entries[0].undoOf, std::string("original-id"));
    }
}

TEST_CASE("journal on a missing file loads nothing and reports no error") {
    TempDir temp;
    Journal journal(temp.file("missing.jsonl"));
    std::string error;
    const auto entries = journal.load(&error);
    CHECK(entries.empty());
    CHECK(error.empty());
}
