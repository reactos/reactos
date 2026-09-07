// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------

MtExtern(CDrawingContext);

//------------------------------------------------------------------------------
// Forward declarations
//------------------------------------------------------------------------------

class CMilVisual;
class CMilViewport3DVisual;
class CMilCameraDuce;
class CPreComputeContext;
class CMilGeometryDuce;
class CMilEffectDuce;

typedef CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::LocalRendering> CLocalRenderingMatrix;

#define MAX_EFFECT_SW_INTERMEDIATE_SIZE (1 << 14) // 16384
#define MAX_CACHE_SW_INTERMEDIATE_SIZE (1 << 14) // 16384

//---------------------------------------------------------------------------------
// CDrawingContext
//---------------------------------------------------------------------------------

class CDrawingContext :
    public IDrawingContext,
    public CMilSlaveResource,
    private IGraphIteratorSink
{
    friend class CPreComputeContext;

public:

    //
    // IDrawingContext interface.
    //
    virtual HRESULT DrawLine(
        __in_ecount(1) const MilPoint2D &point0,
        __in_ecount(1) const MilPoint2D &point1,
        __in_ecount_opt(1) CMilPenDuce *pPen,
        __in_ecount_opt(1) CMilSlavePoint *point0Animations,
        __in_ecount_opt(1) CMilSlavePoint *point1Animations
        );

    virtual HRESULT DrawRectangle(
        __in_ecount(1) const MilPointAndSizeD &rect,
        __in_ecount_opt(1) CMilPenDuce *pPen,
        __in_ecount_opt(1) CMilBrushDuce *pBrush,
        __in_ecount_opt(1) CMilSlaveRect *pRectAnimations
        );

    virtual HRESULT DrawRoundedRectangle(
        __in_ecount(1) const MilPointAndSizeD &rect,
        __in_ecount(1) const double &radiusX,
        __in_ecount(1) const double &radiusY,
        __in_ecount_opt(1) CMilPenDuce *pPen,
        __in_ecount_opt(1) CMilBrushDuce *pBrush,
        __in_ecount_opt(1) CMilSlaveRect *pRectangleAnimations,
        __in_ecount_opt(1) CMilSlaveDouble *pRadiusXAnimations,
        __in_ecount_opt(1) CMilSlaveDouble *pRadiusYAnimations
        );

    virtual HRESULT DrawEllipse(
        __in_ecount(1) const MilPoint2D &center,
        __in_ecount(1) const double &radiusX,
        __in_ecount(1) const double &radiusY,
        __in_ecount_opt(1) CMilPenDuce *pPen,
        __in_ecount_opt(1) CMilBrushDuce *pBrush,
        __in_ecount_opt(1) CMilSlavePoint *pCenterAnimations,
        __in_ecount_opt(1) CMilSlaveDouble *pRadiusXAnimations,
        __in_ecount_opt(1) CMilSlaveDouble *pRadiusYAnimations
        );

    virtual HRESULT DrawGeometry(
        __in_ecount_opt(1) CMilBrushDuce *pBrush,
        __in_ecount_opt(1) CMilPenDuce *pPen,
        __in_ecount_opt(1) CMilGeometryDuce *pGeometry
        );

    virtual HRESULT DrawImage(
        __in_ecount_opt(1) CMilSlaveResource *pImage,
        __in_ecount(1) const MilPointAndSizeD *prcDestinationBase,
        __in_ecount_opt(1) CMilSlaveRect *pDestRectAnimations
        );

    virtual HRESULT DrawVideo(
        __in_ecount(1) CMilSlaveVideo *pMediaClock,
        __in_ecount(1) const MilPointAndSizeD *prcDestinationBase,
        __in_ecount_opt(1) CMilSlaveRect *pDestRectAnimations
        );

    virtual HRESULT DrawGlyphRun(
        __in_ecount_opt(1) CMilBrushDuce *pBrush,
        __in_ecount_opt(1) CGlyphRunResource *pGlyphRun
        );
    
    virtual HRESULT DrawDrawing(
        __in_ecount_opt(1) CMilDrawingDuce *pDrawing
        );

    //
    // State stack.
    //
    virtual HRESULT PushClip(
        __in_ecount_opt(1) CMilGeometryDuce *pClipGeometry
        );

    virtual HRESULT PushImageEffect(
        __in_ecount_opt(1) CMilEffectDuce *pEffect,
        __in_ecount_opt(1) CRectF<CoordinateSpace::LocalRendering> const *prcBounds
        );

    virtual HRESULT Pop();

    virtual HRESULT PushOpacity(
        __in_ecount(1) const double &opacity,
        __in_ecount_opt(1) CMilSlaveDouble *pOpacityAnimation
        );

    virtual HRESULT PushOpacityMask(
        __in_ecount_opt(1) CMilBrushDuce *pOpacityMask,
        __in_ecount_opt(1) CRectF<CoordinateSpace::LocalRendering> const *prcBounds
        );

    virtual HRESULT PushTransform(
        __in_ecount_opt(1) CMilTransformDuce *pTransform
        );

    virtual HRESULT PushGuidelineCollection(
        __in_ecount_opt(1) CGuidelineCollection *pGuidelineCollection,
        __out_ecount(1) bool & fNeedMoreCycles
        );

    virtual HRESULT PushGuidelineCollection(
        __in_ecount_opt(1) CMilGuidelineSetDuce *pGuidelines
        );

    virtual void ApplyRenderState();


    VOID GetWorldTransform(__out_ecount(1) CMILMatrix *pMatrix);

    HRESULT PushEffects(
        __in_ecount(1) const DOUBLE &rOpacity,
        __in_ecount_opt(1) CMilGeometryDuce *pGeometryMask,
        __in_ecount_opt(1) CMilBrushDuce *pOpacityMaskBrush,
        __in_ecount_opt(1) CMilEffectDuce *pEffect,
        __in_ecount_opt(1) CRectF<CoordinateSpace::LocalRendering> const *pSurfaceBoundsLocalSpace
    );

    HRESULT PushRenderOptions(__in_ecount(1) const MilRenderOptions *pRenderOptions);

    VOID PopTransform();

    VOID PopGuidelineCollection();

    HRESULT PopEffects();

    HRESULT PopRenderOptions();


    HRESULT PushTransformPostOffset(
        float rPostOffsetX,
        float rPostOffsetY
        );
    
protected:

    CDrawingContext(
        __inout_ecount(1) CComposition * const pComposition
        );
    HRESULT Initialize();
    VOID Uninitialize();

    static VOID DbgReadControlFlags();

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CDrawingContext));
    virtual ~CDrawingContext();

