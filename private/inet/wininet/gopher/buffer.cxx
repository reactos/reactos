/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    buffer.cxx

Abstract:

    Contains functions for managing BUFFER_INFO 'objects'

    Contents:
        CreateBuffer
        DestroyBuffer
        AcquireBufferLock
        ReleaseBufferLock
        ReferenceBuffer
        DereferenceBuffer

Author:

    Richard L Firth (rfirth) 02-Nov-1994

Revision History:

    02-Nov-1994 rfirth
        Created

--*/

#include <wininetp.h>
#include "gfrapih.h"

//
// data
//

DEBUG_DATA(LONG, NumberOfBuffers, 0);

//
// functions
//


LPBUFFER_INFO
CreateBuffer(
    OUT LPDWORD Error
    )

/*++

Routine Description:

    Creates and initializes a BUFFER_INFO

Arguments:

    Error   - pointer to returned error

Return Value:

    LPBUFFER_INFO
        Success - pointer to new BUFFER_INFO
        Failure - NULL

--*/

{
    LPBUFFER_INFO bufferInfo;
    DWORD error;
    HANDLE hEvent;

    bufferInfo = NEW(BUFFER_INFO);
    if (bufferInfo != NULL) {

        //
        // create an event that is initially unsignalled. The thread that
        // creates this buffer info will signal the event when the response
        // has been received from the server. At that point, any other
        // concurrent requesters can access the data
        //

//        bufferInfo->RequestEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
//        if (bufferInfo->RequestEvent != NULL) {
            //bufferInfo->ConnectedSocket.Socket = INVALID_SOCKET;

//            InitializeCriticalSection(&bufferInfo->CriticalSection);
            bufferInfo->Socket = new ICSocket();

            if ( bufferInfo->Socket != NULL )
            {
                error = ERROR_SUCCESS;

                BUFFER_CREATED();
            }
            else
                error = ERROR_NOT_ENOUGH_MEMORY;


//        } else {
//            error = GetLastError();
//        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    if (error != ERROR_SUCCESS) {
        if (bufferInfo != NULL) {
            DEL(bufferInfo);
        }
        *Error = error;
    }

    return bufferInfo;
}


VOID
DestroyBuffer(
    IN LPBUFFER_INFO BufferInfo
    )

/*++

Routine Description:

    Frees resources belonging to BUFFER_INFO and frees memory occupied by
    BUFFER_INFO. BufferInfo must have been removed from any lists first

Arguments:

    BufferInfo  - pointer to BUFFER_INFO to destroy

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                None,
                "DestroyBuffer",
                "%x",
                BufferInfo
                ));

    INET_ASSERT(BufferInfo != NULL);
    INET_ASSERT(BufferInfo->ReferenceCount == 0);
//    INET_ASSERT(! BufferInfo->Socket->IsValid());

    //if (BufferInfo->ConnectedSocket.Socket != INVALID_SOCKET) {
    INET_ASSERT(BufferInfo->Socket);

    if (BufferInfo->Socket->IsValid()) {

        //
        // BUGBUG - do we need to set linger or don't linger?
        //

        //
        // close the socket
        //

        BufferInfo->Socket->Close();
    }

    BufferInfo->Socket->Dereference();

//    if (BufferInfo->RequestEvent != NULL) {
//        CloseHandle(BufferInfo->RequestEvent);
//    }

    //
    // if we allocated a buffer for the response then free it
    //

    if ((BufferInfo->Buffer != NULL)
    && (BufferInfo->Flags & BI_BUFFER_RESPONSE)) {

        FREE_FIXED_MEMORY(BufferInfo->Buffer);

    }

//    DeleteCriticalSection(&BufferInfo->CriticalSection);

    DEL(BufferInfo);

    BUFFER_DESTROYED();

    DEBUG_LEAVE(0);
}


VOID
AcquireBufferLock(
    IN LPBUFFER_INFO BufferInfo
    )

/*++

Routine Description:

    Locks the BUFFER_INFO against simultaneous updates of the Buffer

Arguments:

    BufferInfo  - pointer to BUFFER_INFO to lock

Return Value:

    None.

--*/

{
    INET_ASSERT(BufferInfo != NULL);

//    EnterCriticalSection(&BufferInfo->CriticalSection);
}


VOID
ReleaseBufferLock(
    IN LPBUFFER_INFO BufferInfo
    )

/*++

Routine Description:

    Opposite of AcquireBufferLock

Arguments:

    BufferInfo  - pointer to BUFFER_INFO to unlock

Return Value:

    None.

--*/

{
    INET_ASSERT(BufferInfo != NULL);

//    LeaveCriticalSection(&BufferInfo->CriticalSection);
}


VOID
ReferenceBuffer(
    IN LPBUFFER_INFO BufferInfo
    )

/*++

Routine Description:

    Increments the BUFFER_INFO reference count

Arguments:

    BufferInfo  - pointer to BUFFER_INFO to reference (by 1)

Return Value:

    None.

--*/

{
    INET_ASSERT(BufferInfo != NULL);

    InterlockedIncrement(&BufferInfo->ReferenceCount);
}


LPBUFFER_INFO
DereferenceBuffer(
    IN LPBUFFER_INFO BufferInfo
    )

/*++

Routine Description:

    Reduces the BUFFER_INFO reference count by 1. If the reference count goes
    to zero, the BUFFER_INFO is destroyed

Arguments:

    BufferInfo  - pointer to BUFFER_INFO to dereference/destroy

Return Value:

    DWORD
        previous value of reference count

--*/

{
    INET_ASSERT(BufferInfo != NULL);
    INET_ASSERT(BufferInfo->ReferenceCount >= 1);

    if (InterlockedDecrement(&BufferInfo->ReferenceCount) == 0) {
        DestroyBuffer(BufferInfo);
        BufferInfo = NULL;
    }
    return BufferInfo;
}
