/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    async.c

Abstract:

    This module contains code for the WinSock asynchronous processing
    thread.  It is necessary to have this as a separate thread because
    of postprocessing required for WSAAsyncSelect and WSAConnect on
    non-blocking sockets.  We cannot use application threads as we are
    not guaranteed that such threads wait alertably (so we can run APCs
    in them)

Author:

    David Treadwell (davidtr)    25-May-1992

Revision History:
    Vadim Eydelman  (vadime)    Summer 1998.  Rewrote to use completion port
                                instead of APCs to remove one context switch

--*/

#include "winsockp.h"

#if DBG
LONG   SockpAsyncThreadId = 0;
#endif

DWORD
SockAsyncThread (
    IN PVOID Param
    );


BOOLEAN
SockCheckAndReferenceAsyncThread (
    VOID
    )
/*
    Initializes async thread if not already running and references it
    to make sure it won't go away until IO is completed
*/

{
    NTSTATUS status;
    HANDLE threadHandle;
    DWORD threadId;
    HINSTANCE instance;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;
    CHAR moduleFileName[MAX_PATH];
    LONG    count;
    PWINSOCK_TLS_DATA tlsData;
    HANDLE  threadEvent;

    //
    // Try to get a reference to a thread
    //
    do {
        //
        // If thread has already been initialized, just
        // increment the reference count, otherwise start it.
        //
        count = (LONG volatile)SockAsyncThreadReferenceCount;
        WS_ASSERT (count>=0);
        if ((count>0) && 
                (InterlockedCompareExchange (
                    &SockAsyncThreadReferenceCount,
                    (count + 1),
                    count)==count)) {
            return TRUE;
        }
    }
    while (count>0);

    //
    // Acquire the global lock to synchronize the thread startup.
    //

	SockAcquireRwLockExclusive (&SocketGlobalLock);


    //
    // Repeat under the lock.
    //
    do {
        //
        // If thread has already been initialized, just
        // increment the reference count, otherwise start it.
        //
        count = (LONG volatile)SockAsyncThreadReferenceCount;
        WS_ASSERT (count>=0);
        if ((count>0) && 
                (InterlockedCompareExchange (
                    &SockAsyncThreadReferenceCount,
                    (count + 1),
                    count)==count)) {
            SockReleaseRwLockExclusive (&SocketGlobalLock);
            return TRUE;
        }
    }
    while (count>0);


    //
    // Initialize globals for the async thread.
    //

    if (SockAsyncQueuePort==NULL) {
        SYSTEM_INFO systemInfo;

        GetSystemInfo( &systemInfo );

        //
        // Only create completion port if not already created.
        // This can happen if we are cleaned up but not unloaded before
        // this call is made.
        //
        status = NtCreateIoCompletion(
                     &SockAsyncQueuePort,
                     IO_COMPLETION_ALL_ACCESS,
                     NULL,                              // Object attributes
                     systemInfo.dwNumberOfProcessors+1  // Count
                     );

        if ( !NT_SUCCESS(status) ) {
            WS_PRINT(( "SockCheckAndInitAsyncThread: NtCreateIoCompletion failed: %X\n",
                           status ));
            SockReleaseRwLockExclusive (&SocketGlobalLock);
            return FALSE;
        }

        //
        // Protect our handle from being closed by random NtClose call
        // by some messed up dll or application
        //
        HandleInfo.ProtectFromClose = TRUE;
        HandleInfo.Inherit = FALSE;

        status = NtSetInformationObject( SockAsyncQueuePort,
                        ObjectHandleFlagInformation,
                        &HandleInfo,
                        sizeof( HandleInfo )
                      );
        //
        // This shouldn't fail
        //
        WS_ASSERT (NT_SUCCESS (status));
    }

    
    //
    // Add an artificial reference to MSAFD.DLL so that it doesn't
    // go away unexpectedly. We'll remove this reference when we shut
    // down the async thread.
    //

    instance = NULL;

    if( GetModuleFileName(
            SockModuleHandle,
            moduleFileName,
            sizeof(moduleFileName) / sizeof(moduleFileName[0])
            ) ) {

        instance = LoadLibrary( moduleFileName );

    }

    if( instance == NULL ) {

        SockReleaseRwLockExclusive (&SocketGlobalLock);
        WS_PRINT((
            "SockCheckAndInitAsyncThread: LoadLibrary failed: %ld\n",
            GetLastError()
            ));


        return FALSE;

    }


    IF_DEBUG(INIT) {
        WS_PRINT(( "SockThreadInitialize: TEB = %lx\n",
                       NtCurrentTeb( ) ));
    }

    //
    // Create the async thread's event.
    //

    status = NtCreateEvent(
                 &threadEvent,
                 EVENT_ALL_ACCESS,
                 NULL,
                 NotificationEvent,
                 FALSE
                 );
    if ( !NT_SUCCESS(status) ) {
        WS_PRINT(( "SockCheckAndInitAsyncThread: Unable to create async thread event: %lx\n",
                       status ));
        SockReleaseRwLockExclusive (&SocketGlobalLock);
        FreeLibrary( instance );
        return FALSE;
    }

    //
    // Allocate space for per-thread data the DLL will have.
    //

    tlsData = ALLOCATE_THREAD_DATA( sizeof(*tlsData) );
    if ( tlsData == NULL ) {
        WS_PRINT(( "SockCheckAndInitAsyncThread: unable to allocate async thread data.\n" ));
        SockReleaseRwLockExclusive (&SocketGlobalLock);
        status = NtClose (threadEvent);
        WS_ASSERT (NT_SUCCESS (status));
        FreeLibrary( instance );
        return FALSE;
    }

    //
    // Initialize the thread data.
    //

    RtlZeroMemory( tlsData, sizeof(*tlsData) );

#if DBG
    tlsData->IndentLevel = 0;
#endif
    tlsData->SocketHandle = (SOCKET)instance;
    tlsData->EventHandle = threadEvent;

#ifdef _AFD_SAN_SWITCH_
	tlsData->UpcallLevel = 0;
	tlsData->UpcallOvListHead = NULL;
	tlsData->UpcallOvListTail = NULL;
#endif //_AFD_SAN_SWITCH_

    //
    // Create the async thread itself.
    //

    threadHandle = CreateThread(
                       NULL,
                       0,
                       SockAsyncThread,
                       (PVOID)tlsData,
                       0,
                       &threadId
                       );

    if ( threadHandle == NULL ) {
        WS_PRINT(( "SockCheckAndInitAsyncThread: CreateThread failed: %ld\n",
                       GetLastError( ) ));
        SockReleaseRwLockExclusive (&SocketGlobalLock);
        FREE_THREAD_DATA (tlsData);
        status = NtClose (threadEvent);
        WS_ASSERT (NT_SUCCESS (status));
        FreeLibrary( instance );
        return FALSE;
    }

    //
    // We no longer need the thread handle.
    //

    NtClose( threadHandle );

    //
    // Thread creation succeeded, add reference to make sure
    // that the thread is running while it is referenced.
    //

    InterlockedExchangeAdd (&SockAsyncThreadReferenceCount, 2);

#if DBG
    SockpAsyncThreadId = threadId;
#endif
    //
    // The async thread was successfully started.
    //

    IF_DEBUG(ASYNC) {
        WS_PRINT(( "SockCheckAndInitAsyncThread: async thread (%ld) successfully "
                   "created.\n", threadId ));
    }


    SockReleaseRwLockExclusive (&SocketGlobalLock);

    return TRUE;

} // SockCheckAndReferenceAsyncThread