public:

    static HRESULT Create(
        __inout_ecount(1) CComposition *pDevice,
        __deref_out_ecount(1) CDrawingContext **ppDrawingContext
        );


    __override virtual bool IsOfType(MIL_RESOURCE_TYPE type) const
    {
        return type == TYPE_DRAWINGCONTEXT;
    }

    __outro_xcount_opt(GetDirtyRegionCount()) const MilRectF *GetUninflatedDirtyRegions();
    UINT GetDirtyRegionCount() const;

    HRESULT Render(
        __in_ecount(1) CMilVisual *pRoot,
        __in_ecount(1) IMILRenderTarget *pIRenderTarget,
        __in_ecount(1) MilColorF const *pClearColor,
        __in_ecount(1) CMilRectF const &rcSurfaceBounds,
        BOOL fFullRender,
        UINT uNumInvalidTargetRegions,
        __in_ecount_opt(uNumInvalidTargetRegions) MilRectF const *rgInvalidTargetRegions,
        bool fCanAccelerateScroll,
        __out_ecount(1) BOOL *pfNeedsFullPresent
        );

    HRESULT Render3D(
        __in_ecount(1) CMilVisual3D *pRootVisual3D,
        __in_ecount(1) CMilCameraDuce *pCamera,
        __in_ecount(1) const MilPointAndSizeD *pViewport,
        __in_ecount(1) const CRectF<CoordinateSpace::LocalRendering> &rcBounds
        );

    //
    // Some utility functions to support nine-grid.
    //

    HRESULT GetState(__out_ecount(1) CRenderState **ppRenderState);

    //
    // Brush utility function.
    //

    HRESULT GetBrushRealizer(
        __in_ecount_opt(1) CMilSlaveResource *pBrush,
        __in_ecount(1) const BrushContext *pBrushContext,
        __deref_out_ecount(1) CBrushRealizer **ppBrushRealizer
        );

    //
    // 3DContext.
    //

    VOID Get3DBrushContext(
        __in_ecount(1) const CRectF<CoordinateSpace::BaseSampling> &rcBrushSizingBounds,
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::IdealSampling> *pmatWorldToIdealSampleSpace,
        __out_ecount(1) BrushContext **ppBrushContext
        );

    //
    // Utility function for bounds render pass check.
    //
    override BOOL IsBounding()
    {
        return (m_dwInternalRenderTargetType & BoundsRenderTarget);
    }


    HRESULT BeginFrame(
        __in_ecount(1) IMILRenderTarget *pIRenderTarget
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
        );

    VOID EndFrame(bool fNestedDrawingContext = false);

    __out_ecount(1) CMilVisual *GetCurrentVisual() const;

    HRESULT DrawVisualTree(
        __in_ecount(1) CMilVisual *pRoot,
        __in_ecount_opt(1) MilColorF const *pClearColor,
        __in_ecount(1) CMilRectF const &dirtyRect,
        bool fDrawingIntoVisualBrush = false
        );

    HRESULT DrawCacheVisualTree(
        __in_ecount(1) CMilVisual *pRoot,
        __in_ecount(1) MilColorF const *pClearColor,
        __in_ecount(1) CMilRectF const &dirtyRect,
        bool fDrawingIntoVisualBrush = true
        );

    HRESULT DrawBitmap(
        __in_ecount(1) CMilSlaveResource *pBitmap,
        MilBitmapWrapMode::Enum wrapMode
        );

    HRESULT DrawBitmap(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        __in_ecount(1) const MilRectF *prcSource,
        __in_ecount(1) const MilRectF *prcDest,
        float opacity
        );

    HRESULT DrawDrawing(
        __in_ecount(1) CMilDrawingDuce *pDrawing,
        __in_ecount(1) const CMilRectF *prcDest
        );

    HRESULT DrawShape(
        __in_ecount(1) IShapeData *pShapeData,
        __in_ecount_opt(1) CMilBrushDuce *pFill,
        __in_ecount_opt(1) CMilPenDuce *pPen
        );

    HRESULT DrawRectangle(
        __in_ecount(1) MilColorF const *pColor,
        __in_ecount(1) MilPointAndSizeF const *pRect
        );

    HRESULT FillShapeWithBitmap(
        __in_ecount(1) IWGXBitmapSource *pIWGXBitmapSource,
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::LocalRendering> *pTextureToLocalTransform,
        __in_ecount(1) CShapeBase *pShape,
        __in_ecount_opt(1) IMILEffectList *pEffectList,
        MilBitmapWrapMode::Enum wrapMode = MilBitmapWrapMode::Extend
        );

    static HRESULT GetBitmapSource(
        __in_ecount_opt(1) CMilSlaveResource *pImage,
        __out_ecount(1) CMilRectF *prcSrc,
        __deref_out_ecount_opt(1) IWGXBitmapSource **ppBitmapSource
        );

    static const MilBitmapInterpolationMode::Enum DefaultInterpolationMode = MilBitmapInterpolationMode::Linear;

