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
#include <debug.h>

extern ULONG ExpInitializationPhase;

PVOID KeUserPopEntrySListEnd;
PVOID KeUserPopEntrySListFault;
PVOID KeUserPopEntrySListResume;

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

UNICODE_STRING PsNtDllPathName =
    RTL_CONSTANT_STRING(L"\\SystemRoot\\System32\\ntdll.dll");

PHANDLE_TABLE PspCidTable;

PEPROCESS PsInitialSystemProcess = NULL;
PEPROCESS PsIdleProcess = NULL;
HANDLE PspInitialSystemProcessHandle = NULL;

ULONG PsMinimumWorkingSet, PsMaximumWorkingSet;
struct
{
    LIST_ENTRY List;
    KGUARDED_MUTEX Lock;
} PspWorkingSetChangeHead;
ULONG PspDefaultPagedLimit, PspDefaultNonPagedLimit, PspDefaultPagefileLimit;
BOOLEAN PspDoingGiveBacks;

/* PRIVATE FUNCTIONS *********************************************************/

static CODE_SEG("INIT")
NTSTATUS
PspLookupSystemDllEntryPoint(
    _In_ PCSTR Name,
    _Out_ PVOID* EntryPoint)
{
    /* Call the internal API */
    return RtlpFindExportedRoutineByName(PspSystemDllBase,
                                         Name,
                                         EntryPoint,
                                         NULL,
                                         STATUS_PROCEDURE_NOT_FOUND);
}

static CODE_SEG("INIT")
NTSTATUS
PspLookupKernelUserEntryPoints(VOID)
{
    NTSTATUS Status;

    /* Get user-mode APC trampoline */
    Status = PspLookupSystemDllEntryPoint("KiUserApcDispatcher",
                                          &KeUserApcDispatcher);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get user-mode exception dispatcher */
    Status = PspLookupSystemDllEntryPoint("KiUserExceptionDispatcher",
                                          &KeUserExceptionDispatcher);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get user-mode callback dispatcher */
    Status = PspLookupSystemDllEntryPoint("KiUserCallbackDispatcher",
                                          &KeUserCallbackDispatcher);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get user-mode exception raise trampoline */
    Status = PspLookupSystemDllEntryPoint("KiRaiseUserExceptionDispatcher",
                                          &KeRaiseUserExceptionDispatcher);
    if (!NT_SUCCESS(Status)) return Status;

    /* Get user-mode SLIST exception functions for page fault rollback race hack */
    Status = PspLookupSystemDllEntryPoint("ExpInterlockedPopEntrySListEnd",
                                          &KeUserPopEntrySListEnd);
    if (!NT_SUCCESS(Status)) { DPRINT1("this not found\n"); return Status; }
    Status = PspLookupSystemDllEntryPoint("ExpInterlockedPopEntrySListFault",
                                          &KeUserPopEntrySListFault);
    if (!NT_SUCCESS(Status)) { DPRINT1("this not found\n"); return Status; }
    Status = PspLookupSystemDllEntryPoint("ExpInterlockedPopEntrySListResume",
                                          &KeUserPopEntrySListResume);
    if (!NT_SUCCESS(Status)) { DPRINT1("this not found\n"); return Status; }

    /* On x86, there are multiple ways to do a system call, find the right stubs */
#if defined(_X86_)
    /* Check if this is a machine that supports SYSENTER */
    if (KeFeatureBits & KF_FAST_SYSCALL)
    {
        /* Get user-mode sysenter stub */
        SharedUserData->SystemCall = (PsNtosImageBase >> (PAGE_SHIFT + 1));
        Status = PspLookupSystemDllEntryPoint("KiFastSystemCall",
                                              (PVOID)&SharedUserData->
                                              SystemCall);
        if (!NT_SUCCESS(Status)) return Status;

        /* Get user-mode sysenter return stub */
        Status = PspLookupSystemDllEntryPoint("KiFastSystemCallRet",
                                              (PVOID)&SharedUserData->
                                              SystemCallReturn);
        if (!NT_SUCCESS(Status)) return Status;
    }
    else
    {
        /* Get the user-mode interrupt stub */
        Status = PspLookupSystemDllEntryPoint("KiIntSystemCall",
                                              (PVOID)&SharedUserData->
                                              SystemCall);
        if (!NT_SUCCESS(Status)) return Status;
    }

    /* Set the test instruction */
    SharedUserData->TestRetInstruction = 0xC3;
#endif

    /* Return the status */
    return Status;
}

