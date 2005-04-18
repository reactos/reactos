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

VOID INIT_FUNCTION PsInitClientIDManagment(VOID);

PEPROCESS EXPORTED PsInitialSystemProcess = NULL;
PEPROCESS PsIdleProcess = NULL;

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

#define MAX_PROCESS_NOTIFY_ROUTINE_COUNT    8
#define MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT  8

static PCREATE_PROCESS_NOTIFY_ROUTINE
PiProcessNotifyRoutine[MAX_PROCESS_NOTIFY_ROUTINE_COUNT];
static PLOAD_IMAGE_NOTIFY_ROUTINE
PiLoadImageNotifyRoutine[MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT];
                             
/* FUNCTIONS *****************************************************************/

PEPROCESS
PsGetNextProcess(PEPROCESS OldProcess)
{
   PEPROCESS NextProcess;
   NTSTATUS Status;
   
   if (OldProcess == NULL)
     {
       Status = ObReferenceObjectByPointer(PsIdleProcess,
				           PROCESS_ALL_ACCESS,
				           PsProcessType,
				           KernelMode);   
       if (!NT_SUCCESS(Status))
         {
	   CPRINT("PsGetNextProcess(): ObReferenceObjectByPointer failed for PsIdleProcess\n");
	   KEBUGCHECK(0);
	 }
       return PsIdleProcess;
     }
   
   ExAcquireFastMutex(&PspActiveProcessMutex);
   NextProcess = OldProcess;
   while (1)
     {
       PLIST_ENTRY Flink = (NextProcess == PsIdleProcess ? PsActiveProcessHead.Flink :
                                                           NextProcess->ProcessListEntry.Flink);
       if (Flink != &PsActiveProcessHead)
         {
           NextProcess = CONTAINING_RECORD(Flink,
			                   EPROCESS,
			                   ProcessListEntry);
         }
       else
         {
           NextProcess = NULL;
           break;
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
	   continue;
	 }
     }

   ExReleaseFastMutex(&PspActiveProcessMutex);
   ObDereferenceObject(OldProcess);
   
   return(NextProcess);
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
	     PspTerminateProcessThreads(current, STATUS_SUCCESS);
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
   PsProcessType->Delete = PspDeleteProcess;
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
    * Initialize the idle process
    */
   Status = ObCreateObject(KernelMode,
			   PsProcessType,
			   NULL,
			   KernelMode,
			   NULL,
			   sizeof(EPROCESS),
			   0,
			   0,
			   (PVOID*)&PsIdleProcess);
   if (!NT_SUCCESS(Status))
     {
        DPRINT1("Failed to create the idle process object, Status: 0x%x\n", Status);
        KEBUGCHECK(0);
        return;
     }

   RtlZeroMemory(PsIdleProcess, sizeof(EPROCESS));
   
   PsIdleProcess->Pcb.Affinity = 0xFFFFFFFF;
   PsIdleProcess->Pcb.IopmOffset = 0xffff;
   PsIdleProcess->Pcb.LdtDescriptor[0] = 0;
   PsIdleProcess->Pcb.LdtDescriptor[1] = 0;
   PsIdleProcess->Pcb.BasePriority = PROCESS_PRIO_IDLE;
   PsIdleProcess->Pcb.ThreadQuantum = 6;
   InitializeListHead(&PsIdleProcess->Pcb.ThreadListHead);
   InitializeListHead(&PsIdleProcess->ThreadListHead);
   InitializeListHead(&PsIdleProcess->ProcessListEntry);
   KeInitializeDispatcherHeader(&PsIdleProcess->Pcb.DispatcherHeader,
				ProcessObject,
				sizeof(EPROCESS),
				FALSE);
   PsIdleProcess->Pcb.DirectoryTableBase =
     (LARGE_INTEGER)(LONGLONG)(ULONG)MmGetPageDirectory();
   strcpy(PsIdleProcess->ImageFileName, "Idle");

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
        DPRINT1("Failed to create the system process object, Status: 0x%x\n", Status);
        KEBUGCHECK(0);
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
				ProcessObject,
				sizeof(EPROCESS),
				FALSE);
   KProcess = &PsInitialSystemProcess->Pcb;
   
   MmInitializeAddressSpace(PsInitialSystemProcess,
			    &PsInitialSystemProcess->AddressSpace);
   
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
   
   PsInitialSystemProcess->Win32WindowStation = (HANDLE)0;
   
   InsertHeadList(&PsActiveProcessHead,
		  &PsInitialSystemProcess->ProcessListEntry);
   InitializeListHead(&PsInitialSystemProcess->ThreadListHead);
   
