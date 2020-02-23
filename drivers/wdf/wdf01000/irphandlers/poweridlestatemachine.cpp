#include "common/fxpoweridlestatemachine.h"
#include "common/fxpkgpnp.h"
#include "common/fxglobals.h"
#include "common/fxdevice.h"
#include "common/fxdriver.h"


const FxIdleStateTable FxPowerIdleMachine::m_StateTable[] =
{
    NULL
    // TODO: Fill this array
};

FxPowerIdleMachine::FxPowerIdleMachine(
    VOID
    )
/*++

Routine Description:
    Constructs the power idle state machine

Arguments:
    None

Return Value:
    None

  --*/
{
    //
    // m_Lock and m_PowerTimeoutTimer are now being initialized in Init method 
    // since they may fail for UM.
    //

    m_PowerTimeout.QuadPart = 0;
    m_CurrentIdleState = FxIdleStopped;
    
    m_EventHistoryIndex = 0;
    m_StateHistoryIndex = 0;

    RtlZeroMemory(&m_EventHistory[0], sizeof(m_EventHistory));
    RtlZeroMemory(&m_StateHistory[0], sizeof(m_StateHistory));

    m_TagTracker = NULL;
}

FxPowerIdleMachine::~FxPowerIdleMachine(
    VOID
    )
{
    if (m_TagTracker != NULL)
    {
        delete m_TagTracker;
        m_TagTracker = NULL;
    }
}

_Must_inspect_result_
NTSTATUS
FxPowerIdleMachine::Init(
    VOID
    )
{
    NTSTATUS status;

    //
    // For KM, event initialize always succeeds. For UM, it might fail.
    //
    status = m_D0NotificationEvent.Initialize(NotificationEvent, TRUE);
    if (!NT_SUCCESS(status))
    {
        return status;
    }
    
    //
    // For KM, timer initialize always succeeds. For UM, it might fail.
    //
    status = m_PowerTimeoutTimer.Initialize(this, _PowerTimeoutDpcRoutine, 0);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    Reset();

    return STATUS_SUCCESS;
}

__inline
FxPkgPnp*
GetPnpPkg(
    __inout FxPowerIdleMachine* This
    )
{
    return CONTAINING_RECORD(This,
                             FxPowerPolicyOwnerSettings,
                             m_PowerIdleMachine)->m_PkgPnp;
}

VOID
FxPowerIdleMachine::Reset(
    VOID
    )
/*++

Routine Description:
    Reset the state machine to a known state on a PDO restart.

Arguments:
    None

Return Value:
    None

  --*/
{
    FxPkgPnp* pPkgPnp;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    
    ASSERT(m_CurrentIdleState == FxIdleStopped);

    m_IoCount = 0;
    m_Flags = 0x0;

    pPkgPnp = GetPnpPkg(this);
    pFxDriverGlobals = pPkgPnp->GetDriverGlobals();

    if (pFxDriverGlobals->DebugExtension != NULL &&
        pFxDriverGlobals->DebugExtension->TrackPower != FxTrackPowerNone)
    {
        //
        // Ignore potential failure, power ref tracking is not an essential feature.
        //


        //
        (void)FxTagTracker::CreateAndInitialize(&m_TagTracker,
                                                pFxDriverGlobals,
                                                FxTagTrackerTypePower,
                                                pFxDriverGlobals->DebugExtension->TrackPower == FxTrackPowerRefsAndStack,
                                                (FxObject*)pPkgPnp->GetDevice());
    }

    SendD0Notification();
}

VOID
FxPowerIdleMachine::SendD0Notification(
    VOID
    )
{
    m_D0NotificationEvent.Set();

    if (m_Flags & FxPowerIdleSendPnpPowerUpEvent)
    {

        m_Flags &= ~FxPowerIdleSendPnpPowerUpEvent;

        //
        // Send an event to the Pnp state machine indicating that the device is
        // now in D0.
        //
#if (FX_CORE_MODE==FX_CORE_KERNEL_MODE)
        GetPnpPkg(this)->PnpProcessEvent(PnpEventDeviceInD0);
#else
        GetPnpPkg(this)->PnpProcessEvent(
            PnpEventDeviceInD0, 
            TRUE // ProcessEventOnDifferentThread
            );
#endif

    }

    return;
}

