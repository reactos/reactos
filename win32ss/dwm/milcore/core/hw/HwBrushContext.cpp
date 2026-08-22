// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Abstract:
//      CHwBrushContext implementation.
//      Contains data passed through the pipeline related to hw brush creation.
//

#include "precomp.hpp"


//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushContext::GetRealizationBoundsAndTransforms
//
//  Synopsis:
//      Initialize CDelayComputedBounds object for realization sampling and set
//      other related realization transforms.
//
//-----------------------------------------------------------------------------

void 
CHwBrushContext::GetRealizationBoundsAndTransforms(
    __in_ecount(1) CMILBrushBitmap *pBitmapBrush,
    __out_ecount(1) CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> &matBitmapToIdealRealization,
    __out_ecount(1) BitmapToXSpaceTransform &matRealizationToGivenSampleBoundsSpace,
    __out_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> &rcRealizationBounds
    ) const
{
    //
    // Compute Bitmap to sample space transform
    //

    pBitmapBrush->GetBitmapToSampleSpaceTransform(
        GetWorld2DToIdealSamplingSpace(),
        matBitmapToIdealRealization
        );

    //
    // Determine given sampling space used as basis (source) for transforming
    // to bitmap space
    //

    const CoordinateSpaceId::Enum eSourceCoordSpace =
        m_pContextState->GetSamplingSourceCoordSpace();

    WHEN_DBG(matRealizationToGivenSampleBoundsSpace.DbgSetXSpace(eSourceCoordSpace));

    //
    // Compute Bitmap to Device Transform
    //

    if (eSourceCoordSpace == CoordinateSpaceId::BaseSampling)
    {
        pBitmapBrush->GetBitmapToWorldSpaceTransform(
            matRealizationToGivenSampleBoundsSpace.matBitmapSpaceToXSpace
            );

        rcRealizationBounds.SetBoundsRectAndInverseTransform(
            m_rcSamplingBounds.BaseSampling(),
            &static_cast<CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling> &>(matRealizationToGivenSampleBoundsSpace.matBitmapSpaceToXSpace)
            );
    }
    else
    {
        Assert(eSourceCoordSpace == CoordinateSpaceId::Device);
        matRealizationToGivenSampleBoundsSpace.matBitmapSpaceToXSpace = matBitmapToIdealRealization;

        rcRealizationBounds.SetBoundsRectAndInverseTransform(
            m_rcSamplingBounds.Device(),
            &static_cast<CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> &>(matRealizationToGivenSampleBoundsSpace.matBitmapSpaceToXSpace)
            );
    }
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBrushContext::GetRealizationBoundsAndTransforms
//
//  Synopsis:
//      Initialize CDelayComputedBounds object for realization sampling and set
//      other related realization transforms.
//
//-----------------------------------------------------------------------------

void
CHwBrushContext::GetRealizationBoundsAndTransforms(
    __in_ecount(1) const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling> &matRealizationToBaseSampling,
    __out_ecount(1) CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> &matBitmapToIdealRealization,
    __out_ecount(1) BitmapToXSpaceTransform &matRealizationToGivenSampleBoundsSpace,
    __out_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> &rcRealizationBounds
    ) const
{
    //
    // Compute Bitmap to sample space transform
    //

    matBitmapToIdealRealization.SetToMultiplyResult(
        matRealizationToBaseSampling,
        GetWorld2DToIdealSamplingSpace()
        );

    //
    // Determine given sampling space used as basis (source) for transforming
    // to bitmap space
    //

    const CoordinateSpaceId::Enum eSourceCoordSpace =
        m_pContextState->GetSamplingSourceCoordSpace();

    WHEN_DBG(matRealizationToGivenSampleBoundsSpace.DbgSetXSpace(eSourceCoordSpace));

    //
    // Compute Bitmap to Device Transform
    //

    if (eSourceCoordSpace == CoordinateSpaceId::BaseSampling)
    {
        matRealizationToGivenSampleBoundsSpace.matBitmapSpaceToXSpace =
            matRealizationToBaseSampling;

        rcRealizationBounds.SetBoundsRectAndInverseTransform(
            m_rcSamplingBounds.BaseSampling(),
            &static_cast<CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling> &>(matRealizationToGivenSampleBoundsSpace.matBitmapSpaceToXSpace)
            );
    }
    else
    {
        Assert(eSourceCoordSpace == CoordinateSpaceId::Device);
        matRealizationToGivenSampleBoundsSpace.matBitmapSpaceToXSpace = matBitmapToIdealRealization;

        rcRealizationBounds.SetBoundsRectAndInverseTransform(
            m_rcSamplingBounds.Device(),
            &static_cast<CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> &>(matRealizationToGivenSampleBoundsSpace.matBitmapSpaceToXSpace)
            );
    }
}



