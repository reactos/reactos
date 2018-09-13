/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inetapiw.cxx

Abstract:

    Contains the wide-character Internet APIs

    Contents:
        InternetCrackUrlW
        InternetCreateUrlW
        InternetCanonicalizeUrlW
        InternetCombineUrlW
        InternetOpenW
        InternetConnectW
        InternetOpenUrlW
        InternetReadFileExW
        InternetWriteFileExW
        InternetFindNextFileW
        InternetQueryOptionW
        InternetSetOptionW
        InternetGetLastResponseInfoW
        InternetSetStatusCallbackW

Author:

    Richard L Firth (rfirth) 02-Mar-1995

Environment:

    Win32(s) user-mode DLL

Revision History:

    02-Mar-1995 rfirth
        Created

--*/

#include <wininetp.h>

//  because wininet doesnt know about IStream
#define NO_SHLWAPI_STREAM
#include <shlwapi.h>
#include <shlwapip.h>

extern BOOL TransformFtpFindDataToW(LPWIN32_FIND_DATAA pfdA, LPWIN32_FIND_DATAW pfdW);
extern BOOL TransformGopherFindDataToW(LPGOPHER_FIND_DATAA pgfdA, LPGOPHER_FIND_DATAW pgfdW);

// -- FixStrings ------

//  Used in InternetCrackUrlW only.
//  Either
//  (a) If we have an ansi string, AND a unicode buffer, convert from ansi to unicode
//  (b) If we have an ansi string, but NO unicode buffer, determine where the ansi string
//         occurs in the unicode URL, and point the component there.

VOID
FixStrings(    
    LPSTR& pszA, 
    DWORD cbA, 
    LPWSTR& pszW, 
    DWORD& ccW, 
    LPSTR pszUrlA, 
    LPCWSTR pszUrlW)
{
    if (!pszA)
        return;

    if (pszW) 
    {
        ccW = MultiByteToWideChar(CP_ACP, 0, pszA, cbA+1, pszW, ccW) - 1; 
    } 
    else 
    { 
        pszW = (LPWSTR)(pszUrlW + MultiByteToWideChar(CP_ACP, 0, 
                pszUrlA, (int) (pszA-pszUrlA), NULL, 0)); 
        ccW = MultiByteToWideChar(CP_ACP, 0, pszA, cbA, NULL, 0); 
    } 

    if ((LONG)ccW < 0)
        ccW = 0;
}

//
// functions
//


INTERNETAPI
BOOL
WINAPI
InternetCrackUrlW(
    IN LPCWSTR pszUrlW,
    IN DWORD dwUrlLengthW,
    IN DWORD dwFlags,
    IN OUT LPURL_COMPONENTSW pUCW
    )

