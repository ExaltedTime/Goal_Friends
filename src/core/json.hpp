// Minimal JSON value, parser and writer. Enough for save files; not a general-purpose library.
#pragma once
#include <map>
#include <string>
#include <vector>

namespace json {

struct Value {
    enum class Type { Null, Bool, Number, String, Array, Object };
    Type type = Type::Null;
    bool b = false;
    double n = 0;
    std::string s;
    std::vector<Value> a;
    std::map<std::string, Value> o;

    Value() = default;
    Value(bool v) : type(Type::Bool), b(v) {}
    Value(int v) : type(Type::Number), n(v) {}
    Value(long long v) : type(Type::Number), n(static_cast<double>(v)) {}
    Value(double v) : type(Type::Number), n(v) {}
    Value(const char* v) : type(Type::String), s(v) {}
    Value(const std::string& v) : type(Type::String), s(v) {}

    static Value array() { Value v; v.type = Type::Array; return v; }
    static Value object() { Value v; v.type = Type::Object; return v; }

    void push(Value v) { if (type != Type::Array) { type = Type::Array; a.clear(); } a.push_back(std::move(v)); }
    Value& operator[](const std::string& k) { if (type != Type::Object) { type = Type::Object; o.clear(); } return o[k]; }

    bool has(const std::string& k) const { return type == Type::Object && o.count(k) > 0; }
    const Value& at(const std::string& k) const {
        static const Value null_value;
        auto it = o.find(k);
        return it == o.end() ? null_value : it->second;
    }
    int asInt(int def = 0) const { return type == Type::Number ? static_cast<int>(n) : def; }
    long long asInt64(long long def = 0) const { return type == Type::Number ? static_cast<long long>(n) : def; }
    double asNum(double def = 0) const { return type == Type::Number ? n : def; }
    bool asBool(bool def = false) const { return type == Type::Bool ? b : def; }
    std::string asStr(const std::string& def = "") const { return type == Type::String ? s : def; }
    const std::vector<Value>& arr() const { return a; }
};

// Returns false on a syntax error (out is left unspecified).
bool parse(const std::string& text, Value& out);
std::string dump(const Value& v, int indent = 0);

}  // namespace json
