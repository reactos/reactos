/*
 * PROJECT:     ReactOS Console Utilities Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Provides basic abstraction wrappers around CRT streams or
 *              Win32 console API I/O functions, to deal with i18n + Unicode
 *              related problems.
 * COPYRIGHT:   Copyright 2017-2018 ReactOS Team
 *              Copyright 2017-2018 Hermes Belusca-Maito
 */

/**
 * @file    outstream.h
 * @ingroup ConUtils
 *
 * @brief   Console I/O utility API -- Output
 **/

#ifndef __OUTSTREAM_H__
#define __OUTSTREAM_H__

#pragma once

/*
 * Enable this define if you want to only use CRT functions to output
 * UNICODE stream to the console, as in the way explained by
 * http://archives.miloush.net/michkap/archive/2008/03/18/8306597.html
 */
/** NOTE: Experimental! Don't use USE_CRT yet because output to console is a bit broken **/
// #define USE_CRT

#ifndef _UNICODE
#error The ConUtils library at the moment only supports compilation with _UNICODE defined!
#endif

#ifdef __cplusplus
extern "C" {
#endif

// Shadow type, implementation-specific
typedef struct _CON_STREAM CON_STREAM, *PCON_STREAM;

// typedef INT (__stdcall *CON_READ_FUNC)(IN PCON_STREAM, IN PTCHAR, IN DWORD);
                                        // Stream,         szStr,     len
typedef INT (__stdcall *CON_WRITE_FUNC)(IN PCON_STREAM, IN PTCHAR, IN DWORD);


INT
__stdcall
ConWrite(
    IN PCON_STREAM Stream,
    IN PTCHAR szStr,
    IN DWORD  len);

INT
ConStreamWrite(
    IN PCON_STREAM Stream,
    IN PTCHAR szStr,
    IN DWORD  len);

INT
ConPuts(
    IN PCON_STREAM Stream,
    IN LPWSTR szStr);

INT
ConPrintfV(
    IN PCON_STREAM Stream,
    IN LPWSTR  szStr,
    IN va_list args);

INT
__cdecl
ConPrintf(
    IN PCON_STREAM Stream,
    IN LPWSTR szStr,
    ...);

INT
ConResPutsEx(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT   uID,
    IN LANGID LanguageId);

INT
ConResPuts(
    IN PCON_STREAM Stream,
    IN UINT uID);

INT
ConResPrintfExV(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT    uID,
    IN LANGID  LanguageId,
    IN va_list args);

INT
ConResPrintfV(
    IN PCON_STREAM Stream,
    IN UINT    uID,
    IN va_list args);

INT
__cdecl
ConResPrintfEx(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT   uID,
    IN LANGID LanguageId,
    ...);

INT
__cdecl
ConResPrintf(
    IN PCON_STREAM Stream,
    IN UINT uID,
    ...);

INT
ConMsgPuts(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN LPCVOID lpSource OPTIONAL,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId);

INT
ConMsgPrintf2V(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN LPCVOID lpSource OPTIONAL,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    IN va_list args);

INT
ConMsgPrintfV(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN LPCVOID lpSource OPTIONAL,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    IN va_list *Arguments OPTIONAL);

INT
__cdecl
ConMsgPrintf(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN LPCVOID lpSource OPTIONAL,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    ...);

INT
ConResMsgPrintfExV(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN DWORD   dwFlags,
    IN UINT    uID,
    IN LANGID  LanguageId,
    IN va_list *Arguments OPTIONAL);

INT
ConResMsgPrintfV(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN UINT    uID,
    IN va_list *Arguments OPTIONAL);

INT
__cdecl
ConResMsgPrintfEx(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN DWORD  dwFlags,
    IN UINT   uID,
    IN LANGID LanguageId,
    ...);

INT
__cdecl
ConResMsgPrintf(
    IN PCON_STREAM Stream,
    IN DWORD dwFlags,
    IN UINT  uID,
    ...);



VOID
ConClearLine(IN PCON_STREAM Stream);


#ifdef __cplusplus
}
#endif

#endif  /* __OUTSTREAM_H__ */

/* EOF */
