/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/init.c
 * PURPOSE:         Executive Initialization Code
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#include <reactos/buildno.h>
#include "inbv/logo.h"

#define NDEBUG
#include <debug.h>

/* This is the size that we can expect from the win 2003 loader */
#define LOADER_PARAMETER_EXTENSION_MIN_SIZE \
    RTL_SIZEOF_THROUGH_FIELD(LOADER_PARAMETER_EXTENSION, AcpiTableSize)

/* Temporary hack */
CODE_SEG("INIT")
BOOLEAN
NTAPI
MmArmInitSystem(
    IN ULONG Phase,
    IN PLOADER_PARAMETER_BLOCK LoaderBlock
);

typedef struct _INIT_BUFFER
{
    WCHAR DebugBuffer[256];
    CHAR VersionBuffer[256];
    CHAR BootlogHeader[256];
    CHAR VersionNumber[24];
    RTL_USER_PROCESS_INFORMATION ProcessInfo;
    WCHAR RegistryBuffer[256];
} INIT_BUFFER, *PINIT_BUFFER;

/* DATA **********************************************************************/

/* NT Version Info */
ULONG NtMajorVersion = VER_PRODUCTMAJORVERSION;
ULONG NtMinorVersion = VER_PRODUCTMINORVERSION;
#if DBG /* Checked Build */
ULONG NtBuildNumber = VER_PRODUCTBUILD | 0xC0000000;
#else   /* Free Build */
ULONG NtBuildNumber = VER_PRODUCTBUILD;
#endif

/* NT System Info */
ULONG NtGlobalFlag = 0;
ULONG ExSuiteMask;

/* Cm Version Info */
ULONG CmNtSpBuildNumber;
ULONG CmNtCSDVersion;
ULONG CmNtCSDReleaseType;
UNICODE_STRING CmVersionString;
UNICODE_STRING CmCSDVersionString;

CHAR NtBuildLab[] = KERNEL_VERSION_BUILD_STR "."
                    REACTOS_COMPILER_NAME "_" REACTOS_COMPILER_VERSION;

/* Init flags and settings */
ULONG ExpInitializationPhase;
BOOLEAN ExpInTextModeSetup;
BOOLEAN IoRemoteBootClient;
ULONG InitSafeBootMode;
BOOLEAN InitIsWinPEMode, InitWinPEModeType;
BOOLEAN SosEnabled; // Used by driver.c!IopDisplayLoadingMessage()

/* NT Boot Path */
UNICODE_STRING NtSystemRoot;

/* NT Initial User Application */
WCHAR NtInitialUserProcessBuffer[128] = L"\\SystemRoot\\System32\\smss.exe";
ULONG NtInitialUserProcessBufferLength = sizeof(NtInitialUserProcessBuffer) -
                                         sizeof(WCHAR);
ULONG NtInitialUserProcessBufferType = REG_SZ;

/* Boot NLS information */
PVOID ExpNlsTableBase;
ULONG ExpAnsiCodePageDataOffset, ExpOemCodePageDataOffset;
ULONG ExpUnicodeCaseTableDataOffset;
NLSTABLEINFO ExpNlsTableInfo;
SIZE_T ExpNlsTableSize;
PVOID ExpNlsSectionPointer;

/* CMOS Timer Sanity */
BOOLEAN ExCmosClockIsSane = TRUE;
BOOLEAN ExpRealTimeIsUniversal;

/* FUNCTIONS ****************************************************************/

CODE_SEG("INIT")
NTSTATUS
NTAPI
ExpCreateSystemRootLink(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    UNICODE_STRING LinkName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE LinkHandle;
    NTSTATUS Status;
    ANSI_STRING AnsiName;
    CHAR Buffer[256];
    ANSI_STRING TargetString;
    UNICODE_STRING TargetName;

    /* Initialize the ArcName tree */
    RtlInitUnicodeString(&LinkName, L"\\ArcName");
    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               SePublicDefaultUnrestrictedSd);

    /* Create it */
    Status = NtCreateDirectoryObject(&LinkHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED, Status, 1, 0, 0);
    }

    /* Close the LinkHandle */
    NtClose(LinkHandle);

    /* Initialize the Device tree */
    RtlInitUnicodeString(&LinkName, L"\\Device");
    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               SePublicDefaultUnrestrictedSd);

    /* Create it */
    Status = NtCreateDirectoryObject(&LinkHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED, Status, 2, 0, 0);
    }

    /* Close the LinkHandle */
    ObCloseHandle(LinkHandle, KernelMode);

    /* Create the system root symlink name */
    RtlInitAnsiString(&AnsiName, "\\SystemRoot");
    Status = RtlAnsiStringToUnicodeString(&LinkName, &AnsiName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED, Status, 3, 0, 0);
    }

    /* Initialize the attributes for the link */
    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               SePublicDefaultUnrestrictedSd);

    /* Build the ARC name */
    sprintf(Buffer,
            "\\ArcName\\%s%s",
            LoaderBlock->ArcBootDeviceName,
            LoaderBlock->NtBootPathName);
    Buffer[strlen(Buffer) - 1] = ANSI_NULL;

    /* Convert it to Unicode */
    RtlInitString(&TargetString, Buffer);
    Status = RtlAnsiStringToUnicodeString(&TargetName,
                                          &TargetString,
                                          TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, bugcheck */
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED, Status, 4, 0, 0);
    }

    /* Create it */
    Status = NtCreateSymbolicLinkObject(&LinkHandle,
                                        SYMBOLIC_LINK_ALL_ACCESS,
                                        &ObjectAttributes,
                                        &TargetName);

    /* Free the strings */
    RtlFreeUnicodeString(&LinkName);
    RtlFreeUnicodeString(&TargetName);

    /* Check if creating the link failed */
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED, Status, 5, 0, 0);
    }

    /* Close the handle and return success */
    ObCloseHandle(LinkHandle, KernelMode);
    return STATUS_SUCCESS;
}

