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

#define LockEvent Spare0[0]
#define LockCount Spare0[1]
#define LockOwner Spare0[2]

extern LARGE_INTEGER ShortPsLockDelay, PsLockTimeout;
extern LIST_ENTRY PriorityListHead[MAXIMUM_PRIORITY];

static GENERIC_MAPPING PiProcessMapping = {
    STANDARD_RIGHTS_READ    | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
    STANDARD_RIGHTS_WRITE   | PROCESS_CREATE_PROCESS    | PROCESS_CREATE_THREAD   |
    PROCESS_VM_OPERATION    | PROCESS_VM_WRITE          | PROCESS_DUP_HANDLE      |
    PROCESS_TERMINATE       | PROCESS_SET_QUOTA         | PROCESS_SET_INFORMATION |
    PROCESS_SUSPEND_RESUME,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    PROCESS_ALL_ACCESS};

static GENERIC_MAPPING PiThreadMapping = {
    STANDARD_RIGHTS_READ    | THREAD_GET_CONTEXT      | THREAD_QUERY_INFORMATION,
    STANDARD_RIGHTS_WRITE   | THREAD_TERMINATE        | THREAD_SUSPEND_RESUME    |
    THREAD_ALERT            | THREAD_SET_INFORMATION  | THREAD_SET_CONTEXT,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    THREAD_ALL_ACCESS};

extern ULONG NtBuildNumber;
extern ULONG NtMajorVersion;
extern ULONG NtMinorVersion;
extern PVOID KeUserApcDispatcher;
extern PVOID KeUserCallbackDispatcher;
extern PVOID KeUserExceptionDispatcher;
extern PVOID KeRaiseUserExceptionDispatcher;

PVOID PspSystemDllBase = NULL;
PVOID PspSystemDllSection = NULL;
PVOID PspSystemDllEntryPoint = NULL;
PHANDLE_TABLE PspCidTable = NULL;
VOID STDCALL PspKillMostProcesses();
VOID INIT_FUNCTION NTAPI PsInitClientIDManagment(VOID);
NTSTATUS STDCALL INIT_FUNCTION PspLookupKernelUserEntryPoints(VOID);

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, PiInitProcessManager)
#pragma alloc_text(INIT, PsInitClientIDManagment)
#pragma alloc_text(INIT, PsInitThreadManagment)
#pragma alloc_text(INIT, PsInitProcessManagment)
#pragma alloc_text(INIT, PspLookupKernelUserEntryPoints)
#pragma alloc_text(INIT, PsLocateSystemDll)
#endif

/* FUNCTIONS ***************************************************************/

VOID
FASTCALL
KiIdleLoop(VOID);

/* FUNCTIONS *****************************************************************/

/*
 * HACK-O-RAMA
 * Antique vestigial code left alive for the sole purpose of First/Idle Thread
 * creation until I can merge my fix for properly creating them.
 */
VOID
INIT_FUNCTION
NTAPI
PsInitHackThread(VOID)
{
    PETHREAD IdleThread;

    IdleThread = ExAllocatePool(NonPagedPool, sizeof(ETHREAD));
    RtlZeroMemory(IdleThread, sizeof(ETHREAD));
    IdleThread->ThreadsProcess = PsIdleProcess;
    KeInitializeThread(&PsIdleProcess->Pcb,
                       &IdleThread->Tcb,
                       PspSystemThreadStartup,
                       (PVOID)KiIdleLoop,
                       NULL,
                       NULL,
                       NULL,
                       (PVOID)((ULONG_PTR)MmCreateKernelStack(FALSE) +
                              KERNEL_STACK_SIZE));
    InitializeListHead(&IdleThread->IrpList);

    KeReadyThread(&IdleThread->Tcb);

    KeGetCurrentPrcb()->IdleThread = &IdleThread->Tcb;
    KeSetPriorityThread(&IdleThread->Tcb, LOW_PRIORITY);
    KeSetAffinityThread(&IdleThread->Tcb, 1 << 0);
}

