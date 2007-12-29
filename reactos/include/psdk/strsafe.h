/*
 * STRSAFE
 *
 * Copyright 2007 Dmitry Chapyshev <dmitry@reactos.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef _STRSAFE_H_INCLUDED_
#define _STRSAFE_H_INCLUDED_

#ifdef __cplusplus
#define _STRSAFE_EXTERN_C    extern "C"
extern "C" {
#else
#define _STRSAFE_EXTERN_C    extern
#endif

#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <windows.h>

#define STRSAFE_INLINE_API  __inline HRESULT __stdcall
#define STRSAFEAPI          _STRSAFE_EXTERN_C HRESULT __stdcall

#ifndef _NTSTRSAFE_H_INCLUDED_

#define STRSAFE_MAX_CCH                             2147483647

#define STRSAFE_FILL_BEHIND_NULL                    0x00000200
#define STRSAFE_IGNORE_NULLS                        0x00000100
#define STRSAFE_FILL_ON_FAILURE                     0x00000400
#define STRSAFE_NULL_ON_FAILURE                     0x00000800
#define STRSAFE_NO_TRUNCATION                       0x00001000
#define STRSAFE_IGNORE_NULL_UNICODE_STRINGS         0x00010000
#define STRSAFE_UNICODE_STRING_DEST_NULL_TERMINATED 0x00020000

#define STRSAFE_VALID_FLAGS                (0x000000FF | STRSAFE_IGNORE_NULLS | STRSAFE_FILL_BEHIND_NULL | STRSAFE_FILL_ON_FAILURE | STRSAFE_NULL_ON_FAILURE | STRSAFE_NO_TRUNCATION)
#define STRSAFE_UNICODE_STRING_VALID_FLAGS (STRSAFE_VALID_FLAGS | STRSAFE_IGNORE_NULL_UNICODE_STRINGS | STRSAFE_UNICODE_STRING_DEST_NULL_TERMINATED)
#define STRSAFE_FILL_BYTE(x)               ((unsigned long)((x & 0x000000FF) | STRSAFE_FILL_BEHIND_NULL))
#define STRSAFE_FAILURE_BYTE(x)            ((unsigned long)((x & 0x000000FF) | STRSAFE_FILL_ON_FAILURE))
#define STRSAFE_GET_FILL_PATTERN(dwFlags)  ((int)(dwFlags & 0x000000FF))

#endif /* _NTSTRSAFE_H_INCLUDED_ */

#define STRSAFE_E_INSUFFICIENT_BUFFER      ((HRESULT)0x8007007AL)
#define STRSAFE_E_INVALID_PARAMETER        ((HRESULT)0x80070057L)
#define STRSAFE_E_END_OF_FILE              ((HRESULT)0x80070026L)

#ifndef STRSAFE_NO_CCH_FUNCTIONS
STRSAFEAPI
StringCchCatA(LPTSTR pszDest,
              size_t cchDest,
              LPCTSTR pszSrc);

STRSAFEAPI
StringCchCatW(LPWSTR pszDest,
              size_t cchDest,
              LPCWSTR pszSrc);

STRSAFEAPI
StringCchCatExA(LPTSTR pszDest,
                size_t cchDest,
                LPCTSTR pszSrc,
                LPTSTR *ppszDestEnd,
                size_t* pcchRemaining,
                unsigned long dwFlags);

STRSAFEAPI
StringCchCatExW(LPWSTR pszDest,
                size_t cchDest,
                LPCWSTR pszSrc,
                LPWSTR *ppszDestEnd,
                size_t* pcchRemaining,
                unsigned long dwFlags);

STRSAFEAPI
StringCchCatNA(LPTSTR pszDest,
               size_t cchDest,
               LPCTSTR pszSrc,
               size_t cchMaxAppend);

STRSAFEAPI
StringCchCatNW(LPWSTR pszDest,
               size_t cchDest,
               LPCWSTR pszSrc,
               size_t cchMaxAppend);

