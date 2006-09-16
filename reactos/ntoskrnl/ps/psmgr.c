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

extern LARGE_INTEGER ShortPsLockDelay;
extern LIST_ENTRY PriorityListHead[MAXIMUM_PRIORITY];

GENERIC_MAPPING PspProcessMapping =
{
    STANDARD_RIGHTS_READ    | PROCESS_QUERY_INFORMATION | PROCESS_VM_READ,
    STANDARD_RIGHTS_WRITE   | PROCESS_CREATE_PROCESS    | PROCESS_CREATE_THREAD   |
    PROCESS_VM_OPERATION    | PROCESS_VM_WRITE          | PROCESS_DUP_HANDLE      |
    PROCESS_TERMINATE       | PROCESS_SET_QUOTA         | PROCESS_SET_INFORMATION |
    PROCESS_SUSPEND_RESUME,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    PROCESS_ALL_ACCESS
};

GENERIC_MAPPING PspThreadMapping =
{
    STANDARD_RIGHTS_READ    | THREAD_GET_CONTEXT      | THREAD_QUERY_INFORMATION,
    STANDARD_RIGHTS_WRITE   | THREAD_TERMINATE        | THREAD_SUSPEND_RESUME    |
    THREAD_ALERT            | THREAD_SET_INFORMATION  | THREAD_SET_CONTEXT,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    THREAD_ALL_ACCESS
};

extern ULONG NtBuildNumber;
extern ULONG NtMajorVersion;
extern ULONG NtMinorVersion;
extern PVOID KeUserApcDispatcher;
extern PVOID KeUserCallbackDispatcher;
extern PVOID KeUserExceptionDispatcher;
extern PVOID KeRaiseUserExceptionDispatcher;

PVOID PspSystemDllBase;
PVOID PspSystemDllSection;
PVOID PspSystemDllEntryPoint;

PHANDLE_TABLE PspCidTable;

PEPROCESS PsInitialSystemProcess = NULL;
PEPROCESS PsIdleProcess = NULL;
HANDLE PspInitialSystemProcessHandle;

ULONG PsMinimumWorkingSet, PsMaximumWorkingSet;
struct
{
    LIST_ENTRY List;
    KGUARDED_MUTEX Lock;
} PspWorkingSetChangeHead;
ULONG PspDefaultPagedLimit, PspDefaultNonPagedLimit, PspDefaultPagefileLimit;
BOOLEAN PspDoingGiveBacks;

extern PTOKEN PspBootAccessToken;

extern GENERIC_MAPPING PspJobMapping;
extern POBJECT_TYPE PsJobType;

VOID
NTAPI
PspInitializeJobStructures(VOID);

VOID
NTAPI
PspDeleteJob(IN PVOID ObjectBody);

/* FUNCTIONS ***************************************************************/

NTSTATUS
NTAPI
PspCreateProcess(OUT PHANDLE ProcessHandle,
                 IN ACCESS_MASK DesiredAccess,
                 IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                 IN HANDLE ParentProcess OPTIONAL,
                 IN ULONG Flags,
                 IN HANDLE SectionHandle OPTIONAL,
                 IN HANDLE DebugPort OPTIONAL,
                 IN HANDLE ExceptionPort OPTIONAL,
                 IN BOOLEAN InJob);

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
ExPhase2Init(
    IN PVOID Context
);

