#ifndef _FXVERIFIER_H_
#define _FXVERIFIER_H_

#include "fxglobals.h"

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

#endif //_FXVERIFIER_H_