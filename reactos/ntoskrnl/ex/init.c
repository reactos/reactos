/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/init.c
 * PURPOSE:         Executive initalization
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net) - Added ExpInitializeExecutive
 *                                                    and optimized/cleaned it.
 *                  Eric Kohl (ekohl@abo.rhein-zeitung.de)
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* DATA **********************************************************************/

#define BUILD_OSCSDVERSION(major, minor) (((major & 0xFF) << 8) | (minor & 0xFF))

/* NT Version Info */
ULONG NtMajorVersion = 5;
ULONG NtMinorVersion = 0;
ULONG NtOSCSDVersion = BUILD_OSCSDVERSION(4, 0);
ULONG NtBuildNumber = KERNEL_VERSION_BUILD;
ULONG NtGlobalFlag = 0;

extern ULONG MmCoreDumpType;
extern LOADER_MODULE KeLoaderModules[64];
extern ULONG KeLoaderModuleCount;
extern PRTL_MESSAGE_RESOURCE_DATA KiBugCodeMessages;
BOOLEAN NoGuiBoot = FALSE;
static BOOLEAN ForceAcpiDisable = FALSE;

/* Init flags and settings */
ULONG ExpInitializationPhase;
BOOLEAN ExpInTextModeSetup;
BOOLEAN IoRemoteBootClient;
ULONG InitSafeBootMode;

/* Boot NLS information */
PVOID ExpNlsTableBase;
ULONG ExpAnsiCodePageDataOffset, ExpOemCodePageDataOffset;
ULONG ExpUnicodeCaseTableDataOffset;
NLSTABLEINFO ExpNlsTableInfo;

/* FUNCTIONS ****************************************************************/

