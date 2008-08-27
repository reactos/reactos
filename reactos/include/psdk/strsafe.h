#ifndef __STRSAFE_H_
#define __STRSAFE_H_

#include <stdlib.h>
#include <stdarg.h>

#if defined(STRSAFE_NO_CCH_FUNCTIONS) && defined(STRSAFE_NO_CB_FUNCTIONS)
#error Both STRSAFE_NO_CCH_FUNCTIONS and STRSAFE_NO_CB_FUNCTIONS are defined
#endif

#define STRSAFE_MAX_CCH 2147483647
#define STRSAFE_E_INVALID_PARAMETER ((HRESULT)0x80070057L)
#ifndef S_OK
#define S_OK  ((HRESULT)0x00000000L)
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
#ifndef STRSAFE_NO_CB_FUNCTIONS
#define STRSAFE_CB
#define STRSAFE_UNICODE 0
# include <strsafe.h>
#undef STRSAFE_UNICODE
#define STRSAFE_UNICODE 1
# include <strsafe.h>
#undef STRSAFE_UNICODE
#undef STRSAFE_CB
#endif // !STRSAFE_NO_CB_FUNCTIONS

/* Implement Cch functions for ansi and unicode */
#ifndef STRSAFE_NO_CCH_FUNCTIONS
#define STRSAFE_UNICODE 0
# include <strsafe.h>
#undef STRSAFE_UNICODE
#define STRSAFE_UNICODE 1
# include <strsafe.h>
#undef STRSAFE_UNICODE
#endif // !STRSAFE_NO_CCH_FUNCTIONS

#undef STRSAFE_PASS2

/* Now define the functions depending on UNICODE */
#if defined(UNICODE)
# define STRSAFE_UNICODE 1
#else
# define STRSAFE_UNICODE 0
#endif
#include <strsafe.h>
#undef STRSAFE_UNICODE

#endif // !__STRSAFE_H_

/*****************************************************************************/

#if defined(STRSAFE_UNICODE)
#if (STRSAFE_UNICODE == 1)

#define STRSAFE_LPTSTR STRSAFE_LPWSTR
#define STRSAFE_LPCTSTR STRSAFE_LPCWSTR
#define STRSAFE_TCHAR wchar_t

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

#else // (STRSAFE_UNICODE != 1)

#define STRSAFE_LPTSTR STRSAFE_LPSTR
#define STRSAFE_LPCTSTR STRSAFE_LPCSTR
#define STRSAFE_TCHAR char

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
#define StringCchCat StringCchCatA
#define StringCchCatEx StringCchCatExA
#define StringCchCatN StringCchCatNA
#define StringCchCatNEx StringCchCatNExA
#define StringCchCopy StringCchCopyA
#define StringCchCopyEx StringCchCopyExA
#define StringCchCopyN StringCchCopyNA
#define StringCchCopyNEx StringCchCopyNExA
#define StringCchGets StringCchGetsA
#define StringCchGetsEx StringCchGetsExA
#define StringCchLength StringCchLengthA
#define StringCchPrintf StringCchPrintfA
#define StringCchPrintfEx StringCchPrintfExA
#define StringCchVPrintf StringCchVPrintfA
#define StringCchVPrintfEx StringCchVPrintfExA

#endif // (STRSAFE_UNICODE != 1)
#endif // defined(STRSAFE_UNICODE)

/*****************************************************************************/

#if defined (STRSAFE_PASS2)

#if defined(STRSAFE_CB)

#define STRSAFE_CXXtoCB(x) (x)
#define STRSAFE_CBtoCXX(x) (x)
#define STRSAFE_CXXtoCCH(x) (x)*sizeof(STRSAFE_TCHAR)
#define STRSAFE_CCHtoCXX(x) (x)/sizeof(STRSAFE_TCHAR)
#define StringCxxCat StringCbCat
#define StringCxxCatEx StringCbCatEx
#define StringCxxCatN StringCbCatN
#define StringCxxCatNEx StringCbCatNEx
#define StringCxxCopy StringCbCopy
#define StringCxxCopyEx StringCbCopyEx
#define StringCxxCopyN StringCbCopyN
#define StringCxxCopyNEx StringCbCopyNEx
#define StringCxxGets StringCbGets
#define StringCxxGetsEx StringCbGetsEx
#define StringCxxLength StringCbLength
#define StringCxxPrintf StringCbPrintf
#define StringCxxPrintfEx StringCbPrintfEx
#define StringCxxVPrintf StringCbVPrintf
#define StringCxxVPrintfEx StringCbVPrintfEx

#else // !STRSAFE_CB

