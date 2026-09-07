// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwSurfaceRenderTarget which implements IInternalRenderTarget
//

class CD3DDeviceLevel1;
#if DBG_STEP_RENDERING
class CHwDisplayRenderTarget;
#endif DBG_STEP_RENDERING


class CHwBlurShader;


struct CHwRenderTargetLayerData
{
public:
    CHwRenderTargetLayerData()
    {
        m_pSourceBitmap = NULL;
    }

    ~CHwRenderTargetLayerData()
    {
        ReleaseInterfaceNoNULL(m_pSourceBitmap);
    }

public:
    //
    // Anything that needs to persist between BeginLayer
    // and EndLayer should not be evictable. BeginLayer
    // has its own UseContext so if we run out of memory
    // after BeginLayer but before EndLayer, evictable
    // resources created by BeginLayer may get destroyed.
    //
    CHwDestinationTexture *m_pSourceBitmap;

};



//------------------------------------------------------------------------------
//
//  Class: CHwSurfaceRenderTarget
//
//  Description:
//      This object is the base class for CHwHWNDRenderTarget and provides a
//      basic render target that can output to a dx9 surface.
//
//------------------------------------------------------------------------------

class CHwSurfaceRenderTarget :
    public CBaseSurfaceRenderTarget<CHwRenderTargetLayerData>
{
protected:

    CHwSurfaceRenderTarget(
        __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        MilPixelFormat::Enum fmtTarget,
        D3DFORMAT d3dfmtTarget,
        DisplayId associatedDisplay
        );

    virtual ~CHwSurfaceRenderTarget();

    //
    // additional CHwSurfaceRenderTarget methods
    //

    //+------------------------------------------------------------------------
    //
    //  Member:    IsValid
    //
    //  Synopsis:  Returns FALSE when rendering with this render target or any
    //             use is no longer allowed.  Mode change is a common cause of
    //             of invalidation.
    //
    //-------------------------------------------------------------------------

    virtual bool IsValid() const PURE;


public:

    //
    // IMILRenderTarget methods
    //

    STDMETHOD(Clear)(
        __in_ecount_opt(1) const MilColorF *pColor,
        __in_ecount_opt(1) const CAliasedClip *pAliasedClip
        );

    override STDMETHOD(Begin3D)(
        __in_ecount(1) MilRectF const &rcBounds,
        MilAntiAliasMode::Enum AntiAliasMode,
        bool fUseZBuffer,
        FLOAT rZ
        );

    override STDMETHOD(End3D)(
        );

    //
    // IRenderTargetInternal methods
    //

    // Draw a surface.

    STDMETHOD(DrawBitmap)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount(1) IWGXBitmapSource *pIBitmap,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    // Draw a mesh.

    STDMETHOD(DrawMesh3D)(
        __inout_ecount(1) CContextState* pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) CMILMesh3D *pMesh3D,
        __inout_ecount_opt(1) CMILShader *pShader,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    // Draw a path.

    STDMETHOD(DrawPath)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) IShapeData *pShape,
        __inout_ecount_opt(1) CPlainPen *pPen,
        __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
        __inout_ecount_opt(1) CBrushRealizer *pFillBrush
        );

    // Fill render target with a brush.

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

    // Draw a glyph run

    STDMETHOD(DrawGlyphs)(
        __inout_ecount(1) DrawGlyphsParameters &pars
        );

    override STDMETHOD(CreateRenderTargetBitmap)(
        UINT width,
        UINT height,
        IntermediateRTUsage usageInfo,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetBitmap,
        __in_opt DynArray<bool> const *pActiveDisplays = NULL
        );

    HRESULT BeginLayerInternal(
        __inout_ecount(1) CRenderTargetLayer *pNewLayer
        );

    HRESULT EndLayerInternal(
        );
    
    STDMETHOD(GetNumQueuedPresents)(
        __out_ecount(1) UINT *puNumQueuedPresents
        );

    // Draw Video

    STDMETHOD(DrawVideo)(
        __inout_ecount(1) CContextState *pContextState,
        __inout_ecount_opt(1) IAVSurfaceRenderer *pSurfaceRenderer,
        __inout_ecount_opt(1) IWGXBitmapSource *pBitmapSource,
        __inout_ecount_opt(1) IMILEffectList *pIEffect
        );

    // This method is used to determine if the render target is being
    // used to render hardware or software, or if it's merely being used 
    // for bounds accumulation, hit test, etc.
    STDMETHOD(GetType) (__out DWORD *pRenderTargetType) 
    { 
        *pRenderTargetType = HWRasterRenderTarget; 
        RRETURN(S_OK);
    }

    override UINT GetRealizationCacheIndex()
    {
        return m_pD3DDevice->GetRealizationCacheIndex();
    }

    bool CanUseShaderPipeline() const
    {
        return CHwShaderPipeline::CanRunWithDevice(
            m_pD3DDevice
            );
    }

