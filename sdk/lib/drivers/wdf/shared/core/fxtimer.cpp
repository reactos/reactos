/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxTimer.hpp

Abstract:

    This module implements a frameworks managed TIMER that
    can synchrononize with driver frameworks object locks.

Author:



Environment:

    Both kernel and user mode

Revision History:


--*/

#include "coreprivshared.hpp"

#include "FxTimer.hpp"

// Tracing support
extern "C" {
#include "FxTimer.tmh"
}

//
// Public constructors
//

FxTimer::FxTimer(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    ) :
    FxNonPagedObject(FX_TYPE_TIMER, sizeof(FxTimer), FxDriverGlobals)
{
   m_Object = NULL;
   m_Period = 0;
   m_TolerableDelay = 0;
   m_CallbackLock = NULL;
   m_CallbackLockObject = NULL;
   m_Callback = NULL;
   m_RunningDown = FALSE;
   m_SystemWorkItem = NULL;
   m_CallbackThread = NULL;
   m_StopThread = NULL;
   m_StopAgain = FALSE;
   m_StartAborted = FALSE;

   //
   // Mark the object has having passive level dispose so that KeFlushQueuedDpcs
   // can be called in Dispose().
   //
   MarkPassiveDispose(ObjectDoNotLock);

   MarkDisposeOverride(ObjectDoNotLock);
}

FxTimer::~FxTimer()
{
    //
    // If this hits, its because someone destroyed the TIMER by
    // removing too many references by mistake without calling WdfObjectDelete
    //
    if (m_Object != NULL) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFTIMER %p destroyed without calling WdfObjectDelete, "
            "or by Framework processing DeviceRemove. Possible reference count "
            "problem?", GetObjectHandleUnchecked());
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }

    ASSERT(m_SystemWorkItem == NULL);
}