protected:

    HRESULT PreCompute(
        __in_ecount(1) CMilVisual *pRoot,
        __in_ecount(1) const CMilRectF *prcSurfaceBounds,
        UINT uNumInvalidTargetRegions,
        __in_ecount_opt(uNumInvalidTargetRegions) MilRectF const *rgInvalidTargetRegions,
        float allowedDirtyRegionOverhead,
        BOOL fFullRender,
        __in_opt ScrollArea *pScrollArea
        );
    
    void GetClipBoundsWorld(__out_ecount(1) CRectF<CoordinateSpace::PageInPixels> *pClipBounds);

    VOID InvalidateClipRealization()
    {
        // Set the clip-changed flag so that the clip is realized during
        // the next call to ApplyRenderState.
        m_fClipChanged = true;
    }

    VOID InvalidateTransformRealization()
    {
        // Set the transform-changed flag so that the clip is realized during
        // the next call to ApplyRenderState.
        m_fTransformChanged = true;
    }

    // This function is necessary because of 2 hacks:
    //
    // 1) PushOpacity/PopOpacity is implemented in the context, not the render target
    // 2) We don't yet have a decent way to store/retrieve the clip realization.

    // This method addrefs the new RT.
    HRESULT ChangeRenderTarget(
        __in_ecount(1) IRenderTargetInternal *pirtNew
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
        );

    VOID ReleaseLayers();

    VOID    TemporarilySetWorldTransform(__in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> &matTempWorld);

    HRESULT PushTransformStackStateAndInvalidate();
    HRESULT PushClipStackState();

    HRESULT PushOffset(float offsetX, float offsetY);

