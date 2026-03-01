#ifndef CRYPTO_H
#define CRYPTO_H

#include <stddef.h>
#include <stdint.h>

/* SHA256 컨텍스트 */
typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
} SHA256_CTX;

/* SHA256 초기화 */
void sha256_init(SHA256_CTX *ctx);

/* SHA256 업데이트 */
void sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len);

/* SHA256 최종화 */
void sha256_final(SHA256_CTX *ctx, uint8_t *digest);

/* 한 번에 SHA256 계산 */
void sha256(const uint8_t *data, size_t len, uint8_t *digest);

/* HMAC-SHA256 */
void hmac_sha256(const uint8_t *key, size_t key_len,
                 const uint8_t *msg, size_t msg_len,
                 uint8_t *digest);

/* Base64url 인코딩 */
int base64url_encode(const uint8_t *in, size_t in_len, char *out, size_t out_size);

/* Base64url 디코딩 */
int base64url_decode(const char *in, uint8_t *out, size_t *out_len);

#endif
