#include "core/catalog.h"

namespace vnm {

const std::vector<TweakSpec>& catalog() {
    // Release 0.1 ships exactly one real tweak on purpose: the point of this
    // release is to prove the whole pipeline (detect -> gate -> apply ->
    // journal -> undo) end to end. New items are added here and nowhere else.
    static const std::vector<TweakSpec> items = {
        TweakSpec{
            .id = "win.gamemode",
            .module = "Windows Performance",
            .title = "Game Mode",
            .target = "HKCU\\Software\\Microsoft\\GameBar",
            .valueName = "AutoGameModeEnabled",
            .valueType = "REG_DWORD",
            .risk = Risk::Safe,
            .revert = Revert::Instant,
        },
    };
    return items;
}

const TweakSpec* find_spec(std::string_view id) {
    for (const auto& spec : catalog()) {
        if (spec.id == id) {
            return &spec;
        }
    }
    return nullptr;
}

}  // namespace vnm
