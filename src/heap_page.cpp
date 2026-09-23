#include "heap_page.h"

#include <cstring>
#include <stdexcept>
#include <iostream>
#include "constants.h"

struct HeapPageHeader
{
    uint64_t wal_lsn;
    uint16_t lower_offset;
    uint16_t upper_offset;
    uint8_t flag;
};

struct Slot
{
    uint32_t raw;
};
static_assert(sizeof(Slot) == 4);

static HeapPageHeader *header(uint8_t *data)
{
    return reinterpret_cast<HeapPageHeader *>(data);
}

static const HeapPageHeader *header(const uint8_t *data)
{
    return reinterpret_cast<const HeapPageHeader *>(data);
}

static inline uint32_t read_packed_slot(const uint8_t* data, uint16_t slot_id)
{
    uint32_t v;
    std::memcpy(&v, data + heap::HEADER_SIZE + slot_id * heap::SLOT_TUPLE_SIZE, sizeof(uint32_t));
    return v;
}

static inline uint16_t slot_offset(uint32_t v)
{
    return (v >> 17) & 0x7FFF;
}

static inline uint16_t slot_length(uint32_t v)
{
    return (v >> 2) & 0x7FFF;
}

static inline uint8_t slot_flags(uint32_t v)
{
    return v & 0x3;
}

HeapPage::HeapPage(uint8_t *data)
    : data_(data)
{
    // HEAP PAGE IS ALREADY INITIALISED FROM page.cpp
    const HeapPageHeader *hdr = header(data_);
    wal_start_slot = (hdr->lower_offset - heap::HEADER_SIZE) / heap::SLOT_TUPLE_SIZE; // current lsn data pointer
}

size_t HeapPage::free_space() const
{
    const HeapPageHeader *hdr = header(data_);

    return hdr->upper_offset - hdr->lower_offset;
}

uint8_t *HeapPage::data() const
{
    return data_;
}

bool HeapPage::can_fit(size_t tuple_size) const
{
    return free_space() >= tuple_size + heap::SLOT_TUPLE_SIZE;
}

uint16_t HeapPage::insert_tuple(const Tuple &tuple)
{
    HeapPageHeader *hdr = header(data_);

    size_t needed = tuple.size() + heap::SLOT_TUPLE_SIZE;
    if (free_space() < needed)
    {
        throw std::runtime_error("HeapPage: not enough free space");
    }

    // gives space for tuple
    uint16_t tuple_offset =
        static_cast<uint16_t>(hdr->upper_offset - tuple.size());

    // copies tuple bytes
    std::memcpy(
        data_ + tuple_offset,
        tuple.data(),
        tuple.size());

    // Slot slot{tuple_offset, tuple.size(), 0};

    uint32_t packed_slot =
        ((uint32_t)(tuple_offset & 0x7FFF) << 17) | ((uint32_t)(tuple.size() & 0x7FFF) << 2) | (uint32_t)(0 & 0x3);

    // copies ref slot
    std::memcpy(
        data_ + hdr->lower_offset,
        &packed_slot,
        sizeof(uint32_t));

    uint16_t new_lower = hdr->lower_offset + heap::SLOT_TUPLE_SIZE;
    uint16_t new_upper = hdr->upper_offset - tuple.size();

    // sets new range values
    std::memcpy(data_ + 8, &new_lower, sizeof(uint16_t));
    std::memcpy(data_ + 10, &new_upper, sizeof(uint16_t));

    return 0;
}

// sends back data of tuple from slot id in page
const Tuple HeapPage::get_tuple(uint16_t slot_id) const
{
    uint32_t packed = read_packed_slot(data_, slot_id);

    uint16_t offset = slot_offset(packed);
    uint16_t length = slot_length(packed);

    Tuple t;
    std::memcpy(
        t.data_.data(),
        data_ + offset,
        length
    );

    return t;
}

uint16_t HeapPage::get_slot_count() const
{
    const HeapPageHeader *hdr = header(data_);
    return (hdr->lower_offset - heap::HEADER_SIZE) / heap::SLOT_TUPLE_SIZE; // find number of slots from lower_offset position
}

void HeapPage::set_lsn(uint64_t lsn)
{
    std::memcpy(data_, &lsn, sizeof(uint64_t));
}
