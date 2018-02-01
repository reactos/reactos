/*
 * PROJECT:     ReactOS Console Utilities Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Base set of functions for loading string resources
 *              and message strings, and handle type identification.
 * COPYRIGHT:   Copyright 2017-2018 ReactOS Team
 *              Copyright 2017-2018 Hermes Belusca-Maito
 */

/**
 * @file    utils.h
 * @ingroup ConUtils
 *
 * @brief   General-purpose utility functions (wrappers around
 *          or reimplementations of Win32 APIs).
 **/

#ifndef __UTILS_H__
#define __UTILS_H__

#pragma once

#ifndef _UNICODE
#error The ConUtils library at the moment only supports compilation with _UNICODE defined!
#endif

#ifdef __cplusplus
extern "C" {
#endif

INT
WINAPI
K32LoadStringExW(
    IN  HINSTANCE hInstance OPTIONAL,
    IN  UINT   uID,
    IN  LANGID LanguageId,
    OUT LPWSTR lpBuffer,
    IN  INT    nBufferMax);

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


#ifdef __cplusplus
}
#endif

#endif  /* __UTILS_H__ */

/* EOF */
