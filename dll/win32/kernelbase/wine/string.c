/*
 * Copyright 2019 Nikolay Sivov for CodeWeavers
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "shlwapi.h"
#include "winternl.h"

#include "kernelbase.h"
#include "wine/debug.h"
#include "wine/exception.h"

WINE_DEFAULT_DEBUG_CHANNEL(string);

static BOOL char_compare(WORD ch1, WORD ch2, DWORD flags)
{
    char str1[3], str2[3];

    str1[0] = LOBYTE(ch1);
    if (IsDBCSLeadByte(str1[0]))
    {
        str1[1] = HIBYTE(ch1);
        str1[2] = '\0';
    }
    else
        str1[1] = '\0';

    str2[0] = LOBYTE(ch2);
    if (IsDBCSLeadByte(str2[0]))
    {
        str2[1] = HIBYTE(ch2);
        str2[2] = '\0';
    }
    else
        str2[1] = '\0';

    return CompareStringA(GetThreadLocale(), flags, str1, -1, str2, -1) - CSTR_EQUAL;
}

int WINAPI lstrcmpA( LPCSTR str1, LPCSTR str2 )
{
    if (!str1 && !str2) return 0;
    if (!str1) return -1;
    if (!str2) return 1;
    return CompareStringA( GetThreadLocale(), LOCALE_USE_CP_ACP, str1, -1, str2, -1 ) - 2;
}

int WINAPI lstrcmpW(LPCWSTR str1, LPCWSTR str2)
{
    if (!str1 && !str2) return 0;
    if (!str1) return -1;
    if (!str2) return 1;
    return CompareStringW( GetThreadLocale(), 0, str1, -1, str2, -1 ) - 2;
}

int WINAPI lstrcmpiA(LPCSTR str1, LPCSTR str2)
{
    if (!str1 && !str2) return 0;
    if (!str1) return -1;
    if (!str2) return 1;
    return CompareStringA( GetThreadLocale(), NORM_IGNORECASE|LOCALE_USE_CP_ACP, str1, -1, str2, -1 ) - 2;
}

int WINAPI lstrcmpiW(LPCWSTR str1, LPCWSTR str2)
{
    if (!str1 && !str2) return 0;
    if (!str1) return -1;
    if (!str2) return 1;
    return CompareStringW( GetThreadLocale(), NORM_IGNORECASE, str1, -1, str2, -1 ) - 2;
}

LPSTR WINAPI KERNELBASE_lstrcpynA( LPSTR dst, LPCSTR src, INT n )
{
    /* Note: this function differs from the UNIX strncpy, it _always_ writes
     * a terminating \0.
     *
     * Note: n is an INT but Windows treats it as unsigned, and will happily
     * copy a gazillion chars if n is negative.
     */
    __TRY
    {
        LPSTR d = dst;
        LPCSTR s = src;
        UINT count = n;

        while ((count > 1) && *s)
        {
            count--;
            *d++ = *s++;
        }
        if (count) *d = 0;
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    __ENDTRY
    return dst;
}

