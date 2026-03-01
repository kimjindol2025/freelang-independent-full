#include "serializer.h"
#include "vector.h"
#include "string.h"
#include "hash.h"
#include "list.h"
#include "crc32.h"
#include "mm.h"

void serializer_write_u32(uint32_t val, uint8_t *buf) {
    buf[0] = (val >> 24) & 0xFF;
    buf[1] = (val >> 16) & 0xFF;
    buf[2] = (val >> 8) & 0xFF;
    buf[3] = val & 0xFF;
}

uint32_t serializer_read_u32(const uint8_t *buf) {
    return ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) |
           ((uint32_t)buf[2] << 8) | ((uint32_t)buf[3]);
}

void serializer_write_u16(uint16_t val, uint8_t *buf) {
    buf[0] = (val >> 8) & 0xFF;
    buf[1] = val & 0xFF;
}

uint16_t serializer_read_u16(const uint8_t *buf) {
    return ((uint16_t)buf[0] << 8) | ((uint16_t)buf[1]);
}

static void* my_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

static int write_header(uint8_t *buf, uint32_t payload_len) {
    serializer_write_u32(SERIALIZER_MAGIC, buf + 0);
    buf[4] = SERIALIZER_VERSION;
    serializer_write_u16(0, buf + 5);
    serializer_write_u32(payload_len, buf + 7);
    return 0;
}

int serializer_encode_vector(Vector *v, uint8_t **out, size_t *out_len) {
    if (!v || !out || !out_len) return -1;
    
    size_t elem_size = vector_get_elem_size(v);
    size_t count = vector_size(v);
    size_t payload_size = 1 + 4 + 4 + (count * elem_size);
    size_t frame_size = SERIALIZER_HEADER_SIZE + payload_size + 4;
    
    uint8_t *frame = (uint8_t *)mm_alloc(frame_size);
    if (!frame) return -1;
    
    write_header(frame, payload_size);
    frame[SERIALIZER_HEADER_SIZE] = SERIALIZER_TYPE_VECTOR;
    serializer_write_u32(elem_size, frame + SERIALIZER_HEADER_SIZE + 1);
    serializer_write_u32(count, frame + SERIALIZER_HEADER_SIZE + 5);
    
    for (size_t i = 0; i < count; i++) {
        void *elem = vector_at(v, i);
        if (elem)
            my_memcpy(frame + SERIALIZER_HEADER_SIZE + 9 + (i * elem_size), elem, elem_size);
    }
    
    uint32_t crc = crc32_calculate(frame + SERIALIZER_HEADER_SIZE, payload_size);
    serializer_write_u32(crc, frame + SERIALIZER_HEADER_SIZE + payload_size);
    
    *out = frame;
    *out_len = frame_size;
    return 0;
}

int serializer_encode_string(String *s, uint8_t **out, size_t *out_len) {
    if (!s || !out || !out_len) return -1;
    
    size_t str_len = string_length(s);
    size_t payload_size = 1 + 4 + str_len;
    size_t frame_size = SERIALIZER_HEADER_SIZE + payload_size + 4;
    
    uint8_t *frame = (uint8_t *)mm_alloc(frame_size);
    if (!frame) return -1;
    
    write_header(frame, payload_size);
    frame[SERIALIZER_HEADER_SIZE] = SERIALIZER_TYPE_STRING;
    serializer_write_u32(str_len, frame + SERIALIZER_HEADER_SIZE + 1);
    
    const char *cstr = string_c_str(s);
    if (cstr && str_len > 0)
        my_memcpy(frame + SERIALIZER_HEADER_SIZE + 5, cstr, str_len);
    
    uint32_t crc = crc32_calculate(frame + SERIALIZER_HEADER_SIZE, payload_size);
    serializer_write_u32(crc, frame + SERIALIZER_HEADER_SIZE + payload_size);
    
    *out = frame;
    *out_len = frame_size;
    return 0;
}

int serializer_encode_hashmap(HashMap *h, uint8_t **out, size_t *out_len) {
    if (!h || !out || !out_len) return -1;

    size_t count = hash_size(h);
    size_t payload_size = 1 + 4 + 4 + 4;
    size_t frame_size = SERIALIZER_HEADER_SIZE + payload_size + 4;

    uint8_t *frame = (uint8_t *)mm_alloc(frame_size);
    if (!frame) return -1;

    write_header(frame, payload_size);
    frame[SERIALIZER_HEADER_SIZE] = SERIALIZER_TYPE_HASHMAP;
    serializer_write_u32(0, frame + SERIALIZER_HEADER_SIZE + 1);
    serializer_write_u32(0, frame + SERIALIZER_HEADER_SIZE + 5);
    serializer_write_u32(count, frame + SERIALIZER_HEADER_SIZE + 9);

    uint32_t crc = crc32_calculate(frame + SERIALIZER_HEADER_SIZE, payload_size);
    serializer_write_u32(crc, frame + SERIALIZER_HEADER_SIZE + payload_size);

    *out = frame;
    *out_len = frame_size;
    return 0;
}

