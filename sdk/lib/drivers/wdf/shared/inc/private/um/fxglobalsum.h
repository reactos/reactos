/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxGlobalsUm.h

Abstract:

    This module contains user-mode specific globals definitions
    for the frameworks.

    For common definitions common between km and um please see
    FxGlobals.h

Author:

Environment:

    kernel mode only

Revision History:


--*/
#ifdef __cplusplus
extern "C" {
#endif

#include "FxGlobals.h"

extern IUMDFPlatform *g_IUMDFPlatform;
extern IWudfHost2 *g_IWudfHost2;

_Must_inspect_result_
__inline
BOOLEAN
FxIsProcessorGroupSupported(
    VOID
    )
{
    //
    // UMDF 2.0 is targeted for platforms that support processor groups.
    //
    return TRUE;
}


__inline
VOID
FX_TRACK_DRIVER(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    UNREFERENCED_PARAMETER(FxDriverGlobals);
    //
    // Not yet supported for UMDF
    //
}

_Must_inspect_result_
__inline
PVOID
FxAllocateFromNPagedLookasideListNoTracking (
    __in PNPAGED_LOOKASIDE_LIST Lookaside
    )
{
    UNREFERENCED_PARAMETER(Lookaside);
    ASSERTMSG("Not implemented for UMDF!\n", FALSE);
    return NULL;
}

__inline
PVOID
FxAllocateFromNPagedLookasideList (
    _In_ PNPAGED_LOOKASIDE_LIST Lookaside,
    _In_opt_ size_t ElementSize = 0
    )
{
    UNREFERENCED_PARAMETER(Lookaside);

    //
    // UMDF doesn't yet use a look-aside list, so just alloc memory from pool.
    //
    return MxMemory::MxAllocatePoolWithTag(NonPagedPool, // not used
                                          ElementSize,
                                          0             // not used
                                          );
}

__inline
PVOID
FxAllocateFromPagedLookasideList (
    __in PPAGED_LOOKASIDE_LIST Lookaside
    )
{
    UNREFERENCED_PARAMETER(Lookaside);
    ASSERTMSG("Not implemented for UMDF!\n", FALSE);
    return NULL;
}

__inline
VOID
FxFreeToNPagedLookasideListNoTracking (
    __in PNPAGED_LOOKASIDE_LIST Lookaside,
    __in PVOID Entry
    )
{
    UNREFERENCED_PARAMETER(Lookaside);
    UNREFERENCED_PARAMETER(Entry);
    ASSERTMSG("Not implemented for UMDF!\n", FALSE);
}

__inline
VOID
FxFreeToPagedLookasideList (
    __in PPAGED_LOOKASIDE_LIST Lookaside,
    __in PVOID Entry
    )
{
    UNREFERENCED_PARAMETER(Lookaside);
    UNREFERENCED_PARAMETER(Entry);
    ASSERTMSG("Not implemented for UMDF!\n", FALSE);
}

__inline
VOID
FxFreeToNPagedLookasideList (
    __in PNPAGED_LOOKASIDE_LIST Lookaside,
    __in PVOID Entry
    )
{
    UNREFERENCED_PARAMETER(Lookaside);

    MxMemory::MxFreePool(Entry);
}

__inline
BOOL
IsCurrentThreadImpersonated( )
{
    return g_IWudfHost2->IsCurrentThreadImpersonated();
}

__inline
PWDF_ACTIVATION_FRAME *
GetActivationList(
    VOID
    )
{
    return g_IUMDFPlatform->GetActivationListHead();
}

//
// This has to be a macro (as opposed an inline function) beacause of the activation frame is
// allocated in the caller's stack.
//
// NOTE: This must not be wrapped in {}'s since that puts the activation frame in a very
//       short lived scope.  It's destructor will be called when control leaves the block
//       rather than when the function returns and that defeats the entire purpose of the
//       activation frame (which is to live for the life of the DDI call).
//
// NOTE 2:
// WDF_ACTIVATION constructor includes a default argument which is the _ReturnAddress()
// instrinsic. This macro should be placed in methods such that the _ReturnAddress
// points to calling driver code.
//

#define DDI_ENTRY_IMPERSONATION_OK()                     \
    WDF_ACTIVATION activationFrame(GetActivationList()); \

#define DDI_ENTRY()                                                             \
    DDI_ENTRY_IMPERSONATION_OK()                                                \
    FX_VERIFY(                                                                  \
        DRIVER(BadArgument, TODO),                                              \
        CHECK("It is illegal to invoke this DDI while "                         \
              "thread is impersonated",                                         \
              (FALSE == IsCurrentThreadImpersonated())) \
        );

#ifdef __cplusplus
}
#endif
