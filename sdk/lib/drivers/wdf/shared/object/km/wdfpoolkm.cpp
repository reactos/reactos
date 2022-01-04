/*++

Copyright (c) Microsoft Corporation

Module Name:

    wdfpoolkm.cpp

Abstract:

    This module implements the driver frameworks pool routines
    functionality only applicable in kernel mode

Author:




Environment:

    Kernel mode only

Revision History:


--*/

#include "fxobjectpch.hpp"

// We use DoTraceMessage
extern "C" {
#if defined(EVENT_TRACING)
#include "wdfpoolkm.tmh"
#endif
}

// undo the previous masking
#undef IoAllocateMdl
#undef IoFreeMdl

__drv_maxIRQL(DISPATCH_LEVEL)
NTKERNELAPI
PMDL
STDCALL
IoAllocateMdl(
    __in_opt __drv_aliasesMem PVOID VirtualAddress,
    __in ULONG Length,
    __in BOOLEAN SecondaryBuffer,
    __in BOOLEAN ChargeQuota,
    __inout_opt PIRP Irp
    );

__drv_maxIRQL(DISPATCH_LEVEL)
NTKERNELAPI
VOID
STDCALL
IoFreeMdl(
    PMDL Mdl
    );

//
// Windows Driver Framework Pool Tracking
//
// This module implements a generic pool tracking mechanism
// if pool verifier mode is enabled.
//
// There can be multiple pools, each represented by a FX_POOL header.
//
// When the framework is supplied as a DLL, there is a global
// pool that represents allocations for the framework DLL itself.  These
// allocations are pool allocations and object allocations.
//
// The driver's pool allocations are not currently tracked.  If the driver needs
// to use pool outside of the framework objects, it calls the WDM
// ExAllocatePoolWithTag and ExFreePool(WithTag) APIs.
//



PMDL
FxMdlAllocateDebug(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxObject* Owner,
    __in PVOID VirtualAddress,
    __in ULONG Length,
    __in BOOLEAN SecondaryBuffer,
    __in BOOLEAN ChargeQuota,
    __in PVOID CallersAddress
    )
{
    FxDriverGlobalsDebugExtension* pExtension;
    FxAllocatedMdls* pAllocated, **ppNext;
    ULONG i;
    PMDL pMdl;
    KIRQL irql;

    pExtension = FxDriverGlobals->DebugExtension;
    if (pExtension == NULL) {
        return IoAllocateMdl(VirtualAddress,
                             Length,
                             SecondaryBuffer,
                             ChargeQuota,
                             NULL);
    }

    pAllocated = &pExtension->AllocatedMdls;
    ppNext = NULL;
    pMdl = NULL;

    KeAcquireSpinLock(&pExtension->AllocatedMdlsLock, &irql);

    while (pAllocated != NULL && pAllocated->Count == NUM_MDLS_IN_INFO) {
        ppNext = &pAllocated->Next;
        pAllocated = pAllocated->Next;
    }

    if (pAllocated == NULL) {
        //
        // No more entries, allocate a new table
        //
        pAllocated = (FxAllocatedMdls*) ExAllocatePoolWithTag(
            NonPagedPool, sizeof(FxAllocatedMdls), FxDriverGlobals->Tag);

        if (pAllocated != NULL) {
            //
            // Zero out the new buffer and link it in to the list
            //
            RtlZeroMemory(pAllocated, sizeof(*pAllocated));
            *ppNext = pAllocated;
        }
        else {
            //
            // Could not allocate a new table, return error
            //
            KeReleaseSpinLock(&pExtension->AllocatedMdlsLock, irql);

            return NULL;
        }
    }

    for (i = 0; i < NUM_MDLS_IN_INFO; i++) {
        if (pAllocated->Info[i].Mdl != NULL) {
            continue;
        }

        pMdl =  IoAllocateMdl(VirtualAddress,
                              Length,
                              SecondaryBuffer,
                              ChargeQuota,
                              NULL);

        if (pMdl != NULL) {
            pAllocated->Info[i].Mdl = pMdl;
            pAllocated->Info[i].Owner = Owner;
            pAllocated->Info[i].Caller = CallersAddress;
            pAllocated->Count++;
        }
        break;
    }

    KeReleaseSpinLock(&pExtension->AllocatedMdlsLock, irql);

    return pMdl;
}

VOID
FxMdlFreeDebug(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PMDL Mdl
    )
{
    FxDriverGlobalsDebugExtension* pExtension;
    FxAllocatedMdls* pAllocated, **ppNext;
    ULONG i;
    KIRQL irql;
    BOOLEAN found;

    pExtension = FxDriverGlobals->DebugExtension;
    if (pExtension == NULL) {
        IoFreeMdl(Mdl);
        return;
    }

    found = FALSE;

    pAllocated = &pExtension->AllocatedMdls;
    ppNext = NULL;

    KeAcquireSpinLock(&pExtension->AllocatedMdlsLock, &irql);

    while (pAllocated != NULL) {
        for (i = 0; i < NUM_MDLS_IN_INFO; i++) {
            if (pAllocated->Info[i].Mdl != Mdl) {
                continue;
            }

            RtlZeroMemory(&pAllocated->Info[i],
                          sizeof(pAllocated->Info[i]));

            pAllocated->Count--;

            if (pAllocated->Count == 0 &&
                pAllocated != &pExtension->AllocatedMdls) {
                //
                // Remove the current table from the chain
                //
                *ppNext = pAllocated->Next;

                //
                // And free it
                //
                ExFreePool(pAllocated);
            }

            IoFreeMdl(Mdl);
            found = TRUE;
            break;
        }

        if (found) {
            break;
        }

        ppNext = &pAllocated->Next;
        pAllocated = pAllocated->Next;
    }

    KeReleaseSpinLock(&pExtension->AllocatedMdlsLock, irql);

    if (found == FALSE) {



        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
}

VOID
FxMdlDump(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    FxAllocatedMdls *pCur;
    BOOLEAN leak;

    if (FxDriverGlobals->DebugExtension == NULL) {
        return;
    }

    leak = FALSE;

    for (pCur = &FxDriverGlobals->DebugExtension->AllocatedMdls;
         pCur != NULL;
         pCur = pCur->Next) {
        ULONG i;

        for (i = 0; i < NUM_MDLS_IN_INFO; i++) {
            if (pCur->Info[i].Mdl != NULL) {
                leak = TRUE;

                DoTraceLevelMessage(
                    FxDriverGlobals, TRACE_LEVEL_ERROR, TRACINGDEVICE,
                    "PMDL 0x%p leaked, FxObject owner %p, Callers Address %p",
                    pCur->Info[i].Mdl, pCur->Info[i].Owner,
                    pCur->Info[i].Caller);
            }
        }
    }

    if (leak) {
        FxVerifierDbgBreakPoint(FxDriverGlobals);
    }
}

