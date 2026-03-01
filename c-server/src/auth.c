#include "auth.h"
#include "crypto.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>


// SHA256 비밀번호 해싱
void hash_password(const char *password, const char *secret,
                   char *out_hex) {
    uint8_t hash[32];
    SHA256_CTX ctx;

    sha256_init(&ctx);
    sha256_update(&ctx, (uint8_t*)password, strlen(password));
    sha256_update(&ctx, (uint8_t*)secret, strlen(secret));
    sha256_final(&ctx, hash);

    // Hex 변환
    for (int i = 0; i < 32; i++) {
        sprintf(out_hex + (i * 2), "%02x", hash[i]);
    }
    out_hex[64] = '\0';
}

// HMAC-SHA256 서명
int jwt_sign(const char *message, const char *secret,
             unsigned char *out_sig, unsigned int *out_len) {
    hmac_sha256((uint8_t*)secret, strlen(secret),
                (uint8_t*)message, strlen(message),
                out_sig);
    *out_len = 32;  // SHA256 출력은 항상 32바이트
    return 0;
}

// JWT 생성
int jwt_create(const char *username, const char *role, const char *secret,
               int expiry_seconds, char *out_token, size_t out_size) {
    char header_json[128];
    char payload_json[512];
    char header_b64[256];
    char payload_b64[512];
    char message[1024];
    unsigned char signature[32];
    unsigned int sig_len;
    char signature_b64[64];

    // 헤더 (JSON)
    snprintf(header_json, sizeof(header_json),
             "{\"alg\":\"HS256\",\"typ\":\"JWT\"}");

    // 페이로드 (JSON)
    time_t now = time(NULL);
    time_t exp = now + expiry_seconds;

    snprintf(payload_json, sizeof(payload_json),
             "{\"username\":\"%s\",\"role\":\"%s\",\"iat\":%ld,\"exp\":%ld}",
             username, role, now, exp);

    // Base64url 인코딩
    base64url_encode((uint8_t*)header_json, strlen(header_json),
                     header_b64, sizeof(header_b64));
    base64url_encode((uint8_t*)payload_json, strlen(payload_json),
                     payload_b64, sizeof(payload_b64));

    // 메시지 구성 (header.payload)
    snprintf(message, sizeof(message), "%s.%s", header_b64, payload_b64);

    // 서명
    if (jwt_sign(message, secret, signature, &sig_len) < 0) {
        return -1;
    }

    // 서명을 Base64url로 인코딩
    base64url_encode(signature, sig_len, signature_b64, sizeof(signature_b64));

    // 최종 JWT
    snprintf(out_token, out_size, "%s.%s", message, signature_b64);

    return 0;
}

// JWT 검증 및 파싱
int jwt_verify(const char *token, const char *secret, JwtPayload *out_payload) {
    // 토큰 형식: header.payload.signature

    // 첫 번째 점 위치 찾기
    char *first_dot = strchr(token, '.');
    if (!first_dot) {
        return -1;
    }

    // 두 번째 점 위치 찾기
    char *second_dot = strchr(first_dot + 1, '.');
    if (!second_dot) {
        return -1;
    }

    // header.payload 부분
    int msg_len = second_dot - token;
    char message[1024];
    strncpy(message, token, msg_len);
    message[msg_len] = '\0';

    // 서명 부분
    const char *signature_b64 = second_dot + 1;

    // 서명 검증
    unsigned char expected_sig[32];
    unsigned int expected_sig_len;

    if (jwt_sign(message, secret, expected_sig, &expected_sig_len) < 0) {
        return -1;
    }

    // 제공된 서명을 디코딩
    uint8_t provided_sig[32];
    size_t provided_sig_len;

    if (base64url_decode(signature_b64, provided_sig, &provided_sig_len) < 0) {
        return -1;
    }

    // 서명 비교 (타이밍 공격 방지용)
    if (expected_sig_len != provided_sig_len) {
        return -1;
    }

    int sig_match = 0;
    for (unsigned int i = 0; i < expected_sig_len; i++) {
        sig_match |= expected_sig[i] ^ provided_sig[i];
    }

    if (sig_match != 0) {
        return -1;  // 서명 불일치
    }

    // 페이로드 파싱
    char *payload_start = first_dot + 1;
    int payload_len = second_dot - payload_start;

    uint8_t payload_raw[512];
    size_t payload_raw_len;

    char payload_b64[256];
    strncpy(payload_b64, payload_start, payload_len);
    payload_b64[payload_len] = '\0';

    if (base64url_decode(payload_b64, payload_raw, &payload_raw_len) < 0) {
        return -1;
    }

    char payload_json[512];
    strncpy(payload_json, (const char*)payload_raw, payload_raw_len);
    payload_json[payload_raw_len] = '\0';

    // JSON 파싱 (간단한 버전)
    // {"username":"...", "role":"...", "iat":..., "exp":...}

    char *username_start = strstr(payload_json, "\"username\":\"");
    if (!username_start) {
        return -1;
    }
    username_start += 12;
    char *username_end = strchr(username_start, '"');
    if (!username_end) {
        return -1;
    }

    strncpy(out_payload->username, username_start,
            (size_t)(username_end - username_start));
    out_payload->username[(size_t)(username_end - username_start)] = '\0';

    char *role_start = strstr(payload_json, "\"role\":\"");
    if (!role_start) {
        return -1;
    }
    role_start += 8;
    char *role_end = strchr(role_start, '"');
    if (!role_end) {
        return -1;
    }

    strncpy(out_payload->role, role_start,
            (size_t)(role_end - role_start));
    out_payload->role[(size_t)(role_end - role_start)] = '\0';

    // exp 파싱
    char *exp_start = strstr(payload_json, "\"exp\":");
    if (!exp_start) {
        return -1;
    }
    out_payload->exp = (time_t)atol(exp_start + 6);

    // 만료 확인
    time_t now = time(NULL);
    if (now > out_payload->exp) {
        return -1;  // 토큰 만료됨
    }

    return 0;
}
