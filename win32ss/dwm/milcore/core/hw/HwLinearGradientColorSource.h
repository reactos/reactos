// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwLinearGradientColorSource declaration
//

MtExtern(CHwLinearGradientColorSource);

//+----------------------------------------------------------------------------
//
//  Class:     CHwLinearGradientColorSource
//
//  Synopsis:  Provides a linear gradient color source for a HW device
//
//-----------------------------------------------------------------------------

class CHwLinearGradientColorSource : public CHwTexturedColorSource
{
public:

    static HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __deref_out_ecount(1) CHwLinearGradientColorSource **ppHwLinGradCS
        );

private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwLinearGradientColorSource));

protected:

    CHwLinearGradientColorSource(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice
        );
    ~CHwLinearGradientColorSource();

public:

    HRESULT SetBrushAndContext(
        __in_ecount(1) CMILBrushGradient *pGradBrush,
        __in_ecount(1) const CBaseMatrix *pmatWorld2DToSampleSpace,
        __in_ecount(1) const CContextState *pContextState
        );

    void InvalidateRealization();

    __out_ecount(1) const CMILBrushGradient *GetGradientBrushNoRef() const { return m_pGradBrushNoRef; }

    __out_ecount(1) const CMILMatrix* GetWorld2DToTexture() const { Assert(m_pGradBrushNoRef); return &m_matWorld2DToTexture; }
    
    UINT GetTexelCount() const { Assert(m_pGradBrushNoRef); return m_cDesiredTextureWidth; }

    
    FLOAT GetGradientSpanEnd() const { Assert(m_pGradBrushNoRef); return m_gradientSpanInfo.GetSpanEndTextureSpace(); }

    //
    // CHwColorSource methods
    //

    override bool IsOpaque(
        ) const;

    override HRESULT Realize(
        );

    override HRESULT SendDeviceStates(
        DWORD dwStage,
        DWORD dwSampler
        );

private:

    HRESULT FillGradientTexture(
        __in_ecount(1) CGradientColorData const &oColorData,
        MilGradientWrapMode::Enum wrapMode,
        MilColorInterpolationMode::Enum colorInterpolationMode,
        BOOL fIsRadial
        );

private:

    UINT m_cDesiredTextureWidth;     // Number of texels for desired texture width
    UINT m_cRealizedTextureWidth;    // Number of texels in currently allocated texture manager

    CGradientSpanInfo m_gradientSpanInfo;

    CHwVidMemTextureManager m_vidMemManager;

    CMILBrushGradient *m_pGradBrushNoRef;   // The current device
                                            // independent brush being
                                            // realized

    bool m_fColorsNeedUpdating;   // True if the current sys mem surface does
                                  // not contain the colors from the gradient
                                  // brush (hidden inside the texture manager)
                                  // contains a useful realization of the
                                  // current device independent brush


    //
    // Info that the radial gradient shader parameter might need (if there is one)
    //
    CMILMatrix m_matWorld2DToTexture;
};