static
VOID
INIT_FUNCTION
InitSystemSharedUserPage (PCSZ ParameterLine)
{
    UNICODE_STRING ArcDeviceName;
    UNICODE_STRING ArcName;
    UNICODE_STRING BootPath;
    UNICODE_STRING DriveDeviceName;
    UNICODE_STRING DriveName;
    WCHAR DriveNameBuffer[20];
    PCHAR ParamBuffer;
    PWCHAR ArcNameBuffer;
    PCHAR p;
    NTSTATUS Status;
    ULONG Length;
    OBJECT_ATTRIBUTES ObjectAttributes;
    HANDLE Handle;
    ULONG i;
    BOOLEAN BootDriveFound = FALSE;

   /*
    * NOTE:
    *   The shared user page has been zeroed-out right after creation.
    *   There is NO need to do this again.
    */
    Ki386SetProcessorFeatures();

    /* Set the Version Data */
    SharedUserData->NtProductType = NtProductWinNt;
    SharedUserData->ProductTypeIsValid = TRUE;
    SharedUserData->NtMajorVersion = 5;
    SharedUserData->NtMinorVersion = 0;

    /*
     * Retrieve the current dos system path
     * (e.g.: C:\reactos) from the given arc path
     * (e.g.: multi(0)disk(0)rdisk(0)partititon(1)\reactos)
     * Format: "<arc_name>\<path> [options...]"
     */

    /* Create local parameter line copy */
    ParamBuffer = ExAllocatePool(PagedPool, 256);
    strcpy (ParamBuffer, (const char *)ParameterLine);
    DPRINT("%s\n", ParamBuffer);

    /* Cut options off */
    p = strchr (ParamBuffer, ' ');
    if (p) *p = 0;
    DPRINT("%s\n", ParamBuffer);

    /* Extract path */
    p = strchr (ParamBuffer, '\\');
    if (p) {

        DPRINT("Boot path: %s\n", p);
        RtlCreateUnicodeStringFromAsciiz (&BootPath, p);
        *p = 0;

    } else {

        DPRINT("Boot path: %s\n", "\\");
        RtlCreateUnicodeStringFromAsciiz (&BootPath, "\\");
    }
    DPRINT("Arc name: %s\n", ParamBuffer);

    /* Only ARC Name left - Build full ARC Name */
    ArcNameBuffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));
    swprintf (ArcNameBuffer, L"\\ArcName\\%S", ParamBuffer);
    RtlInitUnicodeString (&ArcName, ArcNameBuffer);
    DPRINT("Arc name: %wZ\n", &ArcName);

    /* Free ParamBuffer */
    ExFreePool (ParamBuffer);

    /* Allocate ARC Device Name string */
    ArcDeviceName.Length = 0;
    ArcDeviceName.MaximumLength = 256 * sizeof(WCHAR);
    ArcDeviceName.Buffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));

    /* Open the Symbolic Link */
    InitializeObjectAttributes(&ObjectAttributes,
                               &ArcName,
                               OBJ_OPENLINK,
                               NULL,
                               NULL);
    Status = NtOpenSymbolicLinkObject(&Handle,
                                      SYMBOLIC_LINK_ALL_ACCESS,
                                      &ObjectAttributes);

    /* Free the String */
    ExFreePool(ArcName.Buffer);

    /* Check for Success */
    if (!NT_SUCCESS(Status)) {

        /* Free the Strings */
        RtlFreeUnicodeString(&BootPath);
        ExFreePool(ArcDeviceName.Buffer);
        CPRINT("NtOpenSymbolicLinkObject() failed (Status %x)\n", Status);
        KEBUGCHECK(0);
    }

    /* Query the Link */
    Status = NtQuerySymbolicLinkObject(Handle,
                                       &ArcDeviceName,
                                       &Length);
    NtClose (Handle);

    /* Check for Success */
    if (!NT_SUCCESS(Status)) {

        /* Free the Strings */
        RtlFreeUnicodeString(&BootPath);
        ExFreePool(ArcDeviceName.Buffer);
        CPRINT("NtQuerySymbolicLinkObject() failed (Status %x)\n", Status);
        KEBUGCHECK(0);
    }
    DPRINT("Length: %lu ArcDeviceName: %wZ\n", Length, &ArcDeviceName);

    /* Allocate Device Name string */
    DriveDeviceName.Length = 0;
    DriveDeviceName.MaximumLength = 256 * sizeof(WCHAR);
    DriveDeviceName.Buffer = ExAllocatePool (PagedPool, 256 * sizeof(WCHAR));

    /* Loop Drives */
    for (i = 0; i < 26; i++)  {

        /* Setup the String */
        swprintf (DriveNameBuffer, L"\\??\\%C:", 'A' + i);
        RtlInitUnicodeString(&DriveName,
                             DriveNameBuffer);

        /* Open the Symbolic Link */
        InitializeObjectAttributes(&ObjectAttributes,
                                   &DriveName,
                                   OBJ_OPENLINK,
                                   NULL,
                                   NULL);
        Status = NtOpenSymbolicLinkObject(&Handle,
                                          SYMBOLIC_LINK_ALL_ACCESS,
                                          &ObjectAttributes);

        /* If it failed, skip to the next drive */
        if (!NT_SUCCESS(Status)) {
            DPRINT("Failed to open link %wZ\n", &DriveName);
            continue;
        }

        /* Query it */
        Status = NtQuerySymbolicLinkObject(Handle,
                                           &DriveDeviceName,
                                           &Length);

        /* If it failed, skip to the next drive */
        if (!NT_SUCCESS(Status)) {
            DPRINT("Failed to query link %wZ\n", &DriveName);
            continue;
        }
        DPRINT("Opened link: %wZ ==> %wZ\n", &DriveName, &DriveDeviceName);

        /* See if we've found the boot drive */
        if (!RtlCompareUnicodeString (&ArcDeviceName, &DriveDeviceName, FALSE)) {

            DPRINT("DOS Boot path: %c:%wZ\n", 'A' + i, &BootPath);
            swprintf(SharedUserData->NtSystemRoot, L"%C:%wZ", 'A' + i, &BootPath);
            BootDriveFound = TRUE;
        }

        /* Close this Link */
        NtClose (Handle);
    }

    /* Free all the Strings we have in memory */
    RtlFreeUnicodeString (&BootPath);
    ExFreePool(DriveDeviceName.Buffer);
    ExFreePool(ArcDeviceName.Buffer);

    /* Make sure we found the Boot Drive */
    if (BootDriveFound == FALSE) {

        DbgPrint("No system drive found!\n");
        KEBUGCHECK (NO_BOOT_DEVICE);
    }
}