#define STRSAFE_CXXtoCB(x) (x)/sizeof(STRSAFE_TCHAR)
#define STRSAFE_CBtoCXX(x) (x)*sizeof(STRSAFE_TCHAR)
#define STRSAFE_CXXtoCCH(x) (x)
#define STRSAFE_CCHtoCXX(x) (x)
#define StringCxxCat StringCchCat
#define StringCxxCatEx StringCchCatEx
#define StringCxxCatN StringCchCatN
#define StringCxxCatNEx StringCchCatNEx
#define StringCxxCopy StringCchCopy
#define StringCxxCopyEx StringCchCopyEx
#define StringCxxCopyN StringCchCopyN
#define StringCxxCopyNEx StringCchCopyNEx
#define StringCxxGets StringCchGets
#define StringCxxGetsEx StringCchGetsEx
#define StringCxxLength StringCchLength
#define StringCxxPrintf StringCchPrintf
#define StringCxxPrintfEx StringCchPrintfEx
#define StringCxxVPrintf StringCchVPrintf
#define StringCxxVPrintfEx StringCchVPrintfEx

#endif // !STRSAFE_CB

#ifdef STRSAFE_LIB

/* Normal function prototypes only */
#define STRSAFEAPI HRESULT __stdcall

STRSAFEAPI StringCxxCat(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc);
STRSAFEAPI StringCxxCatEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags);
STRSAFEAPI StringCxxCatN(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc, size_t cbMaxAppend);
STRSAFEAPI StringCxxCatNEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc, size_t cbMaxAppend, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags);
STRSAFEAPI StringCxxCopy(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc);
STRSAFEAPI StringCxxCopyEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags);
STRSAFEAPI StringCxxCopyN(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc, size_t cbSrc);
STRSAFEAPI StringCxxCopyNEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc, size_t cbSrc, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags);
STRSAFEAPI StringCxxGets(STRSAFE_LPTSTR pszDest, size_t cxDest);
STRSAFEAPI StringCxxGetsEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags);
STRSAFEAPI StringCxxLength(STRSAFE_LPCTSTR psz, size_t cxMax, size_t *pcb);
STRSAFEAPI StringCxxPrintf(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszFormat, ...);
STRSAFEAPI StringCxxPrintfEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags, STRSAFE_LPCTSTR pszFormat, ...);
STRSAFEAPI StringCxxVPrintf(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszFormat, va_list args);
STRSAFEAPI StringCxxVPrintfEx(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPTSTR *ppszDestEnd, size_t *pcbRemaining, STRSAFE_DWORD dwFlags, LPCTSTR pszFormat, va_list args);

#else // !STRSAFE_LIB

/* Create inlined versions */
#define STRSAFEAPI HRESULT static __inline__

#ifndef STRSAFE_NO_CB_FUNCTIONS

STRSAFEAPI StringCxxCat(STRSAFE_LPTSTR pszDest, size_t cxDest, STRSAFE_LPCTSTR pszSrc)
{
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

STRSAFEAPI StringCxxLength(STRSAFE_LPCTSTR psz, size_t cxMax, size_t *pcx)
{
    size_t cch = STRSAFE_CXXtoCCH(cxMax);

    /* Default return on error */
    if (pcx)
        *pcx = 0;

    if (!psz || cch > STRSAFE_MAX_CCH || cch == 0)
    {
        return STRSAFE_E_INVALID_PARAMETER;
    }

    for (--psz; *(++psz) != 0 && --cch > 0;);

    if (cch == 0)
    {
        return STRSAFE_E_INVALID_PARAMETER;
    }

    if (pcx)
        *pcx = cxMax - STRSAFE_CCHtoCXX(cch);

    return S_OK;
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

#undef StringCbCat
#undef StringCbCatEx
#undef StringCbCatN
#undef StringCbCatNEx
#undef StringCbCopy
#undef StringCbCopyEx
#undef StringCbCopyN
#undef StringCbCopyNEx
#undef StringCbGets
#undef StringCbGetsEx
#undef StringCbLength
#undef StringCbPrintf
#undef StringCbPrintfEx
#undef StringCbVPrintf
#undef StringCbVPrintfEx
#undef StringCchCat
#undef StringCchCatEx
#undef StringCchCatN
#undef StringCchCatNEx
#undef StringCchCopy
#undef StringCchCopyEx
#undef StringCchCopyN
#undef StringCchCopyNEx
#undef StringCchGets
#undef StringCchGetsEx
#undef StringCchLength
#undef StringCchPrintf
#undef StringCchPrintfEx
#undef StringCchVPrintf
#undef StringCchVPrintfEx

#undef STRSAFE_LPTSTR
#undef STRSAFE_LPCTSTR
#undef STRSAFE_TCHAR

#undef STRSAFE_CXXtoCB
#undef STRSAFE_CBtoCXX
#undef STRSAFE_CXXtoCCH
#undef STRSAFE_CCHtoCXX

#endif // defined (STRSAFE_PASS2)

