/* $Id: create.c,v 1.4 1999/12/20 02:14:40 dwelch Exp $
 *
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/thread.c
 * PURPOSE:                Thread managment
 * PROGRAMMER:             David Welch (welch@mcmail.com)
 * REVISION HISTORY: 
 *               23/06/98: Created
 *               12/10/99: Phillip Susi:  Thread priorities, and APC work
 */

/*
 * NOTE:
 * 
 * All of the routines that manipulate the thread queue synchronize on
 * a single spinlock
 * 
 */

/* INCLUDES ****************************************************************/

#include <ddk/ntddk.h>
#include <internal/ke.h>
#include <internal/ob.h>
#include <string.h>
#include <internal/string.h>
#include <internal/hal.h>
#include <internal/ps.h>
#include <internal/ob.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBAL *******************************************************************/

static ULONG PiNextThreadUniqueId = 0;

extern KSPIN_LOCK PiThreadListLock;
extern ULONG PiNrThreads;

extern LIST_ENTRY PiThreadListHead;

/* FUNCTIONS ***************************************************************/

static VOID PiTimeoutThread( struct _KDPC *dpc, PVOID Context, PVOID arg1, PVOID arg2 )
{
   // wake up the thread, and tell it it timed out
   NTSTATUS Status = STATUS_TIMEOUT;
   PsUnfreezeThread( (ETHREAD *)Context, &Status );
}

VOID PiBeforeBeginThread(VOID)
{
   DPRINT("PiBeforeBeginThread()\n");
   //KeReleaseSpinLock(&PiThreadListLock, PASSIVE_LEVEL);
   KeLowerIrql(PASSIVE_LEVEL);
   DPRINT("KeGetCurrentIrql() %d\n", KeGetCurrentIrql());
}

VOID PsBeginThread(PKSTART_ROUTINE StartRoutine, PVOID StartContext)
{
   NTSTATUS Ret;
   
//   KeReleaseSpinLock(&PiThreadListLock,PASSIVE_LEVEL);
   KeLowerIrql(PASSIVE_LEVEL);
   Ret = StartRoutine(StartContext);
   PsTerminateSystemThread(Ret);
   KeBugCheck(0);
}

VOID PiDeleteThread(PVOID ObjectBody)
{
   KIRQL oldIrql;
   
   DPRINT1("PiDeleteThread(ObjectBody %x)\n",ObjectBody);
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   ObDereferenceObject(((PETHREAD)ObjectBody)->ThreadsProcess);
   ((PETHREAD)ObjectBody)->ThreadsProcess = NULL;
   PiNrThreads--;
   RemoveEntryList(&((PETHREAD)ObjectBody)->Tcb.ThreadListEntry);
   HalReleaseTask((PETHREAD)ObjectBody);
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);
   DPRINT1("PiDeleteThread() finished\n");
}

VOID PiCloseThread(PVOID ObjectBody, ULONG HandleCount)
{
   DPRINT("PiCloseThread(ObjectBody %x)\n", ObjectBody);
   DPRINT("ObGetReferenceCount(ObjectBody) %d "
	   "ObGetHandleCount(ObjectBody) %d\n",
	   ObGetReferenceCount(ObjectBody),
	   ObGetHandleCount(ObjectBody));
}

