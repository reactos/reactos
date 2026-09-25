// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+---------------------------------------------------------------------------
//

//
//  Description:
//      Declares CShapeClipperForFEB
//

#pragma once

class CSnappingFrame;


//+------------------------------------------------------------------------
// 
//  Class:  CShapeClipperForFEB
// 
//  Synopsis:
//      Helper for path rendering: class to execute clipping of arbitrary
//      shape with the shape contained in Finite Extent Brush (FEB).
//      Yet another role of this helper is to provide pixel snapping.
//
//      Instance of CShapeClipperForFEB supposed to live in stack frame.
//      To do its job, it might create temporary objects.
//      Life time of temporary objects is controlled by constructor
//      and destructor.
//
//  Usage pattern:
//
//      1. Create an instance:
//              CShapeClipperForFEB clipper(shape parameters)
//
//      2. Optionally, call clipper.ApplyGuidelines();
//         Execute pixel snapping. See GuidelineCollection.h for more details.
//
//      3. Optionally, call clipper.ApplyBrush().
//         Note it does not necessary mean that given shape will be changed.
//
//      4. Use clipper.Get*() functions to get results.
//

class CShapeClipperForFEB
{
public:

    // CShapeClipperForFEB(); no default constructor for this class
    // ~CShapeClipperForFEB(); no destructor required

    CShapeClipperForFEB(
        __in_ecount(1) const IShapeData *pShape,
        __in_ecount(1) const CRectF<CoordinateSpace::Shape> *prcGivenShapeBounds,
        __in_ecount_opt(1) const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *pmatShapeToDevice
        );

    HRESULT ApplyGuidelines(
        __in_ecount_opt(1) CSnappingFrame *pSnappingFrame,
        __inout_ecount(1) CShape *pScratchShape
        );

    HRESULT ApplyBrush(
        __in_ecount_opt(1) const CMILBrush *pBrush,
        __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &matWorldToDevice,
        __inout_ecount(1) CShape *pScratchShape
        );

    bool ShapeHasBeenCorrected() const { return m_pFinalShape != m_pGivenShape; }

    HRESULT GetBoundsInDeviceSpace(
        __out_ecount(1) CRectF<CoordinateSpace::Device> *prcBoundsDeviceSpace
        ) const
    {
        HRESULT hr = S_OK;

        if (m_fGivenShapeBoundsEmpty)
        {
            prcBoundsDeviceSpace->SetEmpty();
        }
        else
        {
            IFC(m_finalShapeBoundsDeviceSpace.GetCachedBounds(*prcBoundsDeviceSpace));
        }

    Cleanup:
        RRETURN(hr);
    }

    const IShapeData* GetShape() const { return m_pFinalShape; }
    const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *
        GetShapeToDeviceTransformOrNull() const { return m_pmatShapeToDevice; }
    const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> *
        GetShapeToDeviceTransform() const
    {
        return m_pmatShapeToDevice ? m_pmatShapeToDevice :
            m_pmatShapeToDevice->pIdentity();
    }

private:
    HRESULT DoApplyGuidelines(
        __in_ecount(1) CSnappingFrame *pSnappingFrame,
        __inout_ecount(1) CShape *pScratchShape
        );

private:
    const IShapeData *m_pGivenShape;
    bool m_fGivenShapeBoundsEmpty;

    const IShapeData* m_pFinalShape;
    CParallelogram m_finalShapeBoundsDeviceSpace;
    const CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> * m_pmatShapeToDevice;
    CMatrix<CoordinateSpace::Shape,CoordinateSpace::Device> m_matShapeToDevice;
};

//+------------------------------------------------------------------------
//
//  Function:  CShapeClipperForFEB::ApplyGuidelines
//
//  Synopsis:
//      Execute pixel snapping: shift every point in the shape by
//      small offset (up to 1/2 of pixel), using given CSnappingFrame.
//
//-------------------------------------------------------------------------
MIL_FORCEINLINE HRESULT
CShapeClipperForFEB::ApplyGuidelines(
    __in_ecount_opt(1) CSnappingFrame *pSnappingFrame,
    __inout_ecount(1) CShape *pScratchShape
    )
{
    return pSnappingFrame && !pSnappingFrame->IsEmpty()
        ? DoApplyGuidelines(pSnappingFrame, pScratchShape)
        : S_OK;
}



