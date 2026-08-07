// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description: Surface Render Target (RT)
//
//------------------------------------------------------------------------------

MtExtern(CSwRenderTargetBitmap);

class CHw3DSoftwareSurface;


struct CSwRenderTargetLayerData
{
public:
    CSwRenderTargetLayerData()
    {
        m_pSourceBitmap = NULL;
    }

    ~CSwRenderTargetLayerData()
    {
        ReleaseInterfaceNoNULL(m_pSourceBitmap);
    }

public:

    IWGXBitmap *m_pSourceBitmap;

};


class CSwRenderTargetSurface :
    public CMILCOMBase,
    public CBaseSurfaceRenderTarget<CSwRenderTargetLayerData>,
    public CSpanSink
{
protected:
    CSwRenderTargetSurface(DisplayId associatedDisplay);
    virtual ~CSwRenderTargetSurface();

public:
    // IMILRenderTarget.

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

    // Draw a surface.

    STDMETHOD(DrawBitmap)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IWGXBitmapSource *pIBitmap,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        ) override;

    // Draw a mesh.

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
        ) override;

    // Draw the glyph run

    STDMETHOD(DrawGlyphs)(
        __inout_ecount(1) DrawGlyphsParameters &pars
        ) override;

    STDMETHOD(DrawVideo)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IAVSurfaceRenderer *pSurfaceRenderer,
        __inout_ecount(1) IWGXBitmapSource *pBitmapSource,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        ) override;

    STDMETHOD(CreateRenderTargetBitmap)(
        UINT width,
        UINT height,
        IntermediateRTUsage usageInfo,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
        __in_opt DynArray<bool> const *pActiveDisplays = NULL
        ) override;

    HRESULT BeginLayerInternal(
        __inout_ecount(1) CRenderTargetLayer *pNewLayer
        ) override;

    HRESULT EndLayerInternal(
        ) override;
    
    // This method is used to determine if the render target is being
    // used to render hardware or software, or if it's merely being used 
    // for bounds accumulation, hit test, etc.
    STDMETHOD(GetType) (__out DWORD *pRenderTargetType) 
    { 
        *pRenderTargetType = SWRasterRenderTarget; 
        RRETURN(S_OK);
    }


    UINT GetRealizationCacheIndex() override
    {
        return CMILResourceCache::SwRealizationCacheIndex;
    }

    STDMETHOD(GetNumQueuedPresents)(
        __out_ecount(1) UINT *puNumQueuedPresents
        );

    // CSpanSink interface

    void OutputSpan(INT y, INT xMin, INT xMax)  override;
    void AddDirtyRect(__in_ecount(1) const MilPointAndSizeL *prcDirty) override;

    HRESULT SetupPipeline(
        MilPixelFormat::Enum fmtColorData,
        __in_ecount(1) CColorSource *pColorSource,
        BOOL fPPAA,
        bool fComplementAlpha,
        MilCompositingMode::Enum eCompositingMode,
        __in_ecount(1) CSpanClipper *pSpanClipper,
        __in_ecount_opt(1) IMILEffectList *pIEffectList,
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Effect,CoordinateSpace::Device> *pmatEffectToDevice,
        __in_ecount(1) const CContextState *pContextState
        ) override;

    HRESULT SetupPipelineForText(
        __in_ecount(1) CColorSource *pColorSource,
        MilCompositingMode::Enum eCompositingMode,
        __inout_ecount(1) CSWGlyphRunPainter &painter,
        bool fNeedsAA
        ) override;

    VOID ReleaseExpensiveResources() override;

    VOID SetAntialiasedFiller(__inout_ecount(1) CAntialiasedFiller *pFiller) override;

    // misc

    void Cleanup3DResources();

protected:

    HRESULT SetSurface(
        __in_ecount(1) IWGXBitmap *pISurface
        );

    HRESULT LockInternalSurface(
        __in_ecount_opt(1) const WICRect *pRect,
        DWORD dwLockFlags
        );

    void UnlockInternalSurface();

private:

    HRESULT ClearLockedSurface(
        __in_ecount(1) const MilColorF *pColor,
        __in_ecount_opt(1) const CAliasedClip *pAliasedClip
        );

    void CleanUp(
        BOOL fRelease3DRT
        );

    bool UpdateCurrentClip(
        __in_ecount(1) const CAliasedClip &aliasedClip,
        __out_ecount(1) CRectClipper *pRectClipperOut
        );

    bool HasAlpha() const override;

    // Implementation for DrawPath and DrawInfinitePath.
    // Treats NULL pShape as infinite shape.
    HRESULT DrawPathInternal(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount_opt(1) const IShapeData *pShape,
        __inout_ecount_opt(1) const CPlainPen *pPen,
        __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
        __inout_ecount_opt(1) CBrushRealizer *pFillBrush
        );

