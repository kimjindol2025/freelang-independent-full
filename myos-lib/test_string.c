/**
 * MyOS String Test Suite (test_string.c)
 *
 * 테스트 케이스:
 * 1. string_new - 문자열 생성
 * 2. string_append - 문자열 연결
 * 3. string_insert - 특정 위치 삽입
 * 4. string_remove - 범위 삭제
 * 5. string_replace - 문자열 치환
 * 6. string_trim - 공백 제거
 * 7. string_to_uppercase/lowercase - 대소문자 변환
 * 8. string_find/contains - 검색
 * 9. string_split - 문자열 분할
 * 10. string_conversion - 숫자 변환
 */

#include "src/string.h"
#include "src/mm.h"
#include <unistd.h>
#include <sys/syscall.h>

/* ============ Test Utilities ============ */

static int test_count = 0;
static int test_passed = 0;
static int test_failed = 0;

static void test_write(const char *msg) {
    syscall(SYS_write, 1, msg, __builtin_strlen(msg));
}

static void test_writeln(const char *msg) {
    test_write(msg);
    test_write("\n");
}

static void assert_eq_str(const char *name, const char *actual, const char *expected) {
    test_count++;
    int match = 1;

    if (!actual) actual = "";
    if (!expected) expected = "";

    while (*actual && *expected && *actual == *expected) {
        actual++;
        expected++;
    }

    if (*actual == *expected) {  // Both ended or both same
        test_write("  ✅ ");
        test_write(name);
        test_writeln(" PASS");
        test_passed++;
    } else {
        test_write("  ❌ ");
        test_write(name);
        test_writeln(" FAIL");
        test_failed++;
    }
}

static void assert_eq_int(const char *name, int actual, int expected) {
    test_count++;
    if (actual == expected) {
        test_write("  ✅ ");
        test_write(name);
        test_writeln(" PASS");
        test_passed++;
    } else {
        test_write("  ❌ ");
        test_write(name);
        test_write(" FAIL (got ");
        char buf[32];
        __builtin_snprintf(buf, sizeof(buf), "%d", actual);
        test_write(buf);
        test_write(", expected ");
        __builtin_snprintf(buf, sizeof(buf), "%d", expected);
        test_write(buf);
        test_writeln(")");
        test_failed++;
    }
}

static void assert_eq_size_t(const char *name, size_t actual, size_t expected) {
    test_count++;
    if (actual == expected) {
        test_write("  ✅ ");
        test_write(name);
        test_writeln(" PASS");
        test_passed++;
    } else {
        test_write("  ❌ ");
        test_write(name);
        test_write(" FAIL (got ");
        char buf[32];
        __builtin_snprintf(buf, sizeof(buf), "%lu", actual);
        test_write(buf);
        test_write(", expected ");
        __builtin_snprintf(buf, sizeof(buf), "%lu", expected);
        test_write(buf);
        test_writeln(")");
        test_failed++;
    }
}

static void assert_not_null(const char *name, void *ptr) {
    test_count++;
    if (ptr != NULL) {
        test_write("  ✅ ");
        test_write(name);
        test_writeln(" PASS");
        test_passed++;
    } else {
        test_write("  ❌ ");
        test_write(name);
        test_writeln(" FAIL (got NULL)");
        test_failed++;
    }
}

/* ============ Test Cases ============ */

static void test_string_new() {
    test_writeln("\n[TEST 1] string_new - 문자열 생성");

    String *s = string_new("hello");
    assert_not_null("string_new returns non-NULL", s);
    assert_eq_size_t("length is 5", string_length(s), 5);
    assert_eq_str("content is hello", string_c_str(s), "hello");
    assert_eq_int("is_empty returns 0", string_is_empty(s), 0);

    String *empty = string_new("");
    assert_eq_size_t("empty string length is 0", string_length(empty), 0);
    assert_eq_int("empty string is_empty returns 1", string_is_empty(empty), 1);

    string_free(s);
    string_free(empty);
}

static void test_string_append() {
    test_writeln("\n[TEST 2] string_append - 문자열 연결");

    String *s = string_new("hello");
    int result = string_append(s, " world");
    assert_eq_int("append returns 0", result, 0);
    assert_eq_str("content is hello world", string_c_str(s), "hello world");
    assert_eq_size_t("length is 11", string_length(s), 11);

    string_append_char(s, '!');
    assert_eq_str("content with char", string_c_str(s), "hello world!");
    assert_eq_size_t("length is 12", string_length(s), 12);

    string_free(s);
}

static void test_string_insert() {
    test_writeln("\n[TEST 3] string_insert - 위치 삽입");

    String *s = string_new("hello");
    int result = string_insert(s, 5, " world");
    assert_eq_int("insert returns 0", result, 0);
    assert_eq_str("content after insert", string_c_str(s), "hello world");

    string_insert(s, 0, "say ");
    assert_eq_str("insert at start", string_c_str(s), "say hello world");

    string_free(s);
}

