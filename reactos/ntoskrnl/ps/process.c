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

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

PEPROCESS SystemProcess = NULL;
HANDLE SystemProcessHandle = NULL;

POBJECT_TYPE PsProcessType = NULL;

/* FUNCTIONS *****************************************************************/

VOID PsInitProcessManagment(VOID)
{
   ANSI_STRING AnsiString;
   PKPROCESS KProcess;
   
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
   
   /*
    * Initialize the system process
    */
   SystemProcess = ObCreateObject(NULL,
				  PROCESS_ALL_ACCESS,
				  NULL,
				  PsProcessType);
   KProcess = &SystemProcess->Pcb;  
   
   InitializeListHead(&(KProcess->MemoryAreaList));
   ObCreateHandleTable(NULL,FALSE,SystemProcess);
   KProcess->PageTableDirectory = get_page_directory();
   
   ObCreateHandle(SystemProcess,
		  SystemProcess,
		  PROCESS_ALL_ACCESS,
		  FALSE,
		  &SystemProcessHandle);
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

NTSTATUS STDCALL NtCreateProcess(
			   OUT PHANDLE ProcessHandle,
			   IN ACCESS_MASK DesiredAccess,
			   IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
			   IN HANDLE ParentProcessHandle,
			   IN BOOLEAN InheritObjectTable,
			   IN HANDLE SectionHandle OPTIONAL,
			   IN HANDLE DebugPort OPTIONAL,
			   IN HANDLE ExceptionPort OPTIONAL)
{
   return(ZwCreateProcess(ProcessHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  ParentProcessHandle,
			  InheritObjectTable,
			  SectionHandle,
			  DebugPort,
			  ExceptionPort));
}

NTSTATUS STDCALL ZwCreateProcess(
			   OUT PHANDLE ProcessHandle,
			   IN ACCESS_MASK DesiredAccess,
			   IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
			   IN HANDLE ParentProcessHandle,
			   IN BOOLEAN InheritObjectTable,
			   IN HANDLE SectionHandle OPTIONAL,
			   IN HANDLE DebugPort OPTIONAL,
			   IN HANDLE ExceptionPort OPTIONAL)
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
   
   DPRINT("ZwCreateProcess(ObjectAttributes %x)\n",ObjectAttributes);

   Status = ObReferenceObjectByHandle(ParentProcessHandle,
				      PROCESS_CREATE_PROCESS,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&ParentProcess,
				      NULL);

   if (Status != STATUS_SUCCESS)
     {
	DPRINT("ZwCreateProcess() = %x\n",Status);
	return(Status);
     }

   Process = ObCreateObject(ProcessHandle,
			    DesiredAccess,
			    ObjectAttributes,
			    PsProcessType);
   KeInitializeDispatcherHeader(&Process->Pcb.DispatcherHeader,
				ProcessType,
				sizeof(EPROCESS),
				FALSE);
   KProcess = &(Process->Pcb);
   
   InitializeListHead(&(KProcess->MemoryAreaList));
   ObCreateHandleTable(ParentProcess,
		       InheritObjectTable,
		       Process);
   MmCopyMmInfo(ParentProcess, Process);

   /*
    * FIXME: I don't what I'm supposed to know with a section handle
    */
   if (SectionHandle != NULL)
     {
	DbgPrint("ZwCreateProcess() non-NULL SectionHandle\n");
	return(STATUS_UNSUCCESSFUL);
     }

   Process->Pcb.ProcessState = PROCESS_STATE_ACTIVE;
   ObDereferenceObject(Process);
   ObDereferenceObject(ParentProcess);
   return(STATUS_SUCCESS);
}


NTSTATUS STDCALL NtOpenProcess (OUT PHANDLE ProcessHandle,
				IN ACCESS_MASK DesiredAccess,
				IN POBJECT_ATTRIBUTES ObjectAttributes,
				IN PCLIENT_ID ClientId)
{
   return(ZwOpenProcess(ProcessHandle,
			DesiredAccess,
			ObjectAttributes,
			ClientId));
}

NTSTATUS STDCALL ZwOpenProcess (OUT PHANDLE ProcessHandle,
				IN ACCESS_MASK DesiredAccess,
				IN POBJECT_ATTRIBUTES ObjectAttributes,
				IN PCLIENT_ID ClientId)
{
   UNIMPLEMENTED;
}

NTSTATUS STDCALL NtQueryInformationProcess(IN HANDLE ProcessHandle,
					   IN CINT ProcessInformationClass,
					   OUT PVOID ProcessInformation,
					   IN ULONG ProcessInformationLength,
					   OUT PULONG ReturnLength)
{
   return(ZwQueryInformationProcess(ProcessHandle,
				    ProcessInformationClass,
				    ProcessInformation,
				    ProcessInformationLength,
				    ReturnLength));
}

NTSTATUS STDCALL ZwQueryInformationProcess(IN HANDLE ProcessHandle,
					   IN CINT ProcessInformationClass,
					   OUT PVOID ProcessInformation,
					   IN ULONG ProcessInformationLength,
					   OUT PULONG ReturnLength)
{
//   PEPROCESS Process;
   NTSTATUS Status;
   
/*   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_QUERY_INFORMATION,
				      PsProcessType,
				      UserMode,
				      &ProcessHandle,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }*/
   
   switch (ProcessInformationClass)
     {
      case ProcessBasicInformation:
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
   return(Status);
}

NTSTATUS STDCALL NtSetInformationProcess(IN HANDLE ProcessHandle,
					 IN CINT ProcessInformationClass,
					 IN PVOID ProcessInformation,
					 IN ULONG ProcessInformationLength)
{
   return(ZwSetInformationProcess(ProcessHandle,
				  ProcessInformationClass,
				  ProcessInformation,
				  ProcessInformationLength));
}

NTSTATUS STDCALL ZwSetInformationProcess(IN HANDLE ProcessHandle,
					 IN CINT ProcessInformationClass,
					 IN PVOID ProcessInformation,
					 IN ULONG ProcessInformationLength)
{
//   PEPROCESS Process;
   NTSTATUS Status;
   
/*   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_SET_INFORMATION,
				      PsProcessType,
				      UserMode,
				      &ProcessHandle,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }*/
   
   switch (ProcessInformationClass)
     {
      case ProcessBasicInformation:
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
   return(Status);
}
