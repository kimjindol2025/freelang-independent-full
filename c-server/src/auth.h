#ifndef AUTH_H
#define AUTH_H

#include <time.h>

#define JWT_MAX_TOKEN_LEN 1024
#define JWT_MAX_PAYLOAD_LEN 512

// JWT 페이로드
typedef struct {
    char username[64];
    char role[32];
    time_t exp;           // 만료 시간 (Unix timestamp)
    time_t iat;           // 발급 시간
} JwtPayload;

// 비밀번호 해싱 (SHA256)
void hash_password(const char *password, const char *secret,
                   char *out_hex);

// JWT 서명 (HMAC-SHA256)
int jwt_sign(const char *message, const char *secret,
             unsigned char *out_sig, unsigned int *out_len);

// JWT 생성
int jwt_create(const char *username, const char *role, const char *secret,
               int expiry_seconds, char *out_token, size_t out_size);

// JWT 검증 및 파싱
int jwt_verify(const char *token, const char *secret, JwtPayload *out_payload);

// Base64url 인코딩
int base64url_encode(const unsigned char *in, size_t in_len,
                     char *out, size_t out_size);

// Base64url 디코딩
int base64url_decode(const char *in, unsigned char *out, size_t *out_len);

#endif
