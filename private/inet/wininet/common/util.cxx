 /*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    util.cxx

Abstract:

    Contains utility functions

    Contents:
        new
        delete
        NewString
        CatString
        ResizeBuffer
        _memrchr
        strnistr
        PrivateStrChr
        PlatformType
        PlatformSupport
        GetTimeoutValue
        ProbeReadBuffer
        ProbeWriteBuffer
        ProbeAndSetDword
        ProbeString
        LoadDllEntryPoints
        UnloadDllEntryPoints
        MapInternetError
        CalculateHashValue
        GetCurrentGmtTime
        GetFileExtensionFromUrl
        CheckExpired
        FTtoString
        PrintFileTimeInInternetFormat
        InternetSettingsChanged
        CertHashToStr
        ConvertSecurityInfoIntoCertInfoStruct
        FormatCertInfo
        UnicodeToUtf8
        CountUnicodeToUtf8
        ConvertUnicodeToUtf8
        StringContainsHighAnsi
        IsAddressValidIPString

Author:

    Richard L Firth (rfirth) 31-Oct-1994

Revision History:

    31-Oct-1994 rfirth
        Created

--*/

#include <wininetp.h>

#if !defined(PAGE_SIZE)
#define PAGE_SIZE   4096
#endif
#define DEFAULT_MAX_EXTENSION_LENGTH    8

//
// private prototypes
//



//
// functions
//
void * __cdecl operator new(size_t Size) {
    return (void *)ALLOCATE_FIXED_MEMORY((UINT)Size);
}

void __cdecl operator delete(void * Pointer) {
    FREE_MEMORY((HLOCAL)Pointer);
}

LPSTR
NewString(
    IN LPCSTR lpszIn,
    IN DWORD dwLen
    )

/*++

Routine Description:

    kind of version of strdup() but using LocalAlloc to allocate memory

Arguments:

    String  - pointer to string to make copy of

Return Value:

    LPSTR
        Success - pointer to duplicated string
        Failure - NULL

--*/

{
    int len = (dwLen ? dwLen : strlen(lpszIn));
    LPSTR lpszOut;

    if (lpszOut = (LPSTR)ALLOCATE_FIXED_MEMORY(len+1)) {
        memcpy(lpszOut, lpszIn, len);
        *(lpszOut + len) = '\0';
    }
    return lpszOut;
}

LPWSTR
NewStringW(
    IN LPCWSTR lpszIn,
    IN DWORD dwLen
    )

/*++

Routine Description:

    kind of version of strdup() but using LocalAlloc to allocate memory

Arguments:

    String  - pointer to string to make copy of

Return Value:

    LPSTR
        Success - pointer to duplicated string
        Failure - NULL

--*/

{
    int len = (dwLen ? dwLen : lstrlenW(lpszIn));
    LPWSTR lpszOut;

    if (lpszOut = (LPWSTR)ALLOCATE_FIXED_MEMORY((sizeof(WCHAR)*(len+1)))) {
        memcpy(lpszOut, lpszIn, len*sizeof(WCHAR));
        *(lpszOut + len) = L'\0';
    }
    return lpszOut;
}

/*++

Routine Description:

    kind of version of strcat() but using LocalAlloc to allocate memory

Arguments:

    strings to concatenate

Return Value:

    LPSTR
        Success - pointer to duplicated string
        Failure - NULL

--*/

LPSTR
CatString (
    IN LPCSTR lpszLeft,
    IN LPCSTR lpszRight
    )
{
    int cbLeft  = strlen(lpszLeft);
    int cbRight = strlen(lpszRight) + 1; // include null termination
    LPSTR lpszOut;

    if (lpszOut = (LPSTR) ALLOCATE_FIXED_MEMORY (cbLeft + cbRight)) {
        memcpy (lpszOut, lpszLeft, cbLeft);
        memcpy (lpszOut + cbLeft, lpszRight, cbRight);
    }
    return lpszOut;
}



HLOCAL
ResizeBuffer(
    IN HLOCAL BufferHandle,
    IN DWORD Size,
    IN BOOL Moveable
    )

/*++

Routine Description:

    Allocate, reallocate or free a buffer. If the buffer is moveable memory
    then it must be unlocked. If reallocating, the buffer can be grown or
    shrunk, depending on the current and required sizes

    Caveat Programmer:

    Regardless of whether a pre-existing buffer is moveable or fixed memory,
    it will be reallocated with the LMEM_MOVEABLE flag, possibly causing the
    output pointer to be different from the pre-existing pointer

Arguments:

    BufferHandle    - current handle of memory buffer. If NULL, a buffer will
                      be allocated

    Size            - size of buffer to allocate (or shrink to). If 0, the
                      buffer will be freed

    Moveable        - if TRUE and allocating memory then allocates a moveable
                      memory buffer, else fixed

Return Value:

    HLOCAL
        Success - handle of moveable memory buffer

        Failure - NULL;

--*/

{
    INET_ASSERT(!Moveable);

    if (BufferHandle == NULL) {

        //
        // don't allocate anything if no size - LocalAlloc() will return pointer
        // to memory object marked as discarded if we request a zero-length
        // moveable buffer. But I know that if Size is also 0, I don't want a
        // buffer at all, discarded or otherwise
        //

        if (Size != 0) {
            BufferHandle = ALLOCATE_MEMORY(Moveable ? LMEM_MOVEABLE : LMEM_FIXED, Size);
        }
    } else if (Size == 0) {
        BufferHandle = FREE_MEMORY(BufferHandle);

        INET_ASSERT(BufferHandle == NULL);

    } else {
        BufferHandle = REALLOCATE_MEMORY(BufferHandle, Size, LMEM_MOVEABLE);
    }
    return BufferHandle;
}


LPSTR
_memrchr(
    IN LPSTR lpString,
    IN CHAR cTarget,
    IN INT iLength
    )

/*++

Routine Description:

    Reverse find character in string

Arguments:

    lpString    - pointer to string in which to locate character

    cTarget     - target character to find

    iLength     - length of string

Return Value:

    LPSTR   - pointer to located character or NULL

--*/

{
    for (--iLength; (iLength >= 0) && (lpString[iLength] != cTarget); --iLength) {

        //
        // empty loop
        //

    }
    return (iLength < 0) ? NULL : &lpString[iLength];
}


LPSTR
strnistr(
    IN LPSTR str1,
    IN LPSTR str2,
    IN DWORD Length
    )

/*++

Routine Description:

    Case-insensitive search for substring

Arguments:

    str1    - string to search in

    str2    - substring to find

    Length  - of str1

Return Value:

    LPSTR   - pointer to located str2 in str1 or NULL

--*/

{
    if (!*str2) {
        return str1;
    }

    for (LPSTR cp = str1; *cp && Length; ++cp, --Length) {

        LPSTR s1 = cp;
        LPSTR s2 = str2;
        DWORD l2 = Length;

        while (*s1 && *s2 && l2 && (toupper(*s1) == toupper(*s2))) {
            ++s1;
            ++s2;
            --l2;
        }

        if (!*s2) {
            return cp;
        }

        if (!l2) {
            break;
        }
    }

    return NULL;
}

LPSTR
FASTCALL
PrivateStrChr(
    IN LPCSTR lpStart,
    IN WORD wMatch
    )
