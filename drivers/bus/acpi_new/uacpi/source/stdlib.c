#include <uacpi/internal/stdlib.h>
#include <uacpi/internal/utilities.h>

#ifdef UACPI_USE_BUILTIN_STRING

#ifndef uacpi_memcpy
void *uacpi_memcpy(void *dest, const void *src, uacpi_size count)
{
    uacpi_char *cd = dest;
    const uacpi_char *cs = src;

    while (count--)
        *cd++ = *cs++;

    return dest;
}
#endif

#ifndef uacpi_memmove
void *uacpi_memmove(void *dest, const void *src, uacpi_size count)
{
    uacpi_char *cd = dest;
    const uacpi_char *cs = src;

    if (src < dest) {
        cs += count;
        cd += count;

        while (count--)
            *--cd = *--cs;
    } else {
        while (count--)
            *cd++ = *cs++;
    }

    return dest;
}
#endif

#ifndef uacpi_memset
void *uacpi_memset(void *dest, uacpi_i32 ch, uacpi_size count)
{
    uacpi_u8 fill = ch;
    uacpi_u8 *cdest = dest;

    while (count--)
        *cdest++ = fill;

    return dest;
}
#endif

#ifndef uacpi_memcmp
uacpi_i32 uacpi_memcmp(const void *lhs, const void *rhs, uacpi_size count)
{
    const uacpi_u8 *byte_lhs = lhs;
    const uacpi_u8 *byte_rhs = rhs;
    uacpi_size i;

    for (i = 0; i < count; ++i) {
        if (byte_lhs[i] != byte_rhs[i])
            return byte_lhs[i] - byte_rhs[i];
    }

    return 0;
}
#endif

#endif // UACPI_USE_BUILTIN_STRING

#ifndef uacpi_strlen
uacpi_size uacpi_strlen(const uacpi_char *str)
{
    const uacpi_char *str1;

    for (str1 = str; *str1; str1++);

    return str1 - str;
}
#endif

#ifndef UACPI_BAREBONES_MODE

#ifndef uacpi_strnlen
uacpi_size uacpi_strnlen(const uacpi_char *str, uacpi_size max)
{
    const uacpi_char *str1;

    for (str1 = str; max-- && *str1; str1++);

    return str1 - str;
}
#endif

#ifndef uacpi_strcmp
uacpi_i32 uacpi_strcmp(const uacpi_char *lhs, const uacpi_char *rhs)
{
    uacpi_size i = 0;
    typedef const uacpi_u8 *cucp;

    while (lhs[i] && rhs[i]) {
        if (lhs[i] != rhs[i])
            return *(cucp)&lhs[i] - *(cucp)&rhs[i];

        i++;
    }

    return *(cucp)&lhs[i] - *(cucp)&rhs[i];
}
#endif

void uacpi_memcpy_zerout(void *dst, const void *src,
    uacpi_size dst_size, uacpi_size src_size)
{
    uacpi_size bytes_to_copy = UACPI_MIN(src_size, dst_size);

    if (bytes_to_copy)
        uacpi_memcpy(dst, src, bytes_to_copy);

    if (dst_size > bytes_to_copy)
        uacpi_memzero((uacpi_u8 *)dst + bytes_to_copy, dst_size - bytes_to_copy);
}

uacpi_u8 uacpi_bit_scan_forward(uacpi_u64 value)
{
#if defined(_MSC_VER) && !defined(__clang__)
    unsigned char ret;
    unsigned long index;

#ifdef _WIN64
    ret = _BitScanForward64(&index, value);
    if (ret == 0)
        return 0;

    return (uacpi_u8)index + 1;
#else
    ret = _BitScanForward(&index, value);
    if (ret == 0) {
        ret = _BitScanForward(&index, value >> 32);
        if (ret == 0)
            return 0;

        return (uacpi_u8)index + 33;
    }

    return (uacpi_u8)index + 1;
#endif

#else
    return __builtin_ffsll(value);
#endif
}

