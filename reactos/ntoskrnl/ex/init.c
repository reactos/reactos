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
#include <internal/debug.h>

/* DATA **********************************************************************/

/* HACK */
extern BOOLEAN KiClockSetupComplete;

#define BUILD_OSCSDVERSION(major, minor) (((major & 0xFF) << 8) | (minor & 0xFF))

/* NT Version Info */
ULONG NtMajorVersion = 5;
ULONG NtMinorVersion = 0;
ULONG NtOSCSDVersion = BUILD_OSCSDVERSION(4, 0);
ULONG NtBuildNumber = KERNEL_VERSION_BUILD;
ULONG NtGlobalFlag;
ULONG ExSuiteMask;

/* Init flags and settings */
ULONG ExpInitializationPhase;
BOOLEAN ExpInTextModeSetup;
BOOLEAN IoRemoteBootClient;
ULONG InitSafeBootMode;

BOOLEAN NoGuiBoot = FALSE;

/* NT Boot Path */
UNICODE_STRING NtSystemRoot;

/* Boot NLS information */
PVOID ExpNlsTableBase;
ULONG ExpAnsiCodePageDataOffset, ExpOemCodePageDataOffset;
ULONG ExpUnicodeCaseTableDataOffset;
NLSTABLEINFO ExpNlsTableInfo;
ULONG ExpNlsTableSize;
PVOID ExpNlsSectionPointer;

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
                               SePublicDefaultSd);

    /* Create it */
    Status = NtCreateDirectoryObject(&LinkHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
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
                               SePublicDefaultSd);

    /* Create it */
    Status = NtCreateDirectoryObject(&LinkHandle,
                                     DIRECTORY_ALL_ACCESS,
                                     &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED, Status, 2, 0, 0);
    }

    /* Close the LinkHandle */
    ObCloseHandle(LinkHandle, KernelMode);

    /* Create the system root symlink name */
    RtlInitAnsiString(&AnsiName, "\\SystemRoot");
    Status = RtlAnsiStringToUnicodeString(&LinkName, &AnsiName, TRUE);
    if (!NT_SUCCESS(Status))
    {
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED, Status, 3, 0, 0);
    }

    /* Initialize the attributes for the link */
    InitializeObjectAttributes(&ObjectAttributes,
                               &LinkName,
                               OBJ_CASE_INSENSITIVE | OBJ_PERMANENT,
                               NULL,
                               SePublicDefaultSd);

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
            }

            /* Go to the next block */
            NextEntry = MdBlock->ListEntry.Flink;
        }

        /*
         * In NT, the memory blocks are contiguous, but in ReactOS they aren't,
         * so unless someone fixes FreeLdr, we'll have to use this icky hack.
         */
        ExpNlsTableSize += 2 * PAGE_SIZE; // BIAS FOR FREELDR. HACK!

        /* Allocate the a new buffer since loader memory will be freed */
        ExpNlsTableBase = ExAllocatePoolWithTag(NonPagedPool,
                                                ExpNlsTableSize,
                                                TAG('R', 't', 'l', 'i'));
        if (!ExpNlsTableBase) KeBugCheck(PHASE0_INITIALIZATION_FAILED);

        /* Copy the codepage data in its new location. */
        RtlCopyMemory(ExpNlsTableBase,
                      LoaderBlock->NlsData->AnsiCodePageData,
                      ExpNlsTableSize);

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

VOID
INIT_FUNCTION
ExpDisplayNotice(VOID)
{
    CHAR str[50];
   
    if (ExpInTextModeSetup)
    {
        HalDisplayString(
        "\n\n\n     ReactOS " KERNEL_VERSION_STR " Setup \n");
        HalDisplayString(
        "    \xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD\xCD");
        HalDisplayString(
        "\xCD\xCD\n");
        return;
    }
    
    HalDisplayString("Starting ReactOS "KERNEL_VERSION_STR" (Build "
                     KERNEL_VERSION_BUILD_STR")\n");
    HalDisplayString(RES_STR_LEGAL_COPYRIGHT);
    HalDisplayString("\n\nReactOS is free software, covered by the GNU General "
                     "Public License, and you\n");
    HalDisplayString("are welcome to change it and/or distribute copies of it "
                     "under certain\n");
    HalDisplayString("conditions. There is absolutely no warranty for "
                      "ReactOS.\n\n");

    /* Display number of Processors */
    sprintf(str,
            "Found %x system processor(s). [%lu MB Memory]\n",
            (int)KeNumberProcessors,
            (MmFreeLdrMemHigher + 1088)/ 1024);
    HalDisplayString(str);
    
}

