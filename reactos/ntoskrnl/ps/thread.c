/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/thread.c
 * PURPOSE:         Thread managment
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Phillip Susi
 */

/*
 * NOTE:
 *
 * All of the routines that manipulate the thread queue synchronize on
 * a single spinlock
 *
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

extern LIST_ENTRY PsActiveProcessHead;
extern PEPROCESS PsIdleProcess;

POBJECT_TYPE EXPORTED PsThreadType = NULL;

extern PVOID Ki386InitialStackArray[MAXIMUM_PROCESSORS];
extern ULONG IdleProcessorMask;
extern LIST_ENTRY PriorityListHead[MAXIMUM_PRIORITY];


BOOLEAN DoneInitYet = FALSE;
static GENERIC_MAPPING PiThreadMapping = {STANDARD_RIGHTS_READ | THREAD_GET_CONTEXT | THREAD_QUERY_INFORMATION,
					  STANDARD_RIGHTS_WRITE | THREAD_TERMINATE | THREAD_SUSPEND_RESUME | THREAD_ALERT |
                      THREAD_SET_INFORMATION | THREAD_SET_CONTEXT,
                      STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
					  THREAD_ALL_ACCESS};

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
PKTHREAD STDCALL KeGetCurrentThread(VOID)
{
#ifdef CONFIG_SMP
   ULONG Flags;
   PKTHREAD Thread;
   Ke386SaveFlags(Flags);
   Ke386DisableInterrupts();
   Thread = KeGetCurrentPrcb()->CurrentThread;
   Ke386RestoreFlags(Flags);
   return Thread;
#else
   return(KeGetCurrentPrcb()->CurrentThread);
#endif
}

/*
 * @implemented
 */
HANDLE STDCALL PsGetCurrentThreadId(VOID)
{
   return(PsGetCurrentThread()->Cid.UniqueThread);
}

/*
 * @implemented
 */
