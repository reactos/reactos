/* $Id: process.c,v 1.72 2002/01/02 21:00:02 chorns Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * FILE:              ntoskrnl/ps/process.c
 * PURPOSE:           Process managment
 * PROGRAMMER:        David Welch (welch@cwcom.net)
 * REVISION HISTORY:
 *              21/07/98: Created
 */

/* INCLUDES ******************************************************************/

#include <limits.h>
#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/mm.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <internal/id.h>
#include <napi/teb.h>
#include <internal/ldr.h>
#include <internal/port.h>
#include <napi/dbg.h>
#include <internal/dbg.h>
#include <napi/shared_data.h>
#include <internal/pool.h>
#include <roscfg.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

PEPROCESS EXPORTED PsInitialSystemProcess = NULL;
HANDLE SystemProcessHandle = NULL;

POBJECT_TYPE EXPORTED PsProcessType = NULL;

LIST_ENTRY PsProcessListHead;
static KSPIN_LOCK PsProcessListLock;
static ULONG PiNextProcessUniqueId = 0;

static GENERIC_MAPPING PiProcessMapping = {PROCESS_READ,
					   PROCESS_WRITE,
					   PROCESS_EXECUTE,
					   PROCESS_ALL_ACCESS};

/* FUNCTIONS *****************************************************************/


PEPROCESS 
PsGetNextProcess(PEPROCESS OldProcess)
{
   KIRQL oldIrql;
   PEPROCESS NextProcess;
   NTSTATUS Status;
   
   if (OldProcess == NULL)
     {
	return(PsInitialSystemProcess);
     }
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   if (OldProcess->ProcessListEntry.Flink == &PsProcessListHead)
     {
	NextProcess = CONTAINING_RECORD(PsProcessListHead.Flink,
					EPROCESS,
					ProcessListEntry);	
     }
   else
     {
	NextProcess = CONTAINING_RECORD(OldProcess->ProcessListEntry.Flink, 
					EPROCESS,
					ProcessListEntry);
     }
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
   Status = ObReferenceObjectByPointer(NextProcess,
				       PROCESS_ALL_ACCESS,
				       PsProcessType,
				       KernelMode);   
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("PsGetNextProcess(): ObReferenceObjectByPointer failed\n");
	KeBugCheck(0);
     }
   ObDereferenceObject(OldProcess);
   
   return(NextProcess);
}

NTSTATUS STDCALL 
NtOpenProcessToken(IN	HANDLE		ProcessHandle,
		   IN	ACCESS_MASK	DesiredAccess,
		   OUT	PHANDLE		TokenHandle)
{
  return(STATUS_UNSUCCESSFUL);
}

NTSTATUS STDCALL 
_NtOpenProcessToken(IN	HANDLE		ProcessHandle,
		   IN	ACCESS_MASK	DesiredAccess,
		   OUT	PHANDLE		TokenHandle)
{
   PACCESS_TOKEN Token;
   NTSTATUS Status;
  
   Status = PsOpenTokenOfProcess(ProcessHandle,
				 &Token);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   Status = ObCreateHandle(PsGetCurrentProcess(),
			   Token,
			   DesiredAccess,
			   FALSE,
			   ProcessHandle);
   ObDereferenceObject(Token);
   return(Status);
}


PACCESS_TOKEN STDCALL
PsReferencePrimaryToken(PEPROCESS Process)
{
   ObReferenceObjectByPointer(Process->Token,
			      TOKEN_ALL_ACCESS,
			      SeTokenType,
			      UserMode);
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
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   *Token = PsReferencePrimaryToken(Process);
   ObDereferenceObject(Process);
   return(STATUS_SUCCESS);
}


VOID 
PiKillMostProcesses(VOID)
{
   KIRQL oldIrql;
   PLIST_ENTRY current_entry;
   PEPROCESS current;
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   
   current_entry = PsProcessListHead.Flink;
   while (current_entry != &PsProcessListHead)
     {
	current = CONTAINING_RECORD(current_entry, EPROCESS, 
				    ProcessListEntry);
	current_entry = current_entry->Flink;
	
	if (current->UniqueProcessId != PsInitialSystemProcess->UniqueProcessId &&
	    current->UniqueProcessId != (ULONG)PsGetCurrentProcessId())
	  {
	     PiTerminateProcessThreads(current, STATUS_SUCCESS);
	  }
     }
   
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
}


