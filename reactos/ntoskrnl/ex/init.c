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
#include <ntos/bootvid.h>
#define NDEBUG
#include <internal/debug.h>

/* DATA **********************************************************************/

extern ULONG MmCoreDumpType;
extern CHAR KiTimerSystemAuditing;
extern PVOID Ki386InitialStackArray[MAXIMUM_PROCESSORS];
extern ADDRESS_RANGE KeMemoryMap[64];
extern ULONG KeMemoryMapRangeCount;
extern ULONG_PTR FirstKrnlPhysAddr;
extern ULONG_PTR LastKrnlPhysAddr;
extern ULONG_PTR LastKernelAddress;
extern LOADER_MODULE KeLoaderModules[64];
extern PRTL_MESSAGE_RESOURCE_DATA KiBugCodeMessages;
extern LIST_ENTRY KiProfileListHead;
extern LIST_ENTRY KiProfileSourceListHead;
extern KSPIN_LOCK KiProfileLock;
BOOLEAN SetupMode = TRUE;

VOID PspPostInitSystemProcess(VOID);

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
    strcpy (ParamBuffer, (char *)ParameterLine);
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

inline
VOID
STDCALL
ExecuteRuntimeAsserts(VOID)
{
    /*
     * Fail at runtime if someone has changed various structures without
     * updating the offsets used for the assembler code.
     */
    ASSERT(FIELD_OFFSET(KTHREAD, InitialStack) == KTHREAD_INITIAL_STACK);
    ASSERT(FIELD_OFFSET(KTHREAD, Teb) == KTHREAD_TEB);
    ASSERT(FIELD_OFFSET(KTHREAD, KernelStack) == KTHREAD_KERNEL_STACK);
    ASSERT(FIELD_OFFSET(KTHREAD, NpxState) == KTHREAD_NPX_STATE);
    ASSERT(FIELD_OFFSET(KTHREAD, ServiceTable) == KTHREAD_SERVICE_TABLE);
    ASSERT(FIELD_OFFSET(KTHREAD, PreviousMode) == KTHREAD_PREVIOUS_MODE);
    ASSERT(FIELD_OFFSET(KTHREAD, TrapFrame) == KTHREAD_TRAP_FRAME);
    ASSERT(FIELD_OFFSET(KTHREAD, CallbackStack) == KTHREAD_CALLBACK_STACK);
    ASSERT(FIELD_OFFSET(KTHREAD, ApcState.Process) == KTHREAD_APCSTATE_PROCESS);
    ASSERT(FIELD_OFFSET(KPROCESS, DirectoryTableBase) == KPROCESS_DIRECTORY_TABLE_BASE);
    ASSERT(FIELD_OFFSET(KPROCESS, IopmOffset) == KPROCESS_IOPM_OFFSET);
    ASSERT(FIELD_OFFSET(KPROCESS, LdtDescriptor) == KPROCESS_LDT_DESCRIPTOR0);
    ASSERT(FIELD_OFFSET(KTRAP_FRAME, Reserved9) == KTRAP_FRAME_RESERVED9);
    ASSERT(FIELD_OFFSET(KV86M_TRAP_FRAME, SavedExceptionStack) == TF_SAVED_EXCEPTION_STACK);
    ASSERT(FIELD_OFFSET(KV86M_TRAP_FRAME, regs) == TF_REGS);
    ASSERT(FIELD_OFFSET(KV86M_TRAP_FRAME, orig_ebp) == TF_ORIG_EBP);
    ASSERT(FIELD_OFFSET(KPCR, Tib.ExceptionList) == KPCR_EXCEPTION_LIST);
    ASSERT(FIELD_OFFSET(KPCR, Self) == KPCR_SELF);
    ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, CurrentThread) == KPCR_CURRENT_THREAD);
    ASSERT(FIELD_OFFSET(KIPCR, PrcbData) + FIELD_OFFSET(KPRCB, NpxThread) == KPCR_NPX_THREAD);
    ASSERT(FIELD_OFFSET(KTSS, Esp0) == KTSS_ESP0);
    ASSERT(FIELD_OFFSET(KTSS, Eflags) == KTSS_EFLAGS);
    ASSERT(FIELD_OFFSET(KTSS, IoMapBase) == KTSS_IOMAPBASE);
    ASSERT(sizeof(FX_SAVE_AREA) == SIZEOF_FX_SAVE_AREA);
}