uacpi_u8 uacpi_bit_scan_backward(uacpi_u64 value)
{
#if defined(_MSC_VER) && !defined(__clang__)
    unsigned char ret;
    unsigned long index;

#ifdef _WIN64
    ret = _BitScanReverse64(&index, value);
    if (ret == 0)
        return 0;

    return (uacpi_u8)index + 1;
#else
    ret = _BitScanReverse(&index, value >> 32);
    if (ret == 0) {
        ret = _BitScanReverse(&index, value);
        if (ret == 0)
            return 0;

        return (uacpi_u8)index + 1;
    }

    return (uacpi_u8)index + 33;
#endif

#else
    if (value == 0)
        return 0;

    return 64 - __builtin_clzll(value);
#endif
}

#ifndef UACPI_NATIVE_ALLOC_ZEROED
void *uacpi_builtin_alloc_zeroed(uacpi_size size)
{
    void *ptr;

    ptr = uacpi_kernel_alloc(size);
    if (uacpi_unlikely(ptr == UACPI_NULL))
        return ptr;

    uacpi_memzero(ptr, size);
    return ptr;
}
#endif

#endif // !UACPI_BAREBONES_MODE

#ifndef uacpi_vsnprintf
struct fmt_buf_state {
    uacpi_char *buffer;
    uacpi_size capacity;
    uacpi_size bytes_written;
};

struct fmt_spec {
    uacpi_u8 is_signed      : 1;
    uacpi_u8 prepend        : 1;
    uacpi_u8 uppercase      : 1;
    uacpi_u8 left_justify   : 1;
    uacpi_u8 alternate_form : 1;
    uacpi_u8 has_precision  : 1;
    uacpi_char pad_char;
    uacpi_char prepend_char;
    uacpi_u64 min_width;
    uacpi_u64 precision;
    uacpi_u32 base;
};

static void write_one(struct fmt_buf_state *fb_state, uacpi_char c)
{
    if (fb_state->bytes_written < fb_state->capacity)
        fb_state->buffer[fb_state->bytes_written] = c;

    fb_state->bytes_written++;
}

static void write_many(
    struct fmt_buf_state *fb_state, const uacpi_char *string, uacpi_size count
)
{
    if (fb_state->bytes_written < fb_state->capacity) {
        uacpi_size count_to_write;

        count_to_write = UACPI_MIN(
            count, fb_state->capacity - fb_state->bytes_written
        );
        uacpi_memcpy(
            &fb_state->buffer[fb_state->bytes_written], string, count_to_write
        );
    }

    fb_state->bytes_written += count;
}

static uacpi_char hex_char(uacpi_bool upper, uacpi_u64 value)
{
    static const uacpi_char upper_hex[] = "0123456789ABCDEF";
    static const uacpi_char lower_hex[] = "0123456789abcdef";

    return (upper ? upper_hex : lower_hex)[value];
}

static void write_padding(
    struct fmt_buf_state *fb_state, struct fmt_spec *fm, uacpi_size repr_size
)
{
    uacpi_u64 mw = fm->min_width;

    if (mw <= repr_size)
        return;

    mw -= repr_size;

    while (mw--)
        write_one(fb_state, fm->left_justify ? ' ' : fm->pad_char);
}

#define REPR_BUFFER_SIZE 32

