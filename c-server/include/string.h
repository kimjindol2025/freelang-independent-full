/**
 * MyOS String Engine (string.h)
 *
 * 동적 문자열 관리 (String Handle)
 * - Memory Manager (mm.c) 기반
 * - Zero-copy 부분 문자열 (slice)
 * - 자동 리사이징
 * - String 타입 선택 (ASCII, UTF-8)
 *
 * 사용 예시:
 *   String *s = string_new("hello");
 *   string_append(s, " world");
 *   char *ptr = string_c_str(s);
 *   printf("%s\n", ptr);  // "hello world"
 *   string_free(s);
 */

#ifndef MYOS_STRING_H
#define MYOS_STRING_H

#include <stddef.h>
#include <stdint.h>

/* ============ String Type ============ */

/**
 * String - 동적 문자열 구조체
 *
 * 내부 필드 (사용자가 직접 접근하면 안 됨):
 * - data: 문자열 버퍼
 * - len: 현재 문자열 길이 (null 제외)
 * - capacity: 할당된 버퍼 크기
 */
typedef struct {
    char *data;
    size_t len;
    size_t capacity;
} String;

/* ============ String Creation/Destruction ============ */

/**
 * string_new - 새 문자열 생성
 * @str: 초기 문자열 (NULL이면 빈 문자열)
 *
 * 반환: 새로운 String 포인터
 * 실패 시 NULL 반환
 */
String* string_new(const char *str);

/**
 * string_new_with_capacity - 지정된 용량으로 문자열 생성
 * @capacity: 초기 용량
 *
 * 반환: 새로운 String 포인터
 */
String* string_new_with_capacity(size_t capacity);

/**
 * string_free - 문자열 정리
 * @s: 문자열
 *
 * 모든 메모리 해제
 * s가 NULL이면 아무 것도 하지 않음
 */
void string_free(String *s);

/* ============ String Operations ============ */

/**
 * string_append - 문자열 연결
 * @s: 대상 문자열
 * @str: 추가할 문자열
 *
 * 필요 시 자동으로 용량 확대
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_append(String *s, const char *str);

/**
 * string_append_char - 문자 추가
 * @s: 문자열
 * @c: 추가할 문자
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_append_char(String *s, char c);

/**
 * string_append_n - 길이 지정해서 문자열 연결
 * @s: 대상 문자열
 * @str: 추가할 문자열
 * @n: 추가할 길이
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_append_n(String *s, const char *str, size_t n);

/**
 * string_insert - 특정 위치에 문자열 삽입
 * @s: 문자열
 * @pos: 삽입 위치
 * @str: 삽입할 문자열
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_insert(String *s, size_t pos, const char *str);

/**
 * string_remove - 문자열 범위 삭제
 * @s: 문자열
 * @start: 시작 위치
 * @len: 삭제할 길이
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_remove(String *s, size_t start, size_t len);

/**
 * string_clear - 문자열 내용 초기화
 * @s: 문자열
 *
 * 용량은 유지, 길이만 0으로 설정
 */
void string_clear(String *s);

/**
 * string_replace - 문자열 치환
 * @s: 대상 문자열
 * @from: 찾을 문자열
 * @to: 바꿀 문자열
 *
 * 모든 occurrence를 치환
 * 반환: 치환된 횟수 (0이면 못 찾음)
 */
int string_replace(String *s, const char *from, const char *to);

/**
 * string_trim - 앞뒤 공백 제거
 * @s: 문자열
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_trim(String *s);

/**
 * string_trim_left - 앞 공백 제거
 * @s: 문자열
 */
int string_trim_left(String *s);

/**
 * string_trim_right - 뒤 공백 제거
 * @s: 문자열
 */
int string_trim_right(String *s);

/**
 * string_to_uppercase - 대문자로 변환
 * @s: 문자열
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_to_uppercase(String *s);

/**
 * string_to_lowercase - 소문자로 변환
 * @s: 문자열
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_to_lowercase(String *s);

/* ============ String Queries ============ */

/**
 * string_length - 문자열 길이
 * @s: 문자열
 *
 * null 종료자 제외
 */
size_t string_length(const String *s);

/**
 * string_capacity - 할당된 용량
 * @s: 문자열
 */
size_t string_capacity(const String *s);

/**
 * string_is_empty - 문자열이 비어있는지 확인
 * @s: 문자열
 *
 * 반환: 1 (비어있음) 또는 0 (데이터 있음)
 */
int string_is_empty(const String *s);

/**
 * string_c_str - C 문자열 포인터 반환
 * @s: 문자열
 *
 * 반환: null-terminated 문자열 포인터
 * 이 포인터는 string_free() 까지 유효
 */
const char* string_c_str(const String *s);

/**
 * string_at - 특정 위치의 문자
 * @s: 문자열
 * @idx: 인덱스
 *
 * 반환: 문자 (범위 밖이면 '\0')
 */
char string_at(const String *s, size_t idx);

/**
 * string_find - 부분 문자열 검색
 * @s: 문자열
 * @substr: 검색할 부분 문자열
 *
 * 반환: 시작 위치 (못 찾으면 -1)
 */
int string_find(const String *s, const char *substr);

/**
 * string_contains - 부분 문자열 포함 확인
 * @s: 문자열
 * @substr: 검색할 부분 문자열
 *
 * 반환: 1 (포함) 또는 0 (미포함)
 */
int string_contains(const String *s, const char *substr);

/**
 * string_starts_with - 문자열 접두사 확인
 * @s: 문자열
 * @prefix: 확인할 접두사
 *
 * 반환: 1 (있음) 또는 0 (없음)
 */
