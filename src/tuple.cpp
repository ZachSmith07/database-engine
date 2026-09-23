#include "tuple.h"
#include <cstring>
#include <cassert>

namespace
{
    template <typename T>
    void append(std::vector<uint8_t> &out, T v)
    {
        uint8_t *p = reinterpret_cast<uint8_t *>(&v);
        out.insert(out.end(), p, p + sizeof(T));
    }

    void append_bytes(std::vector<uint8_t> &out, const std::vector<uint8_t> &bytes)
    {
        out.insert(out.end(), bytes.begin(), bytes.end());
    }
}

Tuple Tuple::from_row(
    const TableSchema &schema,
    const Row &row,
    uint32_t xmin)
{
    Tuple tuple;

    const size_t ncols = schema.columns.size();
    const size_t null_bytes = (ncols + 7) / 8; // number of bytes for nullable columns
    
    std::vector<const Value *> values(ncols, nullptr);
    for (size_t i = 0; i < ncols; ++i)
    {
        auto it = row.find(schema.columns[i].id);
        if (it != row.end())
            values[i] = &it->second;
    }

    // VALIDATION
    for (size_t i = 0; i < ncols; ++i)
    {
        const auto &col = schema.columns[i];
        const Value *v = values[i];

        if (!v && !col.nullable)
        {
            throw std::runtime_error(
                "NULL value for NOT NULL column: " + col.name);
        }

        if (!v)
            continue;

        if (v->type != col.type.kind)
        {
            throw std::runtime_error(
                "Type mismatch for column: " + col.name);
        }

        // max length check for variable length string
        if (col.type.is_variable_length())
        {
            size_t len =
                (col.type.kind == TypeKind::Bytes)
                    ? v->as_bytes().size()
                    : v->as_string().size();

            if (col.type.has_length_limit() &&
                len > col.type.max_length())
            {
                throw std::runtime_error(
                    "Value too long for column: " + col.name);
            }
        }
    }

    // PRECOMPUTE SIZE
    size_t total =
        sizeof(uint32_t) * 2 +
        sizeof(uint16_t) * 2 +
        null_bytes;

    for (size_t i = 0; i < ncols; ++i)
    {
        const auto &col = schema.columns[i];
        const Value *v = values[i];

        if (!col.type.is_variable_length())
        {
            total += fixed_size(col.type.kind);
        }
        else
        {
            total += 4;
            if (v)
            {
                total += (col.type.kind == TypeKind::Bytes)
                             ? v->as_bytes().size()
                             : v->as_string().size();
            }
        }
    }

    tuple.data_.resize(total);
    uint8_t *p = tuple.data_.data();

    // HEADER
    uint32_t xmax = 0;
    uint16_t flags = 0;
    uint16_t hoff =
        sizeof(uint32_t) * 2 +
        sizeof(uint16_t) * 2 +
        null_bytes;

    memcpy(p, &xmin, 4);
    p += 4;
    memcpy(p, &xmax, 4);
    p += 4;
    memcpy(p, &flags, 2);
    p += 2;
    memcpy(p, &hoff, 2);
    p += 2;

    // NULL BITMAP
    uint8_t *nullmap = p;
    memset(nullmap, 0, null_bytes);
    p += null_bytes;

    for (size_t i = 0; i < ncols; ++i)
    {
        if (!values[i])
        {
            nullmap[i >> 3] |= (1u << (i & 7));
        }
    }

    // ACTUAL DATA
    for (size_t i = 0; i < ncols; ++i)
    {
        const auto &col = schema.columns[i];
        const Value *v = values[i];

        if (!col.type.is_variable_length())
        {
            size_t sz = fixed_size(col.type.kind);

            if (!v)
            {
                memset(p, 0, sz);
                p += sz;
                continue;
            }

            switch (col.type.kind)
            {
            case TypeKind::Int32:
            {
                int32_t x = v->as_int32();
                memcpy(p, &x, 4);
                p += 4;
                break;
            }
            case TypeKind::Int64:
            {
                int64_t x = v->as_int64();
                memcpy(p, &x, 8);
                p += 8;
                break;
            }
            case TypeKind::Bool:
                *p++ = v->as_bool() ? 1 : 0;
                break;
            case TypeKind::Float64:
            {
                double x = v->as_float64();
                memcpy(p, &x, 8);
                p += 8;
                break;
            }
            }
        }
        else
        {
            if (!v)
            {
                uint32_t zero = 0;
                memcpy(p, &zero, 4);
                p += 4;
                continue;
            }

            if (col.type.kind == TypeKind::Bytes)
            {
                const auto &b = v->as_bytes();
                uint32_t len = b.size();
                memcpy(p, &len, 4);
                p += 4;
                memcpy(p, b.data(), len);
                p += len;
            }
            else
            {
                const auto &s = v->as_string();
                uint32_t len = s.size();
                memcpy(p, &len, 4);
                p += 4;
                memcpy(p, s.data(), len);
                p += len;
            }
        }
    }

    return tuple;
}

Row Tuple::to_row(const TableSchema &schema) const
{
    return Row();
}