/*++

Routine Description:

    Cracks an URL into its constituent parts. Optionally escapes the url-path.
    We assume that the user has supplied large enough buffers for the various
    URL parts

Arguments:

    pszUrl         - pointer to URL to crack

    dwUrlLength     - 0 if pszUrl is ASCIIZ string, else length of pszUrl

    dwFlags         - flags controlling operation

    lpUrlComponents - pointer to URL_COMPONENTS

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCrackUrlW",
                     "%wq, %#x, %#x, %#x",
                     pszUrlW,
                     dwUrlLengthW,
                     dwFlags,
                     pUCW
                     ));

    INET_ASSERT(pszUrlW);
    INET_ASSERT(pUCW);

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    BOOL fContinue;
    DWORD c;
    MEMORYPACKET mpUrlA, mpHostName, mpUserName, mpScheme, mpPassword, mpUrlPath, mpExtraInfo;
    URL_COMPONENTSA UCA;

    UCA.dwStructSize = sizeof(URL_COMPONENTSA); 
    ALLOC_MB(pszUrlW, dwUrlLengthW, mpUrlA);
    if (!mpUrlA.psStr) {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
    }
    UNICODE_TO_ANSI(pszUrlW, mpUrlA);

    for (c=0; c<=5; c++) {
        LPWSTR pszWorker;
        DWORD ccLen;
        MEMORYPACKET* pmpWorker;
        
        switch(c)
        {
        case 0:
            pszWorker = pUCW->lpszScheme;
            ccLen = pUCW->dwSchemeLength;
            pmpWorker = &mpScheme;
            break;

        case 1:
            pszWorker = pUCW->lpszHostName;
            ccLen = pUCW->dwHostNameLength;
            pmpWorker = &mpHostName;
            break;

        case 2:
            pszWorker = pUCW->lpszUserName;
            ccLen = pUCW->dwUserNameLength;
            pmpWorker = &mpUserName;
            break;

        case 3:
            pszWorker = pUCW->lpszPassword;
            ccLen = pUCW->dwPasswordLength;
            pmpWorker = &mpPassword;
            break;

        case 4:
            pszWorker = pUCW->lpszUrlPath;
            ccLen = pUCW->dwUrlPathLength;
            pmpWorker = &mpUrlPath;
            break;

        case 5:
            pszWorker = pUCW->lpszExtraInfo;
            ccLen = pUCW->dwExtraInfoLength;
            pmpWorker = &mpExtraInfo;
            break;
        }

        if (pszWorker) { 
            ALLOC_MB(pszWorker,ccLen,(*pmpWorker)); 
            if (!pmpWorker->psStr) {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
        } else { 
            pmpWorker->dwAlloc = ccLen; 
        }
    };

    REASSIGN_ALLOC(mpScheme,UCA.lpszScheme,UCA.dwSchemeLength);
    REASSIGN_ALLOC(mpHostName, UCA.lpszHostName,UCA.dwHostNameLength);
    REASSIGN_ALLOC(mpUserName, UCA.lpszUserName,UCA.dwUserNameLength);
    REASSIGN_ALLOC(mpPassword,UCA.lpszPassword,UCA.dwPasswordLength);
    REASSIGN_ALLOC(mpUrlPath,UCA.lpszUrlPath,UCA.dwUrlPathLength);
    REASSIGN_ALLOC(mpExtraInfo,UCA.lpszExtraInfo,UCA.dwExtraInfoLength);
                
    fResult = InternetCrackUrlA(mpUrlA.psStr, mpUrlA.dwSize, dwFlags, &UCA);
    if (fResult) {
        FixStrings(UCA.lpszScheme, UCA.dwSchemeLength, pUCW->lpszScheme, 
                    pUCW->dwSchemeLength, mpUrlA.psStr, pszUrlW);
        FixStrings(UCA.lpszHostName, UCA.dwHostNameLength, pUCW->lpszHostName, 
                    pUCW->dwHostNameLength, mpUrlA.psStr, pszUrlW);
        FixStrings(UCA.lpszUserName, UCA.dwUserNameLength, pUCW->lpszUserName, 
                    pUCW->dwUserNameLength, mpUrlA.psStr, pszUrlW);
        FixStrings(UCA.lpszPassword, UCA.dwPasswordLength, pUCW->lpszPassword, 
                    pUCW->dwPasswordLength, mpUrlA.psStr, pszUrlW);
        FixStrings(UCA.lpszUrlPath, UCA.dwUrlPathLength, pUCW->lpszUrlPath, 
                    pUCW->dwUrlPathLength, mpUrlA.psStr, pszUrlW);
        FixStrings(UCA.lpszExtraInfo, UCA.dwExtraInfoLength, pUCW->lpszExtraInfo, 
                    pUCW->dwExtraInfoLength, mpUrlA.psStr, pszUrlW);
        pUCW->nScheme = UCA.nScheme;
        pUCW->nPort = UCA.nPort;
        pUCW->dwStructSize = sizeof(URL_COMPONENTSW);
    }

cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
InternetCreateUrlW(
    IN LPURL_COMPONENTSW pUCW,
    IN DWORD dwFlags,
    OUT LPWSTR pszUrlW,
    IN OUT LPDWORD pdwUrlLengthW
    )

/*++

Routine Description:

    Creates an URL from its constituent parts

Arguments:

Return Value:

    BOOL
        Success - URL written to pszUrl

        Failure - call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCreateUrlW",
                     "%#x, %#x, %#x, %#x",
                     pUCW,
                     dwFlags,
                     pszUrlW,
                     pdwUrlLengthW
                     ));

    INET_ASSERT(pszUrlW);
    INET_ASSERT(pUCW);

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpUrlA, mpHostName, mpUserName, mpScheme, mpPassword, mpUrlPath, mpExtraInfo;
    URL_COMPONENTSA UCA;

    if (!pdwUrlLengthW)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    if (pszUrlW)
    {
        ALLOC_MB(pszUrlW, *pdwUrlLengthW, mpUrlA);
        if (!mpUrlA.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }
    mpUrlA.dwSize = mpUrlA.dwAlloc;
    UCA.dwStructSize = sizeof(URL_COMPONENTSA);

    UCA.nScheme = pUCW->nScheme;
    UCA.nPort = pUCW->nPort;
    if (pUCW->lpszScheme)
    {
        ALLOC_MB(pUCW->lpszScheme, pUCW->dwSchemeLength, mpScheme);
        if (!mpScheme.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pUCW->lpszScheme, mpScheme);
    }
    REASSIGN_SIZE(mpScheme, UCA.lpszScheme, UCA.dwSchemeLength);
    if (pUCW->lpszHostName)
    {
        ALLOC_MB(pUCW->lpszHostName, pUCW->dwHostNameLength, mpHostName);
        if (!mpHostName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pUCW->lpszHostName, mpHostName);
    }
    REASSIGN_SIZE(mpHostName, UCA.lpszHostName, UCA.dwHostNameLength);
    if (pUCW->lpszUserName)
    {
        ALLOC_MB(pUCW->lpszUserName, pUCW->dwUserNameLength, mpUserName);
        if (!mpUserName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pUCW->lpszUserName, mpUserName);
    }
    REASSIGN_SIZE(mpUserName, UCA.lpszUserName, UCA.dwUserNameLength);
    if (pUCW->lpszPassword)
    {
        ALLOC_MB(pUCW->lpszPassword, pUCW->dwPasswordLength, mpPassword);
        if (!mpPassword.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pUCW->lpszPassword, mpPassword);
    }
    REASSIGN_SIZE(mpPassword, UCA.lpszPassword, UCA.dwPasswordLength);
    if (pUCW->lpszUrlPath)
    {
        ALLOC_MB(pUCW->lpszUrlPath, pUCW->dwUrlPathLength, mpUrlPath); 
        if (!mpUrlPath.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pUCW->lpszUrlPath, mpUrlPath);
    }
    REASSIGN_SIZE(mpUrlPath, UCA.lpszUrlPath, UCA.dwUrlPathLength);
    if (pUCW->lpszExtraInfo)
    {
        ALLOC_MB(pUCW->lpszExtraInfo, pUCW->dwExtraInfoLength, mpExtraInfo);
        if (!mpExtraInfo.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pUCW->lpszExtraInfo, mpExtraInfo);
    }
    REASSIGN_SIZE(mpExtraInfo, UCA.lpszExtraInfo, UCA.dwExtraInfoLength);
    fResult = InternetCreateUrlA(&UCA, dwFlags, mpUrlA.psStr, &mpUrlA.dwSize);
    if (fResult)
    {
        MAYBE_COPY_ANSI(mpUrlA, pszUrlW, *pdwUrlLengthW);
    }
    else
    {
        *pdwUrlLengthW = mpUrlA.dwSize*sizeof(WCHAR);
    }

cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }

    DEBUG_LEAVE_API(fResult);
    return fResult;
}


// implemented in inetapia.cxx
DWORD ICUHrToWin32Error(HRESULT);


INTERNETAPI
BOOL
WINAPI
InternetCanonicalizeUrlW(
    IN LPCWSTR pszUrl,
    OUT LPWSTR pszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Combines a relative URL with a base URL to form a new full URL.

Arguments:

    pszUrl             - pointer to URL to be canonicalize

    pszBuffer          - pointer to buffer where new URL is written

    lpdwBufferLength    - size of buffer on entry, length of new URL on exit

    dwFlags             - flags controlling operation

Return Value:

    BOOL                - TRUE if successful, FALSE if not

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCanonicalizeUrlW",
                     "%wq, %#x, %#x [%d], %#x",
                     pszUrl,
                     pszBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0,
                     dwFlags
                     ));

    HRESULT hr ;
    BOOL bRet;

    INET_ASSERT(pszUrl);
    INET_ASSERT(pszBuffer);
    INET_ASSERT(lpdwBufferLength && (*lpdwBufferLength > 0));

    //
    //  the flags for the Url* APIs in shlwapi should be the same
    //  except that NO_ENCODE is on by default.  so we need to flip it
    //
    dwFlags ^= ICU_NO_ENCODE;

    // Check for invalid parameters

    if (!pszUrl || !pszBuffer || !lpdwBufferLength || *lpdwBufferLength == 0)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = UrlCanonicalizeW(pszUrl, pszBuffer,
                    lpdwBufferLength, dwFlags | URL_WININET_COMPATIBILITY);
    }

    if(FAILED(hr))
    {
        DWORD dw = ICUHrToWin32Error(hr);

        bRet = FALSE;

        DEBUG_ERROR(INET, dw);

        SetLastError(dw);
    }
    else
        bRet = TRUE;

    DEBUG_LEAVE_API(bRet);
    return bRet;
}


INTERNETAPI
BOOL
WINAPI
InternetCombineUrlW(
    IN LPCWSTR pszBaseUrl,
    IN LPCWSTR pszRelativeUrl,
    OUT LPWSTR pszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Combines a relative URL with a base URL to form a new full URL.

Arguments:

    pszBaseUrl         - pointer to base URL

    pszRelativeUrl     - pointer to relative URL

    pszBuffer          - pointer to buffer where new URL is written

    lpdwBufferLength    - size of buffer on entry, length of new URL on exit

    dwFlags             - flags controlling operation

Return Value:

    BOOL                - TRUE if successful, FALSE if not

--*/
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCombineUrlW",
                     "%wq, %wq, %#x, %#x [%d], %#x",
                     pszBaseUrl,
                     pszRelativeUrl,
                     pszBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0,
                     dwFlags
                     ));

    HRESULT hr ;
    BOOL bRet;

    INET_ASSERT(pszBaseUrl);
    INET_ASSERT(pszRelativeUrl);
    INET_ASSERT(lpdwBufferLength);

    //
    //  the flags for the Url* APIs in shlwapi should be the same
    //  except that NO_ENCODE is on by default.  so we need to flip it
    //
    dwFlags ^= ICU_NO_ENCODE;

    // Check for invalid parameters

    if (!pszBaseUrl || !pszRelativeUrl || !lpdwBufferLength)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = UrlCombineW(pszBaseUrl, pszRelativeUrl, pszBuffer,
                    lpdwBufferLength, dwFlags | URL_WININET_COMPATIBILITY);
    }

    if(FAILED(hr))
    {
        DWORD dw = ICUHrToWin32Error(hr);

        bRet = FALSE;

        DEBUG_ERROR(INET, dw);

        SetLastError(dw);
    }
    else
        bRet = TRUE;

    DEBUG_LEAVE_API(bRet);
    return bRet;
}


