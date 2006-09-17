/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/psmgr.c
 * PURPOSE:         Process Manager: Initialization Code
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

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

PVOID PspSystemDllBase;
PVOID PspSystemDllSection;
PVOID PspSystemDllEntryPoint;

ANSI_STRING ThunkName = RTL_CONSTANT_STRING("LdrInitializeThunk");
ANSI_STRING ApcName = RTL_CONSTANT_STRING("KiUserApcDispatcher");
ANSI_STRING ExceptName = RTL_CONSTANT_STRING("KiUserExceptionDispatcher");
ANSI_STRING CallbackName = RTL_CONSTANT_STRING("KiUserCallbackDispatcher");
ANSI_STRING RaiseName = RTL_CONSTANT_STRING("KiRaiseUserExceptionDispatcher");

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

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
PspLookupSystemDllEntryPoint(IN PANSI_STRING Name,
                             IN PVOID *EntryPoint)
{
    /* Call the LDR Routine */
    return LdrGetProcedureAddress(PspSystemDllBase, Name, 0, EntryPoint);
}

NTSTATUS
NTAPI
PspLookupKernelUserEntryPoints(VOID)
{
    NTSTATUS Status;

    /* Get user-mode startup thunk */
    Status = PspLookupSystemDllEntryPoint(&ThunkName,
                                          &PspSystemDllEntryPoint);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get user-mode APC trampoline */
    Status = PspLookupSystemDllEntryPoint(&ApcName,
                                          &KeUserApcDispatcher);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get user-mode exception dispatcher */
    Status = PspLookupSystemDllEntryPoint(&ExceptName,
                                          &KeUserExceptionDispatcher);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get user-mode callback dispatcher */
    Status = PspLookupSystemDllEntryPoint(&CallbackName,
                                          &KeUserCallbackDispatcher);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get user-mode exception raise trampoline */
    Status = PspLookupSystemDllEntryPoint(&RaiseName,
                                          &KeRaiseUserExceptionDispatcher);
    return Status;
}

NTSTATUS
NTAPI
PspMapSystemDll(IN PEPROCESS Process,
                IN PVOID *DllBase)
{
    NTSTATUS Status;
    LARGE_INTEGER Offset = {{0}};
    SIZE_T ViewSize = 0;
    PVOID ImageBase = 0;

    /* Map the System DLL */
    Status = MmMapViewOfSection(PspSystemDllSection,
                                Process,
                                (PVOID*)&ImageBase,
                                0,
                                0,
                                &Offset,
                                &ViewSize,
                                ViewShare,
                                0,
                                PAGE_READWRITE);

    /* Write the image base and return status */
    if (DllBase) *DllBase = ImageBase;
    return Status;
}

NTSTATUS
NTAPI
PsLocateSystemDll(VOID)
{
    UNICODE_STRING DllName = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\ntdll.dll");
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle, SectionHandle;
    NTSTATUS Status;
    ULONG_PTR HardErrorParameters;
    ULONG HardErrorResponse;

    /* Locate and open NTDLL to determine ImageBase and LdrStartup */
    InitializeObjectAttributes(&ObjectAttributes, &DllName, 0, NULL, NULL);
    Status = ZwOpenFile(&FileHandle,
                        FILE_READ_ACCESS,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        0);
    if (!NT_SUCCESS(Status)) return Status;

    /* FIXME: Check if the image is valid */
    Status = STATUS_SUCCESS; //MmCheckSystemImage(FileHandle, TRUE);
    if (Status == STATUS_IMAGE_CHECKSUM_MISMATCH)
    {
        /* Raise a hard error */
        HardErrorParameters = (ULONG_PTR)&DllName;
        NtRaiseHardError(Status,
                         1,
                         1,
                         &HardErrorParameters,
                         OptionOk,
                         &HardErrorResponse);
        return Status;
    }

    /* Create a section for NTDLL */
    Status = ZwCreateSection(&SectionHandle,
                             SECTION_ALL_ACCESS,
                             NULL,
                             NULL,
                             PAGE_EXECUTE,
                             SEC_IMAGE,
                             FileHandle);
    ZwClose(FileHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Reference the Section */
    Status = ObReferenceObjectByHandle(SectionHandle,
                                       SECTION_ALL_ACCESS,
                                       MmSectionObjectType,
                                       KernelMode,
                                       (PVOID*)&PspSystemDllSection,
                                       NULL);
    ZwClose(SectionHandle);
    if (!NT_SUCCESS(Status)) return Status;

    /* Map it */
    Status = PspMapSystemDll(PsGetCurrentProcess(), &PspSystemDllBase);

    /* Now get the Entrypoints */
    PspLookupKernelUserEntryPoints();
    return STATUS_SUCCESS;
}

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
    PsInitialSystemProcess->Pcb.DirectoryTableBase.LowPart = (ULONG)MmGetPageDirectory();
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

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
PsGetVersion(IN PULONG MajorVersion OPTIONAL,
             IN PULONG MinorVersion OPTIONAL,
             IN PULONG BuildNumber OPTIONAL,
             IN PUNICODE_STRING CSDVersion OPTIONAL)
{
    if (MajorVersion) *MajorVersion = NtMajorVersion;
    if (MinorVersion) *MinorVersion = NtMinorVersion;
    if (BuildNumber) *BuildNumber = NtBuildNumber;

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