public:

    HRESULT Begin3DInternal(
        FLOAT rZ,
        bool fUseZBuffer,
        __inout_ecount(1) D3DMULTISAMPLE_TYPE &MultisampleType
        );

    HRESULT EnsureState(
        __in_ecount(1) CContextState const *pContextState
        );

    //
    // internal methods
    //

    HRESULT GetPixelFormat(
        __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
        );

    HRESULT GetSize(
        __out_ecount(1) UINT *puWidth,
        __out_ecount(1) UINT *puHeight
        ) const;

    D3DFORMAT GetD3DTextureFormat() const
    {
        return m_d3dfmtTargetSurface;
    }

    HRESULT GetHwDestinationTexture(
        __in_ecount(1) const CMILSurfaceRect &rcDestRect,
        __in_ecount_opt(crgSubDestCopyRects) const CMILSurfaceRect *prgSubDestCopyRects,
        UINT crgSubDestCopyRects,
        bool fUseLayeredDestinationTexture,
        __deref_out_ecount(1) CHwDestinationTexture **ppHwDestinationTexture
        );

    HRESULT PopulateDestinationTexture(
        __in_ecount(1) const CMILSurfaceRect *prcSource,
        __in_ecount(1) const CMILSurfaceRect *prcDest,
        __inout_ecount(1) IDirect3DTexture9 *pD3DTexture
        );

protected:
    HRESULT SetAsRenderTarget(
        );

    HRESULT SetAsRenderTargetFor3D(
        );

protected:

    HRESULT Ensure2DState(
        );

    HRESULT Ensure3DState(
        __in_ecount(1) CContextState const *pContextState
        );

    void Ensure3DRenderTarget(
        D3DMULTISAMPLE_TYPE MultisampleType
        );

    HRESULT EnsureDepthState(
        );

    HRESULT EnsureClip(
        __in_ecount(1) const CContextState *pContextState
        );

    HRESULT DrawPathInternal(
        __in_ecount(1) const CContextState *pContextState,
        __in_ecount_opt(1) CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> const *pmatShapeToDevice,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __inout_ecount(1) const IShapeData *pShape,
        __inout_ecount_opt(1) const CPlainPen *pPen,
        __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
        __inout_ecount_opt(1) CBrushRealizer *pFillBrush
        );

    HRESULT SoftwareFillPath(
        __in_ecount(1) const CContextState *pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice,
        __in_ecount(1) const IShapeData *pShape,
        __in_ecount(1) CBrushRealizer *pBrushRealizer,
        HRESULT hrReasonForFallback
        );

    HRESULT AcceleratedFillPath(
        MilCompositingMode::Enum CompositingMode,
        __in_ecount(1) IGeometryGenerator *pGeometryGenerator,
        __in_ecount(1) CHwBrush *pBrush,
        __in_ecount_opt(1) const IMILEffectList *pIEffects,
        __in_ecount(1) const CHwBrushContext *pEffectContext,
        __in_ecount_opt(1) const CMILSurfaceRect *prcOutsideBounds = NULL,
        bool fNeedInside = true
        );

    HRESULT ShaderAcceleratedFillPath(
        MilCompositingMode::Enum CompositingMode,
        __in_ecount(1) IGeometryGenerator *pGeometryGenerator,
        __in_ecount(1) CHwBrush *pBrush,
        __in_ecount_opt(1) const IMILEffectList *pIEffects,
        __in_ecount(1) const CHwBrushContext *pEffectContext,
        __in_ecount_opt(1) const CMILSurfaceRect *prcOutsideBounds = NULL,
        bool fNeedInside = true
        );

    HRESULT FillPath(
        __in_ecount(1) const CContextState *pContextState,
        __inout_ecount_opt(1) BrushContext *pBrushContext,
        __in_ecount(1) const IShapeData *pShape,
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice, // (NULL OK)
        __in_ecount(1) const CRectF<CoordinateSpace::Shape> *prcShapeBounds,      // in shape space
        __in_ecount(1) CBrushRealizer *pBrushRealizer,
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorldToDevice
        );

    HRESULT FillPathWithBrush(
        __in_ecount(1) const CContextState *pContextState,
        __in_ecount(1) const IShapeData *pShape,
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice,// (NULL OK)
        __in_ecount(1) const CRectF<CoordinateSpace::Shape> *prcShapeBounds,      // in shape space
        __in_ecount(1) CMILBrush *pFillBrush,
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorldToDevice,
        __in_ecount_opt(1) const IMILEffectList *pIEffect
        );

    HRESULT HwShaderFillPath(
        __in_ecount(1) const CContextState *pContextState,
        __in_ecount(1) CHwShader *pHwShader,
        __in_ecount(1) const IShapeData *pShapeData,
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDeviceOrNULL,
        __in_ecount(1) const CMilRectL &rcRenderingBounds
        );

    HRESULT SoftwareDrawGlyphs(
        __inout_ecount(1) DrawGlyphsParameters &pars,
        bool fTargetSupportsClearType,
        HRESULT hrReasonForFallback
        );

    bool IntersectsRenderTargetBounds(
        __in_ecount(1) const CMilPointAndSizeF &rcShapeBounds
        );

    override bool HasAlpha() const;

