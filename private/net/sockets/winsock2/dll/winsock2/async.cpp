/*++

Copyright (c) 1996 Microsoft Corporation

Module Name:

    Async.c

Abstract:

    This module contains code for the WinSock asynchronous processing
    thread.

Author:

    David Treadwell (davidtr)    25-May-1992

Revision History:

    Keith Moore (keithmo)        18-Jun-1996
        Moved it over to WS2_32.DLL.

--*/


#include "precomp.h"


//
// Private types
//

typedef struct _SOCK_ASYNC_THREAD_PARAMS
{
      LIST_ENTRY    SockAsyncQueueHead;

      HANDLE        SockAsyncQueueEvent;

      LIST_ENTRY    SocketList;

} SOCK_ASYNC_THREAD_PARAMS, *PSOCK_ASYNC_THREAD_PARAMS;

//
// Private globals.
//

BOOL SockAsyncThreadInitialized;
CRITICAL_SECTION SockAsyncLock;
HANDLE SockAsyncQueueEvent;
PLIST_ENTRY SockAsyncQueueHead;
HANDLE SockAsyncCurrentTaskHandle;
HANDLE SockAsyncCancelledTaskHandle;
HMODULE SockAsyncModuleHandle;
LONG SockAsyncTaskHandleCounter;

//
// Private prototypes.
//

DWORD
WINAPI
SockAsyncThread(
    IN PSOCK_ASYNC_THREAD_PARAMS pThreadParams
    );

BOOL
WINAPI
SockAsyncThreadBlockingHook(
    VOID
    );

VOID
SockProcessAsyncGetHost (
    IN HANDLE TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN int Length,
    IN int Type,
    IN char FAR *Buffer,
    IN int BufferLength
    );

VOID
SockProcessAsyncGetProto (
    IN HANDLE TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN char FAR *Buffer,
    IN int BufferLength
    );

VOID
SockProcessAsyncGetServ (
    IN HANDLE TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN char FAR *Protocol,
    IN char FAR *Buffer,
    IN int BufferLength
    );

DWORD
CopyHostentToBuffer(
    IN char FAR * Buffer,
    IN int BufferLength,
    IN PHOSTENT Hostent
    );

DWORD
CopyServentToBuffer(
    IN char FAR *Buffer,
    IN int BufferLength,
    IN PSERVENT Servent
    );

DWORD
CopyProtoentToBuffer(
    IN char FAR *Buffer,
    IN int BufferLength,
    IN PPROTOENT Protoent
    );

DWORD
BytesInHostent(
    PHOSTENT Hostent
    );

DWORD
BytesInServent(
    PSERVENT Servent
    );

DWORD
BytesInProtoent(
    IN PPROTOENT Protoent
    );

#define SockAcquireGlobalLock() EnterCriticalSection( &SockAsyncLock )
#define SockReleaseGlobalLock() LeaveCriticalSection( &SockAsyncLock )


//
// Public functions.
//


BOOL
SockAsyncGlobalInitialize(
    )
{
    
    assert (gDllHandle!=NULL);
    __try {
        InitializeCriticalSection( &SockAsyncLock );
        return TRUE;
    }
    __except (WS2_EXCEPTION_FILTER ()) {
        return FALSE;
    }
}   // SockAsyncGlobalInitialize

VOID
SockAsyncGlobalTerminate(
    VOID
    )
{

    DeleteCriticalSection( &SockAsyncLock );

}   // SockAsyncGlobalTerminate