__inline
VOID
STDCALL
ParseCommandLine(PBOOLEAN NoGuiBoot,
                 PBOOLEAN ForceAcpiDisable)
{
    PCHAR p1, p2;

    p1 = KeLoaderBlock->LoadOptions;
    while(*p1 && (p2 = strchr(p1, '/'))) {

        p2++;
        if (!_strnicmp(p2, "NOGUIBOOT", 9)) {

            p2 += 9;
            *NoGuiBoot = TRUE;

        } else if (!_strnicmp(p2, "CRASHDUMP", 9)) {

            p2 += 9;
            if (*p2 == ':') {

                p2++;
                if (!_strnicmp(p2, "FULL", 4)) {

                    MmCoreDumpType = MM_CORE_DUMP_TYPE_FULL;

                } else {

                    MmCoreDumpType = MM_CORE_DUMP_TYPE_NONE;
                }
            }
        } else if (!_strnicmp(p2, "NOACPI", 6)) {

            p2 += 6;
            *ForceAcpiDisable = TRUE;
        }

        p1 = p2;
    }
}

VOID
FORCEINLINE
ParseAndCacheLoadedModules(VOID)
{
    ULONG i;
    PCHAR Name;

    /* Loop the Module List and get the modules we want */
    for (i = 1; i < KeLoaderModuleCount; i++)
    {
        /* Get the Name of this Module */
        if (!(Name = strrchr((PCHAR)KeLoaderModules[i].String, '\\')))
        {
            /* Save the name */
            Name = (PCHAR)KeLoaderModules[i].String;
        }
        else
        {
            /* No name, skip */
            Name++;
        }

        /* Now check for any of the modules we will need later */
        if (!_stricmp(Name, "ansi.nls"))
        {
            CachedModules[AnsiCodepage] = &KeLoaderModules[i];
        }
        else if (!_stricmp(Name, "oem.nls"))
        {
            CachedModules[OemCodepage] = &KeLoaderModules[i];
        }
        else if (!_stricmp(Name, "casemap.nls"))
        {
            CachedModules[UnicodeCasemap] = &KeLoaderModules[i];
        }
        else if (!_stricmp(Name, "system") || !_stricmp(Name, "system.hiv"))
        {
            CachedModules[SystemRegistry] = &KeLoaderModules[i];
        }
        else if (!_stricmp(Name, "hardware") || !_stricmp(Name, "hardware.hiv"))
        {
            CachedModules[HardwareRegistry] = &KeLoaderModules[i];
        }
    }
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

INIT_FUNCTION
NTSTATUS
ExpLoadInitialProcess(PHANDLE ProcessHandle,
                      PHANDLE ThreadHandle)
{
    UNICODE_STRING CurrentDirectory;
    UNICODE_STRING ImagePath = RTL_CONSTANT_STRING(L"\\SystemRoot\\system32\\smss.exe");
    NTSTATUS Status;
    PRTL_USER_PROCESS_PARAMETERS Params=NULL;
    RTL_USER_PROCESS_INFORMATION Info;

    RtlInitUnicodeString(&CurrentDirectory,
                         SharedUserData->NtSystemRoot);

    /* Create the Parameters */
    Status = RtlCreateProcessParameters(&Params,
                                        &ImagePath,
                                        NULL,
                                        &CurrentDirectory,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL,
                                        NULL);
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create ppb!\n");
        return Status;
    }

    DPRINT("Creating process\n");
    Status = RtlCreateUserProcess(&ImagePath,
                                  OBJ_CASE_INSENSITIVE,
                                  Params,
                                  NULL,
                                  NULL,
                                  NULL,
                                  FALSE,
                                  NULL,
                                  NULL,
                                  &Info);
    
    /* Close the handle and free the params */
    RtlDestroyProcessParameters(Params);

    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtCreateProcess() failed (Status %lx)\n", Status);
        return(Status);
    }

    /* Start it up */
    ZwResumeThread(Info.ThreadHandle, NULL);

    /* Return Handles */
    *ProcessHandle = Info.ProcessHandle;
    *ThreadHandle = Info.ThreadHandle;
    DPRINT("Process created successfully\n");
    return STATUS_SUCCESS;
}

VOID
NTAPI
ExInit2(VOID)
{
    ExpInitLookasideLists();
    ExpInitializeHandleTables();
}

