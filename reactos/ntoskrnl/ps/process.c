/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * FILE:              ntoskrnl/ps/process.c
 * PURPOSE:           Process managment
 * PROGRAMMER:        David Welch (welch@cwcom.net)
 * REVISION HISTORY:
 *              21/07/98: Created
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <internal/ob.h>
#include <internal/mm.h>
#include <internal/ke.h>
#include <internal/ps.h>
#include <string.h>
#include <internal/string.h>
#include <internal/id.h>
#include <internal/teb.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

PEPROCESS SystemProcess = NULL;
HANDLE SystemProcessHandle = NULL;

POBJECT_TYPE PsProcessType = NULL;

static LIST_ENTRY PsProcessListHead;
static KSPIN_LOCK PsProcessListLock;
static ULONG PiNextProcessUniqueId = 0;

/* FUNCTIONS *****************************************************************/

VOID PsInitProcessManagment(VOID)
{
   ANSI_STRING AnsiString;
   PKPROCESS KProcess;
   KIRQL oldIrql;
   
   /*
    * Register the process object type
    */   
   
   PsProcessType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
   
   PsProcessType->TotalObjects = 0;
   PsProcessType->TotalHandles = 0;
   PsProcessType->MaxObjects = ULONG_MAX;
   PsProcessType->MaxHandles = ULONG_MAX;
   PsProcessType->PagedPoolCharge = 0;
   PsProcessType->NonpagedPoolCharge = sizeof(EPROCESS);
   PsProcessType->Dump = NULL;
   PsProcessType->Open = NULL;
   PsProcessType->Close = NULL;
   PsProcessType->Delete = PiDeleteProcess;
   PsProcessType->Parse = NULL;
   PsProcessType->Security = NULL;
   PsProcessType->QueryName = NULL;
   PsProcessType->OkayToClose = NULL;
   
   RtlInitAnsiString(&AnsiString,"Process");
   RtlAnsiStringToUnicodeString(&PsProcessType->TypeName,&AnsiString,TRUE);
   
   InitializeListHead(&PsProcessListHead);
   KeInitializeSpinLock(&PsProcessListLock);
   
   /*
    * Initialize the system process
    */
   SystemProcess = ObCreateObject(NULL,
				  PROCESS_ALL_ACCESS,
				  NULL,
				  PsProcessType);
   KeInitializeDispatcherHeader(&SystemProcess->Pcb.DispatcherHeader,
				ID_PROCESS_OBJECT,
				sizeof(EPROCESS),
				FALSE);
   KProcess = &SystemProcess->Pcb;  
   
   InitializeListHead(&(KProcess->MemoryAreaList));
   ObCreateHandleTable(NULL,FALSE,SystemProcess);
   KProcess->PageTableDirectory = get_page_directory();
   SystemProcess->UniqueProcessId = 
     InterlockedIncrement(&PiNextProcessUniqueId);
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   InsertHeadList(&PsProcessListHead, &KProcess->ProcessListEntry);
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
   
   ObCreateHandle(SystemProcess,
		  SystemProcess,
		  PROCESS_ALL_ACCESS,
		  FALSE,
		  &SystemProcessHandle);
}

VOID PiDeleteProcess(PVOID ObjectBody)
{
   KIRQL oldIrql;
   
   DPRINT("PiDeleteProcess(ObjectBody %x)\n",ObjectBody);
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   RemoveEntryList(&((PEPROCESS)ObjectBody)->Pcb.ProcessListEntry);
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);
   (VOID)MmReleaseMmInfo((PEPROCESS)ObjectBody);
}


