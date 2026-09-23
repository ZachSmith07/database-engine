#include "page_manager.h"

Page& PageManager::get_page(const PageId& id) {
    return pages.at(id);
}

Page& PageManager::get_or_create_page(const PageId& id) {
    return pages.try_emplace(id, create_heap_page()).first->second;
}

bool PageManager::has_page(const PageId& id) const {
    return pages.find(id) != pages.end();
}
