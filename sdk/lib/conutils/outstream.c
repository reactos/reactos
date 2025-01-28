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
 * @file    outstream.c
 * @ingroup ConUtils
 *
 * @brief   Console I/O utility API -- Output
 **/

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


// Also known as: RC_STRING_MAX_SIZE, MAX_BUFFER_SIZE (some programs:
// wlanconf, shutdown, set it to 5024), OUTPUT_BUFFER_SIZE (name given
// in cmd/console.c), MAX_STRING_SIZE (name given in diskpart) or
// MAX_MESSAGE_SIZE (set to 512 in shutdown).
#define CON_RC_STRING_MAX_SIZE  4096


/**
 * @name ConWrite
 *     Writes a counted string to a stream.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   szStr
 *     Pointer to the counted string to write.
 *
 * @param[in]   len
 *     Length of the string pointed by @p szStr, specified
 *     in number of characters.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @note
 *     This function is used as an internal function.
 *     Use the ConStreamWrite() function instead.
 *
 * @remark
 *     Should be called with the stream locked.
 **/
INT
__stdcall
ConWrite(
    IN PCON_STREAM Stream,
    IN PCTCH szStr,
    IN DWORD len)
{
#ifndef USE_CRT
    DWORD TotalLen = len, dwNumBytes = 0;
    LPCVOID p;

    /* If we do not write anything, just return */
    if (!szStr || len == 0)
        return 0;

    /* Check whether we are writing to a console */
    // if (IsConsoleHandle(Stream->hHandle))
    if (Stream->IsConsole)
    {
        // TODO: Check if (Stream->Mode == WideText or UTF16Text) ??

        /*
         * This code is inspired from _cputws, in particular from the fact that,
         * according to MSDN: https://learn.microsoft.com/en-us/windows/console/writeconsole
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
     *   OemToCharBuffW (resp. CharToOemBuffW), but these latter functions
     *   uselessly depend on user32.dll, while MultiByteToWideChar and
     *   WideCharToMultiByte only need kernel32.dll.
     */
    if ((Stream->Mode == WideText) || (Stream->Mode == UTF16Text))
    {
#ifndef _UNICODE // UNICODE means that TCHAR == WCHAR == UTF-16
        /* Convert from the current process/thread's code page to UTF-16 */
        PWCHAR buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * sizeof(WCHAR));
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
         * write the part BEFORE the newline, then emit
         * a carriage-return + newline sequence and finally
         * write the remaining part of the buffer.
         *
         * This fixes output in files and serial console.
         */
        while (len > 0)
        {
            /* Loop until we find a newline character */
            p = szStr;
            while (len > 0 && *(PCWCH)p != L'\n')
            {
                /* Advance one character */
                p = (LPCVOID)((PCWCH)p + 1);
                --len;
            }

            /* Write everything up to \n */
            dwNumBytes = ((PCWCH)p - (PCWCH)szStr) * sizeof(WCHAR);
            WriteFile(Stream->hHandle, szStr, dwNumBytes, &dwNumBytes, NULL);

            /*
             * If we hit a newline and the previous character is not a carriage-return,
             * emit a carriage-return + newline sequence, otherwise just emit the newline.
             */
            if (len > 0 && *(PCWCH)p == L'\n')
            {
                if (p == (LPCVOID)szStr || (p > (LPCVOID)szStr && *((PCWCH)p - 1) != L'\r'))
                    WriteFile(Stream->hHandle, L"\r\n", 2 * sizeof(WCHAR), &dwNumBytes, NULL);
                else
                    WriteFile(Stream->hHandle, L"\n", sizeof(WCHAR), &dwNumBytes, NULL);

                /* Skip \n */
                p = (LPCVOID)((PCWCH)p + 1);
                --len;
            }
            szStr = p;
        }

#ifndef _UNICODE
        HeapFree(GetProcessHeap(), 0, buffer);
#endif
    }
    else if ((Stream->Mode == UTF8Text) || (Stream->Mode == AnsiText))
    {
        UINT CodePage;
        PCHAR buffer;

        /*
         * Resolve the current code page if it has not been assigned yet
         * (we do this only if the stream is in ANSI mode; in UTF8 mode
         * the code page is always set to CP_UTF8). Otherwise use the
         * current stream's code page.
         */
        if (/*(Stream->Mode == AnsiText) &&*/ (Stream->CodePage == INVALID_CP))
            CodePage = GetConsoleOutputCP(); // CP_ACP, CP_OEMCP
        else
            CodePage = Stream->CodePage;

#ifdef _UNICODE // UNICODE means that TCHAR == WCHAR == UTF-16
        /* Convert from UTF-16 to either UTF-8 or ANSI, using the stream code page */
        // NOTE: MB_LEN_MAX defined either in limits.h or in stdlib.h .
        buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len * MB_LEN_MAX);
        if (!buffer)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
        len = WideCharToMultiByte(CodePage, 0,
                                  szStr, len, buffer, len * MB_LEN_MAX,
                                  NULL, NULL);
        szStr = (PVOID)buffer;
#else
        /*
         * Convert from the current process/thread's code page to either
         * UTF-8 or ANSI, using the stream code page.
         * We need to perform a double conversion, by going through UTF-16.
         */
        // TODO!
        #error "Need to implement double conversion!"
#endif

        /*
         * Find any newline character in the buffer,
         * write the part BEFORE the newline, then emit
         * a carriage-return + newline sequence and finally
         * write the remaining part of the buffer.
         *
         * This fixes output in files and serial console.
         */
        while (len > 0)
        {
            /* Loop until we find a newline character */
            p = szStr;
            while (len > 0 && *(PCCH)p != '\n')
            {
                /* Advance one character */
                p = (LPCVOID)((PCCH)p + 1);
                --len;
            }

            /* Write everything up to \n */
            dwNumBytes = ((PCCH)p - (PCCH)szStr) * sizeof(CHAR);
            WriteFile(Stream->hHandle, szStr, dwNumBytes, &dwNumBytes, NULL);

            /*
             * If we hit a newline and the previous character is not a carriage-return,
             * emit a carriage-return + newline sequence, otherwise just emit the newline.
             */
            if (len > 0 && *(PCCH)p == '\n')
            {
                if (p == (LPCVOID)szStr || (p > (LPCVOID)szStr && *((PCCH)p - 1) != '\r'))
                    WriteFile(Stream->hHandle, "\r\n", 2, &dwNumBytes, NULL);
                else
                    WriteFile(Stream->hHandle, "\n", 1, &dwNumBytes, NULL);

                /* Skip \n */
                p = (LPCVOID)((PCCH)p + 1);
                --len;
            }
            szStr = p;
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
    (Stream)->WriteFunc((Stream), (Str), (Len))

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
    CON_STREAM_WRITE_CALL((Stream), (Str), (Len))

#endif


/**
 * @name ConStreamWrite
 *     Writes a counted string to a stream.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   szStr
 *     Pointer to the counted string to write.
 *
 * @param[in]   len
 *     Length of the string pointed by @p szStr, specified
 *     in number of characters.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 **/
INT
ConStreamWrite(
    IN PCON_STREAM Stream,
    IN PCTCH szStr,
    IN DWORD len)
{
    INT Len;
    CON_STREAM_WRITE2(Stream, szStr, len, Len);
    return Len;
}

/**
 * @name ConPuts
 *     Writes a NULL-terminated string to a stream.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   szStr
 *     Pointer to the NULL-terminated string to write.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @remark
 *     Contrary to the CRT puts() function, ConPuts() does not append
 *     a terminating new-line character. In this way it behaves more like
 *     the CRT fputs() function.
 **/
INT
ConPuts(
    IN PCON_STREAM Stream,
    IN PCWSTR szStr)
{
    INT Len;

    Len = wcslen(szStr);
    CON_STREAM_WRITE2(Stream, szStr, Len, Len);

    /* Fixup returned length in case of errors */
    if (Len < 0)
        Len = 0;

    return Len;
}

/**
 * @name ConPrintfV
 *     Formats and writes a NULL-terminated string to a stream.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   szStr
 *     Pointer to the NULL-terminated format string, that follows the same
 *     specifications as the @a szStr format string in ConPrintf().
 *
 * @param[in]   args
 *     Parameter describing a variable number of arguments,
 *     initialized with va_start(), that can be expected by the function,
 *     depending on the @p szStr format string. Each argument is used to
 *     replace a <em>format specifier</em> in the format string.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @see ConPrintf(), printf(), vprintf()
 **/
INT
ConPrintfV(
    IN PCON_STREAM Stream,
    IN PCWSTR  szStr,
    IN va_list args)
{
    INT Len;
    WCHAR bufSrc[CON_RC_STRING_MAX_SIZE];

    // Len = vfwprintf(Stream->fStream, szStr, args); // vfprintf for direct ANSI

    /*
     * Re-use szStr as the pointer to end-of-string, so as
     * to compute the string length instead of calling wcslen().
     */
    // StringCchVPrintfW(bufSrc, ARRAYSIZE(bufSrc), szStr, args);
    // Len = wcslen(bufSrc);
    StringCchVPrintfExW(bufSrc, ARRAYSIZE(bufSrc), (PWSTR*)&szStr, NULL, 0, szStr, args);
    Len = szStr - bufSrc;

    CON_STREAM_WRITE2(Stream, bufSrc, Len, Len);

    /* Fixup returned length in case of errors */
    if (Len < 0)
        Len = 0;

    return Len;
}

/**
 * @name ConPrintf
 *     Formats and writes a NULL-terminated string to a stream.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   szStr
 *     Pointer to the NULL-terminated format string, that follows the same
 *     specifications as the @a format string in printf(). This string can
 *     optionally contain embedded <em>format specifiers</em> that are
 *     replaced by the values specified in subsequent additional arguments
 *     and formatted as requested.
 *
 * @param[in]   ...
 *     Additional arguments that can be expected by the function, depending
 *     on the @p szStr format string. Each argument is used to replace a
 *     <em>format specifier</em> in the format string.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @see ConPrintfV(), printf(), vprintf()
 **/
INT
__cdecl
ConPrintf(
    IN PCON_STREAM Stream,
    IN PCWSTR szStr,
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

/**
 * @name ConResPutsEx
 *     Writes a string resource to a stream.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   hInstance
 *     Optional handle to an instance of the module whose executable file
 *     contains the string resource. Can be set to NULL to get the handle
 *     to the application itself.
 *
 * @param[in]   uID
 *     The identifier of the string to be written.
 *
 * @param[in]   LanguageId
 *     The language identifier of the resource. If this parameter is
 *     <tt>MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)</tt>, the current language
 *     associated with the calling thread is used. To specify a language other
 *     than the current language, use the @c MAKELANGID macro to create this
 *     parameter.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @remark
 *     Similarly to ConPuts(), no terminating new-line character is appended.
 *
 * @see ConPuts(), ConResPuts()
 **/
INT
ConResPutsEx(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT   uID,
    IN LANGID LanguageId)
{
    INT Len;
    PWCHAR szStr = NULL;

    Len = K32LoadStringExW(hInstance, uID, LanguageId, (PWSTR)&szStr, 0);
    if (szStr && Len)
        // Len = ConPuts(Stream, szStr);
        CON_STREAM_WRITE2(Stream, szStr, Len, Len);

    /* Fixup returned length in case of errors */
    if (Len < 0)
        Len = 0;

    return Len;
}

/**
 * @name ConResPuts
 *     Writes a string resource contained in the current application
 *     to a stream.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   uID
 *     The identifier of the string to be written.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @remark
 *     Similarly to ConPuts(), no terminating new-line character is appended.
 *
 * @see ConPuts(), ConResPutsEx()
 **/
INT
ConResPuts(
    IN PCON_STREAM Stream,
    IN UINT uID)
{
    return ConResPutsEx(Stream, NULL /*GetModuleHandleW(NULL)*/,
                        uID, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL));
}

/**
 * @name ConResPrintfExV
 *     Formats and writes a string resource to a stream.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   hInstance
 *     Optional handle to an instance of the module whose executable file
 *     contains the string resource. Can be set to NULL to get the handle
 *     to the application itself.
 *
 * @param[in]   uID
 *     The identifier of the format string. The format string follows the
 *     same specifications as the @a szStr format string in ConPrintf().
 *
 * @param[in]   LanguageId
 *     The language identifier of the resource. If this parameter is
 *     <tt>MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)</tt>, the current language
 *     associated with the calling thread is used. To specify a language other
 *     than the current language, use the @c MAKELANGID macro to create this
 *     parameter.
 *
 * @param[in]   args
 *     Parameter describing a variable number of arguments,
 *     initialized with va_start(), that can be expected by the function,
 *     depending on the @p szStr format string. Each argument is used to
 *     replace a <em>format specifier</em> in the format string.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @see ConPrintf(), ConResPrintfEx(), ConResPrintfV(), ConResPrintf()
 **/
INT
ConResPrintfExV(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT    uID,
    IN LANGID  LanguageId,
    IN va_list args)
{
    INT Len;
    WCHAR bufSrc[CON_RC_STRING_MAX_SIZE];

    // NOTE: We may use the special behaviour where nBufMaxSize == 0
    Len = K32LoadStringExW(hInstance, uID, LanguageId, bufSrc, ARRAYSIZE(bufSrc));
    if (Len)
        Len = ConPrintfV(Stream, bufSrc, args);

    return Len;
}

/**
 * @name ConResPrintfV
 *     Formats and writes a string resource contained in the
 *     current application to a stream.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   uID
 *     The identifier of the format string. The format string follows the
 *     same specifications as the @a szStr format string in ConPrintf().
 *
 * @param[in]   args
 *     Parameter describing a variable number of arguments,
 *     initialized with va_start(), that can be expected by the function,
 *     depending on the @p szStr format string. Each argument is used to
 *     replace a <em>format specifier</em> in the format string.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @see ConPrintf(), ConResPrintfExV(), ConResPrintfEx(), ConResPrintf()
 **/
INT
ConResPrintfV(
    IN PCON_STREAM Stream,
    IN UINT    uID,
    IN va_list args)
{
    return ConResPrintfExV(Stream, NULL /*GetModuleHandleW(NULL)*/,
                           uID, MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                           args);
}

/**
 * @name ConResPrintfEx
 *     Formats and writes a string resource to a stream.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   hInstance
 *     Optional handle to an instance of the module whose executable file
 *     contains the string resource. Can be set to NULL to get the handle
 *     to the application itself.
 *
 * @param[in]   uID
 *     The identifier of the format string. The format string follows the
 *     same specifications as the @a szStr format string in ConPrintf().
 *
 * @param[in]   LanguageId
 *     The language identifier of the resource. If this parameter is
 *     <tt>MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)</tt>, the current language
 *     associated with the calling thread is used. To specify a language other
 *     than the current language, use the @c MAKELANGID macro to create this
 *     parameter.
 *
 * @param[in]   ...
 *     Additional arguments that can be expected by the function, depending
 *     on the @p szStr format string. Each argument is used to replace a
 *     <em>format specifier</em> in the format string.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @see ConPrintf(), ConResPrintfExV(), ConResPrintfV(), ConResPrintf()
 **/
INT
__cdecl
ConResPrintfEx(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN UINT   uID,
    IN LANGID LanguageId,
    ...)
{
    INT Len;
    va_list args;

    va_start(args, LanguageId);
    Len = ConResPrintfExV(Stream, hInstance, uID, LanguageId, args);
    va_end(args);

    return Len;
}

/**
 * @name ConResPrintf
 *     Formats and writes a string resource contained in the
 *     current application to a stream.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   uID
 *     The identifier of the format string. The format string follows the
 *     same specifications as the @a szStr format string in ConPrintf().
 *
 * @param[in]   ...
 *     Additional arguments that can be expected by the function, depending
 *     on the @p szStr format string. Each argument is used to replace a
 *     <em>format specifier</em> in the format string.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @see ConPrintf(), ConResPrintfExV(), ConResPrintfEx(), ConResPrintfV()
 **/
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

/**
 * @name ConMsgPuts
 *     Writes a message string to a stream without formatting. The function
 *     requires a message definition as input. The message definition can come
 *     from a buffer passed to the function. It can come from a message table
 *     resource in an already-loaded module, or the caller can ask the function
 *     to search the system's message table resource(s) for the message definition.
 *     Please refer to the Win32 FormatMessage() function for more details.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   dwFlags
 *     The formatting options, and how to interpret the @p lpSource parameter.
 *     See FormatMessage() for more details. The @b FORMAT_MESSAGE_ALLOCATE_BUFFER
 *     and @b FORMAT_MESSAGE_ARGUMENT_ARRAY flags are always ignored.
 *     The function implicitly uses the @b FORMAT_MESSAGE_IGNORE_INSERTS flag
 *     to implement its behaviour.
 *
 * @param[in]   lpSource
 *     The location of the message definition. The type of this parameter
 *     depends upon the settings in the @p dwFlags parameter.
 *
 * @param[in]   dwMessageId
 *     The message identifier for the requested message. This parameter
 *     is ignored if @p dwFlags includes @b FORMAT_MESSAGE_FROM_STRING.
 *
 * @param[in]   dwLanguageId
 *     The language identifier for the requested message. This parameter
 *     is ignored if @p dwFlags includes @b FORMAT_MESSAGE_FROM_STRING.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @remark
 *     Similarly to ConPuts(), no terminating new-line character is appended.
 *
 * @see ConPuts(), ConResPuts() and associated functions,
 *      <a href="https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage">FormatMessage() (on MSDN)</a>
 **/
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
     * Sanitize dwFlags. This version always ignores explicitly the inserts
     * as we emulate the behaviour of the (f)puts function.
     */
    dwFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER; // Always allocate an internal buffer.
    dwFlags |= FORMAT_MESSAGE_IGNORE_INSERTS;  // Ignore inserts for FormatMessage.
    dwFlags &= ~FORMAT_MESSAGE_ARGUMENT_ARRAY;

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
                                  0,
                                  NULL);
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

/**
 * @name ConMsgPrintf2V
 *     Formats and writes a message string to a stream.
 *
 * @remark For internal use only.
 *
 * @see ConMsgPrintfV()
 **/
INT
ConMsgPrintf2V(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN LPCVOID lpSource OPTIONAL,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    IN va_list args)
{
    INT Len;
    DWORD dwLength  = 0;
    LPWSTR lpMsgBuf = NULL;

    /*
     * Sanitize dwFlags. This version always ignores explicitly the inserts.
     * The string that we will return to the user will not be pre-formatted.
     */
    dwFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER; // Always allocate an internal buffer.
    dwFlags |= FORMAT_MESSAGE_IGNORE_INSERTS;  // Ignore inserts for FormatMessage.
    dwFlags &= ~FORMAT_MESSAGE_ARGUMENT_ARRAY;

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
                                  0,
                                  NULL);
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

/**
 * @name ConMsgPrintfV
 *     Formats and writes a message string to a stream. The function requires
 *     a message definition as input. The message definition can come from a
 *     buffer passed to the function. It can come from a message table resource
 *     in an already-loaded module, or the caller can ask the function to search
 *     the system's message table resource(s) for the message definition.
 *     Please refer to the Win32 FormatMessage() function for more details.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   dwFlags
 *     The formatting options, and how to interpret the @p lpSource parameter.
 *     See FormatMessage() for more details.
 *     The @b FORMAT_MESSAGE_ALLOCATE_BUFFER flag is always ignored.
 *
 * @param[in]   lpSource
 *     The location of the message definition. The type of this parameter
 *     depends upon the settings in the @p dwFlags parameter.
 *
 * @param[in]   dwMessageId
 *     The message identifier for the requested message. This parameter
 *     is ignored if @p dwFlags includes @b FORMAT_MESSAGE_FROM_STRING.
 *
 * @param[in]   dwLanguageId
 *     The language identifier for the requested message. This parameter
 *     is ignored if @p dwFlags includes @b FORMAT_MESSAGE_FROM_STRING.
 *
 * @param[in]   Arguments
 *     Optional pointer to an array of values describing a variable number of
 *     arguments, depending on the message string. Each argument is used to
 *     replace an <em>insert sequence</em> in the message string.
 *     By default, the @p Arguments parameter is of type @c va_list*, initialized
 *     with va_start(). The state of the @c va_list argument is undefined upon
 *     return from the function. To use the @c va_list again, destroy the variable
 *     argument list pointer using va_end() and reinitialize it with va_start().
 *     If you do not have a pointer of type @c va_list*, then specify the
 *     @b FORMAT_MESSAGE_ARGUMENT_ARRAY flag and pass a pointer to an array
 *     of @c DWORD_PTR values; those values are input to the message formatted
 *     as the insert values. Each insert must have a corresponding element in
 *     the array.
 *
 * @remark
 *     Contrary to printf(), ConPrintf(), ConResPrintf() and associated functions,
 *     the ConMsg* functions work on format strings that contain <em>insert sequences</em>.
 *     These sequences extend the standard <em>format specifiers</em> as they
 *     allow to specify an <em>insert number</em> referring which precise value
 *     given in arguments to use.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @see ConPrintf(), ConResPrintf() and associated functions, ConMsgPrintf(),
 *      <a href="https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage">FormatMessage() (on MSDN)</a>
 **/
INT
ConMsgPrintfV(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN LPCVOID lpSource OPTIONAL,
    IN DWORD   dwMessageId,
    IN DWORD   dwLanguageId,
    IN va_list *Arguments OPTIONAL)
{
    INT Len;
    DWORD dwLength  = 0;
    LPWSTR lpMsgBuf = NULL;

    /* Sanitize dwFlags */
    dwFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER; // Always allocate an internal buffer.

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
                                  0,
                                  Arguments);

    Len = (INT)dwLength;

    if (!lpMsgBuf)
    {
        // ASSERT(dwLength == 0);
    }
    else
    {
        // ASSERT(dwLength != 0);

        CON_STREAM_WRITE2(Stream, lpMsgBuf, dwLength, Len);

        /* Fixup returned length in case of errors */
        if (Len < 0)
            Len = 0;

        /* Free the buffer allocated by FormatMessage */
        LocalFree(lpMsgBuf);
    }

    return Len;
}