STRSAFEAPI
StringCchCatNExA(LPTSTR pszDest,
                 size_t cchDest,
                 LPCTSTR pszSrc,
                 size_t cchMaxAppend,
                 LPTSTR *ppszDestEnd,
                 size_t* pcchRemaining,
                 unsigned long dwFlags);

STRSAFEAPI
StringCchCatNExW(LPWSTR pszDest,
                 size_t cchDest,
                 LPCWSTR pszSrc,
                 size_t cchMaxAppend,
                 LPWSTR *ppszDestEnd,
                 size_t* pcchRemaining,
                 unsigned long dwFlags);

STRSAFEAPI
StringCchCopyA(LPTSTR pszDest,
               size_t cchDest,
               LPCTSTR pszSrc);

STRSAFEAPI
StringCchCopyW(LPWSTR pszDest,
               size_t cchDest,
               LPCWSTR pszSrc);

STRSAFEAPI
StringCchCopyExA(LPTSTR pszDest,
                 size_t cchDest,
                 LPCTSTR pszSrc,
                 LPTSTR *ppszDestEnd,
                 size_t* pcchRemaining,
                 unsigned long dwFlags);

STRSAFEAPI
StringCchCopyExW(LPWSTR pszDest,
                 size_t cchDest,
                 LPCWSTR pszSrc,
                 LPWSTR *ppszDestEnd,
                 size_t* pcchRemaining,
                 unsigned long dwFlags);

STRSAFEAPI
StringCchCopyNA(LPTSTR pszDest,
                size_t cchDest,
                LPCTSTR pszSrc,
                size_t cchSrc);

STRSAFEAPI
StringCchCopyNW(LPWSTR pszDest,
                size_t cchDest,
                LPCWSTR pszSrc,
                size_t cchSrc);

STRSAFEAPI
StringCchCopyNExA(LPTSTR pszDest,
                  size_t cchDest,
                  LPCTSTR pszSrc,
                  size_t cchSrc,
                  LPTSTR *ppszDestEnd,
                  size_t* pcchRemaining,
                  unsigned long dwFlags);

STRSAFEAPI
StringCchCopyNExW(LPWSTR pszDest,
                  size_t cchDest,
                  LPCWSTR pszSrc,
                  size_t cchSrc,
                  LPWSTR *ppszDestEnd,
                  size_t* pcchRemaining,
                  unsigned long dwFlags);

STRSAFE_INLINE_API
StringCchGetsA(LPTSTR pszDest,
               size_t cchDest);

STRSAFE_INLINE_API
StringCchGetsW(LPWSTR pszDest,
               size_t cchDest);

STRSAFE_INLINE_API
StringCchGetsExA(LPTSTR pszDest,
                 size_t cchDest,
                 LPTSTR *ppszDestEnd,
                 size_t* pcchRemaining,
                 unsigned long dwFlags);

STRSAFE_INLINE_API
StringCchGetsExW(LPWSTR pszDest,
                 size_t cchDest,
                 LPWSTR *ppszDestEnd,
                 size_t* pcchRemaining,
                 unsigned long dwFlags);

STRSAFEAPI
StringCchVPrintfA(LPTSTR pszDest,
                  size_t cchDest,
                  LPCTSTR pszFormat,
                  va_list argList);

STRSAFEAPI
StringCchVPrintfW(LPWSTR pszDest,
                  size_t cchDest,
                  LPCWSTR pszFormat,
                  va_list argList);

STRSAFEAPI
StringCchVPrintfExA(LPTSTR pszDest,
                    size_t cchDest,
                    LPTSTR *ppszDestEnd,
                    size_t* pcchRemaining,
                    unsigned long dwFlags,
                    LPCTSTR pszFormat,
                    va_list argList);

STRSAFEAPI
StringCchVPrintfExW(LPWSTR pszDest,
                    size_t cchDest,
                    LPWSTR *ppszDestEnd,
                    size_t* pcchRemaining,
                    unsigned long dwFlags,
                    LPCWSTR pszFormat,
                    va_list argList);

