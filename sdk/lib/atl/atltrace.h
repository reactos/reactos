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
#include <crtdbg.h>

extern "C"
{
// FIXME: Enabling _DEBUG at top level causes assertion failures...
int __cdecl _CrtDbgReport(int reportType, const char *filename, int linenumber, const char *moduleName, const char *format, ...);
int __cdecl _CrtDbgReportW(int reportType, const wchar_t *filename, int linenumber, const wchar_t *moduleName, const wchar_t *format, ...);
}

namespace ATL
{

template <UINT t_category = (1 << 19), UINT t_level = 0>
class CTraceCategoryEx
{
public:
    enum
    {
        TraceGeneral = (1 << 0),
        TraceCom = (1 << 1),
        TraceQI = (1 << 2),
        TraceRegistrar = (1 << 3),
        TraceRefcount = (1 << 4),
        TraceWindowing = (1 << 5),
        TraceControls = (1 << 6),
        TraceHosting = (1 << 7),
        TraceDBClient = (1 << 8),
        TraceDBProvider = (1 << 9),
        TraceSnapin = (1 << 10),
        TraceNotImpl = (1 << 11),
        TraceAllocation = (1 << 12),
        TraceException = (1 << 13),
        TraceTime = (1 << 14),
        TraceCache = (1 << 15),
        TraceStencil = (1 << 16),
        TraceString = (1 << 17),
        TraceMap = (1 << 18),
        TraceUtil = (1 << 19),
        TraceSecurity = (1 << 20),
        TraceSync = (1 << 21),
        TraceISAPI = (1 << 22),
        TraceUser = TraceUtil
    };

    CTraceCategoryEx(LPCTSTR name = NULL) : m_name(name)
    {
    }

    static UINT GetLevel()      { return t_level; }
    static UINT GetCategory()   { return t_category; }
    operator UINT() const       { return GetCategory(); }
    LPCTSTR GetCategoryName() const { return m_name; }

protected:
    LPCTSTR m_name;
};

class CTraceCategory : public CTraceCategoryEx<>
{
    CTraceCategory(LPCTSTR name = NULL) : CTraceCategoryEx<>(name)
    {
    }
};

#define DEFINE_TRACE_CATEGORY(name, cat) extern const DECLSPEC_SELECTANY CTraceCategoryEx<cat, 0> name(TEXT(#name))
DEFINE_TRACE_CATEGORY(atlTraceGeneral,    CTraceCategoryEx<>::TraceGeneral);
DEFINE_TRACE_CATEGORY(atlTraceCOM,        CTraceCategoryEx<>::TraceCom);
DEFINE_TRACE_CATEGORY(atlTraceQI,         CTraceCategoryEx<>::TraceQI);
DEFINE_TRACE_CATEGORY(atlTraceRegistrar,  CTraceCategoryEx<>::TraceRegistrar);
DEFINE_TRACE_CATEGORY(atlTraceRefcount,   CTraceCategoryEx<>::TraceRefcount);
DEFINE_TRACE_CATEGORY(atlTraceWindowing,  CTraceCategoryEx<>::TraceWindowing);
DEFINE_TRACE_CATEGORY(atlTraceControls,   CTraceCategoryEx<>::TraceControls);
DEFINE_TRACE_CATEGORY(atlTraceHosting,    CTraceCategoryEx<>::TraceHosting);
DEFINE_TRACE_CATEGORY(atlTraceDBClient,   CTraceCategoryEx<>::TraceDBClient);
DEFINE_TRACE_CATEGORY(atlTraceDBProvider, CTraceCategoryEx<>::TraceDBProvider);
DEFINE_TRACE_CATEGORY(atlTraceSnapin,     CTraceCategoryEx<>::TraceSnapin);
DEFINE_TRACE_CATEGORY(atlTraceNotImpl,    CTraceCategoryEx<>::TraceNotImpl);
DEFINE_TRACE_CATEGORY(atlTraceAllocation, CTraceCategoryEx<>::TraceAllocation);
DEFINE_TRACE_CATEGORY(atlTraceException,  CTraceCategoryEx<>::TraceException);
DEFINE_TRACE_CATEGORY(atlTraceTime,       CTraceCategoryEx<>::TraceTime);
DEFINE_TRACE_CATEGORY(atlTraceCache,      CTraceCategoryEx<>::TraceCache);
DEFINE_TRACE_CATEGORY(atlTraceStencil,    CTraceCategoryEx<>::TraceStencil);
DEFINE_TRACE_CATEGORY(atlTraceString,     CTraceCategoryEx<>::TraceString);
DEFINE_TRACE_CATEGORY(atlTraceMap,        CTraceCategoryEx<>::TraceMap);
DEFINE_TRACE_CATEGORY(atlTraceUtil,       CTraceCategoryEx<>::TraceUtil);
DEFINE_TRACE_CATEGORY(atlTraceSecurity,   CTraceCategoryEx<>::TraceSecurity);
DEFINE_TRACE_CATEGORY(atlTraceSync,       CTraceCategoryEx<>::TraceSync);
DEFINE_TRACE_CATEGORY(atlTraceISAPI,      CTraceCategoryEx<>::TraceISAPI);
#undef DEFINE_TRACE_CATEGORY

struct CTraceCategoryEasy
{
    UINT m_category;
    UINT m_level;
    LPCTSTR m_name;

