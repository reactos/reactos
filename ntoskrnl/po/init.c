/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager Initialization infrastructure
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

BOOLEAN PopAcpiPresent;
POP_POWER_ACTION PopAction;
SYSTEM_POWER_CAPABILITIES PopCapabilities;
ADMINISTRATOR_POWER_POLICY PopAdminPowerPolicy;
BOOLEAN PopSimulate = FALSE;

/* PRIVATE FUNCTIONS **********************************************************/

static
VOID
PopInitShutdownList(VOID)
{
    PAGED_CODE();

    /* Initialize the shutdown trigger event */
    KeInitializeEvent(&PopShutdownEvent, NotificationEvent, FALSE);

    /* Setup the centralized shutdown queue entries list */
    PopShutdownThreadList = NULL;
    InitializeListHead(&PopShutdownQueue);

    /*
     * Setup the shutdown list protect lock and acknowledge the Power
     * Manager the list is ready to be used.
     */
    KeInitializeGuardedMutex(&PopShutdownListMutex);
    PopShutdownListAvailable = TRUE;
}

static
VOID
PopInitPolicyManager(VOID)
{
    PAGED_CODE();

    /* Initialize the power policy lock */
    ExInitializeResourceLite(&PopPowerPolicyLock);
    PopPowerPolicyOwnerLockThread = NULL;

    /* Initialize AC/DC power policies */
    PopInitializePowerPolicy(&PopAcPowerPolicy);
    PopInitializePowerPolicy(&PopDcPowerPolicy);

    /*
     * Always assume that we are being powered up by the PSU than
     * a battery when we are firing up the system. A system battery
     * presence is determined when we talk to the battery composite driver.
     * It is the said driver that has to notify us of batteries.
     */
    PopSetDefaultPowerPolicy(PolicyAc);

    /* Initialize the power policy worker protect lock and policy IRPs list */
    KeInitializeSpinLock(&PopPowerPolicyWorkerLock);
    InitializeListHead(&PopPowerPolicyIrpQueueList);

    /* Setup the policy worker item thread */
    ExInitializeWorkItem(&PopPowerPolicyWorkItem,
                         PopPolicyManagerWorker,
                         NULL);

    /* No policy workers are pending when the Power Manager initializes */
    PopPendingPolicyWorker = FALSE;

    /*
     * Register default power policy workers with the Power Manager.
     * The Power Manager will invoke the corresponding worker routines
     * each time it checks for outstanding policy workers to get deployed.
     */
    PopRegisterPowerPolicyWorker(PolicyWorkerNotification,
                                 PopPowerPolicyNotification);
    PopRegisterPowerPolicyWorker(PolicyWorkerSystemIdle,
                                 PopPowerPolicySystemIdle);
    PopRegisterPowerPolicyWorker(PolicyWorkerTimeChange,
                                 PopPowerPolicyTimeChange);
}

static
VOID
PopInitAdminPowerPolicyOverrides(
    _Out_ PADMINISTRATOR_POWER_POLICY AdminPolicy)
{
    PAGED_CODE();

    /* Zero out the admin policy parameter to caller */
    RtlZeroMemory(AdminPolicy, sizeof(ADMINISTRATOR_POWER_POLICY));

    /* Setup the admin power policy overrides */
    AdminPolicy->MinSleep = PowerSystemSleeping1;
    AdminPolicy->MaxSleep = PowerSystemHibernate;
    AdminPolicy->MinVideoTimeout = 0;
    AdminPolicy->MaxVideoTimeout = 0xFFFFFFFF;
    AdminPolicy->MinSpindownTimeout = 0;
    AdminPolicy->MaxSpindownTimeout = 0xFFFFFFFF;
}

static
VOID
PopInitPowerSettingCallbacks(VOID)
{
    PAGED_CODE();

    /* Initialize the power setting callback list */
    InitializeListHead(&PopPowerSettingCallbacksList);

    /* Initialize the power setting callback lock */
    ExInitializeFastMutex(&PopPowerSettingLock);
    PopPowerSettingOwnerLockThread = NULL;

    /*
     * Setup the power setting callbacks counter. The Power Manager will
     * increment this counter as device drivers register new callbacks.
     * This is used for debugging purposes.
     */
    PopPowerSettingCallbacksCount = 0;
}

