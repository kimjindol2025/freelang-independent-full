#ifndef MYOS_SERIALIZER_H
#define MYOS_SERIALIZER_H

#include <stddef.h>
#include <stdint.h>
#include "vector.h"
#include "string.h"
#include "hash.h"
#include "list.h"

#define SERIALIZER_MAGIC         0x4D59004FU
#define SERIALIZER_VERSION       1
#define SERIALIZER_HEADER_SIZE   11

#define SERIALIZER_TYPE_VECTOR   0x01
#define SERIALIZER_TYPE_STRING   0x02
#define SERIALIZER_TYPE_HASHMAP  0x03
#define SERIALIZER_TYPE_LIST     0x04
#define SERIALIZER_TYPE_BYTES    0x05
#define SERIALIZER_TYPE_NULL     0xFF

int serializer_encode_vector(Vector *v, uint8_t **out, size_t *out_len);
int serializer_encode_string(String *s, uint8_t **out, size_t *out_len);
int serializer_encode_hashmap(HashMap *h, uint8_t **out, size_t *out_len);
int serializer_encode_list(LinkedList *l, uint8_t **out, size_t *out_len);
int serializer_encode_bytes(const uint8_t *data, size_t len, uint8_t **out, size_t *out_len);

Vector* serializer_decode_vector(const uint8_t *data, size_t len);
String* serializer_decode_string(const uint8_t *data, size_t len);
HashMap* serializer_decode_hashmap(const uint8_t *data, size_t len);
LinkedList* serializer_decode_list(const uint8_t *data, size_t len);
int serializer_decode_bytes(const uint8_t *data, size_t len, uint8_t **out_buf, size_t *out_len);

int serializer_validate(const uint8_t *frame, size_t len);
int serializer_parse_header(const uint8_t *frame, size_t len, uint8_t *out_version,
                            uint16_t *out_flags, uint32_t *out_payload_len, uint8_t *out_type_id);

void serializer_write_u32(uint32_t val, uint8_t *buf);
uint32_t serializer_read_u32(const uint8_t *buf);
void serializer_write_u16(uint16_t val, uint8_t *buf);
uint16_t serializer_read_u16(const uint8_t *buf);

#endif