_Must_inspect_result_
NTSTATUS
FxPowerIdleMachine::PowerReferenceWorker(
    __in BOOLEAN WaitForD0,
    __in FxPowerReferenceFlags Flags,
    __in_opt PVOID Tag,
    __in_opt LONG Line,
    __in_opt PCSTR File
    )
/*++

Routine Description:
    Caller wants to move the device into D0 manually.  The caller may optionally
    wait synchronously for the transition to occur if the device is currently in
    Dx.

Arguments:
    WaitForD0 - TRUE if the caller wants to synchronously wait for the Dx to D0
                transition

    QueryPnpPending - TRUE if we are being called to bring the device back to 
                working state when a QueryRemove or a QueryStop 
                
Return Value:
    NTSTATUS

    STATUS_SUCCESS - success
    STATUS_PENDING - transition is occurring
    STATUS_POWER_STATE_INVALID - ower transition has failed

  --*/
{
    NTSTATUS status;
    KIRQL irql;
    ULONG count = 0;
    
    //
    // Poke the state machine
    //
    status = IoIncrementWithFlags(Flags, &count);

    //
    // STATUS_PENDING indicates a Dx to D0 transition is occurring right now
    //
    if (status == STATUS_PENDING)
    {
        if (WaitForD0)
        {
            FxPkgPnp* pPkgPnp;

            ASSERT(Mx::MxGetCurrentIrql() <= APC_LEVEL);

            //
            // With the current usage, if WaitForD0 is TRUE, then the only 
            // acceptable flag is FxPowerReferenceDefault. 
            //
            // If the usage changes in the future such that it is acceptable to
            // have WaitForD0 set to TRUE and some flag(s) set, then the ASSERT
            // below should be updated accordingly (or removed altogether).
            //
            ASSERT(FxPowerReferenceDefault == Flags);

            pPkgPnp = GetPnpPkg(this);

            DoTraceLevelMessage(
                pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "WDFDEVICE %p in thread %p waiting synchronously for Dx to D0 "
                "transition",
                pPkgPnp->GetDevice()->GetHandle(),
                Mx::MxGetCurrentThread());

            //
            // Returns success always
            //
            (void) FxPowerIdleMachine::WaitForD0();

            m_Lock.Acquire(&irql);

            //
            // If WaitForD0 is TRUE, then the FxPowerIdleSendPnpPowerUpEvent 
            // flag can't be set. That flag is only used when the PnP state 
            // machine waits asynchronously for the device to power up during
            // query-remove.
            //
            ASSERT(0 == (m_Flags & FxPowerIdleSendPnpPowerUpEvent));

            if ((m_Flags & FxPowerIdlePowerFailed) != 0x0 ||
                (m_Flags & FxPowerIdleIsStarted) == 0x0)
            {
                //
                // Event was set because a power up or down failure occurred
                //
                status = STATUS_POWER_STATE_INVALID;

                if (m_Flags & FxPowerIdlePowerFailed)
                {
                    DoTraceLevelMessage(
                        pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "WDFDEVICE %p waiting for D0 in thread %p failed because of "
                        "power failure, %!STATUS!",
                        pPkgPnp->GetDevice()->GetHandle(),
                        Mx::MxGetCurrentThread(),
                        status);
                }
                else
                {
                    COVERAGE_TRAP();
                    DoTraceLevelMessage(
                        pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                        "WDFDEVICE %p waiting for D0 in thread %p failed because of "
                        "invalid state , %!STATUS!",
                        pPkgPnp->GetDevice()->GetHandle(),
                        Mx::MxGetCurrentThread(), status);
                }

                //
                // Decrement the io count that was taken above
                //
                ASSERT(m_IoCount > 0);
                m_IoCount--;
                ProcessEventLocked(PowerIdleEventIoDecrement);
            }
            else
            {
                //
                // Successfully returned to D0
                //
                status = STATUS_SUCCESS;
            }
            m_Lock.Release(irql);
        }
    }

    if (m_TagTracker != NULL)
    {
        //
        // Only track the reference if the call was successful
        // and the counter was actually incremented.
        //
        if (status == STATUS_SUCCESS || status == STATUS_PENDING)
        {
            m_TagTracker->UpdateTagHistory(Tag, Line, File, TagAddRef, count);
        }
    }

    return status;
}

VOID
FxPowerIdleMachine::IoDecrement(
    __in_opt PVOID Tag,
    __in_opt LONG Line,
    __in_opt PCSTR File
    )
