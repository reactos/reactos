/*
 * PROJECT:     ReactOS ATL
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Providing ATLTRACE macro
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
 
#pragma once

#include "atldef.h"
#if DBG
    #include <stdio.h>
#endif

#if DBG

namespace ATL
{

inline VOID __stdcall
AtlVTraceEx(_In_z_                 PCSTR   file,
            _In_                   INT     line,
            _Printf_format_string_ PCSTR   format,
            _In_                   va_list va)
{
    char szBuff[512];
    size_t cch;

    if (!IsDebuggerPresent())
        return;

#ifdef _STRSAFE_H_INCLUDED_
    StringCchPrintfA(szBuff, _countof(szBuff), "%s (%d): ", file, line);
    StringCchLengthA(szBuff, _countof(szBuff), &cch);
    StringCchVPrintfA(&szBuff[cch], _countof(szBuff) - cch, format, va);
#else
    cch = _snprintf(szBuff, _countof(szBuff), "%s (%d): ", file, line);
    _vsnprintf(&szBuff[cch], _countof(szBuff) - cch, format, va);
#endif

    OutputDebugStringA(szBuff);
    va_end(va);
}

inline VOID __cdecl
AtlTraceEx(_In_z_                 PCSTR file,
           _In_                   INT line,
           _Printf_format_string_ PCSTR format,
           ...)
{
    va_list va;
    va_start(va, format);
    AtlVTraceEx(file, line, format, va);
    va_end(va);
}

inline VOID __cdecl AtlTrace(_Printf_format_string_ PCSTR format, ...)
{
    va_list va;
    va_start(va, format);
    AtlVTraceEx("(null)", -1, format, va);
    va_end(va);
}

inline VOID __stdcall
AtlTraceEx(_In_z_ PCSTR file,
           _In_   INT   line,
           _In_   DWORD value)
{
    AtlTraceEx(file, line, "%ld (0x%lX)\n", value, value);
}

} // namespace ATL

#endif // DBG

#if DBG
    #define ATLTRACE(format, ...)      ATL::AtlTraceEx(__FILE__, __LINE__, format, ##__VA_ARGS__)
    #define ATLTRACENOTIMPL(funcname)  (ATLTRACE(#funcname " is not implemented.\n"), E_NOTIMPL)
#else
    #define ATLTRACE(format, ...)      ((void)0)
    #define ATLTRACENOTIMPL(funcname)  E_NOTIMPL
#endif

#define ATLTRACE2 ATLTRACE

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE
