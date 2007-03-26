/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ex/init.c
 * PURPOSE:         Executive Initialization Code
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>
//#include <ntoskrnl/cm/newcm.h>
#include "ntoskrnl/cm/cm.h"
#include <ntverp.h>

/* DATA **********************************************************************/

/* NT Version Info */
ULONG NtMajorVersion = VER_PRODUCTMAJORVERSION;
ULONG NtMinorVersion = VER_PRODUCTMINORVERSION;
#if DBG
ULONG NtBuildNumber = VER_PRODUCTBUILD | 0xC0000000;
#else
ULONG NtBuildNumber = VER_PRODUCTBUILD;
#endif

/* NT System Info */
ULONG NtGlobalFlag;
ULONG ExSuiteMask;

/* Cm Version Info */
ULONG CmNtSpBuildNumber;
ULONG CmNtCSDVersion;
ULONG CmNtCSDReleaseType;
UNICODE_STRING CmVersionString;
UNICODE_STRING CmCSDVersionString;
CHAR NtBuildLab[] = KERNEL_VERSION_BUILD_STR;

/* Init flags and settings */
ULONG ExpInitializationPhase;
BOOLEAN ExpInTextModeSetup;
BOOLEAN IoRemoteBootClient;
ULONG InitSafeBootMode;
BOOLEAN InitIsWinPEMode, InitWinPEModeType;

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
ULONG ExpNlsTableSize;
PVOID ExpNlsSectionPointer;

/* CMOS Timer Sanity */
BOOLEAN ExCmosClockIsSane = TRUE;

/* FUNCTIONS ****************************************************************/

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

VOID
NTAPI
ExpInitNls(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    LARGE_INTEGER SectionSize;
    NTSTATUS Status;
    HANDLE NlsSection;
    PVOID SectionBase = NULL;
    ULONG ViewSize = 0;
    LARGE_INTEGER SectionOffset = {{0}};
    PLIST_ENTRY ListHead, NextEntry;
    PMEMORY_ALLOCATION_DESCRIPTOR MdBlock;
    ULONG NlsTablesEncountered = 0;
    ULONG NlsTableSizes[3]; /* 3 NLS tables */

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
                                                TAG('R', 't', 'l', 'i'));
        if (!ExpNlsTableBase) KeBugCheck(PHASE0_INITIALIZATION_FAILED);

        /* Copy the codepage data in its new location. */
        //RtlCopyMemory(ExpNlsTableBase,
        //              LoaderBlock->NlsData->AnsiCodePageData,
        //              ExpNlsTableSize);

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
                             SEC_COMMIT,
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
    ZwClose(NlsSection);
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
    RtlCopyMemory(SectionBase, ExpNlsTableBase, ExpNlsTableSize);

    /* Free the previously allocated buffer and set the new location */
    ExFreePool(ExpNlsTableBase);
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

