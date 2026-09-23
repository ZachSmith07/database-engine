#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>

#include "schema.h"
#include "value.h"
#include "row.h"

// tuple data stored in heap pages
class Tuple
{
public:
    Tuple() = default;

    // makes tuple bytes
    static Tuple from_row(const TableSchema &schema, const Row &row, uint32_t xmin);

    // decodes bytes to Row
    Row to_row(const TableSchema &schema) const;

    const uint8_t *data() const { return data_.data(); }
    size_t size() const { return data_.size(); }

private:
    std::vector<uint8_t> data_;

    // lets HeapPage to construct tuples from rows
    friend class HeapPage;

    explicit Tuple(size_t size) : data_(size) {}
};
