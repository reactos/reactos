/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */

/* The most basic things, including string handling functions */
#include "base_util.h"
#include "str_util.h"
#include "str_strsafe.h"

/* TODO: should probably be based on MSVC version */
#if defined(__GNUC__) || !defined(_WIN32) || (_MSC_VER < 1400)
void strcpy_s(char *dst, size_t dstLen, const char *src)
{
    size_t  toCopy;

    assert(dst);
    assert(src);
    assert(dstLen > 0);

    if (!dst || !src || dstLen <= 0)
        return;

    toCopy = strlen(src);
    if (toCopy > (dstLen-1))
        toCopy = dstLen - 1;

    strncpy(dst, src, toCopy);
    dst[toCopy] = 0;
}
#endif

void no_op(void)
{
    /* This really is a no-op, just to silence the compiler */
}

int char_is_ws_or_zero(char c)
{
    switch (c) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
        case 0:
            return TRUE;
    }
    return FALSE;
}

int char_is_ws(char c)
{
    switch (c) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            return TRUE;
    }
    return FALSE;
}

int char_is_digit(char c)
{
    if ((c >= '0') && (c <= '9'))
        return TRUE;
    return FALSE;
}

/* Concatenate 4 strings. Any string can be NULL.
   Caller needs to free() memory. */
char *str_cat4(const char *str1, const char *str2, const char *str3, const char *str4)
{
    char *str;
    char *tmp;
    size_t str1_len = 0;
    size_t str2_len = 0;
    size_t str3_len = 0;
    size_t str4_len = 0;

    if (str1)
        str1_len = strlen(str1);
    if (str2)
        str2_len = strlen(str2);
    if (str3)
        str3_len = strlen(str3);
    if (str4)
        str4_len = strlen(str4);

    str = (char*)zmalloc(str1_len + str2_len + str3_len + str4_len + 1);
    if (!str)
        return NULL;

    tmp = str;
    if (str1) {
        memcpy(tmp, str1, str1_len);
        tmp += str1_len;
    }
    if (str2) {
        memcpy(tmp, str2, str2_len);
        tmp += str2_len;
    }
    if (str3) {
        memcpy(tmp, str3, str3_len);
        tmp += str3_len;
    }
    if (str4) {
        memcpy(tmp, str4, str1_len);
    }
    return str;
}

/* Concatenate 3 strings. Any string can be NULL.
   Caller needs to free() memory. */
char *str_cat3(const char *str1, const char *str2, const char *str3)
{
    return str_cat4(str1, str2, str3, NULL);
}

/* Concatenate 2 strings. Any string can be NULL.
   Caller needs to free() memory. */
char *str_cat(const char *str1, const char *str2)
{
    return str_cat4(str1, str2, NULL, NULL);
}

char *str_dup(const char *str)
{
    return str_cat4(str, NULL, NULL, NULL);
}

char *str_dupn(const char *str, size_t str_len_cch)
{
    char *copy;

    if (!str)
        return NULL;
    copy = (char*)malloc(str_len_cch+1);
    if (!copy)
        return NULL;
    memcpy(copy, str, str_len_cch);
    copy[str_len_cch] = 0;
    return copy;
}

int str_copyn(char *dst, size_t dst_cch_size, const char *src, size_t src_cch_size)
{
    char *end = dst + dst_cch_size - 1;
    if (0 == dst_cch_size) {
        if (0 == src_cch_size)
            return TRUE;
        else
            return FALSE;
    }

    while ((dst < end) && (src_cch_size > 0)) {
        *dst++ = *src++;
        --src_cch_size;
    }
    *dst = 0;
    if (0 == src_cch_size)
        return TRUE;
    else
        return FALSE;
}

int str_copy(char *dst, size_t dst_cch_size, const char *src)
{
    char *end = dst + dst_cch_size - 1;
    if (0 == dst_cch_size)
        return FALSE;

    while ((dst < end) && *src) {
        *dst++ = *src++;
    }
    *dst = 0;
    if (0 == *src)
        return TRUE;
    else
        return FALSE;
}