NTSTATUS
NTAPI
ExpLoadInitialProcess(IN OUT PRTL_USER_PROCESS_INFORMATION ProcessInformation)
{
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters = NULL;
    NTSTATUS Status;
    ULONG Size;
    PWSTR p;
    UNICODE_STRING NullString = RTL_CONSTANT_STRING(L"");
    UNICODE_STRING SmssName, Environment, SystemDriveString;
    PVOID EnvironmentPtr = NULL;

    /* Allocate memory for the process parameters */
    Size = sizeof(RTL_USER_PROCESS_PARAMETERS) +
           ((MAX_PATH * 6) * sizeof(WCHAR));
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     (PVOID)&ProcessParameters,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(SESSION1_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Setup the basic header, and give the process the low 1MB to itself */
    ProcessParameters->Length = Size;
    ProcessParameters->MaximumLength = Size;
    ProcessParameters->Flags = RTL_USER_PROCESS_PARAMETERS_NORMALIZED |
                               RTL_USER_PROCESS_PARAMETERS_RESERVE_1MB;

    /* Allocate a page for the environment */
    Size = PAGE_SIZE;
    Status = ZwAllocateVirtualMemory(NtCurrentProcess(),
                                     &EnvironmentPtr,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(SESSION2_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Write the pointer */
    ProcessParameters->Environment = EnvironmentPtr;

    /* Make a buffer for the DOS path */
    p = (PWSTR)(ProcessParameters + 1);
    ProcessParameters->CurrentDirectory.DosPath.Buffer = p;
    ProcessParameters->
        CurrentDirectory.DosPath.MaximumLength = MAX_PATH * sizeof(WCHAR);

    /* Copy the DOS path */
    RtlCopyUnicodeString(&ProcessParameters->CurrentDirectory.DosPath,
                         &NtSystemRoot);

    /* Make a buffer for the DLL Path */
    p = (PWSTR)((PCHAR)ProcessParameters->CurrentDirectory.DosPath.Buffer +
                ProcessParameters->CurrentDirectory.DosPath.MaximumLength);
    ProcessParameters->DllPath.Buffer = p;
    ProcessParameters->DllPath.MaximumLength = MAX_PATH * sizeof(WCHAR);

    /* Copy the DLL path and append the system32 directory */
    RtlCopyUnicodeString(&ProcessParameters->DllPath,
                         &ProcessParameters->CurrentDirectory.DosPath);
    RtlAppendUnicodeToString(&ProcessParameters->DllPath, L"\\System32");

    /* Make a buffer for the image name */
    p = (PWSTR)((PCHAR)ProcessParameters->DllPath.Buffer +
                ProcessParameters->DllPath.MaximumLength);
    ProcessParameters->ImagePathName.Buffer = p;
    ProcessParameters->ImagePathName.MaximumLength = MAX_PATH * sizeof(WCHAR);

    /* Make sure the buffer is a valid string which within the given length */
    if ((NtInitialUserProcessBufferType != REG_SZ) ||
        ((NtInitialUserProcessBufferLength != -1) &&
         ((NtInitialUserProcessBufferLength < sizeof(WCHAR)) ||
          (NtInitialUserProcessBufferLength >
           sizeof(NtInitialUserProcessBuffer) - sizeof(WCHAR)))))
    {
        /* Invalid initial process string, bugcheck */
        KeBugCheckEx(SESSION2_INITIALIZATION_FAILED,
                     (ULONG_PTR)STATUS_INVALID_PARAMETER,
                     NtInitialUserProcessBufferType,
                     NtInitialUserProcessBufferLength,
                     sizeof(NtInitialUserProcessBuffer));
    }

    /* Cut out anything after a space */
    p = NtInitialUserProcessBuffer;
    while (*p && *p != L' ') p++;

    /* Set the image path length */
    ProcessParameters->ImagePathName.Length =
        (USHORT)((PCHAR)p - (PCHAR)NtInitialUserProcessBuffer);

    /* Copy the actual buffer */
    RtlCopyMemory(ProcessParameters->ImagePathName.Buffer,
                  NtInitialUserProcessBuffer,
                  ProcessParameters->ImagePathName.Length);

    /* Null-terminate it */
    ProcessParameters->
        ImagePathName.Buffer[ProcessParameters->ImagePathName.Length /
                             sizeof(WCHAR)] = UNICODE_NULL;

    /* Make a buffer for the command line */
    p = (PWSTR)((PCHAR)ProcessParameters->ImagePathName.Buffer +
                ProcessParameters->ImagePathName.MaximumLength);
    ProcessParameters->CommandLine.Buffer = p;
    ProcessParameters->CommandLine.MaximumLength = MAX_PATH * sizeof(WCHAR);

    /* Add the image name to the command line */
    RtlAppendUnicodeToString(&ProcessParameters->CommandLine,
                             NtInitialUserProcessBuffer);

    /* Create the environment string */
    RtlInitEmptyUnicodeString(&Environment,
                              ProcessParameters->Environment,
                              (USHORT)Size);

    /* Append the DLL path to it */
    RtlAppendUnicodeToString(&Environment, L"Path=" );
    RtlAppendUnicodeStringToString(&Environment, &ProcessParameters->DllPath);
    RtlAppendUnicodeStringToString(&Environment, &NullString );

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

    /* Create SMSS process */
    SmssName = ProcessParameters->ImagePathName;
    Status = RtlCreateUserProcess(&SmssName,
                                  OBJ_CASE_INSENSITIVE,
                                  RtlDeNormalizeProcessParams(
                                  ProcessParameters),
                                  NULL,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  NULL,
                                  NULL,
                                  ProcessInformation);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(SESSION3_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Resume the thread */
    Status = ZwResumeThread(ProcessInformation->ThreadHandle, NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(SESSION4_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Return success */
    return STATUS_SUCCESS;
}

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

    /* Set the suite mask to maximum and return */
    ExSuiteMask = 0xFFFFFFFF;
    return TRUE;
}

BOOLEAN
NTAPI
ExpInitSystemPhase1(VOID)
{
    /* Initialize worker threads */
    ExpInitializeWorkerThreads();

    /* Initialize pushlocks */
    ExpInitializePushLocks();

    /* Initialize events and event pairs */
    ExpInitializeEventImplementation();
    ExpInitializeEventPairImplementation();

    /* Initialize callbacks */
    ExpInitializeCallbacks();

    /* Initialize mutants */
    ExpInitializeMutantImplementation();

    /* Initialize semaphores */
    ExpInitializeSemaphoreImplementation();

    /* Initialize timers */
    ExpInitializeTimerImplementation();

    /* Initialize profiling */
    ExpInitializeProfileImplementation();

    /* Initialize UUIDs */
    ExpInitUuids();

    /* Initialize Win32K */
    ExpWin32kInit();
    return TRUE;
}

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

BOOLEAN
NTAPI
ExpIsLoaderValid(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLOADER_PARAMETER_EXTENSION Extension;

    /* Get the loader extension */
    Extension = LoaderBlock->Extension;

    /* Validate the size (larger structures are OK, we'll just ignore them) */
    if (Extension->Size < sizeof(LOADER_PARAMETER_EXTENSION)) return FALSE;

    /* Don't validate upper versions */
    if (Extension->MajorVersion > 5) return TRUE;

    /* Fail if this is NT 4 */
    if (Extension->MajorVersion < 5) return FALSE;

    /* Fail if this is XP */
    if (Extension->MinorVersion < 2) return FALSE;

    /* This is 2003 or newer, approve it */
    return TRUE;
}

VOID
NTAPI
ExpLoadBootSymbols(IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    ULONG i = 0;
    PLIST_ENTRY NextEntry;
    ULONG Count, Length;
    PWCHAR Name;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    BOOLEAN OverFlow = FALSE;
    CHAR NameBuffer[256];
    ANSI_STRING SymbolString;

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
                    OverFlow = TRUE;
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
                }
            }
            else
            {
                /* This should be a driver, check if it fits */
                if (sizeof(NameBuffer) <
                    (sizeof("\\System32\\Drivers\\") +
                     NtSystemRoot.Length / sizeof(WCHAR) - sizeof(UNICODE_NULL) +
                     LdrEntry->BaseDllName.Length / sizeof(WCHAR) +
                     sizeof(ANSI_NULL)))
                {
                    /* Buffer too small */
                    OverFlow = TRUE;
                    while (TRUE);
                }
                else
                {
                    /* Otherwise build the name. HACKED for GCC :( */
                    sprintf(NameBuffer,
                            "%S\\System32\\Drivers\\%S",
                            &SharedUserData->NtSystemRoot[2],
                            LdrEntry->BaseDllName.Buffer);
                }
            }

            /* Check if the buffer was ok */
            if (!OverFlow)
            {
                /* Initialize the ANSI_STRING for the debugger */
                RtlInitString(&SymbolString, NameBuffer);

                /* Load the symbols */
                DbgLoadImageSymbols(&SymbolString,
                                    LdrEntry->DllBase,
                                    0xFFFFFFFF);
            }
        }

        /* Go to the next entry */
        i++;
        NextEntry = NextEntry->Flink;
    }
}

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
        if (LoaderBlock->SetupLdrBlock->Flags & 1) ExpInTextModeSetup = TRUE;

        /* Check if this is network boot */
        if (LoaderBlock->SetupLdrBlock->Flags & 2)
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
                if (PerfMem)
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
                if (PerfMem)
                {
                    /* FIXME: TODO */
                    DPRINT1("Burnable memory support not yet present."
                            "/BURNMEM option ignored.\n");
                }
            }
        }
    }

    /* Setup NLS Base and offsets */
    NlsData = LoaderBlock->NlsData;
    ExpNlsTableBase = NlsData->AnsiCodePageData;
    ExpAnsiCodePageDataOffset = 0;
    ExpOemCodePageDataOffset = ((ULONG_PTR)NlsData->OemCodePageData -
                                (ULONG_PTR)NlsData->AnsiCodePageData);
    ExpUnicodeCaseTableDataOffset = ((ULONG_PTR)NlsData->UnicodeCodePageData -
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
    RtlInitString(&AnsiPath, Buffer );
    Buffer[--AnsiPath.Length] = ANSI_NULL;

    /* Get the string from KUSER_SHARED_DATA's buffer */
    RtlInitEmptyUnicodeString(&NtSystemRoot,
                              SharedUserData->NtSystemRoot,
                              sizeof(SharedUserData->NtSystemRoot));

    /* Now fill it in */
    Status = RtlAnsiStringToUnicodeString(&NtSystemRoot, &AnsiPath, FALSE);
    if (!NT_SUCCESS(Status)) KEBUGCHECK(SESSION3_INITIALIZATION_FAILED);

    /* Setup bugcheck messages */
    KiInitializeBugCheck();

    /* Setup initial system settings (FIXME: Needs Cm Rewrite) */
    CmGetSystemControlValues(LoaderBlock->RegistryBase, CmControlVector);

    /* Initialize the executive at phase 0 */
    if (!ExInitSystem()) KEBUGCHECK(PHASE0_INITIALIZATION_FAILED);

    /* Initialize the memory manager at phase 0 */
    if (!MmInitSystem(0, LoaderBlock)) KeBugCheck(MEMORY1_INITIALIZATION_FAILED);

    /* Load boot symbols */
    ExpLoadBootSymbols(LoaderBlock);

    /* Check if we should break after symbol load */
    if (KdBreakAfterSymbolLoad) DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);

    /* Set system ranges */
    SharedUserData->Reserved1 = (ULONG_PTR)MmHighestUserAddress;
    SharedUserData->Reserved3 = (ULONG_PTR)MmSystemRangeStart;

    /* Make a copy of the NLS Tables */
    ExpInitNls(LoaderBlock);

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
                              TAG('C', 'a', 'l', 'l'));

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
    if (!ObInit()) KEBUGCHECK(OBJECT_INITIALIZATION_FAILED);

    /* Load basic Security for other Managers */
    if (!SeInit()) KEBUGCHECK(SECURITY_INITIALIZATION_FAILED);

    /* Initialize the Process Manager */
    if (!PsInitSystem(LoaderBlock)) KEBUGCHECK(PROCESS_INITIALIZATION_FAILED);

    /* Initialize the PnP Manager */
    if (!PpInitSystem()) KEBUGCHECK(PP0_INITIALIZATION_FAILED);

    /* Initialize the User-Mode Debugging Subsystem */
    DbgkInitialize();

    /* Calculate the tick count multiplier */
    ExpTickCountMultiplier = ExComputeTickCountMultiplier(KeMaximumIncrement);
    SharedUserData->TickCountMultiplier = ExpTickCountMultiplier;

    /* Set the OS Version */
    SharedUserData->NtMajorVersion = NtMajorVersion;
    SharedUserData->NtMinorVersion = NtMinorVersion;

    /* Set the machine type */
