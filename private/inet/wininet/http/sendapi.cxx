/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    sendapi.cxx

Abstract:

    This file contains the implementation of the HttpSendRequestA API.

    Contents:
        HttpSendRequestA
        HttpSendRequestW
        HttpSendRequestExA
        HttpSendRequestExW
        HttpEndRequestA
        HttpEndRequestW
        HttpWrapSendRequest

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

      29-Apr-97 rfirth
        Conversion to FSM

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "httpp.h"

//
// private prototypes
//

PRIVATE
BOOL
HttpWrapSendRequest(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength,
    IN DWORD dwOptionalLengthTotal,
    IN AR_TYPE arRequest
    );

//
// functions
//


INTERNETAPI
BOOL
WINAPI
HttpSendRequestA(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    )

/*++

Routine Description:

    Sends the specified request to the HTTP server.

Arguments:

    hRequest                - An open HTTP request handle returned by
                              HttpOpenRequest()

    lpszHeaders             - Additional headers to be appended to the request.
                              This may be NULL if there are no additional
                              headers to append

    dwHeadersLength         - The length (in characters) of the additional
                              headers. If this is -1L and lpszAdditional is
                              non-NULL, then lpszAdditional is assumed to be
                              zero terminated (ASCIIZ)

    lpOptionalData          - Any optional data to send immediately after the
                              request headers. This is typically used for POST
                              operations. This may be NULL if there is no
                              optional data to send

    dwOptionalDataLength    - The length (in BYTEs) of the optional data. This
                              may be zero if there is no optional data to send

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. For more information call GetLastError(). If the
                  request was async, then GetLastError() will return
                  ERROR_IO_PENDING which means that the operation initially
                  succeeded, and that the caller should wait for the status
                  callback to discover the final success/failure status

--*/

{
    DEBUG_ENTER_API((DBG_API,
                Bool,
                "HttpSendRequestA",
                "%#x, %.80q, %d, %#x, %d",
                hRequest,
                lpszHeaders,
                dwHeadersLength,
                lpOptional,
                dwOptionalLength
                ));


    BOOL fRet= HttpWrapSendRequest(
                hRequest,
                lpszHeaders,
                dwHeadersLength,
                lpOptional,
                dwOptionalLength,
                0,
                AR_HTTP_SEND_REQUEST
                );


    DEBUG_LEAVE_API(fRet);

    return fRet;
}


INTERNETAPI
BOOL
WINAPI
HttpSendRequestW(
    IN HINTERNET hRequest,
    IN LPCWSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength
    )

/*++

Routine Description:

    Sends the specified request to the HTTP server.

Arguments:

    hRequest                - An open HTTP request handle returned by
                              HttpOpenRequest()

    lpszHeaders             - Additional headers to be appended to the request.
                              This may be NULL if there are no additional
                              headers to append

    dwHeadersLength         - The length (in characters) of the additional
                              headers. If this is -1L and lpszAdditional is
                              non-NULL, then lpszAdditional is assumed to be
                              zero terminated (ASCIIZ)

    lpOptionalData          - Any optional data to send immediately after the
                              request headers. This is typically used for POST
                              operations. This may be NULL if there is no
                              optional data to send

    dwOptionalDataLength    - The length (in BYTEs) of the optional data. This
                              may be zero if there is no optional data to send

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. For more information call GetLastError(). If the
                  request was async, then GetLastError() will return
                  ERROR_IO_PENDING which means that the operation initially
                  succeeded, and that the caller should wait for the status
                  callback to discover the final success/failure status

--*/