CODE_SEG("INIT")
VOID
NTAPI
ExpInitNls(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    LARGE_INTEGER SectionSize;
    NTSTATUS Status;
    HANDLE NlsSection;
    PVOID SectionBase = NULL;
    SIZE_T ViewSize = 0;
    LARGE_INTEGER SectionOffset = {{0, 0}};
    PLIST_ENTRY ListHead, NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    ULONG NlsTablesEncountered = 0;
    SIZE_T NlsTableSizes[3] = {0, 0, 0}; /* 3 NLS tables */

    /* Check if this is boot-time phase 0 initialization */
    if (!ExpInitializationPhase)
    {
        /* Loop the memory descriptors */
        ListHead = &LoaderBlock->MemoryDescriptorListHead;
        NextEntry = ListHead->Flink;
        while (NextEntry != ListHead)
        {
            /* Get the current block */
            MdBlock = CONTAINING_RECORD(NextEntry,
                                        MEMORY_ALLOCATION_DESCRIPTOR,
                                        ListEntry);

            /* Check if this is an NLS block */
            if (MdBlock->MemoryType == LoaderNlsData)
            {
                /* Increase the table size */
                ExpNlsTableSize += MdBlock->PageCount * PAGE_SIZE;

                /* FreeLdr-specific */
                NlsTableSizes[NlsTablesEncountered] = MdBlock->PageCount * PAGE_SIZE;
                NlsTablesEncountered++;
                ASSERT(NlsTablesEncountered < 4);
            }

            /* Go to the next block */
            NextEntry = MdBlock->ListEntry.Flink;
        }

        /* Allocate the a new buffer since loader memory will be freed */
        ExpNlsTableBase = ExAllocatePoolWithTag(NonPagedPool,
                                                ExpNlsTableSize,
                                                TAG_RTLI);
        if (!ExpNlsTableBase) KeBugCheck(PHASE0_INITIALIZATION_FAILED);

        /* Copy the codepage data in its new location. */
        if (NlsTablesEncountered == 1)
        {
            /* Ntldr-way boot process */
            RtlCopyMemory(ExpNlsTableBase,
                          LoaderBlock->NlsData->AnsiCodePageData,
                          ExpNlsTableSize);
        }
        else
        {
            /*
            * In NT, the memory blocks are contiguous, but in ReactOS they aren't,
            * so unless someone fixes FreeLdr, we'll have to use this icky hack.
            */
            RtlCopyMemory(ExpNlsTableBase,
                          LoaderBlock->NlsData->AnsiCodePageData,
                          NlsTableSizes[0]);

            RtlCopyMemory((PVOID)((ULONG_PTR)ExpNlsTableBase + NlsTableSizes[0]),
                          LoaderBlock->NlsData->OemCodePageData,
                          NlsTableSizes[1]);

            RtlCopyMemory((PVOID)((ULONG_PTR)ExpNlsTableBase + NlsTableSizes[0] +
                          NlsTableSizes[1]),
                          LoaderBlock->NlsData->UnicodeCodePageData,
                          NlsTableSizes[2]);
            /* End of Hack */
        }

        /* Initialize and reset the NLS TAbles */
        RtlInitNlsTables((PVOID)((ULONG_PTR)ExpNlsTableBase +
                                 ExpAnsiCodePageDataOffset),
                         (PVOID)((ULONG_PTR)ExpNlsTableBase +
                                 ExpOemCodePageDataOffset),
                         (PVOID)((ULONG_PTR)ExpNlsTableBase +
                                 ExpUnicodeCaseTableDataOffset),
                         &ExpNlsTableInfo);
        RtlResetRtlTranslations(&ExpNlsTableInfo);
        return;
    }

    /* Set the section size */
    SectionSize.QuadPart = ExpNlsTableSize;

    /* Create the NLS Section */
    Status = ZwCreateSection(&NlsSection,
                             SECTION_ALL_ACCESS,
                             NULL,
                             &SectionSize,
                             PAGE_READWRITE,
                             SEC_COMMIT | 0x1,
                             NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 1, 0, 0);
    }

    /* Get a pointer to the section */
    Status = ObReferenceObjectByHandle(NlsSection,
                                       SECTION_ALL_ACCESS,
                                       MmSectionObjectType,
                                       KernelMode,
                                       &ExpNlsSectionPointer,
                                       NULL);
    ObCloseHandle(NlsSection, KernelMode);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 2, 0, 0);
    }

    /* Map the NLS Section in system space */
    Status = MmMapViewInSystemSpace(ExpNlsSectionPointer,
                                    &SectionBase,
                                    &ExpNlsTableSize);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 3, 0, 0);
    }

    /* Copy the codepage data in its new location. */
    ASSERT(SectionBase >= MmSystemRangeStart);
    RtlCopyMemory(SectionBase, ExpNlsTableBase, ExpNlsTableSize);

    /* Free the previously allocated buffer and set the new location */
    ExFreePoolWithTag(ExpNlsTableBase, TAG_RTLI);
    ExpNlsTableBase = SectionBase;

    /* Initialize the NLS Tables */
    RtlInitNlsTables((PVOID)((ULONG_PTR)ExpNlsTableBase +
                             ExpAnsiCodePageDataOffset),
                     (PVOID)((ULONG_PTR)ExpNlsTableBase +
                             ExpOemCodePageDataOffset),
                     (PVOID)((ULONG_PTR)ExpNlsTableBase +
                             ExpUnicodeCaseTableDataOffset),
                     &ExpNlsTableInfo);
    RtlResetRtlTranslations(&ExpNlsTableInfo);

    /* Reset the base to 0 */
    SectionBase = NULL;

    /* Map the section in the system process */
    Status = MmMapViewOfSection(ExpNlsSectionPointer,
                                PsGetCurrentProcess(),
                                &SectionBase,
                                0L,
                                0L,
                                &SectionOffset,
                                &ViewSize,
                                ViewShare,
                                0L,
                                PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 5, 0, 0);
    }

    /* Copy the table into the system process and set this as the base */
    RtlCopyMemory(SectionBase, ExpNlsTableBase, ExpNlsTableSize);
    ExpNlsTableBase = SectionBase;
}