DWORD
SockAsyncThread (
    IN PVOID Param
    )
{
    PVOID                   context;
    PASYNC_COMPLETION_PROC  proc;
    IO_STATUS_BLOCK         ioStatus;
    NTSTATUS                status;
    PWINSOCK_TLS_DATA       tlsData = (PWINSOCK_TLS_DATA)Param;
    HINSTANCE               hModule = (HINSTANCE)tlsData->SocketHandle;
    BOOL                    res;
    static const LONGLONG   IdleWaitTimeout 
                        = -(((LONGLONG)MAX_ASYNC_THREAD_IDLE_TIME)*10000000i64);
                            

    IF_DEBUG(ASYNC) {
        WS_PRINT(( "SockAsyncThread: Entered.\n" ));
    }

    tlsData->SocketHandle = INVALID_SOCKET;

    //
    // Setup per-thread data. We must do this "manually" because this
    // thread doesn't go through the normal SockEnterApi() routine.
    //

    if( !SET_THREAD_DATA(tlsData) ) {

        WS_ASSERT(!"TlsSetValue failed");
#if !defined(USE_TEB_FIELD)
        SockTlsSlot = 0xFFFFFFFF;
#endif  // !USE_TEB_FIELD
        InterlockedDecrement (&SockAsyncThreadReferenceCount);
        return 1;
    }

    res = SetThreadPriority (GetCurrentThread (), THREAD_PRIORITY_ABOVE_NORMAL);
    WS_ASSERT (res==TRUE);

    //
    // Loop forever dispatching actions.
    //

    do {

        status = NtRemoveIoCompletion (
                        SockAsyncQueuePort,
                        (PVOID *)&proc,
                        &context,
                        &ioStatus,
                        (PLARGE_INTEGER)&IdleWaitTimeout);
        WS_ASSERT (NT_SUCCESS (status));
        if (status==STATUS_SUCCESS) {
            if (proc!=ASYNC_TERMINATION_PROC) {
				SockHandleAsyncIndication(proc, context, &ioStatus);
            }
            else {
                //
                // We were asked to terminate, treat this like a timeout
                // to avoid additional wait
                //
                status = STATUS_TIMEOUT;
                SockDereferenceAsyncThread ();

            }
        }
        else {
            if (SockAsyncThreadReferenceCount>1 && 
                    NT_ERROR (status)) {
                DbgPrint ("msafd(%lx): Sleeping 1 sec because of NtRemoveIoCompletion failure, status: %lx\n", GetCurrentProcessId (), status);
                ASSERT (FALSE);
                Sleep (1000);
            }
        }
    }
    //
    // Continue while we have not hit a timeout or have more pending
    // items.
    //
    while (((status!=STATUS_TIMEOUT) && (SockWspStartupCount>0))
            || InterlockedCompareExchange (
                    &SockAsyncThreadReferenceCount,
                    0,
                    1)!=1);

    IF_DEBUG(ASYNC) {
        WS_PRINT(( "SockAsyncThread: Exiting because of %s.\n",
            ((proc==ASYNC_TERMINATION_PROC)||(SockWspStartupCount==0)) ? "cleanup" : "idle timeout"));
    }
#if DBG
    SockAcquireRwLockShared (&SocketGlobalLock);
    if (SockWspStartupCount>0) {
        IO_COMPLETION_BASIC_INFORMATION iocInfo;

        WS_ASSERT (SockAsyncQueuePort!=NULL);
        
        status = NtQueryIoCompletion (
                        SockAsyncQueuePort,
                        IoCompletionBasicInformation,
                        &iocInfo,
                        sizeof (iocInfo),
                        NULL
                        );
        WS_ASSERT (NT_SUCCESS (status));
        WS_ASSERT (iocInfo.Depth==0 || SockAsyncThreadReferenceCount>0);
        if (iocInfo.Depth>0) {
            WS_PRINT (( "SockAsynThread: Exiting while there are outstanding items"
                        " in completion queue!!!\n"));
        }
    }
    SockReleaseRwLockShared (&SocketGlobalLock);

    InterlockedCompareExchange (&SockpAsyncThreadId, 0, GetCurrentThreadId());
#endif
    //
    // Remove the artifical reference we added in
    // SockCheckAndInitAsyncThread() and exit this thread.
    //

    FreeLibraryAndExitThread(
        hModule,
        0
        );

    //
    // We should never get here, but just in case...
    //

    return 0;

} // SockAsyncThread



