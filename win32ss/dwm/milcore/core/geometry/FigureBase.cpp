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
//      The implementation of CFigureBase.
//
//      CFigureBase captures the most general type of figure (sub-path)
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

#pragma optimize("t", on)

///////////////////////////////////////////////////////////////////////////////
// Implementation of CFigureBase

//+-----------------------------------------------------------------------------
//
//  Synopsis:
//      Update the bounds with this figure
//
//------------------------------------------------------------------------------
HRESULT
CFigureBase::UpdateBounds(
    __inout_ecount(1) CBounds &bounds,
        // In/out: Bounds, updated here
    __in_ecount_opt(1) const CMILMatrix *pMatrix
        // In: Transformation (NULL OK)
    ) const
{
    HRESULT hr = S_OK;

    if (!m_refData.IsEmpty())
    {
        if (pMatrix == NULL &&
            m_refData.IsAxisAlignedRectangle()
            )
        {
            MilPoint2F ptCorners[2];
            m_refData.GetRectangleCorners(ptCorners);

            bounds.UpdateWithPoint(ptCorners[0]);
            bounds.UpdateWithPoint(ptCorners[1]);
        }
        else
        {
            CBoundsTask task(bounds, m_refData.GetStartPoint(), pMatrix);
            IFC(task.TraverseForward(m_refData));
        }
    }

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureBase::Populate
//
//  Synopsis:
//      Populate a scanner
//
//  Notes:
//      The traversal here is very similar to CFigureTask::TraverseForward, so
//      why not derive CScanPopulator from CFigureTask and use TraverseForward? 
//      Because here we need to know which joins between segments are smooth. 
//      TraverseForward does not check smoothness because none of its users is
//      interested in smoothness.  This check is not free, so the generic
//      TravereForward should not be burdened with this extra cost.
//
//------------------------------------------------------------------------------
HRESULT
CFigureBase::Populate(
    __in_ecount(1) IPopulationSink *scanner,
        // The receiving scanner
    __in_ecount_opt(1) const CMILMatrix *pMatrix) const
        // Transformation matrix (NULL OK)
{
    HRESULT hr = S_OK;
    const MilPoint2F *pPt;
    BYTE bType;
    GpPointR ptCurrent;

    if (m_refData.HasNoSegments())
        goto Cleanup;
    
    // Starting point
    ptCurrent = GpPointR(m_refData.GetStartPoint(), pMatrix);
    IFC(scanner->StartFigure(ptCurrent));

    // Traverse the segments
    if (!m_refData.SetToFirstSegment())
        goto Cleanup;

    do 
    {
        m_refData.GetCurrentSegment(bType, pPt);
        bool fAtGap = m_refData.IsAtAGap();

        scanner->SetStrokeState(!fAtGap);

        if (MilCoreSeg::TypeLine == bType)
        {          
            ptCurrent = GpPointR(*pPt, pMatrix);
            IFC(scanner->AddLine(ptCurrent));
        }
        else
        {
            Assert(MilCoreSeg::TypeBezier == bType);

            GpPointR BezierPoints[3];
            
            if (pMatrix)
            {
                TransformPoints(*pMatrix, 3, pPt, BezierPoints);
            }
            else
            {
                for (int i = 0;  i < 3;  i++)
                {
                    BezierPoints[i] = pPt[i];
                }
            }

            IFC(scanner->AddCurve(BezierPoints));
        }
        scanner->SetCurrentVertexSmooth(m_refData.IsAtASmoothJoin());
    }
    while (m_refData.SetToNextSegment());
    
    IFC(scanner->EndFigure(m_refData.IsClosed()));

Cleanup:
    RRETURN(hr);
}
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureBase::AddToGpPath
//
//  Synopsis:
//      Add this figure to a leagacy GDI+ style path
//
//------------------------------------------------------------------------------
HRESULT 
CFigureBase::AddToGpPath(
    __inout_ecount(1) DynArray<MilPoint2F> &rgPoints,
        // Path points
    __inout_ecount(1) DynArray<BYTE> &rgTypes,
        // Path types
    bool fSkipGaps
        // Skip no-stroke segments if true
    ) const
{
    HRESULT hr = S_OK;
    bool fPreviousSegmentIsAGap = false;
    MilPoint2F ptGapEnd;
    BYTE bType;
    const MilPoint2F *pt;

//  This method is used for legacy code that does not support a figure with a 
//  single point, so we'll ignore such figures.

    if (m_refData.HasNoSegments())
    {
        goto Cleanup;
    }

    // Add the figure start
    IFC(rgPoints.Add(m_refData.GetStartPoint()));
    IFC(rgTypes.Add(PathPointTypeStart));

    // Add the bulk of the figure
    if (!(m_refData.SetToFirstSegment()))
        goto Cleanup;

    do 
    {
        m_refData.GetCurrentSegment(bType, pt);
        if (fSkipGaps)
        {
            // Irrelevant unless we were told to skip gaps
            if (m_refData.IsAtAGap())
            {
                // This segment is a gap; record the fact and move on
                fPreviousSegmentIsAGap = true;
                if (MilCoreSeg::TypeLine == bType)
                {
                    ptGapEnd = *pt;
                }
                else
                {
                    ptGapEnd = pt[2];
                }
                continue;   // Skipping the rest
            }
            else if (fPreviousSegmentIsAGap)
            {
                // The previous segment was a gap and this one is not.
                // Start a new figure at the gap's endpoint
                IFC(rgPoints.Add(ptGapEnd));
                IFC(rgTypes.Add(PathPointTypeStart));
                fPreviousSegmentIsAGap = false;
            }
        }

        if (MilCoreSeg::TypeLine == bType)
        {
            IFC(rgPoints.Add(*pt));
            IFC(rgTypes.Add(PathPointTypeLine));
        }
        else
        {
            Assert(MilCoreSeg::TypeBezier == bType);

            IFC(rgPoints.AddMultipleAndSet(pt, 3));
            IFC(rgTypes.AddAndSet(3, PathPointTypeBezier));
        }
    }
    while (m_refData.SetToNextSegment());

    if (m_refData.IsClosed() && !fPreviousSegmentIsAGap)
    {
        // Close the figure
        rgTypes.Last() |= PathPointTypeCloseSubpath;
    }
Cleanup:
    RRETURN(hr);
}
#ifdef DBG
//+-----------------------------------------------------------------------------
//
//  Member:
//      CFigureBase::Dump
//
//  Synopsis:
//      Debug dump
//
//------------------------------------------------------------------------------
void 
CFigureBase::Dump() const
{
    OutputDebugString(L"Figure\n");

    if (!m_refData.IsEmpty())
    {
        MILDebugOutput(
            L"Start at = (%f, %f)\n",
             m_refData.GetStartPoint().X,
             m_refData.GetStartPoint().Y);
    }

    if (!m_refData.HasNoSegments())
    {
        CFigureDumper dumper;

        // This is debug spew... don't care about failure.
        IGNORE_HR(dumper.TraverseForward(m_refData));
    }
}
#endif



