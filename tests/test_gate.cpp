#include "test_framework.h"

#include "core/gate.h"

using namespace vnm;

TEST_CASE("gate allows changes when a restore point exists for today") {
    GateInputs inputs;
    inputs.pointCreatedToday = true;
    inputs.protectionEnabled = false;  // an existing point satisfies the rule
    inputs.elevated = false;
    CHECK(decide_gate(inputs) == GateDecision::Allow);
}

TEST_CASE("gate asks for System Protection when it is disabled") {
    GateInputs inputs;
    inputs.pointCreatedToday = false;
    inputs.protectionEnabled = false;
    inputs.elevated = true;
    CHECK(decide_gate(inputs) == GateDecision::EnableProtection);
}

TEST_CASE("gate asks for elevation before creating a point") {
    GateInputs inputs;
    inputs.pointCreatedToday = false;
    inputs.protectionEnabled = true;
    inputs.elevated = false;
    CHECK(decide_gate(inputs) == GateDecision::Elevate);
}

TEST_CASE("gate creates a point when protection is on and the helper is elevated") {
    GateInputs inputs;
    inputs.pointCreatedToday = false;
    inputs.protectionEnabled = true;
    inputs.elevated = true;
    CHECK(decide_gate(inputs) == GateDecision::CreatePoint);
}

TEST_CASE("acknowledgement phrase must match exactly, ignoring case and spaces") {
    CHECK(verify_ack("I ACCEPT THE RISK"));
    CHECK(verify_ack("  i accept the risk  "));
    CHECK(!verify_ack("I accept the risk."));
    CHECK(!verify_ack("accept"));
    CHECK(!verify_ack(""));
    CHECK(!verify_ack("I ACCEPT THE RISK NOW"));
}