/*
 * HACK-O-RAMA
 * Antique vestigial code left alive for the sole purpose of First/Idle Thread
 * creation until I can merge my fix for properly creating them.
 */
VOID
INIT_FUNCTION
NTAPI
PsInitHackThread2(IN PETHREAD *Hack)
{
    PETHREAD IdleThread;

    IdleThread = ExAllocatePool(NonPagedPool, sizeof(ETHREAD));
    RtlZeroMemory(IdleThread, sizeof(ETHREAD));
    IdleThread->ThreadsProcess = PsInitialSystemProcess;
    KeInitializeThread(&PsInitialSystemProcess->Pcb,
                       &IdleThread->Tcb,
                       PspSystemThreadStartup,
                       NULL,
                       NULL,
                       NULL,
                       NULL,
                       P0BootStack);
    InitializeListHead(&IdleThread->IrpList);
    *Hack = IdleThread;
}

VOID
INIT_FUNCTION
NTAPI
PiInitProcessManager(VOID)
{
   PsInitJobManagment();
   PsInitProcessManagment();
   PsInitThreadManagment();
   PsInitHackThread();
}

VOID
INIT_FUNCTION
NTAPI
PsInitClientIDManagment(VOID)
{
  PspCidTable = ExCreateHandleTable(NULL);
  ASSERT(PspCidTable);
}

VOID
INIT_FUNCTION
NTAPI
PsInitThreadManagment(VOID)
/*
 * FUNCTION: Initialize thread managment
 */
{
   UNICODE_STRING Name;
   OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
   PETHREAD FirstThread;
   ULONG i;

   for (i=0; i < MAXIMUM_PRIORITY; i++)
     {
	InitializeListHead(&PriorityListHead[i]);
     }

    DPRINT("Creating Thread Object Type\n");
  
    /*  Initialize the Thread type  */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Thread");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(ETHREAD);
    ObjectTypeInitializer.GenericMapping = PiThreadMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = THREAD_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = PspDeleteThread;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &PsThreadType);

   PsInitHackThread2(&FirstThread);
   FirstThread->Tcb.State = Running;
   FirstThread->Tcb.FreezeCount = 0;
   FirstThread->Tcb.UserAffinity = (1 << 0);   /* Set the affinity of the first thread to the boot processor */
   FirstThread->Tcb.Affinity = (1 << 0);
   KeGetCurrentPrcb()->CurrentThread = (PVOID)FirstThread;

   DPRINT("FirstThread %x\n",FirstThread);

   ExInitializeWorkItem(&PspReaperWorkItem, PspReapRoutine, NULL);
}

VOID
INIT_FUNCTION
NTAPI
PsInitProcessManagment(VOID)
{
   PKPROCESS KProcess;
   NTSTATUS Status;
   UNICODE_STRING Name;
   OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;

   ShortPsLockDelay.QuadPart = -100LL;

   /*
    * Register the process object type
    */

    DPRINT("Creating Process Object Type\n");
  
   /*  Initialize the Process type */
   RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
   RtlInitUnicodeString(&Name, L"Process");
   ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
   ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(EPROCESS);
   ObjectTypeInitializer.GenericMapping = PiProcessMapping;
   ObjectTypeInitializer.PoolType = NonPagedPool;
   ObjectTypeInitializer.ValidAccessMask = PROCESS_ALL_ACCESS;
   ObjectTypeInitializer.DeleteProcedure = PspDeleteProcess;
   ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &PsProcessType);

   InitializeListHead(&PsActiveProcessHead);
   KeInitializeGuardedMutex(&PspActiveProcessMutex);

   /* Setup the quantum table */
   PsChangeQuantumTable(FALSE, PsRawPrioritySeparation);

   /*
    * Initialize the default quota block.
    */

   RtlZeroMemory(&PspDefaultQuotaBlock, sizeof(PspDefaultQuotaBlock));
   PspDefaultQuotaBlock.QuotaEntry[PagedPool].Limit = (SIZE_T)-1;
   PspDefaultQuotaBlock.QuotaEntry[NonPagedPool].Limit = (SIZE_T)-1;
   PspDefaultQuotaBlock.QuotaEntry[2].Limit = (SIZE_T)-1; /* Page file */

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
   PsIdleProcess->Pcb.BasePriority = PROCESS_PRIORITY_IDLE;
   PsIdleProcess->Pcb.QuantumReset = 6;
   InitializeListHead(&PsIdleProcess->Pcb.ThreadListHead);
   InitializeListHead(&PsIdleProcess->ThreadListHead);
   InitializeListHead(&PsIdleProcess->ActiveProcessLinks);
   ObInitializeFastReference(&PsIdleProcess->Token, NULL);
   KeInitializeDispatcherHeader(&PsIdleProcess->Pcb.Header,
				ProcessObject,
				sizeof(EPROCESS) / sizeof(LONG),
				FALSE);
   PsIdleProcess->Pcb.DirectoryTableBase.QuadPart = (ULONG_PTR)MmGetPageDirectory();
   strcpy(PsIdleProcess->ImageFileName, "Idle");
   PspInheritQuota(PsIdleProcess, NULL);

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
   RtlZeroMemory(PsInitialSystemProcess, sizeof(EPROCESS));
