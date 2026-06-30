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
//      Contains the implementation of the linear gradient UCE resource.
//
//      This resource references the constant & animate properties of a linear
//      gradient brush defined at our API, and is able to resolve those
//      properties into a procedural color source.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(LinearGradientBrushResource, MILRender, "LinearGradientBrush Resource");

MtDefine(CMilLinearGradientBrushDuce, LinearGradientBrushResource, "CMilLinearGradientBrushDuce");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilLinearGradientBrushDuce::~CMilLinearGradientBrushDuce
//
//  Synopsis:
//      Class d'tor.
//
//------------------------------------------------------------------------------
CMilLinearGradientBrushDuce::~CMilLinearGradientBrushDuce()
{
    UnRegisterNotifiers();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilLinearGradientBrushDuce::GetBrushRealizationInternal
//
//  Synopsis:
//      After obtaining the immediate value of the LinearGradientBrush
//      properties, this method updates the cached realization with them.
//
//------------------------------------------------------------------------------
HRESULT
CMilLinearGradientBrushDuce::GetBrushRealizationInternal(
    __in_ecount(1) const BrushContext *pBrushContext,
    __deref_inout_ecount_opt(1) CMILBrush **ppBrushRealizationNoRef
    )
{
    HRESULT hr = S_OK;

    const CMILBrush *pOldRealizationNoRef = *ppBrushRealizationNoRef;

    // Gradient stops
    CGradientColorData realizedGradientStops;

     // Get realized gradient stops
    IFC(GetGradientColorData(this, &realizedGradientStops));

    // Update gradient realization if there are 2 or more gradient stops
    if (realizedGradientStops.GetCount() >= 2)
    {
        IFC(GetLinearGradientRealization(
            &(pBrushContext->rcWorldBrushSizingBounds),
            &realizedGradientStops,
            &m_realizedGradientBrush
            ));

        *ppBrushRealizationNoRef = &m_realizedGradientBrush;
    }
    // Update solid realization if there is only one gradient stop
    else if (realizedGradientStops.GetCount() == 1)
    {
        IFC(GetSolidColorRealization(
            &realizedGradientStops,
            &m_realizedSolidBrush
            ));

        *ppBrushRealizationNoRef = &m_realizedSolidBrush;
    }
    else
    {   // Set brush to empty for 0 gradient stops
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
        // Solid color brushes don't have any resources, so we don't need to
        // worry about them.
        //
        IFC(m_realizedGradientBrush.ReleaseResources());
    }
Cleanup:

    if (SUCCEEDED(hr))
    {
        // Save brush sizing bounds used during realization
        m_cachedBrushSizingBounds = pBrushContext->rcWorldBrushSizingBounds;
    }
    else
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
//      CMilLinearGradientBrushDuce::UpdateGradientRealization
//
//  Synopsis:
//      Realizes each property of the gradient brush and sets it on the cached
//      realization.
//
//------------------------------------------------------------------------------
HRESULT
CMilLinearGradientBrushDuce::GetLinearGradientRealization(
     __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,           
        // Bounds to size relative brush properties to
     __in_ecount(1) CGradientColorData *pColorData,
        // Realized gradient stops
     __inout_ecount(1) CMILBrushLinearGradient *pLinearGradientRealization
    )
{
    HRESULT hr = S_OK;

    // Realized gradient points
    MilPoint2F ptStartPointF;
    MilPoint2F ptEndPointF;
    MilPoint2F ptDirectionPointF;

    // Get realized gradient points
    IFC(RealizeGradientPoints(
        pBrushSizingBounds,
        &ptStartPointF,
        &ptEndPointF,
        &ptDirectionPointF
        ));

    //
    // Set realized values on brush realization
    //

    // Set gradient stops
    IFC(pLinearGradientRealization->GetColorData()->CopyFrom(pColorData));

    // Set gradient points
    pLinearGradientRealization->SetEndPoints(
        &ptStartPointF,
        &ptEndPointF,
        &ptDirectionPointF
        );

    // Set wrap mode
    IFC(pLinearGradientRealization->SetWrapMode(
        MILGradientWrapModeFromMIL_GRADIENT_SPREAD_METHOD(m_data.m_SpreadMethod)
        ));

    // Set color interpolation mode
    IFC(pLinearGradientRealization->SetColorInterpolationMode(m_data.m_ColorInterpolationMode));

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMilLinearGradientBrushDuce::RealizeGradientPoints, CMilBrushDuce
//
//  Synopsis:
//      Obtains the absolute position of the points which define this gradient. 
//      It does this by obtaining the current value of the gradient start, end,
//      & direction points, and then transformes them by the current
//      user-specified brush transform.
//
//------------------------------------------------------------------------------
HRESULT
CMilLinearGradientBrushDuce::RealizeGradientPoints(
    __in_ecount(1) const MilPointAndSizeD *pBrushSizingBounds,     
        // Bounds to size relative brush properties to
    __out_ecount(1) MilPoint2F *pStartPoint,   
        // Begin point of gradient vector
    __out_ecount(1) MilPoint2F *pEndPoint,     
        // End point of gradient vector
    __out_ecount(1) MilPoint2F *pDirectionPoint
        // Point on gradient line that intersects pStartPoint
    )
{
    HRESULT hr = S_OK;

    Assert(pBrushSizingBounds);
    Assert(pStartPoint);
    Assert(pEndPoint);
    Assert(pDirectionPoint);

    CMILMatrix matBrushTransform;

    // Get points
    MilPoint2D ptStartPointD = *GetPoint(&m_data.m_StartPoint, m_data.m_pStartPointAnimation);
    MilPoint2D ptEndPointD = *GetPoint(&m_data.m_EndPoint, m_data.m_pEndPointAnimation);

    *pStartPoint = MilPoint2FFromMilPoint2D(ptStartPointD);
    *pEndPoint = MilPoint2FFromMilPoint2D(ptEndPointD);

    // Calculate the direction point.  This point is needed to define
    // the direction of the gradient color bands w.r.t. the gradient vector
    // (the gradient vector is defined as the difference between the line
    // points: (pEndPoint - pStartPoint))
    // By default (i.e., if there is no transform) the bands of color in a
    // gradient are perpendicular  to the gradient vector.
    //
    // Consider the following illustration where the bands of color in the left
    // example are perpendicular to the gradient vector, and the right example
    // where a 45 degree shear transform has been applied.  The direction point
    // is required to define this shear.

    //+-------------------------------------------------------------------------
    //
    //  Key:
    //      Gradient Vector: --------------------
    //      Gradient color bands:  Pipe (|) and Slash (/)
    //
    //  Red     Purple      Blue            Red     Purple      Blue
    //  v         v          v              /         /          /
    //  ||||||||||||||||||||||||            ////////////////////////
    //  ------------------------            ------------------------
    //  ^                      ^            ^                      ^
    //  pStartPoint         pEndPoint     pStartPoint         pEndPoint
    //
    //--------------------------------------------------------------------------

    // If points are relative, calculate absolute points
    if (MilBrushMappingMode::RelativeToBoundingBox == m_data.m_MappingMode)
    {
        Assert(pBrushSizingBounds);
        // Convert points from relative brush space to absolute brush space
        AdjustRelativePoint(pBrushSizingBounds, pStartPoint);
        AdjustRelativePoint(pBrushSizingBounds, pEndPoint);
    }

    // Default direction point is on a vector based at pStartPoint and
    // perpendicular  to the gradient vector
    pDirectionPoint->X = -(pEndPoint->Y - pStartPoint->Y) + pStartPoint->X;
    pDirectionPoint->Y = (pEndPoint->X - pStartPoint->X) + pStartPoint->Y;

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
    
    matBrushTransform.Transform(pStartPoint, pStartPoint, 1);
    matBrushTransform.Transform(pEndPoint, pEndPoint, 1);
    matBrushTransform.Transform(pDirectionPoint, pDirectionPoint, 1);

Cleanup:

    RRETURN(hr);
}



