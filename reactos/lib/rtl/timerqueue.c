/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS system libraries
 * PURPOSE:           Timer Queue implementation
 * FILE:              lib/rtl/timerqueue.c
 * PROGRAMMER:        
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ***************************************************************/

typedef VOID (CALLBACK *WAITORTIMERCALLBACKFUNC) (PVOID, BOOLEAN );

HANDLE TimerThreadHandle = NULL;

NTSTATUS
RtlpInitializeTimerThread(VOID)
{
    return STATUS_NOT_IMPLEMENTED;
}

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlCreateTimer(HANDLE TimerQueue,
               PHANDLE phNewTimer,
               WAITORTIMERCALLBACKFUNC Callback,
               PVOID Parameter,
               ULONG DueTime,
               ULONG Period,
               ULONG Flags)
{
  DPRINT1("RtlCreateTimer: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlCreateTimerQueue(PHANDLE TimerQueue)
{
  DPRINT1("RtlCreateTimerQueue: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlDeleteTimer(HANDLE TimerQueue,
               HANDLE Timer,
	       HANDLE CompletionEvent)
{
  DPRINT1("RtlDeleteTimer: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlDeleteTimerQueue(HANDLE TimerQueue)
{
  DPRINT1("RtlDeleteTimerQueue: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlDeleteTimerQueueEx(HANDLE TimerQueue,
                      HANDLE CompletionEvent)
{
  DPRINT1("RtlDeleteTimerQueueEx: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}


/*
 * @unimplemented
 */
NTSTATUS
NTAPI
RtlUpdateTimer(HANDLE TimerQueue,
               HANDLE Timer,
	       ULONG DueTime,
	       ULONG Period)
{
  DPRINT1("RtlUpdateTimer: stub\n");
  return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
