/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/tinfo.c
 * PURPOSE:         Getting/setting thread information
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 *                  Updated 09/08/2003 by Skywing (skywing@valhallalegends.com)
 *                   to suppport thread-eventpairs.
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* GLOBALS *****************************************************************/

/*
 * FIXME:
 *   Remove the Implemented value if all functions are implemented.
 */

static const struct
{
   BOOLEAN Implemented;
   ULONG Size;
} QueryInformationData[MaxThreadInfoClass + 1] = 
{
    {TRUE, sizeof(THREAD_BASIC_INFORMATION)},	// ThreadBasicInformation
    {TRUE, sizeof(KERNEL_USER_TIMES)},		// ThreadTimes
    {TRUE, 0},					// ThreadPriority
    {TRUE, 0},					// ThreadBasePriority
    {TRUE, 0},					// ThreadAffinityMask
    {TRUE, 0},					// ThreadImpersonationToken
    {FALSE, 0},					// ThreadDescriptorTableEntry
    {TRUE, 0},					// ThreadEnableAlignmentFaultFixup
    {TRUE, 0},					// ThreadEventPair
    {TRUE, sizeof(PVOID)},			// ThreadQuerySetWin32StartAddress
    {TRUE, 0},					// ThreadZeroTlsCell
    {TRUE, sizeof(LARGE_INTEGER)},		// ThreadPerformanceCount
    {TRUE, sizeof(BOOLEAN)},			// ThreadAmILastThread
    {TRUE, 0},					// ThreadIdealProcessor
    {FALSE, 0},					// ThreadPriorityBoost
    {TRUE, 0},					// ThreadSetTlsArrayAddress
    {FALSE, 0},					// ThreadIsIoPending
    {TRUE, 0}					// ThreadHideFromDebugger
};

