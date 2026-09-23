#pragma once

#include <vector>
#include <cstdint>
#include <chrono>
#include "page_manager.h"
#include "wal.h"
#include "heap_page.h"
#include "tuple.h"
#include "row.h"
#include "value.h"
#include "schema.h"


class HeapManager
{
public:
    HeapManager(PageManager& pm, WalManager& wm);

    uint16_t insert_row(
        const TableSchema& schema,
        Row& row,
        uint32_t& xmin,
        uint32_t relation_id);

private:
    PageManager& page_manager;
    WalManager& wal_manager;

    PageId current_page_id;
    std::optional<HeapPage> current_heap_page;

    bool initialised = false;

    void fetch_new_page();
    void wal_inserts();
};