CODE_SEG("INIT")
VOID
NTAPI
ExpLoadInitialProcess(IN PINIT_BUFFER InitBuffer,
                      OUT PRTL_USER_PROCESS_PARAMETERS *ProcessParameters,
                      OUT PCHAR *ProcessEnvironment)
{
    NTSTATUS Status;
    SIZE_T Size;
    PWSTR p;
    UNICODE_STRING NullString = RTL_CONSTANT_STRING(L"");
    UNICODE_STRING SmssName, Environment, SystemDriveString, DebugString;
    PVOID EnvironmentPtr = NULL;
    PRTL_USER_PROCESS_INFORMATION ProcessInformation;
    PRTL_USER_PROCESS_PARAMETERS ProcessParams = NULL;

    NullString.Length = sizeof(WCHAR);

    /* Use the initial buffer, after the strings */
    ProcessInformation = &InitBuffer->ProcessInfo;

    /* Allocate memory for the process parameters */
    Size = sizeof(*ProcessParams) + ((MAX_WIN32_PATH * 6) * sizeof(WCHAR));
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     (PVOID*)&ProcessParams,
                                     0,
                                     &Size,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, display error */
        _snwprintf(InitBuffer->DebugBuffer,
                   sizeof(InitBuffer->DebugBuffer)/sizeof(WCHAR),
                   L"INIT: Unable to allocate Process Parameters. 0x%lx",
                   Status);
        RtlInitUnicodeString(&DebugString, InitBuffer->DebugBuffer);
        ZwDisplayString(&DebugString);

        /* Bugcheck the system */
        KeBugCheckEx(SESSION1_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Setup the basic header, and give the process the low 1MB to itself */
    ProcessParams->Length = (ULONG)Size;
    ProcessParams->MaximumLength = (ULONG)Size;
    ProcessParams->Flags = RTL_USER_PROCESS_PARAMETERS_NORMALIZED |
                           RTL_USER_PROCESS_PARAMETERS_RESERVE_1MB;

    /* Allocate a page for the environment */
    Size = PAGE_SIZE;
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     &EnvironmentPtr,
                                     0,
                                     &Size,
                                     MEM_RESERVE | MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, display error */
        _snwprintf(InitBuffer->DebugBuffer,
                   sizeof(InitBuffer->DebugBuffer)/sizeof(WCHAR),
                   L"INIT: Unable to allocate Process Environment. 0x%lx",
                   Status);
        RtlInitUnicodeString(&DebugString, InitBuffer->DebugBuffer);
        ZwDisplayString(&DebugString);

        /* Bugcheck the system */
        KeBugCheckEx(SESSION2_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Write the pointer */
    ProcessParams->Environment = EnvironmentPtr;

    /* Make a buffer for the DOS path */
    p = (PWSTR)(ProcessParams + 1);
    ProcessParams->CurrentDirectory.DosPath.Buffer = p;
    ProcessParams->CurrentDirectory.DosPath.MaximumLength = MAX_WIN32_PATH *
                                                            sizeof(WCHAR);

    /* Copy the DOS path */
    RtlCopyUnicodeString(&ProcessParams->CurrentDirectory.DosPath,
                         &NtSystemRoot);

    /* Make a buffer for the DLL Path */
    p = (PWSTR)((PCHAR)ProcessParams->CurrentDirectory.DosPath.Buffer +
                ProcessParams->CurrentDirectory.DosPath.MaximumLength);
    ProcessParams->DllPath.Buffer = p;
    ProcessParams->DllPath.MaximumLength = MAX_WIN32_PATH * sizeof(WCHAR);

    /* Copy the DLL path and append the system32 directory */
    RtlCopyUnicodeString(&ProcessParams->DllPath,
                         &ProcessParams->CurrentDirectory.DosPath);
    RtlAppendUnicodeToString(&ProcessParams->DllPath, L"\\System32");

    /* Make a buffer for the image name */
    p = (PWSTR)((PCHAR)ProcessParams->DllPath.Buffer +
                ProcessParams->DllPath.MaximumLength);
    ProcessParams->ImagePathName.Buffer = p;
    ProcessParams->ImagePathName.MaximumLength = MAX_WIN32_PATH * sizeof(WCHAR);

    /* Make sure the buffer is a valid string which within the given length */
    if ((NtInitialUserProcessBufferType != REG_SZ) ||
        ((NtInitialUserProcessBufferLength != MAXULONG) &&
         ((NtInitialUserProcessBufferLength < sizeof(WCHAR)) ||
          (NtInitialUserProcessBufferLength >
           sizeof(NtInitialUserProcessBuffer) - sizeof(WCHAR)))))
    {
        /* Invalid initial process string, bugcheck */
        KeBugCheckEx(SESSION2_INITIALIZATION_FAILED,
                     STATUS_INVALID_PARAMETER,
                     NtInitialUserProcessBufferType,
                     NtInitialUserProcessBufferLength,
                     sizeof(NtInitialUserProcessBuffer));
    }

    /* Cut out anything after a space */
    p = NtInitialUserProcessBuffer;
    while ((*p) && (*p != L' ')) p++;

    /* Set the image path length */
    ProcessParams->ImagePathName.Length =
        (USHORT)((PCHAR)p - (PCHAR)NtInitialUserProcessBuffer);

    /* Copy the actual buffer */
    RtlCopyMemory(ProcessParams->ImagePathName.Buffer,
                  NtInitialUserProcessBuffer,
                  ProcessParams->ImagePathName.Length);

    /* Null-terminate it */
    ProcessParams->ImagePathName.Buffer[ProcessParams->ImagePathName.Length /
                                        sizeof(WCHAR)] = UNICODE_NULL;

    /* Make a buffer for the command line */
    p = (PWSTR)((PCHAR)ProcessParams->ImagePathName.Buffer +
                ProcessParams->ImagePathName.MaximumLength);
    ProcessParams->CommandLine.Buffer = p;
    ProcessParams->CommandLine.MaximumLength = MAX_WIN32_PATH * sizeof(WCHAR);

    /* Add the image name to the command line */
    RtlAppendUnicodeToString(&ProcessParams->CommandLine,
                             NtInitialUserProcessBuffer);

    /* Create the environment string */
    RtlInitEmptyUnicodeString(&Environment,
                              ProcessParams->Environment,
                              (USHORT)Size);

    /* Append the DLL path to it */
    RtlAppendUnicodeToString(&Environment, L"Path=");
    RtlAppendUnicodeStringToString(&Environment, &ProcessParams->DllPath);
    RtlAppendUnicodeStringToString(&Environment, &NullString);

    /* Create the system drive string */
    SystemDriveString = NtSystemRoot;
    SystemDriveString.Length = 2 * sizeof(WCHAR);

    /* Append it to the environment */
    RtlAppendUnicodeToString(&Environment, L"SystemDrive=");
    RtlAppendUnicodeStringToString(&Environment, &SystemDriveString);
    RtlAppendUnicodeStringToString(&Environment, &NullString);

    /* Append the system root to the environment */
    RtlAppendUnicodeToString(&Environment, L"SystemRoot=");
    RtlAppendUnicodeStringToString(&Environment, &NtSystemRoot);
    RtlAppendUnicodeStringToString(&Environment, &NullString);

    /* Prepare the prefetcher */
    //CcPfBeginBootPhase(150);

    /* Create SMSS process */
    SmssName = ProcessParams->ImagePathName;
    Status = RtlCreateUserProcess(&SmssName,
                                  OBJ_CASE_INSENSITIVE,
                                  RtlDeNormalizeProcessParams(ProcessParams),
                                  NULL,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  NULL,
                                  NULL,
                                  ProcessInformation);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, display error */
        _snwprintf(InitBuffer->DebugBuffer,
                   sizeof(InitBuffer->DebugBuffer)/sizeof(WCHAR),
                   L"INIT: Unable to create Session Manager. 0x%lx",
                   Status);
        RtlInitUnicodeString(&DebugString, InitBuffer->DebugBuffer);
        ZwDisplayString(&DebugString);

        /* Bugcheck the system */
        KeBugCheckEx(SESSION3_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Resume the thread */
    Status = ZwResumeThread(ProcessInformation->ThreadHandle, NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, display error */
        _snwprintf(InitBuffer->DebugBuffer,
                   sizeof(InitBuffer->DebugBuffer)/sizeof(WCHAR),
                   L"INIT: Unable to resume Session Manager. 0x%lx",
                   Status);
        RtlInitUnicodeString(&DebugString, InitBuffer->DebugBuffer);
        ZwDisplayString(&DebugString);

        /* Bugcheck the system */
        KeBugCheckEx(SESSION4_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Return success */
    *ProcessParameters = ProcessParams;
    *ProcessEnvironment = EnvironmentPtr;
}

CODE_SEG("INIT")
ULONG
NTAPI
ExComputeTickCountMultiplier(IN ULONG ClockIncrement)
{
    ULONG MsRemainder = 0, MsIncrement;
    ULONG IncrementRemainder;
    ULONG i;

    /* Count the number of milliseconds for each clock interrupt */
    MsIncrement = ClockIncrement / (10 * 1000);

    /* Count the remainder from the division above, with 24-bit precision */
    IncrementRemainder = ClockIncrement - (MsIncrement * (10 * 1000));
    for (i= 0; i < 24; i++)
    {
        /* Shift the remainders */
        MsRemainder <<= 1;
        IncrementRemainder <<= 1;

        /* Check if we've went past 1 ms */
        if (IncrementRemainder >= (10 * 1000))
        {
            /* Increase the remainder by one, and substract from increment */
            IncrementRemainder -= (10 * 1000);
            MsRemainder |= 1;
        }
    }

    /* Return the increment */
    return (MsIncrement << 24) | MsRemainder;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitSystemPhase0(VOID)
{
    /* Initialize EXRESOURCE Support */
    ExpResourceInitialization();

    /* Initialize the environment lock */
    ExInitializeFastMutex(&ExpEnvironmentLock);

    /* Initialize the lookaside lists and locks */
    ExpInitLookasideLists();

    /* Initialize the Firmware Table resource and listhead */
    InitializeListHead(&ExpFirmwareTableProviderListHead);
    ExInitializeResourceLite(&ExpFirmwareTableResource);
    ExInitializeResourceLite(&ExpTimeRefreshLock);

    /* Set the suite mask to maximum and return */
    ExSuiteMask = 0xFFFFFFFF;
    return TRUE;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitSystemPhase1(VOID)
{
    /* Initialize worker threads */
    ExpInitializeWorkerThreads();

    /* Initialize pushlocks */
    ExpInitializePushLocks();

    /* Initialize events and event pairs */
    if (ExpInitializeEventImplementation() == FALSE)
    {
        DPRINT1("Executive: Event initialization failed\n");
        return FALSE;
    }
    if (ExpInitializeEventPairImplementation() == FALSE)
    {
        DPRINT1("Executive: Event Pair initialization failed\n");
        return FALSE;
    }

    /* Initialize mutants */
    if (ExpInitializeMutantImplementation() == FALSE)
    {
        DPRINT1("Executive: Mutant initialization failed\n");
        return FALSE;
    }

    /* Initialize callbacks */
    if (ExpInitializeCallbacks() == FALSE)
    {
        DPRINT1("Executive: Callback initialization failed\n");
        return FALSE;
    }

    /* Initialize semaphores */
    if (ExpInitializeSemaphoreImplementation() == FALSE)
    {
        DPRINT1("Executive: Semaphore initialization failed\n");
        return FALSE;
    }

    /* Initialize timers */
    if (ExpInitializeTimerImplementation() == FALSE)
    {
        DPRINT1("Executive: Timer initialization failed\n");
        return FALSE;
    }

    /* Initialize profiling */
    if (ExpInitializeProfileImplementation() == FALSE)
    {
        DPRINT1("Executive: Profile initialization failed\n");
        return FALSE;
    }

    /* Initialize UUIDs */
    if (ExpUuidInitialization() == FALSE)
    {
        DPRINT1("Executive: Uuid initialization failed\n");
        return FALSE;
    }

    /* Initialize keyed events */
    if (ExpInitializeKeyedEventImplementation() == FALSE)
    {
        DPRINT1("Executive: Keyed event initialization failed\n");
        return FALSE;
    }

    /* Initialize Win32K */
    if (ExpWin32kInit() == FALSE)
    {
        DPRINT1("Executive: Win32 initialization failed\n");
        return FALSE;
    }
    return TRUE;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExInitSystem(VOID)
{
    /* Check the initialization phase */
    switch (ExpInitializationPhase)
    {
        case 0:

            /* Do Phase 0 */
            return ExpInitSystemPhase0();

        case 1:

            /* Do Phase 1 */
            return ExpInitSystemPhase1();

        default:

            /* Don't know any other phase! Bugcheck! */
            KeBugCheck(UNEXPECTED_INITIALIZATION_CALL);
            return FALSE;
    }
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpIsLoaderValid(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLOADER_PARAMETER_EXTENSION Extension;

    /* Get the loader extension */
    Extension = LoaderBlock->Extension;

    /* Validate the size (Windows 2003 loader doesn't provide more) */
    if (Extension->Size < LOADER_PARAMETER_EXTENSION_MIN_SIZE) return FALSE;

    /* Don't validate upper versions */
    if (Extension->MajorVersion > VER_PRODUCTMAJORVERSION) return TRUE;

    /* Fail if this is NT 4 */
    if (Extension->MajorVersion < VER_PRODUCTMAJORVERSION) return FALSE;

    /* Fail if this is XP */
    if (Extension->MinorVersion < VER_PRODUCTMINORVERSION) return FALSE;

    /* This is 2003 or newer, approve it */
    return TRUE;
}

CODE_SEG("INIT")
VOID
NTAPI
ExpLoadBootSymbols(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG i = 0;
    PLIST_ENTRY NextEntry;
    ULONG Count, Length;
    PWCHAR Name;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    CHAR NameBuffer[256];
    STRING SymbolString;
    NTSTATUS Status;

    /* Loop the driver list */
    NextEntry = LoaderBlock->LoadOrderListHead.Flink;
    while (NextEntry != &LoaderBlock->LoadOrderListHead)
    {
        /* Skip the first two images */
        if (i >= 2)
        {
            /* Get the entry */
            LdrEntry = CONTAINING_RECORD(NextEntry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InLoadOrderLinks);
            if (LdrEntry->FullDllName.Buffer[0] == L'\\')
            {
                /* We have a name, read its data */
                Name = LdrEntry->FullDllName.Buffer;
                Length = LdrEntry->FullDllName.Length / sizeof(WCHAR);

                /* Check if our buffer can hold it */
                if (sizeof(NameBuffer) < Length + sizeof(ANSI_NULL))
                {
                    /* It's too long */
                    Status = STATUS_BUFFER_OVERFLOW;
                }
                else
                {
                    /* Copy the name */
                    Count = 0;
                    do
                    {
                        /* Copy the character */
                        NameBuffer[Count++] = (CHAR)*Name++;
                    } while (Count < Length);

                    /* Null-terminate */
                    NameBuffer[Count] = ANSI_NULL;
                    Status = STATUS_SUCCESS;
                }
            }
            else
            {
                /* Safely print the string into our buffer */
                Status = RtlStringCbPrintfA(NameBuffer,
                                            sizeof(NameBuffer),
                                            "%S\\System32\\Drivers\\%wZ",
                                            &SharedUserData->NtSystemRoot[2],
                                            &LdrEntry->BaseDllName);
            }

            /* Check if the buffer was ok */
            if (NT_SUCCESS(Status))
            {
                /* Initialize the STRING for the debugger */
                RtlInitString(&SymbolString, NameBuffer);

                /* Load the symbols */
                DbgLoadImageSymbols(&SymbolString,
                                    LdrEntry->DllBase,
                                    (ULONG_PTR)PsGetCurrentProcessId());
            }
        }

        /* Go to the next entry */
        i++;
        NextEntry = NextEntry->Flink;
    }
}

CODE_SEG("INIT")
VOID
NTAPI
ExBurnMemory(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
             IN ULONG_PTR PagesToDestroy,
             IN TYPE_OF_MEMORY MemoryType)
{
    PLIST_ENTRY ListEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MemDescriptor;

    DPRINT1("Burn RAM amount: %lu pages\n", PagesToDestroy);

    /* Loop the memory descriptors, beginning at the end */
    for (ListEntry = LoaderBlock->MemoryDescriptorListHead.Blink;
         ListEntry != &LoaderBlock->MemoryDescriptorListHead;
         ListEntry = ListEntry->Blink)
    {
        /* Get the memory descriptor structure */
        MemDescriptor = CONTAINING_RECORD(ListEntry,
                                          MEMORY_ALLOCATION_DESCRIPTOR,
                                          ListEntry);

        /* Is memory free there or is it temporary? */
        if (MemDescriptor->MemoryType == LoaderFree ||
            MemDescriptor->MemoryType == LoaderFirmwareTemporary)
        {
            /* Check if the descriptor has more pages than we want */
            if (MemDescriptor->PageCount > PagesToDestroy)
            {
                /* Change block's page count, ntoskrnl doesn't care much */
                MemDescriptor->PageCount -= PagesToDestroy;
                break;
            }
            else
            {
                /* Change block type */
                MemDescriptor->MemoryType = MemoryType;
                PagesToDestroy -= MemDescriptor->PageCount;

                /* Check if we are done */
                if (PagesToDestroy == 0) break;
            }
        }
    }
}

CODE_SEG("INIT")
VOID
NTAPI
ExpInitializeExecutive(IN ULONG Cpu,
                       IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PNLS_DATA_BLOCK NlsData;
    CHAR Buffer[256];
    ANSI_STRING AnsiPath;
    NTSTATUS Status;
    PCHAR CommandLine, PerfMem;
    ULONG PerfMemUsed;
    PLDR_DATA_TABLE_ENTRY NtosEntry;
    PMESSAGE_RESOURCE_ENTRY MsgEntry;
    ANSI_STRING CSDString;
    size_t Remaining = 0;
    PCHAR RcEnd = NULL;
    CHAR VersionBuffer[65];

    /* Validate Loader */
    if (!ExpIsLoaderValid(LoaderBlock))
    {
        /* Invalid loader version */
        KeBugCheckEx(MISMATCHED_HAL,
                     3,
                     LoaderBlock->Extension->Size,
                     LoaderBlock->Extension->MajorVersion,
                     LoaderBlock->Extension->MinorVersion);
    }

    /* Initialize PRCB pool lookaside pointers */
    ExInitPoolLookasidePointers();

    /* Check if this is an application CPU */
    if (Cpu)
    {
        /* Then simply initialize it with HAL */
        if (!HalInitSystem(ExpInitializationPhase, LoaderBlock))
        {
            /* Initialization failed */
            KeBugCheck(HAL_INITIALIZATION_FAILED);
        }

        /* We're done */
        return;
    }

    /* Assume no text-mode or remote boot */
    ExpInTextModeSetup = FALSE;
    IoRemoteBootClient = FALSE;

    /* Check if we have a setup loader block */
    if (LoaderBlock->SetupLdrBlock)
    {
        /* Check if this is text-mode setup */
        if (LoaderBlock->SetupLdrBlock->Flags & SETUPLDR_TEXT_MODE)
            ExpInTextModeSetup = TRUE;

        /* Check if this is network boot */
        if (LoaderBlock->SetupLdrBlock->Flags & SETUPLDR_REMOTE_BOOT)
        {
            /* Set variable */
            IoRemoteBootClient = TRUE;

            /* Make sure we're actually booting off the network */
            ASSERT(!_memicmp(LoaderBlock->ArcBootDeviceName, "net(0)", 6));
        }
    }

    /* Set phase to 0 */
    ExpInitializationPhase = 0;

    /* Get boot command line */
    CommandLine = LoaderBlock->LoadOptions;
    if (CommandLine)
    {
        /* Upcase it for comparison and check if we're in performance mode */
        _strupr(CommandLine);
        PerfMem = strstr(CommandLine, "PERFMEM");
        if (PerfMem)
        {
            /* Check if the user gave a number of bytes to use */
            PerfMem = strstr(PerfMem, "=");
            if (PerfMem)
            {
                /* Read the number of pages we'll use */
                PerfMemUsed = atol(PerfMem + 1) * (1024 * 1024 / PAGE_SIZE);
                if (PerfMemUsed)
                {
                    /* FIXME: TODO */
                    DPRINT1("BBT performance mode not yet supported."
                            "/PERFMEM option ignored.\n");
                }
            }
        }

        /* Check if we're burning memory */
        PerfMem = strstr(CommandLine, "BURNMEMORY");
        if (PerfMem)
        {
            /* Check if the user gave a number of bytes to use */
            PerfMem = strstr(PerfMem, "=");
            if (PerfMem)
            {
                /* Read the number of pages we'll use */
                PerfMemUsed = atol(PerfMem + 1) * (1024 * 1024 / PAGE_SIZE);
                if (PerfMemUsed) ExBurnMemory(LoaderBlock, PerfMemUsed, LoaderBad);
            }
        }
    }

    /* Setup NLS Base and offsets */
    NlsData = LoaderBlock->NlsData;
    ExpNlsTableBase = NlsData->AnsiCodePageData;
    ExpAnsiCodePageDataOffset = 0;
    ExpOemCodePageDataOffset = (ULONG)((ULONG_PTR)NlsData->OemCodePageData -
                                       (ULONG_PTR)NlsData->AnsiCodePageData);
    ExpUnicodeCaseTableDataOffset = (ULONG)((ULONG_PTR)NlsData->UnicodeCodePageData -
                                            (ULONG_PTR)NlsData->AnsiCodePageData);

    /* Initialize the NLS Tables */
    RtlInitNlsTables((PVOID)((ULONG_PTR)ExpNlsTableBase +
                             ExpAnsiCodePageDataOffset),
                     (PVOID)((ULONG_PTR)ExpNlsTableBase +
                             ExpOemCodePageDataOffset),
                     (PVOID)((ULONG_PTR)ExpNlsTableBase +
                             ExpUnicodeCaseTableDataOffset),
                     &ExpNlsTableInfo);
    RtlResetRtlTranslations(&ExpNlsTableInfo);

    /* Now initialize the HAL */
    if (!HalInitSystem(ExpInitializationPhase, LoaderBlock))
    {
        /* HAL failed to initialize, bugcheck */
        KeBugCheck(HAL_INITIALIZATION_FAILED);
    }

    /* Make sure interrupts are active now */
    _enable();

    /* Clear the crypto exponent */
    SharedUserData->CryptoExponent = 0;

    /* Set global flags for the checked build */
#if DBG
    NtGlobalFlag |= FLG_ENABLE_CLOSE_EXCEPTIONS |
                    FLG_ENABLE_KDEBUG_SYMBOL_LOAD;
#endif

    /* Setup NT System Root Path */
    sprintf(Buffer, "C:%s", LoaderBlock->NtBootPathName);

    /* Convert to ANSI_STRING and null-terminate it */
    RtlInitString(&AnsiPath, Buffer);
    Buffer[--AnsiPath.Length] = ANSI_NULL;

    /* Get the string from KUSER_SHARED_DATA's buffer */
    RtlInitEmptyUnicodeString(&NtSystemRoot,
                              SharedUserData->NtSystemRoot,
                              sizeof(SharedUserData->NtSystemRoot));

    /* Now fill it in */
    Status = RtlAnsiStringToUnicodeString(&NtSystemRoot, &AnsiPath, FALSE);
    if (!NT_SUCCESS(Status)) KeBugCheck(SESSION3_INITIALIZATION_FAILED);

    /* Setup bugcheck messages */
    KiInitializeBugCheck();

    /* Setup initial system settings */
    CmGetSystemControlValues(LoaderBlock->RegistryBase, CmControlVector);

    /* Set the Service Pack Number and add it to the CSD Version number if needed */
    CmNtSpBuildNumber = VER_PRODUCTBUILD_QFE;
    if (((CmNtCSDVersion & 0xFFFF0000) == 0) && (CmNtCSDReleaseType == 1))
    {
        CmNtCSDVersion |= (VER_PRODUCTBUILD_QFE << 16);
    }

    /* Add loaded CmNtGlobalFlag value */
    NtGlobalFlag |= CmNtGlobalFlag;

    /* Initialize the executive at phase 0 */
    if (!ExInitSystem()) KeBugCheck(PHASE0_INITIALIZATION_FAILED);

    /* Initialize the memory manager at phase 0 */
    if (!MmArmInitSystem(0, LoaderBlock)) KeBugCheck(PHASE0_INITIALIZATION_FAILED);

    /* Load boot symbols */
    ExpLoadBootSymbols(LoaderBlock);

    /* Check if we should break after symbol load */
    if (KdBreakAfterSymbolLoad) DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);

    /* Check if this loader is compatible with NT 5.2 */
    if (LoaderBlock->Extension->Size >= sizeof(LOADER_PARAMETER_EXTENSION))
    {
        /* Setup headless terminal settings */
        HeadlessInit(LoaderBlock);
    }

    /* Set system ranges */
#ifdef _M_AMD64
    SharedUserData->Reserved1 = MM_HIGHEST_USER_ADDRESS_WOW64;
    SharedUserData->Reserved3 = MM_SYSTEM_RANGE_START_WOW64;
#else
    SharedUserData->Reserved1 = (ULONG_PTR)MmHighestUserAddress;
    SharedUserData->Reserved3 = (ULONG_PTR)MmSystemRangeStart;
#endif

    /* Make a copy of the NLS Tables */
    ExpInitNls(LoaderBlock);

    /* Get the kernel's load entry */
    NtosEntry = CONTAINING_RECORD(LoaderBlock->LoadOrderListHead.Flink,
                                  LDR_DATA_TABLE_ENTRY,
                                  InLoadOrderLinks);

    /* Check if this is a service pack */
    if (CmNtCSDVersion & 0xFFFF)
    {
        /* Get the service pack string */
        Status = RtlFindMessage(NtosEntry->DllBase,
                                11,
                                0,
                                WINDOWS_NT_CSD_STRING,
                                &MsgEntry);
        if (NT_SUCCESS(Status))
        {
            /* Setup the string */
            RtlInitAnsiString(&CSDString, (PCHAR)MsgEntry->Text);

            /* Remove trailing newline */
            while ((CSDString.Length > 0) &&
                   ((CSDString.Buffer[CSDString.Length - 1] == '\r') ||
                    (CSDString.Buffer[CSDString.Length - 1] == '\n')))
            {
                /* Skip the trailing character */
                CSDString.Length--;
            }

            /* Fill the buffer with version information */
            Status = RtlStringCbPrintfA(Buffer,
                                        sizeof(Buffer),
                                        "%Z %u%c",
                                        &CSDString,
                                        (CmNtCSDVersion & 0xFF00) >> 8,
                                        (CmNtCSDVersion & 0xFF) ?
                                        'A' + (CmNtCSDVersion & 0xFF) - 1 :
                                        ANSI_NULL);
        }
        else
        {
            /* Build default string */
            Status = RtlStringCbPrintfA(Buffer,
                                        sizeof(Buffer),
                                        "CSD %04x",
                                        CmNtCSDVersion);
        }

        /* Check for success */
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            KeBugCheckEx(PHASE0_INITIALIZATION_FAILED, Status, 0, 0, 0);
        }
    }
    else
    {
        /* Then this is a beta */
        Status = RtlStringCbCopyExA(Buffer,
                                    sizeof(Buffer),
                                    VER_PRODUCTBETA_STR,
                                    NULL,
                                    &Remaining,
                                    0);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            KeBugCheckEx(PHASE0_INITIALIZATION_FAILED, Status, 0, 0, 0);
        }

        /* Update length */
        CmCSDVersionString.MaximumLength = sizeof(Buffer) - (USHORT)Remaining;
    }

    /* Check if we have an RC number */
    if ((CmNtCSDVersion & 0xFFFF0000) && (CmNtCSDReleaseType == 1))
    {
        /* Check if we have no version data yet */
        if (!(*Buffer))
        {
            /* Set defaults */
            Remaining = sizeof(Buffer);
            RcEnd = Buffer;
        }
        else
        {
            /* Add comma and space */
            Status = RtlStringCbCatExA(Buffer,
                                       sizeof(Buffer),
                                       ", ",
                                       &RcEnd,
                                       &Remaining,
                                       0);
            if (!NT_SUCCESS(Status))
            {
                /* Fail */
                KeBugCheckEx(PHASE0_INITIALIZATION_FAILED, Status, 0, 0, 0);
            }
        }

        /* Add the version format string */
        Status = RtlStringCbPrintfA(RcEnd,
                                    Remaining,
                                    "v.%u",
                                    (CmNtCSDVersion & 0xFFFF0000) >> 16);
        if (!NT_SUCCESS(Status))
        {
            /* Fail */
            KeBugCheckEx(PHASE0_INITIALIZATION_FAILED, Status, 0, 0, 0);
        }
    }

    /* Now setup the final string */
    RtlInitAnsiString(&CSDString, Buffer);
    Status = RtlAnsiStringToUnicodeString(&CmCSDVersionString,
                                          &CSDString,
                                          TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        KeBugCheckEx(PHASE0_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Add our version */
    Status = RtlStringCbPrintfA(VersionBuffer,
                                sizeof(VersionBuffer),
                                "%u.%u",
                                VER_PRODUCTMAJORVERSION,
                                VER_PRODUCTMINORVERSION);
    if (!NT_SUCCESS(Status))
    {
        /* Fail */
        KeBugCheckEx(PHASE0_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Build the final version string */
    RtlCreateUnicodeStringFromAsciiz(&CmVersionString, VersionBuffer);

    /* Check if the user wants a kernel stack trace database */
    if (NtGlobalFlag & FLG_KERNEL_STACK_TRACE_DB)
    {
        /* FIXME: TODO */
        DPRINT1("Kernel-mode stack trace support not yet present."
                "FLG_KERNEL_STACK_TRACE_DB flag ignored.\n");
    }

    /* Check if he wanted exception logging */
    if (NtGlobalFlag & FLG_ENABLE_EXCEPTION_LOGGING)
    {
        /* FIXME: TODO */
        DPRINT1("Kernel-mode exception logging support not yet present."
                "FLG_ENABLE_EXCEPTION_LOGGING flag ignored.\n");
    }

    /* Initialize the Handle Table */
    ExpInitializeHandleTables();

#if DBG
    /* On checked builds, allocate the system call count table */
    KeServiceDescriptorTable[0].Count =
        ExAllocatePoolWithTag(NonPagedPool,
                              KiServiceLimit * sizeof(ULONG),
                              'llaC');

    /* Use it for the shadow table too */
    KeServiceDescriptorTableShadow[0].Count = KeServiceDescriptorTable[0].Count;

    /* Make sure allocation succeeded */
    if (KeServiceDescriptorTable[0].Count)
    {
        /* Zero the call counts to 0 */
        RtlZeroMemory(KeServiceDescriptorTable[0].Count,
                      KiServiceLimit * sizeof(ULONG));
    }
#endif

    /* Create the Basic Object Manager Types to allow new Object Types */
    if (!ObInitSystem()) KeBugCheck(OBJECT_INITIALIZATION_FAILED);

    /* Load basic Security for other Managers */
    if (!SeInitSystem()) KeBugCheck(SECURITY_INITIALIZATION_FAILED);

    /* Initialize the Process Manager */
    if (!PsInitSystem(LoaderBlock)) KeBugCheck(PROCESS_INITIALIZATION_FAILED);

    /* Initialize the PnP Manager */
    if (!PpInitSystem()) KeBugCheck(PP0_INITIALIZATION_FAILED);

    /* Initialize the User-Mode Debugging Subsystem */
    DbgkInitialize();

    /* Calculate the tick count multiplier */
    ExpTickCountMultiplier = ExComputeTickCountMultiplier(KeMaximumIncrement);
    SharedUserData->TickCountMultiplier = ExpTickCountMultiplier;

    /* Set the OS Version */
    SharedUserData->NtMajorVersion = NtMajorVersion;
    SharedUserData->NtMinorVersion = NtMinorVersion;

    /* Set the machine type */
    SharedUserData->ImageNumberLow = IMAGE_FILE_MACHINE_NATIVE;
    SharedUserData->ImageNumberHigh = IMAGE_FILE_MACHINE_NATIVE;

    /* ReactOS magic */
    *(PULONG)(KI_USER_SHARED_DATA + PAGE_SIZE - sizeof(ULONG)) = 0x8eac705;
}

VOID
NTAPI
MmFreeLoaderBlock(IN PLOADER_PARAMETER_BLOCK LoaderBlock);

CODE_SEG("INIT")
VOID
NTAPI
Phase1InitializationDiscard(IN PVOID Context)
{
    PLOADER_PARAMETER_BLOCK LoaderBlock = Context;
    NTSTATUS Status, MsgStatus;
    TIME_FIELDS TimeFields;
    LARGE_INTEGER SystemBootTime, UniversalBootTime, OldTime, Timeout;
    BOOLEAN NoGuiBoot, ResetBias = FALSE, AlternateShell = FALSE;
    PLDR_DATA_TABLE_ENTRY NtosEntry;
    PMESSAGE_RESOURCE_ENTRY MsgEntry;
    PCHAR CommandLine, Y2KHackRequired, SafeBoot, Environment;
    PCHAR StringBuffer, EndBuffer, BeginBuffer, MpString = "";
    PINIT_BUFFER InitBuffer;
    ANSI_STRING TempString;
    ULONG LastTzBias, Length, YearHack = 0, Disposition, MessageCode = 0;
    SIZE_T Size;
    size_t Remaining;
    PRTL_USER_PROCESS_INFORMATION ProcessInfo;
    KEY_VALUE_PARTIAL_INFORMATION KeyPartialInfo;
    UNICODE_STRING KeyName;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE KeyHandle, OptionHandle;
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters = NULL;

    /* Allocate the initialization buffer */
    InitBuffer = ExAllocatePoolWithTag(NonPagedPool,
                                       sizeof(INIT_BUFFER),
                                       TAG_INIT);
    if (!InitBuffer)
    {
        /* Bugcheck */
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, STATUS_NO_MEMORY, 8, 0, 0);
    }

    /* Set to phase 1 */
    ExpInitializationPhase = 1;

    /* Set us at maximum priority */
    KeSetPriorityThread(KeGetCurrentThread(), HIGH_PRIORITY);

    /* Do Phase 1 HAL Initialization */
    if (!HalInitSystem(1, LoaderBlock)) KeBugCheck(HAL1_INITIALIZATION_FAILED);

    /* Get the command line and upcase it */
    CommandLine = (LoaderBlock->LoadOptions ? _strupr(LoaderBlock->LoadOptions) : NULL);

    /* Check if GUI Boot is enabled */
    NoGuiBoot = (CommandLine && strstr(CommandLine, "NOGUIBOOT") != NULL);

    /* Get the SOS setting */
    SosEnabled = (CommandLine && strstr(CommandLine, "SOS") != NULL);

    /* Setup the boot video driver */
    InbvEnableBootDriver(!NoGuiBoot);
    InbvDriverInitialize(LoaderBlock, IDB_MAX_RESOURCES);

    /* Check if GUI boot is enabled */
    if (!NoGuiBoot)
    {
        /* It is, display the boot logo and enable printing strings */
        InbvEnableDisplayString(SosEnabled);
        DisplayBootBitmap(SosEnabled);
    }
    else
    {
        /* Release display ownership if not using GUI boot */
        InbvNotifyDisplayOwnershipLost(NULL);

        /* Don't allow boot-time strings */
        InbvEnableDisplayString(FALSE);
    }

    /* Check if this is LiveCD (WinPE) mode */
    if (CommandLine && strstr(CommandLine, "MININT") != NULL)
    {
        /* Setup WinPE Settings */
        InitIsWinPEMode = TRUE;
        InitWinPEModeType |= (strstr(CommandLine, "INRAM") != NULL) ? 0x80000000 : 0x00000001;
    }

    /* Get the kernel's load entry */
    NtosEntry = CONTAINING_RECORD(LoaderBlock->LoadOrderListHead.Flink,
                                  LDR_DATA_TABLE_ENTRY,
                                  InLoadOrderLinks);

    /* Find the banner message */
    MsgStatus = RtlFindMessage(NtosEntry->DllBase,
                               11,
                               0,
                               WINDOWS_NT_BANNER,
                               &MsgEntry);

    /* Setup defaults and check if we have a version string */
    StringBuffer = InitBuffer->VersionBuffer;
    BeginBuffer = StringBuffer;
    EndBuffer = StringBuffer;
    Remaining = sizeof(InitBuffer->VersionBuffer);
    if (CmCSDVersionString.Length)
    {
        /* Print the version string */
        Status = RtlStringCbPrintfExA(StringBuffer,
                                      Remaining,
                                      &EndBuffer,
                                      &Remaining,
                                      0,
                                      ": %wZ",
                                      &CmCSDVersionString);
        if (!NT_SUCCESS(Status))
        {
            /* Bugcheck */
            KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 7, 0, 0);
        }
    }
    else
    {
        /* No version */
        *EndBuffer = ANSI_NULL; /* Null-terminate the string */
    }

    /* Skip over the null-terminator to start a new string */
    ++EndBuffer;
    --Remaining;

    /* Build the version number */
    StringBuffer = InitBuffer->VersionNumber;
    Status = RtlStringCbPrintfA(StringBuffer,
                                sizeof(InitBuffer->VersionNumber),
                                "%u.%u",
                                VER_PRODUCTMAJORVERSION,
                                VER_PRODUCTMINORVERSION);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 7, 0, 0);
    }

    /* Check if we had found a banner message */
    if (NT_SUCCESS(MsgStatus))
    {
        /* Create the banner message */
        /* ReactOS specific: Report ReactOS version, NtBuildLab information and reported NT kernel version */
        Status = RtlStringCbPrintfA(EndBuffer,
                                    Remaining,
                                    (PCHAR)MsgEntry->Text,
                                    KERNEL_VERSION_STR,
                                    NtBuildLab,
                                    StringBuffer,
                                    NtBuildNumber & 0xFFFF,
                                    BeginBuffer);
        if (!NT_SUCCESS(Status))
        {
            /* Bugcheck */
            KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 7, 0, 0);
        }
    }
    else
    {
        /* Use hard-coded banner message */
        Status = RtlStringCbCopyA(EndBuffer, Remaining, "REACTOS (R)\r\n");
        if (!NT_SUCCESS(Status))
        {
            /* Bugcheck */
            KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 7, 0, 0);
        }
    }

    /* Display the version string on-screen */
    InbvDisplayString(EndBuffer);

    /* Initialize Power Subsystem in Phase 0 */
    if (!PoInitSystem(0)) KeBugCheck(INTERNAL_POWER_ERROR);

    /* Check for Y2K hack */
    Y2KHackRequired = CommandLine ? strstr(CommandLine, "YEAR") : NULL;
    if (Y2KHackRequired) Y2KHackRequired = strstr(Y2KHackRequired, "=");
    if (Y2KHackRequired) YearHack = atol(Y2KHackRequired + 1);

    /* Query the clock */
    if ((ExCmosClockIsSane) && (HalQueryRealTimeClock(&TimeFields)))
    {
        /* Check if we're using the Y2K hack */
        if (Y2KHackRequired) TimeFields.Year = (CSHORT)YearHack;

        /* Convert to time fields */
        RtlTimeFieldsToTime(&TimeFields, &SystemBootTime);
        UniversalBootTime = SystemBootTime;

        /* Check if real time is GMT */
        if (!ExpRealTimeIsUniversal)
        {
            /* Check if we don't have a valid bias */
            if (ExpLastTimeZoneBias == MAXULONG)
            {
                /* Reset */
                ResetBias = TRUE;
                ExpLastTimeZoneBias = ExpAltTimeZoneBias;
            }

            /* Calculate the bias in seconds */
            ExpTimeZoneBias.QuadPart = Int32x32To64(ExpLastTimeZoneBias * 60,
                                                    10000000);

            /* Set the boot time-zone bias */
            SharedUserData->TimeZoneBias.High2Time = ExpTimeZoneBias.HighPart;
            SharedUserData->TimeZoneBias.LowPart = ExpTimeZoneBias.LowPart;
            SharedUserData->TimeZoneBias.High1Time = ExpTimeZoneBias.HighPart;

            /* Convert the boot time to local time, and set it */
            UniversalBootTime.QuadPart = SystemBootTime.QuadPart +
                                         ExpTimeZoneBias.QuadPart;
        }

        /* Update the system time and notify the system */
        KeSetSystemTime(&UniversalBootTime, &OldTime, FALSE, NULL);
        PoNotifySystemTimeSet();

        /* Remember this as the boot time */
        KeBootTime = UniversalBootTime;
        KeBootTimeBias = 0;
    }

#ifdef CONFIG_SMP
    /* Start Application Processors */
    KeStartAllProcessors();
#endif

    /* Initialize all processors */
    if (!HalAllProcessorsStarted()) KeBugCheck(HAL1_INITIALIZATION_FAILED);

#ifdef CONFIG_SMP
    /* HACK: We should use RtlFindMessage and not only fallback to this */
    MpString = "MultiProcessor Kernel\r\n";
#endif

    /* Setup the "MP" String */
    RtlInitAnsiString(&TempString, MpString);

    /* Make sure to remove the \r\n if we actually have a string */
    while ((TempString.Length > 0) &&
           ((TempString.Buffer[TempString.Length - 1] == '\r') ||
            (TempString.Buffer[TempString.Length - 1] == '\n')))
    {
        /* Skip the trailing character */
        TempString.Length--;
    }

    /* Get the information string from our resource file */
    MsgStatus = RtlFindMessage(NtosEntry->DllBase,
                               11,
                               0,
                               KeNumberProcessors > 1 ?
                               WINDOWS_NT_INFO_STRING_PLURAL :
                               WINDOWS_NT_INFO_STRING,
                               &MsgEntry);

    /* Get total RAM size, in MiB */
    /* Round size up. Assumed to better match actual physical RAM size */
    Size = ALIGN_UP_BY(MmNumberOfPhysicalPages * PAGE_SIZE, 1024 * 1024) / (1024 * 1024);

    /* Create the string */
    StringBuffer = InitBuffer->VersionBuffer;
    Status = RtlStringCbPrintfA(StringBuffer,
                                sizeof(InitBuffer->VersionBuffer),
                                NT_SUCCESS(MsgStatus) ?
                                (PCHAR)MsgEntry->Text :
                                "%u System Processor [%Iu MB Memory] %Z\r\n",
                                KeNumberProcessors,
                                Size,
                                &TempString);
    if (!NT_SUCCESS(Status))
    {
        /* Bugcheck */
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 4, 0, 0);
    }

    /* Display RAM and CPU count */
    InbvDisplayString(StringBuffer);

    /* Update the progress bar */
    InbvUpdateProgressBar(5);

    /* Call OB initialization again */
    if (!ObInitSystem()) KeBugCheck(OBJECT1_INITIALIZATION_FAILED);

    /* Initialize Basic System Objects and Worker Threads */
    if (!ExInitSystem()) KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, 0, 0, 1, 0);

    /* Initialize the later stages of the kernel */
    if (!KeInitSystem()) KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, 0, 0, 2, 0);

    /* Call KD Providers at Phase 1 */
    if (!KdInitSystem(ExpInitializationPhase, KeLoaderBlock))
    {
        /* Failed, bugcheck */
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, 0, 0, 3, 0);
    }

    /* Initialize the SRM in Phase 1 */
    if (!SeInitSystem()) KeBugCheck(SECURITY1_INITIALIZATION_FAILED);

    /* Update the progress bar */
    InbvUpdateProgressBar(10);

    /* Create SystemRoot Link */
    Status = ExpCreateSystemRootLink(LoaderBlock);
    if (!NT_SUCCESS(Status))
    {
        /* Failed to create the system root link */
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Set up Region Maps, Sections and the Paging File */
    if (!MmInitSystem(1, LoaderBlock)) KeBugCheck(MEMORY1_INITIALIZATION_FAILED);

    /* Create NLS section */
    ExpInitNls(LoaderBlock);

    /* Initialize Cache Views */
    if (!CcInitializeCacheManager()) KeBugCheck(CACHE_INITIALIZATION_FAILED);

    /* Initialize the Registry */
    if (!CmInitSystem1()) KeBugCheck(CONFIG_INITIALIZATION_FAILED);

    /* Initialize Prefetcher */
    CcPfInitializePrefetcher();

    /* Update progress bar */
    InbvUpdateProgressBar(15);

    /* Update timezone information */
    LastTzBias = ExpLastTimeZoneBias;
    ExRefreshTimeZoneInformation(&SystemBootTime);

    /* Check if we're resetting timezone data */
    if (ResetBias)
    {
        /* Convert the local time to system time */
        ExLocalTimeToSystemTime(&SystemBootTime, &UniversalBootTime);
        KeBootTime = UniversalBootTime;
        KeBootTimeBias = 0;

        /* Set the new time */
        KeSetSystemTime(&UniversalBootTime, &OldTime, FALSE, NULL);
    }
    else
    {
        /* Check if the timezone switched and update the time */
        if (LastTzBias != ExpLastTimeZoneBias)
            ZwSetSystemTime(NULL, NULL);
    }

    /* Initialize the File System Runtime Library */
    if (!FsRtlInitSystem()) KeBugCheck(FILE_INITIALIZATION_FAILED);

    /* Initialize range lists */
    RtlInitializeRangeListPackage();

    /* Report all resources used by HAL */
    HalReportResourceUsage();

    /* Call the debugger DLL */
    KdDebuggerInitialize1(LoaderBlock);

    /* Setup PnP Manager in phase 1 */
    if (!PpInitSystem()) KeBugCheck(PP1_INITIALIZATION_FAILED);

    /* Update progress bar */
    InbvUpdateProgressBar(20);

    /* Initialize LPC */
    if (!LpcInitSystem()) KeBugCheck(LPC_INITIALIZATION_FAILED);

    /* Make sure we have a command line */
    if (CommandLine)
    {
        /* Check if this is a safe mode boot */
        SafeBoot = strstr(CommandLine, "SAFEBOOT:");
        if (SafeBoot)
        {
            /* Check what kind of boot this is */
            SafeBoot += 9;
            if (!strncmp(SafeBoot, "MINIMAL", 7))
            {
                /* Minimal mode */
                InitSafeBootMode = 1;
                SafeBoot += 7;
                MessageCode = BOOTING_IN_SAFEMODE_MINIMAL;
            }
            else if (!strncmp(SafeBoot, "NETWORK", 7))
            {
                /* With Networking */
                InitSafeBootMode = 2;
                SafeBoot += 7;
                MessageCode = BOOTING_IN_SAFEMODE_NETWORK;
            }
            else if (!strncmp(SafeBoot, "DSREPAIR", 8))
            {
                /* Domain Server Repair */
                InitSafeBootMode = 3;
                SafeBoot += 8;
                MessageCode = BOOTING_IN_SAFEMODE_DSREPAIR;

            }
            else
            {
                /* Invalid */
                InitSafeBootMode = 0;
            }

            /* Check if there's any settings left */
            if (*SafeBoot)
            {
                /* Check if an alternate shell was requested */
                if (!strncmp(SafeBoot, "(ALTERNATESHELL)", 16))
                {
                    /* Remember this for later */
                    AlternateShell = TRUE;
                }
            }

            /* Find the message to print out */
            Status = RtlFindMessage(NtosEntry->DllBase,
                                    11,
                                    0,
                                    MessageCode,
                                    &MsgEntry);
            if (NT_SUCCESS(Status))
            {
                /* Display it */
                InbvDisplayString((PCHAR)MsgEntry->Text);
            }
        }
    }

    /* Make sure we have a command line */
    if (CommandLine)
    {
        /* Check if bootlogging is enabled */
        if (strstr(CommandLine, "BOOTLOG"))
        {
            /* Find the message to print out */
            Status = RtlFindMessage(NtosEntry->DllBase,
                                    11,
                                    0,
                                    BOOTLOG_ENABLED,
                                    &MsgEntry);
            if (NT_SUCCESS(Status))
            {
                /* Display it */
                InbvDisplayString((PCHAR)MsgEntry->Text);
            }

            /* Setup boot logging */
            //IopInitializeBootLogging(LoaderBlock, InitBuffer->BootlogHeader);
        }
    }

    /* Setup the Executive in Phase 2 */
    //ExInitSystemPhase2();

    /* Update progress bar */
    InbvUpdateProgressBar(25);

    /* No KD Time Slip is pending */
    KdpTimeSlipPending = 0;

    /* Initialize in-place execution support */
    XIPInit(LoaderBlock);

    /* Set maximum update to 75% */
    InbvSetProgressBarSubset(25, 75);

    /* Initialize the I/O Subsystem */
    if (!IoInitSystem(LoaderBlock)) KeBugCheck(IO1_INITIALIZATION_FAILED);

    /* Set maximum update to 100% */
    InbvSetProgressBarSubset(0, 100);

    /* Are we in safe mode? */
    if (InitSafeBootMode)
    {
        /* Open the safe boot key */
        RtlInitUnicodeString(&KeyName,
                             L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET"
                             L"\\CONTROL\\SAFEBOOT");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = ZwOpenKey(&KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            /* First check if we have an alternate shell */
            if (AlternateShell)
            {
                /* Make sure that the registry has one setup */
                RtlInitUnicodeString(&KeyName, L"AlternateShell");
                Status = NtQueryValueKey(KeyHandle,
                                         &KeyName,
                                         KeyValuePartialInformation,
                                         &KeyPartialInfo,
                                         sizeof(KeyPartialInfo),
                                         &Length);
                if (!(NT_SUCCESS(Status) || Status == STATUS_BUFFER_OVERFLOW))
                {
                    AlternateShell = FALSE;
                }
            }

            /* Create the option key */
            RtlInitUnicodeString(&KeyName, L"Option");
            InitializeObjectAttributes(&ObjectAttributes,
                                       &KeyName,
                                       OBJ_CASE_INSENSITIVE,
                                       KeyHandle,
                                       NULL);
            Status = ZwCreateKey(&OptionHandle,
                                 KEY_ALL_ACCESS,
                                 &ObjectAttributes,
                                 0,
                                 NULL,
                                 REG_OPTION_VOLATILE,
                                 &Disposition);
            NtClose(KeyHandle);

            /* Check if the key create worked */
            if (NT_SUCCESS(Status))
            {
                /* Write the safe boot type */
                RtlInitUnicodeString(&KeyName, L"OptionValue");
                NtSetValueKey(OptionHandle,
                              &KeyName,
                              0,
                              REG_DWORD,
                              &InitSafeBootMode,
                              sizeof(InitSafeBootMode));

                /* Check if we have to use an alternate shell */
                if (AlternateShell)
                {
                    /* Remember this for later */
                    Disposition = TRUE;
                    RtlInitUnicodeString(&KeyName, L"UseAlternateShell");
                    NtSetValueKey(OptionHandle,
                                  &KeyName,
                                  0,
                                  REG_DWORD,
                                  &Disposition,
                                  sizeof(Disposition));
                }

                /* Close the options key handle */
                NtClose(OptionHandle);
            }
        }
    }

    /* Are we in Win PE mode? */
    if (InitIsWinPEMode)
    {
        /* Open the safe control key */
        RtlInitUnicodeString(&KeyName,
                             L"\\REGISTRY\\MACHINE\\SYSTEM\\CURRENTCONTROLSET"
                             L"\\CONTROL");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);
        Status = ZwOpenKey(&KeyHandle, KEY_ALL_ACCESS, &ObjectAttributes);
        if (!NT_SUCCESS(Status))
        {
            /* Bugcheck */
            KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 6, 0, 0);
        }

        /* Create the MiniNT key */
        RtlInitUnicodeString(&KeyName, L"MiniNT");
        InitializeObjectAttributes(&ObjectAttributes,
                                   &KeyName,
                                   OBJ_CASE_INSENSITIVE,
                                   KeyHandle,
                                   NULL);
        Status = ZwCreateKey(&OptionHandle,
                             KEY_ALL_ACCESS,
                             &ObjectAttributes,
                             0,
                             NULL,
                             REG_OPTION_VOLATILE,
                             &Disposition);
        if (!NT_SUCCESS(Status))
        {
            /* Bugcheck */
            KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, Status, 6, 0, 0);
        }

        /* Close the handles */
        NtClose(KeyHandle);
        NtClose(OptionHandle);
    }

    /* FIXME: This doesn't do anything for now */
    MmArmInitSystem(2, LoaderBlock);

    /* Update progress bar */
    InbvUpdateProgressBar(80);

    /* Initialize VDM support */
