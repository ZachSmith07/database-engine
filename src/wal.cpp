#include "wal.h"
#include <iostream>
#include <cstdio>
#include <cinttypes>
#include "crc32.h"
#include <iomanip>
#include <sstream>

static void wal_log(const char *fmt, ...)
{
    char buf[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);

    OutputDebugStringA(buf);
    
    std::fprintf(stderr, "%s", buf);
    std::fflush(stderr);
}


static void wal_log_last_error(const char *where)
{
    DWORD e = GetLastError();
    wal_log("[WAL][ERR] %s failed. GetLastError=%lu\n", where, (unsigned long)e);
}

void walpage_set_checksum(WalPage &page)
{
    constexpr size_t crc_index = wal::CRC_HEADER_INDEX;
    constexpr size_t crc_size = sizeof(uint32_t);

    std::memset(page.data + crc_index, 0, crc_size);

    if (page.write_pos < sizeof(page.data))
    {
        std::memset(page.data + page.write_pos,
                    0,
                    sizeof(page.data) - page.write_pos);
    }

    uint32_t crc = crc32c(page.data, sizeof(page.data));

    std::memcpy(page.data + crc_index, &crc, crc_size);
}

std::wstring make_segment_filename(uint64_t lsn)
{
    uint64_t segment_start =
        (lsn / wal::SEGMENT_SIZE) * wal::SEGMENT_SIZE;

    std::wstringstream ss;
    ss << std::uppercase
       << std::setfill(L'0')
       << std::setw(16)
       << std::hex
       << segment_start;

    return ss.str() + L".wal";
}

void push_u64(std::vector<uint8_t> &bytes, uint64_t v)
{
    for (int i = 0; i < 8; i++)
    {
        bytes.push_back(static_cast<uint8_t>((v >> (8 * i)) & 0xFF));
    }
}

void push_u32(std::vector<uint8_t> &bytes, uint32_t v)
{
    bytes.push_back(static_cast<uint8_t>(v & 0xFF));
    bytes.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((v >> 16) & 0xFF));
    bytes.push_back(static_cast<uint8_t>((v >> 24) & 0xFF));
}

void push_u16(std::vector<uint8_t> &bytes, uint16_t v)
{
    bytes.push_back(static_cast<uint8_t>(v & 0xFF));
    bytes.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

inline bool walpage_put_bytes(WalPage &page, const void *src, uint16_t len)
{
    if (page.write_pos + len > sizeof(page.data))
        return false;

    std::memcpy(page.data + page.write_pos, src, len);
    page.write_pos += len;
    return true;
}

inline bool walpage_put_u16(WalPage &page, uint16_t value)
{
    return walpage_put_bytes(page, &value, sizeof(value));
}

inline bool walpage_put_u32(WalPage &page, uint32_t value)
{
    return walpage_put_bytes(page, &value, sizeof(value));
}

inline bool walpage_put_u64(WalPage &page, uint64_t value)
{
    return walpage_put_bytes(page, &value, sizeof(value));
}

static inline uint16_t slot_offset(uint32_t v)
{
    return (v >> 17) & 0x7FFF;
}

static inline uint16_t slot_length(uint32_t v)
{
    return (v >> 2) & 0x7FFF;
}

std::vector<uint8_t> get_heap_page_insert_wal(
    const PageId &page_id,
    const HeapPage &page)
{
    std::vector<uint8_t> bytes;
    bytes.reserve(8192);

    uint16_t slot_count = page.get_slot_count();
    uint16_t tuple_count = slot_count - page.wal_start_slot;

    uint8_t *page_data = page.data();

    uint32_t payload_len = 0;

    payload_len += sizeof(uint32_t); // relation
    payload_len += sizeof(uint32_t); // page
    payload_len += sizeof(uint16_t); // wal_start_slot
    payload_len += sizeof(uint16_t); // tuple_count

    payload_len += (slot_count - page.wal_start_slot) * heap::SLOT_TUPLE_SIZE;

    for (int i = page.wal_start_slot; i < slot_count; i++)
    {
        uint32_t v = *reinterpret_cast<uint32_t *>(
            page_data + heap::HEADER_SIZE + i * heap::SLOT_TUPLE_SIZE);

        uint16_t inner_length = slot_length(v);
        payload_len += inner_length;
    }

    uint32_t total_len =
        sizeof(uint8_t) +  // type
        sizeof(uint32_t) + // total_len
        sizeof(uint32_t) + // crc
        payload_len;

    uint8_t type = wal::INSERT_HEAP_TYPE;

    bytes.push_back(type);
    push_u32(bytes, total_len);

    // space reserved for checksum
    size_t crc_offset = bytes.size();
    push_u32(bytes, 0); // placeholder

    push_u32(bytes, page_id.relation);
    push_u32(bytes, page_id.page);
    push_u16(bytes, page.wal_start_slot);
    push_u16(bytes, tuple_count);

    bytes.insert(
        bytes.end(),
        page_data + heap::HEADER_SIZE + page.wal_start_slot * heap::SLOT_TUPLE_SIZE,
        page_data + heap::HEADER_SIZE + slot_count * heap::SLOT_TUPLE_SIZE);

    for (int i = page.wal_start_slot; i < slot_count; i++)
    {
        uint32_t v = *reinterpret_cast<uint32_t *>(
            page_data + heap::HEADER_SIZE + i * heap::SLOT_TUPLE_SIZE);

        uint16_t offset = slot_offset(v);
        uint16_t tuple_length = slot_length(v);

        bytes.insert(
            bytes.end(),
            page_data + offset,
            page_data + offset + tuple_length);
    }

    const uint8_t *crc_start = bytes.data();
    size_t crc_len = bytes.size();

    uint32_t crc = 0xFFFFFFFF;

    crc = crc32c(bytes.data(), 1, crc);

    crc = crc32c(bytes.data() + sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t),
                 bytes.size() - (sizeof(uint8_t) + sizeof(uint32_t) + sizeof(uint32_t)),
                 crc);

    crc = ~crc;

    // CRC inserted to WAL
    std::memcpy(bytes.data() + crc_offset, &crc, sizeof(uint32_t));

    return bytes;
}