/**
 * @name ConMsgPrintf
 *     Formats and writes a message string to a stream. The function requires
 *     a message definition as input. The message definition can come from a
 *     buffer passed to the function. It can come from a message table resource
 *     in an already-loaded module, or the caller can ask the function to search
 *     the system's message table resource(s) for the message definition.
 *     Please refer to the Win32 FormatMessage() function for more details.
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   dwFlags
 *     The formatting options, and how to interpret the @p lpSource parameter.
 *     See FormatMessage() for more details. The @b FORMAT_MESSAGE_ALLOCATE_BUFFER
 *     and @b FORMAT_MESSAGE_ARGUMENT_ARRAY flags are always ignored.
 *
 * @param[in]   lpSource
 *     The location of the message definition. The type of this parameter
 *     depends upon the settings in the @p dwFlags parameter.
 *
 * @param[in]   dwMessageId
 *     The message identifier for the requested message. This parameter
 *     is ignored if @p dwFlags includes @b FORMAT_MESSAGE_FROM_STRING.
 *
 * @param[in]   dwLanguageId
 *     The language identifier for the requested message. This parameter
 *     is ignored if @p dwFlags includes @b FORMAT_MESSAGE_FROM_STRING.
 *
 * @param[in]   ...
 *     Additional arguments that can be expected by the function, depending
 *     on the message string. Each argument is used to replace an
 *     <em>insert sequence</em> in the message string.
 *
 * @remark
 *     Contrary to printf(), ConPrintf(), ConResPrintf() and associated functions,
 *     the ConMsg* functions work on format strings that contain <em>insert sequences</em>.
 *     These sequences extend the standard <em>format specifiers</em> as they
 *     allow to specify an <em>insert number</em> referring which precise value
 *     given in arguments to use.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @see ConPrintf(), ConResPrintf() and associated functions, ConMsgPrintfV(),
 *      <a href="https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage">FormatMessage() (on MSDN)</a>
 **/
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

    /* Sanitize dwFlags */
    dwFlags &= ~FORMAT_MESSAGE_ARGUMENT_ARRAY;

    va_start(args, dwLanguageId);
    Len = ConMsgPrintfV(Stream,
                        dwFlags,
                        lpSource,
                        dwMessageId,
                        dwLanguageId,
                        &args);
    va_end(args);

    return Len;
}

/**
 * @name ConResMsgPrintfExV
 *     Formats and writes a message string to a stream. The function requires
 *     a message definition as input. Contrary to the ConMsg* or the Win32
 *     FormatMessage() functions, the message definition comes from a resource
 *     string table, much like the strings for ConResPrintf(), but is formatted
 *     according to the rules of ConMsgPrintf().
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   hInstance
 *     Optional handle to an instance of the module whose executable file
 *     contains the string resource. Can be set to NULL to get the handle
 *     to the application itself.
 *
 * @param[in]   dwFlags
 *     The formatting options, see FormatMessage() for more details.
 *     The only valid flags are @b FORMAT_MESSAGE_ARGUMENT_ARRAY,
 *     @b FORMAT_MESSAGE_IGNORE_INSERTS and @b FORMAT_MESSAGE_MAX_WIDTH_MASK.
 *     All the other flags are internally overridden by the function
 *     to implement its behaviour.
 *
 * @param[in]   uID
 *     The identifier of the message string. The format string follows the
 *     same specifications as the @a lpSource format string in ConMsgPrintf().
 *
 * @param[in]   LanguageId
 *     The language identifier of the resource. If this parameter is
 *     <tt>MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)</tt>, the current language
 *     associated with the calling thread is used. To specify a language other
 *     than the current language, use the @c MAKELANGID macro to create this
 *     parameter.
 *
 * @param[in]   Arguments
 *     Optional pointer to an array of values describing a variable number of
 *     arguments, depending on the message string. Each argument is used to
 *     replace an <em>insert sequence</em> in the message string.
 *     By default, the @p Arguments parameter is of type @c va_list*, initialized
 *     with va_start(). The state of the @c va_list argument is undefined upon
 *     return from the function. To use the @c va_list again, destroy the variable
 *     argument list pointer using va_end() and reinitialize it with va_start().
 *     If you do not have a pointer of type @c va_list*, then specify the
 *     @b FORMAT_MESSAGE_ARGUMENT_ARRAY flag and pass a pointer to an array
 *     of @c DWORD_PTR values; those values are input to the message formatted
 *     as the insert values. Each insert must have a corresponding element in
 *     the array.
 *
 * @remark
 *     Contrary to printf(), ConPrintf(), ConResPrintf() and associated functions,
 *     the ConMsg* functions work on format strings that contain <em>insert sequences</em>.
 *     These sequences extend the standard <em>format specifiers</em> as they
 *     allow to specify an <em>insert number</em> referring which precise value
 *     given in arguments to use.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @see ConPrintf(), ConResPrintf() and associated functions, ConMsgPrintf(),
 *      <a href="https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage">FormatMessage() (on MSDN)</a>
 **/
INT
ConResMsgPrintfExV(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN DWORD   dwFlags,
    IN UINT    uID,
    IN LANGID  LanguageId,
    IN va_list *Arguments OPTIONAL)
{
    INT Len;
    DWORD dwLength  = 0;
    LPWSTR lpMsgBuf = NULL;
    WCHAR bufSrc[CON_RC_STRING_MAX_SIZE];

    /* Retrieve the string from the resource string table */
    // NOTE: We may use the special behaviour where nBufMaxSize == 0
    Len = K32LoadStringExW(hInstance, uID, LanguageId, bufSrc, ARRAYSIZE(bufSrc));
    if (Len == 0)
        return Len;

    /* Sanitize dwFlags */
    dwFlags |= FORMAT_MESSAGE_ALLOCATE_BUFFER; // Always allocate an internal buffer.

    /* The string has already been manually loaded */
    dwFlags &= ~(FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_FROM_SYSTEM);
    dwFlags |= FORMAT_MESSAGE_FROM_STRING;

    /*
     * Retrieve the message string without appending extra newlines.
     * Use the "safe" FormatMessage version (SEH-protected) to protect
     * from invalid string parameters.
     */
    dwLength = FormatMessageSafeW(dwFlags,
                                  bufSrc,
                                  0, 0,
                                  (LPWSTR)&lpMsgBuf,
                                  0,
                                  Arguments);

    Len = (INT)dwLength;

    if (!lpMsgBuf)
    {
        // ASSERT(dwLength == 0);
    }
    else
    {
        // ASSERT(dwLength != 0);

        CON_STREAM_WRITE2(Stream, lpMsgBuf, dwLength, Len);

        /* Fixup returned length in case of errors */
        if (Len < 0)
            Len = 0;

        /* Free the buffer allocated by FormatMessage */
        LocalFree(lpMsgBuf);
    }

    return Len;
}

/**
 * @name ConResMsgPrintfV
 *     Formats and writes a message string to a stream. The function requires
 *     a message definition as input. Contrary to the ConMsg* or the Win32
 *     FormatMessage() functions, the message definition comes from a resource
 *     string table, much like the strings for ConResPrintf(), but is formatted
 *     according to the rules of ConMsgPrintf().
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   dwFlags
 *     The formatting options, see FormatMessage() for more details.
 *     The only valid flags are @b FORMAT_MESSAGE_ARGUMENT_ARRAY,
 *     @b FORMAT_MESSAGE_IGNORE_INSERTS and @b FORMAT_MESSAGE_MAX_WIDTH_MASK.
 *     All the other flags are internally overridden by the function
 *     to implement its behaviour.
 *
 * @param[in]   uID
 *     The identifier of the message string. The format string follows the
 *     same specifications as the @a lpSource format string in ConMsgPrintf().
 *
 * @param[in]   Arguments
 *     Optional pointer to an array of values describing a variable number of
 *     arguments, depending on the message string. Each argument is used to
 *     replace an <em>insert sequence</em> in the message string.
 *     By default, the @p Arguments parameter is of type @c va_list*, initialized
 *     with va_start(). The state of the @c va_list argument is undefined upon
 *     return from the function. To use the @c va_list again, destroy the variable
 *     argument list pointer using va_end() and reinitialize it with va_start().
 *     If you do not have a pointer of type @c va_list*, then specify the
 *     @b FORMAT_MESSAGE_ARGUMENT_ARRAY flag and pass a pointer to an array
 *     of @c DWORD_PTR values; those values are input to the message formatted
 *     as the insert values. Each insert must have a corresponding element in
 *     the array.
 *
 * @remark
 *     Contrary to printf(), ConPrintf(), ConResPrintf() and associated functions,
 *     the ConMsg* functions work on format strings that contain <em>insert sequences</em>.
 *     These sequences extend the standard <em>format specifiers</em> as they
 *     allow to specify an <em>insert number</em> referring which precise value
 *     given in arguments to use.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @see ConPrintf(), ConResPrintf() and associated functions, ConMsgPrintf(),
 *      <a href="https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage">FormatMessage() (on MSDN)</a>
 **/
