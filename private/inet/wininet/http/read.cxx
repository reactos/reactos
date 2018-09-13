/*++

Copyright (c) 1994-1997 Microsoft Corporation

Module Name:

    read.cxx

Abstract:

    This file contains the implementation of the HttpReadData API.

    Contents:
        HttpReadData
        CFsm_HttpReadData::RunSM
        HTTP_REQUEST_HANDLE_OBJECT::HttpReadData_Fsm
        HTTP_REQUEST_HANDLE_OBJECT::ReadData
        CFsm_ReadData::RunSM
        HTTP_REQUEST_HANDLE_OBJECT::ReadData_Fsm
        HTTP_REQUEST_HANDLE_OBJECT::QueryDataAvailable
        CFsm_HttpQueryAvailable::RunSM
        HTTP_REQUEST_HANDLE_OBJECT::QueryAvailable_Fsm
        HTTP_REQUEST_HANDLE_OBJECT::DrainResponse
        CFsm_DrainResponse::RunSM
        HTTP_REQUEST_HANDLE_OBJECT::DrainResponse_Fsm

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

    Modified to make HttpReadData remotable. madana (2/8/95)

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "httpp.h"

//
// private prototypes
//

PRIVATE
VOID
FilterHeaders(
    IN LPSTR lpszHeaderInfo,
    OUT LPDWORD lpdwLen
    );

//
// functions
//


DWORD
HttpReadData(
    IN HINTERNET hRequest,
    OUT LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead,
    IN DWORD dwSocketFlags
    )

/*++

Routine Description:

    Reads a block of data from an outstanding HTTP request

    Assumes: 1. this function can only be called from InternetReadFile() which
        globally validates parameters for all Internet data read
        functions

         2. We will never get a request for 0 bytes at this level. This
        request will have been handled in InternetReadFile()

Arguments:

    hRequest                - mapped HTTP request handle

    lpBuffer                - pointer to the buffer to receive the data

    dwNumberOfBytesToRead   - number of bytes to read into lpBuffer

    lpdwNumberOfBytesRead   - number of bytes read into lpBuffer

    dwSocketFlags           - controlling socket operation

Return Value:

    TRUE - The data was read successfully. lpdwNumberOfBytesRead points to the
    number of BYTEs actually read. This value will be set to zero
    when the transfer has completed.

    FALSE - The operation failed. Error status is available by calling
    GetLastError().

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HttpReadData",
                 "%#x, %#x, %d, %#x, %#x",
                 hRequest,
                 lpBuffer,
                 dwNumberOfBytesToRead,
                 lpdwNumberOfBytesRead,
                 dwSocketFlags
                 ));

    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT* pRequest =
        (HTTP_REQUEST_HANDLE_OBJECT*) hRequest;

    if (pRequest->IsReadRequest()) {

        //
        // Invoke ReadLoop fsm only if the read and write positions
        // are different, otherwise older code is more efficient.
        //

        error = DoFsm (new CFsm_ReadLoop (pRequest, dwSocketFlags,
            (PBYTE) lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead));

    } else {

        error = DoFsm(new CFsm_HttpReadData(lpBuffer,
                                            dwNumberOfBytesToRead,
                                            lpdwNumberOfBytesRead,
                                            dwSocketFlags,
                                            pRequest
                                           ));
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_HttpReadData::RunSM(
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
                 "CFsm_HttpReadData::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;
    CFsm_HttpReadData * stateMachine = (CFsm_HttpReadData *)Fsm;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();
    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pRequest->HttpReadData_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::HttpReadData_Fsm(
    IN CFsm_HttpReadData * Fsm
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
                 "HTTP_REQUEST_HANDLE_OBJECT::HttpReadData_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_HttpReadData & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if (fsm.GetState() == FSM_STATE_INIT) {

        if (!IsValidHttpState(READ)) {
            error = ERROR_INTERNET_INCORRECT_HANDLE_STATE;
            goto quit;
        }
        error = ReadData(fsm.m_lpBuffer,
                         fsm.m_dwNumberOfBytesToRead,
                         fsm.m_lpdwNumberOfBytesRead,
                         FALSE, // BUGBUG RFirthRemove on chkin
                         fsm.m_dwSocketFlags
                         );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }
    if (IsCacheWriteInProgress()) {
        if (*fsm.m_lpdwNumberOfBytesRead == 0) {

            DEBUG_PRINT(CACHE,
                        INFO,
                        ("Cache write complete\r\n"
                        ));

            LocalEndCacheWrite((error == ERROR_SUCCESS)
                               && (GetBytesInSocket() == 0));
        } else if (!HaveReadFileExData()) {

            INET_ASSERT(!IsCacheReadInProgress());

            if (WriteCache((LPBYTE)fsm.m_lpBuffer,
                           *fsm.m_lpdwNumberOfBytesRead) != ERROR_SUCCESS) {

                DEBUG_PRINT(CACHE,
                            ERROR,
                            ("Error in Cache write\n"
                            ));

                LocalEndCacheWrite(FALSE);
            }
        }
    }

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    PERF_LOG(PE_TRACE, 0x1002);

    DEBUG_LEAVE(error);

    return error;
}

//
// HTTP_REQUEST_HANDLE_OBJECT methods
//


DWORD
HTTP_REQUEST_HANDLE_OBJECT::ReadData(
    OUT LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead,
    IN BOOL fNoAsync, // BUGBUG RFirthRemove on DrainSocket checkin
    IN DWORD dwSocketFlags
    )

/*++

Routine Description:

    HTTP_REQUEST_HANDLE_OBJECT ReadData method

    Reads data into users buffer. Reads from header buffer if data exists
    there, or reads from the socket

Arguments:

    lpBuffer                - pointer to users buffer

    dwNumberOfBytesToRead   - size of buffer/number of bytes to read

    lpdwNumberOfBytesRead   - pointer to returned number of bytes read

    fNoAsync                - TRUE if we want to override defaults and have
                              no Async Read.

    dwSocketFlags           - controlling socket operation

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::ReadData",
                 "%#x, %d, %#x, %B, %#x",
                 lpBuffer,
                 dwNumberOfBytesToRead,
                 lpdwNumberOfBytesRead,
                 fNoAsync,
                 dwSocketFlags
                 ));

    DWORD error = DoFsm(new CFsm_ReadData(lpBuffer,
                                          dwNumberOfBytesToRead,
                                          lpdwNumberOfBytesRead,
                                          fNoAsync,
                                          dwSocketFlags,
                                          this
                                          ));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_ReadData::RunSM(
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
                 "CFsm_ReadData::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;
    CFsm_ReadData * stateMachine = (CFsm_ReadData *)Fsm;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();
    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pRequest->ReadData_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::ReadData_Fsm(
    IN CFsm_ReadData * Fsm
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
                 "HTTP_REQUEST_HANDLE_OBJECT::ReadData_Fsm",
                 "%#x",
                 Fsm
                 ));

    PERF_LOG(PE_TRACE, 0x6001);

    CFsm_ReadData & fsm = *Fsm;
    DWORD error = ERROR_SUCCESS;

    if (fsm.GetState() == FSM_STATE_CONTINUE) {

        PERF_LOG(PE_TRACE, 0x6101);

        error = fsm.GetError();
        goto receive_continue;
    }

    fsm.m_dwBytesRead = 0;
    fsm.m_dwBufferLeft = fsm.m_dwNumberOfBytesToRead;
    fsm.m_nBytesCopied = 0;

    //
    // if there's no data then we're done
    //

    if (!IsData()) {

        DEBUG_PRINT(HTTP,
                    ERROR,
                    ("!IsData()\n"
                    ));

        SetState(HttpRequestStateReopen);

        INET_ASSERT(error == ERROR_SUCCESS);

        goto quit;
    }

    //
    // If using keep-alive, reduce output buffer so we don't over-read.
    //

    if (IsKeepAlive() && IsContentLength()) {
        if (_BytesRemaining == 0) {

            INET_ASSERT(error == ERROR_SUCCESS);

            PERF_LOG(PE_TRACE, 0x6102);

            goto done;
        }

        PERF_LOG(PE_TRACE, 0x6103);

        fsm.m_dwBufferLeft = min(fsm.m_dwBufferLeft, _BytesRemaining);
    }

    //
    // if there's data left in the response buffer then copy it
    //

    fsm.m_bEof = FALSE;

    if (IsBufferedData()) {

        DWORD amountToCopy = min(fsm.m_dwNumberOfBytesToRead, BufferDataAvailToRead());

        if (amountToCopy != 0) {

            PERF_LOG(PE_TRACE, 0x6104);

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("Copying %d (%#x) bytes from header buffer @ %#x - %d left\n",
                        amountToCopy,
                        amountToCopy,
                        BufferedDataStart(),
                        BufferDataAvailToRead() - amountToCopy
                        ));

            memcpy(fsm.m_lpBuffer, BufferedDataStart(), amountToCopy);
            ReduceDataAvailToRead(amountToCopy);
            fsm.m_dwBytesRead += amountToCopy;
            fsm.m_dwBufferLeft -= amountToCopy;
            fsm.m_nBytesCopied += amountToCopy;

            //
            // we don't update lpBuffer here. Receive() takes the address of
            // the start of the buffer
            //

        }

        //
        // if we exhausted all the buffer space, then we're done
        //

        if (fsm.m_dwBufferLeft == 0) {

            PERF_LOG(PE_TRACE, 0x6105);

            goto done;
        }
    }

    //
    // find out if we're async. Even though the handle was created for async I/O
    // the request may be satisfied immediately
    //

    DWORD asyncFlags;

    if ( fsm.m_fNoAsync )   // BUGBUG RFirthRemove on Checkin of DrainSocket
        asyncFlags = 0;
    else
        asyncFlags = (IsAsyncHandle()
                        && (fsm.m_dwBufferLeft > AvailableDataLength()))
                   ? SF_NON_BLOCKING
                   : 0
                   ;

    //
    // if we have data already received in the query buffer, then return that
    //

    if (HaveQueryData()) {

        PERF_LOG(PE_TRACE, 0x6106);

        DWORD nCopied;

        nCopied = CopyQueriedData((LPVOID)((LPBYTE)fsm.m_lpBuffer + fsm.m_dwBytesRead),
                                  fsm.m_dwBufferLeft
                                  );

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("Copied %d (%#x) bytes from query buffer @ %#x - %d left\n",
                    nCopied,
                    nCopied,
                    (LPBYTE)_QueryBuffer - _QueryOffset,
                    _QueryBytesAvailable
                    ));

        fsm.m_dwBytesRead += nCopied;
        fsm.m_dwBufferLeft -= nCopied;
        fsm.m_nBytesCopied += nCopied;
        if (fsm.m_dwBufferLeft == 0) {
            goto done;
        }
    }

    //
    // If the Chunk parser claims we're done, then we're done,
    //  stop ready and tell the reader
    //

    //if ( IsChunkEncoding() && IsChunkedEncodingFinished() )
    //{
    //    fsm.m_bEof = TRUE;
    //    goto done;
    //}


    if (HaveReadFileExData()) {
        PERF_LOG(PE_TRACE, 0x6107);
        *(LPBYTE)fsm.m_lpBuffer = GetReadFileExData();
        --fsm.m_dwNumberOfBytesToRead;
        --fsm.m_dwBufferLeft;
        ++fsm.m_dwBytesRead;

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("Copied 1 byte (%#x) from ReadFileEx buffer %#x\n",
                    (BYTE)_ReadFileExData & 0xff,
                    &_ReadFileExData
                    ));

        if (fsm.m_dwBufferLeft == 0) {
            goto done;
        }
    }

    //
    // If the Chunk parser claims we're done, then we're done,
    //  stop ready and tell the reader
    //

    if ( IsChunkEncoding() && IsChunkedEncodingFinished() )
    {
        PERF_LOG(PE_TRACE, 0x6108);
        fsm.m_bEof = TRUE;
        goto done;
    }

    //
    // we're about to check the socket. Make sure its valid to do so
    //

    //INET_ASSERT(_Socket != NULL);

    if ((_Socket == NULL) || !_Socket->IsOpen()) {

        //
        // socket was closed - no more data
        //

        //
        // there is no more data to be received on this object
        //

        SetData(FALSE);

        //
        // this object can now be re-used
        //

        SetState(HttpRequestStateReopen);
        fsm.m_bEof = TRUE;
        PERF_LOG(PE_TRACE, 0x6109);
        goto quit;
    }

read_again:

    fsm.m_nBytes = fsm.m_dwBytesRead;

    //
    // if we had a content-length and we don't think there is any data left to
    // read then we're done
    //

    if (IsContentLength() && (_BytesInSocket == 0)) {
        fsm.m_bEof = TRUE;
        PERF_LOG(PE_TRACE, 0x6110);
        goto done;
    }

    //
    // receive data into user's buffer. Because we don't own the buffer, we
    // cannot resize it
    //

    LPVOID lpBuffer;
    DWORD dwBytesToRead;
    DWORD dwBufferLeft;
    DWORD dwBytesRead;

    lpBuffer = fsm.m_lpBuffer;
    dwBytesToRead = fsm.m_dwNumberOfBytesToRead;
    dwBufferLeft = fsm.m_dwBufferLeft;
    dwBytesRead = fsm.m_dwBytesRead;

    //INET_ASSERT(!(fsm.m_dwSocketFlags & SF_NO_WAIT)
    //            ? (fsm.m_dwBufferLeft <= _BytesRemaining)
    //            : TRUE);

    PERF_LOG(PE_TRACE, 0x6111);

    if (IsBadNSServer() && !IsConnCloseResponse()) {
        SetBadNSReceiveTimeout();
    }

    error = _Socket->Receive(&fsm.m_lpBuffer,
                             &fsm.m_dwNumberOfBytesToRead,
                             &fsm.m_dwBufferLeft,
                             &fsm.m_dwBytesRead,
                             0,
                             SF_INDICATE
                             | ((fsm.m_dwSocketFlags & SF_NO_WAIT)
                                ? SF_NO_WAIT
                                : (IsChunkEncoding() ? 0 : SF_RECEIVE_ALL)),
                             &fsm.m_bEof
                             );

    //
    // only if we performed an asynchronous no-wait receive and there was no
    // data available in the socket will we get WSAEWOULDBLOCK. Make another
    // receive request, this time without no-wait. It will complete
    // asynchronously and the app must make another no-wait request
    //

    if (error == WSAEWOULDBLOCK) {

        PERF_LOG(PE_TRACE, 0x6112);

        INET_ASSERT(fsm.m_dwSocketFlags & SF_NO_WAIT);
        INET_ASSERT(!fsm.m_bEof);

        //
        // BUGBUG - IsAsyncHandle() || IsAsyncRequest()
        //

        if ((fsm.m_dwBytesRead == 0) && IsAsyncHandle()) {

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("Initiating wait-for-data (1-byte read)\n"
                        ));

            fsm.m_lpBuffer = (LPVOID)&_ReadFileExData;
            fsm.m_dwNumberOfBytesToRead = 1;
            fsm.m_dwBufferLeft = 1;
            fsm.m_dwSocketFlags &= ~SF_NO_WAIT;

            INET_ASSERT(!_HaveReadFileExData);

            SetReadFileExData();

            _ReadFileExData = 0;

            //INET_ASSERT(fsm.m_dwBufferLeft <= _BytesRemaining);

            PERF_LOG(PE_TRACE, 0x6113);

            if (IsBadNSServer() && !IsConnCloseResponse()) {
                SetBadNSReceiveTimeout();
            }

            error = _Socket->Receive(&fsm.m_lpBuffer,
                                     &fsm.m_dwNumberOfBytesToRead,
                                     &fsm.m_dwBufferLeft,
                                     &fsm.m_dwBytesRead,
                                     0,
                                     fsm.m_dwSocketFlags,
                                     &fsm.m_bEof
                                     );
            if (error == ERROR_SUCCESS) {

                PERF_LOG(PE_TRACE, 0x6114);

                BOOL fReadNothing = (fsm.m_dwBytesRead == 0 ? TRUE : FALSE);

                //
                // we have successfully read a single byte from the socket.
                //

                //INET_ASSERT(FALSE);

                fsm.m_lpBuffer = lpBuffer;
                fsm.m_dwNumberOfBytesToRead = dwBytesToRead;
                fsm.m_dwBufferLeft = dwBufferLeft;
                fsm.m_dwBytesRead = dwBytesRead;
                if (fReadNothing) {
                    // Don't copy if nothing was actually read.
                    ResetReadFileExData();
                }
                else {
                    *(LPBYTE)fsm.m_lpBuffer = GetReadFileExData();
                    --fsm.m_dwBufferLeft;
                    ++fsm.m_dwBytesRead;
                }

                //
                // BUGBUG - if socket unblocked already, should go round & read
                //          again, not just return 1 byte
                //

            }

            PERF_LOG(PE_TRACE, 0x6115);

        } else {

            PERF_LOG(PE_TRACE, 0x6116);

            DEBUG_PRINT(HTTP,
                        WARNING,
                        ("Not initiating wait-for-data: bytesRead = %d, asyncHandle = %B\n",
                        fsm.m_dwBytesRead,
                        IsAsyncHandle()
                        ));

            //
            // read data from buffers but nothing available from socket
            //

            error = ERROR_SUCCESS;
        }
    }

    if (error == ERROR_IO_PENDING) {
        PERF_LOG(PE_TRACE, 0x6117);
        goto quit_pending;
    }

receive_continue:

    PERF_LOG(PE_TRACE, 0x6118);

    //
    // if we timed-out while talking to 'bad' NS server (returns HTTP/1.1 but
    // content-length or chunked encoding info) then close the connection and
    // reset any RFX status. We return SUCCESS in this case
    //

    if ((error == ERROR_INTERNET_TIMEOUT) && IsBadNSServer()) {

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("Bad NS server: Closing connection %#x/%d on timeout\n",
                    _Socket ? _Socket->GetSocket() : -1,
                    _Socket ? _Socket->GetSourcePort() : -1
                    ));

        CloseConnection(TRUE);
        ResetReadFileExData();
        SetData(FALSE);
        _dwCurrentStreamPosition += fsm.m_dwBytesRead;
        fsm.m_bEof = TRUE;
        error = ERROR_SUCCESS;
        goto quit;
    }
    if (error == ERROR_SUCCESS) {
        if (IsContentLength()) {

            INET_ASSERT(fsm.m_dwBytesRead >= fsm.m_nBytes);

            _BytesInSocket -= fsm.m_dwBytesRead - fsm.m_nBytes;

            INET_ASSERT((int)_BytesInSocket >= 0);

            if ((int)_BytesInSocket < 0) {
                _BytesInSocket = 0;
            }
        }

        if ( IsChunkEncoding() && !(HaveReadFileExData()))
        {

            PERF_LOG(PE_TRACE, 0x6119);

            LPSTR lpszNewBuffer;
            DWORD dwNewBufferLength;
            DWORD dwBytesJustRead = (fsm.m_dwBytesRead - fsm.m_nBytes);

            error = _ctChunkInfo.ParseChunkInput(
                (((LPSTR) fsm.m_lpBuffer) + fsm.m_nBytesCopied),
                (fsm.m_dwBytesRead - fsm.m_nBytesCopied),
                &lpszNewBuffer,
                &dwNewBufferLength
                );

            //
            // hack - wait for more data
            //

            if ((error == ERROR_SUCCESS)
            && (dwNewBufferLength == 0)
            && !IsChunkedEncodingFinished()
            && (fsm.m_nBytesCopied == 0)) {

                PERF_LOG(PE_TRACE, 0x6120);

                fsm.m_dwBufferLeft += fsm.m_dwBytesRead;
                fsm.m_dwBytesRead = 0;

                struct fd_set read_fds;
                struct fd_set write_fds;
                struct fd_set except_fds;

                FD_ZERO(&read_fds);
                FD_ZERO(&write_fds);
                FD_ZERO(&except_fds);

                FD_SET(_Socket->GetSocket(), &read_fds);

                struct timeval to;

                to.tv_sec = 30;
                to.tv_usec = 0;
                int n = _I_select(1, &read_fds, &write_fds, &except_fds, &to);
                goto read_again;
            }

            fsm.m_dwBufferLeft += (fsm.m_dwBytesRead - fsm.m_nBytesCopied);
            fsm.m_dwBytesRead  -= (fsm.m_dwBytesRead - fsm.m_nBytesCopied);

            fsm.m_dwBufferLeft -= dwNewBufferLength;
            fsm.m_dwBytesRead  += dwNewBufferLength;
            fsm.m_nBytesCopied += dwNewBufferLength;

            INET_ASSERT(error == ERROR_SUCCESS); // I want to see this happen.

            if ( error != ERROR_SUCCESS )
            {
                goto quit;
            }

            if ( IsChunkedEncodingFinished() )
            {
                fsm.m_bEof = TRUE;
            }
        }
    } else {

        PERF_LOG(PE_TRACE, 0x6121);

        DEBUG_PRINT(HTTP,
                    ERROR,
                    ("error %d on socket %#x\n",
                    error,
                    _Socket->GetSocket()
                    ));

        //
        // socket error
        //

        SetState(HttpRequestStateError);

        //
        // cause connection to be closed/released
        //

        fsm.m_bEof = TRUE;
    }

done:

    //
    // only update bytes remaining, EOF and the current stream position values
    // if we're returning data. If we just completed reading ReadFileEx data
    // then don't update. The 1 byte of ReadFileEx data will be read on the next
    // read proper
    //

    if (HaveReadFileExData()) {
        goto quit;
    }

    //
    // whether the data came from the response buffer or the socket, if we have
    // a content-length, update the amount of data left to retrieve
    //

    if (IsChunkEncoding()
        && IsChunkedEncodingFinished()
        && (_QueryBytesAvailable == 0)
        && (BufferDataAvailToRead() == 0)) {
        fsm.m_bEof = TRUE;
    } else if (IsKeepAlive() && IsContentLength()) {
        _BytesRemaining -= fsm.m_dwBytesRead;

        INET_ASSERT((int)_BytesRemaining >= 0);

        //
        // if we have read all the entity-body then we can release the keep-alive
        // connection, or close the socket
        //

        if (_BytesRemaining == 0) {
            fsm.m_bEof = TRUE;
        }
    }

    DEBUG_PRINT(HTTP,
                INFO,
                ("read %d bytes\n",
                fsm.m_dwBytesRead
                ));

    _dwCurrentStreamPosition += fsm.m_dwBytesRead;

    //
    // if we reached the end of the connection - either the end of the server
    // connection for real, or we received all indicated data on a keep-alive
    // connection - then close the connection
    //

    if (fsm.m_bEof) {

        PERF_LOG(PE_TRACE, 0x6122);

        //
        // if we don't need to keep hold of the connection, release it. In the
        // case of multi-part authentication (NTLM) over keep-alive connection
        // we need to keep the connection. With Kerberos, we don't need to keep
        // the connection.
        //

        if (GetAuthState() != AUTHSTATE_CHALLENGE) {

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("end of data - freeing connection %#x (Auth State = %s)\n",
                        _Socket ? _Socket->GetSocket() : 0,
                        (GetAuthState() == AUTHSTATE_NONE)
                            ? "NONE"
                            : ((GetAuthState() == AUTHSTATE_NEGOTIATE)
                                ? "NEGOTIATE"
                                : ((GetAuthState() == AUTHSTATE_CHALLENGE)
                                    ? "CHALLENGE"
                                    : "?"))
                        ));

            CloseConnection(FALSE);

        } else {

            // AUTHSTATE_CHALLENGE - check if request is through proxy or is kerberos.

            // When IsRequestUsingProxy returns TRUE, there are three types of connections possible:
            // 1) http request forwarded by the proxy to the server
            // 2) connect request to proxy to establish https tunnel
            // 3) using https tunnel through proxy to the server

            // I believe the various methods return:
            //                                              http    conn.   tunnel
            // IsRequestUsingProxy                          1      1        1
            // IsViaProxy                                   1      1        0
            // IsTunnel                                     0      1        0
            // IsTalkingToSecureServerViaProxy              0      0        1

            INET_ASSERT(_pAuthCtx->GetSchemeType() != AUTHCTX::SCHEME_NEGOTIATE);

            if (GetAuthCtx()->GetSchemeType() == AUTHCTX::SCHEME_KERBEROS)
            {
                DEBUG_PRINT(HTTP,
                            INFO,
                            ("freeing connection - kerberos and auth state challenge\n"
                            ));
                CloseConnection(FALSE);
            }                
            else if (IsRequestUsingProxy()
                && !(IsTunnel() || IsTalkingToSecureServerViaProxy())
                && (_pAuthCtx->GetFlags() & PLUGIN_AUTH_FLAGS_KEEP_ALIVE_NOT_REQUIRED)
                && !_pAuthCtx->_fIsProxy)
            {
                // Ordinarily, if the auth state is AUTHSTATE_CHALLENGE we wish to keep
                // the current connection open (keep alive) so that the response will go
                // out on the same socket. NTLM, which requires keep-alive, does not
                // work when going through a proxy. DPA on the other hand can work through
                // a proxy. In the case that the proxy does not return keep-alive with the
                // challenge (Catapult appears to be the only proxy that does) we want to
                // close the socket to ensure that it is not subsequently used for the response.

                DEBUG_PRINT(HTTP,
                            INFO,
                            ("freeing connection - auth state challenge\n"
                            ));
                CloseConnection(FALSE);
            }
            else
            {
                //  Keep alive required - don't close socket.
                DEBUG_PRINT(HTTP,
                            INFO,
                            ("not freeing connection - auth state challenge\n"
                            ));
            }

        }

        //
        // there is no more data to be received on this object
        //

        SetData(FALSE);

        //
        // this object can now be re-used
        //

        SetState(HttpRequestStateReopen);
    }

quit:

    //
    // update the amount of data returned then we're outta here
    //

    *fsm.m_lpdwNumberOfBytesRead = fsm.m_dwBytesRead;

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

quit_pending:

    PERF_LOG(PE_TRACE, 0x6002);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryDataAvailable(
    OUT LPDWORD lpdwNumberOfBytesAvailable
    )

/*++

Routine Description:

    Determines how much data is available to be read by the caller

    BUGBUG - need cache case

Arguments:

    lpdwNumberOfBytesAvailable  - returned number of bytes available

Return Value:

    DWORD
    Success - ERROR_SUCCESS

    Failure - ERROR_INTERNET_INCORRECT_HANDLE_STATE

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::QueryDataAvailable",
                 "%#x",
                 lpdwNumberOfBytesAvailable
                 ));

    DWORD error = DoFsm(new CFsm_HttpQueryAvailable(lpdwNumberOfBytesAvailable,
                                                    this
                                                    ));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_HttpQueryAvailable::RunSM(
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
                 "CFsm_HttpQueryAvailable::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;
    CFsm_HttpQueryAvailable * stateMachine = (CFsm_HttpQueryAvailable *)Fsm;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();
    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pRequest->QueryAvailable_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryAvailable_Fsm(
    IN CFsm_HttpQueryAvailable * Fsm
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
                 "QueryAvailable_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_HttpQueryAvailable & fsm = *Fsm;
    DWORD error = fsm.GetError();
    DWORD bytesAvailable = 0;

    if (fsm.GetState() == FSM_STATE_CONTINUE) {
        goto fsm_continue;
    }

    INET_ASSERT(fsm.GetState() == FSM_STATE_INIT);

    if (IsReadRequest()) {
        *fsm.m_lpdwNumberOfBytesAvailable = 0;
        if (_Socket != NULL) {

            //
            // Invoked ReadLoop fsm only if the read and write positions
            // are different, otherwise older code is more efficient.
            //
            error = DoFsm (new CFsm_ReadLoop (this, 0, NULL, 1, NULL));

            if (error == ERROR_SUCCESS) {
                *fsm.m_lpdwNumberOfBytesAvailable = AvailableDataLength();
            }
        } else {

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("no socket\n"
                        ));

            fsm.m_bEof = TRUE;
        }
        goto done;
    }

    //
    // the handle must be readable
    //

    if (!IsValidHttpState(READ)) {
        error = ERROR_INTERNET_INCORRECT_HANDLE_STATE;
        goto quit;
    }

    fsm.m_bEof = FALSE;

    //
    // error must be ERROR_SUCCESS - we just read it out of FSM & didn't jump
    // anywhere
    //

    INET_ASSERT(error == ERROR_SUCCESS);

    //
    // first check if there is data to receive at all
    //

    if (IsData()) {

        //
        // if there's buffered data still available from receiving the headers,
        // then return that length, else query the information from the socket
        //

        if (IsBufferedData()) {
            bytesAvailable = BufferDataAvailToRead();

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("%d bytes available in buffer\n",
                        bytesAvailable
                        ));

        } else if (_Socket != NULL) {

            //
            // the rest of the data must be read from the socket
            //

            BOOL checkSocket;

            if (IsKeepAlive() && IsContentLength()) {
                checkSocket = ((int)_BytesInSocket > 0) ? TRUE : FALSE;
            } else if (IsChunkEncoding()) {
                checkSocket = !IsChunkedEncodingFinished();
            } else {
                checkSocket = TRUE;
            }
            if (checkSocket) {
                if (_QueryBuffer != NULL) {
                    bytesAvailable = _QueryBytesAvailable;
                    checkSocket = (bytesAvailable == 0) ? TRUE : FALSE;
                } else {
                    error = _Socket->AllocateQueryBuffer(&_QueryBuffer,
                                                         &_QueryBufferLength
                                                         );
                    if (error != ERROR_SUCCESS) {
                        checkSocket = FALSE;
                    }
                }
            } else if (IsKeepAlive() && IsContentLength() && (_BytesRemaining == 0)) {
                fsm.m_bEof = TRUE;
            } else if (IsChunkEncoding() && IsChunkedEncodingFinished()) {
                fsm.m_bEof = TRUE;
            }
            if (checkSocket) {

                INET_ASSERT(_Socket->IsValid());
                INET_ASSERT(_QueryBytesAvailable == 0);

                //
                // reset the query buffer offset
                //

                _QueryOffset = 0;

                //
                // don't create another FSM just for the DataAvailable2 wrapper.
                // If it ever becomes more than a call to Receive() then create
                // an FSM
                //

read_again:

                fsm.m_lpBuffer = _QueryBuffer;
                fsm.m_dwBufferLength = (IsKeepAlive() && IsContentLength())
                                     ? min(_BytesRemaining, _QueryBufferLength)
                                     : _QueryBufferLength;
                fsm.m_dwBufferLeft = fsm.m_dwBufferLength;

                //INET_ASSERT(fsm.m_dwBufferLeft <= _BytesRemaining);

                if (IsBadNSServer() && !IsConnCloseResponse()) {
                    SetBadNSReceiveTimeout();
                }

                error = _Socket->Receive(&fsm.m_lpBuffer,
                                         &fsm.m_dwBufferLength,
                                         &fsm.m_dwBufferLeft, // don't care about this
                                         &_QueryBytesAvailable,
                                         0,
                                         0,
                                         &fsm.m_bEof
                                         );
                if (error == ERROR_IO_PENDING) {
                    goto done;
                }

fsm_continue:

                if ((error == ERROR_INTERNET_TIMEOUT) && IsBadNSServer()) {

                    DEBUG_PRINT(HTTP,
                                INFO,
                                ("Bad NS server: Closing connection %#x/%d on timeout\n",
                                _Socket ? _Socket->GetSocket() : -1,
                                _Socket ? _Socket->GetSourcePort() : -1
                                ));

                    CloseConnection(TRUE);
                    _QueryBytesAvailable = 0;
                    error = ERROR_SUCCESS;
                }
                if (error == ERROR_SUCCESS) {


                    //if ( IsChunkEncoding() )
                    if ( IsChunkEncoding() && (_QueryBytesAvailable != 0))
                    {
                        LPSTR lpszNewBuffer;
                        DWORD dwNewBufferLength;

                        error = _ctChunkInfo.ParseChunkInput(
                            (LPSTR) _QueryBuffer,
                            _QueryBytesAvailable,
                            &lpszNewBuffer,
                            &dwNewBufferLength
                            );

                        _QueryBytesAvailable = dwNewBufferLength;

                        INET_ASSERT(error == ERROR_SUCCESS); // I want to see this.

                        if ( error != ERROR_SUCCESS )
                        {
                            goto quit;
                        }

                        //
                        // hack - wait for more data
                        //

                        if ((dwNewBufferLength == 0) && !IsChunkedEncodingFinished()) {

                            struct fd_set read_fds;
                            struct fd_set write_fds;
                            struct fd_set except_fds;

                            FD_ZERO(&read_fds);
                            FD_ZERO(&write_fds);
                            FD_ZERO(&except_fds);

                            FD_SET(_Socket->GetSocket(), &read_fds);

                            struct timeval to;

                            to.tv_sec = 30;
                            to.tv_usec = 0;
                            int n = _I_select(1, &read_fds, &write_fds, &except_fds, &to);
                            if (n > 0) {
                                goto read_again;
                            }
                        }
                    }


                    bytesAvailable = _QueryBytesAvailable;

                    //
                    // note the amount of data that is available immediately.
                    // This allows e.g. async InternetReadFile() to complete
                    // synchronously if the next request is for <= bytesAvailable
                    //

                    //SetAvailableDataLength(bytesAvailable);

                    DEBUG_PRINT(HTTP,
                                INFO,
                                ("%d bytes available in socket %#x\n",
                                bytesAvailable,
                                (_Socket ? _Socket->GetSocket() : 0)
                                ));

                    if ((bytesAvailable == 0)
                    && (IsChunkEncoding() ? IsChunkedEncodingFinished() : TRUE)) {
                        fsm.m_bEof = TRUE;
                    }
                    if (IsKeepAlive() && IsContentLength()) {
                        _BytesInSocket -= bytesAvailable;

                        INET_ASSERT((int)_BytesInSocket >= 0);

                        if ((int)_BytesInSocket < 0) {
                            _BytesInSocket = 0;
                        }
                    }
                }
            }
        } else {

            //
            // all data read from socket & socket released
            //

            INET_ASSERT(error == ERROR_SUCCESS);
            INET_ASSERT(bytesAvailable == 0);

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("no socket\n"
                        ));

            fsm.m_bEof = TRUE;
        }
    } else {

        INET_ASSERT(error == ERROR_SUCCESS);

        //
        // we may have already removed all the data from the socket
        //

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("all data has been read\n"
                    ));

        fsm.m_bEof = TRUE;
    }

quit:

    if ((error == ERROR_SUCCESS) && (bytesAvailable == 0)) {
        if (IsCacheWriteInProgress()) {
            LocalEndCacheWrite(TRUE);
        }
    }

    *fsm.m_lpdwNumberOfBytesAvailable = bytesAvailable;

    //
    // if we have reached the end of the data then we can release the connection
    //

    /*
    if (fsm.m_bEof || (bytesAvailable >= _BytesRemaining)) {
        if (_Socket != NULL) {
            CloseConnection(FALSE);
        }
    }
    */
    if (fsm.m_bEof) {
        if (_Socket != NULL) {
            CloseConnection(FALSE);
        }
    }

done:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::DrainResponse(
    OUT LPBOOL lpbDrained
    )

/*++

Routine Description:

    Receives any remaining response data into the buffer we allocated for the
    headers. Used in redirection: if the server returns some HTML page (e.g.)
    with the redirection response, we give the app a chance to read it. This
    way, we allow the app to retrieve the data immediately during the status
    callback in which we indicate that the request has been redirected

Arguments:

    lpbDrained  - TRUE if we really drained the socket else FALSE

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - WSA error mapped to ERROR_INTERNET_XXX

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::DrainResponse",
                 "%#x",
                 lpbDrained
                 ));

    DWORD error = DoFsm(new CFsm_DrainResponse(lpbDrained, this));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_DrainResponse::RunSM(
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
                 "CFsm_DrainResponse::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;
    CFsm_DrainResponse * stateMachine = (CFsm_DrainResponse *)Fsm;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();
    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = pRequest->DrainResponse_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::DrainResponse_Fsm(
    IN CFsm_DrainResponse * Fsm
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
                 "HTTP_REQUEST_HANDLE_OBJECT::DrainResponse_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_DrainResponse & fsm = *Fsm;
    DWORD error = fsm.GetError();
    BOOL drainIt = FALSE;

    if (error != ERROR_SUCCESS) {
        goto quit;
    }
    if (fsm.GetState() == FSM_STATE_CONTINUE) {
        drainIt = TRUE;
        goto fsm_continue;
    }

    PERF_LOG(PE_TRACE, 0x8001);

    drainIt = TRUE;

    //
    // if the socket is already closed, we can't drain it
    //

    if ((_Socket == NULL) || !_Socket->IsValid()) {
        drainIt = FALSE;
    } else if (IsWantKeepAlive()) {

        //
        // IIS 1.0 has a bug where it can return a failure indication to a
        // request that was made using a keep-alive connection. The response
        // doesn't contain a keep-alive header but the server has left open the
        // connection AND it has returned us fewer bytes than was claimed in
        // the content-length header. If we try to drain the response buffer at
        // this point, we will wait a long time waiting for the server to send
        // us the non-existent additional bytes. Therefore, if we are talking
        // to an IIS 1.0 server, we don't drain the response buffer
        //

        LPSTR lpszServerBuf;
        DWORD serverBufferLength;

        _ResponseHeaders.LockHeaders();

        error = FastQueryResponseHeader(HTTP_QUERY_SERVER,
                                        (LPVOID *)&lpszServerBuf,
                                        &serverBufferLength,
                                        0
                                        );
        if (error == ERROR_SUCCESS) {

#define IIS         "Microsoft-IIS/"
#define IIS_LEN     (sizeof(IIS) - 1)

#define PWS         "Microsoft-PWS/"
#define PWS_LEN     (sizeof(PWS) - 1)

#define PWS95       "Microsoft-PWS-95/"
#define PWS95_LEN   (sizeof(PWS95) - 1)

#define IIS10       "Microsoft-Internet-Information-Server/"
#define IIS10_LEN   (sizeof(IIS10) - 1)

            if ((serverBufferLength > IIS_LEN)
                && !strnicmp(lpszServerBuf, IIS, IIS_LEN)) {

                int major_num = 0;

                for (DWORD i = IIS_LEN; i < serverBufferLength; ++i) {

                    char ch = lpszServerBuf[i];

                    if (isdigit(ch)) {
                        major_num = major_num * 10 + (int)(ch - '0');
                    } else {
                        break;
                    }
                }
                if (major_num < 4) {
                    drainIt = FALSE;
                }
            } else if (IsBadNSServer()) {
                drainIt = FALSE;
            } else if (((serverBufferLength > IIS10_LEN)
                        && !strncmp(lpszServerBuf, IIS10, IIS10_LEN))
                       || ((serverBufferLength > PWS_LEN)
                           && !strncmp(lpszServerBuf, PWS, PWS_LEN))
                       || ((serverBufferLength > PWS95_LEN)
                           && !strncmp(lpszServerBuf, PWS95, PWS95_LEN))) {
                drainIt = FALSE;
            }
        }
        _ResponseHeaders.UnlockHeaders();
    }

    error = ERROR_SUCCESS;

    if (drainIt) {

        fsm.m_dwAsyncFlags = IsAsyncHandle() ? SF_WAIT : 0;
        fsm.m_dwAmountToRead = IsContentLength() ? _BytesInSocket : (DWORD)-1;
        //DWORD bufferLeft = _ResponseBufferLength - _BytesReceived;
        fsm.m_dwBufferLeft = min(fsm.m_dwAmountToRead, _ResponseBufferLength - _BytesReceived);

        if (IsChunkEncoding() && IsChunkedEncodingFinished()) {
            fsm.m_dwAmountToRead = 0;
            fsm.m_bEof = TRUE;

            INET_ASSERT(fsm.m_dwBytesReceived == 0);

        }

        //
        // either receive the amount specified in the "Content-Length" header, or
        // receive until we hit the end of the connection. We may have already
        // received the entire response
        //

        while ((fsm.m_dwAmountToRead != 0) && !fsm.m_bEof && (error == ERROR_SUCCESS)) {

            fsm.m_dwPreviousBytesReceived = _BytesReceived;

            //
            // receive the rest of the data. We are assuming here that it is a
            // couple of K at the most. Notice that we don't care to make status
            // callbacks to the app while we are doing this
            //

            //INET_ASSERT(fsm.m_dwBufferLeft <= _BytesRemaining);

            error = _Socket->Receive((LPVOID *)&_ResponseBuffer,
                                     &_ResponseBufferLength,
                                     &fsm.m_dwBufferLeft,
                                     &_BytesReceived,
                                     0,   // dwExtraSpace
                                     SF_EXPAND
                                     | SF_COMPRESS
                                     | fsm.m_dwAsyncFlags,
                                     &fsm.m_bEof
                                     );
            if (error == ERROR_IO_PENDING) {
                goto quit;
            }

fsm_continue:

            if (error == ERROR_SUCCESS) {

                DWORD nRead = _BytesReceived - fsm.m_dwPreviousBytesReceived;

                if (IsContentLength()) {
                    fsm.m_dwAmountToRead -= nRead;

                    INET_ASSERT((int)fsm.m_dwAmountToRead >= 0);

                    _BytesInSocket -= nRead;

                    INET_ASSERT((int)_BytesInSocket >= 0);

                    if (IsKeepAlive()) {
                        _BytesRemaining -= nRead;

                        INET_ASSERT((int)_BytesRemaining >= 0);

                        //
                        // if we have read all the entity-body then we can
                        // release the keep-alive connection, or close the
                        // socket
                        //

                        //
                        // BUGBUG - put back post-ie30a
                        //

                        //if (_BytesRemaining == 0) {
                        //    fsm.m_bEof = TRUE;
                        //}
                    }
                }

                if ( IsChunkEncoding() )
                {
                    LPSTR lpszNewBuffer;
                    DWORD dwNewBufferLength;

                    INET_ASSERT(!IsContentLength());

                    error = _ctChunkInfo.ParseChunkInput(
                        (LPSTR) (_ResponseBuffer + fsm.m_dwPreviousBytesReceived),
                        nRead,
                        &lpszNewBuffer,
                        &dwNewBufferLength
                        );

                    nRead = dwNewBufferLength;
                    _BytesReceived = nRead + fsm.m_dwPreviousBytesReceived;

                    INET_ASSERT(error == ERROR_SUCCESS); // I want to see this happen.
                    if ( error != ERROR_SUCCESS )
                    {
                        break;
                    }

                    if ( IsChunkedEncodingFinished() )
                    {
                        fsm.m_bEof = TRUE;
                        break;
                    }
                }

                fsm.m_dwBytesReceived += nRead;
                fsm.m_dwPreviousBytesReceived = _BytesReceived;
            }
        }
    }

    if (error == ERROR_SUCCESS) {

        //
        // update the amount of data immediately available to the caller
        //

        IncreaseAvailableDataLength(fsm.m_dwBytesReceived);

        //
        // and set the end-of-file indication in the top level handle object
        //

        SetEndOfFile();

        //
        // there is no more data to be received on this HTTP object
        //

        //SetData(FALSE);

        //
        // this object can now be re-used
        //

        SetState(HttpRequestStateReopen);
    }

    //
    // return indication that we drained the socket
    //

    DEBUG_PRINT(HTTP,
                INFO,
                ("returning *lpbDrained = %B\n",
                drainIt
                ));

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
        *fsm.m_lpbDrained = drainIt;
    }

    PERF_LOG(PE_TRACE, 0x8002);

    DEBUG_LEAVE(error);

    return error;
}


VOID
HTTP_REQUEST_HANDLE_OBJECT::SetBadNSReceiveTimeout(
    VOID
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_REQUEST_HANDLE_OBJECT::SetBadNSReceiveTimeout",
                 NULL
                 ));

    if ((_Socket != NULL)
        && !IsContentLength()
        && !IsChunkEncoding()) {

        CServerInfo * pServerInfo = GetServerInfo();

        if (pServerInfo) {

            DWORD timeout = max(5000, 5 * pServerInfo->GetRTT());

            _Socket->SetTimeout(RECEIVE_TIMEOUT, timeout);
            SetTimeout(INTERNET_OPTION_RECEIVE_TIMEOUT, timeout);
        }
    }

    DEBUG_LEAVE(0);
}
