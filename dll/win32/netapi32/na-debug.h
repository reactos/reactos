/*
 * PROJECT:     ReactOS NETAPI32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Debugging NETAPI32
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <winnls.h>
#include <stdlib.h>
#include <objbase.h>

typedef enum tagDEBUGCHANNEL
{
    DbgCh_netapi32 = 0,
    DbgCh_netbios,
} DEBUGCHANNEL;

#if DBG
    #ifndef __RELFILE__
        #define __RELFILE__ __FILE__
    #endif

    #define ERR_LEVEL   0x1
    #define TRACE_LEVEL 0x8

    #define WINE_DEFAULT_DEBUG_CHANNEL(x) static int DbgDefaultChannel = DbgCh_##x;
    #define DBG_IS_CHANNEL_ENABLED(ch) IntIsDebugChannelEnabled(ch)

    #define DBG_PRINT(ch, level, tag, fmt, ...) (void)( \
        (((level) == ERR_LEVEL) || DBG_IS_CHANNEL_ENABLED(ch)) ? \
        (DbgPrint("(%s:%d) %s" fmt, __RELFILE__, __LINE__, (tag), ##__VA_ARGS__), FALSE) : TRUE \
    )

    #define ERR(fmt, ...)   DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "err: ",   fmt, ##__VA_ARGS__)
    #define WARN(fmt, ...)  DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "warn: ",  fmt, ##__VA_ARGS__)
    #define FIXME(fmt, ...) DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "fixme: ", fmt, ##__VA_ARGS__)
    #define TRACE(fmt, ...) DBG_PRINT(DbgDefaultChannel, TRACE_LEVEL, "",        fmt, ##__VA_ARGS__)

    #define UNIMPLEMENTED FIXME("%s is UNIMPLEMENTED\n", __FUNCTION__)
#else
    #define WINE_DEFAULT_DEBUG_CHANNEL(x)
    #define DBG_IS_CHANNEL_ENABLED(ch,level)
    #define DBG_PRINT(ch,level)
    #define ERR(fmt, ...)
    #define WARN(fmt, ...)
    #define FIXME(fmt, ...)
    #define TRACE(fmt, ...)
    #define UNIMPLEMENTED
#endif

#if DBG
static inline BOOL
IntIsDebugChannelEnabled(DEBUGCHANNEL channel)
{
    CHAR szValue[MAX_PATH], *pch0, *pch;

    if (!GetEnvironmentVariableA("DEBUGCHANNEL", szValue, _countof(szValue)))
        return FALSE;

    for (pch0 = szValue;; pch0 = pch + 1)
    {
        pch = strchr(pch0, ',');
        if (pch)
            *pch = ANSI_NULL;
        if (channel == DbgCh_netapi32 && _stricmp(pch0, "netapi32") == 0)
            return TRUE;
        if (channel == DbgCh_netbios && _stricmp(pch0, "netbios") == 0)
            return TRUE;
        if (!pch)
            return FALSE;
    }
}
#endif /* DBG */

#define DEBUG_ESCAPE_TAIL_LEN 5

static inline PSTR
debugstr_escape(PSTR pszBuf, INT cchBuf, PCSTR pszSrc)
{
    PCH pch = pszBuf;
    PCCH pchSrc = pszSrc;

    if (!pszSrc)
        return "(null)";

    *pch++ = '"';
    --cchBuf;

    for (; cchBuf > DEBUG_ESCAPE_TAIL_LEN; ++pchSrc)
    {
        switch (*pchSrc)
        {
            case '\'': case '\"': case '\\': case '\t': case '\r': case '\n':
                if (cchBuf <= DEBUG_ESCAPE_TAIL_LEN)
                    goto finish;
                *pch++ = '\\';
                if (*pchSrc == '\t')
                    *pch++ = 't';
                else if (*pchSrc == '\r')
                    *pch++ = 'r';
                else if (*pchSrc == '\n')
                    *pch++ = 'n';
                else
                    *pch++ = *pchSrc;
                cchBuf -= 4;
                break;
            default:
            {
                if (*pchSrc >= ' ')
                {
                    *pch++ = *pchSrc;
                    --cchBuf;
                }
                else
                {
                    if (cchBuf <= DEBUG_ESCAPE_TAIL_LEN)
                        goto finish;
                    *pch++ = '\\';
                    *pch++ = 'x';
                    *pch++ = "0123456789ABCDEF"[*pchSrc & 0xF];
                    *pch++ = "0123456789ABCDEF"[(*pchSrc >> 4) & 0xF];
                    cchBuf -= 4;
                }
                break;
            }
        }
    }

finish:
    if (cchBuf <= DEBUG_ESCAPE_TAIL_LEN)
    {
        *pch++ = '.';
        *pch++ = '.';
        *pch++ = '.';
    }
    *pch++ = '"';
    *pch = ANSI_NULL;
    return pszBuf;
}

#define DEBUG_BUF_SIZE MAX_PATH
#define DEBUG_NUM_BUFS 4

static inline PCSTR
debugstr_a(PCSTR pszA)
{
    static CHAR s_bufs[DEBUG_NUM_BUFS][DEBUG_BUF_SIZE];
    static SIZE_T s_index = 0;
    PCHAR ptr = debugstr_escape(s_bufs[s_index], _countof(s_bufs[s_index]), pszA);
    s_index = (s_index + 1) % _countof(s_bufs);
    return ptr;
}

static inline PCSTR
debugstr_w(PCWSTR pszW)
{
    static CHAR s_bufs[DEBUG_NUM_BUFS][DEBUG_BUF_SIZE];
    static SIZE_T s_index = 0;
    CHAR buf[DEBUG_BUF_SIZE];
    PCHAR ptr;

    if (!pszW)
        return "(null)";

    WideCharToMultiByte(CP_ACP, 0, pszW, -1, buf, _countof(buf), NULL, NULL);
    buf[_countof(buf) - 1] = ANSI_NULL;

    ptr = debugstr_escape(s_bufs[s_index], _countof(s_bufs[s_index]), buf);
    s_index = (s_index + 1) % _countof(s_bufs);
    return ptr;
}

static inline PCSTR
debugstr_guid(const GUID *guid)
{
    static CHAR s_bufs[DEBUG_NUM_BUFS][50];
    WCHAR szW[50];
    static SIZE_T s_index = 0;
    CHAR buf[DEBUG_BUF_SIZE];
    PCHAR ptr;

    if (!guid)
        return "(null)";

    StringFromGUID2(guid, szW, _countof(szW));
    WideCharToMultiByte(CP_ACP, 0, szW, -1, buf, _countof(s_bufs[0]), NULL, NULL);
    s_bufs[s_index][_countof(s_bufs[0]) - 1] = ANSI_NULL;

    ptr = s_bufs[s_index];
    s_index = (s_index + 1) % _countof(s_bufs);
    return ptr;
}
