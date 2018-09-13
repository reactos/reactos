/*++

Copyright (c) 1994-98  Microsoft Corporation

Module Name:

    options.cxx

Abstract:

    Contains the Internet*Option APIs

    Contents:
        InternetQueryOptionA
        InternetSetOptionA
        InternetSetOptionExA
        InternetQueryOptionW
        InternetSetOptionW
        InternetSetOptionExW
        (FValidCacheHandleType)

Author:

    Richard L Firth (rfirth) 02-Mar-1995

Environment:

    Win32 user-mode DLL

Revision History:

    02-Mar-1995 rfirth
        Created

    07-Mar-1995 madana

    07-Jul-1998 Forked by akabir

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "inetapiu.h"
#include "autodial.h"
#include "secinit.h"

//
// private macros
//

//
// IS_PER_THREAD_OPTION - options applicable to the thread (HINTERNET is NULL).
// Subset of IS_VALID_OPTION()
//

//#define IS_PER_THREAD_OPTION(option)                        \
//    (( ((option) == INTERNET_OPTION_SERVER_ERROR_CALLBACK)  \
//    || ((option) == INTERNET_OPTION_ASYNC_ID)               \
//    || ((option) == INTERNET_OPTION_EXTENDED_ERROR)         \
//    ) ? TRUE : FALSE)

#define IS_PER_THREAD_OPTION(option)                        \
    (( ((option) == INTERNET_OPTION_ASYNC_ID)               \
    || ((option) == INTERNET_OPTION_EXTENDED_ERROR)         \
    || ((option) == INTERNET_OPTION_PER_CONNECTION_OPTION)  \
    ) ? TRUE : FALSE)

//
// IS_PER_PROCESS_OPTION - options applicable to the process (HINTERNET is NULL).
// Subset of IS_VALID_OPTION()
//

#define IS_PER_PROCESS_OPTION(option)                       \
    (( ((option) == INTERNET_OPTION_GET_DEBUG_INFO)         \
    || ((option) == INTERNET_OPTION_SET_DEBUG_INFO)         \
    || ((option) == INTERNET_OPTION_GET_HANDLE_COUNT)       \
    || ((option) == INTERNET_OPTION_CONNECT_TIMEOUT)        \
    || ((option) == INTERNET_OPTION_CONNECT_RETRIES)        \
    || ((option) == INTERNET_OPTION_CONNECT_BACKOFF)        \
    || ((option) == INTERNET_OPTION_SEND_TIMEOUT)           \
    || ((option) == INTERNET_OPTION_RECEIVE_TIMEOUT)        \
    || ((option) == INTERNET_OPTION_DATA_SEND_TIMEOUT)      \
    || ((option) == INTERNET_OPTION_DATA_RECEIVE_TIMEOUT)   \
    || ((option) == INTERNET_OPTION_FROM_CACHE_TIMEOUT)     \
    || ((option) == INTERNET_OPTION_REFRESH)                \
    || ((option) == INTERNET_OPTION_PROXY)                  \
    || ((option) == INTERNET_OPTION_SETTINGS_CHANGED)       \
    || ((option) == INTERNET_OPTION_VERSION)                \
    || ((option) == INTERNET_OPTION_END_BROWSER_SESSION)    \
    || ((option) == INTERNET_OPTION_RESET_URLCACHE_SESSION) \
    || ((option) == INTERNET_OPTION_OFFLINE_TIMEOUT)        \
    || ((option) == INTERNET_OPTION_LINE_STATE)             \
    || ((option) == INTERNET_OPTION_IDLE_STATE)             \
    || ((option) == INTERNET_OPTION_OFFLINE_SEMANTICS)      \
    || ((option) == INTERNET_OPTION_HTTP_VERSION)           \
    || ((option) == INTERNET_OPTION_BYPASS_EDITED_ENTRY)    \
    || ((option) == INTERNET_OPTION_MAX_CONNS_PER_SERVER)   \
    || ((option) == INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER)    \
    || ((option) == INTERNET_OPTION_DIGEST_AUTH_UNLOAD)    \
    || ((option) == INTERNET_OPTION_PER_CONNECTION_OPTION)  \
    ) ? TRUE : FALSE)

//
// IS_DEBUG_OPTION - the set of debug-specific options
//

#define IS_DEBUG_OPTION(option)                     \
    (( ((option) >= INTERNET_FIRST_DEBUG_OPTION)    \
    && ((option) <= INTERNET_LAST_DEBUG_OPTION)     \
    ) ? TRUE : FALSE)

//
// IS_VALID_OPTION - the set of known option values, for a HINTERNET, thread, or
// process. In the retail version, debug options are invalid
//

#if INET_DEBUG

#define IS_VALID_OPTION(option)             \
    (((((option) >= INTERNET_FIRST_OPTION)  \
    && ((option) <= INTERNET_LAST_OPTION_INTERNAL))  \
    || IS_DEBUG_OPTION(option)              \
    ) ? TRUE : FALSE)

#else

#define IS_VALID_OPTION(option)             \
    (((((option) >= INTERNET_FIRST_OPTION)  \
    && ((option) <= INTERNET_LAST_OPTION_INTERNAL))  \
    ) ? TRUE : FALSE)

#endif // INET_DEBUG

//
// IS_CONNECT_HANDLE_TYPE - TRUE if handle type contains INTERNET_CONNECT_HANDLE_OBJECT
//

#define IS_CONNECT_HANDLE_TYPE(handleType)          \
    ((handleType == TypeHttpRequestHandle)          \
     || (handleType == TypeHttpConnectHandle)       \
     || (handleType == TypeFtpConnectHandle)        \
     || (handleType == TypeFtpFileHandle)           \
     || (handleType == TypeFtpFindHandle)           \
     || (handleType == TypeFtpFileHandleHtml)       \
     || (handleType == TypeFtpFindHandleHtml)       \
     || (handleType == TypeGopherConnectHandle)     \
     || (handleType == TypeGopherFileHandle)        \
     || (handleType == TypeGopherFindHandle)        \
     || (handleType == TypeGopherFileHandleHtml)    \
     || (handleType == TypeGopherFindHandleHtml))

//
// private prototypes
//

PRIVATE
BOOL
FValidCacheHandleType(
    HINTERNET_HANDLE_TYPE   hType
    );

PRIVATE
VOID
InitIPCOList(LPINTERNET_PER_CONN_OPTION_LISTW plistW, LPINTERNET_PER_CONN_OPTION_LISTA plistA)
{
    plistA->dwSize = sizeof(INTERNET_PER_CONN_OPTION_LISTA);
    plistA->dwOptionCount = plistW->dwOptionCount;
    if (plistW->pszConnection && *plistW->pszConnection)
    {
        SHUnicodeToAnsi(plistW->pszConnection, plistA->pszConnection, RAS_MaxEntryName + 1);
    }
    else
    {
        plistA->pszConnection = NULL;
    }
}

//
// functions
//


INTERNETAPI
BOOL
WINAPI
InternetQueryOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns information about various handle-specific variables

Arguments:

    hInternet           - handle of object for which information will be
                          returned

    dwOption            - the handle-specific INTERNET_OPTION to query

    lpBuffer            - pointer to a buffer which will receive results

    lpdwBufferLength    - IN: number of bytes available in lpBuffer
                          OUT: number of bytes returned in lpBuffer

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info:
                    ERROR_INVALID_HANDLE
                        hInternet does not identify a valid Internet handle
                        object

                    ERROR_INTERNET_INTERNAL_ERROR
                        Shouldn't see this?

                    ERROR_INVALID_PARAMETER
                        One of the parameters was bad

                    ERROR_INSUFFICIENT_BUFFER
                        lpBuffer is not large enough to hold the requested
                        information; *lpdwBufferLength contains the number of
                        bytes needed

                    ERROR_INTERNET_INCORRECT_HANDLE_TYPE
                        The handle is the wrong type for the requested option

                    ERROR_INTERNET_INVALID_OPTION
                        The option is unrecognized

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetQueryOptionA",
                     "%#x, %s (%d), %#x, %#x [%d]",
                     hInternet,
                     InternetMapOption(dwOption),
                     dwOption,
                     lpBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength
                        ? (!IsBadReadPtr(lpdwBufferLength, sizeof(DWORD))
                            ? *lpdwBufferLength
                            : 0)
                        : 0
                     ));

    DWORD error;
    BOOL success;
    HINTERNET_HANDLE_TYPE handleType;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD requiredSize = 0;
    LPVOID lpSource;
    DWORD dwValue;
    DWORD_PTR dwPtrValue;
    HANDLE hValue;
    HINTERNET hObjectMapped = NULL;
    INTERNET_CONNECT_HANDLE_OBJECT * lphRequest;
    BOOL isString = FALSE;
    INTERNET_DIAGNOSTIC_SOCKET_INFO socketInfo;

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto done;
        }
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    //
    // validate parameters
    //

    if (!ARGUMENT_PRESENT(lpdwBufferLength)) {
        error = ERROR_INVALID_PARAMETER;
        goto done;
    }

    if (!ARGUMENT_PRESENT(lpBuffer)) {
        *lpdwBufferLength = 0;
    }

    //
    // validate the handle and get its type
    //

    HINTERNET hOriginal;

    hOriginal = hInternet;
    if (ARGUMENT_PRESENT(hInternet)) {

        //
        // map the handle
        //

        error = MapHandleToAddress(hInternet, (LPVOID *)&hInternet, FALSE);
        if (error == ERROR_SUCCESS) {
            hObjectMapped = hInternet;
            lphRequest = (INTERNET_CONNECT_HANDLE_OBJECT *)hInternet;
            error = RGetHandleType(hInternet, &handleType);
        }
    } else if (IS_PER_THREAD_OPTION(dwOption)) {

        //
        // this option updates the per-thread information block, so this is a
        // good point at which to get it
        //

        if (lpThreadInfo != NULL) {
            error = ERROR_SUCCESS;
        } else {

            DEBUG_PRINT(INET,
                        ERROR,
                        ("InternetGetThreadInfo() returns NULL\n"
                        ));

            //
            // we never expect this - ERROR_INTERNET_SPANISH_INQUISITION
            //

            INET_ASSERT(FALSE);

            error = ERROR_INTERNET_INTERNAL_ERROR;
        }
    } else if (IS_PER_PROCESS_OPTION(dwOption)) {
        error = ERROR_SUCCESS;
    } else {

        //
        // catch any invalid options for the NULL handle. If the option is valid
        // then it is incorrect for this handle type, otherwise its an invalid
        // option, period
        //

        error = IS_VALID_OPTION(dwOption)
                    ? ERROR_INTERNET_INCORRECT_HANDLE_TYPE
                    : ERROR_INTERNET_INVALID_OPTION
                    ;
    }

    //
    // if the option and handle combination is valid then query the option value
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }
    switch (dwOption) {
    case INTERNET_OPTION_CALLBACK:
        requiredSize = sizeof(INTERNET_STATUS_CALLBACK);
        if (hInternet != NULL) {
            error = RGetStatusCallback(hInternet,
                                       (LPINTERNET_STATUS_CALLBACK)&dwValue
                                       );
            lpSource = (LPVOID)&dwValue;
        } else {
            error = ERROR_INVALID_HANDLE;
        }
        break;

    case INTERNET_OPTION_CONNECT_TIMEOUT:
    case INTERNET_OPTION_CONNECT_RETRIES:
    case INTERNET_OPTION_CONNECT_BACKOFF:
    case INTERNET_OPTION_SEND_TIMEOUT:
    case INTERNET_OPTION_RECEIVE_TIMEOUT:
    case INTERNET_OPTION_DATA_SEND_TIMEOUT:
    case INTERNET_OPTION_DATA_RECEIVE_TIMEOUT:
    case INTERNET_OPTION_FROM_CACHE_TIMEOUT:
        requiredSize = sizeof(DWORD);

        //
        // remember hInternet in the INTERNET_THREAD_INFO then call
        // GetTimeoutValue(). If hInternet refers to a valid Internet
        // object handle, then the relevant timeout value will be
        // returned from that, else we will return the global value
        // corresponding to the requested option
        //

        InternetSetObjectHandle(hOriginal, hInternet);
        dwValue = GetTimeoutValue(dwOption);
        lpSource = (LPVOID)&dwValue;
        break;

    case INTERNET_OPTION_HANDLE_TYPE:
        requiredSize = sizeof(dwValue);
        switch (handleType) {
        case TypeInternetHandle:
            dwValue = INTERNET_HANDLE_TYPE_INTERNET;
            break;

        case TypeFtpConnectHandle:
            dwValue = INTERNET_HANDLE_TYPE_CONNECT_FTP;
            break;

        case TypeFtpFindHandle:
            dwValue = INTERNET_HANDLE_TYPE_FTP_FIND;
            break;

        case TypeFtpFindHandleHtml:
            dwValue = INTERNET_HANDLE_TYPE_FTP_FIND_HTML;
            break;

        case TypeFtpFileHandle:
            dwValue = INTERNET_HANDLE_TYPE_FTP_FILE;
            break;

        case TypeFtpFileHandleHtml:
            dwValue = INTERNET_HANDLE_TYPE_FTP_FILE_HTML;
            break;

        case TypeGopherConnectHandle:
            dwValue = INTERNET_HANDLE_TYPE_CONNECT_GOPHER;
            break;

        case TypeGopherFindHandle:
            dwValue = INTERNET_HANDLE_TYPE_GOPHER_FIND;
            break;

        case TypeGopherFindHandleHtml:
            dwValue = INTERNET_HANDLE_TYPE_GOPHER_FIND_HTML;
            break;

        case TypeGopherFileHandle:
            dwValue = INTERNET_HANDLE_TYPE_GOPHER_FILE;
            break;

        case TypeGopherFileHandleHtml:
            dwValue = INTERNET_HANDLE_TYPE_GOPHER_FILE_HTML;
            break;

        case TypeHttpConnectHandle:
            dwValue = INTERNET_HANDLE_TYPE_CONNECT_HTTP;
            break;

        case TypeHttpRequestHandle:
            dwValue = INTERNET_HANDLE_TYPE_HTTP_REQUEST;
            break;

        case TypeFileRequestHandle:
            dwValue = INTERNET_HANDLE_TYPE_FILE_REQUEST;
            break;

        default:
            error = ERROR_INTERNET_INTERNAL_ERROR;
            break;
        }
        lpSource = (LPVOID)&dwValue;
        break;

    case INTERNET_OPTION_CONTEXT_VALUE:
    case INTERNET_OPTION_CONTEXT_VALUE_OLD: // see InternetSetOption
        requiredSize = sizeof(DWORD_PTR);
        error = RGetContext(hInternet, &dwPtrValue);
        lpSource = (LPVOID)&dwPtrValue;
        break;

    //case INTERNET_OPTION_NAME_RES_THREAD:
    //    requiredSize = sizeof(BOOL);
    //    lpSource = (LPVOID)&MultiThreadedNameResolution;
    //    break;

    case INTERNET_OPTION_READ_BUFFER_SIZE:
    case INTERNET_OPTION_WRITE_BUFFER_SIZE:
        if (IS_CONNECT_HANDLE_TYPE(handleType)) {
            requiredSize = sizeof(DWORD);
            error = RGetBufferSize(hInternet, dwOption, &dwValue);
            lpSource = (LPVOID)&dwValue;
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    //case INTERNET_OPTION_GATEWAY_NAME:
    //    error = ERROR_CALL_NOT_IMPLEMENTED;
    //    break;

    case INTERNET_OPTION_ASYNC_ID:
        error = ERROR_CALL_NOT_IMPLEMENTED;
        break;

    case INTERNET_OPTION_ASYNC_PRIORITY:
        error = ERROR_CALL_NOT_IMPLEMENTED;
        break;

    //case INTERNET_OPTION_ASYNC_REQUEST_COUNT:
    //    requiredSize = sizeof(dwValue);
    //    error = RGetAsyncRequestCount(hInternet, &dwValue);
    //    lpSource = (LPVOID)&dwValue;
    //    break;

    case INTERNET_OPTION_PARENT_HANDLE:
        hInternet = ((HANDLE_OBJECT *)hInternet)->GetParent();
        if (hInternet != NULL) {
            hInternet = ((HANDLE_OBJECT *)hInternet)->GetPseudoHandle();
        }
        requiredSize = sizeof(hInternet);
        lpSource = (LPVOID)&hInternet;
        break;

    case INTERNET_OPTION_KEEP_CONNECTION:
        if (handleType == TypeHttpConnectHandle) {
            requiredSize = sizeof(BOOL);

            //
            // we return TRUE or FALSE based on whether the connect
            // object believes the server supports Keep-Alive
            //

            //dwValue = RGetKeepAliveState(hInternet);
            lpSource = (LPVOID)&dwValue;
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_REQUEST_FLAGS:
        requiredSize = sizeof(dwValue);
        if (FValidCacheHandleType(handleType)) {
            dwValue = 0;
            if (lphRequest->IsFromCache()) {
                dwValue |= INTERNET_REQFLAG_FROM_CACHE;
                if (lphRequest->IsNetFailed()) {
                    dwValue |= INTERNET_REQFLAG_NET_TIMEOUT;
                }
            }
            if (lphRequest->IsViaProxy()) {
                dwValue |= INTERNET_REQFLAG_VIA_PROXY;
            }
            if (lphRequest->IsNoHeaders()) {
                dwValue |= INTERNET_REQFLAG_NO_HEADERS;
            }
            if (lphRequest->IsCacheWriteDisabled()) {
                dwValue |= INTERNET_REQFLAG_CACHE_WRITE_DISABLED;
            }
            lpSource = (LPVOID)&dwValue;
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_EXTENDED_ERROR:
        requiredSize = sizeof(lpThreadInfo->dwMappedErrorCode);
        lpSource = (LPVOID)&lpThreadInfo->dwMappedErrorCode;
        break;

    case INTERNET_OPTION_OFFLINE_MODE:
        error = ERROR_CALL_NOT_IMPLEMENTED;
        break;

    case INTERNET_OPTION_CACHE_STREAM_HANDLE:
        requiredSize = sizeof(HANDLE);
        if (FValidCacheHandleType(handleType)) {
            error = lphRequest->GetCacheStream((LPBYTE)&hValue,
                                               sizeof(hValue)
                                               );
            lpSource = (LPVOID)&hValue;
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    BOOL fUser, fProxy;

    case INTERNET_OPTION_USERNAME:
        fUser = IS_USER;
        fProxy = IS_SERVER;
        goto callGetUserOrPass;

    case INTERNET_OPTION_PASSWORD:
        fUser = IS_PASS;
        fProxy = IS_SERVER;
        goto callGetUserOrPass;

    case INTERNET_OPTION_PROXY_USERNAME:
        fUser = IS_USER;
        fProxy = IS_PROXY;
        goto callGetUserOrPass;

    case INTERNET_OPTION_PROXY_PASSWORD:
        fUser = IS_PASS;
        fProxy = IS_PROXY;
        goto callGetUserOrPass;

callGetUserOrPass:

        if ((handleType != TypeInternetHandle)
            && (handleType != TypeFileRequestHandle)) {
            lpSource = lphRequest->GetUserOrPass(fUser, fProxy);
            isString = TRUE;
            error = ERROR_SUCCESS;
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;


    case INTERNET_OPTION_ASYNC:
        error = ERROR_CALL_NOT_IMPLEMENTED;
        break;

    case INTERNET_OPTION_SECURITY_FLAGS:

        if (handleType != TypeHttpRequestHandle)
        {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        else
        {
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;

            requiredSize = sizeof(dwValue);
            dwValue = 0;
            lpSource = (LPVOID)&dwValue;

            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;

            dwValue = lphHttpRqst->GetSecureFlags();

            DEBUG_PRINT(INET,
                        INFO,
                        ("SECURITY_FLAGS: %X\n",
                        dwValue
                        ));


            error = ERROR_SUCCESS;
        }

        break;


    case INTERNET_OPTION_DATAFILE_NAME:
        if ((handleType == TypeHttpRequestHandle)
        || (handleType == TypeFtpFindHandle)
        || (handleType == TypeFtpFindHandleHtml)
        || (handleType == TypeFtpFileHandle)
        || (handleType == TypeFtpFileHandleHtml)
        || (handleType == TypeGopherFindHandle)
        || (handleType == TypeGopherFindHandleHtml)
        || (handleType == TypeGopherFileHandle)
        || (handleType == TypeGopherFileHandleHtml)
        || (handleType == TypeFileRequestHandle )) {

            //
            // DATAFILE_NAME is slightly different from the other string
            // options: if the name is not present then we return an error
            // to the effect that we couldn't find it. The others just
            // return an empty string
            //

            if ( handleType != TypeFileRequestHandle ) {  
                lpSource = lphRequest->GetDataFileName();
            }
            else {
                lpSource = 
                   ((INTERNET_FILE_HANDLE_OBJECT *)lphRequest)->GetDataFileName();
            }

            if (lpSource != NULL) {
                isString = TRUE;

                INET_ASSERT(error == ERROR_SUCCESS);

            } else {
                error = ERROR_INTERNET_ITEM_NOT_FOUND;
            }
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_URL:

        //
        // return the URL associated with the request handle. This may be
        // different from the original URL due to redirections
        //

        if ((handleType == TypeHttpRequestHandle)
        || (handleType == TypeFtpFindHandle)
        || (handleType == TypeFtpFindHandleHtml)
        || (handleType == TypeFtpFileHandle)
        || (handleType == TypeFtpFileHandleHtml)
        || (handleType == TypeGopherFindHandle)
        || (handleType == TypeGopherFindHandleHtml)
        || (handleType == TypeGopherFileHandle)
        || (handleType == TypeGopherFileHandleHtml)) {

            //
            // only these handle types (retrieved object handles) can have
            // associated URLs
            //

            lpSource = lphRequest->GetURL();
            isString = TRUE;

            INET_ASSERT(error == ERROR_SUCCESS);

        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;
    case INTERNET_OPTION_SECURITY_CERTIFICATE:

        if (handleType != TypeHttpRequestHandle) {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        } else {
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;
            INTERNET_SECURITY_INFO siInfo;
            INTERNET_CERTIFICATE_INFO ciInfo;
            DWORD dwciInfoSize = sizeof(INTERNET_CERTIFICATE_INFO);

            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;

            if (ERROR_SUCCESS == lphHttpRqst->GetSecurityInfo(&siInfo))
            {
                error = ConvertSecurityInfoIntoCertInfoStruct(&siInfo, &ciInfo, &dwciInfoSize);
                if(siInfo.pCertificate)
                {
                    CertFreeCertificateContext(siInfo.pCertificate);
                }

                if ( error == ERROR_SUCCESS )
                {
                    LPTSTR szResult = NULL;
                    DWORD cchNeedLen = 0;


                    szResult = FormatCertInfo(
                                &ciInfo
                                );

                    if (NULL == szResult)
                    {
                        error = ERROR_INTERNET_INVALID_OPERATION;
                        goto secOptEnd;
                    }

                    cchNeedLen = lstrlen(szResult) + 1;
                    if (*lpdwBufferLength < cchNeedLen)
                    {
                        error = ERROR_INSUFFICIENT_BUFFER;
                        goto secOptEnd;
                    }

                    if (ARGUMENT_PRESENT(lpBuffer))
                    {
                        memcpy(
                            lpBuffer,
                            szResult,
                            (cchNeedLen) * sizeof(TCHAR));
                        cchNeedLen--;
                    }

secOptEnd:
                    if (NULL != szResult) {
                        FREE_MEMORY(szResult);
                    }
                    if (NULL != ciInfo.lpszSubjectInfo) {
                        LocalFree(ciInfo.lpszSubjectInfo);
                    }
                    if (NULL != ciInfo.lpszIssuerInfo) {
                        LocalFree(ciInfo.lpszIssuerInfo);
                    }
                    if (NULL != ciInfo.lpszSignatureAlgName) {
                        LocalFree(ciInfo.lpszSignatureAlgName);
                    }
                    if (NULL != ciInfo.lpszEncryptionAlgName) {
                        LocalFree(ciInfo.lpszEncryptionAlgName);
                    }
                    if (NULL != ciInfo.lpszProtocolName) {
                        LocalFree(ciInfo.lpszProtocolName);
                    }

                    *lpdwBufferLength = cchNeedLen;
                    requiredSize = *lpdwBufferLength;
                }

                goto quit;
            }
            else
            {
                error = ERROR_INTERNET_INVALID_OPERATION;
            }
        }
        break;

    case INTERNET_OPTION_SECURITY_CONNECTION_INFO:
        //
        // Caller is expected to pass in an INTERNET_SECURITY_CONNECTION_INFO structure.

        if (handleType != TypeHttpRequestHandle) {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        } else if (*lpdwBufferLength < (DWORD)sizeof(INTERNET_SECURITY_CONNECTION_INFO)) {
            requiredSize = sizeof(INTERNET_SECURITY_CONNECTION_INFO);
            *lpdwBufferLength = requiredSize;
            error = ERROR_INSUFFICIENT_BUFFER;
        } else {
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;
            LPINTERNET_SECURITY_CONNECTION_INFO lpSecConnInfo;
            INTERNET_SECURITY_INFO ciInfo;

            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *)hInternet;
            lpSecConnInfo = (LPINTERNET_SECURITY_CONNECTION_INFO)lpBuffer;
            requiredSize = sizeof(INTERNET_SECURITY_CONNECTION_INFO);

            if ((error = lphHttpRqst->GetSecurityInfo(&ciInfo)) == ERROR_SUCCESS) {
                // Set up that data members in the structure passed in.
                lpSecConnInfo->fSecure = TRUE;

                lpSecConnInfo->dwProtocol = ciInfo.dwProtocol;
                lpSecConnInfo->aiCipher = ciInfo.aiCipher;
                lpSecConnInfo->dwCipherStrength = ciInfo.dwCipherStrength;
                lpSecConnInfo->aiHash = ciInfo.aiHash;
                lpSecConnInfo->dwHashStrength = ciInfo.dwHashStrength;
                lpSecConnInfo->aiExch = ciInfo.aiExch;
                lpSecConnInfo->dwExchStrength = ciInfo.dwExchStrength;

                if (ciInfo.pCertificate)
                {
                    CertFreeCertificateContext(ciInfo.pCertificate);
                }

            } else if (error == ERROR_INTERNET_INTERNAL_ERROR)  {
                // This implies we are not secure.
                error = ERROR_SUCCESS;
                lpSecConnInfo->fSecure = FALSE;
            }

            lpSecConnInfo->dwSize = requiredSize;
            *lpdwBufferLength = requiredSize;
        }

        goto quit;


    case INTERNET_OPTION_SECURITY_CERTIFICATE_STRUCT:

        //
        // Allocates memory that caller is expected to free.
        //

        if (handleType != TypeHttpRequestHandle) {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        } else {
            LPTSTR szResult = NULL;
            DWORD cchNeedLen = 0;
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;
            INTERNET_SECURITY_INFO cInfo;
            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;
            requiredSize = sizeof(INTERNET_CERTIFICATE_INFO);

            if (ERROR_SUCCESS == lphHttpRqst->GetSecurityInfo(&cInfo))
            {
                error = ConvertSecurityInfoIntoCertInfoStruct(&cInfo, (LPINTERNET_CERTIFICATE_INFO)lpBuffer, lpdwBufferLength);
                if(cInfo.pCertificate)
                {
                    CertFreeCertificateContext(cInfo.pCertificate);
                }
                goto quit;
            }
            else
            {
                error = ERROR_INTERNET_INVALID_OPERATION;
            }
        }
        break;

    case INTERNET_OPTION_SECURITY_KEY_BITNESS:

        if (handleType != TypeHttpRequestHandle)
        {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        else
        {
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;
            INTERNET_SECURITY_INFO secInfo;

            requiredSize = sizeof(dwValue);
            dwValue = 0;
            lpSource = (LPVOID)&dwValue;

            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;
            if (ERROR_SUCCESS != lphHttpRqst->GetSecurityInfo(&secInfo)) {
                error = ERROR_INTERNET_INVALID_OPERATION;
            } else {
                dwValue = secInfo.dwCipherStrength;
                CertFreeCertificateContext(secInfo.pCertificate);

                INET_ASSERT (error == ERROR_SUCCESS);

                DEBUG_PRINT(INET,
                            INFO,
                            ("SECURITY_KEY_BITNESS: %X\n",
                            dwValue
                            ));

            }
        }

        break;


    case INTERNET_OPTION_PROXY:
        if (!ARGUMENT_PRESENT(hInternet)) {

            if (!GlobalProxyInfo.IsModifiedInProcess())
            {
                FixProxySettingsForCurrentConnection(
                    FALSE
                    );
            }

            error = GlobalProxyInfo.GetProxyStringInfo(lpBuffer, lpdwBufferLength);
            requiredSize = *lpdwBufferLength;
            goto quit;
        } else if (handleType == TypeInternetHandle) {

            //
            // GetProxyInfo() will return the data, or calculate the buffer
            // length required
            //

            error = ((INTERNET_HANDLE_OBJECT *)hInternet)->GetProxyStringInfo(
                lpBuffer,
                lpdwBufferLength
                );
            requiredSize = *lpdwBufferLength;
            goto quit;
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_VERSION:
        requiredSize = sizeof(InternetVersionInfo);
        lpSource = (LPVOID)&InternetVersionInfo;
        break;

    case INTERNET_OPTION_USER_AGENT:
        if (handleType == TypeInternetHandle) {
            lpSource = ((INTERNET_HANDLE_OBJECT *)hInternet)->GetUserAgent();
            isString = TRUE;
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_LINE_STATE:

        if(InternetSettingsChanged())
        {
            ChangeGlobalSettings();     // refreshes GlobalDllState
        }

        requiredSize = sizeof(DWORD);
        lpSource = (LPVOID)&dwValue;
        dwValue = GlobalDllState
                & (INTERNET_LINE_STATE_MASK | INTERNET_STATE_OFFLINE_USER);
        break;

    case INTERNET_OPTION_IDLE_STATE:
        requiredSize = sizeof(DWORD);
        lpSource = (LPVOID)&dwValue;
        dwValue = GlobalDllState & INTERNET_STATE_IDLE;
        break;

    case INTERNET_OPTION_OFFLINE_SEMANTICS:
        requiredSize = sizeof(DWORD);
        dwValue = FALSE;
        lpSource = (LPVOID)&dwValue;
        break;

    case INTERNET_OPTION_SECONDARY_CACHE_KEY:
        if (handleType == TypeHttpRequestHandle) {
                        lphRequest = (INTERNET_CONNECT_HANDLE_OBJECT *)hInternet;
                        lpSource = lphRequest->GetSecondaryCacheKey();
                        isString = TRUE;
                } else {
                        error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
                }
        break;

    case INTERNET_OPTION_CALLBACK_FILTER:
        error = ERROR_NOT_SUPPORTED;
        break;

    case INTERNET_OPTION_CONNECT_TIME:
        error = ERROR_NOT_SUPPORTED;
        break;

    case INTERNET_OPTION_SEND_THROUGHPUT:
        error = ERROR_NOT_SUPPORTED;
        break;

    case INTERNET_OPTION_RECEIVE_THROUGHPUT:
        error = ERROR_NOT_SUPPORTED;
        break;

    case INTERNET_OPTION_REQUEST_PRIORITY:
        if (handleType == TypeHttpRequestHandle) {
            requiredSize = sizeof(dwValue);
            dwValue = ((HTTP_REQUEST_HANDLE_OBJECT *)hInternet)->GetPriority();
            lpSource = (LPVOID)&dwValue;
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_HTTP_VERSION:
        requiredSize = sizeof(HttpVersionInfo);
        lpSource = (LPVOID)&HttpVersionInfo;
        break;

    case INTERNET_OPTION_NET_SPEED:
        break;

    case INTERNET_OPTION_BYPASS_EDITED_ENTRY:
        requiredSize = sizeof(BOOL);
        dwValue = GlobalBypassEditedEntry;
        lpSource = (LPVOID)&dwValue;
        break;

    case INTERNET_OPTION_DIAGNOSTIC_SOCKET_INFO:

        //
        // internal option
        //

        if (handleType == TypeHttpRequestHandle) {
            requiredSize = sizeof(socketInfo);
            lpSource = (LPVOID)&socketInfo;

            HTTP_REQUEST_HANDLE_OBJECT * pReq;

            pReq = (HTTP_REQUEST_HANDLE_OBJECT *)hInternet;
            socketInfo.Socket = pReq->GetSocket();
            socketInfo.SourcePort = pReq->GetSourcePort();
            socketInfo.DestPort = pReq->GetDestPort();
            socketInfo.Flags = (pReq->FromKeepAlivePool()
                                    ? IDSI_FLAG_KEEP_ALIVE : 0)
                                | (pReq->IsSecure()
                                    ? IDSI_FLAG_SECURE : 0)
                                | (pReq->IsRequestUsingProxy()
                                    ? IDSI_FLAG_PROXY : 0)
                                | (pReq->IsTunnel()
                                    ? IDSI_FLAG_TUNNEL : 0);
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_CACHE_TIMESTAMPS:
        if (handleType == TypeHttpRequestHandle) {
            if (*lpdwBufferLength == sizeof(INTERNET_CACHE_TIMESTAMPS)) {
                INTERNET_CACHE_TIMESTAMPS* ts =
                    (INTERNET_CACHE_TIMESTAMPS*)lpBuffer;
                BOOL bU1;
                BOOL bU2;
                BOOL bU3;
                FILETIME ftPostCheck;

                ((HTTP_REQUEST_HANDLE_OBJECT *)hInternet)->
                    GetTimeStampsForCache(
                        &(ts->ftExpires),
                        &(ts->ftLastModified),
                        &ftPostCheck,
                        &bU1, &bU2, &bU3 );
            } else {
                error = ERROR_INTERNET_BAD_OPTION_LENGTH;
            }
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        goto quit;
        break;

    case INTERNET_OPTION_DISABLE_AUTODIAL:
        error = ERROR_NOT_SUPPORTED;
        break;

    // IE5 #23845: Wininet: Various auth related feature requests for FP
    // This will return TRUE if post data will be sent on a request.
    case INTERNET_OPTION_DETECT_POST_SEND:
        if (handleType == TypeHttpRequestHandle)
        {
            requiredSize = sizeof(DWORD);
            lpSource = (LPVOID)&dwValue;
            HTTP_REQUEST_HANDLE_OBJECT *pRequest;
            pRequest = (HTTP_REQUEST_HANDLE_OBJECT*) hInternet;

            if (pRequest->GetAuthState() == AUTHSTATE_NEGOTIATE
                && !((PLUG_CTX*)(pRequest->GetAuthCtx()))->_fNTLMProxyAuth
                && !(pRequest->GetAuthCtx()->GetSchemeType() == AUTHCTX::SCHEME_DPA))
            {
                dwValue = 0;
            }
            else
            {
                dwValue = 1;
            }
        }
        else
        {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_MAX_CONNS_PER_SERVER:
        if( !hInternet )  {
            requiredSize = sizeof(dwValue);
            dwValue = 0;
            lpSource = (LPVOID)&dwValue;
            dwValue = GlobalMaxConnectionsPerServer;
        }
        else
            error = ERROR_INTERNET_INVALID_OPERATION;

        break;

    case INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER:
        if( !hInternet ) {
            requiredSize = sizeof(dwValue);
            dwValue = 0;
            lpSource = (LPVOID)&dwValue;
            dwValue = GlobalMaxConnectionsPer1_0Server;
        }
        else
            error = ERROR_INTERNET_INVALID_OPERATION;

        break;

    case INTERNET_OPTION_PER_CONNECTION_OPTION:
        {
            if (handleType != TypeInternetHandle) {
                hInternet = NULL;
            }

            error = QueryPerConnOptions(hInternet,
                                        lpThreadInfo->IsAutoProxyProxyThread,
                                        (LPINTERNET_PER_CONN_OPTION_LIST)lpBuffer);

            requiredSize = *lpdwBufferLength;
            goto quit;
        }

#if INET_DEBUG

    case INTERNET_OPTION_GET_DEBUG_INFO:
        error = InternetGetDebugInfo((LPINTERNET_DEBUG_INFO)lpBuffer,
                                     lpdwBufferLength
                                     );

        //
        // everything updated, so quit without going through common buffer
        // processing
        //

        goto quit;
        break;

    case INTERNET_OPTION_GET_HANDLE_COUNT:
        requiredSize = sizeof(DWORD);
        dwValue = InternetHandleCount();
        lpSource = (LPVOID)&dwValue;
        break;

#endif // INET_DEBUG

    default:
        requiredSize = 0;
        error = ERROR_INVALID_PARAMETER;
        break;
    }

    //
    // if we have a buffer and enough space, then copy the data
    //

    if (error == ERROR_SUCCESS) {

        //
        // if we are returning a string, calculate the amount of space
        // required to hold it
        //

        if (isString) {
            if (lpSource != NULL) {
                requiredSize = lstrlen((LPCSTR)lpSource) + 1;
            } else {

                //
                // option string is NULL: return an empty string
                //

                lpSource = "";
                requiredSize = 1;
            }
        }

        INET_ASSERT(lpSource != NULL);

        if ((*lpdwBufferLength >= requiredSize)
        && ARGUMENT_PRESENT(lpBuffer)) {
            memcpy(lpBuffer, lpSource, requiredSize);
            if (isString) {

                //
                // string copied successfully. Returned length is string
                // length, not buffer length, i.e. drop 1 for '\0'
                //

                --requiredSize;
            }
        } else {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
    }

quit:

    //
    // return the amount the app needs to supply, or the amount of data in the
    // buffer, depending on success/failure status
    //

    *lpdwBufferLength = requiredSize;

    if (hObjectMapped != NULL) {
        DereferenceObject((LPVOID)hObjectMapped);
    }

done:

    if (error == ERROR_SUCCESS) {
        success = TRUE;

        IF_DEBUG(API) {

            if (isString) {

                DEBUG_PRINT_API(API,
                                INFO,
                                ("returning %q (%d chars)\n",
                                lpBuffer,
                                requiredSize
                                ));

            } else {

                DEBUG_DUMP_API(API,
                               "option data:\n",
                               lpBuffer,
                               requiredSize
                               );

            }
        }
    } else {

        DEBUG_ERROR(API, error);

        IF_DEBUG(API) {

            if (error == ERROR_INSUFFICIENT_BUFFER) {

                DEBUG_PRINT_API(API,
                                INFO,
                                ("*lpdwBufferLength (%#x)= %d\n",
                                lpdwBufferLength,
                                *lpdwBufferLength
                                ));

            }
        }

        SetLastError(error);
        success = FALSE;
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
BOOL
WINAPI
InternetQueryOptionW(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    OUT LPVOID lpBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hInternet           -

    dwOption            -

    lpBuffer            -

    lpdwBufferLength    -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetQueryOptionW",
                     "%#x, %s (%d), %#x, %#x [%d]",
                     hInternet,
                     InternetMapOption(dwOption),
                     dwOption,
                     lpBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength
                        ? (!IsBadReadPtr(lpdwBufferLength, sizeof(DWORD))
                            ? *lpdwBufferLength
                            : 0)
                        : 0
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpBuffer;

    switch (dwOption)
    {
    case INTERNET_OPTION_USERNAME:
    case INTERNET_OPTION_PASSWORD:
    case INTERNET_OPTION_DATAFILE_NAME:
    case INTERNET_OPTION_URL:
    case INTERNET_OPTION_USER_AGENT:
    case INTERNET_OPTION_PROXY_USERNAME:
    case INTERNET_OPTION_PROXY_PASSWORD:
    case INTERNET_OPTION_SECONDARY_CACHE_KEY:
        if (lpBuffer)
        {
            mpBuffer.dwAlloc = mpBuffer.dwSize = *lpdwBufferLength;
            mpBuffer.psStr = (LPSTR)ALLOC_BYTES(mpBuffer.dwAlloc*sizeof(CHAR));
            if (!mpBuffer.psStr)
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                break;
            }
        }
        fResult = InternetQueryOptionA(hInternet,
                                  dwOption,
                                  (LPVOID)mpBuffer.psStr,
                                  &mpBuffer.dwSize
                                 );
        if (fResult)
        {
            *lpdwBufferLength = MultiByteToWideChar(CP_ACP, 0, mpBuffer.psStr, mpBuffer.dwSize + 1,
                        NULL, 0);
            if (*lpdwBufferLength*sizeof(WCHAR)<=mpBuffer.dwAlloc && lpBuffer)
            {
                MultiByteToWideChar(CP_ACP, 0, mpBuffer.psStr, mpBuffer.dwSize+1,
                        (LPWSTR)lpBuffer, *lpdwBufferLength);
                (*lpdwBufferLength)--;
            }
            else
            {
                *lpdwBufferLength *= sizeof(WCHAR);
                fResult = FALSE;
                dwErr = ERROR_INSUFFICIENT_BUFFER;
            }
        }
        else
        {
            if (GetLastError()==ERROR_INSUFFICIENT_BUFFER)
            {
                *lpdwBufferLength = mpBuffer.dwSize*sizeof(WCHAR);
            }
        }
        break;

    case INTERNET_OPTION_PER_CONNECTION_OPTION:
        {
            if (!lpBuffer)
            {
                dwErr = ERROR_INVALID_PARAMETER;
                break;
            }

            INTERNET_PER_CONN_OPTION_LISTA listA;
            LPINTERNET_PER_CONN_OPTION_LISTW plistW = (LPINTERNET_PER_CONN_OPTION_LISTW)lpBuffer;
            CHAR szEntryA[RAS_MaxEntryName + 1];
            listA.pszConnection = szEntryA;
            
            InitIPCOList(plistW, &listA);
            listA.pOptions = (LPINTERNET_PER_CONN_OPTIONA)_alloca(sizeof(INTERNET_PER_CONN_OPTIONA)*listA.dwOptionCount);

            for (DWORD i=0; i<listA.dwOptionCount; i++)
            {
                listA.pOptions[i].dwOption = plistW->pOptions[i].dwOption;
                listA.pOptions[i].Value.pszValue = NULL;
                plistW->pOptions[i].Value.pszValue = NULL;
            }

            fResult = InternetQueryOptionA(hInternet,
                                  dwOption,
                                  (PVOID)&listA,
                                  lpdwBufferLength);

            // Now, convert from ansi to unicode

            if (fResult)
            {
                for (DWORD i=0; i<listA.dwOptionCount; i++)
                {
                    switch (listA.pOptions[i].dwOption)
                    {
                    case INTERNET_PER_CONN_FLAGS:
                    case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
                    case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
                        plistW->pOptions[i].Value.dwValue = listA.pOptions[i].Value.dwValue;
                        break;

                    case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_TIME:
                        plistW->pOptions[i].Value.ftValue = listA.pOptions[i].Value.ftValue;
                        break;
                    
                    case INTERNET_PER_CONN_PROXY_SERVER:
                    case INTERNET_PER_CONN_PROXY_BYPASS:
                    case INTERNET_PER_CONN_AUTOCONFIG_URL:
                    case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
                    case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_URL:
                        if (listA.pOptions[i].Value.pszValue && *listA.pOptions[i].Value.pszValue)
                        {
                            DWORD cc = MultiByteToWideChar(CP_ACP, 
                                                           0, 
                                                           listA.pOptions[i].Value.pszValue, 
                                                           -1,
                                                           NULL,
                                                           0);
                            plistW->pOptions[i].Value.pszValue = (PWSTR)GlobalAlloc(GPTR, cc*sizeof(WCHAR));
                            if (!plistW->pOptions[i].Value.pszValue)
                            {
                                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                                goto iopco_cleanup;
                            }
                            MultiByteToWideChar(CP_ACP, 
                                                0, 
                                                listA.pOptions[i].Value.pszValue, 
                                                -1,
                                                plistW->pOptions[i].Value.pszValue,
                                                cc);
                        }
                        break;

                    default:
                        INET_ASSERT(FALSE);
                        dwErr = ERROR_INVALID_PARAMETER;
                        goto iopco_cleanup;
                        break;
                    }
                }
            }
            else
            {
                plistW->dwOptionError = listA.dwOptionError;
            }
            
        iopco_cleanup:
            // Free all the allocated buffers
            for (i=0; i<listA.dwOptionCount; i++)
            {
                switch (listA.pOptions[i].dwOption)
                {
                case INTERNET_PER_CONN_PROXY_SERVER:
                case INTERNET_PER_CONN_PROXY_BYPASS:
                case INTERNET_PER_CONN_AUTOCONFIG_URL:
                case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
                case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_URL:
                    // These should have been converted from ansi to unicode
                    // and can be freed now
                    if (listA.pOptions[i].Value.pszValue)
                    {
                        GlobalFree(listA.pOptions[i].Value.pszValue);
                    }
                    // No point in passing back buffers in the event of an error
                    // condition
                    if (dwErr && plistW->pOptions[i].Value.pszValue)
                    {
                        GlobalFree(plistW->pOptions[i].Value.pszValue);
                    }
                    break;

                default:
                    // No need to do anything
                    break;
                }
            }
        }
        break;

    default:
        fResult = InternetQueryOptionA(hInternet,
                                  dwOption,
                                  lpBuffer,
                                  lpdwBufferLength
                                 );
    }

    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
InternetSetOptionA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )

/*++

Routine Description:

    Sets a handle-specific variable, or a per-thread variable

Arguments:

    hInternet           - handle of object for which information will be set,
                          or NULL if the option defines a per-thread variable

    dwOption            - the handle-specific INTERNET_OPTION to set

    lpBuffer            - pointer to a buffer containing value to set

    dwBufferLength      - size of lpBuffer

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info:
                    ERROR_INVALID_HANDLE
                        hInternet does not identify a valid Internet handle
                        object

                    ERROR_INTERNET_INTERNAL_ERROR
                        Shouldn't see this?

                    ERROR_INVALID_PARAMETER
                        One of the parameters was bad

                    ERROR_INTERNET_INVALID_OPTION
                        The requested option cannot be set

                    ERROR_INTERNET_OPTION_NOT_SETTABLE
                        Can't set this option, only query it

                    ERROR_INTERNET_BAD_OPTION_LENGTH
                        The dwBufferLength parameter is incorrect for the
                        expected type of the option

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetSetOptionA",
                     "%#x, %s (%d), %#x [%#x], %d",
                     hInternet,
                     InternetMapOption(dwOption),
                     dwOption,
                     lpBuffer,
                     lpBuffer
                        ? (!IsBadReadPtr(lpBuffer, sizeof(DWORD))
                            ? *(LPDWORD)lpBuffer
                            : 0)
                        : 0,
                     dwBufferLength
                     ));

    DWORD error;
    BOOL success = TRUE;
    HINTERNET_HANDLE_TYPE handleType;
    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD requiredSize;
    HINTERNET hObjectMapped = NULL;
    INTERNET_CONNECT_HANDLE_OBJECT *lphRequest;

    //
    // Auth code can query on connect handle by walking up from request handle,
    // unbeknownst to the client, who might try to set the option concurrently.
    // Ideally access would be serialized only for the four combinations of
    // {user,pass} and {server,proxy}, but unconditional keeps the code simple.
    //

    //AuthLock();

    //
    // validate parameters
    //

    if ((dwBufferLength == 0) || IsBadReadPtr(lpBuffer, dwBufferLength)) {


        switch (dwOption) {

            //
            // these options don't require a buffer - don't fail request because
            // no buffer supplied
            //

            case INTERNET_OPTION_SETTINGS_CHANGED:
            case INTERNET_OPTION_END_BROWSER_SESSION:
            case INTERNET_OPTION_RESET_URLCACHE_SESSION:
            case INTERNET_OPTION_REFRESH:
            case INTERNET_OPTION_DIGEST_AUTH_UNLOAD:
            case INTERNET_OPTION_IGNORE_OFFLINE:
                break;

            default:
                error = ERROR_INVALID_PARAMETER;
                goto quit;
        }
    }

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto done;
        }
    }

    //
    // validate the handle and get its type
    //

    if (ARGUMENT_PRESENT(hInternet)) {

        //
        // map the handle
        //

        error = MapHandleToAddress(hInternet, (LPVOID *)&hInternet, FALSE);
        if (error == ERROR_SUCCESS) {
            hObjectMapped = hInternet;
            error = RGetHandleType(hInternet, &handleType);
        }
    } else if (IS_PER_THREAD_OPTION(dwOption)) {

        //
        // this option updates the per-thread information block, so this is a
        // good point at which to get it
        //

        lpThreadInfo = InternetGetThreadInfo();
        if (lpThreadInfo != NULL) {
            error = ERROR_SUCCESS;
        } else {

            DEBUG_PRINT(INET,
                        ERROR,
                        ("InternetGetThreadInfo() returns NULL\n"
                        ));

            //
            // we never expect this - ERROR_INTERNET_SPANISH_INQUISITION
            //

            INET_ASSERT(FALSE);

            error = ERROR_INTERNET_INTERNAL_ERROR;
        }
    } else if (IS_PER_PROCESS_OPTION(dwOption)) {
        error = ERROR_SUCCESS;
    } else {

        //
        // catch any invalid options for the NULL handle. If the option is valid
        // then it is incorrect for this handle type, otherwise its an invalid
        // option, period
        //

        error = IS_VALID_OPTION(dwOption)
                    ? ERROR_INTERNET_INCORRECT_HANDLE_TYPE
                    : ERROR_INTERNET_INVALID_OPTION
                    ;
    }

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // if the option and handle combination is valid then set the option value
    //

    switch (dwOption) {
    case INTERNET_OPTION_CALLBACK:
    case INTERNET_OPTION_HANDLE_TYPE:
    //case INTERNET_OPTION_GATEWAY_NAME:
    case INTERNET_OPTION_KEEP_CONNECTION:
    case INTERNET_OPTION_ASYNC_ID:
    case INTERNET_OPTION_ASYNC_REQUEST_COUNT:
    case INTERNET_OPTION_ASYNC_QUEUE_DEPTH:
    case INTERNET_OPTION_WORKER_THREAD_TIMEOUT:
    case INTERNET_OPTION_IDLE_STATE:
    case INTERNET_OPTION_CONNECT_TIME:
    case INTERNET_OPTION_SEND_THROUGHPUT:
    case INTERNET_OPTION_RECEIVE_THROUGHPUT:
    case INTERNET_OPTION_NET_SPEED:

        //
        // these options cannot be set by this function
        //

        error = ERROR_INTERNET_OPTION_NOT_SETTABLE;
        break;

    case INTERNET_OPTION_BYPASS_EDITED_ENTRY:
        requiredSize = sizeof(BOOL);
        if (dwBufferLength != requiredSize) {
            error = ERROR_INTERNET_BAD_OPTION_LENGTH;
            break;
        }
        // Only support global, not per handle yet
        if (hInternet != NULL) {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
            break;
        }

        // Made it this far, so buffer is right size and handle is NULL
        GlobalBypassEditedEntry = *(LPBOOL)lpBuffer;
        break;


    case INTERNET_OPTION_CONNECT_TIMEOUT:
    case INTERNET_OPTION_CONNECT_RETRIES:
    case INTERNET_OPTION_CONNECT_BACKOFF:
    case INTERNET_OPTION_SEND_TIMEOUT:
    case INTERNET_OPTION_RECEIVE_TIMEOUT:
    case INTERNET_OPTION_DATA_SEND_TIMEOUT:
    case INTERNET_OPTION_DATA_RECEIVE_TIMEOUT:
    case INTERNET_OPTION_FROM_CACHE_TIMEOUT:
        requiredSize = sizeof(DWORD);
        if (dwBufferLength != requiredSize) {
            error = ERROR_INTERNET_BAD_OPTION_LENGTH;
            break;
        }

        //
        // if hInternet is NULL then the app is requesting that we set the
        // global timeout values, not handle-specific ones
        //

        if (hInternet == NULL) {
            switch (dwOption) {
            case INTERNET_OPTION_CONNECT_TIMEOUT:
                GlobalConnectTimeout = *(LPDWORD)lpBuffer;
                break;

            case INTERNET_OPTION_CONNECT_RETRIES:
                GlobalConnectRetries = *(LPDWORD)lpBuffer;
                break;

            case INTERNET_OPTION_SEND_TIMEOUT:
                GlobalSendTimeout = *(LPDWORD)lpBuffer;
                break;

            case INTERNET_OPTION_RECEIVE_TIMEOUT:
                GlobalReceiveTimeout = *(LPDWORD)lpBuffer;
                break;

            case INTERNET_OPTION_DATA_SEND_TIMEOUT:
                GlobalDataSendTimeout = *(LPDWORD)lpBuffer;
                break;

            case INTERNET_OPTION_DATA_RECEIVE_TIMEOUT:
                GlobalDataReceiveTimeout = *(LPDWORD)lpBuffer;
                break;

            case INTERNET_OPTION_FROM_CACHE_TIMEOUT:
                GlobalFromCacheTimeout = *(LPDWORD)lpBuffer;
                break;
            }
            break;
        }


        //
        // we have a non-NULL context handle: the app wants to set specific
        // protocol timeouts
        //

        switch (handleType) {
        case TypeInternetHandle:
        case TypeFtpConnectHandle:
        case TypeFtpFindHandle:
        case TypeFtpFindHandleHtml:
        case TypeFtpFileHandle:
        case TypeFtpFileHandleHtml:
        case TypeGopherConnectHandle:
        case TypeGopherFindHandle:
        case TypeGopherFindHandleHtml:
        case TypeGopherFileHandle:
        case TypeGopherFileHandleHtml:
        case TypeHttpConnectHandle:
        case TypeHttpRequestHandle:

            //
            // N.B. For some of these handle types, setting a timeout etc.
            // value will have absolutely no affect (we have already gotten
            // the information after connecting, sending etc.), but we'll
            // allow the app to go ahead anyway (its benign)
            //

            error = RSetTimeout(hInternet,
                                dwOption,
                                *(LPDWORD)lpBuffer
                                );
            break;

        default:

            //
            // any other handle type (?) cannot have timeouts set for it
            //

            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
            break;
        }
        break;

    case INTERNET_OPTION_CONTEXT_VALUE:

        //
        // BUGBUG - can't change context if async operation is pending
        //

        if (dwBufferLength == sizeof(lpThreadInfo->Context)) {
            error = RSetContext(hInternet, *((DWORD*) lpBuffer));
        } else {
            error = ERROR_INTERNET_BAD_OPTION_LENGTH;
        }
        break;

    case INTERNET_OPTION_NAME_RES_THREAD:
        //if (dwBufferLength == sizeof(MultiThreadedNameResolution)) {
        //    MultiThreadedNameResolution = (BOOL)(*(LPDWORD)lpBuffer != 0);
        //} else {
        //    error = ERROR_INTERNET_BAD_OPTION_LENGTH;
        //}
        break;

    case INTERNET_OPTION_READ_BUFFER_SIZE:
    case INTERNET_OPTION_WRITE_BUFFER_SIZE:
        if (IS_CONNECT_HANDLE_TYPE(handleType)) {
            if (dwBufferLength == sizeof(DWORD)) {

                DWORD bufferSize;

                bufferSize = *(LPDWORD)lpBuffer;
                if (bufferSize > 0) {
                    error = RSetBufferSize(hInternet, dwOption, bufferSize);
                } else {

                    //
                    // the read/write buffer size cannot be set to 0
                    //

                    error = ERROR_INVALID_PARAMETER;
                }
            } else {
                error = ERROR_INTERNET_BAD_OPTION_LENGTH;
            }
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    BOOL fUser, fProxy;

    case INTERNET_OPTION_ASYNC_PRIORITY:
        error = ERROR_CALL_NOT_IMPLEMENTED;
        break;

    case INTERNET_OPTION_USERNAME:
        fUser = IS_USER;
        fProxy = IS_SERVER;
        goto callSetUserOrPass;


    case INTERNET_OPTION_PASSWORD:
        fUser = IS_PASS;
        fProxy = IS_SERVER;
        goto callSetUserOrPass;

    case INTERNET_OPTION_PROXY_USERNAME:
        fUser = IS_USER;
        fProxy = IS_PROXY;
        goto callSetUserOrPass;

    case INTERNET_OPTION_PROXY_PASSWORD:
        fUser = IS_PASS;
        fProxy = IS_PROXY;
        goto callSetUserOrPass;


callSetUserOrPass:

        if (handleType != TypeInternetHandle) {
            AuthLock();
            lphRequest = (INTERNET_CONNECT_HANDLE_OBJECT *)hInternet;
            lphRequest->SetUserOrPass ((LPSTR)lpBuffer, fUser, fProxy);
            AuthUnlock();
            error = ERROR_SUCCESS;
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }

    break;


    case INTERNET_OPTION_SECURITY_SELECT_CLIENT_CERT:

        if (handleType != TypeHttpRequestHandle)
        {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        else if (dwBufferLength < sizeof(DWORD))
        {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            CERT_CONTEXT_ARRAY* pCertContextArray;
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;

            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;

            pCertContextArray =
                lphHttpRqst->GetCertContextArray();

            if ( ! pCertContextArray )
            {
                error = ERROR_INTERNET_CLIENT_AUTH_NOT_SETUP;
            }
            else
            {

                if ( ( *((LPDWORD)lpBuffer) >= 0 && *((LPINT)lpBuffer) < (INT) pCertContextArray->GetArraySize() )
                      || *((LPINT)lpBuffer) == -1 )
                {
                    pCertContextArray->SelectCertContext( *((LPDWORD)lpBuffer) );

                    error = ERROR_SUCCESS;
                }
                else
                {
                    error = ERROR_INVALID_PARAMETER;
                }
            }
        }
        break;

    case INTERNET_OPTION_SECURITY_FLAGS:

        if (handleType != TypeHttpRequestHandle)
        {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        else if (dwBufferLength < sizeof(DWORD))
        {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            HTTP_REQUEST_HANDLE_OBJECT *lphHttpRqst;

            lphHttpRqst = (HTTP_REQUEST_HANDLE_OBJECT *) hInternet;

            lphHttpRqst->SetSecureFlags(*(LPDWORD)lpBuffer);

            error = ERROR_SUCCESS;
        }

        break;

    case INTERNET_OPTION_REFRESH:

        //
        // BUGBUG - can only accept global or InternetOpen() handles currently
        //

        if (!ARGUMENT_PRESENT(hInternet)) {
            if (!GlobalProxyInfo.IsModifiedInProcess()) {
                FixProxySettingsForCurrentConnection(TRUE);
                error = ERROR_SUCCESS;
            } else {
                INET_ASSERT(error == ERROR_SUCCESS);
            }
        } else if (handleType == TypeInternetHandle) {
            error = ((INTERNET_HANDLE_OBJECT *)hInternet)->Refresh(0);
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_PROXY:
        if ((handleType == TypeInternetHandle) || !ARGUMENT_PRESENT(hInternet)) {

            LPINTERNET_PROXY_INFO lpInfo = (LPINTERNET_PROXY_INFO)lpBuffer;

            //
            // validate parameters
            //

            if (dwBufferLength != sizeof(*lpInfo)) {
                error = ERROR_INTERNET_BAD_OPTION_LENGTH;
            } else if (!((lpInfo->dwAccessType == INTERNET_OPEN_TYPE_DIRECT)
                || (lpInfo->dwAccessType == INTERNET_OPEN_TYPE_PROXY)
                || (lpInfo->dwAccessType == INTERNET_OPEN_TYPE_PRECONFIG))
            || ((lpInfo->dwAccessType == INTERNET_OPEN_TYPE_PROXY)
                && ((lpInfo->lpszProxy == NULL) || (*lpInfo->lpszProxy == '\0')))) {
                error = ERROR_INVALID_PARAMETER;
            } else {
                if (!ARGUMENT_PRESENT(hInternet)) {

                    INTERNET_PROXY_INFO_EX info;
                    memset(&info, 0, sizeof(info));
                    info.dwFlags = PROXY_TYPE_DIRECT;
                    

                    switch (lpInfo->dwAccessType) {
                        case INTERNET_OPEN_TYPE_PRECONFIG:
                            FixProxySettingsForCurrentConnection(TRUE);
                            error = ERROR_SUCCESS;
                            goto quit;
                        case INTERNET_OPEN_TYPE_DIRECT:
                            info.dwFlags |= PROXY_TYPE_DIRECT;
                            break;
                        case INTERNET_OPEN_TYPE_PROXY:     
                            info.dwFlags |= PROXY_TYPE_PROXY;
                            info.lpszProxy = lpInfo->lpszProxy;
                            info.lpszProxyBypass = lpInfo->lpszProxyBypass;
                            break;
                    }                    
                    GlobalProxyInfo.SetProxySettings(&info, TRUE ); 

                } else {
                    error = ((INTERNET_HANDLE_OBJECT *)hInternet)->SetProxyInfo(
                                lpInfo->dwAccessType,
                                lpInfo->lpszProxy,
                                lpInfo->lpszProxyBypass
                                );
                }
            }
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_SETTINGS_CHANGED:
        {
            DWORD dwVer;

            IncrementCurrentSettingsVersion(&dwVer);
            // eat the update settings for this process,
            //  since calling ChangeGlobalSettings should suffice
            InternetSettingsChanged(); 
            ChangeGlobalSettings();
            PurgeKeepAlives(PKA_NOW);
        }

        break;

    case INTERNET_OPTION_USER_AGENT:
        if (*(LPSTR)lpBuffer == '\0') {
            error = ERROR_INTERNET_BAD_OPTION_LENGTH;
        } else {
            if (handleType == TypeInternetHandle) {
                ((INTERNET_HANDLE_OBJECT *)hInternet)->SetUserAgent((LPSTR)lpBuffer);
            } else {
                error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
            }
        }
        break;

    case INTERNET_OPTION_END_BROWSER_SESSION:

        //
        // Flush the in-memory password cache and drop any keep-alive
        // sockets which had authorization (IIS retains the credentials.
        //

        AuthFlush();
        PurgeKeepAlives (PKA_AUTH_FAILED);

        //
        // Empty the content cache if registry key is set.
        //

        UrlCacheFlush();

        //
        // Flush session cookies.
        //

        PurgeCookieJarOfStaleCookies();

        //
        // Purge Proxy Script Cache
        //

        UPDATE_GLOBAL_PROXY_VERSION();

        //
        // Flush all cached SSL Certificates.
        //

        //GlobalCertCache.ClearList();

        //
        // Need to close a global key
        //
        ResetAutodialModule();

        //
        // Close global Key
        //
        CloseInternetSettingsKey();

        //
        // Close Security Key?
        //
        CloseMyCertStore();

    // Look out: intentional fall through.

    case INTERNET_OPTION_RESET_URLCACHE_SESSION:

    // Look out: intentional fall through.

        //
        // Restart the session used for cache syncmode.
        //

        GetCurrentGmtTime ((LPFILETIME)&dwdwSessionStartTime);
        dwdwSessionStartTime -= dwdwSessionStartTimeDefaultDelta;
        
        error = ERROR_SUCCESS;
        break;

    case INTERNET_OPTION_DIGEST_AUTH_UNLOAD:
        if (DIGEST_CTX::g_pFuncTbl)
        {
            DIGEST_CTX::Logoff();
            DIGEST_CTX::g_pFuncTbl = NULL;
        }            
        break;

    case INTERNET_OPTION_LINE_STATE:
        if (dwBufferLength == sizeof(INTERNET_ONLINE_OFFLINE_INFO)) {

            LPINTERNET_ONLINE_OFFLINE_INFO lpInfo;
            DWORD state;

            lpInfo = (LPINTERNET_ONLINE_OFFLINE_INFO)lpBuffer;
            state = lpInfo->dwOfflineState;

            //
            // we allow app to pass in INTERNET_STATE_OFFLINE_USER and interpret
            // it as same as INTERNET_STATE_OFFLINE
            //

            if (state == INTERNET_STATE_OFFLINE_USER) {
                state = INTERNET_STATE_OFFLINE;
            }
            if (((state == INTERNET_STATE_ONLINE)
            || (state == INTERNET_STATE_OFFLINE))
            && ((lpInfo->dwFlags & ~ISO_FORCE_OFFLINE) == 0)) {
                error = SetOfflineUserState(state,
                                            (lpInfo->dwFlags & ISO_FORCE_OFFLINE)
                                                ? TRUE
                                                : FALSE
                                            );

                // update registry value
                InternetWriteRegistryDword("GlobalUserOffline",
                        ((state == INTERNET_STATE_ONLINE) ? 0 : 1));

                // notification
                DWORD dwOp = CACHE_NOTIFY_SET_OFFLINE;
                if( state == INTERNET_STATE_ONLINE )
                {
                    dwOp = CACHE_NOTIFY_SET_ONLINE;
                }

                UrlCacheSendNotification(dwOp);

                // invalidate global info so other instances pick it up
                DWORD dwVer;
                IncrementCurrentSettingsVersion(&dwVer);

            } else {
                error = ERROR_INVALID_PARAMETER;
            }
        } else {
            error = ERROR_INTERNET_BAD_OPTION_LENGTH;
        }
        break;

    case INTERNET_OPTION_OFFLINE_SEMANTICS:
        break;

    case INTERNET_OPTION_SECONDARY_CACHE_KEY:
                if( handleType == TypeHttpRequestHandle ) {
                        lphRequest = (INTERNET_CONNECT_HANDLE_OBJECT *)hInternet;
                        if (!lphRequest->SetSecondaryCacheKey((LPSTR) lpBuffer)) {
                                error = ERROR_NOT_ENOUGH_MEMORY;
                        } else {
                                INET_ASSERT (error == ERROR_SUCCESS);
                        }
                } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }

        break;

    case INTERNET_OPTION_CALLBACK_FILTER:
        error = ERROR_NOT_SUPPORTED;
        break;

    case INTERNET_OPTION_REQUEST_PRIORITY:
        if (handleType == TypeHttpRequestHandle) {
            if (dwBufferLength == sizeof(LONG)) {
                ((HTTP_REQUEST_HANDLE_OBJECT *)hInternet)->
                    SetPriority(*(LPLONG)lpBuffer);
            } else {
                error = ERROR_INTERNET_BAD_OPTION_LENGTH;
            }
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_HTTP_VERSION:
        if (dwBufferLength == sizeof(HTTP_VERSION_INFO)) {
            HttpVersionInfo = *(LPHTTP_VERSION_INFO)lpBuffer;
        } else {
            error = ERROR_INTERNET_BAD_OPTION_LENGTH;
        }
        break;

    case INTERNET_OPTION_ERROR_MASK:
        lphRequest = (INTERNET_CONNECT_HANDLE_OBJECT *)hInternet;
        if (dwBufferLength == sizeof(DWORD)) {
            if ( *((LPDWORD) lpBuffer) & ~(INTERNET_ERROR_MASK_INSERT_CDROM |
                                           INTERNET_ERROR_MASK_COMBINED_SEC_CERT |
                                           INTERNET_ERROR_MASK_LOGIN_FAILURE_DISPLAY_ENTITY_BODY)) {

                error = ERROR_INVALID_PARAMETER;
            } else {
                lphRequest->SetErrorMask(*(LPDWORD) lpBuffer);
            }
        } else {
            error = ERROR_INTERNET_BAD_OPTION_LENGTH;
        }
        break;

    case INTERNET_OPTION_CODEPAGE:
        if (handleType == TypeHttpRequestHandle) {
            if (dwBufferLength == sizeof(DWORD)) {
                ((HTTP_REQUEST_HANDLE_OBJECT *)hInternet)->
                    SetCodePage(*(LPDWORD)lpBuffer);
            } else {
                error = ERROR_INTERNET_BAD_OPTION_LENGTH;
            }
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_DISABLE_AUTODIAL:
        if (dwBufferLength == sizeof(DWORD)) {

            DWORD dwValue = *(LPDWORD)lpBuffer;

            SetAutodialEnable(dwValue == 0);
        } else {
            error = ERROR_INTERNET_BAD_OPTION_LENGTH;
        }
        break;

    // Override to disable NTLM preauth.
    case INTERNET_OPTION_DISABLE_NTLM_PREAUTH:
        if (handleType == TypeHttpRequestHandle) {
            if (dwBufferLength == sizeof(DWORD)) {
                ((HTTP_REQUEST_HANDLE_OBJECT *)hInternet)->
                    SetDisableNTLMPreauth(*(LPDWORD)lpBuffer);
            } else {
                error = ERROR_INTERNET_BAD_OPTION_LENGTH;
            }
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

    case INTERNET_OPTION_MAX_CONNS_PER_SERVER:
        if (dwBufferLength == sizeof(DWORD)) {
            DWORD dwValue = *(LPDWORD)lpBuffer;

            if( !hInternet )
                GlobalMaxConnectionsPerServer = dwValue;
            else
                error = ERROR_INTERNET_INVALID_OPERATION;

        } else {
            error = ERROR_INTERNET_BAD_OPTION_LENGTH;
        }
        break;


        break;
    case INTERNET_OPTION_MAX_CONNS_PER_1_0_SERVER:
        if (dwBufferLength == sizeof(DWORD)) {
            DWORD dwValue = *(LPDWORD)lpBuffer;

            if( !hInternet )
                GlobalMaxConnectionsPer1_0Server = dwValue;
            else
                error = ERROR_INTERNET_INVALID_OPERATION;

        } else {
            error = ERROR_INTERNET_BAD_OPTION_LENGTH;
        }

        break;

    case INTERNET_OPTION_PER_CONNECTION_OPTION:
        {
            if (handleType != TypeInternetHandle) {
                hInternet = NULL;
            }

            error = SetPerConnOptions(hInternet, 
                                      lpThreadInfo->IsAutoProxyProxyThread,
                                      (LPINTERNET_PER_CONN_OPTION_LIST)lpBuffer);
            break;
        }
     
    case INTERNET_OPTION_IGNORE_OFFLINE:
        if (handleType == TypeHttpRequestHandle) {
            ((HTTP_REQUEST_HANDLE_OBJECT *)hInternet)->SetIgnoreOffline();
        } else {
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        }
        break;

#if INET_DEBUG

    case INTERNET_OPTION_SET_DEBUG_INFO:
        error = InternetSetDebugInfo((LPINTERNET_DEBUG_INFO)lpBuffer,
                                     dwBufferLength
                                     );
        break;

#endif // INET_DEBUG

    default:

        //
        // this option is not recognized
        //

        error = ERROR_INTERNET_INVALID_OPTION;
    }

quit:

    //AuthUnlock();

    if (hObjectMapped != NULL) {
        DereferenceObject((LPVOID)hObjectMapped);
    }

done:

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(API, error);

        SetLastError(error);
        success = FALSE;
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
BOOL
WINAPI
InternetSetOptionW(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hInternet       -

    dwOption        -

    lpBuffer        -

    dwBufferLength  -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetSetOptionW",
                     "%#x, %s (%d), %#x [%#x], %d",
                     hInternet,
                     InternetMapOption(dwOption),
                     dwOption,
                     lpBuffer,
                     lpBuffer
                        ? (!IsBadReadPtr(lpBuffer, sizeof(DWORD))
                            ? *(LPDWORD)lpBuffer
                            : 0)
                        : 0,
                     dwBufferLength
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    MEMORYPACKET mpBuffer;
    BOOL fResult = FALSE;

    switch (dwOption)
    {
    case INTERNET_OPTION_USERNAME:
    case INTERNET_OPTION_PASSWORD:
    case INTERNET_OPTION_DATAFILE_NAME:
    case INTERNET_OPTION_URL:
    case INTERNET_OPTION_USER_AGENT:
    case INTERNET_OPTION_PROXY_USERNAME:
    case INTERNET_OPTION_PROXY_PASSWORD:
    case INTERNET_OPTION_SECONDARY_CACHE_KEY:
        ALLOC_MB((LPWSTR)lpBuffer, dwBufferLength, mpBuffer);
        if (!mpBuffer.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI((LPWSTR)lpBuffer, mpBuffer);
        fResult = InternetSetOptionA(hInternet,
                                  dwOption,
                                  mpBuffer.psStr,
                                  mpBuffer.dwSize
                                 );
        break;

    case INTERNET_OPTION_PER_CONNECTION_OPTION:
        {
            if (!lpBuffer)
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }

            INTERNET_PER_CONN_OPTION_LISTA listA;
            LPINTERNET_PER_CONN_OPTION_LISTW plistW = (LPINTERNET_PER_CONN_OPTION_LISTW)lpBuffer;
            CHAR szEntryA[RAS_MaxEntryName + 1];
            listA.pszConnection = szEntryA;
            
            InitIPCOList(plistW, &listA);
            listA.pOptions = (LPINTERNET_PER_CONN_OPTIONA)_alloca(sizeof(INTERNET_PER_CONN_OPTIONA)*listA.dwOptionCount);
            
            for (DWORD i=0; i<listA.dwOptionCount; i++)
            {
                listA.pOptions[i].dwOption = plistW->pOptions[i].dwOption;

                switch (listA.pOptions[i].dwOption)
                {
                case INTERNET_PER_CONN_FLAGS:
                case INTERNET_PER_CONN_AUTODISCOVERY_FLAGS:
                case INTERNET_PER_CONN_AUTOCONFIG_RELOAD_DELAY_MINS:
                    listA.pOptions[i].Value.dwValue = plistW->pOptions[i].Value.dwValue;
                    break;

                case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_TIME:
                    listA.pOptions[i].Value.ftValue = plistW->pOptions[i].Value.ftValue;
                    break;
                    
                case INTERNET_PER_CONN_PROXY_SERVER:
                case INTERNET_PER_CONN_PROXY_BYPASS:
                case INTERNET_PER_CONN_AUTOCONFIG_URL:
                case INTERNET_PER_CONN_AUTOCONFIG_SECONDARY_URL:
                case INTERNET_PER_CONN_AUTOCONFIG_LAST_DETECT_URL:
                    if (plistW->pOptions[i].Value.pszValue && *plistW->pOptions[i].Value.pszValue)
                    {
                        // ** WARNING ** NO UTF8 ENCODING HERE
                        DWORD cb = WideCharToMultiByte(CP_ACP, 
                                        0, 
                                        plistW->pOptions[i].Value.pszValue,
                                        -1,
                                        0, 
                                        0,
                                        NULL,
                                        NULL);
                        listA.pOptions[i].Value.pszValue = (PSTR)_alloca(cb);
                        WideCharToMultiByte(CP_ACP, 
                                        0, 
                                        plistW->pOptions[i].Value.pszValue,
                                        -1,
                                        listA.pOptions[i].Value.pszValue, 
                                        cb,
                                        NULL,
                                        NULL);
                    }
                    else
                    {
                        listA.pOptions[i].Value.pszValue = NULL; 
                    }
                    break;
                    
                default:
                    dwErr = ERROR_INVALID_PARAMETER;
                    goto cleanup;
                    break;
                }
            }
            fResult = InternetSetOptionA(hInternet,
                              dwOption,
                              (PVOID)&listA,
                              dwBufferLength);
            plistW->dwOptionError = listA.dwOptionError;
        }
        break;

    default:
        fResult = InternetSetOptionA(hInternet,
                                  dwOption,
                                  lpBuffer,
                                  dwBufferLength
                                 );
    }

cleanup:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(INET, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
InternetSetOptionExA(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Sets a handle-specific variable, or a per-thread variable

Arguments:

    hInternet           - handle of object for which information will be set,
                          or NULL if the option defines a per-thread variable

    dwOption            - the handle-specific INTERNET_OPTION to set

    lpBuffer            - pointer to a buffer containing value to set

    dwBufferLength      - size of lpBuffer

    dwFlags             - flags controlling operation. Possible values are:

                            ISO_GLOBAL      - set this option globally. The
                                              shared Wininet data segment will
                                              be updated with this value

                            ISO_REGISTRY    - this value will be written to the
                                              registry for the corresponding
                                              entry

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info:
                    ERROR_INVALID_HANDLE
                        hInternet does not identify a valid Internet handle
                        object

                    ERROR_INTERNET_INTERNAL_ERROR
                        Shouldn't see this?

                    ERROR_INVALID_PARAMETER
                        One of the parameters was bad

                    ERROR_INTERNET_INVALID_OPTION
                        The requested option cannot be set

                    ERROR_INTERNET_OPTION_NOT_SETTABLE
                        Can't set this option, only query it

                    ERROR_INTERNET_BAD_OPTION_LENGTH
                        The dwBufferLength parameter is incorrect for the
                        expected type of the option

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetSetOptionExA",
                     "%#x, %s (%d), %#x [%#x], %d, %#x",
                     hInternet,
                     InternetMapOption(dwOption),
                     dwOption,
                     lpBuffer,
                     lpBuffer
                        ? (!IsBadReadPtr(lpBuffer, sizeof(DWORD))
                            ? *(LPDWORD)lpBuffer
                            : 0)
                        : 0,
                     dwBufferLength,
                     dwFlags
                     ));

    BOOL success;

    //
    // validate parameters
    //

    //
    // currently, dwFlags MBZ
    //

    if (dwFlags == 0) {
        success = InternetSetOptionA(hInternet,
                                     dwOption,
                                     lpBuffer,
                                     dwBufferLength
                                     );
    } else {
        DEBUG_ERROR(INET, ERROR_INVALID_PARAMETER);
        SetLastError(ERROR_INVALID_PARAMETER);
        success = FALSE;
    }

    DEBUG_LEAVE_API(success);
    return success;
}


INTERNETAPI
BOOL
WINAPI
InternetSetOptionExW(
    IN HINTERNET hInternet OPTIONAL,
    IN DWORD dwOption,
    IN LPVOID lpBuffer,
    IN DWORD dwBufferLength,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hInternet       -

    dwOption        -

    lpBuffer        -

    dwBufferLength  -

    dwFlags         -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetSetOptionExW",
                     "%#x, %s (%d), %#x [%#x], %d, %#x",
                     hInternet,
                     InternetMapOption(dwOption),
                     dwOption,
                     lpBuffer,
                     lpBuffer
                        ? (!IsBadReadPtr(lpBuffer, sizeof(DWORD))
                            ? *(LPDWORD)lpBuffer
                            : 0)
                        : 0,
                     dwBufferLength,
                     dwFlags
                     ));

    BOOL fResult = FALSE;

    if (dwFlags)
    {
        DEBUG_ERROR(INET, ERROR_INVALID_PARAMETER);
        SetLastError(ERROR_INVALID_PARAMETER);
    }
    else
    {
        fResult = InternetSetOptionW(hInternet,
                                     dwOption,
                                     lpBuffer,
                                     dwBufferLength
                                     );
    }

    DEBUG_LEAVE_API(fResult);
    return fResult;
}


PRIVATE
BOOL
FValidCacheHandleType(
    HINTERNET_HANDLE_TYPE   hType
    )
{
    return ((hType != TypeInternetHandle)   &&
            (hType != TypeFtpConnectHandle) &&
            (hType != TypeGopherConnectHandle) &&
            (hType != TypeFileRequestHandle) &&
            (hType != TypeHttpConnectHandle));
}