INTERNETAPI
HINTERNET
WINAPI
InternetOpenW(
    IN LPCWSTR pszAgentW,
    IN DWORD dwAccessType,
    IN LPCWSTR pszProxyW OPTIONAL,
    IN LPCWSTR pszProxyBypassW OPTIONAL,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    pszAgent       -

    dwAccessType    -

    pszProxy       -

    pszProxyBypass -

    dwFlags         -

Return Value:

    HINTERNET

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetOpenW",
                     "%wq, %s (%d), %wq, %wq, %#x",
                     pszAgentW,
                     InternetMapOpenType(dwAccessType),
                     dwAccessType,
                     pszProxyW,
                     pszProxyBypassW,
                     dwFlags
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    HINTERNET hInternet = NULL;
    MEMORYPACKET mpAgentA, mpProxyA, mpProxyBypassA;

    ALLOC_MB(pszAgentW,0,mpAgentA);
    if (!mpAgentA.psStr)
    {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
    }
    UNICODE_TO_ANSI(pszAgentW,mpAgentA);
    if (pszProxyW)
    {
        ALLOC_MB(pszProxyW,0,mpProxyA);
        if (!mpProxyA.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pszProxyW,mpProxyA);
    }
    if (pszProxyBypassW)
    {
        ALLOC_MB(pszProxyBypassW,0,mpProxyBypassA);
        if (!mpProxyBypassA.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pszProxyBypassW,mpProxyBypassA);
    }

    hInternet = InternetOpenA(mpAgentA.psStr, dwAccessType, mpProxyA.psStr, 
                                        mpProxyBypassA.psStr, dwFlags);

                                        
cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }

    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}


