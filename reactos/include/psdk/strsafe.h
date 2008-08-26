#ifndef __STRSAFE_H_
#define __STRSAFE_H_

#include <stdlib.h>
#include <stdarg.h>

#if defined(STRSAFE_NO_CCH_FUNCTIONS) && defined(STRSAFE_NO_CB_FUNCTIONS)
#error Both STRSAFE_NO_CCH_FUNCTIONS and STRSAFE_NO_CB_FUNCTIONS are defined
#endif


#ifndef _HRESULT_DEFINED
#define _HRESULT_DEFINED
typedef long HRESULT;
#endif

typedef char * STRSAFE_LPSTR;
typedef const char * STRSAFE_LPCSTR;
typedef wchar_t * STRSAFE_LPWSTR;
typedef const wchar_t * STRSAFE_LPCWSTR;
typedef unsigned long STRSAFE_DWORD;

#define STRSAFE_PASS2

/* Implement Cb functions for ansi and unicode */
#define STRSAFE_CB
#define STRSAFE_CXX_CB(x)
#define STRSAFE_CXX_CCH(x) x *= sizeof(STRSAFE_TCHAR)
#define STRSAFE_UNICODE 0
# include <strsafe.h>
#undef STRSAFE_UNICODE
#define STRSAFE_UNICODE 1
# include <strsafe.h>
#undef STRSAFE_UNICODE
#undef STRSAFE_CXX
#undef STRSAFE_CB
#undef STRSAFE_CXX_CB
#undef STRSAFE_CXX_CCH

/* Implement Cch functions for ansi and unicode */
#define STRSAFE_CXX_CB(x) x /= sizeof(STRSAFE_TCHAR)
#define STRSAFE_CXX_CCH(x)
#define STRSAFE_UNICODE 0
# include <strsafe.h>
#undef STRSAFE_UNICODE
#define STRSAFE_UNICODE 1
# include <strsafe.h>
#undef STRSAFE_UNICODE
#undef STRSAFE_CXX_CB
#undef STRSAFE_CXX_CCH

#undef STRSAFE_PASS2

/* Now define the functions depending on UNICODE */
#if defined(UNICODE)

#define StringCbCat StringCbCatW
#define StringCbCatEx StringCbCatExW
#define StringCbCatN StringCbCatNW
#define StringCbCatNEx StringCbCatNExW
#define StringCbCopy StringCbCopyW
#define StringCbCopyEx StringCbCopyExW
#define StringCbCopyN StringCbCopyNW
#define StringCbCopyNEx StringCbCopyNExW
#define StringCbGets StringCbGetsW
#define StringCbGetsEx StringCbGetsExW
#define StringCbLength StringCbLengthW
#define StringCbPrintf StringCbPrintfW
#define StringCbPrintfEx StringCbPrintfExW
#define StringCbVPrintf StringCbVPrintfW
#define StringCbVPrintfEx StringCbVPrintfExW
#define StringCchCat StringCchCatW
#define StringCchCatEx StringCchCatExW
#define StringCchCatN StringCchCatNW
#define StringCchCatNEx StringCchCatNExW
#define StringCchCopy StringCchCopyW
#define StringCchCopyEx StringCchCopyExW
#define StringCchCopyN StringCchCopyNW
#define StringCchCopyNEx StringCchCopyNExW
#define StringCchGets StringCchGetsW
#define StringCchGetsEx StringCchGetsExW
#define StringCchLength StringCchLengthW
#define StringCchPrintf StringCchPrintfW
#define StringCchPrintfEx StringCchPrintfExW
#define StringCchVPrintf StringCchVPrintfW
#define StringCchVPrintfEx StringCchVPrintfExW

#else // !UNICODE

#define StringCbCat StringCbCatA
#define StringCbCatEx StringCbCatExA
#define StringCbCatN StringCbCatNA
#define StringCbCatNEx StringCbCatNExA
#define StringCbCopy StringCbCopyA
#define StringCbCopyEx StringCbCopyExA
#define StringCbCopyN StringCbCopyNA
#define StringCbCopyNEx StringCbCopyNExA
#define StringCbGets StringCbGetsA
#define StringCbGetsEx StringCbGetsExA
#define StringCbLength StringCbLengthA
#define StringCbPrintf StringCbPrintfA
#define StringCbPrintfEx StringCbPrintfExA
#define StringCbVPrintf StringCbVPrintfA
#define StringCbVPrintfEx StringCbVPrintfExA

#endif // !UNICODE

#endif // !__STRSAFE_H_

/*****************************************************************************/

