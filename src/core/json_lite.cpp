#include "core/json_lite.h"

#include <cctype>

namespace vnm::json {
namespace {

struct Parser {
    std::string_view text;
    std::string& error;
    std::size_t pos = 0;

    void skip_space() {
        while (pos < text.size()) {
            const char c = text[pos];
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                ++pos;
            } else {
                break;
            }
        }
    }

    bool at_end() const { return pos >= text.size(); }

    bool expect(char wanted) {
        skip_space();
        if (at_end() || text[pos] != wanted) {
            return false;
        }
        ++pos;
        return true;
    }

    bool parse_string(std::string& out) {
        skip_space();
        if (at_end() || text[pos] != '"') {
            error = "expected a quoted string";
            return false;
        }
        ++pos;
        out.clear();
        while (!at_end()) {
            const char c = text[pos++];
            if (c == '"') {
                return true;
            }
            if (c != '\\') {
                out.push_back(c);
                continue;
            }
            if (at_end()) {
                error = "unterminated escape sequence";
                return false;
            }
            const char esc = text[pos++];
            switch (esc) {
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                case '/': out.push_back('/'); break;
                case 'b': out.push_back('\b'); break;
                case 'f': out.push_back('\f'); break;
                case 'n': out.push_back('\n'); break;
                case 'r': out.push_back('\r'); break;
                case 't': out.push_back('\t'); break;
                case 'u': {
                    if (pos + 4 > text.size()) {
                        error = "truncated \\u escape";
                        return false;
                    }
                    unsigned code = 0;
                    for (int i = 0; i < 4; ++i) {
                        const char h = text[pos++];
                        code <<= 4U;
                        if (h >= '0' && h <= '9') {
                            code |= static_cast<unsigned>(h - '0');
                        } else if (h >= 'a' && h <= 'f') {
                            code |= static_cast<unsigned>(h - 'a' + 10);
                        } else if (h >= 'A' && h <= 'F') {
                            code |= static_cast<unsigned>(h - 'A' + 10);
                        } else {
                            error = "bad hex digit in \\u escape";
                            return false;
                        }
                    }
                    // The subset is ASCII-only; anything else becomes '?'.
                    out.push_back(code < 0x80 ? static_cast<char>(code) : '?');
                    break;
                }
                default:
                    error = "unsupported escape sequence";
                    return false;
            }
        }
        error = "unterminated string";
        return false;
    }
};

}  // namespace

std::string escape(std::string_view value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (const char c : value) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\b': out += "\\b"; break;
            case '\f': out += "\\f"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (static_cast<unsigned char>(c) < 0x20) {
                    out += "?";
                } else {
                    out.push_back(c);
                }
        }
    }
    return out;
}

std::string write_object(const Object& fields) {
    std::string out = "{";
    bool first = true;
    for (const auto& [key, value] : fields) {
        if (!first) {
            out += ',';
        }
        first = false;
        out += '"';
        out += escape(key);
        out += "\":\"";
        out += escape(value);
        out += '"';
    }
    out += '}';
    return out;
}

std::optional<Object> parse_object(std::string_view text, std::string& error) {
    error.clear();
    Parser parser{text, error};

    if (!parser.expect('{')) {
        if (error.empty()) {
            error = "expected '{'";
        }
        return std::nullopt;
    }

    Object fields;
    parser.skip_space();
    if (!parser.at_end() && parser.text[parser.pos] == '}') {
        ++parser.pos;
    } else {
        while (true) {
            std::string key;
            std::string value;
            if (!parser.parse_string(key)) {
                return std::nullopt;
            }
            if (!parser.expect(':')) {
                error = "expected ':' after key";
                return std::nullopt;
            }
            if (!parser.parse_string(value)) {
                return std::nullopt;
            }
            fields.emplace_back(std::move(key), std::move(value));

            parser.skip_space();
            if (parser.at_end()) {
                error = "unterminated object";
                return std::nullopt;
            }
            if (parser.text[parser.pos] == ',') {
                ++parser.pos;
                continue;
            }
            if (parser.text[parser.pos] == '}') {
                ++parser.pos;
                break;
            }
            error = "expected ',' or '}'";
            return std::nullopt;
        }
    }

    parser.skip_space();
    if (!parser.at_end()) {
        error = "trailing content after object";
        return std::nullopt;
    }
    return fields;
}

const std::string* find(const Object& object, std::string_view key) {
    for (const auto& [k, v] : object) {
        if (k == key) {
            return &v;
        }
    }
    return nullptr;
}

}  // namespace vnm::json
