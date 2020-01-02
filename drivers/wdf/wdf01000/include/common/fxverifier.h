#ifndef _FXVERIFIER_H_
#define _FXVERIFIER_H_

#include "fxglobals.h"
#include "common/mxgeneral.h"

enum FxEnhancedVerifierBitFlags {
    //
    // low 2 bytes are used for function table Hooking  
    //
    FxEnhancedVerifierCallbackIrqlAndCRCheck      = 0x00000001,
    //
    // Lower nibble of 3rd byte  for forward progress
    //
    FxEnhancedVerifierForwardProgressFailAll      = 0x00010000,
    FxEnhancedVerifierForwardProgressFailRandom   = 0x00020000,

    //
    // bit masks
    //
    FxEnhancedVerifierFunctionTableHookMask       = 0x0000ffff,
    FxEnhancedVerifierForwardProgressMask         = 0x000f0000,

    //
    // higher nibble of 3rd byte for performance analysis
    // 
    FxEnhancedVerifierPerformanceAnalysisMask      = 0x00f00000,
};

__inline
BOOLEAN
IsFxVerifierFunctionTableHooking(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
    if (FxDriverGlobals->FxEnhancedVerifierOptions & 
            FxEnhancedVerifierFunctionTableHookMask)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

//
// FxVerifierDbgBreakPoint and FxVerifierBreakOnDeviceStateError are mapped
// to FX_VERIFY in UMDF and break regardless of any flags
//
__inline
VOID
FxVerifierDbgBreakPoint(
    __in PFX_DRIVER_GLOBALS FxDriverGlobals
    )
{
#if FX_CORE_MODE == FX_CORE_KERNEL_MODE
    CHAR ext[] = "sys";
#else
    CHAR ext[] = "dll";
#endif

    Mx::MxDbgPrint("WDF detected potentially invalid operation by %s.%s "
             "Dump the driver log (!wdflogdump %s.%s) for more information.\n",
             FxDriverGlobals->Public.DriverName, ext,
             FxDriverGlobals->Public.DriverName, ext
             );
    
    if (FxDriverGlobals->FxVerifierDbgBreakOnError)
    {
        Mx::MxDbgBreakPoint();
    }
    else
    {
        Mx::MxDbgPrint("Turn on framework verifier for %s.%s to automatically "
            "break into the debugger next time it happens.\n",
            FxDriverGlobals->Public.DriverName, ext);
    }
}

#endif //_FXVERIFIER_H_