_Must_inspect_result_
NTSTATUS
FxTimer::_Create(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PWDF_TIMER_CONFIG Config,
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in FxObject* ParentObject,
    __out WDFTIMER* Timer
    )
{
    FxTimer* pFxTimer;
    NTSTATUS status;

    pFxTimer = new(FxDriverGlobals, Attributes) FxTimer(FxDriverGlobals);

    if (pFxTimer == NULL) {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = pFxTimer->Initialize(
        Attributes,
        Config,
        ParentObject,
        Timer
        );

    if (!NT_SUCCESS(status)) {
        pFxTimer->DeleteFromFailedCreate();
    }

    return status;
}

_Must_inspect_result_
NTSTATUS
FxTimer::Initialize(
    __in PWDF_OBJECT_ATTRIBUTES Attributes,
    __in PWDF_TIMER_CONFIG Config,
    __in FxObject* ParentObject,
    __out WDFTIMER* Timer
    )
{
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    IFxHasCallbacks* pCallbacks;
    NTSTATUS status;
    BOOLEAN isPassiveTimer;

    pFxDriverGlobals = GetDriverGlobals();
    pCallbacks = NULL;
    isPassiveTimer = FALSE;

    m_Period = Config->Period;

    // Set tolerable delay.
    if (Config->Size > sizeof(WDF_TIMER_CONFIG_V1_7)) {
        m_TolerableDelay = Config->TolerableDelay;
    }

    if (Config->Size > sizeof(WDF_TIMER_CONFIG_V1_11)) {
        m_UseHighResolutionTimer = Config->UseHighResolutionTimer;
    }

    // Set users callback function
    m_Callback = Config->EvtTimerFunc;

    //
    // Decide whether to use the legacy KTimer or the new Ktimer2/ExTimer
    // and call the appropriate initialization routine.
    // The new ExTimers expose two kind of timers:no wake timers and the
    // high resolution timers. For kernel mode, these timers are only exposed
    // to new clients.
    // For user mode,the underlying Threadpool APIs internally were upgraded
    // to using the no wake timers and we don't expose the High
    // resolution timers. Therefore the user mode code does not need to use
    // the ex initialization.
    //

#if (FX_CORE_MODE == FX_CORE_KERNEL_MODE)
    if (pFxDriverGlobals->IsVersionGreaterThanOrEqualTo(1,13)) {
        status = m_Timer.InitializeEx(this, FxTimer::_FxTimerExtCallbackThunk, m_Period,
                                      m_TolerableDelay, m_UseHighResolutionTimer);
    } else {
        status = m_Timer.Initialize(this, FxTimer::_FxTimerDpcThunk, m_Period);
    }
#else
    status = m_Timer.Initialize(this, FxTimer::_FxTimerDpcThunk, m_Period);
#endif

    if (!NT_SUCCESS(status)) {
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "Failed to initialize timer %!STATUS!", status);
        return status;
    }

    //
    // As long as we are associated, the parent object holds a reference
    // count on the TIMER.
    //
    // We keep an extra reference count since on Dispose, we wait until
    // all outstanding DPCs complete before allowing finalization.
    //
    // This reference must be taken early before we return any failure,
    // since Dispose() expects this extra reference, and Dispose() will
    // be called even if we return a failure status right now.
    //
    ADDREF(this);

    m_DeviceBase = FxDeviceBase::_SearchForDevice(ParentObject, &pCallbacks);
    if (m_DeviceBase == NULL) {
        return STATUS_INVALID_DEVICE_REQUEST;
    }

    if (Attributes->ExecutionLevel == WdfExecutionLevelPassive) {
        isPassiveTimer = TRUE;
    }

    //
    // Configure Serialization for the callbacks on the supplied object.
    //
    status = _GetEffectiveLock(
        ParentObject,
        pCallbacks,
        Config->AutomaticSerialization,
        isPassiveTimer,
        &m_CallbackLock,
        &m_CallbackLockObject
        );

    if (!NT_SUCCESS(status)) {
        if (status == STATUS_WDF_INCOMPATIBLE_EXECUTION_LEVEL) {






            DoTraceLevelMessage(
                pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                "ParentObject %p cannot automatically synchronize callbacks "
                "with a Timer since it is configured for passive level callback "
                "constraints. Set AutomaticSerialization to FALSE. %!STATUS!",
                Attributes->ParentObject, status);
        }

        return status;
    }

    //
    // If the caller wants passive callback then create a workitem.
    //
    if (isPassiveTimer) {
        status = FxSystemWorkItem::_Create(pFxDriverGlobals,
                                          m_Device->GetDeviceObject(),
                                          &m_SystemWorkItem
                                          );
        if (!NT_SUCCESS(status)) {
            DoTraceLevelMessage(pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                        "Could not allocate workitem: %!STATUS!", status);
            return status;
        }
    }

    //
    // We automatically synchronize with and reference count
    // the lifetime of the framework object to prevent any TIMER races
    // that can access the object while it is going away.
    //

    //
    // The caller supplied object is the object the caller wants the
    // TIMER to be associated with, and the framework must ensure this
    // object remains live until the TIMER object is destroyed. Otherwise,
    // it could access either object context memory, or an object API
    // on a freed object.
    //
    // Due to the locking model of the framework, the lock may actually
    // be owned by a higher level object as well. This is the lockObject
    // returned. As long was we are a child of this object, the lockObject
    // does not need to be dereferenced since it will notify us of Cleanup
    // before it goes away.
    //

    //
    // Associate the FxTimer with the object. When this object Cleans up, it
    // will notify our Dispose function as well.
    //

    //
    // Add a reference to the parent object we are associated with.
    // We will be notified of Cleanup to release this reference.
    //
    ParentObject->ADDREF(this);

    //
    // Save the ptr to the object the TIMER is associated with
    //
    m_Object = ParentObject;

    //
    // Attributes->ParentObject is the same as ParentObject. Since we already
    // converted it to an object, use that.
    //
    status = Commit(Attributes, (WDFOBJECT*)Timer, ParentObject);

    if (!NT_SUCCESS(status)) {
        return status;
    }

    return status;
}

VOID
FxTimer::TimerHandler(
    VOID
    )
{
    FX_TRACK_DRIVER(GetDriverGlobals());

    if (m_Callback != NULL) {

        //
        // Save the current thread object pointer. We will use this avoid
        // deadlock if the driver tries to delete or stop the timer from within
        // the callback.
        //
        m_CallbackThread = Mx::MxGetCurrentThread();

        if (m_CallbackLock != NULL) {
            KIRQL irql = 0;

            m_CallbackLock->Lock(&irql);
            m_Callback(GetHandle());
            m_CallbackLock->Unlock(irql);
        }
        else {
           m_Callback(GetHandle());
        }

        m_CallbackThread = NULL;
    }
}

VOID
FxTimer::_FxTimerDpcThunk(
    __in PKDPC TimerDpc,
    __in PVOID DeferredContext,
    __in PVOID SystemArgument1,
    __in PVOID SystemArgument2
    )
/*++

Routine Description:

    This is the C routine called by the kernel's TIMER DPC handler

Arguments:

    TimerDpc        -   our DPC object associated with our Timer
    DeferredContext -   Context for the TIMER that we setup in DriverEntry
    SystemArgument1 -
    SystemArgument2 -

Return Value:

    Nothing.

--*/