int string_starts_with(const String *s, const char *prefix);

/**
 * string_ends_with - 문자열 접미사 확인
 * @s: 문자열
 * @suffix: 확인할 접미사
 *
 * 반환: 1 (있음) 또는 0 (없음)
 */
int string_ends_with(const String *s, const char *suffix);

/**
 * string_compare - 문자열 비교
 * @s1: 첫 번째 문자열
 * @s2: 두 번째 문자열
 *
 * 반환: 0 (같음), 음수 (s1 < s2), 양수 (s1 > s2)
 */
int string_compare(const String *s1, const String *s2);

/**
 * string_compare_c_str - C 문자열과 비교
 * @s: 문자열
 * @str: C 문자열
 *
 * 반환: 0 (같음), 음수, 양수
 */
int string_compare_c_str(const String *s, const char *str);

/* ============ String Conversion ============ */

/**
 * string_to_int - 정수 변환
 * @s: 문자열
 * @out_value: 결과를 저장할 포인터
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_to_int(const String *s, int *out_value);

/**
 * string_to_long - long 정수 변환
 * @s: 문자열
 * @out_value: 결과를 저장할 포인터
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_to_long(const String *s, long *out_value);

/**
 * string_to_double - 실수 변환
 * @s: 문자열
 * @out_value: 결과를 저장할 포인터
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_to_double(const String *s, double *out_value);

/**
 * string_from_int - 정수에서 문자열 생성
 * @value: 정수값
 *
 * 반환: 새로운 String 포인터
 */
String* string_from_int(int value);

/**
 * string_from_long - long 정수에서 문자열 생성
 * @value: long 정수값
 *
 * 반환: 새로운 String 포인터
 */
String* string_from_long(long value);

/**
 * string_from_double - 실수에서 문자열 생성
 * @value: 실수값
 *
 * 반환: 새로운 String 포인터
 */
String* string_from_double(double value);

/* ============ String Splitting ============ */

/**
 * string_split - 구분자로 문자열 분할
 * @s: 문자열
 * @delimiter: 구분자 문자
 * @out_count: 분할된 부분 개수 저장 포인터
 *
 * 반환: String 배열 포인터 (동적 할당)
 * 사용 후 각 String과 배열 자체 해제 필요
 */
String** string_split(const String *s, char delimiter, int *out_count);

/**
 * string_split_str - 문자열 구분자로 분할
 * @s: 문자열
 * @delimiter: 구분자 문자열
 * @out_count: 분할된 부분 개수 저장 포인터
 *
 * 반환: String 배열 포인터 (동적 할당)
 */
String** string_split_str(const String *s, const char *delimiter, int *out_count);

/**
 * string_split_free - string_split() 결과 메모리 해제
 * @parts: 분할 결과 배열
 * @count: 배열 크기
 */
void string_split_free(String **parts, int count);

/* ============ String Substring ============ */

/**
 * string_substring - 부분 문자열 추출
 * @s: 문자열
 * @start: 시작 위치
 * @len: 추출할 길이
 *
 * 반환: 새로운 String 포인터 (동적 할당)
 * 범위 초과 시 가능한 부분만 추출
 */
String* string_substring(const String *s, size_t start, size_t len);

/**
 * string_slice - 부분 문자열 (포인터 반환)
 * @s: 문자열
 * @start: 시작 위치
 *
 * 반환: 해당 위치의 포인터 (원본 데이터)
 * 주의: 원본 문자열 수정 시 포인터 무효화 가능
 */
const char* string_slice(const String *s, size_t start);

/* ============ String Utilities ============ */

/**
 * string_reverse - 문자열 역순 배열
 * @s: 문자열
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_reverse(String *s);

/**
 * string_repeat - 문자열 반복
 * @s: 대상 문자열
 * @count: 반복 횟수
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_repeat(String *s, size_t count);

/**
 * string_clone - 문자열 복사
 * @s: 원본 문자열
 *
 * 반환: 새로운 String 포인터
 */
String* string_clone(const String *s);

/**
 * string_concat - 두 문자열을 합쳐 새 문자열 생성
 * @s1: 첫 번째 문자열
 * @s2: 두 번째 문자열
 *
 * 반환: 새로운 String 포인터
 */
String* string_concat(const String *s1, const String *s2);

/**
 * string_join - 배열 문자열을 구분자로 합치기
 * @strings: String 배열
 * @count: 배열 크기
 * @separator: 구분자 문자열
 *
 * 반환: 새로운 String 포인터
 */
String* string_join(const String **strings, int count, const char *separator);

/**
 * string_pad_left - 왼쪽 패딩
 * @s: 문자열
 * @width: 목표 너비
 * @pad_char: 패딩 문자
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_pad_left(String *s, size_t width, char pad_char);

/**
 * string_pad_right - 오른쪽 패딩
 * @s: 문자열
 * @width: 목표 너비
 * @pad_char: 패딩 문자
 *
 * 반환: 0 (성공) 또는 -1 (실패)
 */
int string_pad_right(String *s, size_t width, char pad_char);

/* ============ Debug Functions ============ */

/**
 * string_dump - 문자열 정보 출력 (syscall 사용)
 * @s: 문자열
 */
void string_dump(const String *s);

/**
 * string_validate - 문자열 무결성 검증
 * @s: 문자열
 *
 * 반환: 0 (정상) 또는 -1 (오류)
 */
int string_validate(const String *s);

#endif /* MYOS_STRING_H */