BOOL
SockCheckAndInitAsyncThread(
    VOID
    )
{

    HANDLE threadHandle;
    DWORD threadId;
    BOOL  startup_done;
    PSOCK_ASYNC_THREAD_PARAMS pThreadParams;
    union {
        WSADATA WSAData;
        TCHAR   ModuleName[MAX_PATH];
    } u;


    //
    // If the async thread is already initialized, return.
    //

    if( SockAsyncThreadInitialized ) {
        return TRUE;
    }

    //
    // Acquire the global lock to synchronize the thread startup.
    //

    SockAcquireGlobalLock();


    //
    // Check again, in case another thread has already initialized
    // the async thread.
    //

    if( !SockAsyncThreadInitialized ) {

        SockAsyncModuleHandle = NULL;
        SockAsyncQueueEvent = NULL;
        pThreadParams = NULL;
        startup_done = FALSE;

        TRY_START (guard_lock) {
            //
            // Initialize globals for the async thread
            //

            pThreadParams = new SOCK_ASYNC_THREAD_PARAMS;
            if (pThreadParams==NULL) {
                DEBUGF (DBG_ERR,
                    ("Allocating async thread parameter block.\n"));
                TRY_THROW(guard_lock);
            }


            SockAsyncQueueHead = &pThreadParams->SockAsyncQueueHead;

            InitializeListHead( SockAsyncQueueHead );

            SockAsyncQueueEvent =
            pThreadParams->SockAsyncQueueEvent = CreateEvent(
                                      NULL,
                                      FALSE,
                                      FALSE,
                                      NULL
                                      );

            if( SockAsyncQueueEvent == NULL ) {
                DEBUGF (DBG_ERR,
                    ("Creating async queue event.\n"));
                TRY_THROW (guard_lock);
            }


            //
            // Add an artificial reference to WS2_32.DLL so that it doesn't
            // go away unexpectedly. We'll remove this reference when we shut
            // down the async thread.
            //

            if( (GetModuleFileName(
                            gDllHandle,
                            u.ModuleName,
                            sizeof (u.ModuleName)
                            )==0) ||
                ((SockAsyncModuleHandle = LoadLibrary(
                            u.ModuleName 
                            ))==NULL) ) {

                DEBUGF (DBG_ERR,
                    ("Referencing ws2_32.dll.\n"));
                TRY_THROW (guard_lock);
            }

            //
            // Add an artificial reference to the startup count so that we
            // won't clean up this dll while the SockAsyncThread is still
            // processing a request
            //

            if ( WSAStartup( 0x202, &u.WSAData ) != 0 ) {
                DEBUGF (DBG_ERR,
                    ("Starting up ws2_32.dll.\n"));
                TRY_THROW (guard_lock);
            }
            startup_done = TRUE;

            //
            // Create the async thread itself.
            //

            threadHandle = CreateThread(
                               NULL,
                               0,
                               (LPTHREAD_START_ROUTINE) SockAsyncThread,
                               pThreadParams,
                               0,
                               &threadId
                               );

            if( threadHandle == NULL ) {
                DEBUGF (DBG_ERR,
                    ("Creating async thread.\n"));
                TRY_THROW (guard_lock);
            }
            //
            // Close the thread handle, indicate a successful result,
            // and jump down to the right cleanup step
            //

            CloseHandle( threadHandle );

            SockAsyncThreadInitialized = TRUE;
        }
        TRY_CATCH (guard_lock) {
            if (startup_done)
                WSACleanup ();

            if (SockAsyncModuleHandle!=NULL) {
                FreeLibrary (SockAsyncModuleHandle);
                SockAsyncModuleHandle = NULL;
            }

            if (SockAsyncQueueEvent!=NULL) {
                CloseHandle( SockAsyncQueueEvent );
                SockAsyncQueueEvent = NULL;
            }
            if (pThreadParams!=NULL)
                delete pThreadParams;
        } TRY_END(guard_lock);

    }


    SockReleaseGlobalLock();

    return SockAsyncThreadInitialized;

} // SockCheckAndInitializeAsyncThread


VOID
SockTerminateAsyncThread(
    VOID
    )
{

    PWINSOCK_CONTEXT_BLOCK contextBlock;

    //
    // If the thread's not running, there's not much to do.
    //

    if( !SockAsyncThreadInitialized ) {

        return;

    }

    //
    // Get an async context block.
    //

    contextBlock = SockAllocateContextBlock( 0 );

    if( contextBlock == NULL ) {

        //
        // !!! Use brute force method!
        //

        return;

    }

    //
    // Initialize the context block for this operation.
    //

    contextBlock->OpCode = WS_OPCODE_TERMINATE;

    //
    // Queue the request to the async thread.  The async thread will
    // kill itself when it receives this request.
    //

    SockQueueRequestToAsyncThread( contextBlock );

    SockAsyncThreadInitialized = FALSE;

} // SockTerminateAsyncThread


PWINSOCK_CONTEXT_BLOCK
SockAllocateContextBlock(
    DWORD AdditionalSpace
    )
{

    PWINSOCK_CONTEXT_BLOCK contextBlock;

    //
    // Allocate memory for the context block plus any additional
    // space requested.
    //

    AdditionalSpace += sizeof(*contextBlock);

    contextBlock = (PWINSOCK_CONTEXT_BLOCK)new BYTE[AdditionalSpace];

    if( contextBlock == NULL ) {

        return NULL;

    }

    //
    // Get a task handle for this context block.
    //

    contextBlock->TaskHandle = (HANDLE)InterlockedIncrement(
                                   &SockAsyncTaskHandleCounter
                                   );

    assert( contextBlock->TaskHandle != NULL );

    //
    // Return the task handle we allocated.
    //

    return contextBlock;

} // SockAllocateContextBlock


VOID
SockFreeContextBlock(
    IN PWINSOCK_CONTEXT_BLOCK ContextBlock
    )
{
    //
    // Just free the block to process heap.
    //

    delete ContextBlock;

} // SockFreeContextBlock


VOID
SockQueueRequestToAsyncThread(
    IN PWINSOCK_CONTEXT_BLOCK ContextBlock
    )
{
    BOOL result;

    //
    // Acquire the lock that protects the async queue list.
    //

    SockAcquireGlobalLock();

    //
    // Insert the context block at the end of the queue.
    //

    InsertTailList(
        SockAsyncQueueHead,
        &ContextBlock->AsyncThreadQueueListEntry
        );

    //
    // Set the queue event so that the async thread wakes up to service
    // this request.
    //

    result = SetEvent( SockAsyncQueueEvent );
    assert( result );

    //
    // Release the resource and return.
    //

    SockReleaseGlobalLock();
    return;

} // SockQueueRequestToAsyncThread

INT
SockCancelAsyncRequest(
    IN HANDLE TaskHandle
    )
{

    PLIST_ENTRY entry;
    PWINSOCK_CONTEXT_BLOCK contextBlock;

    //
    // If the async thread has not been initialized, then this must
    // be an invalid context handle.
    //

    if( !SockAsyncThreadInitialized ) {

        return WSAEINVAL;

    }

    //
    // Hold the lock that protects the async thread context block queue
    // while we do this.  This prevents the async thread from starting
    // new requests while we determine how to execute this cancel.
    //

    SockAcquireGlobalLock();

    //
    // If the specified task handle is currently being processed by the
    // async thread, just set this task handle as the cancelled async
    // thread task handle.  The async thread's blocking hook routine
    // will cancel the request, and the handler routine will not
    // post the message for completion of the request.
    //
    // *** Note that it is possible to complete this request with a
    //     WSAEINVAL while an async request completion message is
    //     about to be posted to the application.  Does this matter?
    //     There is no way an app can distinguish this case from
    //     where the post occurs just before the call to this routine.

    if( TaskHandle == SockAsyncCurrentTaskHandle ) {

        SockAsyncCancelledTaskHandle = TaskHandle;
        SockReleaseGlobalLock();

        return NO_ERROR;

    }

    //
    // Attempt to find the task handle in the queue of context blocks to
    // the async thread.
    //

    for( entry = SockAsyncQueueHead->Flink;
         entry != SockAsyncQueueHead;
         entry = entry->Flink ) {

        contextBlock = CONTAINING_RECORD(
                           entry,
                           WINSOCK_CONTEXT_BLOCK,
                           AsyncThreadQueueListEntry
                           );

        if( TaskHandle == contextBlock->TaskHandle ) {

            //
            // We found the correct task handle.  Remove it from the list.
            //

            RemoveEntryList( entry );

            //
            // Release the lock, free the context block, and return.
            //

            SockReleaseGlobalLock( );
            SockFreeContextBlock( contextBlock );

            return NO_ERROR;

        }

    }

    //
    // The task handle was not found on the list.  Either the request
    // was already completed or the task handle was just plain bogus.
    // In either case, fail the request.
    //

    SockReleaseGlobalLock();
    return WSAEINVAL;

}   // SockCancelAsyncRequest


