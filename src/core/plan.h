#pragma once

// Change-plan files: the only thing the GUI can ask VNMHelper to do.
//
// The plan carries no privilege claims of its own - risk, revert policy and the
// legal registry target all come from the compiled-in catalogue. The helper
// re-validates every field before it writes anything.

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "core/catalog.h"
#include "core/tweak.h"

namespace vnm {

std::string serialize_plan(const ChangePlan& plan);
std::optional<ChangePlan> parse_plan(std::string_view text, std::string& error);

// Rejects: unknown item, different path/value/type than the catalogue, values
// that are not plain decimal words, and out-of-range values.
bool validate_plan(const ChangePlan& plan, const TweakSpec& spec, std::string& error);

bool parse_dword(std::string_view text, std::uint32_t& out);

}  // namespace vnm
