/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Utilities Library
 * FILE:            sdk/lib/conutils/utils.h
 * PURPOSE:         Base set of functions for loading string resources
 *                  and message strings, and handle type identification.
 * PROGRAMMERS:     - Hermes Belusca-Maito (for the library);
 *                  - All programmers who wrote the different console applications
 *                    from which I took those functions and improved them.
 */

#ifndef __UTILS_H__
#define __UTILS_H__

#ifndef _UNICODE
#error The ConUtils library at the moment only supports compilation with _UNICODE defined!
#endif

/*
 * General-purpose utility functions (wrappers around,
 * or reimplementations of, Win32 APIs).
 */

INT
WINAPI
K32LoadStringW(
    IN  HINSTANCE hInstance OPTIONAL,
    IN  UINT   uID,
    OUT LPWSTR lpBuffer,
    IN  INT    nBufferMax);

DWORD
WINAPI
FormatMessageSafeW(
    IN  DWORD   dwFlags,
    IN  LPCVOID lpSource OPTIONAL,
    IN  DWORD   dwMessageId,
    IN  DWORD   dwLanguageId,
    OUT LPWSTR  lpBuffer,
    IN  DWORD   nSize,
    IN  va_list *Arguments OPTIONAL);

BOOL
IsTTYHandle(IN HANDLE hHandle);

BOOL
IsConsoleHandle(IN HANDLE hHandle);


// #include <wincon.h>


#endif  /* __UTILS_H__ */