DWORD
WINAPI
SockAsyncThread(
    IN PSOCK_ASYNC_THREAD_PARAMS    pThreadParams
    )
{

    PWINSOCK_CONTEXT_BLOCK contextBlock;
    PLIST_ENTRY listEntry;
    FARPROC previousHook;
    HANDLE sockAsyncQueueEvent = pThreadParams->SockAsyncQueueEvent;
    PLIST_ENTRY sockAsyncQueueHead = &pThreadParams->SockAsyncQueueHead;

    //
    // Set up our blocking hook routine.  We'll use it to handle
    // cancelling async requests.
    //

    previousHook = WSASetBlockingHook(
                       (FARPROC)SockAsyncThreadBlockingHook
                       );

    //
    // Loop forever dispatching actions.
    //

    while( TRUE ) {

        //
        // Wait for the async queue event to indicate that there is
        // something on the queue.
        //

        WaitForSingleObject(
            sockAsyncQueueEvent,
            INFINITE
            );

        //
        // Acquire the lock that protects the async queue.
        //

        SockAcquireGlobalLock();

        //
        // As long as there are items to process, process them.
        //

        while( !IsListEmpty( sockAsyncQueueHead ) ) {

            //
            // Remove the first item from the queue.
            //

            listEntry = RemoveHeadList( sockAsyncQueueHead );

            contextBlock = CONTAINING_RECORD(
                               listEntry,
                               WINSOCK_CONTEXT_BLOCK,
                               AsyncThreadQueueListEntry
                               );

            //
            // Remember the task handle that we're processing.  This
            // is necessary in order to support WSACancelAsyncRequest.
            //

            SockAsyncCurrentTaskHandle = contextBlock->TaskHandle;

            //
            // Release the list lock while we're processing the request.
            //

            SockReleaseGlobalLock();

            //
            // Act based on the opcode in the context block.
            //

            switch( contextBlock->OpCode ) {

            case WS_OPCODE_GET_HOST_BY_ADDR:
            case WS_OPCODE_GET_HOST_BY_NAME:

                SockProcessAsyncGetHost(
                    contextBlock->TaskHandle,
                    contextBlock->OpCode,
                    contextBlock->Overlay.AsyncGetHost.hWnd,
                    contextBlock->Overlay.AsyncGetHost.wMsg,
                    contextBlock->Overlay.AsyncGetHost.Filter,
                    contextBlock->Overlay.AsyncGetHost.Length,
                    contextBlock->Overlay.AsyncGetHost.Type,
                    contextBlock->Overlay.AsyncGetHost.Buffer,
                    contextBlock->Overlay.AsyncGetHost.BufferLength
                    );

                break;

            case WS_OPCODE_GET_PROTO_BY_NUMBER:
            case WS_OPCODE_GET_PROTO_BY_NAME:

                SockProcessAsyncGetProto(
                    contextBlock->TaskHandle,
                    contextBlock->OpCode,
                    contextBlock->Overlay.AsyncGetProto.hWnd,
                    contextBlock->Overlay.AsyncGetProto.wMsg,
                    contextBlock->Overlay.AsyncGetProto.Filter,
                    contextBlock->Overlay.AsyncGetProto.Buffer,
                    contextBlock->Overlay.AsyncGetProto.BufferLength
                    );

                break;

            case WS_OPCODE_GET_SERV_BY_PORT:
            case WS_OPCODE_GET_SERV_BY_NAME:

                SockProcessAsyncGetServ(
                    contextBlock->TaskHandle,
                    contextBlock->OpCode,
                    contextBlock->Overlay.AsyncGetServ.hWnd,
                    contextBlock->Overlay.AsyncGetServ.wMsg,
                    contextBlock->Overlay.AsyncGetServ.Filter,
                    contextBlock->Overlay.AsyncGetServ.Protocol,
                    contextBlock->Overlay.AsyncGetServ.Buffer,
                    contextBlock->Overlay.AsyncGetServ.BufferLength
                    );

                break;

            case WS_OPCODE_TERMINATE:

                //
                // Decrement our init ref count (can safely clean up now)
                //

                WSACleanup();

                //
                // Free the termination context block.
                //

                SockFreeContextBlock( contextBlock );

                //
                // Clear out the queue of async requests.
                //

                SockAcquireGlobalLock();

                while( !IsListEmpty( sockAsyncQueueHead ) ) {

                    listEntry = RemoveHeadList( sockAsyncQueueHead );

                    contextBlock = CONTAINING_RECORD(
                                       listEntry,
                                       WINSOCK_CONTEXT_BLOCK,
                                       AsyncThreadQueueListEntry
                                       );

                    SockFreeContextBlock( contextBlock );

                }

                SockReleaseGlobalLock();

                //
                // Clean up thread-specific resources
                //

                CloseHandle( sockAsyncQueueEvent );

                delete pThreadParams;

                //
                // Remove the artifical reference we added in
                // SockCheckAndInitAsyncThread() and exit this thread.
                //

                FreeLibraryAndExitThread(
                    SockAsyncModuleHandle,
                    0
                    );

                //
                // We should never get here, but just in case...
                //

                return 0;

            default:

                //
                // We got a bogus opcode.
                //

                assert( !"Bogus async opcode" );
                break;
            }

            //
            // Set the variable that holds the task handle that we're
            // currently processing to 0, since we're not actually
            // processing a task handle right now.
            //

            SockAsyncCurrentTaskHandle = NULL;

            //
            // Free the context block, reacquire the list lock, and
            // continue.
            //

            SockFreeContextBlock( contextBlock );
            SockAcquireGlobalLock();

        }

        //
        // Release the list lock and redo the wait.
        //

        SockReleaseGlobalLock();

    }

} // SockAsyncThread


BOOL
WINAPI
SockAsyncThreadBlockingHook(
    VOID
    )
{

    //
    // If the current async request is being cancelled, blow away
    // the current blocking call.
    //

    if( SockAsyncCurrentTaskHandle == SockAsyncCancelledTaskHandle ) {

        int error;

        error = WSACancelBlockingCall();
        assert( error == NO_ERROR );
    }

    return FALSE;

} // SockAsyncThreadBlockingHook