//
// Called when non-timeout completion happens on SockAsyncQueuePort
//
VOID
SockHandleAsyncIndication(
    PASYNC_COMPLETION_PROC  Proc,
	PVOID					Context,
	PIO_STATUS_BLOCK        IoStatus
	)
{
#ifdef _AFD_SAN_SWITCH_
	if ((Proc==SockAsyncConnectCompletion)
				|| (Proc==SockAsyncSelectCompletion)
				|| (Proc==SockProcessQueuedAsyncSelect)
				|| (Proc==SockSanAsyncFreeProvider)
				|| (Proc==SockSanDupSockCtrlMsg)
				|| (Proc==SockSanProviderChange)) {

		(*Proc) (Context, IoStatus);
	}
	else {
		if (Proc == SockSanCompletion) {
			if (Context == &SockSanAddressNotifyContext) {
				//
				// PnP notification
				//
				SockSanProviderAddressUpdate();
			}
			else {
				//
				// accept() completion from AFD
				//
				SockSanCompletion(Context, IoStatus);
			}
		}
		else {
			//
			// ReadFile/WriteFile indirection
			//
			SockSanRedirectRequest ((PVOID)Proc, Context, IoStatus);
		}
	}
#else
	WS_ASSERT ((Proc==SockAsyncConnectCompletion)
				|| (Proc==SockAsyncSelectCompletion)
				|| (Proc==SockProcessQueuedAsyncSelect));
	(*Proc) (Context, IoStatus);
#endif
}



