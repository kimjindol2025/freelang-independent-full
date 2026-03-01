/**
 * MyOS_Lib CRC32 Implementation (crc32.h)
 *
 * IEEE 802.3 CRC32 체크섬
 * - Polynomial: 0x04C11DB7
 * - 초기값: 0xFFFFFFFF
 * - 최종 XOR: 0xFFFFFFFF
 * - 반사: 입력/출력 모두 반사
 */

#ifndef MYOS_CRC32_H
#define MYOS_CRC32_H

#include <stddef.h>
#include <stdint.h>

/* ============================================================================
 * CRC32 계산
 * ============================================================================ */

/**
 * 데이터의 CRC32 체크섬 계산
 * @param data 입력 데이터
 * @param len 데이터 길이 (바이트)
 * @return CRC32 값 (32비트)
 */
uint32_t crc32_calculate(const uint8_t *data, size_t len);

/**
 * 누적 CRC32 계산 (스트리밍)
 * @param crc 이전 CRC 값 (첫 호출 시 0xFFFFFFFF)
 * @param data 입력 데이터
 * @param len 데이터 길이
 * @return 업데이트된 CRC
 */
uint32_t crc32_update(uint32_t crc, const uint8_t *data, size_t len);

/**
 * 최종 CRC32 값 생성
 * @param crc 누적 CRC 값
 * @return 최종 CRC32 (XOR 0xFFFFFFFF 적용됨)
 */
uint32_t crc32_finalize(uint32_t crc);

#endif /* MYOS_CRC32_H */
