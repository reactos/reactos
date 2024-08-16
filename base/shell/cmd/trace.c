/*
 * PROJECT:     ReactOS Command Shell
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Dynamic trace (generates debugging output to console output)
 * COPYRIGHT:   Copyright 2011 Hans Harder <hans@atbas.org>
 *              Copyright 2024 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#include "precomp.h"

#ifdef FEATURE_DYNAMIC_TRACE

BOOL g_bDynamicTrace = FALSE;

VOID CmdTrace(INT type, LPCSTR file, INT line, LPCSTR func, LPCSTR fmt, ...)
{
    va_list va;
    int cch;
    char szTextA[800];
#ifdef _UNICODE
    wchar_t szTextW[800];
#endif
    static struct __wine_debug_functions s_Debug;

    va_start(va, fmt);

    /* Console output */
    if (g_bDynamicTrace)
    {
        StringCchPrintfA(szTextA, _countof(szTextA), "%s (%d): ", file, line);
        cch = lstrlenA(szTextA);
        StringCchVPrintfA(&szTextA[cch], _countof(szTextA) - cch, fmt, va);

#ifdef _UNICODE
        MultiByteToWideChar(OutputCodePage, 0, szTextA, -1, szTextW, _countof(szTextW));
        szTextW[_countof(szTextW) - 1] = UNICODE_NULL; /* Avoid buffer overrun */
        ConOutPuts(szTextW);
#else
        ConOutPuts(szTextA);
#endif
    }

    if (!s_Debug.dbg_vlog)
        __wine_dbg_set_functions(NULL, &s_Debug, sizeof(s_Debug));

    /* Debug logging */
    switch (type)
    {
        case __WINE_DBCL_FIXME:
            if (__WINE_IS_DEBUG_ON(_FIXME, __wine_dbch___default))
                s_Debug.dbg_vlog(__WINE_DBCL_FIXME, __wine_dbch___default, file, func, line, fmt, va);
            break;
        case __WINE_DBCL_ERR:
            if (__WINE_IS_DEBUG_ON(_ERR, __wine_dbch___default))
                s_Debug.dbg_vlog(__WINE_DBCL_ERR, __wine_dbch___default, file, func, line, fmt, va);
            break;
        case __WINE_DBCL_WARN:
            if (__WINE_IS_DEBUG_ON(_WARN, __wine_dbch___default))
                s_Debug.dbg_vlog(__WINE_DBCL_WARN, __wine_dbch___default, file, func, line, fmt, va);
            break;
        case __WINE_DBCL_TRACE:
            if (__WINE_IS_DEBUG_ON(_TRACE, __wine_dbch___default))
                s_Debug.dbg_vlog(__WINE_DBCL_TRACE, __wine_dbch___default, file, func, line, fmt, va);
            break;
        default:
            break;
    }

    va_end(va);
}

#endif /* def FEATURE_DYNAMIC_TRACE */