NTSTATUS
NTAPI
PspMapSystemDll(IN PEPROCESS Process,
                IN PVOID *DllBase,
                IN BOOLEAN UseLargePages)
{
    NTSTATUS Status;
    LARGE_INTEGER Offset = {{0, 0}};
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
    if (Status != STATUS_SUCCESS)
    {
        /* Normalize status code */
        Status = STATUS_CONFLICTING_ADDRESSES;
    }

    /* Write the image base and return status */
    if (DllBase) *DllBase = ImageBase;
    return Status;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
PsLocateSystemDll(VOID)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    IO_STATUS_BLOCK IoStatusBlock;
    HANDLE FileHandle, SectionHandle;
    NTSTATUS Status;
    ULONG_PTR HardErrorParameters;
    ULONG HardErrorResponse;

    /* Locate and open NTDLL to determine ImageBase and LdrStartup */
    InitializeObjectAttributes(&ObjectAttributes,
                               &PsNtDllPathName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenFile(&FileHandle,
                        FILE_READ_ACCESS,
                        &ObjectAttributes,
                        &IoStatusBlock,
                        FILE_SHARE_READ,
                        0);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, bugcheck */
        KeBugCheckEx(PROCESS1_INITIALIZATION_FAILED, Status, 2, 0, 0);
    }

    /* Check if the image is valid */
    Status = MmCheckSystemImage(FileHandle);
    if (Status == STATUS_IMAGE_CHECKSUM_MISMATCH || Status == STATUS_INVALID_IMAGE_PROTECT)
    {
        /* Raise a hard error */
        HardErrorParameters = (ULONG_PTR)&PsNtDllPathName;
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
    if (!NT_SUCCESS(Status))
    {
        /* Failed, bugcheck */
        KeBugCheckEx(PROCESS1_INITIALIZATION_FAILED, Status, 3, 0, 0);
    }

    /* Reference the Section */
    Status = ObReferenceObjectByHandle(SectionHandle,
                                       SECTION_ALL_ACCESS,
                                       MmSectionObjectType,
                                       KernelMode,
                                       (PVOID*)&PspSystemDllSection,
                                       NULL);
    ZwClose(SectionHandle);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, bugcheck */
        KeBugCheckEx(PROCESS1_INITIALIZATION_FAILED, Status, 4, 0, 0);
    }

    /* Map it */
    Status = PspMapSystemDll(PsGetCurrentProcess(), &PspSystemDllBase, FALSE);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, bugcheck */
        KeBugCheckEx(PROCESS1_INITIALIZATION_FAILED, Status, 5, 0, 0);
    }

    /* Return status */
    return Status;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
PspInitializeSystemDll(VOID)
{
    NTSTATUS Status;

    /* Get user-mode startup thunk */
    Status = PspLookupSystemDllEntryPoint("LdrInitializeThunk",
                                          &PspSystemDllEntryPoint);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, bugcheck */
        KeBugCheckEx(PROCESS1_INITIALIZATION_FAILED, Status, 7, 0, 0);
    }

    /* Get all the other entrypoints */
    Status = PspLookupKernelUserEntryPoints();
    if (!NT_SUCCESS(Status))
    {
        /* Failed, bugcheck */
        KeBugCheckEx(PROCESS1_INITIALIZATION_FAILED, Status, 8, 0, 0);
    }

    /* Let KD know we are done */
    KdUpdateDataBlock();

    /* Return status */
    return Status;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
