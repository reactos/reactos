/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/tinfo.c
 * PURPOSE:         Getting/setting thread information
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ps.h>

#include <internal/debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS STDCALL NtSetInformationThread(HANDLE		ThreadHandle,
					THREADINFOCLASS	ThreadInformationClass,
					PVOID		ThreadInformation,
					ULONG ThreadInformationLength)
{
   PETHREAD			Thread;
   NTSTATUS			Status;
   PTHREAD_BASIC_INFORMATION	ThreadBasicInformationP;
   
   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_SET_INFORMATION,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }
   
   switch (ThreadInformationClass)
     {
      case ThreadBasicInformation:
	ThreadBasicInformationP = 
	  (PTHREAD_BASIC_INFORMATION) ThreadInformation;
	ThreadBasicInformationP->ExitStatus = 
	  Thread->ExitStatus;
	ThreadBasicInformationP->TebBaseAddress = 
	  Thread->Tcb.Teb;
	ThreadBasicInformationP->AffinityMask = 
	  Thread->Tcb.Affinity;
	ThreadBasicInformationP->BasePriority = 
	  Thread->Tcb.BasePriority;
	ThreadBasicInformationP->UniqueThreadId = 
	  (ULONG) Thread->Cid.UniqueThread;
	Status = STATUS_SUCCESS;
	break;
	
      case ThreadTimes:
	break;
	
      case ThreadPriority:
	KeSetPriorityThread(&Thread->Tcb, *(KPRIORITY *)ThreadInformation);
	Status = STATUS_SUCCESS;
	break;
	
      case ThreadBasePriority:
	break;
	
      case ThreadAffinityMask:
	break;
	
      case ThreadImpersonationToken:
	Status = PsAssignImpersonationToken(Thread, 
					    *((PHANDLE)ThreadInformation));
	break;
	
      case ThreadDescriptorTableEntry:
	UNIMPLEMENTED;
	break;
	
      case ThreadEventPair:
	UNIMPLEMENTED;
	break;
	
      case ThreadQuerySetWin32StartAddress:
	break;
	
      case ThreadZeroTlsCell:
	break;
	
      case ThreadPerformanceCount:
	break;
	
      case ThreadAmILastThread:
	break;	
	
      case ThreadPriorityBoost:
	break;
	
      default:
	Status = STATUS_UNSUCCESSFUL;
     }
   ObDereferenceObject(Thread);
   return Status;
}


NTSTATUS
STDCALL
NtQueryInformationThread (
	IN	HANDLE		ThreadHandle,
	IN	THREADINFOCLASS	ThreadInformationClass,
	OUT	PVOID		ThreadInformation,
	IN	ULONG		ThreadInformationLength,
	OUT	PULONG		ReturnLength)
{
	UNIMPLEMENTED
}

VOID KeSetPreviousMode(ULONG Mode)
{
   PsGetCurrentThread()->Tcb.PreviousMode = Mode;
}

ULONG
STDCALL
KeGetPreviousMode (
	VOID
	)
{
   /* CurrentThread is in ntoskrnl/ps/thread.c */
   return (ULONG)PsGetCurrentThread()->Tcb.PreviousMode;
}


ULONG
STDCALL
ExGetPreviousMode (
	VOID
	)
{
   /* CurrentThread is in ntoskrnl/ps/thread.c */
   return (ULONG)PsGetCurrentThread()->Tcb.PreviousMode;
}

/* EOF */