#if DBG
    void DbgDrawBoundingRectangles(
        __in_ecount(1) const CContextState *pContextState,
        __in_ecount(1) BrushContext *pBrushContext,
        __in_ecount(1) const IShapeData *pShape,
        __in_ecount_opt(1) const CPlainPen *pPen,
        __in_ecount(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> &matWorldToDevice
        );

    void DbgDrawBox(
        __in_ecount(1) const CContextState *pContextState,
        __in_ecount(1) BrushContext *pBrushContext,
        __in_ecount(1) CRectF<CoordinateSpace::Device> *prcBox,
        __in_ecount(1) MilColorF *pColor
        );
#endif

private:
    HRESULT Setup3DRenderTargetAndDepthState(
        FLOAT rZ,
        bool fUseZBuffer,
        __inout_ecount(1) D3DMULTISAMPLE_TYPE &MultisampleType
        );

protected:
    //
    // Render target state
    //

    CMILSurfaceRect m_rcBoundsPre3D;
    bool m_fIn3D;
    bool m_fZBufferEnabled;

    //
    // D3D state
    //
    CD3DDeviceLevel1  * const m_pD3DDevice;

    CD3DSurface *m_pD3DTargetSurface;

    CD3DSurface *m_pD3DIntermediateMultisampleTargetSurface;

    CD3DSurface *m_pD3DTargetSurfaceFor3DNoRef;

    CD3DSurface *m_pD3DStencilSurface;

    //
    // Local copies of the surface information.
    //

    D3DFORMAT const m_d3dfmtTargetSurface;

#if DBG
private:
    void DbgResetStateUponTraceTag();
#else
    void DbgResetStateUponTraceTag() {}
#endif

#if DBG_STEP_RENDERING
protected:
    CHwDisplayRenderTarget *m_pDisplayRTParent;

public:

    override void DbgGetSurfaceBitmapNoRef(
        __deref_out_ecount_opt(1) IWGXBitmap **ppSurfaceBitmap
        ) const
    {
         *ppSurfaceBitmap = NULL; // DbgGetTargetSurface should be used instead
    }
    override void DbgGetTargetSurface(
        __deref_out_ecount_opt(1) CD3DSurface **ppD3DSurface
        ) const
    { 
        *ppD3DSurface = m_pD3DTargetSurface;
        m_pD3DTargetSurface->AddRef();
    }
    override UINT DbgTargetWidth() const { return m_uWidth; }
    override UINT DbgTargetHeight() const { return m_uHeight; }

    #define HW_DBG_RENDERING_STEP(func)                             \
        do {                                                        \
            if (m_pDisplayRTParent)                                 \
            {                                                       \
                m_pDisplayRTParent->ShowSteppedRendering(           \
                    TEXT(MILCORE_DLL) TEXT("!CHwSurfaceRenderTarget::")        \
                    TEXT(#func),                                    \
                    this);                                          \
            }                                                       \
        } while (UNCONDITIONAL_EXPR(0))
#else
    #define HW_DBG_RENDERING_STEP(func)
#endif DBG_STEP_RENDERING
};



