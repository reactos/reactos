/*
 * PROJECT:     ReactOS Console Utilities Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Base set of functions for loading string resources
 *              and message strings, and handle type identification.
 * COPYRIGHT:   Copyright 2017-2018 ReactOS Team
 *              Copyright 2017-2018 Hermes Belusca-Maito
 */

/**
 * @file    utils.c
 * @ingroup ConUtils
 *
 * @brief   General-purpose utility functions (wrappers around
 *          or reimplementations of Win32 APIs).
 **/

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

/**
 * @name K32LoadStringExW
 *     Loads a string resource from the executable file associated with a
 *     specified module, copies the string into a buffer, and appends a
 *     terminating null character.
 *     This is basically the LoadString() API ripped from user32.dll to
 *     remove any dependency of ConUtils from user32.dll, and to add support
 *     for loading strings from other languages than the current one.
 *
 * @param[in]   hInstance
 *     Optional handle to an instance of the module whose executable file
 *     contains the string resource. Can be set to NULL to get the handle
 *     to the application itself.
 *
 * @param[in]   uID
 *     The identifier of the string to be loaded.
 *
 * @param[in]   LanguageId
 *     The language identifier of the resource. If this parameter is
 *     <tt>MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL)</tt>, the current language
 *     associated with the calling thread is used. To specify a language other
 *     than the current language, use the @c MAKELANGID macro to create this
 *     parameter.
 *
 * @param[out]  lpBuffer
 *     The buffer that receives the string. Must be of sufficient length
 *     to hold a pointer (8 bytes).
 *
 * @param[in]   nBufferMax
 *     The size of the buffer, in characters. The string is truncated and
 *     NULL-terminated if it is longer than the number of characters specified.
 *     If this parameter is 0, then @p lpBuffer receives a read-only pointer
 *     to the resource itself.
 *
 * @return
 *     If the function succeeds, the return value is the number of characters
 *     copied into the buffer, not including the terminating null character,
 *     or zero if the string resource does not exist. To get extended error
 *     information, call GetLastError().
 *
 * @see LoadString(), K32LoadStringW()
 **/
INT
WINAPI
K32LoadStringExW(
    IN  HINSTANCE hInstance OPTIONAL,
    IN  UINT   uID,
    IN  LANGID LanguageId,
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
    hrsrc = FindResourceExW(hInstance,
                            (LPCWSTR)RT_STRING,
                            MAKEINTRESOURCEW((LOWORD(uID) >> 4) + 1),
                            LanguageId);
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

/**
 * @name K32LoadStringW
 *     Loads a string resource from the executable file associated with a
 *     specified module, copies the string into a buffer, and appends a
 *     terminating null character.
 *     This is a restricted version of K32LoadStringExW().
 *
 * @see LoadString(), K32LoadStringExW()
 **/
INT
WINAPI
K32LoadStringW(
    IN  HINSTANCE hInstance OPTIONAL,
    IN  UINT   uID,
    OUT LPWSTR lpBuffer,
    IN  INT    nBufferMax)
{
    // NOTE: Instead of using LANG_NEUTRAL, one might use LANG_USER_DEFAULT...
    return K32LoadStringExW(hInstance, uID,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                            lpBuffer, nBufferMax);
}

/**
 * @name FormatMessageSafeW
 *     Loads and formats a message string. The function requires a message
 *     definition as input. The message definition can come from a buffer
 *     passed to the function. It can come from a message table resource in
 *     an already-loaded module, or the caller can ask the function to search
 *     the system's message table resource(s) for the message definition.
 *     Please refer to the Win32 FormatMessage() function for more details.
 *
 * @param[in]   dwFlags
 *     The formatting options, and how to interpret the @p lpSource parameter.
 *     See FormatMessage() for more details.
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
 * @param[out]  lpBuffer
 *     A pointer to a buffer that receives the null-terminated string that
 *     specifies the formatted message. If @p dwFlags includes
 *     @b FORMAT_MESSAGE_ALLOCATE_BUFFER, the function allocates a buffer
 *     using the LocalAlloc() function, and places the pointer to the buffer
 *     at the address specified in @p lpBuffer.
 *     This buffer cannot be larger than 64kB.
 *
 * @param[in]   nSize
 *     If the @b FORMAT_MESSAGE_ALLOCATE_BUFFER flag is not set, this parameter
 *     specifies the size of the output buffer, in @b TCHARs.
 *     If @b FORMAT_MESSAGE_ALLOCATE_BUFFER is set, this parameter specifies
 *     the minimum number of @b TCHARs to allocate for an output buffer.
 *     The output buffer cannot be larger than 64kB.
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
 * @return
 *     If the function succeeds, the return value is the number of characters
 *     copied into the buffer, not including the terminating null character,
 *     or zero if the string resource does not exist. To get extended error
 *     information, call GetLastError().
 *
 * @remark
 *     This function is a "safe" version of FormatMessage(), that does not
 *     crash if a malformed source string is retrieved and then being used
 *     for formatting. It basically wraps calls to FormatMessage() within SEH.
 *
 * @see <a href="https://msdn.microsoft.com/en-us/library/windows/desktop/ms679351(v=vs.85).aspx">FormatMessage() (on MSDN)</a>
 **/
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

/**
 * @name IsTTYHandle
 *     Checks whether a handle refers to a valid TTY object.
 *     A TTY object may be a console or a "communications" (e.g. serial) port.
 *
 * @param[in]   hHandle
 *     Handle to the TTY object to check for.
 *
 * @return
 *     @b TRUE when the handle refers to a valid TTY object,
 *     @b FALSE if it does not.
 *
 * @remark
 *     This test is more general than IsConsoleHandle() as it is not limited
 *     to Win32 console objects only.
 *
 * @see IsConsoleHandle()
 **/
BOOL
IsTTYHandle(IN HANDLE hHandle)
{
    /*
     * More general test than IsConsoleHandle(). Consoles, as well as serial
     * (communications) ports, etc... verify this test, but only consoles
     * verify the IsConsoleHandle() test: indeed the latter checks whether
     * the handle is really handled by the console subsystem.
     */
    return ((GetFileType(hHandle) & ~FILE_TYPE_REMOTE) == FILE_TYPE_CHAR);
}

/**
 * @name IsConsoleHandle
 *     Checks whether a handle refers to a valid Win32 console object.
 *
 * @param[in]   hHandle
 *     Handle to the Win32 console object to check for:
 *     console input buffer, console output buffer.
 *
 * @return
 *     @b TRUE when the handle refers to a valid Win32 console object,
 *     @b FALSE if it does not.
 *
 * @see IsTTYHandle()
 **/
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

/* EOF */