VOID
PsInitProcessManagment(VOID)
{

   PKPROCESS KProcess;
   KIRQL oldIrql;
   NTSTATUS Status;
   
   /*
    * Register the process object type
    */
   
   PsProcessType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));

   PsProcessType->Tag = TAG('P', 'R', 'O', 'C');
   PsProcessType->TotalObjects = 0;
   PsProcessType->TotalHandles = 0;
   PsProcessType->MaxObjects = ULONG_MAX;
   PsProcessType->MaxHandles = ULONG_MAX;
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
   
   InitializeListHead(&PsProcessListHead);
   KeInitializeSpinLock(&PsProcessListLock);
   
   /*
    * Initialize the system process
    */
   Status = ObCreateObject(NULL,
			   PROCESS_ALL_ACCESS,
			   NULL,
			   PsProcessType,
			   (PVOID*)&PsInitialSystemProcess);
   if (!NT_SUCCESS(Status))
     {
	return;
     }
   
   /* System threads may run on any processor. */
   PsInitialSystemProcess->Pcb.Affinity = 0xFFFFFFFF;
   PsInitialSystemProcess->Pcb.BasePriority = PROCESS_PRIO_NORMAL;
   KeInitializeDispatcherHeader(&PsInitialSystemProcess->Pcb.DispatcherHeader,
				InternalProcessType,
				sizeof(EPROCESS),
				FALSE);
   KProcess = &PsInitialSystemProcess->Pcb;
   
   MmInitializeAddressSpace(PsInitialSystemProcess,
			    &PsInitialSystemProcess->AddressSpace);
   ObCreateHandleTable(NULL,FALSE,PsInitialSystemProcess);
   KProcess->DirectoryTableBase[0] = MmGetPageDirectory();
   PsInitialSystemProcess->UniqueProcessId = 
     InterlockedIncrement(&PiNextProcessUniqueId);
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   InsertHeadList(&PsProcessListHead, 
		  &PsInitialSystemProcess->ProcessListEntry);
   InitializeListHead(&PsInitialSystemProcess->ThreadListHead);
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
   
   strcpy(PsInitialSystemProcess->ImageFileName, "SYSTEM");
   
   ObCreateHandle(PsInitialSystemProcess,
		  PsInitialSystemProcess,
		  PROCESS_ALL_ACCESS,
		  FALSE,
		  &SystemProcessHandle);
}

#ifdef KDBG

VOID
PiFreeSymbols(PPEB Peb)
{
  PLIST_ENTRY CurrentEntry;
  PLDR_MODULE Current;
  PSYMBOL NextSymbol;
  PSYMBOL Symbol;

  assert (Peb);
  assert (Peb->Ldr);

	CurrentEntry = Peb->Ldr->InLoadOrderModuleList.Flink;
	while ((CurrentEntry != &Peb->Ldr->InLoadOrderModuleList) && (CurrentEntry != NULL))
	{
	  Current = CONTAINING_RECORD (CurrentEntry, LDR_MODULE, InLoadOrderModuleList);
    Symbol = Current->Symbols.Symbols;
		while (Symbol != NULL)
		{
      NextSymbol = Symbol->Next;
      ExFreePool (Symbol);
	    Symbol = NextSymbol;
		}
    Current->Symbols.SymbolCount = 0;
    Current->Symbols.Symbols = NULL;
    CurrentEntry = CurrentEntry->Flink;
  }
}

#endif /* KDBG */

VOID STDCALL
PiDeleteProcess(PVOID ObjectBody)
{
   KIRQL oldIrql;
   
   DPRINT("PiDeleteProcess(ObjectBody %x)\n",ObjectBody);
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   RemoveEntryList(&((PEPROCESS)ObjectBody)->ProcessListEntry);
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);

