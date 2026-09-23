#include "value.h"

#include <cstring>
#include <type_traits>
#include <stdexcept>


template <typename T>
static void append_le(std::vector<uint8_t>& out, T v) {
    static_assert(std::is_integral_v<T>);
    for (size_t i = 0; i < sizeof(T); ++i) {
        out.push_back(uint8_t((uint64_t(v) >> (i * 8)) & 0xFF));
    }
}

template <typename T>
static T read_le(const uint8_t*& p) {
    static_assert(std::is_integral_v<T>);
    T v = 0;
    for (size_t i = 0; i < sizeof(T); ++i) {
        v |= (T(p[i]) << (i * 8));
    }
    p += sizeof(T);
    return v;
}


Value Value::Int32(int32_t v) {
    return { TypeKind::Int32, v };
}

Value Value::Int64(int64_t v) {
    return { TypeKind::Int64, v };
}

Value Value::Bool(bool v) {
    return { TypeKind::Bool, v };
}

Value Value::Float64(double v) {
    return { TypeKind::Float64, v };
}

Value Value::Varchar(std::string v) {
    return { TypeKind::Varchar, std::move(v) };
}

Value Value::Text(std::string v) {
    return { TypeKind::Text, std::move(v) };
}

Value Value::Bytes(std::vector<uint8_t> v) {
    return { TypeKind::Bytes, std::move(v) };
}


int32_t Value::as_int32() const {
    assert(type == TypeKind::Int32);
    return std::get<int32_t>(data);
}

int64_t Value::as_int64() const {
    assert(type == TypeKind::Int64);
    return std::get<int64_t>(data);
}

bool Value::as_bool() const {
    assert(type == TypeKind::Bool);
    return std::get<bool>(data);
}

double Value::as_float64() const {
    assert(type == TypeKind::Float64);
    return std::get<double>(data);
}

const std::string& Value::as_string() const {
    assert(type == TypeKind::Varchar || type == TypeKind::Text);
    return std::get<std::string>(data);
}

const std::vector<uint8_t>& Value::as_bytes() const {
    assert(type == TypeKind::Bytes);
    return std::get<std::vector<uint8_t>>(data);
}


void Value::to_bytes(std::vector<uint8_t>& out) const {
    switch (type) {
        case TypeKind::Int32:
            append_le(out, std::get<int32_t>(data));
            break;

        case TypeKind::Int64:
            append_le(out, std::get<int64_t>(data));
            break;

        case TypeKind::Bool:
            out.push_back(std::get<bool>(data) ? 1 : 0);
            break;

        case TypeKind::Float64: {
            double v = std::get<double>(data);
            uint64_t bits;
            static_assert(sizeof(bits) == sizeof(v));
            std::memcpy(&bits, &v, sizeof(v));
            append_le(out, bits);
            break;
        }

        case TypeKind::Varchar:
        case TypeKind::Text: {
            const auto& s = std::get<std::string>(data);
            append_le(out, uint32_t(s.size()));
            out.insert(out.end(), s.begin(), s.end());
            break;
        }

        case TypeKind::Bytes: {
            const auto& b = std::get<std::vector<uint8_t>>(data);
            append_le(out, uint32_t(b.size()));
            out.insert(out.end(), b.begin(), b.end());
            break;
        }
    }
}

Value Value::from_bytes(TypeKind type, const uint8_t*& p) {
    switch (type) {
        case TypeKind::Int32:
            return Value::Int32(read_le<int32_t>(p));

        case TypeKind::Int64:
            return Value::Int64(read_le<int64_t>(p));

        case TypeKind::Bool:
            return Value::Bool(*p++ != 0);

        case TypeKind::Float64: {
            uint64_t bits = read_le<uint64_t>(p);
            double v;
            std::memcpy(&v, &bits, sizeof(v));
            return Value::Float64(v);
        }

        case TypeKind::Varchar:
        case TypeKind::Text: {
            uint32_t len = read_le<uint32_t>(p);
            std::string s(reinterpret_cast<const char*>(p), len);
            p += len;
            return type == TypeKind::Varchar
                ? Value::Varchar(std::move(s))
                : Value::Text(std::move(s));
        }

        case TypeKind::Bytes: {
            uint32_t len = read_le<uint32_t>(p);
            std::vector<uint8_t> b(p, p + len);
            p += len;
            return Value::Bytes(std::move(b));
        }
    }

    throw std::runtime_error("Invalid TypeKind in Value::from_bytes");
}