{
    FxTimer* pTimer = (FxTimer*)DeferredContext;

    UNREFERENCED_PARAMETER(TimerDpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    if (pTimer->m_SystemWorkItem == NULL) {

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
        FxPerfTraceDpc(&pTimer->m_Callback);
#endif

        //
        // Dispatch-level timer callback
        //
        pTimer->TimerHandler();
    }
    else {
        //
        // Passive timer callback.Queue only if the previous one is completed.
        //
        pTimer->m_SystemWorkItem->TryToEnqueue(_FxTimerWorkItemCallback, pTimer);
    }

    return;
}

VOID
FxTimer::_FxTimerExtCallbackThunk(
    __in PEX_TIMER Timer,
    __in PVOID Context
    )
/*++

Routine Description:

    This is the C routine called by the kernel's ex timer

Arguments:

    Timer        -      Ex timer
    Context      -      Context for the TIMER that we passed while creating it

Return Value:

    Nothing.

--*/

{
    FxTimer* pTimer = (FxTimer*)Context;

    UNREFERENCED_PARAMETER(Timer);

    if (pTimer->m_SystemWorkItem == NULL) {

#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
        FxPerfTraceDpc(&pTimer->m_Callback);
#endif

        //
        // Dispatch-level timer callback
        //
        pTimer->TimerHandler();
    }
    else {
        //
        // Passive timer callback.Queue only if the previous one is completed.
        //
        pTimer->m_SystemWorkItem->TryToEnqueue(_FxTimerWorkItemCallback, pTimer);
    }

    return;
}

VOID
FxTimer::_FxTimerWorkItemCallback(
    __in PVOID Parameter
    )
/*++

Routine Description:
   Thunk used when callback must be made at passive-level

--*/
{
    FxTimer* pTimer = (FxTimer*)Parameter;

    pTimer->TimerHandler();

    return;
}

//
// Called when DeleteObject is called, or when the parent
// is being deleted or Disposed.
//
// Also invoked directly by the cleanup list at our request after
// a Dispose occurs and must be deferred if not at passive level.
//
BOOLEAN
FxTimer::Dispose()
{
    KIRQL   irql;

    // MarkPassiveDispose() in Initialize ensures this
    ASSERT(Mx::MxGetCurrentIrql() == PASSIVE_LEVEL);

    //
    // Signal that we are running down.
    //
    Lock(&irql);
    m_RunningDown = TRUE;
    Unlock(irql);

    //
    // Cancel the timer, wait for its callback, then cleanup.
    //
    FlushAndRundown();

    return TRUE;
}

VOID
FxTimer::FlushAndRundown(
    VOID
    )
/*++

Routine Description:
    Called by the system work item to finish the rundown.

Arguments:
    None

Return Value:
    None

  --*/
{
    FxObject* pObject;

    if (m_CallbackThread == Mx::MxGetCurrentThread()) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                        "Deleting WDFTIMER %p from with in the callback will "
                        "lead to deadlock, PRKTHREAD %p",
                        GetHandle(), m_CallbackThread);
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }

    //
    // Cancel the timer, wait for its callback.
    //
    Stop(TRUE);

    //
    // Delete will also wait for the workitem to exit.
    //
    if (m_SystemWorkItem != NULL) {
        m_SystemWorkItem->DeleteObject();
        m_SystemWorkItem = NULL;
    }

    //
    // Release our reference count to the associated parent object if present
    //
    if (m_Object != NULL) {
        pObject = m_Object;
        m_Object = NULL;

        pObject->RELEASE(this);
    }

    //
    // Perform our final release to ourselves, destroying the FxTimer
    //
    RELEASE(this);
}

BOOLEAN
FxTimer::Start(
    __in LARGE_INTEGER DueTime
    )

/*++

Routine Description:

    Start or restart the timer

Arguments:

    DueTime - Time when the timer will be scheduled

Returns:

    TRUE if a previous timer was reset to the new duetime.
    FALSE otherwise.

--*/

{
    KIRQL   irql;
    BOOLEAN result = FALSE;
    BOOLEAN startTimer = FALSE;

    Lock(&irql);

    //
    // Basic check to make sure timer object is not deleted. Note that this
    // logic is not foolproof b/c someone may dispose the timer just after this
    // validation and before the start routine queues the timer; but at least it
    // is better than nothing.
    //
    if (m_RunningDown) {
        DoTraceLevelMessage(GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                        "Calling WdfTimerStart when the timer object %p is"
                        " running down will lead to a crash",
                        GetHandle());
        FxVerifierDbgBreakPoint(GetDriverGlobals());
    }
    else if (m_StopThread != NULL) {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
            "WDFTIMER 0x%p is been stopped by PRKTHREAD 0x%p. "
            "Ignoring the request to start timer",
            GetHandle(), m_StopThread);

        //
        // Let the stop thread know that we aborted this start operation.
        //
        m_StartAborted = TRUE;
    }
    else {
        //
        // Yes, the timer can be started.
        //
        startTimer = TRUE;
    }

    Unlock(irql);

    if (startTimer) {
        //
        // It may be possible for the timer to fire before the call from
        // KeSetTimerEx completes. If this happens and if the timer callback
        // disposes the timer object, a dispose work-item is queued.
        // This work-item in turn may also run before KeSetTimerEx completes,
        // making the object invalid by the time we try to take its Lock()
        // below. This ADDREF() prevents the object from going away and it is
        // matched by a RELEASE() when we are done.
        //
        ADDREF(this);

        //
        // Call the tolerable timer API only if OS supports it and driver
        // requested it.
        //
        result = m_Timer.StartWithReturn(DueTime, m_TolerableDelay);

        Lock(&irql);
        if (m_StopThread != NULL) {
            m_StopAgain = TRUE;
        }
        Unlock(irql);

        //
        // See ADDREF() comment above.
        //
        RELEASE(this);
    }

    return result;
}

