//
//    Copyright (C) Microsoft.  All rights reserved.
//
#ifndef __FXMDL_H__
#define __FXMDL_H__

PMDL
FxMdlAllocateDebug(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxObject* Owner,
    __in PVOID VirtualAddress,
    __in ULONG Length,
    __in BOOLEAN SecondaryBuffer,
    __in BOOLEAN ChargeQuota,
    __in PVOID CallersAddress
    );

VOID
FxMdlFreeDebug(
    __in  PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PMDL Mdl
    );

VOID
FxMdlDump(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

PMDL
FORCEINLINE
FxMdlAllocate(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in FxObject* Owner,
    __in PVOID VirtualAddress,
    __in ULONG Length,
    __in BOOLEAN SecondaryBuffer,
    __in BOOLEAN ChargeQuota
    )
{
    if (FxDriverGlobals->FxVerifierOn)  {
        return FxMdlAllocateDebug(FxDriverGlobals,
                                  Owner,
                                  VirtualAddress,
                                  Length,
                                  SecondaryBuffer,
                                  ChargeQuota,
                                  _ReturnAddress());
    }
    else {
        return IoAllocateMdl(VirtualAddress,
                             Length,
                             SecondaryBuffer,
                             ChargeQuota,
                             NULL);
    }
}

VOID
__inline
FxMdlFree(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PMDL Mdl
    )
{
    if (FxDriverGlobals->FxVerifierOn) {
        FxMdlFreeDebug(FxDriverGlobals, Mdl);
    }
    else {
        IoFreeMdl(Mdl);
    }
}

VOID
__inline
FxIrpMdlFree(
    __in PMDL Mdl
    )
{
    IoFreeMdl(Mdl);
}

// Do not allow accidental usage by redefining these DDIs to uncompilable names.
// Do this after we define our own functions so that our own functions use
// the correct DDI.
#undef IoAllocateMdl
#undef IoFreeMdl

#define IoAllocateMdl use_FxMdlAllocate_instead
#define IoFreeMdl use_FxMdlFree_instead

#endif // __FXMDL_H__