LPWSTR WINAPI KERNELBASE_lstrcpynW( LPWSTR dst, LPCWSTR src, INT n )
{
    /* Note: this function differs from the UNIX strncpy, it _always_ writes
     * a terminating \0
     *
     * Note: n is an INT but Windows treats it as unsigned, and will happily
     * copy a gazillion chars if n is negative.
     */
    __TRY
    {
        LPWSTR d = dst;
        LPCWSTR s = src;
        UINT count = n;

        while ((count > 1) && *s)
        {
            count--;
            *d++ = *s++;
        }
        if (count) *d = 0;
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    __ENDTRY
    return dst;
}

INT WINAPI KERNELBASE_lstrlenA( LPCSTR str )
{
    INT ret;
    __TRY
    {
        ret = strlen(str);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    __ENDTRY
    return ret;
}

INT WINAPI KERNELBASE_lstrlenW( LPCWSTR str )
{
    INT ret;
    __TRY
    {
        ret = wcslen(str);
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return 0;
    }
    __ENDTRY
    return ret;
}

DWORD WINAPI StrCmpCA(const char *str, const char *cmp)
{
    return lstrcmpA(str, cmp);
}

DWORD WINAPI StrCmpCW(const WCHAR *str, const WCHAR *cmp)
{
    return lstrcmpW(str, cmp);
}

DWORD WINAPI StrCmpICA(const char *str, const char *cmp)
{
    return lstrcmpiA(str, cmp);
}

DWORD WINAPI StrCmpICW(const WCHAR *str, const WCHAR *cmp)
{
    return lstrcmpiW(str, cmp);
}

DWORD WINAPI StrCmpNICA(const char *str, const char *cmp, DWORD len)
{
    return StrCmpNIA(str, cmp, len);
}

DWORD WINAPI StrCmpNICW(const WCHAR *str, const WCHAR *cmp, DWORD len)
{
    return StrCmpNIW(str, cmp, len);
}

char * WINAPI StrChrA(const char *str, WORD ch)
{
    TRACE("%s, %#x\n", wine_dbgstr_a(str), ch);

    if (!str)
        return NULL;

    while (*str)
    {
        if (!char_compare(*str, ch, 0))
            return (char *)str;
        str = CharNextA(str);
    }

    return NULL;
}

WCHAR * WINAPI StrChrW(const WCHAR *str, WCHAR ch)
{
    TRACE("%s, %#x\n", wine_dbgstr_w(str), ch);

    if (!str)
        return NULL;

    return wcschr(str, ch);
}

char * WINAPI StrChrIA(const char *str, WORD ch)
{
    TRACE("%s, %i\n", wine_dbgstr_a(str), ch);

    if (!str)
        return NULL;

    while (*str)
    {
        if (!ChrCmpIA(*str, ch))
            return (char *)str;
        str = CharNextA(str);
    }

    return NULL;
}

WCHAR * WINAPI StrChrIW(const WCHAR *str, WCHAR ch)
{
    TRACE("%s, %#x\n", wine_dbgstr_w(str), ch);

    if (!str)
        return NULL;

    ch = towupper(ch);
    while (*str)
    {
        if (towupper(*str) == ch)
            return (WCHAR *)str;
        str++;
    }
    str = NULL;

    return (WCHAR *)str;
}

WCHAR * WINAPI StrChrNW(const WCHAR *str, WCHAR ch, UINT max_len)
{
    TRACE("%s, %#x, %u\n", wine_dbgstr_wn(str, max_len), ch, max_len);

    if (!str)
        return NULL;

    while (*str && max_len-- > 0)
    {
        if (*str == ch)
            return (WCHAR *)str;
        str++;
    }

    return NULL;
}

char * WINAPI StrDupA(const char *str)
{
    unsigned int len;
    char *ret;

    TRACE("%s\n", wine_dbgstr_a(str));

    len = str ? strlen(str) + 1 : 1;
    ret = LocalAlloc(LMEM_FIXED, len);

    if (ret)
    {
        if (str)
            memcpy(ret, str, len);
        else
            *ret = '\0';
    }

    return ret;
}

WCHAR * WINAPI StrDupW(const WCHAR *str)
{
    unsigned int len;
    WCHAR *ret;

    TRACE("%s\n", wine_dbgstr_w(str));

    len = (str ? lstrlenW(str) + 1 : 1) * sizeof(WCHAR);
    ret = LocalAlloc(LMEM_FIXED, len);

    if (ret)
    {
        if (str)
            memcpy(ret, str, len);
        else
            *ret = '\0';
    }

    return ret;
}

BOOL WINAPI ChrCmpIA(WORD ch1, WORD ch2)
{
    TRACE("%#x, %#x\n", ch1, ch2);

    return char_compare(ch1, ch2, NORM_IGNORECASE);
}

BOOL WINAPI ChrCmpIW(WCHAR ch1, WCHAR ch2)
{
    return CompareStringW(GetThreadLocale(), NORM_IGNORECASE, &ch1, 1, &ch2, 1) - CSTR_EQUAL;
}

char * WINAPI StrStrA(const char *str, const char *search)
{
    const char *end;
    size_t len;

    TRACE("%s, %s\n", wine_dbgstr_a(str), wine_dbgstr_a(search));

    if (!str || !search || !*search) return NULL;

    len = strlen(search);
    end = str + strlen(str);

    while (str + len <= end)
    {
        if (!StrCmpNA(str, search, len)) return (char *)str;
        str = CharNextA(str);
    }
    return NULL;
}

WCHAR * WINAPI StrStrW(const WCHAR *str, const WCHAR *search)
{
    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(search));

    if (!str || !search || !*search)
        return NULL;

    return wcsstr(str, search);
}

WCHAR * WINAPI StrStrNW(const WCHAR *str, const WCHAR *search, UINT max_len)
{
    unsigned int i, len;

    TRACE("%s, %s, %u\n", wine_dbgstr_w(str), wine_dbgstr_w(search), max_len);

    if (!str || !search || !*search || !max_len)
        return NULL;

    len = lstrlenW(search);

    for (i = max_len; *str && (i > 0); i--, str++)
    {
        if (!wcsncmp(str, search, len))
            return (WCHAR *)str;
    }

    return NULL;
}

int WINAPI StrCmpNIA(const char *str, const char *cmp, int len)
{
    TRACE("%s, %s, %i\n", wine_dbgstr_a(str), wine_dbgstr_a(cmp), len);
    return CompareStringA(GetThreadLocale(), NORM_IGNORECASE, str, len, cmp, len) - CSTR_EQUAL;
}

WCHAR * WINAPI StrStrNIW(const WCHAR *str, const WCHAR *search, UINT max_len)
{
    unsigned int i, len;

    TRACE("%s, %s, %u\n", wine_dbgstr_w(str), wine_dbgstr_w(search), max_len);

    if (!str || !search || !*search || !max_len)
        return NULL;

    len = lstrlenW(search);

    for (i = max_len; *str && (i > 0); i--, str++)
    {
        if (!StrCmpNIW(str, search, len))
            return (WCHAR *)str;
    }

    return NULL;
}

int WINAPI StrCmpNA(const char *str, const char *comp, int len)
{
    TRACE("%s, %s, %i\n", wine_dbgstr_a(str), wine_dbgstr_a(comp), len);
    return CompareStringA(GetThreadLocale(), 0, str, len, comp, len) - CSTR_EQUAL;
}

int WINAPI StrCmpNW(const WCHAR *str, const WCHAR *comp, int len)
{
    TRACE("%s, %s, %i\n", wine_dbgstr_w(str), wine_dbgstr_w(comp), len);
    return CompareStringW(GetThreadLocale(), 0, str, len, comp, len) - CSTR_EQUAL;
}

DWORD WINAPI StrCmpNCA(const char *str, const char *comp, int len)
{
    return StrCmpNA(str, comp, len);
}

DWORD WINAPI StrCmpNCW(const WCHAR *str, const WCHAR *comp, int len)
{
    return StrCmpNW(str, comp, len);
}

int WINAPI StrCmpNIW(const WCHAR *str, const WCHAR *comp, int len)
{
    TRACE("%s, %s, %i\n", wine_dbgstr_w(str), wine_dbgstr_w(comp), len);
    return CompareStringW(GetThreadLocale(), NORM_IGNORECASE, str, len, comp, len) - CSTR_EQUAL;
}

int WINAPI StrCmpW(const WCHAR *str, const WCHAR *comp)
{
    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(comp));
    return CompareStringW(GetThreadLocale(), 0, str, -1, comp, -1) - CSTR_EQUAL;
}