int str_eq(const char *str1, const char *str2)
{
    if (!str1 && !str2)
        return TRUE;
    if (!str1 || !str2)
        return FALSE;
    if (0 == strcmp(str1, str2))
        return TRUE;
    return FALSE;
}

int str_ieq(const char *str1, const char *str2)
{
    if (!str1 && !str2)
        return TRUE;
    if (!str1 || !str2)
        return FALSE;
    if (0 == _stricmp(str1, str2))
        return TRUE;
    return FALSE;
}

int str_eqn(const char *str1, const char *str2, int len)
{
    if (!str1 && !str2)
        return TRUE;
    if (!str1 || !str2)
        return FALSE;
    if (0 == strncmp(str1, str2, len))
        return TRUE;
    return FALSE;
}

/* return true if 'str' starts with 'txt', case-sensitive */
int  str_startswith(const char *str, const char *txt)
{
    if (!str && !txt)
        return TRUE;
    if (!str || !txt)
        return FALSE;

    if (0 == strncmp(str, txt, strlen(txt)))
        return TRUE;
    return FALSE;
}

/* return true if 'str' starts with 'txt', NOT case-sensitive */
int  str_startswithi(const char *str, const char *txt)
{
    if (!str && !txt)
        return TRUE;
    if (!str || !txt)
        return FALSE;

    if (0 == _strnicmp(str, txt, strlen(txt)))
        return TRUE;
    return FALSE;
}

int str_endswith(const char *txt, const char *end)
{
    size_t end_len;
    size_t txt_len;

    if (!txt || !end)
        return FALSE;

    txt_len = strlen(txt);
    end_len = strlen(end);
    if (end_len > txt_len)
        return FALSE;
    if (str_eq(txt+txt_len-end_len, end))
        return TRUE;
    return FALSE;
}

int str_endswithi(const char *txt, const char *end)
{
    size_t end_len;
    size_t txt_len;

    if (!txt || !end)
        return FALSE;

    txt_len = strlen(txt);
    end_len = strlen(end);
    if (end_len > txt_len)
        return FALSE;
    if (str_ieq(txt+txt_len-end_len, end))
        return TRUE;
    return FALSE;
}

int str_endswith_char(const char *str, char c)
{
    char end[2];
    end[0] = c;
    end[1] = 0;
    return str_endswith(str, end);
}

int str_empty(const char *str)
{
    if (!str)
        return TRUE;
    if (0 == *str)
        return TRUE;
    return FALSE;
}

/* Find character 'c' in string 'txt'.
   Return pointer to this character or NULL if not found */
const char *str_find_char(const char *txt, char c)
{
    while (*txt != c) {
        if (0 == *txt)
            return NULL;
        ++txt;
    }
    return txt;
}

/* split a string '*txt' at the border character 'c'. Something like python's
   string.split() except called iteratively.
   Returns a copy of the string (must be free()d by the caller).
   Returns NULL to indicate there's no more items. */
char *str_split_iter(char **txt, char c)
{
    const char *tmp;
    const char *pos;
    char *result;

    tmp = (const char*)*txt;
    if (!tmp)
        return NULL;

    pos = str_find_char(tmp, c);
    if (pos) {
         result = str_dupn(tmp, (int)(pos-tmp));
         *txt = (char*)pos+1;
    } else {
        result = str_dup(tmp);
        *txt = NULL; /* next iteration will return NULL */
    }
    return result;
}

/* Replace all posible versions (Unix, Windows, Mac) of newline character
   with 'replace'. Returns newly allocated string with normalized newlines
   or NULL if error.
   Caller needs to free() the result */
