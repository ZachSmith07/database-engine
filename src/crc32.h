#include <cstdint>
#include <cstddef>

// wal checksum code

static uint32_t crc32c_table[256];
static bool crc32c_table_init = false;

static void crc32c_init_table()
{
    // CRC32C polynomial
    constexpr uint32_t POLY = 0x82F63B78u;

    for (uint32_t i = 0; i < 256; i++) {
        uint32_t crc = i;
        for (int k = 0; k < 8; k++) {
            crc = (crc >> 1) ^ (POLY & (-(int32_t)(crc & 1)));
        }
        crc32c_table[i] = crc;
    }
    crc32c_table_init = true;
}

uint32_t crc32c(const void* data, size_t len, uint32_t crc = 0xFFFFFFFFu)
{
    if (!crc32c_table_init) crc32c_init_table();

    const uint8_t* p = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < len; i++) {
        crc = (crc >> 8) ^ crc32c_table[(crc ^ p[i]) & 0xFFu];
    }
    return ~crc;
}