int WINAPI StrCmpIW(const WCHAR *str, const WCHAR *comp)
{
    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(comp));
    return CompareStringW(GetThreadLocale(), NORM_IGNORECASE, str, -1, comp, -1) - CSTR_EQUAL;
}

WCHAR * WINAPI StrCpyNW(WCHAR *dst, const WCHAR *src, int count)
{
    const WCHAR *s = src;
    WCHAR *d = dst;

    TRACE("%p, %s, %i\n", dst, wine_dbgstr_w(src), count);

    if (s)
    {
        while ((count > 1) && *s)
        {
            count--;
            *d++ = *s++;
        }
    }
    if (count) *d = 0;

    return dst;
}

char * WINAPI StrStrIA(const char *str, const char *search)
{
    const char *end;
    size_t len;

    TRACE("%s, %s\n", wine_dbgstr_a(str), debugstr_a(search));

    if (!str || !search || !*search) return NULL;

    len = strlen(search);
    end = str + strlen(str);

    while (str + len <= end)
    {
        if (!StrCmpNIA(str, search, len)) return (char *)str;
        str = CharNextA(str);
    }
    return NULL;
}

WCHAR * WINAPI StrStrIW(const WCHAR *str, const WCHAR *search)
{
    unsigned int len;
    const WCHAR *end;

    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(search));

    if (!str || !search || !*search)
        return NULL;

    len = lstrlenW(search);
    end = str + lstrlenW(str);

    while (str + len <= end)
    {
        if (!StrCmpNIW(str, search, len))
            return (WCHAR *)str;
        str++;
    }

    return NULL;
}

int WINAPI StrSpnA(const char *str, const char *match)
{
    const char *ptr = str;

    TRACE("%s, %s\n", wine_dbgstr_a(str), wine_dbgstr_a(match));

    if (!str || !match) return 0;

    while (*ptr)
    {
        if (!StrChrA(match, *ptr)) break;
        ptr = CharNextA(ptr);
    }
    return ptr - str;
}

int WINAPI StrSpnW(const WCHAR *str, const WCHAR *match)
{
    if (!str || !match) return 0;
    return wcsspn(str, match);
}

int WINAPI StrCSpnA(const char *str, const char *match)
{
    const char *ptr = str;

    TRACE("%s, %s\n", wine_dbgstr_a(str), wine_dbgstr_a(match));

    if (!str || !match) return 0;

    while (*ptr)
    {
        if (StrChrA(match, *ptr)) break;
        ptr = CharNextA(ptr);
    }
    return ptr - str;
}

int WINAPI StrCSpnW(const WCHAR *str, const WCHAR *match)
{
    if (!str || !match)
        return 0;

    return wcscspn(str, match);
}

int WINAPI StrCSpnIA(const char *str, const char *match)
{
    const char *ptr = str;

    TRACE("%s, %s\n", wine_dbgstr_a(str), wine_dbgstr_a(match));

    if (!str || !match) return 0;

    while (*ptr)
    {
        if (StrChrIA(match, *ptr)) break;
        ptr = CharNextA(ptr);
    }
    return ptr - str;
}