static NTSTATUS
PsCreatePeb(HANDLE ProcessHandle)
{
   NTSTATUS Status;
   PVOID PebBase;
   ULONG PebSize;
   NT_PEB Peb;
   ULONG BytesWritten;
   
   PebBase = (PVOID)PEB_BASE;
   PebSize = 0x1000;
   Status = NtAllocateVirtualMemory(ProcessHandle,
				    &PebBase,
				    0,
				    &PebSize,
				    MEM_COMMIT,
				    PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   memset(&Peb, 0, sizeof(Peb));

   ZwWriteVirtualMemory(ProcessHandle,
			(PVOID)PEB_BASE,
			&Peb,
			sizeof(Peb),
			&BytesWritten);

   DbgPrint ("PsCreatePeb: Peb created at %x\n", PebBase);
//   DPRINT("PsCreatePeb: Peb created at %x\n", PebBase);

   return(STATUS_SUCCESS);
}


PKPROCESS KeGetCurrentProcess(VOID)
/*
 * FUNCTION: Returns a pointer to the current process
 */
{
   return(&(PsGetCurrentProcess()->Pcb));
}

struct _EPROCESS* PsGetCurrentProcess(VOID)
/*
 * FUNCTION: Returns a pointer to the current process
 */
{
   if (PsGetCurrentThread() == NULL || 
       PsGetCurrentThread()->ThreadsProcess == NULL)
     {
	return(SystemProcess);
     }
   else
     {
	return(PsGetCurrentThread()->ThreadsProcess);
     }
}

NTSTATUS
STDCALL
NtCreateProcess (
	OUT	PHANDLE			ProcessHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes	OPTIONAL,
	IN	HANDLE			ParentProcessHandle,
	IN	BOOLEAN			InheritObjectTable,
	IN	HANDLE			SectionHandle		OPTIONAL,
	IN	HANDLE			DebugPort		OPTIONAL,
	IN	HANDLE			ExceptionPort		OPTIONAL
	)
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
   
   DPRINT("NtCreateProcess(ObjectAttributes %x)\n",ObjectAttributes);

   Status = ObReferenceObjectByHandle(ParentProcessHandle,
				      PROCESS_CREATE_PROCESS,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&ParentProcess,
				      NULL);

   if (Status != STATUS_SUCCESS)
     {
	DPRINT("NtCreateProcess() = %x\n",Status);
	return(Status);
     }

   Process = ObCreateObject(ProcessHandle,
			    DesiredAccess,
			    ObjectAttributes,
			    PsProcessType);
   KeInitializeDispatcherHeader(&Process->Pcb.DispatcherHeader,
				ID_PROCESS_OBJECT,
				sizeof(EPROCESS),
				FALSE);
   KProcess = &(Process->Pcb);
   
   InitializeListHead(&(KProcess->MemoryAreaList));
   Process->UniqueProcessId = InterlockedIncrement(&PiNextProcessUniqueId);
   Process->InheritedFromUniqueProcessId = ParentProcess->UniqueProcessId;
   ObCreateHandleTable(ParentProcess,
		       InheritObjectTable,
		       Process);
   MmCopyMmInfo(ParentProcess, Process);
   Process->UniqueProcessId = InterlockedIncrement(&PiNextProcessUniqueId);
   
   KeAcquireSpinLock(&PsProcessListLock, &oldIrql);
   InsertHeadList(&PsProcessListHead, &KProcess->ProcessListEntry);
   KeReleaseSpinLock(&PsProcessListLock, oldIrql);

   Status = PsCreatePeb (*ProcessHandle);
   if (!NT_SUCCESS(Status))
     {
//        DPRINT("NtCreateProcess() Peb creation failed: Status %x\n",Status);
        DbgPrint ("NtCreateProcess() Peb creation failed: Status %x\n",Status);
	return(Status);
     }

   /*
    * FIXME: I don't what I'm supposed to know with a section handle
    */
   if (SectionHandle != NULL)
     {
	DbgPrint("NtCreateProcess() non-NULL SectionHandle\n");
	return(STATUS_UNSUCCESSFUL);
     }

   Process->Pcb.ProcessState = PROCESS_STATE_ACTIVE;
   ObDereferenceObject(Process);
   ObDereferenceObject(ParentProcess);
   return(STATUS_SUCCESS);
}


NTSTATUS
STDCALL
NtOpenProcess (
	OUT	PHANDLE			ProcessHandle,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes,
	IN	PCLIENT_ID		ClientId
	)
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
					 Pcb.ProcessListEntry);
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


NTSTATUS
STDCALL
NtQueryInformationProcess (
	IN	HANDLE	ProcessHandle,
	IN	CINT	ProcessInformationClass,
	OUT	PVOID	ProcessInformation,
	IN	ULONG	ProcessInformationLength,
	OUT	PULONG	ReturnLength
	)
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
	memset(ProcessBasicInformationP, 0, sizeof(PROCESS_BASIC_INFORMATION));
	ProcessBasicInformationP->AffinityMask = Process->Pcb.Affinity;
        ProcessBasicInformationP->UniqueProcessId =
          Process->UniqueProcessId;
        ProcessBasicInformationP->InheritedFromUniqueProcessId =
          Process->InheritedFromUniqueProcessId;
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


NTSTATUS
STDCALL
NtSetInformationProcess (
	IN	HANDLE	ProcessHandle,
	IN	CINT	ProcessInformationClass,
	IN	PVOID	ProcessInformation,
	IN	ULONG	ProcessInformationLength
	)
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
