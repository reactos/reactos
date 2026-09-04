// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics
//      $Keywords:
//
//  $Description:
//      Implementation of the bound-calculating render target.  This class
//      accumulates the bounding rectangle of whatever is "rendered" to it.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CSwRenderTargetGetBounds);

class CSwRenderTargetGetBounds :
    public CMILCOMBase,
    public CObjectUniqueness,
    public IRenderTargetInternal
{
protected:
    CSwRenderTargetGetBounds();
    virtual ~CSwRenderTargetGetBounds();

public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CSwRenderTargetGetBounds));

    DECLARE_COM_BASE;

    STDMETHOD(GetNumQueuedPresents)(
        __out_ecount(1) UINT *puNumQueuedPresents
        ) override;

    // IMILRenderTarget.

    STDMETHOD_(VOID, GetBounds)(
        __out_ecount(1) MilRectF * const pBounds
        ) override;

    STDMETHOD(Clear)(
        __in_ecount_opt(1) const MilColorF *pColor,
        __in_ecount_opt(1) const CAliasedClip *pAliasedClip
        ) override;

    STDMETHOD(Begin3D)(
        __in_ecount(1) MilRectF const &rcBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        bool fUseZBuffer,
        FLOAT rZ
        ) override;

    STDMETHOD(End3D)(
        ) override;


    //
    // IRenderTargetInternal methods
    //

    STDMETHOD_(__outro_ecount(1) const CMILMatrix *, GetDeviceTransform)() const override;

    // Draw a surface.

    STDMETHOD(DrawBitmap)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IWGXBitmapSource *pIBitmap,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        ) override;

    // Draw a mesh3D.

    STDMETHOD(DrawMesh3D)(
        __inout_ecount(1) CContextState* pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) CMILMesh3D *pMesh3D,
        __inout_ecount_opt(1) CMILShader *pShader,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        ) override;

    // Draw a path.

    STDMETHOD(DrawPath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) IShapeData *pShape,
        __inout_ecount_opt(1) CPlainPen *pPen,
        __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
        __inout_ecount_opt(1) CBrushRealizer *pFillBrush
        ) override;

    // Fill render target with a brush.

    STDMETHOD(DrawInfinitePath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) BrushContext *pBrushContext,
        __inout_ecount(1) CBrushRealizer *pFillBrush
        ) override;

    STDMETHOD(ComposeEffect)(
        __inout_ecount(1) CContextState *pContextState,
        __in_ecount(1) CMILMatrix *pScaleTransform,
        __inout_ecount(1) CMilEffectDuce* pEffect,
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt IMILRenderTargetBitmap* pImplicitInput
        )  override { return E_NOTIMPL; };


    // Draw the glyph run

    STDMETHOD(DrawGlyphs)(
        __inout_ecount(1) DrawGlyphsParameters &pars
        ) override;

    STDMETHOD(DrawVideo)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) IAVSurfaceRenderer *pSurfaceRenderer,
        __inout_ecount_opt(1) IWGXBitmapSource *pMILBitmapSource,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        ) override;

    STDMETHOD(CreateRenderTargetBitmap)(
        UINT width,
        UINT height,
        IntermediateRTUsage usageInfo,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
        __in_opt DynArray<bool> const *pActiveDisplays = NULL
        )  override;

    STDMETHOD(BeginLayer)(
        __in_ecount(1) MilRectF const &LayerBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        __in_ecount_opt(1) IShapeData const *pGeometricMask,
        __in_ecount_opt(1) CMILMatrix const *pGeometricMaskToTarget,
        FLOAT flAlphaScale,
        __in_ecount_opt(1) CBrushRealizer *pAlphaMask
        )  override;

    STDMETHOD(EndLayer)(
        )  override;

    STDMETHOD_(void, EndAndIgnoreAllLayers)(
        ) override;

    STDMETHOD(ReadEnabledDisplays) (
        __inout DynArray<bool> *pEnabledDisplays
        )  override;

    // This method is used to determine if the render target is being
    // used to render, or if it's merely being used for bounds accumulation,
    // hit test, etc.
    STDMETHOD(GetType) (__out DWORD *pRenderTargetType) override
    { 
        *pRenderTargetType = BoundsRenderTarget; 
        RRETURN(S_OK);
    }

    // This method is used to allow a developer to force ClearType use in
    // intermediate render targets with alpha channels.
    STDMETHOD(SetClearTypeHint)(
        __in bool forceClearType
        ) override
    {
        RRETURN(S_OK);
    }

    UINT GetRealizationCacheIndex() override;

    VOID ResetBounds() {
        m_rcResult = CMilPointAndSizeF::sc_rcEmpty;
    }

    const MilRectF &GetAccumulatedBounds() const {
        return m_rcResult;
    } 

    static HRESULT Create(
        __deref_out_ecount(1) CSwRenderTargetGetBounds **ppRT
        );

private:

    void AddBounds(
        __in_ecount(1) const CMilRectF &rcBounds,
        __in_ecount(1) const CAliasedClip& aliasedClip
        );


protected:

    //
    // The bounding rectangle.
    // This is set to empty when bounding starts,
    // and accumulated during rendering.

    CMilRectF m_rcResult;

    //
    // RenderTarget State
    //

    CMILMatrix m_DeviceTransform;  // Always identity. We keep this just because
                                 // GetDeviceTransform returns a pointer to it.

    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv) override;

protected:

    HRESULT HrInit();

};