NTSTATUS PsInitializeThread(HANDLE			ProcessHandle, 
			    PETHREAD		* ThreadPtr,
			    PHANDLE			ThreadHandle,
			    ACCESS_MASK		DesiredAccess,
			    POBJECT_ATTRIBUTES	ThreadAttributes)
{
   PETHREAD Thread;
   NTSTATUS Status;
   KIRQL oldIrql;
   PEPROCESS Process;
   
   /*
    * Reference process
    */
   if (ProcessHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(ProcessHandle,
					   PROCESS_CREATE_THREAD,
					   PsProcessType,
					   UserMode,
					   (PVOID*)&Process,
					   NULL);
	if (Status != STATUS_SUCCESS)
	  {
	     DPRINT("Failed at %s:%d\n",__FILE__,__LINE__);
	     return(Status);
	  }
     }
   else
     {
	Process = SystemProcess;
	ObReferenceObjectByPointer(Process,
				   PROCESS_CREATE_THREAD,
				   PsProcessType,
				   UserMode);
     }
   
   /*
    * Create and initialize thread
    */
   Thread = ObCreateObject(ThreadHandle,
			   DesiredAccess,
			   ThreadAttributes,
			   PsThreadType);
   DPRINT("Thread = %x\n",Thread);
   
   PiNrThreads++;
   
   Thread->Tcb.State = THREAD_STATE_SUSPENDED;
   Thread->Tcb.SuspendCount = 0;
   Thread->Tcb.FreezeCount = 1;
   InitializeListHead(&Thread->Tcb.ApcState.ApcListHead[0]);
   InitializeListHead(&Thread->Tcb.ApcState.ApcListHead[1]);
   Thread->Tcb.KernelApcDisable = 1;
   Thread->Tcb.WaitIrql = PASSIVE_LEVEL;
   Thread->ThreadsProcess = Process;
   KeInitializeDpc( &Thread->Tcb.TimerDpc, PiTimeoutThread, Thread );
   Thread->Tcb.WaitBlockList = NULL;
   InsertTailList( &Thread->ThreadsProcess->Pcb.ThreadListHead, &Thread->Tcb.ProcessThreadListEntry );
   KeInitializeDispatcherHeader(&Thread->Tcb.DispatcherHeader,
                                InternalThreadType,
                                sizeof(ETHREAD),
                                FALSE);

   InitializeListHead(&Thread->IrpList);
   Thread->Cid.UniqueThread = (HANDLE)InterlockedIncrement(
					      &PiNextThreadUniqueId);
   Thread->Cid.UniqueProcess = (HANDLE)Thread->ThreadsProcess->UniqueProcessId;
   DPRINT("Thread->Cid.UniqueThread %d\n",Thread->Cid.UniqueThread);
   
   *ThreadPtr = Thread;
   
   KeAcquireSpinLock(&PiThreadListLock, &oldIrql);
   InsertTailList(&PiThreadListHead, &Thread->Tcb.ThreadListEntry);
   KeReleaseSpinLock(&PiThreadListLock, oldIrql);

   Thread->Tcb.BasePriority = Thread->ThreadsProcess->Pcb.BasePriority;
   Thread->Tcb.Priority = Thread->Tcb.BasePriority;
   
   return(STATUS_SUCCESS);
}


static NTSTATUS PsCreateTeb (HANDLE ProcessHandle,
			     PNT_TEB *TebPtr,
			     PETHREAD Thread,
			     PINITIAL_TEB InitialTeb)
{
   MEMORY_BASIC_INFORMATION Info;
   NTSTATUS Status;
   ULONG ByteCount;
   ULONG RegionSize;
   ULONG TebSize;
   PVOID TebBase;
   NT_TEB Teb;

   TebBase = (PVOID)0x7FFDE000;
   TebSize = PAGESIZE;

   while (TRUE)
     {
        /* The TEB must reside in user space */
        Status = NtAllocateVirtualMemory(ProcessHandle,
                                         &TebBase,
                                         0,
                                         &TebSize,
                                         MEM_COMMIT,
                                         PAGE_READWRITE);
        if (NT_SUCCESS(Status))
          {
             DPRINT ("TEB allocated at %x\n", TebBase);
             break;
          }
        else
          {
             DPRINT ("TEB allocation failed! Status %x\n",Status);
          }

        TebBase = Info.BaseAddress - TebSize;
   }

   DPRINT ("TebBase %p TebSize %lu\n", TebBase, TebSize);

   /* set all pointers to and from the TEB */
   Teb.Tib.Self = TebBase;
   if (Thread->ThreadsProcess)
     {
        Teb.Peb = Thread->ThreadsProcess->Peb; /* No PEB yet!! */
     }

   /* store stack information from InitialTeb */
   if (InitialTeb != NULL)
     {
        Teb.Tib.StackBase = InitialTeb->StackBase;
        Teb.Tib.StackLimit = InitialTeb->StackLimit;

        /*
         * I don't know if this is really stored in a WNT-TEB,
         * but it's needed to free the thread stack. (Eric Kohl)
         */
        Teb.StackCommit = InitialTeb->StackCommit;
        Teb.StackCommitMax = InitialTeb->StackCommitMax;
        Teb.StackReserved = InitialTeb->StackReserved;
     }


   /* more initialization */
   Teb.Cid.UniqueThread = Thread->Cid.UniqueThread;
   Teb.Cid.UniqueProcess = Thread->Cid.UniqueProcess;

   /* write TEB data into teb page */
   Status = NtWriteVirtualMemory(ProcessHandle,
                                 TebBase,
                                 &Teb,
                                 sizeof(NT_TEB),
                                 &ByteCount);

   if (!NT_SUCCESS(Status))
     {
        /* free TEB */
        DPRINT ("Writing TEB failed!\n");

        RegionSize = 0;
        NtFreeVirtualMemory(ProcessHandle,
                            TebBase,
                            &RegionSize,
                            MEM_RELEASE);

        return Status;
     }

   /* FIXME: fs:[0] = TEB */

   if (TebPtr != NULL)
     {
//        *TebPtr = (PNT_TEB)TebBase;
     }

   DPRINT ("TEB allocated at %p\n", TebBase);

   return Status;
}