#ifndef SCHED_REWRITE
    PTOKEN BootToken;
        
    /* No parent, this is the Initial System Process. Assign Boot Token */
    BootToken = SepCreateSystemProcessToken();
    BootToken->TokenInUse = TRUE;
    PsInitialSystemProcess->Token = BootToken;
    ObReferenceObject(BootToken);
#endif
}

VOID
PspPostInitSystemProcess(VOID)
{
  NTSTATUS Status;
  
  /* this routine is called directly after the exectuive handle tables were
     initialized. We'll set up the Client ID handle table and assign the system
     process a PID */
  PsInitClientIDManagment();
  
  ObCreateHandleTable(NULL, FALSE, PsInitialSystemProcess);
  ObpKernelHandleTable = PsInitialSystemProcess->ObjectTable;
  
  Status = PsCreateCidHandle(PsInitialSystemProcess,
                             PsProcessType,
                             &PsInitialSystemProcess->UniqueProcessId);
  if(!NT_SUCCESS(Status))
  {
    DPRINT1("Failed to create CID handle (unique process id) for the system process!\n");
    KEBUGCHECK(0);
  }
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
STDCALL
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
    PEPORT pDebugPort = NULL;
    PEPORT pExceptionPort = NULL;
    PSECTION_OBJECT SectionObject = NULL;
    NTSTATUS Status = STATUS_SUCCESS;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PHYSICAL_ADDRESS DirectoryTableBase;
    KAFFINITY Affinity;
    DirectoryTableBase.QuadPart = (ULONGLONG)0;

    DPRINT("PspCreateProcess(ObjectAttributes %x)\n", ObjectAttributes);
   
    /* Reference the Parent if there is one */
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
        
        /* Inherit Parent process's Affinity. */
        Affinity = pParentProcess->Pcb.Affinity;  
        
    } 
    else 
    {
        pParentProcess = NULL;
        Affinity = KeActiveProcessors;
    }

    /* Add the debug port */
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

    /* Add the exception port */
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

    /* Add the Section */
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

    /* Create the Object */
    DPRINT("Creating Process Object\n");
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
        goto exitdereferenceobjects;
    }
  
    /* Clean up the Object */
    DPRINT("Cleaning Process Object\n");
    RtlZeroMemory(Process, sizeof(EPROCESS));
    
    /* Inherit stuff from the Parent since we now have the object created */
    if (pParentProcess) 
    {
        Process->InheritedFromUniqueProcessId = pParentProcess->UniqueProcessId;
        Process->SessionId = pParentProcess->SessionId;
    }
    
    /* FIXME: Set up the Quota Block from the Parent
    PspInheritQuota(Parent, Process); */
               
    /* FIXME: Set up Dos Device Map from the Parent
    ObInheritDeviceMap(Parent, Process) */
               
    /* Set the Process' LPC Ports */
    Process->DebugPort = pDebugPort;
    Process->ExceptionPort = pExceptionPort;
    
    /* Setup the Lock Event */
    DPRINT("Initialzing Process Lock\n");
    KeInitializeEvent(&Process->LockEvent, SynchronizationEvent, FALSE);
    
    /* Setup the Thread List Head */
    DPRINT("Initialzing Process ThreadListHead\n");
    InitializeListHead(&Process->ThreadListHead);
        
    /* Create or Clone the Handle Table */
    DPRINT("Initialzing Process Handle Table\n");
    ObCreateHandleTable(pParentProcess, InheritObjectTable,  Process);
    DPRINT("Handle Table: %x\n", Process->ObjectTable);
   
    /* Set Process's Directory Base */
    DPRINT("Initialzing Process Directory Base\n");
    MmCopyMmInfo(pParentProcess ? pParentProcess : PsInitialSystemProcess, 
                 Process,
                 &DirectoryTableBase);
   
    /* Now initialize the Kernel Process */
    DPRINT("Initialzing Kernel Process\n");
    KeInitializeProcess(&Process->Pcb,
                        PROCESS_PRIO_NORMAL,
                        Affinity,
                        DirectoryTableBase);
      
    /* Duplicate Parent Token */
    DPRINT("Initialzing Process Token\n");
    Status = PspInitializeProcessSecurity(Process, pParentProcess);
    if (!NT_SUCCESS(Status)) 
    {
        DbgPrint("PspInitializeProcessSecurity failed (Status %x)\n", Status);
        ObDereferenceObject(Process);
        goto exitdereferenceobjects;
    }
    
    /* Create the Process' Address Space */
    DPRINT("Initialzing Process Address Space\n");
    Status = MmCreateProcessAddressSpace(Process, SectionObject);
    if (!NT_SUCCESS(Status)) 
    {
        DPRINT1("Failed to create Address Space\n");
        ObDereferenceObject(Process);
        goto exitdereferenceobjects;
    }
    
    if (SectionObject) 
    {
        /* Map the System Dll */
        DPRINT("Mapping System DLL\n");
        LdrpMapSystemDll(Process, NULL);
    }

    /* Create a handle for the Process */
    DPRINT("Initialzing Process CID Handle\n");
    Status = PsCreateCidHandle(Process,
                               PsProcessType,
                               &Process->UniqueProcessId);
    if(!NT_SUCCESS(Status)) 
    {
        DPRINT1("Failed to create CID handle (unique process ID)! Status: 0x%x\n", Status);
        ObDereferenceObject(Process);
        goto exitdereferenceobjects;
   }
   
    /* FIXME: Insert into Job Object */

    /* Create PEB only for User-Mode Processes */
    if (pParentProcess) 
    {
        DPRINT("Creating PEB\n");
        Status = MmCreatePeb(Process);
        if (!NT_SUCCESS(Status)) 
        {
            DbgPrint("NtCreateProcess() Peb creation failed: Status %x\n",Status);
            ObDereferenceObject(Process);
            goto exitdereferenceobjects;
        }
        
        /* Let's take advantage of this time to kill the reference too */
        ObDereferenceObject(pParentProcess);
    }

    /* W00T! The process can now be activated */
    DPRINT("Inserting into Active Process List\n");
    ExAcquireFastMutex(&PspActiveProcessMutex);
    InsertTailList(&PsActiveProcessHead, &Process->ProcessListEntry);
    ExReleaseFastMutex(&PspActiveProcessMutex);
    
    /* FIXME: SeCreateAccessStateEx */
               
    /* Insert the Process into the Object Directory */
    DPRINT("Inserting Process Object\n");
    Status = ObInsertObject(Process,
                            NULL,
                            DesiredAccess,
                            0,
                            NULL,
                            &hProcess);
    if (!NT_SUCCESS(Status)) 
    {
        DPRINT1("Could not get a handle to the Process Object\n");
        ObDereferenceObject(Process);
        goto exitdereferenceobjects;
    }
               
    DPRINT("Done. Returning handle: %x\n", hProcess);
    if (NT_SUCCESS(Status)) 
    {
        _SEH_TRY 
        {
            *ProcessHandle = hProcess;
        } 
        _SEH_HANDLE 
        {
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
    }
    
    /* FIXME: ObGetObjectSecurity(Process, &SecurityDescriptor)
              SeAccessCheck
    */
   ObReferenceObject(Process);
   ObReferenceObject(Process);
   return Status;
   
exitdereferenceobjects:
    if(SectionObject != NULL) ObDereferenceObject(SectionObject);
    if(pExceptionPort != NULL) ObDereferenceObject(pExceptionPort);
    if(pDebugPort != NULL) ObDereferenceObject(pDebugPort);
    if(pParentProcess != NULL) ObDereferenceObject(pParentProcess);
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
  PHANDLE_TABLE_ENTRY CidEntry;
  PEPROCESS FoundProcess;

  PAGED_CODE();

  ASSERT(Process);

  CidEntry = PsLookupCidHandle(ProcessId, PsProcessType, (PVOID*)&FoundProcess);
  if(CidEntry != NULL)
  {
    ObReferenceObject(FoundProcess);

    PsUnlockCidHandle(CidEntry);

    *Process = FoundProcess;
    return STATUS_SUCCESS;
  }

  return STATUS_INVALID_PARAMETER;
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
PsLockProcess(PEPROCESS Process, BOOLEAN Timeout)
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
          break;
        }
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
