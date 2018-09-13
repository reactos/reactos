/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    prefetch.cxx

Abstract:

    Functions and methods to support http random read access.

    Contents:
        AttemptReadFromFile
        SetStreamPointer
        SetLengthFromCache
        ReadLoop_Fsm
        WriteResponseBufferToCache
        WriteQueryBufferToCache
Author:

    Rajeev Dujari (rajeevd) March 1996

Revision History:

    Ahsan Kabir (akabir) November 1997
--*/

#include <wininetp.h>


#define READLOOP_BUFSIZE 2048



BOOL
HTTP_REQUEST_HANDLE_OBJECT::AttemptReadFromFile(

    LPVOID lpBuf,
    DWORD cbToRead,
    DWORD* pcbRead
    )
{
    DEBUG_ENTER((DBG_CACHE,
                 Bool,
                 "INTERNET_CONNECT_HANDLE_OBJECT::AttemptReadFromFile",
                 "%#x, %d, %#x",
                 lpBuf,
                 cbToRead,
                 pcbRead
                 ));

    BOOL fSuccess;
    DWORD dwBytesToCopy = 0;

    if (!cbToRead)
    {
        *pcbRead = 0;
        DEBUG_LEAVE(TRUE);
        return TRUE;
    }

    if (IsCacheReadInProgress())
    {
        INET_ASSERT(_VirtualCacheFileSize == _RealCacheFileSize);

        // Entire read should be satisfied from cache.
        *pcbRead = cbToRead;
        if (ReadUrlCacheEntryStream
            (_hCacheStream, _dwCurrentStreamPosition, lpBuf, pcbRead, 0))
        {
            AdvanceReadPosition (*pcbRead);
            DEBUG_LEAVE(TRUE);
            return TRUE;
        }
        else
        {
            *pcbRead = 0;
            DEBUG_LEAVE(FALSE);
            return FALSE;
        }
    }

    else if (IsCacheWriteInProgress())
    {

        // See if the read is completely within the file.
        if (!IsEndOfFile() && _dwCurrentStreamPosition + cbToRead > _VirtualCacheFileSize)
        {

            DEBUG_PRINT(HTTP, ERROR, ("AttemptRead Failed streampos=%d cbToRead=%d, _VitrualCacheFileSize=%d\n",
                            _dwCurrentStreamPosition, cbToRead, _VirtualCacheFileSize));

            DEBUG_LEAVE(FALSE);
            return FALSE;
        }

        HANDLE hfRead;
        hfRead = GetDownloadFileReadHandle();
        if (hfRead == INVALID_HANDLE_VALUE) 
        {
            DEBUG_LEAVE(FALSE);
            return FALSE;
        }

        // Read the data from the file.
        SetFilePointer (hfRead, _dwCurrentStreamPosition, NULL, FILE_BEGIN);
        fSuccess = ReadFile (hfRead, lpBuf, cbToRead, pcbRead, NULL);
        if (!fSuccess)
        {
            DEBUG_LEAVE(FALSE);
            return FALSE;
        }

        AdvanceReadPosition (*pcbRead);
        DEBUG_LEAVE(TRUE);
        return TRUE;
    }

    else
    {
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }
}


// Called from InternetSetFilePointer
DWORD
HTTP_REQUEST_HANDLE_OBJECT::SetStreamPointer(
    LONG lDistanceToMove,
    DWORD dwMoveMethod
    )
{
    DWORD dwErr = ERROR_SUCCESS;

    // Fail if data is not from cache or going to cache.
    if (!IsCacheReadInProgress() && !IsCacheWriteInProgress())
    {
        dwErr = ERROR_INTERNET_INVALID_OPERATION;
        goto done;
    }

    // BUGBUG: we don't handle chunked transfer, new with http 1.1
    if (IsChunkEncoding())
    {
        dwErr = ERROR_INTERNET_INVALID_OPERATION;
        goto done;
    }

    switch (dwMoveMethod)
    {

        case FILE_BEGIN:
            _dwCurrentStreamPosition = (DWORD) lDistanceToMove;
            break;

        case FILE_CURRENT:
            if (lDistanceToMove < 0
                && ((DWORD) -lDistanceToMove) > _dwCurrentStreamPosition)
            {
                dwErr = ERROR_NEGATIVE_SEEK;
            }
            else
            {
                _dwCurrentStreamPosition += lDistanceToMove;
            }
            break;

        case FILE_END:
            if (!IsContentLength())
                 dwErr = ERROR_INTERNET_INVALID_OPERATION;
            else if (lDistanceToMove < 0 && ((DWORD) -lDistanceToMove) > _ContentLength)
                dwErr = ERROR_NEGATIVE_SEEK;
            else
                _dwCurrentStreamPosition = _ContentLength + lDistanceToMove;
            break;

        default:
            dwErr = ERROR_INVALID_PARAMETER;
            break;
    }


done:

    if (dwErr == ERROR_SUCCESS)
    {
        if (IsKeepAlive() && IsContentLength())
            _BytesRemaining = _ContentLength - _dwCurrentStreamPosition;

        if (_VirtualCacheFileSize > _dwCurrentStreamPosition)
            SetAvailableDataLength (_VirtualCacheFileSize - _dwCurrentStreamPosition);
        else
            SetAvailableDataLength (0);

        return _dwCurrentStreamPosition;
    }
    else
    {
        SetLastError (dwErr);
        return (DWORD) -1L;
    }
}

