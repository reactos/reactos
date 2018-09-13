/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    sockets.cxx

Abstract:

    Contains functions to interface between gopher APIs and Winsock

    Contents:
        GopherConnect
        GopherDisconnect
        GopherSendRequest
        GopherReceiveResponse

Author:

    Richard L Firth (rfirth) 11-Oct-1994

Environment:

    Win32(s) user-mode DLL

Revision History:

    11-Oct-1994 rfirth
        Created

--*/

#include <wininetp.h>
#include "gfrapih.h"

//
// manifests
//

#define DEFAULT_RESPONSE_BUFFER_LENGTH  (4 K)

//
// functions
//


DWORD
GopherConnect(
    IN LPVIEW_INFO ViewInfo
    )

/*++

Routine Description:

    Makes a connection to a (gopher) server. Sets a receive timeout on the
    connected socket

Arguments:

    ViewInfo    - pointer to VIEW_INFO containing pointer to SESSION_INFO which
                  describes server to connect to

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                Dword,
                "GopherConnect",
                "%#x",
                ViewInfo
                ));

    DWORD error;
    BOOL  fSuccess;
    INTERNET_CONNECT_HANDLE_OBJECT *pConnect;
    PROXY_STATE *pProxyState = NULL;

    INET_ASSERT(ViewInfo->BufferInfo != NULL);
    INET_ASSERT(ViewInfo->SessionInfo != NULL);

    //
    // determine sync or async
    //

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    DWORD asyncFlags;

    asyncFlags = 0;
    //asyncFlags = lpThreadInfo->IsAsyncWorkerThread ? SF_NON_BLOCKING : 0;

    //
    // Set the port we're using on the socket object.
    //

    ViewInfo->BufferInfo->Socket->SetPort((INTERNET_PORT) ViewInfo->SessionInfo->Port);

    //
    // Using the object handle, check to see if we have a socks proxy.
    //  If so, use it to do our connections.
    //

    INTERNET_HANDLE_OBJECT * pInternet;
    HINTERNET hConnectMapped;

    INET_ASSERT(lpThreadInfo != NULL);
    INET_ASSERT(lpThreadInfo->hObjectMapped != NULL);
    INET_ASSERT(ViewInfo->SessionInfo->Host != NULL);

    //
    // Get the Mapped Connect Handle Object...
    //

    INET_ASSERT( (ViewInfo->ViewType == ViewTypeFile) ||
                 (ViewInfo->ViewType == ViewTypeFind) );

    hConnectMapped = ((HANDLE_OBJECT *)lpThreadInfo->hObjectMapped)->GetParent();

    INET_ASSERT(hConnectMapped);

    //
    // Finally get the Internet Object, so we can query proxy information
    //  out of it.
    //

    pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *) hConnectMapped;

    pInternet = (INTERNET_HANDLE_OBJECT *)
                    ((INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped)->GetParent();

    INET_ASSERT(pInternet);

    {

        AUTO_PROXY_ASYNC_MSG proxyInfoQuery(
                                    INTERNET_SCHEME_GOPHER,
                                    pConnect->GetURL(),
                                    lstrlen(pConnect->GetURL()),
                                    ViewInfo->SessionInfo->Host,
                                    lstrlen(ViewInfo->SessionInfo->Host),
                                    (INTERNET_PORT) ViewInfo->SessionInfo->Port
                                    );

        AUTO_PROXY_ASYNC_MSG *pProxyInfoQuery;

        proxyInfoQuery.SetBlockUntilCompletetion(TRUE);

        pProxyInfoQuery = &proxyInfoQuery;

        error = pInternet->GetProxyInfo(
                                &pProxyInfoQuery
                                );

        if ( error != ERROR_SUCCESS )
        {
            goto quit;
        }

        if (pProxyInfoQuery->IsUseProxy() &&
            pProxyInfoQuery->GetProxyScheme() == INTERNET_SCHEME_SOCKS &&
            pProxyInfoQuery->_lpszProxyHostName )
        {
            //
            //  If there is Socks enabled, then turned it on.
            //

            error = ViewInfo->BufferInfo->Socket->EnableSocks(
                                                               pProxyInfoQuery->_lpszProxyHostName,
                                                               pProxyInfoQuery->_nProxyHostPort
                                                               );
        }

        if ( pProxyInfoQuery && pProxyInfoQuery->IsAlloced() )
        {
            delete pProxyInfoQuery;
            pProxyInfoQuery = NULL;
        }

        if ( error != ERROR_SUCCESS )
        {
            goto quit;
        }

    }


    error = ViewInfo->BufferInfo->Socket->Connect(
                          GetTimeoutValue(INTERNET_OPTION_CONNECT_TIMEOUT),
                          GetTimeoutValue(INTERNET_OPTION_CONNECT_RETRIES),
                          SF_INDICATE | asyncFlags
                          );

    if (error == ERROR_SUCCESS) {

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("GopherConnect(): ConnectSocket() returns socket %#x\n",
                    //ViewInfo->BufferInfo->ConnectedSocket.Socket
                    ViewInfo->BufferInfo->Socket->GetSocket()
                    ));

        //
        // we have made a connection with the server. Set the receive timeout.
        // If this fails for any reason, ignore it (although the socket is
        // probably bad if this is true)
        //

        ViewInfo->BufferInfo->Socket->SetTimeout(
                                RECEIVE_TIMEOUT,
                                GetTimeoutValue(INTERNET_OPTION_RECEIVE_TIMEOUT)
                                );
    } else {

        DEBUG_PRINT(SOCKETS,
                    ERROR,
                    ("GopherConnect(): ConnectSocket() returns %d\n",
                    error
                    ));

    }

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
GopherDisconnect(
    IN LPVIEW_INFO ViewInfo,
    IN BOOL AbortConnection
    )

/*++

Routine Description:

    Disconnects from the gopher server if the session is not flagged as
    persistent. The socket is closed

Arguments:

    ViewInfo        - pointer to VIEW_INFO containing pointer to SESSION_INFO
                      which describes connection to gopher server

    AbortConnection - TRUE if the connection is to be terminated, even if it is
                      a persistent connection

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                Dword,
                "GopherDisconnect",
                "%#x, %B",
                ViewInfo,
                AbortConnection
                ));

    DWORD error;
    LPSESSION_INFO sessionInfo;

    INET_ASSERT(ViewInfo->SessionInfo != NULL);

    sessionInfo = ViewInfo->SessionInfo;
    if (!(sessionInfo->Flags & SI_PERSISTENT) || AbortConnection) {

        LPBUFFER_INFO bufferInfo;

        INET_ASSERT(ViewInfo->BufferInfo != NULL);

        bufferInfo = ViewInfo->BufferInfo;

        DEBUG_PRINT(SOCKETS,
                    INFO,
                    ("GopherDisconnect(): closing socket %#x\n",
                    //bufferInfo->ConnectedSocket.Socket
                    bufferInfo->Socket->GetSocket()
                    ));

        error = bufferInfo->Socket->Disconnect();

        if (error != ERROR_SUCCESS) {

            DEBUG_PRINT(SOCKETS,
                        ERROR,
                        ("GopherDisconnect(): Disconnect(%#x) returns %d\n",
                        ViewInfo->BufferInfo->Socket->GetSocket(),
                        error
                        ));

        }
    } else {
        error = ERROR_SUCCESS;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
GopherSendRequest(
    IN LPVIEW_INFO ViewInfo
    )

/*++

Routine Description:

    Sends a gopher request to the server we are currently connected to. The
    request is the selector string used to tell the gopher server what to return

Arguments:

    ViewInfo    - pointer to VIEW_INFO describing request. Contains pointer to
                  CR/LF terminated text string gopher request

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                Dword,
                "GopherSendRequest",
                "%#x",
                ViewInfo
                ));

    DWORD error;

    //
    // determine sync or async
    //

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    DWORD asyncFlags;

    asyncFlags = 0;
    //asyncFlags = lpThreadInfo->IsAsyncWorkerThread ? SF_NON_BLOCKING : 0;

    error = ViewInfo->BufferInfo->Socket->Send(ViewInfo->Request,
                                               ViewInfo->RequestLength,
                                               SF_INDICATE
                                               );

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
GopherReceiveResponse(
    IN OUT LPVIEW_INFO ViewInfo,
    OUT LPDWORD BytesReceived
    )

/*++

Routine Description:

    This function is called iteratively to receive the gopher response data. The
    first time this function is called, we determine the type of response -
    success or failure, gopher0 or gopher+.

    The buffer information passed in can refer to a buffer that we maintain
    internally (for directories), or to a buffer that the caller supplies to the
    GopherReadFile() function. In the latter case, the buffer address and length
    may be 0.

    This function assumes that directory responses will be received into a
    buffer which we allocate in this function, and that file requests are
    received into caller-supplied (user-supplied) buffers

Arguments:

    ViewInfo        - pointer to VIEW_INFO structure (including BUFFER_INFO)
                      describing the gopher request and the response from the
                      server

    BytesReceived   - number of bytes received in this response

Return Value:

     DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_EXTENDED_ERROR
                    The server sent us some form of error text. The app should
                    use InternetGetLastResponseInfo() to get the text

--*/

