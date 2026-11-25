#include "core/gamemode.h"

#include <cstdint>

#include "core/catalog.h"
#include "core/plan.h"

namespace vnm {
namespace {

std::string describe(const TweakSpec& spec, const RegistryValue& value) {
    if (!value.exists) {
        return std::string(spec.valueName) + " is not set in " + std::string(spec.target);
    }
    return std::string(spec.valueName) + " = " + value.data + " in " + std::string(spec.target);
}

JournalEntry make_entry(const TweakSpec& spec, const ChangePlan& plan, const RegistryValue& current) {
    JournalEntry entry;
    entry.itemId = plan.itemId;
    entry.title = plan.title.empty() ? std::string(spec.title) : plan.title;
    entry.target = plan.target;
    entry.valueName = plan.valueName;
    entry.valueType = plan.valueType;
    entry.before = current.data;
    entry.after = plan.after;
    entry.existedBefore = current.exists ? "1" : "0";
    entry.risk = std::string(to_string(spec.risk));
    entry.revert = spec.revert == Revert::NeedsReboot ? "NeedsReboot" : "Instant";
    entry.session = plan.session;
    entry.restorePoint = plan.restorePoint;
    entry.appVersion = "0.1.0";
    return entry;
}

}  // namespace

GameModeTweak::GameModeTweak(IRegistry& registry, Journal& journal)
    : registry_(registry), journal_(journal) {}

DetectResult GameModeTweak::detect() const {
    const TweakSpec* spec = find_spec("win.gamemode");
    if (spec == nullptr) {
        return {TweakState::Unknown, "Game Mode is missing from the catalogue"};
    }
    const auto value = registry_.read(spec->target, spec->valueName);
    if (!value.exists) {
        return {TweakState::Off,
                "Not configured - Windows is using its default. " + describe(*spec, value)};
    }
    return {value.data == "1" ? TweakState::On : TweakState::Off, describe(*spec, value)};
}

ChangePlan GameModeTweak::preview(bool enable) const {
    const TweakSpec* spec = find_spec("win.gamemode");
    ChangePlan plan;
    if (spec == nullptr) {
        return plan;
    }
    const auto current = registry_.read(spec->target, spec->valueName);
    plan.itemId = std::string(spec->id);
    plan.title = std::string(spec->title);
    plan.target = std::string(spec->target);
    plan.valueName = std::string(spec->valueName);
    plan.valueType = std::string(spec->valueType);
    plan.before = current.exists ? current.data : std::string();
    plan.after = enable ? "1" : "0";
    plan.risk = spec->risk;
    plan.revert = spec->revert;
    return plan;
}

bool GameModeTweak::apply(const ChangePlan& plan, std::string& error) {
    const TweakSpec* spec = find_spec(plan.itemId);
    if (spec == nullptr) {
        error = "unknown item: " + plan.itemId;
        return false;
    }
    if (!validate_plan(plan, *spec, error)) {
        return false;
    }

    std::uint32_t target = 0;
    if (!parse_dword(plan.after, target)) {
        error = "target value is not a decimal DWORD";
        return false;
    }

    const auto current = registry_.read(spec->target, spec->valueName);
    if (current.exists && current.data == plan.after) {
        error = "nothing to do: the value already equals " + plan.after;
        return false;
    }

    std::string writeError;
    if (!registry_.write_dword(spec->target, spec->valueName, target, writeError)) {
        error = "could not write the registry value: " + writeError;
        return false;
    }

    auto entry = make_entry(*spec, plan, current);
    std::string journalError;
    if (!journal_.append(entry, journalError)) {
        // Put the machine back rather than leave an unrecorded change behind.
        std::string rollbackError;
        bool rolledBack = false;
        if (current.exists) {
            std::uint32_t previous = 0;
            if (parse_dword(current.data, previous)) {
                rolledBack = registry_.write_dword(spec->target, spec->valueName, previous,
                                                   rollbackError);
            }
        } else {
            rolledBack = registry_.delete_value(spec->target, spec->valueName, rollbackError);
        }
        error = "could not record the change (" + journalError + ")";
        error += rolledBack ? "; the change was rolled back"
                            : "; ROLLBACK FAILED: " + rollbackError;
        return false;
    }
    return true;
}

bool GameModeTweak::undo(const JournalEntry& entry, std::string& error) {
    const TweakSpec* spec = find_spec(entry.itemId);
    if (spec == nullptr) {
        error = "this entry belongs to an item this version does not know about";
        return false;
    }

    const auto current = registry_.read(entry.target, entry.valueName);
    const bool existedBeforeUndo = current.exists;

    if (entry.existedBefore == "0") {
        std::string deleteError;
        if (!registry_.delete_value(entry.target, entry.valueName, deleteError)) {
            error = "could not remove the value: " + deleteError;
            return false;
        }
    } else {
        std::uint32_t previous = 0;
        if (!parse_dword(entry.before, previous)) {
            error = "the journal entry does not contain a restorable value";
            return false;
        }
        std::string writeError;
        if (!registry_.write_dword(entry.target, entry.valueName, previous, writeError)) {
            error = "could not restore the previous value: " + writeError;
            return false;
        }
    }

    JournalEntry audit;
    audit.itemId = entry.itemId;
    audit.title = "Undo: " + entry.title;
    audit.target = entry.target;
    audit.valueName = entry.valueName;
    audit.valueType = entry.valueType;
    audit.before = existedBeforeUndo ? current.data : std::string();
    audit.existedBefore = existedBeforeUndo ? "1" : "0";
    audit.after = entry.before;
    audit.risk = std::string(to_string(spec->risk));
    audit.revert = "Instant";
    audit.restorePoint = entry.restorePoint;
    audit.session = entry.session;
    audit.undoOf = entry.id;
    audit.appVersion = "0.1.0";

    std::string journalError;
    if (!journal_.append(audit, journalError)) {
        error = "the value was restored but the undo could not be journaled: " + journalError;
        return false;
    }
    return true;
}

}  // namespace vnm
