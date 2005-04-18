/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/psmgr.c
 * PURPOSE:         Process management
 * 
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 */

/* INCLUDES **************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

extern LARGE_INTEGER ShortPsLockDelay, PsLockTimeout;

static GENERIC_MAPPING PiProcessMapping = {
    STANDARD_RIGHTS_READ    | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
    STANDARD_RIGHTS_WRITE   | PROCESS_CREATE_PROCESS    | PROCESS_CREATE_THREAD   |
    PROCESS_VM_OPERATION    | PROCESS_VM_WRITE          | PROCESS_DUP_HANDLE      |
    PROCESS_TERMINATE       | PROCESS_SET_QUOTA         | PROCESS_SET_INFORMATION | PROCESS_SET_PORT,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    PROCESS_ALL_ACCESS};

VOID 
INIT_FUNCTION
PsInitClientIDManagment(VOID);

VOID STDCALL PspKillMostProcesses();
/* FUNCTIONS ***************************************************************/

VOID PiShutdownProcessManager(VOID)
{
   DPRINT("PiShutdownProcessManager()\n");
   
   PspKillMostProcesses();
}

VOID INIT_FUNCTION
PiInitProcessManager(VOID)
{
   PsInitJobManagment();
   PsInitProcessManagment();
   PsInitThreadManagment();
   PsInitIdleThread();
   PsInitialiseW32Call();
}

VOID 
INIT_FUNCTION
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
/**********************************************************************
 * NAME							EXPORTED
 *	PsGetVersion
 *
 * DESCRIPTION
 *	Retrieves the current OS version.
 *
 * ARGUMENTS
 *	MajorVersion	Pointer to a variable that will be set to the
 *			major version of the OS. Can be NULL.
 *
 *	MinorVersion	Pointer to a variable that will be set to the
 *			minor version of the OS. Can be NULL.
 *
 *	BuildNumber	Pointer to a variable that will be set to the
 *			build number of the OS. Can be NULL.
 *
 *	CSDVersion	Pointer to a variable that will be set to the
 *			CSD string of the OS. Can be NULL.
 *
 * RETURN VALUE
 *	TRUE	OS is a checked build.
 *	FALSE	OS is a free build.
 *
 * NOTES
 *	The DDK docs say something about a 'CmCSDVersionString'.
 *	How do we determine in the build is checked or free??
 *
 * @unimplemented
 */

BOOLEAN
STDCALL
PsGetVersion (
	PULONG		MajorVersion	OPTIONAL,
	PULONG		MinorVersion	OPTIONAL,
	PULONG		BuildNumber	OPTIONAL,
	PUNICODE_STRING	CSDVersion	OPTIONAL
	)
{
	if (MajorVersion)
		*MajorVersion = 4;

	if (MinorVersion)
		*MinorVersion = 0;

	if (BuildNumber)
		*BuildNumber = 1381;

	if (CSDVersion)
	{
		CSDVersion->Length = 0;
		CSDVersion->MaximumLength = 0;
		CSDVersion->Buffer = NULL;
#if 0
		CSDVersion->Length = CmCSDVersionString.Length;
		CSDVersion->MaximumLength = CmCSDVersionString.Maximum;
		CSDVersion->Buffer = CmCSDVersionString.Buffer;
#endif
	}

	/* FIXME: How do we determine if build is checked or free? */
	return FALSE;
}

/* EOF */
