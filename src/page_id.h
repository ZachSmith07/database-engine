#pragma once
#include <cstdint>

using RelationId = uint32_t;
using PageNumber = uint32_t;

enum class PageType {
    Heap,
    BTreeInternal,
    BTreeLeaf
};

struct PageId {
    RelationId relation;
    PageNumber page;
    PageType pageType;

    bool operator==(const PageId& other) const {
        return relation == other.relation && page == other.page;
    }
};