char *str_normalize_newline(const char *txt, const char *replace)
{
    size_t          replace_len;
    char            c;
    char *          result;
    const char *    tmp;
    char *          tmp_out;
    size_t          result_len = 0;

    replace_len = strlen(replace);
    tmp = txt;
    for (;;) {
        c = *tmp++;
        if (!c)
            break;
        if (0xa == c) {
            /* a single 0xa => Unix */
            result_len += replace_len;
        } else if (0xd == c) {
            if (0xa == *tmp) {
                /* 0xd 0xa => dos */
                result_len += replace_len;
                ++tmp;
            }
            else {
                /* just 0xd => Mac */
                result_len += replace_len;
            }
        } else
            ++result_len;
    }

    if (0 == result_len)
        return NULL;

    result = (char*)malloc(result_len+1);
    if (!result)
        return NULL;
    tmp_out = result;
    for (;;) {
        c = *txt++;
        if (!c)
            break;
        if (0xa == c) {
            /* a single 0xa => Unix */
            memcpy(tmp_out, replace, replace_len);
            tmp_out += replace_len;
        } else if (0xd == c) {
            if (0xa == *txt) {
                /* 0xd 0xa => dos */
                memcpy(tmp_out, replace, replace_len);
                tmp_out += replace_len;
                ++txt;
            }
            else {
                /* just 0xd => Mac */
                memcpy(tmp_out, replace, replace_len);
                tmp_out += replace_len;
            }
        } else
            *tmp_out++ = c;
    }

    *tmp_out = 0;
    return result;
}

#define WHITE_SPACE_CHARS " \n\t\r"

/* Strip all 'to_strip' characters from the beginning of the string.
   Does stripping in-place */
void str_strip_left(char *txt, const char *to_strip)
{
    char *new_start = txt;
    char c;
    if (!txt || !to_strip)
        return;
    for (;;) {
        c = *new_start;
        if (0 == c)
            break;
        if (!str_contains(to_strip, c))
            break;
        ++new_start;
    }

    if (new_start != txt) {
        memmove(txt, new_start, strlen(new_start)+1);
    }
}

/* Strip white-space characters from the beginning of the string.
   Does stripping in-place */
void str_strip_ws_left(char *txt)
{
    str_strip_left(txt, WHITE_SPACE_CHARS);
}

void str_strip_right(char *txt, const char *to_strip)
{
    char * new_end;
    char   c;
    if (!txt || !to_strip)
        return;
    if (0 == *txt)
        return;
    /* point at the last character in the string */
    new_end = txt + strlen(txt) - 1;
    for (;;) {
        c = *new_end;
        if (!str_contains(to_strip, c))
            break;
        if (txt == new_end)
            break;
        --new_end;
    }
    if (str_contains(to_strip, *new_end))
        new_end[0] = 0;
    else
        new_end[1] = 0;
}

void str_strip_ws_right(char *txt)
{
    str_strip_right(txt, WHITE_SPACE_CHARS);
}

void str_strip_both(char *txt, const char *to_strip)
{
    str_strip_left(txt, to_strip);
    str_strip_right(txt, to_strip);
}

void str_strip_ws_both(char *txt)
{
    str_strip_ws_left(txt);
    str_strip_ws_right(txt);
}

#if 0
int utf8_eq(const utf8* str1, const utf8* str2)
{
    return str_eq(str1, str2);
}

int utf8_eqn(const utf8* str1, const utf8* str2, int len)
{
    return str_eqn(str1, str2, len);
}

int   utf8_copy(utf8 *dst, int dst_size_bytes, utf8* src)
{
    return str_copy(dst, dst_size_bytes, src);
}

utf8 *utf8_dup(const utf8 *str)
{
    return str_dup(str);
}

utf8 *utf8_cat4(const utf8 *str1, const utf8 *str2, const utf8 *str3, const utf8 *str4)
{
    return str_cat4(str1, str2, str3, str4);
}

utf8 *utf8_cat3(const utf8 *str1, const utf8 *str2, const utf8 *str3)
{
    return str_cat4(str1, str2, str3, NULL);
}