INTERNETAPI
HINTERNET
WINAPI
InternetConnectW(
    IN HINTERNET hInternetSession,
    IN LPCWSTR pszServerNameW,
    IN INTERNET_PORT nServerPort,
    IN LPCWSTR pszUserNameW,
    IN LPCWSTR pszPasswordW,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hInternetSession    -
    pszServerName      -
    nServerPort         -
    pszUserName        -
    pszPassword        -
    dwService           -
    dwFlags             -
    dwContext           -

Return Value:

    HINTERNET

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetConnectW",
                     "%#x, %wq, %d, %wq, %wq, %s (%d), %#08x, %#x",
                     hInternetSession,
                     pszServerNameW,
                     nServerPort,
                     pszUserNameW,
                     pszPasswordW,
                     InternetMapService(dwService),
                     dwService,
                     dwFlags,
                     dwContext
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpServerNameA, mpUserNameA, mpPasswordA;
    HINTERNET hInternet = NULL;

    if (!pszServerNameW)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    ALLOC_MB(pszServerNameW, 0, mpServerNameA);
    if (!mpServerNameA.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(pszServerNameW, mpServerNameA);
    if (pszUserNameW)
    {
        ALLOC_MB(pszUserNameW, 0, mpUserNameA);
        if (!mpUserNameA.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pszUserNameW, mpUserNameA);
    }
    if (pszPasswordW)
    {
        ALLOC_MB(pszPasswordW, 0, mpPasswordA);
        if (!mpPasswordA.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pszPasswordW, mpPasswordA);
    }
    hInternet = InternetConnectA(hInternetSession, mpServerNameA.psStr, nServerPort,
                                mpUserNameA.psStr, mpPasswordA.psStr, dwService, dwFlags, dwContext);

cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }

    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}


