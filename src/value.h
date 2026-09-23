#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <variant>
#include <cassert>

#include "type.h"

struct Value {
    TypeKind type;

    using Storage = std::variant<
        int32_t,
        int64_t,
        bool,
        double,
        std::string,
        std::vector<uint8_t>
    >;

    Storage data;

    // CONSTRUCTORS

    static Value Int32(int32_t v);
    static Value Int64(int64_t v);
    static Value Bool(bool v);
    static Value Float64(double v);
    static Value Varchar(std::string v);
    static Value Text(std::string v);
    static Value Bytes(std::vector<uint8_t> v);


    int32_t as_int32() const;
    int64_t as_int64() const;
    bool as_bool() const;
    double as_float64() const;
    const std::string& as_string() const;
    const std::vector<uint8_t>& as_bytes() const;


    void to_bytes(std::vector<uint8_t>& out) const;

    static Value from_bytes(TypeKind type, const uint8_t*& p);
};