static void write_integer(
    struct fmt_buf_state *fb_state, struct fmt_spec *fm, uacpi_u64 value
)
{
    uacpi_char repr_buffer[REPR_BUFFER_SIZE];
    uacpi_size index = REPR_BUFFER_SIZE;
    uacpi_u64 remainder;
    uacpi_char repr;
    uacpi_bool negative = UACPI_FALSE;
    uacpi_size repr_size;

    if (fm->is_signed) {
        uacpi_i64 as_ll = value;

        if (as_ll < 0) {
            value = -as_ll;
            negative = UACPI_TRUE;
        }
    }

    if (fm->prepend || negative)
        write_one(fb_state, negative ? '-' : fm->prepend_char);

    while (value) {
        remainder = value % fm->base;
        value /= fm->base;

        if (fm->base == 16) {
            repr = hex_char(fm->uppercase, remainder);
        } else if (fm->base == 8 || fm->base == 10) {
            repr = remainder + '0';
        } else {
            repr = '?';
        }

        repr_buffer[--index] = repr;
    }
    repr_size = REPR_BUFFER_SIZE - index;

    if (repr_size == 0) {
        repr_buffer[--index] = '0';
        repr_size = 1;
    }

    if (fm->alternate_form) {
        if (fm->base == 16) {
            repr_buffer[--index] = fm->uppercase ? 'X' : 'x';
            repr_buffer[--index] = '0';
            repr_size += 2;
        } else if (fm->base == 8) {
            repr_buffer[--index] = '0';
            repr_size += 1;
        }
    }

    if (fm->left_justify) {
        write_many(fb_state, &repr_buffer[index], repr_size);
        write_padding(fb_state, fm, repr_size);
    } else {
        write_padding(fb_state, fm, repr_size);
        write_many(fb_state, &repr_buffer[index], repr_size);
    }
}

static uacpi_bool string_has_at_least(
    const uacpi_char *string, uacpi_size characters
)
{
    while (*string) {
        if (--characters == 0)
            return UACPI_TRUE;

        string++;
    }

    return UACPI_FALSE;
}

static uacpi_bool consume_digits(
    const uacpi_char **string, uacpi_size *out_size
)
{
    uacpi_size size = 0;

    for (;;) {
        char c = **string;
        if (c < '0' || c > '9')
            break;

        size++;
        *string += 1;
    }

    if (size == 0)
        return UACPI_FALSE;

    *out_size = size;
    return UACPI_TRUE;
}

enum parse_number_mode {
    PARSE_NUMBER_MODE_MAYBE,
    PARSE_NUMBER_MODE_MUST,
};

static uacpi_bool parse_number(
    const uacpi_char **fmt, enum parse_number_mode mode, uacpi_u64 *out_value
)
{
    uacpi_status ret;
    uacpi_size num_digits;
    const uacpi_char *digits = *fmt;

    if (!consume_digits(fmt, &num_digits))
        return mode != PARSE_NUMBER_MODE_MUST;

    ret = uacpi_string_to_integer(digits, num_digits, UACPI_BASE_DEC, out_value);
    return ret == UACPI_STATUS_OK;
}

static uacpi_bool consume(const uacpi_char **string, const uacpi_char *token)
{
    uacpi_size token_size;

    token_size = uacpi_strlen(token);

    if (!string_has_at_least(*string, token_size))
        return UACPI_FALSE;

    if (!uacpi_memcmp(*string, token, token_size)) {
        *string += token_size;
        return UACPI_TRUE;
    }

    return UACPI_FALSE;
}

static uacpi_bool is_one_of(uacpi_char c, const uacpi_char *list)
{
    for (; *list; list++) {
        if (c == *list)
            return UACPI_TRUE;
    }

    return UACPI_FALSE;
}

static uacpi_bool consume_one_of(
    const uacpi_char **string, const uacpi_char *list, uacpi_char *consumed_char
)
{
    uacpi_char c = **string;
    if (!c)
        return UACPI_FALSE;

    if (is_one_of(c, list)) {
        *consumed_char = c;
        *string += 1;
        return UACPI_TRUE;
    }

    return UACPI_FALSE;
}

static uacpi_u32 base_from_specifier(uacpi_char specifier)
{
    switch (specifier)
    {
        case 'x':
        case 'X':
            return 16;
        case 'o':
            return 8;
        default:
            return 10;
    }
}