VOID
NTAPI
ExInit3(VOID)
{
    ExpInitializeEventImplementation();
    ExpInitializeEventPairImplementation();
    ExpInitializeMutantImplementation();
    ExpInitializeSemaphoreImplementation();
    ExpInitializeTimerImplementation();
    LpcpInitSystem();
    ExpInitializeProfileImplementation();
    ExpWin32kInit();
    ExpInitUuids();
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

    /* This is 2003 or newer, aprove it */
    return TRUE;
}

VOID
NTAPI
ExpInitializeExecutive(IN ULONG Cpu,
                       IN PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PNLS_DATA_BLOCK NlsData;

    /* Parse Command Line Settings */
    ParseCommandLine(&NoGuiBoot, &ForceAcpiDisable);

    /* FIXME: Deprecate soon */
    ParseAndCacheLoadedModules();

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
            KEBUGCHECK(HAL_INITIALIZATION_FAILED);
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

    /* Setup bugcheck messages */
    KiInitializeBugCheck();

    /* Setup system time */
    KiInitializeSystemClock();

    /* Initialize the second stage of the kernel */
    KeInit2();

    /* Initialize resources */
    ExpResourceInitialization();

    /* Load basic Security for other Managers */
    if (!SeInit1()) KEBUGCHECK(SECURITY_INITIALIZATION_FAILED);

    /* Initialize Lookaside Lists and Handle Table */
    ExInit2();

    /* Create the Basic Object Manager Types to allow new Object Types */
    ObInit();

    /* Set up Region Maps, Sections and the Paging File */
    MmInit2();

    /* Initialize Tokens now that the Object Manager is ready */
    if (!SeInit2()) KEBUGCHECK(SECURITY1_INITIALIZATION_FAILED);

    /* Initalize the Process Manager */
    PspInitPhase0();

    /* Break into the Debugger if requested */
    if (KdPollBreakIn()) DbgBreakPointWithStatus (DBG_STATUS_CONTROL_C);

    /* Initialize all processors */
    HalAllProcessorsStarted();

    /* Do Phase 1 HAL Initalization */
    HalInitSystem(1, KeLoaderBlock);
}

