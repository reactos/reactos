#include "common/fxpowerstatemachine.h"
#include "common/fxpkgpnp.h"
#include "common/fxdevice.h"
#include "common/fxwatchdog.h"




#if FX_STATE_MACHINE_VERIFY
    #define VALIDATE_POWER_STATE(_CurrentState, _NewState)                        \
        ValidatePowerStateEntryFunctionReturnValue((_CurrentState), (_NewState))
#else
    #define VALIDATE_POWER_STATE(_CurrentState, _NewState)   (0)
#endif  //FX_STATE_MACHINE_VERIFY


//
// The Power State Machine
//
// This state machine responds to power IRPs that relate
// to devices.  It is a subordinate, responding either to IRPs
// that are sent by other drivers, or by IRPs that are
// sent by the Power Policy State Machine.
//
// It responds to:
//
// IRP_MN_SET_POWER -- device power IRPs only
// IRP_MN_WAIT_WAKE
// IRP_MN_WAIT_WAKE Complete
// PowerImplicitD0
// PowerImplicitD3
// ParentMovesToD0
// PowerPolicyStop
// PowerMarkPageable
// PowerMarkNonpageable
//


const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerD0OtherStates[] =
{
    { PowerImplicitD3, WdfDevStatePowerGotoD3Stopped },
    { PowerMarkNonpageable, WdfDevStatePowerDecideD0State },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerD0NPOtherStates[] =
{
    { PowerImplicitD3, WdfDevStatePowerGotoD3Stopped },
    { PowerMarkPageable, WdfDevStatePowerDecideD0State },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerD0BusWakeOwnerOtherStates[] =
{
    { PowerWakeArrival, WdfDevStatePowerEnablingWakeAtBus },
    { PowerImplicitD3, WdfDevStatePowerGotoD3Stopped },
    { PowerMarkNonpageable, WdfDevStatePowerDecideD0State },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerD0BusWakeOwnerNPOtherStates[] =
{
    { PowerWakeArrival, WdfDevStatePowerEnablingWakeAtBusNP },
    { PowerImplicitD3, WdfDevStatePowerGotoD3Stopped },
    { PowerMarkPageable, WdfDevStatePowerD0BusWakeOwner },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerD0ArmedForWakeOtherStates[] =
{
    { PowerWakeSucceeded, WdfDevStatePowerD0DisarmingWakeAtBus },
    { PowerWakeFailed, WdfDevStatePowerD0DisarmingWakeAtBus },
    { PowerWakeCanceled, WdfDevStatePowerD0DisarmingWakeAtBus },
    { PowerMarkNonpageable, WdfDevStatePowerD0ArmedForWakeNP },
    { PowerImplicitD3, WdfDevStatePowerGotoD3Stopped },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerD0ArmedForWakeNPOtherStates[] =
{
    { PowerWakeSucceeded, WdfDevStatePowerD0DisarmingWakeAtBusNP },
    { PowerWakeFailed, WdfDevStatePowerD0DisarmingWakeAtBusNP },
    { PowerWakeCanceled, WdfDevStatePowerD0DisarmingWakeAtBusNP },
    { PowerMarkPageable, WdfDevStatePowerD0ArmedForWake },
    { PowerImplicitD3, WdfDevStatePowerGotoD3Stopped },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerDNotZeroOtherStates[] =
{
    { PowerImplicitD3, WdfDevStatePowerGotoDxStopped },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerDNotZeroNPOtherStates[] =
{
    { PowerImplicitD3, WdfDevStatePowerGotoDxStoppedDisableInterruptNP },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_DxArmedForWakeOtherStates[] =
{
    { PowerWakeSucceeded, WdfDevStatePowerWakePending },
    { PowerWakeCanceled, WdfDevStatePowerWakePending },
    { PowerWakeFailed, WdfDevStatePowerWakePending },
    { PowerImplicitD3, WdfDevStatePowerDxStoppedDisarmWake },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_DxArmedForWakeNPOtherStates[] =
{
    { PowerWakeSucceeded, WdfDevStatePowerWakePendingNP },
    { PowerWakeFailed, WdfDevStatePowerWakePendingNP },
    { PowerWakeCanceled, WdfDevStatePowerWakePendingNP },
    { PowerImplicitD3, WdfDevStatePowerDxStoppedDisarmWakeNP },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_WakePendingOtherStates[] =
{
    { PowerImplicitD0, WdfDevStatePowerCheckParentStateArmedForWake },
    { PowerImplicitD3, WdfDevStatePowerDxStoppedDisarmWake },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_WakePendingNPOtherStates[] =
{
    { PowerImplicitD0, WdfDevStatePowerCheckParentStateArmedForWakeNP },
    { PowerImplicitD3, WdfDevStatePowerDxStoppedDisarmWakeNP },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerStoppedOtherStates[] =
{
    { PowerDx, WdfDevStatePowerStoppedCompleteDx },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_EVENT_TARGET_STATE FxPkgPnp::m_PowerDxStoppedOtherStates[] =
{
    { PowerD0, WdfDevStatePowerGotoStopped },
    { PowerEventMaximum, WdfDevStatePowerNull },
};

const POWER_STATE_TABLE FxPkgPnp::m_WdfPowerStates[] =
{
    { NULL,
      { PowerImplicitD0, WdfDevStatePowerStartingCheckDeviceType },
      NULL,
      { TRUE,
        PowerD0
        | PowerDx
        | PowerWakeArrival
        | PowerMarkPageable
        | PowerMarkNonpageable
        | PowerCompleteD0
      },
    },

    { FxPkgPnp::PowerCheckDeviceType,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerCheckDeviceTypeNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerCheckParentState,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerCheckParentStateNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerEnablingWakeAtBus,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerEnablingWakeAtBusNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerD0ArmedForWake,
      { PowerDx, WdfDevStatePowerGotoDx },
      FxPkgPnp::m_PowerD0OtherStates,
      { TRUE,
        PowerD0
        | PowerDx
        | PowerSingularEventMask
        | PowerParentToD0
      },
    },

    { FxPkgPnp::PowerD0NP,
      { PowerDx, WdfDevStatePowerGotoDxNP },
      FxPkgPnp::m_PowerD0NPOtherStates,
      { TRUE,
        PowerD0
        | PowerDx
      },
    },

    { FxPkgPnp::PowerD0BusWakeOwner,
      { PowerDx, WdfDevStatePowerGotoDx },
      FxPkgPnp::m_PowerD0BusWakeOwnerOtherStates,
      { TRUE,
        PowerD0
        | PowerDx
        | PowerWakeFailed
        | PowerImplicitD0
        | PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerD0BusWakeOwnerNP,
      { PowerDx, WdfDevStatePowerGotoDxNP },
      FxPkgPnp::m_PowerD0BusWakeOwnerNPOtherStates,
      { TRUE,
        PowerD0
        | PowerDx
        | PowerWakeFailed
        | PowerImplicitD0
        | PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerD0ArmedForWake,
      { PowerDx, WdfDevStatePowerGotoDxArmedForWake },
      FxPkgPnp::m_PowerD0ArmedForWakeOtherStates,
      { TRUE,
        PowerD0
        | PowerDx
        | PowerWakeSucceeded
        | PowerImplicitD0
        | PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerD0NP,
      { PowerDx, WdfDevStatePowerGotoDxArmedForWakeNP },
      FxPkgPnp::m_PowerD0ArmedForWakeNPOtherStates,
      { TRUE,
        PowerD0
        | PowerDx
        | PowerWakeSucceeded
        | PowerImplicitD0
        | PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerD0DisarmingWakeAtBus,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerD0DisarmingWakeAtBusNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerD0Starting,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerD0StartingConnectInterrupt,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerD0StartingDmaEnable,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerD0StartingStartSelfManagedIo,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerDecideD0State,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerGotoD3Stopped,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { NULL,
      { PowerImplicitD0, WdfDevStatePowerStartingCheckDeviceType },
      FxPkgPnp::m_PowerStoppedOtherStates,
      { TRUE,
        PowerD0
        | PowerDx
        | PowerWakeCanceled
        | PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerStartingCheckDeviceType,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerStartingChild,
      { PowerParentToD0, WdfDevStatePowerD0Starting },
      NULL,
      { TRUE,
        PowerD0
      },
    },

    { FxPkgPnp::PowerDxDisablingWakeAtBus,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerDxDisablingWakeAtBusNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerGotoDxArmedForWake,
      { PowerCompleteDx, WdfDevStatePowerGotoDxIoStopped },
      NULL,
      { FALSE,
        PowerWakeSucceeded
        | PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerGotoDxArmedForWake,
      { PowerCompleteDx, WdfDevStatePowerGotoDxIoStoppedNP },
      NULL,
      { FALSE,
        PowerWakeSucceeded
        | PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerGotoDNotZeroIoStopped,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerGotoDNotZeroIoStoppedNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerGotoDxNPFailed,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { NULL,
      { PowerD0, WdfDevStatePowerCheckDeviceType },
      FxPkgPnp::m_PowerDNotZeroOtherStates,
      { TRUE,
        PowerD0
        | PowerWakeArrival
        | PowerWakeSucceeded
        | PowerWakeFailed
        | PowerImplicitD0
        | PowerSingularEventMask
        | PowerParentToD0
        | PowerMarkPageable
      },
    },

    { NULL,
      { PowerD0, WdfDevStatePowerCheckDeviceTypeNP },
      FxPkgPnp::m_PowerDNotZeroNPOtherStates,
      { TRUE,
        PowerD0
        | PowerWakeArrival
        | PowerWakeSucceeded
        | PowerWakeFailed
        | PowerImplicitD0
        | PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerGotoDxArmedForWake,
      { PowerCompleteDx, WdfDevStatePowerGotoDxIoStoppedArmedForWake },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerGotoDxArmedForWake,
      { PowerCompleteDx, WdfDevStatePowerGotoDxIoStoppedArmedForWakeNP },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerGotoDxIoStoppedArmedForWake,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerGotoDxIoStoppedArmedForWakeNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { NULL,
      { PowerD0, WdfDevStatePowerCheckParentStateArmedForWake },
      FxPkgPnp::m_DxArmedForWakeOtherStates,
      { TRUE,
        PowerD0
        | PowerMarkPageable
      },
    },

    { NULL,
      { PowerD0, WdfDevStatePowerCheckParentStateArmedForWakeNP },
      FxPkgPnp::m_DxArmedForWakeNPOtherStates,
      { TRUE,
        PowerD0
        | PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerCheckParentStateArmedForWake,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerCheckParentStateArmedForWakeNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { NULL,
      { PowerParentToD0, WdfDevStatePowerDxDisablingWakeAtBus },
      NULL,
      { TRUE,
        PowerD0
      },
    },

    { NULL,
      { PowerParentToD0, WdfDevStatePowerDxDisablingWakeAtBusNP },
      NULL,
      { TRUE,
        PowerD0
      },
    },

    { FxPkgPnp::PowerStartSelfManagedIo,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerStartSelfManagedIoNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerStartSelfManagedIoFailed,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerStartSelfManagedIoFailedNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { NULL,
      { PowerParentToD0, WdfDevStatePowerWaking },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { NULL,
      { PowerParentToD0, WdfDevStatePowerWakingNP },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerWakePending,
      { PowerD0, WdfDevStatePowerCheckParentStateArmedForWake },
      FxPkgPnp::m_WakePendingOtherStates,
      { TRUE,
        PowerD0
        | PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerWakePending,
      { PowerD0, WdfDevStatePowerCheckParentStateArmedForWakeNP },
      FxPkgPnp::m_WakePendingNPOtherStates,
      { TRUE,
        PowerD0
        | PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerWaking,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerWakingNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerWakingConnectInterrupt,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerWakingConnectInterruptNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerWakingConnectInterruptFailed,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerWakingConnectInterruptFailedNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerWakingDmaEnable,
      { PowerCompleteD0, WdfDevStatePowerStartSelfManagedIo },
      NULL,
      { FALSE,
        PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerWakingDmaEnableNP,
      { PowerCompleteD0, WdfDevStatePowerStartSelfManagedIoNP },
      NULL,
      { FALSE,
        PowerMarkPageable
      },
    },

    { FxPkgPnp::PowerWakingDmaEnableFailed,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerWakingDmaEnableFailedNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerReportPowerUpFailedDerefParent,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerReportPowerUpFailed,
      { PowerImplicitD3, WdfDevStatePowerPowerFailedPowerDown },
      NULL,
      { TRUE,
        PowerD0
      },
    },

    { FxPkgPnp::PowerPowerFailedPowerDown,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerReportPowerDownFailed,
      { PowerImplicitD3, WdfDevStatePowerPowerFailedPowerDown },
      NULL,
      { TRUE,
        PowerD0
      },
    },

    { FxPkgPnp::PowerInitialConnectInterruptFailed,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerInitialDmaEnableFailed,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerInitialSelfManagedIoFailed,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerInitialPowerUpFailedDerefParent,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerInitialPowerUpFailed,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerDxStoppedDisarmWake,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerDxStoppedDisarmWakeNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerGotoDxStoppedDisableInterruptNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerGotoDxStopped,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { NULL,
      { PowerImplicitD0, WdfDevStatePowerDxStoppedDecideDxState },
      FxPkgPnp::m_PowerDxStoppedOtherStates,
      { TRUE,
        PowerD0
      },
    },

    { FxPkgPnp::PowerGotoStopped,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerStoppedCompleteDx,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerDxStoppedDecideDxState,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerDxStoppedArmForWake,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerDxStoppedArmForWakeNP,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { FxPkgPnp::PowerFinalPowerDownFailed,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

    { NULL,
      { PowerEventMaximum, WdfDevStatePowerNull },
      NULL,
      { FALSE,
        PowerEventInvalid },
    },

};




_Must_inspect_result_
NTSTATUS
FxPowerMachine::Init(
    __inout FxPkgPnp* Pnp,
    __in PFN_PNP_EVENT_WORKER WorkerRoutine
    )
{
    NTSTATUS status;
    
    status = FxThreadedEventQueue::Init(Pnp, WorkerRoutine);
    if (!NT_SUCCESS(status))
    {
        return status;
    }

    return STATUS_SUCCESS;
}

VOID
FxPkgPnp::PowerProcessEventInner(
    __inout FxPostProcessInfo* Info
    )
/*++

Routine Description:
    This routine runs the state machine.  It implements steps 10-15 of the
    algorithm described above.

--*/
{
    WDF_DEVICE_POWER_STATE currentPowerState, newState;
    CPPOWER_STATE_TABLE entry;
    FxPowerEvent event;
    KIRQL       oldIrql;

    //
    // Process as many events as we can.
    //
    for (;;)
    {
        newState = WdfDevStatePowerNull;
        currentPowerState = m_Device->GetDevicePowerState();
        entry = GetPowerTableEntry(currentPowerState);

        //
        // Get an event from the queue.
        //
        m_PowerMachine.Lock(&oldIrql);

        if (m_PowerMachine.IsEmpty())
        {
            m_PowerMachine.GetFinishedState(Info);

            //
            // The queue is empty.
            //
            m_PowerMachine.Unlock(oldIrql);
            return;
        }

        event = (FxPowerEvent) m_PowerMachine.m_Queue.Events[m_PowerMachine.GetHead()];

        //
        // At this point, we need to determine whether we can process this
        // event.
        //
        if (event & PowerPriorityEventsMask)
        {
            //
            // These are always possible to handle.
            //
            DO_NOTHING();
        }
        else
        {
            //
            // Check to see if this state can handle new events.
            //
            if (entry->StateInfo.Bits.QueueOpen == FALSE)
            {
                //
                // This state can't handle new events.
                //
                m_PowerMachine.Unlock(oldIrql);
                return;
            }
        }

        //
        // If the event obtained from the queue was a singular event, then
        // clear the flag to allow other similar events to be put into this
        // queue for processing.
        //
        if (m_PowerMachine.m_SingularEventsPresent & event)
        {
           m_PowerMachine.m_SingularEventsPresent &= ~event;
        }

        m_PowerMachine.IncrementHead();
        m_PowerMachine.Unlock(oldIrql);

        //
        // Find the entry in the power state table that corresponds to this event
        //
        if (entry->FirstTargetState.PowerEvent == event)
        {
            newState = entry->FirstTargetState.TargetState;

            DO_EVENT_TRAP(&entry->FirstTargetState);
        }
        else if (entry->OtherTargetStates != NULL)
        {
            ULONG i = 0;

            for (i = 0;
                 entry->OtherTargetStates[i].PowerEvent != PowerEventMaximum;
                 i++)
            {
                if (entry->OtherTargetStates[i].PowerEvent == event)
                {
                    newState = entry->OtherTargetStates[i].TargetState;
                    DO_EVENT_TRAP(&entry->OtherTargetStates[i]);
                    break;
                }
            }
        }

        if (newState == WdfDevStatePowerNull)
        {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_VERBOSE, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p current power state "
                "%!WDF_DEVICE_POWER_STATE! dropping event %!FxPowerEvent!",
                m_Device->GetHandle(), 
                m_Device->GetDeviceObject(),
                m_Device->GetDevicePowerState(), event);

            //
            // This state doesn't respond to the Event.  Potentially throw
            // the event away.
            //
            if ((entry->StateInfo.Bits.KnownDroppedEvents & event) == 0)
            {
                COVERAGE_TRAP();

                DoTraceLevelMessage(
                    GetDriverGlobals(), TRACE_LEVEL_WARNING, TRACINGPNP,
                    "WDFDEVICE %p !devobj 0x%p current state "
                    "%!WDF_DEVICE_POWER_STATE! event %!FxPowerEvent! is not a "
                    "known dropped event, known dropped events are "
                    "%!FxPowerEvent!", m_Device->GetHandle(),
                    m_Device->GetDeviceObject(), 
                    m_Device->GetDevicePowerState(),
                    event, entry->StateInfo.Bits.KnownDroppedEvents);
            }

            switch (event) {
            case PowerWakeSucceeded:
            case PowerWakeFailed:
            case PowerWakeCanceled:
                //
                // There are states where we don't care if the wake completed.
                // Since the completion/cancellation of the wake request posts
                // an event which it assumes will complete the request, we must
                // catch these events here and complete the request.
                //
                PowerCompletePendedWakeIrp();
                break;

            case PowerD0:
            case PowerDx:
                //
                // There are some (non WDF) power policy owner implementations
                // which send Dx to Dx or D0 to D0 transitions to the stack.
                //
                // We don't explicitly handle them in the state machine.
                //
                // Instead, we complete the pended irp if are about to drop it
                // on the floor.
                //
                PowerReleasePendingDeviceIrp();
                break;
            }
        }
        else
        {
            //
            // Now enter the new state.
            //
            PowerEnterNewState(newState);
        }
    }
}

VOID
FxPkgPnp::PowerEnterNewState(
    __in WDF_DEVICE_POWER_STATE State
    )
/*++

Routine Description:
    This function looks up the handler for a state and
    then calls it.

Arguments:
    Event - Current PnP event

Return Value:

    NTSTATUS

--*/
{
    CPPOWER_STATE_TABLE    entry;
    WDF_DEVICE_POWER_STATE currentState, newState;
    WDF_DEVICE_POWER_NOTIFICATION_DATA data;
    FxWatchdog watchdog(this);

    currentState = m_Device->GetDevicePowerState();
    newState = State;

    while (newState != WdfDevStatePowerNull)
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNPPOWERSTATES,
            "WDFDEVICE 0x%p !devobj 0x%p entering Power State "
            "%!WDF_DEVICE_POWER_STATE! from %!WDF_DEVICE_POWER_STATE!",
            m_Device->GetHandle(), 
            m_Device->GetDeviceObject(),
            newState, currentState);

        if (m_PowerStateCallbacks != NULL)
        {
            //
            // Callback for leaving the old state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationLeaveState;
            data.Data.LeaveState.CurrentState = currentState;
            data.Data.LeaveState.NewState = newState;

            m_PowerStateCallbacks->Invoke(currentState,
                                          StateNotificationLeaveState,
                                          m_Device->GetHandle(),
                                          &data);
        }

        m_PowerMachine.m_States.History[m_PowerMachine.IncrementHistoryIndex()] =
            (USHORT) newState;

        if (m_PowerStateCallbacks != NULL)
        {
            //
            // Callback for entering the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationEnterState;
            data.Data.EnterState.CurrentState = currentState;
            data.Data.EnterState.NewState = newState;

            m_PowerStateCallbacks->Invoke(newState,
                                          StateNotificationEnterState,
                                          m_Device->GetHandle(),
                                          &data);
        }

        m_Device->SetDevicePowerState(newState);
        currentState = newState;

        entry = GetPowerTableEntry(currentState);

        //
        // And call the state handler, if there is one.
        //
        if (entry->StateFunc != NULL)
        {
            watchdog.StartTimer(currentState);
            newState = entry->StateFunc(this);
            watchdog.CancelTimer(currentState);

            //
            // Validate the return value if FX_STATE_MACHINE_VERIFY is enabled
            //
            VALIDATE_POWER_STATE(currentState, newState);
        }
        else
        {
            newState = WdfDevStatePowerNull;
        }

        if (m_PowerStateCallbacks != NULL)
        {
            //
            // Callback for post processing the new state
            //
            RtlZeroMemory(&data, sizeof(data));

            data.Type = StateNotificationPostProcessState;
            data.Data.PostProcessState.CurrentState = currentState;

            m_PowerStateCallbacks->Invoke(currentState,
                                          StateNotificationPostProcessState,
                                          m_Device->GetHandle(),
                                          &data);
        }
    }
}

VOID
FxPkgPnp::PowerCompletePendedWakeIrp(
    VOID
    )
/*++

Routine Description:
    Completes the wait wake request that was pended by the power state machine.
    It is valid if there is no request to complete (the only time there will be
    a request to complete is when this power state machine is the wait wake owner

Arguments:
    None

Return Value:
    None

  --*/
{
    PLIST_ENTRY ple;
    KIRQL irql;

    if (m_SharedPower.m_WaitWakeOwner == FALSE)
    {
        COVERAGE_TRAP();
        return;
    }

    //
    // Pop an irp off of the list
    //
    m_PowerMachine.m_WaitWakeLock.Acquire(&irql);
    ASSERT(IsListEmpty(&m_PowerMachine.m_WaitWakeIrpToBeProcessedList) == FALSE);
    ple = RemoveHeadList(&m_PowerMachine.m_WaitWakeIrpToBeProcessedList);
    m_PowerMachine.m_WaitWakeLock.Release(irql);

    InitializeListHead(ple);
    
    FxIrp irp(FxIrp::GetIrpFromListEntry(ple));

    CompletePowerRequest(&irp, irp.GetStatus());
}

VOID
FxPkgPnp::PowerProcessEvent(
    __in FxPowerEvent Event,
    __in BOOLEAN ProcessOnDifferentThread
    )
/*++

Routine Description:
    This function implements steps 1-8 of the algorithm described above.

Arguments:
    Event - Current Power event

    ProcessOnDifferentThread - Process the event on a different thread 
        regardless of IRQL. By default this is FALSE as per the declaration.

Return Value:

    NTSTATUS

--*/
{
    NTSTATUS status;
    KIRQL irql;

    //
    // Take the lock, raising to DISPATCH_LEVEL.
    //
    m_PowerMachine.Lock(&irql);

    //
    // If the input Event is any of the events described by PowerSingularEventMask,
    // then check whether it is already queued up. If so, then dont enqueue this
    // Event.
    //
    if (Event & PowerSingularEventMask)
    {
        if ((m_PowerMachine.m_SingularEventsPresent & Event) == 0x00)
        {
            m_PowerMachine.m_SingularEventsPresent |= Event;
        }
        else
        {
            DoTraceLevelMessage(
                GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
                "WDFDEVICE 0x%p !devobj 0x%p current pwr pol state "
                "%!WDF_DEVICE_POWER_STATE! dropping event %!FxPowerEvent! because "
                "the Event is already enqueued.", m_Device->GetHandle(),
                m_Device->GetDeviceObject(), 
                m_Device->GetDevicePowerState(),
                Event);

            m_PowerMachine.Unlock(irql);
            return;
        }
    }

    if (m_PowerMachine.IsFull())
    {
        //
        // The queue is full.  Bail.
        //
        m_PowerMachine.Unlock(irql);

        ASSERT(!"The Power queue is full.  This shouldn't be able to happen.");
        return;
    }

    if (m_PowerMachine.IsClosedLocked())
    {
        DoTraceLevelMessage(
            GetDriverGlobals(), TRACE_LEVEL_INFORMATION, TRACINGPNP,
            "WDFDEVICE 0x%p !devobj 0x%p current pwr pol state "
            "%!WDF_DEVICE_POWER_STATE! dropping event %!FxPowerEvent! because "
            "of a closed queue", m_Device->GetHandle(),
            m_Device->GetDeviceObject(), 
            m_Device->GetDevicePowerState(),
            Event);

        //
        // The queue is closed.  Bail
        //
        m_PowerMachine.Unlock(irql);

        return;
    }

    //
    // Enqueue the event.  Whether the event goes on the front
    // or the end of the queue depends on which event it is.
    //
    if (Event & PowerPriorityEventsMask)
    {
        //
        // Stick it on the front of the queue, making it the next
        // event that will be processed.
        //
        m_PowerMachine.m_Queue.Events[m_PowerMachine.InsertAtHead()] = (USHORT) Event;
    }
    else
    {
        //
        // Stick it on the end of the queue.
        //
        m_PowerMachine.m_Queue.Events[m_PowerMachine.InsertAtTail()] = (USHORT) Event;
    }

    //
    // Drop the lock.
    //
    m_PowerMachine.Unlock(irql);

    //
    // Now, if we are running at PASSIVE_LEVEL, attempt to run the state
    // machine on this thread.  If we can't do that, then queue a work item.
    //

    if (irql == PASSIVE_LEVEL &&
        FALSE == ProcessOnDifferentThread)
    {
        LONGLONG timeout = 0;

        status = m_PowerMachine.m_StateMachineLock.AcquireLock(
            GetDriverGlobals(), &timeout);

        if (FxWaitLockInternal::IsLockAcquired(status))
        {
            FxPostProcessInfo info;
            //
            // We now hold the state machine lock.  So call the function that
            // dispatches the next state.
            //
            PowerProcessEventInner(&info);

            //
            // The pnp state machine should be the only one deleting the object
            //
            ASSERT(info.m_DeleteObject == FALSE);

            m_PowerMachine.m_StateMachineLock.ReleaseLock(GetDriverGlobals());

            info.Evaluate(this);

            return;
        }
    }

    //
    // The tag added above will be released when the work item runs
    //

    // For one reason or another, we couldn't run the state machine on this
    // thread.  So queue a work item to do it.  If m_PnPWorkItemEnqueuing
    // is non-zero, that means that the work item is already being enqueued
    // on another thread.  This is significant, since it means that we can't do
    // anything with the work item on this thread, but it's okay, since the
    // work item will pick up our work and do it.
    //
    m_PowerMachine.QueueToThread();
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerCheckDeviceType(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Check Type state.  Its only task
    is to dispatch to the FDO and PDO logic.

Arguments:
    none

Return Value:

    new power state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerCheckDeviceTypeNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Check Type (NP) state.  Its only task
    is to dispatch to the FDO and PDO logic.

Arguments:
    none

Return Value:

    new power state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerCheckParentState(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Check Parent state.  Its only task
    is to dispatch to the FDO and PDO logic and handle error.

Arguments:
    none

Return Value:

    new power state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerCheckParentStateNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Check Parent (NP) state.  Its only task
    is to dispatch to the FDO and PDO logic.

Arguments:
    none

Return Value:

    new power state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerEnablingWakeAtBus(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function requests the driver to arm the device in a bus generic fashion.

Arguments:
    The package which contains this instance of the state machine

Return Value:
    new power state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerEnablingWakeAtBusNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function requests the driver to arm the device in a bus generic fashion.

Arguments:
    The package which contains this instance of the state machine

Return Value:
    new power state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0NP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    D0 state where we can't cause paging IO and we cannot wake the machine

Arguments:
    This - Instance of this state machine

Return Value:
    new power state machine state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0BusWakeOwner(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the D0 state.  It's job is to figure out whether
    we need to swtich to the D0NP state, and to wait around for an event
    of some kind or another.

Arguments:
    none

Return Value:

    new power state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0BusWakeOwnerNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the D0NP state.  It's job is to figure out whether
    we need to swtich to the D0 state, and to wait around for an event
    of some kind or another.

Arguments:
    none

Return Value:

    new power state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0ArmedForWake(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Device is in D0 and armed for wake.  Complete any pended D0 irp if the power
    policy owner make a D0 to D0 transition.  Transition the NP version of
    this state if we are no longer pageable.

Arguments:
    This - instance of the state machine

Return Value:
    new state machine state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0DisarmingWakeAtBus(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disarms the bus after we have armed it.  The device is still in D0 so it has
    not yet powered down.

Arguments:
    This - This instance of the state machine

Return Value:
    None.

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0DisarmingWakeAtBusNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disarms the bus after we have armed it.  The device is still in D0 so it has
    not yet powered down.

Arguments:
    This - This instance of the state machine

Return Value:
    None.

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0Starting(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function calls D0Entry and the moves to the next state based on the
    result.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerInitialPowerUpFailedDerefParent
    WdfDevStatePowerInitialPowerUpFailedPowerDown
    WdfDevStatePowerD0StartingConnectInterrupt

--*/
{
    NTSTATUS    status;

    //
    // Call the driver to tell it to put the hardware into the working
    // state.
    //
    // m_DevicePowerState is the "old" state because we update it after the
    // D0Entry callback.
    //
    status = This->m_DeviceD0Entry.Invoke(
        This->m_Device->GetHandle(),
        (WDF_POWER_DEVICE_STATE) This->m_DevicePowerState);

    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "PowerD0Starting WDFDEVICE 0x%p !devobj 0x%p, "
            "old state %!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(), 
            This->m_Device->GetDeviceObject(),
            This->m_DevicePowerState, status);

        return WdfDevStatePowerInitialPowerUpFailedDerefParent;
    }

    return WdfDevStatePowerD0StartingConnectInterrupt;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0StartingConnectInterrupt(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Continues bringing the device into D0 from the D3 final state.  This routine
    connects and enables the interrupts.  If successful it will open up the power
    managed i/o queues.

Arguments:
    This - instance of the state machine

Return Value:
    new state machine state

  --*/
{
    NTSTATUS status;

    //
    // Connect the interrupt and enable it
    //
    status = This->NotifyResourceObjectsD0(NotifyResourcesNoFlags);
    if (!NT_SUCCESS(status))
    {
        //
        // NotifyResourceObjectsD0 has already logged the error, no need to
        // repeat any error messsages here
        //
        return WdfDevStatePowerInitialConnectInterruptFailed;
    }

    status = This->m_DeviceD0EntryPostInterruptsEnabled.Invoke(
        This->m_Device->GetHandle(),
        (WDF_POWER_DEVICE_STATE) This->m_DevicePowerState);

    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            This->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "EvtDeviceD0EntryPostInterruptsEnabed WDFDEVICE 0x%p !devobj 0x%p, "
            "old state %!WDF_POWER_DEVICE_STATE! failed, %!STATUS!",
            This->m_Device->GetHandle(), 
            This->m_Device->GetDeviceObject(),
            This->m_DevicePowerState, status);
        return WdfDevStatePowerInitialConnectInterruptFailed;
    }

    //
    // Last, figure out which state to drop into.  This is the juncture
    // where we figure out if we're doing power in a pageable fashion.
    //
    return WdfDevStatePowerD0StartingDmaEnable;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0StartingDmaEnable(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is movig to D0 for the first time.  Enable any DMA enablers
    attached to the device.

Arguments:
    This - instance of the state machine

Return Value:
    new machine state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerD0StartingStartSelfManagedIo(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is entering D0 from the final Dx state (either start or restart
    perhaps).  Send a start event to self managed io and then release

Arguments:
    This - instance of the state machine

Return Value:
    new state machine state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDecideD0State(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Decide which D0 state we should transition to given the wait wake ownership
    of this device and if DO_POWER_PAGABLE is set or not.

Arguments:
    This - instance of the state machine

Return Value:
    new power state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoD3Stopped(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the D3 Stopped state.

Arguments:
    none

Return Value:

    new power state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStartingCheckDeviceType(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is implicitly powering up from the created or stopped state.
    Determine if this is a PDO or not to determine if we must bring the parent
    back into D0.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerStartingChild or WdfDevStatePowerD0Starting

  --*/
{
    if (This->m_Device->IsPdo())
    {
        return WdfDevStatePowerStartingChild;
    }
    else
    {
        return WdfDevStatePowerD0Starting;
    }
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStartingChild(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Get the parent into a D0 state

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerNull or WdfDevStatePowerD0Starting

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxDisablingWakeAtBus(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Disable Wake at Bus state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerWaking

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxDisablingWakeAtBusNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Disable Wake at Bus state.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerWakingNP

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDNotZeroIoStopped(
    __inout FxPkgPnp*   This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDNotZeroIoStoppedNP(
    __inout FxPkgPnp*   This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxNPFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Going to Dx in the NP path failed.  Disconnect all the interrupts.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerReportPowerDownFailed

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxArmedForWake(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Dx state when we are armed for wake.

Arguments:
    This - The instance of the state machine

Return Value:

    new power state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxIoStoppedArmedForWake(
    __inout FxPkgPnp*   This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxIoStoppedArmedForWakeNP(
    __inout FxPkgPnp*   This
    )
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerCheckParentStateArmedForWake(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The PDO was armed for wake in Dx and needs to be disarmed at the bus level.
    The child can only be disarmed while the parent is in D0, so check the state
    of the parent.  If in D0, move directly to the disarm state, otherwise move
    into a wait state and disarm once the parent is in D0.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/

{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerCheckParentStateArmedForWakeNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Same as PowerCheckParentStateArmedForWake, but we are in the NP path

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStartSelfManagedIo(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Start Self-Managed I/O state.  It tells the
    driver that it can resume operations that were not interlocked with
    the PnP and Power state machines.

Arguments:
    This - The instance of the state machine

Return Value:

    new power state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStartSelfManagedIoNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Start Self-Managed I/O state.  It tells the
    driver that it can resume operations that were not interlocked with
    the PnP and Power state machines.

Arguments:
    This - The instance of the state machine

Return Value:

    new power state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStartSelfManagedIoFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Starting self managed io back up from an explicit Dx to D0 transition failed.
    Hold the power managed queues and proceed down the power up failure path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerWakingDmaEnableFailed

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStartSelfManagedIoFailedNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Starting self managed io back up from an explicit Dx to D0 transition failed.
    Hold the power managed queues and proceed down the power up failure path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerWakingDmaEnableFailedNP

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}


WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakePending(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    State that indicates a successful wake from Dx.  Primarily exists so that
    the driver writer can register to know about the entrance into this state.
    It also completes the pended wait wake request (which posts the appopriate
    events to the power policy state machine if it's listening).

Arguments:
    This - The instance of the state machine

Return Value:
    return WdfDevStatePowerNull

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakePendingNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    State that indicates a successful wake from Dx.  Primarily exists so that
    the driver writer can register to know about the entrance into this state.
    It also completes the pended wait wake request (which posts the appopriate
    events to the power policy state machine if it's listening).

Arguments:
    This - The instance of the state machine

Return Value:
    return WdfDevStatePowerNull

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWaking(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the Waking state.  Its job is to call into the
    driver to tell it to restore its hardware, and then to connect interrupts
    and release the queues.

Arguments:
    This - The instance of the state machine

Return Value:

    new power state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}


WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    This function implements the WakingNP state.  Its job is to call into the
    driver to tell it to restore its hardware and release the queues.

Arguments:
    This - The instance of the state machine

Return Value:

    new power state

--*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingConnectInterrupt(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is returning to D0 in the power pageable path.  Connect and
    enable the interrupts.  If that succeeds, scan for children and then
    open the power managed I/O queues.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingConnectInterruptNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is returning to D0 in the power pageable path.
    Enable the interrupts.  If that succeeds, scan for children and then
    open the power managed I/O queues.

Arguments:
    This - instance of the state machine

Return Value:
    new state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingConnectInterruptFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Connecting or enabling the interrupts failed.  Disable and disconnect any
    interrupts which have been connected and maybe enabled.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerReportPowerUpFailed

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingConnectInterruptFailedNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Enabling the interrupts failed.  Disable and disconnect any
    interrupts which have been connected and maybe enabled.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerReportPowerUpFailed

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingDmaEnable(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is returning to D0 from Dx.  Power up all DMA enablers and scan
    for children.

Arguments:
    This - instance of the state machine

Return Value:
    new machine state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingDmaEnableNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The device is returning to D0 from Dx in the NP path.  Power up all DMA
    enablers and scan for children.

Arguments:
    This - instance of the state machine

Return Value:
    new machine state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingDmaEnableFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Powering up a DMA enabler failed.  Power down all DMA enablers and progress
    down the failed power up path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerWakingConnectInterruptFailed

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerWakingDmaEnableFailedNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Powering up a DMA enabler failed in the NP path.  Power down all DMA
    enablers and progress down the failed power up path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerWakingConnectInterruptFailedNP

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerReportPowerUpFailedDerefParent(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Power up failed.  Release the reference on the parent that was taken at the
    start of power up.

Arguments:
    This - instance of the state machine.

Return Value:
    WdfDevStatePowerInitialPowerUpFailed

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerReportPowerUpFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Posts PowerUpFailed event to the PnP state machine and then waits for death.
    The pended Power IRP is also completed.

Arguments:
    This - The instance of the state machine

Return Value:
    new power state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerPowerFailedPowerDown(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We failed to power up/down properly and now the power policy state machine wants
    to power down the device.  Since power policy and pnp rely on power posting
    power down events to make forward progress, we must do that here

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerStopped

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerReportPowerDownFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Posts PowerDownFailed event to the PnP state machine and then waits for death.
    The pended Power IRP is also completed.

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerNull

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerInitialConnectInterruptFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    When bringing the device out of the D3 final state, connecting or enabling
    the interrupts failed.  Disconnect and disable any interrupts which are
    connected and possibly  enabled.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerInitialPowerUpFailedDerefParent

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}


WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerInitialDmaEnableFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Initial power up of the device failed while enabling DMA.  Disable any
    started DMA enablers and proceed down the initial power up failure path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerInitialConnectInterruptFailed

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerInitialSelfManagedIoFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    The self managed io start failed when bringing the device out of Dx with an
    implicit D0 transition.  Hold the power queues that were previous open can
    continue to cleanup

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerInitialDmaEnableFailed

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerInitialPowerUpFailedDerefParent(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Dereferences the parent's power ref count we took during start up

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerInitialPowerUpFailed

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerInitialPowerUpFailedPowerDown(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Transitions the client driver into Dx

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerInitialPowerUpFailedDerefParent

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerUpFailedPowerDown(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Notifies Cx of failure to enter D0 state

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerUpFailedDerefParent

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerInitialPowerUpFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Posts PowerUpFailed event to the PnP state machine and then waits for death.

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerStopped

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxStoppedDisarmWake(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disarms the device from wake because the device is being stopped/removed
    while in Dx.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerDxStopped

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxStoppedDisarmWakeNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disarms the device from wake because the device is being stopped/removed
    while in Dx.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerGotoDxStoppedDisableInterruptNP

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxStoppedDisableInterruptNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Device is in Dx and was implicitly powered down. Disable the interrupt since
    the interrupt was not disconnected in the NP power down path.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerGotoDxStopped

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoDxStopped(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Device is in Dx and being removed/stopped.  Inform the other state machines
    that the device is powered down so that they may continue.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerStopped

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerGotoStopped(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We are in the Dx state and the power policy owner sent an explicit D0 irp
    to the stack.  Transition to the stopped state which will transition to the
    D0 state when receiving an implicit D0.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerStopped

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerStoppedCompleteDx(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We were in the Stopped state and the power policy owner sent an explicit
    Dx request to the stack.  We will move to the DxStopped state where we will
    transition to a Dx state when an implicit D0 is received.

Arguments:
    This - instance of the state machine

Return Value:
    WdfDevStatePowerDxStopped

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxStoppedDecideDxState(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We are implicitly powering up the stack while in the Dx state.  Decide
    which Dx "holding" state to transition to.

Arguments:
    This - instance of the state machine

Return Value:
    new power state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxStoppedArmForWake(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We are implicitly powering the stack back up while the device is in Dx
    and there is a wait wake request presnt on this device.  Enable wake at
    the bus level and then move to the appropriate state based on the enabling
    status.

Arguments:
    This - instance of the state machine

Return Value:
    new power state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerDxStoppedArmForWakeNP(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    We are implicitly powering the stack back up while the device is in Dx
    and there is a wait wake request presnt on this device.  Enable wake at
    the bus level and then move to the appropriate state based on the enabling
    status.

Arguments:
    This - instance of the state machine

Return Value:
    new power state

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerFinalPowerDownFailed(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Posts PowerDownFailed event to the PnP state machine and then waits for death.

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerStopped

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}

WDF_DEVICE_POWER_STATE
FxPkgPnp::PowerUpFailedDerefParent(
    __inout FxPkgPnp*   This
    )
/*++

Routine Description:
    Disconnects interrupts

Arguments:
    This - The instance of the state machine

Return Value:
    WdfDevStatePowerReportPowerUpFailed

  --*/
{
    WDFNOTIMPLEMENTED();
    return WdfDevStatePowerInvalid;
}