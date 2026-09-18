// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_brush
//      $Keywords:
//
//  $Description:
//      Contains the implementation of the radial gradient UCE resource.
//
//      This resource references the constant & animate properties of a radial
//      gradient brush defined at our API, and is able to resolve those
//      properties into a procedural or texture color source.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(RadialGradientBrushResource, MILRender, "RadialGradientBrush Resource");

MtDefine(CMilRadialGradientBrushDuce, RadialGradientBrushResource, "CMilRadialGradientBrushDuce");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilRadialGradientBrushDuce::~CMilRadialGradientBrushDuce
//
//  Synopsis:
//      Class d'tor.
//
//------------------------------------------------------------------------------
CMilRadialGradientBrushDuce::~CMilRadialGradientBrushDuce()
{
    UnRegisterNotifiers();

    ReleaseInterfaceNoNULL(m_pRealizedBitmapBrush);
    ReleaseInterfaceNoNULL(m_pIntermediateBrushRealizer);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilRadialGradientBrushDuce::GetRealizer
//
//  Synopsis:
//      Gets a reference to an object which can be used to obtain a realization
//      of this brush
//

HRESULT
CMilRadialGradientBrushDuce::GetRealizer(
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_out_ecount(1) CBrushRealizer** ppBrushRealizer
    )
{
    HRESULT hr = S_OK;

    //
    // We must use a different brush realizers when realizing a radial gradient
    // to an intermediate. The reason is, if we used the same realizer for both
    // the intermediate CMILBrushBitmap and the immediate
    // CMILBrushRadialGradient, the two objects would be realized at the same
    // time with the same realizer. This is a very nasty recursion problem.
    //
    if (pBrushContext->fRealizeProceduralBrushesAsIntermediates)
    {
        if (!m_pIntermediateBrushRealizer)
        {
            IFC(CBrushRealizer::CreateResourceRealizer(
                this,
                &m_pIntermediateBrushRealizer
                ));
        }
    
        *ppBrushRealizer = m_pIntermediateBrushRealizer;
        m_pIntermediateBrushRealizer->AddRef();
    }
    else
    {
        IFC(CMilGradientBrushDuce::GetRealizer(
            pBrushContext,
            ppBrushRealizer
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilRadialGradientBrushDuce::GetBrushRealizationInternal
//
//  Synopsis:
//      After obtaining the immediate value of the RadialGradientBrush
//      properties, this method updates the cached realization with them.
//
//------------------------------------------------------------------------------
HRESULT
CMilRadialGradientBrushDuce::GetBrushRealizationInternal(
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
    )
{
    HRESULT hr = S_OK;

    const CMILBrush *pOldRealizationNoRef = *ppBrushRealizationNoRef;

    CGradientColorData realizedGradientStops;

    IFC(GetGradientColorData(this, &realizedGradientStops));

    // Update gradient realization if there are 2 or more gradient stops
    if (realizedGradientStops.GetCount() >= 2)
    {
        if (pBrushContext->fRealizeProceduralBrushesAsIntermediates)
        {
            //
            // Realize the brush as an intermediate. Note that when this
            // realization is rendered we will recursively call into this
            // function and this same brush will be realized as a gradient.
            // 
            IFC(GetIntermediateSurfaceRealization(
                pBrushContext,
                ppBrushRealizationNoRef
                ));
    
            //
            // RadialGradient stress crash
            //
            // If the realized brush area is empty, we won't create a brush.
            // FreeRealizationResources() was crashing because m_pRealizedBitmapBrush
            // was NULL
            //
            if (*ppBrushRealizationNoRef)
            {
                m_fProceduralBrushRealizedAsIntermediate = TRUE;
            }
        }
        else
        {
            //
            // Realize the brush as a gradient brush.
            // 
            IFC(UpdateGradientRealization(
                &(pBrushContext->rcWorldBrushSizingBounds),
                &realizedGradientStops,
                &m_realizedGradientBrush
                ));
    
            m_fProceduralBrushRealizedAsIntermediate = FALSE;

            //
            // Save bounding box of drawing instruction used during
            // realization. We only cache the bounds during the gradient
            // realization since this is the only type of realization that
            // depends on the bounds. See HasRealizationContextChanged().
            //
            m_cachedBrushSizingBounds = pBrushContext->rcWorldBrushSizingBounds;

            *ppBrushRealizationNoRef = &m_realizedGradientBrush;
        }
    }
    else if (realizedGradientStops.GetCount() == 1)
    {
        //
        // Realize the brush as a solid color brush
        //
        IFC(GetSolidColorRealization(
            &realizedGradientStops,
            &m_realizedSolidBrush
            ));

        *ppBrushRealizationNoRef = &m_realizedSolidBrush;
    }
    else
    {
        // Set brush to empty for 0 gradient stops
        *ppBrushRealizationNoRef = NULL;
    }

    //
    // Release resources appropriately when switching brush types
    // 
    if (   pOldRealizationNoRef == &m_realizedGradientBrush
        && (   *ppBrushRealizationNoRef == NULL
            || *ppBrushRealizationNoRef == &m_realizedSolidBrush
           )
       )
    {
        //
        // The old realization was a gradient brush and now we are in a
        // degenerate solid color brush or empty brush. Release any cached
        // gradient colorsources on the brush since we are no longer using
        // them.
        //
        // Note that this is the only case where we should release resources.
        // If we are switching from a gradient brush to a bitmap brush or vice
        // versa, we should not release resources since these two brushes are
        // used together for 3d. Furthermore, we don't need to worry about
        // bitmap brushes keeping extra resources since they are released after
        // the draw call anyway. Solid color brushes don't have any resources,
        // so we don't need to worry about them.
        //
        IFC(m_realizedGradientBrush.ReleaseResources());
    }

Cleanup:
    if (FAILED(hr))
    {
        // Set to empty so we don't check against an old bounding box
        // in a future call.
        m_cachedBrushSizingBounds = MilEmptyPointAndSizeD;
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilRadialGradientBrushDuce::HasRealizationContextChanged, CMilBrushDuce
//
//  Synopsis:
//      If this brush can determine whether or not the brush context has changed
//      since the last call to GetBrushRealizationInternal it returns this determination
//      (either TRUE that it has changed, or FALSE that it hasn't).  If it can't
//      make this determination it always assumes the context has changed and
//      returns TRUE.
//
//------------------------------------------------------------------------------
BOOL 
CMilRadialGradientBrushDuce::HasRealizationContextChanged(
    __in_ecount(1) const BrushContext *pBrushContext
        // Context state to realize brush for    
    ) const
{
    BOOL fChanged = TRUE;

    //
    // Intermediate surfaces are dependent on the brush context clip & world
    // transform, which we don't cache.  Thus, we can only return false for
    // radial gradient brushes that aren't represented using intermediates.
    //
    if (!pBrushContext->fRealizeProceduralBrushesAsIntermediates)
    {
        //
        // If the currently cached brush is not a procedural brush
        //
        if (!m_fProceduralBrushRealizedAsIntermediate)
        {
            //
            // If the mapping mode is absolute, or if it's relative to the 
            // brush sizing box and the brush sizing box hasn't changed, then we 
            // don't need to update our realization.
            //
            if (m_data.m_MappingMode == MilBrushMappingMode::Absolute ||
                
                // We use exact equality here because fuzzy checks are expensive, coming up 
                // with a fuzzy threshold that defines the point at which visible changes
                // occur isn't straightforward (i.e., the brush sizing bounds aren't
                // in device space), and exact equality handles the case we need to optimize
                // for where a brush fills the exact same geometry more than once.
                IsExactlyEqualRectD(pBrushContext->rcWorldBrushSizingBounds, m_cachedBrushSizingBounds))
            {
                fChanged = FALSE;
            }
        }
    }

    return fChanged;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilRadialGradientBrushDuce::UpdateGradientRealization, CMilBrushDuce
//
//  Synopsis:
//      Creates a procedural realization of the immediate values of this
//      RadialGradientBrush.
//
//------------------------------------------------------------------------------
HRESULT
CMilRadialGradientBrushDuce::UpdateGradientRealization(
    __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,                 
        // Bounds to size relative brush properties to
    __in_ecount(1) CGradientColorData *pColorData,
        // Realized gradient stops
    __inout_ecount(1) CMILBrushRadialGradient *pGradientBrush
    )
{
    HRESULT hr = S_OK;

    // Gradient points
    MilPoint2F ptCenterF;
    MilPoint2F ptRightExtentF; // ptCenter + RadiusX
    MilPoint2F ptTopExtentF;   // ptCenter - RadiusY
    MilPoint2F ptGradientOriginF;
    BOOL fHasSeparateOriginFromCenter;

    // Get gradient points
    IFC(RealizeGradientPoints(
        pBrushSizingBounds,
        &ptCenterF,
        &ptRightExtentF,
        &ptTopExtentF,
        &ptGradientOriginF,
        &fHasSeparateOriginFromCenter
        ));

    //
    // Set properties on realized brush
    //

    // Set gradient stops
    IFC(pGradientBrush->GetColorData()->CopyFrom(pColorData));

    // Set gradient points
    pGradientBrush->SetEndPoints(&ptCenterF, &ptRightExtentF, &ptTopExtentF);
    
    pGradientBrush->SetGradientOrigin(
        fHasSeparateOriginFromCenter, 
        &ptGradientOriginF
        );

    // Set wrap mode
    IFC(pGradientBrush->SetWrapMode(
        MILGradientWrapModeFromMIL_GRADIENT_SPREAD_METHOD(m_data.m_SpreadMethod)
        ));

    // Set color interpolation mode
    IFC(pGradientBrush->SetColorInterpolationMode(m_data.m_ColorInterpolationMode));

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilRadialGradientBrushDuce::RealizeGradientPoints, CMilBrushDuce
//
//  Synopsis:
//      Obtains the absolute position of the points which define this gradient. 
//      It does this by obtaining the current value of the gradient center,
//      right extent, top extent, and gradient origin, and then transformes them
//      by the current user-specified brush transform.
//
//------------------------------------------------------------------------------
HRESULT
CMilRadialGradientBrushDuce::RealizeGradientPoints(
    __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,        
        // Bounds to size relative brush properties to
    __out_ecount(1) MilPoint2F *pCenter,          
        // Center point
    __out_ecount(1) MilPoint2F *pRightExtent,     
        // Right extent of ellipse
    __out_ecount(1) MilPoint2F *pTopExtent,       
        // Top extent of ellipse
    __out_ecount(1) MilPoint2F *pGradientOrigin,  
        // Gradient origin point
    __out_ecount(1) BOOL *pfHasSeparateOriginFromCenter 
        // Are the user-specifed center & gradient origin the same?
    )
{
    HRESULT hr = S_OK;

    Assert(pBrushSizingBounds);
    Assert(pCenter);
    Assert(pRightExtent);
    Assert(pTopExtent);
    Assert(pGradientOrigin);

    CMILMatrix matBrushTransform;

    //
    // Get realized values
    //

    // Get center point
    MilPoint2D ptCenterD = *GetPoint(&m_data.m_Center, m_data.m_pCenterAnimation);

    // Get RadiusX
    DOUBLE rRadiusXD = GetDouble(m_data.m_RadiusX, m_data.m_pRadiusXAnimation);

    // Get RadiusY
    DOUBLE rRadiusYD = GetDouble(m_data.m_RadiusY, m_data.m_pRadiusYAnimation);

    // Get focal point
    MilPoint2D ptGradientOriginD = *GetPoint(&m_data.m_GradientOrigin, m_data.m_pGradientOriginAnimation);

    //
    // Do center vs. gradient origin comparison 
    //
    // Using exact equality (instead of a fuzzy comparison) allows us to more easily communicate when
    // the less-performant gradient-origin-differs (i.e., focal) code-path is taken.
    // This is done before any calculations occur that could introduce rounding error.
    //
    if ( (ptCenterD.X == ptGradientOriginD.X) &&
         (ptCenterD.Y == ptGradientOriginD.Y))
    {
        *pfHasSeparateOriginFromCenter = FALSE;
    }
    else
    {
        *pfHasSeparateOriginFromCenter = TRUE;
    }

    // Calculate right and top extents from center & radii

    MilPoint2D ptRightExtentD;
    MilPoint2D ptTopExtentD;
    ptRightExtentD.X = ptCenterD.X + rRadiusXD;
    ptRightExtentD.Y = ptCenterD.Y;
    ptTopExtentD.X = ptCenterD.X;
    ptTopExtentD.Y = ptCenterD.Y - rRadiusYD;

    *pCenter = MilPoint2FFromMilPoint2D(ptCenterD);
    *pRightExtent = MilPoint2FFromMilPoint2D(ptRightExtentD);
    *pTopExtent = MilPoint2FFromMilPoint2D(ptTopExtentD);
    *pGradientOrigin = MilPoint2FFromMilPoint2D(ptGradientOriginD);

    // If values are relative, calculate absolute values
    if (MilBrushMappingMode::RelativeToBoundingBox == m_data.m_MappingMode)
    {
        Assert(pBrushSizingBounds);

        // Convert points from relative brush space to absolute brush space
        AdjustRelativePoint(pBrushSizingBounds, pCenter);
        AdjustRelativePoint(pBrushSizingBounds, pRightExtent);
        AdjustRelativePoint(pBrushSizingBounds, pTopExtent);
        AdjustRelativePoint(pBrushSizingBounds, pGradientOrigin);
    }

    // Apply transform to gradient points if one exists
    // Must apply transform after converting points from relative brush
    // space to absolute brush space because the transform translation
    // is in absolute units

    const CMILMatrix *pmatRelative;
    const CMILMatrix *pmatTransform;

    IFC(GetMatrixCurrentValue(m_data.m_pRelativeTransform, &pmatRelative));
    IFC(GetMatrixCurrentValue(m_data.m_pTransform, &pmatTransform));         

    CBrushTypeUtils::GetBrushTransform(
        pmatRelative,
        pmatTransform,
        pBrushSizingBounds,
        &matBrushTransform
        );
    
    matBrushTransform.Transform(pCenter, pCenter, 1);
    matBrushTransform.Transform(pRightExtent, pRightExtent, 1);
    matBrushTransform.Transform(pTopExtent, pTopExtent, 1);
    matBrushTransform.Transform(pGradientOrigin, pGradientOrigin, 1);

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilRadialGradientBrushDuce::UpdateIntermediateSurfaceRealization,
//      CMilBrushDuce
//
//  Synopsis:
//      Creates an intermediate surface realization of the immediate values of
//      this RadialGradientBrush.
//
//------------------------------------------------------------------------------
HRESULT 
CMilRadialGradientBrushDuce::GetIntermediateSurfaceRealization(
    __in_ecount(1) const BrushContext *pBrushContext,
        // Context state to realize brush for    
    __deref_out_ecount(1) CMILBrush **ppBrushRealizationNoRef
    )
{
    HRESULT hr = S_OK;

    IMILRenderTargetBitmap *pRenderTarget = NULL;
    CDrawingContext *pDrawingContext = NULL;
    CMILMatrix matSurfaceToXSpace;
    XSpaceDefinition xSpaceDefinition;
    BOOL fBrushEmpty;
    CParallelogram rectShape;

    IWGXBitmapSource *pBitmapSource = NULL;

    AssertMsgW(pBrushContext->fBrushIsUsedFor3D, L"We shouldn't create intermediate render targets for radial gradients in 2D");

    //
    // The Viewbox to Viewport transform is identity because the coordinate
    // space of the drawing operations is the same as the viewport.
    //
    IFC(CTileBrushUtils::CreateTileBrushIntermediate(
        pBrushContext, 
        &IdentityMatrix,            // pmatContentToViewport
        &IdentityMatrix,            // pmatViewportToWorld
        &(pBrushContext->rcWorldBrushSizingBounds), // prcdViewport
        NULL,                       // pCachingParams
        MilTileMode::None,               // tileMode (Using none allows hw)
        NULL,                       // ppCachedSurface
        &pRenderTarget,
        &pDrawingContext,
        &matSurfaceToXSpace,        // pmatSurfaceToXSpace
        &fBrushEmpty,
        NULL,                       // pfUseSourceClip (don't care about source clipping)
        NULL,                       // pfSourceClipIsEntireSource (don't care about source clipping)
        NULL,                       // pSourceClipSampleSpace (don't care about source clipping)
        &xSpaceDefinition
        ));

    //
    // Early out if the brush is empty
    //
    if (fBrushEmpty)
    {
        *ppBrushRealizationNoRef = NULL;
        goto Cleanup;
    }

    //
    // Start rendering in the rendercontext
    //
    IFC(pDrawingContext->BeginFrame(
        pRenderTarget
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Device)
        ));

    //
    // We need to render our radial gradient as a rectangle in the intermediate
    // surface.
    //
    {
        CRectF<CoordinateSpace::BaseSampling> rcWorldBrushSizingBoundsF;

        MilRectFFromMilPointAndSizeD(
            OUT rcWorldBrushSizingBoundsF,
            pBrushContext->rcWorldBrushSizingBounds
            );

        rectShape.Set(
            rcWorldBrushSizingBoundsF
            );
    }

    //
    // Used to keep the 2d filling of an intermediate with this radialgradient
    // brush aliased.  This helps ensure that we see no antialiasing falloff
    // around the edges of the intermediate surface.
    //
    {
        MilRenderOptions renderOptions = { 0 };
        renderOptions.Flags    = MilRenderOptionFlags::EdgeMode;
        renderOptions.EdgeMode = MilEdgeMode::Aliased;
        IFC(pDrawingContext->PushRenderOptions(&renderOptions));
    }

    // ApplyRenderState must be called before rendering
    pDrawingContext->ApplyRenderState();

    {
        // Render to the intermediate suface
        MIL_THR(pDrawingContext->DrawShape(
            &rectShape, 
            this,
            NULL 
            ));
    
        //
        // We don't really need to call PopRenderOptions because this drawing context is
        // not used to draw anything else. We call it here to make debugging the
        // state stack easier.
        //
        pDrawingContext->PopRenderOptions();
            
        //
        // End rendering on the context.
        //
        pDrawingContext->EndFrame();

        if (FAILED(hr))
        {
            goto Cleanup;
        }
    }

    IFC(pRenderTarget->GetBitmapSource(&pBitmapSource));

    //
    // Delay create the bitmap brush so that 2D radial gradients don't consume
    // this memory
    // 
    if (m_pRealizedBitmapBrush == NULL)
    {
        IFC(CMILBrushBitmap::Create(&m_pRealizedBitmapBrush));
    }

    //
    //  Set the intermediate surface to this bitmap.
    //
    IFC(m_pRealizedBitmapBrush->SetBitmap(pBitmapSource));

    //
    // Setting the wrap mode to extend to make sure we get HW support, but it
    // shouldn't matter.  We shouldn't render anything outside of the brushspace
    // anyway.
    //
    IFC(m_pRealizedBitmapBrush->SetWrapMode(
        MilBitmapWrapMode::Extend, 
        NULL
        ));

    //
    // Set the transform calculated by CreateTileBrushRenderTarget
    //
    m_pRealizedBitmapBrush->SetBitmapToXSpaceTransform(
        &matSurfaceToXSpace,
        xSpaceDefinition
        DBG_COMMA_PARAM(&pBrushContext->matWorldToSampleSpace)
        );

    *ppBrushRealizationNoRef = m_pRealizedBitmapBrush;

Cleanup:

    ReleaseInterfaceNoNULL(pRenderTarget);
    ReleaseInterfaceNoNULL(pDrawingContext);

    ReleaseInterfaceNoNULL(pBitmapSource);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilRadialGradientBrushDuce::FreeRealizationResources, CMilBrushDuce
//
//  Synopsis:
//      Frees realized resource that shouldn't last longer than a single
//      primitive.  That is currently true for intermediate RTs, which this
//      object may retain if it's being used in 3D.
//
//------------------------------------------------------------------------------
void
CMilRadialGradientBrushDuce::FreeRealizationResources()
{
    // Free intermediate if this brush is holding onto one.
    if (m_fProceduralBrushRealizedAsIntermediate)
    {
        m_pRealizedBitmapBrush->SetBitmap(NULL);
        // Be sure to mark as dirty since no proper realization exists now.
        SetDirty(TRUE);
    }
}