utf8 *utf8_cat(const utf8 *str1, const utf8 *str2)
{
    return str_cat4(str1, str2, NULL, NULL);
}

int utf8_endswith(const utf8 *str, const utf8 *end)
{
    return str_endswith(str, end);
}
#endif

#define  HEX_NUMBERS "0123456789ABCDEF"
static void char_to_hex(unsigned char c, char* buffer)
{
    buffer[0] = HEX_NUMBERS[c / 16];
    buffer[1] = HEX_NUMBERS[c % 16];
}

int str_contains(const char *str, char c)
{
    const char *pos = str_find_char(str, c);
    if (!pos)
        return FALSE;
    return TRUE;
}

#define CHAR_URL_DONT_ENCODE   "-_.!~*'()"

int char_needs_url_encode(char c)
{
    if ((c >= 'a') && (c <= 'z'))
        return FALSE;
    if ((c >= 'A') && (c <= 'Z'))
        return FALSE;
    if ((c >= '0') && (c <= '9'))
        return FALSE;
    if (str_contains(CHAR_URL_DONT_ENCODE, c))
        return FALSE;
    return TRUE;
}

/* url-encode 'str'. Returns NULL in case of error. Caller needs to free()
   the result */
char *str_url_encode(const char *str)
{
    char *          encoded;
    char *          result;
    int             res_len = 0;
    const char *    tmp = str;

    /* calc the size of the string after url encoding */
    while (*tmp) {
        if (char_needs_url_encode(*tmp))
            res_len += 3;
        else
            ++res_len;
        tmp++;
    }
    if (0 == res_len)
        return NULL;

    encoded = (char*)malloc(res_len+1);
    if (!encoded)
        return NULL;

    result = encoded;
    tmp = str;
    while (*tmp) {
        if (char_needs_url_encode(*tmp)) {
            *encoded++ = '%';
            char_to_hex(*tmp, encoded);
            encoded += 2;
        } else {
            if (' ' == *tmp)
                *encoded++ = '+';
            else
                *encoded++ = *tmp;
        }
        tmp++;
    }
    *encoded = 0;
    return result;
}

char *str_escape(const char *txt)
{
    /* TODO: */
    return str_dup(txt);
}

char *str_printf(const char *format, ...)
{
    char *result;
    va_list     args;
    va_start(args, format);
    result = str_printf_args(format, args);
    va_end(args);
    return result;
}

char *str_printf_args(const char *format, va_list args)
{
#ifdef _WIN32
    HRESULT     hr;
    char        message[256];
    char  *     buf;
    size_t      bufCchSize;
    char *      result = NULL;

    buf = &(message[0]);
    bufCchSize = sizeof(message);

    for (;;)
    {
        /* TODO: this only works on windows with recent C library */
        hr = StringCchVPrintfA(buf, bufCchSize, format, args);
        if (S_OK == hr)
            break;
        if (STRSAFE_E_INSUFFICIENT_BUFFER != hr)
        {
            /* any error other than buffer not big enough:
               a) should not happen
               b) means we give up */
            assert(FALSE);
            goto Error;
        }
        /* we have to make the buffer bigger. The algorithm used to calculate
           the new size is arbitrary (aka. educated guess) */
        if (buf != &(message[0]))
            free(buf);
        if (bufCchSize < 4*1024)
            bufCchSize += bufCchSize;
        else
            bufCchSize += 1024;
        buf = (char *)malloc(bufCchSize*sizeof(char));
        if (NULL == buf)
            goto Error;
    }

    /* free the buffer if it was dynamically allocated */
    if (buf == &(message[0]))
        return str_dup(buf);

    return buf;
Error:
    if (buf != &(message[0]))
        free((void*)buf);

    return NULL;
#else
    char*   buf;
    int     len = vasprintf(&buf, format, args);
    return buf;
#endif
}

