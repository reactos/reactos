/* $Id: create.c,v 1.85 2004/11/10 02:51:00 ion Exp $
 *
 * COPYRIGHT:              See COPYING in the top level directory
 * PROJECT:                ReactOS kernel
 * FILE:                   ntoskrnl/ps/thread.c
 * PURPOSE:                Thread managment
 * PROGRAMMER:             David Welch (welch@mcmail.com)
 * REVISION HISTORY: 
 *               23/06/98: Created
 *               12/10/99: Phillip Susi:  Thread priorities, and APC work
 *               09/08/03: Skywing:       ThreadEventPair support (delete)
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

/* GLOBAL *******************************************************************/

extern KSPIN_LOCK PiThreadLock;

#define MAX_THREAD_NOTIFY_ROUTINE_COUNT    8

static ULONG PiThreadNotifyRoutineCount = 0;
static PCREATE_THREAD_NOTIFY_ROUTINE
PiThreadNotifyRoutine[MAX_THREAD_NOTIFY_ROUTINE_COUNT];

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
NTSTATUS STDCALL
PsAssignImpersonationToken(PETHREAD Thread,
			   HANDLE TokenHandle)
{
   PACCESS_TOKEN Token;
   SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
   NTSTATUS Status;

   if (TokenHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(TokenHandle,
					   TOKEN_IMPERSONATE,
					   SepTokenObjectType,
					   KeGetPreviousMode(),
					   (PVOID*)&Token,
					   NULL);
	if (!NT_SUCCESS(Status))
	  {
	     return(Status);
	  }
	ImpersonationLevel = Token->ImpersonationLevel;
     }
   else
     {
	Token = NULL;
	ImpersonationLevel = 0;
     }

   PsImpersonateClient(Thread,
		       Token,
		       FALSE,
		       FALSE,
		       ImpersonationLevel);
   if (Token != NULL)
     {
	ObDereferenceObject(Token);
     }

   return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
VOID STDCALL
PsRevertToSelf (VOID)
{
    PsRevertThreadToSelf(PsGetCurrentThread());
}

/*
 * @implemented
 */
VOID
STDCALL
PsRevertThreadToSelf(
	IN PETHREAD Thread
	)
{
  if (Thread->ActiveImpersonationInfo == TRUE)
    {
      ObDereferenceObject (Thread->ImpersonationInfo->Token);
      Thread->ActiveImpersonationInfo = FALSE;
    }
}

/*
 * @implemented
 */
VOID STDCALL
PsImpersonateClient (IN PETHREAD Thread,
		     IN PACCESS_TOKEN Token,
		     IN BOOLEAN CopyOnOpen,
		     IN BOOLEAN EffectiveOnly,
		     IN SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
  if (Token == NULL)
    {
      if (Thread->ActiveImpersonationInfo == TRUE)
	{
	  Thread->ActiveImpersonationInfo = FALSE;
	  if (Thread->ImpersonationInfo->Token != NULL)
	    {
	      ObDereferenceObject (Thread->ImpersonationInfo->Token);
	    }
	}
      return;
    }

  if (Thread->ImpersonationInfo == NULL)
    {
      Thread->ImpersonationInfo = ExAllocatePool (NonPagedPool,
						  sizeof(PS_IMPERSONATION_INFORMATION));
    }

  Thread->ImpersonationInfo->Level = ImpersonationLevel;
  Thread->ImpersonationInfo->CopyOnOpen = CopyOnOpen;
  Thread->ImpersonationInfo->EffectiveOnly = EffectiveOnly;
  Thread->ImpersonationInfo->Token = Token;
  ObReferenceObjectByPointer (Token,
			      0,
			      SepTokenObjectType,
			      KernelMode);
  Thread->ActiveImpersonationInfo = TRUE;
}


PACCESS_TOKEN
PsReferenceEffectiveToken(PETHREAD Thread,
			  PTOKEN_TYPE TokenType,
			  PBOOLEAN EffectiveOnly,
			  PSECURITY_IMPERSONATION_LEVEL Level)
{
   PEPROCESS Process;
   PACCESS_TOKEN Token;
   
   if (Thread->ActiveImpersonationInfo == FALSE)
     {
	Process = Thread->ThreadsProcess;
	*TokenType = TokenPrimary;
	*EffectiveOnly = FALSE;
	Token = Process->Token;
     }
   else
     {
	Token = Thread->ImpersonationInfo->Token;
	*TokenType = TokenImpersonation;
	*EffectiveOnly = Thread->ImpersonationInfo->EffectiveOnly;
	*Level = Thread->ImpersonationInfo->Level;
     }
   return(Token);
}


NTSTATUS STDCALL
NtImpersonateThread(IN HANDLE ThreadHandle,
		    IN HANDLE ThreadToImpersonateHandle,
		    IN PSECURITY_QUALITY_OF_SERVICE SecurityQualityOfService)
{
  SECURITY_CLIENT_CONTEXT ClientContext;
  PETHREAD Thread;
  PETHREAD ThreadToImpersonate;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle (ThreadHandle,
				      0,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
  if (!NT_SUCCESS (Status))
    {
      return Status;
    }

  Status = ObReferenceObjectByHandle (ThreadToImpersonateHandle,
				      0,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&ThreadToImpersonate,
				      NULL);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (Thread);
      return Status;
    }

  Status = SeCreateClientSecurity (ThreadToImpersonate,
				   SecurityQualityOfService,
				   0,
				   &ClientContext);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (ThreadToImpersonate);
      ObDereferenceObject (Thread);
      return Status;
     }

  SeImpersonateClient (&ClientContext,
		       Thread);
  if (ClientContext.Token != NULL)
    {
      ObDereferenceObject (ClientContext.Token);
    }

  ObDereferenceObject (ThreadToImpersonate);
  ObDereferenceObject (Thread);

  return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS STDCALL
NtOpenThreadToken (IN HANDLE ThreadHandle,
		   IN ACCESS_MASK DesiredAccess,
		   IN BOOLEAN OpenAsSelf,
		   OUT PHANDLE TokenHandle)
{
  PACCESS_TOKEN Token;
  PETHREAD Thread;
  NTSTATUS Status;

  Status = ObReferenceObjectByHandle (ThreadHandle,
				      0,
				      PsThreadType,
				      UserMode,
				      (PVOID*)&Thread,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

  if (OpenAsSelf)
    {
      Token = Thread->ThreadsProcess->Token;
    }
  else
    {
      if (Thread->ActiveImpersonationInfo == FALSE)
	{
	  ObDereferenceObject (Thread);
	  return STATUS_NO_TOKEN;
	}

      Token = Thread->ImpersonationInfo->Token;
    }

  if (Token == NULL)
    {
      ObDereferenceObject (Thread);
      return STATUS_NO_TOKEN;
    }

  Status = ObCreateHandle (PsGetCurrentProcess(),
			   Token,
			   DesiredAccess,
			   FALSE,
			   TokenHandle);

  ObDereferenceObject (Thread);

  return Status;
}


/*
 * @implemented
 */
PACCESS_TOKEN STDCALL
PsReferenceImpersonationToken(IN PETHREAD Thread,
			      OUT PBOOLEAN CopyOnOpen,
			      OUT PBOOLEAN EffectiveOnly,
			      OUT PSECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
  if (Thread->ActiveImpersonationInfo == FALSE)
    {
      return NULL;
    }

  *ImpersonationLevel = Thread->ImpersonationInfo->Level;
  *CopyOnOpen = Thread->ImpersonationInfo->CopyOnOpen;
  *EffectiveOnly = Thread->ImpersonationInfo->EffectiveOnly;
  ObReferenceObjectByPointer (Thread->ImpersonationInfo->Token,
			      TOKEN_ALL_ACCESS,
			      SepTokenObjectType,
			      KernelMode);

  return Thread->ImpersonationInfo->Token;
}

/*
 * @unimplemented
 */
VOID
STDCALL
PsDereferenceImpersonationToken(
    IN PACCESS_TOKEN ImpersonationToken
    )
{
	UNIMPLEMENTED;	
}

/*
 * @unimplemented
 */
VOID
STDCALL
PsDereferencePrimaryToken(
    IN PACCESS_TOKEN PrimaryToken
    )
{
	UNIMPLEMENTED;	
}

/*
 * @unimplemented
 */
BOOLEAN
STDCALL
PsDisableImpersonation(
    IN PETHREAD Thread,
    IN PSE_IMPERSONATION_STATE ImpersonationState
    )
{
	UNIMPLEMENTED;
	return FALSE;	
}

/*
 * @unimplemented
 */                       
VOID
STDCALL
PsRestoreImpersonation(
	IN PETHREAD   	 Thread,
	IN PSE_IMPERSONATION_STATE  	ImpersonationState
     	)
{
	UNIMPLEMENTED;
}

VOID
PiBeforeBeginThread(CONTEXT c)
{
   KeLowerIrql(PASSIVE_LEVEL);
}


VOID STDCALL
PiDeleteThread(PVOID ObjectBody)
{
  PETHREAD Thread;
  PEPROCESS Process;

  Thread = (PETHREAD)ObjectBody;

  DPRINT("PiDeleteThread(ObjectBody %x)\n",ObjectBody);

  Process = Thread->ThreadsProcess;
  Thread->ThreadsProcess = NULL;

  PsDeleteCidHandle(Thread->Cid.UniqueThread, PsThreadType);

  KeReleaseThread(ETHREAD_TO_KTHREAD(Thread));
  
  ObDereferenceObject(Process);
  
  DPRINT("PiDeleteThread() finished\n");
}


NTSTATUS
PsInitializeThread(PEPROCESS Process,
		   PETHREAD* ThreadPtr,
		   PHANDLE ThreadHandle,
		   ACCESS_MASK	DesiredAccess,
		   POBJECT_ATTRIBUTES ThreadAttributes,
		   BOOLEAN First)
{
   PETHREAD Thread;
   NTSTATUS Status;
   KIRQL oldIrql;

   if (Process == NULL)
     {
	Process = PsInitialSystemProcess;
     }

   /*
    * Reference process
    */
   ObReferenceObjectByPointer(Process,
                              PROCESS_CREATE_THREAD,
                              PsProcessType,
                              KernelMode);
   
   /*
    * Create and initialize thread
    */
   Status = ObCreateObject(UserMode,
			   PsThreadType,
			   ThreadAttributes,
			   UserMode,
			   NULL,
			   sizeof(ETHREAD),
			   0,
			   0,
			   (PVOID*)&Thread);
   if (!NT_SUCCESS(Status))
     {
        ObDereferenceObject (Process);
        return(Status);
     }

  /* create a client id handle */
  Status = PsCreateCidHandle(Thread, PsThreadType, &Thread->Cid.UniqueThread);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (Thread);
      ObDereferenceObject (Process);
      return Status;
    }
  Thread->ThreadsProcess = Process;
  Thread->Cid.UniqueProcess = (HANDLE)Thread->ThreadsProcess->UniqueProcessId;
  
  Status = ObInsertObject ((PVOID)Thread,
			   NULL,
			   DesiredAccess,
			   0,
			   NULL,
			   ThreadHandle);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject (Thread);
      ObDereferenceObject (Process);
      return Status;
    }

   DPRINT("Thread = %x\n",Thread);

   KeInitializeThread(&Process->Pcb, &Thread->Tcb, First);
   InitializeListHead(&Thread->TerminationPortList);
   KeInitializeSpinLock(&Thread->ActiveTimerListLock);
   InitializeListHead(&Thread->IrpList);
   Thread->DeadThread = FALSE;
   Thread->HasTerminated = FALSE;
   Thread->Tcb.Win32Thread = NULL;
   DPRINT("Thread->Cid.UniqueThread %d\n",Thread->Cid.UniqueThread);
   

   Thread->Tcb.BasePriority = (CHAR)Process->Pcb.BasePriority;
   Thread->Tcb.Priority = Thread->Tcb.BasePriority;

   /*
    * Local Procedure Call facility (LPC)
    */
   KeInitializeSemaphore  (& Thread->LpcReplySemaphore, 0, LONG_MAX);
   Thread->LpcReplyMessage = NULL;
   Thread->LpcReplyMessageId = 0; /* not valid */
   /* Thread->LpcReceiveMessageId = 0; */
   Thread->LpcExitThreadCalled = FALSE;
   Thread->LpcReceivedMsgIdValid = FALSE;

   KeAcquireSpinLock(&PiThreadLock, &oldIrql);
   InsertTailList(&Process->ThreadListHead,
		  &Thread->ThreadListEntry);
   KeReleaseSpinLock(&PiThreadLock, oldIrql);

   *ThreadPtr = Thread;

   return(STATUS_SUCCESS);
}