#ifdef KDBG
   PiFreeSymbols(((PEPROCESS)ObjectBody)->Peb);
#endif /* KDBG */

   (VOID)MmReleaseMmInfo((PEPROCESS)ObjectBody);
   ObDeleteHandleTable((PEPROCESS)ObjectBody);
}


static NTSTATUS
PsCreatePeb(HANDLE ProcessHandle,
	    PVOID ImageBase,
	    PVOID* RPeb)
{
   NTSTATUS Status;
   PVOID PebBase;
   ULONG PebSize;
   PEB Peb;
   ULONG BytesWritten;
   
   memset(&Peb, 0, sizeof(Peb));
   Peb.ImageBaseAddress = ImageBase;
   
   PebBase = (PVOID)PEB_BASE;
   PebSize = 0x1000;
   Status = NtAllocateVirtualMemory(ProcessHandle,
				    &PebBase,
				    0,
				    &PebSize,
				    MEM_RESERVE | MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   NtWriteVirtualMemory(ProcessHandle,
			(PVOID)PEB_BASE,
			&Peb,
			sizeof(Peb),
			&BytesWritten);

   DPRINT("PsCreatePeb: Peb created at %x\n", PebBase);
   
   *RPeb = PebBase;
   
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

HANDLE STDCALL
PsGetCurrentProcessId(VOID)
{
   return((HANDLE)PsGetCurrentProcess()->UniqueProcessId);
}

/*
 * FUNCTION: Returns a pointer to the current process
 */
PEPROCESS STDCALL
PsGetCurrentProcess(VOID)
{
   if (PsGetCurrentThread() == NULL || 
       PsGetCurrentThread()->ThreadsProcess == NULL)
     {
	return(PsInitialSystemProcess);
     }
   else
     {
	return(PsGetCurrentThread()->ThreadsProcess);
     }
}

PEPROCESS STDCALL
IoGetCurrentProcess(VOID)
{
   return(PsGetCurrentProcess());
}

NTSTATUS STDCALL
PsCreateSystemProcess(PHANDLE ProcessHandle,
		      ACCESS_MASK DesiredAccess,
		      POBJECT_ATTRIBUTES ObjectAttributes)
{
   return NtCreateProcess(ProcessHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  SystemProcessHandle,
			  FALSE,
			  NULL,
			  NULL,
			  NULL);
}

NTSTATUS STDCALL
NtCreateProcess(OUT PHANDLE ProcessHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
		IN HANDLE ParentProcessHandle,
		IN BOOLEAN InheritObjectTable,
		IN HANDLE SectionHandle OPTIONAL,
		IN HANDLE DebugPortHandle OPTIONAL,
		IN HANDLE ExceptionPortHandle OPTIONAL)
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
   PEPROCESS Process;
   PEPROCESS ParentProcess;
   PKPROCESS KProcess;
   NTSTATUS Status;
   KIRQL oldIrql;
   PVOID LdrStartupAddr;
   PVOID ImageBase;
   PVOID Peb;
   PEPORT DebugPort;
   PEPORT ExceptionPort;
   PVOID BaseAddress;
   PMEMORY_AREA MemoryArea;
   
   DPRINT("NtCreateProcess(ObjectAttributes %x)\n",ObjectAttributes);

   Status = ObReferenceObjectByHandle(ParentProcessHandle,
				      PROCESS_CREATE_PROCESS,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&ParentProcess,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtCreateProcess() = %x\n",Status);
	return(Status);
     }

   Status = ObCreateObject(ProcessHandle,
			   DesiredAccess,
			   ObjectAttributes,
			   PsProcessType,
			   (PVOID*)&Process);
   if (!NT_SUCCESS(Status))
     {
	ObDereferenceObject(ParentProcess);
	DPRINT("ObCreateObject() = %x\n",Status);
	return(Status);
     }

   KeInitializeDispatcherHeader(&Process->Pcb.DispatcherHeader,
				InternalProcessType,
				sizeof(EPROCESS),
				FALSE);
   KProcess = &Process->Pcb;
   /* Inherit parent process's affinity. */
   KProcess->Affinity = ParentProcess->Pcb.Affinity;
   KProcess->BasePriority = PROCESS_PRIO_NORMAL;
   MmInitializeAddressSpace(Process,
			    &Process->AddressSpace);
   Process->UniqueProcessId = InterlockedIncrement(&PiNextProcessUniqueId);
   Process->InheritedFromUniqueProcessId = 
     (HANDLE)ParentProcess->UniqueProcessId;
   ObCreateHandleTable(ParentProcess,
		       InheritObjectTable,
		       Process);
   MmCopyMmInfo(ParentProcess, Process);
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   InsertHeadList(&PsProcessListHead, &Process->ProcessListEntry);
   InitializeListHead(&Process->ThreadListHead);
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
   
   Process->Pcb.State = PROCESS_STATE_ACTIVE;
   
   /*
    * Add the debug port
    */
   if (DebugPortHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(DebugPortHandle,
					   PORT_ALL_ACCESS,
					   ExPortType,
					   UserMode,
					   (PVOID*)&DebugPort,
					   NULL);   
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(Process);
	     ObDereferenceObject(ParentProcess);
	     ZwClose(*ProcessHandle);
	     *ProcessHandle = NULL;
	     return(Status);
	  }
	Process->DebugPort = DebugPort;
     }
	
   /*
    * Add the exception port
    */
   if (ExceptionPortHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(ExceptionPortHandle,
					   PORT_ALL_ACCESS,
					   ExPortType,
					   UserMode,
					   (PVOID*)&ExceptionPort,
					   NULL);   
	if (!NT_SUCCESS(Status))
	  {
	     ObDereferenceObject(Process);
	     ObDereferenceObject(ParentProcess);
	     ZwClose(*ProcessHandle);
	     *ProcessHandle = NULL;
	     return(Status);
	  }
	Process->ExceptionPort = ExceptionPort;
     }
   
   /*
    * Now we have created the process proper
    */
   
   /*
    * Create the shared data page
    */
   MmLockAddressSpace(&Process->AddressSpace);
   BaseAddress = (PVOID)USER_SHARED_DATA_BASE;
   Status = MmCreateMemoryArea(Process,
			       &Process->AddressSpace,
			       MEMORY_AREA_SHARED_DATA,
			       &BaseAddress,
			       PAGESIZE,
			       PAGE_READONLY,
			       &MemoryArea,
			       FALSE);
   MmUnlockAddressSpace(&Process->AddressSpace);
   if (!NT_SUCCESS(Status))
     {
	DPRINT1("Failed to create shared data page\n");
	KeBugCheck(0);
     }
   
   /*
    * Map ntdll
    */
   Status = LdrpMapSystemDll(*ProcessHandle,
			     &LdrStartupAddr);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("LdrpMapSystemDll failed (Status %x)\n", Status);
	ObDereferenceObject(Process);
	ObDereferenceObject(ParentProcess);
	return(Status);
     }
   
   /*
    * Map the process image
    */
   if (SectionHandle != NULL)
     {
	DPRINT("Mapping process image\n");
	Status = LdrpMapImage(*ProcessHandle,
			      SectionHandle,
			      &ImageBase);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("LdrpMapImage failed (Status %x)\n", Status);
	     ObDereferenceObject(Process);
	     ObDereferenceObject(ParentProcess);
	     return(Status);
	  }
     }
   else
     {
	ImageBase = NULL;
     }
   
   /*
    * 
    */
   DPRINT("Creating PEB\n");
   Status = PsCreatePeb(*ProcessHandle,
			ImageBase,
			&Peb);
   if (!NT_SUCCESS(Status))
     {
        DbgPrint("NtCreateProcess() Peb creation failed: Status %x\n",Status);
	ObDereferenceObject(Process);
	ObDereferenceObject(ParentProcess);
	ZwClose(*ProcessHandle);
	*ProcessHandle = NULL;
	return(Status);
     }
   Process->Peb = Peb;
   
   /*
    * Maybe send a message to the creator process's debugger
    */