int WINAPI StrCSpnIW(const WCHAR *str, const WCHAR *match)
{
    const WCHAR *ptr = str;

    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(match));

    if (!str || !*str || !match)
        return 0;

    while (*ptr)
    {
        if (StrChrIW(match, *ptr)) break;
        ptr++;
    }

    return ptr - str;
}

char * WINAPI StrRChrA(const char *str, const char *end, WORD ch)
{
    const char *ret = NULL;

    TRACE("%s, %s, %#x\n", wine_dbgstr_a(str), wine_dbgstr_a(end), ch);

    if (!str) return NULL;
    if (!end) end = str + lstrlenA(str);
    while (*str && str <= end)
    {
        WORD ch2 = IsDBCSLeadByte(*str) ? *str << 8 | str[1] : *str;
        if (!char_compare(ch, ch2, 0)) ret = str;
        str = CharNextA(str);
    }
    return (char *)ret;
}

WCHAR * WINAPI StrRChrW(const WCHAR *str, const WCHAR *end, WORD ch)
{
    WCHAR *ret = NULL;

    if (!str) return NULL;
    if (!end) end = str + lstrlenW(str);
    while (str < end)
    {
        if (*str == ch) ret = (WCHAR *)str;
        str++;
    }
    return ret;
}

char * WINAPI StrRChrIA(const char *str, const char *end, WORD ch)
{
    const char *ret = NULL;

    TRACE("%s, %s, %#x\n", wine_dbgstr_a(str), wine_dbgstr_a(end), ch);

    if (!str) return NULL;
    if (!end) end = str + lstrlenA(str);

    while (*str && str <= end)
    {
        WORD ch2 = IsDBCSLeadByte(*str) ? *str << 8 | str[1] : *str;
        if (!ChrCmpIA(ch, ch2)) ret = str;
        str = CharNextA(str);
    }
    return (char *)ret;
}

WCHAR * WINAPI StrRChrIW(const WCHAR *str, const WCHAR *end, WORD ch)
{
    WCHAR *ret = NULL;

    if (!str) return NULL;
    if (!end) end = str + lstrlenW(str);
    while (str < end)
    {
        if (!ChrCmpIW(*str, ch)) ret = (WCHAR *)str;
        str++;
    }
    return ret;
}

char * WINAPI StrRStrIA(const char *str, const char *end, const char *search)
{
    char *ret = NULL;
    WORD ch1, ch2;
    int len;

    TRACE("%s, %s\n", wine_dbgstr_a(str), wine_dbgstr_a(search));

    if (!str || !search || !*search)
        return NULL;

    if (IsDBCSLeadByte(*search))
        ch1 = *search << 8 | (UCHAR)search[1];
    else
        ch1 = *search;
    len = lstrlenA(search);

    if (!end)
        end = str + lstrlenA(str);
    else /* reproduce the broken behaviour on Windows */
        end += min(len - 1, lstrlenA(end));

    while (str + len <= end && *str)
    {
        ch2 = IsDBCSLeadByte(*str) ? *str << 8 | (UCHAR)str[1] : *str;
        if (!ChrCmpIA(ch1, ch2))
        {
            if (!StrCmpNIA(str, search, len))
                ret = (char *)str;
        }

        str = CharNextA(str);
    }

    return ret;
}

WCHAR * WINAPI StrRStrIW(const WCHAR *str, const WCHAR *end, const WCHAR *search)
{
    WCHAR *ret = NULL;
    int len;

    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(search));

    if (!str || !search || !*search)
        return NULL;

    len = lstrlenW(search);

    if (!end)
        end = str + lstrlenW(str);
    else
        end += min(len - 1, lstrlenW(end));

    while (str + len <= end && *str)
    {
        if (!ChrCmpIW(*search, *str))
        {
            if (!StrCmpNIW(str, search, len))
                ret = (WCHAR *)str;
        }
        str++;
    }

    return ret;
}

char * WINAPI StrPBrkA(const char *str, const char *match)
{
    TRACE("%s, %s\n", wine_dbgstr_a(str), wine_dbgstr_a(match));

    if (!str || !match || !*match)
        return NULL;

    while (*str)
    {
        if (StrChrA(match, *str))
            return (char *)str;
        str = CharNextA(str);
    }

    return NULL;
}

WCHAR * WINAPI StrPBrkW(const WCHAR *str, const WCHAR *match)
{
    if (!str || !match) return NULL;
    return wcspbrk(str, match);
}