BOOLEAN
NTAPI
PspInitPhase0(VOID)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE SysThreadHandle;
    PETHREAD SysThread;
    MM_SYSTEMSIZE SystemSize;
    UNICODE_STRING Name;
    ULONG i;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;

    /* FIXME: Initialize Lock Data do it STATIC */
    ShortPsLockDelay.QuadPart = -100LL;

    /* Get the system size */
    SystemSize = MmQuerySystemSize();

    /* Setup some memory options */
    PspDefaultPagefileLimit = -1;
    switch (SystemSize)
    {
        /* Medimum systems */
        case MmMediumSystem:

            /* Increase the WS sizes a bit */
            PsMinimumWorkingSet += 10;
            PsMaximumWorkingSet += 100;

        /* Large systems */
        case MmLargeSystem:

            /* Increase the WS sizes a bit more */
            PsMinimumWorkingSet += 30;
            PsMaximumWorkingSet += 300;

        /* Small and other systems */
        default:
            break;
    }

    /* Setup the quantum table */
    PsChangeQuantumTable(FALSE, PsRawPrioritySeparation);

    /* Setup callbacks when we implement Generic Callbacks */

    /* Set quota settings */
    if (!PspDefaultPagedLimit) PspDefaultPagedLimit = 0;
    if (!PspDefaultNonPagedLimit) PspDefaultNonPagedLimit = 0;
    if (!(PspDefaultNonPagedLimit) && !(PspDefaultPagedLimit))
    {
        /* Enable give-backs */
        PspDoingGiveBacks = TRUE;
    }
    else
    {
        /* Disable them */
        PspDoingGiveBacks = FALSE;
    }

    /* Now multiply limits by 1MB */
    PspDefaultPagedLimit <<= 20;
    PspDefaultNonPagedLimit <<= 20;
    if (PspDefaultPagefileLimit != -1) PspDefaultPagefileLimit <<= 20;

    /* Initialize the Active Process List */
    InitializeListHead(&PsActiveProcessHead);
    KeInitializeGuardedMutex(&PspActiveProcessMutex);

    /* Get the idle process */
    PsIdleProcess = PsGetCurrentProcess();

    /* Setup the locks */
    PsIdleProcess->ProcessLock.Value = 0;
    ExInitializeRundownProtection(&PsIdleProcess->RundownProtect);

    /* Initialize the thread list */
    InitializeListHead(&PsIdleProcess->ThreadListHead);

    /* Clear kernel time */
    PsIdleProcess->Pcb.KernelTime = 0;

    /* Initialize the Process type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Process");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(EPROCESS);
    ObjectTypeInitializer.GenericMapping = PspProcessMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = PROCESS_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = PspDeleteProcess;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &PsProcessType);

    /* Setup ROS Scheduler lists (HACK!) */
    for (i = 0; i < MAXIMUM_PRIORITY; i++)
    {
        InitializeListHead(&PriorityListHead[i]);
    }

    /*  Initialize the Thread type  */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Thread");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(ETHREAD);
    ObjectTypeInitializer.GenericMapping = PspThreadMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = THREAD_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = PspDeleteThread;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &PsThreadType);

    /*  Initialize the Job type  */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Job");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(EJOB);
    ObjectTypeInitializer.GenericMapping = PspJobMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = JOB_OBJECT_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.DeleteProcedure = PspDeleteJob;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &PsJobType);

    /* Initialize job structures external to this file */
    PspInitializeJobStructures();

    /* Initialize the Working Set data */
    InitializeListHead(&PspWorkingSetChangeHead.List);
    KeInitializeGuardedMutex(&PspWorkingSetChangeHead.Lock);

    /* Create the CID Handle table */
    PspCidTable = ExCreateHandleTable(NULL);

    /* FIXME: Initialize LDT/VDM support */

    /* Setup the reaper */
    ExInitializeWorkItem(&PspReaperWorkItem, PspReapRoutine, NULL);

    /* Set the boot access token */
    PspBootAccessToken = (PTOKEN)(PsIdleProcess->Token.Value & ~MAX_FAST_REFS);

    /* Setup default object attributes */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);

    /* Create the Initial System Process */
    Status = PspCreateProcess(&PspInitialSystemProcessHandle,
                              PROCESS_ALL_ACCESS,
                              &ObjectAttributes,
                              0,
                              FALSE,
                              0,
                              0,
                              0,
                              FALSE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Get a reference to it */
    ObReferenceObjectByHandle(PspInitialSystemProcessHandle,
                              0,
                              PsProcessType,
                              KernelMode,
                              (PVOID*)&PsInitialSystemProcess,
                              NULL);

    /* The PD we gave it is invalid at this point, do what old ROS did */
    PsInitialSystemProcess->Pcb.DirectoryTableBase = (LARGE_INTEGER)
                                                     (LONGLONG)
                                                     (ULONG)MmGetPageDirectory();
    PsIdleProcess->Pcb.DirectoryTableBase = PsInitialSystemProcess->Pcb.DirectoryTableBase;

    /* Copy the process names */
    strcpy(PsIdleProcess->ImageFileName, "Idle");
    strcpy(PsInitialSystemProcess->ImageFileName, "System");

    /* Allocate a structure for the audit name */
    PsIdleProcess->SeAuditProcessCreationInfo.ImageFileName =
        ExAllocatePoolWithTag(PagedPool, sizeof(UNICODE_STRING), TAG_SEPA);
    if (!PsIdleProcess->SeAuditProcessCreationInfo.ImageFileName) KEBUGCHECK(0);

    /* Setup the system initailization thread */
    Status = PsCreateSystemThread(&SysThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  0,
                                  NULL,
                                  ExPhase2Init,
                                  NULL);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Create a handle to it */
    ObReferenceObjectByHandle(SysThreadHandle,
                              0,
                              PsThreadType,
                              KernelMode,
                              (PVOID*)&SysThread,
                              NULL);
    ZwClose(SysThreadHandle);

    /* Return success */
    return TRUE;
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
