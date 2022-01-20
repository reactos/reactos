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
AtlVTraceEx(PCSTR file, INT line, _In_z_ _Printf_format_string_ PCSTR format, va_list va)
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
AtlTraceEx(PCSTR file, INT line, _In_z_ _Printf_format_string_ PCSTR format, ...)
{
    va_list va;
    va_start(va, format);
    AtlVTraceEx(file, line, format, va);
    va_end(va);
}

inline VOID __stdcall AtlTraceEx(PCSTR file, INT line, DWORD value)
{
    AtlTraceEx(file, line, "%ld (0x%lX)\n", value, value);
}

inline VOID __cdecl AtlTrace(_In_z_ _Printf_format_string_ PCSTR format, ...)
{
    va_list va;
    va_start(va, format);
    AtlVTraceEx("(null)", -1, format, va);
    va_end(va);
}

} // namespace ATL

#endif // DBG

#ifndef ATLTRACE
    #if DBG
        #define ATLTRACE(format, ...) ATL::AtlTraceEx(__FILE__, __LINE__, format, ##__VA_ARGS__)
    #else
        #define ATLTRACE(format, ...)
    #endif
#endif

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE
