/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Utilities Library
 * FILE:            sdk/lib/conutils/utils.c
 * PURPOSE:         Base set of functions for loading string resources
 *                  and message strings, and handle type identification.
 * PROGRAMMERS:     - Hermes Belusca-Maito (for the library);
 *                  - All programmers who wrote the different console applications
 *                    from which I took those functions and improved them.
 */

/* FIXME: Temporary HACK before we cleanly support UNICODE functions */
#define UNICODE
#define _UNICODE

#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <winuser.h> // MAKEINTRESOURCEW, RT_STRING
#include <wincon.h>  // Console APIs (only if kernel32 support included)
#include <strsafe.h>

/* PSEH for SEH Support */
#include <pseh/pseh2.h>

// #include "conutils.h"
#include "utils.h"

/*
 * General-purpose utility functions (wrappers around,
 * or reimplementations of, Win32 APIs).
 */

#if 0 // The following function may be useful in the future...

// Performs MultiByteToWideChar then WideCharToMultiByte .
// See https://github.com/pcman-bbs/pcman-windows/blob/master/Lite/StrUtils.h#l33
// and http://www.openfoundry.org/svn/pcman/branches/OpenPCMan_2009/Lite/StrUtils.cpp
// for the idea.
int
MultiByteToMultiByte(
    // IN WORD wTranslations,
    IN DWORD dwFlags,
    IN UINT SrcCodePage,
    IN LPCSTR lpSrcString,
    IN int cbSrcChar,
    IN UINT DestCodePage,
    OUT LPSTR wDestString OPTIONAL,
    IN int cbDestChar
);

#endif

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

BOOL
IsTTYHandle(IN HANDLE hHandle)
{
    /*
     * More general test than IsConsoleHandle. Consoles, as well as
     * serial ports, etc... verify this test, but only consoles verify
     * the IsConsoleHandle test: indeed the latter checks whether
     * the handle is really handled by the console subsystem.
     */
    return ((GetFileType(hHandle) & ~FILE_TYPE_REMOTE) == FILE_TYPE_CHAR);
}

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
