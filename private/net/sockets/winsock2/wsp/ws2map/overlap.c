/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    overlap.c

Abstract:

    This module contains overlapped IO routines used by the Winsock 2 to
    Winsock 1.1 Mapper Service Provider.

    The following routines are exported by this module:

        SockCompleteRequest()
        SockInitializeOverlappedThread()

Author:

    Keith Moore (keithmo) 11-Jul-1996

Revision History:

--*/


#include "precomp.h"
#pragma hdrstop


//
// Private prototypes.
//

VOID
CALLBACK
SockUserApc(
    ULONG_PTR Context
    );


//
// Public functions.
//


VOID
SockCompleteRequest(
    PSOCKET_INFORMATION SocketInfo,
    DWORD Status,
    DWORD Information,
    LPWSAOVERLAPPED Overlapped,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine,
    LPWSATHREADID ThreadId
    )

/*++

Routine Description:

    Completes a socket IO request. This routine checks the state of the
    given socket. If the socket is overlapped, and an overlapped structure
    was provided, then the appropriate completion mechanism is invoked.

Arguments:

    SocketInfo - The socket to complete.

    Status - The completion status.

    Information - Completion information. Usually the number of bytes
        transferred.

    Overlapped - Pointer to an WSAOVERLAPPED structure.

    CompletionRoutine - Optional pointer to a completion routine to
        schedule.

    ThreadId - Identifies the target thread for the completion routine.

Return Value:

    None.

--*/

{

    INT result;
    INT err;
    PSOCK_IO_STATUS ioStatus;

    SOCK_ASSERT( SocketInfo != NULL );
    SOCK_ASSERT( ThreadId != NULL );
    SOCK_ASSERT( Status != WSA_IO_PENDING );

    IF_DEBUG(OVERLAP) {

        SOCK_PRINT((
            "SockCompleteRequest: socket %lx, status %d, info %lu\n",
            SocketInfo,
            Status,
            Information
            ));

    }

    //
    // Determine if we need to even bother with this stuff.
    //

    if( ( SocketInfo->CreationFlags & WSA_FLAG_OVERLAPPED ) == 0 ||
        Overlapped == NULL ) {

        return;

    }

    //
    // OK, we've got an overlapped socket and an overlapped structure.
    // Update the info in the overlapped structure.
    //

    ioStatus = SOCK_OVERLAPPED_TO_IO_STATUS(Overlapped);

    ioStatus->Status = Status;
    ioStatus->Information = Information;

    //
    // If we've got a completion routine, schedule it. Otherwise, if the
    // overlapped structure has an event handle, signal it.
    //

    if( CompletionRoutine != NULL ) {

        //
        // On 64-bit system both Offset and OffsetHigh fields are used to store
        // the pointer to the completion routine
        //
#ifdef _WIN64
        *((LPVOID *)(&Overlapped->Offset)) = CompletionRoutine;
#else
        Overlapped->Offset = (DWORD)CompletionRoutine;
#endif

        result = SockUpcallTable.lpWPUQueueApc(
                     ThreadId,
                     &SockUserApc,
                     (ULONG_PTR)Overlapped,
                     &err
                     );

        if( result != NO_ERROR ) {

            SOCK_PRINT((
                "SockCompleteRequest: WPUQueueApc failed, error %d\n",
                err
                ));

        }

    } else {

        if( Overlapped->hEvent != NULL ) {

            SetEvent( Overlapped->hEvent );

        }

    }

}   // SockCompleteRequest



BOOL
SockInitializeOverlappedThread(
    PSOCKET_INFORMATION SocketInfo,
    PSOCK_OVERLAPPED_DATA OverlappedData,
    LPTHREAD_START_ROUTINE ThreadStartAddress
    )

/*++

Routine Description:

    Initializes the SOCK_OVERLAPPED_DATA for the given socket, starting
    the worker thread if necessary.

    N.B. This MUST be called with the socket lock held!

Arguments:

    SocketInfo - The socket.

    OverlappedData - Points to other SocketInfo->OverlappedRecv or
        SocketInfo->OverlappedSend.

    ThreadStartAddress - Points to the worker thread.

Return Value:

    BOOL - TRUE if everything initialized successfully, FALSE otherwise.

--*/

{

    HANDLE threadHandle;
    DWORD threadId;

    //
    // Sanity check.
    //

    SOCK_ASSERT( SocketInfo != NULL );
    SOCK_ASSERT( OverlappedData != NULL );
    SOCK_ASSERT( ThreadStartAddress != NULL );
    SOCK_ASSERT( OverlappedData == &SocketInfo->OverlappedRecv ||
                 OverlappedData == &SocketInfo->OverlappedSend );

    //
    // Bail if everything is already initialized.
    //

    if( OverlappedData->WakeupEvent != NULL ) {

        return TRUE;

    }

    //
    // Create the event object.
    //

    OverlappedData->WakeupEvent = CreateEvent(
                                      NULL,
                                      FALSE,
                                      FALSE,
                                      NULL
                                      );

    if( OverlappedData->WakeupEvent == NULL ) {

        return FALSE;

    }

    //
    // Add a new reference to the socket. The worker thread
    // will remove this reference before the thread exits.
    //

    SockReferenceSocket( SocketInfo );

    //
    // Create the worker thread.
    //

    threadHandle = SockCreateWorkerThread(
                       NULL,
                       0,
                       ThreadStartAddress,
                       (LPVOID)SocketInfo,
                       0,
                       &threadId
                       );

    if( threadHandle == NULL ) {

        CloseHandle( OverlappedData->WakeupEvent );
        OverlappedData->WakeupEvent = NULL;

        SockDereferenceSocket( SocketInfo );
        return FALSE;

    }

    return TRUE;

}   // SockInitializeOverlappedThread


//
// Private functions.
//


VOID
CALLBACK
SockUserApc(
    ULONG_PTR Context
    )

/*++

Routine Description:

    Private APC completion routine. This routine unpacks the necessary
    parameters, then invokes the user's completion routine.

Arguments:

    Context - Actually a pointer to the request's WSAOVERLAPPED structure.

Return Value:

    None.

--*/

{

    LPWSAOVERLAPPED Overlapped;
    LPWSAOVERLAPPED_COMPLETION_ROUTINE CompletionRoutine;
    PSOCK_IO_STATUS ioStatus;

    //
    // Retrieve a pointer to the WSAOVERLAPPED structure, extract the
    // pointer to the completion routine, and construct a pointer to
    // the SOCK_IO_STATUS block.
    //

    Overlapped = (LPWSAOVERLAPPED)Context;
    SOCK_ASSERT( Overlapped != NULL );

    //
    // On 64-bit system both Offset and OffsetHigh fields are used to store the pointer
    // to the completion routine
    //
#ifdef _WIN64
    CompletionRoutine = *((LPWSAOVERLAPPED_COMPLETION_ROUTINE *)(&Overlapped->Offset));
#else
    CompletionRoutine = (LPWSAOVERLAPPED_COMPLETION_ROUTINE)Overlapped->Offset;
#endif

    SOCK_ASSERT( CompletionRoutine != NULL );

    ioStatus = SOCK_OVERLAPPED_TO_IO_STATUS(Overlapped);

    //
    // Invoke the user's completion routine.
    //

    (CompletionRoutine)(
        ioStatus->Status,
        ioStatus->Information,
        Overlapped,
        0
        );

}   // SockUserApc