INTERNETAPI
HINTERNET
WINAPI
InternetOpenUrlW(
    IN HINTERNET hInternetSession,
    IN LPCWSTR pszUrlW,
    IN LPCWSTR pszHeadersW,
    IN DWORD dwHeadersLengthW,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hInternetSession    -
    pszUrl             -
    pszHeaders         -
    dwHeadersLength     -
    dwFlags             -
    dwContext           -

Return Value:

    HINTERNET

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetOpenUrlW",
                     "%#x, %wq, %.80wq, %d, %#08x, %#x",
                     hInternetSession,
                     pszUrlW,
                     pszHeadersW,
                     dwHeadersLengthW,
                     dwFlags,
                     dwContext
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpHeadersA, mpUrlA;
    HINTERNET hInternet = NULL;
    
    ALLOC_MB(pszUrlW, 0, mpUrlA);
    if (!mpUrlA.psStr)
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto cleanup;
    }
    UNICODE_TO_ANSI(pszUrlW, mpUrlA);
    if (pszHeadersW)
    {
        ALLOC_MB(pszHeadersW, (dwHeadersLengthW==-1L? 0 : dwHeadersLengthW), mpHeadersA);
        if (!mpHeadersA.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pszHeadersW, mpHeadersA);
    }
    hInternet = InternetOpenUrlA(hInternetSession, mpUrlA.psStr, mpHeadersA.psStr, 
                                    mpHeadersA.dwSize, dwFlags, dwContext);

cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }

    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}