NTSTATUS STDCALL NtCreateThread (PHANDLE			ThreadHandle,
				 ACCESS_MASK		DesiredAccess,
				 POBJECT_ATTRIBUTES	ObjectAttributes,
				 HANDLE			ProcessHandle,
				 PCLIENT_ID		Client,
				 PCONTEXT		ThreadContext,
				 PINITIAL_TEB		InitialTeb,
				 BOOLEAN CreateSuspended)
{
   PETHREAD Thread;
   PNT_TEB  TebBase;
   NTSTATUS Status;
   
   DPRINT("NtCreateThread(ThreadHandle %x, PCONTEXT %x)\n",
	  ThreadHandle,ThreadContext);
   
   Status = PsInitializeThread(ProcessHandle,&Thread,ThreadHandle,
			       DesiredAccess,ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Status = HalInitTaskWithContext(Thread,ThreadContext);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   Status = PsCreateTeb (ProcessHandle,
                         &TebBase,
                         Thread,
                         InitialTeb);
   if (!NT_SUCCESS(Status))
     {
        return(Status);
     }

   /* Attention: TebBase is in user memory space */
//   Thread->Tcb.Teb = TebBase;

   Thread->StartAddress=NULL;

   if (Client!=NULL)
     {
	*Client=Thread->Cid;
     }  
   
   if (!CreateSuspended)
     {
        DPRINT("Not creating suspended\n");
	PsUnfreezeThread(Thread, NULL);
     }
   DPRINT("Thread %x\n", Thread);
   DPRINT("ObGetReferenceCount(Thread) %d ObGetHandleCount(Thread) %x\n",
	  ObGetReferenceCount(Thread), ObGetHandleCount(Thread));
   DPRINT("Finished PsCreateThread()\n");
   return(STATUS_SUCCESS);
}


NTSTATUS PsCreateSystemThread(PHANDLE ThreadHandle,
			      ACCESS_MASK DesiredAccess,
			      POBJECT_ATTRIBUTES ObjectAttributes,
			      HANDLE ProcessHandle,
			      PCLIENT_ID ClientId,
			      PKSTART_ROUTINE StartRoutine,
			      PVOID StartContext)
/*
 * FUNCTION: Creates a thread which executes in kernel mode
 * ARGUMENTS:
 *       ThreadHandle (OUT) = Caller supplied storage for the returned thread 
 *                            handle
 *       DesiredAccess = Requested access to the thread
 *       ObjectAttributes = Object attributes (optional)
 *       ProcessHandle = Handle of process thread will run in
 *                       NULL to use system process
 *       ClientId (OUT) = Caller supplied storage for the returned client id
 *                        of the thread (optional)
 *       StartRoutine = Entry point for the thread
 *       StartContext = Argument supplied to the thread when it begins
 *                     execution
 * RETURNS: Success or failure status
 */
{
   PETHREAD Thread;
   NTSTATUS Status;
   
   DPRINT("PsCreateSystemThread(ThreadHandle %x, ProcessHandle %x)\n",
	    ThreadHandle,ProcessHandle);
   
   Status = PsInitializeThread(ProcessHandle,&Thread,ThreadHandle,
			       DesiredAccess,ObjectAttributes);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Thread->StartAddress=StartRoutine;
   Status = HalInitTask(Thread,StartRoutine,StartContext);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   if (ClientId!=NULL)
     {
	*ClientId=Thread->Cid;
     }  

   PsUnfreezeThread(Thread, NULL);
   
   return(STATUS_SUCCESS);
}