VOID
SockTerminateAsyncThread (
    VOID
    )
{
    HANDLE      Handle;
    NTSTATUS    status;
    OBJECT_HANDLE_FLAG_INFORMATION HandleInfo;
    //
    // Protect access to globals with the lock
    //

	SockAcquireRwLockExclusive (&SocketGlobalLock);

    //
    // Close helper endpoints first so that completion indication
    // for all outstanding selects/connects complete first.
    //
    if (SockAsyncSelectHelperHandle!=NULL) {
        Handle = SockAsyncSelectHelperHandle;
        SockAsyncSelectHelperHandle = NULL;
        //
        // UnProtect our handle from being closed by random NtClose call
        // by some messed up dll or application
        //
        HandleInfo.ProtectFromClose = FALSE;
        HandleInfo.Inherit = FALSE;

        status = NtSetInformationObject( Handle,
                        ObjectHandleFlagInformation,
                        &HandleInfo,
                        sizeof( HandleInfo )
                      );
        //
        // This shouldn't fail
        //
        WS_ASSERT (NT_SUCCESS (status));

        __try {
            status = NtClose (Handle);
        }
        __except (SOCK_EXCEPTION_FILTER ()) {
            status = GetExceptionCode ();
        }
        WS_ASSERT (NT_SUCCESS (status));
    }

    if (SockAsyncConnectHelperHandle!=NULL) {
        Handle = SockAsyncConnectHelperHandle;
        SockAsyncConnectHelperHandle = NULL;
        //
        // UnProtect our handle from being closed by random NtClose call
        // by some messed up dll or application
        //
        HandleInfo.ProtectFromClose = FALSE;
        HandleInfo.Inherit = FALSE;

        status = NtSetInformationObject( Handle,
                        ObjectHandleFlagInformation,
                        &HandleInfo,
                        sizeof( HandleInfo )
                      );
        //
        // This shouldn't fail
        //
        WS_ASSERT (NT_SUCCESS (status));

        __try {
            status = NtClose (Handle);
        }
        __except (SOCK_EXCEPTION_FILTER ()) {
            status = GetExceptionCode ();
        }
        WS_ASSERT (NT_SUCCESS (status));
    }


    if (SockAsyncQueuePort!=NULL) {
        LONG    count;
        do {
            //
            // If thread has already been initialized,
            // then increment reference count and post work
            // item to terminate the thread.
            //
            count = (LONG volatile)SockAsyncThreadReferenceCount;
            WS_ASSERT (count>=0);
            if ((count>0) && 
                    (InterlockedCompareExchange (
                        &SockAsyncThreadReferenceCount,
                        (count + 1),
                        count)==count)) {
                status = NtSetIoCompletion (SockAsyncQueuePort,
                                    ASYNC_TERMINATION_PROC,
                                    (PVOID)-1,
                                    0,
                                    0);
                WS_ASSERT (NT_SUCCESS (status));
                break;
            }
        }
        while (count>0);
    }

    SockReleaseRwLockExclusive (&SocketGlobalLock);

} // SockTerminateAsyncThread
