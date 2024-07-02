/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Policy Manager & Policy Worker Manager
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

ERESOURCE PopPowerPolicyLock;
LIST_ENTRY PopPowerPolicyIrpQueueList;
WORK_QUEUE_ITEM PopPowerPolicyWorkItem;
PKTHREAD PopPowerPolicyOwnerLockThread;
KSPIN_LOCK PopPowerPolicyWorkerLock;
BOOLEAN PopPendingPolicyWorker;
SYSTEM_POWER_POLICY PopAcPowerPolicy;
SYSTEM_POWER_POLICY PopDcPowerPolicy;
PSYSTEM_POWER_POLICY PopDefaultPowerPolicy;
PKWIN32_POWEREVENT_CALLOUT PopEventCallout = NULL;
POP_POLICY_WORKER PopPolicyWorker[PolicyWorkerMax] = {0};

/* PRIVATE FUNCTIONS **********************************************************/

static
VOID
PopDispatchPowerEvent(
    _In_ PWIN32_POWEREVENT_PARAMETERS Win32PwrEventParams)
{
    PVOID Session;
    ULONG SessionId;
    ULONG ProcessFlags;
    KAPC_STATE ApcState;
    NTSTATUS Status;

    /* If we get here and the power callout was not registered yet, just quit */
    if (!PopEventCallout)
    {
        return;
    }

    /* Ensure the power callout is within the session space in order to be usable */
    ASSERT(MmIsSessionAddress(PopEventCallout) == TRUE);

    /*
     * FIXME: Unfortunately ReactOS currently does not support multiple
     * sessions. As a consequence we have to dispatch the power event
     * to only the active single session. This is not a problem per se
     * but when ReactOS will ever get multi-sessions support, this code
     * needs adaptations as it currently only dispatches power events to
     * only single session. For GDI events (PsW32GdiOn && PsW32GdiOff), these
     * shall be dispatched to the console session only, everything else have
     * to be broadcasted to all sessions.
     */

    /* Dispatch the power event directly if the process is within the session space */
    ProcessFlags = PsGetCurrentProcess()->Flags;
    SessionId = PsGetCurrentProcessSessionId();
    if (ProcessFlags & PSF_PROCESS_IN_SESSION_BIT)
    {
        PopEventCallout(Win32PwrEventParams);
        return;
    }

    /* It is not within the session space, get the session */
    Session = MmGetSessionById(SessionId);
    if (!Session)
    {
        /*
         * We cannot dispatch the power event to a session that does not
         * even exist. This would probably mean the process is within
         * a bogus session that was never inserted, so alert the debugger
         * in such case.
         */
        ASSERT(Session != NULL);
        return;
    }

    /* Attach to that session in order to dispatch the power event */
    Status = MmAttachSession(Session, &ApcState);
    if (!NT_SUCCESS(Status))
    {
        /* Failing to attach signifies a major problem, bail out */
        ASSERT(Status == STATUS_SUCCESS);
        return;
    }

    /* Dispatch the power event */
    PopEventCallout(Win32PwrEventParams);
    MmDetachSession(Session, &ApcState);
    MmQuitNextSession(Session);
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
PopPowerPolicyNotification(VOID)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

VOID
NTAPI
PopPowerPolicySystemIdle(VOID)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

VOID
NTAPI
PopPowerPolicyTimeChange(VOID)
{
    WIN32_POWEREVENT_PARAMETERS Params;

    /* Invoke the Win32k power callout to dispatch the time change event */
    Params.EventNumber = PsW32SystemTime;
    Params.Code = 0;
    PopDispatchPowerEvent(&Params);
}

VOID
NTAPI
PopInitializePowerPolicy(
    _Out_ PSYSTEM_POWER_POLICY PowerPolicy)
{
    ULONG PolicyIndex;

    PAGED_CODE();

    /*
     * Zero out the structure and set the policy revision. Up to this day
     * the Revision 1 is universal across all editions of Windows. This is
     * unlikely to change, unless a huge revamp occurs in the power manager.
     */
    RtlZeroMemory(PowerPolicy, sizeof(SYSTEM_POWER_POLICY));
    PowerPolicy->Revision = POP_SYSTEM_POWER_POLICY_REVISION_V1;

    /* Let Winlogon behave normally for this policy (no logon lock on sleep) */
    PowerPolicy->WinLogonFlags = 0;

    /* Set the default power actions and states for this policy */
    PowerPolicy->LidOpenWake = PowerSystemWorking;
    PowerPolicy->LidClose.Action = PowerActionNone;
    PowerPolicy->ReducedLatencySleep = PowerSystemSleeping1;
    PowerPolicy->MinSleep = PowerSystemSleeping1;
    PowerPolicy->MaxSleep = PowerSystemSleeping3;
    PopSetButtonPowerAction(&PowerPolicy->PowerButton, PowerActionShutdownOff);
    PopSetButtonPowerAction(&PowerPolicy->SleepButton, PowerActionSleep);

    /* Setup the default minimum sleep state for all the discharge policies */
    for (PolicyIndex = 0; PolicyIndex < NUM_DISCHARGE_POLICIES; PolicyIndex++)
    {
        PowerPolicy->DischargePolicy[PolicyIndex].MinSystemState = PowerSystemSleeping1;
    }

    /* Set the default fan throttle constants */
    PowerPolicy->FanThrottleTolerance = 100;
    PowerPolicy->ForcedThrottle = 100;
    PowerPolicy->OverThrottled.Action = PowerActionNone;

    /*
     * Let the system be notified of at least one resolution of a system
     * power state change event.
     */
    PowerPolicy->BroadcastCapacityResolution = 1;
}

VOID
NTAPI
PopDefaultPolicies(VOID)
{
    UNIMPLEMENTED;
    return;
}

VOID
NTAPI
PopRegisterPowerPolicyWorker(
    _In_ POP_POWER_POLICY_WORKER_TYPES WorkerType,
    _In_ PPOP_POLICY_WORKER_FUNC WorkerFunction)
{
    PAGED_CODE();

    /* Do not let the caller on tricking us with an unknown policy type */
    ASSERT((WorkerType >= PolicyWorkerNotification) && (WorkerType <= PolicyWorkerMax));

    /* Register a policy worker of this type */
    PopPolicyWorker[WorkerType].Pending = FALSE;
    PopPolicyWorker[WorkerType].Thread = NULL;
    PopPolicyWorker[WorkerType].WorkerFunction = WorkerFunction;
}

VOID
NTAPI
PopRequestPolicyWorker(
    _In_ POP_POWER_POLICY_WORKER_TYPES WorkerType)
{
    BOOLEAN PendingWorker;
    KIRQL Irql;

    /*
     * Check if this kind of worker type has not been requested before.
     * By rule there can be only one worker enqueued for each type.
     * It does not matter if somebody wants to enqueue a worker that is
     * already pending, because once a policy worker fires up it will
     * address the latest changes that will take effect on the system.
     */
    PopAcquirePowerPolicyWorkerLock(&Irql);
    PendingWorker = PopPolicyWorker[WorkerType].Pending;
    if (!PendingWorker)
    {
        /* This type of worker was never requested, set the pending mark */
        PopPolicyWorker[WorkerType].Pending = TRUE;

        /*
         * For debugging purposes attach the current calling thread to
         * this policy worker so that we know which thread requested it before.
         */
        PopPolicyWorker[WorkerType].Thread = KeGetCurrentThread();
    }

    PopReleasePowerPolicyWorkerLock(Irql);
}

VOID
NTAPI
PopCheckForPendingWorkers(VOID)
{
    ULONG WorkerIndex;
    KIRQL Irql;
    BOOLEAN PendingFound = FALSE;

    /*
     * The current calling thread owns the policy lock. This means the
     * thread is currently doing changes at certain power policies.
     * We will dispatch any outstanding workers as soon as the calling
     * thread leaves the policy manager.
     */
    if (PopPowerPolicyOwnerLockThread == KeGetCurrentThread())
    {
        return;
    }

    /* Do not allow any further worker requests, check for any pending work */
    PopAcquirePowerPolicyWorkerLock(&Irql);
    for (WorkerIndex = PolicyWorkerNotification;
         WorkerIndex < PolicyWorkerMax;
         WorkerIndex++)
    {
        /* Is this worker pending? */
        if (PopPolicyWorker[WorkerIndex].Pending)
        {
            /* It is, stop locking */
            PendingFound = TRUE;
            break;
        }
    }

    /* If no outstanding work was found then just quit */
    if (!PendingFound)
    {
        PopReleasePowerPolicyWorkerLock(Irql);
        return;
    }

    /*
     * At least one policy worker has been requested and currently
     * pending for dispatch. Check that the policy manager worker
     * thread is not already running. Enqueue it if need be.
     */
    if (!PopPendingPolicyWorker)
    {
        PopPendingPolicyWorker = TRUE;
        ExQueueWorkItem(&PopPowerPolicyWorkItem, DelayedWorkQueue);
    }

    /* Allow worker requests now */
    PopReleasePowerPolicyWorkerLock(Irql);
}

_Use_decl_annotations_
VOID
NTAPI
PopPolicyManagerWorker(
    _In_ PVOID Parameter)
{
    ULONG WorkerIndex;
    KIRQL Irql;

    /*
     * Iterate over the registered policy workers and look for any of these
     * that are currently pending for dispatch.
     */
    PopAcquirePowerPolicyWorkerLock(&Irql);
    for (WorkerIndex = PolicyWorkerNotification;
         WorkerIndex < PolicyWorkerMax;
         WorkerIndex++)
    {
        /* Process this policy worker only if it has been requested */
        if (PopPolicyWorker[WorkerIndex].Pending)
        {
            /* Clear the pending mark, we will serve this policy worker now */
            PopPolicyWorker[WorkerIndex].Pending = FALSE;
            PopPolicyWorker[WorkerIndex].Thread = NULL;
            PopReleasePowerPolicyWorkerLock(Irql);

            /* Dispatch it */
            PopPolicyWorker[WorkerIndex].WorkerFunction();

            /* Look for the next policy worker */
            PopAcquirePowerPolicyWorkerLock(&Irql);
        }
    }

    /* The policy manager worker thread has finished its job */
    PopPendingPolicyWorker = FALSE;
    PopReleasePowerPolicyWorkerLock(Irql);
}

/* EOF */