INT
ConResMsgPrintfV(
    IN PCON_STREAM Stream,
    IN DWORD   dwFlags,
    IN UINT    uID,
    IN va_list *Arguments OPTIONAL)
{
    return ConResMsgPrintfExV(Stream, NULL /*GetModuleHandleW(NULL)*/,
                              dwFlags, uID,
                              MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                              Arguments);
}

/**
 * @name ConResMsgPrintfEx
 *     Formats and writes a message string to a stream. The function requires
 *     a message definition as input. Contrary to the ConMsg* or the Win32
 *     FormatMessage() functions, the message definition comes from a resource
 *     string table, much like the strings for ConResPrintf(), but is formatted
 *     according to the rules of ConMsgPrintf().
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   hInstance
 *     Optional handle to an instance of the module whose executable file
 *     contains the string resource. Can be set to NULL to get the handle
 *     to the application itself.
 *
 * @param[in]   dwFlags
 *     The formatting options, see FormatMessage() for more details.
 *     The only valid flags are @b FORMAT_MESSAGE_IGNORE_INSERTS and
 *     @b FORMAT_MESSAGE_MAX_WIDTH_MASK. All the other flags are internally
 *     overridden by the function to implement its behaviour.
 *
 * @param[in]   uID
 *     The identifier of the message string. The format string follows the
 *     same specifications as the @a lpSource format string in ConMsgPrintf().
 *
 * @param[in]   LanguageId
 *     The language identifier of the resource. If this parameter is
 *     <tt>MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)</tt>, the current language
 *     associated with the calling thread is used. To specify a language other
 *     than the current language, use the @c MAKELANGID macro to create this
 *     parameter.
 *
 * @param[in]   ...
 *     Additional arguments that can be expected by the function, depending
 *     on the message string. Each argument is used to replace an
 *     <em>insert sequence</em> in the message string.
 *
 * @remark
 *     Contrary to printf(), ConPrintf(), ConResPrintf() and associated functions,
 *     the ConMsg* functions work on format strings that contain <em>insert sequences</em>.
 *     These sequences extend the standard <em>format specifiers</em> as they
 *     allow to specify an <em>insert number</em> referring which precise value
 *     given in arguments to use.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @see ConPrintf(), ConResPrintf() and associated functions, ConMsgPrintf(),
 *      <a href="https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage">FormatMessage() (on MSDN)</a>
 **/
