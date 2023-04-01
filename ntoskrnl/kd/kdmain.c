/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Kernel Debugger Initialization
 * COPYRIGHT:   Copyright 2005 Alex Ionescu <alex.ionescu@reactos.org>
 *              Copyright 2020 Hervé Poussineau <hpoussin@reactos.org>
 *              Copyright 2023 Hermès Bélusca-Maïto <hermes.belusca-maito@reactos.org>
 */

#include <ntoskrnl.h>
#include "kd.h"
#include "kdterminal.h"

#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS *********************************************************/

static VOID
KdpGetTerminalSettings(
    _In_ PCSTR p1)
{
#define CONST_STR_LEN(x) (sizeof(x)/sizeof(x[0]) - 1)

    while (p1 && *p1)
    {
        /* Skip leading whitespace */
        while (*p1 == ' ') ++p1;

        if (!_strnicmp(p1, "KDSERIAL", CONST_STR_LEN("KDSERIAL")))
        {
            p1 += CONST_STR_LEN("KDSERIAL");
            KdbDebugState |= KD_DEBUG_KDSERIAL;
            KdpDebugMode.Serial = TRUE;
        }
        else if (!_strnicmp(p1, "KDNOECHO", CONST_STR_LEN("KDNOECHO")))
        {
            p1 += CONST_STR_LEN("KDNOECHO");
            KdbDebugState |= KD_DEBUG_KDNOECHO;
        }

        /* Move on to the next option */
        p1 = strchr(p1, ' ');
    }
}

static PCHAR
KdpGetDebugMode(
    _In_ PCHAR Currentp2)
{
    PCHAR p1, p2 = Currentp2;
    ULONG Value;

    /* Check for Screen Debugging */
    if (!_strnicmp(p2, "SCREEN", 6))
    {
        /* Enable It */
        p2 += 6;
        KdpDebugMode.Screen = TRUE;
    }
    /* Check for Serial Debugging */
    else if (!_strnicmp(p2, "COM", 3))
    {
        /* Check for a valid Serial Port */
        p2 += 3;
        if (*p2 != ':')
        {
            Value = (ULONG)atol(p2);
            if (Value > 0 && Value < 5)
            {
                /* Valid port found, enable Serial Debugging */
                KdpDebugMode.Serial = TRUE;

                /* Set the port to use */
                SerialPortNumber = Value;
            }
        }
        else
        {
            Value = strtoul(p2 + 1, NULL, 0);
            if (Value)
            {
                KdpDebugMode.Serial = TRUE;
                SerialPortInfo.Address = UlongToPtr(Value);
                SerialPortNumber = 0;
            }
        }
    }
    /* Check for Debug Log Debugging */
    else if (!_strnicmp(p2, "FILE", 4))
    {
        /* Enable It */
        p2 += 4;
        KdpDebugMode.File = TRUE;
        if (*p2 == ':')
        {
            p2++;
            p1 = p2;
            while (*p2 != '\0' && *p2 != ' ') p2++;
            KdpLogFileName.MaximumLength = KdpLogFileName.Length = p2 - p1;
            KdpLogFileName.Buffer = p1;
        }
    }

    return p2;
}

