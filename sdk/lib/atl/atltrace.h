/*
 * PROJECT:     ReactOS ATL
 * LICENSE:     LGPL-2.0-or-later (https://spdx.org/licenses/LGPL-2.0-or-later)
 * PURPOSE:     Providing ATLTRACE macro
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */
 
#pragma once

#include "atldef.h"

#ifdef NDEBUG
    #undef DBG
    #undef _DEBUG
#endif

#if DBG && !defined(_DEBUG)
    #define _DEBUG
#endif

#ifdef _DEBUG

#include <stdio.h>

namespace ATL
{

struct CTraceCategory
{
    explicit CTraceCategory(LPCSTR name, UINT level = 0) { }

    operator UINT() const { return (1 << 15); }
};

#define DEFINE_TRACE_CATEGORY(name, cat) extern const DECLSPEC_SELECTANY UINT name = cat
DEFINE_TRACE_CATEGORY(atlTraceGeneral,    (1 << 0));
DEFINE_TRACE_CATEGORY(atlTraceCOM,        (1 << 1));
DEFINE_TRACE_CATEGORY(atlTraceQI,         (1 << 2));
DEFINE_TRACE_CATEGORY(atlTraceRegistrar,  (1 << 3));
DEFINE_TRACE_CATEGORY(atlTraceRefcount,   (1 << 4));
DEFINE_TRACE_CATEGORY(atlTraceWindowing,  (1 << 5));
DEFINE_TRACE_CATEGORY(atlTraceControls,   (1 << 6));
DEFINE_TRACE_CATEGORY(atlTraceHosting,    (1 << 7));
DEFINE_TRACE_CATEGORY(atlTraceDBClient,   (1 << 8));
DEFINE_TRACE_CATEGORY(atlTraceDBProvider, (1 << 9));
DEFINE_TRACE_CATEGORY(atlTraceSnapin,     (1 << 10));
DEFINE_TRACE_CATEGORY(atlTraceNotImpl,    (1 << 11));
DEFINE_TRACE_CATEGORY(atlTraceAllocation, (1 << 12));
#undef DEFINE_TRACE_CATEGORY

struct CTrace
{
    enum
    {
        DefaultTraceLevel = 0,
        DisableTracing = 0xFFFFFFFF,
        EnableAllCategories = 0xFFFFFFFF
    };

    static UINT GetLevel()      { return s_level; }
    static UINT GetCategories() { return s_categories; }
    static void SetLevel(UINT level)           { s_level = level; }
    static void SetCategories(UINT categories) { s_categories = categories; }

    static bool IsTracingEnabled(UINT category, UINT level)
    {
        if (s_level == DisableTracing || s_level < level || !(s_categories & category))
            return false;
        return ::IsDebuggerPresent();
    }

protected:
    static UINT s_categories;
    static UINT s_level;
};

DECLSPEC_SELECTANY UINT CTrace::s_categories = CTrace::EnableAllCategories;
DECLSPEC_SELECTANY UINT CTrace::s_level      = CTrace::DefaultTraceLevel;

inline VOID __stdcall
AtlTraceV(_In_opt_z_                     PCSTR   file,
          _In_                           INT     line,
          _In_                           UINT    cat,
          _In_                           UINT    level,
          _In_z_ _Printf_format_string_  PCSTR   format,
          _In_                           va_list va)
{
    char szBuff[1024];
    size_t cch;

    if (!CTrace::IsTracingEnabled(cat, level))
        return;

#ifdef _STRSAFE_H_INCLUDED_
    StringCchPrintfA(szBuff, _countof(szBuff), "%s(%d) : ", file, line);
    StringCchLengthA(szBuff, _countof(szBuff), &cch);
    StringCchVPrintfA(&szBuff[cch], _countof(szBuff) - cch, format, va);
#else
    cch = _snprintf(szBuff, _countof(szBuff), "%s(%d) : ", file, line);
    _vsnprintf(&szBuff[cch], _countof(szBuff) - cch, format, va);
#endif

    OutputDebugStringA(szBuff);
}

inline VOID __stdcall
AtlTraceV(_In_opt_z_                     PCSTR   file,
          _In_                           INT     line,
          _In_                           UINT    cat,
          _In_                           UINT    level,
          _In_z_ _Printf_format_string_  PCWSTR  format,
          _In_                           va_list va)
{
    WCHAR szBuff[1024];
    size_t cch;

    if (!CTrace::IsTracingEnabled(cat, level))
        return;

#ifdef _STRSAFE_H_INCLUDED_
    StringCchPrintfW(szBuff, _countof(szBuff), L"%hs(%d) : ", file, line);
    StringCchLengthW(szBuff, _countof(szBuff), &cch);
    StringCchVPrintfW(&szBuff[cch], _countof(szBuff) - cch, format, va);
#else
    cch = _snwprintf(szBuff, _countof(szBuff), L"%hs(%d) : ", file, line);
    _vsnwprintf(&szBuff[cch], _countof(szBuff) - cch, format, va);
#endif

    OutputDebugStringW(szBuff);
}

template <typename T_CHAR>
inline VOID __cdecl
AtlTraceEx(_In_opt_z_                    PCSTR         file,
           _In_                          INT           line,
           _In_                          UINT          cat,
           _In_                          UINT          level,
           _In_z_ _Printf_format_string_ const T_CHAR *format,
           ...)
{
    va_list va;
    va_start(va, format);
    AtlTraceV(file, line, cat, level, format, va);
    va_end(va);
}

template <typename T_CHAR>
inline VOID __cdecl
AtlTraceEx(_In_opt_z_                    PCSTR         file,
           _In_                          INT           line,
           _In_z_ _Printf_format_string_ const T_CHAR *format,
           ...)
{
    va_list va;
    va_start(va, format);
    AtlTraceV(file, line, atlTraceGeneral, 0, format, va);
    va_end(va);
}

inline VOID __stdcall
AtlTraceEx(_In_opt_z_ PCSTR file,
           _In_       INT   line,
           _In_       DWORD value)
{
    AtlTraceEx(file, line, "%ld (0x%lX)\n", value, value);
}

template <typename T_CHAR>
inline VOID __cdecl
AtlTrace(_In_z_ _Printf_format_string_ const T_CHAR *format, ...)
{
    va_list va;
    va_start(va, format);
    AtlTraceV(NULL, -1, atlTraceGeneral, 0, format, va);
    va_end(va);
}

} // namespace ATL

#endif // def _DEBUG

#ifndef ATLTRACE
    #ifdef _DEBUG
        #define ATLTRACE(format, ...) ATL::AtlTraceEx(__FILE__, __LINE__, format, ##__VA_ARGS__)
    #else
        #define ATLTRACE(format, ...) ((void)0)
    #endif
#endif

#define ATLTRACE2 ATLTRACE

#ifdef _DEBUG
    #define ATLTRACENOTIMPL(funcname) do { \
        ATLTRACE(atlTraceNotImpl, 0, #funcname " is not implemented.\n"); \
        return E_NOTIMPL; \
    } while (0)
#else
    #define ATLTRACENOTIMPL(funcname) return E_NOTIMPL
#endif

#ifndef _ATL_NO_AUTOMATIC_NAMESPACE
using namespace ATL;
#endif //!_ATL_NO_AUTOMATIC_NAMESPACE