static
BOOLEAN
PopInitBattery(VOID)
{
    PAGED_CODE();

    /* Allocate memory space for the composite battery structure */
    PopBattery = PopAllocatePool(sizeof(*PopBattery),
                                 FALSE,
                                 TAG_PO_COMPOSITE_BATTERY);
    if (!PopBattery)
    {
        return FALSE;
    }

    /*
     * For now we are not sure if this device is powered up by a battery
     * or by a PSU until a power policy device driver informs us of an
     * upcoming composite battery of which we must connect to.
     */
    PopBattery->Flags |= POP_CB_NO_BATTERY;
    PopBattery->Mode = POP_CB_NO_MODE;
    PopBattery->DeviceObject = NULL;
    PopBattery->Irp = NULL;
    return TRUE;
}

static
VOID
PopInitPoFxManager(VOID)
{
    /* FIXME */
    PAGED_CODE();
    UNIMPLEMENTED;
    return;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
PopCreatePowerRequestObjectType(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING Name;
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    LARGE_INTEGER PowerRequestScanDueTime;

    PAGED_CODE();

    /* Initialize the Power Request object type data */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"PowerRequest");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.SecurityRequired = TRUE;
    ObjectTypeInitializer.DefaultPagedPoolCharge = sizeof(POP_POWER_REQUEST);
    ObjectTypeInitializer.GenericMapping = PopPowerRequestGenericMapping;
    ObjectTypeInitializer.PoolType = PagedPool;
    ObjectTypeInitializer.ValidAccessMask = STANDARD_RIGHTS_ALL;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.CloseProcedure = PopClosePowerRequestObject;
    ObjectTypeInitializer.DeleteProcedure = NULL;

    /* Register it with the Object Manager */
    Status = ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &PoPowerRequestObjectType);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to create the power request object type (Status 0x%lx)\n", Status);
        return Status;
    }

#if DBG
    /* Initialize the debug power request monitoring mechanism */
    KeInitializeTimerEx(&PopScanActivePowerRequestsTiimer, NotificationTimer);
    KeInitializeDpc(&PopScanActivePowerRequestsDpc, PopScanForActivePowerRequestsDpcRoutine, NULL);
#endif

    /* Initialize the power request terminate reaper mechanism */
    KeInitializeTimerEx(&PopReapTerminatePowerRequestsTimer, NotificationTimer);
    KeInitializeDpc(&PopReapTerminatePowerRequestsDpc, PopReapTerminatePowerRequestsDpcRoutine, NULL);

    /* Initialize the miscellaneous power request constructs */
    PopTotalKernelPowerRequestsCount = 0;
    PopTotalUserPowerRequestsCount = 0;
    PopPowerRequestOwnerLockThread = NULL;
    InitializeListHead(&PopPowerRequestsList);
    KeInitializeSpinLock(&PopPowerRequestLock);

#if DBG
    /* Fire up the debug power request monitoring */
    PowerRequestScanDueTime.QuadPart = Int32x32To64(PopScanActivePowerRequestsIntervalInSeconds,
                                                    -10 * 1000 * 1000);
    KeSetTimerEx(&PopScanActivePowerRequestsTiimer,
                 PowerRequestScanDueTime,
                 60000, // Recurring interval in 60 seconds (60000 ms = 60 s)
                 &PopScanActivePowerRequestsDpc);