NTSTATUS
NTAPI
KdDebuggerInitialize0(
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PCHAR CommandLine, Port = NULL;
    ULONG i;
    BOOLEAN Success = FALSE;

    if (LoaderBlock)
    {
        /* Check if we have a command line */
        CommandLine = LoaderBlock->LoadOptions;
        if (CommandLine)
        {
            /* Upcase it */
            _strupr(CommandLine);

            /* Get terminal settings */
            KdpGetTerminalSettings(CommandLine);

            /* Get the port */
            Port = strstr(CommandLine, "DEBUGPORT");
        }
    }

    /* Check if we got the /DEBUGPORT parameter(s) */
    while (Port)
    {
        /* Move past the actual string, to reach the port*/
        Port += sizeof("DEBUGPORT") - 1;

        /* Now get past any spaces and skip the equal sign */
        while (*Port == ' ') Port++;
        Port++;

        /* Get the debug mode and wrapper */
        Port = KdpGetDebugMode(Port);
        Port = strstr(Port, "DEBUGPORT");
    }

    /* Use serial port then */
    if (KdpDebugMode.Value == 0)
        KdpDebugMode.Serial = TRUE;

    /* Call the providers at Phase 0 */
    for (i = 0; i < RTL_NUMBER_OF(DispatchTable); i++)
    {
        DispatchTable[i].InitStatus = InitRoutines[i](&DispatchTable[i], 0);
        Success = (Success || NT_SUCCESS(DispatchTable[i].InitStatus));
    }

    /* Return success if at least one of the providers succeeded */
    return (Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
}


/**
 * @brief   Reinitialization routine.
 * DRIVER_REINITIALIZE
 *
 * Calls each registered provider for reinitialization at Phase >= 2.
 * I/O is now set up for disk access, at different phases.
 **/
static VOID
NTAPI
KdpDriverReinit(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_opt_ PVOID Context,
    _In_ ULONG Count)
{
    PLIST_ENTRY CurrentEntry;
    PKD_DISPATCH_TABLE CurrentTable;
    PKDP_INIT_ROUTINE KdpInitRoutine;
    ULONG BootPhase = (Count + 1); // Do BootPhase >= 2
    BOOLEAN ScheduleReinit = FALSE;

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    DPRINT("*** KD %sREINITIALIZATION - Phase %d ***\n",
           Context ? "" : "BOOT ", BootPhase);

    /* Call the registered providers */
    for (CurrentEntry = KdProviders.Flink;
         CurrentEntry != &KdProviders; NOTHING)
    {
        /* Get the provider */
        CurrentTable = CONTAINING_RECORD(CurrentEntry,
                                         KD_DISPATCH_TABLE,
                                         KdProvidersList);
        /* Go to the next entry (the Init routine may unlink us) */
        CurrentEntry = CurrentEntry->Flink;

        /* Call it if it requires a reinitialization */
        if (CurrentTable->KdpInitRoutine)
        {
            /* Get the initialization routine and reset it */
            KdpInitRoutine = CurrentTable->KdpInitRoutine;
            CurrentTable->KdpInitRoutine = NULL;
            CurrentTable->InitStatus = KdpInitRoutine(CurrentTable, BootPhase);
            DPRINT("KdpInitRoutine(%p) returned 0x%08lx\n",
                   CurrentTable, CurrentTable->InitStatus);

            /* Check whether it needs to be reinitialized again */
            ScheduleReinit = (ScheduleReinit || CurrentTable->KdpInitRoutine);
        }
    }

    DPRINT("ScheduleReinit: %s\n", ScheduleReinit ? "TRUE" : "FALSE");
    if (ScheduleReinit)
    {
        /*
         * Determine when to reinitialize.
         * If Context == NULL, we are doing a boot-driver reinitialization.
         * It is initially done once (Count == 1), and is rescheduled once
         * after all other boot drivers get loaded (Count == 2).
         * If further reinitialization is needed, switch to system-driver
         * reinitialization and do it again, not more than twice.
         */
        if (Count <= 1)
        {
            IoRegisterBootDriverReinitialization(DriverObject,
                                                 KdpDriverReinit,
                                                 (PVOID)FALSE);
        }
        else if (Count <= 3)
        {
            IoRegisterDriverReinitialization(DriverObject,
                                             KdpDriverReinit,
                                             (PVOID)TRUE);
        }
        else
        {
            /* Too late, no more reinitializations! */
            DPRINT("Cannot reinitialize anymore!\n");
            ScheduleReinit = FALSE;
        }
    }

    if (!ScheduleReinit)
    {
        /* All the necessary reinitializations are done,
         * the driver object is not needed anymore. */
        ObMakeTemporaryObject(DriverObject);
        IoDeleteDriver(DriverObject);
    }
}

/**
 * @brief   Entry point for the auxiliary driver.
 * DRIVER_INITIALIZE
 **/
static NTSTATUS
NTAPI
KdpDriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath)
{
    UNREFERENCED_PARAMETER(RegistryPath);

    /* Register for reinitialization after the other drivers are loaded */
    IoRegisterBootDriverReinitialization(DriverObject,
                                         KdpDriverReinit,
                                         (PVOID)FALSE);

    /* Set the driver as initialized */
    DriverObject->Flags |= DRVO_INITIALIZED;
    return STATUS_SUCCESS;
}