#ifdef CONFIG_SMP   
   /* FIXME:
    *   Only the boot cpu is initialized. Threads of the 
    *   system process should be able to run on all cpus.
    */
   PsInitialSystemProcess->Pcb.Affinity = 0xffffffff;
#else
   PsInitialSystemProcess->Pcb.Affinity = KeActiveProcessors;
#endif   
   PsInitialSystemProcess->Pcb.IopmOffset = 0xffff;
   PsInitialSystemProcess->Pcb.BasePriority = PROCESS_PRIORITY_NORMAL;
   PsInitialSystemProcess->Pcb.QuantumReset = 6;
   InitializeListHead(&PsInitialSystemProcess->Pcb.ThreadListHead);
   KeInitializeDispatcherHeader(&PsInitialSystemProcess->Pcb.Header,
				ProcessObject,
				sizeof(EPROCESS) / sizeof(LONG),
				FALSE);
   KProcess = &PsInitialSystemProcess->Pcb;
   PspInheritQuota(PsInitialSystemProcess, NULL);

   MmInitializeAddressSpace(PsInitialSystemProcess,
			    (PMADDRESS_SPACE)&(PsInitialSystemProcess)->VadRoot);

   (PsInitialSystemProcess)->LockEvent = 
       ExAllocatePoolWithTag(PagedPool, sizeof(KEVENT), TAG('P', 's', 'L', 'k'));
   KeInitializeEvent((PsInitialSystemProcess)->LockEvent, SynchronizationEvent, FALSE);

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
		  &PsInitialSystemProcess->ActiveProcessLinks);
   InitializeListHead(&PsInitialSystemProcess->ThreadListHead);

#ifndef SCHED_REWRITE
    {
    PTOKEN BootToken;

    /* No parent, this is the Initial System Process. Assign Boot Token */
    BootToken = SepCreateSystemProcessToken();
    BootToken->TokenInUse = TRUE;
    ObInitializeFastReference(&PsInitialSystemProcess->Token, BootToken);
	}
#endif
}

VOID
PspPostInitSystemProcess(VOID)
{
  HANDLE_TABLE_ENTRY CidEntry;

  /* this routine is called directly after the exectuive handle tables were
     initialized. We'll set up the Client ID handle table and assign the system
     process a PID */
  PsInitClientIDManagment();

  ObpCreateHandleTable(NULL, PsInitialSystemProcess);
  ObpKernelHandleTable = PsInitialSystemProcess->ObjectTable;

  CidEntry.Object = PsInitialSystemProcess;
  CidEntry.GrantedAccess = 0;
  PsInitialSystemProcess->UniqueProcessId = ExCreateHandle(PspCidTable, &CidEntry);

  if(!PsInitialSystemProcess->UniqueProcessId)
  {
    DPRINT1("Failed to create CID handle (unique process id) for the system process!\n");
    KEBUGCHECK(0);
  }
}

