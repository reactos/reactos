/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    buffer.hxx

Abstract:

    Contains types, manifests, prototypes for double buffering object
    (in buffer.cxx)

Author:

    Arthur L Bierer (arthurbi) 20-March-1996

Revision History:

    20-March-1996 arthurbi
        Created

--*/

#ifndef BUFFER_H_
#define BUFFER_H_

//
// DBLBUFFER Class
//

#define DBLBUFFER_DEFAULT_SIZE  1500
#define DBLBUFFER_MAX_SIZE              (32*1024)

class DBLBUFFER {

private:

    //
    // _lpBuffer - pointer to allocated buffer.
    //
    LPBYTE  _lpBuffer;

    //
    // _dwBufferLength - current allocated size of the buffer
    //

    DWORD   _dwBufferLength;

    //
    // _lpBufferIn - pointer where Input Buffer starts
    //

    LPBYTE  _lpBufferIn;

    //
    // _dwBufferIn - size of input buffer in bytes.
    //

    DWORD   _dwBufferIn;

    //
    // _lpBufferOut - pointer to where output buffer starts
    //

    LPBYTE  _lpBufferOut;

    //
    // _dwBufferOut - size of output buffer.
    //

    DWORD   _dwBufferOut;

    //
    // _dwInitDefaultBufferSize - default initial size of main buffer.
    //

    DWORD   _dwInitDefaultBufferSize;

    //
    // Internal Varibles -----------------------------
    //

    //
    // _lpEndOfBuffer - points to end of the buffer.
    //

    LPBYTE  _lpEndOfBuffer;

    //
    // fDblBufferMode - TRUE if we maintain an input buffer seperate from an output buffer
    //              otherwise the input/output is considered the same (ie input buffer is not used)
    //

    BOOL    _fDblBufferMode;


    VOID
    UpdateVars() {

        INET_ASSERT(_lpBuffer);

        _lpEndOfBuffer = _lpBuffer + _dwBufferLength;

        //
        // Dock Pointers at end of buffer for sanity checking later on
        //

        if ( _dwBufferOut == 0 )
            _lpBufferOut = _lpEndOfBuffer;

        if ( _dwBufferIn == 0 )
            _lpBufferIn  = _lpEndOfBuffer;
    }


public:

    DBLBUFFER(
        DWORD dwInitBufferSize) {

        _dwInitDefaultBufferSize = dwInitBufferSize;
        _dwBufferOut = 0;
        _dwBufferIn  = 0;
        _lpBufferOut = NULL;
        _lpBufferIn  = NULL;
        _lpBuffer    = NULL;
        _dwBufferLength = 0;
        _fDblBufferMode = FALSE;

    }

    DBLBUFFER(
        VOID) {

        _dwInitDefaultBufferSize = DBLBUFFER_DEFAULT_SIZE;
        _dwBufferOut = 0;
        _dwBufferIn  = 0;
        _lpBufferOut = NULL;
        _lpBufferIn  = NULL;
        _lpBuffer    = NULL;
        _dwBufferLength = 0;
        _fDblBufferMode = FALSE;

    }



    ~DBLBUFFER() {
        if ( _lpBuffer )
        {
            FREE_MEMORY(_lpBuffer);
        }
    }


    LPBYTE
    GetInputBufferPointer(
        VOID
    ) {
        DEBUG_ENTER((DBG_INET,
                     Pointer,
                     "DBLBUFFER::GetInputBufferPointer",
                     NULL
                     ));

        if ( _fDblBufferMode )
        {
            DEBUG_LEAVE((( _lpBufferIn == _lpEndOfBuffer ) ? _lpBuffer : _lpBufferIn));
            return ( _lpBufferIn == _lpEndOfBuffer ) ? _lpBuffer : _lpBufferIn;
        }
        else
        {
            DEBUG_LEAVE((( _lpBufferOut == _lpEndOfBuffer ) ? _lpBuffer : _lpBufferOut));
            return ( _lpBufferOut == _lpEndOfBuffer ) ? _lpBuffer : _lpBufferOut;
        }
    }


    DWORD
    GetInputBufferSize(
        VOID
    ) {

        DEBUG_ENTER((DBG_INET,
                     Int,
                     "DBLBUFFER::GetInputBufferSize",
                     NULL
                     ));


        if ( _fDblBufferMode )
        {
            DEBUG_LEAVE(_dwBufferIn);
            return _dwBufferIn;
        }
        else
        {
            DEBUG_LEAVE(_dwBufferOut);
            return _dwBufferOut;
        }
    }

    VOID
    SetInputBufferSize(
        IN DWORD  dwNewInputBufferSize
    ) {

        DEBUG_ENTER((DBG_INET,
                     None,
                     "DBLBUFFER::SetInputBufferSize",
                     "%u",
                     dwNewInputBufferSize
                     ));


        if ( _fDblBufferMode )
        {
            _dwBufferIn = dwNewInputBufferSize;
        }
        else
        {
            _dwBufferOut = dwNewInputBufferSize;
        }

        DEBUG_LEAVE(0);
    }

    DWORD
    GetOutputBufferSize(
        VOID
    ) {

        DEBUG_ENTER((DBG_INET,
                     Int,
                     "DBLBUFFER::GetOutputBufferSize",
                     NULL
                     ));


        DEBUG_LEAVE(_dwBufferOut);

        return _dwBufferOut;
    }



    BOOL
    InitBuffer(
        BOOL fDblBufferMode
        );


    DWORD
    GetInputBufferRemaining(
        VOID
        );



    BOOL
    CopyOut(
        OUT LPBYTE         lpBuffer,
        IN OUT LPDWORD lpdwBufferRemaining,
        IN OUT LPDWORD lpdwBytesReceived,
        IN OUT LPDWORD lpdwBytesRead
        );


    BOOL
    CopyIn(
        IN LPBYTE      lpBuffer,
        IN DWORD       dwBufferSize
        );

    BOOL
    CompressInputBufferUsage(
        VOID
        );

    BOOL
    ConcatenateOutputBufferUsage(
        IN LPBYTE lpSecondOutputBuffer,
        IN DWORD  dwSecondOutputBufferSize
        );

    BOOL
    ResizeBufferIfNeeded(
        IN DWORD dwAddlBufferNeeded
        );

    BOOL
    SetOutputInputBuffer(
        IN LPBYTE lpNewOutputBuffer,
        IN DWORD  dwNewOutputBufferSize,
        IN LPBYTE lpNewInputBuffer,
        IN DWORD  dwNewInputBufferSize,
        IN BOOL   fConcatenatePreviousOutput
        );

};

#endif