#ifdef _WIN32
void win32_dbg_out(const char *format, ...)
{
    char        buf[4096];
    char *      p = buf;
    int         written;
    va_list     args;

    va_start(args, format);
    written = _vsnprintf(p,sizeof(buf), format, args);
/*    printf(buf);
    fflush(stdout); */
    OutputDebugStringA(buf);
    va_end(args);
}

void win32_dbg_out_hex(const char *dsc, const unsigned char *data, int dataLen)
{
    unsigned char    buf[64+1];
    unsigned char *  curPos;
    int              bufCharsLeft;

    if (dsc) win32_dbg_out(dsc); /* a bit dangerous if contains formatting codes */
    if (!data) return;

    bufCharsLeft = sizeof(buf)-1;
    curPos = buf;
    while (dataLen > 0) {
        if (bufCharsLeft <= 1) {
            *curPos = 0;
            win32_dbg_out((char*)buf);
            bufCharsLeft = sizeof(buf)-1;
            curPos = buf;
        }
        char_to_hex(*data, curPos);
        curPos += 2;
        bufCharsLeft -= 2;
        --dataLen;
        ++data;
    }

    if (curPos != buf) {
        *curPos = 0;
        win32_dbg_out(buf);
    }
    win32_dbg_out("\n");
}
#endif

/* Given a pointer to a string in '*txt', skip past whitespace in the string
   and put the result in '*txt' */
void str_skip_ws(char **txtInOut)
{
    char *cur;
    if (!txtInOut)
        return;
    cur = *txtInOut;
    if (!cur)
        return;
    while (char_is_ws(*cur)) {
        ++cur;
    }
    *txtInOut = cur;
}

char *str_parse_quoted(char **txt)
{
    char *      strStart;
    char *      strCopy;
    char *      cur;
    char *      dst;
    char        c;
    size_t      len;

    assert(txt);
    if (!txt) return NULL;
    strStart = *txt;
    assert(strStart);
    if (!strStart) return NULL;

    assert('"' == *strStart);
    /* TODO: rewrite as 2-phase logic so that counting and copying are always in sync */
    ++strStart;
    cur = strStart;
    len = 0;
    for (;;) {
        c = *cur;
        if ((0 == c) || ('"' == c))
            break;
        if ('\\' == c) {
            /* TODO: should I un-escape more than '"' ?
               I used to un-escape '\' as well, but it wasn't right and
               files with UNC path like "\\foo\file.pdf" failed to load */
            if ('"' == cur[1]) {
                ++cur;
                c = *cur;
            }
        }
        ++cur;
        ++len;
    }

    strCopy = (char*)malloc(len+1);
    if (!strCopy)
        return NULL;

    cur = strStart;
    dst = strCopy;
    for (;;) {
        c = *cur;
        if (0 == c)
            break;
        if ('"' == c) {
            ++cur;
            break;
        }
        if ('\\' == c) {
            /* TODO: should I un-escape more than '"' ?
               I used to un-escape '\' as well, but it wasn't right and
               files with UNC path like "\\foo\file.pdf" failed to load */
            if ('"' == cur[1]) {
                ++cur;
                c = *cur;
            }
        }
        *dst++ = c;
        ++cur;
    }
    *dst = 0;
    *txt = cur;
    return strCopy;
}

char *str_parse_non_quoted(char **txt)
{
    char *  cur;
    char *  strStart;
    char *  strCopy;
    char    c;
    size_t  strLen;

    strStart = *txt;
    assert(strStart);
    if (!strStart) return NULL;
    assert('"' != *strStart);
    cur = strStart;
    for (;;) {
        c = *cur;
        if (char_is_ws_or_zero(c))
            break;
        ++cur;
    }

    strLen = cur - strStart;
    assert(strLen > 0);
    strCopy = str_dupn(strStart, strLen);
    *txt = cur;
    return strCopy;
}

/* 'txt' is path that can be:
  - escaped, in which case it starts with '"', ends with '"' and each '"' that is part of the name is escaped
    with '\'
  - unescaped, in which case it start with != '"' and ends with ' ' or eol (0)
  This function extracts escaped or unescaped path from 'txt'. Returns NULL in case of error.
  Caller needs to free() the result. */
