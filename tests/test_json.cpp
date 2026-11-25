#include "test_framework.h"

#include <optional>
#include <string>

#include "core/json_lite.h"

using namespace vnm;

TEST_CASE("json round-trips strings with quotes and control characters") {
    const json::Object original = {
        {"target", "HKCU\\Software\\Microsoft\\GameBar"},
        {"title", "Game \"Mode\""},
        {"detail", "line one\nline two\ttabbed"},
    };
    const std::string text = json::write_object(original);

    std::string error;
    const auto parsed = json::parse_object(text, error);
    CHECK(parsed.has_value());
    CHECK(error.empty());
    if (parsed) {
        CHECK_EQ(parsed->size(), original.size());
        CHECK_EQ(std::string(*json::find(*parsed, "title")), std::string("Game \"Mode\""));
        CHECK_EQ(std::string(*json::find(*parsed, "target")),
                 std::string("HKCU\\Software\\Microsoft\\GameBar"));
        CHECK_EQ(std::string(*json::find(*parsed, "detail")),
                 std::string("line one\nline two\ttabbed"));
    }
}

TEST_CASE("json writes an empty object") {
    const std::string text = json::write_object({});
    CHECK_EQ(text, std::string("{}"));

    std::string error;
    const auto parsed = json::parse_object(text, error);
    CHECK(parsed.has_value());
    CHECK(parsed->empty());
}

TEST_CASE("json rejects malformed input instead of guessing") {
    std::string error;
    CHECK(!json::parse_object("{\"a\":\"b\",}", error).has_value());
    CHECK(!error.empty());

    error.clear();
    CHECK(!json::parse_object("{\"a\":1}", error).has_value());

    error.clear();
    CHECK(!json::parse_object("{\"a\":\"b\"} trailing", error).has_value());

    error.clear();
    CHECK(!json::parse_object("a=b", error).has_value());

    error.clear();
    CHECK(!json::parse_object("{\"a\":\"b\"", error).has_value());
}

TEST_CASE("json decodes basic escapes and unicode") {
    std::string error;
    const auto parsed = json::parse_object("{\"a\":\"x\\u0041y\\n\"}", error);
    CHECK(parsed.has_value());
    if (parsed) {
        CHECK_EQ(std::string(*json::find(*parsed, "a")), std::string("xAy\n"));
    }
}

TEST_CASE("json find returns null for a missing key") {
    std::string error;
    const auto parsed = json::parse_object("{\"a\":\"b\"}", error);
    CHECK(parsed.has_value());
    if (parsed) {
        CHECK(json::find(*parsed, "missing") == nullptr);
    }
}
