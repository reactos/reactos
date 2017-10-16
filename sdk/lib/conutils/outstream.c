/*
 * PROJECT:     ReactOS Console Utilities Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Provides basic abstraction wrappers around CRT streams or
 *              Win32 console API I/O functions, to deal with i18n + Unicode
 *              related problems.
 * COPYRIGHT:   Copyright 2017-2018 ReactOS Team
 *              Copyright 2017-2018 Hermes Belusca-Maito
 */

/*
 * Enable this define if you want to only use CRT functions to output
 * UNICODE stream to the console, as in the way explained by
 * http://archives.miloush.net/michkap/archive/2008/03/18/8306597.html
 */
/** NOTE: Experimental! Don't use USE_CRT yet because output to console is a bit broken **/
// #define USE_CRT

/* FIXME: Temporary HACK before we cleanly support UNICODE functions */
#define UNICODE
#define _UNICODE

#ifdef USE_CRT
#include <fcntl.h>
#include <io.h>
#endif /* USE_CRT */

#include <stdlib.h> // limits.h // For MB_LEN_MAX

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winuser.h> // MAKEINTRESOURCEW, RT_STRING
#include <wincon.h>  // Console APIs (only if kernel32 support included)
#include <strsafe.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

#include "conutils.h"
#include "stream.h"
#include "stream_private.h"


// #define RC_STRING_MAX_SIZE  4096
#define CON_RC_STRING_MAX_SIZE  4096
// #define MAX_BUFFER_SIZE     4096    // Some programs (wlanconf, shutdown) set it to 5024
// #define OUTPUT_BUFFER_SIZE  4096    // Name given in cmd/console.c
// MAX_STRING_SIZE  // Name given in diskpart

// #define MAX_MESSAGE_SIZE    512     // See shutdown...


/*
 * Console I/O utility API -- Output
 */

