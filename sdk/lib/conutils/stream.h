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
 * @file    stream.h
 * @ingroup ConUtils
 *
 * @brief   Console I/O streams
 **/

#ifndef __STREAM_H__
#define __STREAM_H__

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

/*
 * See http://archives.miloush.net/michkap/archive/2009/08/14/9869928.html
 * for more information.
 */
typedef enum _CON_STREAM_MODE
{
    Binary = 0, // #define _O_BINARY    0x8000  // file mode is binary (untranslated)
                // #define _O_RAW       _O_BINARY
    AnsiText,   // #define _O_TEXT      0x4000  // file mode is text (translated) -- "ANSI"
    WideText,   // #define _O_WTEXT     0x10000 // file mode is UTF16 with BOM (translated) -- "Unicode" of Windows
    UTF16Text,  // #define _O_U16TEXT   0x20000 // file mode is UTF16   no BOM (translated) --    ""          ""
    UTF8Text,   // #define _O_U8TEXT    0x40000 // file mode is UTF8    no BOM (translated)
} CON_STREAM_MODE, *PCON_STREAM_MODE;

#define INVALID_CP  ((UINT)-1)

// Shadow type, implementation-specific
typedef struct _CON_STREAM CON_STREAM, *PCON_STREAM;

// typedef INT (__stdcall *CON_READ_FUNC)(IN PCON_STREAM, IN PTCHAR, IN DWORD);
                                        // Stream,         szStr,     len
typedef INT (__stdcall *CON_WRITE_FUNC)(IN PCON_STREAM, IN PTCHAR, IN DWORD);

/*
 * Standard console streams, initialized by
 * calls to ConStreamInit/ConInitStdStreams.
 */
#if 0 // FIXME!
extern CON_STREAM StdStreams[3];
#define StdIn   (&StdStreams[0])
#define StdOut  (&StdStreams[1])
#define StdErr  (&StdStreams[2])
#else
extern CON_STREAM csStdIn;
extern CON_STREAM csStdOut;
extern CON_STREAM csStdErr;
#define StdIn   (&csStdIn )
#define StdOut  (&csStdOut)
#define StdErr  (&csStdErr)
#endif

BOOL
ConStreamInitEx(
    OUT PCON_STREAM Stream,
    IN  PVOID Handle,
    IN  CON_STREAM_MODE Mode,
    IN  UINT CacheCodePage OPTIONAL,
    // IN ReadWriteMode ????
    // IN  CON_READ_FUNC ReadFunc OPTIONAL,
    IN  CON_WRITE_FUNC WriteFunc OPTIONAL);

BOOL
ConStreamInit(
    OUT PCON_STREAM Stream,
    IN  PVOID Handle,
    // IN ReadWriteMode ????
    IN  CON_STREAM_MODE Mode,
    IN  UINT CacheCodePage OPTIONAL);


/* Console Standard Streams initialization helpers */
#ifdef USE_CRT
#define ConInitStdStreamsAndMode(Mode, CacheCodePage) \
do { \
    ConStreamInit(StdIn , stdin , (Mode), (CacheCodePage)); \
    ConStreamInit(StdOut, stdout, (Mode), (CacheCodePage)); \
    ConStreamInit(StdErr, stderr, (Mode), (CacheCodePage)); \
} while(0)
#else
#define ConInitStdStreamsAndMode(Mode, CacheCodePage) \
do { \
    ConStreamInit(StdIn , GetStdHandle(STD_INPUT_HANDLE) , (Mode), (CacheCodePage)); \
    ConStreamInit(StdOut, GetStdHandle(STD_OUTPUT_HANDLE), (Mode), (CacheCodePage)); \
    ConStreamInit(StdErr, GetStdHandle(STD_ERROR_HANDLE) , (Mode), (CacheCodePage)); \
} while(0)
#endif /* defined(USE_CRT) */

/*
 * Use ANSI by default for file output, with no cached code page.
 * Note that setting the stream mode to AnsiText and the code page value
 * to CP_UTF8 sets the stream to UTF8 mode, and has the same effect as if
 * the stream mode UTF8Text had been specified instead.
 */
#define ConInitStdStreams() \
    ConInitStdStreamsAndMode(AnsiText, INVALID_CP)

/* Stream translation modes */
BOOL
ConStreamSetMode(
    IN PCON_STREAM Stream,
    IN CON_STREAM_MODE Mode,
    IN UINT CacheCodePage OPTIONAL);

#ifdef USE_CRT
// FIXME!
#warning The ConStreamSetCacheCodePage function does not make much sense with the CRT!
#else
BOOL
ConStreamSetCacheCodePage(
    IN PCON_STREAM Stream,
    IN UINT CacheCodePage);
#endif

HANDLE
ConStreamGetOSHandle(
    IN PCON_STREAM Stream);

BOOL
ConStreamSetOSHandle(
    IN PCON_STREAM Stream,
    IN HANDLE Handle);


#ifdef __cplusplus
}
#endif

#endif  /* __STREAM_H__ */

/* EOF */