    template <UINT t_category, UINT t_level>
    CTraceCategoryEasy(const CTraceCategoryEx<t_category, t_level>& cat)
    {
        m_category = t_category;
        m_level = t_level;
        m_name = cat.GetCategoryName();
    }

    operator UINT() const { return m_category; }

    BOOL IsGeneral() const
    {
        return lstrcmp(m_name, TEXT("atlTraceGeneral")) == 0;
    }
};

struct CTrace
{
    enum
    {
        DefaultTraceLevel = 0,
        DisableTracing = 0xFFFFFFFF,
        EnableAllCategories = 0xFFFFFFFF
    };

    static UINT GetLevel()                     { return s_level; }
    static UINT GetCategories()                { return s_categories; }
    static void SetLevel(UINT level)           { s_level = level; }
    static void SetCategories(UINT categories) { s_categories = categories; }

    static bool IsTracingEnabled(UINT category, UINT level)
    {
        return (s_level != DisableTracing && s_level >= level && (s_categories & category));
    }

protected:
    static UINT s_categories;
    static UINT s_level;
};

DECLSPEC_SELECTANY UINT CTrace::s_categories = CTrace::EnableAllCategories;
DECLSPEC_SELECTANY UINT CTrace::s_level      = CTrace::DefaultTraceLevel;

template <typename X_CHAR>
inline VOID __stdcall
AtlTraceV(_In_opt_z_                     const X_CHAR *            file,
          _In_                           INT                       line,
          _In_                           const CTraceCategoryEasy& cat,
          _In_                           UINT                      level,
          _In_z_ _Printf_format_string_  PCSTR                     format,
          _In_                           va_list                   va)
{
    char szBuff[1024], szFile[MAX_PATH];
    size_t cch = 0;
    const BOOL bUnicode = (sizeof(TCHAR) == 2);

    if (!CTrace::IsTracingEnabled(cat, level))
        return;

#ifdef _STRSAFE_H_INCLUDED_
    StringCchPrintfA(szFile, _countof(szFile), ((sizeof(X_CHAR) == 2) ? "%ls" : "%hs"), file);
    if (!cat.IsGeneral())
    {
        StringCchPrintfA(szBuff, _countof(szBuff), (bUnicode ? "%ls - " : "%hs - "), cat.m_name);
        StringCchLengthA(szBuff, _countof(szBuff), &cch);
    }
    StringCchVPrintfA(&szBuff[cch], _countof(szBuff) - cch, format, va);
#else
    _snprintf(szFile, _countof(szFile), ((sizeof(X_CHAR) == 2) ? "%ls" : "%hs"), file);
    if (!cat.IsGeneral())
        cch = _snprintf(szBuff, _countof(szBuff), (bUnicode ? "%ls - " : "%hs - "), cat.m_name);
    _vsnprintf(&szBuff[cch], _countof(szBuff) - cch, format, va);
#endif

    _CrtDbgReport(_CRT_WARN, szFile, line, NULL, "%hs", szBuff);
}

template <typename X_CHAR>
inline VOID __stdcall
AtlTraceV(_In_opt_z_                     const X_CHAR *            file,
          _In_                           INT                       line,
          _In_                           const CTraceCategoryEasy& cat,
          _In_                           UINT                      level,
          _In_z_ _Printf_format_string_  PCWSTR                    format,
          _In_                           va_list                   va)
{
    WCHAR szBuff[1024], szFile[MAX_PATH];
    size_t cch = 0;
    const BOOL bUnicode = (sizeof(TCHAR) == 2);

    if (!CTrace::IsTracingEnabled(cat, level))
        return;

#ifdef _STRSAFE_H_INCLUDED_
    StringCchPrintfW(szFile, _countof(szFile), ((sizeof(X_CHAR) == 2) ? L"%ls" : L"%hs"), file);
    if (!cat.IsGeneral())
    {
        StringCchPrintfW(szBuff, _countof(szBuff), (bUnicode ? L"%ls - " : L"%hs - "), cat.m_name);
        StringCchLengthW(szBuff, _countof(szBuff), &cch);
    }
    StringCchVPrintfW(&szBuff[cch], _countof(szBuff) - cch, format, va);
#else
    _snwprintf(szFile, _countof(szFile), ((sizeof(X_CHAR) == 2) ? L"%ls" : L"%hs"), file);
    if (!cat.IsGeneral())
        cch = _snwprintf(szBuff, _countof(szBuff), (bUnicode ? L"%ls - " : L"%hs - "), cat.m_name);
    _vsnwprintf(&szBuff[cch], _countof(szBuff) - cch, format, va);
#endif

    _CrtDbgReportW(_CRT_WARN, szFile, line, NULL, L"%ls", szBuff);
}

template <typename X_CHAR, typename Y_CHAR>
inline VOID __cdecl
AtlTraceEx(_In_opt_z_                    const X_CHAR *            file,
           _In_                          INT                       line,
           _In_                          const CTraceCategoryEasy& cat,
           _In_                          UINT                      level,
           _In_z_ _Printf_format_string_ const Y_CHAR *            format,
           ...)
{
    va_list va;
    va_start(va, format);
    AtlTraceV(file, line, cat, level, format, va);
    va_end(va);
}

template <typename X_CHAR, typename Y_CHAR>
inline VOID __cdecl
AtlTraceEx(_In_opt_z_                    const X_CHAR *file,
           _In_                          INT           line,
           _In_z_ _Printf_format_string_ const Y_CHAR *format,
           ...)
{
    va_list va;
    va_start(va, format);
    AtlTraceV(file, line, atlTraceGeneral, 0, format, va);
    va_end(va);
}

inline VOID __stdcall
AtlTraceEx(_In_opt_z_ PCTSTR file,
           _In_       INT    line,
           _In_       DWORD  value)
{
    AtlTraceEx(file, line, TEXT("%ld (0x%lX)\n"), value, value);
}

template <typename X_CHAR>
inline VOID __cdecl
AtlTrace(_In_z_ _Printf_format_string_ const X_CHAR *format, ...)
{
    va_list va;
    va_start(va, format);
    AtlTraceV(NULL, -1, atlTraceGeneral, 0, format, va);
    va_end(va);
}

} // namespace ATL

#endif // DBG

#ifndef ATLTRACE
    #if DBG
        #define ATLTRACE(format, ...) ATL::AtlTraceEx(__FILE__, __LINE__, format, ##__VA_ARGS__)
    #else
        #define ATLTRACE(format, ...) ((void)0)
    #endif
#endif

#define ATLTRACE2 ATLTRACE

#if DBG
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