VOID
SockProcessAsyncGetHost(
    IN HANDLE TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN int Length,
    IN int Type,
    IN char FAR *Buffer,
    IN int BufferLength
    )
{

    PHOSTENT returnHost;
    DWORD requiredBufferLength = 0;
    LPARAM lParam;
    INT error;
    PWINSOCK_POST_ROUTINE   sockPostRoutine;

    assert( OpCode == WS_OPCODE_GET_HOST_BY_ADDR ||
            OpCode == WS_OPCODE_GET_HOST_BY_NAME );

    //
    // Get the necessary information.
    //

    if( OpCode == WS_OPCODE_GET_HOST_BY_ADDR ) {

        returnHost = gethostbyaddr(
                          Filter,
                          Length,
                          Type
                          );

    } else {

        returnHost = gethostbyname(
                          Filter
                          );

    }

    if( returnHost == NULL ) {

        error = WSAGetLastError();

    }

    //
    // Hold the lock that protects the async thread context block queue
    // while we do this.  This prevents a race between this thread and
    // any thread invoking WSACancelAsyncRequest().
    //

    SockAcquireGlobalLock();

    //
    // If this request was cancelled, just return.
    //

    if( TaskHandle == SockAsyncCancelledTaskHandle ) {

        SockReleaseGlobalLock();
        return;

    }

    //
    // Copy the hostent structure to the output buffer.
    //

    if( returnHost != NULL ) {

        requiredBufferLength = CopyHostentToBuffer(
                                   Buffer,
                                   BufferLength,
                                   returnHost
                                   );

        if( requiredBufferLength > (DWORD)BufferLength ) {

            error = WSAENOBUFS;

        } else {

            error = NO_ERROR;

        }

    }

    //
    // Set the current async thread task handle to 0 so that if a cancel
    // request comes in after this point it is failed properly.
    //

    SockAsyncCurrentTaskHandle = NULL;

    //
    // Release the global lock.
    //

    SockReleaseGlobalLock();

    //
    // Build lParam for the message we'll post to the application.
    // The high 16 bits are the error code, the low 16 bits are
    // the minimum buffer size required for the operation.
    //

    lParam = WSAMAKEASYNCREPLY( requiredBufferLength, error );

    //
    // Post a message to the application indication that the data it
    // requested is available.
    //

    assert( sizeof(TaskHandle) == sizeof(HANDLE) );

    sockPostRoutine = GET_SOCK_POST_ROUTINE ();

    //
    // !!! Need a mechanism to repost if the post failed!
    //

    if (!sockPostRoutine || !sockPostRoutine(
                 hWnd,
                 wMsg,
                 (WPARAM)TaskHandle,
                 lParam
                 )) {


        // Rem assert, since this might be an "orphaned" SockAsyncThread
        // in the process of tearing itself down
        //
        //assert( !"SockPostRoutine failed" );

    }

}   // SockProcessAsyncGetHost


VOID
SockProcessAsyncGetProto(
    IN HANDLE TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN char FAR *Buffer,
    IN int BufferLength
    )
{

    PPROTOENT returnProto;
    DWORD requiredBufferLength = 0;
    LPARAM lParam;
    INT error;
    PWINSOCK_POST_ROUTINE   sockPostRoutine;

    assert( OpCode == WS_OPCODE_GET_PROTO_BY_NAME ||
            OpCode == WS_OPCODE_GET_PROTO_BY_NUMBER );

    //
    // Get the necessary information.
    //

    if( OpCode == WS_OPCODE_GET_PROTO_BY_NAME ) {

        returnProto = getprotobyname( Filter );

    } else {

        returnProto = getprotobynumber( (int)(LONG_PTR)Filter );

    }

    if( returnProto == NULL ) {

        error = WSAGetLastError();

    }

    //
    // Hold the lock that protects the async thread context block queue
    // while we do this.  This prevents a race between this thread and
    // any thread invoking WSACancelAsyncRequest().
    //

    SockAcquireGlobalLock();

    //
    // If this request was cancelled, just return.
    //

    if( TaskHandle == SockAsyncCancelledTaskHandle ) {

        SockReleaseGlobalLock();
        return;

    }

    //
    // Copy the protoent structure to the output buffer.
    //

    if( returnProto != NULL ) {

        requiredBufferLength = CopyProtoentToBuffer(
                                   Buffer,
                                   BufferLength,
                                   returnProto
                                   );

        if( requiredBufferLength > (DWORD)BufferLength ) {

            error = WSAENOBUFS;

        } else {

            error = NO_ERROR;

        }

    }

    //
    // Set the current async thread task handle to 0 so that if a cancel
    // request comes in after this point it is failed properly.
    //

    SockAsyncCurrentTaskHandle = NULL;

    //
    // Release the global lock.
    //

    SockReleaseGlobalLock();

    //
    // Build lParam for the message we'll post to the application.
    // The high 16 bits are the error code, the low 16 bits are
    // the minimum buffer size required for the operation.
    //

    lParam = WSAMAKEASYNCREPLY( requiredBufferLength, error );

    //
    // Post a message to the application indication that the data it
    // requested is available.
    //

    assert( sizeof(TaskHandle) == sizeof(HANDLE) );

    sockPostRoutine = GET_SOCK_POST_ROUTINE ();
    //
    // !!! Need a mechanism to repost if the post failed!
    //

    if (!sockPostRoutine || !sockPostRoutine(
                 hWnd,
                 wMsg,
                 (WPARAM)TaskHandle,
                 lParam
                 )) {


        // Rem assert, since this might be an "orphaned" SockAsyncThread
        // in the process of tearing itself down
        //
        //assert( !"SockPostRoutine failed" );

    }

}   // SockProcessAsyncGetProto


