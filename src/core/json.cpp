#include "json.hpp"

#include <cmath>
#include <cstdio>
#include <cstdlib>

namespace json {
namespace {

struct Parser {
    const std::string& t;
    size_t i = 0;
    explicit Parser(const std::string& text) : t(text) {}

    void ws() { while (i < t.size() && (t[i] == ' ' || t[i] == '\n' || t[i] == '\r' || t[i] == '\t')) ++i; }
    bool lit(const char* word) {
        size_t n = 0;
        while (word[n]) ++n;
        if (t.compare(i, n, word) == 0) { i += n; return true; }
        return false;
    }

    bool str(std::string& out) {
        if (i >= t.size() || t[i] != '"') return false;
        ++i;
        out.clear();
        while (i < t.size() && t[i] != '"') {
            char c = t[i++];
            if (c == '\\') {
                if (i >= t.size()) return false;
                char e = t[i++];
                switch (e) {
                    case 'n': out += '\n'; break;
                    case 't': out += '\t'; break;
                    case 'r': out += '\r'; break;
                    case 'b': out += '\b'; break;
                    case 'f': out += '\f'; break;
                    case '/': out += '/'; break;
                    case '\\': out += '\\'; break;
                    case '"': out += '"'; break;
                    case 'u': {
                        if (i + 4 > t.size()) return false;
                        unsigned cp = static_cast<unsigned>(std::strtoul(t.substr(i, 4).c_str(), nullptr, 16));
                        i += 4;
                        if (cp < 0x80) out += static_cast<char>(cp);
                        else if (cp < 0x800) { out += static_cast<char>(0xC0 | (cp >> 6)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
                        else { out += static_cast<char>(0xE0 | (cp >> 12)); out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F)); out += static_cast<char>(0x80 | (cp & 0x3F)); }
                        break;
                    }
                    default: return false;
                }
            } else {
                out += c;
            }
        }
        if (i >= t.size()) return false;
        ++i;
        return true;
    }

    bool value(Value& v, int depth) {
        if (depth > 64) return false;
        ws();
        if (i >= t.size()) return false;
        char c = t[i];
        if (c == '{') {
            ++i;
            v = Value::object();
            ws();
            if (i < t.size() && t[i] == '}') { ++i; return true; }
            while (true) {
                ws();
                std::string k;
                if (!str(k)) return false;
                ws();
                if (i >= t.size() || t[i] != ':') return false;
                ++i;
                Value child;
                if (!value(child, depth + 1)) return false;
                v.o[k] = std::move(child);
                ws();
                if (i < t.size() && t[i] == ',') { ++i; continue; }
                if (i < t.size() && t[i] == '}') { ++i; return true; }
                return false;
            }
        }
        if (c == '[') {
            ++i;
            v = Value::array();
            ws();
            if (i < t.size() && t[i] == ']') { ++i; return true; }
            while (true) {
                Value child;
                if (!value(child, depth + 1)) return false;
                v.a.push_back(std::move(child));
                ws();
                if (i < t.size() && t[i] == ',') { ++i; continue; }
                if (i < t.size() && t[i] == ']') { ++i; return true; }
                return false;
            }
        }
        if (c == '"') { v.type = Value::Type::String; return str(v.s); }
        if (lit("true")) { v = Value(true); return true; }
        if (lit("false")) { v = Value(false); return true; }
        if (lit("null")) { v = Value(); return true; }
        char* end = nullptr;
        double d = std::strtod(t.c_str() + i, &end);
        if (end == t.c_str() + i) return false;
        i = static_cast<size_t>(end - t.c_str());
        v = Value(d);
        return true;
    }
};

void esc(const std::string& s, std::string& out) {
    out += '"';
    for (unsigned char c : s) {
        switch (c) {
            case '"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default:
                if (c < 0x20) { char buf[8]; std::snprintf(buf, sizeof buf, "\\u%04x", c); out += buf; }
                else out += static_cast<char>(c);
        }
    }
    out += '"';
}

void write(const Value& v, std::string& out, int indent, int level) {
    auto nl = [&](int lv) { if (indent > 0) { out += '\n'; out.append(static_cast<size_t>(lv * indent), ' '); } };
    switch (v.type) {
        case Value::Type::Null: out += "null"; break;
        case Value::Type::Bool: out += v.b ? "true" : "false"; break;
        case Value::Type::Number: {
            char buf[64];
            if (std::floor(v.n) == v.n && std::fabs(v.n) < 1e15) std::snprintf(buf, sizeof buf, "%lld", static_cast<long long>(v.n));
            else std::snprintf(buf, sizeof buf, "%.10g", v.n);
            out += buf;
            break;
        }
        case Value::Type::String: esc(v.s, out); break;
        case Value::Type::Array: {
            out += '[';
            bool first = true;
            for (const auto& e : v.a) {
                if (!first) out += ',';
                first = false;
                nl(level + 1);
                write(e, out, indent, level + 1);
            }
            if (!v.a.empty()) nl(level);
            out += ']';
            break;
        }
        case Value::Type::Object: {
            out += '{';
            bool first = true;
            for (const auto& kv : v.o) {
                if (!first) out += ',';
                first = false;
                nl(level + 1);
                esc(kv.first, out);
                out += indent > 0 ? ": " : ":";
                write(kv.second, out, indent, level + 1);
            }
            if (!v.o.empty()) nl(level);
            out += '}';
            break;
        }
    }
}

}  // namespace

bool parse(const std::string& text, Value& out) {
    Parser p(text);
    if (!p.value(out, 0)) return false;
    p.ws();
    return p.i == text.size();
}

std::string dump(const Value& v, int indent) {
    std::string out;
    write(v, out, indent, 0);
    return out;
}

}  // namespace json