static void test_string_remove() {
    test_writeln("\n[TEST 4] string_remove - 범위 삭제");

    String *s = string_new("hello world");
    int result = string_remove(s, 5, 6);  // Remove " world"
    assert_eq_int("remove returns 0", result, 0);
    assert_eq_str("content after remove", string_c_str(s), "hello");
    assert_eq_size_t("length is 5", string_length(s), 5);

    string_free(s);
}

static void test_string_replace() {
    test_writeln("\n[TEST 5] string_replace - 문자열 치환");

    String *s = string_new("hello hello hello");
    int count = string_replace(s, "hello", "hi");
    assert_eq_int("replaced 3 occurrences", count, 3);
    assert_eq_str("content is hi hi hi", string_c_str(s), "hi hi hi");

    string_free(s);
}

static void test_string_trim() {
    test_writeln("\n[TEST 6] string_trim - 공백 제거");

    String *s = string_new("  hello world  ");
    int result = string_trim(s);
    assert_eq_int("trim returns 0", result, 0);
    assert_eq_str("content trimmed", string_c_str(s), "hello world");
    assert_eq_size_t("length is 11", string_length(s), 11);

    string_free(s);
}

static void test_string_case() {
    test_writeln("\n[TEST 7] string_to_uppercase/lowercase - 대소문자");

    String *s = string_new("Hello World");
    string_to_uppercase(s);
    assert_eq_str("uppercase", string_c_str(s), "HELLO WORLD");

    string_to_lowercase(s);
    assert_eq_str("lowercase", string_c_str(s), "hello world");

    string_free(s);
}

static void test_string_find() {
    test_writeln("\n[TEST 8] string_find/contains - 검색");

    String *s = string_new("hello world");

    int pos = string_find(s, "world");
    assert_eq_int("find position is 6", pos, 6);

    pos = string_find(s, "xyz");
    assert_eq_int("not found returns -1", pos, -1);

    int contains = string_contains(s, "hello");
    assert_eq_int("contains hello", contains, 1);

    contains = string_contains(s, "xyz");
    assert_eq_int("not contains xyz", contains, 0);

    string_free(s);
}

static void test_string_starts_ends() {
    test_writeln("\n[TEST 9] string_starts_with/ends_with");

    String *s = string_new("hello world");

    int result = string_starts_with(s, "hello");
    assert_eq_int("starts_with hello", result, 1);

    result = string_starts_with(s, "world");
    assert_eq_int("not starts_with world", result, 0);

    result = string_ends_with(s, "world");
    assert_eq_int("ends_with world", result, 1);

    result = string_ends_with(s, "hello");
    assert_eq_int("not ends_with hello", result, 0);

    string_free(s);
}

static void test_string_split() {
    test_writeln("\n[TEST 10] string_split - 문자열 분할");

    String *s = string_new("one,two,three");
    int count = 0;
    String **parts = string_split(s, ',', &count);

    assert_eq_int("split count is 3", count, 3);
    assert_eq_str("part 0 is one", string_c_str(parts[0]), "one");
    assert_eq_str("part 1 is two", string_c_str(parts[1]), "two");
    assert_eq_str("part 2 is three", string_c_str(parts[2]), "three");

    string_split_free(parts, count);
    string_free(s);
}

static void test_string_substring() {
    test_writeln("\n[TEST 11] string_substring - 부분 문자열");

    String *s = string_new("hello world");
    String *sub = string_substring(s, 0, 5);

    assert_eq_str("substring is hello", string_c_str(sub), "hello");
    assert_eq_size_t("substring length is 5", string_length(sub), 5);

    string_free(sub);

    sub = string_substring(s, 6, 5);
    assert_eq_str("substring is world", string_c_str(sub), "world");

    string_free(sub);
    string_free(s);
}

static void test_string_compare() {
    test_writeln("\n[TEST 12] string_compare - 문자열 비교");

    String *s1 = string_new("hello");
    String *s2 = string_new("hello");
    String *s3 = string_new("world");

    int result = string_compare(s1, s2);
    assert_eq_int("equal strings", result, 0);

    result = string_compare(s1, s3);
    assert_eq_int("s1 < s3", result < 0, 1);

    result = string_compare_c_str(s1, "hello");
    assert_eq_int("compare with c_str", result, 0);

    string_free(s1);
    string_free(s2);
    string_free(s3);
}

