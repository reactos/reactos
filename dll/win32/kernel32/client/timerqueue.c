/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/timerqueue.c
 * PURPOSE:         Timer Queue Functions
 * PROGRAMMERS:     Thomas Weidenmueller <w3seek@reactos.com>
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

HANDLE BasepDefaultTimerQueue = NULL;

/* PRIVATE FUNCTIONS **********************************************************/

/* FIXME - make this thread safe */
BOOL
WINAPI
BasepCreateDefaultTimerQueue(VOID)
{
    NTSTATUS Status;

    /* Create the timer queue */
    Status = RtlCreateTimerQueue(&BasepDefaultTimerQueue);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        DPRINT1("Unable to create the default timer queue!\n");
        return FALSE;
    }

    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/


/*
 * @implemented
 */
BOOL
WINAPI
CancelTimerQueueTimer(IN HANDLE TimerQueue,
                      IN HANDLE Timer)
{
    NTSTATUS Status;

    if (!TimerQueue)
    {
        /* Use the default timer queue */
        TimerQueue = BasepDefaultTimerQueue;
        if (!TimerQueue)
        {
            /* A timer is being cancelled before one was created... fail */
            SetLastError(ERROR_INVALID_HANDLE);
            return FALSE;
        }
    }

    /* Delete the timer */
    Status = RtlDeleteTimer(TimerQueue, Timer, NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
ChangeTimerQueueTimer(IN HANDLE TimerQueue,
                      IN HANDLE Timer,
                      IN ULONG DueTime,
                      IN ULONG Period)
{
    NTSTATUS Status;

    if (!TimerQueue)
    {
        /* Use the default timer queue */
        TimerQueue = BasepDefaultTimerQueue;
        if (!TimerQueue)
        {
            /* A timer is being cancelled before one was created... fail */
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    /* Delete the timer */
    Status = RtlUpdateTimer(TimerQueue, Timer, DueTime, Period);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
HANDLE
WINAPI
CreateTimerQueue(VOID)
{
    HANDLE Handle;
    NTSTATUS Status;

    /* Create the timer queue */
    Status = RtlCreateTimerQueue(&Handle);
    if(!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return NULL;
    }

    return Handle;
}

/*
 * @implemented
 */
BOOL
WINAPI
CreateTimerQueueTimer(OUT PHANDLE phNewTimer,
                      IN HANDLE TimerQueue,
                      IN WAITORTIMERCALLBACK Callback,
                      IN PVOID Parameter,
                      IN DWORD DueTime,
                      IN DWORD Period,
                      IN ULONG Flags)
{
    NTSTATUS Status;

    /* Parameter is not optional -- clear it now */
    *phNewTimer = NULL;

    /* Check if no queue was given */
    if (!TimerQueue)
    {
        /* Create the queue if it didn't already exist */
        if (!(BasepDefaultTimerQueue) && !(BasepCreateDefaultTimerQueue()))
        {
            return FALSE;
        }

        /* Use the default queue */
        TimerQueue = BasepDefaultTimerQueue;
    }

    /* Create the timer. Note that no parameter checking is done here */
    Status = RtlCreateTimer(TimerQueue,
                            phNewTimer,
                            Callback,
                            Parameter,
                            DueTime,
                            Period,
                            Flags);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DeleteTimerQueue(IN HANDLE TimerQueue)
{
    /* We don't allow the user to delete the default timer queue */
    if (!TimerQueue)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Delete the timer queue */
    RtlDeleteTimerQueueEx(TimerQueue, 0);
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DeleteTimerQueueEx(IN HANDLE TimerQueue,
                   IN HANDLE CompletionEvent)
{
    NTSTATUS Status;

    /* We don't allow the user to delete the default timer queue */
    if (!TimerQueue)
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    /* Delete the timer queue */
    Status = RtlDeleteTimerQueueEx(TimerQueue, CompletionEvent);
    if (((CompletionEvent != INVALID_HANDLE_VALUE) && (Status == STATUS_PENDING)) ||
        !(NT_SUCCESS(Status)))
    {
        /* In case CompletionEvent == NULL, RtlDeleteTimerQueueEx() returns before
           all callback routines returned. We set the last error code to STATUS_PENDING
           and return FALSE. In case CompletionEvent == INVALID_HANDLE_VALUE we only
           can get here if another error occured. In case CompletionEvent is something
           else, we get here and fail, even though it isn't really an error (if Status == STATUS_PENDING).
           We also handle all other failures the same way. */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DeleteTimerQueueTimer(IN HANDLE TimerQueue,
                      IN HANDLE Timer,
                      IN HANDLE CompletionEvent)
{
    NTSTATUS Status;

    if (!TimerQueue)
    {
        /* Use the default timer queue */
        TimerQueue = BasepDefaultTimerQueue;
        if (!TimerQueue)
        {
            /* A timer is being cancelled before one was created... fail */
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    /* Delete the timer */
    Status = RtlDeleteTimer(TimerQueue, Timer, CompletionEvent);
    if (((CompletionEvent != INVALID_HANDLE_VALUE) && (Status == STATUS_PENDING)) ||
        !(NT_SUCCESS(Status)))
    {
        /* In case CompletionEvent == NULL, RtlDeleteTimerQueueEx() returns before
           all callback routines returned. We set the last error code to STATUS_PENDING
           and return FALSE. In case CompletionEvent == INVALID_HANDLE_VALUE we only
           can get here if another error occured. In case CompletionEvent is something
           else, we get here and fail, even though it isn't really an error (if Status == STATUS_PENDING).
           We also handle all other failures the same way. */
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
HANDLE
WINAPI
SetTimerQueueTimer(IN HANDLE TimerQueue,
                   IN WAITORTIMERCALLBACK Callback,
                   IN PVOID Parameter,
                   IN DWORD DueTime,
                   IN DWORD Period,
                   IN BOOL PreferIo)
{
    HANDLE Timer;
    NTSTATUS Status;

    /* Check if no queue was given */
    if (!TimerQueue)
    {
        /* Create the queue if it didn't already exist */
        if (!(BasepDefaultTimerQueue) && !(BasepCreateDefaultTimerQueue()))
        {
            return FALSE;
        }

        /* Use the default queue */
        TimerQueue = BasepDefaultTimerQueue;
    }

    /* Create the timer */
    Status = RtlCreateTimer(TimerQueue,
                            &Timer,
                            Callback,
                            Parameter,
                            DueTime,
                            Period,
                            PreferIo ? WT_EXECUTEINIOTHREAD : WT_EXECUTEDEFAULT);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return NULL;
    }

    return Timer;
}

/* EOF */