#if defined(STRSAFE_UNICODE)
#if (STRSAFE_UNICODE == 1)

#define STRSAFE_LPTSTR STRSAFE_LPWSTR
#define STRSAFE_LPCTSTR STRSAFE_LPCWSTR
#define STRSAFE_TCHAR wchar_t

#if defined(STRSAFE_CB)
#define StringCxxCat StringCbCatW
#define StringCxxCatEx StringCbCatExW
#define StringCxxCatN StringCbCatNW
#define StringCxxCatNEx StringCbCatNExW
#define StringCxxCopy StringCbCopyW
#define StringCxxCopyEx StringCbCopyExW
#define StringCxxCopyN StringCbCopyNW
#define StringCxxCopyNEx StringCbCopyNExW
#define StringCxxGets StringCbGetsW
#define StringCxxGetsEx StringCbGetsExW
#define StringCxxLength StringCbLengthW
#define StringCxxPrintf StringCbPrintfW
#define StringCxxPrintfEx StringCbPrintfExW
#define StringCxxVPrintf StringCbVPrintfW
#define StringCxxVPrintfEx StringCbVPrintfExW
#else // !STRSAFE_CB
#define StringCxxCat StringCchCatW
#define StringCxxCatEx StringCchCatExW
#define StringCxxCatN StringCchCatNW
#define StringCxxCatNEx StringCchCatNExW
#define StringCxxCopy StringCchCopyW
#define StringCxxCopyEx StringCchCopyExW
#define StringCxxCopyN StringCchCopyNW
#define StringCxxCopyNEx StringCchCopyNExW
#define StringCxxGets StringCchGetsW
#define StringCxxGetsEx StringCchGetsExW
#define StringCxxLength StringCchLengthW
#define StringCxxPrintf StringCchPrintfW
#define StringCxxPrintfEx StringCchPrintfExW
#define StringCxxVPrintf StringCchVPrintfW
#define StringCxxVPrintfEx StringCchVPrintfExW
#endif // !STRSAFE_CB

#else // (STRSAFE_UNICODE != 1)

#define STRSAFE_LPTSTR STRSAFE_LPSTR
#define STRSAFE_LPCTSTR STRSAFE_LPCSTR
#define STRSAFE_TCHAR char

#if defined(STRSAFE_CB)
#define StringCxxCat StringCbCatA
#define StringCxxCatEx StringCbCatExA
#define StringCxxCatN StringCbCatNA
#define StringCxxCatNEx StringCbCatNExA
#define StringCxxCopy StringCbCopyA
#define StringCxxCopyEx StringCbCopyExA
#define StringCxxCopyN StringCbCopyNA
#define StringCxxCopyNEx StringCbCopyNExA
#define StringCxxGets StringCbGetsA
#define StringCxxGetsEx StringCbGetsExA
#define StringCxxLength StringCbLengthA
#define StringCxxPrintf StringCbPrintfA
#define StringCxxPrintfEx StringCbPrintfExA
#define StringCxxVPrintf StringCbVPrintfA
#define StringCxxVPrintfEx StringCbVPrintfExA
#else // !STRSAFE_CB
#define StringCxxCat StringCchCatA
#define StringCxxCatEx StringCchCatExA
#define StringCxxCatN StringCchCatNA
#define StringCxxCatNEx StringCchCatNExA
#define StringCxxCopy StringCchCopyA
#define StringCxxCopyEx StringCchCopyExA
#define StringCxxCopyN StringCchCopyNA
#define StringCopyNEx StringCchCopyNExA
#define StringCxxGets StringCchGetsA
#define StringCxxGetsEx StringCchGetsExA
#define StringCxxLength StringCchLengthA
#define StringCxxPrintf StringCchPrintfA
#define StringCxxPrintfEx StringCchPrintfExA
#define StringCxxVPrintf StringCchVPrintfA
#define StringCxxVPrintfEx StringCchVPrintfExA
#endif // !STRSAFE_CB

#endif // (STRSAFE_UNICODE != 1)
#endif // defined(STRSAFE_UNICODE)

/*****************************************************************************/

#if defined (STRSAFE_PASS2)

#ifdef STRSAFE_LIB

