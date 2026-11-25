#include "core/gate.h"

#include <algorithm>
#include <cctype>

namespace vnm {

GateDecision decide_gate(const GateInputs& inputs) {
    if (inputs.pointCreatedToday) {
        return GateDecision::Allow;
    }
    if (!inputs.protectionEnabled) {
        return GateDecision::EnableProtection;
    }
    if (!inputs.elevated) {
        return GateDecision::Elevate;
    }
    return GateDecision::CreatePoint;
}

std::string_view to_string(GateDecision decision) {
    switch (decision) {
        case GateDecision::Allow: return "allow";
        case GateDecision::CreatePoint: return "create-restore-point";
        case GateDecision::EnableProtection: return "enable-protection";
        case GateDecision::Elevate: return "elevate";
        case GateDecision::Blocked: return "blocked";
    }
    return "blocked";
}

bool verify_ack(std::string_view typed) {
    auto trim = [](std::string_view text) {
        std::size_t begin = 0;
        std::size_t end = text.size();
        while (begin < end && std::isspace(static_cast<unsigned char>(text[begin])) != 0) {
            ++begin;
        }
        while (end > begin && std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
            --end;
        }
        return text.substr(begin, end - begin);
    };

    const std::string_view wanted = kRiskAckPhrase;
    const std::string_view actual = trim(typed);
    if (actual.size() != wanted.size()) {
        return false;
    }
    for (std::size_t i = 0; i < wanted.size(); ++i) {
        if (std::toupper(static_cast<unsigned char>(actual[i])) !=
            std::toupper(static_cast<unsigned char>(wanted[i]))) {
            return false;
        }
    }
    return true;
}

}  // namespace vnm