/*++

Routine Description:
    Public function which allows the caller decrement the pending io count on
    this state machine.  If the count goes to zero and idle is enabled, then
    the timer is started.

Arguments:
    None

Return Value:
    None

  --*/
{
    KIRQL irql;
    FxPkgPnp* pPkgPnp;
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    ULONG count;

    pPkgPnp = GetPnpPkg(this);
    pFxDriverGlobals = pPkgPnp->GetDriverGlobals();

    m_Lock.Acquire(&irql);

    if (m_IoCount == 0)
    {
        //
        // We can get here for the following reasons:
        // 1. Driver called WdfDevicveStopIdle/WdfDeviceResumeIdle in a mismatched
        //    manner. This is a driver bug.
        // 2. Framework did power deref without a corresponding power ref.
        //    This would be a framework bug. 
        //
        // We will break into debugger if verifier is turned on. This will allow
        // developers to catch this problem during develeopment.
        // We limit this break to version 1.11+ because otherwise older drivers 
        // may hit this, and if they cannot be fixed for some reason, then 
        // verifier would need to be turned off to avoid the break which is not 
        // desirable. 
        // 
        DoTraceLevelMessage(
            pFxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p The device is being power-dereferenced"
            " without a matching power-reference. This could occur if driver"
            " incorrectly calls WdfDeviceResumeIdle without a matching call to"
            " WdfDeviceStopIdle.",
            pPkgPnp->GetDevice()->GetHandle(),
            pPkgPnp->GetDevice()->GetDeviceObject());
        
        /*if (pFxDriverGlobals->IsVerificationEnabled(1, 11, OkForDownLevel))
        {
           FxVerifierDbgBreakPoint(pFxDriverGlobals);
        }*/
    }

    ASSERT(m_IoCount > 0);
    count = --m_IoCount;
    ProcessEventLocked(PowerIdleEventIoDecrement);
    m_Lock.Release(irql);

    if (m_TagTracker != NULL)
    {
        m_TagTracker->UpdateTagHistory(Tag, Line, File, TagRelease, count);
    }
}

VOID
FxPowerIdleMachine::ProcessEventLocked(
    __in FxPowerIdleEvents Event
    )
/*++

Routine Description:
    Processes an event and runs it through the state machine

Arguments:


Return Value:


  --*/

{
    const FxIdleStateTable* entry;
    FxPowerIdleStates newState;
    FxPkgPnp* pPkgPnp;

    pPkgPnp = GetPnpPkg(this);

    m_EventHistory[m_EventHistoryIndex] = Event;
    m_EventHistoryIndex = (m_EventHistoryIndex + 1) %
                          (sizeof(m_EventHistory)/sizeof(m_EventHistory[0]));

    entry = &m_StateTable[m_CurrentIdleState-FxIdleStopped];
    newState = FxIdleMax;

    for (ULONG i = 0; i < entry->TargetStatesCount; i++)
    {
        if (entry->TargetStates[i].PowerIdleEvent == Event)
        {
            DO_EVENT_TRAP(&entry->TargetStates[i]);
            newState = entry->TargetStates[i].PowerIdleState;
            break;
        }
    }

    if (newState == FxIdleMax)
    {
        switch (Event) {
        case PowerIdleEventIoIncrement:
        case PowerIdleEventIoDecrement:
            //
            // We always can handle io increment, io decrement, and query return
            // to idle from any state...
            //
            break;

        case PowerIdleEventEnabled:
            if (m_Flags & FxPowerIdleTimerEnabled)
            {
                //
                // Getting an enable event while enabled is OK
                //
                break;
            }
            //  ||   ||  Fall    ||  ||
            //  \/   \/  through \/  \/

        default:
            //
            // ...but we should not be dropping any other events from this state.
            //

            //
            DoTraceLevelMessage(
                pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p power idle state %!FxPowerIdleStates!"
                " dropping event %!FxPowerIdleEvents!",
                pPkgPnp->GetDevice()->GetHandle(),
                pPkgPnp->GetDevice()->GetDeviceObject(),
                m_CurrentIdleState, Event);

            COVERAGE_TRAP();
        }
    }

    while (newState != FxIdleMax)
    {
        DoTraceLevelMessage(
            pPkgPnp->GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
            "WDFDEVICE 0x%p !devobj 0x%p entering power idle state "
            "%!FxPowerIdleStates! from %!FxPowerIdleStates!",
            pPkgPnp->GetDevice()->GetHandle(),
            pPkgPnp->GetDevice()->GetDeviceObject(),
            newState, m_CurrentIdleState);

        m_StateHistory[m_StateHistoryIndex] = newState;
        m_StateHistoryIndex = (m_StateHistoryIndex + 1) %
                              (sizeof(m_StateHistory)/sizeof(m_StateHistory[0]));

        m_CurrentIdleState = newState;
        entry = &m_StateTable[m_CurrentIdleState-FxIdleStopped];

        if (entry->StateFunc != NULL)
        {
            newState = entry->StateFunc(this);
        }
        else
        {
            newState = FxIdleMax;
        }
    }
}