{
    DEBUG_ENTER((DBG_SOCKETS,
                Dword,
                "GopherReceiveResponse",
                "%#x, %#x",
                ViewInfo,
                BytesReceived
                ));

    LPBUFFER_INFO bufferInfo;
    DWORD error;
    int bufferLength;
    LPBYTE buffer;
    LPBYTE responseBuffer;
    int bytesReceived;
    BOOL discardBuffer;
    BOOL terminateConnection;
    BOOL checkResponse;

    *BytesReceived = 0;

    bytesReceived = 0;
    terminateConnection = FALSE;
    checkResponse = FALSE;

    //
    // variables for SocketReceive()
    //

    LPVOID psrBuffer;
    DWORD srLength;
    DWORD srLeft;
    DWORD srReceived;
    BOOL eof;

    bufferInfo = ViewInfo->BufferInfo;

    INET_ASSERT(bufferInfo != NULL);

    //
    // determine sync or async
    //

    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto cleanup;
    }

    DWORD asyncFlags;

    asyncFlags = 0;
    //asyncFlags = lpThreadInfo->IsAsyncWorkerThread ? SF_NON_BLOCKING : 0;

    //
    // if this is the first receive for this buffer then figure out what we
    // received. At this point we may have no buffer (GopherOpenFile()) so we
    // set up the BUFFER_INFO with the required information for the next
    // GopherReadFile()
    //

    if (bufferInfo->Flags & BI_FIRST_RECEIVE) {

        //
        // determine what we have received
        //

        psrBuffer = (LPVOID)bufferInfo->ResponseInfo;
        srLength = sizeof(bufferInfo->ResponseInfo);
        srLeft = sizeof(bufferInfo->ResponseInfo);
        srReceived = 0;

        error = bufferInfo->Socket->Receive(&psrBuffer,
                                            &srLength,
                                            &srLeft,
                                            &srReceived,
                                            0,    // dwExtraSpace
                                            SF_INDICATE,
                                            &eof
                                            );
        if (error != ERROR_SUCCESS) {
            goto cleanup;
        } else if (srReceived == 0) {

            INET_ASSERT(eof);

            //
            // the server has closed the connection already.  We either end up
            // with a zero-length file, or return ERROR_NO_MORE_FILES from a
            // directory listing
            //

            DEBUG_PRINT(SOCKETS,
                        ERROR,
                        ("SocketsReceive() returns 0 bytes on initial read\n"
                        ));

            bufferInfo->Flags |= BI_RECEIVE_COMPLETE;
            goto cleanup;
        }

        //
        // BytesRemaining is the number of data bytes in ResponseInfo
        //

        bufferInfo->BytesRemaining = srReceived;
        bufferInfo->DataBytes = (LPBYTE)bufferInfo->ResponseInfo;

        if (ViewInfo->Flags & VI_GOPHER_PLUS) {

            //
            // gopher+ request: expecting gopher+ response: +/-#\r\n
            //

            if (bufferInfo->ResponseInfo[0] == GOPHER_PLUS_ERROR_INDICATOR) {
                bufferInfo->Flags |= BI_ERROR_RESPONSE;
            } else if (bufferInfo->ResponseInfo[0] != GOPHER_PLUS_SUCCESS_INDICATOR) {

                //
                // buffer does not start with + or -; assume we received a
                // gopher0 response, and down-grade the request
                //

                ViewInfo->Flags &= ~VI_GOPHER_PLUS;

                DEBUG_PRINT(SOCKETS,
                            WARNING,
                            ("Gopher+ request resulted in gopher0 response!\n"
                            ));

            }
        } else if ((ViewInfo->ViewType != ViewTypeFile)
        && (bufferInfo->ResponseInfo[0] == GOPHER_PLUS_ERROR_INDICATOR)) {

            //
            // if this is a gopher+ error response then we promote the request/
            // response to gopher+ and handle the error below. If it actually
            // turns out that someone has invented a locator that starts '-'
            // then we will again down-grade the request
            //

            bufferInfo->Flags |= BI_ERROR_RESPONSE;
            ViewInfo->Flags |= VI_GOPHER_PLUS;

            DEBUG_PRINT(SOCKETS,
                        WARNING,
                        ("Gopher0 request resulted in gopher+ response!\n"
                        ));

        }

        //
        // if we still think the request/response is gopher+ then extract the
        // response length
        //

        if (ViewInfo->Flags & VI_GOPHER_PLUS) {

            BOOL ok;
            LPSTR pNumber;

            pNumber = &bufferInfo->ResponseInfo[1];
            ok = ExtractInt(&pNumber,
                            0,
                            &bufferInfo->ResponseLength
                            );
            if (!ok || (*pNumber != '\r') || (*(pNumber + 1) != '\n')) {
                ViewInfo->Flags &= ~VI_GOPHER_PLUS;

                DEBUG_PRINT(SOCKETS,
                            WARNING,
                            ("Gopher+ request resulted in gopher0 response!\n"
                            ));

                //
                // probably isn't an error either
                //

                bufferInfo->Flags &= ~BI_ERROR_RESPONSE;
            } else {

                int numberLength;

                //
                // this is the number of bytes we copied to the ResponseInfo
                // buffer that belong in the response proper
                //

                numberLength = (int) (pNumber - bufferInfo->ResponseInfo) + 2;
                bufferInfo->DataBytes = (LPBYTE)&bufferInfo->ResponseInfo[numberLength];
                bufferInfo->BytesRemaining -= numberLength;
            }
        }

        //
        // if we are receiving a gopher0 (directory) response then we need to
        // check below if we really received an error
        //

        if (!(ViewInfo->Flags & VI_GOPHER_PLUS)) {
            checkResponse = TRUE;
        }

        //
        // no longer the first time receive
        //

        bufferInfo->Flags &= ~BI_FIRST_RECEIVE;
    }

    //
    // either get the user buffer pointer, or allocate/resize a moveable buffer
    // (for directory listings and errors)
    //
    // N.B. A special case here is an error received in response to a gopher0
    // directory request: we don't know at this point if a gopher0 directory
    // request has resulted in an error, but we have to allocate a buffer
    // anyway, so we will determine error status after we have received the
    // data
    //

    discardBuffer = FALSE;
    if (!(bufferInfo->Flags & (BI_BUFFER_RESPONSE | BI_ERROR_RESPONSE))) {
        buffer = bufferInfo->Buffer;
        bufferLength = bufferInfo->BufferLength;

        //
        // if we are given a zero length user buffer then quit now; the bytes
        // received parameter has already been zeroed
        //

        if (bufferLength == 0) {
            goto quit;
        }
    } else {

        //
        // if we know how much data is in the response AND it is less than the
        // default length, then allocate a buffer that will exactly fit the
        // response. Otherwise allocate a buffer large enough to fit the default
        // response length
        //

        if (bufferInfo->ResponseLength > 0) {
            bufferLength = min(bufferInfo->ResponseLength,
                               DEFAULT_RESPONSE_BUFFER_LENGTH
                               );
        } else {

            //
            // the response length is -1 or -2, or its a gopher0 response: we
            // don't know how large it is
            //

            bufferLength = DEFAULT_RESPONSE_BUFFER_LENGTH;
        }

        //
        // if this is the first time we have called this function, allocate the
        // buffer, else grow it to the new size
        //

        bufferInfo->Buffer = (LPBYTE)ResizeBuffer(bufferInfo->Buffer,
                                                  bufferInfo->BufferLength
                                                  + bufferLength,
                                                  FALSE
                                                  );
        if (bufferInfo->Buffer == NULL) {
            terminateConnection = TRUE;

            INET_ASSERT(FALSE);

            goto last_error_exit;
        }

        bufferInfo->Flags |= BI_OWN_BUFFER;

        //
        // start receiving the next chunk at the end of the previous one
        //

        buffer = bufferInfo->Buffer + bufferInfo->BufferLength;
    }

    //
    // if we haven't already received all the data (in DetermineGopherResponse)
    // then receive the next chunk
    //

    responseBuffer = buffer;
    bytesReceived = bufferLength;

    //
    // receive the data. N.B. Don't change this loop without first examining
    // the checks below
    //

    int n;

    for (n = 0; bufferLength > 0; ) {

        //
        // if there is still data to copy in ResponseInfo then copy that
        //

        if (bufferInfo->BytesRemaining) {
            n = min(bufferInfo->BytesRemaining, bufferLength);
            memcpy(buffer, bufferInfo->DataBytes, n);
            bufferInfo->BytesRemaining -= n;
            bufferInfo->DataBytes += n;
        } else {

            //
            // if there is data left to copy in the look-ahead buffer that we
            // allocated in DetermineGopherResponse() then copy that data to
            // the response buffer
            //

            psrBuffer = (LPVOID)buffer;
            srLength = bufferLength;
            srLeft = bufferLength;
            srReceived = 0;

            error = bufferInfo->Socket->Receive(&psrBuffer,
                                                &srLength,
                                                &srLeft,
                                                &srReceived,
                                                0,    // dwExtraSpace
                                                SF_INDICATE,
                                                &eof
                                                );
            if (error != ERROR_SUCCESS) {
                discardBuffer = TRUE;
                terminateConnection = TRUE;
                goto cleanup;
            }
            n = srReceived;
            if (n == 0) {
                break;
            }
        }
        bufferLength -= n;
        buffer += n;
    }

    //
    // at this point we have one of the following:
    //
    //  * we reached the end of the response (n == 0)
    //  * we reached the end of the buffer (n != 0 && bufferLength == 0)
    //

    //
    // get the actual number of bytes received for this iteration
    //

    bytesReceived -= bufferLength;

    //
    // if we are receiving a gopher0 directory response then we need to check
    // whether we actually received an error
    //

    if (checkResponse) {

        //
        // allow for long locators (!)
        //

        char locator[2 * MAX_GOPHER_LOCATOR_LENGTH + 1];
        LPSTR destination;
        LPSTR source;
        DWORD destinationLength;
        DWORD sourceLength;

        //
        // we have a gopher0 error if the response starts with a '3' but there
        // is only one locator, or the response data is unformatted
        //

        //
        // N.B. We are making an assumption here that the locator(s) in the
        // response is(are) not larger than our buffer above. This should be a
        // safe assumption, but this test could fail if we get a large locator
        // but then a large locator will probably break all other gopher
        // clients? The reason we copy the locator is so that we get a zero-
        // terminated string for IsValidLocator(), and CopyToEol() compresses
        // multiple carriage-returns which some servers are stupid enough to
        // return
        //

        source = (LPSTR)responseBuffer;
        sourceLength = bytesReceived;
        destination = locator;
        destinationLength = sizeof(locator);
        if (CopyToEol(&destination,
                      &destinationLength,
                      &source,
                      &sourceLength)) {

            if (IsValidLocator(locator, sizeof(locator))) {

                //
                // response contains at least one valid locator. If it starts
                // with the gopher0 error indicator ('3') and its the only one
                // then this is an error response (although it *could* be the
                // one and only locator describing the directory, and the admin
                // decided to make it an error for some reason. We can't
                // differentiate in this case)
                //

                if (locator[0] == GOPHER_CHAR_ERROR) {
                    destination = locator;
                    destinationLength = sizeof(locator);
                    if (!CopyToEol(&destination,
                                   &destinationLength,
                                   &source,
                                   &sourceLength)) {
                        bufferInfo->Flags |= BI_ERROR_RESPONSE;
                    } else if (!IsValidLocator(locator, sizeof(locator))) {
                        bufferInfo->Flags |= BI_ERROR_RESPONSE;
                    }
                }
            } else {

                //
                // response doesn't contain a valid locator: must be an error
                //

                bufferInfo->Flags |= BI_ERROR_RESPONSE;
            }
        } else {

            //
            // we are receiving 4K but couldn't find an end-of-line? Must be an
            // error
            //

            bufferInfo->Flags |= BI_ERROR_RESPONSE;
        }
    }

    //
    // if we finished receivig the response then we can set the error code or
    // shrink the buffer
    //

    if ((n == 0)
    || (bytesReceived == bufferInfo->ResponseLength)
    || (bufferInfo->Flags & BI_ERROR_RESPONSE)) {

        //
        // no more data in the response. We have completed the transfer
        //

        bufferInfo->Flags |= BI_RECEIVE_COMPLETE;

        //
        // if this response type is terminated with a line containing only a dot
        // then remove the dot
        //

        if (bufferInfo->Flags & BI_DOT_AT_END) {

            //
            // BUGBUG - this is insufficient - need to check for different line
            //          termination schemes ('\n', '\r\r\n', '\r\n', etc.)
            //

            if ((bytesReceived >= 3) && (memcmp(buffer - 3, ".\r\n", 3) == 0)) {
                bytesReceived -= 3;
            }
            bufferInfo->Flags &= ~BI_DOT_AT_END;
        }

        //
        // if we received an error response then we must set the last error
        // response data
        //

        if (bufferInfo->Flags & BI_ERROR_RESPONSE) {
            if (bytesReceived > 0) {
                InternetSetLastError(0,
                                     (LPSTR)responseBuffer,
                                     bytesReceived,
                                     SLE_ZERO_TERMINATE
                                     );

                //
                // let the app know that it needs to call InternetGetLastResponseInfo()
                // to retrieve the error text
                //

                error = ERROR_INTERNET_EXTENDED_ERROR;
            } else {
                error = ERROR_SUCCESS;
            }

            //
            // we have copied the data in the error response, no more need for
            // the buffer
            //

            discardBuffer = TRUE;

            //
            // need to indicate to the caller that they need to get the last
            // error response
            //

        } else {
            error = ERROR_SUCCESS;
        }
    } else {

        //
        // this chunk received OK, more to go
        //

        error = ERROR_SUCCESS;
    }

