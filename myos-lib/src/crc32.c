/**
 * MyOS_Lib CRC32 Implementation (crc32.c)
 *
 * IEEE 802.3 CRC32 (reflected)
 * Polynomial: 0xEDB88320 (reflected)
 */

#include "crc32.h"
#include <stdint.h>

/* ============================================================================
 * CRC32 테이블 생성 (런타임)
 * ============================================================================ */

/**
 * CRC32 테이블 항목 계산
 */
static uint32_t crc32_table_entry(int index) {
    uint32_t crc = (uint32_t)index;

    for (int i = 0; i < 8; i++) {
        if (crc & 1) {
            crc = (crc >> 1) ^ 0xEDB88320;  /* Reflected polynomial */
        } else {
            crc = crc >> 1;
        }
    }

    return crc;
}

/* ============================================================================
 * CRC32 계산
 * ============================================================================ */

uint32_t crc32_calculate(const uint8_t *data, size_t len) {
    uint32_t crc = 0xFFFFFFFF;

    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        int index = (int)((crc ^ byte) & 0xFF);
        uint32_t table_val = crc32_table_entry(index);
        crc = (crc >> 8) ^ table_val;
    }

    return crc ^ 0xFFFFFFFF;
}

uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        uint8_t byte = data[i];
        int index = (int)((crc ^ byte) & 0xFF);
        uint32_t table_val = crc32_table_entry(index);
        crc = (crc >> 8) ^ table_val;
    }

    return crc;
}

uint32_t crc32_finalize(uint32_t crc) {
    return crc ^ 0xFFFFFFFF;
}