BOOL WINAPI StrTrimA(char *str, const char *trim)
{
    unsigned int len;
    BOOL ret = FALSE;
    char *ptr = str;

    TRACE("%s, %s\n", debugstr_a(str), debugstr_a(trim));

    if (!str || !*str)
        return FALSE;

    while (*ptr && StrChrA(trim, *ptr))
        ptr = CharNextA(ptr); /* Skip leading matches */

    len = strlen(ptr);

    if (ptr != str)
    {
        memmove(str, ptr, len + 1);
        ret = TRUE;
    }

    if (len > 0)
    {
        ptr = str + len;
        while (StrChrA(trim, ptr[-1]))
            ptr = CharPrevA(str, ptr); /* Skip trailing matches */

        if (ptr != str + len)
        {
            *ptr = '\0';
            ret = TRUE;
        }
    }

    return ret;
}

BOOL WINAPI StrTrimW(WCHAR *str, const WCHAR *trim)
{
    unsigned int len;
    WCHAR *ptr = str;
    BOOL ret = FALSE;

    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(trim));

    if (!str || !*str)
        return FALSE;

    while (*ptr && StrChrW(trim, *ptr))
        ptr++;

    len = lstrlenW(ptr);

    if (ptr != str)
    {
        memmove(str, ptr, (len + 1) * sizeof(WCHAR));
        ret = TRUE;
    }

    if (len > 0)
    {
        ptr = str + len;
        while (StrChrW(trim, ptr[-1]))
            ptr--; /* Skip trailing matches */

        if (ptr != str + len)
        {
            *ptr = '\0';
            ret = TRUE;
        }
    }

    return ret;
}

BOOL WINAPI StrToInt64ExA(const char *str, DWORD flags, LONGLONG *ret)
{
    BOOL negative = FALSE;
    LONGLONG value = 0;

    TRACE("%s, %#lx, %p\n", wine_dbgstr_a(str), flags, ret);

    if (!str || !ret)
        return FALSE;

    if (flags > STIF_SUPPORT_HEX)
        WARN("Unknown flags %#lx\n", flags);

    /* Skip leading space, '+', '-' */
    while (*str == ' ' || *str == '\t' || *str == '\n') str++;

    if (*str == '-')
    {
        negative = TRUE;
        str++;
    }
    else if (*str == '+')
        str++;

    if (flags & STIF_SUPPORT_HEX && *str == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
        /* Read hex number */
        str += 2;

        if (!isxdigit(*str))
            return FALSE;

        while (isxdigit(*str))
        {
            value *= 16;
            if (*str >= '0' && *str <= '9')
                value += (*str - '0');
            else if (*str >= 'A' && *str <= 'F')
                value += 10 + *str - 'A';
            else
                value += 10 + *str - 'a';
            str++;
        }

        *ret = value;
        return TRUE;
    }

    /* Read decimal number */
    if (*str < '0' || *str > '9')
        return FALSE;

    while (*str >= '0' && *str <= '9')
    {
        value *= 10;
        value += (*str - '0');
        str++;
    }

    *ret = negative ? -value : value;
    return TRUE;
}

BOOL WINAPI StrToInt64ExW(const WCHAR *str, DWORD flags, LONGLONG *ret)
{
    BOOL negative = FALSE;
    LONGLONG value = 0;

    TRACE("%s, %#lx, %p\n", wine_dbgstr_w(str), flags, ret);

    if (!str || !ret)
        return FALSE;

    if (flags > STIF_SUPPORT_HEX)
        WARN("Unknown flags %#lx.\n", flags);

    /* Skip leading space, '+', '-' */
    while (*str == ' ' || *str == '\t' || *str == '\n') str++;

    if (*str == '-')
    {
        negative = TRUE;
        str++;
    }
    else if (*str == '+')
        str++;

    if (flags & STIF_SUPPORT_HEX && *str == '0' && (str[1] == 'x' || str[1] == 'X'))
    {
        /* Read hex number */
        str += 2;

        if (!isxdigit(*str))
            return FALSE;

        while (isxdigit(*str))
        {
            value *= 16;
            if (*str >= '0' && *str <= '9')
                value += (*str - '0');
            else if (*str >= 'A' && *str <= 'Z')
                value += 10 + (*str - 'A');
            else
                value += 10 + (*str - 'a');
            str++;
        }

        *ret = value;
        return TRUE;
    }

    /* Read decimal number */
    if (*str < '0' || *str > '9')
        return FALSE;

    while (*str >= '0' && *str <= '9')
    {
        value *= 10;
        value += (*str - '0');
        str++;
    }

    *ret = negative ? -value : value;
    return TRUE;
}

BOOL WINAPI StrToIntExA(const char *str, DWORD flags, INT *ret)
{
    LONGLONG value;
    BOOL res;

    TRACE("%s, %#lx, %p\n", wine_dbgstr_a(str), flags, ret);

    res = StrToInt64ExA(str, flags, &value);
    if (res) *ret = value;
    return res;
}

BOOL WINAPI StrToIntExW(const WCHAR *str, DWORD flags, INT *ret)
{
    LONGLONG value;
    BOOL res;

    TRACE("%s, %#lx, %p\n", wine_dbgstr_w(str), flags, ret);

    res = StrToInt64ExW(str, flags, &value);
    if (res) *ret = value;
    return res;
}

