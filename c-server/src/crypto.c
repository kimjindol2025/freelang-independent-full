#include "crypto.h"
#include <string.h>

/* SHA256 상수 */
static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

static uint32_t rotr(uint32_t x, int n) {
    return (x >> n) | (x << (32 - n));
}

static uint32_t ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

static uint32_t maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

static uint32_t sum0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

static uint32_t sum1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

static uint32_t gamma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

static uint32_t gamma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

void sha256_init(SHA256_CTX *ctx) {
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
}

static void sha256_transform(SHA256_CTX *ctx, const uint8_t *data) {
    uint32_t W[64];
    uint32_t a, b, c, d, e, f, g, h;
    int i;

    for (i = 0; i < 16; i++) {
        W[i] = ((uint32_t)data[i*4] << 24) | ((uint32_t)data[i*4+1] << 16) |
               ((uint32_t)data[i*4+2] << 8) | (uint32_t)data[i*4+3];
    }

    for (i = 16; i < 64; i++) {
        W[i] = gamma1(W[i-2]) + W[i-7] + gamma0(W[i-15]) + W[i-16];
    }

    a = ctx->state[0];
    b = ctx->state[1];
    c = ctx->state[2];
    d = ctx->state[3];
    e = ctx->state[4];
    f = ctx->state[5];
    g = ctx->state[6];
    h = ctx->state[7];

    for (i = 0; i < 64; i++) {
        uint32_t t1 = h + sum1(e) + ch(e, f, g) + K[i] + W[i];
        uint32_t t2 = sum0(a) + maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    ctx->state[0] += a;
    ctx->state[1] += b;
    ctx->state[2] += c;
    ctx->state[3] += d;
    ctx->state[4] += e;
    ctx->state[5] += f;
    ctx->state[6] += g;
    ctx->state[7] += h;
}

void sha256_update(SHA256_CTX *ctx, const uint8_t *data, size_t len) {
    size_t i = 0;
    size_t idx = (ctx->count / 8) % 64;

    ctx->count += len * 8;

    while (i < len) {
        ctx->buffer[idx] = data[i];
        idx = (idx + 1) % 64;
        i++;

        if (idx == 0) {
            sha256_transform(ctx, ctx->buffer);
        }
    }
}

void sha256_final(SHA256_CTX *ctx, uint8_t *digest) {
    uint8_t padding[64];
    uint64_t bits = ctx->count;
    size_t idx = (ctx->count / 8) % 64;
    size_t pad_len = (idx < 56) ? (56 - idx) : (120 - idx);

    padding[0] = 0x80;
    for (size_t i = 1; i < pad_len; i++) {
        padding[i] = 0;
    }

    for (int i = 0; i < 8; i++) {
        padding[pad_len + i] = (uint8_t)(bits >> (56 - i*8));
    }

    sha256_update(ctx, padding, pad_len + 8);

    for (int i = 0; i < 8; i++) {
        digest[i*4]   = (uint8_t)(ctx->state[i] >> 24);
        digest[i*4+1] = (uint8_t)(ctx->state[i] >> 16);
        digest[i*4+2] = (uint8_t)(ctx->state[i] >> 8);
        digest[i*4+3] = (uint8_t)(ctx->state[i]);
    }
}

void sha256(const uint8_t *data, size_t len, uint8_t *digest) {
    SHA256_CTX ctx;
    sha256_init(&ctx);
    sha256_update(&ctx, data, len);
    sha256_final(&ctx, digest);
}

void hmac_sha256(const uint8_t *key, size_t key_len,
                 const uint8_t *msg, size_t msg_len,
                 uint8_t *digest) {
    uint8_t ipad[64], opad[64];
    uint8_t key_hash[32];
    SHA256_CTX ctx;

    /* 키 정규화 */
    if (key_len > 64) {
        sha256(key, key_len, key_hash);
        key = key_hash;
        key_len = 32;
    }

    /* Pad 생성 */
    for (int i = 0; i < 64; i++) {
        ipad[i] = (i < key_len) ? key[i] : 0;
        opad[i] = (i < key_len) ? key[i] : 0;
        ipad[i] ^= 0x36;
        opad[i] ^= 0x5c;
    }

    /* 내부 해시 */
    sha256_init(&ctx);
    sha256_update(&ctx, ipad, 64);
    sha256_update(&ctx, msg, msg_len);
    sha256_final(&ctx, digest);

    /* 외부 해시 */
    sha256_init(&ctx);
    sha256_update(&ctx, opad, 64);
    sha256_update(&ctx, digest, 32);
    sha256_final(&ctx, digest);
}

/* Base64url 문자 테이블 */
static const char base64url_table[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";

int base64url_encode(const uint8_t *in, size_t in_len, char *out, size_t out_size) {
    size_t out_len = 0;

    for (size_t i = 0; i < in_len; i += 3) {
        uint32_t b = (in[i] << 16);
        if (i + 1 < in_len) b |= (in[i + 1] << 8);
        if (i + 2 < in_len) b |= in[i + 2];

        int pad = (3 - (in_len - i));
        if (pad > 2) pad = 2;

        if (out_len + 4 > out_size) return -1;

        out[out_len++] = base64url_table[(b >> 18) & 0x3f];
        out[out_len++] = base64url_table[(b >> 12) & 0x3f];
        out[out_len++] = (pad >= 2) ? '=' : base64url_table[(b >> 6) & 0x3f];
        out[out_len++] = (pad >= 1) ? '=' : base64url_table[b & 0x3f];
    }

    if (out_len >= out_size) return -1;
    out[out_len] = '\0';
    return (int)out_len;
}

int base64url_decode(const char *in, uint8_t *out, size_t *out_len) {
    *out_len = 0;
    size_t in_len = 0;
    while (in[in_len] && in[in_len] != '=') in_len++;

    for (size_t i = 0; i < in_len; i += 4) {
        uint32_t b = 0;
        for (int j = 0; j < 4; j++) {
            char c = in[i + j];
            int v = -1;
            if (c >= 'A' && c <= 'Z') v = c - 'A';
            else if (c >= 'a' && c <= 'z') v = c - 'a' + 26;
            else if (c >= '0' && c <= '9') v = c - '0' + 52;
            else if (c == '-') v = 62;
            else if (c == '_') v = 63;
            else break;
            b = (b << 6) | v;
        }

        if (*out_len < 3) out[(*out_len)++] = (uint8_t)(b >> 16);
        if (*out_len < 3) out[(*out_len)++] = (uint8_t)(b >> 8);
        if (*out_len < 3) out[(*out_len)++] = (uint8_t)b;
    }

    return 0;
}
