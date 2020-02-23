#include "common/fxselfmanagediostatemachine.h"
#include "common/fxpkgpnp.h"
#include "common/fxdevice.h"



FxSelfManagedIoMachine::FxSelfManagedIoMachine(
    _In_ FxPkgPnp* PkgPnp
    )
{
    m_PkgPnp = PkgPnp;

    m_EventHistoryIndex = 0;
    m_StateHistoryIndex = 0;

    m_CurrentState = FxSelfManagedIoCreated;

    RtlZeroMemory(&m_Events, sizeof(m_Events));
    RtlZeroMemory(&m_States, sizeof(m_States));

    //
    // Make sure we can fit the state into a byte
    //
    ASSERT(FxSelfManagedIoMax <= 0xFF);
}

NTSTATUS
FxSelfManagedIoMachine::_CreateAndInit(
    _Inout_ FxSelfManagedIoMachine** SelfManagedIoMachine,
    _In_ FxPkgPnp* PkgPnp
    )
{
    NTSTATUS status;
    FxSelfManagedIoMachine * selfManagedIoMachine;

    *SelfManagedIoMachine = NULL;
        
    selfManagedIoMachine = new (PkgPnp->GetDriverGlobals()) FxSelfManagedIoMachine(PkgPnp);

    if (selfManagedIoMachine == NULL)
    {
        DoTraceLevelMessage(
            PkgPnp->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Self managed I/O state machine allocation failed for "
            "WDFDEVICE 0x%p",
            PkgPnp->GetDevice()->GetHandle());
        
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    status = selfManagedIoMachine->m_StateMachineLock.Initialize();
    if (!NT_SUCCESS(status))
    {
        DoTraceLevelMessage(
            PkgPnp->GetDriverGlobals(), TRACE_LEVEL_ERROR, TRACINGPNP,
            "Self managed I/O state machine lock initialization failed for "
            "WDFDEVICE 0x%p, %!STATUS!",
            PkgPnp->GetDevice()->GetHandle(),
            status);

        delete selfManagedIoMachine;        

        return status;
    }

    *SelfManagedIoMachine = selfManagedIoMachine;
    
    return status;
}

VOID
FxSelfManagedIoMachine::InitializeMachine(
    _In_ PWDF_PNPPOWER_EVENT_CALLBACKS Callbacks
    )
/*++

Routine Description:
    Sets all the function event callbacks.

Arguments:
    Callbacks - list of callbacks to set

Return Value:
    None

  --*/
{
    m_DeviceSelfManagedIoCleanup.Initialize(m_PkgPnp, 
        Callbacks->EvtDeviceSelfManagedIoCleanup);
    m_DeviceSelfManagedIoFlush.Initialize(m_PkgPnp, 
        Callbacks->EvtDeviceSelfManagedIoFlush);
    m_DeviceSelfManagedIoInit.Initialize(m_PkgPnp, 
        Callbacks->EvtDeviceSelfManagedIoInit);
    m_DeviceSelfManagedIoSuspend.Initialize(m_PkgPnp, 
        Callbacks->EvtDeviceSelfManagedIoSuspend);
    m_DeviceSelfManagedIoRestart.Initialize(m_PkgPnp, 
        Callbacks->EvtDeviceSelfManagedIoRestart);
}
