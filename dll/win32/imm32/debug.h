/*
 * PROJECT:     ReactOS IMM32
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Debugging IMM32
 * COPYRIGHT:   Copyright 2025 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

typedef enum tagDEBUGCHANNEL
{
    DbgChimm = 0,
} DEBUGCHANNEL;

#if DBG
    #ifndef __RELFILE__
        #define __RELFILE__ __FILE__
    #endif

    #define ERR_LEVEL   0x1
    #define TRACE_LEVEL 0x8

    #define WINE_DEFAULT_DEBUG_CHANNEL(x) static int DbgDefaultChannel = DbgCh##x;
    #define DBG_IS_CHANNEL_ENABLED(ch) Imm32IsDebugChannelEnabled(ch)

    #define DBG_PRINT(ch, level, tag, fmt, ...) (void)( \
        (((level) == ERR_LEVEL) || DBG_IS_CHANNEL_ENABLED(ch)) ? \
        (DbgPrint("(%s:%d) %s" fmt, __RELFILE__, __LINE__, (tag), ##__VA_ARGS__), FALSE) : TRUE \
    )

    #define ERR(fmt, ...)   DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "err: ",   fmt, ##__VA_ARGS__)
    #define WARN(fmt, ...)  DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "warn: ",  fmt, ##__VA_ARGS__)
    #define FIXME(fmt, ...) DBG_PRINT(DbgDefaultChannel, ERR_LEVEL,   "fixme: ", fmt, ##__VA_ARGS__)
    #define TRACE(fmt, ...) DBG_PRINT(DbgDefaultChannel, TRACE_LEVEL, "",        fmt, ##__VA_ARGS__)
#else
    #define WINE_DEFAULT_DEBUG_CHANNEL(x)
    #define DBG_IS_CHANNEL_ENABLED(ch,level)
    #define DBG_PRINT(ch,level)
    #define ERR(fmt, ...)
    #define WARN(fmt, ...)
    #define FIXME(fmt, ...)
    #define TRACE(fmt, ...)
#endif

#if DBG
static inline BOOL
Imm32IsDebugChannelEnabled(DEBUGCHANNEL channel)
{
    CHAR szValue[MAX_PATH], *pch0, *pch;

    if (!GetEnvironmentVariableA("DEBUGCHANNEL", szValue, _countof(szValue)))
        return FALSE;

    for (pch0 = szValue;; pch0 = pch + 1)
    {
        pch = strchr(pch0, ',');
        if (pch)
            *pch = ANSI_NULL;
        if (channel == DbgChimm && _stricmp(pch0, "imm") == 0)
            return TRUE;
        if (!pch)
            return FALSE;
    }
}
#endif

/* #define UNEXPECTED() (ASSERT(FALSE), TRUE) */
#define UNEXPECTED() TRUE

/* Unexpected Condition Checkers */
#if DBG
    #define FAILED_UNEXPECTEDLY(hr) \
        (FAILED(hr) ? (ERR("FAILED(0x%08X)\n", hr), UNEXPECTED()) : FALSE)
    #define IS_NULL_UNEXPECTEDLY(p) \
        (!(p) ? (ERR("%s was NULL\n", #p), UNEXPECTED()) : FALSE)
    #define IS_ZERO_UNEXPECTEDLY(p) \
        (!(p) ? (ERR("%s was zero\n", #p), UNEXPECTED()) : FALSE)
    #define IS_TRUE_UNEXPECTEDLY(x) \
        ((x) ? (ERR("%s was %d\n", #x, (int)(x)), UNEXPECTED()) : FALSE)
    #define IS_ERROR_UNEXPECTEDLY(x) \
        ((x) != ERROR_SUCCESS ? (ERR("%s was %d\n", #x, (int)(x)), UNEXPECTED()) : FALSE)
#else
    #define FAILED_UNEXPECTEDLY(hr) FAILED(hr)
    #define IS_NULL_UNEXPECTEDLY(p) (!(p))
    #define IS_ZERO_UNEXPECTEDLY(p) (!(p))
    #define IS_TRUE_UNEXPECTEDLY(x) (x)
    #define IS_ERROR_UNEXPECTEDLY(x) ((x) != ERROR_SUCCESS)
#endif

#define IS_CROSS_THREAD_HIMC(hIMC)   IS_TRUE_UNEXPECTEDLY(Imm32IsCrossThreadAccess(hIMC))
#define IS_CROSS_PROCESS_HWND(hWnd)  IS_TRUE_UNEXPECTEDLY(Imm32IsCrossProcessAccess(hWnd))
