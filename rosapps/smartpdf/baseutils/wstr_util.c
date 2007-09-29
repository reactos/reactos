/* Written by Krzysztof Kowalczyk (http://blog.kowalczyk.info)
   The author disclaims copyright to this source code. */

/* The most basic things, including string handling functions */

#include "base_util.h"
#include "wstr_util.h"

#include <strsafe.h>

WCHAR *wstr_cat4(const WCHAR *str1, const WCHAR *str2, const WCHAR *str3, const WCHAR *str4)
{
    WCHAR *str;
    WCHAR *tmp;
    size_t str1_len = 0;
    size_t str2_len = 0;
    size_t str3_len = 0;
    size_t str4_len = 0;

    if (str1)
        str1_len = wstrlen(str1);
    if (str2)
        str2_len = wstrlen(str2);
    if (str3)
        str3_len = wstrlen(str3);
    if (str4)
        str4_len = wstrlen(str4);

    str = (WCHAR*)zmalloc((str1_len + str2_len + str3_len + str4_len + 1)*sizeof(WCHAR));
    if (!str)
        return NULL;

    tmp = str;
    if (str1) {
        memcpy(tmp, str1, str1_len*sizeof(WCHAR));
        tmp += str1_len;
    }
    if (str2) {
        memcpy(tmp, str2, str2_len*sizeof(WCHAR));
        tmp += str2_len;
    }
    if (str3) {
        memcpy(tmp, str3, str3_len*sizeof(WCHAR));
        tmp += str3_len;
    }
    if (str4) {
        memcpy(tmp, str4, str1_len*sizeof(WCHAR));
    }
    return str;
}

WCHAR *wstr_cat3(const WCHAR *str1, const WCHAR *str2, const WCHAR *str3)
{
    return wstr_cat4(str1, str2, str3, NULL);
}

WCHAR *wstr_cat(const WCHAR *str1, const WCHAR *str2)
{
    return wstr_cat4(str1, str2, NULL, NULL);
}

WCHAR *wstr_dupn(const WCHAR *str, int str_len_cch)
{
    WCHAR *copy;

    if (!str)
        return NULL;
    copy = (WCHAR*)malloc((str_len_cch+1)*sizeof(WCHAR));
    if (!copy)
        return NULL;
    memcpy(copy, str, str_len_cch*sizeof(WCHAR));
    copy[str_len_cch] = 0;
    return copy;
}

WCHAR *wstr_dup(const WCHAR *str)
{
    return wstr_cat4(str, NULL, NULL, NULL);
}

int wstr_copyn(WCHAR *dst, int dst_cch_size, const WCHAR *src, int src_cch_size)
{
    WCHAR *end = dst + dst_cch_size - 1;
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

int wstr_copy(WCHAR *dst, int dst_cch_size, const WCHAR *src)
{
    WCHAR *end = dst + dst_cch_size - 1;
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

int wstr_ieq(const WCHAR *str1, const WCHAR *str2)
{
    if (!str1 && !str2)
        return TRUE;
    if (!str1 || !str2)
        return FALSE;
    if (0 == _wcsicmp(str1, str2))
        return TRUE;
    return FALSE;
}

/* return true if 'str' starts with 'txt', case-sensitive */
int  wstr_startswith(const WCHAR *str, const WCHAR *txt)
{
    if (!str && !txt)
        return TRUE;
    if (!str || !txt)
        return FALSE;

    if (0 == wcsncmp(str, txt, wcslen(txt)))
        return TRUE;
    return FALSE;
}

/* return true if 'str' starts with 'txt', NOT case-sensitive */
int  wstr_startswithi(const WCHAR *str, const WCHAR *txt)
{
    if (!str && !txt)
        return TRUE;
    if (!str || !txt)
        return FALSE;

    if (0 == _wcsnicmp(str, txt, wcslen(txt)))
        return TRUE;
    return FALSE;
}

int wstr_empty(const WCHAR *str)
{
    if (!str)
        return TRUE;
    if (0 == *str)
        return TRUE;
    return FALSE;
}

static void wchar_to_hex(WCHAR c, WCHAR* buffer)
{
    const WCHAR* numbers = L"0123456789ABCDEF";
    buffer[0]=numbers[c / 16];
    buffer[1]=numbers[c % 16];
}

int wstr_contains(const WCHAR *str, WCHAR c)
{
    while (*str) {
        if (c == *str++)
            return TRUE;
    }
    return FALSE;
}

#define WCHAR_URL_DONT_ENCODE L"-_.!~*'()"

int wchar_needs_url_encode(WCHAR c)
{
    if ((c >= L'a') && (c <= L'z'))
        return FALSE;
    if ((c >= L'A') && (c <= L'Z'))
        return FALSE;
    if ((c >= L'0') && (c <= L'9'))
        return FALSE;
    if (wstr_contains(WCHAR_URL_DONT_ENCODE, c))
        return FALSE;
    return TRUE;
}

WCHAR *wstr_url_encode(const WCHAR *str)
{
    WCHAR *         encoded;
    WCHAR *         result;
    int             res_len = 0;
    const WCHAR *   tmp = str;

    while (*tmp) {
        if (wchar_needs_url_encode(*tmp))
            res_len += 3;
        else
            ++res_len;
        tmp++;
    }
    if (0 == res_len)
        return NULL;

    encoded = (WCHAR*)malloc((res_len+1)*sizeof(WCHAR));
    if (!encoded)
        return NULL;

    result = encoded;
    tmp = str;
    while (*tmp) {
        if (wchar_needs_url_encode(*tmp)) {
            *encoded++ = L'%';
            wchar_to_hex(*tmp, encoded);
            encoded += 2;
        } else {
            if (L' ' == *tmp)
                *encoded++ = L'+';
            else
                *encoded++ = *tmp;
        }
        tmp++;
    }
    *encoded = 0;
    return result;
}

WCHAR *wstr_printf(const WCHAR *format, ...)
{
    HRESULT     hr;
    va_list     args;
    WCHAR       message[256];
    WCHAR  *    buf;
    size_t      bufCchSize;

    buf = &(message[0]);
    bufCchSize = sizeof(message)/sizeof(message[0]);

    va_start(args, format);
    for (;;)
    {
        hr = StringCchVPrintfW(buf, bufCchSize, format, args);
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
        buf = (WCHAR *)malloc(bufCchSize*sizeof(WCHAR));
        if (NULL == buf)
            goto Error;
    }
    va_end(args);

    /* free the buffer if it was dynamically allocated */
    if (buf == &(message[0]))
        return wstr_dup(buf);

    return buf;
Error:
    if (buf != &(message[0]))
        free((void*)buf);

    return NULL;
}

