/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    PnpStateMachineUm.cpp

Abstract:

    This module implements the PnP state machine for the driver framework.
    This code was split out from FxPkgPnp.cpp.

Author:





Environment:

    User mode only

Revision History:

--*/

#include "../pnppriv.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "PnpStateMachineUM.tmh"
#endif
}

BOOLEAN
FxPkgPnp::PnpCheckAndIncrementRestartCount(
    VOID
    )
/*++

Routine Description:
    This is a mode-dependent wrapper for PnpIncrementRestartCountLogic,
    which determines if this device should ask the bus driver to
    reenumerate the device. Please refer to PnpIncrementRestartCountLogic's
    comment block for more information.

Arguments:
    None

Return Value:
    TRUE if a restart should be requested.

--*/
{
    HRESULT hr;
    FxAutoRegKey restart;
    FxDevice* device;
    IWudfDeviceStack* deviceStack;
    UMINT::WDF_PROPERTY_STORE_ROOT propertyStore;
    UMINT::WDF_PROPERTY_STORE_DISPOSITION disposition;

    device = GetDevice();
    deviceStack = device->GetDeviceStack();

    propertyStore.LengthCb = sizeof(UMINT::WDF_PROPERTY_STORE_ROOT);
    propertyStore.RootClass = UMINT::WdfPropertyStoreRootClassHardwareKey;
    propertyStore.Qualifier.HardwareKey.ServiceName = L"WudfDiagnostics";

    hr = deviceStack->CreateRegistryEntry(&propertyStore,
                                          UMINT::WdfPropertyStoreCreateVolatile,
                                          KEY_QUERY_VALUE | KEY_SET_VALUE,
                                          L"Restart",
                                          (HKEY*)&restart.m_Key,
                                          &disposition);
    if (FAILED(hr)) {
        return FALSE;
    }

    return PnpIncrementRestartCountLogic(restart.m_Key,
                                         disposition == UMINT::CreatedNewStore);
}

BOOLEAN
FxPkgPnp::ShouldProcessPnpEventOnDifferentThread(
    __in KIRQL CurrentIrql,
    __in BOOLEAN CallerSpecifiedProcessingOnDifferentThread
    )
/*++
Routine Description:

    This function returns whether the PnP state machine should process the
    current event on the same thread or on a different one.

    This function has been added to work around a bug in the state machines.
    The idle state machine always calls PnpProcessEvent with the idle state
    machine lock held. Some events sent by the idle state machine can cause the
    Pnp state machine to invoke FxPowerIdleMachine::IoDecrement().
    FxPowerIdleMachine::IoDecrement() will try to acquire the idle state
    machine lock, which is already being held, so it will result in a recursive
    acquire of the idle state machine lock.

    The above bug only affects UMDF, but not KMDF. In KMDF, the idle state
    machine lock is a spinlock. When PnpProcessEvent is called, it is called
    with the spinlock held and hence at dispatch level. Note that if called at
    a non-passive IRQL, PnpProcessEvent will always queue a work item to
    process the event at passive IRQL later. Queuing a work item forces
    processing to happen on a different thread and hence we don't attempt to
    recursively acquire the spinlock. On the other hand, with UMDF we are
    always at passive IRQL and hence we process the event on the same thread
    and run into the recursive acquire problem.








Arguments:

    CurrentIrql - The current IRQL

    CallerSpecifiedProcessingOnDifferentThread - Whether or not caller of
        PnpProcessEvent specified that the event be processed on a different
        thread.

Returns:
    TRUE if the PnP state machine should process the event on a different
       thread.

    FALSE if the PnP state machine should process the event on the same thread

--*/
{
    //
    // For UMDF, we ignore the IRQL and just do what the caller of
    // PnpProcessEvent wants.
    //
    UNREFERENCED_PARAMETER(CurrentIrql);

    return CallerSpecifiedProcessingOnDifferentThread;
}

_Must_inspect_result_
NTSTATUS
FxPkgPnp::CreatePowerThreadIfNeeded(
    VOID
    )
/*++
Routine description:
    If needed, creates a thread for processing power IRPs

Arguments:
    None

Return value:
    An NTSTATUS code indicating success or failure of this function
--*/
{
    //
    // For UMDF, we never need a power thread, so just return success.
    //
    return STATUS_SUCCESS;
}

NTSTATUS
FxPkgPnp::PnpPrepareHardwareInternal(
    VOID
    )
{
    //
    // Update maximum interrupt thread count now that we know how many
    // total interrupts we have.
    //
    return GetDevice()->UpdateInterruptThreadpoolLimits();
}