static uacpi_bool is_uppercase_specifier(uacpi_char specifier)
{
    return specifier == 'X';
}

static const uacpi_char *find_next_conversion(
    const uacpi_char *fmt, uacpi_size *offset
)
{
    *offset = 0;

    while (*fmt) {
        if (*fmt == '%')
            return fmt;

        fmt++;
        *offset += 1;
    }

    return UACPI_NULL;
}

uacpi_i32 uacpi_vsnprintf(
    uacpi_char *buffer, uacpi_size capacity, const uacpi_char *fmt,
    uacpi_va_list vlist
)
{
    struct fmt_buf_state fb_state = {
        .buffer = buffer,
        .capacity = capacity,
        .bytes_written = 0
    };

    uacpi_u64 value;
    const uacpi_char *next_conversion;
    uacpi_size next_offset;
    uacpi_char flag;

    while (*fmt) {
        struct fmt_spec fm = {
            .pad_char = ' ',
            .base = 10,
        };
        next_conversion = find_next_conversion(fmt, &next_offset);

        if (next_offset)
            write_many(&fb_state, fmt, next_offset);

        if (!next_conversion)
            break;

        fmt = next_conversion;
        if (consume(&fmt, "%%")) {
            write_one(&fb_state, '%');
            continue;
        }

        // consume %
        fmt++;

        while (consume_one_of(&fmt, "+- 0#", &flag)) {
            switch (flag) {
            case '+':
            case ' ':
                fm.prepend = UACPI_TRUE;
                fm.prepend_char = flag;
                continue;
            case '-':
                fm.left_justify = UACPI_TRUE;
                continue;
            case '0':
                fm.pad_char = '0';
                continue;
            case '#':
                fm.alternate_form = UACPI_TRUE;
                continue;
            default:
                return -1;
            }
        }

        if (consume(&fmt, "*")) {
            fm.min_width = uacpi_va_arg(vlist, int);
        } else if (!parse_number(&fmt, PARSE_NUMBER_MODE_MAYBE, &fm.min_width)) {
            return -1;
        }

        if (consume(&fmt, ".")) {
            fm.has_precision = UACPI_TRUE;

            if (consume(&fmt, "*")) {
                fm.precision = uacpi_va_arg(vlist, int);
            } else {
                if (!parse_number(&fmt, PARSE_NUMBER_MODE_MUST, &fm.precision))
                    return -1;
            }
        }

        flag = 0;

        if (consume(&fmt, "c")) {
            uacpi_char c = uacpi_va_arg(vlist, int);
            write_one(&fb_state, c);
            continue;
        }

        if (consume(&fmt, "s")) {
            const uacpi_char *string = uacpi_va_arg(vlist, uacpi_char*);
            uacpi_size i;

            if (uacpi_unlikely(string == UACPI_NULL))
                string = "<null>";

            for (i = 0; (!fm.has_precision || i < fm.precision) && string[i]; ++i)
                write_one(&fb_state, string[i]);
            while (i++ < fm.min_width)
                write_one(&fb_state, ' ');
            continue;
        }

        if (consume(&fmt, "p")) {
            value = (uacpi_uintptr)uacpi_va_arg(vlist, void*);
            fm.base = 16;
            fm.min_width = UACPI_POINTER_SIZE * 2;
            fm.pad_char = '0';
            goto write_int;
        }

        if (consume(&fmt, "hh")) {
            if (consume(&fmt, "d") || consume(&fmt, "i")) {
                value = (signed char)uacpi_va_arg(vlist, int);
                fm.is_signed = UACPI_TRUE;
            } else if (consume_one_of(&fmt, "oxXu", &flag)) {
                value = (unsigned char)uacpi_va_arg(vlist, int);
            } else {
                return -1;
            }
            goto write_int;
        }

        if (consume(&fmt, "h")) {
            if (consume(&fmt, "d") || consume(&fmt, "i")) {
                value = (signed short)uacpi_va_arg(vlist, int);
                fm.is_signed = UACPI_TRUE;
            } else if (consume_one_of(&fmt, "oxXu", &flag)) {
                value = (unsigned short)uacpi_va_arg(vlist, int);
            } else {
                return -1;
            }
            goto write_int;
        }

        if (consume(&fmt, "ll") ||
            (sizeof(uacpi_size) == sizeof(long long) && consume(&fmt, "z"))) {
            if (consume(&fmt, "d") || consume(&fmt, "i")) {
                value = uacpi_va_arg(vlist, long long);
                fm.is_signed = UACPI_TRUE;
            } else if (consume_one_of(&fmt, "oxXu", &flag)) {
                value = uacpi_va_arg(vlist, unsigned long long);
            } else {
                return -1;
            }
            goto write_int;
        }

        if (consume(&fmt, "l") ||
            (sizeof(uacpi_size) == sizeof(long) && consume(&fmt, "z"))) {
            if (consume(&fmt, "d") || consume(&fmt, "i")) {
                value = uacpi_va_arg(vlist, long);
                fm.is_signed = UACPI_TRUE;
            } else if (consume_one_of(&fmt, "oxXu", &flag)) {
                value = uacpi_va_arg(vlist, unsigned long);
            } else {
                return -1;
            }
            goto write_int;
        }

        if (consume(&fmt, "d") || consume(&fmt, "i")) {
            value = uacpi_va_arg(vlist, uacpi_i32);
            fm.is_signed = UACPI_TRUE;
        } else if (consume_one_of(&fmt, "oxXu", &flag)) {
            value = uacpi_va_arg(vlist, uacpi_u32);
        } else {
            return -1;
        }

    write_int:
        if (flag != 0) {
            fm.base = base_from_specifier(flag);
            fm.uppercase = is_uppercase_specifier(flag);
        }

        write_integer(&fb_state, &fm, value);
    }

    if (fb_state.capacity) {
        uacpi_size last_char;

        last_char = UACPI_MIN(fb_state.bytes_written, fb_state.capacity - 1);
        fb_state.buffer[last_char] = '\0';
    }

    return fb_state.bytes_written;
}
#endif