__drv_maxIRQL(DISPATCH_LEVEL)
__drv_minIRQL(DISPATCH_LEVEL)
__drv_requiresIRQL(DISPATCH_LEVEL)
__drv_sameIRQL
VOID
FxPowerIdleMachine::_PowerTimeoutDpcRoutine(
    __in     PKDPC Dpc,
    __in_opt PVOID Context,
    __in_opt PVOID SystemArgument1,
    __in_opt PVOID SystemArgument2
    )
/*++

Routine Description:
    Timer DPC which posts the timeout expired event to the power policy state
    machine

Arguments:
    Dpc - DPC
    Context, SysArg1, SysArg2 - Unused

Return Value:
    None

  --*/
{
    FxPowerIdleMachine* pThis;

    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    pThis = (FxPowerIdleMachine*) Context;

    pThis->m_Lock.AcquireAtDpcLevel();
    pThis->ProcessEventLocked(PowerIdleEventTimerExpired);
    
#if FX_IS_KERNEL_MODE
    PFX_DRIVER_GLOBALS pFxDriverGlobals;
    PFN_WDF_DRIVER_DEVICE_ADD pDriverDeviceAdd;
    
    pFxDriverGlobals = GetPnpPkg(pThis)->GetDriverGlobals();

    //
    // We need to provide XPerf with a symbol of the client to figure out
    // which component this idle timer is for. Since AddDevice is always there
    // we use that to pass the symbol along.
    //
    pDriverDeviceAdd = pFxDriverGlobals->Driver->GetDriverDeviceAddMethod();

    //FxPerfTraceDpc(&pDriverDeviceAdd);
#endif

    pThis->m_Lock.ReleaseFromDpcLevel();
}

_Must_inspect_result_
NTSTATUS
FxPowerIdleMachine::IoIncrementWithFlags(
    __in FxPowerReferenceFlags Flags,
    __out_opt PULONG Count
    )
/*++

Routine Description:
    An enchanced version of FxPowerIdleMachine::IoIncrement that has special 
    behavior based on flags passed in by the caller. Please read the routine
    description of FxPowerIdleMachine::IoIncrement as well.

Arguments:
    Flags - The following flags are defined -
         FxPowerReferenceDefault - No special behavior
         FxPowerReferenceSendPnpPowerUpEvent - Set the 
           FxPowerIdleSendPnpPowerUpEvent flag in the idle state machine flags.
           This will indicate to the idle state machine that when the device 
           powers up, it needs to send the PnpEventDeviceInD0 event to the PnP 
           state machine.

Return Value:
    STATUS_PENDING if the state machine is transition from idle to non idle

    STATUS_SUCCESS otherwise

  --*/
{
    NTSTATUS status;
    KIRQL irql;

    m_Lock.Acquire(&irql);

    if (m_Flags & FxPowerIdlePowerFailed)
    {
        //
        // fail without incrementing the count because we are in an
        // invalid power state
        //
        status = STATUS_POWER_STATE_INVALID;
        COVERAGE_TRAP();
    }
    else if ((m_Flags & FxPowerIdleIsStarted) == 0x0)
    {
        //
        // The state machine is not yet in a started state
        //
        status = STATUS_POWER_STATE_INVALID;
    }
    else
    {
        m_IoCount++;
        if (Count != NULL)
        {
            *Count = m_IoCount;
        }
        
        ProcessEventLocked(PowerIdleEventIoIncrement);

        if (InD0Locked())
        {
            status = STATUS_SUCCESS;
        }
        else
        {
            status = STATUS_PENDING;
            if (Flags & FxPowerReferenceSendPnpPowerUpEvent)
            {
                m_Flags |= FxPowerIdleSendPnpPowerUpEvent;
            }
        }
    }
    m_Lock.Release(irql);

    return status;
}