protected:

    IWGXBitmap *m_pIInternalSurface;
    IWGXBitmapLock *m_pILock;

    //
    // Local copies of the surface information.
    //

    UINT m_cbStride;
    UINT m_cbPixel;   // byte size of a pixel.
    void *m_pvBuffer;

    //
    // Data used for the scan pipeline
    //

    CSPIntermediateBuffers m_IntermediateBuffers;
    CScanPipelineRendering m_ScanPipeline;

    //
    // keep a software rasterizer around.
    //

    CSoftwareRasterizer m_sr;

    //
    // persistent glyph painter data
    //
    CGlyphPainterMemory m_glyphPainterMemory;

    //
    // Sw 3D renderer
    //

    CHw3DSoftwareSurface* m_pHw3DRT;

private:
    CObjectUniqueness m_resizeUniqueness;

#if DBG_ANALYSIS
    bool m_fDbgBetweenBeginAndEnd3D;
#endif

#if DBG_STEP_RENDERING
protected:
    
    CMILSurfaceRect m_Dbg3DBounds;
    MilAntiAliasMode::Enum m_Dbg3DAAMode;

    ISteppedRenderingDisplayRT *m_pDisplayRTParent;

public:

    override void DbgGetSurfaceBitmapNoRef(
        __deref_out_ecount_opt(1) IWGXBitmap **ppSurfaceBitmap
        ) const
    {
        *ppSurfaceBitmap = m_pIInternalSurface;
    }
    override void DbgGetTargetSurface(
        __deref_out_ecount_opt(1) CD3DSurface **ppD3DSurface
        ) const
    {
         *ppD3DSurface = NULL; // DbgGetSurfaceBitmapNoRef should be used instead
    }
    override UINT DbgTargetWidth() const { return m_uWidth; }
    override UINT DbgTargetHeight() const { return m_uHeight; }

    #define SW_DBG_RENDERING_STEP(func)                             \
        do {                                                        \
            if (m_pDisplayRTParent)                                 \
            {                                                       \
                m_pDisplayRTParent->ShowSteppedRendering(           \
                    TEXT("WGXCORE!CSwRenderTargetSurface::")        \
                    TEXT(#func),                                    \
                    this);                                          \
            }                                                       \
        } while (UNCONDITIONAL_EXPR(0))
#else
    #define SW_DBG_RENDERING_STEP(func)
#endif DBG_STEP_RENDERING
};


class CSwRenderTargetBitmap :
    public CSwRenderTargetSurface,
    public IMILRenderTargetBitmap
{
public:
    static HRESULT Create(
        __in_ecount(1) IWGXBitmap *pIBitmap,
        DisplayId associatedDisplay,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap
        DBG_STEP_RENDERING_COMMA_PARAM(__inout_ecount_opt(1) ISteppedRenderingDisplayRT *pDisplayRTParent)
        );

    static HRESULT Create(
        UINT width,
        UINT height,
        MilPixelFormat::Enum format,
        FLOAT dpiX,
        FLOAT dpiY,
        DisplayId associatedDisplay,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap
        DBG_STEP_RENDERING_COMMA_PARAM(__inout_ecount_opt(1) ISteppedRenderingDisplayRT *pDisplayRTParent)
        );

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CSwRenderTargetBitmap));

    // IUnknown implementation. This disambiguates multiply inherited IUnknown
    // methods.

    DECLARE_COM_BASE;

    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

    //
    // IMILRenderTarget methods
    //

    STDMETHODIMP_(VOID) GetBounds(
        __out_ecount(1) MilRectF * const pBounds
        );

    STDMETHOD(Clear)(
        __in_ecount_opt(1) const MilColorF *pColor,
        __in_ecount_opt(1) const CAliasedClip *pAliasedClip
        );

    STDMETHOD(Begin3D)(
        __in_ecount(1) MilRectF const &rcBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        bool fUseZBuffer,
        FLOAT rZ
        ) override;

    STDMETHOD(End3D)() override;

    //
    // IMILRenderTargetBitmap methods
    //

    STDMETHOD(GetBitmapSource)(
        __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
        );

    STDMETHOD(GetCacheableBitmapSource)(
        __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
        );

    STDMETHOD(GetBitmap)(
        __deref_out_ecount(1) IWGXBitmap ** const ppIBitmap
        );

    STDMETHOD(GetNumQueuedPresents)(
        __out_ecount(1) UINT *puNumQueuedPresents
        );

protected:
    CSwRenderTargetBitmap(DisplayId associatedDisplay);
    virtual ~CSwRenderTargetBitmap() {};

private:
    void TintBitmapSource();
};


