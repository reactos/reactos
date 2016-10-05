/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Utilities Library
 * FILE:            sdk/lib/conutils/conutils.c
 * PURPOSE:         Provides simple ready-to-use abstraction wrappers around
 *                  CRT streams or Win32 console API I/O functions, to deal with
 *                  i18n + Unicode related problems.
 * PROGRAMMERS:     - Hermes Belusca-Maito (for making this library);
 *                  - All programmers who wrote the different console applications
 *                    from which I took those functions and improved them.
 */

/*
 * Enable this define if you want to only use CRT functions to output
 * UNICODE stream to the console, as in the way explained by
 * http://archives.miloush.net/michkap/archive/2008/03/18/8306597.html
 */
/** NOTE: Experimental! Don't use USE_CRT yet because output to console is a bit broken **/
// #define USE_CRT

#include <stdlib.h> // limits.h // For MB_LEN_MAX

#ifdef USE_CRT
#include <fcntl.h>
#include <io.h>
#endif /* defined(USE_CRT) */

#include <windef.h>
#include <winbase.h>

#include <winnls.h>
#include <winuser.h> // MAKEINTRESOURCEW, RT_STRING

#include <wincon.h>  // Console APIs (only if kernel32 support included)

#include "conutils.h"
#include <strsafe.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>


// #define RC_STRING_MAX_SIZE  4096
// #define MAX_BUFFER_SIZE     4096
// #define OUTPUT_BUFFER_SIZE  4096


/*
 * General-purpose utility functions (wrappers around,
 * or reimplementations of, Win32 APIs).
 */

/*
 * 'LoadStringW' API ripped from user32.dll to remove
 * any dependency of this library from user32.dll
 */
INT
WINAPI
K32LoadStringW(
    IN  HINSTANCE hInstance OPTIONAL,
    IN  UINT   uID,
    OUT LPWSTR lpBuffer,
    IN  INT    nBufferMax)
{
    HRSRC hrsrc;
    HGLOBAL hmem;
    WCHAR *p;
    UINT i;

    if (!lpBuffer)
        return 0;

    /* Use LOWORD (incremented by 1) as ResourceID */
    /* There are always blocks of 16 strings */
    // FindResourceExW(hInstance, RT_STRING, name, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
    // NOTE: Instead of using LANG_NEUTRAL, one might use LANG_USER_DEFAULT...
    hrsrc = FindResourceW(hInstance,
                          MAKEINTRESOURCEW((LOWORD(uID) >> 4) + 1),
                          (LPWSTR)RT_STRING);
    if (!hrsrc) return 0;

    hmem = LoadResource(hInstance, hrsrc);
    if (!hmem) return 0;

    p = LockResource(hmem);
    // FreeResource(hmem);

    /* Find the string we're looking for */
    uID &= 0x000F; /* Position in the block, same as % 16 */
    for (i = 0; i < uID; i++)
        p += *p + 1;

    /*
     * If nBufferMax == 0, then return a read-only pointer
     * to the resource itself in lpBuffer it is assumed that
     * lpBuffer is actually a (LPWSTR *).
     */
    if (nBufferMax == 0)
    {
        *((LPWSTR*)lpBuffer) = p + 1;
        return *p;
    }

    i = min(nBufferMax - 1, *p);
    if (i > 0)
    {
        memcpy(lpBuffer, p + 1, i * sizeof(WCHAR));
        lpBuffer[i] = L'\0';
    }
    else
    {
        if (nBufferMax > 1)
        {
            lpBuffer[0] = L'\0';
            return 0;
        }
    }

    return i;
}

/*
 * "Safe" version of FormatMessageW, that does not crash if a malformed
 * source string is retrieved and then being used for formatting.
 * It basically wraps calls to FormatMessageW within SEH.
 */
