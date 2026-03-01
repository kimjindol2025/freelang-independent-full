# MyOS_Lib Serializer Protocol v1.0

**작성**: 2026-03-01
**상태**: 설계 문서

---

## 🎯 목표

Zero-Dependency 환경에서 모든 자료구조를 바이너리로 직렬화/역직렬화하는 표준 프로토콜 정의

---

## 📐 프로토콜 구조

### Frame Format

```
┌─────────────────────────────────────────────┐
│ Header (11 bytes)                           │
├─────────────────────────────────────────────┤
│ Payload (variable length)                   │
├─────────────────────────────────────────────┤
│ Checksum (4 bytes)                          │
└─────────────────────────────────────────────┘
```

### Header (11 bytes 고정)

```
Offset  Size  Field                 Description
─────────────────────────────────────────────────
0       4     Magic                 0x4D59004F ("MY\0O")
4       1     Version               Protocol version (1)
5       2     Flags                 Bit flags
7       4     Payload Length        Big-endian uint32

Flags (2 bytes):
  Bit 0: Compression (1=deflate, 0=raw)
  Bit 1: Encryption (1=enabled, 0=disabled)
  Bit 2-15: Reserved
```

### Payload (가변 길이)

```
Offset  Size  Field                 Description
─────────────────────────────────────────────────
0       1     Type ID               자료구조 타입
1       N     Data                  타입별 데이터

Type ID:
  0x01 = Vector (동적 배열)
  0x02 = String (문자열)
  0x03 = HashMap (해시맵)
  0x04 = LinkedList (연결 리스트)
  0x05 = Raw Bytes (원시 바이트)
  0xFF = Null (null 포인터)
```

### Checksum (4 bytes)

```
CRC32 (IEEE 802.3)
- 계산: Payload 전체 (Type ID + Data)
- 알고리즘: Polynomial 0x04C11DB7
- 초기값: 0xFFFFFFFF
- 최종 XOR: 0xFFFFFFFF
```

---

## 📝 각 타입별 직렬화 형식

### Vector 직렬화

```
Type: 0x01

Header:
  uint32 elem_size     (Big-endian) - 각 요소의 크기
  uint32 count         (Big-endian) - 요소 개수

Data:
  N × elem_size bytes  - 모든 요소 바이트 배열

예시 (int 배열 [1, 2, 3]):
  Type: 0x01
  elem_size: 0x00000004 (4 bytes)
  count: 0x00000003 (3 elements)
  Data: 0x00000001 0x00000002 0x00000003
```

### String 직렬화

```
Type: 0x02

Header:
  uint32 length        (Big-endian) - 문자열 길이

Data:
  length bytes         - UTF-8 문자열 (null terminator 제외)

예시 ("hello"):
  Type: 0x02
  length: 0x00000005
  Data: "hello" (5 bytes)
```

### HashMap 직렬화

```
Type: 0x03

Header:
  uint32 key_size      (Big-endian)
  uint32 value_size    (Big-endian)
  uint32 count         (Big-endian)

Data:
  count × (key_size + value_size) bytes

Entry Format:
  [key_bytes][value_bytes][key_bytes][value_bytes]...

예시 (int→int map {1→10, 2→20}):
  Type: 0x03
  key_size: 0x00000004
  value_size: 0x00000004
  count: 0x00000002
  Data: [0x00000001 0x0000000A][0x00000002 0x00000014]
```

### LinkedList 직렬화

```
Type: 0x04

Header:
  uint32 elem_size     (Big-endian)
  uint32 count         (Big-endian)

Data:
  count × elem_size bytes

예시 (int list [10, 20, 30]):
  Type: 0x04
  elem_size: 0x00000004
  count: 0x00000003
  Data: 0x0000000A 0x00000014 0x0000001E
```

### Raw Bytes 직렬화

```
Type: 0x05

Header:
  uint32 length        (Big-endian)

Data:
  length bytes

예시 (100 bytes 바이너리):
  Type: 0x05
  length: 0x00000064
  Data: [100 bytes]
```

---

## 🔄 직렬화 예제

### 예제 1: String "hello"

```
Hex Dump:
4D 59 00 4F           [Magic: MY\0O]
01                    [Version: 1]
00 00                 [Flags: 0 (no compression)]
00 00 00 08           [Payload Length: 8 bytes]
02                    [Type: String]
00 00 00 05           [Length: 5]
68 65 6C 6C 6F        [Data: "hello"]
12 34 56 78           [Checksum: CRC32]

총 길이: 11 (header) + 8 (payload) + 4 (checksum) = 23 bytes
```

### 예제 2: Vector [1, 2, 3]

```
Hex Dump:
4D 59 00 4F           [Magic]
01                    [Version: 1]
00 00                 [Flags: 0]
00 00 00 10           [Payload Length: 16 bytes]
01                    [Type: Vector]
00 00 00 04           [elem_size: 4]
00 00 00 03           [count: 3]
00 00 00 01           [Element 1: 1]
00 00 00 02           [Element 2: 2]
00 00 00 03           [Element 3: 3]
AB CD EF 01           [Checksum]

총 길이: 11 + 16 + 4 = 31 bytes
```

---

## 🛡️ 특징

### 1. 호환성
- Big-endian 고정 (네트워크 표준)
- 각 타입별 명확한 길이 필드
- 미래 확장 가능한 구조

### 2. 무결성
- CRC32 체크섬으로 손상 감지
- 각 Frame은 독립적

### 3. 보안 준비
- Compression 플래그 (미래 구현)
- Encryption 플래그 (미래 구현)

### 4. 효율성
- 헤더 고정 크기 (11 bytes)
- 메타데이터 최소화
- 바이너리 형식 (JSON 대비 90% 크기 감소)

---

## 📋 제약사항

### 현재 구현 범위
- Compression: 미지원 (flag만 예약)
- Encryption: 미지원 (flag만 예약)
- Streaming: 단일 Frame만 (멀티-frame은 미지원)

### 타입 제한
- Vector/LinkedList: 모든 elem_size 지원
- String: UTF-8만 (이진 데이터는 Raw Bytes 사용)
- HashMap: 모든 key/value size 지원

---

## 🔧 API 설계

### 직렬화
```c
int serializer_encode_vector(Vector *v, uint8_t **out, size_t *out_len);
int serializer_encode_string(String *s, uint8_t **out, size_t *out_len);
int serializer_encode_hashmap(HashMap *h, uint8_t **out, size_t *out_len);
int serializer_encode_list(LinkedList *l, uint8_t **out, size_t *out_len);
```

### 역직렬화
```c
Vector* serializer_decode_vector(const uint8_t *data, size_t len);
String* serializer_decode_string(const uint8_t *data, size_t len);
HashMap* serializer_decode_hashmap(const uint8_t *data, size_t len);
LinkedList* serializer_decode_list(const uint8_t *data, size_t len);
```

### 유틸리티
```c
uint32_t crc32_calculate(const uint8_t *data, size_t len);
int serializer_validate(const uint8_t *frame, size_t len);
```

---

## 📊 성능 기준

| 작업 | 시간 | 메모리 |
|------|------|--------|
| String 직렬화 | O(n) | O(n) |
| Vector 직렬화 | O(n) | O(n) |
| HashMap 직렬화 | O(n) | O(n) |
| LinkedList 직렬화 | O(n) | O(n) |
| CRC32 계산 | O(n) | O(1) |

여기서 n = 전체 데이터 바이트 수