DWORD
CFsm_ReadLoop::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_ReadLoop::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    HTTP_REQUEST_HANDLE_OBJECT * pRequest;
    CFsm_ReadLoop * stateMachine = (CFsm_ReadLoop *)Fsm;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)Fsm->GetContext();
    switch (Fsm->GetState()) {
        case FSM_STATE_INIT:
        case FSM_STATE_CONTINUE:
            error = pRequest->ReadLoop_Fsm(stateMachine);
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
HTTP_REQUEST_HANDLE_OBJECT::ReadLoop_Fsm(
    IN CFsm_ReadLoop * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::ReadLoop_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_ReadLoop & fsm = *Fsm;
    DWORD dwErr = fsm.GetError();

    if (fsm.GetState() == FSM_STATE_CONTINUE)
    {
        goto receive_continue;
    }

    // Set the goal for reading from the socket.
    fsm.m_dwReadEnd = _dwCurrentStreamPosition +
        ((fsm.m_dwSocketFlags & SF_NO_WAIT)? 1 : fsm.m_cbReadIn);

    // Flush any data in response buffer to download file.
    dwErr = WriteResponseBufferToCache();
    if (dwErr != ERROR_SUCCESS)
        goto done;

    // Flush any data in query buffer to download file.
    dwErr = WriteQueryBufferToCache();
    if (dwErr != ERROR_SUCCESS)
        goto done;

    // BUGBUG: what if HaveReadFileExData?

    // Allocate receive buffer.  We could optimize this to use
    // the client buffer or query buffer when available to avoid
    // reading from the file data that was just written.

    fsm.m_cbBuf = READLOOP_BUFSIZE;
    if (!(fsm.m_pBuf = (PVOID) new BYTE [fsm.m_cbBuf]))
    {
        dwErr = ERROR_NOT_ENOUGH_MEMORY;
        goto done;
    }

    HANDLE hfRead;
    hfRead = GetDownloadFileReadHandle();
    if (hfRead == INVALID_HANDLE_VALUE) 
    {
        dwErr = GetLastError();
        DEBUG_PRINT(CACHE, ERROR, ("Cache: createfile failed error=%ld\n", dwErr));
        goto done;
    }
    
    while (1)
    {
        // Calculate amount of data available for next read.
        if (_VirtualCacheFileSize > _dwCurrentStreamPosition)
            SetAvailableDataLength  (_VirtualCacheFileSize - _dwCurrentStreamPosition);
        else
            SetAvailableDataLength (0);

        // Check if pending request can be satisfied from file.
        if (_EndOfFile || _VirtualCacheFileSize >= fsm.m_dwReadEnd)
        {
            INET_ASSERT (AvailableDataLength());

            if (fsm.m_pRead)
            {
                // The client wants us to fill the buffer.
                if (fsm.m_dwSocketFlags & SF_NO_WAIT)
                {
                    // If the app is greedy, give it all we've got.
                    if (fsm.m_cbReadIn > AvailableDataLength())
                        fsm.m_cbReadIn = AvailableDataLength();
                }
                else
                {
                    INET_ASSERT (fsm.m_cbReadIn <= AvailableDataLength());
                }
            }

            // Read the data from the file.
            LONG lRet;
            lRet = SetFilePointer
                (hfRead, _dwCurrentStreamPosition, NULL, FILE_BEGIN);
            INET_ASSERT ((DWORD) lRet == _dwCurrentStreamPosition);
            if (lRet == -1)
            {
                dwErr = GetLastError();
                goto done;
            }

            BOOL fRet;
            fRet = ReadFile
                (hfRead, fsm.m_pRead, fsm.m_cbReadIn, fsm.m_pcbReadOut, NULL);
            if (!fRet)
            {
                dwErr = GetLastError();
                goto done;
            }

            AdvanceReadPosition (*fsm.m_pcbReadOut);
            INET_ASSERT (dwErr == ERROR_SUCCESS);
            goto done;
        }

        INET_ASSERT (!_EndOfFile);
        INET_ASSERT (_Socket);

        fsm.m_cbRead = fsm.m_cbBuf;
        if (IsKeepAlive() && fsm.m_cbRead > _BytesInSocket)
            fsm.m_cbRead = _BytesInSocket;
        fsm.m_cbRecv = 0;

        // Get some more data from the socket.

        dwErr = _Socket->Receive
        (
            &fsm.m_pBuf,     // IN OUT LPVOID* lplpBuffer,
            &fsm.m_cbBuf,    // IN OUT LPDWORD lpdwBufferLength,
            &fsm.m_cbRead,   // IN OUT LPDWORD lpdwBufferRemaining,
            &fsm.m_cbRecv,   // IN OUT LPDWORD lpdwBytesReceived,
            0,               // IN DWORD dwExtraSpace,
            fsm.m_dwSocketFlags,     // IN DWORD dwFlags,
            &_EndOfFile  // OUT LPBOOL lpbEof
        );

        if (dwErr == ERROR_IO_PENDING)
        {
            if (fsm.m_dwSocketFlags & SF_NO_WAIT)
            {
                // InternetReadFileEx can set IRF_NO_WAIT.  If we must go async
                // in this case, morph the request into a QueryDataAvailable
                // and app will call again to get the data after notification.

                fsm.m_pRead = NULL;
                fsm.m_cbReadIn = 1;
                fsm.m_pcbReadOut = NULL;
            }
            goto done;
        }

receive_continue:

        if (dwErr != ERROR_SUCCESS)
        {
            DEBUG_PRINT(HTTP,
                        ERROR,
                        ("error %d on socket %#x\n",
                        dwErr,
                        _Socket->GetSocket()));

            goto done;
        }

        // Append data to download file.
        dwErr = WriteCache ((PBYTE) fsm.m_pBuf, fsm.m_cbRecv);
        if (dwErr != ERROR_SUCCESS)
            goto done;

        // If content length is known, check for EOF.
        if (IsKeepAlive())
        {
            _BytesInSocket -= fsm.m_cbRecv;
            if (!_BytesInSocket)
                _EndOfFile = TRUE;
        }

        if (_EndOfFile)
        {
            // Set handle state.
            SetState(HttpRequestStateReopen);
            if (!IsKeepAlive())
                CloseConnection(FALSE);
            SetData(FALSE);
        }

    } // end while (1)

done:

    if (dwErr != ERROR_IO_PENDING)
    {
        delete [] fsm.m_pBuf;
        fsm.SetDone();

        if (dwErr != ERROR_SUCCESS)
        {
            DEBUG_PRINT(HTTP, ERROR, ("Readloop: Error = %d\n", dwErr));

            SetState(HttpRequestStateError);
            LocalEndCacheWrite(FALSE);
        }
    }

    DEBUG_LEAVE(dwErr);

    return dwErr;
}


// Called from ReadLoop to write data in response buffer to download file.
DWORD HTTP_REQUEST_HANDLE_OBJECT::WriteResponseBufferToCache (VOID)
{
    DWORD cbData = _BytesReceived - _DataOffset;
    if (!cbData)
        return ERROR_SUCCESS;
    DWORD dwErr = WriteCache
        (((LPBYTE) _ResponseBuffer) + _DataOffset, cbData);
    _DataOffset += cbData;

    DEBUG_PRINT(HTTP, ERROR, ("WriteResponseBufferToCache: Error = %d", dwErr));

    return dwErr;
}

// Called from ReadLoop to write data in query buffer to download file.
DWORD HTTP_REQUEST_HANDLE_OBJECT::WriteQueryBufferToCache (VOID)
{
    if (!_QueryBytesAvailable)
        return ERROR_SUCCESS;
    DWORD dwErr = WriteCache
        (((LPBYTE) _QueryBuffer) + _QueryOffset, _QueryBytesAvailable);
    _QueryOffset += _QueryBytesAvailable;
    _QueryBytesAvailable = 0;

    DEBUG_PRINT(HTTP, ERROR, ("WriteQueryBufferToCache: Error = %d", dwErr));

    return dwErr;
}

