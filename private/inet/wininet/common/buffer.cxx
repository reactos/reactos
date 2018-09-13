/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    buffer.cxx

Abstract:

    Contains code to impliment a double buffering class used in SSL/PCT (secure channel)
        transactions.


Author:

    Arthur L Bierer (arthurbi) 20-March-1996

Revision History:

    20-March-1996 arthurbi
        Created

--*/

#include <wininetp.h>


BOOL
DBLBUFFER::InitBuffer(
    BOOL fDblBufferMode
    )

/*++

Routine Description:

    Allocates, and initalizes internal buffers.

Arguments:

    fDblBufferMode  - TRUE if we are to maintain to buffers,
                      FALSE if we treat the output and input buffers the same

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE, not enough memory to allocate buffers.

Comments:


--*/

{
    DEBUG_ENTER((DBG_BUFFER,
                 Bool,
                 "DBLBUFFER::InitBuffer",
                 "%B",
                 fDblBufferMode
                 ));

    INET_ASSERT(_lpBuffer == NULL);
    INET_ASSERT(_dwInitDefaultBufferSize > 0 );

    _lpBuffer = (LPBYTE)ALLOCATE_FIXED_MEMORY(_dwInitDefaultBufferSize);

    DEBUG_PRINT(API,
                INFO,
                ("allocated %d byte buffer %#x\n",
                _dwInitDefaultBufferSize,
                _lpBuffer
                ));

    BOOL success = FALSE;

    if (_lpBuffer != NULL) {

        INET_ASSERT(_dwBufferOut == 0);
        INET_ASSERT(_dwBufferIn == 0);

        _fDblBufferMode = fDblBufferMode;

        _dwBufferLength = _dwInitDefaultBufferSize ;
        _lpBufferOut = _lpBuffer + _dwBufferLength;
        _lpBufferIn      = _lpBuffer + _dwBufferLength;

        success = TRUE;
    }

    DEBUG_LEAVE(success);

    return success;
}

DWORD
DBLBUFFER::GetInputBufferRemaining(
    VOID
    )

/*++

Routine Description:

    Determines the amount of free bytes availble for reading into the input buffer.
        Will attempt to push current data to the front of the buffer, to make the
        most room in the currently allocated buffer.

Arguments:

    none.

Return Value:

    DWORD
        Number of bytes free.

Comments:

        Assumed to only be called in DblBuffer mode.

--*/


{
    DEBUG_ENTER((DBG_INET,
                 Int,
                 "DBLBUFFER::GetInputBufferRemaining",
                 NULL
                 ));


    INET_ASSERT(_lpBuffer);
    INET_ASSERT(_dwBufferOut == 0 );
    INET_ASSERT(_fDblBufferMode );


    BOOL fIsInputBufferCompressed = (_lpBufferIn == _lpBuffer);

    if ( ! fIsInputBufferCompressed )
    {
        BOOL fIsSuccess;

        fIsSuccess = CompressInputBufferUsage( ) ;

        INET_ASSERT(fIsSuccess);
    }

    INET_ASSERT(_lpBufferIn == _lpBuffer );

    DEBUG_LEAVE((DWORD) (_lpEndOfBuffer - (_dwBufferIn + _lpBuffer) ));

    return (DWORD) (_lpEndOfBuffer - (_dwBufferIn + _lpBuffer) );
}

BOOL
DBLBUFFER::CopyIn(
    IN LPBYTE lpBuffer,
    IN DWORD dwBufferSize
    )
{

    DEBUG_ENTER((DBG_INET,
                 Bool,
                 "DBLBUFFER::CopyIn",
                 "%x, %d",
                 lpBuffer,
                 dwBufferSize
                 ));


    INET_ASSERT(lpBuffer);
    INET_ASSERT(dwBufferSize > 0);

    LPBYTE pbPointer;
    DWORD  dwCurInputSize;

    //
    // Get the current input buffer size
    //

    dwCurInputSize = GetInputBufferSize();

    if ( ! ResizeBufferIfNeeded(dwBufferSize) )
    {
        DEBUG_LEAVE(FALSE);
        return FALSE;
    }

    pbPointer = GetInputBufferPointer()+dwCurInputSize;
    SetInputBufferSize(dwBufferSize+dwCurInputSize);


    CopyMemory(
        pbPointer,             // dest
        lpBuffer,              // src
        dwBufferSize           // size
        );


    DEBUG_LEAVE(TRUE);

    return TRUE;

}