/**
 * @brief   Hooked HalInitPnpDriver() callback.
 * It is initially set by the HAL when HalInitSystem(0, ...)
 * is called earlier on.
 **/
static pHalInitPnpDriver orgHalInitPnpDriver = NULL;

/**
 * @brief
 * HalInitPnpDriver() callback hook installed by KdDebuggerInitialize1().
 *
 * It is called during initialization of the I/O manager and is where
 * the auxiliary driver is created. This driver is needed for receiving
 * reinitialization callbacks in KdpDriverReinit() later.
 * This hook must *always* call the original HalInitPnpDriver() function
 * and return its returned value, or return STATUS_SUCCESS.
 **/
static NTSTATUS
NTAPI
KdpInitDriver(VOID)
{
    static BOOLEAN InitCalled = FALSE;
    NTSTATUS Status;
    UNICODE_STRING DriverName = RTL_CONSTANT_STRING(L"\\Driver\\KdDriver");

    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    /* Ensure we are not called more than once */
    if (_InterlockedCompareExchange8((char*)&InitCalled, TRUE, FALSE) != FALSE)
        return STATUS_SUCCESS;

    /* Create the driver */
    Status = IoCreateDriver(&DriverName, KdpDriverEntry);
    if (!NT_SUCCESS(Status))
        DPRINT1("IoCreateDriver failed: 0x%08lx\n", Status);
    /* Ignore any failure from IoCreateDriver(). If it fails, no I/O-related
     * initialization will happen (no file log debugging, etc.). */

    /* Finally, restore and call the original HalInitPnpDriver() */
    InterlockedExchangePointer((PVOID*)&HalInitPnpDriver, orgHalInitPnpDriver);
    return (HalInitPnpDriver ? HalInitPnpDriver() : STATUS_SUCCESS);
}

