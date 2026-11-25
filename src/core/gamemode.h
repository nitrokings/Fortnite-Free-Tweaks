#pragma once

// Release 0.1's single real tweak: Windows Game Mode. It exists to prove the
// contract end to end - detect, preview, apply through the journal, undo.

#include "core/journal.h"
#include "core/tweak.h"

namespace vnm {

class GameModeTweak {
public:
    GameModeTweak(IRegistry& registry, Journal& journal);

    DetectResult detect() const;
    ChangePlan preview(bool enable) const;

    // Writes the value and journals it. If the journal write fails the change
    // is rolled back, so the machine never ends up changed-but-unrecorded.
    bool apply(const ChangePlan& plan, std::string& error);

    // Restores the value recorded in the entry and appends an audit entry that
    // points back at the entry it undid.
    bool undo(const JournalEntry& entry, std::string& error);

private:
    IRegistry& registry_;
    Journal& journal_;
};

}  // namespace vnm
