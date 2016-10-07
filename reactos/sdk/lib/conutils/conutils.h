/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Utilities Library
 * FILE:            sdk/lib/conutils/conutils.h
 * PURPOSE:         Provides simple ready-to-use abstraction wrappers around
 *                  CRT streams or Win32 console API I/O functions, to deal with
 *                  i18n + Unicode related problems.
 * PROGRAMMERS:     - Hermes Belusca-Maito (for making this library);
 *                  - All programmers who wrote the different console applications
 *                    from which I took those functions and improved them.
 */

#ifndef __CONUTILS_H__
#define __CONUTILS_H__

/*
 * Enable this define if you want to only use CRT functions to output
 * UNICODE stream to the console, as in the way explained by
 * http://archives.miloush.net/michkap/archive/2008/03/18/8306597.html
 */
/** NOTE: Experimental! Don't use USE_CRT yet because output to console is a bit broken **/
// #define USE_CRT

#ifndef _UNICODE
#error The ConUtils library only supports compilation with _UNICODE defined, at the moment!
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
    UTF16Text,  // #define _O_U16TEXT   0x20000 // file mode is UTF16   no BOM (translated) --    ""           ""
    UTF8Text,   // #define _O_U8TEXT    0x40000 // file mode is UTF8    no BOM (translated)
} CON_STREAM_MODE, *PCON_STREAM_MODE;

// Shadow type, implementation-specific
typedef struct _CON_STREAM CON_STREAM, *PCON_STREAM;

                                        // Stream,         szStr,     len
typedef INT (__stdcall *CON_WRITE_FUNC)(IN PCON_STREAM, IN PTCHAR, IN DWORD);

/*
 * Standard console streams, initialized by
 * calls to ConStreamInit/ConInitStdStreams.
 */
#if 0 // FIXME!
extern CON_STREAM StdStreams[3];
#define StdIn   (&StdStreams[0]) // TODO!
#define StdOut  (&StdStreams[1])
#define StdErr  (&StdStreams[2])
#else
extern CON_STREAM csStdIn;
extern CON_STREAM csStdOut;
extern CON_STREAM csStdErr;
#define StdIn   (&csStdIn) // TODO!
#define StdOut  (&csStdOut)
#define StdErr  (&csStdErr)
#endif

// static
BOOL
IsConsoleHandle(IN HANDLE hHandle);

BOOL
ConStreamInitEx(
    OUT PCON_STREAM Stream,
    IN  PVOID Handle,
    IN  CON_STREAM_MODE Mode,
    IN  CON_WRITE_FUNC WriteFunc OPTIONAL);

BOOL
ConStreamInit(
    OUT PCON_STREAM Stream,
    IN  PVOID Handle,
    IN  CON_STREAM_MODE Mode);


/* Console Standard Streams initialization helpers */
#ifdef _UNICODE

#ifdef USE_CRT
#define ConInitStdStreams() \
do { \
    ConStreamInit(StdOut, stdout, UTF16Text); \
    ConStreamInit(StdErr, stderr, UTF16Text); \
} while(0)
#else
#define ConInitStdStreams() \
do { \
    ConStreamInit(StdOut, GetStdHandle(STD_OUTPUT_HANDLE), UTF16Text); \
    ConStreamInit(StdErr, GetStdHandle(STD_ERROR_HANDLE) , UTF16Text); \
} while(0)
#endif /* defined(USE_CRT) */

#else

#ifdef USE_CRT
#define ConInitStdStreams() \
do { \
    ConStreamInit(StdOut, stdout, AnsiText); \
    ConStreamInit(StdErr, stderr, AnsiText); \
} while(0)
#else
#define ConInitStdStreams() \
do { \
    ConStreamInit(StdOut, GetStdHandle(STD_OUTPUT_HANDLE), AnsiText); \
    ConStreamInit(StdErr, GetStdHandle(STD_ERROR_HANDLE) , AnsiText); \
} while(0)
#endif /* defined(USE_CRT) */

#endif /* defined(_UNICODE) */



/*
 * Console I/O utility API
 * (for the moment, only Output)
 */

/*** Redundant defines to keep compat with existing code for now... ***/
/*** Must be removed later! ***/

#define CON_RC_STRING_MAX_SIZE  4096
#define MAX_BUFFER_SIZE     4096    // some exotic programs set it to 5024
#define OUTPUT_BUFFER_SIZE  4096
// MAX_STRING_SIZE

// #define MAX_MESSAGE_SIZE    512


INT
__stdcall
ConWrite(
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
ConResPuts(
    IN PCON_STREAM Stream,
    IN UINT uID);

INT
ConResPrintfV(
    IN PCON_STREAM Stream,
    IN UINT    uID,
    IN va_list args); // arg_ptr

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


/*
 * Those are compatibility #defines for old code!
 */

/*** tree.c ***/

#define PrintStringV(szStr, args)   \
    ConPrintfV(StdOut, (szStr), (args))
#define PrintString(szStr, ...)     \
    ConPrintf(StdOut, (szStr), ##__VA_ARGS__)

/*** network/net/main.c ***/
#define PrintToConsole(szStr, ...)  \
    ConPrintf(StdOut, (szStr), ##__VA_ARGS__)

/*** clip.c, comp.c, help.c, tree.c ***/
/*** subst.c ***/
/*** format.c, network/net/main.c, shutdown.c, wlanconf.c, diskpart.c ***/

#define PrintResourceStringV(uID, args) \
    ConResPrintfV(StdOut, (uID), (args))
#define PrintResourceString(uID, ...)   \
    ConResPrintf(StdOut, (uID), ##__VA_ARGS__)

//
// TODO: Add Console paged-output printf & ResPrintf functions!
//

#endif  /* __CONUTILS_H__ */