BOOL
DBLBUFFER::CopyOut(
    OUT LPBYTE     lpBuffer,
    IN OUT LPDWORD lpdwBufferRemaining,
    IN OUT LPDWORD lpdwBytesReceived,
    IN OUT LPDWORD lpdwBytesRead
    )

/*++

Routine Description:

    Fills passed in buffer with the contents of the output buffer.

Arguments:

    lpBuffer                    -       Buffer to fill with output buffer bytes

        lpdwBufferRemaining -   Number of bytes remaining in Buffer to fill.

        lpdwBytesReceived       -       Number of bytes currently in Buffer to fill.

        lpdwBytesRead           -       Current total of bytes copied into Buffer to fill.

Return Value:

    BOOL
    Success - TRUE

    Failure - FALSE

Comments:


--*/

{

    DEBUG_ENTER((DBG_INET,
                 Bool,
                 "DBLBUFFER::CopyOut",
                 "%x, %u, %u, %u",
                 lpBuffer,
                 (lpdwBufferRemaining ? *lpdwBufferRemaining : 0),
                 (lpdwBytesReceived ? *lpdwBytesReceived : 0),
                 (lpdwBytesRead ? *lpdwBytesRead : 0)
                 ));


    INET_ASSERT(lpBuffer);
    INET_ASSERT(lpdwBufferRemaining);
    INET_ASSERT(lpdwBytesReceived);
    INET_ASSERT(lpdwBytesRead);

    //
    // Figure out the max number of bytes we can copy into our user's buffer
    //      We need to make sure it will fit into the user's buffer.
    //

    DWORD dwBytesToCopy = (*lpdwBufferRemaining  >= _dwBufferOut)
            ? _dwBufferOut : *lpdwBufferRemaining;


    DEBUG_PRINT(API,
                INFO,
                ("DBLBUFFER::CopyOut: Copying ( to: %x bytes-to-copy=%d )\n",
                (lpBuffer+*lpdwBytesReceived),
                dwBytesToCopy
                ));


    //
    // Transfer Data to User's buffer.
    //

    CopyMemory ((lpBuffer+*lpdwBytesReceived),
                            _lpBufferOut,
                            dwBytesToCopy);


    //
    // Update the number of bytes we copied to the user buffer
    //

    *lpdwBytesRead                  += dwBytesToCopy;
    *lpdwBytesReceived              += dwBytesToCopy;
    *lpdwBufferRemaining    -= dwBytesToCopy;

    //
    // Update Our Internal Vars
    //

    _dwBufferOut                    -= dwBytesToCopy;
    _lpBufferOut                    += dwBytesToCopy;

    UpdateVars();

    //
    // We always succeed for now !
    //

    DEBUG_LEAVE(TRUE);

    return TRUE;
}

BOOL
DBLBUFFER::CompressInputBufferUsage(
    VOID
    )
/*++

Routine Description:

        Moves the input buffer to the begining of the internal allocated buffer.
        This produces a larger block of internal free space.

Arguments:

        none.

Return Value:

    BOOL
    Success - TRUE

    Success     - FALSE, there was no need to compress the buffer.

Comments:

  Assumed to be called only if DblBufferMode is enabled.

--*/


{
    DEBUG_ENTER((DBG_INET,
                 Bool,
                 "DBLBUFFER::CompressInputBufferUsage",
                 NULL
                 ));

    BOOL bResult = FALSE;

    //
    // Maximize use of buffer by moving input data to the front
    // of the buffer.
    //

    INET_ASSERT(_lpBuffer);
    INET_ASSERT(_dwBufferOut == 0);
    INET_ASSERT(_fDblBufferMode);

    if (_lpBufferIn > _lpBuffer) {

        DEBUG_PRINT(API,
                    INFO,
                    ("compressing input buffer %d (%#x) @ %#x => %#x\n",
                    _dwBufferIn,
                    _dwBufferIn,
                    _lpBufferIn,
                    _lpBuffer
                    ));

        MoveMemory(_lpBuffer,
                   _lpBufferIn,
                   _dwBufferIn
                   );

        //
        // Input Buffer now starts at the begining of the allocated buffer
        //

        _lpBufferIn = _lpBuffer;
        bResult = TRUE;
    }

    DEBUG_LEAVE(bResult);

    return bResult;
}