static NTSTATUS
PsCreateTeb(HANDLE ProcessHandle,
	    PTEB *TebPtr,
	    PETHREAD Thread,
	    PINITIAL_TEB InitialTeb)
{
   PEPROCESS Process;
   NTSTATUS Status;
   ULONG ByteCount;
   ULONG RegionSize;
   ULONG TebSize;
   PVOID TebBase;
   TEB Teb;

   TebSize = PAGE_SIZE;

   if (NULL == Thread->ThreadsProcess)
     {
       /* We'll be allocating a 64k block here and only use 4k of it, but this
          path should almost never be taken. Actually, I never saw it was taken,
          so maybe we should just ASSERT(NULL != Thread->ThreadsProcess) and
          move on */
       TebBase = NULL;
       Status = ZwAllocateVirtualMemory(ProcessHandle,
                                        &TebBase,
                                        0,
                                        &TebSize,
                                        MEM_RESERVE | MEM_COMMIT | MEM_TOP_DOWN,
                                        PAGE_READWRITE);
       if (! NT_SUCCESS(Status))
         {
           DPRINT1("Failed to allocate virtual memory for TEB\n");
           return Status;
         }
     }
   else
     {
       Process = Thread->ThreadsProcess;
       ExAcquireFastMutex(&Process->TebLock);
       if (NULL == Process->TebBlock ||
           Process->TebBlock == Process->TebLastAllocated)
         {
           Process->TebBlock = NULL;
           RegionSize = MM_VIRTMEM_GRANULARITY;
           Status = ZwAllocateVirtualMemory(ProcessHandle,
                                            &Process->TebBlock,
                                            0,
                                            &RegionSize,
                                            MEM_RESERVE | MEM_TOP_DOWN,
                                            PAGE_READWRITE);
           if (! NT_SUCCESS(Status))
             {
               ExReleaseFastMutex(&Process->TebLock);
               DPRINT1("Failed to reserve virtual memory for TEB\n");
               return Status;
             }
           Process->TebLastAllocated = (PVOID) ((char *) Process->TebBlock + RegionSize);
         }
       TebBase = (PVOID) ((char *) Process->TebLastAllocated - PAGE_SIZE);
       Status = ZwAllocateVirtualMemory(ProcessHandle,
                                        &TebBase,
                                        0,
                                        &TebSize,
                                        MEM_COMMIT,
                                        PAGE_READWRITE);
       if (! NT_SUCCESS(Status))
         {
           DPRINT1("Failed to commit virtual memory for TEB\n");
           return Status;
         }
       Process->TebLastAllocated = TebBase;
       ExReleaseFastMutex(&Process->TebLock);
     }

   DPRINT ("TebBase %p TebSize %lu\n", TebBase, TebSize);
   ASSERT(NULL != TebBase && PAGE_SIZE <= TebSize);

   RtlZeroMemory(&Teb, sizeof(TEB));
   /* set all pointers to and from the TEB */
   Teb.Tib.Self = TebBase;
   if (Thread->ThreadsProcess)
     {
        Teb.Peb = Thread->ThreadsProcess->Peb; /* No PEB yet!! */
     }
   DPRINT("Teb.Peb %x\n", Teb.Peb);
   
   /* store stack information from InitialTeb */
   if(InitialTeb != NULL)
   {
    /* fixed-size stack */
    if(InitialTeb->StackBase && InitialTeb->StackLimit)
    {
     Teb.Tib.StackBase = InitialTeb->StackBase;
     Teb.Tib.StackLimit = InitialTeb->StackLimit;
     Teb.DeallocationStack = InitialTeb->StackLimit;
    }
    /* expandable stack */
    else
    {
     Teb.Tib.StackBase = InitialTeb->StackCommit;
     Teb.Tib.StackLimit = InitialTeb->StackCommitMax;
     Teb.DeallocationStack = InitialTeb->StackReserved;
    }
   }

   /* more initialization */
   Teb.Cid.UniqueThread = Thread->Cid.UniqueThread;
   Teb.Cid.UniqueProcess = Thread->Cid.UniqueProcess;
   Teb.CurrentLocale = PsDefaultThreadLocaleId;

   /* Terminate the exception handler list */
   Teb.Tib.ExceptionList = (PVOID)-1;
   
   DPRINT("sizeof(TEB) %x\n", sizeof(TEB));
   
   /* write TEB data into teb page */
   Status = NtWriteVirtualMemory(ProcessHandle,
                                 TebBase,
                                 &Teb,
                                 sizeof(TEB),
                                 &ByteCount);

   if (!NT_SUCCESS(Status))
     {
        /* free TEB */
        DPRINT1 ("Writing TEB failed!\n");

        RegionSize = 0;
        NtFreeVirtualMemory(ProcessHandle,
                            TebBase,
                            &RegionSize,
                            MEM_RELEASE);

        return Status;
     }

   if (TebPtr != NULL)
     {
        *TebPtr = (PTEB)TebBase;
     }

   DPRINT("TEB allocated at %p\n", TebBase);

   return Status;
}


