#pragma once

// Append-only change journal.
//
// One JSON object per line, one file per month. Each entry stores the exact
// value that existed before the change, so Undo never needs System Restore.
// Entries are hash-chained (FNV-1a 64 over the canonical fields): this is
// tamper *evidence*, not cryptography - it catches an edited or truncated
// journal, which is all support needs.

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace vnm {

struct JournalEntry {
    std::string id;
    std::string timestamp;      // ISO-8601 UTC
    std::string session;
    std::string itemId;
    std::string title;
    std::string target;
    std::string valueName;
    std::string valueType;
    std::string before;
    std::string after;
    std::string existedBefore;  // "1" or "0"
    std::string risk;
    std::string revert;
    std::string restorePoint;
    std::string appVersion;
    std::string undoOf;        // id of the entry this one undid, or ""
    std::string prevHash;
    std::string hash;
};

std::string utc_timestamp_now();
std::string new_entry_id();
std::filesystem::path monthly_journal_path(const std::filesystem::path& directory,
                                           int year, int month);

class Journal {
public:
    explicit Journal(std::filesystem::path file);

    const std::filesystem::path& path() const { return file_; }

    // Fills prevHash/hash, creates the parent directory if needed and appends
    // the line. Returns false and sets `error` on any I/O problem.
    bool append(JournalEntry& entry, std::string& error);

    // Skips unparsable lines but reports the first problem through `error`.
    std::vector<JournalEntry> load(std::string* error = nullptr) const;

    std::optional<JournalEntry> find(std::string_view id) const;

    static std::string hash_entry(const JournalEntry& entry);
    static bool verify_chain(const std::vector<JournalEntry>& entries, std::string& error);

private:
    std::filesystem::path file_;
};

// Serialises one entry to a single JSON line (no trailing newline).
std::string journal_entry_to_line(const JournalEntry& entry);
bool journal_entry_from_line(std::string_view line, JournalEntry& out, std::string& error);

}  // namespace vnm
