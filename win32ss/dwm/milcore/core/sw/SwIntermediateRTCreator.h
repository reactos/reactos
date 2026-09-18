// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        Contains declaration of the CSwIntermediateRTCreator class
//

class CSwIntermediateRTCreator : public CIntermediateRTCreator
{
public:
    CSwIntermediateRTCreator(
        MilPixelFormat::Enum fmtTarget,
        DisplayId associatedDisplay
        DBG_STEP_RENDERING_COMMA_PARAM(__in_ecount_opt(1) ISteppedRenderingDisplayRT *pDisplayRTParent)
        );

    // Create a temporary offscreen render target that is expected to
    // be used later with this render target as a source bitmap.

    STDMETHOD(CreateRenderTargetBitmap)(
        UINT width,
        UINT height,
        IntermediateRTUsage usageInfo,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
        __in_opt DynArray<bool> const *pActiveDisplays = NULL
        ) override;

    STDMETHOD(ReadEnabledDisplays) (
        __inout DynArray<bool> *pEnabledDisplays
        ) override;
    
private:

    MilPixelFormat::Enum m_fmtTarget;
    DisplayId m_associatedDisplay;

#if DBG_STEP_RENDERING
    ISteppedRenderingDisplayRT *m_pDisplayRTParent;
#endif
};


