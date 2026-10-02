// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_targets
//      $Keywords:
//
//  $Description:
//      Contains CDummyRenderTarget which implements do nothing
//      IRenderTargetInternal, IMILRenderTargetBitmap, and IWGXBitmapSource
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CDummyRenderTarget:
    public IRenderTargetInternal,
    public IMILRenderTargetBitmap,
    public IMILRenderTargetHWND,
    public IWGXBitmapSource
{

protected:

    //
    // There is one static instance of this class which held by this class
    // directly.
    //

    CDummyRenderTarget();

public:

    // Reference to static instance of the dummy rt

    static CDummyRenderTarget * const sc_pDummyRT;
//    const static (CDummyRenderTarget &) sc_dummyRT;
    typedef CDummyRenderTarget &CDummyRenderTargetRef;
    static CDummyRenderTargetRef sc_dummyRTRef;


    // IUnknown.

    //
    // This class should only be created as a static object
    // and as such should never really be reference counted.
    //

    override STDMETHODIMP_(ULONG) AddRef(void);
    override STDMETHODIMP_(ULONG) Release(void);
    override STDMETHODIMP QueryInterface(
        __in_ecount(1) REFIID riid,
        __deref_out void **ppv
        );

    // IMILRenderTarget.

    override STDMETHOD_(VOID, GetBounds)(
        __out_ecount(1) MilRectF * const pBounds
        );

    override STDMETHOD(Clear)(
        __in_ecount_opt(1) const MilColorF *pColor,
        __in_ecount_opt(1) const CAliasedClip *pAliasedClip
        );

    override STDMETHOD(Begin3D)(
        __in_ecount(1) MilRectF const &rcBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        bool fUseZBuffer,
        FLOAT rZ
        );

    override STDMETHOD(End3D)();

    // IRenderTargetInternal.

    override STDMETHOD_(__outro_ecount(1) const CMILMatrix *, GetDeviceTransform)() const;

    override STDMETHOD(DrawBitmap)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IWGXBitmapSource *pIBitmap,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    override STDMETHOD(DrawMesh3D)(
        __inout_ecount(1) CContextState* pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) CMILMesh3D *pMesh3D,
        __inout_ecount_opt(1) CMILShader *pShader,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    override STDMETHOD(DrawPath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) IShapeData *pShape,
        __inout_ecount_opt(1) CPlainPen *pPen,
        __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
        __inout_ecount_opt(1) CBrushRealizer *pFillBrush
        );

    override STDMETHOD(DrawInfinitePath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) BrushContext *pBrushContext,
        __inout_ecount(1) CBrushRealizer *pFillBrush
        ); 

    override STDMETHOD(ComposeEffect)(
        __inout_ecount(1) CContextState *pContextState,
        __in_ecount(1) CMILMatrix *pScaleTransform,
        __inout_ecount(1) CMilEffectDuce* pEffect,
        UINT uIntermediateWidth,
        UINT uIntermediateHeight,
        __in_opt IMILRenderTargetBitmap* pImplicitInput
        );
    
    override STDMETHOD(DrawGlyphs)(DrawGlyphsParameters &pars);

    override STDMETHOD(CreateRenderTargetBitmap)(
        UINT width,
        UINT height,
        IntermediateRTUsage usageInfo,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
        __in_opt DynArray<bool> const *pActiveDisplays = NULL
        );

    override STDMETHOD(BeginLayer)(
        __in_ecount(1) MilRectF const &LayerBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        __in_ecount_opt(1) IShapeData const *pGeometricMask,
        __in_ecount_opt(1) CMILMatrix const *pGeometricMaskToTarget,
        FLOAT flAlphaScale,
        __in_ecount_opt(1) CBrushRealizer *pAlphaMask
        );

    override STDMETHOD(EndLayer)();

    override STDMETHOD_(void, EndAndIgnoreAllLayers)();

    override STDMETHOD(ReadEnabledDisplays) (
        __inout DynArray<bool> *pEnabledDisplays
        );
    
    // This method is used to determine if the render target is being
    // used to render, or if it's merely being used for bounds accumulation,
    // hit test, etc.
    STDMETHOD(GetType) (__out DWORD *pRenderTargetType) 
    { 
        *pRenderTargetType = DummyRenderTarget; 
        RRETURN(S_OK);
    }

    // This method is used to allow a developer to force ClearType use in
    // intermediate render targets with alpha channels.
    STDMETHOD(SetClearTypeHint)(
        __in bool forceClearType
        )
    {
        RRETURN(S_OK);
    }

    override UINT GetRealizationCacheIndex();

    override STDMETHOD(DrawVideo)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IAVSurfaceRenderer *pSurfaceRenderer,
        __inout_ecount(1) IWGXBitmapSource *pBitmapSource,        
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    // IMILRenderTargetBitmap.

    override STDMETHOD(GetBitmapSource)(
        __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
        );

    override STDMETHOD(GetCacheableBitmapSource)(
        __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
        );    

    override STDMETHOD(GetBitmap)(
        __deref_out_ecount(1) IWGXBitmap ** const ppIBitmap
        );

    // IMILRenderTargetHWND

    override STDMETHOD(SetPosition)(
        __in_ecount(1) MilRectF const *prc
        );

    override STDMETHOD(GetInvalidRegions)(
        __deref_outro_ecount(*pNumRegions) MilRectF const ** const prgRegions,
        __out_ecount(1) UINT *pNumRegions,
        __out bool *fWholeTargetInvalid
        );

    override STDMETHOD(UpdatePresentProperties)(
        MilTransparency::Flags transparencyFlags,
        FLOAT constantAlpha,
        __in_ecount(1) MilColorF const &colorKey
        );

    override STDMETHODIMP Present(
        );

    override STDMETHODIMP ScrollBlt (
        THIS_
        __in_ecount(1) const RECT *prcSource,
        __in_ecount(1) const RECT *prcDest
        );        

    override STDMETHODIMP Invalidate(
        __in_ecount_opt(1) MilRectF const *prc
        );

    override STDMETHOD_(VOID, GetIntersectionWithDisplay)(
        UINT iDisplay,
        __out_ecount(1) MilRectL &rcIntersection
        );
    
    override STDMETHOD(WaitForVBlank)();

    override STDMETHOD_(VOID, AdvanceFrame)(
        UINT uFrameNumber
        );

    STDMETHOD(GetNumQueuedPresents)(
        __out_ecount(1) UINT *puNumQueuedPresents
        );

    override STDMETHOD_(bool, CanReuseForThisFrame)(
        THIS_
        __in_ecount(1) IRenderTargetInternal* pIRTParent
        );

    override STDMETHOD(CanAccelerateScroll)(
        __out_ecount(1) bool *fCanAccelerateScroll
        );

    // IWGXBitmapSource.

    override STDMETHOD(GetSize)(
        __out_ecount(1) UINT *puWidth,
        __out_ecount(1) UINT *puHeight
        );

    override STDMETHOD(GetPixelFormat)(
        __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
        );

    override STDMETHOD(GetResolution)(
        __out_ecount(1) double *pDpiX,
        __out_ecount(1) double *pDpiY
        );

    override STDMETHOD(CopyPalette)(
        __inout_ecount(1) IWICPalette *pIPalette
        );

    override STDMETHOD(CopyPixels)(
        __in_ecount_opt(1) const MILRect *prc,
        __in UINT cbStride,
        __in UINT cbBufferSize,
        __out_ecount(cbBufferSize) BYTE *pvPixels
        );

private:

    // internal data
    CMILMatrix m_matDeviceTransform;;

};