STRSAFEAPI
StringCchLengthA(LPCTSTR psz,
                 size_t cchMax,
                 size_t* pcch);

STRSAFEAPI
StringCchLengthW(LPCWSTR psz,
                 size_t cchMax,
                 size_t* pcch);
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

/* CB */
#ifndef STRSAFE_NO_CB_FUNCTIONS
STRSAFEAPI
StringCbCatA(LPTSTR pszDest,
             size_t cbDest,
             LPCTSTR pszSrc);

STRSAFEAPI
StringCbCatW(LPWSTR pszDest,
             size_t cbDest,
             LPCWSTR pszSrc);

STRSAFEAPI
StringCbCatExA(LPTSTR pszDest,
               size_t cbDest,
               LPCTSTR pszSrc,
               LPTSTR *ppszDestEnd,
               size_t* pcbRemaining,
               unsigned long dwFlags);

STRSAFEAPI
StringCbCatExW(LPWSTR pszDest,
               size_t cbDest,
               LPCWSTR pszSrc,
               LPWSTR *ppszDestEnd,
               size_t* pcbRemaining,
               unsigned long dwFlags);

STRSAFEAPI
StringCbCopyA(LPTSTR pszDest,
              size_t cbDest,
              LPCTSTR pszSrc);

STRSAFEAPI
StringCbCopyW(LPWSTR pszDest,
              size_t cbDest,
              LPCWSTR pszSrc);

STRSAFEAPI
StringCbCopyExA(LPTSTR pszDest,
                size_t cbDest,
                LPCTSTR pszSrc,
                LPTSTR *ppszDestEnd,
                size_t* pcbRemaining,
                unsigned long dwFlags);

STRSAFEAPI
StringCbCopyExW(LPWSTR pszDest,
                size_t cbDest,
                LPCWSTR pszSrc,
                LPWSTR *ppszDestEnd,
                size_t* pcbRemaining,
                unsigned long dwFlags);

STRSAFEAPI
StringCbCopyNA(LPTSTR pszDest,
               size_t cbDest,
               LPCTSTR pszSrc,
               size_t cbSrc);

STRSAFEAPI
StringCbCopyNW(LPWSTR pszDest,
               size_t cbDest,
               LPCWSTR pszSrc,
               size_t cbSrc);

STRSAFEAPI
StringCbCopyNExA(LPTSTR pszDest,
                 size_t cbDest,
                 LPCTSTR pszSrc,
                 size_t cbSrc,
                 LPTSTR *ppszDestEnd,
                 size_t* pcbRemaining,
                 unsigned long dwFlags);

STRSAFEAPI
StringCbCopyNExW(LPWSTR pszDest,
                 size_t cbDest,
                 LPCWSTR pszSrc,
                 size_t cbSrc,
                 LPWSTR *ppszDestEnd,
                 size_t* pcbRemaining,
                 unsigned long dwFlags);

STRSAFE_INLINE_API
StringCbGetsA(LPTSTR pszDest,
              size_t cbDest);

STRSAFE_INLINE_API
StringCbGetsW(LPWSTR pszDest,
              size_t cbDest);

STRSAFE_INLINE_API
StringCbGetsExA(LPTSTR pszDest,
                size_t cbDest,
                LPTSTR *ppszDestEnd,
                size_t* pbRemaining,
                unsigned long dwFlags);

STRSAFE_INLINE_API
StringCbGetsExW(LPWSTR pszDest,
                size_t cbDest,
                LPWSTR *ppszDestEnd,
                size_t* pcbRemaining,
                unsigned long dwFlags);

STRSAFEAPI
StringCbPrintfExA(LPTSTR pszDest,
                  size_t cbDest,
                  LPTSTR *ppszDestEnd,
                  size_t* pcbRemaining,
                  unsigned long dwFlags,
                  LPCTSTR pszFormat,
                  ...);