VOID
SockProcessAsyncGetServ(
    IN HANDLE TaskHandle,
    IN DWORD OpCode,
    IN HWND hWnd,
    IN unsigned int wMsg,
    IN char FAR *Filter,
    IN char FAR *Protocol,
    IN char FAR *Buffer,
    IN int BufferLength
    )
{

    PSERVENT returnServ;
    DWORD requiredBufferLength = 0;
    LPARAM lParam;
    INT error;
    PWINSOCK_POST_ROUTINE   sockPostRoutine;

    assert( OpCode == WS_OPCODE_GET_SERV_BY_NAME ||
            OpCode == WS_OPCODE_GET_SERV_BY_PORT );

    //
    // Get the necessary information.
    //

    if( OpCode == WS_OPCODE_GET_SERV_BY_NAME ) {

        returnServ = getservbyname(
                         Filter,
                         Protocol
                         );

    } else {

        returnServ = getservbyport(
                         (int)(LONG_PTR)Filter,
                         Protocol
                         );

    }

    if( returnServ == NULL ) {

        error = GetLastError();

    }

    //
    // Hold the lock that protects the async thread context block queue
    // while we do this.  This prevents a race between this thread and
    // any thread invoking WSACancelAsyncRequest().
    //

    SockAcquireGlobalLock();

    //
    // If this request was cancelled, just return.
    //

    if( TaskHandle == SockAsyncCancelledTaskHandle ) {

        SockReleaseGlobalLock();
        return;

    }

    //
    // Copy the servent structure to the output buffer.
    //

    if( returnServ != NULL ) {

        requiredBufferLength = CopyServentToBuffer(
                                   Buffer,
                                   BufferLength,
                                   returnServ
                                   );

        if( requiredBufferLength > (DWORD)BufferLength ) {

            error = WSAENOBUFS;

        } else {

            error = NO_ERROR;

        }

    }

    //
    // Set the current async thread task handle to 0 so that if a cancel
    // request comes in after this point it is failed properly.
    //

    SockAsyncCurrentTaskHandle = NULL;

    //
    // Release the global lock.
    //

    SockReleaseGlobalLock();

    //
    // Build lParam for the message we'll post to the application.
    // The high 16 bits are the error code, the low 16 bits are
    // the minimum buffer size required for the operation.
    //

    lParam = WSAMAKEASYNCREPLY( requiredBufferLength, error );

    //
    // Post a message to the application indication that the data it
    // requested is available.
    //

    assert( sizeof(TaskHandle) == sizeof(HANDLE) );


    sockPostRoutine = GET_SOCK_POST_ROUTINE ();
    //
    // !!! Need a mechanism to repost if the post failed!
    //

    if (!sockPostRoutine || !sockPostRoutine(
                 hWnd,
                 wMsg,
                 (WPARAM)TaskHandle,
                 lParam
                 )) {


        // Rem assert, since this might be an "orphaned" SockAsyncThread
        // in the process of tearing itself down
        //
        //assert( !"SockPostRoutine failed" );

    }
}   // SockProcessAsyncGetServ