static void test_string_conversion() {
    test_writeln("\n[TEST 13] string_to_int/from_int - 숫자 변환");

    String *s = string_new("42");
    int value = 0;
    int result = string_to_int(s, &value);
    assert_eq_int("to_int returns 0", result, 0);
    assert_eq_int("value is 42", value, 42);

    String *s2 = string_from_int(42);
    assert_eq_str("from_int creates 42", string_c_str(s2), "42");

    String *s3 = string_from_int(-123);
    assert_eq_str("from_int creates -123", string_c_str(s3), "-123");

    string_free(s);
    string_free(s2);
    string_free(s3);
}

static void test_string_reverse() {
    test_writeln("\n[TEST 14] string_reverse - 문자열 역순");

    String *s = string_new("hello");
    string_reverse(s);
    assert_eq_str("reversed string", string_c_str(s), "olleh");

    string_free(s);
}

static void test_string_repeat() {
    test_writeln("\n[TEST 15] string_repeat - 문자열 반복");

    String *s = string_new("ab");
    int result = string_repeat(s, 3);
    assert_eq_int("repeat returns 0", result, 0);
    assert_eq_str("repeated string", string_c_str(s), "ababab");
    assert_eq_size_t("length is 6", string_length(s), 6);

    string_free(s);
}

static void test_string_concat() {
    test_writeln("\n[TEST 16] string_concat - 문자열 합치기");

    String *s1 = string_new("hello");
    String *s2 = string_new(" world");
    String *s3 = string_concat(s1, s2);

    assert_not_null("concat returns non-NULL", s3);
    assert_eq_str("concatenated string", string_c_str(s3), "hello world");

    string_free(s1);
    string_free(s2);
    string_free(s3);
}

static void test_string_clone() {
    test_writeln("\n[TEST 17] string_clone - 문자열 복사");

    String *s1 = string_new("hello");
    String *s2 = string_clone(s1);

    assert_not_null("clone returns non-NULL", s2);
    assert_eq_str("cloned string", string_c_str(s2), "hello");
    assert_eq_size_t("cloned length", string_length(s2), string_length(s1));

    string_free(s1);
    string_free(s2);
}

static void test_string_pad() {
    test_writeln("\n[TEST 18] string_pad_left/right - 패딩");

    String *s = string_new("hi");
    string_pad_left(s, 5, ' ');
    assert_eq_str("padded left", string_c_str(s), "   hi");
    assert_eq_size_t("padded length is 5", string_length(s), 5);

    string_free(s);

    s = string_new("hi");
    string_pad_right(s, 5, '*');
    assert_eq_str("padded right", string_c_str(s), "hi***");

    string_free(s);
}

static void test_string_autogrow() {
    test_writeln("\n[TEST 19] 자동 리사이징 테스트");

    String *s = string_new("");
    size_t initial_cap = string_capacity(s);

    // Add enough to trigger resizing
    for (int i = 0; i < 100; i++) {
        string_append_char(s, 'a');
    }

    assert_eq_size_t("length is 100", string_length(s), 100);
    size_t final_cap = string_capacity(s);
    assert_eq_int("capacity grew", (final_cap > initial_cap) ? 1 : 0, 1);

    string_free(s);
}

static void test_string_validate() {
    test_writeln("\n[TEST 20] string_validate - 무결성 검증");

    String *s = string_new("hello world");
    int result = string_validate(s);
    assert_eq_int("validate returns 0", result, 0);

    string_free(s);
}

/* ============ Test Runner ============ */

int main(void) {
    test_writeln("╔════════════════════════════════════════╗");
    test_writeln("║  MyOS String Engine Test Suite        ║");
    test_writeln("║  Dynamic String Management            ║");
    test_writeln("╚════════════════════════════════════════╝");

    /* Memory Manager 초기화 */
    mm_init(10 * 1024 * 1024);  /* 10MB 힙 */

    /* 테스트 실행 */
    test_string_new();
    test_string_append();
    test_string_insert();
    test_string_remove();
    test_string_replace();
    test_string_trim();
    test_string_case();
    test_string_find();
    test_string_starts_ends();
    test_string_split();
    test_string_substring();
    test_string_compare();
    test_string_conversion();
    test_string_reverse();
    test_string_repeat();
    test_string_concat();
    test_string_clone();
    test_string_pad();
    test_string_autogrow();
    test_string_validate();

    /* Memory Manager 정리 */
    mm_destroy();

    /* 결과 요약 */
    test_writeln("\n╔════════════════════════════════════════╗");
    test_writeln("║  Test Summary                          ║");
    test_writeln("╚════════════════════════════════════════╝");

    char summary[256];
    __builtin_snprintf(summary, sizeof(summary),
                      "\nTotal Tests: %d\n✅ Passed: %d\n❌ Failed: %d\n",
                      test_count, test_passed, test_failed);
    test_write(summary);

    if (test_failed == 0) {
        test_writeln("\n🎉 All tests passed!");
        return 0;
    } else {
        test_writeln("\n⚠️  Some tests failed");
        return 1;
    }
}
