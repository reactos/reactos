/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    response.cxx

Abstract:

    This file contains the HTTP Request Handle Object ReceiveResponse method

    Contents:
        CFsm_ReceiveResponse::RunSM
        HTTP_REQUEST_HANDLE_OBJECT::ReceiveResponse_Fsm

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
// private manifests
//

#define DEFAULT_RESPONSE_BUFFER_LENGTH  (1 K)

//
// HTTP Request Handle Object methods
//


DWORD
CFsm_ReceiveResponse::RunSM(
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
                 "CFsm_ReceiveResponse::RunSM",
                 "%#x",
                 Fsm
                 ));

    CFsm_ReceiveResponse * stateMachine = (CFsm_ReceiveResponse *)Fsm;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;
    DWORD error;

    START_SENDREQ_PERF();

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pRequest->ReceiveResponse_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    STOP_SENDREQ_PERF();

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::ReceiveResponse_Fsm(
    IN CFsm_ReceiveResponse * Fsm
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
#if INET_DEBUG
//#define RLF_TEST_CODE
#ifdef RLF_TEST_CODE

//
// single 100 response
//

#define TEST_HEADER_0   "HTTP/1.1 100 Continue\r\n" \
                        "\r\n"

//
// single 100 header
//

#define TEST_HEADER_1   "HTTP/1.1 100 Continue\r\n" \
                        "Server: Richard's Test-Case Virtual Server/1.0\r\n" \
                        "Date: Mon, 01 Apr 2000 00:00:01 GMT\r\n" \
                        "\r\n"

//
// continue header with moderate amount of data
//

#define TEST_HEADER_2   "HTTP/1.1 100 Continue\r\n" \
                        "Server: Richard's Test-Case Virtual Server/1.0\r\n" \
                        "Date: Mon, 01 Apr 2000 00:00:01 GMT\r\n" \
                        "Content-Length: 128\r\n" \
                        "Content-Type: octet/shmoctet\r\n" \
                        "\r\n" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef"

//
// continue header seen from apache server
//

#define TEST_HEADER_3   "HTTP/1.1 100 Continue\r\n" \
                        "\r\n" \
                        "\n\n\n\n\n"

//
// multiple continue headers, no data
//

#define TEST_HEADER_4   "HTTP/1.1 100 Continue\r\n" \
                        "Server: Richard's Test-Case Virtual Server/1.0\r\n" \
                        "Date: Mon, 01 Apr 2000 00:00:01 GMT\r\n" \
                        "\r\n" \
                        "HTTP/1.1 100 Continue\r\n" \
                        "Server: Richard's Test-Case Virtual Server/1.0\r\n" \
                        "Date: Mon, 01 Apr 2000 00:00:01 GMT\r\n" \
                        "\r\n" \
                        "HTTP/1.1 100 Continue\r\n" \
                        "Server: Richard's Test-Case Virtual Server/1.0\r\n" \
                        "Date: Mon, 01 Apr 2000 00:00:01 GMT\r\n" \
                        "\r\n" \
                        "HTTP/1.1 100 Continue\r\n" \
                        "Server: Richard's Test-Case Virtual Server/1.0\r\n" \
                        "Date: Mon, 01 Apr 2000 00:00:01 GMT\r\n" \
                        "\r\n"

//
// single 100 response, preceeded by preamble and containing a chunked response
//

#define TEST_HEADER_5   "!!!! this is a pre-amble, should be ignored even though it includes HTTP !!!!" \
                        "     " \
                        "HTTP/1.1 100 Go ahead punk, make my day\r\n" \
                        "Server: Richard's Test-Case Virtual Server/1.0\r\n" \
                        "Date: Mon, 01 Apr 2000 00:00:01 GMT\r\n" \
                        "Transfer-Encoding: chunked\r\n" \
                        "\r\n" \
                        "0010 this is the first chunk (16 bytes)\r\n" \
                        "0123456789abcdef" \
                        "\r\n" \
                        "  10; this is the second chunk (16 bytes)\r\n" \
                        "0123456789abcdef" \
                        "\r\n" \
                        "00F3\r\n" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "0123456789abcdef" \
                        "012" \
                        "\r\n" \
                        "0000; the final chunk\r\n" \
                        "\r\n" \
                        "Entity-Header: this is the chunk footer\r\n" \
                        "\r\n"

//
// enpty chunk encoded response with empty footer
//

#define TEST_HEADER_6   "HTTP/1.1 100 Continue\r\n" \
                        "Server: Richard's Test-Case Virtual Server/1.0\r\n" \
                        "Date: Mon, 01 Apr 2000 00:00:01 GMT\r\n" \
                        "Transfer-Encoding: chunked\r\n" \
                        "\r\n" \
                        "0\r\n" \
                        "\r\n" \
                        "\r\n"

    const struct {LPSTR ptr; DWORD len;} test_cases[] = {
        TEST_HEADER_0, sizeof(TEST_HEADER_0) - 1,
        TEST_HEADER_1, sizeof(TEST_HEADER_1) - 1,
        TEST_HEADER_2, sizeof(TEST_HEADER_2) - 1,
        TEST_HEADER_3, sizeof(TEST_HEADER_3) - 1,
        TEST_HEADER_4, sizeof(TEST_HEADER_4) - 1,
        TEST_HEADER_5, sizeof(TEST_HEADER_5) - 1,
        TEST_HEADER_6, sizeof(TEST_HEADER_6) - 1
    };
    DWORD test_index = 99;

#endif // def RLF_TEST_CODE
#endif // INET_DEBUG

    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::ReceiveResponse_Fsm",
                 "%#x",
                 Fsm
                 ));

    PERF_ENTER(ReceiveResponse_Fsm);

    CFsm_ReceiveResponse & fsm = *Fsm;
    DWORD error = fsm.GetError();
    FSM_STATE state = fsm.GetState();

    if (error != ERROR_SUCCESS) {

        if (error == ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED) {

            if ((_Socket != NULL) && _Socket->IsSecure())
            {
                if(m_pSecurityInfo)
                {
                    /* SCLE ref */
                    m_pSecurityInfo->Release();
                }
                /* SCLE ref */
                m_pSecurityInfo = ((ICSecureSocket *)_Socket)->GetSecurityEntry();
            }

            SetState(HttpRequestStateOpen);
            CloseConnection(TRUE);
            fsm.SetDone();
            goto quit2;
        }

        goto quit;
    }
    if (state != FSM_STATE_INIT) {
        state = fsm.GetFunctionState();
    }
    do {
        switch (state) {
        case FSM_STATE_INIT:
            if (_ResponseBuffer == NULL) {
                _ResponseBufferLength = DEFAULT_RESPONSE_BUFFER_LENGTH;
                _ResponseBuffer = (LPBYTE)ALLOCATE_MEMORY(LMEM_FIXED,
                                                          _ResponseBufferLength);
                if (_ResponseBuffer == NULL) {
                    _ResponseBufferLength = 0;
                    error = ERROR_NOT_ENOUGH_MEMORY;
                    goto quit;
                }
            }

            INET_ASSERT(_BytesReceived == 0);

            fsm.m_dwResponseLeft = _ResponseBufferLength;
            state = FSM_STATE_2;

            //
            // fall through
            //

#ifdef RLF_TEST_CODE

            InternetGetDebugVariable("WininetTestIndex", &test_index);
            if (test_index < ARRAY_ELEMENTS(test_cases)) {
                _BytesReceived = test_cases[test_index].len;
                memcpy(_ResponseBuffer, test_cases[test_index].ptr, _BytesReceived);
                fsm.m_dwResponseLeft = _ResponseBufferLength - _BytesReceived;
            }

#endif // def RLF_TEST_CODE

        case FSM_STATE_2:

            //
            // we will allow Receive() to expand the buffer (and therefore initially
            // allocate it), and to compress the buffer if we receive the end of the
            // connection. It is up to UpdateResponseHeaders() to figure out when
            // enough data has been read to indicate end of the headers
            //

            fsm.SetFunctionState(FSM_STATE_3);

            INET_ASSERT(_Socket != NULL);

            if (_Socket != NULL) {
                error = _Socket->Receive((LPVOID *)&_ResponseBuffer,
                                         &_ResponseBufferLength,
                                         &fsm.m_dwResponseLeft,
                                         &_BytesReceived,
                                         0,
                                         SF_EXPAND
                                         | SF_COMPRESS
                                         | SF_INDICATE,
                                         &fsm.m_bEofResponseHeaders
                                         );
                if (error == ERROR_IO_PENDING) {
                    goto quit;
                }
            } else {
                error = ERROR_INTERNET_OPERATION_CANCELLED;
            }

            //
            // fall through
            //

        case FSM_STATE_3:

            //
            // if we are using a keep-alive connection that was previously timed-out
            // by the server, we may not find out about it until now
            //
            // Note: it seems we can get a zero length response at this point also,
            // which I take to mean that the server-side socket has been closed
            //

            INET_ASSERT(_BytesReceived <= _ResponseBufferLength);

            if ((error != ERROR_SUCCESS)
            || ((_BytesReceived == 0) && IsKeepAlive())) {

                //
                // We need to reset the state if we got a
                // certificate request.
                //

                if (error == ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED) {

                    if ((_Socket != NULL) && _Socket->IsSecure())
                    {
                        if(m_pSecurityInfo)
                        {
                            /* SCLE ref */
                            m_pSecurityInfo->Release();
                        }
                        /* SCLE ref */
                        m_pSecurityInfo = ((ICSecureSocket *)_Socket)->GetSecurityEntry();
                    }

                    SetState(HttpRequestStateOpen);
                }
                CloseConnection(TRUE);
                goto quit;
            }

            //
            // if we received no data then the server has closed the connection
            // already
            //

            if (_BytesReceived != 0) {

                BOOL bHaveFinalResponse;

                do {
                    bHaveFinalResponse = TRUE;
                    error = UpdateResponseHeaders(&fsm.m_bEofResponseHeaders);
//if (!(rand() % 7)) {
//    error = ERROR_HTTP_INVALID_SERVER_RESPONSE;
//}
                    if (error != ERROR_SUCCESS) {
//dprintf("UpdateResponseHeaders() returns %d\n", error);
                        break;
                    }

                    DWORD statusCode;

                    statusCode = GetStatusCode();

                    //
                    // receive next packet if we didn't get a status code yet
                    //

                    if (statusCode == 0) {
                        break;
                    }

                    //
                    // discard any 1xx responses and get the headers again
                    //

                    if (fsm.m_bEofResponseHeaders
                    && (statusCode >= HTTP_STATUS_CONTINUE)
                    && (statusCode < HTTP_STATUS_OK)) {
                        bHaveFinalResponse = FALSE;
                        fsm.SetFunctionState(FSM_STATE_4);

                        //
                        // get any data that came with the header
                        //

                        fsm.m_bDrained = FALSE;
                        if (IsContentLength() && (_BytesInSocket != 0)) {
                            error = DrainResponse(&fsm.m_bDrained);
                            if (error != ERROR_SUCCESS) {
                                goto quit;
                            }
                        }

                        //
                        // fall through
                        //

        case FSM_STATE_4:

                        //
                        // now that we have drained the socket, we can indicate
                        // the response to the app. This gives apps chance to
                        // perform progress reporting for each 100 response
                        // received, e.g.
                        //

                        InternetIndicateStatus(INTERNET_STATUS_INTERMEDIATE_RESPONSE,
                                               &statusCode,
                                               sizeof(statusCode)
                                               );

                        //
                        // if there is no more data left in the buffer then we
                        // can receive the next response at the start of the
                        // buffer, else continue from where the previous one
                        // ended
                        //

                        if (fsm.m_bDrained || !IsBufferedData()) {
                            fsm.m_dwResponseLeft = _ResponseBufferLength;
                            _BytesReceived = 0;
                            _DataOffset = 0;
                            _ResponseScanned = 0;
                        } else {
                            _ResponseScanned = _DataOffset;
                            if (IsContentLength()) {
                                _ResponseScanned += _ContentLength;
                            }
                            //if (IsChunkEncoding()) {
                            //
                            //    LPSTR lpszNewBuffer;
                            //    DWORD dwNewBufferLength;
                            //
                            //    error = _ctChunkInfo.ParseChunkInput(
                            //        (LPSTR) BufferedDataStart(),
                            //        BufferedDataLength(),
                            //        &lpszNewBuffer,
                            //        &dwNewBufferLength
                            //        );
                            //
                            //    _ResponseBufferDataReadyToRead = dwNewBufferLength;
                            //
                            //    INET_ASSERT(error == ERROR_SUCCESS);
                            //    if ( error != ERROR_SUCCESS )
                            //    {
                            //        goto quit;
                            //    }
                            //}
                            //if (IsChunkEncoding()) {
                            //
                            //    LPSTR lpszNewBuffer;
                            //    DWORD dwNewBufferLength;
                            //    DWORD nRead = 0;
                            //
                            //    INET_ASSERT(!IsContentLength());
                            //
                            //    error = _ctChunkInfo.ParseChunkInput(
                            //        (LPSTR) (_ResponseBuffer + _DataOffset),
                            //        nRead,
                            //        &lpszNewBuffer,
                            //        &dwNewBufferLength
                            //        );
                            //
                            //    nRead = dwNewBufferLength;
                            //    _BytesReceived = nRead + _DataOffset;
                            //
                            //    INET_ASSERT(error == ERROR_SUCCESS); // I want to see this happen.
                            //    if ( error != ERROR_SUCCESS )
                            //    {
                            //        break;
                            //    }
                            //
                            //    if ( IsChunkedEncodingFinished() )
                            //    {
                            //        break;
                            //    }
                            //}
                        }
                        _ResponseHeaders.FreeHeaders();
                        _ResponseHeaders.Initialize();
                        ZapFlags();
                        _ContentLength = 0;
                        _BytesRemaining = 0;
                        _BytesInSocket = 0;
                        fsm.m_bEofResponseHeaders = FALSE;
                        if (_DataOffset == 0) {

                            //
                            // need to read next response - nothing left in
                            // buffer
                            //

                            break;
                        }
                    }

                    // If we have a server authentication context
                    // and the response is anything but 401, mark
                    // the socket as authenticated.
                    AUTHCTX *pAuthCtx;
                    pAuthCtx = GetAuthCtx();
                    if (pAuthCtx && !pAuthCtx->_fIsProxy
                        && (statusCode != HTTP_STATUS_DENIED))
                    {

#define MICROSOFT_IIS_SERVER_SZ "Microsoft-IIS/"
#define MICROSOFT_PWS_SERVER_SZ "Microsoft-PWS/"

#define MICROSOFT_IIS_SERVER_LEN (sizeof(MICROSOFT_IIS_SERVER_SZ) - 1)
#define MICROSOFT_PWS_SERVER_LEN (sizeof(MICROSOFT_PWS_SERVER_SZ) - 1)

                        LPSTR pszBuf;
                        DWORD cbBuf;
                        cbBuf = MAX_PATH;
                        if (FastQueryResponseHeader(HTTP_QUERY_SERVER, 
                            (LPVOID*) &pszBuf, &cbBuf, 0) == ERROR_SUCCESS)
                        {
                            if (cbBuf >= MICROSOFT_IIS_SERVER_LEN 
                                && (!strncmp(pszBuf, MICROSOFT_IIS_SERVER_SZ, MICROSOFT_IIS_SERVER_LEN)
                                    || !strncmp(pszBuf, MICROSOFT_PWS_SERVER_SZ, MICROSOFT_PWS_SERVER_LEN)))

                            {                            
                                // Found an IIS header. Mark socket as authenticated if 
                                // IIS 1, 2 or 3. Lengths of both strings are same.
                                CHAR *pVer = pszBuf + MICROSOFT_IIS_SERVER_LEN;
                                if (*pVer == '1'
                                    || *pVer == '2'
                                    || *pVer == '3'
                                    )
                                {
                                    // IIS 1, 2 or 3  - mark dirty.
                                    _Socket->SetAuthenticated();
                                }
                            }                                
                        }
                        else
                        {
                            // Unknown server; may be IIS 1,2 or 3.
                            _Socket->SetAuthenticated();
                        }
                    }

                } while (!bHaveFinalResponse);
            } else {
                error = ERROR_HTTP_INVALID_SERVER_RESPONSE;
            }

            //
            // set state to perform next receive
            //

            state = FSM_STATE_2;
        }
    } while ((error == ERROR_SUCCESS) && !fsm.m_bEofResponseHeaders);

    //
    // we should update the RTT as soon as we get received data from
    // the socket, but then we'd have to store the RTT in the socket
    // object or access this one, etc. Just keep it here for now -
    // its a reasonable approximation in the normal IE case: not too
    // much time spent in callbacks etc.
    //

    UpdateRTT();
