/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    send.cxx

Abstract:

    This file contains the implementation of the HttpSendRequestA API.

    Contents:
        HTTP_REQUEST_HANDLE_OBJECT::InitBeginSendRequest
        HTTP_REQUEST_HANDLE_OBJECT::CheckClientRequestHeaders
        CFsm_HttpSendRequest::RunSM
        HTTP_REQUEST_HANDLE_OBJECT::HttpSendRequest_Start
        HTTP_REQUEST_HANDLE_OBJECT::HttpSendRequest_Finish
        HTTP_REQUEST_HANDLE_OBJECT::UpdateProxyInfo
        HTTP_REQUEST_HANDLE_OBJECT::FindConnCloseRequestHeader

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

      29-Apr-97 rfirth
        Conversion to FSM

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "httpp.h"
#include "autodial.h"

//
// HTTP Request Handle Object methods
//


DWORD
HTTP_REQUEST_HANDLE_OBJECT::InitBeginSendRequest(
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN LPVOID *lplpOptional,
    IN LPDWORD lpdwOptionalLength,
    IN DWORD dwOptionalLengthTotal,
    OUT LPBOOL pfGoneOffline
    )

/*++

Routine Description:

    Performs Initiatization of the HTTP Request by setting up the necessary
     headers and preparing the POST data.

Arguments:

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

    dwOptionalLengthTotal   - Total Length for File Upload.

    pfGoneOffline           - Pointer to BOOL that is set to TRUE if durning the course
                            of this call, the connection was forced offline

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - One of the Win32 Error values.

  Comments:

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::InitBeginSendRequest",
                 "%#x, %d, %d, %#x",
                 lplpOptional ? *lplpOptional : NULL,
                 lpdwOptionalLength ? *lpdwOptionalLength : NULL,
                 dwOptionalLengthTotal,
                 pfGoneOffline
                 ));

    DWORD error = ERROR_SUCCESS;
    LPVOID lpOptional       = *lplpOptional;
    DWORD dwOptionalLength  = *lpdwOptionalLength;

    INET_ASSERT(pfGoneOffline);

    *pfGoneOffline = FALSE;

    //
    // validate parameters
    //

    if ((lpOptional == NULL) || (dwOptionalLength == 0)) {
        lpOptional = NULL;
        dwOptionalLength = 0;
    }

    //
    // the headers lengths can be -1 meaning that we should calculate the
    // string lengths. We must do this before calling MakeAsyncRequest()
    // which is expecting the parameters to be correct
    //

    __try {
        if (dwHeadersLength == -1) {
            dwHeadersLength = lstrlen((LPCSTR)lpszHeaders);
        } else {

            //
            // probe headers
            //

            for (DWORD i = 0; i < dwHeadersLength; ++i) {

                volatile char ch = lpszHeaders[i];

            }
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        error = ERROR_INVALID_PARAMETER;
    }
    ENDEXCEPT
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // if the caller specified some additional headers, then add them before
    // we make the request asynchronously
    //

    if (ARGUMENT_PRESENT(lpszHeaders) && (*lpszHeaders != '\0')) {

        //
        // we use the API here because the headers came from the app, and
        // we don't trust it
        //

        if (!HttpAddRequestHeaders(GetPseudoHandle(),
                                   lpszHeaders,
                                   dwHeadersLength,

                                   //
                                   // if the object is being re-used then
                                   // replace the headers to avoid
                                   // duplicating original headers
                                   //

                                   IS_VALID_HTTP_STATE(this, REUSE, TRUE)
                                    ? ( HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD ): 0
                                    //? HTTP_ADDREQ_FLAG_REPLACE : 0
                                   )) {
            error = GetLastError();
            goto quit;
        }
    }

    if (lpOptional && !GetSecondaryCacheKey()) {

        //
        // IE3 compat: set expiration on cache entry, if any, for this URL.
        //

        ExpireUrl();
    }

    //
    // if we are in offline mode or we have transitioned to offline mode
    // AND the caller didn't request RELOAD then allow only non-data GET
    // requests
    //

    BOOL bDisconnected;

    bDisconnected = IsOffline();

    BOOL bOfflineMode;

    bOfflineMode = (((GetInternetOpenFlags() | GetCacheFlags())
                        & INTERNET_FLAG_OFFLINE)
                    || (bDisconnected));

    if( bOfflineMode && _fIgnoreOffline )
        bOfflineMode = FALSE;

    if (  bOfflineMode  )
    {
        *pfGoneOffline = TRUE;

        if ( !GetSecondaryCacheKey()
        && (   (lpOptional != NULL)
            || (GetMethodType() != HTTP_METHOD_TYPE_GET))) {
            error = ERROR_FILE_NOT_FOUND;
            goto quit;
        }
    }

    CheckClientRequestHeaders();

    //
    // if there's no data to send then if we're offline then we attempt to
    // retrieve the file from cache
    //

    if (lpOptional == NULL || GetSecondaryCacheKey()) {
        if (bOfflineMode) {
            error = FHttpBeginCacheRetrieval(FALSE, TRUE);
            if (error == ERROR_SUCCESS) {
                error = ERROR_INTERNET_CACHE_SUCCESS;
                goto quit;
            }
            else{
                goto quit;
            }
        } else {
            error = GetFromCachePreNetIO();

            //
            // if we succeeded in getting the file from the cache or we're
            // disconnected then we're done
            //

            if ((error == ERROR_SUCCESS) || bDisconnected)
            {
                if ( error == ERROR_SUCCESS )
                {
                    error = ERROR_INTERNET_CACHE_SUCCESS;
                }
                goto quit;
            }
            else if (error == ERROR_INTERNET_INSERT_CDROM)
            {
                // Check if it's ok to return this error. We do this if the
                // client has indicated it allows ERROR_INTERNET_INSERT_CDROM
                // via InternetSetOption and an insert cdrom dialog is not active.
                if (GetErrorMask() & INTERNET_ERROR_MASK_INSERT_CDROM)
                {
                    if (!(BOOL) InterlockedExchange((LONG*) &fCdromDialogActive, (LONG) TRUE))
                        goto quit;
                }
            }
        }
    }

    //
    // If we fall through then we are connected and a) either the thing
    // is not in the cache or we did a conditional get or c) there was
    // some cache error
    //

    error = ERROR_SUCCESS;

    //
    // if the app supplied a user-agent string to InternetOpen() AND hasn't
    // added a "User-Agent:" header, then add it
    //

    LPSTR userAgent;
    DWORD userAgentLength;

    userAgent = GetUserAgent(&userAgentLength);
    if (userAgent != NULL) {
        ReplaceRequestHeader(HTTP_QUERY_USER_AGENT,
                             userAgent,
                             userAgentLength,
                             0, // dwIndex,
                             ADD_HEADER_IF_NEW
                             );
    }

    //
    // do the same thing with the "Host:" header. The header-value is the host
    // name supplied to InternetConnect() (or the name of the redirected host)
    //

    LPSTR hostName;
    DWORD hostNameLength;
    INTERNET_PORT hostPort;

    hostName = GetHostName(&hostNameLength);
    hostPort = GetHostPort();

    INET_ASSERT((hostName != NULL) && (hostNameLength > 0));

    char hostValue[INTERNET_MAX_HOST_NAME_LENGTH + sizeof(":4294967295")];

    if ((hostPort != INTERNET_DEFAULT_HTTP_PORT)
    && (hostPort != INTERNET_DEFAULT_HTTPS_PORT)) {
        if (lstrlen(hostName) > INTERNET_MAX_HOST_NAME_LENGTH)
        {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }
        hostNameLength = wsprintf(hostValue, "%s:%d", hostName, (hostPort & 0xffff));
        hostName = hostValue;
    }
    ReplaceRequestHeader(HTTP_QUERY_HOST,
                         hostName,
                         hostNameLength,
                         0, // dwIndex,
                         ADD_HEADER_IF_NEW
                         );

    //
    // if the app requested keep-alive then add the header; if we're going via
    // proxy then use the proxy-connection header
    //

    //if (pRequest->GetOpenFlags() & INTERNET_FLAG_KEEP_CONNECTION) {
    //    pRequest->SetWantKeepAlive(TRUE);
    //}

    //
    // add the content-length header IF we are sending data OR this is a POST,
    // AND ONLY if the app has not already added the header
    //

    if (dwOptionalLength || dwOptionalLengthTotal)
        SetMethodBody();
        
    if (((dwOptionalLength != 0) || (dwOptionalLengthTotal != 0))

    //
    // BUGBUG - just comparing against a method type is insufficient. We need
    //          a test of whether the method implies sending data (PUT, etc).
    //          We make the same test in other places
    //

    || (GetMethodType() != HTTP_METHOD_TYPE_GET)) {

        DWORD dwContentLength;

        char number[sizeof("4294967295")];

        //
        // For File Upload we need to add the Content-Length
        //   header off of the Total Length, Not the current
        //   data size.  Since we get more data via InternetWriteFile
        //

        if ( dwOptionalLengthTotal != 0 )
        {
            dwContentLength = dwOptionalLengthTotal;
        }
        else
        {
            dwContentLength = dwOptionalLength;
        }

        // _itoa(dwOptionalLength, number, 10);
        wsprintf(number, "%d", dwContentLength);

        DWORD numberLength = lstrlen(number);

        /*----------------------------------------------------------------------

        #62953 NOTE --  Authstate can never be in the AUTHSTATE_NEGOTIATE
        state here. It is not necessary to zero out the content length
        header here when omitting post data on NTLM negotiate since this
        will be done later in the request. The commented-out code is not
        necessary.

        if ((GetMethodType() == HTTP_METHOD_TYPE_POST)
            && (GetAuthState() == AUTHSTATE_NEGOTIATE))
        {

            ReplaceRequestHeader(HTTP_QUERY_CONTENT_LENGTH,
                                "0",
                                1,
                                0,   // dwIndex
                                ADD_HEADER
                                );
        }

        ---------------------------------------------------------------------*/

        // Normally we don't over-write the content-length
        // header if one already exists.
        DWORD dwAddHeader;
        dwAddHeader = ADD_HEADER_IF_NEW;

        // But if we're posting data and have an auth ctx
        // over-write the content-length header which will
        // have been reset to 0 to omit post data on the
        // negotiate phase.
        AUTHCTX *pAuthCtx;
        pAuthCtx = GetAuthCtx();
        if (pAuthCtx)
        {
            dwAddHeader = ADD_HEADER;
        }


        ReplaceRequestHeader(HTTP_QUERY_CONTENT_LENGTH,
                            (LPSTR)number,
                            numberLength,
                            0,   // dwIndex
                             dwAddHeader
                            );

    }

