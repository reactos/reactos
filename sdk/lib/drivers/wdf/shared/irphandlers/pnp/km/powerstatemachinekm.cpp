/*++
Copyright (c) Microsoft. All rights reserved.

Module Name:

    PowerStateMachineKm.cpp

Abstract:

    This module implements the Power state machine for the driver framework.
    This code was split out from FxPkgPnp.cpp.

Author:




Environment:

    Kernel mode only

Revision History:

--*/

#include "../pnppriv.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "PowerStateMachineKm.tmh"
#endif
}

_Must_inspect_result_
BOOLEAN
FxPkgPnp::PowerDmaPowerUp(
    VOID
    )
/*++

Routine Description:
    Calls FxDmaEnabler::PowerUp on all registered FxDmaEnabler objects.  As soon
    as a PowerUp call fails, we stop iterating over the list.

Arguments:
    None

Return Value:
    TRUE if PowerUp succeeded on all enablers, FALSE otherwise

  --*/

{
    // FxTransactionedEntry* ple;
    // NTSTATUS status;
    BOOLEAN result;

    result = TRUE;

    //
    // Power up each dma enabler
    //
    // if (m_DmaEnablerList != NULL) {
    //     m_DmaEnablerList->LockForEnum(GetDriverGlobals());

    //     ple = NULL;
    //     while ((ple = m_DmaEnablerList->GetNextEntry(ple)) != NULL) {
    //         status = ((FxDmaEnabler*) ple->GetTransactionedObject())->PowerUp();

    //         if (!NT_SUCCESS(status)) {
    //             result = FALSE;
    //             break;
    //         }
    //     }

    //     m_DmaEnablerList->UnlockFromEnum(GetDriverGlobals());
    // }

    return result;
}

BOOLEAN
FxPkgPnp::PowerDmaPowerDown(
    VOID
    )
/*++

Routine Description:
    Calls FxDmaEnabler::PowerDown on all registered FxDmaEnabler objects.  All
    errors are accumulated, all enablers will be PowerDown'ed.

Arguments:
    None

Return Value:
    TRUE if PowerDown succeeded on all enablers, FALSE otherwise

  --*/
{
    // FxTransactionedEntry* ple;
    // NTSTATUS status;
    BOOLEAN result;

    result = TRUE;

    //
    // Power up each dma enabler
    //
    // if (m_DmaEnablerList != NULL) {
    //     m_DmaEnablerList->LockForEnum(GetDriverGlobals());

    //     ple = NULL;
    //     while ((ple = m_DmaEnablerList->GetNextEntry(ple)) != NULL) {
    //         status = ((FxDmaEnabler*) ple->GetTransactionedObject())->PowerDown();

    //         if (!NT_SUCCESS(status)) {
    //             //
    //             // We do not break out of the loop on power down failure.  We will
    //             // continue to power down each channel regardless of the previous
    //             // channel's power down status.
    //             //
    //             result = FALSE;
    //         }
    //     }

    //     m_DmaEnablerList->UnlockFromEnum(GetDriverGlobals());
    // }

    return result;
}

VOID
FxPkgPnp::_PowerSetSystemWakeSource(
    __in FxIrp* Irp
    )
/*++

Routine Description:
    Set source of wake if OS supports this.

Arguments:
    Irp

Return Value:
    None

  --*/
{
    PoSetSystemWake(Irp->GetIrp());
}

