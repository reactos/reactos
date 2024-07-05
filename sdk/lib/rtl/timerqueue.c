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

#undef LIST_FOR_EACH
#undef LIST_FOR_EACH_SAFE
#include <wine/list.h>

/* FUNCTIONS ***************************************************************/

extern PRTL_START_POOL_THREAD RtlpStartThreadFunc;
extern PRTL_EXIT_POOL_THREAD RtlpExitThreadFunc;
HANDLE TimerThreadHandle = NULL;

NTSTATUS
RtlpInitializeTimerThread(VOID)
{
    return STATUS_NOT_IMPLEMENTED;
}

static inline PLARGE_INTEGER get_nt_timeout( PLARGE_INTEGER pTime, ULONG timeout )
{
    if (timeout == INFINITE) return NULL;
    pTime->QuadPart = (ULONGLONG)timeout * -10000;
    return pTime;
}

struct timer_queue;
struct queue_timer
{
    struct timer_queue *q;
    struct list entry;
    ULONG runcount;             /* number of callbacks pending execution */
    WAITORTIMERCALLBACKFUNC callback;
    PVOID param;
    DWORD period;
    ULONG flags;
    ULONGLONG expire;
    BOOL destroy;               /* timer should be deleted; once set, never unset */
    HANDLE event;               /* removal event */
};

struct timer_queue
{
    DWORD magic;
    RTL_CRITICAL_SECTION cs;
    struct list timers;         /* sorted by expiration time */
    BOOL quit;                  /* queue should be deleted; once set, never unset */
    HANDLE event;
    HANDLE thread;
};

#define EXPIRE_NEVER (~(ULONGLONG) 0)
#define TIMER_QUEUE_MAGIC  0x516d6954   /* TimQ */

NTSTATUS
WINAPI
RtlSetTimer(
    HANDLE TimerQueue,
    PHANDLE NewTimer,
    WAITORTIMERCALLBACKFUNC Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    ULONG Flags)
{
    return RtlCreateTimer(TimerQueue,
                          NewTimer,
                          Callback,
                          Parameter,
                          DueTime,
                          Period,
                          Flags);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlCancelTimer(HANDLE TimerQueue, HANDLE Timer)
{
    return RtlDeleteTimer(TimerQueue, Timer, NULL);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
RtlDeleteTimerQueue(HANDLE TimerQueue)
{
    return RtlDeleteTimerQueueEx(TimerQueue, INVALID_HANDLE_VALUE);
}

/* EOF */
