/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * PURPOSE:         Timer Queue functions
 * FILE:            dll/win32/kernel32/misc/timerqueue.c
 * PROGRAMER:       Thomas Weidenmueller <w3seek@reactos.com>
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


/* FUNCTIONS *****************************************************************/

HANDLE DefaultTimerQueue = NULL;

/*
 * Create the default timer queue for the current process. This function is only
 * called if CreateTimerQueueTimer() or SetTimerQueueTimer() is called.
 * However, ChangeTimerQueueTimer() fails with ERROR_INVALID_PARAMETER if the
 * default timer queue has not been created, because it assumes there has to be
 * a timer queue with a timer if it want's to be changed.
 */
static BOOL
IntCreateDefaultTimerQueue(VOID)
{
  NTSTATUS Status;

  /* FIXME - make this thread safe */

  /* create the timer queue */
  Status = RtlCreateTimerQueue(&DefaultTimerQueue);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    DPRINT1("Unable to create the default timer queue!\n");
    return FALSE;
  }

  return TRUE;
}

/*
 * @implemented
 */
BOOL
STDCALL
CancelTimerQueueTimer(HANDLE TimerQueue,
                      HANDLE Timer)
{
  /* Since this function is not documented in PSDK and apparently does nothing
     but delete the timer, we just do the same as DeleteTimerQueueTimer(), without
     passing a completion event. */
  NTSTATUS Status;

  if(TimerQueue == NULL)
  {
    /* let's use the process' default timer queue. We assume the default timer
       queue has been created with a previous call to CreateTimerQueueTimer() or
       SetTimerQueueTimer(), otherwise this call wouldn't make much sense. */
    if(!(TimerQueue = DefaultTimerQueue))
    {
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  }

  if(Timer == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  /* delete the timer */
  Status = RtlDeleteTimer(TimerQueue, Timer, NULL);

  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}

/*
 * @implemented
 */
BOOL
STDCALL
ChangeTimerQueueTimer(HANDLE TimerQueue,
                      HANDLE Timer,
                      ULONG DueTime,
                      ULONG Period)
{
  NTSTATUS Status;

  if(TimerQueue == NULL)
  {
    /* let's use the process' default timer queue. We assume the default timer
       queue has been created with a previous call to CreateTimerQueueTimer() or
       SetTimerQueueTimer(), otherwise this call wouldn't make much sense. */
    if(!(TimerQueue = DefaultTimerQueue))
    {
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  }

  if(Timer == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  /* update the timer */
  Status = RtlUpdateTimer(TimerQueue, Timer, DueTime, Period);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}

/*
 * @implemented
 */
HANDLE
STDCALL
CreateTimerQueue(VOID)
{
  HANDLE Handle;
  NTSTATUS Status;

  /* create the timer queue */
  Status = RtlCreateTimerQueue(&Handle);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return NULL;
  }

  return Handle;
}

/*
 * @implemented
 */
BOOL
STDCALL
CreateTimerQueueTimer(PHANDLE phNewTimer,
                      HANDLE TimerQueue,
                      WAITORTIMERCALLBACK Callback,
                      PVOID Parameter,
                      DWORD DueTime,
                      DWORD Period,
                      ULONG Flags)
{
  NTSTATUS Status;

  /* windows seems not to test this parameter at all, so we'll try to clear it here
     so we don't crash somewhere inside ntdll */
  *phNewTimer = NULL;

  if(TimerQueue == NULL)
  {
    /* the default timer queue is requested, try to create it if it hasn't been already */
    if(!(TimerQueue = DefaultTimerQueue))
    {
      if(!IntCreateDefaultTimerQueue())
      {
        /* IntCreateDefaultTimerQueue() set the last error code already, just fail */
        return FALSE;
      }
      TimerQueue = DefaultTimerQueue;
    }
  }

  /* !!! Win doesn't even check if Callback == NULL, so we don't, too! That'll
         raise a nice exception later... */

  /* create the timer */
  Status = RtlCreateTimer(TimerQueue, phNewTimer, Callback, Parameter, DueTime,
                          Period, Flags);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}

/*
 * @implemented
 */
BOOL
STDCALL
DeleteTimerQueue(HANDLE TimerQueue)
{
  NTSTATUS Status;

  /* We don't allow the user to delete the default timer queue */
  if(TimerQueue == NULL)
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  /* delete the timer queue */
  Status = RtlDeleteTimerQueue(TimerQueue);
  return NT_SUCCESS(Status);
}

/*
 * @implemented
 */
BOOL
STDCALL
DeleteTimerQueueEx(HANDLE TimerQueue,
                   HANDLE CompletionEvent)
{
  NTSTATUS Status;

  /* We don't allow the user to delete the default timer queue */
  if(TimerQueue == NULL)
  {
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
  }

  /* delete the queue */
  Status = RtlDeleteTimerQueueEx(TimerQueue, CompletionEvent);

  if((CompletionEvent != INVALID_HANDLE_VALUE && Status == STATUS_PENDING) ||
     !NT_SUCCESS(Status))
  {
    /* In case CompletionEvent == NULL, RtlDeleteTimerQueueEx() returns before
       all callback routines returned. We set the last error code to STATUS_PENDING
       and return FALSE. In case CompletionEvent == INVALID_HANDLE_VALUE we only
       can get here if another error occured. In case CompletionEvent is something
       else, we get here and fail, even though it isn't really an error (if Status == STATUS_PENDING).
       We also handle all other failures the same way. */

    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}

/*
 * @implemented
 */
BOOL
STDCALL
DeleteTimerQueueTimer(HANDLE TimerQueue,
                      HANDLE Timer,
                      HANDLE CompletionEvent)
{
  NTSTATUS Status;

  if(TimerQueue == NULL)
  {
    /* let's use the process' default timer queue. We assume the default timer
       queue has been created with a previous call to CreateTimerQueueTimer() or
       SetTimerQueueTimer(), otherwise this call wouldn't make much sense. */
    if(!(TimerQueue = DefaultTimerQueue))
    {
      SetLastError(ERROR_INVALID_HANDLE);
      return FALSE;
    }
  }

  if(Timer == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  /* delete the timer */
  Status = RtlDeleteTimer(TimerQueue, Timer, CompletionEvent);

  if((CompletionEvent != INVALID_HANDLE_VALUE && Status == STATUS_PENDING) ||
     !NT_SUCCESS(Status))
  {
    /* In case CompletionEvent == NULL, RtlDeleteTimer() returns before
       the callback routine returned. We set the last error code to STATUS_PENDING
       and return FALSE. In case CompletionEvent == INVALID_HANDLE_VALUE we only
       can get here if another error occured. In case CompletionEvent is something
       else, we get here and fail, even though it isn't really an error (if Status == STATUS_PENDING).
       We also handle all other failures the same way. */

    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}

/*
 * @implemented
 */
HANDLE
STDCALL
SetTimerQueueTimer(HANDLE TimerQueue,
                   WAITORTIMERCALLBACK Callback,
                   PVOID Parameter,
                   DWORD DueTime,
                   DWORD Period,
                   BOOL PreferIo)
{
  /* Since this function is not documented in PSDK and apparently does nothing
     but create a timer, we just do the same as CreateTimerQueueTimer(). Unfortunately
     I don't really know what PreferIo is supposed to be, it propably just affects the
     Flags parameter of CreateTimerQueueTimer(). Looking at the PSDK documentation of
     CreateTimerQueueTimer() there's only one flag (WT_EXECUTEINIOTHREAD) that causes
     the callback function queued to an I/O worker thread. I guess it uses this flag
     if PreferIo == TRUE, otherwise let's just use WT_EXECUTEDEFAULT. We should
     test this though, this is only guess work and I'm too lazy to do further
     investigation. */

  HANDLE Timer;
  NTSTATUS Status;

  if(TimerQueue == NULL)
  {
    /* the default timer queue is requested, try to create it if it hasn't been already */
    if(!(TimerQueue = DefaultTimerQueue))
    {
      if(!IntCreateDefaultTimerQueue())
      {
        /* IntCreateDefaultTimerQueue() set the last error code already, just fail */
        return FALSE;
      }
      TimerQueue = DefaultTimerQueue;
    }
  }

  /* create the timer */
  Status = RtlCreateTimer(TimerQueue, &Timer, Callback, Parameter, DueTime,
                          Period, (PreferIo ? WT_EXECUTEINIOTHREAD : WT_EXECUTEDEFAULT));
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return NULL;
  }

  return Timer;
}

/* EOF */