{
    DEBUG_ENTER_API((DBG_API,
                Bool,
                "HttpSendRequestW",
                "%#x, %.80wq, %d, %#x, %d",
                hRequest,
                lpszHeaders,
                dwHeadersLength,
                lpOptional,
                dwOptionalLength
                ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpHeaders;

    if (!hRequest)
    {
        dwErr = ERROR_INVALID_HANDLE;
        goto cleanup;
    }
    if (lpszHeaders)
    {
        ALLOC_MB(lpszHeaders, dwHeadersLength, mpHeaders);
        if (!mpHeaders.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszHeaders, mpHeaders);
    }
    fResult = HttpWrapSendRequest(hRequest, mpHeaders.psStr, mpHeaders.dwSize,
                lpOptional, dwOptionalLength, 0, AR_HTTP_SEND_REQUEST);

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(HTTP, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
HttpSendRequestExA(
        IN HINTERNET hRequest,
        IN LPINTERNET_BUFFERSA lpBuffersIn OPTIONAL,
        OUT LPINTERNET_BUFFERSA lpBuffersOut OPTIONAL,
        IN DWORD dwFlags,
        IN DWORD_PTR dwContext
        )

/*++

Routine Description:

    description-of-function.

Arguments:

    hRequest        -
    lpBuffersIn     -
    lpBuffersOut    -
    dwFlags         -
    dwContext       -

Return Value:

    WINAPI

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "HttpSendRequestExA",
                     "%#x, %#x, %#x, %#x, %#x",
                     hRequest,
                     lpBuffersIn,
                     lpBuffersOut,
                     dwFlags,
                     dwContext
                     ));

    DWORD error = ERROR_SUCCESS;
    LPCSTR lpszHeaders = NULL;
    DWORD dwHeadersLength = 0;
    LPVOID lpOptional = NULL;
    DWORD dwOptionalLength = 0;
    DWORD dwOptionalLengthTotal = 0;
    BOOL fRet = FALSE;

    if ( lpBuffersOut )
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    if ( lpBuffersIn )
    {
        if ( IsBadReadPtr(lpBuffersIn, sizeof(INTERNET_BUFFERSA)) ||
             lpBuffersIn->dwStructSize != sizeof(INTERNET_BUFFERSA) )
        {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }

        lpszHeaders           = lpBuffersIn->lpcszHeader;
        dwHeadersLength       = lpBuffersIn->dwHeadersLength;
        lpOptional            = lpBuffersIn->lpvBuffer;
        dwOptionalLength      = lpBuffersIn->dwBufferLength;
                dwOptionalLengthTotal = lpBuffersIn->dwBufferTotal;
    }

    fRet= HttpWrapSendRequest(
                hRequest,
                lpszHeaders,
                dwHeadersLength,
                lpOptional,
                dwOptionalLength,
                dwOptionalLengthTotal,
                AR_HTTP_BEGIN_SEND_REQUEST
                );

quit:

    if ( error != ERROR_SUCCESS )
    {
        SetLastError(error);
        DEBUG_ERROR(HTTP, error);
        fRet = FALSE;
    }

    DEBUG_LEAVE_API(fRet);

    return fRet;
}


INTERNETAPI
BOOL
WINAPI
HttpSendRequestExW(
    IN HINTERNET hRequest,
    IN LPINTERNET_BUFFERSW lpBuffersIn OPTIONAL,
    OUT LPINTERNET_BUFFERSW lpBuffersOut OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hRequest        -
    lpBuffersIn     -
    lpBuffersOut    -
    dwFlags         -
    dwContext       -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "HttpSendRequestExW",
                     "%#x, %#x, %#x, %#x, %#x",
                     hRequest,
                     lpBuffersIn,
                     lpBuffersOut,
                     dwFlags,
                     dwContext
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;
    MEMORYPACKET mpHeaders;
    LPVOID pOptional = NULL;
    DWORD dwOptionalLength = 0;
    DWORD dwOptionalLengthTotal = 0;

    if (!hRequest)
    {
        dwErr = ERROR_INVALID_HANDLE;
    }
    else if (lpBuffersOut)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else 
    {
        if ( lpBuffersIn )
        {
            if (IsBadReadPtr(lpBuffersIn, sizeof(INTERNET_BUFFERS))
                ||
                (lpBuffersIn->dwStructSize != sizeof(INTERNET_BUFFERSW)))
            {
                dwErr = ERROR_INVALID_PARAMETER;
                goto cleanup;
            }

            if (lpBuffersIn->lpcszHeader)
            {
                ALLOC_MB(lpBuffersIn->lpcszHeader, lpBuffersIn->dwHeadersLength, mpHeaders);
                if (!mpHeaders.psStr)
                {
                    dwErr = ERROR_NOT_ENOUGH_MEMORY;
                    goto cleanup;
                }
                UNICODE_TO_ANSI(lpBuffersIn->lpcszHeader, mpHeaders);
            }
            pOptional            = lpBuffersIn->lpvBuffer;
            dwOptionalLength      = lpBuffersIn->dwBufferLength;
            dwOptionalLengthTotal = lpBuffersIn->dwBufferTotal;
        }

        fResult = HttpWrapSendRequest(
                hRequest,
                mpHeaders.psStr,
                mpHeaders.dwSize,
                pOptional,
                dwOptionalLength,
                dwOptionalLengthTotal,
                AR_HTTP_BEGIN_SEND_REQUEST
                );
    }

cleanup: 
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(HTTP, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
BOOL
WINAPI
HttpEndRequestA(
    IN HINTERNET hRequest,
    OUT LPINTERNET_BUFFERSA lpBuffersOut OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hRequest        -
    lpBuffersOut    -
    dwFlags         -
    dwContext       -

Return Value:

    WINAPI

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "HttpEndRequestA",
                     "%#x, %#x, %#x, %#x",
                     hRequest,
                     lpBuffersOut,
                     dwFlags,
                     dwContext
                     ));

    DWORD error = ERROR_SUCCESS;
    IN LPCSTR lpszHeaders = NULL;
    IN DWORD dwHeadersLength = 0;
    IN LPVOID lpOptional = NULL;
    IN DWORD dwOptionalLength = 0;
    BOOL fRet = FALSE;

    if ( lpBuffersOut )
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }


    fRet= HttpWrapSendRequest(
                hRequest,
                NULL,
                0,
                NULL,
                0,
                0,
                AR_HTTP_END_SEND_REQUEST
                );

quit:

    if ( error != ERROR_SUCCESS )
    {
        SetLastError(error);
        DEBUG_ERROR(HTTP, error);
        fRet = FALSE;
    }

    DEBUG_LEAVE_API(fRet);

    return fRet;
}


INTERNETAPI
BOOL
WINAPI
HttpEndRequestW(
    IN HINTERNET hRequest,
    OUT LPINTERNET_BUFFERSW lpBuffersOut OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    hRequest        -
    lpBuffersOut    -
    dwFlags         -
    dwContext       -

Return Value:

    BOOL

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "HttpEndRequestW",
                     "%#x, %#x, %#x, %#x",
                     hRequest,
                     lpBuffersOut,
                     dwFlags,
                     dwContext
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    BOOL fResult = FALSE;

    if (!hRequest)
    {
        dwErr = ERROR_INVALID_HANDLE;
    }
    else if (lpBuffersOut)
    {
        dwErr = ERROR_INVALID_PARAMETER;
    }
    else
    {
        fResult = HttpWrapSendRequest(hRequest, NULL, 0, NULL, 0, 0, AR_HTTP_END_SEND_REQUEST);
    }
    
    if (dwErr!=ERROR_SUCCESS) 
    { 
        SetLastError(dwErr); 
        DEBUG_ERROR(HTTP, dwErr);
    }
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


PRIVATE
BOOL
HttpWrapSendRequest(
    IN HINTERNET hRequest,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID lpOptional OPTIONAL,
    IN DWORD dwOptionalLength,
    IN DWORD dwOptionalLengthTotal,
    IN AR_TYPE arRequest
    )

/*++

Routine Description:

    Sends the specified request to the HTTP server.

Arguments:

    hRequest                - An open HTTP request handle returned by
                              HttpOpenRequest()

    lpszHeaders             - Additional headers to be appended to the request.
                              This may be NULL if there are no additional
                              headers to append

    dwHeadersLength         - The length (in characters) of the additional
                              headers. If this is -1L and lpszAdditional is
                              non-NULL, then lpszAdditional is assumed to be
                              zero terminated (ASCIIZ)

    lpOptionalData          - Any optional data to send immediately after the
                              request headers. This is typically used for POST
                              operations. This may be NULL if there is no
                              optional data to send

    dwOptionalDataLength    - The length (in BYTEs) of the optional data. This
                              may be zero if there is no optional data to send

    dwOptionalLengthTotal   - Total length need to be sent for File Upload.

    arRequest               - Which API the caller is making,
                                assumed to be HttpEndRequestA, HttpSendRequestExA, or
                                HttpSendRequestA

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. For more information call GetLastError(). If the
                  request was async, then GetLastError() will return
                  ERROR_IO_PENDING which means that the operation initially
                  succeeded, and that the caller should wait for the status
                  callback to discover the final success/failure status

Comments:

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Bool,
                 "HttpWrapSendRequest",
                 "%#x, %.80q, %d, %#x, %d, %d",
                 hRequest,
                 lpszHeaders,
                 dwHeadersLength,
                 lpOptional,
                 dwOptionalLength,
                 dwOptionalLengthTotal
                 ));

    PERF_ENTER(HttpWrapSendRequest);

    DWORD error = ERROR_SUCCESS;
    HINTERNET hRequestMapped = NULL;
    BOOL bDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // we will need the thread info for several items
    //

    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    //
    // the only FSMs that can come before this one are InternetOpenUrl() or
    // HttpSendRequest() when we are performing nested send for https://
    // tunnelling through proxy
    //

    INET_ASSERT((lpThreadInfo->Fsm == NULL)
                || (lpThreadInfo->Fsm->GetType() == FSM_TYPE_PARSE_HTTP_URL)
                || (lpThreadInfo->Fsm->GetType() == FSM_TYPE_OPEN_PROXY_TUNNEL)
                );

    INET_ASSERT( arRequest == AR_HTTP_SEND_REQUEST ||
                 arRequest == AR_HTTP_BEGIN_SEND_REQUEST ||
                 arRequest == AR_HTTP_END_SEND_REQUEST );


    //
    // map the handle
    //
    error = MapHandleToAddress(hRequest, (LPVOID *)&hRequestMapped, FALSE);


    if ((error != ERROR_SUCCESS) && (hRequestMapped == NULL)) {
        goto quit;
    }

    //
    // Cast it to the object that we know. We are going to do caching
    // semantics with this
    //

    HTTP_REQUEST_HANDLE_OBJECT * pRequest;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hRequestMapped;

    //
    // set the context and handle info & reset the error variables
    //


    _InternetSetContext(lpThreadInfo,
                        ((INTERNET_HANDLE_OBJECT *)hRequestMapped)->GetContext()
                        );
    _InternetSetObjectHandle(lpThreadInfo, hRequest, hRequestMapped);
    _InternetClearLastError(lpThreadInfo);

    //
    // quit now if the handle was invalid
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // use RIsHandleLocal() to discover 4 things:
    //
    //  1. Handle is valid
    //  2. Handle is of expected type (HTTP Request in this case)
    //  3. Handle is local or remote
    //  4. Handle supports async I/O
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hRequestMapped,
                           &isLocal,
                           &isAsync,
                           TypeHttpRequestHandle
                           );

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // For SEND_REQUEST, and BEGIN_SEND_REQUEST, we need
    //  to do some basic initalization
    //

    if ( arRequest == AR_HTTP_SEND_REQUEST ||
         arRequest == AR_HTTP_BEGIN_SEND_REQUEST)
    {
        BOOL fGoneOffline = FALSE;


        error = pRequest->InitBeginSendRequest(lpszHeaders,
                                       dwHeadersLength,
                                       &lpOptional,
                                       &dwOptionalLength,
                                       dwOptionalLengthTotal,
                                       &fGoneOffline
                                       );

        if ( error != ERROR_SUCCESS || fGoneOffline )
        {
            if ( error == ERROR_INTERNET_CACHE_SUCCESS )
            {
                error = ERROR_SUCCESS;
            }

            goto quit;
        }
    }


    //
    // send the request to the server. This may involve redirections and user
    // authentication
    //

    //error = DoFsm(new CFsm_HttpSendRequest(lpOptional, dwOptionalLength, pRequest, arRequest));
    //if (error == ERROR_IO_PENDING) {
    //    bDeref = FALSE;
    //}
    CFsm_HttpSendRequest * pFsm;

    pFsm = new CFsm_HttpSendRequest(lpOptional, dwOptionalLength, pRequest, arRequest);

    if (pFsm != NULL) {
        if (isAsync && !lpThreadInfo->IsAsyncWorkerThread) {
            error = pFsm->QueueWorkItem();
        } else {
            error = DoFsm(pFsm);
        }
        if (error == ERROR_IO_PENDING) {
            bDeref = FALSE;
        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

quit:

    //
    // if we went async don't deref the handle
    //

    if (bDeref && (hRequestMapped != NULL)) {
        DereferenceObject((LPVOID)hRequestMapped);
    }

done:

    BOOL success = TRUE;

    // SetLastError must be called after PERF_LEAVE !!!
    PERF_LEAVE(HttpWrapSendRequest);

    if (error != ERROR_SUCCESS) {

        SetLastError(error);
        DEBUG_ERROR(HTTP, error);
        success = FALSE;
    }

    DEBUG_LEAVE(success);
    return success;
}