/*++

Routine Description:

    Find first occurrence of character in string

    Private implimentation of StrChrA, this code is based on
     a code snipet from ShlWapi, but by placing it here,
     we can remove the extra NLS support that was needed
     in SHLWAPI.   This piece of code is over twice as fast
     as the call into SHLWAPI.

Arguments:

    lpStart - points to start of null terminated string

    wMatch  - the character to match

Return Value:

    LPSTR   - ptr to the first occurrence of ch in str, NULL if not found.

--*/
{
    for ( ; *lpStart; lpStart++)
    {
        if ((BYTE)*lpStart == LOBYTE(wMatch)) {
            return((LPSTR)lpStart);
        }
    }

    return (NULL);
}



DWORD
PlatformType(
    IN OUT LPDWORD lpdwVersion5os
    )

/*++

Routine Description:

    Returns the platform type based on the operating system information. We use
    our own platform types

Arguments:

    lpdwVersion5os - optional pointer to value, set to TRUE if we on NT 5

Return Value:

    DWORD
        Failure - PLATFORM_TYPE_UNKNOWN
                    either GetVersionEx() failed, or we are running on an
                    unrecognized operating system

        Success - PLATFORM_TYPE_WIN95
                    The world's favourite desktop O/S

                  PLATFORM_TYPE_WINNT
                    The world's best O/S on top of anything

--*/

{
#ifndef UNIX
    OSVERSIONINFO versionInfo;

    *lpdwVersion5os = FALSE;

    versionInfo.dwOSVersionInfoSize = sizeof(versionInfo);
    if (GetVersionEx(&versionInfo)) {
        switch (versionInfo.dwPlatformId) {
        case VER_PLATFORM_WIN32_WINDOWS:
            return PLATFORM_TYPE_WIN95;

        case VER_PLATFORM_WIN32_NT:

            if ( lpdwVersion5os && 
                versionInfo.dwMajorVersion >= 5 ) {                
                *lpdwVersion5os = TRUE;
            }            
            return PLATFORM_TYPE_WINNT;

        }

    }
    return PLATFORM_TYPE_UNKNOWN;
#else
    return PLATFORM_TYPE_UNIX;
#endif /* UNIX */
}

//
//DWORD
//PlatformSupport(
//    VOID
//    )
//
///*++
//
//Routine Description:
//
//    Returns a bitmap of capabilities supported by this operating system
//
//Arguments:
//
//    None.
//
//Return Value:
//
//    DWORD
//
//--*/
//
//{
//    switch (PlatformType()) {
//    case PLATFORM_TYPE_WINNT:
//        return PLATFORM_SUPPORTS_UNICODE;
//    }
//    return 0;
//}


DWORD
GetTimeoutValue(
    IN DWORD TimeoutOption
    )

/*++

Routine Description:

    Gets a timeout value. The timeout is retrieved from the current handle. If
    it is not available in the current handle then the parent handle is checked
    (actually the current handle is derived from the parent, so this doesn't
    really do anything). If the value is still not available, then the global
    default is used

Arguments:

    TimeoutOption   - INTERNET_OPTION_ value used to specify the timeout value

Return Value:

    DWORD
        Requested timeout value

--*/

{
    HINTERNET hInternet;
    DWORD timeout;
    DWORD error;

    hInternet = InternetGetMappedObjectHandle();
    if (hInternet != NULL) {
        error = RGetTimeout(hInternet, TimeoutOption, &timeout);
    }
    if ((hInternet == NULL) || (error != ERROR_SUCCESS)) {
        switch (TimeoutOption) {
        case INTERNET_OPTION_SEND_TIMEOUT:
            timeout = GlobalSendTimeout;
            break;

        case INTERNET_OPTION_RECEIVE_TIMEOUT:
            timeout = GlobalReceiveTimeout;
            break;

        case INTERNET_OPTION_DATA_SEND_TIMEOUT:
            timeout = GlobalDataSendTimeout;
            break;

        case INTERNET_OPTION_DATA_RECEIVE_TIMEOUT:
            timeout = GlobalDataReceiveTimeout;
            break;

        case INTERNET_OPTION_CONNECT_TIMEOUT:
            timeout = GlobalConnectTimeout;
            break;

        case INTERNET_OPTION_CONNECT_RETRIES:
            timeout = GlobalConnectRetries;
            break;

        case INTERNET_OPTION_FROM_CACHE_TIMEOUT:
            timeout = GlobalFromCacheTimeout;
            break;
        }
    }
    return timeout;
}


DWORD
ProbeReadBuffer(
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )

/*++

Routine Description:

    Probes a buffer for readability. Used as part of API parameter validation,
    this function tests the first and last locations in a buffer. This is not
    as strict as the IsBadXPtr() Windows APIs, but it means we don't have to
    test every location in the buffer

Arguments:

    lpBuffer        - pointer to buffer to test

    dwBufferLength  - length of buffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER

--*/

{
    DWORD error;

    //
    // the buffer can be NULL if the probe length is 0. Otherwise, its an error
    //

    if (lpBuffer == NULL) {
        error = (dwBufferLength == 0) ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;
    } else if (dwBufferLength != 0) {
        __try {

            LPBYTE p;
            LPBYTE end;
            volatile BYTE b;

            p = (LPBYTE)lpBuffer;
            end = p + dwBufferLength - 1;
            b = *end;

            //
            // visit every page in the buffer - it doesn't matter that we may
            // test a character in the middle of a page
            //

            for (; p < end; p += PAGE_SIZE) {
                b = *p;
            }
            error = ERROR_SUCCESS;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            error = ERROR_INVALID_PARAMETER;
        }
        ENDEXCEPT
    } else {

        //
        // zero-length buffer
        //

        error = ERROR_INVALID_PARAMETER;
    }

    return error;
}


DWORD
ProbeWriteBuffer(
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )

/*++

Routine Description:

    Probes a buffer for writeability. Used as part of API parameter validation,
    this function tests the first and last locations in a buffer. This is not
    as strict as the IsBadXPtr() Windows APIs, but it means we don't have to
    test every location in the buffer

Arguments:

    lpBuffer        - pointer to buffer to test

    dwBufferLength  - length of buffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER

--*/

{
    DWORD error;

    //
    // the buffer can be NULL if the probe length is 0. Otherwise, its an error
    //

    if (lpBuffer == NULL) {
        error = (dwBufferLength == 0) ? ERROR_SUCCESS : ERROR_INVALID_PARAMETER;
    } else if (dwBufferLength != 0) {
        __try {

            LPBYTE p;
            LPBYTE end;
            volatile BYTE b;

            p = (LPBYTE)lpBuffer;
            end = p + dwBufferLength - 1;
            b = *end;
            *end = b;

            //
            // visit every page in the buffer - it doesn't matter that we may
            // test a character in the middle of a page
            //

            for (; p < end; p += PAGE_SIZE) {
                b = *p;
                *p = b;
            }
            error = ERROR_SUCCESS;
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            error = ERROR_INVALID_PARAMETER;
        }
        ENDEXCEPT
    } else {

        //
        // zero-length buffer
        //

        error = ERROR_SUCCESS;
    }

    return error;
}


DWORD
ProbeAndSetDword(
    IN LPDWORD lpDword,
    IN DWORD dwValue
    )

/*++

Routine Description:

    Probes a single DWORD buffer for writeability, and as a side-effect sets it
    to a default value. Used as part of API parameter validation

Arguments:

    lpDword - pointer to DWORD buffer to test

    dwValue - default value to set

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER

--*/