ULONG
STDCALL
PsGetThreadFreezeCount(
	PETHREAD Thread
	)
{
	return Thread->Tcb.FreezeCount;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsGetThreadHardErrorsAreDisabled(
    PETHREAD	Thread
	)
{
	return Thread->HardErrorsAreDisabled;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadId(
    PETHREAD	Thread
	)
{
	return Thread->Cid.UniqueThread;
}

/*
 * @implemented
 */
PEPROCESS
STDCALL
PsGetThreadProcess(
    PETHREAD	Thread
	)
{
	return Thread->ThreadsProcess;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadProcessId(
    PETHREAD	Thread
	)
{
	return Thread->Cid.UniqueProcess;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetThreadSessionId(
    PETHREAD	Thread
	)
{
	return (HANDLE)Thread->ThreadsProcess->SessionId;
}

/*
 * @implemented
 */
PTEB
STDCALL
PsGetThreadTeb(
    PETHREAD	Thread
	)
{
	return Thread->Tcb.Teb;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetThreadWin32Thread(
    PETHREAD	Thread
	)
{
	return Thread->Tcb.Win32Thread;
}

/*
 * @implemented
 */
KPROCESSOR_MODE
STDCALL
PsGetCurrentThreadPreviousMode (
    	VOID
	)
{
	return (KPROCESSOR_MODE)PsGetCurrentThread()->Tcb.PreviousMode;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetCurrentThreadStackBase (
    	VOID
	)
{
	return PsGetCurrentThread()->Tcb.StackBase;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetCurrentThreadStackLimit (
    	VOID
	)
{
	return (PVOID)PsGetCurrentThread()->Tcb.StackLimit;
}

/*
 * @implemented
 */
BOOLEAN STDCALL
PsIsThreadTerminating(IN PETHREAD Thread)
{
  return (Thread->HasTerminated ? TRUE : FALSE);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsIsSystemThread(PETHREAD Thread)
{
    return (Thread->SystemThread ? TRUE: FALSE);
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsIsThreadImpersonating(
    PETHREAD	Thread
	)
{
  return Thread->ActiveImpersonationInfo;
}

VOID PsDumpThreads(BOOLEAN IncludeSystem)
{
   PLIST_ENTRY AThread, AProcess;
   PEPROCESS Process;
   PETHREAD Thread;
   ULONG nThreads = 0;
   
   AProcess = PsActiveProcessHead.Flink;
   while(AProcess != &PsActiveProcessHead)
   {
     Process = CONTAINING_RECORD(AProcess, EPROCESS, ProcessListEntry);
     /* FIXME - skip suspended, ... processes? */
     if((Process != PsInitialSystemProcess) ||
        (Process == PsInitialSystemProcess && IncludeSystem))
     {
       AThread = Process->ThreadListHead.Flink;
       while(AThread != &Process->ThreadListHead)
       {
         Thread = CONTAINING_RECORD(AThread, ETHREAD, ThreadListEntry);

         nThreads++;
         DbgPrint("Thread->Tcb.State %d Affinity %08x Priority %d PID.TID %d.%d Name %.8s Stack: \n",
                  Thread->Tcb.State,
		  Thread->Tcb.Affinity,
		  Thread->Tcb.Priority,
                  Thread->ThreadsProcess->UniqueProcessId,
                  Thread->Cid.UniqueThread,
                  Thread->ThreadsProcess->ImageFileName);
         if(Thread->Tcb.State == THREAD_STATE_READY ||
            Thread->Tcb.State == THREAD_STATE_SUSPENDED ||
            Thread->Tcb.State == THREAD_STATE_BLOCKED)
         {
           ULONG i = 0;
           PULONG Esp = (PULONG)Thread->Tcb.KernelStack;
           PULONG Ebp = (PULONG)Esp[4];
           DbgPrint("Ebp 0x%.8X\n", Ebp);
           while(Ebp != 0 && Ebp >= (PULONG)Thread->Tcb.StackLimit)
           {
             DbgPrint("%.8X %.8X%s", Ebp[0], Ebp[1], (i % 8) == 7 ? "\n" : "  ");
             Ebp = (PULONG)Ebp[0];
             i++;
           }
           if((i % 8) != 0)
           {
             DbgPrint("\n");
           }
         }
         AThread = AThread->Flink;
       }
     }
     AProcess = AProcess->Flink;
   }
}

VOID
PsFreezeAllThreads(PEPROCESS Process)
     /*
      * Used by the debugging code to freeze all the process's threads
      * while the debugger is examining their state.
      */
{
  KIRQL oldIrql;
  PLIST_ENTRY current_entry;
  PETHREAD current;

  oldIrql = KeAcquireDispatcherDatabaseLock();
  current_entry = Process->ThreadListHead.Flink;
  while (current_entry != &Process->ThreadListHead)
    {
      current = CONTAINING_RECORD(current_entry, ETHREAD,
				  ThreadListEntry);

      /*
       * We have to be careful here, we can't just set the freeze the
       * thread inside kernel mode since it may be holding a lock.
       */

      current_entry = current_entry->Flink;
    }

    KeReleaseDispatcherDatabaseLock(oldIrql);
}

ULONG
PsEnumThreadsByProcess(PEPROCESS Process)
{
  KIRQL oldIrql;
  PLIST_ENTRY current_entry;
  ULONG Count = 0;

  oldIrql = KeAcquireDispatcherDatabaseLock();

  current_entry = Process->ThreadListHead.Flink;
  while (current_entry != &Process->ThreadListHead)
    {
      Count++;
      current_entry = current_entry->Flink;
    }
  
  KeReleaseDispatcherDatabaseLock(oldIrql);
  return Count;
}

/*
 * @unimplemented
 */
NTSTATUS
STDCALL
PsRemoveCreateThreadNotifyRoutine (
    IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;	
}

/*
 * @unimplemented
 */                       
ULONG
STDCALL
PsSetLegoNotifyRoutine(   	
	PVOID LegoNotifyRoutine  	 
	)
{
	UNIMPLEMENTED;
	return 0;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsSetThreadHardErrorsAreDisabled(
    PETHREAD	Thread,
    BOOLEAN	HardErrorsAreDisabled
	)
{
	Thread->HardErrorsAreDisabled = HardErrorsAreDisabled;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsSetThreadWin32Thread(
    PETHREAD	Thread,
    PVOID	Win32Thread
	)
{
	Thread->Tcb.Win32Thread = Win32Thread;
}

VOID
PsApplicationProcessorInit(VOID)
{
   KIRQL oldIrql;
   oldIrql = KeAcquireDispatcherDatabaseLock();
   IdleProcessorMask |= (1 << KeGetCurrentProcessorNumber());
   KeReleaseDispatcherDatabaseLock(oldIrql);
}

VOID INIT_FUNCTION
PsPrepareForApplicationProcessorInit(ULONG Id)
{
  PETHREAD IdleThread;
  PKPRCB Prcb = ((PKPCR)((ULONG_PTR)KPCR_BASE + Id * PAGE_SIZE))->Prcb;

  PsInitializeThread(PsIdleProcess,
		     &IdleThread,
		     NULL,
		     KernelMode,
		     FALSE);
  IdleThread->Tcb.State = THREAD_STATE_RUNNING;
  IdleThread->Tcb.FreezeCount = 0;
  IdleThread->Tcb.Affinity = 1 << Id;
  IdleThread->Tcb.UserAffinity = 1 << Id;
  IdleThread->Tcb.Priority = LOW_PRIORITY;
  IdleThread->Tcb.BasePriority = LOW_PRIORITY;
  Prcb->IdleThread = &IdleThread->Tcb;
  Prcb->CurrentThread = &IdleThread->Tcb;

  Ki386InitialStackArray[Id] = (PVOID)IdleThread->Tcb.StackLimit;

  DPRINT("IdleThread for Processor %d has PID %d\n",
	   Id, IdleThread->Cid.UniqueThread);
}

VOID INIT_FUNCTION
PsInitThreadManagment(VOID)
/*
 * FUNCTION: Initialize thread managment
 */
{
   PETHREAD FirstThread;
   ULONG i;

   for (i=0; i < MAXIMUM_PRIORITY; i++)
     {
	InitializeListHead(&PriorityListHead[i]);
     }

   PsThreadType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));

   PsThreadType->Tag = TAG('T', 'H', 'R', 'T');
   PsThreadType->TotalObjects = 0;
   PsThreadType->TotalHandles = 0;
   PsThreadType->PeakObjects = 0;
   PsThreadType->PeakHandles = 0;
   PsThreadType->PagedPoolCharge = 0;
   PsThreadType->NonpagedPoolCharge = sizeof(ETHREAD);
   PsThreadType->Mapping = &PiThreadMapping;
   PsThreadType->Dump = NULL;
   PsThreadType->Open = NULL;
   PsThreadType->Close = NULL;
   PsThreadType->Delete = PspDeleteThread;
   PsThreadType->Parse = NULL;
   PsThreadType->Security = NULL;
   PsThreadType->QueryName = NULL;
   PsThreadType->OkayToClose = NULL;
   PsThreadType->Create = NULL;
   PsThreadType->DuplicationNotify = NULL;

   RtlInitUnicodeString(&PsThreadType->TypeName, L"Thread");

   ObpCreateTypeObject(PsThreadType);

   PsInitializeThread(NULL, &FirstThread, NULL, KernelMode, TRUE);
   FirstThread->Tcb.State = THREAD_STATE_RUNNING;
   FirstThread->Tcb.FreezeCount = 0;
   FirstThread->Tcb.UserAffinity = (1 << 0);   /* Set the affinity of the first thread to the boot processor */
   FirstThread->Tcb.Affinity = (1 << 0);
   KeGetCurrentPrcb()->CurrentThread = (PVOID)FirstThread;

   DPRINT("FirstThread %x\n",FirstThread);

   DoneInitYet = TRUE;
   
   InitializeListHead(&PspReaperListHead);
   ExInitializeWorkItem(&PspReaperWorkItem, PspReapRoutine, NULL);
}

/**********************************************************************
 *	NtOpenThread/4
 *
 *	@implemented
 */
NTSTATUS STDCALL
NtOpenThread(OUT PHANDLE ThreadHandle,
	     IN	ACCESS_MASK DesiredAccess,
	     IN	POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
	     IN	PCLIENT_ID ClientId  OPTIONAL)
{
   KPROCESSOR_MODE PreviousMode;
   CLIENT_ID SafeClientId;
   HANDLE hThread;
   NTSTATUS Status = STATUS_SUCCESS;

   PAGED_CODE();

   PreviousMode = ExGetPreviousMode();

   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(ThreadHandle,
                     sizeof(HANDLE),
                     sizeof(ULONG));
       if(ClientId != NULL)
       {
         ProbeForRead(ClientId,
                      sizeof(CLIENT_ID),
                      sizeof(ULONG));
         SafeClientId = *ClientId;
         ClientId = &SafeClientId;
       }
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;

     if(!NT_SUCCESS(Status))
     {
       return Status;
     }
   }

   if(!((ObjectAttributes == NULL) ^ (ClientId == NULL)))
   {
     DPRINT("NtOpenThread should be called with either ObjectAttributes or ClientId!\n");
     return STATUS_INVALID_PARAMETER;
   }

   if(ClientId != NULL)
   {
     PETHREAD Thread;

     Status = PsLookupThreadByThreadId(ClientId->UniqueThread,
                                       &Thread);
     if(NT_SUCCESS(Status))
     {
       Status = ObInsertObject(Thread,
                               NULL,
                               DesiredAccess,
                               0,
                               NULL,
                               &hThread);

       ObDereferenceObject(Thread);
     }
   }
   else
   {
     Status = ObOpenObjectByName(ObjectAttributes,
                                 PsThreadType,
                                 NULL,
                                 PreviousMode,
                                 DesiredAccess,
                                 NULL,
                                 &hThread);
   }

   if(NT_SUCCESS(Status))
   {
     _SEH_TRY
     {
       *ThreadHandle = hThread;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }

   return Status;
}

NTSTATUS STDCALL
NtYieldExecution(VOID)
{
  KiDispatchThread(THREAD_STATE_READY);
  return(STATUS_SUCCESS);
}


/*
 * NOT EXPORTED
 */
NTSTATUS STDCALL
NtTestAlert(VOID)
{
  /* Check and Alert Thread if needed */
  return KeTestAlertThread(ExGetPreviousMode()) ? STATUS_ALERTED : STATUS_SUCCESS;
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
