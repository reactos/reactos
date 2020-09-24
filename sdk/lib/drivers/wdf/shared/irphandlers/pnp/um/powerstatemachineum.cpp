/*++
Copyright (c) Microsoft. All rights reserved.

Module Name:

    PowerStateMachineUm.cpp

Abstract:

    This module implements the Power state machine for the driver framework.
    This code was split out from FxPkgPnp.cpp.

Author:





Environment:

    User mode only

Revision History:

--*/

#include "..\pnppriv.hpp"

extern "C" {
#if defined(EVENT_TRACING)
#include "PowerStateMachineUm.tmh"
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
    return TRUE;
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
    return TRUE;
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
    UNREFERENCED_PARAMETER(Irp);




    DO_NOTHING();
}

