#pragma once
#include <cstdint>
#include <optional>

enum class TypeKind : uint8_t
{
    Int32,
    Int64,
    Bool,
    Float64,
    Varchar, // variable-length with limit
    Text, // variable-length but unbounded
    Bytes
};

inline size_t fixed_size(TypeKind k)
{
    switch (k)
    {
    case TypeKind::Int32:
        return 4;
    case TypeKind::Int64:
        return 8;
    case TypeKind::Bool:
        return 1;
    case TypeKind::Float64:
        return 8;
    default:
        return 0; // variable-length
    }
}

struct Type
{
    TypeKind kind{};
    // for Varchar(n) only
    std::optional<uint32_t> length_limit;

    static Type Int32() { return {TypeKind::Int32, std::nullopt}; }
    static Type Int64() { return {TypeKind::Int64, std::nullopt}; }
    static Type Bool() { return {TypeKind::Bool, std::nullopt}; }
    static Type Float64() { return {TypeKind::Float64, std::nullopt}; }
    static Type Text() { return {TypeKind::Text, std::nullopt}; }
    static Type Bytes() { return {TypeKind::Bytes, std::nullopt}; }
    static Type Varchar(uint32_t n) { return {TypeKind::Varchar, n}; }

    bool is_variable_length() const
    {
        return kind == TypeKind::Varchar || kind == TypeKind::Text || kind == TypeKind::Bytes;
    }

    bool has_length_limit() const
    {
        return length_limit.has_value();
    }

    uint32_t max_length() const
    {
        return length_limit.value_or(0);
    }

    bool is_toastable() const
    {
        return kind == TypeKind::Text ||
               kind == TypeKind::Bytes ||
               kind == TypeKind::Varchar;
    }

    bool operator==(const Type &other) const
    {
        return kind == other.kind &&
               length_limit == other.length_limit;
    }
};