DWORD
WINAPI
FormatMessageSafeW(
    IN  DWORD   dwFlags,
    IN  LPCVOID lpSource OPTIONAL,
    IN  DWORD   dwMessageId,
    IN  DWORD   dwLanguageId,
    OUT LPWSTR  lpBuffer,
    IN  DWORD   nSize,
    IN  va_list *Arguments OPTIONAL)
{
    DWORD dwLength = 0;

    _SEH2_TRY
    {
        /*
         * Retrieve the message string. Wrap in SEH
         * to protect from invalid string parameters.
         */
        _SEH2_TRY
        {
            dwLength = FormatMessageW(dwFlags,
                                      lpSource,
                                      dwMessageId,
                                      dwLanguageId,
                                      lpBuffer,
                                      nSize,
                                      Arguments);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            dwLength = 0;

            /*
             * An exception occurred while calling FormatMessage, this is usually
             * the sign that a parameter was invalid, either 'lpBuffer' was NULL
             * but we did not pass the flag FORMAT_MESSAGE_ALLOCATE_BUFFER, or the
             * array pointer 'Arguments' was NULL or did not contain enough elements,
             * and we did not pass the flag FORMAT_MESSAGE_IGNORE_INSERTS, and the
             * message string expected too many inserts.
             * In this last case only, we can call again FormatMessage but ignore
             * explicitely the inserts. The string that we will return to the user
             * will not be pre-formatted.
             */
            if (((dwFlags & FORMAT_MESSAGE_ALLOCATE_BUFFER) || lpBuffer) &&
                !(dwFlags & FORMAT_MESSAGE_IGNORE_INSERTS))
            {
                /* Remove any possible harmful flags and always ignore inserts */
                dwFlags &= ~FORMAT_MESSAGE_ARGUMENT_ARRAY;
                dwFlags |=  FORMAT_MESSAGE_IGNORE_INSERTS;

                /* If this call also throws an exception, we are really dead */
                dwLength = FormatMessageW(dwFlags,
                                          lpSource,
                                          dwMessageId,
                                          dwLanguageId,
                                          lpBuffer,
                                          nSize,
                                          NULL /* Arguments */);
            }
        }
        _SEH2_END;
    }
    _SEH2_FINALLY
    {
    }
    _SEH2_END;

    return dwLength;
}


/*
 * Console I/O streams
 */

typedef struct _CON_STREAM
{
    CON_WRITE_FUNC WriteFunc;

#ifdef USE_CRT
    FILE* fStream;
#else
    HANDLE hHandle;
    BOOL   bIsConsole; // TRUE if 'hHandle' refers to a console,
                       // in which case I/O UNICODE is directly used.

    /*
     * 'Mode' flag is used to know the translation mode
     * when 'hHandle' refers to a file or a pipe.
     */
    CON_STREAM_MODE Mode;
    UINT CodePage;  // Used to convert UTF-16 text to some ANSI codepage.
#endif /* defined(USE_CRT) */
} CON_STREAM, *PCON_STREAM;

/*
 * Standard console streams, initialized by
 * calls to ConStreamInit/ConInitStdStreams.
 */
#if 0 // FIXME!
CON_STREAM StdStreams[3] =
{
    {0}, // StdIn // TODO!
    {0}, // StdOut
    {0}, // StdErr
};
#else
CON_STREAM csStdIn;
CON_STREAM csStdOut;
CON_STREAM csStdErr;
#endif


// static
BOOL
IsConsoleHandle(IN HANDLE hHandle)
{
    DWORD dwMode;

    /* Check whether the handle may be that of a console... */
    if ((GetFileType(hHandle) & ~FILE_TYPE_REMOTE) != FILE_TYPE_CHAR)
        return FALSE;

    /*
     * It may be. Perform another test. The idea comes from the
     * MSDN description of the WriteConsole API:
     *
     * "WriteConsole fails if it is used with a standard handle
     *  that is redirected to a file. If an application processes
     *  multilingual output that can be redirected, determine whether
     *  the output handle is a console handle (one method is to call
     *  the GetConsoleMode function and check whether it succeeds).
     *  If the handle is a console handle, call WriteConsole. If the
     *  handle is not a console handle, the output is redirected and
     *  you should call WriteFile to perform the I/O."
     */
    return GetConsoleMode(hHandle, &dwMode);
}

BOOL
ConStreamInitEx(
    OUT PCON_STREAM Stream,
    IN  PVOID Handle,
    IN  CON_STREAM_MODE Mode,
    IN  CON_WRITE_FUNC WriteFunc OPTIONAL)
{
    /* Parameters validation */
    if (!Stream || !Handle)
        return FALSE;
    if (Mode > UTF8Text)
        return FALSE;

    /* Use the default 'ConWrite' helper if nothing is specified */
    Stream->WriteFunc = (WriteFunc ? WriteFunc : ConWrite);

#ifdef USE_CRT
{
/* Lookup table to convert CON_STREAM_MODE to CRT mode */
    static int ConToCRTMode[] =
    {
        _O_BINARY,  // Binary    (untranslated)
        _O_TEXT,    // AnsiText  (translated)
        _O_WTEXT,   // WideText  (UTF16 with BOM; translated)
        _O_U16TEXT, // UTF16Text (UTF16 without BOM; translated)
        _O_U8TEXT,  // UTF8Text  (UTF8  without BOM; translated)
    };

    Stream->fStream = (FILE*)Handle;

    /* Set the correct file translation mode */
    // NOTE: May the translated mode be cached somehow?
    // NOTE2: We may also call IsConsoleHandle to directly set the mode to
    //        _O_U16TEXT if it's ok??
    if (Mode < ARRAYSIZE(ConToCRTMode))
        _setmode(_fileno(Stream->fStream), ConToCRTMode[Mode]);
    else
        _setmode(_fileno(Stream->fStream), _O_TEXT); // Default to ANSI text.
    // _setmode returns the previous mode, or -1 if failure.
}
#else

    Stream->hHandle    = (HANDLE)Handle;
    Stream->bIsConsole = IsConsoleHandle(Stream->hHandle);
    Stream->Mode       = Mode;

    // NOTE: Or recompute them @ each ConWrite call?
    if (Mode == AnsiText)
        Stream->CodePage = GetConsoleOutputCP(); // CP_ACP, CP_OEMCP
    else if (Mode == UTF8Text)
        Stream->CodePage = CP_UTF8;
    else // Mode == Binary, WideText, UTF16Text
        Stream->CodePage = 0;

#endif /* defined(USE_CRT) */

    return TRUE;
}

