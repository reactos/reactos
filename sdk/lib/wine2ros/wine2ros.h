/*
 * PROJECT:     ReactOS Wine-To-ReactOS
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Reducing dependency on Wine
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

/* <wine/debug.h> */
#if DBG
    #ifndef __RELFILE__
        #define __RELFILE__ __FILE__
    #endif

    #define ERR_LEVEL   0x1
    #define TRACE_LEVEL 0x8

    #define WINE_DEFAULT_DEBUG_CHANNEL(x) static PCSTR DbgDefaultChannel = #x;

    BOOL IntIsDebugChannelEnabled(_In_ PCSTR channel);

    #define DBG_PRINT(ch, level, tag, fmt, ...) (void)( \
        (((level) == ERR_LEVEL) || IntIsDebugChannelEnabled(ch)) ? \
        (DbgPrint("(%s:%d) %s" fmt, __RELFILE__, __LINE__, (tag), ##__VA_ARGS__), FALSE) : TRUE \
    )

    #define ERR(fmt, ...)   DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "err: ",   fmt, ##__VA_ARGS__)
    #define WARN(fmt, ...)  DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "warn: ",  fmt, ##__VA_ARGS__)
    #define FIXME(fmt, ...) DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "fixme: ", fmt, ##__VA_ARGS__)
    #define TRACE(fmt, ...) DBG_PRINT(DbgDefaultChannel, TRACE_LEVEL, "",        fmt, ##__VA_ARGS__)

    #define UNIMPLEMENTED FIXME("%s is unimplemented", __FUNCTION__);

    PCSTR debugstr_a(_In_opt_ PCSTR pszA);
    PCSTR debugstr_w(_In_opt_ PCWSTR pszW);
    PCSTR debugstr_guid(_In_opt_ const GUID *id);
#else
    #define WINE_DEFAULT_DEBUG_CHANNEL(x)
    #define IntIsDebugChannelEnabled(channel) FALSE
    #define DBG_PRINT(ch, level)
    #define ERR(fmt, ...)
    #define WARN(fmt, ...)
    #define FIXME(fmt, ...)
    #define TRACE(fmt, ...)
    #define UNIMPLEMENTED
    #define debugstr_a(pszA) ((PCSTR)NULL)
    #define debugstr_w(pszW) ((PCSTR)NULL)
    #define debugstr_guid(id) ((PCSTR)NULL)
#endif

/* <wine/unicode.h> */
#define memicmpW(s1,s2,n) _wcsnicmp((s1),(s2),(n))
#define strlenW(s) wcslen((s))
#define strcpyW(d,s) wcscpy((d),(s))
#define strcatW(d,s) wcscat((d),(s))
#define strcspnW(d,s) wcscspn((d),(s))
#define strstrW(d,s) wcsstr((d),(s))
#define strtolW(s,e,b) wcstol((s),(e),(b))
#define strchrW(s,c) wcschr((s),(c))
#define strrchrW(s,c) wcsrchr((s),(c))
#define strncmpW(s1,s2,n) wcsncmp((s1),(s2),(n))
#define strncpyW(s1,s2,n) wcsncpy((s1),(s2),(n))
#define strcmpW(s1,s2) wcscmp((s1),(s2))
#define strcmpiW(s1,s2) _wcsicmp((s1),(s2))
#define strncmpiW(s1,s2,n) _wcsnicmp((s1),(s2),(n))
#define strtoulW(s1,s2,b) wcstoul((s1),(s2),(b))
#define strspnW(str, accept) wcsspn((str),(accept))
#define strpbrkW(str, accept) wcspbrk((str),(accept))
#define tolowerW(n) towlower((n))
#define toupperW(n) towupper((n))
#define islowerW(n) iswlower((n))
#define isupperW(n) iswupper((n))
#define isalphaW(n) iswalpha((n))
#define isalnumW(n) iswalnum((n))
#define isdigitW(n) iswdigit((n))
#define isxdigitW(n) iswxdigit((n))
#define isspaceW(n) iswspace((n))
#define iscntrlW(n) iswcntrl((n))
#define atoiW(s) _wtoi((s))
#define atolW(s) _wtol((s))
#define strlwrW(s) _wcslwr((s))
#define struprW(s) _wcsupr((s))
#define sprintfW _swprintf
#define vsprintfW _vswprintf
#define snprintfW _snwprintf
#define vsnprintfW _vsnwprintf
#define isprintW iswprint
