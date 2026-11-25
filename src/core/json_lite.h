#pragma once

// A deliberately tiny JSON subset: a flat object whose values are all strings.
//
// The journal and the change-plan files only ever need `{"key":"value",...}`,
// so this avoids pulling a full JSON library into the build. Numbers are stored
// as their decimal text (the plan says what registry type they represent).
//
// Anything outside the subset is rejected rather than guessed at, because both
// callers treat parse failures as "refuse to proceed".

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace vnm::json {

using Object = std::vector<std::pair<std::string, std::string>>;

std::string escape(std::string_view value);
std::string write_object(const Object& fields);

// Returns std::nullopt and fills `error` on any syntax problem, including
// trailing garbage after the closing brace.
std::optional<Object> parse_object(std::string_view text, std::string& error);

const std::string* find(const Object& object, std::string_view key);

}  // namespace vnm::json