BOOL
DBLBUFFER::ConcatenateOutputBufferUsage(
    IN LPBYTE lpSecondOutputBuffer,
    IN DWORD  dwSecondOutputBufferSize
    )

/*++

Routine Description:

    Combines the current output buffer with the contents of a new buffer.
    (Note: intented for use in combining decrypted data which may be seperated by
    header or trailer data)

Arguments:

    lpSecondOutputBuffer        - New Buffer to combine with internal output buffer.

    dwSecondOutputBufferSize    - Size of New Buffer in bytes.


Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE

Comments:

    If the internal buffer is sized 0, the new buffer replaces the internal buffer.
    Its assumed that the New buffer is a former input buffer turned output by
    some external operation, such as a block decryption operation.
--*/

{


    DEBUG_ENTER((DBG_INET,
                 Bool,
                 "DBLBUFFER::ConcatenateOutputBufferUsage",
                 "%x, %u",
                 lpSecondOutputBuffer,
                 dwSecondOutputBufferSize
                 ));


    //
    // Combinate Two buffers into one.
    //

    INET_ASSERT(_lpBuffer);
    INET_ASSERT(_fDblBufferMode );
    INET_ASSERT(lpSecondOutputBuffer);
    INET_ASSERT(dwSecondOutputBufferSize);

    INET_ASSERT(_lpBufferOut < _lpBufferIn );
    INET_ASSERT(lpSecondOutputBuffer >= _lpBuffer && lpSecondOutputBuffer <=_lpEndOfBuffer );



    if ( _dwBufferOut != 0 )
    {
        DEBUG_PRINT(API,
                    INFO,
                    ("DBLBUFFER::ConcatenateOutputBufferUsage: Combining new data with output buffer\n"
                    ));

        MoveMemory((_lpBufferOut+_dwBufferOut),
                   lpSecondOutputBuffer,
                   dwSecondOutputBufferSize);
        //
        // Output Buffer is now bigger ( sum of orginal + new buffer size )
        //

        _dwBufferOut += dwSecondOutputBufferSize;

    }
    else
    {
        //
        // No previous output buffer, new buffer becomes output buffer
        //

        INET_ASSERT(_lpBufferOut == _lpEndOfBuffer );

        _lpBufferOut = lpSecondOutputBuffer;
        _dwBufferOut = dwSecondOutputBufferSize;

    }

    DEBUG_LEAVE(TRUE);

    return TRUE;
}


BOOL
DBLBUFFER::ResizeBufferIfNeeded(
    IN DWORD dwAddlBufferNeeded
    )

/*++

Routine Description:

    ReSizes internal buffer space to extend size of the buffer by dwAddlBufferNeeded.
    If the additional bytes can be made availble by compressing currently stored
    into one place ( ie the start of the buffer ), the reallocation of the buffer
    will not be done.

Arguments:

    dwAddlBufferNeeded  - Number of additional bytes to resize

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE, the reallocation failed due to lack of memory.

Comments:

    Its assumed the caller will only use this function in dbl buffering mode.

 --*/

