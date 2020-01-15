#ifndef _FXPKGPNP_H_
#define _FXPKGPNP_H_

#include "common/fxpackage.h"
#include "common/mxevent.h"
#include "common/fxpnpstatemachine.h"
#include "common/fxpnpcallbacks.h"
#include "common/fxpowerstatemachine.h"
#include "common/fxpowerpolicystatemachine.h"
#include "common/fxsystemthread.h"

//
// Bit-flags for tracking which callback is currently executing.
//
enum FxDeviceCallbackFlags {
    // Prepare hardware callback is running.
    FXDEVICE_CALLBACK_IN_PREPARE_HARDWARE = 0x01,
};

typedef
VOID
(*PFN_POWER_THREAD_ENQUEUE)(
     __in PVOID Context,
     __in PWORK_QUEUE_ITEM WorkItem
     );
     
//
// workaround overloaded definition (rpc generated headers all define INTERFACE
// to match the class name).
//
#undef INTERFACE

typedef struct _POWER_THREAD_INTERFACE {
    //
    // generic interface header
    //
    INTERFACE Interface;

    PFN_POWER_THREAD_ENQUEUE PowerThreadEnqueue;

} POWER_THREAD_INTERFACE, *PPOWER_THREAD_INTERFACE;

class FxPkgPnp : public FxPackage {

protected:

    FxPkgPnp(
        __in PFX_DRIVER_GLOBALS FxDriverGlobals,
        __in CfxDevice* Device,
        __in WDFTYPE Type
        );

    ~FxPkgPnp();

    MdIrp
    ClearPendingPnpIrp(
        VOID
        )
    {
        MdIrp irp;
    
        irp = m_PendingPnPIrp;
        m_PendingPnPIrp = NULL;
    
        return irp;
    }

    VOID
    PnpFinishProcessingIrp(
        __in BOOLEAN IrpMustBePresent = TRUE
        );

    VOID
    PnpEnterNewState(
        __in WDF_DEVICE_PNP_STATE State
        );

    BOOLEAN
    IsPresentPendingPnpIrp(
        VOID
        )
    {
        return (m_PendingPnPIrp != NULL) ? TRUE : FALSE;
    }

    MdIrp
    GetPendingPnpIrp(
        VOID
        )
    {
        return m_PendingPnPIrp;
    }

    NTSTATUS
    CompletePnpRequest(
        __inout FxIrp* Irp,
        __in    NTSTATUS Status
        );

public:

    VOID
    CleanupDeviceFromFailedCreate(
        __in MxEvent * WaitEvent
        );

    VOID
    CleanupStateMachines(
        __in BOOLEAN ClenaupPnp
        );

    VOID
    PnpProcessEvent(
        __in FxPnpEvent Event,
        __in BOOLEAN ProcessEventOnDifferentThread = FALSE
        );

    BOOLEAN
    ShouldProcessPnpEventOnDifferentThread(
        __in KIRQL CurrentIrql,
        __in BOOLEAN CallerSpecifiedProcessingOnDifferentThread
        );

    VOID
    SignalDeviceRemovedEvent(
        VOID
        )
    {
        m_DeviceRemoveProcessed->Set();
    }

    virtual
    NTSTATUS
    FireAndForgetIrp(
        FxIrp* Irp
        ) =0;

    VOID
    ProcessDelayedDeletion(
        VOID
        );

    VOID
    DeleteDevice(
        VOID
        );

    BOOLEAN
    IsPowerPolicyOwner(
        VOID
        )
    {
        return m_PowerPolicyMachine.m_Owner != NULL ? TRUE : FALSE;
    }

private:

    //
    // The current pnp state changing irp in the stack that we have pended to
    // process in the pnp state machine
    //
    MdIrp m_PendingPnPIrp;

        //
    // Non NULL when this device is exporting the power thread interface.  This
    // would be the lowest device in the stack that supports this interface.
    //
    FxSystemThread* m_PowerThread;

    LONG m_PowerThreadInterfaceReferenceCount;

    FxCREvent* m_PowerThreadEvent;


    FxPnpStateCallback* m_PnpStateCallbacks;

    static const PNP_STATE_TABLE          m_WdfPnpStates[];



private:

    VOID
    ReleasePowerThread(
        VOID
        );

    virtual
    VOID
    ReleaseReenumerationInterface(
        VOID
        ) =0;

public:

    //
    // All 3 state machine engines
    //
    FxPnpMachine m_PnpMachine;
    FxPowerMachine m_PowerMachine;
    FxPowerPolicyMachine m_PowerPolicyMachine;

protected:

    POWER_THREAD_INTERFACE m_PowerThreadInterface;

    //
    // if FALSE, there is no power thread available on the devnode currently
    // and a work item should be enqueued.  if TRUE, there is a power thread
    // and the callback should be enqueued to it.
    //
    BOOLEAN m_HasPowerThread;

    //
    // Event that is set when processing a remove device is complete
    //
    MxEvent* m_DeviceRemoveProcessed;

    //
    // Set the event which indicates that the pnp state machine is done outside
    // of the state machine so that we can drain any remaining state machine
    // events in the removing thread before signaling the event.
    //
    BYTE m_SetDeviceRemoveProcessed;

    static
    VOID
    _PnpProcessEventInner(
        __inout FxPkgPnp* This,
        __inout FxPostProcessInfo* Info,
        __in PVOID WorkerContext
        );

    VOID
    PnpProcessEventInner(
        __inout FxPostProcessInfo* Info
        );

    static
    CPPNP_STATE_TABLE
    GetPnpTableEntry(
        __in WDF_DEVICE_PNP_STATE State
        )
    {
        return &m_WdfPnpStates[WdfDevStateNormalize(State) - WdfDevStatePnpObjectCreated];
    }

};

__inline
VOID
FxPostProcessInfo::Evaluate(
    __inout FxPkgPnp* PkgPnp
    )
{
    if (m_SetRemovedEvent)
    {
        ASSERT(m_DeleteObject == FALSE && m_Event == NULL && m_FireAndForgetIrp == NULL);
        PkgPnp->SignalDeviceRemovedEvent();
        return;
    }

    //
    // Process any irp that should be sent down the stack/forgotten.
    //
    if (m_FireAndForgetIrp != NULL)
    {
        FxIrp irp(m_FireAndForgetIrp);

        m_FireAndForgetIrp = NULL;
        (void) PkgPnp->FireAndForgetIrp(&irp);
    }

    if (m_DeleteObject)
    {
        PkgPnp->ProcessDelayedDeletion();
    }

    if (m_Event != NULL)
    {
        m_Event->Set();
    }
}

#endif //_FXPKGPNP_H_