int WINAPI StrToIntA(const char *str)
{
    int value = 0;

    TRACE("%s\n", wine_dbgstr_a(str));

    if (!str)
        return 0;

    if (*str == '-' || (*str >= '0' && *str <= '9'))
        StrToIntExA(str, 0, &value);

    return value;
}

int WINAPI StrToIntW(const WCHAR *str)
{
    int value = 0;

    TRACE("%s\n", wine_dbgstr_w(str));

    if (!str)
        return 0;

    if (*str == '-' || (*str >= '0' && *str <= '9'))
        StrToIntExW(str, 0, &value);
    return value;
}

char * WINAPI StrCpyNXA(char *dst, const char *src, int len)
{
    TRACE("%p, %s, %i\n", dst, wine_dbgstr_a(src), len);

    if (dst && src && len > 0)
    {
        while ((len-- > 1) && *src)
            *dst++ = *src++;
        if (len >= 0)
            *dst = '\0';
    }

    return dst;
}

WCHAR * WINAPI StrCpyNXW(WCHAR *dst, const WCHAR *src, int len)
{
    TRACE("%p, %s, %i\n", dst, wine_dbgstr_w(src), len);

    if (dst && src && len > 0)
    {
        while ((len-- > 1) && *src)
            *dst++ = *src++;
        if (len >= 0)
            *dst = '\0';
    }

    return dst;
}

LPSTR WINAPI CharLowerA(char *str)
{
    if (IS_INTRESOURCE(str))
    {
        char ch = LOWORD(str);
        CharLowerBuffA( &ch, 1 );
        return (LPSTR)(UINT_PTR)(BYTE)ch;
    }

    __TRY
    {
        CharLowerBuffA( str, strlen(str) );
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError( ERROR_INVALID_PARAMETER );
        return NULL;
    }
    __ENDTRY
    return str;
}

DWORD WINAPI CharLowerBuffA(char *str, DWORD len)
{
    DWORD lenW;
    WCHAR buffer[32];
    WCHAR *strW = buffer;

    if (!str) return 0; /* YES */

    lenW = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
    if (lenW > ARRAY_SIZE(buffer))
    {
        strW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
        if (!strW) return 0;
    }
    MultiByteToWideChar(CP_ACP, 0, str, len, strW, lenW);
    CharLowerBuffW(strW, lenW);
    len = WideCharToMultiByte(CP_ACP, 0, strW, lenW, str, len, NULL, NULL);
    if (strW != buffer) HeapFree(GetProcessHeap(), 0, strW);
    return len;
}

DWORD WINAPI CharLowerBuffW(WCHAR *str, DWORD len)
{
    if (!str) return 0; /* YES */
    return LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_LOWERCASE, str, len, str, len);
}

LPWSTR WINAPI CharLowerW(WCHAR *str)
{
    if (!IS_INTRESOURCE(str))
    {
        CharLowerBuffW(str, lstrlenW(str));
        return str;
    }
    else
    {
        WCHAR ch = LOWORD(str);
        CharLowerBuffW(&ch, 1);
        return (LPWSTR)(UINT_PTR)ch;
    }
}

LPSTR WINAPI CharNextA(const char *ptr)
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByte( ptr[0] ) && ptr[1]) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}

LPSTR WINAPI CharNextExA(WORD codepage, const char *ptr, DWORD flags)
{
    if (!*ptr) return (LPSTR)ptr;
    if (IsDBCSLeadByteEx( codepage, ptr[0] ) && ptr[1]) return (LPSTR)(ptr + 2);
    return (LPSTR)(ptr + 1);
}

LPWSTR WINAPI CharNextW(const WCHAR *x)
{
    if (*x) x++;

    return (WCHAR *)x;
}

LPSTR WINAPI CharPrevA(const char *start, const char *ptr)
{
    while (*start && (start < ptr))
    {
        LPCSTR next = CharNextA(start);
        if (next >= ptr) break;
        start = next;
    }
    return (LPSTR)start;
}

LPSTR WINAPI CharPrevExA(WORD codepage, const char *start, const char *ptr, DWORD flags)
{
    while (*start && (start < ptr))
    {
        LPCSTR next = CharNextExA(codepage, start, flags);
        if (next >= ptr) break;
        start = next;
    }
    return (LPSTR)start;
}

LPWSTR WINAPI CharPrevW(const WCHAR *start, const WCHAR *x)
{
    if (x > start) return (LPWSTR)(x - 1);
    else return (LPWSTR)x;
}

LPSTR WINAPI CharUpperA(LPSTR str)
{
    if (IS_INTRESOURCE(str))
    {
        char ch = LOWORD(str);
        CharUpperBuffA(&ch, 1);
        return (LPSTR)(UINT_PTR)(BYTE)ch;
    }

    __TRY
    {
        CharUpperBuffA(str, strlen(str));
    }
    __EXCEPT_PAGE_FAULT
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return NULL;
    }
    __ENDTRY
    return str;
}