{

    DEBUG_ENTER((DBG_INET,
                 Bool,
                 "DBLBUFFER::ResizeBufferIfNeeded",
                 "%u",
                 dwAddlBufferNeeded
                 ));

    INET_ASSERT(_lpBuffer);
    INET_ASSERT(_dwBufferOut == 0 );
    INET_ASSERT(_fDblBufferMode );

    DWORD dwInputBytesFree = GetInputBufferRemaining();

    INET_ASSERT(_lpBuffer == _lpBufferIn );

    //
    // #bytes needed > #bytes left free in Buffer.
    //

    if ( dwAddlBufferNeeded > dwInputBytesFree )
    {
        HLOCAL hBuffer;

        hBuffer = (HLOCAL) _lpBuffer;

        //
        // length increases by (bytes needed - current bytes free[in old buffer])
        //

        _dwBufferLength += (dwAddlBufferNeeded - dwInputBytesFree);

        DEBUG_PRINT(API,
                    INFO,
                    ("DBLBUFFER::ResizeBufferIfNeeded: Resizing Buffer to %d, addl=%d, free=%d\n",
                    _dwBufferLength,
                    dwAddlBufferNeeded,
                    dwInputBytesFree
                    ));

        INET_ASSERT(_dwBufferLength < DBLBUFFER_MAX_SIZE);

        //
        // Do Resize, and store result
        //

        _lpBuffer = (LPBYTE)ResizeBuffer(hBuffer, _dwBufferLength, FALSE);

        DEBUG_PRINT(BUFFER,
                    INFO,
                    ("resized %#x => %#x, %d bytes\n",
                    hBuffer,
                    _lpBuffer,
                    _dwBufferLength
                    ));

        if ( ! _lpBuffer )
        {
            DEBUG_PRINT(API,
                        ERROR,
                        ("DBLBUFFER::ResizeBufferIfNeeded: Failed while Resizing, Out of Mem?\n"
                        ));

            DEBUG_LEAVE(FALSE);

            return FALSE;  // failing due to NOT_ENOUGH_MEMORY
        }

        //
        // Update ReSized Buffer pointers
        //

        _lpBufferIn = _lpBuffer;

        UpdateVars();
    }

    DEBUG_LEAVE(TRUE);

    return TRUE;
}

BOOL
DBLBUFFER::SetOutputInputBuffer(
    IN LPBYTE lpNewOutputBuffer,
    IN DWORD  dwNewOutputBufferSize,
    IN LPBYTE lpNewInputBuffer,
    IN DWORD  dwNewInputBufferSize,
    IN BOOL   fConcatenatePreviousOutput
    )

/*++

Routine Description:

    Allows caller to specify new addresses for input and output buffer.
    If fConcatenatePreviousOutput is set, SetOutputInputBuffer will combine
    the passed in output buffer with any internal output buffer.
    Also allows size changes to buffers.

Arguments:

    lpNewOutputBuffer           - New Output Buffer.

    dwNewOutputBufferSize       - Size of New Output Buffer.

    lpNewInputBuffer            - New Input Buffer.

    dwNewInputBufferSize        - New Input Buffer Size.

    fConcatenatePreviousOutput  - TRUE if we are to combine internal output buffer
                                  with (passed in) new output buffer
                                  FALSE if we are to just replace output buffer pointers.

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE

Comments:

    Assumed to be called from double buffering mode.

--*/


{

    INET_ASSERT(_lpBuffer);
    INET_ASSERT(lpNewOutputBuffer >= _lpBuffer && lpNewOutputBuffer <= _lpEndOfBuffer );
    INET_ASSERT((lpNewInputBuffer >= _lpBuffer && lpNewInputBuffer <= _lpEndOfBuffer) || dwNewInputBufferSize == 0);
    INET_ASSERT(fConcatenatePreviousOutput || _dwBufferOut == 0 );
    INET_ASSERT(_fDblBufferMode );

    DEBUG_PRINT(API,
         INFO,
         ("DBLBUFFER::SetOutputInputBuffer: Getting New Output( %x, size=%d ) New Input( %x, size=%d)\n",
         lpNewOutputBuffer,
         dwNewOutputBufferSize,
         lpNewInputBuffer,
         dwNewInputBufferSize
         ));


    if ( fConcatenatePreviousOutput )
    {
        ConcatenateOutputBufferUsage(
                    lpNewOutputBuffer,
                    dwNewOutputBufferSize
                    );
    }
    else
    {
        _lpBufferOut = lpNewOutputBuffer;
        _dwBufferOut = dwNewOutputBufferSize;
    }


    _lpBufferIn = lpNewInputBuffer;
    _dwBufferIn = dwNewInputBufferSize;

    UpdateVars();

    return TRUE;
}
