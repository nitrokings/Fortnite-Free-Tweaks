#pragma once

// A deliberately small test harness: no third-party dependency to break the
// build. Each TEST_CASE registers itself; vnm_tests returns non-zero when
// anything failed, which is what CTest reads.

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace t {

struct Case {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<Case>& cases() {
    static std::vector<Case> all;
    return all;
}

inline int& failures() {
    static int count = 0;
    return count;
}

struct Registrar {
    Registrar(std::string name, std::function<void()> fn) {
        cases().push_back({std::move(name), std::move(fn)});
    }
};

template <typename T>
std::string show(const T& value) {
    std::ostringstream stream;
    stream << value;
    return stream.str();
}

inline std::string show(bool value) { return value ? "true" : "false"; }
inline std::string show(const std::string& value) { return "\"" + value + "\""; }
inline std::string show(std::string_view value) { return "\"" + std::string(value) + "\""; }

inline void fail(const char* file, int line, const std::string& message) {
    ++failures();
    std::cout << "    FAIL " << file << ":" << line << "  " << message << '\n';
}

template <typename A, typename B>
void check_equal(const char* file, int line, const char* leftText, const char* rightText,
                 const A& left, const B& right) {
    if (!(left == right)) {
        fail(file, line, std::string(leftText) + " == " + rightText + "  (got " + show(left) +
                             " vs " + show(right) + ")");
    }
}

inline int run_all() {
    int failedCases = 0;
    for (auto& testCase : cases()) {
        const int before = failures();
        try {
            testCase.fn();
        } catch (const std::exception& error) {
            fail(__FILE__, __LINE__, std::string("unexpected exception: ") + error.what());
        } catch (...) {
            fail(__FILE__, __LINE__, "unexpected non-standard exception");
        }
        if (failures() != before) {
            ++failedCases;
            std::cout << "  failed: " << testCase.name << '\n';
        }
    }

    const auto total = static_cast<int>(cases().size());
    std::cout << (total - failedCases) << '/' << total << " test cases passed, " << failures()
              << " assertion failure(s)\n";
    return failures() == 0 ? 0 : 1;
}

}  // namespace t

#define TC_CAT_(a, b) a##b
#define TC_CAT(a, b) TC_CAT_(a, b)

#define TEST_CASE(name)                                                          \
    static void TC_CAT(tc_fn_, __LINE__)();                                      \
    static t::Registrar TC_CAT(tc_reg_, __LINE__)(name, TC_CAT(tc_fn_, __LINE__)); \
    static void TC_CAT(tc_fn_, __LINE__)()

#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            t::fail(__FILE__, __LINE__, "CHECK failed: " #cond);       \
        }                                                              \
    } while (false)

#define CHECK_EQ(left, right) t::check_equal(__FILE__, __LINE__, #left, #right, (left), (right))