#if defined(_M_IX86)
    KeI386VdmInitialize();
#endif

    /* Initialize Power Subsystem in Phase 1*/
    if (!PoInitSystem(1)) KeBugCheck(INTERNAL_POWER_ERROR);

    /* Update progress bar */
    InbvUpdateProgressBar(90);

    /* Initialize the Process Manager at Phase 1 */
    if (!PsInitSystem(LoaderBlock)) KeBugCheck(PROCESS1_INITIALIZATION_FAILED);

    /* Make sure nobody touches the loader block again */
    if (LoaderBlock == KeLoaderBlock) KeLoaderBlock = NULL;
    MmFreeLoaderBlock(LoaderBlock);
    LoaderBlock = Context = NULL;

    /* Initialize the SRM in phase 1 */
    if (!SeRmInitPhase1()) KeBugCheck(PROCESS1_INITIALIZATION_FAILED);

    /* Update progress bar */
    InbvUpdateProgressBar(100);

    /* Clear the screen */
    if (InbvBootDriverInstalled) FinalizeBootLogo();

    /* Allow strings to be displayed */
    InbvEnableDisplayString(TRUE);

    /* Launch initial process */
    ProcessInfo = &InitBuffer->ProcessInfo;
    ExpLoadInitialProcess(InitBuffer, &ProcessParameters, &Environment);

    /* Wait 5 seconds for initial process to initialize */
    Timeout.QuadPart = Int32x32To64(5, -10000000);
    Status = ZwWaitForSingleObject(ProcessInfo->ProcessHandle, FALSE, &Timeout);
    if (Status == STATUS_SUCCESS)
    {
        /* Failed, display error */
        DPRINT1("INIT: Session Manager terminated.\n");

        /* Bugcheck the system if SMSS couldn't initialize */
        KeBugCheck(SESSION5_INITIALIZATION_FAILED);
    }

    /* Close process handles */
    ZwClose(ProcessInfo->ThreadHandle);
    ZwClose(ProcessInfo->ProcessHandle);

    /* Free the initial process environment */
    Size = 0;
    ZwFreeVirtualMemory(NtCurrentProcess(),
                        (PVOID*)&Environment,
                        &Size,
                        MEM_RELEASE);

    /* Free the initial process parameters */
    Size = 0;
    ZwFreeVirtualMemory(NtCurrentProcess(),
                        (PVOID*)&ProcessParameters,
                        &Size,
                        MEM_RELEASE);

    /* Increase init phase */
    ExpInitializationPhase++;

    /* Free the boot buffer */
    ExFreePoolWithTag(InitBuffer, TAG_INIT);
}

VOID
NTAPI
Phase1Initialization(IN PVOID Context)
{
    /* Do the .INIT part of Phase 1 which we can free later */
    Phase1InitializationDiscard(Context);

    /* Jump into zero page thread */
    MmZeroPageThread();
}
