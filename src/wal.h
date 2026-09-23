#pragma once

#define NOMINMAX
#include <vector>
#include "page_id.h"
#include "heap_page.h"
#include <windows.h>
#include <cstdint>
#include "constants.h"

constexpr size_t WAL_FLUSH_THRESHOLD = 4 * 1024 * 1024; // 4MB

std::vector<uint8_t> get_heap_page_insert_wal(const PageId &page_id, const HeapPage &page);
std::vector<uint8_t> get_heap_page_create_wal(const PageId &page_id, const HeapPage &page);

struct WalBuffer
{
    std::vector<uint8_t> data;
    size_t write_pos = 0;

    explicit WalBuffer(size_t cap = 64 * 1024)
    {
        data.resize(cap);
    }
};

struct WalPage
{
    alignas(8) uint8_t data[wal::PAGE_SIZE];
    uint16_t write_pos = 0;

    void reset()
    {
        std::memset(data, 0, wal::PAGE_SIZE);
        write_pos = 0;
    }
};

// struct WalManager
// {
//     HANDLE wal_handle = INVALID_HANDLE_VALUE;
//     WalBuffer buffer;
//     WalPage current_page;
//     uint64_t next_lsn = 0;
//     uint32_t segment_number = 0;
//     uint32_t page_number = 0;
//     uint32_t write_pos = 0;

//     std::vector<uint8_t> wal_write_buffer;
//     size_t buffered_bytes = 0;

//     explicit WalManager(const wchar_t *path);

//     void append(const void *src, size_t len);
//     void add_wal_header(uint32_t continuation_len);
//     // uint64_t append_record(uint16_t type, const void* payload, size_t len);
//     void flush_buffer();
//     void flush();
// };

class WalManager
{
public:
    WalManager(const wchar_t *folder);
    ~WalManager() = default;

    void append(const void *src, size_t len);
    void flush(); // dont use anymore (paged wal)
    uint64_t current_lsn() const;

private:
    void open_segment_at(uint64_t segno, uint64_t offset);
    void flush_buffer();

private:
    HANDLE wal_handle = INVALID_HANDLE_VALUE;
    std::wstring wal_folder;

    WalPage current_page;

    std::vector<uint8_t> wal_write_buffer;
    size_t buffered_bytes = 0;

    uint64_t durable_lsn = 0;
    uint64_t open_segno = UINT64_MAX;
};