BOOL
ConStreamInit(
    OUT PCON_STREAM Stream,
    IN  PVOID Handle,
    IN  CON_STREAM_MODE Mode)
{
    return ConStreamInitEx(Stream, Handle, Mode, ConWrite);
}


/*
 * Console I/O utility API
 * (for the moment, only Output)
 */

// ConWriteStr
INT
__stdcall
ConWrite(
    IN PCON_STREAM Stream,
    IN PTCHAR szStr,
    IN DWORD len)
{
#ifndef USE_CRT
    DWORD TotalLen = len, dwNumBytes = 0;
    PVOID p;

    // CHAR strOem[CON_RC_STRING_MAX_SIZE]; // Some static buffer...

    /* If we do not write anything, just return */
    if (!szStr || len == 0)
        return 0;

    /* Check whether we are writing to a console */
    // if (IsConsoleHandle(Stream->hHandle))
    if (Stream->bIsConsole)
    {
        // TODO: Check if (ConStream->Mode == WideText or UTF16Text) ??
        WriteConsole(Stream->hHandle, szStr, len, &dwNumBytes, NULL);
        return (INT)dwNumBytes; // Really return the number of chars written.
    }

    /*
     * We are redirected and writing to a file or pipe instead of the console.
     * Convert the string from TCHARs to the desired output format, if the two differ.
     *
     * Implementation NOTE:
     *   MultiByteToWideChar (resp. WideCharToMultiByte) are equivalent to
     *   OemToCharBuffW (resp. CharToOemBuffW), but the latters uselessly
     *   depend on user32.dll, while MultiByteToWideChar and WideCharToMultiByte
     *   only need kernel32.dll.
     */
    if ((Stream->Mode == WideText) || (Stream->Mode == UTF16Text))
    {
#ifndef _UNICODE
        WCHAR *buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
        if (!buffer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
        len = (DWORD)MultiByteToWideChar(/* Stream->CodePage */ CP_THREAD_ACP /* CP_ACP -- CP_OEMCP */,
                                         0, szStr, (INT)len, buffer, (INT)len);
        szStr = (PVOID)buffer;
#endif

        /*
         * Find any newline character in the buffer,
         * write the part BEFORE the newline, then write
         * a carriage-return + newline, and then write
         * the remaining part of the buffer.
         *
         * This fixes output in files and serial console.
         */
        while (len > 0)
        {
            /* Loop until we find a \r or \n character */
            // FIXME: What about the pair \r\n ?
            p = szStr;
            while (len > 0 && *(PWCHAR)p != L'\r' && *(PWCHAR)p != L'\n')
            {
                /* Advance one character */
                p = (PVOID)((PWCHAR)p + 1);
                len--;
            }

            /* Write everything up to \r or \n */
            dwNumBytes = ((PWCHAR)p - (PWCHAR)szStr) * sizeof(WCHAR);
            WriteFile(Stream->hHandle, szStr, dwNumBytes, &dwNumBytes, NULL);

            /* If we hit \r or \n ... */
            if (len > 0 && (*(PWCHAR)p == L'\r' || *(PWCHAR)p == L'\n'))
            {
                /* ... send a carriage-return + newline sequence and skip \r or \n */
                WriteFile(Stream->hHandle, L"\r\n", 2 * sizeof(WCHAR), &dwNumBytes, NULL);
                szStr = (PVOID)((PWCHAR)p + 1);
                len--;
            }
        }

#ifndef _UNICODE
        HeapFree(GetProcessHeap(), 0, buffer);
#endif
    }
    else if ((Stream->Mode == UTF8Text) || (Stream->Mode == AnsiText))
    {
#ifdef _UNICODE
        // NOTE: MB_LEN_MAX defined either in limits.h or in stdlib.h .
        CHAR *buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * MB_LEN_MAX);
        if (!buffer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
        len = WideCharToMultiByte(Stream->CodePage, 0, szStr, len, buffer, len * MB_LEN_MAX, NULL, NULL);
        szStr = (PVOID)buffer;
#endif

        /*
         * Find any newline character in the buffer,
         * write the part BEFORE the newline, then write
         * a carriage-return + newline, and then write
         * the remaining part of the buffer.
         *
         * This fixes output in files and serial console.
         */
        while (len > 0)
        {
            /* Loop until we find a \r or \n character */
            // FIXME: What about the pair \r\n ?
            p = szStr;
            while (len > 0 && *(PCHAR)p != '\r' && *(PCHAR)p != '\n')
            {
                /* Advance one character */
                p = (PVOID)((PCHAR)p + 1);
                len--;
            }

            /* Write everything up to \r or \n */
            dwNumBytes = ((PCHAR)p - (PCHAR)szStr) * sizeof(CHAR);
            WriteFile(Stream->hHandle, szStr, dwNumBytes, &dwNumBytes, NULL);

            /* If we hit \r or \n ... */
            if (len > 0 && (*(PCHAR)p == '\r' || *(PCHAR)p == '\n'))
            {
                /* ... send a carriage-return + newline sequence and skip \r or \n */
                WriteFile(Stream->hHandle, "\r\n", 2, &dwNumBytes, NULL);
                szStr = (PVOID)((PCHAR)p + 1);
                len--;
            }
        }

#ifdef _UNICODE
        HeapFree(GetProcessHeap(), 0, buffer);
#endif
    }
    else // if (Stream->Mode == Binary)
    {
        WriteFile(Stream->hHandle, szStr, len, &dwNumBytes, NULL);
    }

    // FIXME!
    return (INT)TotalLen;

#else /* defined(USE_CRT) */

    DWORD total = len;
    DWORD written = 0;

    /* If we do not write anything, just return */
    if (!szStr || len == 0)
        return 0;

    /*
     * See http://archives.miloush.net/michkap/archive/2008/03/18/8306597.html
     * and http://archives.miloush.net/michkap/archive/2009/08/14/9869928.html
     * for more details.
     */
    // _setmode(_fileno(Stream->fStream), _O_U16TEXT); // Normally, already set before.
#if 1
    while (1)
    {
        written = wprintf(L"%.*s", total, szStr);
        if (written < total)
        {
            if (written == 0)
            {
                fputwc(*szStr, Stream->fStream);
                written++;
            }

            szStr += written;
            total -= written;
        }
        else
        {
            break;
        }
    }
    return (INT)len;
#else
    return fwrite(szStr, sizeof(*szStr), len, Stream->fStream);
#endif

#endif /* defined(USE_CRT) */
}

INT
ConPrintfV(
    IN PCON_STREAM Stream,
    IN LPWSTR  szStr,
    IN va_list args) // arg_ptr
{
    INT Len;
    WCHAR bufFormatted[CON_RC_STRING_MAX_SIZE];

#if 1 ///////////////////////////////////////////////////////////////////////  0
    PWSTR pEnd;
    StringCchVPrintfExW(bufFormatted, ARRAYSIZE(bufFormatted), &pEnd, NULL, 0, szStr, args);
    Len = pEnd - bufFormatted;
#else
    StringCchVPrintfW(bufFormatted, ARRAYSIZE(bufFormatted), szStr, args);
    Len = wcslen(bufFormatted);
#endif
    Len = Stream->WriteFunc(Stream, bufFormatted, Len);

    /* Fixup returned length in case of errors */
    if (Len < 0)
        Len = 0;

    return Len;
}

INT
ConPrintf(
    IN PCON_STREAM Stream,
    IN LPWSTR szStr,
    ...)
{
    INT Len;
    va_list args;

#if 0
    Len = vfwprintf(stdout, szMsgBuf, arg_ptr); // vfprintf for direct ANSI
    // or: Len = vwprintf(szMsgBuf, arg_ptr);
#endif

    // StringCchPrintfW
    va_start(args, szStr);
    Len = ConPrintfV(Stream, szStr, args);
    va_end(args);

    return Len;
}

INT
ConResPrintfV(
    IN PCON_STREAM Stream,
    IN UINT    uID,
    IN va_list args) // arg_ptr
{
    INT Len;
    WCHAR bufSrc[CON_RC_STRING_MAX_SIZE];

    // NOTE: We may use the special behaviour where nBufMaxSize == 0
    Len = K32LoadStringW(GetModuleHandleW(NULL), uID, bufSrc, ARRAYSIZE(bufSrc));
    if (Len)
        Len = ConPrintfV(Stream, bufSrc, args);

    return Len;
}

INT
ConResPrintf(
    IN PCON_STREAM Stream,
    IN UINT uID,
    ...)
{
    INT Len;
    va_list args;

    va_start(args, uID);
    Len = ConResPrintfV(Stream, uID, args);
    va_end(args);

    return Len;
}

INT
ConMsgPrintf2V(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN LPCVOID lpSource OPTIONAL,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    IN va_list args) // arg_ptr
{
    INT Len;
    DWORD dwLength  = 0;
    LPWSTR lpMsgBuf = NULL;

    /*
     * Sanitize dwFlags. This version always ignore explicitely the inserts.
     * The string that we will return to the user will not be pre-formatted.
     */
    dwFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER; // Always allocate an internal buffer.
    dwFlags |= FORMAT_MESSAGE_IGNORE_INSERTS;  // Ignore inserts for FormatMessage.
    dwFlags &= ~FORMAT_MESSAGE_ARGUMENT_ARRAY;

    dwFlags |= FORMAT_MESSAGE_MAX_WIDTH_MASK;

    /*
     * Retrieve the message string without appending extra newlines.
     * Wrap in SEH to protect from invalid string parameters.
     */
    _SEH2_TRY
    {
        dwLength = FormatMessageW(dwFlags,
                                  lpSource,
                                  dwMessageId,
                                  dwLanguageId,
                                  (LPWSTR)&lpMsgBuf,
                                  0, NULL);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    Len = (INT)dwLength;

    if (!lpMsgBuf)
    {
        // ASSERT(dwLength == 0);
    }
    else
    {
        // ASSERT(dwLength != 0);

        /* lpMsgBuf is NULL-terminated by FormatMessage */
        Len = ConPrintfV(Stream, lpMsgBuf, args);
        // Len = Stream->WriteFunc(Stream, lpMsgBuf, dwLength);

        /* Fixup returned length in case of errors */
        if (Len < 0)
            Len = 0;

        /* Free the buffer allocated by FormatMessage */
        LocalFree(lpMsgBuf);
    }

    return Len;
}

INT
ConMsgPrintfV(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN LPCVOID lpSource OPTIONAL,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    IN va_list args) // arg_ptr
{
    INT Len;
    DWORD dwLength  = 0;
    LPWSTR lpMsgBuf = NULL;

    /* Sanitize dwFlags */
    dwFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER; // Always allocate an internal buffer.
//  dwFlags &= ~FORMAT_MESSAGE_IGNORE_INSERTS; // We always use arguments.
    dwFlags &= ~FORMAT_MESSAGE_ARGUMENT_ARRAY; // We always use arguments of type 'va_list'.

    //
    // NOTE: Technique taken from eventvwr.c!GetMessageStringFromDll()
    //

    dwFlags |= FORMAT_MESSAGE_MAX_WIDTH_MASK;

    /*
     * Retrieve the message string without appending extra newlines.
     * Use the "safe" FormatMessage version (SEH-protected) to protect
     * from invalid string parameters.
     */
    dwLength = FormatMessageSafeW(dwFlags,
                                  lpSource,
                                  dwMessageId,
                                  dwLanguageId,
                                  (LPWSTR)&lpMsgBuf,
                                  0, &args);

    Len = (INT)dwLength;

    if (!lpMsgBuf)
    {
        // ASSERT(dwLength == 0);
    }
    else
    {
        // ASSERT(dwLength != 0);

        // Len = ConPrintfV(Stream, lpMsgBuf, args);
        Len = Stream->WriteFunc(Stream, lpMsgBuf, dwLength);

        /* Fixup returned length in case of errors */
        if (Len < 0)
            Len = 0;

        /* Free the buffer allocated by FormatMessage */
        LocalFree(lpMsgBuf);
    }

    return Len;
}

INT
ConMsgPrintf(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN LPCVOID lpSource OPTIONAL,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    ...)
{
    INT Len;
    va_list args;

    va_start(args, dwLanguageId);
    // ConMsgPrintf2V
    Len = ConMsgPrintfV(Stream,
                        dwFlags,
                        lpSource,
                        dwMessageId,
                        dwLanguageId,
                        args);
    va_end(args);

    return Len;
}

//
// TODO: Add Console paged-output printf & ResPrintf functions!
//
