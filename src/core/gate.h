#pragma once

// The restore-point gate, as a pure state machine so every branch is unit
// tested without touching a real machine.
//
// Rule (from the product owner): no system change without a restore point.
// Reality: Windows creates at most one restore point per 24 hours, and many
// machines have System Protection switched off. Both cases are resolved by the
// app rather than waved through:
//   - a point already created today satisfies the rule
//   - protection disabled  -> offer to enable it, then retry
//   - not elevated          -> elevate, then retry
// Only when all of those fail does the typed-risk path become available.

#include <string>
#include <string_view>

namespace vnm {

enum class GateDecision { Allow, CreatePoint, EnableProtection, Elevate, Blocked };

struct GateInputs {
    bool pointCreatedToday = false;
    bool protectionEnabled = true;
    bool elevated = true;
};

GateDecision decide_gate(const GateInputs& inputs);
std::string_view to_string(GateDecision decision);

// Typed acknowledgement used only when a restore point cannot be created at
// all. Logged verbatim in the journal.
inline constexpr std::string_view kRiskAckPhrase = "I ACCEPT THE RISK";

bool verify_ack(std::string_view typed);

}  // namespace vnm