INTERNETAPI
BOOL
WINAPI
InternetReadFileExW(
    IN HINTERNET hFile,
    OUT LPINTERNET_BUFFERSW lpBuffersOut,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


INTERNETAPI
BOOL
WINAPI
InternetWriteFileExW(
    IN HINTERNET hFile,
    IN LPINTERNET_BUFFERSW lpBuffersIn,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


INTERNETAPI
BOOL
WINAPI
InternetFindNextFileW(
    IN HINTERNET hFind,
    OUT LPVOID lpvFindData
    )

/*++

Routine Description:

    This function retrieves the next block of data from the server.
    Currently it supports the following protocol data :

    FtpFindNextFile
    GopherFindNext

Arguments:

    hFind       - handle that was obtained by a FindFirst call

    lpvFindData - pointer to buffer where the next block of data is copied

Return Value:

    TRUE if the function successfully returns next block of data.
    FALSE otherwise. GetLastError() will return the error code.

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetFindNextFileW",
                     "%#x, %#x",
                     hFind,
                     lpvFindData
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    HINTERNET hFindMapped;
    HINTERNET_HANDLE_TYPE handleType;
    BOOL fResult = FALSE;
    
    dwErr = MapHandleToAddress(hFind, (LPVOID *)&hFindMapped, FALSE);
    if (dwErr!=ERROR_SUCCESS)
    {
        goto cleanup;
    }
    dwErr = RGetHandleType(hFindMapped, &handleType);
    DereferenceObject(hFindMapped);
    if (dwErr!=ERROR_SUCCESS)
    {
        goto cleanup;
    }
    if ((handleType != TypeFtpFindHandle) && (handleType != TypeGopherFindHandle))
    {
        dwErr = ERROR_INTERNET_INVALID_OPERATION;
        goto cleanup;
    }

    if (handleType == TypeFtpFindHandle)
    {
        WIN32_FIND_DATAA fdA;
        dwErr = ProbeWriteBuffer(lpvFindData, sizeof(WIN32_FIND_DATAW));
        if (dwErr!=ERROR_SUCCESS) 
        {
            goto cleanup;
        }
        if (InternetFindNextFileA(hFind,(LPVOID)&fdA))
            fResult = TransformFtpFindDataToW(&fdA,(LPWIN32_FIND_DATAW)lpvFindData);
    }
    else
    {
        GOPHER_FIND_DATAA gfdA;
        dwErr = ProbeWriteBuffer(lpvFindData, sizeof(GOPHER_FIND_DATAW));
        if (dwErr!=ERROR_SUCCESS) 
        {
            goto cleanup;
        }
        if (InternetFindNextFileA(hFind,(LPVOID)&gfdA))
            fResult = TransformGopherFindDataToW(&gfdA,(LPGOPHER_FIND_DATAW)lpvFindData);
    }

cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
InternetGetLastResponseInfoW(
    OUT LPDWORD lpdwErrorCategory,
    IN LPWSTR pszBufferW,
    IN OUT LPDWORD lpdwBufferLengthW
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    lpdwErrorCategory   -
    pszBuffer          -
    lpdwBufferLength    -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetGetLastResponseInfoW",
                     "%#x, %ws, %#x [%d]",
                     lpdwErrorCategory,
                     pszBufferW,
                     lpdwBufferLengthW,
                     lpdwBufferLengthW ? *lpdwBufferLengthW : 0
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpBufferA;

    if (pszBufferW)
    {
        ALLOC_MB(pszBufferW,*lpdwBufferLengthW,mpBufferA);
        if (!mpBufferA.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }
        
    mpBufferA.dwSize = mpBufferA.dwAlloc;
    fResult = InternetGetLastResponseInfoA(lpdwErrorCategory, mpBufferA.psStr,
                                            &mpBufferA.dwSize);
    if (fResult) {
        MAYBE_COPY_ANSI(mpBufferA, pszBufferW, *lpdwBufferLengthW);
    } else {
        *lpdwBufferLengthW = mpBufferA.dwSize*sizeof(WCHAR);
    }

cleanup:    
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }

    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
InternetCheckConnectionW(
    IN      LPCWSTR   pszUrlW,
    IN      DWORD   dwFlags,
    IN      DWORD   dwReserved
)

/*++

Routine Description:

    This routine tells the caller whether he can establish a connection to the
    network.


Arguments:

    pszUrl     this parameter is an indication to the API to attempt
                a specific host. The use of this parameter is based on the
                flags set in the dwFlags parameter

    dwFlags     a bitwise OR of the following flags

                    INTERNET_FLAG_ICC_FORCE_CONNECTION - force a connection if
                    cannot find one already established

                    INTERNET_FLAG_ICC_CONNECT_SPECIFIC_HOST - try the connection
                    to the specific host. If this flag is not set then the
                    host name is used if a quicker method is not avilable.

    dwReserved  reserved

Return Value:

    TRUE - Success
    FALSE - failure, GetLastError returns the error code

--*/
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCheckConnectionW",
                     "%ws %x",
                     pszUrlW,
                     dwFlags
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpUrlA;
    BOOL fResult = FALSE;
    
    if (pszUrlW)
    {
        ALLOC_MB(pszUrlW,0,mpUrlA);
        if (!mpUrlA.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pszUrlW,mpUrlA);
    }
    fResult = InternetCheckConnectionA(mpUrlA.psStr, dwFlags, dwReserved);

cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }

    DEBUG_LEAVE_API(fResult);
    return fResult;
}

