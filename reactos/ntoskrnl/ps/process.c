/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/process.c
 * PURPOSE:         Process managment
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

PEPROCESS EXPORTED PsInitialSystemProcess = NULL;

POBJECT_TYPE EXPORTED PsProcessType = NULL;

LIST_ENTRY PsActiveProcessHead;
FAST_MUTEX PspActiveProcessMutex;
static LARGE_INTEGER ShortPsLockDelay, PsLockTimeout;

static GENERIC_MAPPING PiProcessMapping = {STANDARD_RIGHTS_READ | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
					   STANDARD_RIGHTS_WRITE | PROCESS_CREATE_PROCESS | PROCESS_CREATE_THREAD |
                       PROCESS_VM_OPERATION | PROCESS_VM_WRITE | PROCESS_DUP_HANDLE |
                       PROCESS_TERMINATE | PROCESS_SET_QUOTA | PROCESS_SET_INFORMATION | PROCESS_SET_PORT,
					   STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
					   PROCESS_ALL_ACCESS};

static const INFORMATION_CLASS_INFO PsProcessInfoClass[] =
{
  ICI_SQ_SAME( sizeof(PROCESS_BASIC_INFORMATION),     sizeof(ULONG), ICIF_QUERY ),                     /* ProcessBasicInformation */
  ICI_SQ_SAME( sizeof(QUOTA_LIMITS),                  sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessQuotaLimits */
  ICI_SQ_SAME( sizeof(IO_COUNTERS),                   sizeof(ULONG), ICIF_QUERY ),                     /* ProcessIoCounters */
  ICI_SQ_SAME( sizeof(VM_COUNTERS),                   sizeof(ULONG), ICIF_QUERY ),                     /* ProcessVmCounters */
  ICI_SQ_SAME( sizeof(KERNEL_USER_TIMES),             sizeof(ULONG), ICIF_QUERY ),                     /* ProcessTimes */
  ICI_SQ_SAME( sizeof(KPRIORITY),                     sizeof(ULONG), ICIF_SET ),                       /* ProcessBasePriority */
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_SET ),                       /* ProcessRaisePriority */
  ICI_SQ_SAME( sizeof(HANDLE),                        sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessDebugPort */
  ICI_SQ_SAME( sizeof(HANDLE),                        sizeof(ULONG), ICIF_SET ),                       /* ProcessExceptionPort */
  ICI_SQ_SAME( sizeof(PROCESS_ACCESS_TOKEN),          sizeof(ULONG), ICIF_SET ),                       /* ProcessAccessToken */
  ICI_SQ_SAME( 0 /* FIXME */,                         sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessLdtInformation */
  ICI_SQ_SAME( 0 /* FIXME */,                         sizeof(ULONG), ICIF_SET ),                       /* ProcessLdtSize */
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessDefaultHardErrorMode */
  ICI_SQ_SAME( 0 /* FIXME */,                         sizeof(ULONG), ICIF_SET ),                       /* ProcessIoPortHandlers */
  ICI_SQ_SAME( sizeof(POOLED_USAGE_AND_LIMITS),       sizeof(ULONG), ICIF_QUERY ),                     /* ProcessPooledUsageAndLimits */
  ICI_SQ_SAME( sizeof(PROCESS_WS_WATCH_INFORMATION),  sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessWorkingSetWatch */
  ICI_SQ_SAME( 0 /* FIXME */,                         sizeof(ULONG), ICIF_SET ),                       /* ProcessUserModeIOPL */
  ICI_SQ_SAME( sizeof(BOOLEAN),                       sizeof(ULONG), ICIF_SET ),                       /* ProcessEnableAlignmentFaultFixup */
  ICI_SQ_SAME( sizeof(PROCESS_PRIORITY_CLASS),        sizeof(USHORT), ICIF_QUERY | ICIF_SET ),         /* ProcessPriorityClass */
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_QUERY ),                     /* ProcessWx86Information */
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_QUERY ),                     /* ProcessHandleCount */
  ICI_SQ_SAME( sizeof(KAFFINITY),                     sizeof(ULONG), ICIF_SET ),                       /* ProcessAffinityMask */
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessPriorityBoost */

  ICI_SQ(/*Q*/ sizeof(((PPROCESS_DEVICEMAP_INFORMATION)0x0)->Query),                                   /* ProcessDeviceMap */
         /*S*/ sizeof(((PPROCESS_DEVICEMAP_INFORMATION)0x0)->Set),
                                                /*Q*/ sizeof(ULONG),
                                                /*S*/ sizeof(ULONG),
                                                                     ICIF_QUERY | ICIF_SET ),

  ICI_SQ_SAME( sizeof(PROCESS_SESSION_INFORMATION),   sizeof(ULONG), ICIF_QUERY | ICIF_SET ),          /* ProcessSessionInformation */
  ICI_SQ_SAME( sizeof(BOOLEAN),                       sizeof(ULONG), ICIF_SET ),                       /* ProcessForegroundInformation */
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_QUERY ),                     /* ProcessWow64Information */
  ICI_SQ_SAME( sizeof(UNICODE_STRING),                sizeof(ULONG), ICIF_QUERY | ICIF_SIZE_VARIABLE), /* ProcessImageFileName */

  /* FIXME */
  ICI_SQ_SAME( 0,                                     0,             0 ),                              /* ProcessLUIDDeviceMapsEnabled */
  ICI_SQ_SAME( 0,                                     0,             0 ),                              /* ProcessBreakOnTermination */
  ICI_SQ_SAME( 0,                                     0,             0 ),                              /* ProcessDebugObjectHandle */
  ICI_SQ_SAME( 0,                                     0,             0 ),                              /* ProcessDebugFlags */
  ICI_SQ_SAME( 0,                                     0,             0 ),                              /* ProcessHandleTracing */
  ICI_SQ_SAME( 0,                                     0,             0 ),                              /* ProcessUnknown33 */
  ICI_SQ_SAME( 0,                                     0,             0 ),                              /* ProcessUnknown34 */
  ICI_SQ_SAME( 0,                                     0,             0 ),                              /* ProcessUnknown35 */
  
  ICI_SQ_SAME( sizeof(ULONG),                         sizeof(ULONG), ICIF_QUERY),                      /* ProcessCookie */
};

#define MAX_PROCESS_NOTIFY_ROUTINE_COUNT    8
#define MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT  8

static PCREATE_PROCESS_NOTIFY_ROUTINE
PiProcessNotifyRoutine[MAX_PROCESS_NOTIFY_ROUTINE_COUNT];
static PLOAD_IMAGE_NOTIFY_ROUTINE
PiLoadImageNotifyRoutine[MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT];


typedef struct
{
    WORK_QUEUE_ITEM WorkQueueItem;
    KEVENT Event;
    PEPROCESS Process;
    BOOLEAN IsWorkerQueue;
} DEL_CONTEXT, *PDEL_CONTEXT;

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
PsExitSpecialApc(PKAPC Apc, 
		 PKNORMAL_ROUTINE *NormalRoutine,
		 PVOID *NormalContext,
		 PVOID *SystemArgument1,
		 PVOID *SystemArgument2)
{
}

PEPROCESS
PsGetNextProcess(PEPROCESS OldProcess)
{
   PEPROCESS NextProcess;
   NTSTATUS Status;
   
   if (OldProcess == NULL)
     {
       Status = ObReferenceObjectByPointer(PsInitialSystemProcess,
				           PROCESS_ALL_ACCESS,
				           PsProcessType,
				           KernelMode);   
       if (!NT_SUCCESS(Status))
         {
	   CPRINT("PsGetNextProcess(): ObReferenceObjectByPointer failed for PsInitialSystemProcess\n");
	   KEBUGCHECK(0);
	 }
       return PsInitialSystemProcess;
     }
   
   ExAcquireFastMutex(&PspActiveProcessMutex);
   NextProcess = OldProcess;
   while (1)
     {
       if (NextProcess->ProcessListEntry.Blink == &PsActiveProcessHead)
         {
	   NextProcess = CONTAINING_RECORD(PsActiveProcessHead.Blink,
					   EPROCESS,
					   ProcessListEntry);
         }
       else
         {
	   NextProcess = CONTAINING_RECORD(NextProcess->ProcessListEntry.Blink,
					   EPROCESS,
					   ProcessListEntry);
         }
       Status = ObReferenceObjectByPointer(NextProcess,
				           PROCESS_ALL_ACCESS,
				           PsProcessType,
				           KernelMode);   
       if (NT_SUCCESS(Status))
         {
	   break;
	 }
       else if (Status == STATUS_PROCESS_IS_TERMINATING)
         {
	   continue;
	 }
       else if (!NT_SUCCESS(Status))
         {
	   CPRINT("PsGetNextProcess(): ObReferenceObjectByPointer failed\n");
	   KEBUGCHECK(0);
	 }
     }

   ExReleaseFastMutex(&PspActiveProcessMutex);
   ObDereferenceObject(OldProcess);
   
   return(NextProcess);
}


/*
 * @implemented
 */
NTSTATUS STDCALL 
NtOpenProcessToken(IN	HANDLE		ProcessHandle,
		   IN	ACCESS_MASK	DesiredAccess,
		   OUT	PHANDLE		TokenHandle)
{
  return NtOpenProcessTokenEx(ProcessHandle,
                              DesiredAccess,
                              0,
                              TokenHandle);
}


/*
 * @unimplemented
 */
NTSTATUS
STDCALL
NtOpenProcessTokenEx(
    IN HANDLE ProcessHandle,
    IN ACCESS_MASK DesiredAccess,
    IN ULONG HandleAttributes,
    OUT PHANDLE TokenHandle
    )
{
   PACCESS_TOKEN Token;
   HANDLE hToken;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PAGED_CODE();
   
   PreviousMode = ExGetPreviousMode();
   
   if(PreviousMode == UserMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(TokenHandle,
                     sizeof(HANDLE),
                     sizeof(ULONG));
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

   Status = PsOpenTokenOfProcess(ProcessHandle,
				 &Token);
   if(NT_SUCCESS(Status))
   {
     Status = ObCreateHandle(PsGetCurrentProcess(),
			     Token,
			     DesiredAccess,
			     FALSE,
			     &hToken);
     ObDereferenceObject(Token);

     _SEH_TRY
     {
       *TokenHandle = hToken;
     }
     _SEH_HANDLE
     {
       Status = _SEH_GetExceptionCode();
     }
     _SEH_END;
   }
   
   return Status;
}


/*
 * @implemented
 */
PACCESS_TOKEN STDCALL
PsReferencePrimaryToken(PEPROCESS Process)
{
   ObReferenceObjectByPointer(Process->Token,
			      TOKEN_ALL_ACCESS,
			      SepTokenObjectType,
			      KernelMode);
   return(Process->Token);
}


NTSTATUS
PsOpenTokenOfProcess(HANDLE ProcessHandle,
		     PACCESS_TOKEN* Token)
{
   PEPROCESS Process;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_QUERY_INFORMATION,
				      PsProcessType,
				      ExGetPreviousMode(),
				      (PVOID*)&Process,
				      NULL);
   if(NT_SUCCESS(Status))
   {
     *Token = PsReferencePrimaryToken(Process);
     ObDereferenceObject(Process);
   }
   
   return Status;
}


