/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxVerifierUm.cpp

Abstract:

    Verifier code specific to UMDF

Environment:

    user mode

Revision History:



--*/

#ifndef _FXVERIFIERUM_H_
#define _FXVERIFIERUM_H_

__inline
VOID
FxVerifierCheckNxPoolType(
    _In_ PFX_DRIVER_GLOBALS FxDriverGlobals,
    _In_ POOL_TYPE PoolType,
    _In_ ULONG PoolTag
    )

/*++

Routine Description:

    This function is an empty stub for UMDF. We don't perform "no execute"
    pool type checks for UMDF client drivers.

Arguments:

    FxDriverGlobals - Supplies a pointer to the WDF driver object
        globals.

    PoolType - Supplies the pool type.

    PoolTag - Supplies an optional pool tag.

Return Value:

    None.

--*/

{
    UNREFERENCED_PARAMETER(FxDriverGlobals);
    UNREFERENCED_PARAMETER(PoolType);
    UNREFERENCED_PARAMETER(PoolTag);
}

#endif	//_FXVERIFIERUM_H_
