/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Rtl user wait functions
 * FILE:              lib/rtl/wait.c
 * PROGRAMERS:
 *                    Alex Ionescu (alex@relsoft.net)
 *                    Eric Kohl
 *                    KJK::Hyperion
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

typedef struct _RTLP_WAIT
{
    HANDLE Object;
    BOOLEAN CallbackInProgress;
    HANDLE CancelEvent;
    LONG DeleteCount;
    HANDLE CompletionEvent;
    ULONG Flags;
    WAITORTIMERCALLBACKFUNC Callback;
    PVOID Context;
    ULONG Milliseconds;
} RTLP_WAIT, *PRTLP_WAIT;

/* PRIVATE FUNCTIONS *******************************************************/

static inline PLARGE_INTEGER get_nt_timeout( PLARGE_INTEGER pTime, ULONG timeout )
{
    if (timeout == INFINITE) return NULL;
    pTime->QuadPart = (ULONGLONG)timeout * -10000;
    return pTime;
}

static VOID
NTAPI
Wait_thread_proc(LPVOID Arg)
{
    PRTLP_WAIT Wait = (PRTLP_WAIT) Arg;
    NTSTATUS Status;
    BOOLEAN alertable = (Wait->Flags & WT_EXECUTEINIOTHREAD) ? TRUE : FALSE;
    HANDLE handles[2] = { Wait->Object, Wait->CancelEvent };
    LARGE_INTEGER timeout;
    HANDLE completion_event;

//    TRACE("\n");

    while (TRUE)
    {
        Status = NtWaitForMultipleObjects( 2,
                                           handles,
                                           WaitAny,
                                           alertable,
                                           get_nt_timeout( &timeout, Wait->Milliseconds ) );

        if (Status == STATUS_WAIT_0 || Status == STATUS_TIMEOUT)
        {
            BOOLEAN TimerOrWaitFired;

            if (Status == STATUS_WAIT_0)
            {
   //             TRACE( "object %p signaled, calling callback %p with context %p\n",
   //                 Wait->Object, Wait->Callback,
   //                 Wait->Context );
                TimerOrWaitFired = FALSE;
            }
            else
            {
    //            TRACE( "wait for object %p timed out, calling callback %p with context %p\n",
    //                Wait->Object, Wait->Callback,
    //                Wait->Context );
                TimerOrWaitFired = TRUE;
            }
            Wait->CallbackInProgress = TRUE;
            Wait->Callback( Wait->Context, TimerOrWaitFired );
            Wait->CallbackInProgress = FALSE;

            if (Wait->Flags & WT_EXECUTEONLYONCE)
                break;
        }
        else
            break;
    }

    completion_event = Wait->CompletionEvent;
    if (completion_event) NtSetEvent( completion_event, NULL );

    if (_InterlockedIncrement( &Wait->DeleteCount ) == 2 )
    {
       NtClose( Wait->CancelEvent );
       RtlFreeHeap( RtlGetProcessHeap(), 0, Wait );
    }
}


/* FUNCTIONS ***************************************************************/


/***********************************************************************
 *              RtlRegisterWait
 *
 * Registers a wait for a handle to become signaled.
 *
 * PARAMS
 *  NewWaitObject [I] Handle to the new wait object. Use RtlDeregisterWait() to free it.
 *  Object   [I] Object to wait to become signaled.
 *  Callback [I] Callback function to execute when the wait times out or the handle is signaled.
 *  Context  [I] Context to pass to the callback function when it is executed.
 *  Milliseconds [I] Number of milliseconds to wait before timing out.
 *  Flags    [I] Flags. See notes.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 *
 * NOTES
 *  Flags can be one or more of the following:
 *|WT_EXECUTEDEFAULT - Executes the work item in a non-I/O worker thread.
 *|WT_EXECUTEINIOTHREAD - Executes the work item in an I/O worker thread.
 *|WT_EXECUTEINPERSISTENTTHREAD - Executes the work item in a thread that is persistent.
 *|WT_EXECUTELONGFUNCTION - Hints that the execution can take a long time.
 *|WT_TRANSFER_IMPERSONATION - Executes the function with the current access token.
 */