INT
__cdecl
ConResMsgPrintfEx(
    IN PCON_STREAM Stream,
    IN HINSTANCE hInstance OPTIONAL,
    IN DWORD  dwFlags,
    IN UINT   uID,
    IN LANGID LanguageId,
    ...)
{
    INT Len;
    va_list args;

    /* Sanitize dwFlags */
    dwFlags &= ~FORMAT_MESSAGE_ARGUMENT_ARRAY;

    va_start(args, LanguageId);
    Len = ConResMsgPrintfExV(Stream,
                             hInstance,
                             dwFlags,
                             uID,
                             LanguageId,
                             &args);
    va_end(args);

    return Len;
}

/**
 * @name ConResMsgPrintf
 *     Formats and writes a message string to a stream. The function requires
 *     a message definition as input. Contrary to the ConMsg* or the Win32
 *     FormatMessage() functions, the message definition comes from a resource
 *     string table, much like the strings for ConResPrintf(), but is formatted
 *     according to the rules of ConMsgPrintf().
 *
 * @param[in]   Stream
 *     Stream to which the write operation is issued.
 *
 * @param[in]   dwFlags
 *     The formatting options, see FormatMessage() for more details.
 *     The only valid flags are @b FORMAT_MESSAGE_IGNORE_INSERTS and
 *     @b FORMAT_MESSAGE_MAX_WIDTH_MASK. All the other flags are internally
 *     overridden by the function to implement its behaviour.
 *
 * @param[in]   uID
 *     The identifier of the message string. The format string follows the
 *     same specifications as the @a lpSource format string in ConMsgPrintf().
 *
 * @param[in]   ...
 *     Additional arguments that can be expected by the function, depending
 *     on the message string. Each argument is used to replace an
 *     <em>insert sequence</em> in the message string.
 *
 * @remark
 *     Contrary to printf(), ConPrintf(), ConResPrintf() and associated functions,
 *     the ConMsg* functions work on format strings that contain <em>insert sequences</em>.
 *     These sequences extend the standard <em>format specifiers</em> as they
 *     allow to specify an <em>insert number</em> referring which precise value
 *     given in arguments to use.
 *
 * @return
 *     Numbers of characters successfully written to @p Stream.
 *
 * @see ConPrintf(), ConResPrintf() and associated functions, ConMsgPrintf(),
 *      <a href="https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-formatmessage">FormatMessage() (on MSDN)</a>
 **/
INT
__cdecl
ConResMsgPrintf(
    IN PCON_STREAM Stream,
    IN DWORD dwFlags,
    IN UINT  uID,
    ...)
{
    INT Len;
    va_list args;

    /* Sanitize dwFlags */
    dwFlags &= ~FORMAT_MESSAGE_ARGUMENT_ARRAY;

    va_start(args, uID);
    Len = ConResMsgPrintfV(Stream, dwFlags, uID, &args);
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