#endif

    return STATUS_SUCCESS;
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
PopCreateThermalRequestObjectType(VOID)
{
    /* FIXME */
    PAGED_CODE();
    UNIMPLEMENTED;
    return STATUS_SUCCESS;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
PopInitSystemPhase0(VOID)
{
    NTSTATUS Status;
    PCHAR CommandLine;

    PAGED_CODE();

    /*
     * Prepare the centralized power capabilities and power actions.
     * We will determine the capabilities of this machine as soon as
     * we discover its functionalities.
     */
    RtlZeroMemory(&PopCapabilities, sizeof(SYSTEM_POWER_CAPABILITIES));
    RtlZeroMemory(&PopAction, sizeof(POP_POWER_ACTION));

    /*
     * Check if the system supports ACPI or the user promptly forced
     * disabling of ACPI. Keep in mind that a lack of ACPI support does
     * not necessarily mean the system may support APM, a kernel mode
     * APM driver has to tell us that therefore we cannot set ApmPresent here.
     */
    CommandLine = KeLoaderBlock->LoadOptions;
    _strupr(CommandLine);
    if (strstr(CommandLine, "NOACPI"))
    {
        PopAcpiPresent = FALSE;
    }
    else
    {
        PopAcpiPresent = KeLoaderBlock->Extension->AcpiTable != NULL ? TRUE : FALSE;
    }

    /* Only enable soft off shutdown in presence of ACPI as stated above */
    if (PopAcpiPresent)
    {
        PopCapabilities.SystemS5 = TRUE;
    }

    /* Do not apply any power action as we are initializing the power manager */
    PopApplyPowerAction(PowerActionNone);

    /* Initialize the power IRP skeleton infrastructure */
    InitializeListHead(&PopDispatchWorkerIrpList);
    InitializeListHead(&PopQueuedIrpList);
    InitializeListHead(&PopQueuedInrushIrpList);
    InitializeListHead(&PopIrpThreadList);
    InitializeListHead(&PopIrpDataList);

    PopInrushIrp = NULL;
    PopPendingIrpDispatcWorkerCount = 0;
    PopIrpDispatchWorkerCount = 0;
    PopIrpOwnerLockThread = NULL;
    PopIrpDispatchWorkerPending = FALSE;

    KeInitializeEvent(&PopIrpDispatchPendingEvent,
                      NotificationEvent,
                      FALSE);

    KeInitializeSemaphore(&PopIrpDispatchMasterSemaphore,
                          0,
                          MAXLONG);

    KeInitializeSpinLock(&PopIrpLock);

    Status = PopCreateIrpWorkerThread(&PopMasterDispatchIrp);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Creation of power IRP master dispatcher has FAILED (Status 0x%lx)\n", Status);
        return FALSE;
    }

    /* Initialize volumes and DOPE support */
    InitializeListHead(&PopVolumeDevices);
    KeInitializeGuardedMutex(&PopVolumeLock);
    KeInitializeSpinLock(&PopDopeGlobalLock);

    /* Initialize the device notification lock */
    ExInitializeResourceLite(&PopNotifyDeviceLock);

    /*
     * Initialize the thermal protect lock and list of thermal zones. Do not
     * set the thermal zone state as active until we figure out if the system
     * has at least one thermal zone device supported.
     */
    KeInitializeSpinLock(&PopThermalZoneLock);
    InitializeListHead(&PopThermalZones);
    PopApplyThermalZoneState(POP_THERMAL_ZONE_NONE);

    /* Initialize the idle detection skeleton infrastructure */
    InitializeListHead(&PopIdleDetectList);
    KeInitializeTimerEx(&PopIdleScanDevicesTimer, NotificationTimer);
    KeInitializeDpc(&PopIdleScanDevicesDpc, PopScanForIdleStateDevicesDpcRoutine, NULL);

    /* Initialize the list of power switches and action waiters */
    InitializeListHead(&PopControlSwitches);
    InitializeListHead(&PopActionWaiters);

    /*
     * Initialize the memory unlock worker thread and corresponding
     * completion event. These constructs are specifically used in
     * NtSetSystemPowerState in case memory pages/sections cannot be
     * grabbed into the hibernation file.
     */
    ExInitializeWorkItem(&PopUnlockMemoryWorkItem,
                         PopUnlockMemoryWorker,
                         NULL);

    KeInitializeEvent(&PopUnlockMemoryCompleteEvent,
                      NotificationEvent,
                      FALSE);

    /* Initialize the shutdown core mechanism */
    PopInitShutdownList();

    /* Initialize the list of devices that woke the system */
    InitializeListHead(&PopWakeSourceDevicesList);
    PopSystemFullWake = 0; // FIXME: Set a proper wake mode for the system

    /*
     * Assign the default shutdown power states upon the initialization of the
     * power manager. This is to ensure proper behavior when we are shutting
     * down the system, in case none of the HALs has registered their power states.
     */
    Status = PopRegisterSystemStateHandler(PowerStateShutdownOff,
                                           FALSE,
                                           PopShutdownHandler,
                                           NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("System state handler registration failed (Status 0x%lx)\n", Status);
        return FALSE;
    }

    /* Initialize the composite battery */
    if (!PopInitBattery())
    {
        DPRINT1("Composite battery initialization has failed\n");
        return FALSE;
    }

    /* Initialize the power setting callbacks */
    PopInitPowerSettingCallbacks();

    /* Initialize the power policy and worker manager */
    PopInitPolicyManager();

    /* Initialize the global power admin overrides */
    PopInitAdminPowerPolicyOverrides(&PopAdminPowerPolicy);

    /*
     * Initialize core PPM management at early phase. We do not check
     * for function status as the initial core initialization always suceeds.
     */
    PpmInitialize(TRUE);

    /* Initialize the power framework (PoFx) */
    PopInitPoFxManager();

    DPRINT1("Hold on there buster\n");
    //__debugbreak();
    return TRUE;
}