std::vector<uint8_t> get_heap_page_create_wal(
    const PageId &page_id,
    const HeapPage &page)
{
    std::vector<uint8_t> bytes;

    bytes.reserve(6);
    bytes.push_back(static_cast<uint8_t>(wal::CREATE_HEAP_TYPE));

    const uint8_t *p = page.data();

    bytes.insert(
        bytes.end(),
        p + 8,
        p + 13);

    return bytes;
}

WalManager::WalManager(const wchar_t* folder)
    : wal_folder(folder)
{
    // folder check
    if (!wal_folder.empty()) {
        wchar_t last = wal_folder.back();
        if (last != L'/' && last != L'\\')
            wal_folder.push_back(L'\\');
    }

    wal_write_buffer.reserve(WAL_FLUSH_THRESHOLD + wal::PAGE_SIZE);
    current_page.reset();

    durable_lsn = 0;

    // open first segment
    open_segment_at(0, 0);
}

void WalManager::open_segment_at(uint64_t segno, uint64_t offset)
{
    if (wal_handle != INVALID_HANDLE_VALUE) {
        CloseHandle(wal_handle);
        wal_handle = INVALID_HANDLE_VALUE;
    }

    // filename = <segment_start_lsn in hex>.wal
    // segment_start_lsn = segno * SEGMENT_SIZE
    uint64_t segment_start_lsn = segno * wal::SEGMENT_SIZE;

    std::wstringstream ss;
    ss << std::uppercase
       << std::setfill(L'0')
       << std::setw(16)
       << std::hex
       << segment_start_lsn;

    std::wstring filename = wal_folder + ss.str() + L".wal";

    std::wcout << filename << std::endl;

    wal_handle = CreateFileW(
        filename.c_str(),
        GENERIC_WRITE,
        FILE_SHARE_READ,
        nullptr,
        OPEN_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        nullptr
    );

    if (wal_handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error("Failed to open WAL segment");

    // seek to exact offset within this segment (NOT FILE_END)
    LARGE_INTEGER li;
    li.QuadPart = static_cast<LONGLONG>(offset);

    if (!SetFilePointerEx(wal_handle, li, nullptr, FILE_BEGIN))
        throw std::runtime_error("Failed to seek WAL segment");

    open_segno = segno;
}

uint64_t WalManager::current_lsn() const
{
    return durable_lsn + buffered_bytes + current_page.write_pos;
}

void WalManager::append(const void* src, size_t len)
{
    if (!src && len > 0)
        throw std::runtime_error("append nullptr");

    const uint8_t* p = static_cast<const uint8_t*>(src);

    while (len > 0)
    {
        size_t space = wal::PAGE_SIZE - current_page.write_pos;
        size_t to_copy = std::min(space, len);

        std::memcpy(current_page.data + current_page.write_pos, p, to_copy);
        current_page.write_pos += static_cast<uint16_t>(to_copy);

        p += to_copy;
        len -= to_copy;

        if (current_page.write_pos == wal::PAGE_SIZE)
        {
            wal_write_buffer.insert(
                wal_write_buffer.end(),
                current_page.data,
                current_page.data + wal::PAGE_SIZE
            );

            buffered_bytes += wal::PAGE_SIZE;
            current_page.reset();

            if (buffered_bytes >= WAL_FLUSH_THRESHOLD)
                flush_buffer();
        }
    }
}

void WalManager::flush() // OLD FLUSH (FOR PAGED WAL)
{
    if (current_page.write_pos > 0)
    {
        wal_write_buffer.insert(
            wal_write_buffer.end(),
            current_page.data,
            current_page.data + current_page.write_pos
        );

        buffered_bytes += current_page.write_pos;
        current_page.write_pos = 0;
    }

    flush_buffer();
}


void WalManager::flush_buffer()
{
    if (buffered_bytes == 0)
        return;

    size_t buf_off = 0;

    while (buf_off < buffered_bytes)
    {
        uint64_t segno  = durable_lsn / wal::SEGMENT_SIZE;
        uint64_t segoff = durable_lsn % wal::SEGMENT_SIZE;

        if (open_segno != segno)
            open_segment_at(segno, segoff);
        else {
            LARGE_INTEGER li;
            li.QuadPart = static_cast<LONGLONG>(segoff);
            if (!SetFilePointerEx(wal_handle, li, nullptr, FILE_BEGIN))
                throw std::runtime_error("Failed to find WAL segment");
        }

        uint64_t space_in_seg = wal::SEGMENT_SIZE - segoff;
        size_t remaining = buffered_bytes - buf_off;
        size_t chunk = static_cast<size_t>(std::min<uint64_t>(space_in_seg, remaining));

        DWORD written = 0;
        BOOL ok = WriteFile(
            wal_handle,
            wal_write_buffer.data() + buf_off,
            static_cast<DWORD>(chunk),
            &written,
            nullptr
        );

        if (!ok || written != chunk)
            throw std::runtime_error("WAL write failed");

        durable_lsn += written;
        buf_off += written;
    }

    if (!FlushFileBuffers(wal_handle))
        throw std::runtime_error("WAL fsync failed");

    wal_write_buffer.clear();
    buffered_bytes = 0;
}