public:
    CContentBounder *GetContentBounder()
    {
        return m_pContentBounder;
    }

    inline void UpdateDpiProvider(IDpiProvider* pDpiProvider)
    { 
        m_contextState.SetDpiProvider(pDpiProvider);
    }

    HRESULT PushTransform(__in_ecount(1) const CMILMatrix *pTransform, bool multiply = true);
    HRESULT PushClipRect(
        __in_ecount(1) CMilRectF const &clip
        );

    __out_xcount(m_renderedRegionCount) const CMilRectF* GetRenderedRegions(
        __out_ecount(1) UINT *renderedRegionCount
        ) const
    {
        *renderedRegionCount = m_renderedRegionCount;
        return m_renderedRegions;
    }

    void GetClippedWorldSpaceBounds(
        __in_ecount(1) CRectF<CoordinateSpace::LocalRendering> const * pBoundsInLocalSpace,
        __out_ecount(1) CRectF<CoordinateSpace::PageInPixels> * pAAInflatedClippedBoundsWorld
        );

protected:
    HRESULT PushExactClip(
        __in_ecount(1) const MilRectF &clip,
        BOOL fPushState
        );
    VOID PopClip(BOOL fPopState);

    HRESULT PushNoModificationLayer();
    HRESULT PushLayer(CLayer layer, __in_ecount_opt(1) const CRectF<CoordinateSpace::LocalRendering> *pSurfaceBoundsLocalSpace, bool fForceIntermediate = false);
    HRESULT PopLayer(__out_ecount(1) CLayer *pLayer);
    HRESULT DrawLayer(CLayer layer, __inout_ecount_opt(1) IMILEffectList *pEffectList);
    HRESULT DrawEffectLayer(CLayer layer);

    // IGraphIteratorSink interface implementation

    HRESULT PreSubgraph(__out_ecount(1) BOOL *pfVisitChildren);
    HRESULT PostSubgraph();

    HRESULT FillOrStrokeShape(
        BOOL fFillShape,
        __in_ecount(1) IShapeData *pShapeData,
        __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,
        __in_ecount(1) const CMilRectF *pWorldSpaceBounds,
        __in_ecount_opt(1) CPlainPen *pPlainPen,
        __in_ecount(1) CMilSlaveResource *pBrush
        );

    HRESULT GetStrokeBounds(
        __in_ecount(1) IShapeData const *pShapeData,
            // The shape for whose stroke we compute the bounds
        __in_ecount(1) CPlainPen const *pPen,
            // The stroking pen
        __out_ecount(1) CMilRectF &bounds) const;
            // The bounds

    HRESULT FillAndStrokeShapeForBounds(
        __in_ecount(1) IShapeData *pShapeData,
            // Shape to fill and stroke
        __in_ecount_opt(1) CMilBrushDuce *pBrush,
            // Fill brush resource
        __in_ecount_opt(1) CMilPenDuce *pPen);
            // Pen resource

    HRESULT CreateAndFillLayer(
        __in_ecount(1) CMilSlaveResource *pFillBrush,
        __in_ecount(1) const CRectF<CoordinateSpace::LocalRendering> *pSurfaceBoundsLocalSpace,
        __out_ecount(1) CLayer *pLayer
        );

    virtual MilAntiAliasMode::Enum GetDefaultAntiAliasMode()
    {
        return MilAntiAliasMode::EightByEight;
    }
        