extern INTERNETAPI
INTERNET_STATUS_CALLBACK
WINAPI
InternetSetStatusCallbackCore(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback,
    IN BOOL fType
    );

INTERNETAPI
INTERNET_STATUS_CALLBACK
WINAPI
InternetSetStatusCallbackW(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
    )

/*++

Routine Description:

    Sets the status callback function for the DLL or the handle object

Arguments:

    hInternet               - handle of the object for which we wish to set the
                              status callback

    lpfnInternetCallback    - pointer to caller-supplied status function

Return Value:

    FARPROC
        Success - previous status callback function address

        Failure - INTERNET_INVALID_STATUS_CALLBACK. Call GetLastErrorInfo() for
                  more information:

                    ERROR_INVALID_PARAMETER
                        The callback function is invalid

                    ERROR_INTERNET_INCORRECT_HANDLE_TYPE
                        Cannot set the callback on the supplied handle (probably
                        a NULL handle - per-process callbacks no longer
                        supported)

--*/

{
    DEBUG_ENTER((DBG_INET,
                 Pointer,
                 "InternetSetStatusCallbackW",
                 "%#x, %#x",
                 hInternet,
                 lpfnInternetCallback
                 ));

    INTERNET_STATUS_CALLBACK pfn = InternetSetStatusCallbackCore(hInternet,lpfnInternetCallback,TRUE);

    DEBUG_LEAVE(pfn);
    return pfn;
}

#ifdef IGCURLW
INTERNETAPI
BOOL
WINAPI
InternetGetCertByURLW(
    IN LPWSTR     lpszURL,
    IN OUT LPWSTR lpszCertText,
    OUT DWORD    dwcbCertText
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
    return InternetGetCertByURLA(lpszURL,lpszCertText,dwcbCertText);
}

#endif

INTERNETAPI
BOOL
WINAPI
InternetShowSecurityInfoByURLW(
    IN LPWSTR    pszUrlW,
    IN HWND      hwndRootWindow
    )
/*++

Routine Description:

    Does a high-level lookup against the Certificate Cache.
    Searches by URL (broken down into hostname) for the Certificate,
    and returns it in a formatted (&localized) string.

Arguments:

    lpszUrl         - pointer to URL to crack

    lpszCertText    - Output of formatted certifcate

    dwcbCertText    - Size of lpszCertText

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_INET,
                 Bool,
                 "InternetShowSecurityInfoW",
                 "%wq, %#x",
                 pszUrlW,
                 hwndRootWindow
                 ));

    BOOL fResult = FALSE;
    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpUrlA;
    
    if (pszUrlW)
    {
        ALLOC_MB(pszUrlW,0,mpUrlA);
        if (!mpUrlA.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(pszUrlW,mpUrlA);
    }
    fResult = InternetShowSecurityInfoByURLA(mpUrlA.psStr, hwndRootWindow);

cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}

INTERNETAPI
BOOL 
InternetAlgIdToStringW(
    IN ALG_ID       ai,
    IN LPWSTR       lpstr,
    IN OUT LPDWORD  lpdwstrLength,
    IN DWORD        dwReserved /* Must be 0 */
    )