BOOLEAN
FxTimer::Stop(
    __in BOOLEAN  Wait
    )
{
    KIRQL   irql;
    BOOLEAN result;
#ifdef DBG
    ULONG   retryCount = 0;
#endif

    if (Wait) {
        //
        // If the caller specified wait, we will flush the queued DPC's
        // to ensure any outstanding timer DPC has finished executing.
        //
        // The return value of timers is ambiguous in the case of periodic
        // timers, so we flush whenever the caller desires to ensure all
        // callbacks are complete.
        //

        //
        // Make sure the stop is not called from within the callback
        // because it's semantically incorrect and can lead to deadlock
        // if the wait parameter is set.
        //
        if (m_CallbackThread == Mx::MxGetCurrentThread()) {
            DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Calling WdfTimerStop from within the WDFTIMER "
                    "%p callback will lead to deadlock, PRKTHREAD %p",
                    GetHandle(), m_CallbackThread);
            FxVerifierDbgBreakPoint(GetDriverGlobals());
            return FALSE;
        }

        if (GetDriverGlobals()->FxVerifierOn) {
            if (Mx::MxGetCurrentIrql() != PASSIVE_LEVEL) {
                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "WdfTimerStop(Wait==TRUE) called at IRQL > PASSIVE_LEVEL, "
                    "current IRQL = 0x%x", Mx::MxGetCurrentIrql());
                FxVerifierDbgBreakPoint(GetDriverGlobals());
                return FALSE;
            }
        }

        //
        // Prevent the callback from restarting the timer.
        //
        Lock(&irql);

        //
        // Driver issue.
        //
        if (GetDriverGlobals()->IsVerificationEnabled(1, 9, OkForDownLevel) &&
            m_StopThread != NULL) {
            DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "Detected multiple calls to WdfTimerStop for "
                    "WDFTIMER 0x%p, stop in progress on PRKTHREAD 0x%p, "
                    "current PRKTHREAD 0x%p",
                    GetHandle(), m_StopThread, Mx::MxGetCurrentThread());
            FxVerifierDbgBreakPoint(GetDriverGlobals());
        }

        //
        // Reset the flag to find out if the timer's start logic aborts
        // b/c of this stop operation.
        //
        m_StartAborted = FALSE;

        //
        // This field is used for the following purposes:
        // (a) Let the start thread know not to restart the timer while stop
        //     is running.
        // (b) Detect concurrent calls to stop the timer.
        // (c) To remember the thread id of the stopping thread.
        //
        m_StopThread = Mx::MxGetCurrentThread();

        do {
#ifdef DBG
            retryCount++;
#endif
            //
            // Reset flag to catch when timer callback is restarting the
            // timer.
            //
            m_StopAgain = FALSE;
            Unlock(irql);

            //
            // Cancel the timer
            //
            result = m_Timer.Stop();

            //
            // Wait for the timer's DPC.
            //
            m_Timer.FlushQueuedDpcs();

            //
            // Wait for the timer's passive work item.
            //
            if (m_SystemWorkItem != NULL) {
                m_SystemWorkItem->WaitForExit();
            }

            Lock(&irql);
#ifdef DBG
            //
            // This loop is run for a max of 2 times.
            //
            ASSERT(retryCount < 3);
#endif
            //
            // Re-stop timer if timer was not in queue and
            // it got restarted in callback.
            //
        }while (result == FALSE && m_StopAgain);

        //
        // Stop completed.
        //
        m_StopThread = NULL;
        m_StopAgain = FALSE;

        //
        // Return TRUE (i.e., timer in queue) if
        // (a) stop logic successfully cancelled the timer or
        // (b) the start logic aborted b/c of this stop.
        //
        if (m_StartAborted) {
            result = TRUE;
            m_StartAborted = FALSE;
        }

        Unlock(irql);
    }
    else {
        //
        // Caller doesn't want any synchronization.
        // Cancel the timer.
        //
        result = m_Timer.Stop();
    }

    return result;
}
