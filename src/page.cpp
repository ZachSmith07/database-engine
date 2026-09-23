#include "page.h"
#include <cstring>
#include <iostream>

Page create_heap_page() {
    Page page{};

    *reinterpret_cast<uint16_t*>(page.data + 8)  = 13;
    *reinterpret_cast<uint16_t*>(page.data + 10) = PAGE_SIZE;
    page.wal_initialised = false;
    // std::cout << "CREATED PAGE?" << std::endl;
    return page;
}