CODE_SEG("INIT")
BOOLEAN
NTAPI
PopInitSystemPhase1(VOID)
{
    NTSTATUS Status;
    PVOID NotificationEntry;
    LARGE_INTEGER IdleScanDueTime;

    PAGED_CODE();

    /*
     * Register the policy device drivers of which notifications may arrive.
     * Drivers that we care about are the class input, battery, lid, thermal
     * zones, system buttons, fans and ACPI memory devices.
     */
    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&GUID_DEVICE_SYS_BUTTON,
                                            IopRootDeviceNode->PhysicalDeviceObject->DriverObject,
                                            PopDevicePolicyCallback,
                                            (PVOID)(ULONG_PTR)PolicyDeviceSystemButton,
                                            &NotificationEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register PnP notifications for the POWER BUTTON (Status 0x%lx)\n", Status);
        return FALSE;
    }

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&GUID_CLASS_INPUT,
                                            IopRootDeviceNode->PhysicalDeviceObject->DriverObject,
                                            PopDevicePolicyCallback,
                                            (PVOID)(ULONG_PTR)PolicyDeviceSystemButton,
                                            &NotificationEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register PnP notifications for CLASS INPUT (Status 0x%lx)\n", Status);
        return FALSE;
    }

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&GUID_DEVICE_LID,
                                            IopRootDeviceNode->PhysicalDeviceObject->DriverObject,
                                            PopDevicePolicyCallback,
                                            (PVOID)(ULONG_PTR)PolicyDeviceSystemButton,
                                            &NotificationEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register PnP notifications for DEVICE LID (Status 0x%lx)\n", Status);
        return FALSE;
    }

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&GUID_DEVICE_MEMORY,
                                            IopRootDeviceNode->PhysicalDeviceObject->DriverObject,
                                            PopDevicePolicyCallback,
                                            (PVOID)(ULONG_PTR)PolicyDeviceMemory,
                                            &NotificationEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register PnP notifications for DEVICE MEMORY (Status 0x%lx)\n", Status);
        return FALSE;
    }

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&GUID_DEVICE_BATTERY,
                                            IopRootDeviceNode->PhysicalDeviceObject->DriverObject,
                                            PopDevicePolicyCallback,
                                            (PVOID)(ULONG_PTR)PolicyDeviceBattery,
                                            &NotificationEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register PnP notifications for BATTERY (Status 0x%lx)\n", Status);
        return FALSE;
    }

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&GUID_DEVICE_THERMAL_ZONE,
                                            IopRootDeviceNode->PhysicalDeviceObject->DriverObject,
                                            PopDevicePolicyCallback,
                                            (PVOID)(ULONG_PTR)PolicyDeviceThermalZone,
                                            &NotificationEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register PnP notifications for THERMAL ZONES (Status 0x%lx)\n", Status);
        return FALSE;
    }

    Status = IoRegisterPlugPlayNotification(EventCategoryDeviceInterfaceChange,
                                            PNPNOTIFY_DEVICE_INTERFACE_INCLUDE_EXISTING_INTERFACES,
                                            (PVOID)&GUID_DEVICE_FAN,
                                            IopRootDeviceNode->PhysicalDeviceObject->DriverObject,
                                            PopDevicePolicyCallback,
                                            (PVOID)(ULONG_PTR)PolicyDeviceFan,
                                            &NotificationEntry);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to register PnP notifications for FANS (Status 0x%lx)\n", Status);
        return FALSE;
    }

    /* Initialize the Power registry database if it was not created before */
    PopCreatePowerPolicyDatabase();

    /*
     * Check if power simulation is triggered for this system.
     * Apply some capabilities in this case.
     */
    if (PopSimulate)
    {
        /* Simulate the presence of a battery */
        PopCapabilities.SystemBatteriesPresent = TRUE;
        PopCapabilities.BatteryScale[0].Granularity = 100;
        PopCapabilities.BatteryScale[0].Capacity = 400;
        PopCapabilities.BatteryScale[1].Granularity = 10;
        PopCapabilities.BatteryScale[1].Capacity = 0xFFFF;

        /* Set the RTC and latency to some default states for simulation */
        PopCapabilities.RtcWake = PowerSystemSleeping3;
        PopCapabilities.DefaultLowLatencyWake = PowerSystemSleeping1;

        /* And simulate the presence of ACPI devices and sleep states */
        PopCapabilities.PowerButtonPresent = TRUE;
        PopCapabilities.SleepButtonPresent = TRUE;
        PopCapabilities.LidPresent = TRUE;
        PopCapabilities.SystemS1 = TRUE;
        PopCapabilities.SystemS2 = TRUE;
        PopCapabilities.SystemS3 = TRUE;
        PopCapabilities.SystemS4 = TRUE;
    }

    /* Read the AC/DC, administrator and miscellanea policies and apply them where necessary */
    PopAcquirePowerPolicyLock();
    PopDefaultPolicies();
    PopReleasePowerPolicyLock();

    /* Initialize the power request object implementation */
    Status = PopCreatePowerRequestObjectType();
    if (!NT_SUCCESS(Status))
    {
        /* If this fails, then sacrifice the system later on... */
        return FALSE;
    }

    /* Initialize the thermal request implementation as well */
    Status = PopCreateThermalRequestObjectType();
    if (!NT_SUCCESS(Status))
    {
        /* If this fails, then sacrifice the system later on... */
        return FALSE;
    }

    /* Fire up the idle scan timer for idle detection */
    IdleScanDueTime.QuadPart = Int32x32To64(PopIdleScanIntervalInSeconds,
                                            -10 * 1000 * 1000);
    KeSetTimerEx(&PopIdleScanDevicesTimer,
                 IdleScanDueTime,
                 1000, // Recurring interval in 1 second (1000 ms = 1 s)
                 &PopIdleScanDevicesDpc);

    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