NTSTATUS
NTAPI
ExpLoadInitialProcess(IN PHANDLE ProcessHandle,
                      IN PHANDLE ThreadHandle)
{
    PRTL_USER_PROCESS_PARAMETERS ProcessParameters = NULL;
    NTSTATUS Status;
    ULONG Size;
    RTL_USER_PROCESS_INFORMATION ProcessInformation;
    PWSTR p;
    UNICODE_STRING NullString = RTL_CONSTANT_STRING(L"");
    UNICODE_STRING SmssName, Environment, SystemDriveString;

    /* Allocate memory for the process parameters */
    Size = sizeof(RTL_USER_PROCESS_PARAMETERS) +
           ((MAX_PATH * 4) * sizeof(WCHAR));
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
                                     (PVOID)&ProcessParameters->Environment,
                                     0,
                                     &Size,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(SESSION2_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

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

    /* Append the system path and session manager name */
    RtlAppendUnicodeToString(&ProcessParameters->ImagePathName,
                             L"\\SystemRoot\\System32");
    RtlAppendUnicodeToString(&ProcessParameters->ImagePathName,
                             L"\\smss.exe");

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

    /* Get and set the command line equal to the image path */
    ProcessParameters->CommandLine = ProcessParameters->ImagePathName;
    SmssName = ProcessParameters->ImagePathName;

    /* Create SMSS process */
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
                                  &ProcessInformation);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(SESSION3_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Resume the thread */
    Status = ZwResumeThread(ProcessInformation.ThreadHandle, NULL);
    if (!NT_SUCCESS(Status))
    {
        /* Failed */
        KeBugCheckEx(SESSION4_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Return Handles */
    *ProcessHandle = ProcessInformation.ProcessHandle;
    *ThreadHandle = ProcessInformation.ThreadHandle;
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
ExpInitializeExecutive(IN ULONG Cpu,
                       IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PNLS_DATA_BLOCK NlsData;
    CHAR Buffer[256];
    ANSI_STRING AnsiPath;
    NTSTATUS Status;

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
    NtSystemRoot.Buffer = SharedUserData->NtSystemRoot;
    NtSystemRoot.MaximumLength = sizeof(SharedUserData->NtSystemRoot) / sizeof(WCHAR);
    NtSystemRoot.Length = 0;

    /* Now fill it in */
    Status = RtlAnsiStringToUnicodeString(&NtSystemRoot, &AnsiPath, FALSE);
    if (!NT_SUCCESS(Status)) KEBUGCHECK(SESSION3_INITIALIZATION_FAILED);

    /* Setup bugcheck messages */
    KiInitializeBugCheck();

    /* Initialize the executive at phase 0 */
    if (!ExInitSystem()) KEBUGCHECK(PHASE0_INITIALIZATION_FAILED);

    /* Break into the Debugger if requested */
    if (KdPollBreakIn()) DbgBreakPointWithStatus(DBG_STATUS_CONTROL_C);

    /* Set system ranges */
    SharedUserData->Reserved1 = (ULONG_PTR)MmHighestUserAddress;
    SharedUserData->Reserved3 = (ULONG_PTR)MmSystemRangeStart;

    /* Make a copy of the NLS Tables */
    ExpInitNls(LoaderBlock);

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

    /* Set up Region Maps, Sections and the Paging File */
    MmInit2();

    /* Initialize the boot video. */
    InbvDisplayInitialize();

    /* Initialize the Process Manager */
    if (!PsInitSystem()) KEBUGCHECK(PROCESS_INITIALIZATION_FAILED);

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
#elif
#error "Unsupported ReactOS Target"
#endif
}

VOID
NTAPI
ExPhase2Init(PVOID Context)
{
    LARGE_INTEGER Timeout;
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
    NTSTATUS Status;
    TIME_FIELDS TimeFields;
    LARGE_INTEGER SystemBootTime, UniversalBootTime;

    /* Set to phase 1 */
    ExpInitializationPhase = 1;

    /* Set us at maximum priority */
    KeSetPriorityThread(KeGetCurrentThread(), HIGH_PRIORITY);

    /* Do Phase 1 HAL Initialization */
    HalInitSystem(1, KeLoaderBlock);

    /* Check if GUI Boot is enabled */
    if (strstr(KeLoaderBlock->LoadOptions, "NOGUIBOOT")) NoGuiBoot = TRUE;

    /* Display the boot screen image if not disabled */
    if (!ExpInTextModeSetup) InbvDisplayInitialize2(NoGuiBoot);
    if (!NoGuiBoot) InbvDisplayBootLogo();

    /* Clear the screen to blue and display the boot notice and debug status */
    HalInitSystem(2, KeLoaderBlock);
    if (NoGuiBoot) ExpDisplayNotice();
    KdInitSystem(2, KeLoaderBlock);

    /* Initialize Power Subsystem in Phase 0 */
    PoInit(0, AcpiTableDetected);

    /* Query the clock */
    if (HalQueryRealTimeClock(&TimeFields))
    {
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
        KiSetSystemTime(&UniversalBootTime);

        /* Remember this as the boot time */
        KeBootTime = UniversalBootTime;
    }

    /* The clock is ready now (FIXME: HACK FOR OLD HAL) */
    KiClockSetupComplete = TRUE;

    /* Initialize all processors */
    HalAllProcessorsStarted();

    /* Call OB initialization again */
    if (!ObInit()) KeBugCheck(OBJECT1_INITIALIZATION_FAILED);

    /* Initialize Basic System Objects and Worker Threads */
    if (!ExInitSystem()) KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, 1, 0, 0, 0);

    /* Initialize the later stages of the kernel */
    if (!KeInitSystem()) KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, 2, 0, 0, 0);

    /* Call KD Providers at Phase 1 */
    if (!KdInitSystem(ExpInitializationPhase, KeLoaderBlock))
    {
        /* Failed, bugcheck */
        KeBugCheckEx(PHASE1_INITIALIZATION_FAILED, 3, 0, 0, 0);
    }

    /* Create SystemRoot Link */
    Status = ExpCreateSystemRootLink(KeLoaderBlock);
    if (!NT_SUCCESS(Status))
    {
        KeBugCheckEx(SYMBOLIC_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Create NLS section */
    ExpInitNls(KeLoaderBlock);

    /* Initialize Cache Views */
    CcInitializeCacheManager();

    /* Initialize the Registry */
    if (!CmInitSystem1()) KeBugCheck(CONFIG_INITIALIZATION_FAILED);

    /* Update timezone information */
    ExRefreshTimeZoneInformation(&SystemBootTime);

    /* Initialize the File System Runtime Library */
    FsRtlInitSystem();

    /* Report all resources used by HAL */
    HalReportResourceUsage();

    /* Initialize LPC */
    LpcpInitSystem();

    /* Enter the kernel debugger before starting up the boot drivers */
    if (KdDebuggerEnabled && KdpEarlyBreak) DbgBreakPoint();

    /* Initialize the I/O Subsystem */
    if (!IoInitSystem(KeLoaderBlock)) KeBugCheck(IO1_INITIALIZATION_FAILED);

    /* Unmap Low memory, and initialize the MPW and Balancer Thread */
    MmInit3();
#if DBG
    extern ULONG Guard;
#endif
    ASSERT(Guard == 0xCACA1234);

    /* Initialize VDM support */
    KeI386VdmInitialize();

    /* Initialize Power Subsystem in Phase 1*/
    PoInit(1, AcpiTableDetected);

    /* Initialize the Process Manager at Phase 1 */
    if (!PsInitSystem()) KeBugCheck(PROCESS1_INITIALIZATION_FAILED);

    /* Launch initial process */
    Status = ExpLoadInitialProcess(&ProcessHandle, &ThreadHandle);

    /* Wait 5 seconds for it to initialize */
    Timeout.QuadPart = Int32x32To64(5, -10000000);
    Status = ZwWaitForSingleObject(ProcessHandle, FALSE, &Timeout);
    if (!NoGuiBoot) InbvFinalizeBootLogo();
    if (Status == STATUS_SUCCESS)
    {
        /* Bugcheck the system if SMSS couldn't initialize */
        KeBugCheck(SESSION5_INITIALIZATION_FAILED);
    }
    else
    {
        /* Close process handles */
        ZwClose(ThreadHandle);
        ZwClose(ProcessHandle);

        /* FIXME: We should free the initial process' memory!*/

        /* Increase init phase */
        ExpInitializationPhase += 1;

        /* Jump into zero page thread */
        MmZeroPageThreadMain(NULL);
    }
}
/* EOF */