cleanup:

    //
    // if we no longer need the buffer then discard it, else if we have completed
    // receiving the response, shrink the buffer to free up any unused space
    //

    if (bufferInfo->Flags & BI_OWN_BUFFER) {

        DWORD newBufferLength;
        BOOL resize;

        if (discardBuffer) {
            newBufferLength = 0;
            resize = TRUE;
        } else {

            //
            // update the amount of data received - i.e. the number of bytes in
            // the buffer (excluding header info)
            //

            bufferInfo->BufferLength += bytesReceived;
            if (bufferInfo->Flags & BI_RECEIVE_COMPLETE) {
                newBufferLength = bufferInfo->BufferLength;
                resize = TRUE;

                DEBUG_PRINT(SOCKETS,
                            ERROR,
                            ("received 0 bytes in response\n"
                            ));

            } else {
                resize = FALSE;
            }
        }

        //
        // if resize is TRUE then we are either shrinking the data buffer to
        // exclude any unused space, or we are shrinking it to nothing because
        // we have no data or it is being discarded
        //

        if (resize) {
            bufferInfo->Buffer = (LPBYTE)ResizeBuffer(bufferInfo->Buffer,
                                                      newBufferLength,
                                                      FALSE
                                                      );

            if (newBufferLength == 0) {

                //
                // we no longer own the buffer
                //

                bufferInfo->Flags &= ~BI_OWN_BUFFER;

                //
                // DEBUG version: ensure that the buffer has really been freed
                //

                INET_ASSERT(bufferInfo->Buffer == NULL);

            }
        }
    }

    //
    // if this is a gopher+ response and we know the length of the response then
    // reduce the outstanding size of the response by the amount we received
    //

    if (bufferInfo->ResponseLength > 0) {
        bufferInfo->ResponseLength -= bytesReceived;
    }

    //
    // if we completed the response and the connection is not persistent, or an
    // error occurred, close the connection
    //

    if ((bufferInfo->Flags & BI_RECEIVE_COMPLETE) || (error != ERROR_SUCCESS)) {

        //
        // let the app know we have finished receiving data
        //

        InternetIndicateStatus(INTERNET_STATUS_RESPONSE_RECEIVED,
                               &bytesReceived,
                               sizeof(bytesReceived)
                               );

        GopherDisconnect(ViewInfo, terminateConnection);
    }

    //
    // if we didn't receive error text from the server then we return the amount
    // of data received. This is only interesting to ReadData()
    //

    if (!(bufferInfo->Flags & BI_ERROR_RESPONSE)) {
        *BytesReceived = bytesReceived;
    }

quit:

    DEBUG_LEAVE(error);

    return error;

last_error_exit:

    error = GetLastError();

    INET_ASSERT(error != ERROR_SUCCESS);

    goto cleanup;
}
