/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: suspend.c,v 1.8 2002/08/09 17:23:57 dwelch Exp $
 *
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/suspend.c
 * PURPOSE:                Thread managment
 * PROGRAMMER:             David Welch (welch@mcmail.com)
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* NOTES **********************************************************************
 *
 */

/* GLOBALS *******************************************************************/

static FAST_MUTEX SuspendMutex;

/* FUNCTIONS *****************************************************************/

VOID STDCALL
PiSuspendThreadRundownRoutine(PKAPC Apc)
{
}


VOID STDCALL
PiSuspendThreadKernelRoutine(PKAPC Apc,
			     PKNORMAL_ROUTINE* NormalRoutine,
			     PVOID* NormalContext,
			     PVOID* SystemArgument1,
			     PVOID* SystemArguemnt2)
{
}


VOID STDCALL
PiSuspendThreadNormalRoutine(PVOID NormalContext,
			     PVOID SystemArgument1,
			     PVOID SystemArgument2)
{
  PETHREAD CurrentThread = PsGetCurrentThread();
  while (CurrentThread->Tcb.SuspendCount > 0)
    {
      KeWaitForSingleObject(&CurrentThread->Tcb.SuspendSemaphore,
			    0,
			    UserMode,
			    TRUE,
			    NULL);
    }
}


NTSTATUS
PsResumeThread(PETHREAD Thread, PULONG SuspendCount)
{
  ExAcquireFastMutex(&SuspendMutex);
  if (SuspendCount != NULL)
    {
      *SuspendCount = Thread->Tcb.SuspendCount;
    }
  if (Thread->Tcb.SuspendCount > 0)
    {
      Thread->Tcb.SuspendCount--;
      if (Thread->Tcb.SuspendCount == 0)
	{
	  KeReleaseSemaphore(&Thread->Tcb.SuspendSemaphore, IO_NO_INCREMENT, 
			     1, FALSE);
	}      
    }
  ExReleaseFastMutex(&SuspendMutex);
  return(STATUS_SUCCESS);
}


NTSTATUS
PsSuspendThread(PETHREAD Thread, PULONG PreviousSuspendCount)
{
  ULONG OldValue;

  ExAcquireFastMutex(&SuspendMutex);
  OldValue = Thread->Tcb.SuspendCount;
  Thread->Tcb.SuspendCount++;
  if (!Thread->Tcb.SuspendApc.Inserted)
    {
      KeInsertQueueApc(&Thread->Tcb.SuspendApc,
		       NULL,
		       NULL,
		       0);
    }
  ExReleaseFastMutex(&SuspendMutex);
  if (PreviousSuspendCount != NULL)
    {
      *PreviousSuspendCount = OldValue;
    }
  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtResumeThread(IN HANDLE ThreadHandle,
	       IN PULONG SuspendCount)
/*
 * FUNCTION: Decrements a thread's resume count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        ResumeCount =  The resulting resume count.
 * RETURNS: Status
 */
{
  PETHREAD Thread;
  NTSTATUS Status;
  ULONG Count;

  DPRINT("NtResumeThead(ThreadHandle %lx  SuspendCount %p)\n",
	 ThreadHandle, SuspendCount);

  Status = ObReferenceObjectByHandle(ThreadHandle,
				     THREAD_SUSPEND_RESUME,
				     PsThreadType,
				     UserMode,
				     (PVOID*)&Thread,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Status = PsResumeThread(Thread, &Count);
  if (SuspendCount != NULL)
    {
      *SuspendCount = Count;
    }

  ObDereferenceObject((PVOID)Thread);

  return(STATUS_SUCCESS);
}


NTSTATUS STDCALL
NtSuspendThread(IN HANDLE ThreadHandle,
		IN PULONG PreviousSuspendCount)
/*
 * FUNCTION: Increments a thread's suspend count
 * ARGUMENTS: 
 *        ThreadHandle = Handle to the thread that should be resumed
 *        PreviousSuspendCount =  The resulting/previous suspend count.
 * REMARK:
 *        A thread will be suspended if its suspend count is greater than 0. 
 *        This procedure maps to the win32 SuspendThread function. ( 
 *        documentation about the the suspend count can be found here aswell )
 *        The suspend count is not increased if it is greater than 
 *        MAXIMUM_SUSPEND_COUNT.
 * RETURNS: Status
 */
{
  PETHREAD Thread;
  NTSTATUS Status;
  ULONG Count;

  Status = ObReferenceObjectByHandle(ThreadHandle,
				     THREAD_SUSPEND_RESUME,
				     PsThreadType,
				     UserMode,
				     (PVOID*)&Thread,
				     NULL);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Status = PsSuspendThread(Thread, &Count);
  if (PreviousSuspendCount != NULL)
    {
      *PreviousSuspendCount = Count;
    }

  ObDereferenceObject((PVOID)Thread);

  return(STATUS_SUCCESS);
}

VOID
PsInitialiseSuspendImplementation(VOID)
{
  ExInitializeFastMutex(&SuspendMutex);
}

/* EOF */