CODE_SEG("INIT")
BOOLEAN
NTAPI
PoInitSystem(
    _In_ ULONG BootPhase)
{
    BOOLEAN Success;

    switch (BootPhase)
    {
        /* Early phase initialization */
        case 0:
        {
            Success = PopInitSystemPhase0();
            break;
        }

        /* Last phase initialization */
        case 1:
        {
            Success = PopInitSystemPhase1();
            break;
        }

        /* Bail out on any other unknown phases */
        default:
        {
            KeBugCheckEx(INTERNAL_POWER_ERROR,
                         0,
                         POP_PO_INIT_FAILURE,
                         BootPhase,
                         0);
        }
    }

    return Success;
}

CODE_SEG("INIT")
VOID
NTAPI
PoInitializePrcb(
    _Inout_ PKPRCB Prcb)
{
    /* Initialize the power state for this processor */
    RtlZeroMemory(&Prcb->PowerState, sizeof(Prcb->PowerState));
    Prcb->PowerState.Idle0KernelTimeLimit = 0xFFFFFFFF;
    Prcb->PowerState.CurrentThrottle = POP_CURRENT_THROTTLE_MAX;
    Prcb->PowerState.CurrentThrottleIndex = 0;
    Prcb->PowerState.IdleFunction = PpmIdle;

    /* Register the performance routine for this processor and the timer */
    KeInitializeDpc(&Prcb->PowerState.PerfDpc, PpmPerfIdleDpcRoutine, Prcb);
    KeSetTargetProcessorDpc(&Prcb->PowerState.PerfDpc, Prcb->Number);
    KeInitializeTimerEx(&Prcb->PowerState.PerfTimer, SynchronizationTimer);
}

/* EOF */