#if defined(_X86_)
    SharedUserData->ImageNumberLow = IMAGE_FILE_MACHINE_I386;
    SharedUserData->ImageNumberHigh = IMAGE_FILE_MACHINE_I386;
#elif defined(_PPC_) // <3 Arty
    SharedUserData->ImageNumberLow = IMAGE_FILE_MACHINE_POWERPC;
    SharedUserData->ImageNumberHigh = IMAGE_FILE_MACHINE_POWERPC;
#elif defined(_MIPS_)
    SharedUserData->ImageNumberLow = IMAGE_FILE_MACHINE_R4000;
    SharedUserData->ImageNumberHigh = IMAGE_FILE_MACHINE_R4000;
#else
#error "Unsupported ReactOS Target"
#endif
}

VOID
NTAPI
Phase1InitializationDiscard(PVOID Context)
{
    PLOADER_PARAMETER_BLOCK LoaderBlock = Context;
    PCHAR CommandLine, Y2KHackRequired;
    LARGE_INTEGER Timeout;
    NTSTATUS Status;
    TIME_FIELDS TimeFields;
    LARGE_INTEGER SystemBootTime, UniversalBootTime, OldTime;
    PRTL_USER_PROCESS_INFORMATION ProcessInfo;
    BOOLEAN SosEnabled, NoGuiBoot;
    ULONG YearHack = 0;

    /* Allocate initial process information */
    ProcessInfo = ExAllocatePoolWithTag(NonPagedPool,
                                        sizeof(RTL_USER_PROCESS_INFORMATION),
                                        TAG('I', 'n', 'i', 't'));
    if (!ProcessInfo)
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
    CommandLine = _strupr(LoaderBlock->LoadOptions);

    /* Check if GUI Boot is enabled */
    NoGuiBoot = (strstr(CommandLine, "NOGUIBOOT")) ? TRUE: FALSE;

    /* Get the SOS setting */
    SosEnabled = strstr(CommandLine, "SOS") ? TRUE: FALSE;

    /* Setup the boot driver */
    InbvEnableBootDriver(!NoGuiBoot);
    InbvDriverInitialize(LoaderBlock, 18);

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
    if (strstr(CommandLine, "MININT"))
    {
        /* Setup WinPE Settings */
        InitIsWinPEMode = TRUE;
        InitWinPEModeType |= (strstr(CommandLine, "INRAM")) ? 0x80000000 : 1;
    }

    /* FIXME: Print product name, version, and build */

    /* Initialize Power Subsystem in Phase 0 */
    if (!PoInitSystem(0, AcpiTableDetected)) KeBugCheck(INTERNAL_POWER_ERROR);

    /* Check for Y2K hack */
    Y2KHackRequired = strstr(CommandLine, "YEAR");
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

#if 0 // FIXME: Won't work until we can read registry data here
        /* FIXME: This assumes that the RTC is not already in GMT */
        ExpTimeZoneBias.QuadPart = Int32x32To64(ExpLastTimeZoneBias * 60,
                                                10000000);

        /* Set the boot time-zone bias */
        SharedUserData->TimeZoneBias.High2Time = ExpTimeZoneBias.HighPart;
        SharedUserData->TimeZoneBias.LowPart = ExpTimeZoneBias.LowPart;
        SharedUserData->TimeZoneBias.High1Time = ExpTimeZoneBias.HighPart;

        /* Convert the boot time to local time, and set it */
        UniversalBootTime.QuadPart = SystemBootTime.QuadPart +
                                     ExpTimeZoneBias.QuadPart;
#endif

        /* Update the system time */
        KeSetSystemTime(&UniversalBootTime, &OldTime, FALSE, NULL);

        /* Remember this as the boot time */
        KeBootTime = UniversalBootTime;
        KeBootTimeBias = 0;
    }

    /* Initialize all processors */
    if (!HalAllProcessorsStarted()) KeBugCheck(HAL1_INITIALIZATION_FAILED);

    /* FIXME: Print CPU and Memory */

    /* Update the progress bar */
    InbvUpdateProgressBar(5);

    /* Call OB initialization again */
    if (!ObInit()) KeBugCheck(OBJECT1_INITIALIZATION_FAILED);

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
    if (!SeInit()) KEBUGCHECK(SECURITY1_INITIALIZATION_FAILED);

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
    ExpInitNls(KeLoaderBlock);

    /* Initialize Cache Views */
    if (!CcInitializeCacheManager()) KeBugCheck(CACHE_INITIALIZATION_FAILED);

    /* Initialize the Registry */
    if (!CmInitSystem1()) KeBugCheck(CONFIG_INITIALIZATION_FAILED);

    /* Update progress bar */
    InbvUpdateProgressBar(15);

    /* Update timezone information */
    ExRefreshTimeZoneInformation(&SystemBootTime);

    /* Initialize the File System Runtime Library */
    if (!FsRtlInitSystem()) KeBugCheck(FILE_INITIALIZATION_FAILED);

    /* Report all resources used by HAL */
    HalReportResourceUsage();

    /* Call the debugger DLL once we have KD64 6.0 support */
    KdDebuggerInitialize1(LoaderBlock);

    /* Setup PnP Manager in phase 1 */
    if (!PpInitSystem()) KeBugCheck(PP1_INITIALIZATION_FAILED);

    /* Update progress bar */
    InbvUpdateProgressBar(20);

    /* Initialize LPC */
    if (!LpcInitSystem()) KeBugCheck(LPC_INITIALIZATION_FAILED);

    /* Initialize the I/O Subsystem */
    if (!IoInitSystem(KeLoaderBlock)) KeBugCheck(IO1_INITIALIZATION_FAILED);

    /* Unmap Low memory, and initialize the MPW and Balancer Thread */
    MmInitSystem(2, LoaderBlock);

    /* Update progress bar */
    InbvUpdateProgressBar(80);

    /* Initialize VDM support */
    KeI386VdmInitialize();

    /* Initialize Power Subsystem in Phase 1*/
    if (!PoInitSystem(1, AcpiTableDetected)) KeBugCheck(INTERNAL_POWER_ERROR);

    /* Initialize the Process Manager at Phase 1 */
    if (!PsInitSystem(LoaderBlock)) KeBugCheck(PROCESS1_INITIALIZATION_FAILED);

    /* Update progress bar */
    InbvUpdateProgressBar(85);

    /* Make sure nobody touches the loader block again */
    if (LoaderBlock == KeLoaderBlock) KeLoaderBlock = NULL;
    LoaderBlock = Context = NULL;

    /* Update progress bar */
    InbvUpdateProgressBar(90);

    /* Launch initial process */
    Status = ExpLoadInitialProcess(ProcessInfo);

    /* Update progress bar */
    InbvUpdateProgressBar(100);

    /* Allow strings to be displayed */
    InbvEnableDisplayString(TRUE);

    /* Wait 5 seconds for it to initialize */
    Timeout.QuadPart = Int32x32To64(5, -10000000);
    Status = ZwWaitForSingleObject(ProcessInfo->ProcessHandle, FALSE, &Timeout);
    if (InbvBootDriverInstalled) FinalizeBootLogo();

    if (Status == STATUS_SUCCESS)
    {
        /* Bugcheck the system if SMSS couldn't initialize */
        KeBugCheck(SESSION5_INITIALIZATION_FAILED);
    }

    /* Close process handles */
    ZwClose(ProcessInfo->ThreadHandle);
    ZwClose(ProcessInfo->ProcessHandle);

    /* FIXME: We should free the initial process' memory!*/

    /* Increase init phase */
    ExpInitializationPhase += 1;

    /* Free the process information */
    ExFreePool(ProcessInfo);
}

VOID
NTAPI
Phase1Initialization(IN PVOID Context)
{
    /* Do the .INIT part of Phase 1 which we can free later */
    Phase1InitializationDiscard(Context);

    /* Jump into zero page thread */
    MmZeroPageThreadMain(NULL);
}



