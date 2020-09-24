/*++

Copyright (c) Microsoft. All rights reserved.

Module Name:

    PnpStateMachine.cpp

Abstract:

    This module implements the PnP state machine for the driver framework.
    This code was split out from FxPkgPnp.cpp.

Author:




Environment:

    Kernel mode only

Revision History:

--*/

#include "..\pnppriv.hpp"
#include <wdmguid.h>

#include<ntstrsafe.h>

extern "C" {
#if defined(EVENT_TRACING)
#include "PnpStateMachineKM.tmh"
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
    NTSTATUS status;
    FxAutoRegKey settings, restart;
    ULONG disposition = 0;

    DECLARE_CONST_UNICODE_STRING(keyNameRestart, L"Restart");

    status = m_Device->OpenSettingsKey(&settings.m_Key);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    //
    // We ask for a volatile key so that upon reboot, the count is purged and we
    // start a new fresh restart count.
    //
    status = FxRegKey::_Create(settings.m_Key,
                               &keyNameRestart,
                               &restart.m_Key,
                               KEY_ALL_ACCESS,
                               REG_OPTION_VOLATILE,
                               &disposition);
    if (!NT_SUCCESS(status)) {
        return FALSE;
    }

    return PnpIncrementRestartCountLogic(restart.m_Key,
                                         disposition == REG_CREATED_NEW_KEY);
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






























Arguemnts:

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
    // For KMDF, we ignore what the caller of PnpProcessEvent specified (which
    // should always be FALSE, BTW) and base our decision on the current IRQL.
    // If we are running at PASSIVE_LEVEL, we process on the same thread else
    // we queue a work item.
    //
    UNREFERENCED_PARAMETER(CallerSpecifiedProcessingOnDifferentThread);

    ASSERT(FALSE == CallerSpecifiedProcessingOnDifferentThread);

    return (CurrentIrql == PASSIVE_LEVEL) ? FALSE : TRUE;
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
    NTSTATUS status = STATUS_SUCCESS;
    MxDeviceObject pTopOfStack;












    pTopOfStack.SetObject(
        m_Device->GetAttachedDeviceReference());

    ASSERT(pTopOfStack.GetObject() != NULL);

    if (pTopOfStack.GetObject() != NULL) {
        //
        // If the top of the stack is not power pageable, the stack needs a power
        // thread.  Query for it if we are not a PDO (and create it if the lower
        // stack does not support it), and create it if we are a PDO.
        //
        // Some stacks send a usage notification when processing a start device, so
        // a notification could have already traveled through the stack by the time
        // the start irp has completed back up to this device.
        //
        if ((pTopOfStack.GetFlags() & DO_POWER_PAGABLE) == 0 &&
            HasPowerThread() == FALSE) {
            status = QueryForPowerThread();

            if (!NT_SUCCESS(status)) {
                SetInternalFailure();
                SetPendingPnpIrpStatus(status);
            }
        }

        pTopOfStack.DereferenceObject();
        pTopOfStack.SetObject(NULL);
    }

    return status;
}

NTSTATUS
FxPkgPnp::PnpPrepareHardwareInternal(
    VOID
    )
/*++
Routine description:
    This is mode-specific routine for Prepare hardware

Arguments:
    None

Return value:
    none.
--*/
{
    //
    // nothing to do for KMDF.
    //
    return STATUS_SUCCESS;
}

