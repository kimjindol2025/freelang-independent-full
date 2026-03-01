/**
 * MyOS String Engine (string.c)
 *
 * 동적 문자열 구현
 * - Memory Manager (mm.c) 기반
 * - 자동 리사이징 (2배 성장)
 * - null-terminated C 호환성
 */

#include "string.h"
#include "mm.h"
#include <stdint.h>
#include <unistd.h>
#include <sys/syscall.h>

/* ============ Utility Functions ============ */

static void my_memcpy(void *dest, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dest;
    const unsigned char *s = (const unsigned char *)src;
    for (size_t i = 0; i < n; i++) {
        d[i] = s[i];
    }
}

static void my_memmove(void *dest, const void *src, size_t n) {
    if (dest < src) {
        my_memcpy(dest, src, n);
    } else {
        unsigned char *d = (unsigned char *)dest + n;
        const unsigned char *s = (const unsigned char *)src + n;
        while (n--) {
            *(--d) = *(--s);
        }
    }
}

static size_t my_strlen(const char *str) {
    if (!str) return 0;
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static int my_strcmp(const char *s1, const char *s2) {
    if (!s1) s1 = "";
    if (!s2) s2 = "";
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static int my_strncmp(const char *s1, const char *s2, size_t n) {
    if (!s1) s1 = "";
    if (!s2) s2 = "";
    for (size_t i = 0; i < n; i++) {
        if (!s1[i] || !s2[i] || s1[i] != s2[i]) {
            return (unsigned char)s1[i] - (unsigned char)s2[i];
        }
    }
    return 0;
}

static void debug_write(const char *msg) {
    syscall(SYS_write, 1, msg, my_strlen(msg));
}

/* ============ String Creation/Destruction ============ */

String* string_new(const char *str) {
    String *s = (String*)mm_alloc(sizeof(String));
    if (!s) return NULL;

    size_t len = my_strlen(str);
    size_t capacity = (len < 16) ? 16 : (len * 2);

    s->data = (char*)mm_alloc(capacity + 1);
    if (!s->data) {
        mm_free(s);
        return NULL;
    }

    if (str) {
        my_memcpy(s->data, str, len);
    }
    s->data[len] = '\0';
    s->len = len;
    s->capacity = capacity;

    return s;
}

String* string_new_with_capacity(size_t capacity) {
    String *s = (String*)mm_alloc(sizeof(String));
    if (!s) return NULL;

    if (capacity < 16) capacity = 16;

    s->data = (char*)mm_alloc(capacity + 1);
    if (!s->data) {
        mm_free(s);
        return NULL;
    }

    s->data[0] = '\0';
    s->len = 0;
    s->capacity = capacity;

    return s;
}

void string_free(String *s) {
    if (!s) return;
    if (s->data) mm_free(s->data);
    mm_free(s);
}

/* ============ Helper: Resize ============ */

static int string_ensure_capacity(String *s, size_t required) {
    if (required <= s->capacity) return 0;

    size_t new_capacity = s->capacity * 2;
    while (new_capacity < required) {
        new_capacity *= 2;
    }

    char *new_data = (char*)mm_alloc(new_capacity + 1);
    if (!new_data) return -1;

    my_memcpy(new_data, s->data, s->len);
    new_data[s->len] = '\0';

    mm_free(s->data);
    s->data = new_data;
    s->capacity = new_capacity;

    return 0;
}

/* ============ String Operations ============ */

int string_append(String *s, const char *str) {
    if (!s || !str) return -1;
    return string_append_n(s, str, my_strlen(str));
}

int string_append_char(String *s, char c) {
    if (!s) return -1;
    if (string_ensure_capacity(s, s->len + 1) != 0) return -1;

    s->data[s->len] = c;
    s->len++;
    s->data[s->len] = '\0';

    return 0;
}

int string_append_n(String *s, const char *str, size_t n) {
    if (!s || !str) return -1;

    if (string_ensure_capacity(s, s->len + n) != 0) return -1;

    my_memcpy(s->data + s->len, str, n);
    s->len += n;
    s->data[s->len] = '\0';

    return 0;
}

int string_insert(String *s, size_t pos, const char *str) {
    if (!s || !str) return -1;
    if (pos > s->len) pos = s->len;

    size_t str_len = my_strlen(str);
    if (string_ensure_capacity(s, s->len + str_len) != 0) return -1;

    my_memmove(s->data + pos + str_len, s->data + pos, s->len - pos);
    my_memcpy(s->data + pos, str, str_len);
    s->len += str_len;
    s->data[s->len] = '\0';

    return 0;
}

int string_remove(String *s, size_t start, size_t len) {
    if (!s) return -1;
    if (start >= s->len) return 0;
    if (start + len > s->len) len = s->len - start;

    my_memmove(s->data + start, s->data + start + len, s->len - start - len);
    s->len -= len;
    s->data[s->len] = '\0';

    return 0;
}

void string_clear(String *s) {
    if (!s) return;
    s->len = 0;
    s->data[0] = '\0';
}

int string_replace(String *s, const char *from, const char *to) {
    if (!s || !from || !to) return 0;

    size_t from_len = my_strlen(from);
    size_t to_len = my_strlen(to);
    int count = 0;

    if (from_len == 0) return 0;

    for (size_t pos = 0; pos <= s->len - from_len; ) {
        if (my_strncmp(s->data + pos, from, from_len) == 0) {
            if (from_len != to_len) {
                if (from_len < to_len) {
                    if (string_ensure_capacity(s, s->len + to_len - from_len) != 0) {
                        return count;
                    }
                }
                my_memmove(s->data + pos + to_len,
                          s->data + pos + from_len,
                          s->len - pos - from_len);
                s->len = s->len - from_len + to_len;
                s->data[s->len] = '\0';
            }
            my_memcpy(s->data + pos, to, to_len);
            pos += to_len;
            count++;
        } else {
            pos++;
        }
    }

    return count;
}

int string_trim(String *s) {
    if (string_trim_left(s) != 0) return -1;
    return string_trim_right(s);
}

int string_trim_left(String *s) {
    if (!s) return -1;

    size_t i = 0;
    while (i < s->len && (s->data[i] == ' ' || s->data[i] == '\t' ||
                          s->data[i] == '\n' || s->data[i] == '\r')) {
        i++;
    }

    if (i > 0) {
        my_memmove(s->data, s->data + i, s->len - i);
        s->len -= i;
        s->data[s->len] = '\0';
    }

    return 0;
}

int string_trim_right(String *s) {
    if (!s) return -1;

    while (s->len > 0) {
        char c = s->data[s->len - 1];
        if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
            break;
        }
        s->len--;
    }
    s->data[s->len] = '\0';

    return 0;
}

int string_to_uppercase(String *s) {
    if (!s) return -1;

    for (size_t i = 0; i < s->len; i++) {
        if (s->data[i] >= 'a' && s->data[i] <= 'z') {
            s->data[i] = s->data[i] - 'a' + 'A';
        }
    }

    return 0;
}

int string_to_lowercase(String *s) {
    if (!s) return -1;

    for (size_t i = 0; i < s->len; i++) {
        if (s->data[i] >= 'A' && s->data[i] <= 'Z') {
            s->data[i] = s->data[i] - 'A' + 'a';
        }
    }

    return 0;
}

/* ============ String Queries ============ */

size_t string_length(const String *s) {
    return s ? s->len : 0;
}

size_t string_capacity(const String *s) {
    return s ? s->capacity : 0;
}

int string_is_empty(const String *s) {
    return !s || s->len == 0 ? 1 : 0;
}

const char* string_c_str(const String *s) {
    return s ? s->data : "";
}

char string_at(const String *s, size_t idx) {
    if (!s || idx >= s->len) return '\0';
    return s->data[idx];
}

int string_find(const String *s, const char *substr) {
    if (!s || !substr) return -1;

    size_t sub_len = my_strlen(substr);
    if (sub_len == 0 || sub_len > s->len) return -1;

    for (size_t i = 0; i <= s->len - sub_len; i++) {
        if (my_strncmp(s->data + i, substr, sub_len) == 0) {
            return (int)i;
        }
    }

    return -1;
}

int string_contains(const String *s, const char *substr) {
    return string_find(s, substr) >= 0 ? 1 : 0;
}

int string_starts_with(const String *s, const char *prefix) {
    if (!s || !prefix) return 0;

    size_t prefix_len = my_strlen(prefix);
    if (prefix_len > s->len) return 0;

    return my_strncmp(s->data, prefix, prefix_len) == 0 ? 1 : 0;
}

int string_ends_with(const String *s, const char *suffix) {
    if (!s || !suffix) return 0;

    size_t suffix_len = my_strlen(suffix);
    if (suffix_len > s->len) return 0;

    return my_strcmp(s->data + s->len - suffix_len, suffix) == 0 ? 1 : 0;
}

int string_compare(const String *s1, const String *s2) {
    const char *str1 = s1 ? s1->data : "";
    const char *str2 = s2 ? s2->data : "";
    return my_strcmp(str1, str2);
}

int string_compare_c_str(const String *s, const char *str) {
    const char *s_data = s ? s->data : "";
    if (!str) str = "";
    return my_strcmp(s_data, str);
}

/* ============ String Conversion ============ */

static int parse_int(const char *str, size_t len, int *out) {
    if (!str || len == 0) return -1;

    int sign = 1;
    size_t pos = 0;

    if (str[0] == '-') {
        sign = -1;
        pos = 1;
    } else if (str[0] == '+') {
        pos = 1;
    }

    if (pos >= len) return -1;

    int result = 0;
    for (size_t i = pos; i < len; i++) {
        if (str[i] < '0' || str[i] > '9') return -1;
        result = result * 10 + (str[i] - '0');
    }

    *out = result * sign;
    return 0;
}

int string_to_int(const String *s, int *out_value) {
    if (!s || !out_value) return -1;
    return parse_int(s->data, s->len, out_value);
}

int string_to_long(const String *s, long *out_value) {
    if (!s || !out_value) return -1;

    int int_val;
    if (parse_int(s->data, s->len, &int_val) != 0) return -1;

    *out_value = int_val;
    return 0;
}

int string_to_double(const String *s, double *out_value) {
    if (!s || !out_value) return -1;
    // Simplified: just convert integer part for now
    int int_val;
    if (parse_int(s->data, s->len, &int_val) != 0) return -1;
    *out_value = (double)int_val;
    return 0;
}

static String* int_to_string_impl(long value, int is_long) {
    char buf[32];
    int pos = 0;

    if (value < 0) {
        buf[pos++] = '-';
        value = -value;
    }

    if (value == 0) {
        buf[pos++] = '0';
    } else {
        char temp[32];
        int temp_pos = 0;
        long v = value;
        while (v > 0) {
            temp[temp_pos++] = '0' + (v % 10);
            v /= 10;
        }
        for (int i = temp_pos - 1; i >= 0; i--) {
            buf[pos++] = temp[i];
        }
    }
    buf[pos] = '\0';

    return string_new(buf);
}

String* string_from_int(int value) {
    return int_to_string_impl((long)value, 0);
}

String* string_from_long(long value) {
    return int_to_string_impl(value, 1);
}

String* string_from_double(double value) {
    // Simplified: convert to int for now
    return int_to_string_impl((long)value, 1);
}

/* ============ String Splitting ============ */

String** string_split(const String *s, char delimiter, int *out_count) {
    if (!s || !out_count) return NULL;

    // Count parts
    int count = 1;
    for (size_t i = 0; i < s->len; i++) {
        if (s->data[i] == delimiter) count++;
    }

    String **parts = (String**)mm_alloc(sizeof(String*) * count);
    if (!parts) return NULL;

    size_t start = 0;
    int part_idx = 0;

    for (size_t i = 0; i <= s->len; i++) {
        if (i == s->len || s->data[i] == delimiter) {
            size_t len = i - start;
            parts[part_idx] = string_substring(s, start, len);
            if (!parts[part_idx]) {
                // Cleanup and return error
                for (int j = 0; j < part_idx; j++) {
                    string_free(parts[j]);
                }
                mm_free(parts);
                return NULL;
            }
            part_idx++;
            start = i + 1;
        }
    }

    *out_count = count;
    return parts;
}

String** string_split_str(const String *s, const char *delimiter, int *out_count) {
    if (!s || !delimiter || !out_count) return NULL;

    size_t delim_len = my_strlen(delimiter);
    if (delim_len == 0) return NULL;

    // Count parts
    int count = 1;
    for (size_t i = 0; i + delim_len <= s->len; i++) {
        if (my_strncmp(s->data + i, delimiter, delim_len) == 0) {
            count++;
        }
    }

    String **parts = (String**)mm_alloc(sizeof(String*) * count);
    if (!parts) return NULL;

    size_t start = 0;
    int part_idx = 0;

    for (size_t i = 0; i <= s->len; ) {
        int found = 0;
        if (i + delim_len <= s->len && my_strncmp(s->data + i, delimiter, delim_len) == 0) {
            found = 1;
        }

        if (found || i == s->len) {
            size_t len = i - start;
            parts[part_idx] = string_substring(s, start, len);
            if (!parts[part_idx]) {
                for (int j = 0; j < part_idx; j++) {
                    string_free(parts[j]);
                }
                mm_free(parts);
                return NULL;
            }
            part_idx++;
            start = i + (found ? delim_len : 1);
            i += (found ? delim_len : 1);
        } else {
            i++;
        }
    }

    *out_count = count;
    return parts;
}

void string_split_free(String **parts, int count) {
    if (!parts) return;
    for (int i = 0; i < count; i++) {
        if (parts[i]) string_free(parts[i]);
    }
    mm_free(parts);
}

/* ============ String Substring ============ */

String* string_substring(const String *s, size_t start, size_t len) {
    if (!s) return NULL;
    if (start >= s->len) return string_new("");

    if (start + len > s->len) {
        len = s->len - start;
    }

    String *result = string_new_with_capacity(len + 1);
    if (!result) return NULL;

    if (len > 0) {
        my_memcpy(result->data, s->data + start, len);
    }
    result->data[len] = '\0';
    result->len = len;

    return result;
}

const char* string_slice(const String *s, size_t start) {
    if (!s || start >= s->len) return "";
    return s->data + start;
}

/* ============ String Utilities ============ */

int string_reverse(String *s) {
    if (!s) return -1;

    size_t left = 0;
    size_t right = s->len - 1;

    while (left < right) {
        char temp = s->data[left];
        s->data[left] = s->data[right];
        s->data[right] = temp;
        left++;
        right--;
    }

    return 0;
}

int string_repeat(String *s, size_t count) {
    if (!s || count == 0) return -1;
    if (count == 1) return 0;

    if (string_ensure_capacity(s, s->len * count) != 0) return -1;

    size_t original_len = s->len;
    for (size_t i = 1; i < count; i++) {
        my_memcpy(s->data + (i * original_len), s->data, original_len);
    }
    s->len *= count;
    s->data[s->len] = '\0';

    return 0;
}

String* string_clone(const String *s) {
    if (!s) return NULL;
    return string_substring(s, 0, s->len);
}

String* string_concat(const String *s1, const String *s2) {
    if (!s1) return s2 ? string_clone(s2) : string_new("");
    if (!s2) return string_clone(s1);

    String *result = string_clone(s1);
    if (!result) return NULL;

    if (string_append(result, s2->data) != 0) {
        string_free(result);
        return NULL;
    }

    return result;
}

String* string_join(const String **strings, int count, const char *separator) {
    if (!strings || count == 0) return string_new("");

    String *result = string_clone(strings[0]);
    if (!result) return NULL;

    for (int i = 1; i < count; i++) {
        if (separator && string_append(result, separator) != 0) {
            string_free(result);
            return NULL;
        }
        if (strings[i] && string_append(result, strings[i]->data) != 0) {
            string_free(result);
            return NULL;
        }
    }

    return result;
}

int string_pad_left(String *s, size_t width, char pad_char) {
    if (!s || s->len >= width) return 0;

    size_t pad_count = width - s->len;
    if (string_ensure_capacity(s, width) != 0) return -1;

    my_memmove(s->data + pad_count, s->data, s->len);
    for (size_t i = 0; i < pad_count; i++) {
        s->data[i] = pad_char;
    }
    s->len = width;
    s->data[s->len] = '\0';

    return 0;
}

int string_pad_right(String *s, size_t width, char pad_char) {
    if (!s || s->len >= width) return 0;

    size_t pad_count = width - s->len;
    if (string_ensure_capacity(s, width) != 0) return -1;

    for (size_t i = 0; i < pad_count; i++) {
        s->data[s->len + i] = pad_char;
    }
    s->len = width;
    s->data[s->len] = '\0';

    return 0;
}

/* ============ Debug Functions ============ */

void string_dump(const String *s) {
    if (!s) {
        debug_write("String: NULL\n");
        return;
    }

    char buf[256];
    __builtin_snprintf(buf, sizeof(buf),
                      "=== String Info ===\n"
                      "Length: %lu\n"
                      "Capacity: %lu\n"
                      "Data: \"",
                      s->len, s->capacity);
    debug_write(buf);

    if (s->len > 0) {
        size_t write_len = s->len > 100 ? 100 : s->len;
        syscall(SYS_write, 1, s->data, write_len);
        if (s->len > 100) {
            debug_write("...");
        }
    }

    debug_write("\"\n===================\n");
}

int string_validate(const String *s) {
    if (!s) return -1;
    if (!s->data) return -1;
    if (s->len > s->capacity) return -1;
    if (s->data[s->len] != '\0') return -1;

    return 0;
}
