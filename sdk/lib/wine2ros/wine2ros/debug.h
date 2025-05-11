/*
 * PROJECT:     ReactOS Wine-To-ReactOS
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Reducing dependency on Wine
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#include <winnls.h>

#if DBG
    #ifndef __RELFILE__
        #define __RELFILE__ __FILE__
    #endif

    #define ERR_LEVEL   0x1
    #define TRACE_LEVEL 0x8

    #define WINE_DEFAULT_DEBUG_CHANNEL(x) \
        static PCSTR DbgDefaultChannel = #x; \
        static const PCSTR * const _DbgDefaultChannel_ = &DbgDefaultChannel;

    BOOL IntIsDebugChannelEnabled(_In_ PCSTR channel);

    ULONG __cdecl DbgPrint(_In_z_ _Printf_format_string_ PCSTR Format, ...);

    #define DBG_PRINT(ch, level, tag, fmt, ...) (void)( \
        (((level) == ERR_LEVEL) || IntIsDebugChannelEnabled(ch)) ? \
        (DbgPrint("(%s:%d) %s" fmt, __RELFILE__, __LINE__, (tag), ##__VA_ARGS__), FALSE) : TRUE \
    )

    #define TRACE_ON(ch) IntIsDebugChannelEnabled(#ch)
    #define ERR(fmt, ...)   DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "err: ",   fmt, ##__VA_ARGS__)
    #define WARN(fmt, ...)  DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "warn: ",  fmt, ##__VA_ARGS__)
    #define FIXME(fmt, ...) DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "fixme: ", fmt, ##__VA_ARGS__)
    #define TRACE(fmt, ...) DBG_PRINT(DbgDefaultChannel, TRACE_LEVEL, "",        fmt, ##__VA_ARGS__)

    #define UNIMPLEMENTED FIXME("%s is unimplemented", __FUNCTION__);

    PCSTR debugstr_an(_In_opt_ PCSTR pszA, _In_ INT cchA);
    PCSTR debugstr_wn(_In_opt_ PCWSTR pszW, _In_ INT cchW);
    PCSTR debugstr_guid(_In_opt_ const GUID *id);
    PCSTR wine_dbgstr_rect(_In_opt_ LPCRECT prc);
    PCSTR wine_dbg_sprintf(_In_ PCSTR format, ...);

    #define debugstr_a(pszA) debugstr_an((pszA), -1)
    #define debugstr_w(pszW) debugstr_wn((pszW), -1)
#else
    #define WINE_DEFAULT_DEBUG_CHANNEL(x)
    #define IntIsDebugChannelEnabled(channel) FALSE
    #define DBG_PRINT(ch, level)
    #define TRACE_ON(ch) FALSE
    #define ERR(fmt, ...)
    #define WARN(fmt, ...)
    #define FIXME(fmt, ...)
    #define TRACE(fmt, ...)
    #define UNIMPLEMENTED
    #define debugstr_an(pszA, cchA) ((PCSTR)NULL)
    #define debugstr_wn(pszW, cchW) ((PCSTR)NULL)
    #define debugstr_guid(id) ((PCSTR)NULL)
    #define wine_dbgstr_rect(prc) ((PCSTR)NULL)
    #define wine_dbg_sprintf(format, ... ) ((PCSTR)NULL)
    #define debugstr_a(pszA) ((PCSTR)NULL)
    #define debugstr_w(pszW) ((PCSTR)NULL)
#endif
