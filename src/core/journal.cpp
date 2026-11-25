#include "core/journal.h"

#include <chrono>
#include <cstdio>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <random>
#include <sstream>
#include <system_error>

#include "core/json_lite.h"

namespace vnm {
namespace {

std::tm to_utc(std::time_t when) {
    std::tm out{};
#ifdef _WIN32
    gmtime_s(&out, &when);
#else
    gmtime_r(&when, &out);
#endif
    return out;
}

std::string hex64(std::uint64_t value) {
    std::ostringstream stream;
    stream << std::hex << std::setw(16) << std::setfill('0') << value;
    return stream.str();
}

// FNV-1a, 64 bit. Deterministic and dependency-free.
std::uint64_t fnv1a(std::string_view data) {
    std::uint64_t hash = 1469598103934665603ULL;
    for (const char c : data) {
        hash ^= static_cast<unsigned char>(c);
        hash *= 1099511628211ULL;
    }
    return hash;
}

}  // namespace

std::string utc_timestamp_now() {
    const auto now = std::chrono::system_clock::now();
    const std::time_t seconds = std::chrono::system_clock::to_time_t(now);
    const std::tm tm = to_utc(seconds);
    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(4) << (tm.tm_year + 1900) << '-'
           << std::setw(2) << (tm.tm_mon + 1) << '-' << std::setw(2) << tm.tm_mday << 'T'
           << std::setw(2) << tm.tm_hour << ':' << std::setw(2) << tm.tm_min << ':'
           << std::setw(2) << tm.tm_sec << 'Z';
    return stream.str();
}

std::string new_entry_id() {
    const std::string stamp = utc_timestamp_now();
    std::string compact;
    for (const char c : stamp) {
        if (c != '-' && c != ':') {
            compact.push_back(c);
        }
    }
    std::random_device device;
    const auto random = (static_cast<std::uint64_t>(device()) << 32U) ^ device();
    return compact + "-" + hex64(random).substr(0, 6);
}

std::filesystem::path monthly_journal_path(const std::filesystem::path& directory, int year,
                                           int month) {
    std::ostringstream stream;
    stream << std::setfill('0') << std::setw(4) << year << '-' << std::setw(2) << month
           << ".jsonl";
    return directory / stream.str();
}

std::string journal_entry_to_line(const JournalEntry& entry) {
    return json::write_object({
        {"id", entry.id},
        {"ts", entry.timestamp},
        {"session", entry.session},
        {"itemId", entry.itemId},
        {"title", entry.title},
        {"target", entry.target},
        {"valueName", entry.valueName},
        {"valueType", entry.valueType},
        {"before", entry.before},
        {"after", entry.after},
        {"existedBefore", entry.existedBefore},
        {"risk", entry.risk},
        {"revert", entry.revert},
        {"restorePoint", entry.restorePoint},
        {"appVersion", entry.appVersion},
        {"undoOf", entry.undoOf},
        {"prevHash", entry.prevHash},
        {"hash", entry.hash},
    });
}

bool journal_entry_from_line(std::string_view line, JournalEntry& out, std::string& error) {
    const auto fields = json::parse_object(line, error);
    if (!fields) {
        return false;
    }

    auto take = [&](std::string_view key, std::string& target) {
        if (const std::string* value = json::find(*fields, key)) {
            target = *value;
        } else {
            target.clear();
        }
    };

    take("id", out.id);
    take("ts", out.timestamp);
    take("session", out.session);
    take("itemId", out.itemId);
    take("title", out.title);
    take("target", out.target);
    take("valueName", out.valueName);
    take("valueType", out.valueType);
    take("before", out.before);
    take("after", out.after);
    take("existedBefore", out.existedBefore);
    take("risk", out.risk);
    take("revert", out.revert);
    take("restorePoint", out.restorePoint);
    take("appVersion", out.appVersion);
    take("undoOf", out.undoOf);
    take("prevHash", out.prevHash);
    take("hash", out.hash);

    if (out.id.empty()) {
        error = "journal entry has no id";
        return false;
    }
    return true;
}

std::string Journal::hash_entry(const JournalEntry& entry) {
    std::string canonical;
    canonical.reserve(512);
    const std::string fields[] = {
        entry.id,       entry.timestamp,   entry.session,      entry.itemId,
        entry.title,    entry.target,      entry.valueName,    entry.valueType,
        entry.before,   entry.after,       entry.existedBefore, entry.risk,
        entry.revert,   entry.restorePoint, entry.appVersion,  entry.undoOf,
        entry.prevHash,
    };
    for (const auto& field : fields) {
        canonical += field;
        canonical.push_back('\x1f');
    }
    return hex64(fnv1a(canonical));
}

Journal::Journal(std::filesystem::path file) : file_(std::move(file)) {}

bool Journal::append(JournalEntry& entry, std::string& error) {
    error.clear();
    if (entry.id.empty()) {
        entry.id = new_entry_id();
    }
    if (entry.timestamp.empty()) {
        entry.timestamp = utc_timestamp_now();
    }

    if (!file_.parent_path().empty()) {
        std::error_code ec;
        std::filesystem::create_directories(file_.parent_path(), ec);
        if (ec && !std::filesystem::exists(file_.parent_path())) {
            error = "cannot create journal directory: " + ec.message();
            return false;
        }
    }

    std::string loadError;
    const auto existing = load(&loadError);
    if (!loadError.empty()) {
        // A damaged journal must not silently lose its chain.
        error = "journal is unreadable, refusing to append: " + loadError;
        return false;
    }

    entry.prevHash = existing.empty() ? std::string("0") : existing.back().hash;
    entry.hash = hash_entry(entry);

    std::ofstream stream(file_, std::ios::app | std::ios::binary);
    if (!stream) {
        error = "cannot open journal for writing: " + file_.string();
        return false;
    }
    stream << journal_entry_to_line(entry) << '\n';
    stream.flush();
    if (!stream) {
        error = "failed while writing the journal entry";
        return false;
    }
    return true;
}

std::vector<JournalEntry> Journal::load(std::string* error) const {
    std::vector<JournalEntry> entries;
    if (error != nullptr) {
        error->clear();
    }

    std::error_code ec;
    if (!std::filesystem::exists(file_, ec)) {
        return entries;
    }

    std::ifstream stream(file_, std::ios::binary);
    if (!stream) {
        if (error != nullptr) {
            *error = "cannot open journal: " + file_.string();
        }
        return entries;
    }

    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(stream, line)) {
        ++lineNumber;
        if (line.empty()) {
            continue;
        }
        JournalEntry entry;
        std::string parseError;
        if (!journal_entry_from_line(line, entry, parseError)) {
            if (error != nullptr && error->empty()) {
                *error = "line " + std::to_string(lineNumber) + ": " + parseError;
            }
            continue;
        }
        entries.push_back(std::move(entry));
    }
    return entries;
}

std::optional<JournalEntry> Journal::find(std::string_view id) const {
    const auto entries = load();
    for (auto it = entries.rbegin(); it != entries.rend(); ++it) {
        if (it->id == id) {
            return *it;
        }
    }
    return std::nullopt;
}

bool Journal::verify_chain(const std::vector<JournalEntry>& entries, std::string& error) {
    error.clear();
    std::string previous = "0";
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        if (entry.prevHash != previous) {
            error = "entry " + std::to_string(i) + " (" + entry.id +
                    ") does not link to the previous hash";
            return false;
        }
        if (Journal::hash_entry(entry) != entry.hash) {
            error = "entry " + std::to_string(i) + " (" + entry.id +
                    ") has been modified after it was written";
            return false;
        }
        previous = entry.hash;
    }
    return true;
}

}  // namespace vnm