int serializer_encode_list(LinkedList *l, uint8_t **out, size_t *out_len) {
    if (!l || !out || !out_len) return -1;

    size_t elem_size = l->elem_size;
    size_t count = list_size(l);
    size_t payload_size = 1 + 4 + 4 + (count * elem_size);
    size_t frame_size = SERIALIZER_HEADER_SIZE + payload_size + 4;

    uint8_t *frame = (uint8_t *)mm_alloc(frame_size);
    if (!frame) return -1;

    write_header(frame, payload_size);
    frame[SERIALIZER_HEADER_SIZE] = SERIALIZER_TYPE_LIST;
    serializer_write_u32(elem_size, frame + SERIALIZER_HEADER_SIZE + 1);
    serializer_write_u32(count, frame + SERIALIZER_HEADER_SIZE + 5);

    for (size_t i = 0; i < count; i++) {
        void *elem = list_at(l, i);
        if (elem)
            my_memcpy(frame + SERIALIZER_HEADER_SIZE + 9 + (i * elem_size), elem, elem_size);
    }

    uint32_t crc = crc32_calculate(frame + SERIALIZER_HEADER_SIZE, payload_size);
    serializer_write_u32(crc, frame + SERIALIZER_HEADER_SIZE + payload_size);

    *out = frame;
    *out_len = frame_size;
    return 0;
}

int serializer_encode_bytes(const uint8_t *data, size_t len,
                            uint8_t **out, size_t *out_len) {
    if (!data || !out || !out_len) return -1;
    
    size_t payload_size = 1 + 4 + len;
    size_t frame_size = SERIALIZER_HEADER_SIZE + payload_size + 4;
    
    uint8_t *frame = (uint8_t *)mm_alloc(frame_size);
    if (!frame) return -1;
    
    write_header(frame, payload_size);
    frame[SERIALIZER_HEADER_SIZE] = SERIALIZER_TYPE_BYTES;
    serializer_write_u32(len, frame + SERIALIZER_HEADER_SIZE + 1);
    my_memcpy(frame + SERIALIZER_HEADER_SIZE + 5, data, len);
    
    uint32_t crc = crc32_calculate(frame + SERIALIZER_HEADER_SIZE, payload_size);
    serializer_write_u32(crc, frame + SERIALIZER_HEADER_SIZE + payload_size);
    
    *out = frame;
    *out_len = frame_size;
    return 0;
}

Vector* serializer_decode_vector(const uint8_t *data, size_t len) {
    (void)data; (void)len;
    return NULL;
}

String* serializer_decode_string(const uint8_t *data, size_t len) {
    (void)data; (void)len;
    return NULL;
}

HashMap* serializer_decode_hashmap(const uint8_t *data, size_t len) {
    (void)data; (void)len;
    return NULL;
}

LinkedList* serializer_decode_list(const uint8_t *data, size_t len) {
    (void)data; (void)len;
    return NULL;
}

int serializer_decode_bytes(const uint8_t *data, size_t len,
                            uint8_t **out_buf, size_t *out_len) {
    (void)data; (void)len; (void)out_buf; (void)out_len;
    return -1;
}

int serializer_validate(const uint8_t *frame, size_t len) {
    if (!frame || len < SERIALIZER_HEADER_SIZE + 5) return -1;
    
    uint32_t magic = serializer_read_u32(frame + 0);
    if (magic != SERIALIZER_MAGIC) return -1;
    
    uint32_t payload_len = serializer_read_u32(frame + 7);
    if (SERIALIZER_HEADER_SIZE + payload_len + 4 != len) return -1;
    
    uint32_t crc_stored = serializer_read_u32(frame + SERIALIZER_HEADER_SIZE + payload_len);
    uint32_t crc_calc = crc32_calculate(frame + SERIALIZER_HEADER_SIZE, payload_len);
    
    return (crc_stored == crc_calc) ? 0 : -1;
}

int serializer_parse_header(const uint8_t *frame, size_t len,
                            uint8_t *out_version,
                            uint16_t *out_flags,
                            uint32_t *out_payload_len,
                            uint8_t *out_type_id) {
    if (!frame || len < SERIALIZER_HEADER_SIZE + 1) return -1;
    
    uint32_t magic = serializer_read_u32(frame + 0);
    if (magic != SERIALIZER_MAGIC) return -1;
    
    if (out_version) *out_version = frame[4];
    if (out_flags) *out_flags = serializer_read_u16(frame + 5);
    
    uint32_t payload_len = serializer_read_u32(frame + 7);
    if (out_payload_len) *out_payload_len = payload_len;
    if (out_type_id) *out_type_id = frame[SERIALIZER_HEADER_SIZE];
    
    return 0;
}