VOID 
PiKillMostProcesses(VOID)
{
   PLIST_ENTRY current_entry;
   PEPROCESS current;
   
   ExAcquireFastMutex(&PspActiveProcessMutex);
   
   current_entry = PsActiveProcessHead.Flink;
   while (current_entry != &PsActiveProcessHead)
     {
	current = CONTAINING_RECORD(current_entry, EPROCESS, 
				    ProcessListEntry);
	current_entry = current_entry->Flink;
	
	if (current->UniqueProcessId != PsInitialSystemProcess->UniqueProcessId &&
	    current->UniqueProcessId != PsGetCurrentProcessId())
	  {
	     PiTerminateProcessThreads(current, STATUS_SUCCESS);
	  }
     }
   
   ExReleaseFastMutex(&PspActiveProcessMutex);
}


VOID INIT_FUNCTION
PsInitProcessManagment(VOID)
{
   PKPROCESS KProcess;
   NTSTATUS Status;
   
   ShortPsLockDelay.QuadPart = -100LL;
   PsLockTimeout.QuadPart = -10000000LL; /* one second */
   /*
    * Register the process object type
    */
   
   PsProcessType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));

   PsProcessType->Tag = TAG('P', 'R', 'O', 'C');
   PsProcessType->TotalObjects = 0;
   PsProcessType->TotalHandles = 0;
   PsProcessType->PeakObjects = 0;
   PsProcessType->PeakHandles = 0;
   PsProcessType->PagedPoolCharge = 0;
   PsProcessType->NonpagedPoolCharge = sizeof(EPROCESS);
   PsProcessType->Mapping = &PiProcessMapping;
   PsProcessType->Dump = NULL;
   PsProcessType->Open = NULL;
   PsProcessType->Close = NULL;
   PsProcessType->Delete = PiDeleteProcess;
   PsProcessType->Parse = NULL;
   PsProcessType->Security = NULL;
   PsProcessType->QueryName = NULL;
   PsProcessType->OkayToClose = NULL;
   PsProcessType->Create = NULL;
   PsProcessType->DuplicationNotify = NULL;
   
   RtlInitUnicodeString(&PsProcessType->TypeName, L"Process");
   
   ObpCreateTypeObject(PsProcessType);

   InitializeListHead(&PsActiveProcessHead);
   ExInitializeFastMutex(&PspActiveProcessMutex);

   RtlZeroMemory(PiProcessNotifyRoutine, sizeof(PiProcessNotifyRoutine));
   RtlZeroMemory(PiLoadImageNotifyRoutine, sizeof(PiLoadImageNotifyRoutine));
   
   /*
    * Initialize the system process
    */
   Status = ObCreateObject(KernelMode,
			   PsProcessType,
			   NULL,
			   KernelMode,
			   NULL,
			   sizeof(EPROCESS),
			   0,
			   0,
			   (PVOID*)&PsInitialSystemProcess);
   if (!NT_SUCCESS(Status))
     {
	return;
     }
   
   /* System threads may run on any processor. */
   PsInitialSystemProcess->Pcb.Affinity = 0xFFFFFFFF;
   PsInitialSystemProcess->Pcb.IopmOffset = 0xffff;
   PsInitialSystemProcess->Pcb.LdtDescriptor[0] = 0;
   PsInitialSystemProcess->Pcb.LdtDescriptor[1] = 0;
   PsInitialSystemProcess->Pcb.BasePriority = PROCESS_PRIO_NORMAL;
   PsInitialSystemProcess->Pcb.ThreadQuantum = 6;
   InitializeListHead(&PsInitialSystemProcess->Pcb.ThreadListHead);
   KeInitializeDispatcherHeader(&PsInitialSystemProcess->Pcb.DispatcherHeader,
				InternalProcessType,
				sizeof(EPROCESS),
				FALSE);
   KProcess = &PsInitialSystemProcess->Pcb;
   
   MmInitializeAddressSpace(PsInitialSystemProcess,
			    &PsInitialSystemProcess->AddressSpace);
   ObCreateHandleTable(NULL,FALSE,PsInitialSystemProcess);
   
   KeInitializeEvent(&PsInitialSystemProcess->LockEvent, SynchronizationEvent, FALSE);
   PsInitialSystemProcess->LockCount = 0;
   PsInitialSystemProcess->LockOwner = NULL;

#if defined(__GNUC__)
   KProcess->DirectoryTableBase = 
     (LARGE_INTEGER)(LONGLONG)(ULONG)MmGetPageDirectory();
#else
   {
     LARGE_INTEGER dummy;
     dummy.QuadPart = (LONGLONG)(ULONG)MmGetPageDirectory();
     KProcess->DirectoryTableBase = dummy;
   }
#endif

   strcpy(PsInitialSystemProcess->ImageFileName, "System");
   
   Status = PsCreateCidHandle(PsInitialSystemProcess,
                              PsProcessType,
                              &PsInitialSystemProcess->UniqueProcessId);
   if(!NT_SUCCESS(Status))
   {
     DPRINT1("Failed to create CID handle (unique process id) for the system process!\n");
     return;
   }
   
   PsInitialSystemProcess->Win32WindowStation = (HANDLE)0;
   
   InsertHeadList(&PsActiveProcessHead,
		  &PsInitialSystemProcess->ProcessListEntry);
   InitializeListHead(&PsInitialSystemProcess->ThreadListHead);

   SepCreateSystemProcessToken(PsInitialSystemProcess);
}

VOID STDCALL
PiDeleteProcessWorker(PVOID pContext)
{
  PDEL_CONTEXT Context;
  PEPROCESS CurrentProcess;
  PEPROCESS Process;

  Context = (PDEL_CONTEXT)pContext;
  Process = Context->Process;
  CurrentProcess = PsGetCurrentProcess();

  DPRINT("PiDeleteProcess(ObjectBody %x)\n",Process);

  if (CurrentProcess != Process)
    {
      KeAttachProcess(&Process->Pcb);
    }

  ExAcquireFastMutex(&PspActiveProcessMutex);
  RemoveEntryList(&Process->ProcessListEntry);
  ExReleaseFastMutex(&PspActiveProcessMutex);

  /* KDB hook */
  KDB_DELETEPROCESS_HOOK(Process);

  ObDereferenceObject(Process->Token);
  ObDeleteHandleTable(Process);

  if (CurrentProcess != Process)
    {
      KeDetachProcess();
    }

  MmReleaseMmInfo(Process);
  if (Context->IsWorkerQueue)
    {
      KeSetEvent(&Context->Event, IO_NO_INCREMENT, FALSE);
    }
}

VOID STDCALL 
PiDeleteProcess(PVOID ObjectBody)
{
  DEL_CONTEXT Context;

  Context.Process = (PEPROCESS)ObjectBody;

  if (PsGetCurrentProcess() == Context.Process ||
      PsGetCurrentThread()->ThreadsProcess == Context.Process)
    {
       KEBUGCHECK(0);
    }

  if (PsGetCurrentThread()->ThreadsProcess == PsGetCurrentProcess())
    {
      Context.IsWorkerQueue = FALSE;
      PiDeleteProcessWorker(&Context);
    }
  else
    {
      Context.IsWorkerQueue = TRUE;
      KeInitializeEvent(&Context.Event, NotificationEvent, FALSE);
      ExInitializeWorkItem (&Context.WorkQueueItem, PiDeleteProcessWorker, &Context);
      ExQueueWorkItem(&Context.WorkQueueItem, HyperCriticalWorkQueue);
      if (KeReadStateEvent(&Context.Event) == 0)
        {
          KeWaitForSingleObject(&Context.Event, Executive, KernelMode, FALSE, NULL);
	}
    }

  if(((PEPROCESS)ObjectBody)->Win32Process != NULL)
  {
    /* delete the W32PROCESS structure if there's one associated */
    ExFreePool (((PEPROCESS)ObjectBody)->Win32Process);
  }
}

static NTSTATUS
PsCreatePeb(HANDLE ProcessHandle,
	    PEPROCESS Process,
	    PVOID ImageBase)
{
  ULONG AllocSize;
  ULONG PebSize;
  PPEB Peb;
  LARGE_INTEGER SectionOffset;
  ULONG ViewSize;
  PVOID TableBase;
  NTSTATUS Status;
  
  PAGED_CODE();

  /* Allocate the Process Environment Block (PEB) */
  Process->TebBlock = (PVOID) MM_ROUND_DOWN(PEB_BASE, MM_VIRTMEM_GRANULARITY);
  AllocSize = MM_VIRTMEM_GRANULARITY;
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   &Process->TebBlock,
				   0,
				   &AllocSize,
				   MEM_RESERVE,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtAllocateVirtualMemory() failed (Status %lx)\n", Status);
      return(Status);
    }
  ASSERT((ULONG_PTR) Process->TebBlock <= PEB_BASE &&
         PEB_BASE + PAGE_SIZE <= (ULONG_PTR) Process->TebBlock + AllocSize);
  Peb = (PPEB)PEB_BASE;
  PebSize = PAGE_SIZE;
  Status = NtAllocateVirtualMemory(ProcessHandle,
				   (PVOID*)&Peb,
				   0,
				   &PebSize,
				   MEM_COMMIT,
				   PAGE_READWRITE);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtAllocateVirtualMemory() failed (Status %lx)\n", Status);
      return(Status);
    }
  DPRINT("Peb %p  PebSize %lu\n", Peb, PebSize);
  ASSERT((PPEB) PEB_BASE == Peb && PAGE_SIZE <= PebSize);
  Process->TebLastAllocated = (PVOID) Peb;

  ViewSize = 0;
  SectionOffset.QuadPart = (ULONGLONG)0;
  TableBase = NULL;
  Status = MmMapViewOfSection(NlsSectionObject,
			      Process,
			      &TableBase,
			      0,
			      0,
			      &SectionOffset,
			      &ViewSize,
			      ViewShare,
			      MEM_TOP_DOWN,
			      PAGE_READONLY);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("MmMapViewOfSection() failed (Status %lx)\n", Status);
      return(Status);
    }
  DPRINT("TableBase %p  ViewSize %lx\n", TableBase, ViewSize);

  KeAttachProcess(&Process->Pcb);

  /* Initialize the PEB */
  RtlZeroMemory(Peb, sizeof(PEB));
  Peb->ImageBaseAddress = ImageBase;

  Peb->OSMajorVersion = 4;
  Peb->OSMinorVersion = 0;
  Peb->OSBuildNumber = 1381;
  Peb->OSPlatformId = 2; //VER_PLATFORM_WIN32_NT;
  Peb->OSCSDVersion = 6 << 8;

  Peb->AnsiCodePageData     = (char*)TableBase + NlsAnsiTableOffset;
  Peb->OemCodePageData      = (char*)TableBase + NlsOemTableOffset;
  Peb->UnicodeCaseTableData = (char*)TableBase + NlsUnicodeTableOffset;

  Process->Peb = Peb;
  KeDetachProcess();

  DPRINT("PsCreatePeb: Peb created at %p\n", Peb);

  return(STATUS_SUCCESS);
}


