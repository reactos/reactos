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

/* Implement for ansi and unicode */
#define STRSAFE_PASS2
#define STRSAFE_UNICODE 0
# include <strsafe.h>
#undef STRSAFE_UNICODE
#define STRSAFE_UNICODE 1
# include <strsafe.h>
#undef STRSAFE_UNICODE
#undef STRSAFE_PASS2

/* Now define the functions depending on UNICODE */
#ifdef UNICODE
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

#define StringCbCat StringCbCatW
#define StringCbCopy StringCbCopyW
#define StringCbVPrintf StringCbVPrintfW
#define StringCbPrintf StringCbPrintfW

#else // (STRSAFE_UNICODE != 1)

#define STRSAFE_LPTSTR STRSAFE_LPSTR
#define STRSAFE_LPCTSTR STRSAFE_LPCSTR

#define StringCbCat StringCbCatA
#define StringCbCopy StringCbCopyA
#define StringCbVPrintf StringCbVPrintfA
#define StringCbPrintf StringCbPrintfA

#endif // (STRSAFE_UNICODE != 1)
#endif // defined(STRSAFE_UNICODE)

/*****************************************************************************/

#if defined (STRSAFE_PASS2)

#ifdef STRSAFE_LIB

/* Normal function prototypes only */
#define STRSAFEAPI HRESULT __stdcall

#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI StringCbCat(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszSrc);
STRSAFEAPI StringCbCopy(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszSrc);
STRSAFEAPI StringCbVPrintf(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszFormat, va_list args);
STRSAFEAPI StringCbPrintf(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszFormat, ...);
#endif // !STRSAFE_NO_CB_FUNCTIONS

#ifndef STRSAFE_NO_CCH_FUNCTIONS

#endif // !STRSAFE_NO_CCH_FUNCTIONS


#else // !STRSAFE_LIB

/* Create inlined versions */
#define STRSAFEAPI HRESULT static __inline__

#ifndef STRSAFE_NO_CB_FUNCTIONS

STRSAFEAPI StringCbCat(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszSrc)
{
    return 0; // FIXME
}

STRSAFEAPI
StringCbCopy(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszSrc)
{
    return 0; // FIXME
}

STRSAFEAPI
StringCbVPrintf(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszFormat, va_list args)
{
    return 0; // FIXME
}

STRSAFEAPI
StringCbPrintf(STRSAFE_LPTSTR pszDest, size_t cbDest, STRSAFE_LPCTSTR pszFormat, ...)
{
    HRESULT result;
    va_list args;
    va_start(args, pszFormat);
    result = StringCbVPrintf(pszDest, cbDest, pszFormat, args);
    va_end(args);
    return result;
}

#endif // !STRSAFE_NO_CB_FUNCTIONS

#ifndef STRSAFE_NO_CCH_FUNCTIONS

#endif // !STRSAFE_NO_CCH_FUNCTIONS

#endif // !STRSAFE_LIB

/* Functions are implemented or defined, clear #defines for next pass */
#undef StringCbCat
#undef StringCbCopy
#undef StringCbVPrintf
#undef StringCbPrintf

#undef STRSAFE_LPTSTR
#undef STRSAFE_LPCTSTR

#endif // defined (STRSAFE_PASS2)

