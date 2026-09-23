#pragma once
#include <unordered_map>
#include "page.h"
#include "page_id.h"

class PageManager
{
public:
    // WHY BOTH?
    // gets an already existing page
    Page &get_page(const PageId &id);

    // gets (or creates) a HEAP page (mainly used for creating pages)
    Page &get_or_create_page(const PageId &id);

    bool has_page(const PageId &id) const;

private:
    // hash for page table
    struct PageIdHash
    {
        size_t operator()(const PageId &id) const
        {
            uint64_t hi = static_cast<uint64_t>(id.relation);
            uint64_t lo = static_cast<uint64_t>(id.page);
            return static_cast<size_t>((hi << 32) | lo); // simple hash for page id
        }
    };

    std::unordered_map<PageId, Page, PageIdHash> pages;
};