inline
VOID
STDCALL
ParseAndCacheLoadedModules(VOID)
{
    ULONG i;
    PCHAR Name;

    /* Loop the Module List and get the modules we want */
    for (i = 1; i < KeLoaderBlock.ModsCount; i++) {

        /* Get the Name of this Module */
        if (!(Name = strrchr((PCHAR)KeLoaderModules[i].String, '\\'))) {

            /* Save the name */
            Name = (PCHAR)KeLoaderModules[i].String;

        } else {

            /* No name, skip */
            Name++;
        }

        /* Now check for any of the modules we will need later */
        if (!_stricmp(Name, "ansi.nls")) {

            CachedModules[AnsiCodepage] = &KeLoaderModules[i];

        } else if (!_stricmp(Name, "oem.nls")) {

            CachedModules[OemCodepage] = &KeLoaderModules[i];

        } else if (!_stricmp(Name, "casemap.nls")) {

            CachedModules[UnicodeCasemap] = &KeLoaderModules[i];

        } else if (!_stricmp(Name, "system") || !_stricmp(Name, "system.hiv")) {

            CachedModules[SystemRegistry] = &KeLoaderModules[i];
            SetupMode = FALSE;

        } else if (!_stricmp(Name, "hardware") || !_stricmp(Name, "hardware.hiv")) {

            CachedModules[HardwareRegistry] = &KeLoaderModules[i];
        }
    }
}

inline
VOID
STDCALL
ParseCommandLine(PULONG MaxMem,
                 PBOOLEAN NoGuiBoot,
                 PBOOLEAN BootLog,
                 PBOOLEAN ForceAcpiDisable)
{
    PCHAR p1, p2;

    p1 = (PCHAR)KeLoaderBlock.CommandLine;
    while(*p1 && (p2 = strchr(p1, '/'))) {

        p2++;
        if (!_strnicmp(p2, "MAXMEM", 6)) {

            p2 += 6;
            while (isspace(*p2)) p2++;

            if (*p2 == '=') {

                p2++;

                while(isspace(*p2)) p2++;

                if (isdigit(*p2)) {
                    while (isdigit(*p2)) {
                        *MaxMem = *MaxMem * 10 + *p2 - '0';
                        p2++;
                    }
                    break;
                }
            }
        } else if (!_strnicmp(p2, "NOGUIBOOT", 9)) {

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
        } else if (!_strnicmp(p2, "BOOTLOG", 7)) {

            p2 += 7;
            *BootLog = TRUE;
        } else if (!_strnicmp(p2, "NOACPI", 6)) {

            p2 += 6;
            *ForceAcpiDisable = TRUE;
        }

        p1 = p2;
    }
}
   
VOID
INIT_FUNCTION
ExpDisplayNotice(VOID)
{
    CHAR str[50];
   
    if (SetupMode)
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
            (KeLoaderBlock.MemHigher + 1088)/ 1024);
    HalDisplayString(str);
    
}
   
