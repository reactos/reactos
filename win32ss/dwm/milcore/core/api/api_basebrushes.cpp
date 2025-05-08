// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains definition of CMILBrushGradient class
//
//-----------------------------------------------------------------------------

#include "precomp.hpp"

/*=========================================================================*\
    CGradientColorData - Keeps the color information for the gradient brushes.
\*=========================================================================*/

//+------------------------------------------------------------------------
//
//  Member:     
//      CGradientColorData::SetColors
//
//  Synopsis:   
//      This method sets the colors at equally spaced positions on the gradient
//      line
//
//  Note:
//      This path is currently only used by (lots of) test code
//-------------------------------------------------------------------------
HRESULT CGradientColorData::SetColors(
    __in_ecount(nCount) MilColorF const *pColors,
        // Colors to set on equally spaced gradient stops
    UINT nCount
        // Number of colors
    )
{
    HRESULT hr = S_OK;

    Clear();

    if (nCount > 0)
    {
        UINT uStopCountMinusOne = nCount - 1;

        //
        // Add the colors to the color array
        // 
        
        if (nCount == 1)
        {
            // Produce a solid color by adding the same color at 0.0 and 1.0
            IFC(m_rgColors.Add(*pColors));
            IFC(m_rgColors.Add(*pColors));            
        }
        else
        {
            IFC(m_rgColors.AddMultipleAndSet(pColors, nCount));
        }

        //
        // Derive equal-spaced positions for stops.
        //
        
        // Set first position
        //
        // Set position to exactly 0.0 to avoid any potential for 
        // rounding error during divide.
        IFC(m_rgPositions.Add(0.0f));

        // Set middle stops, if any exist
        for (UINT i = 1; i < uStopCountMinusOne; i++)
        {
            IFC(m_rgPositions.Add(static_cast<FLOAT>(i) / static_cast<FLOAT>(uStopCountMinusOne)));
        }

        // Set last position
        //
        // Set position to exactly 1.0 to avoid any potential for 
        // rounding error during divide.        
        IFC(m_rgPositions.Add(1.0f));
    }
    
Cleanup:

    if (FAILED(hr))
    {
        // Reset the color/position arrays upon failure to ensure we remain in a consistent state.
        //
        // The count of the color & position arrays *must* be kept in-sync, because only
        // a single count is used by the consumers if this data.  Otherwise, those callers could
        // end up reading past the end of one of the buffers.
        Clear();
    }
    
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     
//      CGradientColorData::AddColorWithPosition
//
//  Synopsis:   
//      Adds a single gradient stop to this gradient stop array.
//
//-------------------------------------------------------------------------
HRESULT 
CGradientColorData::AddColorWithPosition(
    __in_ecount(1) MilColorF const *pColor,
        // Color of the gradient stop to add
    FLOAT rPosition
        // Position of the gradient stop to add
    )
{
    HRESULT hr = S_OK;

    IFC(m_rgColors.Add(*pColor));
    IFC(m_rgPositions.Add(rPosition));

Cleanup:

    // Reset the color/position arrays upon failure to ensure we remain in a consistent state.
    //
    // The count of the color & position arrays *must* be kept in-sync, because only
    // a single count is used by the consumers if this data.  Otherwise, those callers could
    // end up reading past the end of one of the buffers.
    //
    // Because RemoveAt can fail, we can't rollback a single operation without the possibility
    // of another failure, so resetting both arrays is the safest option.   
    if (FAILED(hr))
    {
        Clear();
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     
//      CGradientColorData::CopyFrom
//
//  Synopsis:   
//      Copies the gradient data from one CGradientColorData instance
//      to another
//
//-------------------------------------------------------------------------
HRESULT 
CGradientColorData::CopyFrom(
    __in_ecount(1) CGradientColorData const *pColorData
        // CGradientColorData instance whose gradient stops should be copied.
    )
{
    HRESULT hr = S_OK;

    Clear();
    IFC(m_rgColors.Copy(pColorData->m_rgColors));
    IFC(m_rgPositions.Copy(pColorData->m_rgPositions));

Cleanup:

    // Reset the color/position arrays upon failure to ensure we remain in a consistent state.
    //
    // The count of the color & position arrays *must* be kept in-sync, because only
    // a single count is used by the consumers if this data.  Otherwise, those callers could
    // end up reading past the end of one of the buffers.
    if (FAILED(hr))
    {
        Clear();
    }
    
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CGradientColorData::ApplyOpacity
//
//  Synopsis:   Applies the opacity to the color data by multiplying through
//
//  Note:       Multiple calls to ApplyOpacity will apply an opacity to the
//              gradient stops multiple times, not replace the current opacity.
//              E.g., ApplyOpacity(0.5) then ApplyOpacity(0.4) will result in
//              a combined opacity of 0.5 * 0.4 = 0.2, not 0.4 (the last value).
//
//-------------------------------------------------------------------------
HRESULT CGradientColorData::ApplyOpacity(
    FLOAT rOpacity
    )
{

    // Assert opacity is within change
    Assert ((rOpacity >= 0.0) &&
            (rOpacity <= 1.0));

    UINT uiNumColors = m_rgColors.GetCount();
    MilColorF *pColors = m_rgColors.GetDataBuffer();

    Assert(pColors ||           // There is a color array
           (0 == uiNumColors)); // or there are no colors

    // Apply a constant opacity to all gradient stops
    for (UINT i = 0; i < uiNumColors; i++)
    {
        pColors[i].a *= rOpacity;
    }

    RRETURN(S_OK);
}

/*=========================================================================*\
    CMILBrushGradient - MIL Linear Gradient Brush Object
\*=========================================================================*/

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushGradient::CMILBrushGradient
//
//  Synopsis:   ctor
//
//-------------------------------------------------------------------------
CMILBrushGradient::CMILBrushGradient(
    __in_ecount_opt(1) CMILFactory *pFactory
    )
    : CMILObject(pFactory)
{
    ZeroMemory(&m_ptStartPointOrCenter, sizeof(m_ptStartPointOrCenter));
    ZeroMemory(&m_ptEndPoint, sizeof(m_ptEndPoint));
    ZeroMemory(&m_ptDirPointOrEndPoint2, sizeof(m_ptDirPointOrEndPoint2));
    
    m_WrapMode = MilGradientWrapMode::Extend;
    m_ColorInterpolationMode = MilColorInterpolationMode::SRgbLinearInterpolation;
}

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushGradient::~CMILBrushGradient
//
//  Synopsis:   dctor
//
//-------------------------------------------------------------------------
CMILBrushGradient::~CMILBrushGradient()
{
}

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushGradient::SetColorInterpolationMode
//
//  Synopsis:   Sets the interpolation mode to the specified value
//
//-------------------------------------------------------------------------
HRESULT
CMILBrushGradient::SetColorInterpolationMode(
    MilColorInterpolationMode::Enum colorInterpolationMode    
    )
{
    HRESULT hr = S_OK;

    m_ColorInterpolationMode = colorInterpolationMode;

    UpdateUniqueCount();

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushGradient::SetWrapMode
//
//  Synopsis:   Sets the wrap mode to the specified value
//
//-------------------------------------------------------------------------
HRESULT
CMILBrushGradient::SetWrapMode(
    MilGradientWrapMode::Enum wrapMode
    )
{
    HRESULT hr = S_OK;

    switch (wrapMode)
    {
    case MilGradientWrapMode::Extend:
    case MilGradientWrapMode::Flip:
    case MilGradientWrapMode::Tile:
        m_WrapMode = wrapMode;
        break;
    default:
        hr = E_INVALIDARG;
        break;
    }

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushGradient::SetEndPoints
//
//  Synopsis:   This function sets the end points of the gradient. Depending
//              on whether this is a linear or radient gradient this can
//              mean slightly different things.
//
//              pptStartPointOrCenter-
//                  linear: the start point of the gradient (also the origin)
//                  radial: the center of the ellipse that defines the end
//              pptEndPoint
//                  linear: the end point of the gradient
//                  radial: one point of the ellipse that defines the end
//              pptDirPointOrEndPoint2
//                  linear: the point defining the angle of parallel gradient lines
//                  radial: another point on the ellipse that defines the end
//
//  Justification for dual meaning of this method:
//                  Whether the gradient is linear or radial, this method
//              determines three things:
//              1) where the end of the gradient is-- what positions
//                 in space the last gradient stop will occupy.
//              2) the orientation of the gradient
//                  For linear gradients, this requires the direction point
//                  For radial gradients, no extra information is needed since
//              the ellipse already has an orientation.
//              3) The default starting point/origin of the gradient
//                  For linear gradients this "default" start point cannot be changed
//                  For radial gradients, it can be changed-- see SetGradientOrigin().
//              We call the first parameter the "Center" because while we don't know
//              if it is going to be the origin, we know it is the center of the
//              ellipse that defines the end of the gradient.
//
//                  While the meanings of these parameters may be slightly
//              different depending on the type of gradient, the mathematics
//              to deal with them are almost the same. For this reason it
//              makes sense to collapse this similar information in the
//              base class here.
//
//-------------------------------------------------------------------------
void CMILBrushGradient::SetEndPoints(
    __in_ecount(1) MilPoint2F const *pptStartPointOrCenter,
    __in_ecount(1) MilPoint2F const *pptEndPoint,
    __in_ecount(1) MilPoint2F const *pptDirPointOrEndPoint2
    )
{
    m_ptStartPointOrCenter = *pptStartPointOrCenter;
    m_ptEndPoint = *pptEndPoint;
    m_ptDirPointOrEndPoint2 = *pptDirPointOrEndPoint2;
}

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushGradient::GetEndPoints
//
//  Synopsis:   This function gets the end points of the gradient- see
//              See SetEndPoints for a description of what these mean.
//
//-------------------------------------------------------------------------
void
CMILBrushGradient::GetEndPoints(
    __out_ecount(1) MilPoint2F *pptStartPointOrCenter,
    __out_ecount(1) MilPoint2F *pptEndPoint,
    __out_ecount(1) MilPoint2F *pptDirPointOrEndPoint2
    ) const
{
    *pptStartPointOrCenter = m_ptStartPointOrCenter;
    *pptEndPoint = m_ptEndPoint;
    *pptDirPointOrEndPoint2 = m_ptDirPointOrEndPoint2;
}

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushGradient::SetColors
//
//  Synopsis:   Sets the colors of the gradient
//
//-------------------------------------------------------------------------
HRESULT
CMILBrushGradient::SetColors(
    __in_ecount(nCount) MilColorF const *pColors,
    UINT nCount
    )
{
    HRESULT hr = S_OK;

    if (pColors == NULL || nCount < 2)
    {
        MIL_THR(E_INVALIDARG);
    }

    if (SUCCEEDED(hr))
    {
        MIL_THR(m_ColorData.SetColors(pColors, nCount));
        UpdateUniqueCount();        
    }

    RRETURN(hr);
}
//+------------------------------------------------------------------------
//
//  Member:     
//      CMILBrushGradient::AddColorWithPosition
//
//  Synopsis:   
//      Adds a color & position to the gradient brush
//
//  Notes:
//      This method is currently only used by test code.
//
//-------------------------------------------------------------------------
HRESULT CMILBrushGradient::AddColorWithPosition(
        __in_ecount(1) MilColorF const *pColor,
        FLOAT rPosition
    )
{
    HRESULT hr = S_OK;

    if (pColor == NULL)
    {
        IFC(E_INVALIDARG);
    }

    MIL_THR(m_ColorData.AddColorWithPosition(pColor, rPosition));

    UpdateUniqueCount();

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CMILBrushGradient::GetUniquenessToken
//
//  Synopsis:   Gets the uniqueness token of the gradient
//
//-------------------------------------------------------------------------
STDMETHODIMP_(void)
CMILBrushGradient::GetUniquenessToken(
    __out_ecount(1) UINT *puToken
    ) const
{
    *puToken = GetUniqueCount();
}


