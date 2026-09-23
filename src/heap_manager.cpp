#include "heap_manager.h"
#include <iostream>

HeapManager::HeapManager(PageManager &pm,
                         WalManager &wm)
    : page_manager(pm),
      wal_manager(wm)
{
}

uint16_t HeapManager::insert_row(
    const TableSchema &schema,
    Row &row,
    uint32_t &xmin,
    uint32_t relation_id)
{
    if (!initialised)
    {
        current_page_id.page = 0;
        current_page_id.pageType = PageType::Heap;
        current_page_id.relation = relation_id;

        fetch_new_page();

        initialised = true;
    }

    Tuple t = Tuple::from_row(schema, row, xmin);

    if (!(*current_heap_page).can_fit(t.size()))
    {
        wal_inserts();
        
        current_page_id.page += 1;

        fetch_new_page();
    }

    uint16_t slot = (*current_heap_page).insert_tuple(t);

    return 0;
}

void HeapManager::fetch_new_page()
{
    Page &first_page = page_manager.get_or_create_page(current_page_id);
    current_heap_page = HeapPage(first_page.data);

    std::vector<uint8_t> bytes_res1 = get_heap_page_create_wal(current_page_id, *current_heap_page);
    wal_manager.append(bytes_res1.data(), bytes_res1.size());
}

void HeapManager::wal_inserts()
{
    (*current_heap_page).set_lsn(wal_manager.current_lsn());
    std::vector<uint8_t> bytes_res = get_heap_page_insert_wal(current_page_id, *current_heap_page);
    wal_manager.append(bytes_res.data(), bytes_res.size());
}