// NOTE: Should be called with the stream locked.
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
    if (Stream->IsConsole)
    {
        // TODO: Check if (ConStream->Mode == WideText or UTF16Text) ??

        /*
         * This code is inspired from _cputws, in particular from the fact that,
         * according to MSDN: https://msdn.microsoft.com/en-us/library/ms687401(v=vs.85).aspx
         * the buffer size must be less than 64 KB.
         *
         * A similar code can be used for implementing _cputs too.
         */

        DWORD cchWrite;
        TotalLen = len, dwNumBytes = 0;

        while (len > 0)
        {
            cchWrite = min(len, 65535 / sizeof(WCHAR));

            // FIXME: Check return value!
            WriteConsole(Stream->hHandle, szStr, cchWrite, &dwNumBytes, NULL);

            szStr += cchWrite;
            len -= cchWrite;
        }

        return (INT)TotalLen; // FIXME: Really return the number of chars written!
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
#ifndef _UNICODE // UNICODE means that TCHAR == WCHAR == UTF-16
        /* Convert from the current process/thread's codepage to UTF-16 */
        WCHAR *buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
        if (!buffer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
        len = (DWORD)MultiByteToWideChar(CP_THREAD_ACP, // CP_ACP, CP_OEMCP
                                         0, szStr, (INT)len, buffer, (INT)len);
        szStr = (PVOID)buffer;
#else
        /*
         * Do not perform any conversion since we are already in UTF-16,
         * that is the same encoding as the stream.
         */
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
        CHAR *buffer;

        /*
         * Resolve the codepage cache if it was not assigned yet
         * (only if the stream is in ANSI mode; in UTF8 mode the
         * codepage was already set to CP_UTF8).
         */
        if (/*(Stream->Mode == AnsiText) &&*/ (Stream->CodePage == INVALID_CP))
            Stream->CodePage = GetConsoleOutputCP(); // CP_ACP, CP_OEMCP

#ifdef _UNICODE // UNICODE means that TCHAR == WCHAR == UTF-16
        /* Convert from UTF-16 to either UTF-8 or ANSI, using stream codepage */
        // NOTE: MB_LEN_MAX defined either in limits.h or in stdlib.h .
        buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * MB_LEN_MAX);
        if (!buffer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
        len = WideCharToMultiByte(Stream->CodePage, 0,
                                  szStr, len, buffer, len * MB_LEN_MAX,
                                  NULL, NULL);
        szStr = (PVOID)buffer;
#else
        /*
         * Convert from the current process/thread's codepage to either
         * UTF-8 or ANSI, using stream codepage.
         * We need to perform a double conversion, by going through UTF-16.
         */
        // TODO!
        #error "Need to implement double conversion!"
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
#else
        // TODO!
#endif
    }
    else // if (Stream->Mode == Binary)
    {
        /* Directly output the string */
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

#if 1
    /*
     * There is no "counted" printf-to-stream or puts-like function, therefore
     * we use this trick to output the counted string to the stream.
     */
    while (1)
    {
        written = fwprintf(Stream->fStream, L"%.*s", total, szStr);
        if (written < total)
        {
            /*
             * Some embedded NULL or special character
             * was encountered, print it apart.
             */
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
    /* ANSI text or Binary output only */
    _setmode(_fileno(Stream->fStream), _O_TEXT); // _O_BINARY
    return fwrite(szStr, sizeof(*szStr), len, Stream->fStream);
#endif

#endif /* defined(USE_CRT) */
}


#define CON_STREAM_WRITE_CALL(Stream, Str, Len) \
    (Stream)->WriteFunc((Stream), (Str), (Len));

/* Lock the stream only in non-USE_CRT mode (otherwise use the CRT stream lock) */
#ifndef USE_CRT

#define CON_STREAM_WRITE2(Stream, Str, Len, RetLen) \
do { \
    EnterCriticalSection(&(Stream)->Lock); \
    (RetLen) = CON_STREAM_WRITE_CALL((Stream), (Str), (Len)); \
    LeaveCriticalSection(&(Stream)->Lock); \
} while(0)

#define CON_STREAM_WRITE(Stream, Str, Len) \
do { \
    EnterCriticalSection(&(Stream)->Lock); \
    CON_STREAM_WRITE_CALL((Stream), (Str), (Len)); \
    LeaveCriticalSection(&(Stream)->Lock); \
} while(0)

#else

#define CON_STREAM_WRITE2(Stream, Str, Len, RetLen) \
do { \
    (RetLen) = CON_STREAM_WRITE_CALL((Stream), (Str), (Len)); \
} while(0)

#define CON_STREAM_WRITE(Stream, Str, Len) \
do { \
    CON_STREAM_WRITE_CALL((Stream), (Str), (Len)); \
} while(0)

#endif


INT
ConStreamWrite(
    IN PCON_STREAM Stream,
    IN PTCHAR szStr,
    IN DWORD len)
{
    INT Len;
    CON_STREAM_WRITE2(Stream, szStr, len, Len);
    return Len;
}

INT
ConPuts(
    IN PCON_STREAM Stream,
    IN LPWSTR szStr)
{
    INT Len;

    Len = wcslen(szStr);
    CON_STREAM_WRITE2(Stream, szStr, Len, Len);

    /* Fixup returned length in case of errors */
    if (Len < 0)
        Len = 0;

    return Len;
}

INT
ConPrintfV(
    IN PCON_STREAM Stream,
    IN LPWSTR  szStr,
    IN va_list args) // arg_ptr
{
    INT Len;
    WCHAR bufSrc[CON_RC_STRING_MAX_SIZE];

    // Len = vfwprintf(Stream->fStream, szStr, args); // vfprintf for direct ANSI

    /*
     * Reuse szStr as the pointer to end-of-string, to compute
     * the string length instead of calling wcslen().
     */
    // StringCchVPrintfW(bufSrc, ARRAYSIZE(bufSrc), szStr, args);
    // Len = wcslen(bufSrc);
    StringCchVPrintfExW(bufSrc, ARRAYSIZE(bufSrc), &szStr, NULL, 0, szStr, args);
    Len = szStr - bufSrc;

    CON_STREAM_WRITE2(Stream, bufSrc, Len, Len);

    /* Fixup returned length in case of errors */
    if (Len < 0)
        Len = 0;

    return Len;
}

INT
__cdecl
ConPrintf(
    IN PCON_STREAM Stream,
    IN LPWSTR szStr,
    ...)
{
    INT Len;
    va_list args;

    // Len = vfwprintf(Stream->fStream, szMsgBuf, args); // vfprintf for direct ANSI

    // StringCchPrintfW
    va_start(args, szStr);
    Len = ConPrintfV(Stream, szStr, args);
    va_end(args);

    return Len;
}

INT
ConResPutsEx(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT uID)
{
    INT Len;
    PWCHAR szStr = NULL;

    Len = K32LoadStringW(hInstance, uID, (PWSTR)&szStr, 0);
    if (szStr && Len)
        // Len = ConPuts(Stream, szStr);
        CON_STREAM_WRITE2(Stream, szStr, Len, Len);

    /* Fixup returned length in case of errors */
    if (Len < 0)
        Len = 0;

    return Len;
}

INT
ConResPuts(
    IN PCON_STREAM Stream,
    IN UINT uID)
{
    return ConResPutsEx(Stream, NULL /*GetModuleHandleW(NULL)*/, uID);
}

INT
ConResPrintfExV(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT    uID,
    IN va_list args) // arg_ptr
{
    INT Len;
    WCHAR bufSrc[CON_RC_STRING_MAX_SIZE];

    // NOTE: We may use the special behaviour where nBufMaxSize == 0
    Len = K32LoadStringW(hInstance, uID, bufSrc, ARRAYSIZE(bufSrc));
    if (Len)
        Len = ConPrintfV(Stream, bufSrc, args);

    return Len;
}

INT
ConResPrintfV(
    IN PCON_STREAM Stream,
    IN UINT    uID,
    IN va_list args) // arg_ptr
{
    return ConResPrintfExV(Stream, NULL /*GetModuleHandleW(NULL)*/, uID, args);
}

INT
__cdecl
ConResPrintfEx(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT uID,
    ...)
{
    INT Len;
    va_list args;

    va_start(args, uID);
    Len = ConResPrintfExV(Stream, hInstance, uID, args);
    va_end(args);

    return Len;
}

INT
__cdecl
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
ConMsgPuts(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN LPCVOID lpSource OPTIONAL,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId)
{
    INT Len;
    DWORD dwLength  = 0;
    LPWSTR lpMsgBuf = NULL;

    /*
     * Sanitize dwFlags. This version always ignore explicitely the inserts
     * as we emulate the behaviour of the *puts function.
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
        // Len = ConPuts(Stream, lpMsgBuf);
        CON_STREAM_WRITE2(Stream, lpMsgBuf, dwLength, Len);

        /* Fixup returned length in case of errors */
        if (Len < 0)
            Len = 0;

        /* Free the buffer allocated by FormatMessage */
        LocalFree(lpMsgBuf);
    }

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
        // CON_STREAM_WRITE2(Stream, lpMsgBuf, dwLength, Len);

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
        CON_STREAM_WRITE2(Stream, lpMsgBuf, dwLength, Len);

        /* Fixup returned length in case of errors */
        if (Len < 0)
            Len = 0;

        /* Free the buffer allocated by FormatMessage */
        LocalFree(lpMsgBuf);
    }

    return Len;
}

INT
__cdecl
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



VOID
ConClearLine(IN PCON_STREAM Stream)
{
    HANDLE hOutput = ConStreamGetOSHandle(Stream);

    /*
     * Erase the full line where the cursor is, and move
     * the cursor back to the beginning of the line.
     */

    if (IsConsoleHandle(hOutput))
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        DWORD dwWritten;

        GetConsoleScreenBufferInfo(hOutput, &csbi);

        csbi.dwCursorPosition.X = 0;
        // csbi.dwCursorPosition.Y;

        FillConsoleOutputCharacterW(hOutput, L' ',
                                    csbi.dwSize.X,
                                    csbi.dwCursorPosition,
                                    &dwWritten);
        SetConsoleCursorPosition(hOutput, csbi.dwCursorPosition);
    }
    else if (IsTTYHandle(hOutput))
    {
        ConPuts(Stream, L"\x1B[2K\x1B[1G"); // FIXME: Just use WriteFile
    }
    // else, do nothing for files
}

/* EOF */
