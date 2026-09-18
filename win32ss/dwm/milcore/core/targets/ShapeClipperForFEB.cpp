// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+---------------------------------------------------------------------------
//

//
//  Description:
//      Contains implementation for CShapeClipperForFEB
//

#include "precomp.hpp"


//+------------------------------------------------------------------------
//
//  Function:  CShapeClipperForFEB::CShapeClipperForFEB
//
//  Synopsis:  ctor.
//
//-------------------------------------------------------------------------
CShapeClipperForFEB::CShapeClipperForFEB(
    __in_ecount(1) const IShapeData *pShape,
    __in_ecount(1) const CRectF<CoordinateSpace::Shape> *prcGivenShapeBounds,
    __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice
    )
{
    m_pGivenShape = pShape;
    m_pFinalShape = m_pGivenShape;

    m_fGivenShapeBoundsEmpty = prcGivenShapeBounds->IsEmpty();

    m_finalShapeBoundsDeviceSpace.Set(*prcGivenShapeBounds);
    if (pmatShapeToDevice)
    {
        m_finalShapeBoundsDeviceSpace.Transform(pmatShapeToDevice);
    }

    m_pmatShapeToDevice = pmatShapeToDevice;

    // m_matShapeToDevice is not initialized until needed - see DoApplyGuidelines
}

