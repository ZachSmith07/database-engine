#pragma once
#include <cstddef>
#include <cstdint>

namespace wal
{
    inline constexpr std::size_t PAGE_SIZE = 8192;
    inline constexpr std::size_t SEGMENT_SIZE = 4 * 1024 * 1024;
    inline constexpr std::uint16_t PAGE_HEADER_SIZE = 24;
    inline constexpr std::size_t CRC_HEADER_INDEX = 20;
    inline constexpr std::uint32_t MAGIC = 0xDABA1234;

    inline constexpr std::uint8_t INSERT_HEAP_TYPE = 0;
    inline constexpr std::uint8_t CREATE_HEAP_TYPE = 1;
}

namespace heap
{
    inline constexpr std::size_t HEADER_SIZE = 13;
    inline constexpr std::size_t SLOT_TUPLE_SIZE = 4;
    inline constexpr std::size_t WAL_INSERT_HEADER_SIZE = 21;

}

inline constexpr std::size_t PAGE_SIZE = 8192;