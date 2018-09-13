/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gfrapiw.cxx

Abstract:

    UNICODE versions of Windows Internet Extensions (WINX) Gopher Protocol APIs
    (in gfrapia.c)

    Contents:
        GopherCreateLocatorW
        GopherGetLocatorType
        GopherFindFirstFileW
        GopherFindNextW
        GopherOpenFileW
        GopherGetAttributeW
        GopherSendDataW

Author:

    Richard L Firth (rfirth) 31-Oct-1994

Environment:

    Win32 user-level DLL

Revision History:

    31-Oct-1994 rfirth
        Created

--*/

#include <wininetp.h>
#include "gfrapih.h"


//
// functions
//


INTERNETAPI
BOOL
WINAPI
GopherCreateLocatorW(
    IN LPCWSTR lpszHost,
    IN INTERNET_PORT nServerPort,
    IN LPCWSTR lpszDisplayString OPTIONAL,
    IN LPCWSTR lpszSelectorString OPTIONAL,
    IN DWORD dwGopherType,
    OUT LPWSTR lpszLocator,
    IN OUT LPDWORD lpdwBufferLength
    )
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "GopherCreateLocatorW",
                     "%wq, %d, %wq, %wq, %#x, %#x, %#x [%d]",
                     lpszHost,
                     nServerPort,
                     lpszDisplayString,
                     lpszSelectorString,
                     dwGopherType,
                     lpszLocator,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpLocator,mpDisplay,mpSelector,mpHost;
    BOOL fResult = FALSE;

    if (!lpszHost || !lpdwBufferLength)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(lpszHost,0,mpHost);
    if (!mpHost.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(lpszHost,mpHost);
    if (lpszDisplayString)
    {
        ALLOC_MB(lpszDisplayString,0,mpDisplay);
        if (!mpDisplay.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszDisplayString,mpDisplay);
    }
    if (lpszSelectorString)
    {
        ALLOC_MB(lpszSelectorString,0,mpSelector);
        if (!mpSelector.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszSelectorString,mpSelector);
    }
    mpLocator.dwSize = *lpdwBufferLength;

    if (lpszLocator)
    {
        mpLocator.dwAlloc = *lpdwBufferLength*sizeof(CHAR);
        mpLocator.psStr = (LPSTR)ALLOC_BYTES(mpLocator.dwAlloc);
        if (!mpLocator.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    fResult = GopherCreateLocatorA(mpHost.psStr,nServerPort,mpDisplay.psStr,mpSelector.psStr,
        dwGopherType,mpLocator.psStr,&mpLocator.dwSize);

    *lpdwBufferLength = mpLocator.dwSize*sizeof(WCHAR);

    if (fResult && (*lpdwBufferLength <= mpLocator.dwAlloc))
    {
        *lpdwBufferLength = (MultiByteToWideChar(CP_ACP, 0, mpLocator.psStr, -1,
                    lpszLocator, mpLocator.dwAlloc/sizeof(WCHAR))+1)*sizeof(WCHAR);
    }

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
GopherGetLocatorTypeW(
    IN LPCWSTR lpszLocator,
    OUT LPDWORD lpdwGopherType
    )
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "GopherGetLocatorTypeW",
                     "%wq, %#x",
                     lpszLocator,
                     lpdwGopherType
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpLocator;

    if (!lpszLocator)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(lpszLocator,0,mpLocator);
    if (!mpLocator.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(lpszLocator,mpLocator);
    fResult = GopherGetLocatorTypeA(mpLocator.psStr,lpdwGopherType);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


BOOL
TransformGopherFindDataToW(LPGOPHER_FIND_DATAA pgfdA, LPGOPHER_FIND_DATAW pgfdW)
{
    pgfdW->GopherType = pgfdA->GopherType;
    pgfdW->SizeLow = pgfdA->SizeLow;
    pgfdW->SizeHigh = pgfdA->SizeHigh;
    pgfdW->LastModificationTime = pgfdA->LastModificationTime;
    MultiByteToWideChar(CP_ACP, 0, pgfdA->DisplayString, -1,
            pgfdW->DisplayString, MAX_GOPHER_DISPLAY_TEXT + 1);
    MultiByteToWideChar(CP_ACP, 0, pgfdA->Locator, -1,
            pgfdW->Locator, MAX_GOPHER_LOCATOR_LENGTH + 1);

    return TRUE;
}


INTERNETAPI
HINTERNET
WINAPI
GopherFindFirstFileW(
    IN HINTERNET hGopherSession,
    IN LPCWSTR lpszLocator OPTIONAL,
    IN LPCWSTR lpszSearchString OPTIONAL,
    OUT LPGOPHER_FIND_DATAW lpBuffer OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "GopherFindFirstFileW",
                     "%#x, %wq, %wq, %#x, %#x, %$x",
                     hGopherSession,
                     lpszLocator,
                     lpszSearchString,
                     lpBuffer,
                     dwFlags,
                     dwContext
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    HINTERNET hInternet=NULL;
    MEMORYPACKET mpLocator, mpSearch;
    GOPHER_FIND_DATAA gfda;

    if (lpszLocator)
    {
        ALLOC_MB(lpszLocator,0,mpLocator);
        if (!mpLocator.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszLocator,mpLocator);
    }
    if (lpszSearchString)
    {
        ALLOC_MB(lpszSearchString,0,mpSearch);
        if (!mpSearch.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszSearchString,mpSearch);
    }
    hInternet = GopherFindFirstFileA(hGopherSession, mpLocator.psStr, mpSearch.psStr,
                                    &gfda, dwFlags, dwContext);
    if (hInternet && lpBuffer)
    {
        TransformGopherFindDataToW(&gfda, lpBuffer);
    }


cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}


BOOL
GopherFindNextW(
    IN HINTERNET hFind,
    OUT LPGOPHER_FIND_DATA lpBuffer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


INTERNETAPI
HINTERNET
WINAPI
GopherOpenFileW(
    IN HINTERNET hGopherSession,
    IN LPCWSTR lpszLocator,
    IN LPCWSTR lpszView OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "GopherOpenFileW",
                     "%#x, %wq, %wq, %#x, %#x",
                     hGopherSession,
                     lpszLocator,
                     lpszView,
                     dwFlags,
                     dwContext
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    HINTERNET hInternet = NULL;
    MEMORYPACKET mpLocator,mpView;

    if (!lpszLocator)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(lpszLocator,0,mpLocator);
    if (!mpLocator.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(lpszLocator,mpLocator);
    if (lpszView)
    {
        ALLOC_MB(lpszView,0,mpView);
        if (!mpView.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszView,mpView);
    }
    hInternet = GopherOpenFileA(hGopherSession,mpLocator.psStr,mpView.psStr,dwFlags,dwContext);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}


INTERNETAPI
BOOL
WINAPI
GopherGetAttributeW(
    IN HINTERNET hGopherSession,
    IN LPCWSTR lpszLocator,
    IN LPCWSTR lpszAttributeName OPTIONAL,
    OUT LPBYTE lpBuffer,
    IN DWORD dwBufferLength,
    OUT LPDWORD lpdwCharactersReturned,
    IN GOPHER_ATTRIBUTE_ENUMERATOR lpfnEnumerator OPTIONAL,
    IN DWORD_PTR dwContext
    )
{
#if !defined(GOPHER_ATTRIBUTE_SUPPORT)

    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;

#else
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "GopherGetAttributeW",
                     "%#x, %wq, %wq, %#x, %d, %#x, %#x, %#x",
                     hGopherSession,
                     lpszLocator,
                     lpszAttributeName,
                     lpBuffer,
                     dwBufferLength,
                     lpdwCharactersReturned,
                     lpfnEnumerator,
                     dwContext
                     ));

    // WARNING: This function may not function after all; You've been warned.
    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpLocator,mpAttribute;

    ALLOC_MB(lpszLocator,0,mpLocator);
    if (!mpLocator.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(lpszLocator,mpLocator);
    if (lpszAttributeName)
    {
        ALLOC_MB(lpszAttributeName,0,mpAttribute);
        if (!mpAttribute.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszAttributeName,mpAttribute);
    }
    fResult = GopherGetAttributeA(hGopherSession,mpLocator.psStr,mpAttribute.psStr,
        lpBuffer,dwBufferLength,lpdwCharactersReturned, lpfnEnumerator, dwContext);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        DEBUG_ERROR(API, dwErr);
        SetLastError(dwErr); 
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
#endif // defined(GOPHER_ATTRIBUTE_SUPPORT)
}

//
//INTERNETAPI
//BOOL
//WINAPI
//GopherSendDataW(
//    IN HINTERNET hGopherSession,
//    IN LPCWSTR lpszLocator,
//    IN LPCWSTR lpszBuffer,
//    IN DWORD dwNumberOfCharactersToSend,
//    OUT LPDWORD lpdwNumberOfCharactersSent,
//    IN DWORD dwContext
//    )
//{
//#if 1
//    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
//    return FALSE;
//#else
//    DWORD dwErr;
//    MEMORYPACKET mpLocator,mpView;
//
//    MAKE_ANSI(lpszLocator,0,mpLocator);
//    if (lpszView)
//    {
//       MAKE_ANSI(lpszView,0,mpView);
//    }
//    return GopherSendDataA(hGopherSession,mpLocator.psStr,mpView.psStr,dwFlags);
//
//    LEAVE_API_CALL(FALSE);
//#endif
//}