STRSAFEAPI
StringCbPrintfExW(LPWSTR pszDest,
                  size_t cbDest,
                  LPWSTR *ppszDestEnd,
                  size_t* pcbRemaining,
                  unsigned long dwFlags,
                  LPCWSTR pszFormat,
                  ...);
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#ifdef UNICODE
#ifndef STRSAFE_NO_CCH_FUNCTIONS
#define StringCchCat       StringCchCatW
#define StringCchCatEx     StringCchCatExW
#define StringCchCatN      StringCchCatNW
#define StringCchCatNEx    StringCchCatNExW
#define StringCchCopy      StringCchCopyW
#define StringCchCopyEx    StringCchCopyExW
#define StringCchCopyN     StringCchCopyNW
#define StringCchCopyNEx   StringCchCopyNExW
#define StringCchGets      StringCchGetsW
#define StringCchGetsEx    StringCchGetsExW
#define StringCchPrintf    StringCchPrintfW
#define StringCchPrintfEx  StringCchPrintfExW
#define StringCchVPrintf   StringCchVPrintfW
#define StringCchVPrintfEx StringCchVPrintfExW
#define StringCchLength    StringCchLengthW
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
#define StringCbCat        StringCbCatW
#define StringCbCatEx      StringCbCatExW
#define StringCbCatN       StringCbCatNW
#define StringCbCatNEx     StringCbCatNExW
#define StringCbCopy       StringCbCopyW
#define StringCbCopyEx     StringCbCopyExW
#define StringCbCopyN      StringCbCopyNW
#define StringCbCopyNEx    StringCbCopyNExW
#define StringCbGets       StringCbGetsW
#define StringCbGetsEx     StringCbGetsExW
#define StringCbPrintf     StringCbPrintfW
#define StringCbPrintfEx   StringCbPrintfExW
#define StringCbVPrintf    StringCbVPrintfW
#define StringCbVPrintfEx  StringCbVPrintfExW
#define StringCbLenght     StringCbLenghtW
#endif /* STRSAFE_NO_CB_FUNCTIONS */

#else
#ifndef STRSAFE_NO_CCH_FUNCTIONS
#define StringCchCat       StringCchCatA
#define StringCchCatEx     StringCchCatExA
#define StringCchCatN      StringCchCatNA
#define StringCchCatNEx    StringCchCatNExA
#define StringCchCopy      StringCchCopyA
#define StringCchCopyEx    StringCchCopyExA
#define StringCchCopyN     StringCchCopyNA
#define StringCchCopyNEx   StringCchCopyNExA
#define StringCchGets      StringCchGetsA
#define StringCchGetsEx    StringCchGetsExA
#define StringCchPrintf    StringCchPrintfA
#define StringCchPrintfEx  StringCchPrintfExA
#define StringCchVPrintf   StringCchVPrintfA
#define StringCchVPrintfEx StringCchVPrintfExA
#define StringCchLength    StringCchLengthA
#endif /* STRSAFE_NO_CCH_FUNCTIONS */

#ifndef STRSAFE_NO_CB_FUNCTIONS
#define StringCbCat        StringCbCatA
#define StringCbCatEx      StringCbCatExA
#define StringCbCatN       StringCbCatNA
#define StringCbCatNEx     StringCbCatNExA
#define StringCbCopy       StringCbCopyA
#define StringCbCopyEx     StringCbCopyExA
#define StringCbCopyN      StringCbCopyNA
#define StringCbCopyNEx    StringCbCopyNExA
#define StringCbGets       StringCbGetsA
#define StringCbGetsEx     StringCbGetsExA
#define StringCbPrintf     StringCbPrintfA
#define StringCbPrintfEx   StringCbPrintfExA
#define StringCbVPrintf    StringCbVPrintfA
#define StringCbVPrintfEx  StringCbVPrintfExA
#define StringCbLenght     StringCbLenghtA
#endif /* STRSAFE_NO_CB_FUNCTIONS */
#endif /* UNICODE */

#ifdef __cplusplus
}
#endif /* cplusplus */

#endif /* _STRSAFE_H_INCLUDED_ */
