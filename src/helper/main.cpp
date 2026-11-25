// VNMHelper - the only process that changes system state.
//
// Every command prints one JSON line on stdout so the GUI never has to parse
// prose. Exit code 0 means the command itself ran; check the "ok" field for the
// operation's result.

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#endif

#include "core/gamemode.h"
#include "core/gate.h"
#include "core/json_lite.h"
#include "core/journal.h"
#include "core/paths.h"
#include "core/plan.h"
#include "platform/win/registry_win.h"
#include "platform/win/restore_win.h"

namespace {

using namespace vnm;

// Set when the GUI runs the helper elevated: an elevated child's stdout cannot
// be captured, so the result is also written to this file.
std::string g_out_path;

void emit(const json::Object& fields) {
    const std::string line = json::write_object(fields);
    std::cout << line << std::endl;
    if (!g_out_path.empty()) {
        std::ofstream file(g_out_path, std::ios::binary | std::ios::trunc);
        if (file) {
            file << line << '\n';
        }
    }
}

std::string read_file(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string last_journal_id(const Journal& journal) {
    const auto entries = journal.load();
    return entries.empty() ? std::string() : entries.back().id;
}

int command_apply(const std::string& plan_path) {
    const std::string text = read_file(plan_path);
    if (text.empty()) {
        emit({{"ok", "0"}, {"error", "the plan file is missing or empty"}});
        return 0;
    }

    std::string error;
    const auto plan = parse_plan(text, error);
    if (!plan) {
        emit({{"ok", "0"}, {"error", "could not read the plan: " + error}});
        return 0;
    }

    const TweakSpec* spec = find_spec(plan->itemId);
    if (spec == nullptr) {
        emit({{"ok", "0"}, {"error", "unknown item: " + plan->itemId}});
        return 0;
    }
    if (!validate_plan(*plan, *spec, error)) {
        emit({{"ok", "0"}, {"error", "the plan was rejected: " + error}});
        return 0;
    }

    WinRegistry registry;
    Journal journal(current_journal_file());
    GameModeTweak tweak(registry, journal);

    if (!tweak.apply(*plan, error)) {
        emit({{"ok", "0"}, {"error", error}});
        return 0;
    }

    emit({{"ok", "1"},
          {"journalId", last_journal_id(journal)},
          {"detail", "Game Mode is now " + plan->after}});
    return 0;
}

int command_undo(const std::string& journal_id) {
    Journal journal(current_journal_file());
    const auto entry = journal.find(journal_id);
    if (!entry) {
        emit({{"ok", "0"}, {"error", "no journal entry with that id"}});
        return 0;
    }

    WinRegistry registry;
    GameModeTweak tweak(registry, journal);
    std::string error;
    if (!tweak.undo(*entry, error)) {
        emit({{"ok", "0"}, {"error", error}});
        return 0;
    }
    emit({{"ok", "1"}, {"journalId", last_journal_id(journal)}, {"detail", "Change undone."}});
    return 0;
}

int command_ensure_restore_point() {
    const auto result = ensure_restore_point_today();
    const bool ok = result.status == RestorePointStatus::ExistsToday ||
                    result.status == RestorePointStatus::Created;
    emit({{"ok", ok ? "1" : "0"},
          {"status", std::string(to_string(result.status))},
          {"detail", result.detail}});
    return 0;
}

int command_enable_protection() {
    std::string detail;
    const bool ok = enable_system_protection(detail);
    emit({{"ok", ok ? "1" : "0"}, {"detail", detail}});
    return 0;
}

int command_log_ack(const std::string& phrase) {
    if (!verify_ack(phrase)) {
        emit({{"ok", "0"}, {"error", "the acknowledgement phrase does not match"}});
        return 0;
    }
    Journal journal(current_journal_file());
    JournalEntry entry;
    entry.itemId = "gate.risk-ack";
    entry.title = "Restore point unavailable - acknowledgement: " + phrase;
    entry.risk = "Advanced";
    entry.revert = "Instant";
    entry.after = "acknowledged";
    entry.appVersion = app_version();
    std::string error;
    if (!journal.append(entry, error)) {
        emit({{"ok", "0"}, {"error", error}});
        return 0;
    }
    emit({{"ok", "1"}, {"journalId", entry.id}, {"detail", "Acknowledgement recorded."}});
    return 0;
}

int run(const std::vector<std::string>& rawArgs) {
    std::vector<std::string> args;
    for (std::size_t i = 0; i < rawArgs.size(); ++i) {
        if (rawArgs[i] == "--out" && i + 1 < rawArgs.size()) {
            g_out_path = rawArgs[i + 1];
            ++i;
            continue;
        }
        args.push_back(rawArgs[i]);
    }

    if (args.empty()) {
        std::cerr << "usage: VNMHelper --apply <plan.json> | --undo <journalId> | "
                     "--ensure-restore-point | --enable-protection | --log-ack <phrase> | "
                     "--version\n";
        return 2;
    }

    const std::string& command = args[0];
    if (command == "--version") {
        std::cout << "VNMHelper " << app_version() << std::endl;
        return 0;
    }
    if (command == "--ensure-restore-point") {
        return command_ensure_restore_point();
    }
    if (command == "--enable-protection") {
        return command_enable_protection();
    }
    if (command == "--apply" && args.size() >= 2) {
        return command_apply(args[1]);
    }
    if (command == "--undo" && args.size() >= 2) {
        return command_undo(args[1]);
    }
    if (command == "--log-ack" && args.size() >= 2) {
        return command_log_ack(args[1]);
    }

    std::cerr << "unknown or incomplete command" << std::endl;
    return 2;
}

}  // namespace

#ifdef _WIN32

int wmain(int argc, wchar_t** argv) {
    std::vector<std::string> args;
    args.reserve(static_cast<std::size_t>(argc > 0 ? argc - 1 : 0));
    for (int i = 1; i < argc; ++i) {
        const int length = WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, nullptr, 0, nullptr, nullptr);
        if (length > 1) {
            std::string text(static_cast<std::size_t>(length - 1), '\0');
            WideCharToMultiByte(CP_UTF8, 0, argv[i], -1, text.data(), length, nullptr, nullptr);
            args.push_back(std::move(text));
        }
    }
    return run(args);
}

#else

int main(int argc, char** argv) {
    std::vector<std::string> args;
    for (int i = 1; i < argc; ++i) {
        args.emplace_back(argv[i]);
    }
    return run(args);
}

#endif