//+------------------------------------------------------------------------
//
//  Function:  CShapeClipperForFEB::ApplyBrush
//
//  Synopsis:  Using the given input parameters, generate the shapes that
//             can be used to stroke and/or fill a path.
//
//  Notes:     The output shapes will be set to the address of one of the
//             input shapes. They should not be deleted, but their lifetime
//             is dependant on the input shapes.
//
//             The shape bounds could potentially be made smaller
//             by intersecting them with the image rectangle. However,
//             there are no current cases where we expect this to be worth
//             the calculation.
//
//-------------------------------------------------------------------------
HRESULT
CShapeClipperForFEB::ApplyBrush(
    __in_ecount_opt(1) const CMILBrush *pBrush,
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorldToDevice,
    __inout_ecount(1) CShape *pScratchShape
    )
{
    // SetShape() should be called before applying brush
    Assert(m_pFinalShape);

    HRESULT hr = S_OK;

    const CMILBrushBitmap *pBitmapBrushNoRef = NULL;
    CParallelogram sourceClipDeviceSpace;

    if (pBrush && pBrush->GetType() == BrushBitmap)
    {
        pBitmapBrushNoRef = DYNCAST(const CMILBrushBitmap, pBrush);
        Assert(pBitmapBrushNoRef);

        if (!pBitmapBrushNoRef->HasSourceClip() || 
             m_fGivenShapeBoundsEmpty // we don't care to do an intersect if the shape was empty
            )
        {
            // skip the intersect
            goto Cleanup;
        }
    }
    else
    {
        // skip the intersect
        goto Cleanup;
    }

    //
    // Calculate sourceClip parallelogram in deviceSpace
    //

    pBitmapBrushNoRef->GetSourceClipSampleSpace(
        &matWorldToDevice,
        &sourceClipDeviceSpace
        );

    //
    // See if we even need an intersection- small shape, big base tile case
    //

    if (sourceClipDeviceSpace.Contains(
            m_finalShapeBoundsDeviceSpace,
            INSIGNIFICANT_PIXEL_COVERAGE_SRGB // rTolerance-- okay to use this as is because we are in device space
            ))
    {
        // skip the intersect
        goto Cleanup;
    }

    //
    // Intersect the destination rectangle with the input shape
    // and put the result in pScratchShape.
    //

    pScratchShape->Reset(FALSE);

    IFC(CShapeBase::ClipWithParallelogram(
        m_pFinalShape,
        &sourceClipDeviceSpace,
        pScratchShape,
        m_pmatShapeToDevice
        ));

    m_pFinalShape = pScratchShape;
    m_pmatShapeToDevice = NULL;

    //
    // Note: The shape bounds could potentially be made smaller
    // by intersecting them with the image rectangle. However,
    // there are no current cases where we expect this to be worth
    // the calculation.
    //

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
// 
//  Class:  CSnappingTask
// 
//  Synopsis:
//      Helper for CShapeClipperForFEB::DoApplyGuidelines().
//      Used to traverse the points in the figure and apply
//      pixel snapping to each of them.
//

class CSnappingTask : public CFigureTask
{
public:
    CSnappingTask(
        __in_ecount(1) CFigureData *pFigure,
        __in_ecount(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmat,
        __in_ecount(1)CSnappingFrame *pSnappingFrame
        )
    {
        m_pFigure = pFigure;
        m_pmat = pmat;
        m_pSnappingFrame = pSnappingFrame;
    }

    virtual HRESULT DoLine(const MilPoint2F &ptEnd)
    {
        MilPoint2F ptTransformed = ptEnd;
        if (m_pmat)
        {
            TransformPoint(*m_pmat, IN OUT ptTransformed);
        }

        SnapPoint(ptTransformed);

        return m_pFigure->LineTo(ptTransformed.X, ptTransformed.Y);
    }
    virtual HRESULT DoBezier(
        __in_ecount(3) const MilPoint2F *pt
        )
    {
        MilPoint2F ptTransformed[3];

        if (m_pmat)
        {
            m_pmat->Transform(IN pt, OUT ptTransformed, 3);
        }
        else
        {
            ptTransformed[0] = pt[0];
            ptTransformed[1] = pt[1];
            ptTransformed[2] = pt[2];
        }

        SnapPoint(ptTransformed[0]);
        SnapPoint(ptTransformed[1]);
        SnapPoint(ptTransformed[2]);

        return m_pFigure->BezierTo(
            ptTransformed[0].X, ptTransformed[0].Y,
            ptTransformed[1].X, ptTransformed[1].Y,
            ptTransformed[2].X, ptTransformed[2].Y
            );
    }

public:

    void SnapPoint(
        __inout_ecount(1) MilPoint2F &point
        )
    {
        m_pSnappingFrame->SnapPoint(point);
    }

private:
    CFigureData *m_pFigure;
    const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *m_pmat;
    CSnappingFrame *m_pSnappingFrame;
};

//+------------------------------------------------------------------------
//
//  Function:  CShapeClipperForFEB::DoApplyGuidelines
//
//  Synopsis:
//      Execute pixel snapping: shift every point in the shape by
//      small offset (up to 1/2 of pixel), using given CSnappingFrame.
//
//-------------------------------------------------------------------------
HRESULT
CShapeClipperForFEB::DoApplyGuidelines(
    __in_ecount(1) CSnappingFrame *pSnappingFrame,
    __inout_ecount(1) CShape *pScratchShape
    )
{
    // SetShape() should be called before applying guidelines
    Assert(m_pFinalShape);

    HRESULT hr = S_OK;

    UINT nFigures = m_pFinalShape->GetFigureCount();
    if (nFigures)
    {
        if (pSnappingFrame->IsSimple())
        {
            // Snapping frame contains not more than one horizontal and one
            // vertical guideline, so the snapping transformation is linear.
            // We need not compose new shape, instead use matrix adjustment.
            // Since m_matShapeToDevice was not initialized by ctor it must be
            // set now.
            if (m_pmatShapeToDevice == NULL)
            {
                // NULL m_pmatShapeToDevice means pretransformation work was
                // done and the effective transform is now identity.
                m_matShapeToDevice = m_matShapeToDevice.refIdentity();
            }
            else
            {
                m_matShapeToDevice = *m_pmatShapeToDevice;
            }

            pSnappingFrame->SnapTransform(m_matShapeToDevice);

            m_pmatShapeToDevice = &m_matShapeToDevice;
        }
        else
        {
            // Can we do the snapping in place?

            pScratchShape->Reset();

            Assert(m_pFinalShape != pScratchShape);

            for (UINT idxFigure = 0; idxFigure < nFigures; idxFigure++)
            {
                CFigureData *pFigure = NULL;
                IFC(pScratchShape->AddFigure(OUT pFigure));

                const IFigureData &fd = m_pFinalShape->GetFigure(idxFigure);
                if (!fd.IsEmpty())
                {
                    MilPoint2F ptStart = fd.GetStartPoint();

                    if (m_pmatShapeToDevice)
                    {
                        TransformPoint(*m_pmatShapeToDevice, ptStart);
                    }

                    CSnappingTask task(
                        pFigure,
                        m_pmatShapeToDevice,
                        pSnappingFrame
                        );

                    task.SnapPoint(ptStart);

                    IFC(pFigure->StartAt(ptStart.X, ptStart.Y));

                    IFC(task.TraverseForward(fd));

                    if (fd.IsClosed())
                    {
                        IFC(pFigure->Close());
                    }

                    pFigure->SetFillable(static_cast<BOOL>(fd.IsFillable()));
                }
            }

            pScratchShape->SetFillMode(m_pFinalShape->GetFillMode());
            m_pFinalShape = pScratchShape;
            // Shape is transformed; set ShapeToDevice to "effective" identity
            m_pmatShapeToDevice = NULL;
        }
    }

Cleanup:
    return hr;
}






