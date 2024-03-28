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

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

CODE_SEG("INIT")
BOOLEAN
NTAPI
PoInitSystem(
    _In_ ULONG BootPhase)
{
    NTSTATUS Status;
    //PVOID NotificationEntry;
    PCHAR CommandLine;

    if (BootPhase == 1)
    {

    }

    /*
     * Prepare the centralized power capabilities variable, we will
     * determine the capabilities of this machine as soon as we discover
     * its functionalities.
     */
    RtlZeroMemory(&PopCapabilities, sizeof(SYSTEM_POWER_CAPABILITIES));

    /*
     * Check if the system supports ACPI or the user promptly forced
     * disabling of ACPI. Keep in mind that a lack of ACPI support does
     * not necessarily mean the system may support APM, a kernel mode
     * APM driver has to tell us that therefore we cannot set ApmPresent here.
     * In both of cases the system will not have SystemS5 support.
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

    /* Initialize the power IRP skeleton infrastructure */
    InitializeListHead(&PopDispatchWorkerIrpList);
    InitializeListHead(&PopQueuedIrpList);
    InitializeListHead(&PopQueuedInrushIrpList);
    InitializeListHead(&PopIrpWatchdogList);
    InitializeListHead(&PopOutstandingIrpList);
    InitializeListHead(&PopIrpThreadWorkerList);

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

    /* FIXME: Initialize shutdown lists */
    /* FIXME: Initialize notifications */
    /* FIXME: Initialize PPM */
    /* FIXME: Initialize power policies (AC/DC and CPU policies) */
    /* FIXME: Register power handlers */
    /* FIXME: Etc etc etc */

    return TRUE;
}

CODE_SEG("INIT")
VOID
NTAPI
PoInitializePrcb(
    _In_ PKPRCB Prcb)
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