{
    DWORD error;

    __try {
        *lpDword = dwValue;
        error = ERROR_SUCCESS;
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        error = ERROR_INVALID_PARAMETER;
    }
    ENDEXCEPT
    return error;
}


DWORD
ProbeString(
    IN LPSTR lpString,
    OUT LPDWORD lpdwStringLength
    )

/*++

Routine Description:

    Probes a string buffer for readability, and returns the length of the string

Arguments:

    lpString            - pointer to string to check

    lpdwStringLength    - returned length of string

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER

--*/

{
    DWORD error;
    DWORD length;

    //
    // initialize string length and return code
    //

    length = 0;
    error = ERROR_SUCCESS;

    //
    // the buffer can be NULL
    //

    if (lpString != NULL) {
        __try {

            //
            // unfortunately, for a string, we have to visit every location in
            // the buffer to find the terminator
            //

            while (*lpString != '\0') {
                ++length;
                ++lpString;
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            error = ERROR_INVALID_PARAMETER;
        }
        ENDEXCEPT
    }

    *lpdwStringLength = length;

    return error;
}

DWORD
ProbeStringW(
    IN LPWSTR lpString,
    OUT LPDWORD lpdwStringLength
    )

/*++

Routine Description:

    Probes a wide string buffer for readability, and returns the length of the string

Arguments:

    lpString            - pointer to string to check

    lpdwStringLength    - returned length of string

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER

--*/

{
    DWORD error;
    DWORD length;

    //
    // initialize string length and return code
    //

    length = 0;
    error = ERROR_SUCCESS;

    //
    // the buffer can be NULL
    //

    if (lpString != NULL) {
        __try {

            //
            // unfortunately, for a string, we have to visit every location in
            // the buffer to find the terminator
            //

            while (*lpString != '\0') {
                ++length;
                ++lpString;
            }
        } __except(EXCEPTION_EXECUTE_HANDLER) {
            error = ERROR_INVALID_PARAMETER;
        }
        ENDEXCEPT
    }

    *lpdwStringLength = length;

    return error;
}


DWORD
LoadDllEntryPoints(
    IN OUT LPDLL_INFO lpDllInfo,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Dynamically loads a DLL and the entry points described in lpDllEntryPoints

    Assumes:    1. Any thread serialization taken care of by caller

                2. Module handle, entry point addresses and reference count
                   already set to 0 if this is first time the DLL_INFO is
                   being used to load the DLL

Arguments:

    lpDllInfo   - pointer to DLL_INFO structure containing all info about DLL
                  and entry points to load

    dwFlags     - flags controlling how this function operates:

                    LDEP_PARTIAL_LOAD_OK
                        - not fatal if we can't load all entry points

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - Win32 error

--*/

{
    DEBUG_ENTER((DBG_UTIL,
                 Dword,
                 "LoadDllEntryPoints",
                 "%x [%q, %d], %#x",
                 lpDllInfo,
                 lpDllInfo->lpszDllName,
                 lpDllInfo->dwNumberOfEntryPoints,
                 dwFlags
                 ));

    DWORD error = ERROR_SUCCESS;

    if (lpDllInfo->hModule == NULL) {

        DWORD dwMode = SetErrorMode(SEM_FAILCRITICALERRORS);
        HMODULE hDll = LoadLibrary(lpDllInfo->lpszDllName);

        if (hDll != NULL) {
            lpDllInfo->hModule = hDll;
            lpDllInfo->LoadCount = 1;

            for (DWORD i = 0; i < lpDllInfo->dwNumberOfEntryPoints; ++i) {

                FARPROC proc = GetProcAddress(
                                    hDll,
                                    lpDllInfo->lpEntryPoints[i].lpszProcedureName
                                    );

                *lpDllInfo->lpEntryPoints[i].lplpfnProcedure = proc;
                if ((proc == NULL) && !(dwFlags & LDEP_PARTIAL_LOAD_OK)) {
                    error = GetLastError();
                    UnloadDllEntryPoints(lpDllInfo, TRUE);
                    break;
                }
            }
        } else {
            error = GetLastError();
        }
        SetErrorMode(dwMode);
    } else {

        DEBUG_PRINT(UTIL,
                    INFO,
                    ("info for %q already loaded\n",
                    lpDllInfo->lpszDllName
                    ));

        InterlockedIncrement(&lpDllInfo->LoadCount);
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
UnloadDllEntryPoints(
    IN OUT LPDLL_INFO lpDllInfo,
    IN BOOL bForce
    )

/*++

Routine Description:

    Undoes the work of LoadDllEntryPoints()

    Assumes:    1. Any thread serialization taken care of by caller

Arguments:

    lpDllInfo   - pointer to DLL_INFO structure containing all info about DLL
                  and (loaded) entry points

    bForce      - TRUE if the DLL will be unloaded irrespective of the usage
                  count

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - Win32 error

--*/

{
    DEBUG_ENTER((DBG_UTIL,
                Dword,
                "UnloadDllEntryPoints",
                "%x [%q, %d], %B",
                lpDllInfo,
                lpDllInfo->lpszDllName,
                lpDllInfo->dwNumberOfEntryPoints,
                bForce
                ));

    DWORD error = ERROR_SUCCESS;

    if (bForce) {
        lpDllInfo->LoadCount = 0;
    } else if (InterlockedDecrement(&lpDllInfo->LoadCount) == 0) {
        bForce = TRUE;
    }
    if (bForce && (lpDllInfo->hModule != NULL)) {
        if (!FreeLibrary(lpDllInfo->hModule)) {
            error = GetLastError();
        }

        //
        // even if FreeLibrary() failed we clear out the load info
        //

        lpDllInfo->hModule = NULL;
        for (DWORD i = 0; i < lpDllInfo->dwNumberOfEntryPoints; ++i) {
            *lpDllInfo->lpEntryPoints[i].lplpfnProcedure = NULL;
        }
    }

    DEBUG_LEAVE(error);

    return error;
}

#ifndef CERT_E_WRONG_USAGE
#   define CERT_E_WRONG_USAGE              _HRESULT_TYPEDEF_(0x800B0110)
#endif


DWORD
MapInternetError(
    IN DWORD dwErrorCode
    )

/*++

Routine Description:

    Maps a winsock/RPC/transport error into a more user-friendly WinInet error,
    and stores the original error in the per-thread context so that the app can
    retrieve it if it really cares

    N.B. We should no longer be receiving winsock errors directly at the WinInet
    interface. They are available via InternetGetLastResponseInfo()

Arguments:

    dwErrorCode - original (winsock) error code to map

Return Value:

    DWORD
        Mapped error code, or the orignal error if its not one that we handle

--*/

{
    LPINTERNET_THREAD_INFO lpThreadInfo;

    DEBUG_ENTER((DBG_UTIL,
                Dword,
                "MapInternetError",
                "%#x [%s]",
                dwErrorCode,
                InternetMapError(dwErrorCode)
                ));

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo) {
        lpThreadInfo->dwMappedErrorCode = dwErrorCode;
    }

    switch (dwErrorCode) {

    case SEC_E_INSUFFICIENT_MEMORY        :
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        break;

    case SEC_E_INVALID_HANDLE             :
    case SEC_E_UNSUPPORTED_FUNCTION       :
    case SEC_E_TARGET_UNKNOWN             :
    case SEC_E_INTERNAL_ERROR             :
    case SEC_E_SECPKG_NOT_FOUND           :
    case SEC_E_NOT_OWNER                  :
    case SEC_E_CANNOT_INSTALL             :
    case SEC_E_INVALID_TOKEN              :
    case SEC_E_CANNOT_PACK                :
    case SEC_E_QOP_NOT_SUPPORTED          :
    case SEC_E_NO_IMPERSONATION           :
    case SEC_E_LOGON_DENIED               :
    case SEC_E_UNKNOWN_CREDENTIALS        :
    case SEC_E_NO_CREDENTIALS             :
    case SEC_E_MESSAGE_ALTERED            :
    case SEC_E_OUT_OF_SEQUENCE            :
    case SEC_E_NO_AUTHENTICATING_AUTHORITY:
    case SEC_I_CONTINUE_NEEDED            :
    case SEC_I_COMPLETE_NEEDED            :
    case SEC_I_COMPLETE_AND_CONTINUE      :
    case SEC_I_LOCAL_LOGON                :
    case SEC_E_BAD_PKGID                  :
    case SEC_E_CONTEXT_EXPIRED            :
    case SEC_E_INCOMPLETE_MESSAGE         :
        dwErrorCode = ERROR_INTERNET_SECURITY_CHANNEL_ERROR;
        break;

    // Cert and Encryption errors

    case CERT_E_EXPIRED:
    case CERT_E_VALIDITYPERIODNESTING:
        dwErrorCode = ERROR_INTERNET_SEC_CERT_DATE_INVALID;
        break;

    case CERT_E_UNTRUSTEDROOT:
        dwErrorCode = ERROR_INTERNET_INVALID_CA;
        break;

    case CERT_E_CN_NO_MATCH:
        dwErrorCode = ERROR_INTERNET_SEC_CERT_CN_INVALID;
        break;

    case CRYPT_E_REVOKED:
        dwErrorCode = ERROR_INTERNET_SEC_CERT_REVOKED;
        break;

    // ignore revocation if the certificate does not have a CDP
    case CRYPT_E_NO_REVOCATION_CHECK:
        dwErrorCode = ERROR_SUCCESS;
        break;

    case CRYPT_E_REVOCATION_OFFLINE:
        dwErrorCode = ERROR_INTERNET_SEC_CERT_REV_FAILED;
        break;

    case CERT_E_ROLE:
    case CERT_E_PATHLENCONST:
    case CERT_E_CRITICAL:
    case CERT_E_PURPOSE:
    case CERT_E_ISSUERCHAINING:
    case CERT_E_MALFORMED:
    case CERT_E_CHAINING:
    // We can't allow connection if server doesn't have a server auth certificate.
    // To force CERT_E_WRONG_USAGE to error out we map it to the error below.
    // In the future we need to map it to it's own non-recoverable error, so we can 
    // give the user a specific error message.
    case CERT_E_WRONG_USAGE:
        dwErrorCode = ERROR_INTERNET_SEC_INVALID_CERT;
        break;

    case WSAEINTR:
    case WSAEBADF:
    case WSAEACCES:
    case WSAEFAULT:
    case WSAEINVAL:
    case WSAEMFILE:
    case WSAEADDRINUSE:
    case WSAEADDRNOTAVAIL:
        dwErrorCode = ERROR_INTERNET_INTERNAL_ERROR;
        break;

    case WSAENOTSOCK:

        //
        // typically, if we see this error its because we tried to use a closed
        // socket handle
        //
        dwErrorCode = ERROR_INTERNET_OPERATION_CANCELLED;
        break;

    case WSAEWOULDBLOCK:
    case WSAEINPROGRESS:
    case WSAEALREADY:
    case WSAEDESTADDRREQ:
    case WSAEPROTOTYPE:
    case WSAENOPROTOOPT:
    case WSAEPROTONOSUPPORT:
    case WSAESOCKTNOSUPPORT:
    case WSAEOPNOTSUPP:
    case WSAEISCONN:
    case WSAETOOMANYREFS:
    case WSAELOOP:
    case WSAENAMETOOLONG:
    case WSAENOTEMPTY:
    case WSAEPROCLIM:
    case WSAEUSERS:
    case WSAEDQUOT:
    case WSAESTALE:
    case WSAEREMOTE:
    case WSAEDISCON:
    case WSASYSNOTREADY:
    case WSAVERNOTSUPPORTED:
    case WSANOTINITIALISED:

        //
        // currently unmapped errors
        //

        break;

    case WSAEMSGSIZE:
        dwErrorCode = ERROR_INSUFFICIENT_BUFFER;
        break;

    case WSAEPFNOSUPPORT:
    case WSAEAFNOSUPPORT:
        dwErrorCode = ERROR_INTERNET_TCPIP_NOT_INSTALLED;
        break;

    case WSAECONNABORTED:
    case WSAESHUTDOWN:
        dwErrorCode = ERROR_INTERNET_CONNECTION_ABORTED;
        break;

    case WSAECONNRESET:
    case WSAENETRESET:
        dwErrorCode = ERROR_INTERNET_CONNECTION_RESET;
        break;

    case WSAENOBUFS:
        dwErrorCode = ERROR_NOT_ENOUGH_MEMORY;
        break;

    case WSAETIMEDOUT:
        dwErrorCode = ERROR_INTERNET_TIMEOUT;
        break;

    case WSAENETDOWN:
    case WSAECONNREFUSED:
    case WSAENETUNREACH:
    case WSAENOTCONN:
        dwErrorCode = ERROR_INTERNET_CANNOT_CONNECT;
        break;

    case WSAEHOSTDOWN:
    case WSAEHOSTUNREACH:
    case WSAHOST_NOT_FOUND:
    case WSATRY_AGAIN:
    case WSANO_RECOVERY:
    case WSANO_DATA:
        dwErrorCode = ERROR_INTERNET_NAME_NOT_RESOLVED;
        break;

    default:

        DEBUG_PRINT(UTIL,
                    WARNING,
                    ("MapInternetError(): unmapped error code %d [%#x]\n",
                    dwErrorCode,
                    dwErrorCode
                    ));

        break;
    }

    DEBUG_LEAVE(dwErrorCode);

    return dwErrorCode;
}


DWORD
CalculateHashValue(
    IN LPSTR lpszString
    )

/*++

Routine Description:

    Calculate a hash number given a string

Arguments:

    lpszString  - string to hash

Return Value:

    DWORD

--*/

{
    DWORD hashValue = 0;
    DWORD position = 1;

    while (*lpszString) {
        hashValue += *lpszString * position;
        ++lpszString;
        ++position;
    }
    return hashValue;
}



VOID GetCurrentGmtTime(
    LPFILETIME  lpFt
    )
/*++

Routine Description:
    This routine returns the current GMT time

Arguments:

    lpFt    FILETIME strucutre in which this is returned

Returns:

Comments:

--*/
{
    SYSTEMTIME sSysT;

    GetSystemTime(&sSysT);
    SystemTimeToFileTime(&sSysT, lpFt);
}

///***    DwRemoveDots    - Remove any dots from a path name
//**
//**  Synopsis
//**      DWORD DwRemoveDots (pchPath)
//**      Lifted from win95 kernel.
//**
//**  Input:
//**      pchPath         - A path string
//**
//**
//**  Output:
//**      returns the number of double dot levels removed from front
//**
//**  Errors:
//**      returns dwInvalid if invalid path
//**
//**  Description:
//**      Removes ..\ and .\ sequences from a path string. The path
//**      string should not include the root drive or net name portion.
//**      The return value of is the number of levels removed from the
//**      start of the string. Levels removed from inside the string
//**      will not be returned. For example:
//**
//**          String          Result              Return
//**
//**          ..\..\dir1      dir1                2
//**          dir1\..\dir2    dir2                0
//**          dir1\..\..\dir2 dir2                1
//**          .\dir1          dir1                0
//**          dir1\.\dir2     dir1\dir2           0
//**
//**      A backslash at the start of the string will be ignored.
//*/
//
//// File and path definitions
//
//#define     chExtSep    '.'
//#define     szExtSep    "."
//#define     chNetIni    '\\'
//#define     chDirSep    '\\'
//#define     szDirSep    "\\"
//#define     chDirSep2   '/'
//#define     chDrvSep    ':'
//#define     chRelDir    '.'
//#define     chEnvSep    ';'
//#define     chWldChr    '?'
//#define     chWldSeq    '*'
//#define     chMinDrv    'A'
//#define     chMaxDrv    'Z'
//#define     chMinDrvLow 'a'
//#define     chMaxDrvLow 'z'
//
//DWORD
//DwRemoveDots (
//    char * pchPath
///*++
//
//Routine Description:
//    Removes ./ ../ etc from a path to normalize it
//
//Arguments:
//
//    pchPath path string
//
//Returns:
//
//    Count of levels dealt with
//
//Comments:
//
//    Lifted from win95 kernel
//
//--*/
//)
//    {
//    BOOL            fInside = FALSE;
//    DWORD           cLevel = 0;
//    DWORD           cBackup;
//    register char * pchR;
//    register char * pchL;
//
//#ifdef MAYBE
//    // Check for invalid characters
//    if (!FFixPathChars(pchPath)) {
//        // No code required.
//        return dwInvalid;
//    }
//
//#endif //MAYBE
//    // Skip slashes
//    for (; *pchPath == chDirSep2; pchPath++)
//        ;
//    pchL = pchR = pchPath;
//
//    // Loop through handling each directory part
//    while (*pchR) {
//        // This part starts with dot. Is it one or more?
//        if (*pchR++ == chRelDir) {
//            for (cBackup = 0; *pchR == chRelDir; cBackup++, pchR++)
//                ;
//            if (cBackup) {
//                // More than one dot. Back up the left pointer.
//                if ((*pchR != chDirSep2) && (*pchR != '\0')) {
//                    // we got a [.]+X (X != '\') might be an LFN
//                    // process this as a name
//                    goto name_processing;
//                }
//                // Doesn't advance for ending ..
//                for (; *pchR == chDirSep2; pchR++)
//                    ;
//                if (fInside) {
//                    for (; cBackup; cBackup--) {
//                        if (pchL <= pchPath) {
//                            cLevel += cBackup;
//                            fInside = FALSE;
//                            break;
//                        }
//                        // Remove the previous part
//                        for (pchL -= 2; *pchL != chDirSep2; pchL--) {
//                            if (pchL <= pchPath) {
//                                fInside = FALSE;
//                                pchL--;
//                                break;
//                            }
//                        }
//                        pchL++;
//                    }
//                } else {
//                    cLevel += cBackup;
//                }
//                // Subtract ending backslash if not root
//                if ((*pchR == '\0') && (pchL != pchPath))
//                    pchL--;
//                strcpy(pchL, pchR);
//                pchR = pchL;
//            } else {
//                // This part starts with one dot. Throw it away.
//                if (*pchR != chDirSep2) {
//                    // Special case "\." by converting it to ""
//                    // unless it is a root, when it becomes "\".
//                    if (*pchR == '\0') {
//                        if (pchL == pchPath)
//                            *(pchR-1) = '\0';   // root
//                        else
//                            *(pchR-2) = '\0';   // not root
//                        return cLevel;
//                    }
//                    // we started with a '.' and then there was no '\'
//                    // might be an LFN name
//                    goto name_processing;
//                }
//                pchR++;
//                strcpy(pchL, pchR);
//                pchR = pchL;
//            }
//        } else {
//name_processing:
//            // This part is a name. Skip it.
//            fInside = TRUE;
//            for (; TRUE; pchR++) {
//                if (*pchR == chDirSep2) {
//                    if (*(pchR-1) == chRelDir) {
//                        // This name has one or more dots at the end.
//                        // Remove the last dot (NT3.5 does this).
//                        pchL = pchR-1;
//                        strcpy(pchL, pchR);
//                        pchR = pchL;    // point to chDirSep2 again
//                    }
//                    for (; *pchR == chDirSep2; pchR++)
//                        ;
//                    break;
//                } else if (*pchR == '\0') {
//                    // Remove trailing dots.
//                    // NB Can't fall off the beginning since the first char
//                    // of the current path element was not chRelDir.
//                    for (; *(pchR-1) == chRelDir; pchR--)
//                        ;
//                    // Overstore the first trailing dot, if there is one.
//                    *pchR = '\0';
//                    break;
//                }
//            }
//            pchL = pchR;
//        }
//    }
//    return cLevel;
//}


//#define OLD

#define EXE_EXTENSION   TEXT(".exe")
#define DLL_EXTENSION   TEXT(".dll")

LPSTR GetFileExtensionFromUrl(
    IN LPSTR lpszUrl,
    IN OUT LPDWORD lpdwLength)
/*++

Routine Description:
    This routine returns a possible file extension from a URL
    It does this by walking back from the end till the first  dot.

Arguments:

    lpszUrl         Url to derive the extension from

    lpdwLength      max length of the extension expected

Returns:

    NULL if no dot within the passed in length or a forward slash or a
    backward slash encountered before the dot. Otherwise returns a pointer
    pointing past the dot in the url string

Comments:



--*/
{
    INET_ASSERT(lpszUrl && lpdwLength);

#ifdef OLD
    if (lpszUrl != NULL) {

        LPSTR p;
        DWORD len=0 , lenLimit= *lpdwLength;

        p = lpszUrl + (strlen(lpszUrl) - 1);

        while ((*p != '.') && (p != lpszUrl)) {

            // if this contains a character that the filesystems
            // don't like, then return NULL
            if (strchr(vszInvalidFilenameChars, *p)) {
                return(NULL);
            }

            if ((*p == '/') || (*p == '\\')) {
                break;
            }
            --p;
            ++len;
        }
        if ((*p == '.')
        && (len != 0)
        && (len < lenLimit)) {
            *lpdwLength = len;
            return p + 1;
        }
    }
    return NULL;
#else
    if (!lpszUrl)
    {
        *lpdwLength = 0;
        return NULL;
    }

    LPSTR pszPeriod = NULL;
    BOOL fContinue = TRUE;

    // Scanning from left to right, note where we last saw a period.
    // If we see a character that cannot be in an extension, and we've seen a period, forget
    // about the period.
    // Repeat this until we've reached the end of the url, a question mark (query) or hash (fragment)

    // 1.6.98: _However_, if the file extension we've discovered is either .dll or .exe, 
    //         we'll continue to scan beyond the query mark for a file extension.

    // 1.20.98: And if we find no extension before the question mark, we'll look after it, then.
    
    while (fContinue)
    {
        switch (*lpszUrl)
        {
        case TEXT('.'):
            pszPeriod = lpszUrl;
            break;

        case TEXT('?'):
            if (pszPeriod)
            {
                if ((!StrCmpNI(pszPeriod, EXE_EXTENSION, ARRAY_ELEMENTS(EXE_EXTENSION)-1))
                    || (!StrCmpNI(pszPeriod, DLL_EXTENSION, ARRAY_ELEMENTS(DLL_EXTENSION)-1)))
                {
                    pszPeriod = NULL;
                    break;
                }
            }
            else
            {
                break;
            }
            
        case TEXT('#'):
        case TEXT('\0'):
            fContinue = FALSE;
            break;

        default:
            if (pszPeriod && strchr(vszInvalidFilenameChars, *lpszUrl))
            {
                pszPeriod = NULL;
            }        
        }
        lpszUrl++;
    }
    // This will be off by one
    lpszUrl--;
    if (pszPeriod)
    {
        if (*lpdwLength < (DWORD)(lpszUrl-pszPeriod))
        {
            pszPeriod = NULL;
        }
        else
        {
            pszPeriod++;
            *lpdwLength = (DWORD)(lpszUrl-pszPeriod);
        }
    }
    return pszPeriod;
#endif
}


DWORD
CheckExpired(
    HINTERNET           hRequestMapped,
    BOOL*               lpfIsExpired,
    CACHE_ENTRY_INFO*   pInfo,
    LONGLONG            DefaultExpiryDelta
    )
/*++

Routine Description:
    This routine checks whether a cache entry has expired for ftp/gopher.
    It uses the globally set synchronization modes to make that decision

Arguments:

    hRequestMapped      a mapped request handle

    lpfIsExpired        returns TRUE if expired, FALSE otherwise

    lpCacheEntryInfo    cache entry info containing all the timestamps

    DefaultExpiryDelta  time delta to use for default expiry calculation

Returns:

    Windows error code

Comments:

--*/
{
    switch (GlobalUrlCacheSyncMode)
    {
        case WININET_SYNC_MODE_NEVER:
            // Never check, unless the page has expired
            *lpfIsExpired = FALSE;
            break;

        case WININET_SYNC_MODE_ALWAYS:
            *lpfIsExpired = TRUE;
            break;

        default:
            if (FT2LL (pInfo->LastSyncTime) < dwdwSessionStartTime)
                *lpfIsExpired = TRUE;
            else
            {
                FILETIME ftCurrent;
                GetCurrentGmtTime (&ftCurrent);
                *lpfIsExpired = (FT2LL(ftCurrent) - FT2LL (pInfo->LastSyncTime)
                    > DefaultExpiryDelta);
            }
            break;
    }

    return ERROR_SUCCESS;
}



LPTSTR
FTtoString(
    IN FILETIME *pftTime)

/*++

FTtoString:

    This routine converts a given FILETIME structure to a string representing
    the given date and time in the local format.

Arguments:

    pftTime supplies the FILETIME structure to convert.

Return Value:

    NULL - Memory allocation failure.
    Otherwise, the address of the string, allocated via LocalAlloc.

Author:

    Doug Barlow (dbarlow) 4/12/1996

--*/

{
    LONG cchTotal, cchNeeded;
    SYSTEMTIME stTime, stLocal;
    LPTSTR szDateTime = NULL;


    //
    // Convert the FILETIME to a SYSTEMTIME.
    //

    if (!FileTimeToSystemTime(pftTime, &stTime))
        goto ErrorExit;

    //
    // For now, leave it in GMT time, function not implimented in Win'95.
    //

    //if ( IsPlatformWinNT() )
    //{
    //    if (!SystemTimeToTzSpecificLocalTime(NULL, &stTime, &stLocal))
    //        goto ErrorExit;
    //}
    //else
    {
        stLocal = stTime;
    }


    //
    // Calculate how long the date string will be.
    //

    cchTotal =
        GetDateFormat(
            LOCALE_SYSTEM_DEFAULT,
            DATE_SHORTDATE,
            &stLocal,
            NULL,
            NULL,
            0);
    if (0 >= cchTotal)
        goto ErrorExit;
    cchNeeded =
        GetTimeFormat(
            LOCALE_SYSTEM_DEFAULT,
            0,
            &stLocal,
            NULL,
            NULL,
            0);
    if (0 >= cchNeeded)
        goto ErrorExit;
    cchTotal += cchNeeded;
    cchTotal += 4 * sizeof(TCHAR);  // space, trailing NULL, and two extra.
    szDateTime = (LPTSTR)ALLOCATE_MEMORY(LMEM_FIXED, cchTotal);
    if (NULL == szDateTime)
        goto ErrorExit;


    //
    // Fill in the time string.
    //

    cchNeeded =
        GetDateFormat(
            LOCALE_SYSTEM_DEFAULT,
            DATE_SHORTDATE,
            &stLocal,
            NULL,
            szDateTime,
            cchTotal);
    if (0 >= cchNeeded)
        goto ErrorExit;
    lstrcat(szDateTime, TEXT(" "));
    cchNeeded = lstrlen(szDateTime);
    cchNeeded =
        GetTimeFormat(
            LOCALE_SYSTEM_DEFAULT,
            0,
            &stLocal,
            NULL,
            &szDateTime[cchNeeded],
            cchTotal - cchNeeded);
    if (0 >= cchNeeded)
        goto ErrorExit;
    return szDateTime;


ErrorExit:
    if (NULL != szDateTime)
        FREE_MEMORY(szDateTime);
    return NULL;
}


BOOL
PrintFileTimeInInternetFormat(
    FILETIME *lpft,
    LPSTR lpszBuff,
    DWORD   dwSize
)
{
    SYSTEMTIME sSysTime;

    if (dwSize < INTERNET_RFC1123_BUFSIZE) {
        SetLastError(ERROR_INSUFFICIENT_BUFFER);
        return (FALSE);
    }
    if (!FileTimeToSystemTime(((CONST FILETIME *)lpft), &sSysTime)) {
        return (FALSE);
    }
    return (InternetTimeFromSystemTime( &sSysTime,
                                        INTERNET_RFC1123_FORMAT,
                                        lpszBuff,
                                        dwSize));

}


BOOL
InternetSettingsChanged(
    VOID
    )

/*++

Routine Description:

    Determines if the global settings have been changed (inter-process)

Arguments:

    None.

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER((DBG_UTIL,
                 Bool,
                 "InternetSettingsChanged",
                 NULL
                 ));

    DWORD dwVer;
    BOOL bChanged = FALSE;

    if (GetCurrentSettingsVersion(&dwVer)) {

        DEBUG_PRINT(UTIL,
                    INFO,
                    ("current settings version = %d\n",
                    dwVer
                    ));

        if (!GlobalSettingsLoaded || (dwVer != GlobalSettingsVersion)) {
            GlobalSettingsLoaded = TRUE;
            GlobalSettingsVersion = dwVer;
            bChanged = TRUE;
        }
    }

    DEBUG_LEAVE(bChanged);

    return bChanged;
}


BOOL
CertHashToStr(
    IN LPSTR lpMD5Hash,
    IN DWORD dwMD5HashSize,
    IN OUT LPSTR *lplpszHashStr
    )
/*++

Routine Description:

    Converts a set of bytes into a neatly formated string of ':' (colon) seperated
      hex digits that can be shown to the user.

Arguments:

    lpMD5Hash - ptr to set of hash bytes

    dwMD5HashSize - size of lpMD5Hash

    lplpszHashStr - ptr to ptr where newly allocated return string will be stored.

Return Value:

    BOOL

--*/

{

    DWORD dwStrSize = (2*dwMD5HashSize) + dwMD5HashSize;
    LPSTR lpszHashStr;

    *lplpszHashStr = new CHAR[dwStrSize];

    if ( *lplpszHashStr == NULL )
    {
        return FALSE;
    }

    lpszHashStr = *lplpszHashStr;

    for ( DWORD i = 0 ; i < dwMD5HashSize; i++ )
    {
        unsigned char uHashByte;

        if ( i != 0 )
        {
            *lpszHashStr = ':';
            lpszHashStr++;
        }

        uHashByte = (unsigned char) * ( ((unsigned char * ) lpMD5Hash) + i);

        wsprintf( lpszHashStr, "%02X", uHashByte);

        lpszHashStr += 2;

    }

    INET_ASSERT( *lpszHashStr == '\0' );

    return TRUE;
}


//
// private functions
//

DWORD
ConvertSecurityInfoIntoCertInfoStruct(
    IN  LPINTERNET_SECURITY_INFO   pSecInfo,
    OUT INTERNET_CERTIFICATE_INFO *pCertificate,
    IN OUT DWORD *pcbCertificate
    )
/*++

Routine Description:

    Converts an X509 Certificate Structure into a WININET struct
    used for storing the same info.

Arguments:

    hContext        - Context handle of the active SSPI session.

    pCertInfo       - Pointer to Structure where info is returned in.

Return Value:

    DWORD
    ERROR_SUCCESS   - if cert cannot be converted

    ERROR_NOT_ENOUGH_MEMORY

--*/

{


    DWORD   error = ERROR_SUCCESS;
    PCERT_INFO pCertInfo = NULL;
    DWORD cbCert = sizeof(INTERNET_CERTIFICATE_INFO),
          cbSubject = 0,
          cbIssuer = 0;

    BOOL fCanAlloc = FALSE;

    if(pSecInfo == NULL)
    {
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    if(pCertificate == NULL || *pcbCertificate == 0)
    {
        *pcbCertificate = sizeof(INTERNET_CERTIFICATE_INFO);
        goto quit;
    }

    if(*pcbCertificate < sizeof(INTERNET_CERTIFICATE_INFO) )
    {
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    ZeroMemory(pCertificate, sizeof(INTERNET_CERTIFICATE_INFO));
    fCanAlloc = TRUE;

    if(pSecInfo->pCertificate &&
       pSecInfo->pCertificate->pCertInfo )
    {
        pCertInfo = pSecInfo->pCertificate->pCertInfo;

        //
        // Now Convert Structures from SSPI format to WININET style.
        //  While in the process, we'll role them all into one
        //  big structure that we'll return to the user.
        //

        cbSubject = CertNameToStr(pSecInfo->pCertificate->dwCertEncodingType,
                                     &pCertInfo->Subject,
                                      CERT_SIMPLE_NAME_STR |
                                      CERT_NAME_STR_CRLF_FLAG |
                                      CERT_NAME_STR_NO_PLUS_FLAG,
                                     NULL,
                                     0);


        if ( cbSubject > 0 )
        {
            // freed by caller outside of wininet
            pCertificate->lpszSubjectInfo = (LPSTR) LocalAlloc(LPTR, cbSubject);

            if ( pCertificate->lpszSubjectInfo == NULL )
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }

            CertNameToStr(pSecInfo->pCertificate->dwCertEncodingType,
                                     &pCertInfo->Subject,
                                      CERT_SIMPLE_NAME_STR |
                                      CERT_NAME_STR_CRLF_FLAG |
                                      CERT_NAME_STR_NO_PLUS_FLAG ,
                                     pCertificate->lpszSubjectInfo,
                                     cbSubject);

        }

        cbIssuer = CertNameToStr(pSecInfo->pCertificate->dwCertEncodingType,
                                     &pCertInfo->Issuer,
                                      CERT_SIMPLE_NAME_STR |
                                      CERT_NAME_STR_CRLF_FLAG |
                                      CERT_NAME_STR_NO_PLUS_FLAG,
                                     NULL,
                                     0);

        if ( cbIssuer > 0 )
        {
            // freed by caller outside of wininet
            pCertificate->lpszIssuerInfo = (LPSTR) LocalAlloc(LPTR, cbIssuer);

            if ( pCertificate->lpszIssuerInfo == NULL )
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }

            CertNameToStr(pSecInfo->pCertificate->dwCertEncodingType,
                                     &pCertInfo->Issuer,
                                      CERT_SIMPLE_NAME_STR |
                                      CERT_NAME_STR_CRLF_FLAG |
                                      CERT_NAME_STR_NO_PLUS_FLAG ,
                                     pCertificate->lpszIssuerInfo,
                                     cbIssuer);

        }

        CopyMemory(
                (PVOID) &pCertificate->ftStart,
                (PVOID) &pCertInfo->NotBefore,
                sizeof(FILETIME)
                );

        CopyMemory(
                (PVOID) &pCertificate->ftExpiry,
                (PVOID) &pCertInfo->NotAfter,
                sizeof(FILETIME)
                );

    }

    /*if(pSecInfo->dwProtocol)
    {
        DWORD dwProtocolID;
        TCHAR lpszProtocol[100];

        ATTR_MAP ProtocolAttrMap[] =
        {
            {SP_PROT_SSL2_CLIENT, IDS_PROTOCOL_SSL2},
            {SP_PROT_SSL3_CLIENT, IDS_PROTOCOL_SSL3},
            {SP_PROT_PCT1_CLIENT, IDS_PROTOCOL_PCT1},
            {SP_PROT_TLS1_CLIENT, IDS_PROTOCOL_TLS1}
        };


        for(j=0; j < sizeof(ProtocolAttrMap)/sizeof(ProtocolAttrMap[0]); j++)
        {
            if(ProtocolAttrMap[j].dwAttr == pSecInfo->dwProtocol)
            {
                dwProtocolID = ProtocolAttrMap[j].dwStringID;
                break;
            }
        }
        if(LoadString(GlobalDllHandle,
                   dwProtocolID,
                   lpszProtocol,
                   sizeof(lpszProtocol)/sizeof(lpszProtocol[0])))
        {
            pCertificate->lpszProtocolName  = NewString(lpszProtocol);
        }
    } */

    pCertificate->dwKeySize = pSecInfo->dwCipherStrength;

quit:

    if ( error != ERROR_SUCCESS &&
         fCanAlloc
        )
    {

        if ( pCertificate->lpszSubjectInfo )
        {
            LocalFree(pCertificate->lpszSubjectInfo);
            pCertificate->lpszSubjectInfo = NULL;
        }

        if ( pCertificate->lpszIssuerInfo )
        {
            LocalFree(pCertificate->lpszIssuerInfo);
            pCertificate->lpszIssuerInfo = NULL;
        }
    }

    return error;
}

/*++

FormatCertInfo:

    This routine formats the information within a INTERNET_CERTIFICATE_INFO
    structure suitable for display with localization.

Arguments:

    pCertInfo supplies a pointer to the INTERNET_CERTIFICATE_INFO structure to
    be formatted.

Return Value:

    NULL - An error occurred.  Otherwise, a pointer to the formatted string.
        This string must be freed by the caller via LocalFree.

Author:

    Doug Barlow (dbarlow) 4/30/1996

--*/

LPTSTR
FormatCertInfo(
    IN INTERNET_CERTIFICATE_INFO *pCertInfo
    )
{
    LPVOID rgpvParams[9];   // Number of insertable elements in the
                            // plszStrings->szCertInfo resource
                            // string.
    LPTSTR szResult = NULL;
    int i = 0;
    PLOCAL_STRINGS plszStrings;
    LPTSTR szFrom = NULL;
    LPTSTR szUntil = NULL;


    //
    // Get the Certificate Information.
    //

    plszStrings = FetchLocalStrings();
    szFrom = FTtoString(&pCertInfo->ftStart);
    szUntil = FTtoString(&pCertInfo->ftExpiry);
    if ((NULL == szUntil) || (NULL == szFrom))
        goto ErrorExit;

    //ChangeCommaSpaceToCRLF(pCertInfo->lpszSubjectInfo);
    //ChangeCommaSpaceToCRLF(pCertInfo->lpszIssuerInfo);

    rgpvParams[i++] = (LPVOID)pCertInfo->lpszSubjectInfo;
    rgpvParams[i++] = (LPVOID)pCertInfo->lpszIssuerInfo;
    rgpvParams[i++] = (LPVOID)szFrom;
    rgpvParams[i++] = (LPVOID)szUntil;
    rgpvParams[i++] = (LPVOID)pCertInfo->lpszProtocolName;
    rgpvParams[i++] = (LPVOID)pCertInfo->lpszSignatureAlgName;
    rgpvParams[i++] = (LPVOID)pCertInfo->lpszEncryptionAlgName;
    rgpvParams[i++] = (LPVOID)pCertInfo->dwKeySize;
    if (96 <= pCertInfo->dwKeySize)  // Recommended Key strength
        rgpvParams[i++] = (LPVOID)plszStrings->szStrengthHigh;
    else if (64 <= pCertInfo->dwKeySize) // Passable key strength
        rgpvParams[i++] = (LPVOID)plszStrings->szStrengthMedium;
    else    // Ick!  Low key strength.
        rgpvParams[i++] = (LPVOID)plszStrings->szStrengthLow;
    INET_ASSERT(i == sizeof(rgpvParams) / sizeof(LPVOID));
    i = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER
            | FORMAT_MESSAGE_FROM_STRING
            | FORMAT_MESSAGE_ARGUMENT_ARRAY,
            plszStrings->szCertInfo,
            0, 0,
            (LPTSTR)&szResult,
            0,
            (va_list *)rgpvParams);

ErrorExit:
    if (NULL != szFrom)
        FREE_MEMORY(szFrom);
    if (NULL != szUntil)
        FREE_MEMORY(szUntil);
    return szResult;
}

#ifdef DONT_USE_IERT
/***
*char *StrTokEx(pstring, control) - tokenize string with delimiter in control
*
*Purpose:
*       StrTokEx considers the string to consist of a sequence of zero or more
*       text tokens separated by spans of one or more control chars. the first
*       call, with string specified, returns a pointer to the first char of the
*       first token, and will write a null char into pstring immediately
*       following the returned token. when no tokens remain
*       in pstring a NULL pointer is returned. remember the control chars with a
*       bit map, one bit per ascii char. the null char is always a control char.
*
*Entry:
*       char **pstring - ptr to ptr to string to tokenize
*       char *control - string of characters to use as delimiters
*
*Exit:
*       returns pointer to first token in string,
*       returns NULL when no more tokens remain.
*       pstring points to the beginning of the next token.
*
*WARNING!!!
*       upon exit, the first delimiter in the input string will be replaced with '\0'
*
*******************************************************************************/

char * StrTokEx (char ** pstring, const char * control)
{
        unsigned char *str;
        const unsigned char *ctrl = (const unsigned char *)control;
        unsigned char map[32];
        int count;

        char *tokenstr;

        if(*pstring == NULL)
            return NULL;
            
        /* Clear control map */
        for (count = 0; count < 32; count++)
                map[count] = 0;

        /* Set bits in delimiter table */
        do
        {
            map[*ctrl >> 3] |= (1 << (*ctrl & 7));
        } while (*ctrl++);

        /* Initialize str. */
        str = (unsigned char *)*pstring;
        
        /* Find beginning of token (skip over leading delimiters). Note that
         * there is no token if this loop sets str to point to the terminal
         * null (*str == '\0') */
        while ( (map[*str >> 3] & (1 << (*str & 7))) && *str )
            str++;

        tokenstr = (char *)str;

        /* Find the end of the token. If it is not the end of the string,
         * put a null there. */
        for ( ; *str ; str++ )
        {
            if ( map[*str >> 3] & (1 << (*str & 7)) ) 
            {
                *str++ = '\0';
                break;
            }
        }

        /* string now points to beginning of next token */
        *pstring = (char *)str;

        /* Determine if a token has been found. */
        if ( tokenstr == (char *)str )
            return NULL;
        else
            return tokenstr;
}

/***
* double StrToDbl(const char *str, char **strStop) - convert string to double
*
* Purpose:
*           To convert a string into a double.  This function supports
*           simple double representations like '1.234', '.5678'.  It also support
*           the a killobyte computaion by appending 'k' to the end of the string
*           as in '1.5k' or '.5k'.  The results would then become 1536 and 512.5.
*
* Return:
*           The double representation of the string.
*           strStop points to the character that caused the scan to stop.
*
*******************************************************************************/

double StrToDbl(const char *str, char **strStop)
{
    double dbl = 0;
    char *psz;
    int iMult = 1;
    int iKB = 1;
    int iVal = 0;
    BOOL bHaveDot = FALSE;

    psz = (char*)str;
    while(*psz)
    {
        if((*psz >= '0') && (*psz <= '9'))
        {
            iVal = (iVal * 10) + (*psz - '0');
            if(bHaveDot)
                iMult *= 10;
        }
        else if((*psz == '.') && !bHaveDot)
        {
            bHaveDot = TRUE;
        }
        else if((*psz == 'k') || (*psz == 'K'))
        {
            iKB = 1024;
            psz++;
            break;
        }
        else
        {
            break;
        }
        psz++;
    }
    *strStop = psz;

    dbl = (double) (iVal * iKB) / iMult;
    
    return(dbl);
}
#endif  // DONT_USE_IERT


// This function does not perform an exhaustive test to determine
// whether or not the string is a valid IP address.  It's only for
// determining whether anything else other than dots and numbers
// are contained within the string.
BOOL IsAddressValidIPString(LPCSTR pszHostIP)
{
    if (pszHostIP)
    {
        LPCSTR pszSearch = (pszHostIP + lstrlen(pszHostIP) - 1);

        while (pszSearch >= pszHostIP)
        {
            if (!isdigit(*pszSearch) && !(*pszSearch == '.'))
                return FALSE;

            --pszSearch;
        }
        // Made it through, so the crude check passed.
        return TRUE;
    }
    else
        return FALSE;
}