NTSTATUS
STDCALL
INIT_FUNCTION
PspLookupKernelUserEntryPoints(VOID)
{
    ANSI_STRING ProcedureName;
    NTSTATUS Status;

    /* Retrieve ntdll's startup address */
    DPRINT("Getting Entrypoint: %p\n", PspSystemDllBase);
    RtlInitAnsiString(&ProcedureName, "LdrInitializeThunk");
    Status = LdrGetProcedureAddress((PVOID)PspSystemDllBase,
                                    &ProcedureName,
                                    0,
                                    &PspSystemDllEntryPoint);

    if (!NT_SUCCESS(Status)) {

        DPRINT1 ("LdrGetProcedureAddress failed (Status %x)\n", Status);
        return (Status);
    }

    /* Get User APC Dispatcher */
    DPRINT("Getting Entrypoint\n");
    RtlInitAnsiString(&ProcedureName, "KiUserApcDispatcher");
    Status = LdrGetProcedureAddress((PVOID)PspSystemDllBase,
                                    &ProcedureName,
                                    0,
                                    &KeUserApcDispatcher);

    if (!NT_SUCCESS(Status)) {

        DPRINT1 ("LdrGetProcedureAddress failed (Status %x)\n", Status);
        return (Status);
    }

    /* Get Exception Dispatcher */
    DPRINT("Getting Entrypoint\n");
    RtlInitAnsiString(&ProcedureName, "KiUserExceptionDispatcher");
    Status = LdrGetProcedureAddress((PVOID)PspSystemDllBase,
                                    &ProcedureName,
                                    0,
                                    &KeUserExceptionDispatcher);

    if (!NT_SUCCESS(Status)) {

        DPRINT1 ("LdrGetProcedureAddress failed (Status %x)\n", Status);
        return (Status);
    }

    /* Get Callback Dispatcher */
    DPRINT("Getting Entrypoint\n");
    RtlInitAnsiString(&ProcedureName, "KiUserCallbackDispatcher");
    Status = LdrGetProcedureAddress((PVOID)PspSystemDllBase,
                                    &ProcedureName,
                                    0,
                                    &KeUserCallbackDispatcher);

    if (!NT_SUCCESS(Status)) {

        DPRINT1 ("LdrGetProcedureAddress failed (Status %x)\n", Status);
        return (Status);
    }

    /* Get Raise Exception Dispatcher */
    DPRINT("Getting Entrypoint\n");
    RtlInitAnsiString(&ProcedureName, "KiRaiseUserExceptionDispatcher");
    Status = LdrGetProcedureAddress((PVOID)PspSystemDllBase,
                                    &ProcedureName,
                                    0,
                                    &KeRaiseUserExceptionDispatcher);

    if (!NT_SUCCESS(Status)) {

        DPRINT1 ("LdrGetProcedureAddress failed (Status %x)\n", Status);
        return (Status);
    }

    /* Return success */
    return(STATUS_SUCCESS);
}

NTSTATUS
STDCALL
PspMapSystemDll(PEPROCESS Process,
                PVOID *DllBase)
{
    NTSTATUS Status;
    SIZE_T ViewSize = 0;
    PVOID ImageBase = 0;

    /* Map the System DLL */
    DPRINT("Mapping System DLL\n");
    Status = MmMapViewOfSection(PspSystemDllSection,
                                Process,
                                (PVOID*)&ImageBase,
                                0,
                                0,
                                NULL,
                                &ViewSize,
                                0,
                                MEM_COMMIT,
                                PAGE_READWRITE);

    if (!NT_SUCCESS(Status)) {

        DPRINT1("Failed to map System DLL Into Process\n");
    }

    if (DllBase) *DllBase = ImageBase;

    return Status;
}

