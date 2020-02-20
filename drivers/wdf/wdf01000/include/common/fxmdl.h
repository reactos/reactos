#ifndef _FXMDL_H_
#define _FXMDL_H_

#include "common/fxglobals.h"

VOID
FxMdlDump(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    );

VOID
FxMdlFreeDebug(
    __in  PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PMDL Mdl
    );

VOID
__inline
FxMdlFree(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals,
    __in PMDL Mdl
    )
{
    if (FxDriverGlobals->FxVerifierOn)
    {
        FxMdlFreeDebug(FxDriverGlobals, Mdl);
    }
    else
    {
        IoFreeMdl(Mdl);
    }
}

#endif //_FXMDL_H_