static const struct
{
   BOOLEAN Implemented;
   ULONG Size;
} SetInformationData[MaxThreadInfoClass + 1] = 
{
    {TRUE, 0},			// ThreadBasicInformation
    {TRUE, 0},			// ThreadTimes
    {TRUE, sizeof(KPRIORITY)},	// ThreadPriority
    {TRUE, sizeof(LONG)},	// ThreadBasePriority
    {TRUE, sizeof(KAFFINITY)},	// ThreadAffinityMask
    {TRUE, sizeof(HANDLE)},	// ThreadImpersonationToken
    {TRUE, 0},			// ThreadDescriptorTableEntry
    {FALSE, 0},			// ThreadEnableAlignmentFaultFixup
#ifdef _ENABLE_THRDEVTPAIR
    {TRUE, sizeof(HANDLE)},	// ThreadEventPair
#else
    {FALSE, 0},			// ThreadEventPair
#endif
    {TRUE, sizeof(PVOID)},	// ThreadQuerySetWin32StartAddress
    {FALSE, 0},			// ThreadZeroTlsCell
    {TRUE, 0},			// ThreadPerformanceCount
    {TRUE, 0},			// ThreadAmILastThread
    {FALSE, 0},			// ThreadIdealProcessor
    {FALSE, 0},			// ThreadPriorityBoost
    {FALSE, 0},			// ThreadSetTlsArrayAddress
    {TRUE, 0},			// ThreadIsIoPending
    {FALSE, 0}			// ThreadHideFromDebugger
};

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtSetInformationThread (IN HANDLE ThreadHandle,
			IN THREADINFOCLASS ThreadInformationClass,
			IN PVOID ThreadInformation,
			IN ULONG ThreadInformationLength)
{
  PETHREAD Thread;
  NTSTATUS Status;
  union
  {
     KPRIORITY Priority;
     LONG Increment;
     KAFFINITY Affinity;
     HANDLE Handle;
     PVOID Address;
  }u;

  if (ThreadInformationClass <= MaxThreadInfoClass &&
      !SetInformationData[ThreadInformationClass].Implemented)
    {
      return STATUS_NOT_IMPLEMENTED;
    }
  if (ThreadInformationClass > MaxThreadInfoClass ||
      SetInformationData[ThreadInformationClass].Size == 0)
    {
      return STATUS_INVALID_INFO_CLASS;
    }
  if (ThreadInformationLength != SetInformationData[ThreadInformationClass].Size)
    {
      return STATUS_INFO_LENGTH_MISMATCH;
    }

  Status = ObReferenceObjectByHandle (ThreadHandle,
				      THREAD_SET_INFORMATION,
				      PsThreadType,
				      ExGetPreviousMode (),
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

   Status = MmCopyFromCaller(&u.Priority,
			     ThreadInformation,
			     SetInformationData[ThreadInformationClass].Size);
   if (NT_SUCCESS(Status))
     {
       switch (ThreadInformationClass)
         {
           case ThreadPriority:
	     if (u.Priority < LOW_PRIORITY || u.Priority >= MAXIMUM_PRIORITY)
	       {
		 Status = STATUS_INVALID_PARAMETER;
		 break;
	       }
	     KeSetPriorityThread(&Thread->Tcb, u.Priority);
	     break;
	
           case ThreadBasePriority:
	     KeSetBasePriorityThread (&Thread->Tcb, u.Increment);
	     break;
	
           case ThreadAffinityMask:
	     Status = KeSetAffinityThread(&Thread->Tcb, u.Affinity);
	     break;
	
           case ThreadImpersonationToken:
	     Status = PsAssignImpersonationToken (Thread, u.Handle);
	     break;
		
#ifdef _ENABLE_THRDEVTPAIR
           case ThreadEventPair:
	     {
	       PKEVENT_PAIR EventPair;

	       Status = ObReferenceObjectByHandle(u.Handle,
					          STANDARD_RIGHTS_ALL,
					          ExEventPairObjectType,
					          ExGetPreviousMode(),
					          (PVOID*)&EventPair,
					          NULL);
	       if (NT_SUCCESS(Status))
	         {
                   ExpSwapThreadEventPair(Thread, EventPair); /* Note that the extra reference is kept intentionally */
		 }
               break;
	     }
#endif /* _ENABLE_THRDEVTPAIR */
	
           case ThreadQuerySetWin32StartAddress:
	     Thread->Win32StartAddress = u.Address;
	     break;

           default:
	     /* Shoult never occure if the data table is correct */
	     KEBUGCHECK(0);
	 }
     }
  ObDereferenceObject (Thread);

  return Status;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
NtQueryInformationThread (IN	HANDLE		ThreadHandle,
			  IN	THREADINFOCLASS	ThreadInformationClass,
			  OUT	PVOID		ThreadInformation,
			  IN	ULONG		ThreadInformationLength,
			  OUT	PULONG		ReturnLength  OPTIONAL)
{
   PETHREAD Thread;
   NTSTATUS Status;
   union
   {
      THREAD_BASIC_INFORMATION TBI;
      KERNEL_USER_TIMES TTI;
      PVOID Address;
      LARGE_INTEGER Count;
      BOOLEAN Last;
   }u;

  if (ThreadInformationClass <= MaxThreadInfoClass &&
      !QueryInformationData[ThreadInformationClass].Implemented)
    {
      return STATUS_NOT_IMPLEMENTED;
    }
  if (ThreadInformationClass > MaxThreadInfoClass ||
      QueryInformationData[ThreadInformationClass].Size == 0)
    {
      return STATUS_INVALID_INFO_CLASS;
    }
  if (ThreadInformationLength != QueryInformationData[ThreadInformationClass].Size)
    {
      return STATUS_INFO_LENGTH_MISMATCH;
    }

   Status = ObReferenceObjectByHandle(ThreadHandle,
				      THREAD_QUERY_INFORMATION,
				      PsThreadType,
				      ExGetPreviousMode(),
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }

   switch (ThreadInformationClass)
     {
       case ThreadBasicInformation:
         /* A test on W2K agains ntdll shows NtQueryInformationThread return STATUS_PENDING
          * as ExitStatus for current/running thread, while KETHREAD's ExitStatus is 
          * 0. So do the conversion here:
          * -Gunnar     */
         u.TBI.ExitStatus = (Thread->ExitStatus == 0) ? STATUS_PENDING : Thread->ExitStatus;
	 u.TBI.TebBaseAddress = Thread->Tcb.Teb;
	 u.TBI.ClientId = Thread->Cid;
	 u.TBI.AffinityMask = Thread->Tcb.Affinity;
	 u.TBI.Priority = Thread->Tcb.Priority;
	 u.TBI.BasePriority = Thread->Tcb.BasePriority;
	 break;
       
       case ThreadTimes:
	 u.TTI.KernelTime.QuadPart = Thread->Tcb.KernelTime * 100000LL;
         u.TTI.UserTime.QuadPart = Thread->Tcb.UserTime * 100000LL;
         u.TTI.CreateTime = Thread->CreateTime;
         /*This works*/
	 u.TTI.ExitTime = Thread->ExitTime;
         break;

       case ThreadQuerySetWin32StartAddress:
         u.Address = Thread->Win32StartAddress;
         break;

       case ThreadPerformanceCount:
         /* Nebbett says this class is always zero */
         u.Count.QuadPart = 0;
         break;

       case ThreadAmILastThread:
         if (Thread->ThreadsProcess->ThreadListHead.Flink->Flink ==
	     &Thread->ThreadsProcess->ThreadListHead)
	   {
	     u.Last = TRUE;
	   }
         else
	   {
	     u.Last = FALSE;
	   }
         break;
       default:
	 /* Shoult never occure if the data table is correct */
	 KEBUGCHECK(0);
     }
   if (QueryInformationData[ThreadInformationClass].Size)
     {
       Status = MmCopyToCaller(ThreadInformation,
                               &u.TBI,
			       QueryInformationData[ThreadInformationClass].Size);
     }
   if (ReturnLength)
     {
       NTSTATUS Status2;
       static ULONG Null = 0;
       Status2 = MmCopyToCaller(ReturnLength,
	                        NT_SUCCESS(Status) ? &QueryInformationData[ThreadInformationClass].Size : &Null,
				sizeof(ULONG));
       if (NT_SUCCESS(Status))
         {
	   Status = Status2;
	 }
     }

   ObDereferenceObject(Thread);
   return(Status);
}


VOID
KeSetPreviousMode (ULONG Mode)
{
  PsGetCurrentThread()->Tcb.PreviousMode = (UCHAR)Mode;
}


/*
 * @implemented
 */
KPROCESSOR_MODE STDCALL
KeGetPreviousMode (VOID)
{
  return (ULONG)PsGetCurrentThread()->Tcb.PreviousMode;
}


/*
 * @implemented
 */
KPROCESSOR_MODE STDCALL
ExGetPreviousMode (VOID)
{
  return (KPROCESSOR_MODE)PsGetCurrentThread()->Tcb.PreviousMode;
}

/* EOF */