DWORD WINAPI CharUpperBuffA(LPSTR str, DWORD len)
{
    DWORD lenW;
    WCHAR buffer[32];
    WCHAR *strW = buffer;

    if (!str) return 0; /* YES */

    lenW = MultiByteToWideChar(CP_ACP, 0, str, len, NULL, 0);
    if (lenW > ARRAY_SIZE(buffer))
    {
        strW = HeapAlloc(GetProcessHeap(), 0, lenW * sizeof(WCHAR));
        if (!strW) return 0;
    }
    MultiByteToWideChar(CP_ACP, 0, str, len, strW, lenW);
    CharUpperBuffW(strW, lenW);
    len = WideCharToMultiByte(CP_ACP, 0, strW, lenW, str, len, NULL, NULL);
    if (strW != buffer) HeapFree(GetProcessHeap(), 0, strW);
    return len;
}

DWORD WINAPI CharUpperBuffW(WCHAR *str, DWORD len)
{
    if (!str) return 0; /* YES */
    return LCMapStringW(LOCALE_USER_DEFAULT, LCMAP_UPPERCASE, str, len, str, len);
}

LPWSTR WINAPI CharUpperW(WCHAR *str)
{
    if (!IS_INTRESOURCE(str))
    {
        CharUpperBuffW(str, lstrlenW(str));
        return str;
    }
    else
    {
        WCHAR ch = LOWORD(str);
        CharUpperBuffW(&ch, 1);
        return (LPWSTR)(UINT_PTR)ch;
    }
}

INT WINAPI DECLSPEC_HOTPATCH LoadStringW(HINSTANCE instance, UINT resource_id, LPWSTR buffer, INT buflen)
{
    int string_num, i;
    HGLOBAL hmem;
    HRSRC hrsrc;
    WCHAR *p;

    TRACE("instance = %p, id = %04x, buffer = %p, length = %d\n", instance, resource_id, buffer, buflen);

    if (!buffer)
        return 0;

    if (!(hrsrc = FindResourceW(instance, MAKEINTRESOURCEW((LOWORD(resource_id) >> 4) + 1), (LPWSTR)RT_STRING)) ||
        !(hmem = LoadResource(instance, hrsrc)))
    {
        TRACE( "Failed to load string.\n" );
        if (buflen > 0) buffer[0] = 0;
        return 0;
    }

    p = LockResource(hmem);
    string_num = resource_id & 0x000f;
    for (i = 0; i < string_num; i++)
        p += *p + 1;

    TRACE("strlen = %d\n", (int)*p );

    /*if buflen == 0, then return a read-only pointer to the resource itself in buffer
    it is assumed that buffer is actually a (LPWSTR *) */
    if (buflen == 0)
    {
        *((LPWSTR *)buffer) = p + 1;
        return *p;
    }

    i = min(buflen - 1, *p);
    memcpy(buffer, p + 1, i * sizeof(WCHAR));
    buffer[i] = 0;

    TRACE("returning %s\n", debugstr_w(buffer));
    return i;
}

INT WINAPI DECLSPEC_HOTPATCH LoadStringA(HINSTANCE instance, UINT resource_id, LPSTR buffer, INT buflen)
{
    DWORD retval = 0;
    HGLOBAL hmem;
    HRSRC hrsrc;

    TRACE("instance = %p, id = %04x, buffer = %p, length = %d\n", instance, resource_id, buffer, buflen);

    if (!buflen) return -1;

    /* Use loword (incremented by 1) as resourceid */
    if ((hrsrc = FindResourceW(instance, MAKEINTRESOURCEW((LOWORD(resource_id) >> 4) + 1), (LPWSTR)RT_STRING )) &&
            (hmem = LoadResource(instance, hrsrc)))
    {
        const WCHAR *p = LockResource(hmem);
        unsigned int id = resource_id & 0x000f;

        while (id--) p += *p + 1;

        RtlUnicodeToMultiByteN(buffer, buflen - 1, &retval, p + 1, *p * sizeof(WCHAR));
    }
    buffer[retval] = 0;
    TRACE("returning %s\n", debugstr_a(buffer));
    return retval;
}

int WINAPI StrCmpLogicalW(const WCHAR *str, const WCHAR *comp)
{
    TRACE("%s, %s\n", wine_dbgstr_w(str), wine_dbgstr_w(comp));

    if (!str || !comp)
        return 0;

    while (*str)
    {
        if (!*comp)
            return 1;
        else if (*str >= '0' && *str <= '9')
        {
            int str_value, comp_value;

            if (*comp < '0' || *comp > '9')
                return -1;

            /* Compare the numbers */
            StrToIntExW(str, 0, &str_value);
            StrToIntExW(comp, 0, &comp_value);

            if (str_value < comp_value)
                return -1;
            else if (str_value > comp_value)
                return 1;

            /* Skip */
            while (*str >= '0' && *str <= '9') str++;
            while (*comp >= '0' && *comp <= '9') comp++;
        }
        else if (*comp >= '0' && *comp <= '9')
            return 1;
        else
        {
            int diff = ChrCmpIW(*str, *comp);
            if (diff > 0)
                return 1;
            else if (diff < 0)
                return -1;

            str++;
            comp++;
        }
    }

    if (*comp)
      return -1;

    return 0;
}