VOID STDCALL
LdrInitApcRundownRoutine(PKAPC Apc)
{
   ExFreePool(Apc);
}


VOID STDCALL
LdrInitApcKernelRoutine(PKAPC Apc,
			PKNORMAL_ROUTINE* NormalRoutine,
			PVOID* NormalContext,
			PVOID* SystemArgument1,
			PVOID* SystemArgument2)
{
  ExFreePool(Apc);
}


NTSTATUS STDCALL
NtCreateThread(OUT PHANDLE ThreadHandle,
	       IN ACCESS_MASK DesiredAccess,
	       IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
	       IN HANDLE ProcessHandle,
	       OUT PCLIENT_ID Client,
	       IN PCONTEXT ThreadContext,
	       IN PINITIAL_TEB InitialTeb,
	       IN BOOLEAN CreateSuspended)
{
  PEPROCESS Process;
  PETHREAD Thread;
  PTEB TebBase;
  NTSTATUS Status;
  PKAPC LdrInitApc;

  DPRINT("NtCreateThread(ThreadHandle %x, PCONTEXT %x)\n",
	 ThreadHandle,ThreadContext);

  Status = ObReferenceObjectByHandle(ProcessHandle,
                                     PROCESS_CREATE_THREAD,
                                     PsProcessType,
                                     UserMode,
                                     (PVOID*)&Process,
                                     NULL);
  if(!NT_SUCCESS(Status))
  {
    return(Status);
  }

  Status = PsInitializeThread(Process,
			      &Thread,
			      ThreadHandle,
			      DesiredAccess,
			      ObjectAttributes,
			      FALSE);

  ObDereferenceObject(Process);

  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Status = KiArchInitThreadWithContext(&Thread->Tcb, ThreadContext);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }

  Status = PsCreateTeb(ProcessHandle,
		       &TebBase,
		       Thread,
		       InitialTeb);
  if (!NT_SUCCESS(Status))
    {
      return(Status);
    }
  Thread->Tcb.Teb = TebBase;

  Thread->StartAddress = NULL;

  if (Client != NULL)
    {
      *Client = Thread->Cid;
    }

  /*
   * Maybe send a message to the process's debugger
   */
  DbgkCreateThread((PVOID)ThreadContext->Eip);

  /*
   * First, force the thread to be non-alertable for user-mode alerts.
   */
  Thread->Tcb.Alertable = FALSE;

  /*
   * If the thread is to be created suspended then queue an APC to
   * do the suspend before we run any userspace code.
   */
  if (CreateSuspended)
    {
      PsSuspendThread(Thread, NULL);
    }

  /*
   * Queue an APC to the thread that will execute the ntdll startup
   * routine.
   */
  LdrInitApc = ExAllocatePool(NonPagedPool, sizeof(KAPC));
  KeInitializeApc(LdrInitApc, &Thread->Tcb, OriginalApcEnvironment, LdrInitApcKernelRoutine,
		  LdrInitApcRundownRoutine, LdrpGetSystemDllEntryPoint(), 
		  UserMode, NULL);
  KeInsertQueueApc(LdrInitApc, NULL, NULL, IO_NO_INCREMENT);
  
  /* 
   * The thread is non-alertable, so the APC we added did not set UserApcPending to TRUE. 
   * We must do this manually. Do NOT attempt to set the Thread to Alertable before the call,
   * doing so is a blatant and erronous hack.
   */
   Thread->Tcb.ApcState.UserApcPending = TRUE;
   Thread->Tcb.Alerted[KernelMode] = TRUE;

  PsUnblockThread(Thread, NULL);

  return(STATUS_SUCCESS);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsCreateSystemThread(PHANDLE ThreadHandle,
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
   
   Status = PsInitializeThread(NULL,
			       &Thread,
			       ThreadHandle,
			       DesiredAccess,
			       ObjectAttributes,
			       FALSE);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   Thread->StartAddress = StartRoutine;
   Status = KiArchInitThread(&Thread->Tcb, StartRoutine, StartContext);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   if (ClientId != NULL)
     {
	*ClientId=Thread->Cid;
     }

   PsUnblockThread(Thread, NULL);
   
   return(STATUS_SUCCESS);
}


VOID STDCALL
PspRunCreateThreadNotifyRoutines(PETHREAD CurrentThread,
				 BOOLEAN Create)
{
  ULONG i;
  CLIENT_ID Cid = CurrentThread->Cid;

  for (i = 0; i < PiThreadNotifyRoutineCount; i++)
    {
      PiThreadNotifyRoutine[i](Cid.UniqueProcess, Cid.UniqueThread, Create);
    }
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsSetCreateThreadNotifyRoutine(IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine)
{
  if (PiThreadNotifyRoutineCount >= MAX_THREAD_NOTIFY_ROUTINE_COUNT)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }

  PiThreadNotifyRoutine[PiThreadNotifyRoutineCount] = NotifyRoutine;
  PiThreadNotifyRoutineCount++;

  return(STATUS_SUCCESS);
}

/* EOF */