/* Normal function prototypes only */
#define STRSAFEAPI HRESULT __stdcall

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCat(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc);
STRSAFEAPI StringCatEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags);
STRSAFEAPI StringCatN(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc, size_t cbMaxAppend);
STRSAFEAPI StringCatNEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc, size_t cbMaxAppend, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags);
STRSAFEAPI StringCopy(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc);
STRSAFEAPI StringCopyEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags);
STRSAFEAPI StringCopyN(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc, size_t cbSrc);
STRSAFEAPI StringCopyNEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc, size_t cbSrc, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags);
STRSAFEAPI StringGets(STRSAFE_LPTSTR pszDest, size_t cxDest);
STRSAFEAPI StringGetsEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags);
STRSAFEAPI StringLength(STRSAFE_LPCTSTR psz, size_t cxMax, size_t *pcb);
STRSAFEAPI StringPrintf(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszFormat, ...);
STRSAFEAPI StringPrintfEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags, STRSAFE_LPCTSTR pszFormat, ...);
STRSAFEAPI StringVPrintf(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszFormat, va_list args);
STRSAFEAPI StringVPrintfEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags, LPCTSTR pszFormat, va_list args);
#endif // !STRSAFE_NO_CB_FUNCTIONS

#ifndef STRSAFE_NO_CCH_FUNCTIONS

#endif // !STRSAFE_NO_CCH_FUNCTIONS


#else // !STRSAFE_LIB

/* Create inlined versions */
#define STRSAFEAPI HRESULT static __inline__

#ifndef STRSAFE_NO_CB_FUNCTIONS

STRSAFEAPI StringCxxCat(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc)
{
    STRSAFE_CXX_CB(cxDest);
    return 0; // FIXME
}

STRSAFEAPI StringCxxCatEx(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszSrc, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags)
{
    return 0; // FIXME
}

STRSAFEAPI StringCxxCatN(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszSrc, size_t cbMaxAppend)
{
    return 0; // FIXME
}

STRSAFEAPI StringCxxCatNEx(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszSrc, size_t cbMaxAppend, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags)
{
    return 0; // FIXME
}

STRSAFEAPI StringCxxCopy(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszSrc)
{
    return 0; // FIXME
}

STRSAFEAPI StringCxxCopyEx(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszSrc, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags)
{
    return 0; // FIXME
}

STRSAFEAPI StringCxxCopyN(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszSrc, size_t cbSrc)
{
    return 0; // FIXME
}

STRSAFEAPI StringCxxCopyNEx(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszSrc, size_t cbSrc, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags)
{
    return 0; // FIXME
}

STRSAFEAPI StringCxxGets(STRSAFE_LPTSTR pszDest, size_t cbDest)
{
    return 0; // FIXME
}

STRSAFEAPI StringCxxGetsEx(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags)
{
    return 0; // FIXME
}

STRSAFEAPI StringCxxLength(STRSAFE_LPCTSTR psz, size_t cbMax, size_t *pcb)
{
    return 0; // FIXME
}

STRSAFEAPI StringCxxVPrintf(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszFormat, va_list args)
{
    return 0; // FIXME
}

STRSAFEAPI StringCxxVPrintfEx(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags, STRSAFE_LPCTSTR pszFormat, va_list args)
{
    return 0; // FIXME
}

STRSAFEAPI StringCxxPrintf(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszFormat, ...)
{
    HRESULT result;
    va_list args;
    va_start(args, pszFormat);
    result = StringCxxVPrintf(pszDest, cbDest, pszFormat, args);
    va_end(args);
    return result;
}

STRSAFEAPI StringCxxPrintfEx(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags, STRSAFE_LPCTSTR pszFormat, ...)
{
    HRESULT result;
    va_list args;
    va_start(args, pszFormat);
    result = StringCxxVPrintfEx(pszDest, cbDest, ppszDestEnd, pcbRemaining, dwFlags, pszFormat, args);
    va_end(args);
    return result;
}

#endif // !STRSAFE_NO_CB_FUNCTIONS

#ifndef STRSAFE_NO_CCH_FUNCTIONS

#endif // !STRSAFE_NO_CCH_FUNCTIONS

#endif // !STRSAFE_LIB

/* Functions are implemented or defined, clear #defines for next pass */
#undef StringCxxCat
#undef StringCxxCatEx
#undef StringCxxCatN
#undef StringCxxCatNEx
#undef StringCxxCopy
#undef StringCxxCopyEx
#undef StringCxxCopyN
#undef StringCxxCopyNEx
#undef StringCxxGets
#undef StringCxxGetsEx
#undef StringCxxLength
#undef StringCxxPrintf
#undef StringCxxPrintfEx
#undef StringCxxVPrintf
#undef StringCxxVPrintfEx

#undef STRSAFE_LPTSTR
#undef STRSAFE_LPCTSTR
#undef STRSAFE_TCHAR

#endif // defined (STRSAFE_PASS2)

