/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Utilities Library
 * FILE:            sdk/lib/conutils/stream.h
 * PURPOSE:         Provides basic abstraction wrappers around CRT streams or
 *                  Win32 console API I/O functions, to deal with i18n + Unicode
 *                  related problems.
 * PROGRAMMERS:     - Hermes Belusca-Maito (for the library);
 *                  - All programmers who wrote the different console applications
 *                    from which I took those functions and improved them.
 */

#ifndef __STREAM_H__
#define __STREAM_H__

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

/*
 * Console I/O streams
 */

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

#ifdef _UNICODE
/*
 * Use UTF8 by default for file output, because this mode is back-compatible
 * with ANSI, and it displays nice on terminals that support UTF8 by default
 * (not many terminals support UTF16 on the contrary).
 */
#define ConInitStdStreams() \
    ConInitStdStreamsAndMode(UTF8Text, INVALID_CP); // Cache code page unused
#else
/* Use ANSI by default for file output */
#define ConInitStdStreams() \
    ConInitStdStreamsAndMode(AnsiText, INVALID_CP);
#endif /* defined(_UNICODE) */

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


/*
 * Console I/O utility API
 * (for the moment, only Output)
 */

INT
__stdcall
ConWrite(
    IN PCON_STREAM Stream,
    IN PTCHAR szStr,
    IN DWORD len);

INT
ConStreamWrite(
    IN PCON_STREAM Stream,
    IN PTCHAR szStr,
    IN DWORD len);

INT
ConPuts(
    IN PCON_STREAM Stream,
    IN LPWSTR szStr);

INT
ConPrintfV(
    IN PCON_STREAM Stream,
    IN LPWSTR  szStr,
    IN va_list args); // arg_ptr

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
    IN UINT uID);

INT
ConResPuts(
    IN PCON_STREAM Stream,
    IN UINT uID);

INT
ConResPrintfExV(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT    uID,
    IN va_list args); // arg_ptr

INT
ConResPrintfV(
    IN PCON_STREAM Stream,
    IN UINT    uID,
    IN va_list args); // arg_ptr

INT
__cdecl
ConResPrintfEx(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT uID,
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
    IN va_list args); // arg_ptr

INT
ConMsgPrintfV(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN LPCVOID lpSource OPTIONAL,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    IN va_list args); // arg_ptr

INT
__cdecl
ConMsgPrintf(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN LPCVOID lpSource OPTIONAL,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    ...);



VOID
ConClearLine(IN PCON_STREAM Stream);


#endif  /* __STREAM_H__ */