#if 0
   if (ParentProcess->DebugPort != NULL)
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
	  sizeof(LPC_MESSAGE_HEADER);
	Message.Type = DBG_EVENT_CREATE_PROCESS;
	Message.Data.CreateProcess.FileHandle = FileHandle;
	Message.Data.CreateProcess.Base = ImageBase;
	Message.Data.CreateProcess.EntryPoint = NULL; //
	
	Status = LpcSendDebugMessagePort(ParentProcess->DebugPort,
					 &Message);
     }
#endif
   
   ObDereferenceObject(Process);
   ObDereferenceObject(ParentProcess);
   return(STATUS_SUCCESS);
}


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
	KIRQL oldIrql;
	PLIST_ENTRY current_entry;
	PEPROCESS current;
	NTSTATUS Status;
	
	KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
	current_entry = PsProcessListHead.Flink;
	while (current_entry != &PsProcessListHead)
	  {
	     current = CONTAINING_RECORD(current_entry, EPROCESS, 
					 ProcessListEntry);
	     if (current->UniqueProcessId == (ULONG)ClientId->UniqueProcess)
	       {
		  ObReferenceObjectByPointer(current,
					     DesiredAccess,
					     PsProcessType,
					     UserMode);
		  KeReleaseSpinLock(&PsProcessListLock, oldIrql);
		  Status = ObCreateHandle(PsGetCurrentProcess(),
					  current,
					  DesiredAccess,
					  FALSE,
					  ProcessHandle);
		  ObDereferenceObject(current);
		  DPRINT("*ProcessHandle %x\n", ProcessHandle);
		  DPRINT("NtOpenProcess() = %x\n", Status);
		  return(Status);
	       }
	     current_entry = current_entry->Flink;
	  }
	KeReleaseSpinLock(&PsProcessListLock, oldIrql);
	DPRINT("NtOpenProcess() = STATUS_UNSUCCESSFUL\n");
	return(STATUS_UNSUCCESSFUL);
     }
   return(STATUS_UNSUCCESSFUL);
}


