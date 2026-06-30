// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_geometry
//      $Keywords:
//
//  $Description:
//      The implementation of CFillTesselator classes
//
//      CFillTesselator generates tessellations for Figures
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#pragma optimize("t", on)

MtDefine(CRectFillTessellator, MILRender, "CRectFillTessellator");
MtDefine(CRegionFillTessellator, MILRender, "CRegionFillTessellator");
MtDefine(CGeneralFillTessellator, MILRender, "CGeneralFillTessellator");

///////////////////////////////////////////////////////////////////////////////

// Implementation of CSpecialCaseFillTessellator

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSpecialCaseFillTessellator::CSpecialCaseFillTessellator
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------
MIL_FORCEINLINE
CSpecialCaseFillTessellator::CSpecialCaseFillTessellator(
    __in_ecount_opt(1) const CBaseMatrix *pMatrix)
        // Transformation matrix (NULL OK)

    : CFillTessellator(pMatrix)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSpecialCaseFillTessellator::TessellateFigure
//
//  Synopsis:
//      Tessellate a rectangle figure
//
//------------------------------------------------------------------------------
HRESULT
CSpecialCaseFillTessellator::TessellateFigure(
    __in_ecount(1) const IFigureData &figure,
        // Figure to tessellate
    __inout_ecount(1) IGeometrySink *pgs)
        // Geometry sink for the resulting tessellation
{
    HRESULT hr = S_OK;
    CMilPoint2F      rgPosition[4];             // Figure vertices
  
    Assert(figure.IsAParallelogram());
    Assert(figure.IsFillable());

    // Generate vertices
    figure.GetParallelogramVertices(rgPosition, m_pMatrix);
    IFC(pgs->AddParallelogram(rgPosition));

Cleanup:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////////////////////

// Implementation of CRectFillTessellator


//+-----------------------------------------------------------------------------
//
//  Member:
//      CRectFillTessellator::CRectFillTessellator
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------

CRectFillTessellator::CRectFillTessellator(
    __in_ecount(1) const IFigureData &figure,
        // The figure to tessellate
    __in_ecount_opt(1) const CBaseMatrix *pMatrix)
        // Transformation (NULL OK)

    : CSpecialCaseFillTessellator(pMatrix),
      m_figure(figure)
{
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CRectFillTessellator::SendGeometry
//
//  Synopsis:
//      Do the tessellation
//
//------------------------------------------------------------------------------
HRESULT
CRectFillTessellator::SendGeometry(
    __inout_ecount(1) IGeometrySink *pgs)
        // Geometry sink to tessellate into
{
    HRESULT hr = S_OK;

    Assert(pgs);

    IFC(TessellateFigure(m_figure, pgs));

Cleanup:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////////////////////

// Implementation of CRegionFillTessellator

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRegionFillTessellator::CRegionFillTessellator
//
//  Synopsis:
//      Constructor
//
//------------------------------------------------------------------------------

CRegionFillTessellator::CRegionFillTessellator(
    __in_ecount(1) const IShapeData  &shape,
        // The shape to tessellate
    __in_ecount_opt(1) const CBaseMatrix *pMatrix)
        // Transformation (NULL OK)

    : CSpecialCaseFillTessellator(pMatrix),
      m_shape(shape)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CRegionFillTessellator::SendGeometry
//
//  Synopsis:
//      Do the tessellation
//
//------------------------------------------------------------------------------
HRESULT
CRegionFillTessellator::SendGeometry(
    __inout_ecount(1) IGeometrySink *pgs)
        // Geometry sink to tessellate into
{
    HRESULT hr = S_OK;

    Assert(pgs);

    for (UINT i = 0;  i < m_shape.GetFigureCount();  i++)
    {
        const IFigureData &figure = m_shape.GetFigure(i);
        if (figure.IsFillable())
        {
            IFC(TessellateFigure(figure, pgs));
        }
    }

Cleanup:
    RRETURN(hr);
}

///////////////////////////////////////////////////////////////////////////////

// Implementation of CGeneralFillTessellator

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGeneralFillTessellator::SendGeometry
//
//  Synopsis:
//      Do the tessellation
//
//------------------------------------------------------------------------------
HRESULT
CGeneralFillTessellator::SendGeometry(
    __inout_ecount(1) IGeometrySink *pgs)
        // Geometry sink to tessellate into
{
    HRESULT hr = S_OK;
    CMilRectF rect;
    bool fDegenerate;
    CDoubleFPU fpu; // Setting floating point state to double precision

    Assert(pgs);
    CTessellator tessellator(*pgs, DEFAULT_FLATTENING_TOLERANCE);

    // Set scanner workspace
    IFC(m_shape.GetTightBounds(rect, NULL /*pen*/, m_pMatrix));
    IFC(tessellator.SetWorkspaceTransform(rect, fDegenerate));
    if (fDegenerate)
        goto Cleanup;

    // Organize the shape into chains
    IFC(m_shape.Populate(&tessellator, m_pMatrix));

    // Tessellate the raw chains
    IFC(tessellator.Scan());

Cleanup:
    RRETURN(hr);
}