private:
    HRESULT Begin3D(
        __in_ecount(1) const CRectF<CoordinateSpace::LocalRendering> &rcViewportRect,
        __in_ecount(1) const CRectF<CoordinateSpace::LocalRendering> &rcBounds
        );

    HRESULT End3D();
    
    HRESULT DrawRectangleOverlay(
        __in_ecount(1) CMilRectF const *renderBounds
    );

    enum EffectCompositionMode
    {
        RenderCompatible,
        PushDummyAndRenderSoftware,
        SkipRender
    };

    static void CheckEffectSupport(__out bool *pHasHardwareSupport, __out bool *pHasSoftwareSupport, __in bool isPS30);

    HRESULT DetermineEffectCompositionMode(
        __in CMilEffectDuce *pEffect,
        __out EffectCompositionMode *pCompositionMode
        );

    bool CanUseCacheAsEffectInput(
        __in_ecount(1) CMilVisual const *pNode, 
        __in_ecount(1) CRectF<CoordinateSpace::LocalRendering> const *prcBounds
        );

    HRESULT DrawEffect(
        __in_ecount(1) CMilVisual const *pNode,
        __in_ecount(1) CRectF<CoordinateSpace::LocalRendering> const *prcBounds
        );
    
    static HRESULT SetupEffectTransform(
        __in_ecount(1) CMilEffectDuce *pEffect,
        __in_ecount(1) const CRectF<CoordinateSpace::LocalRendering> *pSurfaceBoundsLocalSpace,
        __in_ecount(1) CRectF<CoordinateSpace::PageInPixels> *prcClip,
        __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pWorldTransform,
        __out_ecount(1) CMILMatrix &scaleMatrix,
        __out_ecount(1) CMILMatrix &restMatrix,
        __out_ecount(1) CRectF<CoordinateSpace::PageInPixels> &surfaceBoundsWorldSpace
        );

    HRESULT CalculateEffectTextureLimits(
        __in UINT uTextureWidthIn,
        __in UINT uTextureHeightIn,
        __out UINT &uTextureWidthOut,
        __out UINT &uTextureHeightOut,
        __out float &uScaleX,
        __out float &uScaleY
        );

    HRESULT PushDummyLayer(
        __in CRectF<CoordinateSpace::LocalRendering> *pBounds
        );

    HRESULT PopLayerIfDummy();