NTSTATUS STDCALL
NtQueryInformationProcess(IN  HANDLE ProcessHandle,
			  IN  CINT ProcessInformationClass,
			  OUT PVOID ProcessInformation,
			  IN  ULONG ProcessInformationLength,
			  OUT PULONG ReturnLength)
{
   PEPROCESS Process;
   NTSTATUS Status;
   PPROCESS_BASIC_INFORMATION ProcessBasicInformationP;
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_SET_INFORMATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   switch (ProcessInformationClass)
     {
      case ProcessBasicInformation:
	ProcessBasicInformationP = (PPROCESS_BASIC_INFORMATION)
	  ProcessInformation;
	ProcessBasicInformationP->ExitStatus = Process->ExitStatus;
	ProcessBasicInformationP->PebBaseAddress = Process->Peb;
	ProcessBasicInformationP->AffinityMask = Process->Pcb.Affinity;
	ProcessBasicInformationP->UniqueProcessId =
	  Process->UniqueProcessId;
	ProcessBasicInformationP->InheritedFromUniqueProcessId =
	  (ULONG)Process->InheritedFromUniqueProcessId;
	Status = STATUS_SUCCESS;
	break;
	
      case ProcessQuotaLimits:
      case ProcessIoCounters:
      case ProcessVmCounters:
      case ProcessTimes:
      case ProcessBasePriority:
      case ProcessRaisePriority:
      case ProcessDebugPort:
      case ProcessExceptionPort:
      case ProcessAccessToken:
      case ProcessLdtInformation:
      case ProcessLdtSize:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
	
      case ProcessDefaultHardErrorMode:
	*((PULONG)ProcessInformation) = Process->DefaultHardErrorProcessing;
	break;
	
      case ProcessIoPortHandlers:
      case ProcessWorkingSetWatch:
      case ProcessUserModeIOPL:
      case ProcessEnableAlignmentFaultFixup:
	Status = STATUS_NOT_IMPLEMENTED;
	break;

      case ProcessForegroundInformation:
	((PPROCESS_PRIORITY_CLASS)ProcessInformation)->Foreground =
		FALSE; /*FIXME: how to compute it? */
      case ProcessPriorityClass:
	((PPROCESS_PRIORITY_CLASS)ProcessInformation)->PriorityClass =
		Process->PriorityClass;
	break;

      case ProcessWx86Information:
      case ProcessHandleCount:
      case ProcessAffinityMask:
      case ProcessPriorityBoost:
      case ProcessDeviceMap:
      case ProcessSessionInformation:
      case ProcessWow64Information:
	Status = STATUS_NOT_IMPLEMENTED;
	break;
	
      default:
	Status = STATUS_INVALID_INFO_CLASS;
     }
   ObDereferenceObject(Process);
   return(Status);
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
				      SeTokenType,
				      UserMode,
				      (PVOID*)&Token,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   Status = SeExchangePrimaryToken(Process, Token, &OldToken);
   if (!NT_SUCCESS(Status))
     {
	ObDereferenceObject(OldToken);
     }
   ObDereferenceObject(Token);
   return(Status);
}

