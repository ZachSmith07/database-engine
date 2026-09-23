#pragma once
#include <cstdint>
#include "tuple.h"

class HeapPage {
public:
    explicit HeapPage(uint8_t* data);

    uint16_t wal_start_slot;
    bool can_fit(size_t tuple_size) const;
    uint16_t insert_tuple(const Tuple& tuple);
    const Tuple get_tuple(uint16_t slot) const;
    uint16_t get_slot_count() const;
    void set_lsn(uint64_t lsn);

    size_t free_space() const;

    uint8_t* data() const;

private:
    uint8_t* data_;  // raw page bytes (as pointer to first byte)
};