VOID
NTAPI
ExPhase2Init(PVOID Context)
{
    UNICODE_STRING EventName;
    HANDLE InitDoneEventHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    LARGE_INTEGER Timeout;
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
    NTSTATUS Status;

    /* Set us at maximum priority */
    KeSetPriorityThread(KeGetCurrentThread(), HIGH_PRIORITY);

    /* Initialize Basic System Objects and Worker Threads */
    ExInit3();

    /* initialize the worker threads */
    ExpInitializeWorkerThreads();

    /* initialize callbacks */
    ExpInitializeCallbacks();

    /* Call KD Providers at Phase 1 */
    KdInitSystem(1, KeLoaderBlock);

    /* Initialize I/O Objects, Filesystems, Error Logging and Shutdown */
    IoInit();

    /* TBD */
    PoInit(AcpiTableDetected, ForceAcpiDisable);

    /* Initialize the Registry (Hives are NOT yet loaded!) */
    CmInitializeRegistry();

    /* Unmap Low memory, initialize the Page Zeroing and the Balancer Thread */
    MmInit3();

    /* Initialize Cache Views */
    CcInit();

    /* Initialize File Locking */
    FsRtlpInitFileLockingImplementation();

    /* Report all resources used by hal */
    HalReportResourceUsage();

    /* Clear the screen to blue */
    HalInitSystem(2, KeLoaderBlock);

    /* Display version number and copyright/warranty message */
    if (NoGuiBoot) ExpDisplayNotice();

    /* Call KD Providers at Phase 2 */
    KdInitSystem(2, KeLoaderBlock);

    /* Import and create NLS Data and Sections */
    RtlpInitNls();

    /* Import and Load Registry Hives */
    CmInitHives(ExpInTextModeSetup);

    /* Initialize VDM support */
    KeI386VdmInitialize();

    /* Initialize the time zone information from the registry */
    ExpInitTimeZoneInfo();

    /* Enter the kernel debugger before starting up the boot drivers */
    if (KdDebuggerEnabled && KdpEarlyBreak)
        DbgBreakPoint();

    /* Setup Drivers and Root Device Node */
    IoInit2(FALSE);

    /* Display the boot screen image if not disabled */
    if (!NoGuiBoot) InbvEnableBootDriver(TRUE);

    /* Create ARC Names, SystemRoot SymLink, Load Drivers and Assign Letters */
    IoInit3();

    /* Load the System DLL and its Entrypoints */
    PsLocateSystemDll();

    /* Initialize shared user page. Set dos system path, dos device map, etc. */
    InitSystemSharedUserPage (KeLoaderBlock->LoadOptions);

    /* Create 'ReactOSInitDone' event */
    RtlInitUnicodeString(&EventName, L"\\ReactOSInitDone");
    InitializeObjectAttributes(&ObjectAttributes,
                               &EventName,
                               0,
                               NULL,
                               NULL);
    Status = ZwCreateEvent(&InitDoneEventHandle,
                           EVENT_ALL_ACCESS,
                           &ObjectAttributes,
                           SynchronizationEvent,
                           FALSE);

    /* Check for Success */
    if (!NT_SUCCESS(Status)) {

        DPRINT1("Failed to create 'ReactOSInitDone' event (Status 0x%x)\n", Status);
        InitDoneEventHandle = INVALID_HANDLE_VALUE;
    }

    /* Launch initial process */
    Status = ExpLoadInitialProcess(&ProcessHandle,
                                   &ThreadHandle);

    /* Check for success, Bugcheck if we failed */
    if (!NT_SUCCESS(Status)) {

        KEBUGCHECKEX(SESSION4_INITIALIZATION_FAILED, Status, 0, 0, 0);
    }

    /* Wait on the Completion Event */
    if (InitDoneEventHandle != INVALID_HANDLE_VALUE) {

        HANDLE Handles[2]; /* Init event, Initial process */

        /* Setup the Handles to wait on */
        Handles[0] = InitDoneEventHandle;
        Handles[1] = ProcessHandle;

        /* Wait for the system to be initialized */
        Timeout.QuadPart = (LONGLONG)-1200000000;  /* 120 second timeout */
        Status = ZwWaitForMultipleObjects(2,
                                          Handles,
                                          WaitAny,
                                          FALSE,
                                          &Timeout);
        if (!NT_SUCCESS(Status)) {

            DPRINT1("NtWaitForMultipleObjects failed with status 0x%x!\n", Status);

        } else if (Status == STATUS_TIMEOUT) {

            DPRINT1("WARNING: System not initialized after 120 seconds.\n");

        } else if (Status == STATUS_WAIT_0 + 1) {

            /* Crash the system if the initial process was terminated. */
            KEBUGCHECKEX(SESSION5_INITIALIZATION_FAILED, Status, 0, 0, 0);
        }

        /*
         * FIXME: FILIP!
         * Disable the Boot Logo
         */
        if (!NoGuiBoot) InbvEnableBootDriver(FALSE);

        /* Signal the Event and close the handle */
        ZwSetEvent(InitDoneEventHandle, NULL);
        ZwClose(InitDoneEventHandle);

    } else {

        /* On failure to create 'ReactOSInitDone' event, go to text mode ASAP */
        if (!NoGuiBoot) InbvEnableBootDriver(FALSE);

        /* Crash the system if the initial process terminates within 5 seconds. */
        Timeout.QuadPart = (LONGLONG)-50000000;  /* 5 second timeout */
        Status = ZwWaitForSingleObject(ProcessHandle,
                                       FALSE,
                                       &Timeout);

        /* Check for timeout, crash if the initial process didn't initalize */
        if (Status != STATUS_TIMEOUT) KEBUGCHECKEX(SESSION5_INITIALIZATION_FAILED, Status, 1, 0, 0);
    }

    /* Enable the Clock, close remaining handles */
    ZwClose(ThreadHandle);
    ZwClose(ProcessHandle);

    DPRINT1("System initialization complete\n");
    {
        /* FIXME: We should instead jump to zero-page thread */
        /* Free initial kernel memory */
        MiFreeInitMemory();

        /* Set our priority to 0 */
        KeGetCurrentThread()->BasePriority = 0;
        KeSetPriorityThread(KeGetCurrentThread(), 0);

        /* Wait ad-infinitum */
        for (;;)
        {
            LARGE_INTEGER Timeout;
            Timeout.QuadPart = 0x7fffffffffffffffLL;
            KeDelayExecutionThread(KernelMode, FALSE, &Timeout);
        }
    }
}

/* EOF */