#ifndef uacpi_snprintf
uacpi_i32 uacpi_snprintf(
    uacpi_char *buffer, uacpi_size capacity, const uacpi_char *fmt, ...
)
{
    uacpi_va_list vlist;
    uacpi_i32 ret;

    uacpi_va_start(vlist, fmt);
    ret = uacpi_vsnprintf(buffer, capacity, fmt, vlist);
    uacpi_va_end(vlist);

    return ret;
}
#endif

#ifndef UACPI_FORMATTED_LOGGING
void uacpi_log(uacpi_log_level lvl, const uacpi_char *str, ...)
{
    uacpi_char buf[UACPI_PLAIN_LOG_BUFFER_SIZE];
    int ret;

    uacpi_va_list vlist;
    uacpi_va_start(vlist, str);

    ret = uacpi_vsnprintf(buf, sizeof(buf), str, vlist);
    if (uacpi_unlikely(ret < 0))
        return;

    /*
     * If this log message is too large for the configured buffer size, cut off
     * the end and transform into "...\n" to indicate that it didn't fit and
     * prevent the newline from being truncated.
     */
    if (uacpi_unlikely(ret >= UACPI_PLAIN_LOG_BUFFER_SIZE)) {
        buf[UACPI_PLAIN_LOG_BUFFER_SIZE - 5] = '.';
        buf[UACPI_PLAIN_LOG_BUFFER_SIZE - 4] = '.';
        buf[UACPI_PLAIN_LOG_BUFFER_SIZE - 3] = '.';
        buf[UACPI_PLAIN_LOG_BUFFER_SIZE - 2] = '\n';
    }

    uacpi_kernel_log(lvl, buf);

    uacpi_va_end(vlist);
}
#endif
