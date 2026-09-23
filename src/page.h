#pragma once
#include <cstddef>
#include <cstdint>
#include "constants.h"

struct Page {
    uint8_t data[PAGE_SIZE]{};
    bool wal_initialised = true;

    uint8_t* bytes() { return data; }
    const uint8_t* bytes() const { return data; }
};

Page create_heap_page();