quit:

    *lplpOptional       = lpOptional;
    *lpdwOptionalLength = dwOptionalLength;

    DEBUG_LEAVE(error);

    return error;
}


void
HTTP_REQUEST_HANDLE_OBJECT::CheckClientRequestHeaders(
    VOID
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    None.

Return Value:

    None.

--*/

{
    if (IsFirstSendProcessed())
    {
        return; // been there, done that
    }

    SetFirstSendProcessed();

    //
    // record the initial URL for offline mapping in case of redirects
    //

    SetOriginalUrl(GetCacheKey());


    //
    // Disabling cache if OFFLINE flag not set and RELOAD flag set.
    //

    if (    !((GetInternetOpenFlags() | GetCacheFlags()) & INTERNET_FLAG_OFFLINE)
        &&  GetCacheFlags() & INTERNET_FLAG_RELOAD
       )
    {
        SetCacheReadDisabled();
        return;
    }

    //
    // See if the client added any headers that imply caching disabled.
    //

    _RequestHeaders.LockHeaders();
   DWORD Headers[] =
    {
        HTTP_QUERY_IF_MODIFIED_SINCE,
        HTTP_QUERY_IF_NONE_MATCH,
        HTTP_QUERY_RANGE
    };

    DWORD i;
    for (i=0; i<ARRAY_ELEMENTS(Headers); i++)
    {
        if (IsRequestHeaderPresent(Headers[i]))
        {
            SetCacheReadDisabled();
            break;
        }
    }

    _RequestHeaders.UnlockHeaders();
}


DWORD
CFsm_HttpSendRequest::RunSM(
    IN CFsm * Fsm
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    Fsm -

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_HttpSendRequest::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;

    START_SENDREQ_PERF();

    CFsm_HttpSendRequest * stateMachine = (CFsm_HttpSendRequest *)Fsm;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();
    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
        //
        // Make sure appropriate connection and proxy is in place
        //
        InternetAutodialIfNotLocalHost(pRequest->GetURL(), NULL);

        //
        // fall through to FSM_STATE_CONTINUE
        //

    case FSM_STATE_CONTINUE:

        //CHECK_FSM_OWNED(Fsm);

        error = pRequest->HttpSendRequest_Start(stateMachine);
        break;

    case FSM_STATE_FINISH:

        //CHECK_FSM_OWNED(Fsm);

        error = pRequest->HttpSendRequest_Finish(stateMachine);
        break;

    case FSM_STATE_ERROR:

        //CHECK_FSM_OWNED(Fsm);

        error = Fsm->GetError();

        //
        // If we block to call GetProxyInfo async, then
        //  we may get unblocked during a cancel.  We need to
        //   handle it by freeing the object in our destructor.
        //

        INET_ASSERT( (!stateMachine->m_fOwnsProxyInfoQueryObj) ?
                        ( error == ERROR_INTERNET_OPERATION_CANCELLED ||
                          error == ERROR_INTERNET_TIMEOUT )  :
                        TRUE );

        Fsm->SetDone();
        break;

    default:

        //CHECK_FSM_OWNED(Fsm);

        stateMachine->m_fOwnsProxyInfoQueryObj = TRUE;
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    STOP_SENDREQ_PERF();

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::HttpSendRequest_Start(
    IN CFsm_HttpSendRequest * Fsm
    )

/*++

Routine Description:

    Calls SendData() method in a loop, handling redirects (& authentications?)
    until we have successfully started to retrieve what was originally requested

Arguments:

    Fsm - HTTP send request FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    Operation completed successfully

                  ERROR_IO_PENDING
                    Operation will complete asynchronously

        Failure - ERROR_INTERNET_INCORRECT_HANDLE_STATE
                    The HTTP request handle is in the wrong state for this
                    request

                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of resources

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::HttpSendRequest_Start",
                 "%#x",
                 Fsm
                 ));

    PERF_ENTER(HttpSendRequest_Start);

    //CHECK_FSM_OWNED(Fsm);

    CFsm_HttpSendRequest & fsm = *Fsm;

    //
    // we must loop here while the server redirects us or while we authenticate
    // the user
    //

    FSM_STATE state = fsm.GetState();
    DWORD error = fsm.GetError();

    //
    // We set m_fOwnsProxyInfoObj TRUE, because by virtue of being here
    //   we have know that we now own the pointer in our fsm, pointed to by
    //   fsm.m_pProxyInfoQuery.
    //
    // This boolean is used to know when we are allowed to FREE and ACCESS
    //   this pointer.  If FALSE, we CANNOT touch this pointer because
    //   the auto-proxy thread may be accessing it.   The auto-proxy thread
    //   will unblock us, this releasing its "un-offical" lock on this pointer.
    //

    fsm.m_fOwnsProxyInfoQueryObj = TRUE;

    if (state == FSM_STATE_INIT)
    {        
        if ( fsm.m_arRequest == AR_HTTP_END_SEND_REQUEST )
        {
            state = FSM_STATE_4;
            fsm.SetFunctionState(FSM_STATE_4);
        }
    }
    else
    {
        state = fsm.GetFunctionState();
    }

retry_send_request:

    do {
        switch (state) {
        case FSM_STATE_INIT:
        case FSM_STATE_1:

            //CHECK_FSM_OWNED(Fsm);

            fsm.m_iRetries = 2;
            fsm.m_bAuthNotFinished = FALSE;
            fsm.m_dwCookieIndex = 0;

            //
            // Terrible bug that afflicts NS servers while doing SSL,
            //  they lie (those buggers), and claim they do keep-alive,
            //  but when we attempt to reuse their Keep-Alive sockets,
            //  they all fail, so we therefore must increase the retry count
            //  so we can empty all the bad keep-alive sockets out of the pool
            //
              
            if ( (GetOpenFlags() & INTERNET_FLAG_SECURE) )
            {
                CServerInfo * pServerInfo = GetServerInfo();
    
                if ( pServerInfo && pServerInfo->IsBadNSServer() )
                {
                    fsm.m_iRetries = 5;
                }
            }

            //
            // if we're not in the right state to send, drain the socket
            //

            if (!IsValidHttpState(SEND)) {

#define DRAIN_SOCKET_BUFFER_LENGTH  (1 K)

                if (fsm.m_pBuffer == NULL) {
                    fsm.m_pBuffer = (LPVOID)ALLOCATE_MEMORY(
                                        LMEM_FIXED,
                                        DRAIN_SOCKET_BUFFER_LENGTH);
                    if (fsm.m_pBuffer == NULL) {
                        error = ERROR_NOT_ENOUGH_MEMORY;
                        goto quit;
                    }
                }
                do {
                    if (!fsm.m_bSink) {
                        fsm.m_bSink = TRUE;
                        fsm.SetFunctionState(FSM_STATE_2);
                        error = ReadData(fsm.m_pBuffer,
                                         DRAIN_SOCKET_BUFFER_LENGTH,
                                         &fsm.m_dwBytesDrained,
                                         TRUE,
                                         0);
                        if (error == ERROR_IO_PENDING) {
                            goto quit;
                        }
                    }

                    //
                    // fall through to state 2
                    //

        case FSM_STATE_2:

                    fsm.m_bSink = FALSE;
                } while ((error == ERROR_SUCCESS) && (fsm.m_dwBytesDrained != 0));
                if (error != ERROR_SUCCESS) {
                    goto quit;
                }
                if (fsm.m_pBuffer != NULL) {
                    fsm.m_pBuffer = (LPVOID)FREE_MEMORY(fsm.m_pBuffer);

                    INET_ASSERT(fsm.m_pBuffer == NULL);

                }

                ReuseObject();

                INET_ASSERT(!IsData());
                INET_ASSERT(IS_VALID_HTTP_STATE(this, SEND, TRUE));

                //
                // BUGBUG - if we're not in the right state?
                //

            }

            //
            // generate the correct request headers based
            //  on what types, or whether we're using
            //  proxies
            //


            fsm.SetFunctionState(FSM_STATE_3);
            error = UpdateProxyInfo(Fsm, FALSE);

            if (error == ERROR_IO_PENDING) {
                goto done;
            }

            //
            // set function state to not-FSM_STATE_3 to differentiate interrupted
            // path in FSM_STATE_3
            //

            fsm.SetFunctionState(FSM_STATE_BAD);

            //
            // fall through
            //

        case FSM_STATE_3:

            if ((error == ERROR_SUCCESS)
            && (fsm.GetFunctionState() == FSM_STATE_3)) {
                error = UpdateProxyInfo(Fsm, TRUE);
            }

            if (error != ERROR_SUCCESS) {
                fsm.m_bCancelRedoOfProxy = TRUE;
                goto quit;
            }

            //
            // get any cookies required for this site, but only if app didn't
            // tell us it will handle cookies
            //

            if (!(GetOpenFlags() & INTERNET_FLAG_NO_COOKIES) ) {                                
                fsm.SetBlocking(TRUE);
                if (CreateCookieHeaderIfNeeded()) {
                    SetPerUserItem(TRUE);
                }
                fsm.SetBlocking(FALSE);
            }

            //
            // if this URL requires authentication then add its header here, but
            // only if the app didn't tell us not to
            //

            if (!(GetOpenFlags() & INTERNET_FLAG_NO_AUTH)) {
                error = AuthOnRequest(this);
                if (error != ERROR_SUCCESS) {
                    goto quit;
                }
                error = HttpFiltOnRequest(this);
            }
try_again:
            fsm.SetFunctionState(FSM_STATE_4);
            DEBUG_PRINT(HTTP, INFO, ("State_3_Start: lpOptional = 0x%x deOptionalLength = %d\n", fsm.m_lpOptional, fsm.m_dwOptionalLength));
            error = DoFsm(new CFsm_SendRequest(fsm.m_lpOptional,
                                               fsm.m_dwOptionalLength,
                                               this
                                               ));
            if (error == ERROR_IO_PENDING) {
                goto quit;
            }

            //
            // fall through
            //

        case FSM_STATE_4:
            DEBUG_PRINT(HTTP, INFO, ("State_4_start: lpOptional = 0x%x deOptionalLength = %d\n", fsm.m_lpOptional, fsm.m_dwOptionalLength));

            if (error == ERROR_INTERNET_OFFLINE) {
                goto offline_check;
            } else if (((GetOpenFlags() & INTERNET_FLAG_CACHE_IF_NET_FAIL)
                        && ((error == ERROR_INTERNET_CANNOT_CONNECT)
                            || (error == ERROR_INTERNET_CONNECTION_RESET)))
                       || ((error == ERROR_INTERNET_TIMEOUT)
                           && IsFromCacheTimeoutSet()
                           && CanRetrieveFromCache(FALSE))) {

                //CHECK_FSM_OWNED(Fsm);

//if (IsFromCacheTimeoutSet() && IsInCache()) {
//    dprintf("%q timed out on connect/send - getting from cache\n", GetURL());
//}
                error = GetFromCachePostNetIO(HTTP_STATUS_NOT_MODIFIED, TRUE);
                if (error == ERROR_SUCCESS) {
                    SetNetFailed();
                }
                goto quit;
            }

            //CHECK_FSM_OWNED(Fsm);

            //
            // This adds CR-LF for the File Upload case
            //

            //if (_RequestMethod == HTTP_METHOD_TYPE_POST && _AddCRLFToPOST && fsm.m_arRequest == AR_HTTP_END_SEND_REQUEST)
            //{
            //    error = _Socket->Send(gszCRLF, 2, 0);
            //    if (error != ERROR_SUCCESS) {
            //        goto quit;
            //    }
            //}
            fsm.m_bWasKeepAlive = (_bKeepAliveConnection || IsKeepAlive());
            if ((error != ERROR_SUCCESS)
            || ((GetStatusCode() != HTTP_STATUS_OK) && (GetStatusCode() != 0))) {

                //
                // must be doing proxy tunnelling request if status code set
                //

                INET_ASSERT(((GetStatusCode() != HTTP_STATUS_OK)
                            && (GetStatusCode() != 0))
                            ? IsTalkingToSecureServerViaProxy()
                            : TRUE
                            );

                //
                // server may have reset keep-alive connection
                //

                if (((error == ERROR_INTERNET_CONNECTION_RESET)
                || (error == ERROR_INTERNET_CONNECTION_ABORTED))
                && fsm.m_bWasKeepAlive
                && (--fsm.m_iRetries != 0)) {

                    DEBUG_PRINT(HTTP,
                                INFO,
                                ("keep-alive connection failed after send. Retrying\n"
                                ));

//dprintf("*** retrying k-a connection after send\n");
                    CloseConnection(TRUE);
                    goto try_again;
                }
                goto quit;
            }
            if (fsm.m_arRequest == AR_HTTP_BEGIN_SEND_REQUEST) {
                goto quit;
            }
            fsm.SetFunctionState(FSM_STATE_5);
            error = DoFsm(new CFsm_ReceiveResponse(this));
            if (error == ERROR_IO_PENDING) {
                goto quit;
            }

            //
            // fall through
            //

        case FSM_STATE_5:

            DEBUG_PRINT(HTTP, INFO, ("State_5_Start: lpOptional = 0x%x deOptionalLength = %d\n", fsm.m_lpOptional, fsm.m_dwOptionalLength));

            //CHECK_FSM_OWNED(Fsm);

            if (error != ERROR_SUCCESS) {
//dprintf("*** post-receive: error=%d, retries=%d\n", error, fsm.m_iRetries);

                //
                // server may have reset keep-alive connection
                //

                if (((error == ERROR_INTERNET_CONNECTION_RESET)
                || (error == ERROR_INTERNET_CONNECTION_ABORTED)
                || (error == ERROR_HTTP_INVALID_SERVER_RESPONSE))
                && fsm.m_bWasKeepAlive
                && (--fsm.m_iRetries != 0)) {

                    DEBUG_PRINT(HTTP,
                                INFO,
                                ("keep-alive connection failed after receive. Retrying\n"
                                ));

//dprintf("*** retrying k-a connection after receive\n");
                    CloseConnection(TRUE);
                    _ResponseHeaders.FreeHeaders();
                    ResetResponseVariables();
                    _ResponseHeaders.Initialize();
                    _dwCurrentStreamPosition = 0;
                    goto try_again;
                }

                //
                // if we reached this far and discover that the state is offline
                // then we have either transitioned to this state by losing the
                // net connection, or by being put into user-offline state. In
                // either case, we attempt to retrieve the item from the cache
                //

offline_check:
                if (IsOffline()) {
                    // if we already have it retrived, don't do it again
                    if (!_hCacheStream && !_pCacheEntryInfo )
                        error = FHttpBeginCacheRetrieval(TRUE, TRUE, FALSE);
                    else
                        error = FHttpBeginCacheRetrieval(TRUE, TRUE, TRUE);
        
                } else if (((GetOpenFlags() & INTERNET_FLAG_CACHE_IF_NET_FAIL)
                           && ((error == ERROR_INTERNET_CANNOT_CONNECT)
                               || (error == ERROR_INTERNET_CONNECTION_RESET)))
                           || ((error == ERROR_INTERNET_TIMEOUT)
                               && IsFromCacheTimeoutSet()
                               && CanRetrieveFromCache(FALSE))) {

//if (IsFromCacheTimeoutSet() && IsInCache()) {
//    dprintf("%q timed out on response - getting from cache\n", GetURL());
//}
                    error = GetFromCachePostNetIO(HTTP_STATUS_NOT_MODIFIED, TRUE);
                    if (error == ERROR_SUCCESS) {
                        SetNetFailed();
                    }
                }
                goto quit;
            }

            fsm.SetFunctionState(FSM_STATE_6);


        case FSM_STATE_6:
            DEBUG_PRINT(HTTP, INFO, ("State_6_Start: lpOptional = 0x%x deOptionalLength = %d\n", fsm.m_lpOptional, fsm.m_dwOptionalLength));

            //
            // put any received cookie headers in the cookie jar, but only if the
            // app didn't tell us not to
            //

            if (!(GetOpenFlags() & INTERNET_FLAG_NO_COOKIES)
            && IsResponseHeaderPresent(HTTP_QUERY_SET_COOKIE) )
            {
                DWORD dwError;                                    
                fsm.SetBlocking(TRUE);
                dwError = ExtractSetCookieHeaders(&fsm.m_dwCookieIndex);
                fsm.SetBlocking(FALSE);

                if ( dwError == ERROR_SUCCESS ) {
                    SetPerUserItem(TRUE);
                } else if ( dwError == ERROR_IO_PENDING ) {
                    error = ERROR_IO_PENDING;
                    goto quit;
                }
            }

            //
            // we need to handle various intermediary return codes:
            //
            //  30x - redirection
            //  40x - authentication
            //
            // BUT ONLY if the app didn't tell us it wanted to handle these itself
            //

            DWORD statusCode;
            BOOL bNoAuth;

            statusCode = GetStatusCode();
            bNoAuth = (GetOpenFlags() & INTERNET_FLAG_NO_AUTH) ? TRUE : FALSE;

            //
            // if the status is 200 (most frequently return header == success)
            // and we are not authenticating all responses then we're done
            //

            if ((statusCode == HTTP_STATUS_OK) && bNoAuth) {
                goto quit;
            }

            //
            // handle authentication before checking the cache
            //

            if (!bNoAuth) {

                //
                // call filter for cis rpa
                //

                if (!HttpFiltOnResponse(this)) {
                    switch (error = GetLastError()) {
                    case ERROR_INTERNET_FORCE_RETRY:

                        if ( fsm.m_arRequest == AR_HTTP_END_SEND_REQUEST )
                        {
                            goto quit;
                        }

                        fsm.m_bFinished = FALSE;
                        fsm.m_bAuthNotFinished = TRUE;
                        error = ERROR_SUCCESS;
                        break;

                    case ERROR_INTERNET_NEED_UI:
                        error = ERROR_SUCCESS;
                        break;
                    }
                }

                //
                // call packages for basic, ntlm, msn, dpa etc.
                //

                error = AuthOnResponse(this);
                if (error == ERROR_INTERNET_FORCE_RETRY) {

                    if ( fsm.m_arRequest == AR_HTTP_END_SEND_REQUEST )
                    {
                        goto quit;
                    }

                    //
                    // the object has been updated with new info - try again
                    //

                    fsm.m_bFinished = FALSE;
                    fsm.m_bAuthNotFinished = TRUE;
                    error = ERROR_SUCCESS;

                    //
                    // Reset auto-proxy info so we can retry the connection
                    //

                    if ( fsm.m_fOwnsProxyInfoQueryObj && fsm.m_pProxyInfoQuery && fsm.m_pProxyInfoQuery->IsAlloced())
                    {
                        delete fsm.m_pProxyInfoQuery;
                        fsm.m_pProxyInfoQuery = NULL;
                    }


                } else if (error == ERROR_INTERNET_INCORRECT_PASSWORD) {

                    //
                    // just return success to the app which will have to check the
                    // headers and make the request again, with the right password
                    //

                    error = ERROR_SUCCESS;
                    goto quit;
                } else if (error == ERROR_INTERNET_LOGIN_FAILURE_DISPLAY_ENTITY_BODY) {

                    // If app allows this error code return it. Otherwise return ERROR_INTERNET_LOGIN_FAILURE
                    if (!(GetErrorMask() & INTERNET_ERROR_MASK_LOGIN_FAILURE_DISPLAY_ENTITY_BODY))
                    {
                        error = ERROR_INTERNET_LOGIN_FAILURE;
                    }
                    else
                    {
                        goto quit;
                    }
                }
        
            }

            //
            // if we can read from the cache then let us try
            //

            if ((statusCode == HTTP_STATUS_OK)
                || (statusCode == HTTP_STATUS_NOT_MODIFIED)
                || (statusCode == HTTP_STATUS_PRECOND_FAILED)
                || (statusCode == HTTP_STATUS_PARTIAL_CONTENT)
                || (statusCode == 0)) {

                if (GetFromCachePostNetIO(statusCode) == ERROR_SUCCESS) {
                    error = ERROR_SUCCESS;
                    goto quit;
                }
            }

            //
            // handle redirection
            //

            BOOL fDoProxyRedirect;

            fsm.m_tMethodRedirect = HTTP_METHOD_TYPE_UNKNOWN;
            fDoProxyRedirect = FALSE;
            fsm.m_fNeedUserApproval = FALSE;

            fsm.m_bRedirected = FALSE;

            if (((statusCode == HTTP_STATUS_AMBIGUOUS)              // 300
                 || (statusCode == HTTP_STATUS_MOVED)               // 301
                 || (statusCode == HTTP_STATUS_REDIRECT)            // 302
                 || (statusCode == HTTP_STATUS_REDIRECT_METHOD)     // 303
//                 || (statusCode == HTTP_STATUS_USE_PROXY)           // 305
                 || (statusCode == HTTP_STATUS_REDIRECT_KEEP_VERB)) // 307
                && !(GetOpenFlags() & INTERNET_FLAG_NO_AUTO_REDIRECT)) {

                //
                // Clean out expired PROXY_STATE
                //

                error = ERROR_SUCCESS;
                if ( fsm.m_fOwnsProxyInfoQueryObj && fsm.m_pProxyInfoQuery && fsm.m_pProxyInfoQuery->IsAlloced())
                {
                    delete fsm.m_pProxyInfoQuery;
                    fsm.m_pProxyInfoQuery = NULL;
                }
                //fsm.m_pProxyState = NULL;
                SetProxyName(NULL, 0, 0);

                //
                // if we've already had the max allowable redirects then quit
                //

                if (fsm.m_dwRedirectCount == 0) {
                    error = ERROR_HTTP_REDIRECT_FAILED;
                    fsm.m_bRedirectCountedOut = TRUE;
                    goto quit;
                }

                //
                // we got 300 (ambiguous), 301 (permanent move), 302 (temporary
                // move), or 303 (redirection using new method)
                //

                switch (statusCode) {
                case HTTP_STATUS_AMBIGUOUS:

                    //
                    // 300 - multiple choice
                    //

                    //
                    // If there is a Location header, we do an "automatic" redirect
                    //

                    _ResponseHeaders.LockHeaders();

                    if (! IsResponseHeaderPresent(HTTP_QUERY_LOCATION)) {
                        _ResponseHeaders.UnlockHeaders();
                        fsm.m_bFinished = TRUE;
                        break;
                    }

                    _ResponseHeaders.UnlockHeaders();

                    //
                    // fall through
                    //

                case HTTP_STATUS_MOVED:

                    // Table View:
                    //Method            301             302             303             307
                    //  *               *               *           GET         *
                    //POST                  GET         GET             GET             POST
                    //
                    //Put another way:
                    //301 & 302  - All methods are redirected to the same method but POST. POST is
                    //  redirected to a GET.
                    //303 - All methods are redirected to GET
                    //307 - All methods are redirected to the same method.

                    //
                    // 301 - permanently moved
                    //

                    //
                    // fall through
                    //

                case HTTP_STATUS_REDIRECT:

                    //
                    // 302 - temporarily moved (POST => GET, everything stays the same)
                    //
                    fsm.m_tMethodRedirect = GetMethodType();
                    if (fsm.m_tMethodRedirect == HTTP_METHOD_TYPE_POST)
                    //
                    // A POST change method to a GET
                    //
                    {
                        fsm.m_tMethodRedirect = HTTP_METHOD_TYPE_GET;

                        // force no optional data on second and subsequent sends
                        fsm.m_dwOptionalLength = 0;

                        // Don't decorate URL for a GET.
                        FreeSecondaryCacheKey();
                    }

                    // Other than GET and HEAD methods, we always want to
                    // error out to the client with ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION
                    
                    if ((fsm.m_tMethodRedirect != HTTP_METHOD_TYPE_GET) &&
                        (fsm.m_tMethodRedirect != HTTP_METHOD_TYPE_HEAD))
                    {
                        fsm.m_fNeedUserApproval = TRUE;
                    }

                    INET_ASSERT(((fsm.m_tMethodRedirect == HTTP_METHOD_TYPE_GET)
                                 || (fsm.m_tMethodRedirect == HTTP_METHOD_TYPE_HEAD))
                                 ? (fsm.m_dwOptionalLength == 0) : TRUE);

                    fsm.m_bRedirected = TRUE;
                    --fsm.m_dwRedirectCount;

                    break;
                case HTTP_STATUS_REDIRECT_METHOD:

                    //
                    // 303 - see other (POST => GET)
                    //

                    fsm.m_tMethodRedirect = HTTP_METHOD_TYPE_GET;

                    //
                    // force no optional data on second and subsequent sends
                    //

                    fsm.m_dwOptionalLength = 0;

                    //
                    // Don't decorate URL for a GET.
                    //

                    FreeSecondaryCacheKey();

                    fsm.m_bRedirected = TRUE;
                    --fsm.m_dwRedirectCount;
                    break;

                case HTTP_STATUS_REDIRECT_KEEP_VERB:

                    //
                    // 307 - see other (POST => POST)
                    //

                    //if (IsHttp1_1()) {
                    fsm.m_tMethodRedirect = GetMethodType();

                    if ((fsm.m_tMethodRedirect != HTTP_METHOD_TYPE_GET) &&
                        (fsm.m_tMethodRedirect != HTTP_METHOD_TYPE_HEAD))
                    {
                        fsm.m_fNeedUserApproval = TRUE;
                    }


                    INET_ASSERT(((fsm.m_tMethodRedirect == HTTP_METHOD_TYPE_GET)
                                 || (fsm.m_tMethodRedirect == HTTP_METHOD_TYPE_HEAD))
                                 ? (fsm.m_dwOptionalLength == 0) : TRUE);

                    //} else {
                    //    fsm.m_dwOptionalLength = 0;
                    //    fsm.m_tMethodRedirect = HTTP_METHOD_TYPE_GET;
                    //}
                    fsm.m_bRedirected = TRUE;
                    --fsm.m_dwRedirectCount;
                    break;

#if 0 // The http-wg will probably decide that 305 is only for proxies.

                case HTTP_STATUS_USE_PROXY:

                    //
                    // 305 - use proxy, force a re-request but now use a the specifed proxy
                    //

                    if ( IsResponseHttp1_1() )
                    {
                        fsm.m_tMethodRedirect = GetMethodType();
                        fDoProxyRedirect = TRUE;

                        fsm.m_bRedirected = TRUE;
                        --fsm.m_dwRedirectCount;

                        break;
                    }

                    //
                    // fall through
                    //
#endif

                default:
                    fsm.m_tMethodRedirect = HTTP_METHOD_TYPE_GET;

                    //
                    // BUGBUG - force no optional data on second and subsequent
                    //          sends
                    //

                    fsm.m_dwOptionalLength = 0;
                    fsm.m_bRedirected = TRUE;
                    --fsm.m_dwRedirectCount;
                    break;
                }

                //
                // Only allow redirect to continue if we are successful.
                //

                if (fsm.m_bRedirected
                && ((fsm.m_tMethodRedirect != HTTP_METHOD_TYPE_UNKNOWN)
                    || (fsm.m_tMethodRedirect == GetMethodType()))) {
                    fsm.SetFunctionState(FSM_STATE_7);
                    // Remember if it was a forms submit.
                    fsm.m_bFormsSubmit = ((GetOpenFlags() & INTERNET_FLAG_FORMS_SUBMIT) ||
                                          (fsm.m_tMethodRedirect == HTTP_METHOD_TYPE_POST));
                    error = Redirect(fsm.m_tMethodRedirect, fDoProxyRedirect);
                    if (error != ERROR_SUCCESS) {
                        goto quit;
                    }
                }
            } else {

                //
                // not a status that we handle. We're done
                //   BUT WAIT, we're only finshed if also
                //   finished retrying HTTP authentication.
                //
                // if the app told us not to handle authentication auth_not_finished
                // will be FALSE
                //

                if (!fsm.m_bAuthNotFinished) {
                    fsm.m_bFinished = TRUE;
                }
            }

            //
            // fall through
            //

        case FSM_STATE_7:
            DEBUG_PRINT(HTTP, INFO, ("State_7_Start: lpOptional = 0x%x deOptionalLength = %d\n", fsm.m_lpOptional, fsm.m_dwOptionalLength));

            //CHECK_FSM_OWNED(Fsm);

            if (fsm.m_bRedirected) {
                BOOL bFormsSubmit = fsm.m_bFormsSubmit;
                fsm.m_bFormsSubmit = FALSE;
                if (error != ERROR_SUCCESS) {
                    goto quit;
                }

                //if ( IsResponseHttp1_1() &&
                //     fsm.m_fNeedUserApproval &&
                //     GetMethodType() != HTTP_METHOD_TYPE_GET &&
                //     GetMethodType() != HTTP_METHOD_TYPE_HEAD )
                if (fsm.m_fNeedUserApproval)
                {

                    INET_ASSERT((fsm.m_tMethodRedirect != HTTP_METHOD_TYPE_GET) &&
                                (fsm.m_tMethodRedirect != HTTP_METHOD_TYPE_HEAD));
                    //
                    // If servers force us to redirect back
                    //   to another POST, PUT, etc. then we must notify the user.
                    //

                    error = ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION;
                    DEBUG_PRINT(HTTP, INFO, ("User approval needed. returning 0x%x\r\n", error));
                    goto quit;
                }

                // If we are redirecting a forms submit always urlmon a chance
                // to ask the user because the submit might be going to a
                // restricted zone now.
                if ( bFormsSubmit )
                {
                    error = ERROR_HTTP_REDIRECT_NEEDS_CONFIRMATION;
                    DEBUG_PRINT(HTTP, INFO, ("Redirected form needs user approval. returning 0x%x\r\n", error));
                    goto quit;
                }

                INET_ASSERT(error == ERROR_SUCCESS);
                INET_ASSERT(!IsCacheReadInProgress());

                //
                // cleanup response headers from redirection
                //

                ReuseObject();
                if (GetFromCachePreNetIO() == ERROR_SUCCESS) {
                    error = ERROR_SUCCESS;
                    goto quit;
                }

                //
                // Allow Redirects to exit out and force the HttpEndRequestA
                //  caller to notice.
                //

                if ( fsm.m_arRequest == AR_HTTP_END_SEND_REQUEST &&
                     fsm.m_tMethodRedirect != HTTP_METHOD_TYPE_GET &&
                     fsm.m_tMethodRedirect != HTTP_METHOD_TYPE_HEAD
                     )
                {
                    error = ERROR_INTERNET_FORCE_RETRY;
                }
            }
        }
        state = FSM_STATE_INIT;
    } while (!fsm.m_bFinished && (error == ERROR_SUCCESS));

quit:
        DEBUG_PRINT(HTTP, INFO, ("Quit1: error = 0x%x\r\n", error));

    if (error == ERROR_IO_PENDING) {
        goto done;
    }

    if (!fsm.m_bCancelRedoOfProxy
    && fsm.m_pInternet->RedoSendRequest(&error, fsm.m_pProxyInfoQuery, GetOriginServer(), GetServerInfo())) {
        fsm.m_bFinished = FALSE;
        fsm.m_bRedirectCountedOut = FALSE;
        fsm.m_dwRedirectCount = GlobalMaxHttpRedirects;
        fsm.SetState(FSM_STATE_INIT);
        state = FSM_STATE_INIT;
        DEBUG_PRINT(HTTP, INFO, ("Quit2: error = 0x%x\r\n", error));
        goto retry_send_request;
    } else {
        //SetProxyName(NULL, 0, 0);
        DEBUG_PRINT(HTTP, INFO, ("Quit3: error = 0x%x\r\n", error));
        
    }

    //
    // if ERROR_HTTP_REDIRECT_FAILED then we tried to redirect, but found that
    // we couldn't do it (e.g. http:// to ftp:// or file://, etc.) We need to
    // defer this to the caller to clean up & make the new request. They will
    // have all the header info (plus we probably already indicated the new
    // URL during the redirect callback). Just return ERROR_SUCCESS
    //
    // Only do this if we failed because we couldn't change to the new URL, not
    // because we counted out the redirects
    //
    // We also change the error code to ERROR_SUCCESS if we didn't redirect
    // because we were redirected to the same site
    //

    if (((error == ERROR_HTTP_REDIRECT_FAILED)
    || (error == ERROR_HTTP_NOT_REDIRECTED))
    && !fsm.m_bRedirectCountedOut) {
        error = ERROR_SUCCESS;
    }

    //
    // if we received a Refresh header along the way, but no longer have it (we
    // were redirected) then restore it
    //

    if ((error == ERROR_SUCCESS) && IsRefresh()) {

        DWORD err;

        _ResponseHeaders.LockHeaders();

        if (!IsResponseHeaderPresent(HTTP_QUERY_REFRESH))
        {

            DWORD len;
            HEADER_STRING & refreshHeader = GetRefreshHeader();

            if (refreshHeader.HaveString()) {
                err = AddInternalResponseHeader(HTTP_QUERY_REFRESH,
                                                refreshHeader.StringAddress(),
                                                refreshHeader.StringLength()
                                                );

                INET_ASSERT(err == ERROR_SUCCESS);

            }
        }

        _ResponseHeaders.UnlockHeaders();
    }

    fsm.SetNextState(FSM_STATE_FINISH);

done:

    PERF_LEAVE(HttpSendRequest_Start);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::HttpSendRequest_Finish(
    IN CFsm_HttpSendRequest * Fsm
    )

/*++

Routine Description:

    description-of-function.

Arguments:

    Fsm -

Return Value:

    DWORD

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::HttpSendRequest_Finish",
                 "%#x",
                 Fsm
                 ));

    PERF_ENTER(HttpSendRequest_Finish);

    CFsm_HttpSendRequest & fsm = *Fsm;
    DWORD error = fsm.GetError();

    fsm.m_fOwnsProxyInfoQueryObj = TRUE;


    if ((error == ERROR_SUCCESS)
    && !IsCacheReadInProgress()
    && !IsCacheWriteInProgress()
    && (fsm.m_arRequest != AR_HTTP_BEGIN_SEND_REQUEST)) {

        INET_ASSERT(!IsCacheReadInProgress());

        //
        // we are not reading from the cache
        // Let us ask a routine whether we should cache this
        // stuff or not
        //
        // (only need to do this for 200/206 response code)
        //

        if( _StatusCode == HTTP_STATUS_OK || 
            _StatusCode == HTTP_STATUS_PARTIAL_CONTENT)  
        {
            if (FCanWriteToCache() ) 
            {

                DEBUG_PRINT(CACHE,
                        INFO,
                        ("Starting cache write\n"
                        ));

                FHttpBeginCacheWrite();
            } else {

                //
                // If the WININET client must have caching,
                //  Notify him that we are unable to cache his files,
                //  by erroring out.
                //

                if (GetCacheFlags() & INTERNET_FLAG_NEED_FILE) {
                    error = ERROR_INTERNET_UNABLE_TO_CACHE_FILE;
                }
            }
        }
    }

    INET_ASSERT(fsm.m_hRequestMapped != NULL);

    //if (!IsAsyncHandle() && (fsm.m_hRequestMapped != NULL)) {
    //    DereferenceObject((LPVOID)fsm.m_hRequestMapped);
    //}

    //
    // we will return FALSE even if this is an async operation and the error is
    // ERROR_IO_PENDING
    //

    fsm.SetDone(error);
    //fsm.SetApiResult(error == ERROR_SUCCESS);

    PERF_LEAVE(HttpSendRequest_Finish);

    DEBUG_LEAVE(error);

    return error;
}

DWORD
HTTP_REQUEST_HANDLE_OBJECT::BuildProxyMessage(
    IN CFsm_HttpSendRequest * Fsm,
    AUTO_PROXY_ASYNC_MSG * pProxyMsg,
    IN OUT URL_COMPONENTS * pUrlComponents
    )

/*++

Routine Description:

    Calls CrackUrl to parses request URL, and 
      transfers the information to the AUTO_PROXY_ASYNC_MSG

Arguments:

    Fsm - HTTP send request FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -
                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of resources

--*/

{
    DWORD error = ERROR_SUCCESS;

    LPSTR currentUrl;
    DWORD currentUrlLength;

    //
    // Gather the URL off the handle
    //

    currentUrl = GetURL();

    INET_ASSERT(currentUrl != NULL);

    currentUrlLength = lstrlen(currentUrl);

    //
    // BUGBUG [arthurbi] the following can be a slow call,
    //   but its too risky to change the complete behavior where
    //   we cache it
    //

    //
    // crack the current URL
    //

    memset(pUrlComponents, 0, sizeof(URL_COMPONENTS));
    pUrlComponents->dwStructSize = sizeof(URL_COMPONENTS);

    error = CrackUrl(currentUrl,
                     currentUrlLength,
                     FALSE, // don't escape URL-path
                     &(pUrlComponents->nScheme),
                     NULL,  // don't care about Scheme Name
                     NULL,
                     &(pUrlComponents->lpszHostName),
                     &(pUrlComponents->dwHostNameLength),
                     &(pUrlComponents->nPort),
                     NULL,  // don't care about user name
                     NULL,
                     NULL,  // or password
                     NULL,
                     &(pUrlComponents->lpszUrlPath),
                     &(pUrlComponents->dwUrlPathLength),
                     NULL,  // no extra
                     NULL,
                     NULL
                     );

    pProxyMsg->SetProxyMsg(
        pUrlComponents->nScheme,
        currentUrl,
        currentUrlLength,
        pUrlComponents->lpszHostName,
        pUrlComponents->dwHostNameLength,
        pUrlComponents->nPort
        );

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryProxySettings(
    IN CFsm_HttpSendRequest * Fsm,
    INTERNET_HANDLE_OBJECT * pInternet,
    IN OUT URL_COMPONENTS * pUrlComponents
    )

/*++

Routine Description:

    Wrapper over GetProxyInfo call to determine proxy
        settings on our given object

Arguments:

    Fsm - HTTP send request FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -
                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of resources

--*/

{
    DWORD error = ERROR_SUCCESS;
    CFsm_HttpSendRequest & fsm = *Fsm;

    INET_ASSERT(fsm.m_pProxyInfoQuery);
    INET_ASSERT(pInternet);

    SetProxyName(NULL, 0, 0);

    fsm.m_fOwnsProxyInfoQueryObj = FALSE;

    error = pInternet->GetProxyInfo(
                            &fsm.m_pProxyInfoQuery
                            );

    //
    //  If GetProxyInfo returns pending, then we no longer have
    //   access to the pointer that we've passed.
    //

    if ( error == ERROR_IO_PENDING )
    {
        //
        // Bail out, DO NOT TOUCH any OBJECTS or FSMs 
        //

        goto quit;
    }

    // then regardless we own it unless GetProxyInfo went pending with the FSM
    fsm.m_fOwnsProxyInfoQueryObj = TRUE;

    if ( error != ERROR_SUCCESS )
    {
        goto quit;
    }

    INET_ASSERT( error == ERROR_SUCCESS );

    if ( ! ((fsm.m_pProxyInfoQuery)->IsUseProxy()) )
    {
        if ( pUrlComponents->nScheme == INTERNET_SCHEME_FTP ||
             pUrlComponents->nScheme == INTERNET_SCHEME_GOPHER )
        {
            //
            // we cannot go direct on these
            //

            error = ERROR_INTERNET_INTERNAL_ERROR;
        }

        SetIsTalkingToSecureServerViaProxy(FALSE);        
    }

quit:

    return error;
}

DWORD
HTTP_REQUEST_HANDLE_OBJECT::CheckForCachedProxySettings(
    IN AUTO_PROXY_ASYNC_MSG *pProxyMsg,
    OUT CServerInfo **ppProxyServerInfo
    )

/*++

Routine Description:

    Attempts to determine and then resolve if there are cached
     proxy settings saved away in the CServerInfo object,
     which is found in our HTTP_REQUEST_ object.  This can
     be very useful since calling off to an auto-proxy thread
     can be quite expensive in terms of performance.

Arguments:

    pProxyMsg - the object containing our current proxy message
      information, that we use to scripple our proxy state for
      a given request

    ppProxyServerInfo - on return, may contain the resultant
      cached ServerInfo object.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -
                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of resources

--*/

{
    DWORD error = ERROR_INTERNET_INTERNAL_ERROR;

    CServerInfo * pOriginServer = GetOriginServer();
    CServerInfo * pProxyServer;

    INET_ASSERT(pProxyMsg);

    *ppProxyServerInfo = NULL;

    if (pOriginServer)
    {
        BOOL fCachedEntry;

        pProxyServer = 
            pOriginServer->GetCachedProxyServerInfo(            
                pProxyMsg->_tUrlProtocol,
                pProxyMsg->_nUrlPort,
                &fCachedEntry
                );

        if (fCachedEntry)
        {
            if ( pProxyServer )
            {
                if (pProxyServer->CopyCachedProxyInfoToProxyMsg(pProxyMsg))
                {
                    SetOriginServer();
                    *ppProxyServerInfo = pProxyServer;
                    error = ERROR_SUCCESS;
                    goto quit;
                }
            
                // nuke extra ref, sideeffect of GetCachedProxy... call            
                ::ReleaseServerInfo(pProxyServer);
            }  
            else
            {
                // DIRECT, no-proxy cached.
                pProxyMsg->SetUseProxy(FALSE);
                pProxyMsg->_lpszProxyHostName = NULL;
                error = ERROR_SUCCESS;
                goto quit;
            }
        }
    }

    pProxyMsg->SetVersion();

quit:
    return error;
}

DWORD
HTTP_REQUEST_HANDLE_OBJECT::ProcessProxySettings(
    IN CFsm_HttpSendRequest * Fsm,
    IN INTERNET_CONNECT_HANDLE_OBJECT * pConnect,
    IN OUT URL_COMPONENTS * pUrlComponents,
    OUT LPSTR * lplpszRequestObject,
    OUT DWORD * lpdwRequestObjectSize
    )
/*++

Routine Description:

    Armed with the results of the proxy query, this method takes care of 
    assembling the various variables and states to deal with various 
    types of proxies.

    More specifally, this handles HTTP Cern Proxies, SOCKS proxies, 
    SSL-CONNECT/HTTP proxies, and special cases such as FTP URLs
    with passwords through an HTTP Cern Proxy.

Arguments:

    Fsm - HTTP send request FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -
                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of resources

--*/

{
    DWORD error = ERROR_SUCCESS;
    CFsm_HttpSendRequest & fsm = *Fsm;

    LPSTR lpszUrlObject = NULL;
    LPSTR lpszObject = pUrlComponents->lpszUrlPath;
    DWORD dwcbObject = pUrlComponents->dwUrlPathLength;

    if ((fsm.m_pProxyInfoQuery)->GetProxyScheme() == INTERNET_SCHEME_SOCKS)
    {
        if ( pUrlComponents->nScheme == INTERNET_SCHEME_FTP ||
             pUrlComponents->nScheme == INTERNET_SCHEME_GOPHER )
        {
            //
            // we cannot go direct on these,
            //  this would for example an HTTP request over SOCKS,
            //  redirected to an FTP/GOPHER URL
            //

            error = ERROR_INTERNET_INTERNAL_ERROR;
            goto quit;
        }


        SetSocksProxyName((fsm.m_pProxyInfoQuery)->_lpszProxyHostName,
                          (fsm.m_pProxyInfoQuery)->_dwProxyHostNameLength,
                          (fsm.m_pProxyInfoQuery)->_nProxyHostPort
                          );

        (fsm.m_pProxyInfoQuery)->_lpszProxyHostName = NULL;
        (fsm.m_pProxyInfoQuery)->_dwProxyHostNameLength = 0;
    }
    else if (pUrlComponents->nScheme == INTERNET_SCHEME_HTTPS)
    {
        SetIsTalkingToSecureServerViaProxy(TRUE);
    }
    else
    {
        SetIsTalkingToSecureServerViaProxy(FALSE); // default value.

        //
        // if this request is going via proxy then we send the entire URL as the
        // request
        //

        DWORD urlLength;

        //
        // in secure proxy tunnelling case we are going to send the request
        // "CONNECT <host>:<port>"
        //

        if (IsTunnel()) {
            urlLength = pUrlComponents->dwHostNameLength + sizeof(":65535");
        } else {
            urlLength = INTERNET_MAX_URL_LENGTH;
        }

        lpszUrlObject = (LPSTR)ResizeBuffer(NULL, urlLength, FALSE);
        if (lpszUrlObject == NULL)
        {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }

        if (IsTunnel())
        {
#ifdef NTLM_TUNNEL
            if (pUrlComponents->nPort == INTERNET_INVALID_PORT_NUMBER)
            {
                INET_ASSERT (pUrlComponents->nScheme == INTERNET_SCHEME_HTTP);
                pUrlComponents->nPort = INTERNET_DEFAULT_HTTP_PORT;
            }
#endif
            memcpy (lpszUrlObject, pUrlComponents->lpszHostName, pUrlComponents->dwHostNameLength);
            wsprintf (lpszUrlObject + pUrlComponents->dwHostNameLength, ":%d", pUrlComponents->nPort);
        }
        else
        {
            //
            // there may be a user name & password (only if FTP)
            //

            LPSTR userName;
            DWORD userNameLength;
            LPSTR password;
            DWORD passwordLength;

            if (pUrlComponents->nScheme == INTERNET_SCHEME_FTP) {
                userName = pConnect->GetUserOrPass(IS_USER, IS_SERVER);
                userNameLength = userName ? lstrlen(userName)  : 0;
                password = pConnect->GetUserOrPass(IS_PASS, IS_SERVER);
                passwordLength = password ? lstrlen(password)  : 0;
            } else {
                userName = NULL;
                userNameLength = 0;
                password = NULL;
                passwordLength = 0;
            }

            if (pUrlComponents->nPort == INTERNET_INVALID_PORT_NUMBER)
            {
                switch (pUrlComponents->nScheme)
                {
                    case INTERNET_SCHEME_FTP:
                        pUrlComponents->nPort = INTERNET_DEFAULT_FTP_PORT;
                        break;

                    case INTERNET_SCHEME_GOPHER:
                        pUrlComponents->nPort = INTERNET_DEFAULT_GOPHER_PORT;
                        break;

                    case INTERNET_SCHEME_HTTP:
                        pUrlComponents->nPort = INTERNET_DEFAULT_HTTP_PORT;
                        break;

                    case INTERNET_SCHEME_HTTPS:
                        pUrlComponents->nPort = INTERNET_DEFAULT_HTTPS_PORT;
                        break;

                    default:
                        INET_ASSERT(FALSE);
                        break;
                }
            }

            pUrlComponents->lpszUserName = userName;
            pUrlComponents->dwUserNameLength = userNameLength;
            pUrlComponents->lpszPassword = password;
            pUrlComponents->dwPasswordLength = passwordLength;

            if (!InternetCreateUrl(pUrlComponents, 0, lpszUrlObject, &urlLength))
            {
                error = GetLastError();
                goto quit;
            }

            //
            // shrink the buffer to fit
            //

            lpszUrlObject = (LPSTR)ResizeBuffer(lpszUrlObject,
                                                (urlLength + 1) * sizeof(TCHAR),
                                                FALSE
                                                );

            INET_ASSERT(lpszUrlObject != NULL);

            if (lpszUrlObject == NULL)
            {
                error = ERROR_NOT_ENOUGH_MEMORY;
                goto quit;
            }
        }

        SetRequestUsingProxy(TRUE);

        lpszObject = lpszUrlObject;
        dwcbObject = lstrlen(lpszUrlObject);
    }

quit:

    *lplpszRequestObject   = lpszObject;
    *lpdwRequestObjectSize = dwcbObject;

    return error;
}

DWORD
HTTP_REQUEST_HANDLE_OBJECT::UpdateRequestInfo(
    IN CFsm_HttpSendRequest * Fsm,
    IN LPSTR lpszObject,
    IN DWORD dwcbObject,
    IN OUT URL_COMPONENTS * pUrlComponents,
    IN OUT CServerInfo **ppProxyServerInfo
    )

/*++

Routine Description:

    Based on object and URL information, for a given HTTP request, 
    this function assembles the "special cases" and modifes the 
    request headers in prepartion of making the actual request.

    The "special cases" includes the handling of HTTP versioning, 
    HTTP 1.0/1.1 keep-alives, and Pragma headers.

    This function also deals with the update the ServerInfo object
    that contains the host resolution information.


Arguments:

    Fsm - HTTP send request FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -
                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of resources

--*/

{
    DWORD error = ERROR_SUCCESS;

    LPSTR lpszVersion = NULL;
    DWORD dwVersionLen = 0;

    CFsm_HttpSendRequest & fsm = *Fsm;

    if ( lpszObject == NULL) 
    {
        lpszObject = pUrlComponents->lpszUrlPath;
        dwcbObject = pUrlComponents->dwUrlPathLength;
    }
        
    INET_ASSERT(dwcbObject > 0 );

    _RequestHeaders.LockHeaders();

    //
    // if we are going via proxy and HTTP 1.1 through proxy is disabled
    // then modify the request version to HTTP/1.0
    //

    if ((fsm.m_pProxyInfoQuery)->IsUseProxy() &&        
        ((fsm.m_pProxyInfoQuery)->_lpszProxyHostName != NULL) &&
        !GlobalEnableProxyHttp1_1) {
        lpszVersion = "HTTP/1.0";
        dwVersionLen = sizeof("HTTP/1.0") - 1;
    }

    ModifyRequest(GetMethodType(),
                  lpszObject,
                  dwcbObject,
                  lpszVersion,
                  dwVersionLen
                  );

    if ((fsm.m_pProxyInfoQuery)->IsUseProxy() )        
    {
        SetProxyName( (fsm.m_pProxyInfoQuery)->_lpszProxyHostName,
                      (fsm.m_pProxyInfoQuery)->_dwProxyHostNameLength,
                      (fsm.m_pProxyInfoQuery)->_nProxyHostPort
                      );

        if ((fsm.m_pProxyInfoQuery)->_lpszProxyHostName != NULL) {

            if (_ServerInfo != NULL)
            {
                _ServerInfo->SetProxyByPassed(FALSE);
            }

            //
            // changing server info from origin server to proxy server. Keep
            // pointer to origin server so that we can update connect and
            // round-trip times
            //

            SetOriginServer();

            if (*ppProxyServerInfo) {
                // cached server info
                SetServerInfo(*ppProxyServerInfo);
                *ppProxyServerInfo = NULL;
            }
            else
            {            
                error = SetServerInfo((fsm.m_pProxyInfoQuery)->_lpszProxyHostName,
                                      (fsm.m_pProxyInfoQuery)->_dwProxyHostNameLength
                                      );
                if (error != ERROR_SUCCESS) {
                    goto quit;
                }
            }
        }
    }
    else
    {
        if (_ServerInfo != NULL)
        {
            _ServerInfo->SetProxyByPassed(TRUE);

            if ( pUrlComponents->lpszHostName )
            {
                error = SetServerInfo(pUrlComponents->lpszHostName,
                                      pUrlComponents->dwHostNameLength
                                      );

                if (error != ERROR_SUCCESS) {
                    goto quit;
                }
            }

        }
    }

    //
    // determine whether we use persistent connections and ensure the correct
    // type and number of keep-alive headers are present
    //

    //
    // BUGBUG - we need to check for "Connection: keep-alive". There may be
    //          other types of "Connection" header, and the keep-alive header
    //          may contain additional information
    //

    DWORD bufferLength;
    DWORD index;
    DWORD dwHeaderNameIndex;

    if (IsRequestUsingProxy()) {
        RemoveAllRequestHeadersByName(HTTP_QUERY_CONNECTION);
        dwHeaderNameIndex = HTTP_QUERY_PROXY_CONNECTION;
    } else {
        RemoveAllRequestHeadersByName(HTTP_QUERY_PROXY_CONNECTION);
        dwHeaderNameIndex = HTTP_QUERY_CONNECTION;
    }

    if (IsRequestHeaderPresent(dwHeaderNameIndex)) {
        SetWantKeepAlive(TRUE);
        SetOpenFlags(
            GetOpenFlags() | INTERNET_FLAG_KEEP_CONNECTION);
    }

    error = ERROR_SUCCESS;

    //
    // if the global keep-alive switch
    // is off then we don't want any keep-alive headers
    //

    if (GlobalDisableKeepAlive)
    {
        RemoveAllRequestHeadersByName(HTTP_QUERY_CONNECTION);
        RemoveAllRequestHeadersByName(HTTP_QUERY_PROXY_CONNECTION);

        if (IsRequestHttp1_1())
        {
            //
            // Add "Connection: Close" header because we're not doing
            //  keep-alive on this Request, needed for HTTP 1.1
            //

            (void)ReplaceRequestHeader(HTTP_QUERY_CONNECTION,
                                       CLOSE_SZ,
                                       CLOSE_LEN,
                                       0,
                                       REPLACE_HEADER
                                       );
        }

        SetOpenFlags(
            GetOpenFlags() & ~INTERNET_FLAG_KEEP_CONNECTION);
    }

    //
    // if the app requested keep-alive then add the header; if we're going via
    // proxy then use the proxy-connection header
    //

    if (GetOpenFlags() & INTERNET_FLAG_KEEP_CONNECTION)
    {
        SetWantKeepAlive(TRUE);
        (void)ReplaceRequestHeader(dwHeaderNameIndex,
                                   KEEP_ALIVE_SZ,
                                   KEEP_ALIVE_LEN,
                                   0,
                                   ADD_HEADER_IF_NEW
                                   );
    }

    //
    // if app added "connection: close" then we don't want keep-alive
    //

    if (IsRequestHttp1_1()) {

        BOOL bClose = FindConnCloseRequestHeader(dwHeaderNameIndex);
        BOOL bWantKeepAlive;
        DWORD dwOpenFlags = GetOpenFlags();

        if (bClose || (IsTunnel() && GetAuthState() != AUTHSTATE_CHALLENGE)) {
            RemoveAllRequestHeadersByName(dwHeaderNameIndex);

            //
            // For a Tunnel to a proxy we want to make sure that
            //  keep-alive is off since is does not make sense
            //  to do keep-alive with in a HTTP CONNECT request
            //
            // Note: we do not add the Connection: close header
            //  because of its amphorus definition in this case.
            //

            if (!IsTunnel()) {
                (void)ReplaceRequestHeader(dwHeaderNameIndex,
                                           CLOSE_SZ,
                                           CLOSE_LEN,
                                           0,
                                           REPLACE_HEADER
                                           );
            }
            bWantKeepAlive= FALSE;
            dwOpenFlags &= ~INTERNET_FLAG_KEEP_CONNECTION;
        } else {
            bWantKeepAlive = TRUE;
            dwOpenFlags |= INTERNET_FLAG_KEEP_CONNECTION;
        }
        SetWantKeepAlive(bWantKeepAlive);
        SetOpenFlags(dwOpenFlags);
    }

    DWORD dwFlags;
    dwFlags = GetCacheFlags();

    if (GetMethodType() == HTTP_METHOD_TYPE_POST && !GetSecondaryCacheKey())
    {
        //
        // IE30 compat: force reload on post
        //

        dwFlags |= INTERNET_FLAG_RELOAD;
    }

    if (dwFlags & (INTERNET_FLAG_RELOAD | INTERNET_FLAG_PRAGMA_NOCACHE))
    {

        //
        // add "Pragma: No-Cache" header if not already present and only if
        // going via proxy
        //

        if (IsRequestUsingProxy())
        {
            ReplaceRequestHeader(HTTP_QUERY_PRAGMA,
                                 NO_CACHE_SZ,
                                 NO_CACHE_LEN,
                                 0,   // dwIndex
                                 ADD_HEADER_IF_NEW
                                 );
        }
    }

quit:

    _RequestHeaders.UnlockHeaders();

    return error;

}



DWORD
HTTP_REQUEST_HANDLE_OBJECT::UpdateProxyInfo(
    IN CFsm_HttpSendRequest * Fsm,
    IN BOOL fCallback
    )

/*++

Routine Description:

    Queries Proxy Information, and based on the proxy info it assembles the appropriate
     HTTP request.

Arguments:

    Fsm - HTTP send request FSM

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -
                  ERROR_NOT_ENOUGH_MEMORY
                    Ran out of resources

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::UpdateProxyInfo",
                 "%#x, %B",
                 Fsm,
                 fCallback
                 ));

    PERF_ENTER(UpdateProxyInfo);

    DWORD error = ERROR_SUCCESS;

    CFsm_HttpSendRequest & fsm = *Fsm;

    CServerInfo *pProxyServer = NULL;

    INTERNET_HANDLE_OBJECT * pInternet;
    INTERNET_CONNECT_HANDLE_OBJECT * pConnect;

    AUTO_PROXY_ASYNC_MSG proxyInfoQuery;
    URL_COMPONENTS urlComponents;

    LPSTR lpszObject = NULL;
    DWORD dwcbObject = 0;


    // once we're woken up, we own the obj stored in our FSM.
    INET_ASSERT(fsm.m_fOwnsProxyInfoQueryObj); 

    //
    // Get the Obj Pointers we care about
    //

    pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)GetParent();

    INET_ASSERT(pConnect != NULL);
    INET_ASSERT(pConnect->IsValid(TypeHttpConnectHandle) == ERROR_SUCCESS);

    pInternet = (INTERNET_HANDLE_OBJECT *)pConnect->GetParent();

    INET_ASSERT(pInternet != NULL);
    INET_ASSERT(pInternet->IsValid(TypeInternetHandle) == ERROR_SUCCESS);

    //
    // Clear our handle state in regards to proxy settings
    //

    SetSocksProxyName(NULL, NULL, NULL);
    SetRequestUsingProxy(FALSE);

    //
    // Parse URL, this is very lame, I hate doing this every time,
    //  and even worse we need to do this before our caching code
    //  gets hit, but we can't move it because the quit code
    //  depends on the parsed URL.  In the future we should cache this!!
    //

    error = BuildProxyMessage(
                Fsm,
                &proxyInfoQuery,
                &urlComponents
                );

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // No proxy installed on this object, bail out
    //

    if ( ! pInternet->IsProxy() && ! IsOverrideProxyMode() )
    {
        INET_ASSERT(fsm.m_pProxyInfoQuery == NULL);
        fsm.m_pProxyInfoQuery = &proxyInfoQuery; // !!! put our local in the FSM
        goto quit;
    }

    //
    // If we're in the callback, just retrieve the results,
    //  from the orginal blocking call to proxy code
    //

    if ( fsm.m_pProxyInfoQuery )
    {            
        fCallback = TRUE;

        if ( ! (fsm.m_pProxyInfoQuery)->IsBackroundDetectionPending()) {
            (fsm.m_pProxyInfoQuery)->SetQueryOnCallback(TRUE);
        }

        error = QueryProxySettings(Fsm, pInternet, &urlComponents);
        if ( error != ERROR_SUCCESS || !(fsm.m_pProxyInfoQuery)->IsUseProxy())              
        {
            goto quit;
        }
    }
    else if ((fsm.m_pProxyInfoQuery = &proxyInfoQuery) &&  // !!! put our local in the FSM
              CheckForCachedProxySettings(fsm.m_pProxyInfoQuery, &pProxyServer) 
                != ERROR_SUCCESS )
    {
        //
        // If there is nothing cached, then we'll need
        //  to do the actual proxy request to the proxy code
        // 
        
        //fsm.m_pProxyInfoQuery = &proxyInfoQuery; // !!! put our local in the FSM
        proxyInfoQuery.SetBlockUntilCompletetion(TRUE);
        proxyInfoQuery.SetShowIndication(TRUE);        

        if (!IsTunnel() && !IsOverrideProxyMode())
        {
            error = QueryProxySettings(Fsm, pInternet, &urlComponents);
            if ( error != ERROR_SUCCESS || !(fsm.m_pProxyInfoQuery)->IsUseProxy()) {
                goto quit;
            }
        }
        else // fall-back
        {
            //
            // Get the current proxy information,
            //   if we're in an nested SSL tunnell
            //

            GetProxyName(&(fsm.m_pProxyInfoQuery)->_lpszProxyHostName,
                         &(fsm.m_pProxyInfoQuery)->_dwProxyHostNameLength,
                         &(fsm.m_pProxyInfoQuery)->_nProxyHostPort
                         );

            (fsm.m_pProxyInfoQuery)->_tProxyScheme = INTERNET_SCHEME_DEFAULT;
            (fsm.m_pProxyInfoQuery)->SetUseProxy(TRUE);
        }
    }

    //
    // Need to figure out whether we're actually talking
    //  to a Server via proxy.  In this case we need to
    //  special case some logic in the Send so we create
    //  a sub-request to the proxy-server, and then do this
    //  request to the main SSL server.
    //

    if ( (fsm.m_pProxyInfoQuery)->IsUseProxy() ) 
    {
        error = ProcessProxySettings(
                    Fsm,
                    pConnect,
                    &urlComponents,
                    &lpszObject,
                    &dwcbObject
                    );    
    }

quit:

    //
    // If we didn't fail with pending,
    //  go ahead and process the request headers
    //
   
    if ( error != ERROR_IO_PENDING)
    {
        if ( error == ERROR_SUCCESS ) {
            error = UpdateRequestInfo(Fsm, lpszObject, dwcbObject, &urlComponents, &pProxyServer);
        }

        //
        // Now, Unlink the proxyinfomsg struc from the fsm,
        //   if its our stack based variable that we used as a temp
        //

        if ( fsm.m_fOwnsProxyInfoQueryObj &&
             fsm.m_pProxyInfoQuery &&
             ! (fsm.m_pProxyInfoQuery)->IsAlloced() )
        {
            fsm.m_pProxyInfoQuery = NULL;
        }

    }

    //
    // Don't leak objects, Give a hoot, don't pollute !!
    //

    if ( pProxyServer != NULL )
    {
        ::ReleaseServerInfo(pProxyServer);
    }

    if ( lpszObject != NULL &&
         lpszObject != urlComponents.lpszUrlPath)
    {
        FREE_MEMORY(lpszObject);
    }

    PERF_LEAVE(UpdateProxyInfo);

    DEBUG_LEAVE(error);

    return error;
}


BOOL
HTTP_REQUEST_HANDLE_OBJECT::FindConnCloseRequestHeader(
    IN DWORD dwIndex
    )

/*++

Routine Description:

    Determine if Connection: Close added to request headers

Arguments:

    dwIndex - id of Connection header to search for (Connection or
              Proxy-Connection)

Return Value:

    BOOL
        TRUE    - header found

        FALSE   - header not found

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Bool,
                 "HTTP_REQUEST_HANDLE_OBJECT::FindConnCloseRequestHeader",
                 "%d [%s]",
                 dwIndex,
                 InternetMapHttpOption(dwIndex)
                 ));

    BOOL bFound = FALSE;

    if (CheckedConnCloseRequest()) {
        bFound = IsConnCloseRequest(dwIndex == HTTP_QUERY_PROXY_CONNECTION);
    } else {

        LPSTR ptr;
        DWORD len;
        DWORD index = 0;

        while (FastQueryRequestHeader(dwIndex,
                                      (LPVOID *)&ptr,
                                      &len,
                                      index) == ERROR_SUCCESS) {
            if ((len == CLOSE_LEN) && (strnicmp(ptr, CLOSE_SZ, len) == 0)) {
                bFound = TRUE;
                break;
            }
            index++;
        }
        SetCheckedConnCloseRequest(dwIndex == HTTP_QUERY_PROXY_CONNECTION, bFound);
    }

    DEBUG_LEAVE(bFound);

    return bFound;
}