PspInitPhase1(VOID)
{
    /* Initialize the System DLL and return status of operation */
    if (!NT_SUCCESS(PspInitializeSystemDll())) return FALSE;
    return TRUE;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
PspInitPhase0(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE SysThreadHandle;
    PETHREAD SysThread;
    MM_SYSTEMSIZE SystemSize;
    UNICODE_STRING Name;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    ULONG i;

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

    /* Setup callbacks */
    for (i = 0; i < PSP_MAX_CREATE_THREAD_NOTIFY; i++)
    {
        ExInitializeCallBack(&PspThreadNotifyRoutine[i]);
    }
    for (i = 0; i < PSP_MAX_CREATE_PROCESS_NOTIFY; i++)
    {
        ExInitializeCallBack(&PspProcessNotifyRoutine[i]);
    }
    for (i = 0; i < PSP_MAX_LOAD_IMAGE_NOTIFY; i++)
    {
        ExInitializeCallBack(&PspLoadImageNotifyRoutine[i]);
    }

    /* Setup the quantum table */
    PsChangeQuantumTable(FALSE, PsRawPrioritySeparation);

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
    if (PspDefaultPagefileLimit != MAXULONG) PspDefaultPagefileLimit <<= 20;

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

    /* Initialize Object Initializer */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_PERMANENT |
                                              OBJ_EXCLUSIVE |
                                              OBJ_OPENIF;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.SecurityRequired = TRUE;

    /* Initialize the Process type */
    RtlInitUnicodeString(&Name, L"Process");
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(EPROCESS);
    ObjectTypeInitializer.GenericMapping = PspProcessMapping;
    ObjectTypeInitializer.ValidAccessMask = PROCESS_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = PspDeleteProcess;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &PsProcessType);

    /*  Initialize the Thread type  */
    RtlInitUnicodeString(&Name, L"Thread");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(ETHREAD);
    ObjectTypeInitializer.GenericMapping = PspThreadMapping;
    ObjectTypeInitializer.ValidAccessMask = THREAD_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = PspDeleteThread;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &PsThreadType);

    /*  Initialize the Job type  */
    RtlInitUnicodeString(&Name, L"Job");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(EJOB);
    ObjectTypeInitializer.GenericMapping = PspJobMapping;
    ObjectTypeInitializer.InvalidAttributes = 0;
    ObjectTypeInitializer.ValidAccessMask = JOB_OBJECT_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = PspDeleteJob;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &PsJobType);

    /* Initialize job structures external to this file */
    PspInitializeJobStructures();

    /* Initialize the Working Set data */
    InitializeListHead(&PspWorkingSetChangeHead.List);
    KeInitializeGuardedMutex(&PspWorkingSetChangeHead.Lock);

    /* Create the CID Handle table */
    PspCidTable = ExCreateHandleTable(NULL);
    if (!PspCidTable) return FALSE;

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

    /* Copy the process names */
    strcpy(PsIdleProcess->ImageFileName, "Idle");
    strcpy(PsInitialSystemProcess->ImageFileName, "System");

    /* Allocate a structure for the audit name */
    PsInitialSystemProcess->SeAuditProcessCreationInfo.ImageFileName =
        ExAllocatePoolWithTag(PagedPool,
                              sizeof(OBJECT_NAME_INFORMATION),
                              TAG_SEPA);
    if (!PsInitialSystemProcess->SeAuditProcessCreationInfo.ImageFileName)
    {
        /* Allocation failed */
        return FALSE;
    }

    /* Zero it */
    RtlZeroMemory(PsInitialSystemProcess->
                  SeAuditProcessCreationInfo.ImageFileName,
                  sizeof(OBJECT_NAME_INFORMATION));

    /* Setup the system initialization thread */
    Status = PsCreateSystemThread(&SysThreadHandle,
                                  THREAD_ALL_ACCESS,
                                  &ObjectAttributes,
                                  0,
                                  NULL,
                                  Phase1Initialization,
                                  LoaderBlock);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Create a handle to it */
    ObReferenceObjectByHandle(SysThreadHandle,
                              0,
                              PsThreadType,
                              KernelMode,
                              (PVOID*)&SysThread,
                              NULL);
    ObCloseHandle(SysThreadHandle, KernelMode);

    /* Return success */
    return TRUE;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
PsInitSystem(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    /* Check the initialization phase */
    switch (ExpInitializationPhase)
    {
    case 0:

        /* Do Phase 0 */
        return PspInitPhase0(LoaderBlock);

    case 1:

        /* Do Phase 1 */
        return PspInitPhase1();

    default:

        /* Don't know any other phase! Bugcheck! */
        KeBugCheckEx(UNEXPECTED_INITIALIZATION_CALL,
                     1,
                     ExpInitializationPhase,
                     0,
                     0);
        return FALSE;
    }
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
BOOLEAN
NTAPI
PsGetVersion(OUT PULONG MajorVersion OPTIONAL,
             OUT PULONG MinorVersion OPTIONAL,
             OUT PULONG BuildNumber  OPTIONAL,
             OUT PUNICODE_STRING CSDVersion OPTIONAL)
{
    if (MajorVersion) *MajorVersion = NtMajorVersion;
    if (MinorVersion) *MinorVersion = NtMinorVersion;
    if (BuildNumber ) *BuildNumber  = NtBuildNumber & 0x3FFF;

    if (CSDVersion)
    {
        CSDVersion->Length = CmCSDVersionString.Length;
        CSDVersion->MaximumLength = CmCSDVersionString.MaximumLength;
        CSDVersion->Buffer = CmCSDVersionString.Buffer;
    }

    /* Return TRUE if this is a Checked Build */
    return (NtBuildNumber >> 28) == 0xC;
}

/* EOF */