/*++

Routine Description:

    Converts a algid to a user-displayable string.

Arguments:
    
    ai - Algorithm identifiers ( defined in wincrypt.h)

    lpstr - Buffer to copy string into.

    lpdwstrLength - pass in num of characters, return no of characters copied if successful,
                       else no of chars required (including null terminator)
    
    dwReserved = Must be 0

Return Value:
    DWORD
        Win32 or WININET error code.
--*/
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetAlgIdToStringW",
                     "%#x, %wq, %#x, %#x",
                     ai,
                     lpstr,
                     lpdwstrLength,
                     dwReserved
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpBuffer;
    BOOL fResult = FALSE;

    if (dwReserved!=0)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    if (lpstr)
    {
        mpBuffer.dwAlloc = mpBuffer.dwSize = *lpdwstrLength;
        mpBuffer.psStr = (LPSTR)ALLOC_BYTES(mpBuffer.dwAlloc*sizeof(CHAR));

        if (!mpBuffer.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    if (InternetAlgIdToStringA(ai, (LPSTR)mpBuffer.psStr, &mpBuffer.dwSize, dwReserved))
    {
        *lpdwstrLength = MultiByteToWideChar(CP_ACP, 0, mpBuffer.psStr, mpBuffer.dwSize + 1,
                      NULL, 0);
        if (*lpdwstrLength*sizeof(WCHAR) <= mpBuffer.dwAlloc && lpstr)
        {
            MultiByteToWideChar(CP_ACP, 0, mpBuffer.psStr, mpBuffer.dwSize + 1,
                    lpstr, *lpdwstrLength);
            (*lpdwstrLength)--;
            fResult = TRUE;
        }
        else
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
        }
    }
    else 
    {
        dwErr = GetLastError();

        if (dwErr == ERROR_INSUFFICIENT_BUFFER)
        {
            *lpdwstrLength = mpBuffer.dwSize * sizeof(WCHAR);
        }
    }

cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL 
InternetSecurityProtocolToStringW(
    IN DWORD        dwProtocol,
    IN LPWSTR       lpstr,
    IN OUT LPDWORD  lpdwstrLength,
    IN DWORD        dwReserved /* Must be 0 */
    )
/*++

Routine Description:

    Converts a security protocol to a user-displayable string.

Arguments:
    
    dwProtocol - Security protocol identifier ( defined in wincrypt.h)

    lpstr - Buffer to copy string into.

    lpdwstrLength - pass in num of characters, return no of characters copied if successful,
                       else no of chars required (including null terminator)
    
    dwReserved = Must be 0

Return Value:
    DWORD
        Win32 or WININET error code.
--*/
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetSecurityProtocolToStringW",
                     "%d, %wq, %#x, %#x",
                     dwProtocol,
                     lpstr,
                     lpdwstrLength,
                     dwReserved
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpBuffer;
    BOOL fResult = FALSE;

    if (dwReserved!=0)
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    
    if (lpstr)
    {
        mpBuffer.dwAlloc = mpBuffer.dwSize = *lpdwstrLength;
        mpBuffer.psStr = (LPSTR)ALLOC_BYTES(mpBuffer.dwAlloc*sizeof(CHAR));
        if (!mpBuffer.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }

    if (InternetSecurityProtocolToStringA(dwProtocol, (LPSTR)mpBuffer.psStr, &mpBuffer.dwSize, dwReserved))
    {
        *lpdwstrLength = MultiByteToWideChar(CP_ACP, 0, mpBuffer.psStr, mpBuffer.dwSize + 1,
                      NULL, 0);
        if (*lpdwstrLength*sizeof(WCHAR) <= mpBuffer.dwAlloc && lpstr)
        {
            MultiByteToWideChar(CP_ACP, 0, mpBuffer.psStr, mpBuffer.dwSize + 1,
                    lpstr, *lpdwstrLength);
            (*lpdwstrLength)--;
            fResult = TRUE;
        }
        else
        {
            dwErr = ERROR_INSUFFICIENT_BUFFER;
        }
    }
    else 
    {
        dwErr = GetLastError();

        if (dwErr == ERROR_INSUFFICIENT_BUFFER)
        {
            *lpdwstrLength = mpBuffer.dwSize * sizeof(WCHAR);
        }
    }

cleanup:        
    if (dwErr!=ERROR_SUCCESS) { 
        SetLastError(dwErr); 
        DEBUG_ERROR(API, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}