NTSTATUS
NTAPI
KdDebuggerInitialize1(
    _In_opt_ PLOADER_PARAMETER_BLOCK LoaderBlock)
{
    PLIST_ENTRY CurrentEntry;
    PKD_DISPATCH_TABLE CurrentTable;
    PKDP_INIT_ROUTINE KdpInitRoutine;
    BOOLEAN Success = FALSE;
    BOOLEAN ReinitForPhase2 = FALSE;

    /* Make space for the displayed providers' signons */
    HalDisplayString("\r\n");

    /* Call the registered providers */
    for (CurrentEntry = KdProviders.Flink;
         CurrentEntry != &KdProviders; NOTHING)
    {
        /* Get the provider */
        CurrentTable = CONTAINING_RECORD(CurrentEntry,
                                         KD_DISPATCH_TABLE,
                                         KdProvidersList);
        /* Go to the next entry (the Init routine may unlink us) */
        CurrentEntry = CurrentEntry->Flink;

        /* Get the initialization routine and reset it */
        ASSERT(CurrentTable->KdpInitRoutine);
        KdpInitRoutine = CurrentTable->KdpInitRoutine;
        CurrentTable->KdpInitRoutine = NULL;

        /* Call it */
        CurrentTable->InitStatus = KdpInitRoutine(CurrentTable, 1);

        /* Check whether it needs to be reinitialized for Phase 2 */
        Success = (Success || NT_SUCCESS(CurrentTable->InitStatus));
        ReinitForPhase2 = (ReinitForPhase2 || CurrentTable->KdpInitRoutine);
    }

    /* Make space for the displayed providers' signons */
    HalDisplayString("\r\n");

    NtGlobalFlag |= FLG_STOP_ON_EXCEPTION;

    /* If we don't need to reinitialize providers for Phase 2, we are done */
    if (!ReinitForPhase2)
    {
        /* Return success if at least one of them succeeded */
        return (Success ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
    }

    /**
     * We want to be able to perform I/O-related initialization (starting a
     * logger thread for file log debugging, loading KDBinit file for KDBG,
     * etc.). A good place for this would be as early as possible, once the
     * I/O Manager has started the storage and the boot filesystem drivers.
     *
     * Here is an overview of the initialization steps of the NT Kernel and
     * Executive:
     * ----
     * KiSystemStartup(KeLoaderBlock)
     *     if (Cpu == 0) KdInitSystem(0, KeLoaderBlock);
     *     KiSwitchToBootStack() -> KiSystemStartupBootStack()
     *     -> KiInitializeKernel() -> ExpInitializeExecutive(Cpu, KeLoaderBlock)
     *
     * (NOTE: Any unexpected debugger break will call KdInitSystem(0, NULL); )
     * KdInitSystem(0, LoaderBlock) -> KdDebuggerInitialize0(LoaderBlock);
     *
     * ExpInitializeExecutive(Cpu == 0):    ExpInitializationPhase = 0;
     *     HalInitSystem(0, KeLoaderBlock); <-- Sets HalInitPnpDriver callback.
     *     ...
     *     PsInitSystem(LoaderBlock)
     *         PsCreateSystemThread(Phase1Initialization)
     *
     * Phase1Initialization(Discard):       ExpInitializationPhase = 1;
     *     HalInitSystem(1, KeLoaderBlock);
     *     ...
     *     Early initialization of Ob, Ex, Ke.
     *     KdInitSystem(1, KeLoaderBlock);
     *     ...
     *     KdDebuggerInitialize1(LoaderBlock);
     *     ...
     *     IoInitSystem(LoaderBlock);
     *     ...
     * ----
     * As we can see, KdDebuggerInitialize1() is the last KD initialization
     * routine the kernel calls, and is called *before* the I/O Manager starts.
     * Thus, direct Nt/ZwCreateFile ... calls done there would fail. Also,
     * we want to do the I/O initialization as soon as possible. There does
     * not seem to be any exported way to be notified about the I/O manager
     * initialization steps... that is, unless we somehow become a driver and
     * insert ourselves in the flow!
     *
     * Since we are not a regular driver, we need to invoke IoCreateDriver()
     * to create one. However, remember that we are currently running *before*
     * IoInitSystem(), the I/O subsystem is not initialized yet. Due to this,
     * calling IoCreateDriver(), much like any other IO functions, would lead
     * to a crash, because it calls
     * ObCreateObject(..., IoDriverObjectType, ...), and IoDriverObjectType
     * is non-initialized yet (it's NULL).
     *
     * The chosen solution is to hook a "known" exported callback: namely, the
     * HalInitPnpDriver() callback (it initializes the "HAL Root Bus Driver").
     * It is set very early on by the HAL via the HalInitSystem(0, ...) call,
     * and is called early on by IoInitSystem() before any driver is loaded,
     * but after the I/O Manager has been minimally set up so that new drivers
     * can be created.
     * When the hook: KdpInitDriver() is called, we create our driver with
     * IoCreateDriver(), specifying its entrypoint KdpDriverEntry(), then
     * restore and call the original HalInitPnpDriver() callback.
     *
     * Another possible unexplored alternative, could be to insert ourselves
     * in the KeLoaderBlock->LoadOrderListHead boot modules list, or in the
     * KeLoaderBlock->BootDriverListHead boot-driver list. (Note that while
     * we may be able to do this, because boot-drivers are resident in memory,
     * much like we are, we cannot insert ourselves in the system-driver list
     * however, since those drivers are expected to come from PE image files.)
     *
     * Once the KdpDriverEntry() driver entrypoint is called, we register
     * KdpDriverReinit() for re-initialization with the I/O Manager, in order
     * to provide more initialization points. KdpDriverReinit() calls the KD
     * providers at BootPhase >= 2, and schedules further reinitializations
     * (at most 3 more) if any of the providers request so.
     **/
    orgHalInitPnpDriver =
        InterlockedExchangePointer((PVOID*)&HalInitPnpDriver, KdpInitDriver);
    return STATUS_SUCCESS;
}

/* EOF */