NTSTATUS
STDCALL
INIT_FUNCTION
PsLocateSystemDll(VOID)
{
    UNICODE_STRING DllPathname = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\ntdll.dll");
    OBJECT_ATTRIBUTES FileObjectAttributes;
    IO_STATUS_BLOCK Iosb;
    HANDLE FileHandle;
    HANDLE NTDllSectionHandle;
    NTSTATUS Status;
    CHAR BlockBuffer[1024];
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NTHeaders;

    /* Locate and open NTDLL to determine ImageBase and LdrStartup */
    InitializeObjectAttributes(&FileObjectAttributes,
                               &DllPathname,
                               0,
                               NULL,
                               NULL);

    DPRINT("Opening NTDLL\n");
    Status = ZwOpenFile(&FileHandle,
                        FILE_READ_ACCESS,
                        &FileObjectAttributes,
                        &Iosb,
                        FILE_SHARE_READ,
                        FILE_SYNCHRONOUS_IO_NONALERT);

    if (!NT_SUCCESS(Status)) {
        DPRINT1("NTDLL open failed (Status %x)\n", Status);
        return Status;
     }

     /* Load NTDLL is valid */
     DPRINT("Reading NTDLL\n");
     Status = ZwReadFile(FileHandle,
                         0,
                         0,
                         0,
                         &Iosb,
                         BlockBuffer,
                         sizeof(BlockBuffer),
                         0,
                         0);
    if (!NT_SUCCESS(Status) || Iosb.Information != sizeof(BlockBuffer)) {

        DPRINT1("NTDLL header read failed (Status %x)\n", Status);
        ZwClose(FileHandle);
        return Status;
    }

    /* Check if it's valid */
    DosHeader = (PIMAGE_DOS_HEADER)BlockBuffer;
    NTHeaders = (PIMAGE_NT_HEADERS)(BlockBuffer + DosHeader->e_lfanew);

    if ((DosHeader->e_magic != IMAGE_DOS_SIGNATURE) ||
        (DosHeader->e_lfanew == 0L) ||
        (*(PULONG) NTHeaders != IMAGE_NT_SIGNATURE)) {

        DPRINT1("NTDLL format invalid\n");
        ZwClose(FileHandle);
        return(STATUS_UNSUCCESSFUL);
    }

    /* Create a section for NTDLL */
    DPRINT("Creating section\n");
    Status = ZwCreateSection(&NTDllSectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             NULL,
                             PAGE_READONLY,
                             SEC_IMAGE | SEC_COMMIT,
                             FileHandle);
    if (!NT_SUCCESS(Status)) {

        DPRINT1("NTDLL create section failed (Status %x)\n", Status);
        ZwClose(FileHandle);
        return(Status);
    }
    ZwClose(FileHandle);

    /* Reference the Section */
    DPRINT("ObReferenceObjectByHandle section: %d\n", NTDllSectionHandle);
    Status = ObReferenceObjectByHandle(NTDllSectionHandle,
                                       SECTION_ALL_ACCESS,
                                       MmSectionObjectType,
                                       KernelMode,
                                       (PVOID*)&PspSystemDllSection,
                                       NULL);
    if (!NT_SUCCESS(Status)) {

        DPRINT1("NTDLL section reference failed (Status %x)\n", Status);
        return(Status);
    }

    /* Map it */
    PspMapSystemDll(PsGetCurrentProcess(), &PspSystemDllBase);
    DPRINT("LdrpSystemDllBase: %x\n", PspSystemDllBase);

    /* Now get the Entrypoints */
    PspLookupKernelUserEntryPoints();

    return STATUS_SUCCESS;
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
 *
 * @implemented
 */
BOOLEAN
STDCALL
PsGetVersion(PULONG MajorVersion OPTIONAL,
             PULONG MinorVersion OPTIONAL,
             PULONG BuildNumber OPTIONAL,
             PUNICODE_STRING CSDVersion OPTIONAL)
{
    if (MajorVersion)
        *MajorVersion = NtMajorVersion;

    if (MinorVersion)
        *MinorVersion = NtMinorVersion;

    if (BuildNumber)
        *BuildNumber = NtBuildNumber;

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

    /* Check the High word */
    return (NtBuildNumber >> 28) == 0xC;
}

/* EOF */