NTSTATUS STDCALL
NtSetInformationProcess(IN HANDLE ProcessHandle,
			IN CINT ProcessInformationClass,
			IN PVOID ProcessInformation,
			IN ULONG ProcessInformationLength)
{
   PEPROCESS Process;
   NTSTATUS Status;
   PPROCESS_BASIC_INFORMATION ProcessBasicInformationP;
   PHANDLE ProcessAccessTokenP;
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_SET_INFORMATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   switch (ProcessInformationClass)
     {
      case ProcessBasicInformation:
	ProcessBasicInformationP = (PPROCESS_BASIC_INFORMATION)
	  ProcessInformation;
	memset(ProcessBasicInformationP, 0, sizeof(PROCESS_BASIC_INFORMATION));
	Process->Pcb.Affinity = ProcessBasicInformationP->AffinityMask;
	Status = STATUS_SUCCESS;
	break;
	
      case ProcessQuotaLimits:
      case ProcessIoCounters:
      case ProcessVmCounters:
      case ProcessTimes:
      case ProcessBasePriority:
      case ProcessRaisePriority:
      case ProcessDebugPort:
      case ProcessExceptionPort:
      case ProcessAccessToken:
	ProcessAccessTokenP = (PHANDLE)ProcessInformation;
	Status = PspAssignPrimaryToken(Process, *ProcessAccessTokenP);
	break;
	
      case ProcessImageFileName:
	memcpy(Process->ImageFileName, ProcessInformation, 8);
//	DPRINT1("Process->ImageFileName %.8s\n", Process->ImageFileName);
	Status = STATUS_SUCCESS;
	break;
	
      case ProcessLdtInformation:
      case ProcessLdtSize:
      case ProcessDefaultHardErrorMode:
      case ProcessIoPortHandlers:
      case ProcessWorkingSetWatch:
      case ProcessUserModeIOPL:
      case ProcessEnableAlignmentFaultFixup:
      case ProcessPriorityClass:
      case ProcessWx86Information:
      case ProcessHandleCount:
      case ProcessAffinityMask:
      default:
	Status = STATUS_NOT_IMPLEMENTED;
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
	KIRQL		OldIrql;
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
   KeAcquireSpinLock(&PsProcessListLock,
		     &OldIrql);

	/*
	 * Scan the process list. Since the
	 * list is circular, the guard is false
	 * after the last process.
	 */
	for (	CurrentEntryP = PsProcessListHead.Flink;
		(CurrentEntryP != & PsProcessListHead);
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
					Tcb.ThreadListEntry
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
	KeReleaseSpinLock (
		& PsProcessListLock,
		OldIrql
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

LARGE_INTEGER STDCALL
PsGetProcessExitTime(VOID)
{
  LARGE_INTEGER Li;
  Li.QuadPart = PsGetCurrentProcess()->ExitTime.QuadPart;
  return Li;
}

BOOLEAN STDCALL
PsIsThreadTerminating(IN PETHREAD Thread)
{
  return(Thread->DeadThread);
}

/* EOF */