char *str_parse_possibly_quoted(char **txt)
{
    char *  cur;
    char *  str_copy;

    if (!txt)
        return NULL;
    cur = *txt;
    if (!cur)
        return NULL;

    str_skip_ws(&cur);
    if (0 == *cur)
        return NULL;
    if ('"' == *cur)
        str_copy = str_parse_quoted(&cur);
    else
        str_copy = str_parse_non_quoted(&cur);
    *txt = cur;
    return str_copy;
}

void  str_array_init(str_array *str_arr)
{
    assert(str_arr);
    if (!str_arr) return;
    memzero(str_arr, sizeof(str_array));
}

void str_array_free(str_array *str_arr)
{
    int i;

    assert(str_arr);
    if (!str_arr) return;

    for (i = 0; i < str_arr->items_count; i++)
        free(str_arr->items[i]);
    free(str_arr->items);
    str_array_init(str_arr);
}

void str_array_delete(str_array *str_arr)
{
    assert(str_arr);
    if (!str_arr) return;
    str_array_free(str_arr);
    free((void*)str_arr);
}

str_item *str_array_get(str_array *str_arr, int index)
{
    assert(str_arr);
    if (!str_arr) return NULL;
    assert(index >= 0);
    assert(index < str_arr->items_count);
    if ((index < 0) || (index >= str_arr->items_count))
        return NULL;
    return str_arr->items[index];
}

int str_array_get_count(str_array *str_arr)
{
    assert(str_arr);
    if (!str_arr) return 0;
    return str_arr->items_count;
}

/* Set one string at position 'index' in 'str_arr'. Space for the item
   must already be allocated. */
str_item *str_array_set(str_array *str_arr, int index, const char *str)
{
    str_item *  new_item;
    size_t      str_len_cch;

    assert(str_arr);
    if (!str_arr) return NULL;

    if (index >= str_arr->items_count)
        return NULL;

    str_len_cch = str_len(str);
    new_item = (str_item*)malloc(sizeof(str_item) + str_len_cch*sizeof(char));
    if (!new_item)
        return NULL;
    str_copy(new_item->str, str_len_cch+1, str);
    if (str_arr->items[index])
        free(str_arr->items[index]);
    str_arr->items[index] = new_item;
    return new_item;
}

#define STR_ARR_GROW_VALUE 32

/* make a generic array alloc */
str_item *str_array_add(str_array *str_arr, const char *str)
{
    str_item ** tmp;
    str_item *  new_item;
    void *      data;
    int         n;

    if (str_arr->items_count >= str_arr->items_allocated) {
        /* increase memory for items if necessary */
        n = str_arr->items_allocated + STR_ARR_GROW_VALUE;
        tmp = (str_item**)realloc(str_arr->items, n * sizeof(str_item *));
        if (!tmp)
            return NULL;
        str_arr->items = tmp;
        data = &(str_arr->items[str_arr->items_count]);
        memzero(data, STR_ARR_GROW_VALUE * sizeof(str_item *));
        str_arr->items_allocated = n;
    }
    str_arr->items_count++;
    new_item = str_array_set(str_arr, str_arr->items_count - 1, str);
    if (!new_item)
        --str_arr->items_count;
    return new_item;
}

int str_array_exists_no_case(str_array *str_arr, const char *str)
{
    int         count, i;
    str_item *  item;
    char *      item_str;

    if (!str_arr || !str)
        return FALSE;

    count = str_arr->items_count;
    for (i = 0; i < count; i++)
    {
        item = str_arr->items[i];
        item_str = item->str;
        if (str_ieq(str, item_str))
            return TRUE;
    }
    return FALSE;
}

str_item *str_array_add_no_dups(str_array *str_arr, const char *str)
{
    if (str_array_exists_no_case(str_arr, str))
        return NULL;

    return str_array_add(str_arr, str);
}