BOOL WINAPI StrIsIntlEqualA(BOOL case_sensitive, const char *str, const char *cmp, int len)
{
    DWORD flags;

    TRACE("%d, %s, %s, %d\n", case_sensitive, wine_dbgstr_a(str), wine_dbgstr_a(cmp), len);

    /* FIXME: This flag is undocumented and unknown by our CompareString.
     *        We need a define for it.
     */
    flags = 0x10000000;
    if (!case_sensitive)
        flags |= NORM_IGNORECASE;

    return (CompareStringA(GetThreadLocale(), flags, str, len, cmp, len) == CSTR_EQUAL);
}

BOOL WINAPI StrIsIntlEqualW(BOOL case_sensitive, const WCHAR *str, const WCHAR *cmp, int len)
{
    DWORD flags;

    TRACE("%d, %s, %s, %d\n", case_sensitive, debugstr_w(str), debugstr_w(cmp), len);

    /* FIXME: This flag is undocumented and unknown by our CompareString.
     *        We need a define for it.
     */
    flags = 0x10000000;
    if (!case_sensitive)
        flags |= NORM_IGNORECASE;

    return (CompareStringW(GetThreadLocale(), flags, str, len, cmp, len) == CSTR_EQUAL);
}

char * WINAPI StrCatBuffA(char *str, const char *cat, INT max_len)
{
    INT len;

    TRACE("%p, %s, %d\n", str, wine_dbgstr_a(cat), max_len);

    if (!str)
        return NULL;

    len = strlen(str);
    max_len -= len;
    if (max_len > 0)
        StrCpyNA(str + len, cat, max_len);

    return str;
}

WCHAR * WINAPI StrCatBuffW(WCHAR *str, const WCHAR *cat, INT max_len)
{
    INT len;

    TRACE("%p, %s, %d\n", str, wine_dbgstr_w(cat), max_len);

    if (!str)
        return NULL;

    len = lstrlenW(str);
    max_len -= len;
    if (max_len > 0)
        StrCpyNW(str + len, cat, max_len);

    return str;
}

DWORD WINAPI StrCatChainW(WCHAR *str, DWORD max_len, DWORD at, const WCHAR *cat)
{
    TRACE("%s, %lu, %ld, %s\n", wine_dbgstr_w(str), max_len, at, wine_dbgstr_w(cat));

    if (at == -1)
        at = lstrlenW(str);

    if (!max_len)
        return at;

    if (at == max_len)
        at--;

    if (cat && at < max_len)
    {
        str += at;
        while (at < max_len - 1 && *cat)
        {
            *str++ = *cat++;
            at++;
        }
        *str = 0;
    }

    return at;
}

DWORD WINAPI SHTruncateString(char *str, DWORD size)
{
    char *last_byte;

    if (!str || !size)
        return 0;

    last_byte = str + size - 1;

    while (str < last_byte)
        str += IsDBCSLeadByte(*str) ? 2 : 1;

    if (str == last_byte && IsDBCSLeadByte(*str))
    {
        *str = '\0';
        size--;
    }

    return size;
}

HRESULT WINAPI SHLoadIndirectString(const WCHAR *src, WCHAR *dst, UINT dst_len, void **reserved)
{
    WCHAR *dllname = NULL;
    HMODULE hmod = NULL;
    HRESULT hr = E_FAIL;

    TRACE("%s, %p, %#x, %p\n", debugstr_w(src), dst, dst_len, reserved);

    if (src[0] == '@')
    {
        WCHAR *index_str;
        int index;

        dst[0] = 0;
        dllname = StrDupW(src + 1);
        index_str = wcschr(dllname, ',');

        if(!index_str) goto end;

        *index_str = 0;
        index_str++;
        index = wcstol(index_str, NULL, 10);

        hmod = LoadLibraryW(dllname);
        if (!hmod) goto end;

        if (index < 0)
        {
            if (LoadStringW(hmod, -index, dst, dst_len))
                hr = S_OK;
        }
        else
            FIXME("can't handle non-negative indices (%d)\n", index);
    }
    else
    {
        if (dst != src)
            lstrcpynW(dst, src, dst_len);
        hr = S_OK;
    }

    TRACE("returning %s\n", debugstr_w(dst));
end:
    if (hmod) FreeLibrary(hmod);
    LocalFree(dllname);
    return hr;
}
