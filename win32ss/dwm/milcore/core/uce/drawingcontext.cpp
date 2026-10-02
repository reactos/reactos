// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------------
//

//
//---------------------------------------------------------------------------------

#include "precomp.hpp"

DeclareTag(tagTintPushOpacitySurfaces, "MIL", "Tint PushOpacity intermediate surfaces");
MtDefine(CDrawingContext, MILRender, "CDrawingContext");

//---------------------------------------------------------------------------------
// Dirty region control/debug flags
//---------------------------------------------------------------------------------

// Enables/Disables dirty region support.
BOOL g_fDirtyRegion_Enabled = TRUE;

// Clears the back-buffer before each update. That allows you to check which parts
// are getting re-rendered.
BOOL g_fDirtyRegion_ClearBackBuffer = FALSE;

// If the following flag is set, the content updates are highlighted by rendering
// the dirty region in a translucent color.
BOOL g_fDirtyRegion_ShowDirtyRegions = FALSE;

const UINT g_DirtyRegionColorCount = 3;

//
// Debug flags to turn DrawBitmap calls translucent.
BOOL g_fTranslucent_DrawBitmap = FALSE;
float g_fTranslucent_Draw_Scale = 0.5f;

MilColorF g_DirtyRegionColors[g_DirtyRegionColorCount] =
    { /* {r, g, b, a} */
    { 0.7f, 0.7f, 0.7f, 0.7f },
    { 0.7f, 0, 0.7f, 0.7f },
    { 0.7f, 0.7f, 0, 0.7f }};

UINT g_DirtyRegionColor = 0;

//---------------------------------------------------------------------------------
// CDrawingContext::ctor
//---------------------------------------------------------------------------------

CDrawingContext::CDrawingContext(
    __inout_ecount(1) CComposition * const pComposition
    )
    : m_pComposition(pComposition),
      m_pFactory(pComposition->GetMILFactory())
{
    m_pComposition->AddRef();
    m_brushContext.pBrushDeviceNoRef = pComposition;
    m_brushContext.fBrushIsUsedFor3D = false;
    m_brushContext.fRealizeProceduralBrushesAsIntermediates = FALSE;
    m_brushContext.pRenderTargetCreator = NULL;

    m_3DBrushContext.pBrushDeviceNoRef = pComposition;
    m_3DBrushContext.rcSampleSpaceClip = CMilRectF::sc_rcInfinite;
    m_3DBrushContext.fBrushIsUsedFor3D = true;
    m_3DBrushContext.fRealizeProceduralBrushesAsIntermediates = TRUE;
    m_3DBrushContext.pRenderTargetCreator = NULL;

    InvalidateTransformRealization();
    InvalidateClipRealization();

    WHEN_DBG_ANALYSIS(m_dbgTargetCoordSpaceId = CoordinateSpaceId::Invalid);

    m_pScratchBitmapBrush = NULL;

    m_renderedRegionCount = 0;

    for (UINT i = 0; i < CDirtyRegion2::MaxDirtyRegionCount; i++)
    {
        m_renderedRegions[i] = CMilRectF::sc_rcEmpty;
    }

    m_fClearTypeHint = false;
}

//---------------------------------------------------------------------------------
// CDrawingContext::Initialize
//---------------------------------------------------------------------------------