protected:

    //
    // This is the composition partition in which this CDrawingContext is used
    // It is used to create the CContentBounder and CPreComputeContext and
    // to get access to the schedule manager
    //
    
    CComposition * m_pComposition;

    //
    // MIL rendering factory should be used when creating MIL rendering
    // objects.
    //

    CMILFactory * const m_pFactory;

    // The current state used for rendering. We should consider
    // combining those structures.
    CRenderState  m_renderState;
    CContextState m_contextState;   // DrawPath

    // Cached context information needed to create brush realizations.
    BrushContext m_brushContext;
    BrushContext m_3DBrushContext;
    
    // Current render target.
    IRenderTargetInternal *m_pIRenderTarget;

    // State used to implement the "state stack".
    //
    // At the interface level, there is one conceptual stack. We currently use
    // multiple stacks internally to implement this. If this doesn't give good
    // enough performance/working set, we could implement it as a single,
    // polymorphic stack.

    enum StackStateType
    {
        StackStateTypeClip,
        StackStateTypeTransform,
        StackStateTypeGuidelineCollection,
        StackStateTypeBitmapLayer,
        StackStateTypeRTLayer,
        StackStateTypeRenderOptions,
        StackStateTypeNoModification,
    };
    CWatermarkStack<
        StackStateType,
        64 /* MinCapacity */,
        2 /* GrowFactor */,
        10 /* TrimCount */
        >
        m_stateTypeStack;

    CMatrixStack<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> m_transformStack;

    CWatermarkStack<
        CLayer,
        64 /* MinCapacity */,
        2 /* GrowFactor */,
        10 /* TrimCount */
        >
        m_layerStack;

    struct SavedRenderOptions
    {
        bool                             PrefilterEnable;
        bool                             ClearTypeHint;
        MilAntiAliasMode::Enum           AntiAliasMode;
        MilBitmapInterpolationMode::Enum InterpolationMode;
        MilCompositingMode::Enum         CompositingMode;
        MilTextRenderingMode::Enum       TextRenderingMode;
        MilTextHintingMode::Enum         TextHintingMode;    
    };
    CWatermarkStack<
        SavedRenderOptions,
        64 /* MinCapacity */,
        2 /* GrowFactor */,
        10 /* TrimCount */
        >
        m_renderOptionsStack;

    CGenericClipStack m_clipStack;


    // This tells us whether or not we can skip things like Brush realization
    // It is set initially, and then on each render-target update
    DWORD m_dwInternalRenderTargetType;

    //
    // m_pCachedNullBrush is returned during GetBrush for render targets
    // that don't require brushes (NULL is passed to draw path)
    //
    CBrushRealizer *m_pCachedNullBrushRealizer;

    CGraphIterator *m_pGraphIterator;

    // Cached object that retrieves bounds of content for alpha mask effects.
    //
    // NOTE:  If GetDrawingBounds could be called recursively we would need
    // to create a seperate CContentBounder instance for each render data,
    // instead of using this cached instance.
    //
    // This isn't needed today because GetDrawingBounds is only called
    // when popping an alpha mask effect, which currently only exist once per
    // visual (and not multiple times within content).
    CContentBounder *m_pContentBounder;

    //
    // Precompute context for the tree walk.
    //

    CPreComputeContext *m_pPreComputeContext;

    //
    //  Context walkers for 3D content
    //

    CPrerender3DContext  *m_pPrerender3DContext;
    CRender3DContext     *m_pRender3DContext;


    // DBG-only variables used to guard against mismatched stack operations.
    //
    // These variables ensure that stack operations that occur between the
    // BeginFrame and EndFrame calls are properly matched.
#ifdef DBG
    UINT m_uBeginFrameTransformStackCount;
    UINT m_uBeginFrameClipStackCount;
    UINT m_uBeginFrameLayerStackCount;
    UINT m_uBeginFrameStackTypeStackCount;
#endif

    //
    // Scratch Bitmap brush for use when a draw call needs to
    // create a brush but the input only has a bitmap.
    //
    CMILBrushBitmap *m_pScratchBitmapBrush;
    
    //
    // Regions that have been rendered this frame.
    //
    CMilRectF m_renderedRegions[CDirtyRegion2::MaxDirtyRegionCount];
    UINT m_renderedRegionCount;

    // Flags
    bool m_fTransformChanged            : 1;
    bool m_fClipChanged                 : 1;
    bool m_fDrawingIntoVisualBrush      : 1;
    bool m_fClearTypeHint               : 1;

    MilTextRenderingMode::Enum m_textRenderingMode;
    MilTextHintingMode::Enum m_textHintingMode;

#if DBG_ANALYSIS
    bool m_fDbgTargetSpaceChanged       : 1;

    CoordinateSpaceId::Enum m_dbgTargetCoordSpaceId;
#endif
};