VOID
INIT_FUNCTION
STDCALL
ExpInitializeExecutive(VOID)
{
    UNICODE_STRING EventName;
    HANDLE InitDoneEventHandle;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN NoGuiBoot = FALSE;
    BOOLEAN BootLog = FALSE;
    ULONG MaxMem = 0;
    BOOLEAN ForceAcpiDisable = FALSE;
    LARGE_INTEGER Timeout;
    HANDLE ProcessHandle;
    HANDLE ThreadHandle;
    NTSTATUS Status;

    /* Check if the structures match the ASM offset constants */
    ExecuteRuntimeAsserts();

    /* Sets up the Text Sections of the Kernel and HAL for debugging */
    LdrInit1();

    /* Lower the IRQL to Dispatch Level */
    KeLowerIrql(DISPATCH_LEVEL);

    /* Sets up the VDM Data */
    NtEarlyInitVdm();

    /* Parse Command Line Settings */
    ParseCommandLine(&MaxMem, &NoGuiBoot, &BootLog, &ForceAcpiDisable);

    /* Initialize Kernel Memory Address Space */
    MmInit1(FirstKrnlPhysAddr,
            LastKrnlPhysAddr,
            LastKernelAddress,
            (PADDRESS_RANGE)&KeMemoryMap,
            KeMemoryMapRangeCount,
            MaxMem > 8 ? MaxMem : 4096);

    /* Parse the Loaded Modules (by FreeLoader) and cache the ones we'll need */
    ParseAndCacheLoadedModules();


    /* Initialize the Dispatcher, Clock and Bug Check Mechanisms. */
    KeInit2();

    /* Bring back the IRQL to Passive */
    KeLowerIrql(PASSIVE_LEVEL);

    /* Initialize Profiling */
    InitializeListHead(&KiProfileListHead);
    InitializeListHead(&KiProfileSourceListHead);
    KeInitializeSpinLock(&KiProfileLock);

    /* Load basic Security for other Managers */
    if (!SeInit1()) KEBUGCHECK(SECURITY_INITIALIZATION_FAILED);

    /* Create the Basic Object Manager Types to allow new Object Types */
    ObInit();

    /* Initialize Lookaside Lists */
    ExInit2();

    /* Set up Region Maps, Sections and the Paging File */
    MmInit2();

    /* Initialize Tokens now that the Object Manager is ready */
    if (!SeInit2()) KEBUGCHECK(SECURITY1_INITIALIZATION_FAILED);

    /* Set 1 CPU for now, we'll increment this later */
    KeNumberProcessors = 1;
    
    /* Initalize the Process Manager */
    PiInitProcessManager();
    
    /* Break into the Debugger if requested */
    if (KdPollBreakIn()) DbgBreakPointWithStatus (DBG_STATUS_CONTROL_C);

    /* Initialize all processors */
    while (!HalAllProcessorsStarted()) {

        PVOID ProcessorStack;

        /* Set up the Kernel and Process Manager for this CPU */
        KePrepareForApplicationProcessorInit(KeNumberProcessors);
        KeCreateApplicationProcessorIdleThread(KeNumberProcessors);

        /* Allocate a stack for use when booting the processor */
        ProcessorStack = Ki386InitialStackArray[((int)KeNumberProcessors)] + MM_STACK_SIZE;

        /* Tell HAL a new CPU is being started */
        HalStartNextProcessor(0, (ULONG)ProcessorStack - 2*sizeof(FX_SAVE_AREA));
        KeNumberProcessors++;
    }

    /* Do Phase 1 HAL Initalization */
    HalInitSystem(1, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

    /* Initialize Basic System Objects and Worker Threads */
    ExInit3();

    /* Create the system handle table, assign it to the system process, create
       the client id table and assign a PID for the system process. This needs
       to be done before the worker threads are initialized so the system
       process gets the first PID (4) */
    PspPostInitSystemProcess();

    /* initialize the worker threads */
    ExpInitializeWorkerThreads();

    /* initialize callbacks */
    ExpInitializeCallbacks();

    /* Call KD Providers at Phase 1 */
    KdInitSystem(1, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

    /* Initialize I/O Objects, Filesystems, Error Logging and Shutdown */
    IoInit();

    /* TBD */
    PoInit((PLOADER_PARAMETER_BLOCK)&KeLoaderBlock, ForceAcpiDisable);

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
    HalInitSystem(2, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

    /* Display version number and copyright/warranty message */
    ExpDisplayNotice();

    /* Call KD Providers at Phase 2 */
    KdInitSystem(2, (PLOADER_PARAMETER_BLOCK)&KeLoaderBlock);

    /* Import and create NLS Data and Sections */
    RtlpInitNls();

    /* Import and Load Registry Hives */
    CmInitHives(SetupMode);

    /* Initialize the time zone information from the registry */
    ExpInitTimeZoneInfo();

    /* Enter the kernel debugger before starting up the boot drivers */
    if (KdDebuggerEnabled) KdbEnter();

    /* Setup Drivers and Root Device Node */
    IoInit2(BootLog);

    /* Display the boot screen image if not disabled */
    if (!NoGuiBoot) InbvEnableBootDriver(TRUE);

    /* Create ARC Names, SystemRoot SymLink, Load Drivers and Assign Letters */
    IoInit3();

    /* Load the System DLL and its Entrypoints */
    LdrpInitializeSystemDll();

    /* Initialize the Default Locale */
    PiInitDefaultLocale();

    /* Initialize shared user page. Set dos system path, dos device map, etc. */
    InitSystemSharedUserPage ((PCHAR)KeLoaderBlock.CommandLine);

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
    Status = LdrLoadInitialProcess(&ProcessHandle,
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

        /* Disable the Boot Logo */
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
    KiTimerSystemAuditing = 1;
    ZwClose(ThreadHandle);
    ZwClose(ProcessHandle);
}

VOID INIT_FUNCTION
ExInit2(VOID)
{
  ExpInitLookasideLists();
}

VOID INIT_FUNCTION
ExInit3 (VOID)
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
  ExpInitializeHandleTables();
}

/* EOF */