HRESULT
CDrawingContext::Initialize()
{
    HRESULT hr = S_OK;
    
    // Cache a NULL immedate brush used by render targets that don't require brushes
    // and for draw instructions that have no brush specified
    //
    // Initialize shouldn't be called twice (successfully).  We assume this so we don't
    // have to check for non-NULL & release the member variables.
    Assert(!m_pCachedNullBrushRealizer);  // Guard the assumption that Initialize isn't called twice
    IFC(CBrushRealizer::CreateNullBrush(&m_pCachedNullBrushRealizer));

    // Create graph iterator

    Assert(!m_pGraphIterator); // Initialize shouldn't be called twice
    m_pGraphIterator = new CGraphIterator();
    IFCOOM(m_pGraphIterator);

    Assert(!m_pContentBounder); // Initialize shouldn't be called twice
    IFC(CContentBounder::Create(m_pComposition, &m_pContentBounder));
    m_brushContext.pContentBounder = m_pContentBounder;
    m_3DBrushContext.pContentBounder = m_pContentBounder;

    //
    // Set a default pDisplaySettings struct so that even if this CDrawingContext is used outside the
    // scope of a CMetaRenderTarget, the drawing code can still access correct display settings.
    //
    m_contextState.GetCurrentOrDefaultDisplaySettings();

Cleanup:
    if (FAILED(hr))
    {
        // Release cached members upon failure
        ReleaseInterface(m_pCachedNullBrushRealizer);
        ReleaseInterface(m_pIRenderTarget);
        delete m_pGraphIterator;
        m_pGraphIterator = NULL;
        delete m_pContentBounder;
        m_pContentBounder = NULL;
        m_brushContext.pContentBounder = NULL;
        m_3DBrushContext.pContentBounder = NULL;

    }
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CDrawingContext::Uninitialize
//---------------------------------------------------------------------------------
void
CDrawingContext::Uninitialize()
{
    ReleaseLayers();
    ReleaseInterface(m_pCachedNullBrushRealizer);
    ReleaseInterface(m_pIRenderTarget);
    delete m_pGraphIterator;
    m_pGraphIterator = NULL;
    delete m_pContentBounder;
    m_pContentBounder = NULL;
}

//---------------------------------------------------------------------------------
// CDrawingContext::BeginFrame
//---------------------------------------------------------------------------------

HRESULT
CDrawingContext::BeginFrame(
    __in_ecount(1) IMILRenderTarget* pIRenderTarget
    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
    )
{
    Assert(pIRenderTarget);

    HRESULT hr = S_OK;
    IRenderTargetInternal* pInteralRenderTarget = NULL;

    // Set the new render target
    IFC(pIRenderTarget->QueryInterface(IID_IRenderTargetInternal, (void **) &pInteralRenderTarget));
    Assert(pInteralRenderTarget);

    IFC(ChangeRenderTarget(pInteralRenderTarget DBG_ANALYSIS_COMMA_PARAM(dbgTargetCoordSpaceId)));

    // Set the render & context states
    m_renderState.InterpolationMode = DefaultInterpolationMode;
    m_renderState.AntiAliasMode = GetDefaultAntiAliasMode();
    m_contextState.RenderState = &m_renderState;

    // Set current time to context

    if (m_pComposition)
    {
        m_contextState.CurrentTime = m_pComposition->GetScheduleManager()->GetCurrentTime();
    }

#if DBG
    // Save the current stack depths during DBG builds so that we can
    // guard against mismatched stack operation in EndFrame
    m_uBeginFrameTransformStackCount = m_transformStack.GetSize();
    m_uBeginFrameClipStackCount = m_clipStack.GetSize();
    m_uBeginFrameLayerStackCount = m_layerStack.GetSize();
    m_uBeginFrameStackTypeStackCount = m_stateTypeStack.GetSize();
#endif

Cleanup:
    ReleaseInterface(pInteralRenderTarget);
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CDrawingContext::EndFrame
//---------------------------------------------------------------------------------

VOID
CDrawingContext::EndFrame(bool fNestedDrawingContext)
{
#if NEVER
    // Disabling this assert since it causes problems when we get failures during normal
    // composition. See also bug # 1131130.


    // Guard against mismatched stack operations
    Assert(m_uBeginFrameTransformStackCount == m_transformStack.GetSize());
    Assert(m_uBeginFrameClipStackCount == m_clipStack.GetSize());
    Assert(m_uBeginFrameLayerStackCount == m_layerStack.GetSize());
    Assert(m_uBeginFrameStackTypeStackCount == m_stateTypeStack.GetSize());
#endif


    m_transformStack.Clear();
    m_transformStack.Optimize();
    m_clipStack.Clear();
    m_clipStack.Optimize();
    m_stateTypeStack.Clear();
    m_stateTypeStack.Optimize();
    ReleaseLayers();
    m_layerStack.Clear();
    m_layerStack.Optimize();
    m_renderOptionsStack.Clear();
    m_renderOptionsStack.Optimize();

    // This can only be correctly called after ReleaseLayers, because
    // ReleaseLayers may change m_pIRenderTarget.  Note EndAndIgnoreAllLayers
    // is not required for other RT's because they should all just be released.
    // NOTE: This is not done on nested drawing contexts because the render target
    //       will still be in use by the outer drawing context.
    if (m_pIRenderTarget && !fNestedDrawingContext)
    {
        m_pIRenderTarget->EndAndIgnoreAllLayers();
    }
}


//---------------------------------------------------------------------------------
// CDrawingContext::GetCurrentVisual
//---------------------------------------------------------------------------------

__out_ecount(1) CMilVisual *
CDrawingContext::GetCurrentVisual(
    ) const
{
    return DYNCAST(CMilVisual, m_pGraphIterator->CurrentNode());
}


//+---------------------------------------------------------------------------
//
//  Member:
//      CDrawingContext::Begin3D
//
//  Synopsis:
//      Prepares the CContextState of this CDrawingContext for rendering
//      3D content.  This includes initializing the WorldToDevice transform,
//      reseting the lights, clearing the Z-Buffer, etc.
//
//  Notes:
//      Each call to Begin3D should be paired with a call to End3D.
//
//  Returns:
//      S_OK on success, else failed HR.
//
//----------------------------------------------------------------------------

HRESULT
CDrawingContext::Begin3D(
    __in_ecount(1) const CRectF<CoordinateSpace::LocalRendering> &rcViewportRect,
    __in_ecount(1) const CRectF<CoordinateSpace::LocalRendering> &rcBoundsNode
    )
{
    HRESULT hr;

    CRectF<CoordinateSpace::PageInPixels> rcBounds;

    CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels>::Transform2DBoundsNullSafe(
        m_transformStack.GetTopByReference(),
        rcBoundsNode,
        OUT rcBounds
        );

    // set to infinite bounds if rcBounds has NaN
    if (!(rcBounds.IsWellOrdered()))
    {
        rcBounds.SetInfinite();
    }

    Assert(!m_contextState.In3D);

    ApplyRenderState();

    CalcHomogeneousClipTo2D(
        rcViewportRect,
        m_contextState.WorldToDevice,
        OUT m_contextState.ViewportProjectionModifier3D
        );

    this->m_contextState.LightData.Reset();

    // Clear the z-buffer
    IFC(m_pIRenderTarget->Begin3D(
        rcBounds,
        m_renderState.AntiAliasMode,
        /* fUseZBuffer = */ true,
        1.0
        ));

    m_contextState.In3D = true;

    // Setup up other state
    m_contextState.DepthBufferFunction3D = D3DCMP_LESSEQUAL;

    // Reset the world transform
    m_contextState.WorldTransform3D.SetToIdentity();

Cleanup:
    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CDrawingContext::End3D
//
//  Synopsis:
//      Each call to Begin3D should be paired with a call to End3D before
//      this CDrawingContext is used to render 2D.
//
//----------------------------------------------------------------------------

HRESULT
CDrawingContext::End3D()
{
    HRESULT hr = S_OK;

    if (m_contextState.In3D)
    {
        m_contextState.In3D = false;
        MIL_THR(m_pIRenderTarget->End3D());
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CDrawingContext::Create
//---------------------------------------------------------------------------------

HRESULT
CDrawingContext::Create(
    __inout_ecount(1) CComposition *pDevice,
    __deref_out_ecount(1) CDrawingContext **ppDrawingContext
    )
{
    HRESULT hr = S_OK;

    CDrawingContext *pDrawingContext = new CDrawingContext(pDevice);
    IFCOOM(pDrawingContext);

    pDrawingContext->AddRef();

    IFC(pDrawingContext->Initialize());

    *ppDrawingContext = pDrawingContext;

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterfaceNoNULL(pDrawingContext);
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CDrawingContext::dtor
//---------------------------------------------------------------------------------

CDrawingContext::~CDrawingContext()
{
    Uninitialize();
    ReleaseInterface(m_pComposition);
    ReleaseInterface(m_pScratchBitmapBrush);

    delete m_pPreComputeContext;
    delete m_pPrerender3DContext;
    delete m_pRender3DContext;

    ReleaseInterfaceNoNULL(m_pFactory);
}

//+-----------------------------------------------------------------------------
//
//  Member:    ReleaseLayers
//
//  Synopsis:  Releases all temporary layers, resetting m_pIRenderTarget
//             to the original RT.
//
//------------------------------------------------------------------------------

VOID CDrawingContext::ReleaseLayers()
{
    while (!m_layerStack.IsEmpty())
    {
        CLayer layer;
        m_layerStack.Pop(&layer);

        Assert(layer.prtTargetPrev);
        IGNORE_HR(ChangeRenderTarget(layer.prtTargetPrev DBG_ANALYSIS_COMMA_PARAM(layer.dbgTargetPrevCoordSpaceId)));
        ReleaseInterfaceNoNULL(layer.prtTargetPrev);
        delete layer.pGeometricMaskShape;
        //   Is pAlphaMaskBrush ref counted by CLayer
        //  or is the lifetime managed in some other way that may or may not
        //  leak.
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::ApplyRenderState
//
//  Synopsis:   Applies the top of the transform & clip stacks so that they
//              are taken into account when rendering.
//
//  Notes:
//              The top of these stacks are lazily applied by Push*, instead
//              of immediately applied, because realizing clip's is expensive.
//              It was found during performance analysis that visuals that
//              contained clips but no content were common.  This
//              implementation optimizes that scenario by deferring clip
//              realization until content is actually rendered.
//
//------------------------------------------------------------------------------
void
CDrawingContext::ApplyRenderState()
{
    Assert(   (m_dbgTargetCoordSpaceId == CoordinateSpaceId::PageInPixels)
           || (m_dbgTargetCoordSpaceId == CoordinateSpaceId::Device));

    //
    // Apply world->device transform changes
    //

    if (m_fTransformChanged)
    {
        // Cache world transform in render state
        m_transformStack.Top(&
            static_cast<CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> &>
            (m_contextState.WorldToDevice));

        m_fTransformChanged = false;

#if DBG_ANALYSIS
        //
        // Low level render targets always work in device space and assert that
        // they have been given proper transforms.  Normally the meta render
        // target handles conversion from PageInPixels to Device.  But it can't
        // do the conversion if it is not used.  There are just a few cases
        // when the meta RT is not in the call stack.  There is no meta render
        // target when these methods are used to create the render target:
        //      CTileBrushUtils::CreateTileBrushIntermediate
        //      CMILFactory::CreateBitmapRenderTarget
        //      CMILFactory::CreateSWRenderTargetForBitmap
        // Only the first two are use in WPF.
        // CTileBrushUtils::CreateTileBrushIntermediate actually uses
        // CreateRenderTargetBitmap, but does so after sneaking below meta
        // level to low level RT and thus get a low level RT out.
        // CMILFactory::CreateBitmapRenderTarget is used from managed code and
        // is passed to composition as a generic slave render target
        // (printtarget.cpp).
        //
        // Rather than create a meta like wrapper around these render targets
        // CDrawingContext simply keeps track of whether it is configured to
        // directly address Device space or is going through a meta level and
        // is addressing PageInPixels.  This is also useful debug data on its
        // own, but doesn't provide much value to composition level in general
        // since when using a low level RT PageInPixels to Device transform is
        // simply identity and everything can work in that one space.
        //
        // So before calling core rendering level, adjust the "WorldToDevice"
        // transform to to have its out space be Device as needed.  The cast
        // and write from Top above sets the Out space to PageInPixels.
        //
        if (m_dbgTargetCoordSpaceId == CoordinateSpaceId::Device)
        {
            m_contextState.WorldToDevice.DbgChangeToSpace<CoordinateSpace::PageInPixels,CoordinateSpace::Device>();
        }

        m_fDbgTargetSpaceChanged = false;
    }
    else if (m_fDbgTargetSpaceChanged)
    {
        //
        // If the current render target has changed and it has a different
        // required coordinate space update the "WorldToDevice" transform.
        // See above comments about the need for this.
        //
        if (m_dbgTargetCoordSpaceId == CoordinateSpaceId::Device)
        {
            m_contextState.WorldToDevice.DbgChangeToSpace<CoordinateSpace::PageInPixels,CoordinateSpace::Device>();
        }
        else
        {
            m_contextState.WorldToDevice.DbgChangeToSpace<CoordinateSpace::Device,CoordinateSpace::PageInPixels>();
        }

        m_fDbgTargetSpaceChanged = false;
#endif
    }

    //
    // Apply clip changes
    //

    if (m_fClipChanged)
    {
        CMilRectF deviceClipRect;

        //
        // Apply clip changes to brush context
        //

        m_clipStack.Top(&deviceClipRect);

        //
        // Apply clip changes to render state
        //

        m_contextState.AliasedClip = CAliasedClip(&deviceClipRect);

        m_fClipChanged = false;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:    GetClipBoundsWorld()
//
//  Synopsis:  Get the ClipBounds
//
//------------------------------------------------------------------------------

void
CDrawingContext::GetClipBoundsWorld(
    __out_ecount(1) CRectF<CoordinateSpace::PageInPixels> *pClipBounds
    )
{
    m_clipStack.Top(pClipBounds);
}

//+-----------------------------------------------------------------------------
//
//  Member:    TemporarilySetWorldTransform
//
//  Synopsis:  Set the world transform to given. The next call to
//             ApplyRenderState will reset it to the top of the transform stack.
//
//------------------------------------------------------------------------------

VOID
CDrawingContext::TemporarilySetWorldTransform(
    __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> &matTempWorld
    )
{
    m_contextState.WorldToDevice = matTempWorld;
#if DBG_ANALYSIS
    if (m_dbgTargetCoordSpaceId == CoordinateSpaceId::Device)
    {
        m_contextState.WorldToDevice.DbgChangeToSpace<CoordinateSpace::PageInPixels,CoordinateSpace::Device>();
    }
#endif
    InvalidateTransformRealization();
}

//---------------------------------------------------------------------------------
//  Draw a line.
//
// Arguments:
//
//    point 0 - First point
//    point 1 - Second point
//
//    pPen    - Pen to draw the line with
//
//    point0Animations - Animation property for point0
//    point1Animations - Animation property for point1
//
// Return Value:
//
//    HRESULT
//
//---------------------------------------------------------------------------------

HRESULT CDrawingContext::DrawLine(
    __in_ecount(1) const MilPoint2D &point0,
    __in_ecount(1) const MilPoint2D &point1,
    __in_ecount_opt(1) CMilPenDuce *pPen,
    __in_ecount_opt(1) CMilSlavePoint *point0Animations,
    __in_ecount_opt(1) CMilSlavePoint *point1Animations
    )
{
    HRESULT hr = S_OK;

    // Current value of the line
    CLine line;

    //
    // Obtain the current value of the line
    //

    IFC(SetLineCurrentValue(
        &point0,
        point0Animations,
        &point1,
        point1Animations,
        &line
        ));

    //
    // Draw the current value of the line
    //

    IFC(DrawShape(
        &line,
        NULL,  // Lines do not have a fill brush
        pPen
        ));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
//  Draw a Rectangle.
//
// Arguments:
//
//    rect - Rectangle to draw.
//
//    pPen    - Pen to draw the line with
//    pBrusn  - Brush to fill rect with
//
//    pRectAnimations - Animation property for rect
//
// Return Value:
//
//    HRESULT
//
//---------------------------------------------------------------------------------

HRESULT CDrawingContext::DrawRectangle(
    __in_ecount(1) const MilPointAndSizeD &rect,
    __in_ecount_opt(1) CMilPenDuce *pPen,
    __in_ecount_opt(1) CMilBrushDuce *pBrush,
    __in_ecount_opt(1) CMilSlaveRect *pRectAnimations
    )
{
    HRESULT hr = S_OK;

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_RectangleStart(0);
    }

    // Current values of the rectangle parameters
    MilPointAndSizeF rectCurrentValue;

    MilPointAndSizeD rectBaseValue = rect;

    //
    // Obtain the current value of the rectangle.
    // Since this is a non-rounded rectangle, we can use the efficient
    // CParaellogram implmentation of IShapeData.
    //

    IFC(GetRectCurrentValue(
        &rectBaseValue,
        pRectAnimations,
        &rectBaseValue
        ));
    MilPointAndSizeFFromMilPointAndSizeD(&rectCurrentValue, &rectBaseValue);

    // Unlike CShape, CRectangle cannot handle Rect.Empty, so this check is required
    if (!IsRectEmptyOrInvalid(&rectCurrentValue))
    {
        CRectangle rectangle;
        IFC(rectangle.Set(rectCurrentValue, 0.0f /* radius */));

        //
        // Draw the current value of the rectangle
        //
        IFC(DrawShape(
            &rectangle,
            pBrush,
            pPen
            ));
    }

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_RectangleEnd(0);
    }

Cleanup:
    RRETURN(hr);
}


//---------------------------------------------------------------------------------
//  Draw a RoundedRectangle.
//
// Arguments:
//
//      rect - The rectangle to draw
//      radiusX - X-radius of the corner (elliptical arc)
//      radiusY - Y-radius of the corner (elliptical arc)
//      pPen - Pen to draw outline wtih
//      pBrush - Brush to fill rectangle with
//      pRectangleAnimations - Animations for the rectangle itself
//      pRadiusXAnmations - Animations for the X-radius
//      pRadiusYAnmations - Animations for the Y-radius
//
// Return Value:
//
//    HRESULT
//
//---------------------------------------------------------------------------------

HRESULT CDrawingContext::DrawRoundedRectangle(
    __in_ecount(1) const MilPointAndSizeD &rect,
    __in_ecount(1) const double &radiusX,
    __in_ecount(1) const double &radiusY,
    __in_ecount_opt(1) CMilPenDuce *pPen,
    __in_ecount_opt(1) CMilBrushDuce *pBrush,
    __in_ecount_opt(1) CMilSlaveRect *pRectangleAnimations,
    __in_ecount_opt(1) CMilSlaveDouble *pRadiusXAnimations,
    __in_ecount_opt(1) CMilSlaveDouble *pRadiusYAnimations
    )
{
    HRESULT hr = S_OK;

    // Current value of the rectangle
    CShapeBase *pShape;
    CRectangle roundRect;
    CShape shape;

    MilPointAndSizeF rectCurrentValue;
    FLOAT radiusXCurrentValue, radiusYCurrentValue;

    //
    // Obtain the current value of the rounded rectangle
    // Since this is a rounded rectangle, we cannnot use the more efficient
    // CParaellogram implmentation of IShapeData, and must use the more general CShape
    //

    IFC(GetRectangleCurrentValue(
        &rect,
        pRectangleAnimations,
        radiusX,
        pRadiusXAnimations,
        radiusY,
        pRadiusYAnimations,
        &rectCurrentValue,
        &radiusXCurrentValue,
        &radiusYCurrentValue
        ));

    //
    // Unlike CShape, CRectangle cannot handle Rect.Empty,
    // so check for empty.
    //

    if (!IsRectEmptyOrInvalid(&rectCurrentValue) &&
            (radiusXCurrentValue == radiusYCurrentValue))
    {
        IFC(roundRect.Set(
            rectCurrentValue,
            radiusXCurrentValue));

        pShape = &roundRect;
    }
    else
    {
        IFC(shape.AddRoundedRectangle(
                rectCurrentValue,
                radiusXCurrentValue,
                radiusYCurrentValue));

        pShape = &shape;
    }

    //
    // Draw the current value of the rounded rectangle
    //

    IFC(DrawShape(
        pShape,
        pBrush,
        pPen
        ));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
//  Draw an Ellipse.
//
// Arguments:
//
//      center - Center point of the ellipse
//      radiusX - X direction radius
//      radiusY - Y direction radius
//      pPen - Pen to draw outline with
//      pBrush - Brush to fill with
//      pCenterAnimations - Animations of center point
//      pRadiusXAnimations - Animation of X radius
//      pRadiusYAnimations - Animation of Y radius
//
// Return Value:
//
//    HRESULT
//
//---------------------------------------------------------------------------------

HRESULT CDrawingContext::DrawEllipse(
    __in_ecount(1) const MilPoint2D &center,
    __in_ecount(1) const double &radiusX,
    __in_ecount(1) const double &radiusY,
    __in_ecount_opt(1) CMilPenDuce *pPen,
    __in_ecount_opt(1) CMilBrushDuce *pBrush,
    __in_ecount_opt(1) CMilSlavePoint *pCenterAnimations,
    __in_ecount_opt(1) CMilSlaveDouble *pRadiusXAnimations,
    __in_ecount_opt(1) CMilSlaveDouble *pRadiusYAnimations
    )
{
    HRESULT hr = S_OK;

    CShape shape;

    //
    // Obtain the current value of the ellipse
    //

    IFC(AddEllipseCurrentValueToShape(
        &center,
        pCenterAnimations,
        radiusX,
        pRadiusXAnimations,
        radiusY,
        pRadiusYAnimations,
        &shape
        ));

    //
    // Draw the current value of the ellipse
    //

    IFC(DrawShape(
        &shape,
        pBrush,
        pPen
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::DrawGeometry
//
//      pBrush - Brush to fill geometry with
//      pPen - Pen to draw outline of geometry with
//      pGeometry - Geometry to draw
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::DrawGeometry(
    __in_ecount_opt(1) CMilBrushDuce *pBrush,
    __in_ecount_opt(1) CMilPenDuce *pPen,
    __in_ecount_opt(1) CMilGeometryDuce *pGeometry
    )
{
    HRESULT hr = S_OK;

    IShapeData *pShapeData = NULL;

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_GeometryStart(0);
    }

    //
    // Get the current value of the geometry
    //

    IFC(GetGeometryCurrentValue(
        pGeometry,
        &pShapeData
        ));

    //
    // Call DrawShape, if a shape exists
    //

    if (pShapeData)
    {
        // Draw the shape
        IFC(DrawShape(pShapeData, pBrush, pPen));
    }

Cleanup:
    //
    // Future Consideration:  This filter *should* no longer be needed,
    // as we are now explicitly checking for BADNUMBER lower down in the stack.
    // Consider removing this check.
    //
    if (hr == WGXERR_BADNUMBER)
    {
        //
        // We encountered a numerical error when drawing this geometry.  This
        // isn't a big deal -- we'll ignore this geometry and continue on.
        //
        hr = S_OK;
    }
    
    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_GeometryEnd(0);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Method:     CDrawingContext::GetStrokeBounds
//
//  Synopsis:   Get the bounds of the stroke of a given shape with a given pen
//
//  Notes:      The error tolerance is adjusted to the render target's resolution
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::GetStrokeBounds(
    __in_ecount(1) IShapeData const *pShapeData,
        // The shape for whose stroke we compute the bounds
    __in_ecount(1) CPlainPen const *pPen,
        // The stroking pen
    __out_ecount(1) CMilRectF &bounds) const
        // The bounds
{
    HRESULT hr;

    //
    // Tolerance for bounds computation.  The bounds are computed in world space, so a for
    // the tolerance to be suitable for the rendering resolution, it needs to be adjusted by
    // the maximal magnification factor of the world to device transform.
    //
    // Dividing by 0 will produce Infinity.  Since the only operations with the tolerance
    // downstream are squaring and comparison, Infinity is OK
    //

    double tolerance = DEFAULT_FLATTENING_TOLERANCE / m_contextState.WorldToDevice.GetMaxFactor();

    if (pPen->GetDashStyle() == MilDashStyle::Solid)
    {
        // The pen is solid, we can use the original pen for computing the bounds
        IFC(pShapeData->GetTightBoundsNoBadNumber(
                OUT bounds,
                pPen,
                NULL /* identity matrix */,
                tolerance));
    }
    else
    {
        //
        // The pen has dashes, whose animation may cause the bounds to change and the brush to
        // jitter.  To prevent that, we use a solid copy of the pen for computing the bounds
        //

        CPlainPen solid(*pPen);
        IFC(pShapeData->GetTightBoundsNoBadNumber(
                OUT bounds,
                &solid,
                NULL /* identity matrix */,
                tolerance));
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Function:   GetPenAndItsBrush
//
//  Synopsis:   Retrieves the pen and brush from the resource
//
//------------------------------------------------------------------------------
HRESULT
GetPenAndItsBrush(
    __in_ecount(1) CMilPenDuce *pPen,
        // Pen resource
    __out_ecount(1) CPlainPen **ppPen,
        // The pen geometry
    __out_ecount(1) CMilBrushDuce **ppBrush)
        // The pen's brush
{
    HRESULT hr;
    CMilPenRealization *pPenRealization = NULL;

    Assert(pPen);
    Assert(ppPen);
    Assert(ppBrush);

    // Get the pen
    IFC(pPen->GetPen(&pPenRealization));
    *ppPen = pPenRealization->GetPlainPen();
    Assert(*ppPen);

    // Get the brush
    *ppBrush = DYNCAST(CMilBrushDuce, pPenRealization->GetBrush());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::DrawShape
//
//  Synopsis:   Draws the shape with realizations of brush and pen, which are
//              retrieved from the fill & pen resource parameters.
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::DrawShape(
    __in_ecount(1) IShapeData *pShapeData,
    __in_ecount_opt(1) CMilBrushDuce *pFill,
    __in_ecount_opt(1) CMilPenDuce *pPen
    )
{
    HRESULT hr = S_OK;

    CMilBrushDuce *pBrush = NULL;
    CPlainPen *pPlainPen = NULL;

    CMilRectF boundsF;
    MilPointAndSizeD boundsD;

    Assert(pShapeData);

    if (IsBounding())
    {
        // This call is for computing bounds.
        IFC(FillAndStrokeShapeForBounds(pShapeData, pFill, pPen));
    }
    else
    {       
        //
        // FILL - Get the fill bounds if needed, and then call FillOrStrokeShape
        //

        if (NULL != pFill)
        {
            if (pFill->NeedsBounds(&m_brushContext))
            {
                // Computing the brush realization requires a bounding box
                IFC(pShapeData->GetTightBoundsNoBadNumber(boundsF));
                MilPointAndSizeDFromMilRectF(OUT boundsD, boundsF);
            }
            else
            {
                boundsF = CMilRectF::sc_rcEmpty;
                boundsD = MilEmptyPointAndSizeD;
            }

            // Fill the shape
            IFC(FillOrStrokeShape(
                    TRUE,           // This call is for the fill
                    pShapeData,
                    &boundsD,
                    &boundsF,
                    NULL,           // No pen is needed to fill the shape
                    pFill));
        }

        //
        // Stroke - Get the stroke bounds if needed, and then call FillOrStrokeShape
        //

        // Get pen & stroke brush handle if one was specified
        if (NULL != pPen)
        {
            IFC(GetPenAndItsBrush(pPen, &pPlainPen, &pBrush));

            if (pBrush && pBrush->NeedsBounds(&m_brushContext))
            {
                // Computing the brush realization requires a bounding box
                IFC(GetStrokeBounds(pShapeData, pPlainPen, OUT boundsF));
                MilPointAndSizeDFromMilRectF(OUT boundsD, boundsF);
            }
            else
            {
                boundsF = CMilRectF::sc_rcEmpty;
                boundsD = MilEmptyPointAndSizeD;
            }

            // Stroke the shape
            IFC(FillOrStrokeShape(
                    FALSE,           // This call is for the stroke
                    pShapeData,
                    &boundsD,
                    &boundsF,
                    pPlainPen,
                    pBrush));
        }
    }

Cleanup:
    //
    // Future Consideration:  This filter *should* no longer be needed,
    // as we are now explicitly checking for BADNUMBER lower down in the stack.
    // Consider removing this check.
    //
    if (hr == WGXERR_BADNUMBER)
    {
        //
        // We encountered a numerical error when drawing this geometry.  This
        // isn't a big deal -- we'll ignore this geometry and continue on.
        //
        hr = S_OK;
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::FillOrStrokeShape
//
//  Synopsis:   For either the stroke or the fill, this method performs the
//              common steps of retrieving the brush realization, setting
//              the render target properties based on the brush properties,
//              calling DrawPath, the then reseting the render target properties.
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::FillOrStrokeShape(
    BOOL fFillShape,
        // TRUE for fill, FALSE for stroke
    __in_ecount(1) IShapeData *pShapeData,
        // Shape to fill or stroke
    __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,
        // Bounds that relative brush properties should be sized to
    __in_ecount(1) const CMilRectF *pWorldSpaceBounds,
        // Viewable region to create brush realizations for.  Content
        // outside of this region may be clipped by brush realizations.
    __in_ecount_opt(1) CPlainPen *pPlainPen,
        // Pen realization
    __in_ecount(1) CMilSlaveResource *pBrush
        // Fill or stroke brush resource
    )
{
    HRESULT hr = S_OK;
    CBrushRealizer *pBrushRealizer = NULL;

    //
    // If the bounds aren't well ordered and we need them, then we encountered
    // a numerical error. This isn't a critical error, but there's no sense in
    // trying to stroke the shape if we can't compute its bounds.
    //
    if (pWorldSpaceBounds->IsWellOrdered())
    {
        //
        // Retrieve the brush realizations that are passed to DrawPath
        //

        IFC(GetBrushRealizer(
                pBrush,
                &m_brushContext,
                &pBrushRealizer
                ));

        //
        // Set up the brush context
        //

        m_brushContext.rcWorldBrushSizingBounds = *pBrushSizingBounds;
        m_brushContext.rcWorldSpaceBounds = *pWorldSpaceBounds;

        //
        // Call DrawPath to do the actual stroke/fill operation
        //
        MIL_THR(m_pIRenderTarget->DrawPath(
                &m_contextState,
                &m_brushContext,
                pShapeData,
                pPlainPen,
                // If were not filling the shape, pass the stroke brush
                (!fFillShape) ? pBrushRealizer : NULL,
                // If were are filling the shape, pass the fill brush
                (fFillShape) ? pBrushRealizer : NULL
                ));
    }

Cleanup:
    if (pBrushRealizer)
    {
        pBrushRealizer->FreeRealizationResources();
        pBrushRealizer->Release();
    }

    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::FillAndStrokeShapeForBounds
//
//  Synopsis:   A simple version of FillOrStrokeShape, optimized for computing
//              bounds, processing the stroke and the fill simultaneously.
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::FillAndStrokeShapeForBounds(
    __in_ecount(1) IShapeData *pShapeData,
        // Shape to fill and stroke
    __in_ecount_opt(1) CMilBrushDuce *pBrush,
        // Fill brush resource
    __in_ecount_opt(1) CMilPenDuce *pPen)
        // Pen resource
{
    HRESULT hr = S_OK;
    CPlainPen *pPlainPen = NULL;
    CBrushRealizer *pFillBrushRealizer = NULL;
    CBrushRealizer *pStrokeBrushRealizer = NULL;

    Assert(IsBounding());

    if (pPen)
    {
        CMilBrushDuce *pStrokeBrush;
        IFC(GetPenAndItsBrush(pPen, &pPlainPen, &pStrokeBrush));

        IFC(GetBrushRealizer(
            pStrokeBrush,
            &m_brushContext,
            &pStrokeBrushRealizer
            ));
    }

    IFC(GetBrushRealizer(
        pBrush,
        &m_brushContext,
        &pFillBrushRealizer
        ));

    // Call DrawPath to compute the bounds
    MIL_THR(m_pIRenderTarget->DrawPath(
        &m_contextState,
        NULL,
        pShapeData,
        pPlainPen,
        pStrokeBrushRealizer, // pStrokeBrush
        pFillBrushRealizer  // pFillBrush
        ));

Cleanup:
    if (pStrokeBrushRealizer)
    {
        pStrokeBrushRealizer->FreeRealizationResources();
        pStrokeBrushRealizer->Release();
    }

    if (pFillBrushRealizer)
    {
        pFillBrushRealizer->FreeRealizationResources();
        pFillBrushRealizer->Release();
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::DrawImage
//
//      pImage - Image to draw
//      prcDestinationBase - Rectangle containing destination rect for image
//      pDestRectAnimation - Possible animation for prcDestinationBase
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::DrawImage(
    __in_ecount_opt(1) CMilSlaveResource *pImage,
    __in_ecount(1) const MilPointAndSizeD *prcDestinationBase,
    __in_ecount_opt(1) CMilSlaveRect *pDestRectAnimations
    )
{
    HRESULT hr = S_OK;

    MilPointAndSizeD rcDestinationD;
    CMilRectF rcDestination;
    CMilRectF rcSource;
    IWGXBitmapSource *pBitmapSource = NULL;

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_ImageStart(0);
    }

    //
    // Obtain destination rectangle
    //

    IFC(GetRectCurrentValue(
        prcDestinationBase,
        pDestRectAnimations,
        &rcDestinationD
        ));

    // Cast destination rectangle to a float
    MilRectFFromMilPointAndSizeD(rcDestination, rcDestinationD);

    //
    // Obtain bitmap & source rectangle
    //

    IFC(GetBitmapSource(pImage, &rcSource, &pBitmapSource));

    //
    // Draw the image if one exists
    //

    if (pBitmapSource)
    {
        IFC(DrawBitmap(pBitmapSource, &rcSource, &rcDestination, 1.0f));
    }
    else if (pImage)
    {
        //
        // If GetBitmapSource returned pBitmapSource == NULL but we have a non-NULL
        // pImageResource then pImageResource could be a DrawingImage.
        // If this is not the case this means that bitmap data was not
        // ready and we just proceed with no-op.
        //
        if (pImage->IsOfType(TYPE_DRAWINGIMAGE))
        {
            CMilDrawingImageDuce *pDrawingImage = DYNCAST(CMilDrawingImageDuce, pImage);
            Assert(pDrawingImage);

            IFC(DrawDrawing(pDrawingImage->m_data.m_pDrawing, &rcDestination));
        }
    }

Cleanup:
    ReleaseInterfaceNoNULL(pBitmapSource);

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_ImageEnd(0);
    }

    RRETURN(hr);
}

/*++

Routine Description:

    Draws a BitmapSource with a transformation

Arguments:

    pBitmap -
        Pointer to the IWGXBitmapSource to be drawn

    prcSource -
        Source region of interest

    prcDest -
        Destination dimensions

Return Value:

    HRESULT

--*/

HRESULT CDrawingContext::DrawBitmap(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
    __in_ecount(1) const MilRectF *prcSource,
    __in_ecount(1) const MilRectF *prcDest,
    float opacity
    )
{
    HRESULT hr = S_OK;
    static const UINT PARALLELOGRAM_COUNT = 4;
    CCompoundShapeNoRef drawRegionWorldSpace;
    CParallelogram drawRectInLocalSpaceShape;

    bool fPushedTransform = false;


    IMILEffectList *pEffectList = NULL;

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_BitmapStart(0);
    }
    
    //
    // Note that we want the draw rect in local destination space
    // so that we can transform it to world space.
    //
    CRectF<CoordinateSpace::LocalRendering> drawRectInLocalSpace(
        prcDest->left,
        prcDest->top,
        prcDest->right,
        prcDest->bottom,
        LTRB_Parameters
        );

    CRectF<CoordinateSpace::BaseSampling> sourceRectInBaseSamplingSpace(
        prcSource->left,
        prcSource->top,
        prcSource->right,
        prcSource->bottom,
        LTRB_Parameters
        );

    const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::LocalRendering> *pTextureToLocalTransform = NULL;    
    CShapeBase *pShapeToDraw = NULL;

    CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::LocalRendering> textureToLocalTransform;
    textureToLocalTransform.InferAffineMatrix(
        sourceRectInBaseSamplingSpace,
        drawRectInLocalSpace
        );

    // If we have nothing to render, skip to clean-up.
    if (prcDest->right - prcDest->left < DBL_EPSILON || prcDest->bottom - prcDest->top < DBL_EPSILON)
    {
        goto Cleanup;
    }

    //
    // Support for translucent DrawBitmap calls.
    //

    if (!IsCloseReal(opacity, 1.0f) || g_fTranslucent_DrawBitmap)
    {
        AlphaScaleParams scale;
        scale.scale = g_fTranslucent_DrawBitmap ? g_fTranslucent_Draw_Scale : opacity;

        IFC(MILCreateEffectList(&pEffectList));
        IFC(pEffectList->Add(CLSID_MILEffectAlphaScale, sizeof(scale), &scale));
    }

    // Take the shape in local space and use the normal inferred affine transform
    // to transform the bitmap.
    //
    if (pShapeToDraw == NULL)
    {
        drawRectInLocalSpaceShape.Set(drawRectInLocalSpace);

        pShapeToDraw = &drawRectInLocalSpaceShape;
        pTextureToLocalTransform = &textureToLocalTransform;
    }

    IFC(FillShapeWithBitmap(
        pIBitmapSource,
        pTextureToLocalTransform,
        pShapeToDraw,
        pEffectList
        ));


Cleanup:
    // Restore world transform if we pushed one
    if (fPushedTransform)
    {
        PopTransform();
    }
    
    ReleaseInterfaceNoNULL(pEffectList);

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_BitmapEnd(0);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    GetDirtyClippedWorldSpaceBounds
//
//  Synopsis:  Takes bounds in local space and converts them to AA inflation
//             adjusted world space bounds
//
//  1.      Transforms bounds to world space according to current m_transformStack
//  2.      Inflates bounds for anti aliasing if it is enabled
//  3.      Gets world clip bounds (which contain current dirty rect) and clips the results of 2.
//           and returns it as an out parameter.
//
//  Returns: void
// 
//-----------------------------------------------------------------------------


void 
CDrawingContext::GetClippedWorldSpaceBounds(
    __in_ecount(1) CRectF<CoordinateSpace::LocalRendering> const * pBoundsInLocalSpace,
    __out_ecount(1) CRectF<CoordinateSpace::PageInPixels> * pAAInflatedClippedBoundsWorld
    )    
{
    CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> worldTransform;
    CRectF<CoordinateSpace::PageInPixels> clipBoundsWorld;
        
    // 1) Get the world transform.
    m_transformStack.Top(&worldTransform);

    // 2) Transform the node's bounds into world space.
    worldTransform.Transform2DBoundsConservative(
        *pBoundsInLocalSpace,
        OUT *pAAInflatedClippedBoundsWorld);

    // 3) Inflate the bounding box in world space to compensate for AA.

    //    If anti-aliasing is off we need to snap the bounding box correctly. 

    if (m_renderState.AntiAliasMode != MilAntiAliasMode::None)
    {
        if (!pAAInflatedClippedBoundsWorld->IsEmpty())
        {
            InflateRectF_InPlace(pAAInflatedClippedBoundsWorld);
        }
    }

    //
    // 4) Clip the AA only inflated bounds. 
    //
    GetClipBoundsWorld(&clipBoundsWorld);
    clipBoundsWorld.Intersect(*pAAInflatedClippedBoundsWorld);

    // 
    // 5) Return AA only inflated and clipped bounds.
    // 
    *pAAInflatedClippedBoundsWorld = clipBoundsWorld;
       
}


//+-----------------------------------------------------------------------------
//
//  Member:    FillShapeWithBitmap
//
//  Synopsis:  Draw an image clipped by a given shape.
//
//-----------------------------------------------------------------------------

HRESULT CDrawingContext::FillShapeWithBitmap(
    __in_ecount(1) IWGXBitmapSource *pIWGXBitmapSource, // image to be drawn
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::LocalRendering> *pTextureToLocalTransform,
    __in_ecount(1) CShapeBase *pShape,                  // clip region
    __in_ecount_opt(1) IMILEffectList *pEffectList,     // effects to use when drawing
    MilBitmapWrapMode::Enum wrapMode
    )
{
    HRESULT hr = S_OK;

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_ClippedBitmapStart(0);
    }

    // Lazily create a CMILBrushBitmap and cache it
    if (!m_pScratchBitmapBrush)
    {
        IFC(CMILBrushBitmap::Create(
            &m_pScratchBitmapBrush
            ));
    }

    {
        ApplyRenderState();

        // Temporarily set the bitmap source for our brush
        CMILBrushBitmapLocalSetterWrapper brushBitmapLocalWrapper(
            m_pScratchBitmapBrush,
            pIWGXBitmapSource,
            wrapMode,
            pTextureToLocalTransform,
            XSpaceIsWorldSpace
            DBG_COMMA_PARAM(NULL)
            );

        // Create a local fillBrush for the bitmap to use in
        // the DrawPath call
        LocalMILObject<CImmediateBrushRealizer> fillBrush;
        fillBrush.SetMILBrush(
            m_pScratchBitmapBrush,
            pEffectList,
            false // don't skip meta-fixups
            );

        IFC(m_pIRenderTarget->DrawPath(
            &m_contextState,
            NULL,
            pShape,
            NULL,
            NULL,
            &fillBrush
            ));
    }

Cleanup:
    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_ClippedBitmapEnd(0);
    }

    RRETURN(hr);
}

/*++

Routine Description:

    Draws a Drawing to the dimensions of prcDest

Arguments:

    pDrawing-
        Pointer to Drawing to draw

    prcDest -
        Destination dimensions

    (source dimensions come from pDrawing)

Return Value:

    HRESULT

--*/

HRESULT CDrawingContext::DrawDrawing(
    __in_ecount(1) CMilDrawingDuce *pDrawing,
    __in_ecount(1) const CMilRectF *prcDest
    )
{
    HRESULT hr = S_OK;

    BOOL fPushed = FALSE;
    CMILMatrix transform;

    CRectF<CoordinateSpace::LocalRendering> rcSrcF_RB;

    Assert(prcDest);

    if (!pDrawing || prcDest->IsEmpty() || !prcDest->HasValidValues())
    {
        goto Cleanup;
    }

    IFC(m_pContentBounder->GetContentBounds(
        pDrawing,
        &rcSrcF_RB
        ));

    //   We may wish to inflate the source rect slightly
    // to handle near-empty cases. See task 15687.

    transform.InferAffineMatrix(*prcDest, rcSrcF_RB);

    // Push the new transform.
    IFC(PushTransform(&transform));
    fPushed = TRUE;

    // We've added a transform, we need to apply the new state before we draw
    ApplyRenderState();

    IFC(pDrawing->Draw(this));

Cleanup:

    // Pop the transform (if we pushed it).
    if (fPushed)
    {
         PopTransform();

         ApplyRenderState();
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::DrawVideo
//
//  Synopsis:   Draws a video using an media clock resource & destination
//              rectangle
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::DrawVideo(
    __in_ecount(1) CMilSlaveVideo *pMediaClock,
    __in_ecount(1) const MilPointAndSizeD *prcDestinationBase,
    __in_ecount_opt(1) CMilSlaveRect *pDestRectAnimations
    )
{
    HRESULT hr = S_OK;

    IAVSurfaceRenderer *pSurfaceRenderer = NULL;
    IWGXBitmapSource   *pBitmapSource = NULL;

    CMILMatrix matSourceToDestination;

    MilPointAndSizeD rcDestinationD;
    MilPointAndSizeF rcDestination;
    CMilPointAndSizeF rcSource;

    BOOL fPushed = FALSE;

    //
    // Obtain IAVSurfaceRenderer interface from the resource
    //

    // If the user didn't specify a null MediaClock, obtain the surface renderer
    if (pMediaClock)
    {
        IFC(pMediaClock->GetSurfaceRenderer(&pSurfaceRenderer));
    }
    else
    {
        //
        // If we don't have a video slave, bail out now.
        //
        goto Cleanup;
    }

    //
    // If we don't have a surface renderer, bail out.
    //
    if (NULL == pSurfaceRenderer)
    {
        goto Cleanup;
    }
    else
    {
        //
        // Obtain the source rectangle
        //
        IFC(pSurfaceRenderer->GetContentRectF(reinterpret_cast<MilPointAndSizeF*>(&rcSource)));
    }

    if (rcSource.IsEmpty())
    {
        // Handle empty source rectangle gracefully via an early-out
        goto Cleanup;
    }

    //
    // Obtain destination rectangle
    //

    IFC(GetRectCurrentValue(
        prcDestinationBase,
        pDestRectAnimations,
        &rcDestinationD
        ));

    // Cast destination rectangle to a float
    MilPointAndSizeFFromMilPointAndSizeD(&rcDestination, &rcDestinationD);

    //
    // Implement the destination rectangle property by applying a
    // source->destination transform
    //

    // Infer a transform that maps the source to the destination
    matSourceToDestination.InferAffineMatrix(rcDestination, rcSource);

    // Push the new transform.
    IFC(PushTransform(&matSourceToDestination));
    fPushed = TRUE;

    ApplyRenderState();

    //
    // Call DrawVideo to actually render the video on the render target.
    //
    IFC(m_pIRenderTarget->DrawVideo(
        &m_contextState,
        pSurfaceRenderer,
        pBitmapSource,
        NULL
        ));

Cleanup:

    // pop the transform (if we pushed it).
    if (fPushed)
    {
        PopTransform();

        // We've popped the transform, apply the RenderState again
        ApplyRenderState();
    }

    ReleaseInterfaceNoNULL(pSurfaceRenderer);
    ReleaseInterfaceNoNULL(pBitmapSource);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::DrawBitmap
//
//  Synopsis:   Draws a bitmap resource without any source->destination
//              transformation.
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::DrawBitmap(
    __in_ecount(1) CMilSlaveResource *pBitmap,   // Bitmap resource to draw
    MilBitmapWrapMode::Enum wrapMode
    )
{
    HRESULT hr = S_OK;
    IWGXBitmapSource *pIWGXBitmapSource = NULL;
    CMilRectF rcEntireBitmap;
    CParallelogram rcFillShape;

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_BitmapStart(0);
    }

    //
    // Retrieve the current bitmap from the bitmap resource
    //

    IFC(GetBitmapSource(pBitmap, &rcEntireBitmap, &pIWGXBitmapSource));

    //
    // Construct the fill shape
    //
    rcFillShape.Set(rcEntireBitmap);

    //
    // Draw the bitmap
    //

    if (pIWGXBitmapSource)
    {
        IFC(FillShapeWithBitmap(
            pIWGXBitmapSource,
            CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::LocalRendering>::pIdentity(), // pTextureToLocalTransform,
            &rcFillShape,
            NULL,  // pEffectList
            wrapMode
            ));
    }
    // else either pBitmapSource was NULL or we weren't able to get
    // bitmap data out of the resource

Cleanup:
    ReleaseInterfaceNoNULL(pIWGXBitmapSource);

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_BitmapEnd(0);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::DrawGlyphRun
//
//  Synopsis:   Draws a GlyphRun using a GlyphRun & Foreground Brush resource
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::DrawGlyphRun(
    __in_ecount_opt(1) CMilBrushDuce *pBrush,
    __in_ecount_opt(1) CGlyphRunResource *pGlyphRun
    )
{
    HRESULT hr = S_OK;

    CBrushRealizer *pFillBrush = NULL;

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_GlyphRunStart(0);
    }

    // Early-out if the glyph run is null
    if (NULL == pGlyphRun)
    {
        goto Cleanup;
    }

    if (pGlyphRun->ShouldUseGeometry(&m_contextState.WorldToDevice, m_contextState.GetCurrentOrDefaultDisplaySettings()))
    {
        const CMilGeometryDuce *pGeometry = pGlyphRun->GetGeometryRes();
        if (pGeometry)
        {
            // By design, text should be antialiased regardless of
            // RenderOptions.EdgeMode.
            MilAntiAliasMode::Enum oldAntiAliasMode = m_renderState.AntiAliasMode;
            m_renderState.AntiAliasMode = MilAntiAliasMode::EightByEight;

            MIL_THR(DrawGeometry(
                pBrush,
                NULL, // Pen
                pGlyphRun->GetGeometryRes()
                ));

            // Restore temporarily changed mode.
            m_renderState.AntiAliasMode = oldAntiAliasMode;
        }       
    }
    else  // Assume this GlyphRun uses realizations; we will create them on demand as needed
    {
        CRectF<CoordinateSpace::LocalRendering> rcBoundsLocal;
        pGlyphRun->GetBounds(&rcBoundsLocal, &m_contextState.WorldToDevice);

        if (!rcBoundsLocal.IsEmpty())
        {
            IFC(GetBrushRealizer(
                pBrush,
                &m_brushContext,
                &pFillBrush
                ));

            //
            // Set up the brush context
            //

            MilPointAndSizeDFromMilRectF(OUT m_brushContext.rcWorldBrushSizingBounds, rcBoundsLocal);
            m_brushContext.rcWorldSpaceBounds = rcBoundsLocal;

            {
                DrawGlyphsParameters pars;
                    pars.pContextState = &m_contextState;
                    pars.pBrushContext = &m_brushContext;
                    pars.pGlyphRun = pGlyphRun;
                    pars.pBrushRealizer = pFillBrush;


                // calculate bounding box in device space
                m_contextState.WorldToDevice.Transform2DBounds(
                    rcBoundsLocal,
                    OUT pars.rcBounds
                    );

                if (!pars.rcBounds.AnySpace().IsEmpty() && pars.rcBounds.AnySpace().HasValidValues())
                {
                    if (!IsBounding())
                    {
                        // allow half-pixel border for clear type bleeding and subpixel animation
                        pars.rcBounds.AnySpace().Inflate(0.5f, 0.5f);
                    }

                    EventWriteDWMDraw_Info(pars.rcBounds.AnySpace().left, pars.rcBounds.AnySpace().top, pars.rcBounds.AnySpace().right, pars.rcBounds.AnySpace().bottom);

                    IFC(m_pIRenderTarget->DrawGlyphs(pars));
                }
            }
        }
    }

Cleanup:
    if (pFillBrush)
    {
        pFillBrush->FreeRealizationResources();
        pFillBrush->Release();
    }

    if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
    {
        EventWriteDWMDraw_GlyphRunEnd(0);
    }

    if (hr == WGXERR_GLYPHBITMAPMISSED)
    {
        // 
        // We unexpectedly couldn't retrieve glyph bitmaps we thought were
        // available. This should not happen, but if it does, there's little
        // value in crashing the app. Some text may disappear, especially 
        // in some transient situations. 
        // I'm hesitant to crash the app here, because the previous glyph code
        // would handle this failure silently, so I can't be sure that I wouldn't
        // introduce a regression for failure cases that are currently unknown.
        // Settling for logging the error with the stackcapture instrumentation.
        //
        Assert(false);
        hr = S_OK;
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    DrawDrawing
//
//  Synopsis:  Parses a RenderData DrawDrawing command to retrieve the
//             CMilDrawingDuce resource it references, and calls Draw.
//
//  Arguments:
//      pDrawing - The drawing to draw
//
//  Return value:
//      HRESULT
//
//------------------------------------------------------------------------------

HRESULT
CDrawingContext::DrawDrawing(
    __in_ecount_opt(1) CMilDrawingDuce *pDrawing
    )
{
    HRESULT hr = S_OK;

    if (pDrawing)
    {
        IFC(pDrawing->Draw(this));
    }

Cleanup:
    RRETURN(hr);
}


/*++

Routine Description:

    Processes a render data PushClip instruction

Arguments:

    pClipGeometry - Clip geometry resource

Return Value:

    HRESULT

--*/

HRESULT
CDrawingContext::PushClip(
    __in_ecount_opt(1) CMilGeometryDuce *pClipGeometry
    )
{
    HRESULT hr = S_OK;

    IFC(PushEffects(1.0f, pClipGeometry, NULL, NULL, NULL));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
//  Pushes an image effect
//
// Synopsis:
//
//  Adds an image effect as an effect to the effect stack
//
// Arguments:
//
//  pEffect - image effect to push
//  prcBounds - bounds of the effect area
//
// Return Value:
//
//    HRESULT
//
//---------------------------------------------------------------------------------


HRESULT
CDrawingContext::PushImageEffect(
    __in_ecount_opt(1) CMilEffectDuce *pEffect,
    __in_ecount_opt(1) CRectF<CoordinateSpace::LocalRendering> const *prcBounds
    )
{
    HRESULT hr = S_OK;

    IFC(PushEffects(1.0f, NULL, NULL, pEffect, prcBounds));

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
//  Pushes an opacity mask
//
// Synopsis:
//
//  Adds an opacity mask as an effect to the effect stack
//
// Arguments:
//
//  pOpacityMask - Opacity mask to push
//
// Return Value:
//
//    HRESULT
//
//---------------------------------------------------------------------------------


HRESULT
CDrawingContext::PushOpacityMask(
    __in_ecount_opt(1) CMilBrushDuce *pOpacityMask,
    __in_ecount_opt(1) CRectF<CoordinateSpace::LocalRendering> const *prcBounds
    )
{
    HRESULT hr = S_OK;

    IFC(PushEffects(1.0f, NULL, pOpacityMask, NULL, prcBounds));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::PushClipRect
//
//  Synopsis:   Pushes a clip rectangle onto the clip stack.
//
//  Notes:      If this method is called, the caller must call
//              ApplyRenderState() before calling a Draw* method.
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::PushClipRect(
    __in_ecount(1) CMilRectF const &clip  // Clip rectangle to push
    )
{
    HRESULT hr = S_OK;

    // Push the new clip on the clip stack
    IFC(m_clipStack.Push(clip));

    // Push the clip on the stack-state stack
    //  Note: Pop of clip stack is handled by PushClipStackState if it fails.
    IFC(PushClipStackState());

    // Invalidate the clip realization since a potentially new clip was pushed
    InvalidateClipRealization();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::PushExactClip
//
//  Synopsis:   Pushes a specific clip rectangle onto the clip stack.
//
//  Notes:      If this method is called, the caller must call
//              ApplyRenderState() before calling a Draw* method.
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::PushExactClip(
    __in_ecount(1) const MilRectF &rcClip,
    BOOL fPushState
    )
{
    HRESULT hr = S_OK;

    // Push the new exact clip on the clip stack
    IFC(m_clipStack.PushExact(rcClip));

    if (fPushState)
    {
        // Push the clip on the stack-state stack
        //  Note: Pop of clip stack is handled by PushClipStackState if it fails.
        IFC(PushClipStackState());
    }

    // Invalidate the clip realization since a potentially new clip was pushed
    InvalidateClipRealization();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::PushClipStackState
//
//  Synopsis:   This method contains the functionality common to all PushClip
//              operations of pushing the stack state type.  If an error occurs,
//              the previously pushed clip is Pop'd.
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::PushClipStackState()
{
    HRESULT hr = S_OK;

    // Push the type on the stack-state stack so the generic Pop method
    // knows to Pop from the clip stack.
    MIL_THR(m_stateTypeStack.Push(StackStateTypeClip));

    // If the state stack push failed, pop from the clipstack to prevent
    // the two stacks from becoming mismatched.
    if (FAILED(hr))
    {
        m_clipStack.Pop();
    }

    RRETURN(hr);
}

/*++

Routine Description:

    Pop the Clip from the context clip stack.

    The top of the stack represents the accumulated intersection of
    every clip pushed in the stack, rather than the last push. Pop reverts
    the last state thus maintaining the stack accumulation.

    If this method is called, the caller must call ApplyRenderState() before
    calling a Draw* method.

Arguments:

Return Value:

    VOID

--*/

VOID
CDrawingContext::PopClip(
    BOOL fPopState
    )
{
    if (fPopState)
    {
        StackStateType sst;

        Verify(m_stateTypeStack.Pop(&sst));
        Assert(sst == StackStateTypeClip);
    }

    m_clipStack.Pop();

    // Invalidate the current clip realization if we successfully pushed a new clip
    InvalidateClipRealization();
}

//---------------------------------------------------------------------------------
//  PushOpacity.
//
// Arguments:
//
//  opacity - Base opacity value
//  pOpacityAnimation - Animation for opacity property
//
// Return Value:
//
//    HRESULT
//
//---------------------------------------------------------------------------------

HRESULT CDrawingContext::PushOpacity(
    __in_ecount(1) const double &opacity,
    __in_ecount_opt(1) CMilSlaveDouble *pOpacityAnimation
    )
{
    HRESULT hr = S_OK;
    double opacityValue = opacity;

    // Handle the animate case by filling in the non-animate version of the struct
    if (pOpacityAnimation)
    {
        IFC(GetDoubleCurrentValue(
            &opacityValue,
            pOpacityAnimation,
            &opacityValue
            ));
    }

    IFC(PushEffects(opacityValue, NULL, NULL, NULL, NULL));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::PushTransform
//
//  Synopsis:   Pushes a transform referenced by a transform resource onto the
//              transform stack.
//
//  Notes:      If this method is called, the caller must call
//              ApplyRenderState() before calling a Draw* method.
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::PushTransform(
    __in_ecount_opt(1) CMilTransformDuce *pTransform
    )
{
    HRESULT hr = S_OK;

    if (NULL == pTransform)
    {
        // Push an identity matrix to match the corresponding Pop call
        IFC(PushTransform(&IdentityMatrix));
    }
    else
    {
        //
        // Retrieve current matrix value from the transform resource
        //
        const CMILMatrix *pMatrix = NULL;
        IFC(GetMatrixCurrentValue(pTransform, &pMatrix));

        IFC(PushTransform(pMatrix));
    }

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    Push a specific transform onto the context transform stack. The transform
    is remembered on the internal transform stack and all subsequent drawing
    uses the top of the transform stack.

    The top of the stack represents the accumulated multiplication of
    every matrix pushed in the stack, rather than the last push. Pop reverts
    the last multiply thus maintaining the stack accumulation.

    If this method is called, the caller must call ApplyRenderState() before
    calling a Draw* method.

Arguments:

    pTransform -
        Matrix to push on the stack.

Return Value:

    HRESULT

--*/

HRESULT
CDrawingContext::PushTransform(
    __in_ecount(1) const CMILMatrix *pTransform,
    bool multiply
    )
{
    HRESULT hr = S_OK;
    Assert(pTransform);

    // Push the transform on the transform stack
    IFC(m_transformStack.Push(pTransform, multiply));

    // Push the transform stack-state stack
    IFC(PushTransformStackStateAndInvalidate());

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    Push a specific transform onto the context transform stack. The transform
    is remembered on the internal transform stack and all subsequent drawing
    uses the top of the transform stack.

    The top of the stack represents the accumulated multiplication of
    every matrix pushed in the stack, rather than the last push. Pop reverts
    the last multiply thus maintaining the stack accumulation.

    This version causes the incoming matrix to be *post* multiplied with the
    current top of the stack.

    If this method is called, the caller must call ApplyRenderState() before
    calling a Draw* method.

Arguments:

    pTransform -
        Matrix to push on the stack.

Return Value:

    HRESULT

--*/

HRESULT
CDrawingContext::PushTransformPostOffset(
    float rPostOffsetX,
    float rPostOffsetY
    )
{
    HRESULT hr = S_OK;

    // Push the transform on the stack.
    IFC(m_transformStack.PushPostOffset(rPostOffsetX, rPostOffsetY));

    // Push the transform stack-state stack
    IFC(PushTransformStackStateAndInvalidate());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::PushTransformStackStateAndInvalidate
//
//  Synopsis:   This method contains the functionality common to both the
//              PushTransform and PushTransformPostmultiply operations.
//
//              It pushes the transform on the stack-state stack, pops
//              from the transform stack upon failure, and invalidates
//              the transform realization.
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::PushTransformStackStateAndInvalidate()
{
    HRESULT hr = S_OK;

    // Push the type on the stack-state stack so the generic Pop method
    // knows to Pop from the transform stack.
    MIL_THR(m_stateTypeStack.Push(StackStateTypeTransform));

    // If the state stack push failed, pop from the transform stack to prevent
    // the two stacks from becoming mismatched.
    if (FAILED(hr))
    {
        m_transformStack.Pop();
        goto Cleanup;
    }

    // Invalidate the current transform realization if we successfully
    // pushed a new transform
    InvalidateTransformRealization();

Cleanup:
    RRETURN(hr);
}

/*++

Routine Description:

    Pop the last transform from the context transform stack.

    The top of the stack represents the accumulated multiplication of
    every matrix pushed in the stack, rather than the last push. Pop reverts
    the last multiply thus maintaining the stack accumulation.

    If this method is called, the caller must call ApplyRenderState() before
    calling a Draw* method.

Arguments:

    pTransform -
        Matrix to push on the stack.

Return Value:

    VOID

--*/

VOID
CDrawingContext::PopTransform()
{
    StackStateType sst;

    // Pop from the state-type stack
    Verify(m_stateTypeStack.Pop(&sst));
    Assert(sst == StackStateTypeTransform);

    // Pop from the transform stack
    m_transformStack.Pop();

    // Invalidate the current transform realization
    InvalidateTransformRealization();
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDrawingContext::PushGuidelineCollection
//
//  Synopsis:
//      Push a GuidelineFrame to GuidelineFrame stack that
//      resides in m_contextState.m_pSnappingStack.
//      Guideline coordinates are fetched from given CGuidelineCollection
//      and converted to device space.
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::PushGuidelineCollection(
    __in_ecount_opt(1) CMilGuidelineSetDuce *pGuidelines
    )
{
    HRESULT hr = S_OK;

    if (pGuidelines)
    {
        bool fNeedMoreCycles = false;
        CGuidelineCollection* pGuidelineCollection = NULL;

        Assert(pGuidelines);

        IFC(pGuidelines->GetGuidelineCollection(
            &pGuidelineCollection
            ));

        IFC(PushGuidelineCollection(pGuidelineCollection, fNeedMoreCycles));

        if (fNeedMoreCycles && pGuidelines != NULL)
        {
            IFC(pGuidelines->ScheduleRender());
        }
    }
    else
    {
        // When NULL guideline set is pushed we should not switch off
        // currently acting guidelines. This should match the behavior
        // of drawing group.
        IFC( PushNoModificationLayer() );
    }
Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDrawingContext::PushGuidelineCollection
//
//  Synopsis:
//      Push a GuidelineFrame to GuidelineFrame stack that
//      resides in m_contextState.m_pSnappingStack.
//      Guideline coordinates are fetched from given CGuidelineCollection
//      and converted to device space.
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::PushGuidelineCollection(
    __in_ecount_opt(1) CGuidelineCollection *pGuidelineCollection,
    __out_ecount(1) bool & fNeedMoreCycles
    )
{
    HRESULT hr = S_OK;

    // Push the type on the stack-state stack so the generic Pop method
    // knows to Pop from the GuidelineFrame stack.
    IFC(m_stateTypeStack.Push(StackStateTypeGuidelineCollection));

    // Skip PushFrame on bounding computation.
    if (!IsBounding())
    {
        const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pMat = m_transformStack.GetTopByReference();
        if (pMat == NULL)
        {
            pMat = CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels>::pIdentity();
        }

        MIL_THR(CSnappingFrame::PushFrame(
            pGuidelineCollection,
            *pMat,
            m_contextState.CurrentTime,
            fNeedMoreCycles,
            m_fDrawingIntoVisualBrush,
            &m_contextState.m_pSnappingStack
            ));

        // If the GuidelineFrame stack push failed, pop from the state stack to prevent
        // the two stacks from becoming mismatched.
        if (FAILED(hr))
        {
            m_stateTypeStack.Pop();
            goto Cleanup;
        }
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::PopGuidelineCollection
//
//  Synopsis:
//      Undo PushGuidelineCollection().
//
//------------------------------------------------------------------------------
VOID
CDrawingContext::PopGuidelineCollection()
{
    StackStateType sst;

    // Pop from the state-type stack
    Verify(m_stateTypeStack.Pop(&sst));
    Assert(sst == StackStateTypeGuidelineCollection);

    if (!IsBounding())
    {
        // Pop from the GuidelineFrame stack
        CSnappingFrame::PopFrame(
            &m_contextState.m_pSnappingStack
            );
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:    PushEffects
//
//  Synopsis:  Begin a new sublayer for effects to be applied in the
//             corresponding PopEffects.
//
//------------------------------------------------------------------------------

HRESULT CDrawingContext::PushEffects(
    __in_ecount(1) const DOUBLE &rOpacity,     // Opacity in the range [0.0, 1.0], to be applied as a
                                               // constant alpha when the layer is popped.
    __in_ecount_opt(1) CMilGeometryDuce *pGeometryMask,  // Optional clip geometry to push
    __in_ecount_opt(1) CMilBrushDuce *pOpacityMaskBrush, // Handle to the optional Brush which can serve as an alpha mask.
    __in_ecount_opt(1) CMilEffectDuce *pEffect, // Handle to optional bitmap effect
    __in_ecount_opt(1) CRectF<CoordinateSpace::LocalRendering> const *pSurfaceBoundsLocalSpace
    )
{
    HRESULT hr = S_OK;

    FLOAT flOpacity = static_cast<float>(ClampAlpha(rOpacity));

    //
    // Obtain the current value of the mask geometry
    //

    CShape *pMaskShape = NULL;

    if (pGeometryMask)
    {
        IShapeData *pShapeData;

        IFC(pGeometryMask->GetShapeData(&pShapeData));

        pMaskShape = new CShape;
        IFCOOM(pMaskShape);

        // Clone the shape before we transform it
        IFC(pMaskShape->AddShapeData(*pShapeData));

        pMaskShape->SetFillMode(pShapeData->GetFillMode());

        //
        // Transform the geometry into target space now.
        //
        // This has two advantages over transforming later when drawing from
        // the layer:
        //   1. We don't have to worry about cloning and transforming the shape
        //      in case there was a need to call DrawLayer multiple times.
        //   2. The bounds we get will be more accurate which can reduce
        //      wasted rendering.
        //

        pMaskShape->Transform(m_transformStack.GetTopByReference());
    }

    if (IsBounding())
    {
        if (pMaskShape)
        {
            // This code mimicks BoundsDrawingContextWalker::PushClip
            CMilRectF rcClipBounds;

            IFC(pMaskShape->GetTightBoundsNoBadNumber(
                rcClipBounds,
                NULL, // pen
                NULL // pTransform
                ));

            if (!rcClipBounds.IsWellOrdered())
            {
                //
                // We encountered a numerical error when computing bounds. This
                // isn't a critical error, but we still don't know the real bounds
                // of the mask. We'll be conservative and set them to infinite.
                //
                rcClipBounds = CMilRectF::sc_rcInfinite;
            }

            IFC(PushClipRect(rcClipBounds));
        }
        else
        {
            // Opacity effects don't affect the bounds computation.
            IFC(PushNoModificationLayer());
        }
    }
    // Check to see if a layer is actually needed.  If not, we'll push a no-op layer
    // and not replace the render target.
    else if (   (NULL == pOpacityMaskBrush)
             && (NULL == pMaskShape)
             && IsCloseReal(flOpacity, 1.0f)
             && (NULL == pEffect)
            )
    {
        IFC(PushNoModificationLayer());
    }
    else
    {
        IFC(PushLayer(
            CLayer(flOpacity, pMaskShape, pOpacityMaskBrush, pEffect, pSurfaceBoundsLocalSpace),
            pSurfaceBoundsLocalSpace));

        pMaskShape = NULL;  // Successfully owned by layer
    }

Cleanup:
    delete pMaskShape;

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    PopEffects
//
//  Synopsis:  End a sublayer, and apply the effects.
//
//  Note:      At this point in the rendering procedure there should always be
//             a clip.  It is not required here, but expected because PushLayer
//             uses the current clip to determine how large of a layer to
//             create.  Since clips are pushed on the state stack as
//             layers/effects are and this is the pop of the layer the clip
//             will still be on the stack.  Any clip pushed for the express
//             purpose of getting dimensions to PushLayer will be
//             useless/excessive here.  All the same it is left in place now.
//-----------------------------------------------------------------------------

HRESULT CDrawingContext::PopEffects()
{
    HRESULT hr = S_OK;

    IMILEffectList *pEffectList = NULL;

    CLayer layer, alphaMaskLayer;

    // Note that if layer.pbmOutput is non-null we now own the reference!
    IFC(PopLayer(&layer));

    // Future Consideration:   Call Pop from PostSubgraph to simplify
    //  special cases in PopEffects/PopLayer.  For this code it means
    //  a pbmOutput may be relied upon (assuming success of course.)

    // If there is no bitmap in the output of PopLayer, then there is no work to be done.
    if (NULL == layer.pbmOutput)
    {
        goto Cleanup;
    }

    // Image effects are handled separately from the other effects.
    if (layer.pEffect != NULL)
    {
        // If this layer has an image effect, it should not have any other effect applied.
        Assert(layer.pAlphaMaskBrush == NULL && layer.pGeometricMaskShape == NULL && layer.rAlpha == 1.0);
        IFC(DrawEffectLayer(layer));
    }
    // Other effects use the effects list and shader pipeline.
    else
    {
        IFC(MILCreateEffectList(&pEffectList));

        // If we have an alpha mask, it's time to apply it
        if (layer.pAlphaMaskBrush)
        {
            //
            // Apply the AlphaMaskBrush to the current visual node.
            //
            // To do this, we fill a rectangle that has the same bounds
            // as the current visual node into an intermediate surface with
            // the alpha mask brush.  From there we just need to combine
            // the intermediate AlphaMask surface with the current visual content.
            //
            // Earlier during PushEffects we called PushLayer because an AlphaMask
            // brush existed.  This caused all of the visual content to be
            // rendered into an intermediate surface.
            //
            // After both operations, we'll have 2 intermediate surfaces: one with the
            // alpha  mask, and one with the current visual content.  These 2 surfaces
            // are combined when we pass an effect list with the alpha mask surface
            // & the layer with the current visual surface to DrawLayer.
            //
            // We need to obtain the inner bounds (i.e., the bounds before visual's
            // transform and clip are applied) because the current node's
            // transform & clip have been already pushed onto the transform &
            // clip stacks.  These bounds were saved in the temporary layer object.
            CRectF<CoordinateSpace::LocalRendering> rcBounds;
            Assert(layer.fHasBounds);
            rcBounds = layer.rcBounds;
        
            // Obtain the intermediate AlphaMask surface
            //
            // The layering logic is used to create the intermediate AlphaMask
            // surface because we want the AlphaMask to rendered with the same
            // context state (transform, clip, etc.) that the visual's content
            // was rendered to.  This is because the OpacityMask is declared
            // 'inside' the current visual node, it must have the visual's
            // transform & clip applied to it.
            IFC(CreateAndFillLayer(
                layer.pAlphaMaskBrush,
                &rcBounds,
                &alphaMaskLayer
                ));

            // Apply the effect if the layer contains a surface (e.g., it may
            // not contain a surface if the current clip is empty)
            if (alphaMaskLayer.pbmOutput)
            {
                AlphaMaskParams alphaMask;
                CMILMatrix *pAlphaMaskTransform = reinterpret_cast<CMILMatrix*>(&alphaMask.matTransform);

                // IUnknown** isn't covariant with IWGXBitmapSource** so we have to
                // use a temp var.
                IUnknown* pbmOutputAsIUnknown = alphaMaskLayer.pbmOutput;

                //
                // Setup the AlphaMask transform
                //

                pAlphaMaskTransform->SetToIdentity();

                if (alphaMaskLayer.fHasOffset)
                {
                    // Translate the intermediate AlphaMask surface to the origin
                    // of the current visual.
                    pAlphaMaskTransform->SetTranslation(
                        static_cast<REAL>(alphaMaskLayer.ptLayerPosition.X),
                        static_cast<REAL>(alphaMaskLayer.ptLayerPosition.Y)
                        );
                }

                // Update the effect list to pass to DrawLayer
                //
                // This will add a ref to pbmOutputAsIUnknown and own this ref.
                IFC(pEffectList->AddWithResources(
                    CLSID_MILEffectAlphaMask,
                    sizeof(alphaMask),
                    &alphaMask,
                    1,
                    &pbmOutputAsIUnknown
                    ));
            }
        }

        // Handle Opacity after the OpacityMask, and then only if there something to do.
        if (!IsCloseReal(layer.rAlpha, 1.0f) && (layer.rAlpha < 1.0f))
        {
            AlphaScaleParams alphaScale(layer.rAlpha);

            IFC(pEffectList->Add(
               CLSID_MILEffectAlphaScale,
               sizeof(alphaScale),
               &alphaScale
               ));
        }

        IFC(DrawLayer(layer, pEffectList));
    }
    
Cleanup:
    ReleaseInterfaceNoNULL(pEffectList);
    ReleaseInterfaceNoNULL(alphaMaskLayer.pbmOutput);
    ReleaseInterfaceNoNULL(layer.pbmOutput);
    delete layer.pGeometricMaskShape;

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::CreateAndFillLayer
//
//  Synopsis:   Creates an intermediate layer at the specified bounds,
//              fills it with the specified brush, and returns the
//              resulting bitmap.
//
//------------------------------------------------------------------------------
HRESULT
CDrawingContext::CreateAndFillLayer(
    __in_ecount(1) CMilSlaveResource *pFillBrush,   // Brush to fill the layer
    __in_ecount(1) const CRectF<CoordinateSpace::LocalRendering> *pSurfaceBoundsLocalSpace,    // Bounds of intermediate layer to create
    __out_ecount(1) CLayer *pLayer  // Output layer
    )
{
    HRESULT hr = S_OK;

    CBrushRealizer *pBrushRealizer = NULL;

    Assert(pFillBrush && pSurfaceBoundsLocalSpace && pLayer);

    //
    // Render the fill brush into an intermediate layer
    //

    // Convert layer bounds in world space to double precision for use as the
    // brush sizing bounds.
    MilPointAndSizeD rcBoundsD;
    MilPointAndSizeDFromMilRectF(OUT rcBoundsD, *pSurfaceBoundsLocalSpace);

    // Push a new layer into which we will fill.  The resulting image is our OpacityMask.
    IFC(PushLayer(CLayer(), pSurfaceBoundsLocalSpace, true /* Force Intermediate */));

    // We're about to render - need to apply render state.
    ApplyRenderState();

    //
    // Retrieve the brush realizations.
    //

    IFC(GetBrushRealizer(
        pFillBrush,
        &m_brushContext,
        &pBrushRealizer
        ));

    //
    // Set up the brush context
    //

    m_brushContext.rcWorldBrushSizingBounds = rcBoundsD;
    m_brushContext.rcWorldSpaceBounds = CMilRectF::sc_rcInfinite;

    //
    // Call DrawInfinitePath to fill layer with fill brush.
    //
    IFC(m_pIRenderTarget->DrawInfinitePath(
        &m_contextState,
        &m_brushContext,
        pBrushRealizer
        ));


    // We're done with the layer, so Pop it and retrieve the image (aka the OpacityMask)
    IFC(PopLayer(pLayer));

Cleanup:
    if (pBrushRealizer)
    {
        pBrushRealizer->FreeRealizationResources();
        pBrushRealizer->Release();
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    PushNoModificationLayer
//
//  Synopsis:  Push a layer that doesn't effect rendering.  This doesn't do any
//             work, but it ensures that the corresponding Pop or PopLayer is
//             well paired.  Typically, this caller will use this method when
//             it has an opacity of >= 1, etc, such that a layer isn't actually
//             needed.
//-----------------------------------------------------------------------------

HRESULT CDrawingContext::PushNoModificationLayer()
{
    RRETURN(m_stateTypeStack.Push(StackStateTypeNoModification));
}

//+-----------------------------------------------------------------------------
//
//  Member:    PushLayer
//
//  Synopsis:  Begin a new sublayer into which all content will be rendered
//             until PopLayer is called.
//             Often, the results of PopLayer will then be composited via
//             "DrawLayer".
//             Optionally, the caller can pass a bounds in local space for this
//             layer - typically bounding the content destined for this layer.
//
//------------------------------------------------------------------------------

HRESULT CDrawingContext::PushLayer(
    CLayer layer,
    __in_ecount_opt(1) const CRectF<CoordinateSpace::LocalRendering> *pSurfaceBoundsLocalSpace,  // Bounds of surface in local space
    bool fForceIntermediate
    )
{
    //
    // Compute layer bounds
    //
    // The layer bounds should be the intersection of:
    //   1. Render target bounds
    //   2. Clip bounds
    //   3. Bounds of the content to be drawn into the layer
    //   4. Bounds of geometric mask
    //
    // Currently, the caller can pass this data in via the clip.
    // Additionally, if local bounds are provided in prcBoundspSurfaceBoundsLocalSpace,
    // they will be intersected.
    // The geometric mask bounds are dealt with here, when present.
    //
    // For case using BeginLayer it isn't strictly necessary to have the render
    // target bounds, but it can help the composition layer make better
    // decisions about when work really needs to be done.
    //

    HRESULT hr = S_OK;
    BOOL fBeganLayer = FALSE;
    bool fLayerStored = false;
    BOOL fPushedTransform = FALSE;
    BOOL fPushedClip = FALSE;
    IMILRenderTargetBitmap *prtbmLayer = NULL;
    IRenderTargetInternal *prtiLayer = NULL;
    MilPointAndSizeL rcLayer;

    CRectF<CoordinateSpace::PageInPixels> rcClip;
    CMILMatrix matrix;

    m_clipStack.Top(&rcClip);

    if (pSurfaceBoundsLocalSpace != NULL)
    {
        // pSurfaceBoundsLocalSpace is in local space, while clip is in device
        CRectF<CoordinateSpace::PageInPixels> surfaceBoundsWorldSpace;
        const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pWorldTransform = m_transformStack.GetTopByReference();

        //
        // Since a bitmap effect can potentially transform the bounds of a dirty sub-region in a number
        // of ways (including non-affine transforms, which are currently unsupported in native code)
        // we may need to render the entirety of the visual's inner bounds.  We call into the effect
        // to get the necessary bounds to draw.  In any other case, we clip the bounds
        // to the dirty region on top of the clip stack.
        //
        if (layer.pEffect != NULL)
        {
            MIL_THR(SetupEffectTransform(
                layer.pEffect,
                pSurfaceBoundsLocalSpace,
                &rcClip,
                pWorldTransform,
                OUT layer.scaleMatrix,
                OUT layer.restMatrix,
                OUT surfaceBoundsWorldSpace
                ));

            if (hr == WGXERR_BADNUMBER)
            {
                // We have a degenerated world transform, so we have nothing to draw.
                // This shouldn't ever happen, but we don't want to crash if it does, we'll just fail to render
                // the effect.
                Assert(false);
                MIL_THR(PushNoModificationLayer());
                goto Cleanup;
            }       

            // Set the clip to the inflated bounds returned from the Effect.
            rcClip = surfaceBoundsWorldSpace;
        }
        else
        {
            pWorldTransform->Transform2DBounds(*pSurfaceBoundsLocalSpace, surfaceBoundsWorldSpace);
            rcClip.Intersect(surfaceBoundsWorldSpace);
        }
    }

    if (rcClip.IsEmpty() || !rcClip.IsWellOrdered())
    {
        // We have an empty clip region, nothing to do...
        MIL_THR(PushNoModificationLayer());
        goto Cleanup;
    }

    if (rcClip.IsInfinite())
    {
        // Abort rendering if the clip is unbounded.  To determine the size of
        // the intermediate surface to create, a clip must always exist
        // when pushing effects.
        Assert (FALSE);
        IFC(WGXERR_WRONGSTATE);
    }

    if (layer.pGeometricMaskShape)
    {
        //
        // Combine geometry bounds with clip bounds to find smallest layer that
        // may be required, if a layer is even needed.
        //
        // The inflate below is used to make sure we account for any expansion
        // that may come from current antialiasing settings.  It is assumed
        // that the current coordinate space and antialiasing techniques may
        // not expand beyond a single unit of this space.  This problem may
        // happen if the current coordinate space is in a higher resolution
        // than the actual device resolution.  For example:
        //    1) with a maximum AA expansion of one pixel
        //    2) the current coordinate space was setup for 200 dpi
        //    3) the actual target dpi is 96
        // then physical expansion may be 1/96th of an inch, but this expansion
        // will only account for 1/200th of an inch.
        //
        // Therefore it is left to the target to provide a proper resolution
        // (dpi) from IRenderTargetInternal::GetDeviceTransform.
        //
        // Finally, since the bounds are intersected with the current clip
        // bounds there is no concern this inflate yields too large of a
        // result.
        //

        CRectF<CoordinateSpace::PageInPixels> rcGeomBound;

        bool fIsEmpty = (layer.pGeometricMaskShape->GetFigureCount() == 0);
        if (!fIsEmpty)
        {
            IFC(layer.pGeometricMaskShape->GetTightBoundsNoBadNumber(rcGeomBound));
            fIsEmpty = rcGeomBound.IsEmpty();
        }
        if (!fIsEmpty)
        {
            rcGeomBound.Inflate(1, 1);

            fIsEmpty = !rcClip.Intersect(rcGeomBound);
        }

        if (fIsEmpty)
        {
            //
            // We have an empty result, nothing to do except make sure all
            // subsequent rendering is ignored until the corresponding Pop
            // (which may be a PopLayer).
            //
            //   Empty clips can be better served
            //  by ignoring following rendering instructions and avoiding
            //  any realizations along the way, until corresponding Pop.
            //

            MIL_THR(PushExactClip(
                CMilRectF::sc_rcEmpty,
                TRUE // => push clip type on state stack
                ));
            // Note fPushedClip is not set to TRUE on success.  It is not
            // needed since this should be the last step before returing.
            goto Cleanup;
        }
    }

    hr = InflateRectFToPointAndSizeL(rcClip, rcLayer);
    if (WGXERR_BADNUMBER == hr)
    {
        //
        // We encountered a numerical error. Treat as if there were no clip.
        //
        hr = S_OK;
        MIL_THR(PushNoModificationLayer());
        goto Cleanup;
    }
    else
    {
        IFC(hr);
    }

    // Store the offset of this layer in the CLayer
    layer.ptLayerPosition.X = rcLayer.X;
    layer.ptLayerPosition.Y = rcLayer.Y;

    //
    // Create a sublayer
    //

    // PushEffects must not be called during a bounding passs
    //
    // This method doesn't handle the E_NOTIMPL HR returned from
    // CSwRenderTargetGetBounds:: CreateRenderTargetBitmap because
    // it assumes it isn't called during a bounding pass.  PushEffects
    // specifically avoids calling this method during a bounding pass.
    Assert(!IsBounding());

    ApplyRenderState();

    if (   !layer.pAlphaMaskBrush
        && !layer.pEffect
        && !fForceIntermediate)
    {
        //
        // Try to use render target layer suport
        //
        if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
        {
            EventWriteDWMDraw_BeginLayerStart(
                static_cast<float>(static_cast<UINT>(rcClip.left)), 
                static_cast<float>(static_cast<UINT>(rcClip.top)), 
                static_cast<float>(static_cast<UINT>(rcClip.right)),
                static_cast<float>(static_cast<UINT>(rcClip.bottom)));
        }

        // Emits an event for tracking IRT creation.  GetCurrentVisual() won't be called unless
        // event tracing is enabled and at a high enough verbosity.
        EventWriteWClientPotentialIRTResource(static_cast<CMilSlaveResource*>(GetCurrentVisual()));

        MIL_THR(m_pIRenderTarget->BeginLayer(
                    rcClip,
                    m_renderState.AntiAliasMode,
                    layer.pGeometricMaskShape,
                    NULL,
                    layer.rAlpha,
                    NULL
                    ));

        if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
        {
            EventWriteDWMDraw_BeginLayerEnd(
                static_cast<float>(static_cast<UINT>(rcClip.left)),
                static_cast<float>(static_cast<UINT>(rcClip.top)),
                static_cast<float>(static_cast<UINT>(rcClip.right)),
                static_cast<float>(static_cast<UINT>(rcClip.bottom)));
        }

        if (SUCCEEDED(hr))
        {
            fBeganLayer = TRUE;
        }
    }
    else
    {
        Assert(!fBeganLayer);
    }

    if (fBeganLayer)
    {
        //
        // New layer is already restricted to clip as it's size was computed
        // from the current clip, but sub-pushed effects may rely on the
        // current clip size so set the clip to the layer bounds.
        //
        // Note will be the same as current unless there is a geometric clip
        // that made things more restricted.
        //
        // Also note that the invalidate portion of PushExactClip is not likely
        // to be necessary since a layer has been pushed and render targets
        // shouldn't allow any rendering outside of those bounds.  The clip
        // state realization code is currently quick; so, we won't worry about
        // the invalidation.
        //

        IFC(PushExactClip(
            rcClip,
            FALSE // => do not push clip type on state stack
            ));
        fPushedClip = TRUE;

        //
        // Mark need for EndLayer
        //

        IFC(m_stateTypeStack.Push(StackStateTypeRTLayer));
    }
    else
    {
        //
        // RT layer support failed -> Use an intermediate RT
        //

        IntermediateRTUsage rtUsage;
        rtUsage.flags = IntermediateRTUsage::ForBlending;
        rtUsage.wrapMode = MilBitmapWrapMode::Extend;

        UINT intermediateWidth = rcLayer.Width;
        UINT intermediateHeight = rcLayer.Height;
        
        MilRTInitialization::Flags rtInit;
        // If we have an effect and an explicit effect render mode for a custom effect, 
        // we must respect that. Otherwise, we must create a hardware RT if our parent 
        // is hardware or a software RT if our parent is software.  The software effect
        // in hardware rendering case is handled by pushing a "dummy" software layer
        // in Presubgraph().
        if (layer.pEffect != NULL)
        {
            ShaderEffectShaderRenderMode::Enum effectRenderMode = layer.pEffect->GetShaderRenderMode();
            if (effectRenderMode == ShaderEffectShaderRenderMode::SoftwareOnly)
            {
                rtInit = MilRTInitialization::SoftwareOnly;
            }
            else if (effectRenderMode == ShaderEffectShaderRenderMode::HardwareOnly)
            {
                rtInit = MilRTInitialization::HardwareOnly;
            }
            else
            {
                rtInit = MilRTInitialization::ForceCompatible;
            }

            IFC(CalculateEffectTextureLimits(
                rcLayer.Width,
                rcLayer.Height,
                OUT intermediateWidth,
                OUT intermediateHeight,
                OUT layer.surfaceScaleX,
                OUT layer.surfaceScaleY
                ));
            
            // If we've scaled down the intermediate surface, we need to draw our content scaled down to fit.
            if (layer.surfaceScaleX != 1.0f || layer.surfaceScaleY != 1.0f)
            {
                layer.scaleMatrix.Scale(layer.surfaceScaleX, layer.surfaceScaleY);
            }

            // Store the intermediate size on the layer.
            layer.uIntermediateHeight = intermediateHeight;
            layer.uIntermediateWidth = intermediateWidth;
        }
        // If we're rendering in hardware, but we need to render an effect in software, 
        // we pushed a software layer first, then the effect layer (since the effect layer
        // must be compatible).
        else if (layer.isDummyEffectLayer)
        {
            rtInit = MilRTInitialization::SoftwareOnly;
        }
        else
        {
            rtInit = MilRTInitialization::Default;
        }
        
        IFC(m_pIRenderTarget->CreateRenderTargetBitmap(
            intermediateWidth,
            intermediateHeight,
            rtUsage,
            rtInit,
            &prtbmLayer
            ));

        if (MCGEN_ENABLE_CHECK(MICROSOFT_WINDOWS_WPF_PROVIDER_Context, WClientCreateIRT))
        {
            unsigned int effectType = 0L;

            if (layer.pEffect != NULL)
            {
                effectType = IRT_Effect;
            }
            else if (layer.pAlphaMaskBrush != NULL)
            {
                effectType = IRT_OpacityMask;
            }
            else if (layer.isDummyEffectLayer)
            {
                effectType = IRT_Software_Only_Effects;
            }
            else
            {
                effectType = IRT_OpacityMask_Brush_Realization;
            }

            CMilSlaveResource* currentVisual = static_cast<CMilSlaveResource*>(GetCurrentVisual());
            EventWriteWClientCreateIRT(currentVisual, NULL, effectType);
        }
        
        // If we have an image effect, we need to apply the scale and un-offset to render our 
        // element into our intermediate surface.  We will apply the rest of the world transform
        // and re-offset in DrawEffectLayer.
        if (layer.pEffect != NULL)
        {
            CMILMatrix matScale = CMILMatrix(layer.scaleMatrix);

            // If our intermediate is being created at an offset, we need to un-offset to render
            if ((0 != layer.ptLayerPosition.X) ||
                (0 != layer.ptLayerPosition.Y))
            {
              matScale.SetTranslation(
                    -static_cast<float>(layer.ptLayerPosition.X),
                    -static_cast<float>(layer.ptLayerPosition.Y)
                    );
            }

            // Replace the world transform with our decomposed transform to render the image effect content
            IFC(PushTransform(&matScale, false));

            //  Future Consideration:  Split out ImageEffect code from PushLayer
            // We set fHasOffset to true to handle cleaning up the transform we pushed, even if there wasn't
            // an offset.  When we split out the image effect code this will get cleaned up.
            layer.fHasOffset = TRUE; 
            fPushedTransform = TRUE;
        }
        // Do we need to translate?
        else if ((0 != layer.ptLayerPosition.X) ||
            (0 != layer.ptLayerPosition.Y))
        {
            // Translate the drawings which use to be targetting the top left corner to the origin
            IFC(PushTransformPostOffset(
                -static_cast<float>(layer.ptLayerPosition.X),
                -static_cast<float>(layer.ptLayerPosition.Y)
                ));
            layer.fHasOffset = TRUE;
            fPushedTransform = TRUE;
        }

        // New layer is already restricted to clip as it's size was computed from
        // the current clip, but sub-pushed effects may rely on the current clip
        // size so set the clip to the current surface bounds.

        IFC(PushExactClip(
            CMilRectF(0, 0,
                         static_cast<FLOAT>(rcLayer.Width),
                         static_cast<FLOAT>(rcLayer.Height),
                         XYWH_Parameters),
            FALSE // => do not push clip type on state stack
            ));
        fPushedClip = TRUE;

        IFC(prtbmLayer->QueryInterface(
            IID_IRenderTargetInternal,
            reinterpret_cast<void **>(&prtiLayer)
            ));

        //
        // Clear the render target to blank
        //

        {
            MilColorF colBlank = {0, 0, 0, 0};
            IFC(prtiLayer->Clear(&colBlank));
        }

        layer.prtTargetPrev = m_pIRenderTarget;
        WHEN_DBG_ANALYSIS(layer.dbgTargetPrevCoordSpaceId = m_dbgTargetCoordSpaceId);


        IFC(m_layerStack.Push(layer));
        fLayerStored = true;
        MIL_THR(m_stateTypeStack.Push(StackStateTypeBitmapLayer));
        if (FAILED(hr))
        {
            m_layerStack.Pop();
            goto Cleanup;
        }

        // The layer is safely ensconced in the stack - go ahead and put a ref on the render target.
        layer.prtTargetPrev->AddRef();

        MIL_THR(ChangeRenderTarget(
            prtiLayer
            DBG_ANALYSIS_COMMA_PARAM(m_dbgTargetCoordSpaceId)
            ));

        if (FAILED(hr))
        {
            // The pushed layered must be cleaned up now to maintain a consistent
            // state.  Caller is responsible for layer data they created, but which
            // won't be tracked because of this failure.  For example
            // layer.pGeometricMaskShape.
            layer.prtTargetPrev->Release();
            m_stateTypeStack.Pop();
            m_layerStack.Pop();
            goto Cleanup;
        }
    }

Cleanup:
    if (FAILED(hr))
    {
        if (fPushedTransform)
        {
            PopTransform();
        }

        if (fPushedClip)
        {
            PopClip(FALSE);
        }

        if (fBeganLayer)
        {
            // Just clean up any state changes from successful begin.  For that
            // reason and because we are already retuning failure it is okay to
            // ignore the result.  (The render targets are still required to
            // unroll their state on EndLayer even if they can't complete
            // rendering fixups/effects.)

            IGNORE_HR(m_pIRenderTarget->EndLayer());

        }
    }
    else if (!fLayerStored)
    {
        //
        // Clean up CLayer object that isn't stored.
        //
        // Upon success callers expect CLayer structure to be stored and that
        // layer.pGeometricMaskShape is owned by it.  Just clean it up here.
        //
        delete layer.pGeometricMaskShape;
    }

    ReleaseInterface(prtbmLayer);
    ReleaseInterface(prtiLayer);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    PopLayer
//
//  Synopsis:  End the current layer, returning it.
//             Often, the returned layer is then used with DrawLayer.
//             If the layer was a fake layer, a NULL bitmap will be returned in pLayer.
//             If the layer does return a bitmap in pLayer.pbmOutput, the caller
//             now owns the reference to this bitmap.
//
//------------------------------------------------------------------------------

HRESULT CDrawingContext::PopLayer(
    __out_ecount(1) CLayer* pLayer
    )
{
    HRESULT hr = S_OK;

    IMILRenderTargetBitmap *prtBitmap = NULL;
    IWGXBitmapSource *pbmSource = NULL;

    // The default constructor for CLayer will correctly initialize it to No-Op.
    CLayer layerTop;
    StackStateType sst;

    //
    // Pop the top of the stack-state stack
    //

    if (!(m_stateTypeStack.Pop(&sst)))
    {
        RIP("Stack is empty. Pop returns false");
        IFC(E_UNEXPECTED);
    }

    // Future Consideration:   Call Pop from PostSubgraph to simplify
    //  special cases in PopEffects/PopLayer.

    //
    // Handle all of the special cases that PushLayer may actually push.
    //

    if (sst != StackStateTypeBitmapLayer)
    {
        Assert(   (sst == StackStateTypeClip)
               || (sst == StackStateTypeRTLayer)
               || (sst == StackStateTypeNoModification));

        if (sst != StackStateTypeNoModification)
        {
            //
            // Call with fPopState set to FALSE as there state stack should not
            // be popped.  There are two cases:
            //   1) the clip stack state type was just popped
            //   2) RT Layer is being resolved
            //
            PopClip(FALSE);
        }

        if (sst == StackStateTypeRTLayer)
        {
            if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
            {
                EventWriteDWMDraw_EndLayerStart();
            }


            IFC(m_pIRenderTarget->EndLayer());

            if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
            {
                EventWriteDWMDraw_EndLayerEnd();
            }
        }

        // This was a fake or RT handled, there is nothing more to do.
        pLayer->pbmOutput = NULL;
    }
    else
    {
        //
        // Pop the top of the layer stack, if a no-op or RT layer wasn't pushed.
        //

        Assert(sst == StackStateTypeBitmapLayer);
        Verify(m_layerStack.Pop(&layerTop));

        //
        // Pop the sublayer
        //

        // m_pIRenderTarget must be an IMILRenderTargetBitmap, because
        // PushLayer must have set it to one.

        IFC(m_pIRenderTarget->QueryInterface(
            IID_IMILRenderTargetBitmap,
            reinterpret_cast<void **>(&prtBitmap)
            ));

        IFC(ChangeRenderTarget(layerTop.prtTargetPrev DBG_ANALYSIS_COMMA_PARAM(layerTop.dbgTargetPrevCoordSpaceId)));

        // Pop the clip.
        //  Don't pop state stack as Bitmap layer doesn't push one for clip.
        PopClip(FALSE);

        // Did we need to translate?
        if (layerTop.fHasOffset)
        {
            PopTransform();
        }

        //
        // Return the popped sublayer in layerTop.pbmOutput.
        //
        
        IFC(prtBitmap->GetBitmapSource(&pbmSource));

        layerTop.pbmOutput = pbmSource;
        pbmSource = NULL;

        // If we have a bitmap effect, we need to save the render
        // target for use in the effects pipeline.
        if (layerTop.pEffect != NULL)
        {
            layerTop.prtbmOutput = prtBitmap;
            prtBitmap = NULL; // transferring reference to prtbmOutput
        }
    }

Cleanup:
    // If we succeeded, let's fill the out param.
    if (SUCCEEDED(hr))
    {
        *pLayer = layerTop;
    }

    ReleaseInterface(layerTop.prtTargetPrev);
    ReleaseInterface(pbmSource);   
    ReleaseInterface(prtBitmap);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    SetupEffectTransform
//
//  Synopsis:  Returns the scale and rest matrix for rendering an Effect.
//
//------------------------------------------------------------------------------

HRESULT
CDrawingContext::SetupEffectTransform(
    __in_ecount(1) CMilEffectDuce *pEffect,
    __in_ecount(1) const CRectF<CoordinateSpace::LocalRendering> *pSurfaceBoundsLocalSpace,
    __in_ecount(1) CRectF<CoordinateSpace::PageInPixels> *prcClip,
    __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pWorldTransform,
    __out_ecount(1) CMILMatrix &scaleMatrix,
    __out_ecount(1) CMILMatrix &restMatrix,
    __out_ecount(1) CRectF<CoordinateSpace::PageInPixels> &surfaceBoundsWorldSpace
    )
{
    HRESULT hr = S_OK;
    
    CRectF<CoordinateSpace::LocalRendering> rClippedSurfaceBoundsLocalSpace;
    CMILMatrix matScale;
    CMILMatrix matRest;
    BOOL canDecompose;
    
    // We try to clip the area we render into for the bitmap effect, but the size of the clipped
    // region is effect dependent.
    IFC(pEffect->GetLocalSpaceClipBounds(*pSurfaceBoundsLocalSpace, *prcClip, pWorldTransform, &rClippedSurfaceBoundsLocalSpace));

    // For bitmap effects, we need to apply the rotation after rendering with the effect since 
    // effects can be rotation-dependent (like a mirror effect).  We apply the scale component
    // here, and cache both components in the layer.
    pWorldTransform->DecomposeMatrixIntoScaleAndRest(&matScale, &matRest, &canDecompose);

    Assert(canDecompose);
    if (!canDecompose)
    {
        IFC(WGXERR_BADNUMBER);
    }

    scaleMatrix = matScale;
    restMatrix = matRest;
    
    //  Future Consideration:   Note that from this point forward the effect bounds are not, strictly speaking
    //          in either world or local space.  We should add new spaces to CRectF 
    //          or remove the parameterized types altogether to make this code more correct.
    matScale.Transform2DBounds(rClippedSurfaceBoundsLocalSpace, OUT surfaceBoundsWorldSpace);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    CalculateEffectTextureLimits
//
//  Synopsis:  Calculates max texture size limitations.
//
//------------------------------------------------------------------------------

HRESULT
CDrawingContext::CalculateEffectTextureLimits(
    __in UINT uTextureWidthIn,
    __in UINT uTextureHeightIn,
    __out UINT &uTextureWidthOut,
    __out UINT &uTextureHeightOut,
    __out float &uScaleX,
    __out float &uScaleY
    )
{
    HRESULT hr = S_OK;
    
    const CDisplaySet *pDisplaySet = NULL;
    // Custom effects are not clipped to the window bounds, so they
    // could request a very large intermediate surface.  Instead of
    // failing in this case, we clamp the surface to the max texture
    // size, which can cause some pixelation but will render with the
    // effect applied.
    DWORD renderTargetType = 0;
    IFC(m_pIRenderTarget->GetType(&renderTargetType));
    UINT uMaxWidth;
    UINT uMaxHeight;
    if (renderTargetType == HWRasterRenderTarget)
    {
        g_DisplayManager.GetCurrentDisplaySet(&pDisplaySet);
        MilGraphicsAccelerationCaps caps;
        pDisplaySet->GetGraphicsAccelerationCaps(true, NULL, &caps);
        
        uMaxWidth = caps.MaxTextureWidth;
        uMaxHeight = caps.MaxTextureHeight;
    }
    else
    {
        Assert(renderTargetType == SWRasterRenderTarget || renderTargetType == DummyRenderTarget);
        // The width and height are converted to floats when clipping,
        // so we clamp to the largest value allowed for a SW intermediate.
        uMaxWidth = uMaxHeight = MAX_EFFECT_SW_INTERMEDIATE_SIZE;
    }

    // Set the out args as though there were no limitation.
    uScaleX = uScaleY = 1.0f;
    uTextureWidthOut = uTextureWidthIn;
    uTextureHeightOut = uTextureHeightIn;
    
    // Limit the size of the intermediate if necessary.
    if (static_cast<UINT>(uTextureWidthIn) > uMaxWidth)
    {
        uTextureWidthOut = uMaxWidth;
        uScaleX = static_cast<FLOAT>(uMaxWidth) / static_cast<FLOAT>(uTextureWidthIn);
    }
    if (static_cast<UINT>(uTextureHeightIn) > uMaxHeight)
    {
        uTextureHeightOut = uMaxHeight;
        uScaleY = static_cast<FLOAT>(uMaxHeight) / static_cast<FLOAT>(uTextureHeightIn);
    }

Cleanup:
    ReleaseInterface(pDisplaySet);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    DrawEffectLayer
//
//  Synopsis:  Composite the layer which contains an Effect.
//
//------------------------------------------------------------------------------

HRESULT CDrawingContext::DrawEffectLayer(
    CLayer layer
    )
{
    HRESULT hr = S_OK;
    
    //
    // Before going further make sure clipping and transform state are applied.
    //

    ApplyRenderState();


    // Temporarily set the world transform to apply the rest of the world transform
    // (rotate+offset) for the ComposeEffect call. 
    // ApplyRenderState needs to be called before this to apply the clip. If ApplyRenderState
    // is called after this block it will overwrite the world transform.
    {
        CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> matComposed = layer.restMatrix;

        // If we've scaled down for texture limits, we need to draw our layer content scaled back up.
        if (layer.surfaceScaleX != 1.0f || layer.surfaceScaleY != 1.0f)
        {
            CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::LocalRendering> matTextureScale(true); // Set to identity
            matTextureScale.Scale(1.0f / layer.surfaceScaleX, 1.0f / layer.surfaceScaleY);
            // Prepend the scale.
            matComposed.SetToMultiplyResult(matTextureScale, matComposed);
        }

        // If we have an offset, re-apply it
        if ((0 != layer.ptLayerPosition.X) ||
            (0 != layer.ptLayerPosition.Y))
        {
            CMILMatrix matOffset;
            matOffset.SetToIdentity();
            matOffset.SetTranslation(
                static_cast<float>(layer.ptLayerPosition.X),
                static_cast<float>(layer.ptLayerPosition.Y)
                );

            matComposed.CBaseMatrix::SetToMultiplyResult(matOffset, matComposed);
        }

        TemporarilySetWorldTransform(
            matComposed
            );
        
    }
    
    IFC(m_pIRenderTarget->ComposeEffect(
        &m_contextState, 
        &layer.scaleMatrix, 
        layer.pEffect,
        layer.uIntermediateWidth,
        layer.uIntermediateHeight,
        layer.prtbmOutput
        ));

Cleanup:

    // Undo TemporarilySet* by resetting the current transform and clip
    // to the values on top of the clip & transform stacks
    ApplyRenderState();

    // Release the saved render target
    ReleaseInterface(layer.prtbmOutput);
    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  Member:    DrawLayer
//
//  Synopsis:  Composite the layer.
//
//------------------------------------------------------------------------------

HRESULT CDrawingContext::DrawLayer(
    CLayer layer,
    __inout_ecount_opt(1) IMILEffectList *pEffectList
    )
{
    HRESULT hr = S_OK;

    // We cache the old AntiAliasMode in case we need to update/restore it.
    MilAntiAliasMode::Enum oldAntiAliasMode = m_renderState.AntiAliasMode;
    MilBitmapInterpolationMode::Enum oldInterpolationMode = m_renderState.InterpolationMode;


    // If there's nothing to do, well, do nothing.
    if (NULL == layer.pbmOutput)
    {
        // Haven't done anything - just return
        RRETURN(S_OK);
    }

    // 
    //   For this kind of operation, we want to draw without using the context state
    //   (wrap mode, transform, filter mode). We also don't want to have to temporarily
    //   reset these things since that's error-prone.
    // [2005/03/12 JasonHa] So make sure RT layer support covers all cases

    CMILMatrix matLayerToTarget(true);

    // Did we need to translate?
    if (layer.fHasOffset)
    {
        matLayerToTarget.SetTranslation(
            static_cast<float>(layer.ptLayerPosition.X),
            static_cast<float>(layer.ptLayerPosition.Y)
            );
    }

    //
    // Setup Rendering State
    //
    // Temporarily set operation to a 1:1 mapping - this means:
    //  1) integer translation only
    //  2) nearest neighbor sampling to avoid precision issues
    //  3) no antialiasing, unless mask is being applied now
    //
    //

    //
    // Before going further make sure clipping state is properly applied
    //

    ApplyRenderState();

    m_renderState.InterpolationMode = MilBitmapInterpolationMode::NearestNeighbor;

    //
    // Set WorldToDevice matrix to identity
    //
    // The layer shape is pre-transformed into target space.  This occurs
    // during PushLayer for geometric clips, and during the setup of
    // nonClippedLayerShape when there is no geometric clip.
    TemporarilySetWorldTransform(
        CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels>::refIdentity()
        );

    //
    // Setup Fill Brush
    //
    // Use a temporary bitmap brush to be passed to DrawPath.  This stack
    // brush may not be reference counted since its lifetime is exactly the
    // scope in which it is defined, but no longer.  LocalMILObject helps
    // enforce this via asserts on checked builds.
    //

    LocalMILObject<CMILBrushBitmap> bbBrush;

    // Now World == Target(Device) space so LayerToTarget can be used as
    // BrushToWorld.

    {
        Assert(m_contextState.WorldToDevice.IsIdentity());

        CMILBrushBitmapLocalSetterWrapper brushBitmapLocalWrapper(
            &bbBrush,
            layer.pbmOutput,   // !No AddRef!
            MilBitmapWrapMode::Extend,
            &matLayerToTarget, //  pmatBitmapToXSpace
            // NOTICE-2006/04/30-JasonHa  Above meta only WorldSpace may be set
            //  since the meta layer must always be given a chance to adjust
            //  transforms based on actual configuration of sub-RTs.
            //  Technically, Page space could be set here, but there is no code
            //  to have meta inspect this brush and adjust the transform to
            //  device space.  Finally note that LocalRendering to Page
            //  transform is indentity, making it easy to just specify
            //  LocalRendering (which is BaseSampling for 2D and known as
            //  WorldSpace here).
            XSpaceIsWorldSpace
            DBG_COMMA_PARAM(NULL) // pmatDBGWorldToSampleSpace
            );

        //
        // Setup Fill Shape
        //
        // Create a shape to fill with the intermediate layer surface.  This is
        // a geometric clip if one is set, or just the bounds of the intermediate
        // surface otherwise.
        //

        IShapeData *pLayerShape;
        CParallelogram nonClippedLayerShape;

        if (layer.pGeometricMaskShape)
        {
            // If a geometric clip is applied, fill the clip shape with
            // the intermediate layer content.
            pLayerShape = layer.pGeometricMaskShape;
        }
        else
        {
            // If a geometric clip isn't applied, create a fill-shape that
            // is equal to the device-space bounds of the intermediate layer.
            CMilRectF layerBounds;

            Assert (layer.pbmOutput); // Checked for at the beginning of the function
            IFC(GetBitmapSourceBounds(
                layer.pbmOutput,
                &layerBounds
                ));
            nonClippedLayerShape.Set(layerBounds);

            // Geometric clips are transformed into device space during
            // PushEffects, so this shape also needs to be transformed into
            // device space.  For an intermediate layer, only the layer offset
            // is needed for that transformation.
            nonClippedLayerShape.Transform(&matLayerToTarget);

            pLayerShape = &nonClippedLayerShape;

            // Don't anti-alias the edges of the intermediate layer.  We
            // just need to blt the intermediate surface without any additional
            // filtering of the edges.
            m_renderState.AntiAliasMode = MilAntiAliasMode::None;
        }

        //
        // Draw the Layer
        //

        {
            LocalMILObject<CImmediateBrushRealizer> fillBrush;
            fillBrush.SetMILBrush(
                &bbBrush,
                pEffectList,
                false // don't skip meta-fixups
                );

            Assert(m_contextState.WorldToDevice.IsIdentity());

            IFC(m_pIRenderTarget->DrawPath(
                &m_contextState,
                NULL,
                pLayerShape,
                NULL,
                NULL,
                &fillBrush
                ));
        }

        if (IsTagEnabled(tagTintPushOpacitySurfaces))
        {
            //
            // Draw a faint rectangle over the same area.
            //
            // (Hopefully. Watch for errors in this.)
            //

            MilColorF colRect = {0.0f, 0.8f, 0.9f, 0.2f};
            MilPointAndSizeF rcRect;
            UINT uiWidth, uiHeight;

            IFC(layer.pbmOutput->GetSize(&uiWidth, &uiHeight));

            rcRect.X = static_cast<float>(layer.ptLayerPosition.X);
            rcRect.Y = static_cast<float>(layer.ptLayerPosition.Y);
            rcRect.Width = static_cast<float>(uiWidth);
            rcRect.Height = static_cast<float>(uiHeight);

            IFC(DrawRectangle(
                &colRect,
                &rcRect
                ));
        }
    }

Cleanup:

    //
    // Restore state we temporarily changed
    //
    // (This could be avoided if the RT owned sublayer creation.)
    //

    m_renderState.InterpolationMode = oldInterpolationMode;
    m_renderState.AntiAliasMode = oldAntiAliasMode;

    // Undo TemporarilySet* by resetting the current transform and clip
    // to the values on top of the clip & transform stacks
    ApplyRenderState();

    RRETURN(hr);
}

/*++

Routine Description:

    Pushes the specified offset onto the stack.

    The top of the stack represents the accumulated multiplication of
    every matrix pushed in the stack, rather than the last push. Pop reverts
    the last multiply thus maintaining the stack accumulation.

Arguments:

    offsetX, offsetY

Return Value:

    HRESULT

--*/

HRESULT CDrawingContext::PushOffset(
    float offsetX,
    float offsetY
    )
{
    HRESULT hr = S_OK;

    // Push the offset on the transform stack
    IFC(m_transformStack.PushOffset(offsetX, offsetY));

    // Push the offset on the stack-state stack
    IFC(PushTransformStackStateAndInvalidate());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    PushRenderOptions
//
//  Synopsis:  Push the specified render options and modify the render state to
//             reflect the current options.
//
//------------------------------------------------------------------------------

HRESULT CDrawingContext::PushRenderOptions(
    __in_ecount(1) const MilRenderOptions* pRenderOptions
    )
{
    HRESULT hr = S_OK;

    IFC(m_stateTypeStack.Push(StackStateTypeRenderOptions));

    //
    // Save the options in the render state that we might change.
    //
    SavedRenderOptions savedRenderOptions;
    savedRenderOptions.AntiAliasMode              = m_renderState.AntiAliasMode;
    savedRenderOptions.PrefilterEnable            = m_renderState.PrefilterEnable;
    savedRenderOptions.InterpolationMode          = m_renderState.InterpolationMode;
    savedRenderOptions.CompositingMode            = m_renderState.CompositingMode;
    savedRenderOptions.ClearTypeHint              = m_fClearTypeHint;
    savedRenderOptions.TextRenderingMode          = m_renderState.TextRenderingMode;
    savedRenderOptions.TextHintingMode            = m_renderState.TextHintingMode;

    MIL_THR(m_renderOptionsStack.Push(savedRenderOptions));

    // If the render options stack push failed, pop from the state stack to prevent
    // the two stacks from becoming mismatched.
    if (FAILED(hr))
    {
        m_stateTypeStack.Pop();
        goto Cleanup;
    }

    //
    // Modify the desired render options
    //
    if ((pRenderOptions->Flags & MilRenderOptionFlags::EdgeMode) &&
        (pRenderOptions->EdgeMode == MilEdgeMode::Aliased))
    {
        m_renderState.AntiAliasMode = MilAntiAliasMode::None;
    }

    if ((pRenderOptions->Flags & MilRenderOptionFlags::BitmapScalingMode) &&
        (pRenderOptions->BitmapScalingMode != MilBitmapScalingMode::Unspecified))
    {
        switch(pRenderOptions->BitmapScalingMode)
        {
            case MilBitmapScalingMode::HighQuality:
            // case MilBitmapScalingMode::Fant:
                m_renderState.PrefilterEnable = TRUE; // Fant interpolation is currently implemented as a pre-filter.
                m_renderState.InterpolationMode = DefaultInterpolationMode;
                break;
                
            case MilBitmapScalingMode::LowQuality:
            // case MilBitmapScalingMode::Linear:
                m_renderState.PrefilterEnable = FALSE;
                m_renderState.InterpolationMode = MilBitmapInterpolationMode::Linear;
                break;
                
            case MilBitmapScalingMode::NearestNeighbor:
                m_renderState.PrefilterEnable = FALSE;
                m_renderState.InterpolationMode = MilBitmapInterpolationMode::NearestNeighbor;
                break;
        }
    }

    // Save the clear type hint.
    if ((pRenderOptions->Flags & MilRenderOptionFlags::ClearTypeHint) && 
        (pRenderOptions->ClearTypeHint == MilClearTypeHint::Enabled))
    {
        m_fClearTypeHint = true;
        IFC(m_pIRenderTarget->SetClearTypeHint(m_fClearTypeHint));
    }

    if (pRenderOptions->Flags & MilRenderOptionFlags::CompositingMode)
    {
        m_renderState.CompositingMode = pRenderOptions->CompositingMode;
    }

    if (pRenderOptions->Flags & MilRenderOptionFlags::TextRenderingMode)
    {
        m_renderState.TextRenderingMode = pRenderOptions->TextRenderingMode;
    }

    if (pRenderOptions->Flags & MilRenderOptionFlags::TextHintingMode)
    {
        m_renderState.TextHintingMode = pRenderOptions->TextHintingMode;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    PopRenderOptions
//
//  Synopsis:  Pop the render options, which involves restoring the rendering to
//             the previous render options.
//
//------------------------------------------------------------------------------

HRESULT CDrawingContext::PopRenderOptions()
{
    HRESULT hr = S_OK;
    
    StackStateType sst;
    SavedRenderOptions savedRenderOptions;

    // Pop from the state-type stack
    Verify(m_stateTypeStack.Pop(&sst));
    Assert(sst == StackStateTypeRenderOptions);

    Verify(m_renderOptionsStack.Pop(&savedRenderOptions));

    // If our clear type hint is changing, reset it on the render target.
    if (m_fClearTypeHint != savedRenderOptions.ClearTypeHint)
    {
        IFC(m_pIRenderTarget->SetClearTypeHint(savedRenderOptions.ClearTypeHint));
    }
    
    //
    // Restore the render state options
    //
    m_renderState.AntiAliasMode         = savedRenderOptions.AntiAliasMode;
    m_renderState.PrefilterEnable       = savedRenderOptions.PrefilterEnable;
    m_renderState.InterpolationMode     = savedRenderOptions.InterpolationMode;
    m_renderState.CompositingMode       = savedRenderOptions.CompositingMode;
    m_fClearTypeHint                    = savedRenderOptions.ClearTypeHint;
    m_renderState.TextRenderingMode     = savedRenderOptions.TextRenderingMode;
    m_renderState.TextHintingMode       = savedRenderOptions.TextHintingMode;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    Pop
//
//  Synopsis:  Pop from the top of the conceptual stack. (The fact that we use
//             separate stacks for clip, layer and transform, is an
//             implementation detail.)
//
//             Remark: The pop implementation will still cleanup all the stacks
//             even if pop should return a failure.
//
//------------------------------------------------------------------------------

HRESULT CDrawingContext::Pop()
{
    HRESULT hr = S_OK;

    StackStateType sst;

    // The stack implementation returns an E_FAIL if the stack is empty or
    // E_INVALIDARG if the out argument passed to it is NULL. Since we don't expect
    // either here, this method must always succeeded. Therefore we can assert the
    // SUCCESS of this method.
    Assert(!m_stateTypeStack.IsEmpty());
    MIL_THR(m_stateTypeStack.Top(&sst));
    Assert(SUCCEEDED(hr));

    switch (sst)
    {
    case StackStateTypeClip:
        PopClip(TRUE);
        break;
    case StackStateTypeTransform:
        PopTransform();
        break;
    case StackStateTypeGuidelineCollection:
        PopGuidelineCollection();
        break;
    case StackStateTypeRTLayer:
        // No special Pop* method.  PopEffects has to handle this case anyway
        // so just reuse its logic.  PopEffects has to handle this because
        // PreSubgraph calls PushEffects which may set a type other than
        // bitmap layer and then PostSubgraph calls PopEffects.
        //
        // Future Consideration:   Call Pop from PostSubgraph to simplify
        //  special cases in PopEffects/PopLayer.
    case StackStateTypeBitmapLayer:
        IFC(PopEffects());
        break;
    case StackStateTypeRenderOptions:
        PopRenderOptions();
        break;
    case StackStateTypeNoModification:
        m_stateTypeStack.Pop(&sst);
        break;
    default:
#if DBG
        Assert(FALSE);
#else
        __assume(false);
#endif
    }

Cleanup:
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// DrawVisualTree
//
//   Renders the composition scene graph.
//
//   Arguments:
//         pRoot         - Pointer to the root of the composition graph.
//         pClearColor   - if not NULL this color is used to clear the area that is
//                         rendered. (note that this area is restricted by the
//                         specified clip.
//         pClip         - clip
//
//---------------------------------------------------------------------------------

HRESULT
CDrawingContext::DrawVisualTree(
    __in_ecount(1) CMilVisual *pRoot,
    __in_ecount_opt(1) MilColorF const *pClearColor,
    __in_ecount(1) CMilRectF const &dirtyRect,
    bool fDrawingIntoVisualBrush
    )
{
    HRESULT hr = S_OK;
    bool fWasDrawingIntoVisualBrush = m_fDrawingIntoVisualBrush;

    m_fDrawingIntoVisualBrush = fDrawingIntoVisualBrush;

    AssertConstMsg(m_pGraphIterator, "There is a problem with using the render context from the UiThread. You can only call this for visuals.");

    // This rectangle may represent either a dirty rectangle or the bounds of
    // the target surface.  In the later case it is still important because any
    // effects that may get pushed, which don't themselves have a notion of
    // bounds such as PushOpacity, will only have this clip as a basis for
    // creating an intermediate surface.

    IFC(PushClipRect(dirtyRect));

    if (pClearColor)
    {
        //
        // The intersection of the clip state and the clip rect that was
        // just pushed should be rectangular, giving us the opportunity to
        // use an aliased clip.
        //
        // Reason: the clip stack, at the top of this function, should either
        // be empty or contain a rectangle equal to the surface bounds.
        //

        CMilRectF deviceClipRect;
        m_clipStack.Top(&deviceClipRect);

        if (pClearColor && ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
        {
            EventWriteDWMDraw_ClearStart(deviceClipRect.left, deviceClipRect.top, deviceClipRect.right, deviceClipRect.bottom);
        }

        {
            CAliasedClip aliasedClip(&deviceClipRect);

            IFC(m_pIRenderTarget->Clear(pClearColor, &aliasedClip));
        }

        if (pClearColor && ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
        {
            EventWriteDWMDraw_ClearEnd();
        }
    }

    // If we are drawing a node with a cache, it should be valid.  To update a node with
    // an invalid cache use DrawCacheVisualTree.
    Assert(pRoot->m_pCaches == NULL || pRoot->m_pCaches->IsValid());

    IFC(m_pGraphIterator->Walk(pRoot, this));

    // Only call Pop during success because the stack may become mismatched during a failure.
    // This is because PostSubgraph will not have been called during a failure, which pops all
    // of the transforms pushed in PreSubgraph.
    PopClip(TRUE);

Cleanup:
    m_fDrawingIntoVisualBrush = fWasDrawingIntoVisualBrush;
 
    RRETURN(hr);
}

//---------------------------------------------------------------------------------
//
//  CDrawingContext::DrawCacheVisualTree
//
//  Synopsis: 
//     Renders the composition scene graph of pRoot into its cache.
//
//---------------------------------------------------------------------------------

HRESULT
CDrawingContext::DrawCacheVisualTree(
    __in_ecount(1) CMilVisual *pRoot,
    __in_ecount(1) MilColorF const *pClearColor,
    __in_ecount(1) CMilRectF const &dirtyRect,
    bool fDrawingIntoVisualBrush
    )
{
    HRESULT hr = S_OK;
    bool fWasDrawingIntoVisualBrush = m_fDrawingIntoVisualBrush;

    m_fDrawingIntoVisualBrush = fDrawingIntoVisualBrush;

    AssertConstMsg(m_pGraphIterator, "There is a problem with using the render context from the UiThread. You can only call this for visuals.");

    // This rectangle may represent either a dirty rectangle or the bounds of
    // the target surface.  In the later case it is still important because any
    // effects that may get pushed, which don't themselves have a notion of
    // bounds such as PushOpacity, will only have this clip as a basis for
    // creating an intermediate surface.

    IFC(PushClipRect(dirtyRect));

    //
    // The intersection of the clip state and the clip rect that was
    // just pushed should be rectangular, giving us the opportunity to
    // use an aliased clip.
    //
    // Reason: the clip stack, at the top of this function, should either
    // be empty or contain a rectangle equal to the surface bounds.
    //

    {
        CMilRectF deviceClipRect;
        m_clipStack.Top(&deviceClipRect);

        if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
        {
            EventWriteDWMDraw_ClearStart(deviceClipRect.left, deviceClipRect.top, deviceClipRect.right, deviceClipRect.bottom);
        }

        CAliasedClip aliasedClip(&deviceClipRect);

        IFC(m_pIRenderTarget->Clear(pClearColor, &aliasedClip));

        if (ETW_ENABLED_CHECK(TRACE_LEVEL_VERBOSE) && !IsBounding())
        {
            EventWriteDWMDraw_ClearEnd();
        }
    }
    
    // Since we are drawing directly into a cache we should encounter exactly one
    // invalid cache node - the root node of our walk.
    Assert(pRoot->m_pCaches && !pRoot->m_pCaches->IsValid());

    // Since we ignore all the properties above the cache when drawing into it, we'll just
    // render the node's content, and then its children.
    IFC(pRoot->RenderContent(this));

    for (UINT i = 0; i < pRoot->GetChildrenCount(); i++)
    {
        IGraphNode *pChild = pRoot->GetChildAt(i);
        IFC(m_pGraphIterator->Walk(pChild, this));
    }
    
    // Only call Pop during success because the stack may become mismatched during a failure.
    // This is because PostSubgraph will not have been called during a failure, which pops all
    // of the transforms pushed in PreSubgraph.
    PopClip(TRUE);

Cleanup:
    // If anything fails in this function, the drawing context is cleaned up in the
    // EndFrame code. New cleanup code should be added there.
    m_fDrawingIntoVisualBrush = fWasDrawingIntoVisualBrush; 

    RRETURN(hr);
}


//---------------------------------------------------------------------------------
//  Returns true if the state of pNode allows its cache to be re-used
//  as an input to its Effect without creating another intermediate surface.
//---------------------------------------------------------------------------------

bool
CDrawingContext::CanUseCacheAsEffectInput(
    __in_ecount(1) CMilVisual const *pNode, 
    __in_ecount(1) CRectF<CoordinateSpace::LocalRendering> const *prcBoundsNotInflated
    )
{
    HRESULT hr = S_OK;
    IMILRenderTargetBitmap *pIRTB = NULL;
    IRenderTargetInternal *pIRT = NULL;
    
    Assert(pNode->m_pCaches != NULL 
        && pNode->m_pCaches->IsNodeCacheValid()
        && pNode->m_pEffect != NULL);

    // If the node has opacity or an opacity mask set, return false since both
    // must be applied to the cache texture before we can execute the Effect.
    if (pNode->m_pAlphaMaskWrapper != NULL || !IsCloseReal(static_cast<float>(pNode->m_alpha), 1.0f))
    {
        return false;
    }

    // If the Effect requires the input area be padded, return false since the
    // cache texture is not padded.  The cache will be drawn into a padded
    // intermediate for the Effect input instead.
    CMilRectF bounds = *prcBoundsNotInflated;
    IFC(pNode->m_pEffect->TransformBoundsForInflation(&bounds));
    if (!prcBoundsNotInflated->IsEquivalentTo(bounds))
    {
        return false;
    }

    // If our cache texture is in software, all rendering is in software.
    IFC(pNode->m_pCaches->GetNodeCacheRenderTargetBitmap(
        &pIRTB,
        m_pIRenderTarget
        DBG_ANALYSIS_COMMA_PARAM(m_dbgTargetCoordSpaceId)
        ));

    // If we have no cache texture we'll have nothing to draw, we can just return true.
    if (pIRTB == NULL)
    {
        return true;
    }
    
    IFC(pIRTB->QueryInterface(IID_IRenderTargetInternal, (void **)&pIRT));

    // If our cache texture and destination texture are incompatible,
    // skip the optimization.
    DWORD rtTypeDest;
    DWORD rtTypeCache;
    IFC(m_pIRenderTarget->GetType(&rtTypeDest));
    IFC(pIRT->GetType(&rtTypeCache));
    if (rtTypeCache != rtTypeDest)
    {
        return false;
    }

    // If either our destination texture is incompatible with the
    // shader's render mode or machine limitations, skip the optimization.
    EffectCompositionMode effectCompositionMode;
    IFC(DetermineEffectCompositionMode(pNode->m_pEffect, &effectCompositionMode));
    if (effectCompositionMode != RenderCompatible)
    {
        return false;
    }

Cleanup:
    ReleaseInterface(pIRTB);
    ReleaseInterface(pIRT);
    
    // If any of these calls failed, don't try the optimization.
    if (FAILED(hr))
    {
        return false;
    }

    return true;
}

//---------------------------------------------------------------------------------
// CDrawingContext::UseCacheAsEffectInput
//
//  An optimization that uses the cache texture on pNode as input directly
//  into the Effect on this pNode.  
//---------------------------------------------------------------------------------

HRESULT
CDrawingContext::DrawEffect(
    __in_ecount(1) CMilVisual const *pNode, 
    __in_ecount(1) CRectF<CoordinateSpace::LocalRendering> const *prcBounds
    )
{
    HRESULT hr = S_OK;
    
    CMILMatrix matLocalToSurf;
    IMILRenderTargetBitmap *pCacheRTB = NULL;
    IWGXBitmapSource *pBitmapSource = NULL;
    
    CLayer layer;
    layer.pEffect = pNode->m_pEffect;

    MilPointAndSizeL rcLayer;
    CRectF<CoordinateSpace::PageInPixels> rcClip;
    m_clipStack.Top(&rcClip);

    CRectF<CoordinateSpace::PageInPixels> surfaceBoundsWorldSpace;
    const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pWorldTransform = m_transformStack.GetTopByReference();

    IFC(SetupEffectTransform(
        pNode->m_pEffect, 
        prcBounds, 
        &rcClip, 
        pWorldTransform,
        OUT layer.scaleMatrix,
        OUT layer.restMatrix,
        OUT surfaceBoundsWorldSpace
        ));

    rcClip = surfaceBoundsWorldSpace;

    if (rcClip.IsEmpty() || !rcClip.IsWellOrdered())
    {
        // We have an empty clip region, nothing to do...
        goto Cleanup;
    }

    if (rcClip.IsInfinite())
    {
        // Abort rendering if the clip is unbounded.  To determine the size of
        // the intermediate surface to create, a clip must always exist
        // when pushing effects.
        Assert (FALSE);
        IFC(WGXERR_WRONGSTATE);
    }

    hr = InflateRectFToPointAndSizeL(rcClip, rcLayer);
    if (WGXERR_BADNUMBER == hr)
    {
        //
        // We encountered a numerical error. Treat as if there were no clip.
        //
        hr = S_OK;
        goto Cleanup;
    }
    else
    {
        IFC(hr);
    }

    // Store the offset of this layer in the CLayer
    layer.ptLayerPosition.X = rcLayer.X;
    layer.ptLayerPosition.Y = rcLayer.Y;

    // Secondary inputs will still be sized to the "implicit input" size - the scaled-to-world 
    // local bounds of the node, limited by max texture limits.  This ensures they are sized
    // consistently when the node is cached, not cached, or doesn't use the implicit input at all.
    float unusedScaleX;
    float unusedScaleY;
    IFC(CalculateEffectTextureLimits(
        rcLayer.Width,
        rcLayer.Height,
        OUT layer.uIntermediateWidth,
        OUT layer.uIntermediateHeight,
        OUT unusedScaleX,
        OUT unusedScaleY
        ));

    // The scale factors for the max texture size only matter if we're realizing the implicit
    // input, which we won't do here (since we're either not using it or we're using the cache).
    layer.surfaceScaleX = layer.surfaceScaleY = 1.0f;

    // Set the implicit input texture to the cache or to null.
    if (pNode->m_fUseCacheAsEffectInput)
    {
        // Pass ref to layer, it will be cleaned up by DrawEffectLayer.
        IFC(pNode->m_pCaches->GetNodeCacheRenderTargetBitmap(
            OUT &pCacheRTB,
            m_pIRenderTarget
            DBG_ANALYSIS_COMMA_PARAM(m_dbgTargetCoordSpaceId)
            ));

        // If our cache texture is null we have nothing to draw, just skip rendering.
        if (pCacheRTB == NULL)
        {
            goto Cleanup;
        }
    }

    layer.prtbmOutput = pCacheRTB;
    
    IFC(DrawEffectLayer(layer));
    
Cleanup:
    ReleaseInterface(pBitmapSource);
    
    RRETURN(hr);
} 

//---------------------------------------------------------------------------------
// IGraphIteratorSink PreSubgraph
//
//   PreSubgraph is called before the sub-graph of a node is visited. With the
//   output argument fVisitChildren the implementor can control if the sub-graph
//   of this node should be visited at all.
//---------------------------------------------------------------------------------

HRESULT
CDrawingContext::PreSubgraph(__out_ecount(1) BOOL *pfVisitChildren)
{
    HRESULT hr = S_OK;
    BOOL fPushEffect = FALSE;
    CRectF<CoordinateSpace::LocalRendering> localBounds;
    CRectF<CoordinateSpace::PageInPixels> clippedBoundsWorldAAInflated;

    AssertMsg(m_pGraphIterator, "There is a problem with using the render context from the UiThread. You can only call this for visuals.");

    CMilVisual* pNode = GetCurrentVisual();
    Assert(pNode);

    // Track the current resource so for IRT event tracing.
    CMilSlaveResource* savedResource = m_pComposition->GetCurrentResourceNoRef();
    m_pComposition->SetCurrentResource(static_cast<CMilSlaveResource*>(pNode));

    //
    // For now we assume that we need to render this node and all its children.
    //
    *pfVisitChildren = TRUE;
    pNode->m_fSkipNodeRender = FALSE;
    pNode->m_fUseCacheAsEffectInput = FALSE;
    bool fSkipNodeRenderBelowEffect = false;
     
    //
    // First, we handle the node's render options, as this may affect the bounds
    //

    if (pNode->m_renderOptionsFlags)
    {
        MilRenderOptions renderOptions;
        renderOptions.Flags             = pNode->m_renderOptionsFlags;
        renderOptions.EdgeMode          = static_cast<MilEdgeMode::Enum>(pNode->m_edgeMode);
        renderOptions.BitmapScalingMode = static_cast<MilBitmapScalingMode::Enum>(pNode->m_bitmapScalingMode);
        renderOptions.ClearTypeHint     = static_cast<MilClearTypeHint::Enum>(pNode->m_clearTypeHint);
        renderOptions.CompositingMode   = static_cast<MilCompositingMode::Enum>(pNode->m_compositingMode);
        renderOptions.TextRenderingMode = static_cast<MilTextRenderingMode::Enum>(pNode->m_textRenderingMode);
        renderOptions.TextHintingMode   = static_cast<MilTextHintingMode::Enum>(pNode->m_textHintingMode);       

        IFC(PushRenderOptions(&renderOptions));
    }
    
    localBounds = CRectF<CoordinateSpace::LocalRendering>::ReinterpretNonSpaceTyped(pNode->m_Bounds);

    //
    // Gets the nodes bounds in world space, i.e. with world transform and world clipped applied. 
    // We get the bounds with inflation for anti-aliasing (if appropriate) applied, clipped to the world clip. 
    
    GetClippedWorldSpaceBounds(
        &localBounds, 
        &clippedBoundsWorldAAInflated);        

    if (clippedBoundsWorldAAInflated.IsEmpty() && g_fDirtyRegion_Enabled)
    {
        pNode->m_fSkipNodeRender = TRUE;
        *pfVisitChildren = FALSE;
        goto Cleanup;
    }

    //
    // For debug purposes we can disable the dirty region code (g_fDirtyRegion_Enabled).
    //

    //
    // Check if the alpha value of this node is 0 which would mean that we can
    // bail out. Note: alpha values of 1 (or more) are expected to be handled
    // efficiently by PushOpacity.
    //
    
    // If there's no opacity, let's go ahead and skip this subgraph
    float flAlphaValue = static_cast<float>(ClampAlpha(pNode->m_alpha));
    if (IsCloseReal(flAlphaValue, 0.0f))
    {
        pNode->m_fSkipNodeRender = TRUE;
        *pfVisitChildren = FALSE;
        goto Cleanup;
    }
    
    //    
    // Special TS clip goes above all other modifiers
    // Note that we have to apply this clip even if we aren't actually able to accelerate
    // the scroll (eg if we're in hardware) to ensure consistent look between hardware
    // and software
    // See comment on CPreComputeContext::ScrollableAreaHandling()
    //
    if (pNode->m_pScrollBag)
    {
        CMilRectF clipRect = pNode->m_pScrollBag->clipRect;

        CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> transform;
        m_transformStack.Top(&transform);

        CRectF<CoordinateSpace::LocalRendering> localClip = CRectF<CoordinateSpace::LocalRendering>::ReinterpretNonSpaceTyped(clipRect);
        CRectF<CoordinateSpace::PageInPixels> worldSnappedClip;

        CMilVisual::TransformAndSnapScrollableRect(&transform, NULL, &localClip, &worldSnappedClip);
        
        IFC(PushClipRect(*static_cast<CMilRectF*>(&worldSnappedClip)));
    } 

    // Let's find out if we have any effects.
    fPushEffect = pNode->HasEffects();

    // If we're pushing effects, push the bounds as the clip.  This is done so that the
    // alpha code has a better idea how large to make the surface.
    if (fPushEffect)
    {
        IFC(PushClipRect(
            clippedBoundsWorldAAInflated 
            ));
    }

    //
    // Push offset, transform, clip, and effects.
    //

    // If there's a scroll bag we may need to offset this node even if its offset is 0,0
    // because that may not be a 0 offset when transformed and snapped in world space. Fun!
    if (pNode->m_pScrollBag)
    {
        // Must round offset to integer size
        CMilPoint2F offset(pNode->m_offsetX, pNode->m_offsetY);

        CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> transform;
        m_transformStack.Top(&transform);
        
        IFC(CMilVisual::TransformAndSnapOffset(&transform, &offset, true));

        IFC(PushOffset(offset.X, offset.Y));
    }
    else
    {
        if ((pNode->m_offsetX != 0.0f) || (pNode->m_offsetY != 0.0f))
        {
            IFC(PushOffset(pNode->m_offsetX, pNode->m_offsetY));
        }
    }

    if (pNode->m_pTransform != NULL)
    {
        const CMILMatrix *pMatrix;
        IFC(pNode->m_pTransform->GetMatrix(&pMatrix));
        IFC(PushTransform(pMatrix));
    }
    
    // If we have an image effect, we need to push effects in a specific order to account
    // for the different code path for image effect layers, and to match existing bitmap
    // effect behavior. (Clip > Bitmap or Image Effect > Opacity Mask and Opacity)
    if (fPushEffect)
    {
        // If we have a valid cache, we can skip creating a layer for opacity and just
        // draw the cached bitmap with opacity instead.
        float opacity = (pNode->m_pCaches && pNode->m_pCaches->IsNodeCacheValid()) ? 1.0f : static_cast<float>(pNode->m_alpha);
        
        if (pNode->m_pEffect != NULL)
        {
            if (pNode->m_pClip != NULL)
            {
                IFC(PushClip(pNode->m_pClip));
            }

            // If the Effect doesn't use its implicit input (the content and children of this node), there's
            // no need to create an intermediate and render any of it, we can just run the Effect.  The
            // Effect will realize its secondary inputs itself.
            fSkipNodeRenderBelowEffect = !(pNode->m_pEffect->UsesImplicitInput());
            
            //
            // For opacity mask and image effects, we need to calculate the inner bounds of the
            // visual to create intermediates.  Image effects need these bounds because they do
            // not support dirty sub-regions (see PushLayer).  Opacity mask needs them for the 
            // creation of a correctly-sized secondary layer to render the mask for blending (see 
            // PopEffects).
            //
            CRectF<CoordinateSpace::LocalRendering> rcEffectBounds;
            IFC(m_pContentBounder->GetVisualInnerBounds(pNode, &rcEffectBounds));
            CRectF<CoordinateSpace::LocalRendering> rcOpacityBounds(rcEffectBounds);

            // If we have a valid cache on this node there are some scenarios where we can use
            // the cache texture directly as input to an Effect, instead of creating a new layer.
            pNode->m_fUseCacheAsEffectInput =  !fSkipNodeRenderBelowEffect
                                               && pNode->m_pCaches != NULL
                                               && pNode->m_pCaches->IsNodeCacheValid() 
                                               && CanUseCacheAsEffectInput(pNode, &rcEffectBounds);

            // Push layers for the Effect and the other properties, if need be.
            IFC(pNode->m_pEffect->TransformBoundsForInflation(&rcEffectBounds));

            EffectCompositionMode effectCompositionMode;
            IFC(DetermineEffectCompositionMode(pNode->m_pEffect, &effectCompositionMode));

            // If we can render the effect without creating an intermediate render target, do
            // that here.
            if (fSkipNodeRenderBelowEffect || pNode->m_fUseCacheAsEffectInput)
            {   
                switch (effectCompositionMode)
                {
                    case RenderCompatible:
                        // Draw the effect directly without creating a layer.
                        IFC(DrawEffect(pNode, &rcEffectBounds));
                        break;
                    case SkipRender:
                        // Do nothing.
                        break;
                    case PushDummyAndRenderSoftware:
                        // We shouldn't hit this case when using the cache texture directly, it's
                        // explicitly disallowed in CanUseCacheAsEffectInput.  Since we run the
                        // effect directly on the cache texture, we can't use the hardware cache
                        // texture to run a software effect.
                        Assert(!pNode->m_fUseCacheAsEffectInput);
                        
                        IFC(PushDummyLayer(&rcEffectBounds));

                        // Draw the effect without creating another layer.  The dummy layer
                        // ensures we run the effect in software.  
                        IFC(DrawEffect(pNode, &rcEffectBounds));
                        break;
                    default:
                        Assert(false);
                }
                
            }
            else
            {
                // Otherwise, push layers for the Effect and the other properties, if they are set.
                
                // If we are rendering in hardware but need to render an effect in software
                // (either because we have no ps_2_0 support or we the effect's render mode
                // was explicitly set to SW) we must push a "dummy" software layer to render into.
                switch (effectCompositionMode)
                {
                    case RenderCompatible:
                        // If we can render the effect, push the layer for the image effect to be executed on.  
                        // We will create a compatible render target for the effect in PushLayer.
                        IFC(PushImageEffect(pNode->m_pEffect, &rcEffectBounds));
                        break;
                        
                    case SkipRender:
                        // If we can't render the effect, render the content without it.
                        IFC(PushNoModificationLayer());
                        break;
                        
                    case PushDummyAndRenderSoftware:
                        IFC(PushDummyLayer(&rcEffectBounds));

                        // Push the layer for the image effect to be executed on.
                        IFC(PushImageEffect(pNode->m_pEffect, &rcEffectBounds));
                        break;
                        
                    default:
                        Assert(false);
                }

                // Opacity and opacity mask can be handled with one layer.
                if (pNode->m_pAlphaMaskWrapper != NULL)
                {
                    IFC(PushEffects(opacity, NULL, pNode->GetAlphaMask(), NULL, &rcOpacityBounds));
                }
                // If there's no opacity mask, we still need a layer for opacity but we don't need the inner bounds.
                else if (opacity != 1.0f)
                {
                    IFC(PushEffects(opacity, NULL, NULL, NULL, NULL));
                }
            }
        }
        // If we don't have an image effect we can push all the other effects with one layer.
        else
        {
            //
            // Opacity mask needs the inner bounds of the visual to create intermediates for the
            // creation of a correctly-sized secondary layer to render the mask for blending (see 
            // PopEffects).
            //
            if(pNode->m_pAlphaMaskWrapper != NULL)
            {
                CRectF<CoordinateSpace::LocalRendering> rcOpacityBounds;
                IFC(m_pContentBounder->GetVisualInnerBounds(pNode, &rcOpacityBounds));
                IFC(PushEffects(
                    opacity,
                    pNode->m_pClip,
                    pNode->GetAlphaMask(),
                    NULL,
                    &rcOpacityBounds));
            }
            else
            {
                // Passing in null for the bounds will use the dirty region data on the clipStack
                // to determine the bounds for rendering.
                IFC(PushEffects(
                    opacity,
                    pNode->m_pClip,
                    pNode->GetAlphaMask(),
                    NULL,
                    NULL));
            }
        }
    }
    
    // If (pNode->m_pGuidelineCollection == null) then PushGuidelineCollection
    // should be called anyway. The rule for visual is that content of visual is
    // never affected by parent's guidelines (in oppose to drawing group).
    bool fNeedMoreCycles = false;
    IFC(PushGuidelineCollection(pNode->m_pGuidelineCollection, fNeedMoreCycles));
    if (fNeedMoreCycles)
    {
        IFC(pNode->ScheduleRender());
    }    
 
    // Caches are updated after precompute but before the render walk, so whenever 
    // we encounter a node with caches in the render walk, the caches should be valid.
    Assert(pNode->m_pCaches == NULL || pNode->m_pCaches->IsValid());

    // If we don't need to realize the implicit input for our Effect, stop rendering.
    if (fSkipNodeRenderBelowEffect || pNode->m_fUseCacheAsEffectInput)
    {
        Assert(pNode->m_pEffect != NULL);
        *pfVisitChildren = FALSE;
    }
    // If we have a valid cache we don't need to visit this node's children 
    // since this node and its subtree have already been rendered in the cache.
    // If this is the root node of a walk to update an invalid cache (say for a bitmap
    // cache brush), we don't want to draw into it with cached content since our cache
    // might have different cache parameters.
    else if (pNode->m_pCaches && pNode->m_pCaches->IsNodeCacheValid())
    {
        // If we have a valid cache we don't need to visit this node's children 
        // since this node and its subtree have already been rendered in the cache.
        *pfVisitChildren = FALSE;

        // Draw the cache texture into the back buffer.
        IFC(pNode->m_pCaches->RenderNodeCache(
            this,
            m_pIRenderTarget,
            static_cast<float>(pNode->m_alpha)
            DBG_ANALYSIS_COMMA_PARAM(m_dbgTargetCoordSpaceId)
            ));
    }
    else
    {
        // Render the content of the node.
        IFC(pNode->RenderContent(this));
    }

Cleanup:
    
    m_pComposition->SetCurrentResource(savedResource);

    // Note that in case of a failure the graph walker will stop immediately. More importantly
    // there is nothing that is equivalent to the stack unwinding in the recursive case.
    // So cleaning out the stacks has to happen in a different place. This is now done in the
    // prologue of the DrawVisualTree method.

    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// IGraphIteratorSink PostSubgraph
//
//   PostSubgraph is called after the sub-graph of a node was visited.
//---------------------------------------------------------------------------------

HRESULT
CDrawingContext::PostSubgraph()
{
    HRESULT hr = S_OK;
    BOOL fPushEffect = FALSE;
    
    AssertMsg(m_pGraphIterator, "There is a problem with using the render context from the UiThread. You can only call this for visuals.");

    CMilVisual* pNode = GetCurrentVisual();
    Assert(pNode);
  
    // Track the current resource so for IRT event tracing.
    CMilSlaveResource* savedResource = m_pComposition->GetCurrentResourceNoRef();
    m_pComposition->SetCurrentResource(static_cast<CMilSlaveResource*>(pNode));

    if (!pNode->m_fSkipNodeRender)
    {
        PopGuidelineCollection();

        // Find out if we have any effects
        fPushEffect = pNode->HasEffects();

        // If we *do* have any effects, pop them.
        if (fPushEffect)
        {       
            // If we have a valid cache, we skipped creating a layer for opacity and just
            // drew the cached bitmap with opacity instead.
            float opacity = (pNode->m_pCaches && pNode->m_pCaches->IsNodeCacheValid()) ? 1.0f : static_cast<float>(pNode->m_alpha);
            
            // If we have an image effect, we may have called PushEffects multiple times.
            if (pNode->m_pEffect != NULL)
            {
                bool fSkipNodeRenderBelowEffect = !(pNode->m_pEffect->UsesImplicitInput());
                
                // If we optimized the Effect layer away using a cache or because we didn't need to realize the
                // implicit input, we don't need to pop the Effect nor anything below it.
                if (!fSkipNodeRenderBelowEffect && !pNode->m_fUseCacheAsEffectInput)
                {
                    // Otherwise, pop all the layers we pushed for effects and the other properties.
                    if (pNode->m_pAlphaMaskWrapper != NULL || opacity != 1.0f)
                    {
                        IFC(PopEffects());
                    }
                    
                    // Pop the image effect
                    IFC(PopEffects());
                }

                // Whenever an effect is rendered, we need to check to see if a dummy software layer 
                // was pushed to force the effect to run in software.
                IFC(PopLayerIfDummy());

                if (pNode->m_pClip != NULL)
                {
                    IFC(PopEffects());
                }
            }
            // If we don't have an image effect, we only called PushEffects once.
            else
            {
                IFC(PopEffects());
            }
        }

        // Pop transform.
        if (pNode->m_pTransform != NULL)
        {
            PopTransform();
        }
    
        // Pop offset if we pushed one.
        if (   pNode->m_pScrollBag
            || (pNode->m_offsetX != 0.0f) 
            || (pNode->m_offsetY != 0.0f))
        {
            PopTransform();
        }

        // Also, we now need to Pop a Clip which was added to bound the subgraph (for effects)
        if (fPushEffect)
        {
            PopClip(TRUE);
        }

        // Pop special TS clip if we have one.
        if (pNode->m_pScrollBag)
        {
            PopClip(TRUE);
        }            
    }

    // Next, we consider the render options.
    if (pNode->m_renderOptionsFlags)
    {
        PopRenderOptions();
    } 

Cleanup:

    m_pComposition->SetCurrentResource(savedResource);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::PushDummyLayer
//
//  Synopsis:   Pushes a dummy software layer.  Used to enable rendering 
//              software effects in a hardware rendering context.
//
//------------------------------------------------------------------------------

HRESULT
CDrawingContext::PushDummyLayer(
    __in CRectF<CoordinateSpace::LocalRendering> *pBounds
    )
{
    CLayer dummyLayer = CLayer(1.0f, NULL, NULL, NULL, pBounds);
    // Setting this flag forces a software-only layer to be created.
    dummyLayer.isDummyEffectLayer = true;
    RRETURN(PushLayer(dummyLayer, pBounds, true));
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::PopLayerIfDummy
//
//  Synopsis:   Pops the top layer off the layer stack and draws it, if
//              it is a dummy software layer.  Used for rendering software
//              effects in a hardware rendering context.
//
//------------------------------------------------------------------------------

HRESULT
CDrawingContext::PopLayerIfDummy()
{
    HRESULT hr = S_OK;
    CLayer topLayer;  // initializes its fields

    if (!m_layerStack.IsEmpty())
    {
        IFC(m_layerStack.Top(&topLayer));
        if (topLayer.isDummyEffectLayer)
        {
            // We now own the topLayer.pbmOutput reference
            IFC(PopLayer(&topLayer));
            if (topLayer.pbmOutput != NULL)
            {
                IFC(DrawLayer(topLayer, NULL));
            }
        }
    }

Cleanup:
    ReleaseInterfaceNoNULL(topLayer.pbmOutput);
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::CheckEffectSupport
//
//  Synopsis:   Returns true for hw if shader effects can be render with hardware
//              acceleration (requires ps_2_0) and true for sw if they can
//              render with software HLSL JIT (requires SSE2).
//
//------------------------------------------------------------------------------
void
CDrawingContext::CheckEffectSupport(
    __out bool *pHasHardwareSupport,
    __out bool *pHasSoftwareSupport,
    __in bool requiresPS30)
{
    // If we do not have hardware support for effects we must render them into a
    // software layer.  We might be rendering into a hardware layer even without
    // hardware effects support (fixed function) so we need to push an additional
    // software layer to account for this.
    const CDisplaySet *pDisplaySet = NULL;
    g_DisplayManager.GetCurrentDisplaySet(&pDisplaySet);

    MilGraphicsAccelerationCaps caps;
    pDisplaySet->GetGraphicsAccelerationCaps(true, NULL, &caps);
    byte majorVSVersion = D3DSHADER_VERSION_MAJOR(caps.VertexShaderVersion);
    byte majorPSVersion = D3DSHADER_VERSION_MAJOR(caps.PixelShaderVersion);

    ReleaseInterface(pDisplaySet);
    
    bool hasHWSupport = false;
    if (   (majorVSVersion >= 3 && majorPSVersion >= 3) 
        || (!requiresPS30 && majorVSVersion == 2 && majorPSVersion == 2))
    {
        hasHWSupport = true;
    }

    *pHasHardwareSupport = hasHWSupport;
    *pHasSoftwareSupport = !requiresPS30 && !!caps.HasSSE2Support;
}

//+-----------------------------------------------------------------------------
//
//  Member:     CDrawingContext::DetermineEffectCompositionMode
//
//  Synopsis:   Returns RenderCompatible if we can simply render the hw effect
//              into our hw RT or our sw effect into our sw RT.
//              Returns PushDummyAndRenderSoftware if we are rendering in hw
//              but we need to render the effect in software.
//              Returns SkipRender if we cannot render the effect given its
//              ShaderEffectShaderRenderMode and the current machine config.
// Summary of return results:
// HW/SW RT | HW support | SW support | ShaderRenderMode | RESULT
// HW         Y            Y            Auto               RenderCompatible (HW)
// HW         Y            Y            HWOnly             RenderCompatible (HW)
// HW         Y            Y            SWOnly             PushDummyAndRenderSoftware
// HW         Y            N            Auto               RenderCompatible (HW)
// HW         Y            N            HWOnly             RenderCompatible (HW)
// HW         Y            N            SWOnly             SkipRender
// HW         N            Y            Auto               PushDummyAndRenderSoftware
// HW         N            Y            HWOnly             SkipRender
// HW         N            Y            SWOnly             PushDummyAndRenderSoftware
// HW         N            N            Auto               SkipRender
// HW         N            N            HWOnly             SkipRender
// HW         N            N            SWOnly             SkipRender
// SW         Y            Y            Auto               RenderCompatible (SW)
// SW         Y            Y            HWOnly             SkipRender
// SW         Y            Y            SWOnly             RenderCompatible (SW)
// SW         Y            N            Auto               SkipRender
// SW         Y            N            HWOnly             SkipRender
// SW         Y            N            SWOnly             SkipRender
// SW         N            Y            Auto               RenderCompatible (SW)
// SW         N            Y            HWOnly             SkipRender
// SW         N            Y            SWOnly             RenderCompatible (SW)
// SW         N            N            Auto               SkipRender
// SW         N            N            HWOnly             SkipRender
// SW         N            N            SWOnly             SkipRender

//------------------------------------------------------------------------------
HRESULT
CDrawingContext::DetermineEffectCompositionMode(
    __in CMilEffectDuce * pEffect,
    __out EffectCompositionMode * pEffectCompositionMode
    )
{
    HRESULT hr = S_OK;
    
    // Note that the call to GetType() will return HW for Meta RTs if any of the
    // displays is being rendered in hardware.  This means that on any software
    // displays in that situation, we will be pushing an unnecessary layer.
    DWORD renderTargetType = 0;
    IFC(m_pIRenderTarget->GetType(&renderTargetType));
    
    bool hasHardwareSupport = false;
    bool hasSoftwareSupport = false;
    bool requiresPS30 = (pEffect->GetShaderMajorVersion() == 3);
    CheckEffectSupport(&hasHardwareSupport, &hasSoftwareSupport, requiresPS30);

    ShaderEffectShaderRenderMode::Enum effectRenderMode = pEffect->GetShaderRenderMode();

    if (renderTargetType == HWRasterRenderTarget)
    {
        if (hasHardwareSupport && hasSoftwareSupport)
        {
            switch (effectRenderMode)
            {
                case ShaderEffectShaderRenderMode::Auto :
                case ShaderEffectShaderRenderMode::HardwareOnly :
                    *pEffectCompositionMode = RenderCompatible;
                    break;
                case ShaderEffectShaderRenderMode::SoftwareOnly :
                    *pEffectCompositionMode = PushDummyAndRenderSoftware;
                    break;
                default :
                    Assert(false);
            }
        }
        else if (!hasHardwareSupport && hasSoftwareSupport)
        {
            switch (effectRenderMode)
            {
                case ShaderEffectShaderRenderMode::Auto :
                case ShaderEffectShaderRenderMode::SoftwareOnly :
                    *pEffectCompositionMode = PushDummyAndRenderSoftware;
                    break;
                case ShaderEffectShaderRenderMode::HardwareOnly :
                    *pEffectCompositionMode = SkipRender;
                    break;
                default :
                    Assert(false);
            }
        }
        else if (hasHardwareSupport && !hasSoftwareSupport)
        {
            switch (effectRenderMode)
            {
                case ShaderEffectShaderRenderMode::Auto :
                case ShaderEffectShaderRenderMode::HardwareOnly :
                    *pEffectCompositionMode = RenderCompatible;
                    break;
                case ShaderEffectShaderRenderMode::SoftwareOnly :
                    *pEffectCompositionMode = SkipRender;
                    break;
                default :
                    Assert(false);
            }
        }
        else
        {
            Assert(!hasHardwareSupport && !hasSoftwareSupport);
            *pEffectCompositionMode = SkipRender;
        }
    }
    else 
    {
        // We should only ever have a HW or SW render target in our drawing context,
        // since we're in a render pass and not a bounding pass.  We may also have
        // a dummy RT (if the device is invalid), which we can safely ignore.
        Assert(renderTargetType == SWRasterRenderTarget || renderTargetType == DummyRenderTarget);

        if (hasSoftwareSupport)
        {
            switch (effectRenderMode)
            {
                case ShaderEffectShaderRenderMode::Auto :
                case ShaderEffectShaderRenderMode::SoftwareOnly :
                    *pEffectCompositionMode = RenderCompatible;
                    break;
                case ShaderEffectShaderRenderMode::HardwareOnly :
                    *pEffectCompositionMode = SkipRender;
                    break;
                default :
                    Assert(false);
            }
        }
        else 
        {
            Assert(!hasSoftwareSupport);
            // If we're rendering in SW with no SW effects support, we can't do anything.
            *pEffectCompositionMode = SkipRender;
        }
    }

Cleanup:
    RRETURN(hr);
   
}

//---------------------------------------------------------------------------------
// DrawRectangle
//
//    This is a helper function for debug purposes that draws a simple
//    rectangle.
//---------------------------------------------------------------------------------

HRESULT
CDrawingContext::DrawRectangle(
    __in_ecount(1) MilColorF const *pColor,
    __in_ecount(1) MilPointAndSizeF const *pRect
    )
{
    Assert((pColor != NULL) && (pRect != NULL));

    HRESULT hr = S_OK;
    CShape shape;

    ApplyRenderState();

    IFC(shape.AddRectangle(pRect->X, pRect->Y, pRect->Width, pRect->Height));

    {
        LocalMILObject<CImmediateBrushRealizer> fillBrush;
        fillBrush.SetSolidColorBrush(pColor);

        IFC(m_pIRenderTarget->DrawPath(
            &m_contextState,
            NULL,
            &shape,
            NULL,
            NULL,
            &fillBrush
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CDrawingContext::GetBrushRealizer
//
//  Synopsis:  Captures the information necessary to realize a brush in the
//             CBrushRealizer instance. This instance can then be sent down
//             to the internal render target, which is responsible for asking
//             the CBrushRealizer to realize itself.
//

HRESULT CDrawingContext::GetBrushRealizer(
    __in_ecount_opt(1) CMilSlaveResource *pBrush,
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_out_ecount(1) CBrushRealizer **ppBrushRealizer
    )
{
    HRESULT hr = S_OK;

    CBrushRealizer *pBrushRealizer = NULL;

    if (IsBounding())
    {
        if (pBrush != NULL)
        {
            pBrushRealizer = m_pCachedNullBrushRealizer;
            pBrushRealizer->AddRef();
        }
        else
        {
            Assert(pBrushRealizer == NULL);
        }
    }
    else
    {
        if (pBrush == NULL)
        {
            pBrushRealizer = m_pCachedNullBrushRealizer;
            pBrushRealizer->AddRef();
        }
        else
        {
            // Update the brush sizing bounds & viewable extents for the current
            // drawing instruction

            CMilBrushDuce *pBrushRes;

            IFC(GetTypeSpecificResource(
                pBrush,
                TYPE_BRUSH,
                &pBrushRes // doesn't add ref
                ));

            IFC(pBrushRes->GetRealizer(
                pBrushContext,
                &pBrushRealizer
                ));
        }
    }

    *ppBrushRealizer = pBrushRealizer;

Cleanup:
    RRETURN(hr);
}

HRESULT CDrawingContext::GetBitmapSource(
    __in_ecount_opt(1) CMilSlaveResource *pImage,
    __out_ecount(1) CMilRectF *prcSrc,
    __deref_out_ecount_opt(1) IWGXBitmapSource **ppBitmapSource
    )
{
    HRESULT hr = S_OK;

    *ppBitmapSource = NULL;

    if (pImage)
    {
        if (pImage->IsOfType(TYPE_IMAGESOURCE))
        {
            CMilImageSource *pImageSource = NULL;
            pImageSource = DYNCAST(CMilImageSource, pImage);
            Assert(pImageSource);

            IFC(pImageSource->GetBitmapSource(ppBitmapSource));
            if (*ppBitmapSource != NULL)
            {
                IFC(pImageSource->GetBounds(NULL, prcSrc));
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
    }

    if (!*ppBitmapSource)
    {
        RtlZeroMemory(prcSrc, sizeof(*prcSrc));
    }

Cleanup:
    RRETURN(hr);
}

// This function is necessary because of 2 hacks:
//
// 1) PushOpacity/PopOpacity is implemented in the context, not the render target
// 2) We don't yet have a decent way to store/retrieve the clip realization.

// This method addrefs the new RT.

HRESULT CDrawingContext::ChangeRenderTarget(
    __in_ecount(1) IRenderTargetInternal *pirtNew
    DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Enum dbgTargetCoordSpaceId)
    )
{
    HRESULT hr = S_OK;

    Assert(pirtNew);

    InvalidateClipRealization();

    // Set the new render target
    ReplaceInterface(m_pIRenderTarget, pirtNew);
#if DBG_ANALYSIS
    // Invalidate renderstate setting for Out coordinate space
    if (m_dbgTargetCoordSpaceId != dbgTargetCoordSpaceId)
    {
        m_fDbgTargetSpaceChanged = true;
        m_dbgTargetCoordSpaceId = dbgTargetCoordSpaceId;
    }
#endif

    // Set the render target type
    MIL_THR(m_pIRenderTarget->GetType(&m_dwInternalRenderTargetType));

    RRETURN(hr);
}

VOID CDrawingContext::GetWorldTransform(__out_ecount(1) CMILMatrix *pMatrix)
{
    m_transformStack.Top(
        CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels>::ReinterpretBaseForModification
        (pMatrix));
}

HRESULT CDrawingContext::GetState(__out_ecount(1) CRenderState **ppRenderState)
{
    *ppRenderState = &m_renderState;
    return S_OK;
}

VOID CDrawingContext::Get3DBrushContext(
    __in_ecount(1) const CRectF<CoordinateSpace::BaseSampling> &rcBrushSizingBounds,
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::IdealSampling> *pmatWorldToIdealSampleSpace,
    __out_ecount(1) BrushContext **ppBrushContext
    )
{
    // We have code that depends on this matrix being a scale matrix for 3D
    Assert(pmatWorldToIdealSampleSpace->IsPure2DScale());

    MilPointAndSizeDFromMilRectF(OUT m_3DBrushContext.rcWorldBrushSizingBounds, rcBrushSizingBounds);
    m_3DBrushContext.matWorldToSampleSpace = *pmatWorldToIdealSampleSpace;

    // The world clip & brush sizing bounds are the same in 3D.
    m_3DBrushContext.rcWorldSpaceBounds = rcBrushSizingBounds;

    *ppBrushContext = &m_3DBrushContext;
}


//---------------------------------------------------------------------------------
// CDrawingContext::PreCompute
//
// This function updates the dirty regions of the attached visual tree.
//---------------------------------------------------------------------------------

HRESULT
CDrawingContext::PreCompute(
    __in_ecount(1) CMilVisual *pRoot,
    __in_ecount(1) const CMilRectF *prcSurfaceBounds,
    UINT uNumInvalidTargetRegions,
    __in_ecount_opt(uNumInvalidTargetRegions) MilRectF const *rgInvalidTargetRegions,
    float allowedDirtyRegionOverhead,
    BOOL fFullRender,
    __in_opt ScrollArea *pScrollArea    
    )
{
    HRESULT hr = S_OK;

    // Lazy create the precompute context which from now on will be cached.
    if (m_pPreComputeContext == NULL && m_pComposition)
    {
        IFC(CPreComputeContext::Create(m_pComposition, &m_pPreComputeContext));
    }

    if (m_pPreComputeContext)
    {
        IFC(m_pPreComputeContext->PreCompute(
            pRoot, 
            prcSurfaceBounds, 
            uNumInvalidTargetRegions, 
            rgInvalidTargetRegions, 
            allowedDirtyRegionOverhead, 
            DefaultInterpolationMode,
            pScrollArea,
            fFullRender         // No dirty region collection if it's a full render
            ));
    }

Cleanup:
    // In failure cases we release the PreComputeContext because it could be left
    // in an inconsistent state.

    if (FAILED(hr))
    {
        delete m_pPreComputeContext;
        m_pPreComputeContext = NULL;
    }

    RRETURN(hr);
}

//+---------------------------------------------------------------------------
//
//  Member:
//      CDrawingContext::Render3D
//
//  Synopsis:
//      Renders the given Visual3D tree with the given camera and viewport
//      rectangles into this CDrawingContext.
//
//  Returns:
//      S_OK on success, else failed HR.
//
//----------------------------------------------------------------------------

HRESULT
CDrawingContext::Render3D(
    __in_ecount(1) CMilVisual3D *pRootVisual3D,
    __in_ecount(1) CMilCameraDuce *pCamera,
    __in_ecount(1) const MilPointAndSizeD *pViewport,
    __in_ecount(1) const CRectF<CoordinateSpace::LocalRendering> &rcBounds
    )
{
    HRESULT hr = S_OK;

    // Start with an empty interval
    float computedNearPlane = +FLT_MAX;
    float computedFarPlane = -FLT_MAX;
    bool fRenderRequired = false;
    bool fUseComputedPlanes = false;
    CRectF<CoordinateSpace::LocalRendering> viewportRect;

    if (pCamera == NULL)
    {
        // Early exit if the CMilViewport3DVisual has no camera.
        goto Cleanup;
    }

    if (pRootVisual3D == NULL)
    {
        // Early exit if the CMilViewport3DVisual has no 3D children.
        goto Cleanup;
    }

    // Cast our viewport rect from double -> float.
    MilRectFFromMilPointAndSizeD(OUT viewportRect, *reinterpret_cast<const MilPointAndSizeD*>(pViewport));

    if (!viewportRect.IsWellOrdered())
    {
        // Early exit if the viewportRect is not well ordered.  "Not
        // well ordered" includes rectangles which contain NaNs as
        // well as the managed Rect.Empty.
        goto Cleanup;
    }

    IFC(Begin3D(viewportRect, rcBounds));

    //
    //  Prerender - Walk the 3D subtree collecting light and camera info
    //

    if (!m_pPrerender3DContext)
    {
        IFC(CPrerender3DContext::Create(&m_pPrerender3DContext));
    }

    {
        CMILMatrix viewTransform;

        IFC(pCamera->SynchronizeAnimations());
        IFC(pCamera->GetViewTransform(&viewTransform));

        fUseComputedPlanes = pCamera->ShouldComputeClipPlanes();

        if (fUseComputedPlanes)
        {
            IFC(m_pPrerender3DContext->Compute(
                pRootVisual3D,
                &viewTransform,
                &m_contextState.LightData,
                /* out */ computedNearPlane,
                /* out */ computedFarPlane,
                /* inout */ fRenderRequired
                ));
        }
        else
        {
            IFC(m_pPrerender3DContext->Compute(
                pRootVisual3D,
                &viewTransform,
                &m_contextState.LightData,
                /* inout */ fRenderRequired
                ));
        }
    }

    if (!fRenderRequired)
    {
        // Early exit if the there is nothing which requires rendering.
        goto Cleanup;
    }

    //
    //  Set up render state
    //

    IFC(pCamera->ApplyToContextState(
        &m_contextState,
        viewportRect.Width(),
        viewportRect.Height(),
        fUseComputedPlanes,
        computedNearPlane,
        computedFarPlane,
        fRenderRequired
        ));

    if (!fRenderRequired)
    {
        // Early exit if the camera is configured to clip the entire scene.
        goto Cleanup;
    }

    //
    //  Render - Walk the 3D subtree rendering content
    //

    if (!m_pRender3DContext)
    {
        IFC(CRender3DContext::Create(&m_pRender3DContext));
    }

    IFC(m_pRender3DContext->Render(
        pRootVisual3D,
        this,
        &m_contextState,
        m_pIRenderTarget,
        viewportRect.Width(),
        viewportRect.Height()
        ));

Cleanup:
    MIL_THR_SECONDARY(End3D());

    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// Reads the control flags from the control center if one exists.
//---------------------------------------------------------------------------------

VOID
CDrawingContext::DbgReadControlFlags()
{
    if (g_pMediaControl != NULL)
    {
        const CMediaControlFile* pFile = g_pMediaControl->GetDataPtr();
        g_fDirtyRegion_ShowDirtyRegions = pFile->ShowDirtyRegionOverlay;
        g_fDirtyRegion_ClearBackBuffer = pFile->ClearBackBufferBeforeRendering;
        g_fDirtyRegion_Enabled = !(pFile->DisableDirtyRegionSupport);
        g_fTranslucent_DrawBitmap = pFile->EnableTranslucentRendering;
    }
}

//---------------------------------------------------------------------------------
// CDrawingContext::Render
//
// Arguments:
//    pIRenderTarget - Ptr to the render target in which we should render.
//    pSurfaceBoudns - Surface bounds of the render target.
//    pClearColor    - Color that should be used to clear the render target.
//    fFullRender    - Indicates if we should use the dirty region optimization or
//                     not. Certain render target might not be able to support
//                     incremental updates.
//    pfNeedsFullPresent
//                   - Also the composition context might
//                     decide to re-render the surface completely and hence set
//                     fNeedsFullPresent to true. In that case the caller must ensure
//                     that the correct area, i.e. the whole surface is presented.
//                     Currently that is needed for the g_fDirtyRegion_ClearBackBuffer
//                     flag.
//---------------------------------------------------------------------------------

HRESULT
CDrawingContext::Render(
    __in_ecount(1) CMilVisual *pRoot,
    __in_ecount(1) IMILRenderTarget *pIRenderTarget,
    __in_ecount(1) MilColorF const *pClearColor,
    __in_ecount(1) CMilRectF const &rcSurfaceBounds,
    BOOL fFullRender,
    UINT uNumInvalidTargetRegions,
    __in_ecount_opt(uNumInvalidTargetRegions) MilRectF const *rgInvalidTargetRegions,
    bool fCanAccelerateScroll,  
    __out_ecount(1) BOOL *pfNeedsFullPresent
    )
{
    HRESULT hr = S_OK;

    DbgReadControlFlags();

    Assert(pIRenderTarget);

    m_renderedRegionCount = 0;

    if (pRoot != NULL)
    {
        // This is set-up in Initialize which is called when we Create the DC.
        Assert(m_pCachedNullBrushRealizer);

        if (g_fDirtyRegion_ClearBackBuffer)
        {
            IFC(pIRenderTarget->Clear(pClearColor));
        }

        // For now we PreCompute just before we render. This will not work with multiple
        // targets.

        UINT64 data = (UINT64)this;
        EventWriteWClientUcePrecomputeBegin(data);

        IMILRenderTargetHWND *pIMILRenderTargetHWND = NULL; 

        if (fCanAccelerateScroll)
        {
            // Change this to QI if static_cast doesn't work.
            pIMILRenderTargetHWND = static_cast<IMILRenderTargetHWND *>(pIRenderTarget);

            if (   (pIMILRenderTargetHWND == NULL)
                || (m_dwInternalRenderTargetType != SWRasterRenderTarget))
            {
                fCanAccelerateScroll = false;
            }
            else
            {
                IFC(pIMILRenderTargetHWND->CanAccelerateScroll(&fCanAccelerateScroll));
            }            
        }    
        
        ScrollArea scrollArea = {0};

        IFC(PreCompute(
                pRoot,
                &rcSurfaceBounds,
                uNumInvalidTargetRegions,
                rgInvalidTargetRegions,
                50000.0f,
                fFullRender,
                (fCanAccelerateScroll && !fFullRender) ? &scrollArea : NULL
                ));        

        // ETW end trace event
        EventWriteWClientUcePrecomputeEnd(data);

        // ETW start trace event
        EventWriteWClientUceRenderBegin(data);

        if (   fCanAccelerateScroll
            && scrollArea.fDoScroll)
        {
            Assert(pIMILRenderTargetHWND != NULL);
            
            // We have a scroll change, and we have only software render targets. This means we can accelerate scrolls            
            // Scroll the backbuffer only, for now
            // We scroll the front buffer only when we're about to present the other dirty regions. This
            // hopefully helps GDI batch the changes so we don't have tearing.
            IFC(pIMILRenderTargetHWND->ScrollBlt(&(scrollArea.source), &(scrollArea.destination)));
        }

        // Update any caches marked dirty in the PreCompute walk.
        IFC(m_pComposition->GetVisualCacheManagerNoRef()->UpdateCaches());
        
        if ((!fFullRender) &&
            (g_fDirtyRegion_Enabled)) // If dirty regions are disabled we render everything.
        {
            const MilRectF *prgrcDirtyRegions = GetUninflatedDirtyRegions();

            if (prgrcDirtyRegions)
            {
                const UINT dirtyRegionCount = GetDirtyRegionCount();

                for (UINT i = 0; i < dirtyRegionCount; i++)
                {
                    CMilRectF renderBounds(prgrcDirtyRegions[i]);

                    // Inflate the dirty rect for anti-aliasing.
                    InflateRectF_InPlace(&renderBounds);
                        
                    // Intersect the dirty region with the surface bounds.
                    if (renderBounds.Intersect(rcSurfaceBounds))
                    {
                        IFC(DrawVisualTree(pRoot, pClearColor, renderBounds));

                        if (g_fDirtyRegion_ShowDirtyRegions)
                        {
                            IFC(DrawRectangleOverlay(&renderBounds));
                        }
                        m_renderedRegions[m_renderedRegionCount++] = renderBounds;
                    }
                }
            }
        }
        else
        {
            IFC(DrawVisualTree(pRoot, pClearColor, rcSurfaceBounds));

            m_renderedRegions[0] = rcSurfaceBounds;
            m_renderedRegionCount = 1;
        }

        EventWriteWClientUceRenderEnd(data);

        // If the dirty region analysis is disabled or if the user wants to clear before every render, then
        // we need to indicate to the calling code that we rendered everything.
        // The caller of this function (currently only the render target), then must present the whole surface.
        (*pfNeedsFullPresent) = (!g_fDirtyRegion_Enabled || g_fDirtyRegion_ClearBackBuffer);


    }

Cleanup:

    if (FAILED(hr))
    {
        // Release render targets and destroy dependent members
        Uninitialize();
    }

    RRETURN(hr);
}

//---------------------------------------------------------------------------------
// CDrawingContext::GetUninflatedDirtyRegions
//
//    Returns a ptr to an array of rectangles that need to be re-renedered. The
//    rectangle array shall not be freed.
//
//    Note that the rectangles are NOT inflated for anti-aliasing.
//
//    The number of rectangles returned is variable and equal to
//    GetDirtyRegionCount.
//---------------------------------------------------------------------------------

__outro_xcount_opt(GetDirtyRegionCount()) const MilRectF*
CDrawingContext::GetUninflatedDirtyRegions()
{
    if (m_pPreComputeContext)
    {
        return m_pPreComputeContext->GetUninflatedDirtyRegions();
    }
    else
    {
        // It is possible that there is no precompute context. This happens if we
        // haven't had a root node to precompute.
        return NULL;
    }
}

//---------------------------------------------------------------------------------
// CDrawingContext::GetDirtyRegionCount
//
//    Returns the number of rectangles that need to be re-rendered.
//---------------------------------------------------------------------------------

UINT
CDrawingContext::GetDirtyRegionCount() const
{
    if (m_pPreComputeContext)
    {
        return m_pPreComputeContext->GetDirtyRegionCount();
    }
    else
    {
        return 0;
    }
}

//---------------------------------------------------------------------------------
// CDrawingContext::DrawRectangleOverlay
//
// Description:
//      This function overlays alternating transparent colored windows on the
//      parameter rectangle. Designed to be used with the debug tools allowing
//      display of the dirty regions being re-rendered
//---------------------------------------------------------------------------------

HRESULT CDrawingContext::DrawRectangleOverlay(
    __in_ecount(1) CMilRectF const *renderBounds
    )
{
    HRESULT hr = S_OK;

    CMilPointAndSizeF renderBoundsXYWH(
        renderBounds->left,
        renderBounds->top,
        renderBounds->right - renderBounds->left,
        renderBounds->bottom - renderBounds->top
    );

    g_DirtyRegionColor = g_DirtyRegionColor % g_DirtyRegionColorCount;
    
    IFC(DrawRectangle(
        &(g_DirtyRegionColors[g_DirtyRegionColor]),
        &renderBoundsXYWH
        ));

    g_DirtyRegionColor++;
Cleanup:
    RRETURN(hr);
}