PKPROCESS
KeGetCurrentProcess(VOID)
/*
 * FUNCTION: Returns a pointer to the current process
 */
{
  return(&(PsGetCurrentProcess()->Pcb));
}

/*
 * Warning: Even though it returns HANDLE, it's not a real HANDLE but really a
 * ULONG ProcessId! (Skywing)
 */
/*
 * @implemented
 */
HANDLE STDCALL
PsGetCurrentProcessId(VOID)
{
  return((HANDLE)PsGetCurrentProcess()->UniqueProcessId);
}

/*
 * @unimplemented
 */
ULONG
STDCALL
PsGetCurrentProcessSessionId (
    	VOID
	)
{
	return PsGetCurrentProcess()->SessionId;
}

/*
 * FUNCTION: Returns a pointer to the current process
 *
 * @implemented
 */
PEPROCESS STDCALL
IoGetCurrentProcess(VOID)
{
   if (PsGetCurrentThread() == NULL || 
       PsGetCurrentThread()->Tcb.ApcState.Process == NULL)
     {
	return(PsInitialSystemProcess);
     }
   else
     {
	return(PEPROCESS)(PsGetCurrentThread()->Tcb.ApcState.Process);
     }
}

NTSTATUS
PspCreateProcess(OUT PHANDLE ProcessHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
		IN HANDLE ParentProcess  OPTIONAL,
		IN BOOLEAN InheritObjectTable,
		IN HANDLE SectionHandle  OPTIONAL,
		IN HANDLE DebugPort  OPTIONAL,
		IN HANDLE ExceptionPort  OPTIONAL)
{
   HANDLE hProcess;
   PEPROCESS Process;
   PEPROCESS pParentProcess;
   PKPROCESS KProcess;
   PVOID LdrStartupAddr;
   PVOID BaseAddress;
   PMEMORY_AREA MemoryArea;
   PHYSICAL_ADDRESS BoundaryAddressMultiple;
   KPROCESSOR_MODE PreviousMode;
   PVOID ImageBase = NULL;
   PEPORT pDebugPort = NULL;
   PEPORT pExceptionPort = NULL;
   PSECTION_OBJECT SectionObject = NULL;
   NTSTATUS Status = STATUS_SUCCESS;

   DPRINT("PspCreateProcess(ObjectAttributes %x)\n", ObjectAttributes);
   
   PreviousMode = ExGetPreviousMode();

   BoundaryAddressMultiple.QuadPart = 0;
   
   if(ParentProcess != NULL)
   {
     Status = ObReferenceObjectByHandle(ParentProcess,
				        PROCESS_CREATE_PROCESS,
				        PsProcessType,
				        PreviousMode,
				        (PVOID*)&pParentProcess,
				        NULL);
     if (!NT_SUCCESS(Status))
       {
          DPRINT1("Failed to reference the parent process: Status: 0x%x\n", Status);
	  return(Status);
       }
   }
   else
   {
     pParentProcess = NULL;
   }

   /*
    * Add the debug port
    */
   if (DebugPort != NULL)
     {
	Status = ObReferenceObjectByHandle(DebugPort,
					   PORT_ALL_ACCESS,
					   LpcPortObjectType,
					   PreviousMode,
					   (PVOID*)&pDebugPort,
					   NULL);
	if (!NT_SUCCESS(Status))
	  {
             DPRINT1("Failed to reference the debug port: Status: 0x%x\n", Status);
             goto exitdereferenceobjects;
	  }
     }

   /*
    * Add the exception port
    */
   if (ExceptionPort != NULL)
     {
	Status = ObReferenceObjectByHandle(ExceptionPort,
					   PORT_ALL_ACCESS,
					   LpcPortObjectType,
					   PreviousMode,
					   (PVOID*)&pExceptionPort,
					   NULL);
	if (!NT_SUCCESS(Status))
	  {
             DPRINT1("Failed to reference the exception port: Status: 0x%x\n", Status);
             goto exitdereferenceobjects;
	  }
     }

   if (SectionHandle != NULL)
     {
        Status = ObReferenceObjectByHandle(SectionHandle,
                                           0,
                                           MmSectionObjectType,
                                           PreviousMode,
                                           (PVOID*)&SectionObject,
                                           NULL);
	if (!NT_SUCCESS(Status))
	  {
	     DPRINT1("Failed to reference process image section: Status: 0x%x\n", Status);
             goto exitdereferenceobjects;
	  }
     }

   Status = ObCreateObject(PreviousMode,
			   PsProcessType,
			   ObjectAttributes,
			   PreviousMode,
			   NULL,
			   sizeof(EPROCESS),
			   0,
			   0,
			   (PVOID*)&Process);
   if (!NT_SUCCESS(Status))
     {
        DPRINT1("Failed to create process object, Status: 0x%x\n", Status);
        
exitdereferenceobjects:
        if(SectionObject != NULL)
          ObDereferenceObject(SectionObject);
        if(pExceptionPort != NULL)
          ObDereferenceObject(pExceptionPort);
        if(pDebugPort != NULL)
          ObDereferenceObject(pDebugPort);
        if(pParentProcess != NULL)
          ObDereferenceObject(pParentProcess);
	return Status;
     }

   KProcess = &Process->Pcb;
   
   RtlZeroMemory(Process, sizeof(EPROCESS));
   
   Status = PsCreateCidHandle(Process,
                              PsProcessType,
                              &Process->UniqueProcessId);
   if(!NT_SUCCESS(Status))
   {
     DPRINT1("Failed to create CID handle (unique process ID)! Status: 0x%x\n", Status);
     ObDereferenceObject(Process);
     goto exitdereferenceobjects;
   }

   Process->DebugPort = pDebugPort;
   Process->ExceptionPort = pExceptionPort;
   
   if(SectionObject != NULL)
   {
     UNICODE_STRING FileName;
     PWCHAR szSrc;
     PCHAR szDest;
     USHORT lnFName = 0;

     /*
      * Determine the image file name and save it to the EPROCESS structure
      */

     FileName = SectionObject->FileObject->FileName;
     szSrc = (PWCHAR)(FileName.Buffer + (FileName.Length / sizeof(WCHAR)) - 1);
     while(szSrc >= FileName.Buffer)
     {
       if(*szSrc == L'\\')
       {
         szSrc++;
         break;
       }
       else
       {
         szSrc--;
         lnFName++;
       }
     }

     /* copy the image file name to the process and truncate it to 15 characters
        if necessary */
     szDest = Process->ImageFileName;
     lnFName = min(lnFName, sizeof(Process->ImageFileName) - 1);
     while(lnFName-- > 0)
     {
       *(szDest++) = (UCHAR)*(szSrc++);
     }
     /* *szDest = '\0'; */
   }

   KeInitializeDispatcherHeader(&KProcess->DispatcherHeader,
				InternalProcessType,
				sizeof(EPROCESS),
				FALSE);

   /* Inherit parent process's affinity. */
   if(pParentProcess != NULL)
   {
     KProcess->Affinity = pParentProcess->Pcb.Affinity;
     Process->InheritedFromUniqueProcessId = pParentProcess->UniqueProcessId;
     Process->SessionId = pParentProcess->SessionId;
   }
   else
   {
     KProcess->Affinity = KeActiveProcessors;
   }
   
   KProcess->BasePriority = PROCESS_PRIO_NORMAL;
   KProcess->IopmOffset = 0xffff;
   KProcess->LdtDescriptor[0] = 0;
   KProcess->LdtDescriptor[1] = 0;
   InitializeListHead(&KProcess->ThreadListHead);
   KProcess->ThreadQuantum = 6;
   KProcess->AutoAlignment = 0;
   MmInitializeAddressSpace(Process,
			    &Process->AddressSpace);
   
   ObCreateHandleTable(pParentProcess,
		       InheritObjectTable,
		       Process);
   MmCopyMmInfo(pParentProcess ? pParentProcess : PsInitialSystemProcess, Process);
   
   KeInitializeEvent(&Process->LockEvent, SynchronizationEvent, FALSE);
   Process->LockCount = 0;
   Process->LockOwner = NULL;
   
   Process->Win32WindowStation = (HANDLE)0;

   ExAcquireFastMutex(&PspActiveProcessMutex);
   InsertHeadList(&PsActiveProcessHead, &Process->ProcessListEntry);
   InitializeListHead(&Process->ThreadListHead);
   ExReleaseFastMutex(&PspActiveProcessMutex);

   ExInitializeFastMutex(&Process->TebLock);
   Process->Pcb.State = PROCESS_STATE_ACTIVE;
   
   /*
    * Now we have created the process proper
    */

   MmLockAddressSpace(&Process->AddressSpace);

   /* Protect the highest 64KB of the process address space */
   BaseAddress = (PVOID)MmUserProbeAddress;
   Status = MmCreateMemoryArea(Process,
			       &Process->AddressSpace,
			       MEMORY_AREA_NO_ACCESS,
			       &BaseAddress,
			       0x10000,
			       PAGE_NOACCESS,
			       &MemoryArea,
			       FALSE,
			       FALSE,
			       BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
     {
	MmUnlockAddressSpace(&Process->AddressSpace);
	DPRINT1("Failed to protect the highest 64KB of the process address space\n");
	ObDereferenceObject(Process);
        goto exitdereferenceobjects;
     }

   /* Protect the lowest 64KB of the process address space */
#if 0
   BaseAddress = (PVOID)0x00000000;
   Status = MmCreateMemoryArea(Process,
			       &Process->AddressSpace,
			       MEMORY_AREA_NO_ACCESS,
			       &BaseAddress,
			       0x10000,
			       PAGE_NOACCESS,
			       &MemoryArea,
			       FALSE,
			       FALSE,
			       BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
     {
	MmUnlockAddressSpace(&Process->AddressSpace);
	DPRINT1("Failed to protect the lowest 64KB of the process address space\n");
	ObDereferenceObject(Process);
        goto exitdereferenceobjects;
     }
#endif

   /* Protect the 60KB above the shared user page */
   BaseAddress = (char*)USER_SHARED_DATA + PAGE_SIZE;
   Status = MmCreateMemoryArea(Process,
			       &Process->AddressSpace,
			       MEMORY_AREA_NO_ACCESS,
			       &BaseAddress,
			       0x10000 - PAGE_SIZE,
			       PAGE_NOACCESS,
			       &MemoryArea,
			       FALSE,
			       FALSE,
			       BoundaryAddressMultiple);
   if (!NT_SUCCESS(Status))
     {
	MmUnlockAddressSpace(&Process->AddressSpace);
	DPRINT1("Failed to protect the memory above the shared user page\n");
	ObDereferenceObject(Process);
        goto exitdereferenceobjects;
     }

   /* Create the shared data page */
   BaseAddress = (PVOID)USER_SHARED_DATA;
   Status = MmCreateMemoryArea(Process,
			       &Process->AddressSpace,
			       MEMORY_AREA_SHARED_DATA,
			       &BaseAddress,
			       PAGE_SIZE,
			       PAGE_READONLY,
			       &MemoryArea,
			       FALSE,
			       FALSE,
			       BoundaryAddressMultiple);
   MmUnlockAddressSpace(&Process->AddressSpace);
   if (!NT_SUCCESS(Status))
     {
        DPRINT1("Failed to create shared data page\n");
	ObDereferenceObject(Process);
        goto exitdereferenceobjects;
     }

#if 1
   /*
    * FIXME - the handle should be created after all things are initialized, NOT HERE!
    */
   Status = ObInsertObject ((PVOID)Process,
			    NULL,
			    DesiredAccess,
			    0,
			    NULL,
			    &hProcess);
   if (!NT_SUCCESS(Status))
     {
        DPRINT1("Failed to create a handle for the process\n");
	ObDereferenceObject(Process);
        goto exitdereferenceobjects;
     }
#endif

   /*
    * FIXME - Map ntdll
    */
   Status = LdrpMapSystemDll(hProcess, /* FIXME - hProcess shouldn't be available at this point! */
			     &LdrStartupAddr);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("LdrpMapSystemDll failed (Status %x)\n", Status);
	ObDereferenceObject(Process);
        goto exitdereferenceobjects;
     }
   
   /*
    * Map the process image
    */
   if (SectionObject != NULL)
     {
        ULONG ViewSize = 0;
        DPRINT("Mapping process image\n");
	Status = MmMapViewOfSection(SectionObject,
                                    Process,
                                    (PVOID*)&ImageBase,
                                    0,
                                    ViewSize,
                                    NULL,
                                    &ViewSize,
                                    0,
                                    MEM_COMMIT,
                                    PAGE_READWRITE);
        ObDereferenceObject(SectionObject);                            
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Failed to map the process section (Status %x)\n", Status);
	     ObDereferenceObject(Process);
             goto exitdereferenceobjects;
	  }
     }

  if(pParentProcess != NULL)
  {
    /*
     * Duplicate the token
     */
    Status = SepInitializeNewProcess(Process, pParentProcess);
    if (!NT_SUCCESS(Status))
      {
         DbgPrint("SepInitializeNewProcess failed (Status %x)\n", Status);
         ObDereferenceObject(Process);
         goto exitdereferenceobjects;
      }
  }
  else
  {
    /* FIXME */
  }

   /*
    * FIXME - Create PEB
    */
   DPRINT("Creating PEB\n");
   Status = PsCreatePeb(hProcess, /* FIXME - hProcess shouldn't be available at this point! */
			Process,
			ImageBase);
   if (!NT_SUCCESS(Status))
     {
        DbgPrint("NtCreateProcess() Peb creation failed: Status %x\n",Status);
	ObDereferenceObject(Process);
	goto exitdereferenceobjects;
     }
   
   /*
    * Maybe send a message to the creator process's debugger
    */
#if 0
   if (pParentProcess->DebugPort != NULL)
     {
	LPC_DBG_MESSAGE Message;
	HANDLE FileHandle;
	
	ObCreateHandle(NULL, // Debugger Process
		       NULL, // SectionHandle
		       FILE_ALL_ACCESS,
		       FALSE,
		       &FileHandle);
	
	Message.Header.MessageSize = sizeof(LPC_DBG_MESSAGE);
	Message.Header.DataSize = sizeof(LPC_DBG_MESSAGE) -
	  sizeof(LPC_MESSAGE);
	Message.Type = DBG_EVENT_CREATE_PROCESS;
	Message.Data.CreateProcess.FileHandle = FileHandle;
	Message.Data.CreateProcess.Base = ImageBase;
	Message.Data.CreateProcess.EntryPoint = NULL; //
	
	Status = LpcSendDebugMessagePort(pParentProcess->DebugPort,
					 &Message);
     }
#endif

   PspRunCreateProcessNotifyRoutines(Process, TRUE);
   
   /*
    * FIXME - the handle should be created not before this point!
    */
#if 0
   Status = ObInsertObject ((PVOID)Process,
			    NULL,
			    DesiredAccess,
			    0,
			    NULL,
			    &hProcess);
#endif
   if (NT_SUCCESS(Status))
     {
       _SEH_TRY
       {
         *ProcessHandle = hProcess;
       }
       _SEH_HANDLE
       {
         Status = _SEH_GetExceptionCode();
       }
       _SEH_END;
     }
   
   /*
    * don't dereference the debug port, exception port and section object even
    * if ObInsertObject() failed, the process is alive! We just couldn't return
    * the handle to the caller!
    */
    
   ObDereferenceObject(Process);
   if(pParentProcess != NULL)
     ObDereferenceObject(pParentProcess);

   return Status;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsCreateSystemProcess(PHANDLE ProcessHandle,
		      ACCESS_MASK DesiredAccess,
		      POBJECT_ATTRIBUTES ObjectAttributes)
{
   return PspCreateProcess(ProcessHandle,
			   DesiredAccess,
			   ObjectAttributes,
			   NULL, /* no parent process */
			   FALSE,
			   NULL,
			   NULL,
			   NULL);
}


/*
 * @implemented
 */
NTSTATUS STDCALL
NtCreateProcess(OUT PHANDLE ProcessHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
		IN HANDLE ParentProcess,
		IN BOOLEAN InheritObjectTable,
		IN HANDLE SectionHandle  OPTIONAL,
		IN HANDLE DebugPort  OPTIONAL,
		IN HANDLE ExceptionPort  OPTIONAL)
/*
 * FUNCTION: Creates a process.
 * ARGUMENTS:
 *        ProcessHandle (OUT) = Caller supplied storage for the resulting
 *                              handle
 *        DesiredAccess = Specifies the allowed or desired access to the
 *                        process can be a combination of
 *                        STANDARD_RIGHTS_REQUIRED| ..
 *        ObjectAttribute = Initialized attributes for the object, contains
 *                          the rootdirectory and the filename
 *        ParentProcess = Handle to the parent process.
 *        InheritObjectTable = Specifies to inherit the objects of the parent
 *                             process if true.
 *        SectionHandle = Handle to a section object to back the image file
 *        DebugPort = Handle to a DebugPort if NULL the system default debug
 *                    port will be used.
 *        ExceptionPort = Handle to a exception port.
 * REMARKS:
 *        This function maps to the win32 CreateProcess.
 * RETURNS: Status
 */
{
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PAGED_CODE();
  
   PreviousMode = ExGetPreviousMode();
   
   if(PreviousMode != KernelMode)
   {
     _SEH_TRY
     {
       ProbeForWrite(ProcessHandle,
                     sizeof(HANDLE),
                     sizeof(ULONG));
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
   
   if(ParentProcess == NULL)
   {
     Status = STATUS_INVALID_PARAMETER;
   }
   else
   {
     Status = PspCreateProcess(ProcessHandle,
                               DesiredAccess,
                               ObjectAttributes,
                               ParentProcess,
                               InheritObjectTable,
                               SectionHandle,
                               DebugPort,
                               ExceptionPort);
   }
   
   return Status;
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtOpenProcess(OUT PHANDLE	    ProcessHandle,
	      IN  ACCESS_MASK	    DesiredAccess,
	      IN  POBJECT_ATTRIBUTES  ObjectAttributes,
	      IN  PCLIENT_ID	    ClientId)
{
   DPRINT("NtOpenProcess(ProcessHandle %x, DesiredAccess %x, "
	  "ObjectAttributes %x, ClientId %x { UniP %d, UniT %d })\n",
	  ProcessHandle, DesiredAccess, ObjectAttributes, ClientId,
	  ClientId->UniqueProcess, ClientId->UniqueThread);

   PAGED_CODE();
   
   /*
    * Not sure of the exact semantics 
    */
   if (ObjectAttributes != NULL && ObjectAttributes->ObjectName != NULL &&
       ObjectAttributes->ObjectName->Buffer != NULL)
     {
	NTSTATUS Status;
	PEPROCESS Process;
		
	Status = ObReferenceObjectByName(ObjectAttributes->ObjectName,
					 ObjectAttributes->Attributes,
					 NULL,
					 DesiredAccess,
					 PsProcessType,
					 UserMode,
					 NULL,
					 (PVOID*)&Process);
	if (Status != STATUS_SUCCESS)
	  {
	     return(Status);
	  }
	
	Status = ObCreateHandle(PsGetCurrentProcess(),
				Process,
				DesiredAccess,
				FALSE,
				ProcessHandle);
	ObDereferenceObject(Process);
   
	return(Status);
     }
   else
     {
	PLIST_ENTRY current_entry;
	PEPROCESS current;
	NTSTATUS Status;
	
	ExAcquireFastMutex(&PspActiveProcessMutex);
	current_entry = PsActiveProcessHead.Flink;
	while (current_entry != &PsActiveProcessHead)
	  {
	     current = CONTAINING_RECORD(current_entry, EPROCESS, 
					 ProcessListEntry);
	     if (current->UniqueProcessId == ClientId->UniqueProcess)
	       {
	          if (current->Pcb.State == PROCESS_STATE_TERMINATED)
		    {
		      Status = STATUS_PROCESS_IS_TERMINATING;
		    }
		  else
		    {
		      Status = ObReferenceObjectByPointer(current,
					                  DesiredAccess,
					                  PsProcessType,
					                  UserMode);
		    }
		  ExReleaseFastMutex(&PspActiveProcessMutex);
		  if (NT_SUCCESS(Status))
		    {
		      Status = ObCreateHandle(PsGetCurrentProcess(),
					      current,
					      DesiredAccess,
					      FALSE,
					      ProcessHandle);
		      ObDereferenceObject(current);
		      DPRINT("*ProcessHandle %x\n", ProcessHandle);
		      DPRINT("NtOpenProcess() = %x\n", Status);
		    }
		  return(Status);
	       }
	     current_entry = current_entry->Flink;
	  }
	ExReleaseFastMutex(&PspActiveProcessMutex);
	DPRINT("NtOpenProcess() = STATUS_UNSUCCESSFUL\n");
	return(STATUS_UNSUCCESSFUL);
     }
   return(STATUS_UNSUCCESSFUL);
}


/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtQueryInformationProcess(IN  HANDLE ProcessHandle,
			  IN  PROCESSINFOCLASS ProcessInformationClass,
			  OUT PVOID ProcessInformation,
			  IN  ULONG ProcessInformationLength,
			  OUT PULONG ReturnLength  OPTIONAL)
{
   PEPROCESS Process;
   KPROCESSOR_MODE PreviousMode;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PAGED_CODE();
   
   PreviousMode = ExGetPreviousMode();
   
   DefaultQueryInfoBufferCheck(ProcessInformationClass,
                               PsProcessInfoClass,
                               ProcessInformation,
                               ProcessInformationLength,
                               ReturnLength,
                               PreviousMode,
                               &Status);
   if(!NT_SUCCESS(Status))
   {
     DPRINT1("NtQueryInformationProcess() failed, Status: 0x%x\n", Status);
     return Status;
   }
   
   if(ProcessInformationClass != ProcessCookie)
   {
     Status = ObReferenceObjectByHandle(ProcessHandle,
  				      PROCESS_QUERY_INFORMATION,
  				      PsProcessType,
  				      PreviousMode,
  				      (PVOID*)&Process,
  				      NULL);
     if (!NT_SUCCESS(Status))
       {
  	return(Status);
       }
   }
   else if(ProcessHandle != NtCurrentProcess())
   {
     /* retreiving the process cookie is only allowed for the calling process
        itself! XP only allowes NtCurrentProcess() as process handles even if a
        real handle actually represents the current process. */
     return STATUS_INVALID_PARAMETER;
   }
   
   switch (ProcessInformationClass)
     {
      case ProcessBasicInformation:
      {
        PPROCESS_BASIC_INFORMATION ProcessBasicInformationP =
	  (PPROCESS_BASIC_INFORMATION)ProcessInformation;

        _SEH_TRY
        {
	  ProcessBasicInformationP->ExitStatus = Process->ExitStatus;
	  ProcessBasicInformationP->PebBaseAddress = Process->Peb;
	  ProcessBasicInformationP->AffinityMask = Process->Pcb.Affinity;
	  ProcessBasicInformationP->UniqueProcessId =
	    Process->UniqueProcessId;
	  ProcessBasicInformationP->InheritedFromUniqueProcessId =
	    Process->InheritedFromUniqueProcessId;
	  ProcessBasicInformationP->BasePriority =
	    Process->Pcb.BasePriority;

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(PROCESS_BASIC_INFORMATION);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
	break;
      }

      case ProcessQuotaLimits:
      case ProcessIoCounters:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessTimes:
      {
         PKERNEL_USER_TIMES ProcessTimeP = (PKERNEL_USER_TIMES)ProcessInformation;
         _SEH_TRY
         {
	    ProcessTimeP->CreateTime = Process->CreateTime;
            ProcessTimeP->UserTime.QuadPart = Process->Pcb.UserTime * 100000LL;
            ProcessTimeP->KernelTime.QuadPart = Process->Pcb.KernelTime * 100000LL;
	    ProcessTimeP->ExitTime = Process->ExitTime;

	   if (ReturnLength)
	   {
	     *ReturnLength = sizeof(KERNEL_USER_TIMES);
	   }
         }
         _SEH_HANDLE
         {
           Status = _SEH_GetExceptionCode();
         }
         _SEH_END;
	 break;
      }

      case ProcessDebugPort:
      {
        _SEH_TRY
        {
          *(PHANDLE)ProcessInformation = (Process->DebugPort != NULL ? (HANDLE)-1 : NULL);
	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(HANDLE);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        break;
      }
      
      case ProcessLdtInformation:
      case ProcessWorkingSetWatch:
      case ProcessWx86Information:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessHandleCount:
      {
	ULONG HandleCount = ObpGetHandleCountByHandleTable(&Process->HandleTable);
	  
	_SEH_TRY
	{
          *(PULONG)ProcessInformation = HandleCount;
	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(ULONG);
	  }
	}
	_SEH_HANDLE
	{
          Status = _SEH_GetExceptionCode();
	}
	_SEH_END;
	break;
      }

      case ProcessSessionInformation:
      {
        PPROCESS_SESSION_INFORMATION SessionInfo = (PPROCESS_SESSION_INFORMATION)ProcessInformation;

        _SEH_TRY
        {
          SessionInfo->SessionId = Process->SessionId;
          if (ReturnLength)
          {
            *ReturnLength = sizeof(PROCESS_SESSION_INFORMATION);
          }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        break;
      }
      
      case ProcessWow64Information:
        DPRINT1("We currently don't support the ProcessWow64Information information class!\n");
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessVmCounters:
      {
	PVM_COUNTERS pOut = (PVM_COUNTERS)ProcessInformation;
	  
	_SEH_TRY
	{
	  pOut->PeakVirtualSize            = Process->PeakVirtualSize;
	  /*
	   * Here we should probably use VirtualSize.LowPart, but due to
	   * incompatibilities in current headers (no unnamed union),
	   * I opted for cast.
	   */
	  pOut->VirtualSize                = (ULONG)Process->VirtualSize.QuadPart;
	  pOut->PageFaultCount             = Process->Vm.PageFaultCount;
	  pOut->PeakWorkingSetSize         = Process->Vm.PeakWorkingSetSize;
	  pOut->WorkingSetSize             = Process->Vm.WorkingSetSize;
	  pOut->QuotaPeakPagedPoolUsage    = Process->QuotaPeakPoolUsage[0]; // TODO: Verify!
	  pOut->QuotaPagedPoolUsage        = Process->QuotaPoolUsage[0];     // TODO: Verify!
	  pOut->QuotaPeakNonPagedPoolUsage = Process->QuotaPeakPoolUsage[1]; // TODO: Verify!
	  pOut->QuotaNonPagedPoolUsage     = Process->QuotaPoolUsage[1];     // TODO: Verify!
	  pOut->PagefileUsage              = Process->PagefileUsage;
	  pOut->PeakPagefileUsage          = Process->PeakPagefileUsage;

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(VM_COUNTERS);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
	break;
      }

      case ProcessDefaultHardErrorMode:
      {
	PULONG HardErrMode = (PULONG)ProcessInformation;
	_SEH_TRY
	{
	  *HardErrMode = Process->DefaultHardErrorProcessing;
	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(ULONG);
	  }
	}
	_SEH_HANDLE
	{
          Status = _SEH_GetExceptionCode();
	}
	_SEH_END;
	break;
      }

      case ProcessPriorityBoost:
      {
	PULONG BoostEnabled = (PULONG)ProcessInformation;
	  
	_SEH_TRY
	{
	  *BoostEnabled = Process->Pcb.DisableBoost ? FALSE : TRUE;

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(ULONG);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
	break;
      }

      case ProcessDeviceMap:
      {
        PROCESS_DEVICEMAP_INFORMATION DeviceMap;
          
        ObQueryDeviceMapInformation(Process, &DeviceMap);

        _SEH_TRY
        {
          *(PPROCESS_DEVICEMAP_INFORMATION)ProcessInformation = DeviceMap;
	  if (ReturnLength)
          {
	    *ReturnLength = sizeof(PROCESS_DEVICEMAP_INFORMATION);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
	break;
      }

      case ProcessPriorityClass:
      {
	PUSHORT Priority = (PUSHORT)ProcessInformation;

	_SEH_TRY
	{
	  *Priority = Process->PriorityClass;

	  if (ReturnLength)
	  {
	    *ReturnLength = sizeof(USHORT);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
	break;
      }

      case ProcessImageFileName:
      {
        /*
         * We DO NOT return the file name stored in the EPROCESS structure.
         * Propably if we can't find a PEB or ProcessParameters structure for the
         * process!
         */
        if(Process->Peb != NULL)
        {
          PRTL_USER_PROCESS_PARAMETERS ProcParams = NULL;
          UNICODE_STRING LocalDest;
          BOOLEAN Attached;
          ULONG ImagePathLen = 0;
          PUNICODE_STRING DstPath = (PUNICODE_STRING)ProcessInformation;

          /* we need to attach to the process to make sure we're in the right context! */
          Attached = Process != PsGetCurrentProcess();

          if(Attached)
            KeAttachProcess(&Process->Pcb);
          
          _SEH_TRY
          {
            ProcParams = Process->Peb->ProcessParameters;
            ImagePathLen = ProcParams->ImagePathName.Length;
          }
          _SEH_HANDLE
          {
            Status = _SEH_GetExceptionCode();
          }
          _SEH_END;
          
          if(NT_SUCCESS(Status))
          {
            if(ProcessInformationLength < sizeof(UNICODE_STRING) + ImagePathLen + sizeof(WCHAR))
            {
              Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
              PWSTR StrSource = NULL;

              /* create a DstPath structure on the stack */
              _SEH_TRY
              {
                LocalDest.Length = ImagePathLen;
                LocalDest.MaximumLength = ImagePathLen + sizeof(WCHAR);
                LocalDest.Buffer = (PWSTR)(DstPath + 1);

                /* save a copy of the pointer to the source buffer */
                StrSource = ProcParams->ImagePathName.Buffer;
              }
              _SEH_HANDLE
              {
                Status = _SEH_GetExceptionCode();
              }
              _SEH_END;

              if(NT_SUCCESS(Status))
              {
                /* now, let's allocate some anonymous memory to copy the string to.
                   we can't just copy it to the buffer the caller pointed as it might
                   be user memory in another context */
                PWSTR PathCopy = ExAllocatePool(PagedPool, LocalDest.Length + sizeof(WCHAR));
                if(PathCopy != NULL)
                {
                  /* make a copy of the buffer to the temporary buffer */
                  _SEH_TRY
                  {
                    RtlCopyMemory(PathCopy, StrSource, LocalDest.Length);
                    PathCopy[LocalDest.Length / sizeof(WCHAR)] = L'\0';
                  }
                  _SEH_HANDLE
                  {
                    Status = _SEH_GetExceptionCode();
                  }
                  _SEH_END;

                  /* detach from the process */
                  if(Attached)
                    KeDetachProcess();

                  /* only copy the string back to the caller if we were able to
                     copy it into the temporary buffer! */
                  if(NT_SUCCESS(Status))
                  {
                    /* now let's copy the buffer back to the caller */
                    _SEH_TRY
                    {
                      *DstPath = LocalDest;
                      RtlCopyMemory(LocalDest.Buffer, PathCopy, LocalDest.Length + sizeof(WCHAR));
                      if (ReturnLength)
                      {
                        *ReturnLength = sizeof(UNICODE_STRING) + LocalDest.Length + sizeof(WCHAR);
                      }
                    }
                    _SEH_HANDLE
                    {
                      Status = _SEH_GetExceptionCode();
                    }
                    _SEH_END;
                  }

                  /* we're done with the copy operation, free the temporary kernel buffer */
                  ExFreePool(PathCopy);

                  /* we need to bail because we're already detached from the process */
                  break;
                }
                else
                {
                  Status = STATUS_INSUFFICIENT_RESOURCES;
                }
              }
            }
          }
          
          /* don't forget to detach from the process!!! */
          if(Attached)
            KeDetachProcess();
        }
        else
        {
          /* FIXME - what to do here? */
          Status = STATUS_UNSUCCESSFUL;
        }
        break;
      }
      
      case ProcessCookie:
      {
        ULONG Cookie;
        
        /* receive the process cookie, this is only allowed for the current
           process! */

        Process = PsGetCurrentProcess();

        Cookie = Process->Cookie;
        if(Cookie == 0)
        {
          LARGE_INTEGER SystemTime;
          ULONG NewCookie;
          PKPRCB Prcb;
          
          /* generate a new cookie */
          
          KeQuerySystemTime(&SystemTime);
          
          Prcb = &KeGetCurrentKPCR()->PrcbData;

          NewCookie = Prcb->KeSystemCalls ^ Prcb->InterruptTime ^
                      SystemTime.u.LowPart ^ SystemTime.u.HighPart;
          
          /* try to set the new cookie, return the current one if another thread
             set it in the meanwhile */
          Cookie = InterlockedCompareExchange((LONG*)&Process->Cookie,
                                              NewCookie,
                                              Cookie);
          if(Cookie == 0)
          {
            /* successfully set the cookie */
            Cookie = NewCookie;
          }
        }
        
        _SEH_TRY
        {
          *(PULONG)ProcessInformation = Cookie;
	  if (ReturnLength)
          {
	    *ReturnLength = sizeof(ULONG);
	  }
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        break;
      }

      /*
       * Note: The following 10 information classes are verified to not be
       * implemented on NT, and do indeed return STATUS_INVALID_INFO_CLASS;
       */
      case ProcessBasePriority:
      case ProcessRaisePriority:
      case ProcessExceptionPort:
      case ProcessAccessToken:
      case ProcessLdtSize:
      case ProcessIoPortHandlers:
      case ProcessUserModeIOPL:
      case ProcessEnableAlignmentFaultFixup:
      case ProcessAffinityMask:
      case ProcessForegroundInformation:
      default:
	Status = STATUS_INVALID_INFO_CLASS;
     }

   if(ProcessInformationClass != ProcessCookie)
   {
     ObDereferenceObject(Process);
   }
   
   return Status;
}


NTSTATUS
PspAssignPrimaryToken(PEPROCESS Process,
		      HANDLE TokenHandle)
{
   PACCESS_TOKEN Token;
   PACCESS_TOKEN OldToken;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(TokenHandle,
				      0,
				      SepTokenObjectType,
				      UserMode,
				      (PVOID*)&Token,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   Status = SeExchangePrimaryToken(Process, Token, &OldToken);
   if (NT_SUCCESS(Status))
     {
	ObDereferenceObject(OldToken);
     }
   ObDereferenceObject(Token);
   return(Status);
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL
NtSetInformationProcess(IN HANDLE ProcessHandle,
			IN PROCESSINFOCLASS ProcessInformationClass,
			IN PVOID ProcessInformation,
			IN ULONG ProcessInformationLength)
{
   PEPROCESS Process;
   KPROCESSOR_MODE PreviousMode;
   ACCESS_MASK Access;
   NTSTATUS Status = STATUS_SUCCESS;
   
   PAGED_CODE();
   
   PreviousMode = ExGetPreviousMode();

   DefaultSetInfoBufferCheck(ProcessInformationClass,
                             PsProcessInfoClass,
                             ProcessInformation,
                             ProcessInformationLength,
                             PreviousMode,
                             &Status);
   if(!NT_SUCCESS(Status))
   {
     DPRINT1("NtSetInformationProcess() %d %x  %x called\n", ProcessInformationClass, ProcessInformation, ProcessInformationLength);
     DPRINT1("NtSetInformationProcess() %x failed, Status: 0x%x\n", Status);
     return Status;
   }
   
   switch(ProcessInformationClass)
   {
     case ProcessSessionInformation:
       Access = PROCESS_SET_INFORMATION | PROCESS_SET_SESSIONID;
       break;
     case ProcessExceptionPort:
     case ProcessDebugPort:
       Access = PROCESS_SET_INFORMATION | PROCESS_SET_PORT;
       break;

     default:
       Access = PROCESS_SET_INFORMATION;
       break;
   }
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      Access,
				      PsProcessType,
				      PreviousMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }

   switch (ProcessInformationClass)
     {
      case ProcessQuotaLimits:
      case ProcessBasePriority:
      case ProcessRaisePriority:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessDebugPort:
      {
        HANDLE PortHandle = NULL;

        /* make a safe copy of the buffer on the stack */
        _SEH_TRY
        {
          PortHandle = *(PHANDLE)ProcessInformation;
          Status = (PortHandle != NULL ? STATUS_SUCCESS : STATUS_INVALID_PARAMETER);
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(NT_SUCCESS(Status))
        {
          PEPORT DebugPort;

          /* in case we had success reading from the buffer, verify the provided
           * LPC port handle
           */
          Status = ObReferenceObjectByHandle(PortHandle,
                                             0,
                                             LpcPortObjectType,
                                             PreviousMode,
                                             (PVOID)&DebugPort,
                                             NULL);
          if(NT_SUCCESS(Status))
          {
            /* lock the process to be thread-safe! */

            Status = PsLockProcess(Process, FALSE);
            if(NT_SUCCESS(Status))
            {
              /*
               * according to "NT Native API" documentation, setting the debug
               * port is only permitted once!
               */
              if(Process->DebugPort == NULL)
              {
                /* keep the reference to the handle! */
                Process->DebugPort = DebugPort;
                
                if(Process->Peb)
                {
                  /* we're now debugging the process, so set the flag in the PEB
                     structure. However, to access it we need to attach to the
                     process so we're sure we're in the right context! */

                  KeAttachProcess(&Process->Pcb);
                  _SEH_TRY
                  {
                    Process->Peb->BeingDebugged = TRUE;
                  }
                  _SEH_HANDLE
                  {
                    DPRINT1("Trying to set the Peb->BeingDebugged field of process 0x%x failed, exception: 0x%x\n", Process, _SEH_GetExceptionCode());
                  }
                  _SEH_END;
                  KeDetachProcess();
                }
                Status = STATUS_SUCCESS;
              }
              else
              {
                ObDereferenceObject(DebugPort);
                Status = STATUS_PORT_ALREADY_SET;
              }
              PsUnlockProcess(Process);
            }
            else
            {
              ObDereferenceObject(DebugPort);
            }
          }
        }
        break;
      }

      case ProcessExceptionPort:
      {
        HANDLE PortHandle = NULL;

        /* make a safe copy of the buffer on the stack */
        _SEH_TRY
        {
          PortHandle = *(PHANDLE)ProcessInformation;
          Status = STATUS_SUCCESS;
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        if(NT_SUCCESS(Status))
        {
          PEPORT ExceptionPort;
          
          /* in case we had success reading from the buffer, verify the provided
           * LPC port handle
           */
          Status = ObReferenceObjectByHandle(PortHandle,
                                             0,
                                             LpcPortObjectType,
                                             PreviousMode,
                                             (PVOID)&ExceptionPort,
                                             NULL);
          if(NT_SUCCESS(Status))
          {
            /* lock the process to be thread-safe! */
            
            Status = PsLockProcess(Process, FALSE);
            if(NT_SUCCESS(Status))
            {
              /*
               * according to "NT Native API" documentation, setting the exception
               * port is only permitted once!
               */
              if(Process->ExceptionPort == NULL)
              {
                /* keep the reference to the handle! */
                Process->ExceptionPort = ExceptionPort;
                Status = STATUS_SUCCESS;
              }
              else
              {
                ObDereferenceObject(ExceptionPort);
                Status = STATUS_PORT_ALREADY_SET;
              }
              PsUnlockProcess(Process);
            }
            else
            {
              ObDereferenceObject(ExceptionPort);
            }
          }
        }
        break;
      }

      case ProcessAccessToken:
      {
        HANDLE TokenHandle = NULL;

        /* make a safe copy of the buffer on the stack */
        _SEH_TRY
        {
          TokenHandle = ((PPROCESS_ACCESS_TOKEN)ProcessInformation)->Token;
          Status = STATUS_SUCCESS;
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(NT_SUCCESS(Status))
        {
          /* in case we had success reading from the buffer, perform the actual task */
          Status = PspAssignPrimaryToken(Process, TokenHandle);
        }
	break;
      }

      case ProcessDefaultHardErrorMode:
      {
        _SEH_TRY
        {
          InterlockedExchange((LONG*)&Process->DefaultHardErrorProcessing,
                              *(PLONG)ProcessInformation);
          Status = STATUS_SUCCESS;
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        break;
      }
      
      case ProcessSessionInformation:
      {
        PROCESS_SESSION_INFORMATION SessionInfo;
        Status = STATUS_SUCCESS;
        
        _SEH_TRY
        {
          /* copy the structure to the stack */
          SessionInfo = *(PPROCESS_SESSION_INFORMATION)ProcessInformation;
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        if(NT_SUCCESS(Status))
        {
          /* we successfully copied the structure to the stack, continue processing */
          
          /*
           * setting the session id requires the SeTcbPrivilege!
           */
          if(!SeSinglePrivilegeCheck(SeTcbPrivilege,
                                     PreviousMode))
          {
            DPRINT1("NtSetInformationProcess: Caller requires the SeTcbPrivilege privilege for setting ProcessSessionInformation!\n");
            /* can't set the session id, bail! */
            Status = STATUS_PRIVILEGE_NOT_HELD;
            break;
          }
          
          /* FIXME - update the session id for the process token */

          Status = PsLockProcess(Process, FALSE);
          if(NT_SUCCESS(Status))
          {
            Process->SessionId = SessionInfo.SessionId;

            /* Update the session id in the PEB structure */
            if(Process->Peb != NULL)
            {
              /* we need to attach to the process to make sure we're in the right
                 context to access the PEB structure */
              KeAttachProcess(&Process->Pcb);

              _SEH_TRY
              {
                /* FIXME: Process->Peb->SessionId = SessionInfo.SessionId; */

                Status = STATUS_SUCCESS;
              }
              _SEH_HANDLE
              {
                Status = _SEH_GetExceptionCode();
              }
              _SEH_END;

              KeDetachProcess();
            }

            PsUnlockProcess(Process);
          }
        }
        break;
      }
      
      case ProcessPriorityClass:
      {
        PROCESS_PRIORITY_CLASS ppc;

        _SEH_TRY
        {
          ppc = *(PPROCESS_PRIORITY_CLASS)ProcessInformation;
        }
        _SEH_HANDLE
        {
          Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
        
        if(NT_SUCCESS(Status))
        {
        }
        
        break;
      }
	
      case ProcessLdtInformation:
      case ProcessLdtSize:
      case ProcessIoPortHandlers:
      case ProcessWorkingSetWatch:
      case ProcessUserModeIOPL:
      case ProcessEnableAlignmentFaultFixup:
      case ProcessAffinityMask:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessBasicInformation:
      case ProcessIoCounters:
      case ProcessTimes:
      case ProcessPooledUsageAndLimits:
      case ProcessWx86Information:
      case ProcessHandleCount:
      case ProcessWow64Information:
      default:
	Status = STATUS_INVALID_INFO_CLASS;
     }
   ObDereferenceObject(Process);
   return(Status);
}


/**********************************************************************
 * NAME							INTERNAL
 * 	PiQuerySystemProcessInformation
 *
 * DESCRIPTION
 * 	Compute the size of a process+thread snapshot as 
 * 	expected by NtQuerySystemInformation.
 *
 * RETURN VALUE
 * 	0 on error; otherwise the size, in bytes of the buffer
 * 	required to write a full snapshot.
 *
 * NOTE
 * 	We assume (sizeof (PVOID) == sizeof (ULONG)) holds.
 */
NTSTATUS
PiQuerySystemProcessInformation(PVOID Buffer,
				ULONG Size,
				PULONG ReqSize)
{
   return STATUS_NOT_IMPLEMENTED;

#if 0
	PLIST_ENTRY	CurrentEntryP;
	PEPROCESS	CurrentP;
	PLIST_ENTRY	CurrentEntryT;
	PETHREAD	CurrentT;
	
	ULONG		RequiredSize = 0L;
	BOOLEAN		SizeOnly = FALSE;

	ULONG		SpiSize = 0L;
	
	PSYSTEM_PROCESS_INFORMATION	pInfoP = (PSYSTEM_PROCESS_INFORMATION) SnapshotBuffer;
	PSYSTEM_PROCESS_INFORMATION	pInfoPLast = NULL;
	PSYSTEM_THREAD_INFO		pInfoT = NULL;
	

   /* Lock the process list. */
   ExAcquireFastMutex(&PspActiveProcessMutex);

	/*
	 * Scan the process list. Since the
	 * list is circular, the guard is false
	 * after the last process.
	 */
	for (	CurrentEntryP = PsActiveProcessHead.Flink;
		(CurrentEntryP != & PsActiveProcessHead);
		CurrentEntryP = CurrentEntryP->Flink
		)
	{
		/*
		 * Compute how much space is
		 * occupied in the snapshot
		 * by adding this process info.
		 * (at least one thread).
		 */
		SpiSizeCurrent = sizeof (SYSTEM_PROCESS_INFORMATION);
		RequiredSize += SpiSizeCurrent;
		/*
		 * Do not write process data in the
		 * buffer if it is too small.
		 */
		if (TRUE == SizeOnly) continue;
		/*
		 * Check if the buffer can contain
		 * the full snapshot.
		 */
		if (Size < RequiredSize)
		{
			SizeOnly = TRUE;
			continue;
		}
		/* 
		 * Get a reference to the 
		 * process descriptor we are
		 * handling.
		 */
		CurrentP = CONTAINING_RECORD(
				CurrentEntryP,
				EPROCESS, 
				ProcessListEntry
				);
		/*
		 * Write process data in the buffer.
		 */
		RtlZeroMemory (pInfoP, sizeof (SYSTEM_PROCESS_INFORMATION));
		/* PROCESS */
		pInfoP->ThreadCount = 0L;
		pInfoP->ProcessId = CurrentP->UniqueProcessId;
		RtlInitUnicodeString (
			& pInfoP->Name,
			CurrentP->ImageFileName
			);
		/* THREAD */
		for (	pInfoT = & CurrentP->ThreadSysInfo [0],
			CurrentEntryT = CurrentP->ThreadListHead.Flink;
			
			(CurrentEntryT != & CurrentP->ThreadListHead);
			
			pInfoT = & CurrentP->ThreadSysInfo [pInfoP->ThreadCount],
			CurrentEntryT = CurrentEntryT->Flink
			)
		{
			/*
			 * Recalculate the size of the
			 * information block.
			 */
			if (0 < pInfoP->ThreadCount)
			{
				RequiredSize += sizeof (SYSTEM_THREAD_INFORMATION);
			}
			/*
			 * Do not write thread data in the
			 * buffer if it is too small.
			 */
			if (TRUE == SizeOnly) continue;
			/*
			 * Check if the buffer can contain
			 * the full snapshot.
			 */
			if (Size < RequiredSize)
			{
				SizeOnly = TRUE;
				continue;
			}
			/* 
			 * Get a reference to the 
			 * thread descriptor we are
			 * handling.
			 */
			CurrentT = CONTAINING_RECORD(
					CurrentEntryT,
					KTHREAD, 
					ThreadListEntry
					);
			/*
			 * Write thread data.
			 */
			RtlZeroMemory (
				pInfoT,
				sizeof (SYSTEM_THREAD_INFORMATION)
				);
			pInfoT->KernelTime	= CurrentT-> ;	/* TIME */
			pInfoT->UserTime	= CurrentT-> ;	/* TIME */
			pInfoT->CreateTime	= CurrentT-> ;	/* TIME */
			pInfoT->TickCount	= CurrentT-> ;	/* ULONG */
			pInfoT->StartEIP	= CurrentT-> ;	/* ULONG */
			pInfoT->ClientId	= CurrentT-> ;	/* CLIENT_ID */
			pInfoT->ClientId	= CurrentT-> ;	/* CLIENT_ID */
			pInfoT->DynamicPriority	= CurrentT-> ;	/* ULONG */
			pInfoT->BasePriority	= CurrentT-> ;	/* ULONG */
			pInfoT->nSwitches	= CurrentT-> ;	/* ULONG */
			pInfoT->State		= CurrentT-> ;	/* DWORD */
			pInfoT->WaitReason	= CurrentT-> ;	/* KWAIT_REASON */
			/*
			 * Count the number of threads 
			 * this process has.
			 */
			++ pInfoP->ThreadCount;
		}
		/*
		 * Save the size of information
		 * stored in the buffer for the
		 * current process.
		 */
		pInfoP->RelativeOffset = SpiSize;
		/*
		 * Save a reference to the last
		 * valid information block.
		 */
		pInfoPLast = pInfoP;
		/*
		 * Compute the offset of the 
		 * SYSTEM_PROCESS_INFORMATION
		 * descriptor in the snapshot 
		 * buffer for the next process.
		 */
		(ULONG) pInfoP += SpiSize;
	}
	/*
	 * Unlock the process list.
	 */
	ExReleaseFastMutex (
		& PspActiveProcessMutex
		);
	/*
	 * Return the proper error status code,
	 * if the buffer was too small.
	 */
	if (TRUE == SizeOnly)
	{
		if (NULL != RequiredSize)
		{
			*pRequiredSize = RequiredSize;
		}
		return STATUS_INFO_LENGTH_MISMATCH;
	}
	/*
	 * Mark the end of the snapshot.
	 */
	pInfoP->RelativeOffset = 0L;
	/* OK */	
	return STATUS_SUCCESS;
#endif
}

/*
 * @implemented
 */
LARGE_INTEGER STDCALL
PsGetProcessExitTime(VOID)
{
  LARGE_INTEGER Li;
  Li.QuadPart = PsGetCurrentProcess()->ExitTime.QuadPart;
  return Li;
}

/*
 * @implemented
 */
LONGLONG
STDCALL
PsGetProcessCreateTimeQuadPart(
    PEPROCESS	Process
	)
{
	return Process->CreateTime.QuadPart;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessDebugPort(
    PEPROCESS	Process
	)
{
	return Process->DebugPort;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsGetProcessExitProcessCalled(
    PEPROCESS	Process
	)
{
	return Process->ExitProcessCalled;	
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsGetProcessExitStatus(
	PEPROCESS Process
	)
{
	return Process->ExitStatus;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetProcessId(
   	PEPROCESS	Process
	)
{
	return (HANDLE)Process->UniqueProcessId;
}

/*
 * @implemented
 */
LPSTR
STDCALL
PsGetProcessImageFileName(
    PEPROCESS	Process
	)
{
	return (LPSTR)Process->ImageFileName;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetProcessInheritedFromUniqueProcessId(
    PEPROCESS	Process
	)
{
	return Process->InheritedFromUniqueProcessId;
}

/*
 * @implemented
 */
PEJOB
STDCALL
PsGetProcessJob(
	PEPROCESS Process
	)
{
	return Process->Job;
}

/*
 * @implemented
 */
PPEB
STDCALL
PsGetProcessPeb(
    PEPROCESS	Process
	)
{
	return Process->Peb;	
}

/*
 * @implemented
 */
ULONG
STDCALL
PsGetProcessPriorityClass(
    PEPROCESS	Process
	)
{
	return Process->PriorityClass;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessSectionBaseAddress(
    PEPROCESS	Process
	)
{
	return Process->SectionBaseAddress;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessSecurityPort(
	PEPROCESS Process
	)
{
	return Process->SecurityPort;
}

/*
 * @implemented
 */
HANDLE
STDCALL
PsGetProcessSessionId(
    PEPROCESS	Process
	)
{
	return (HANDLE)Process->SessionId;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessWin32Process(
	PEPROCESS Process
	)
{
	return Process->Win32Process;
}

/*
 * @implemented
 */
PVOID
STDCALL
PsGetProcessWin32WindowStation(
    PEPROCESS	Process
	)
{
	return Process->Win32WindowStation;
}

/*
 * @implemented
 */
BOOLEAN
STDCALL
PsIsProcessBeingDebugged(
    PEPROCESS	Process
	)
{
	return FALSE/*Process->IsProcessBeingDebugged*/;
}


/*
 * @implemented
 */
NTSTATUS STDCALL
PsLookupProcessByProcessId(IN HANDLE ProcessId,
			   OUT PEPROCESS *Process)
{
  PLIST_ENTRY current_entry;
  PEPROCESS current;

  ExAcquireFastMutex(&PspActiveProcessMutex);

  current_entry = PsActiveProcessHead.Flink;
  while (current_entry != &PsActiveProcessHead)
    {
      current = CONTAINING_RECORD(current_entry,
				  EPROCESS,
				  ProcessListEntry);
      if (current->UniqueProcessId == ProcessId)
	{
	  *Process = current;
          ObReferenceObject(current);
	  ExReleaseFastMutex(&PspActiveProcessMutex);
	  return(STATUS_SUCCESS);
	}
      current_entry = current_entry->Flink;
    }

  ExReleaseFastMutex(&PspActiveProcessMutex);

  return(STATUS_INVALID_PARAMETER);
}

VOID
STDCALL
PspRunCreateProcessNotifyRoutines
(
 PEPROCESS CurrentProcess,
 BOOLEAN Create
)
{
 ULONG i;
 HANDLE ProcessId = (HANDLE)CurrentProcess->UniqueProcessId;
 HANDLE ParentId = CurrentProcess->InheritedFromUniqueProcessId;
 
 for(i = 0; i < MAX_PROCESS_NOTIFY_ROUTINE_COUNT; ++ i)
  if(PiProcessNotifyRoutine[i])
   PiProcessNotifyRoutine[i](ParentId, ProcessId, Create);
}

/*
 * @implemented
 */
NTSTATUS STDCALL
PsSetCreateProcessNotifyRoutine(IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
				IN BOOLEAN Remove)
{
  ULONG i;

  if (Remove)
  {
     for(i=0;i<MAX_PROCESS_NOTIFY_ROUTINE_COUNT;i++)
     {
        if ((PVOID)PiProcessNotifyRoutine[i] == (PVOID)NotifyRoutine)
        {
           PiProcessNotifyRoutine[i] = NULL;
           break;
        }
     }

     return(STATUS_SUCCESS);
  }

  /*insert*/
  for(i=0;i<MAX_PROCESS_NOTIFY_ROUTINE_COUNT;i++)
  {
     if (PiProcessNotifyRoutine[i] == NULL)
     {
        PiProcessNotifyRoutine[i] = NotifyRoutine;
        break;
     }
  }

  if (i == MAX_PROCESS_NOTIFY_ROUTINE_COUNT)
  {
     return STATUS_INSUFFICIENT_RESOURCES;
  }

  return STATUS_SUCCESS;
}

VOID STDCALL
PspRunLoadImageNotifyRoutines(
   PUNICODE_STRING FullImageName,
   HANDLE ProcessId,
   PIMAGE_INFO ImageInfo)
{
   ULONG i;
 
   for (i = 0; i < MAX_PROCESS_NOTIFY_ROUTINE_COUNT; ++ i)
      if (PiLoadImageNotifyRoutine[i])
         PiLoadImageNotifyRoutine[i](FullImageName, ProcessId, ImageInfo);
}

/*
 * @unimplemented
 */                       
NTSTATUS
STDCALL
PsRemoveLoadImageNotifyRoutine(
    IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine
    )
{
	UNIMPLEMENTED;
	return STATUS_NOT_IMPLEMENTED;	
}

/*
 * @implemented
 */
NTSTATUS STDCALL
PsSetLoadImageNotifyRoutine(IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine)
{
   ULONG i;

   for (i = 0; i < MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT; i++)
   {
      if (PiLoadImageNotifyRoutine[i] == NULL)
      {
         PiLoadImageNotifyRoutine[i] = NotifyRoutine;
         break;
      }
   }

   if (i == MAX_PROCESS_NOTIFY_ROUTINE_COUNT)
   {
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   return STATUS_SUCCESS;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsSetProcessPriorityClass(
    PEPROCESS	Process,
    ULONG	PriorityClass	
	)
{
	Process->PriorityClass = PriorityClass;
}

/*
 * @implemented
 */                       
VOID
STDCALL
PsSetProcessSecurityPort(
    PEPROCESS	Process,
    PVOID	SecurityPort	
	)
{
	Process->SecurityPort = SecurityPort;
}

/*
 * @implemented
 */
VOID
STDCALL
PsSetProcessWin32Process(
    PEPROCESS	Process,
    PVOID	Win32Process
	)
{
	Process->Win32Process = Win32Process;
}

/*
 * @implemented
 */
VOID
STDCALL
PsSetProcessWin32WindowStation(
    PEPROCESS	Process,
    PVOID	WindowStation
	)
{
	Process->Win32WindowStation = WindowStation;
}

/* Pool Quotas */
/*
 * @implemented
 */
VOID
STDCALL
PsChargePoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG_PTR Amount
    )
{
    NTSTATUS Status;

    /* Charge the usage */
    Status = PsChargeProcessPoolQuota(Process, PoolType, Amount);

    /* Raise Exception */
    if (!NT_SUCCESS(Status)) {
        ExRaiseStatus(Status);
    }
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsChargeProcessNonPagedPoolQuota (
    IN PEPROCESS Process,
    IN ULONG_PTR Amount
    )
{
    /* Call the general function */
    return PsChargeProcessPoolQuota(Process, NonPagedPool, Amount);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsChargeProcessPagedPoolQuota (
    IN PEPROCESS Process,
    IN ULONG_PTR Amount
    )
{
    /* Call the general function */
    return PsChargeProcessPoolQuota(Process, PagedPool, Amount);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsChargeProcessPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG_PTR Amount
    )
{
    PEPROCESS_QUOTA_BLOCK QuotaBlock;
    ULONG NewUsageSize;
    ULONG NewMaxQuota;

    /* Get current Quota Block */
    QuotaBlock = Process->QuotaBlock;

    /* Quota Operations are not to be done on the SYSTEM Process */
    if (Process == PsInitialSystemProcess) return STATUS_SUCCESS;

    /* New Size in use */
    NewUsageSize = QuotaBlock->QuotaEntry[PoolType].Usage + Amount;

    /* Does this size respect the quota? */
    if (NewUsageSize > QuotaBlock->QuotaEntry[PoolType].Limit) {

        /* It doesn't, so keep raising the Quota */
        while (MiRaisePoolQuota(PoolType, QuotaBlock->QuotaEntry[PoolType].Limit, &NewMaxQuota)) {
            /* Save new Maximum Quota */
            QuotaBlock->QuotaEntry[PoolType].Limit = NewMaxQuota;

            /* See if the new Maximum Quota fulfills our need */
            if (NewUsageSize <= NewMaxQuota) goto QuotaChanged;
        }

        return STATUS_QUOTA_EXCEEDED;
    }

QuotaChanged:
    /* Save new Usage */
    QuotaBlock->QuotaEntry[PoolType].Usage = NewUsageSize;

    /* Is this a new peak? */
    if (NewUsageSize > QuotaBlock->QuotaEntry[PoolType].Peak) {
        QuotaBlock->QuotaEntry[PoolType].Peak = NewUsageSize;
    }

    /* All went well */
    return STATUS_SUCCESS;
}

/*
 * @unimplemented
 */                       
VOID
STDCALL
PsReturnPoolQuota(
    IN PEPROCESS Process,
    IN POOL_TYPE PoolType,
    IN ULONG_PTR Amount
    )
{
	UNIMPLEMENTED;
} 

/*
 * @unimplemented
 */                       
VOID
STDCALL
PsReturnProcessNonPagedPoolQuota(
    IN PEPROCESS Process,
    IN ULONG_PTR Amount
    )
{
	UNIMPLEMENTED;
} 

/*
 * @unimplemented
 */                       
VOID
STDCALL
PsReturnProcessPagedPoolQuota(
    IN PEPROCESS Process,
    IN ULONG_PTR Amount
    )
{
	UNIMPLEMENTED;
}

NTSTATUS
PsLockProcess(PEPROCESS Process, BOOL Timeout)
{
  ULONG Attempts = 0;
  PKTHREAD PrevLockOwner;
  NTSTATUS Status = STATUS_UNSUCCESSFUL;
  PLARGE_INTEGER Delay = (Timeout ? &PsLockTimeout : NULL);
  PKTHREAD CallingThread = KeGetCurrentThread();
  
  PAGED_CODE();
  
  KeEnterCriticalRegion();
  
  for(;;)
  {
    if(Process->Pcb.State == PROCESS_STATE_TERMINATED)
    {
      KeLeaveCriticalRegion();
      return STATUS_PROCESS_IS_TERMINATING;
    }

    PrevLockOwner = (PKTHREAD)InterlockedCompareExchangePointer(
      &Process->LockOwner, CallingThread, NULL);
    if(PrevLockOwner == NULL || PrevLockOwner == CallingThread)
    {
      /* we got the lock or already locked it */
      if(InterlockedIncrementUL(&Process->LockCount) == 1)
      {
        KeClearEvent(&Process->LockEvent);
      }

      return STATUS_SUCCESS;
    }
    else
    {
      if(++Attempts > 2)
      {
        Status = KeWaitForSingleObject(&Process->LockEvent,
                                       Executive,
                                       KernelMode,
                                       FALSE,
                                       Delay);
        if(!NT_SUCCESS(Status) || Status == STATUS_TIMEOUT)
        {
#ifndef NDEBUG
          if(Status == STATUS_TIMEOUT)
          {
            DPRINT1("PsLockProcess(0x%x) timed out!\n", Process);
          }
#endif
          KeLeaveCriticalRegion();

        }
        break;
      }
      else
      {
        KeDelayExecutionThread(KernelMode, FALSE, &ShortPsLockDelay);
      }
    }
  }
  
  return Status;
}

VOID
PsUnlockProcess(PEPROCESS Process)
{
  PAGED_CODE();
  
  ASSERT(Process->LockOwner == KeGetCurrentThread());
  
  if(InterlockedDecrementUL(&Process->LockCount) == 0)
  {
    InterlockedExchangePointer(&Process->LockOwner, NULL);
    KeSetEvent(&Process->LockEvent, IO_NO_INCREMENT, FALSE);
  }
  
  KeLeaveCriticalRegion();
}

/* EOF */