//dprintf("RTT for %s = %d\n", GetURL(), GetRTT());
//dprintf("OS = %s, PS = %s\n", ((GetOriginServer() != NULL) ? GetOriginServer()->GetHostName() : "none"),
//    ((GetServerInfo() != NULL) ? GetServerInfo()->GetHostName() : "none"));

    //
    // we have received the headers and possibly some (or all) of the data. The
    // app can now query the headers and receive the data
    //

    SetState(HttpRequestStateObjectData);

    //
    // record the amount of data immediately available to the app
    //

    if ( IsChunkEncoding() )
    {
        LPSTR lpszNewBuffer;
        DWORD dwNewBufferLength;

        error = _ctChunkInfo.ParseChunkInput(
            (LPSTR) BufferedDataStart(),
            BufferedDataLength(),
            &lpszNewBuffer,
            &dwNewBufferLength
            );

        _ResponseBufferDataReadyToRead = dwNewBufferLength;

        INET_ASSERT(error == ERROR_SUCCESS);
        if ( error != ERROR_SUCCESS )
        {
            goto quit;
        }
    }

    SetAvailableDataLength(BufferDataAvailToRead());

    //
    // IIS caches authentication credentials on keep-alive sockets.
    //

    if (_Socket) {

        if (IsAuthorized()) {
            _Socket->SetAuthorized();
        }

        if (IsPerUserItem()) {
            _Socket->SetPerUser();
        } else if (_Socket->IsPerUser()) {
            SetPerUserItem(TRUE);
        }
    }

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();

        //
        // if we got the socket from the keep-alive pool, but found no keep-
        // alive header then we no longer have a keep-alive connection
        //

        if (_bKeepAliveConnection && !IsKeepAlive()) {
//dprintf("*** %s - NO LONGER K-A socket %#x\n", GetURL(), _Socket->GetSocket());
            SetNoLongerKeepAlive();
        }

        //
        // don't maintain the connection if there's no more data to read. UNLESS
        // we are in the middle of establishing an authenticated connection
        // (implies using keep-alive connection, e.g. NTLM)
        // IsData() returns FALSE if there's no data at all, otherwise we
        // check to see if we have read all the data already (i.e. with the
        // response headers)
        //

        if ((error != ERROR_SUCCESS)
            || (

                //
                // data-less response (ignoring keep-alive & content-length)
                //

                (!IsData()

                 //
                 // all data body in header buffer
                 //

                 || (IsKeepAlive()
                     && IsContentLength()
                     && (BufferedDataLength() == GetContentLength())
                     )
                 )

                //
                // but only if not in the middle of auth negotiation and if the
                // connection hasn't been dropped by the server
                //

                && ((GetAuthState() != AUTHSTATE_NEGOTIATE)
                    || IsNoLongerKeepAlive())
                )
            ) {

//dprintf("socket %#x [%#x/%d] error=%d, IsData()=%B, K-A=%B, C-L=%d, BDL=%d, AS=%d\n",
//        _Socket,
//        _Socket ? _Socket->GetSocket() : 0,
//        _Socket ? _Socket->GetSourcePort() : 0,
//        error,
//        IsData(),
//        IsKeepAlive(),
//        GetContentLength(),
//        BufferedDataLength(),
//        GetAuthState()
//        );

            //
            // BUGBUG - if this is a new keep-alive connection?
            //

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("closing: error = %d, IsData() = %B, K-A = %B, IsC-L = %B, BDL = %d, C-L = %d\n",
                        error,
                        IsData(),
                        IsKeepAlive(),
                        IsContentLength(),
                        BufferedDataLength(),
                        GetContentLength()
                        ));

            if(GlobalAlwaysDrainOnRedirect || 
                GetStatusCode() != HTTP_STATUS_REDIRECT || 
                (HTTP_METHOD_TYPE_HEAD == GetMethodType()))
            {
                CloseConnection((error != ERROR_SUCCESS) ? TRUE : FALSE);
            }
            else
                DEBUG_PRINT(HTTP, INFO, ("Not closing socket, Status code = %d \n", GetStatusCode()));

            //
            // set the relevant state
            //

            if (error != ERROR_SUCCESS &&
                error != ERROR_INTERNET_CLIENT_AUTH_CERT_NEEDED &&
                error != ERROR_INTERNET_INVALID_CA &&
                error != ERROR_INTERNET_SEC_CERT_DATE_INVALID &&
                error != ERROR_INTERNET_SEC_CERT_CN_INVALID )
            {
                SetState(HttpRequestStateError);
            }
        }

        PERF_LEAVE(ReceiveResponse_Fsm);
    }

quit2:

    DEBUG_LEAVE(error);

    return error;
}