NTSTATUS 
NTAPI
RtlRegisterWait(PHANDLE NewWaitObject, 
                HANDLE Object,
                WAITORTIMERCALLBACKFUNC Callback,
                PVOID Context,
                ULONG Milliseconds,
                ULONG Flags)
{
    PRTLP_WAIT Wait;
    NTSTATUS Status;

    //TRACE( "(%p, %p, %p, %p, %d, 0x%x)\n", NewWaitObject, Object, Callback, Context, Milliseconds, Flags );

    Wait = RtlAllocateHeap( RtlGetProcessHeap(), 0, sizeof(RTLP_WAIT) );
    if (!Wait)
        return STATUS_NO_MEMORY;

    Wait->Object = Object;
    Wait->Callback = Callback;
    Wait->Context = Context;
    Wait->Milliseconds = Milliseconds;
    Wait->Flags = Flags;
    Wait->CallbackInProgress = FALSE;
    Wait->DeleteCount = 0;
    Wait->CompletionEvent = NULL;

    Status = NtCreateEvent( &Wait->CancelEvent, 
                             EVENT_ALL_ACCESS, 
                             NULL,
                             TRUE,
                             FALSE );

    if (Status != STATUS_SUCCESS)
    {
        RtlFreeHeap( RtlGetProcessHeap(), 0, Wait );
        return Status;
    }

    Status = RtlQueueWorkItem( Wait_thread_proc,
                               Wait,
                               Flags & ~WT_EXECUTEONLYONCE );

    if (Status != STATUS_SUCCESS)
    {
        NtClose( Wait->CancelEvent );
        RtlFreeHeap( RtlGetProcessHeap(), 0, Wait );
        return Status;
    }

    *NewWaitObject = Wait;
    return Status;
}

/***********************************************************************
 *              RtlDeregisterWaitEx
 *
 * Cancels a wait operation and frees the resources associated with calling
 * RtlRegisterWait().
 *
 * PARAMS
 *  WaitObject [I] Handle to the wait object to free.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS 
NTAPI 
RtlDeregisterWaitEx(HANDLE WaitHandle,
                    HANDLE CompletionEvent)
{
    PRTLP_WAIT Wait = (PRTLP_WAIT) WaitHandle;
    NTSTATUS Status = STATUS_SUCCESS;

    //TRACE( "(%p)\n", WaitHandle );

    NtSetEvent( Wait->CancelEvent, NULL );
    if (Wait->CallbackInProgress)
    {
        if (CompletionEvent != NULL)
        {
            if (CompletionEvent == INVALID_HANDLE_VALUE)
            {
                Status = NtCreateEvent( &CompletionEvent,
                                         EVENT_ALL_ACCESS,
                                         NULL,
                                         TRUE,
                                         FALSE );

                if (Status != STATUS_SUCCESS)
                    return Status;

                (void)_InterlockedExchangePointer( &Wait->CompletionEvent, CompletionEvent );

                if (Wait->CallbackInProgress)
                    NtWaitForSingleObject( CompletionEvent, FALSE, NULL );

                NtClose( CompletionEvent );
            }
            else
            {
                (void)_InterlockedExchangePointer( &Wait->CompletionEvent, CompletionEvent );

                if (Wait->CallbackInProgress)
                    Status = STATUS_PENDING;
            }
        }
        else
            Status = STATUS_PENDING;
    }

    if (_InterlockedIncrement( &Wait->DeleteCount ) == 2 )
    {
        Status = STATUS_SUCCESS;
        NtClose( Wait->CancelEvent );
        RtlFreeHeap( RtlGetProcessHeap(), 0, Wait );
    }

    return Status;
}

/***********************************************************************
 *              RtlDeregisterWait
 *
 * Cancels a wait operation and frees the resources associated with calling
 * RtlRegisterWait().
 *
 * PARAMS
 *  WaitObject [I] Handle to the wait object to free.
 *
 * RETURNS
 *  Success: STATUS_SUCCESS.
 *  Failure: Any NTSTATUS code.
 */
NTSTATUS
NTAPI
RtlDeregisterWait(HANDLE WaitHandle)
{
    return RtlDeregisterWaitEx(WaitHandle, NULL);
}

/* EOF */