DWORD
CopyHostentToBuffer(
    char FAR *Buffer,
    int BufferLength,
    PHOSTENT Hostent
    )
{
    DWORD requiredBufferLength;
    DWORD bytesFilled;
    PCHAR currentLocation = Buffer;
    DWORD aliasCount;
    DWORD addressCount;
    DWORD i;
    PHOSTENT outputHostent = (PHOSTENT)Buffer;

    //
    // Determine how many bytes are needed to fully copy the structure.
    //

    requiredBufferLength = BytesInHostent( Hostent );

    //
    // Zero the user buffer.
    //

    if ( (DWORD)BufferLength > requiredBufferLength ) {
        ZeroMemory( Buffer, requiredBufferLength );
    } else {
        ZeroMemory( Buffer, BufferLength );
    }

    //
    // Copy over the hostent structure if it fits.
    //

    bytesFilled = sizeof(*Hostent);

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    CopyMemory( currentLocation, Hostent, sizeof(*Hostent) );
    currentLocation = Buffer + bytesFilled;

    outputHostent->h_name = NULL;
    outputHostent->h_aliases = NULL;
    outputHostent->h_addr_list = NULL;

    //
    // Count the host's aliases and set up an array to hold pointers to
    // them.
    //

    for ( aliasCount = 0;
          Hostent->h_aliases[aliasCount] != NULL;
          aliasCount++ );

    bytesFilled += (aliasCount+1) * sizeof(char FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Hostent->h_aliases = NULL;
        return requiredBufferLength;
    }

    outputHostent->h_aliases = (char FAR * FAR *)currentLocation;
    currentLocation = Buffer + bytesFilled;

    //
    // Count the host's addresses and set up an array to hold pointers to
    // them.
    //

    for ( addressCount = 0;
          Hostent->h_addr_list[addressCount] != NULL;
          addressCount++ );

    bytesFilled += (addressCount+1) * sizeof(void FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Hostent->h_addr_list = NULL;
        return requiredBufferLength;
    }

    outputHostent->h_addr_list = (char FAR * FAR *)currentLocation;
    currentLocation = Buffer + bytesFilled;

    //
    // Start filling in addresses.  Do addresses before filling in the
    // host name and aliases in order to avoid alignment problems.
    //

    for ( i = 0; i < addressCount; i++ ) {

        bytesFilled += Hostent->h_length;

        if ( bytesFilled > (DWORD)BufferLength ) {
            outputHostent->h_addr_list[i] = NULL;
            return requiredBufferLength;
        }

        outputHostent->h_addr_list[i] = currentLocation;

        CopyMemory(
            currentLocation,
            Hostent->h_addr_list[i],
            Hostent->h_length
            );

        currentLocation = Buffer + bytesFilled;
    }

    outputHostent->h_addr_list[i] = NULL;

    //
    // Copy the host name if it fits.
    //

    bytesFilled += strlen( Hostent->h_name ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    outputHostent->h_name = currentLocation;

    CopyMemory( currentLocation, Hostent->h_name, strlen( Hostent->h_name ) + 1 );
    currentLocation = Buffer + bytesFilled;

    //
    // Start filling in aliases.
    //

    for ( i = 0; i < aliasCount; i++ ) {

        bytesFilled += strlen( Hostent->h_aliases[i] ) + 1;

        if ( bytesFilled > (DWORD)BufferLength ) {
            outputHostent->h_aliases[i] = NULL;
            return requiredBufferLength;
        }

        outputHostent->h_aliases[i] = currentLocation;

        CopyMemory(
            currentLocation,
            Hostent->h_aliases[i],
            strlen( Hostent->h_aliases[i] ) + 1
            );

        currentLocation = Buffer + bytesFilled;
    }

    outputHostent->h_aliases[i] = NULL;

    return requiredBufferLength;

}   // CopyHostentToBuffer



DWORD
CopyServentToBuffer(
    IN char FAR *Buffer,
    IN int BufferLength,
    IN PSERVENT Servent
    )
{
    DWORD requiredBufferLength;
    DWORD bytesFilled;
    PCHAR currentLocation = Buffer;
    DWORD aliasCount;
    DWORD i;
    PSERVENT outputServent = (PSERVENT)Buffer;

    //
    // Determine how many bytes are needed to fully copy the structure.
    //

    requiredBufferLength = BytesInServent( Servent );

    //
    // Zero the user buffer.
    //

    if ( (DWORD)BufferLength > requiredBufferLength ) {
        ZeroMemory( Buffer, requiredBufferLength );
    } else {
        ZeroMemory( Buffer, BufferLength );
    }

    //
    // Copy over the servent structure if it fits.
    //

    bytesFilled = sizeof(*Servent);

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    CopyMemory( currentLocation, Servent, sizeof(*Servent) );
    currentLocation = Buffer + bytesFilled;

    outputServent->s_name = NULL;
    outputServent->s_aliases = NULL;
    outputServent->s_proto = NULL;

    //
    // Count the service's aliases and set up an array to hold pointers to
    // them.
    //

    for ( aliasCount = 0;
          Servent->s_aliases[aliasCount] != NULL;
          aliasCount++ );

    bytesFilled += (aliasCount+1) * sizeof(char FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Servent->s_aliases = NULL;
        return requiredBufferLength;
    }

    outputServent->s_aliases = (char FAR * FAR *)currentLocation;
    currentLocation = Buffer + bytesFilled;

    //
    // Copy the service name if it fits.
    //

    bytesFilled += strlen( Servent->s_name ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    outputServent->s_name = currentLocation;

    CopyMemory( currentLocation, Servent->s_name, strlen( Servent->s_name ) + 1 );
    currentLocation = Buffer + bytesFilled;

    //
    // Copy the protocol name if it fits.
    //

    bytesFilled += strlen( Servent->s_proto ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    outputServent->s_proto = currentLocation;

    CopyMemory( currentLocation, Servent->s_proto, strlen( Servent->s_proto ) + 1 );
    currentLocation = Buffer + bytesFilled;

    //
    // Start filling in aliases.
    //

    for ( i = 0; i < aliasCount; i++ ) {

        bytesFilled += strlen( Servent->s_aliases[i] ) + 1;

        if ( bytesFilled > (DWORD)BufferLength ) {
            outputServent->s_aliases[i] = NULL;
            return requiredBufferLength;
        }

        outputServent->s_aliases[i] = currentLocation;

        CopyMemory(
            currentLocation,
            Servent->s_aliases[i],
            strlen( Servent->s_aliases[i] ) + 1
            );

        currentLocation = Buffer + bytesFilled;
    }

    outputServent->s_aliases[i] = NULL;

    return requiredBufferLength;

}   // CopyServentToBuffer



DWORD
CopyProtoentToBuffer(
    IN char FAR *Buffer,
    IN int BufferLength,
    IN PPROTOENT Protoent
    )
{
    DWORD requiredBufferLength;
    DWORD bytesFilled;
    PCHAR currentLocation = Buffer;
    DWORD aliasCount;
    DWORD i;
    PPROTOENT outputProtoent = (PPROTOENT)Buffer;

    //
    // Determine how many bytes are needed to fully copy the structure.
    //

    requiredBufferLength = BytesInProtoent( Protoent );

    //
    // Zero the user buffer.
    //

    if ( (DWORD)BufferLength > requiredBufferLength ) {
        ZeroMemory( Buffer, requiredBufferLength );
    } else {
        ZeroMemory( Buffer, BufferLength );
    }

    //
    // Copy over the protoent structure if it fits.
    //

    bytesFilled = sizeof(*Protoent);

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    CopyMemory( currentLocation, Protoent, sizeof(*Protoent) );
    currentLocation = Buffer + bytesFilled;

    outputProtoent->p_name = NULL;
    outputProtoent->p_aliases = NULL;

    //
    // Count the protocol's aliases and set up an array to hold pointers to
    // them.
    //

    for ( aliasCount = 0;
          Protoent->p_aliases[aliasCount] != NULL;
          aliasCount++ );

    bytesFilled += (aliasCount+1) * sizeof(char FAR *);

    if ( bytesFilled > (DWORD)BufferLength ) {
        Protoent->p_aliases = NULL;
        return requiredBufferLength;
    }

    outputProtoent->p_aliases = (char FAR * FAR *)currentLocation;
    currentLocation = Buffer + bytesFilled;

    //
    // Copy the protocol name if it fits.
    //

    bytesFilled += strlen( Protoent->p_name ) + 1;

    if ( bytesFilled > (DWORD)BufferLength ) {
        return requiredBufferLength;
    }

    outputProtoent->p_name = currentLocation;

    CopyMemory( currentLocation, Protoent->p_name, strlen( Protoent->p_name ) + 1 );
    currentLocation = Buffer + bytesFilled;

    //
    // Start filling in aliases.
    //

    for ( i = 0; i < aliasCount; i++ ) {

        bytesFilled += strlen( Protoent->p_aliases[i] ) + 1;

        if ( bytesFilled > (DWORD)BufferLength ) {
            outputProtoent->p_aliases[i] = NULL;
            return requiredBufferLength;
        }

        outputProtoent->p_aliases[i] = currentLocation;

        CopyMemory(
            currentLocation,
            Protoent->p_aliases[i],
            strlen( Protoent->p_aliases[i] ) + 1
            );

        currentLocation = Buffer + bytesFilled;
    }

    outputProtoent->p_aliases[i] = NULL;

    return requiredBufferLength;

}   // CopyProtoentToBuffer



DWORD
BytesInHostent(
    PHOSTENT Hostent
    )
{
    DWORD total;
    int i;

    total = sizeof(HOSTENT);
    total += strlen( Hostent->h_name ) + 1;

    //
    // Account for the NULL terminator pointers at the end of the
    // alias and address arrays.
    //

    total += sizeof(char *) + sizeof(char *);

    for ( i = 0; Hostent->h_aliases[i] != NULL; i++ ) {
        total += strlen( Hostent->h_aliases[i] ) + 1 + sizeof(char *);
    }

    for ( i = 0; Hostent->h_addr_list[i] != NULL; i++ ) {
        total += Hostent->h_length + sizeof(char *);
    }

    //
    // Pad the answer to an eight-byte boundary.
    //

    return (total + 7) & ~7;

}   // BytesInHostent



DWORD
BytesInServent(
    IN PSERVENT Servent
    )
{
    DWORD total;
    int i;

    total = sizeof(SERVENT);
    total += strlen( Servent->s_name ) + 1;
    total += strlen( Servent->s_proto ) + 1;
    total += sizeof(char *);

    for ( i = 0; Servent->s_aliases[i] != NULL; i++ ) {
        total += strlen( Servent->s_aliases[i] ) + 1 + sizeof(char *);
    }

    return total;

}   // BytesInServent



DWORD
BytesInProtoent(
    IN PPROTOENT Protoent
    )
{
    DWORD total;
    int i;

    total = sizeof(PROTOENT);
    total += strlen( Protoent->p_name ) + 1;
    total += sizeof(char *);

    for ( i = 0; Protoent->p_aliases[i] != NULL; i++ ) {
        total += strlen( Protoent->p_aliases[i] ) + 1 + sizeof(char *);
    }

    return total;

}   // BytesInProtoent
