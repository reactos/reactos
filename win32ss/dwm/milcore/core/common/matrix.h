// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Definition of the matrix transform class used by the MIL.
//      This class derives from CBaseMatrix which derives from D3DMATRIX, and
//      adds no additional data to the memory footprint.  This is done to
//      maximize interchangeability between matrix classes and minimize
//      overhead.
//
//------------------------------------------------------------------------

#pragma once


MtExtern(CMatrix);

//+----------------------------------------------------------------------------
//
//  Class:
//      CMatrix<InCoordSpace, OutCoordSpace>
//
//  Synopsis:
//      Extends CBaseMatrix class with coordinate space type safety and
//      coordinate space transform related operations.
//
//-----------------------------------------------------------------------------

template <typename InCoordSpace, typename OutCoordSpace>
class CMatrix : public CBaseMatrix
{
    // No extra data members, including vtables, should ever be added to CMatrix
    void _CAssertSize_() { C_ASSERT(sizeof(CMatrix) == sizeof(CBaseMatrix)); }

public:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMatrix));

    // CMILMatrix constructors that expose CBaseMatrix constructors

    CMatrix(bool fInitialize) : CBaseMatrix(fInitialize) {}
    CMatrix(__in_ecount(1) MilMatrix3x2D const *pMatrix) : CBaseMatrix(pMatrix) {}
    CMatrix(__in_ecount(16) const float * pf) : CBaseMatrix(pf) {}
    CMatrix(__in_ecount(1) const CMatrix& m) : CBaseMatrix(m) { }
    CMatrix(float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23,
            float m30, float m31, float m32, float m33)

            :  CBaseMatrix( m00, m01, m02, m03,
                            m10, m11, m12, m13,
                            m20, m21, m22, m23,
                            m30, m31, m32, m33)
    {
    }

    template<typename... Args>
    inline CMatrix(Args&&...args) : CBaseMatrix(std::forward<Args>(args)...)
    {
        // empty
    }


    //+------------------------------------------------------------------------
    //
    //  Member:
    //      refIdentity / pIdentity
    //
    //  Synopsis:
    //      Convenience methods to get a space specific identity matrix to
    //      transform from one space to another
    //
    //  Notes:
    //      Use should be limited as usage normally indicates preprocessing of
    //      coordinate space and treating the results as it they were not
    //      preprocessed and only transforming them later by this identity
    //      transform.
    //
    //-------------------------------------------------------------------------

    static __returnro CMatrix const &refIdentity()
    {
        return ReinterpretBase(IdentityMatrix);
    }

    static __returnro CMatrix const *pIdentity()
    {
        return ReinterpretBase(&IdentityMatrix);
    }


    //+------------------------------------------------------------------------
    //
    //  Member:
    //      InferAffineMatrix
    //
    //  Synopsis:
    //      Set transform value to the transform needed to convert from InSpace
    //      rectangle to OutSpace parallelogram.  OutSpace parallelogram is
    //      specified either and 3 points or a rectangle.
    //
    //-------------------------------------------------------------------------

    void InferAffineMatrix(
        __in_ecount(1) const CRectF<InCoordSpace> &rcInSpace,
        __in_ecount(3) const MilPoint2F *rgptOutSpace
        )
    {
        CBaseMatrix::InferAffineMatrix(rcInSpace, rgptOutSpace);
    }

    void InferAffineMatrix(
        __in_ecount(1) const CRectF<InCoordSpace> &rcInSpace,
        __in_ecount(1) const CRectF<OutCoordSpace> &rcOutSpace
        )
    {
        CBaseMatrix::InferAffineMatrix(rcInSpace, rcOutSpace);
    }

    //+------------------------------------------------------------------------
    //
    //  Member:
    //      Transform2DBounds*
    //
    //  Synopsis:
    //      Convert a bounding rectangle from one coordinate space to another
    //      always expanding bounds to ensure any point within InSpace bounds
    //      transformed to Out space will fall within output OutSpace bounds.
    //
    //      Transform2DBounds is common implementation and does not check for
    //      NaN results.  It produces NaN results somewhat erratically. See
    //      CBaseMatrix::Transform2DBounds implementation notes.
    //
    //      Transform2DBoundsConservative watches for NaN results and produces
    //      infinite bounds.  See CBaseMatrix::Transform2DBoundsConservative
    //      implementation notes.
    //
    //      Transform2DBoundsNullSafe checks for a NULL matrix (this) and
    //      simply copies result assuming NULL implies identity transform.
    //
    //-------------------------------------------------------------------------

    void Transform2DBounds(
        __in_ecount(1) const CRectF<InCoordSpace> &rcInSpace,
        __out_ecount(1) CRectF<OutCoordSpace> &rcOutSpace
        ) const
    {
        CBaseMatrix::Transform2DBounds(rcInSpace, rcOutSpace);
    }

    void Transform2DBoundsConservative(
        __in_ecount(1) const CRectF<InCoordSpace> &rcInSpace,
        __out_ecount(1) CRectF<OutCoordSpace> &rcOutSpace
        ) const
    {
        CBaseMatrix::Transform2DBoundsConservative(rcInSpace, rcOutSpace);
    }

    static void Transform2DBoundsNullSafe(
        __in_ecount_opt(1) const CMatrix *matrix,
        __in_ecount(1) const CRectF<InCoordSpace> &rcInSpace,
        __out_ecount(1) CRectF<OutCoordSpace> &rcOutSpace
        )
    {
        CBaseMatrix::Transform2DBoundsNullSafe(matrix, rcInSpace, rcOutSpace);
    }

    void Transform2DBoundsNullSafe(
        __in_ecount(1) const CRectF<InCoordSpace> &rcInSpace,
        __out_ecount(1) CRectF<OutCoordSpace> &rcOutSpace
        ) const
    {
        CBaseMatrix::Transform2DBoundsNullSafe(this, rcInSpace, rcOutSpace);
    }


    //+------------------------------------------------------------------------
    //
    //  Member:
    //      SetToMultiplyResult
    //
    //  Synopsis:
    //      Set transform value to the multiplied result of m1 and m2.
    //
    //-------------------------------------------------------------------------

    template <typename CommonCoordSpace>
    VOID SetToMultiplyResult(
        __in_ecount(1) const CMatrix<InCoordSpace,CommonCoordSpace> &m1,
        __in_ecount(1) const CMatrix<CommonCoordSpace,OutCoordSpace>   &m2
        )
    {
        CBaseMatrix::SetToMultiplyResult(m1, m2);
    }


    //+------------------------------------------------------------------------
    //
    //  Member:
    //      Invert
    //
    //  Synopsis:
    //      Set transform value to the inversion of "in".
    //
    //-------------------------------------------------------------------------

    BOOL Invert(
        __in_ecount(1) const CMatrix<OutCoordSpace,InCoordSpace> &in
        )
    {
        return CBaseMatrix::Invert(in);
    }


    //+------------------------------------------------------------------------
    //
    //  Member:
    //      ComputePrefilteringDimensions
    //
    //  Synopsis:
    //      Given a realization of a certain size, estimate an intermediate
    //      size to which the realization could be prefiltered to provide high
    //      quality sampling with a 2x2 linear reconstruction filter.
    //
    //-------------------------------------------------------------------------

    VOID ComputePrefilteringDimensions(
        __range(>=, 1) UINT uRealizationWidth,
        __range(>=, 1) UINT uRealizationHeight,
        REAL rShrinkFactorThreshold,
        __deref_out_range(>=, 1) UINT &uDesiredWidth,
        __deref_out_range(>=, 1) UINT &uDesiredHeight
        ) const
    {
        { C_ASSERT(InCoordSpace::Id == CoordinateSpaceId::RealizationSampling); }
        { C_ASSERT(   (OutCoordSpace::Id == CoordinateSpaceId::Device)
                   || (OutCoordSpace::Id == CoordinateSpaceId::IdealSampling)); }

        CBaseMatrix::ComputePrefilteringDimensions(
            uRealizationWidth,
            uRealizationHeight,
            rShrinkFactorThreshold,
            uDesiredWidth,
            uDesiredHeight
            );
    }

    //+------------------------------------------------------------------------
    //
    //  Member:
    //      AdjustForPrefiltering
    //
    //  Synopsis:
    //      Given a realization of a certain size, estimate an intermediate
    //      size to which the realization could be prefiltered to provide high
    //      quality sampling with a 2x2 linear reconstruction filter.
    //
    //      Then remove the corresponding scale factor from this transform if and only if:
    //      (*puDesiredWidth != uBitmapWidth) || (*puDesiredHeight != uBitmapHeight)
    //
    //-------------------------------------------------------------------------

    VOID AdjustForPrefiltering(
        __range(>=, 1) UINT uRealizationWidth,
        __range(>=, 1) UINT uRealizationHeight,
        REAL rShrinkFactorThreshold,
        __deref_out_range(>=, 1) UINT *puDesiredWidth,
        __deref_out_range(>=, 1) UINT *puDesiredHeight
        )
    {
        { C_ASSERT(InCoordSpace::Id == CoordinateSpaceId::RealizationSampling); }
        { C_ASSERT(   (OutCoordSpace::Id == CoordinateSpaceId::Device)
                   || (OutCoordSpace::Id == CoordinateSpaceId::IdealSampling)); }

        CBaseMatrix::AdjustForPrefiltering(
            uRealizationWidth,
            uRealizationHeight,
            rShrinkFactorThreshold,
            puDesiredWidth,
            puDesiredHeight);
    }



    //=========================================================================
    // Casting Helper Routines
    //

    //+------------------------------------------------------------------------
    //
    //  Member:
    //      ReinterpretAsVariantOut
    //
    //  Synopsis:
    //      Reinterpret this transform as having Variant Out coordinate space. 
    //      Use should be limited.
    //
    //-------------------------------------------------------------------------

    __ecount(1) CMatrix<InCoordSpace,CoordinateSpace::Variant> const &ReinterpretAsVariantOut(
        ) const
    {
        C_ASSERT(OutCoordSpace::Id != CoordinateSpaceId::Invalid);
        C_ASSERT(sizeof(*this) == sizeof(CMatrix<InCoordSpace,CoordinateSpace::Variant>));
        return reinterpret_cast<CMatrix<InCoordSpace,CoordinateSpace::Variant> const &>(*this);
    }


    //+------------------------------------------------------------------------
    //
    //  Member:
    //      ReinterpretBase
    //
    //  Synopsis:
    //      Casting helpers to reinterpret base type as space specific type,
    //      which is safe because all data members should be the same
    //
    //  Notes:
    //      These members should not be used to reinterpret one coordinate
    //      space transform to another.  See Reinterpret*Space1*As*Space2*
    //      methods below for such reinterpretation needs.
    //
    //-------------------------------------------------------------------------

    static __returnro CMatrix const &ReinterpretBase(
        __in_ecount(1) CBaseMatrix const &m
        )
    {
        return static_cast<CMatrix const &>(m);
    }

    static __returnro CMatrix const *ReinterpretBase(
        __in_ecount(1) CBaseMatrix const *pm
        )
    {
        return static_cast<CMatrix const *>(pm);
    }

    static __ecount(1) CMatrix *ReinterpretBaseForModification(
        __in_ecount(1) CBaseMatrix *pm
        )
    {
        return static_cast<CMatrix *>(pm);
    }
};


// Explicit instantiation of at least one type to trigger compile errors early
typedef CMatrix<CoordinateSpace::TextureSampling, CoordinateSpace::IdealSampling> CMatrixTextureSamplingToIdealSampling;


//+----------------------------------------------------------------------------
//
//  Function:
//      Reinterpret*Space1*As*Space2*
//
//  Synopsis:
//      Helper methods to reinterpret one coordinate space as another
//      coordinate space.  Use of helpers are preferred over direct
//      reinterpret_cast, becacuse reinterpret_cast is dangerous and at least
//      here sizes can be asserted.
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Function:
//      ReinterpretLocalRenderingAsBaseSampling
//
//  Synopsis:
//      Helper method to reinterpret LocalRendering coordinate space as
//      BaseSampling coordinate space.  This is a common operation for 2D
//      rendering primitives.
//
//-----------------------------------------------------------------------------

MIL_FORCEINLINE __returnro const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &
ReinterpretLocalRenderingAsBaseSampling(
    __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::Device> &m
    )
{
    C_ASSERT(sizeof(m) == sizeof( CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> ));
    return reinterpret_cast<const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &>(m);
}

//+----------------------------------------------------------------------------
//
//  Function:
//      ReinterpretIdealSamplingAsDevice
//
//  Synopsis:
//      Helper method to reinterpret IdealSampling coordinate space as Device
//      coordinate space.  IdealSampling is the coordinate space used when
//      Device space is desired, but only an approximation can be made.  This
//      happens when rendering primitive does not provide a direct mapping from
//      source to target, i.e. mesh primitives.  At some point we don't care to
//      carry the differentiation any further and use this method to
//      reinterpret IdealSampling as Device.
//
//-----------------------------------------------------------------------------

MIL_FORCEINLINE __returnro const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &
ReinterpretIdealSamplingAsDevice(
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::IdealSampling> &m
    )
{
    C_ASSERT(sizeof(m) == sizeof( CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> ));
    return reinterpret_cast<const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::Device> &>(m);
}

//+----------------------------------------------------------------------------
//
//  Function:
//      ReinterpretLocalRenderingAsRealizationSampling
//
//  Synopsis:
//      Helper method to reinterpret LocalRendering coordinate space as
//      RealizationSampling coordinate space.  Quite similar to reinterpreting
//      LocalRendering as BaseSampling, but for case of DrawBitmap when source
//      based coordinates are used.
//
//-----------------------------------------------------------------------------

MIL_FORCEINLINE __returnro const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> &
ReinterpretLocalRenderingAsRealizationSampling(
    __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::Device> &m
    )
{
    C_ASSERT(sizeof(m) == sizeof( CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> ));
    return reinterpret_cast<const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> &>(m);
}

//+----------------------------------------------------------------------------
//
//  Function:
//      ReinterpretPageInPixelsAsLocalRendering
//
//  Synopsis:
//      Helper method to reinterpret PageInPixels coordinate space as
//      LocalRendering coordinate space.  Useful when LocalRendering (Shape)
//      has been flattened to PageInPixels and LocalToPageInPixel transform is
//      then set to Identity, but source (brush) transform must respect the
//      true LocalRenderingToPageInPixels transform.  
//
//-----------------------------------------------------------------------------

MIL_FORCEINLINE __returnro const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::LocalRendering> &
ReinterpretPageInPixelsAsLocalRendering(
    __in_ecount(1) const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::PageInPixels> &m
    )
{
    C_ASSERT(sizeof(m) == sizeof( CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::LocalRendering> ));
    return reinterpret_cast<const CMatrix<CoordinateSpace::BaseSampling,CoordinateSpace::LocalRendering> &>(m);
}

//+----------------------------------------------------------------------------
//
//  Function:
//      ReinterpretLocalRenderingAsMILMatrix
//
//  Synopsis:
//      Helper method to reinterpret PageInPixels coordinate space as
//      LocalRendering coordinate space.  Useful when LocalRendering (Shape)
//      has been flattened to PageInPixels and LocalToPageInPixel transform is
//      then set to Identity, but source (brush) transform must respect the
//      true LocalRenderingToPageInPixels transform.  See
//      CDrawingContext::DrawVisualTree.
//
//-----------------------------------------------------------------------------

MIL_FORCEINLINE __returnro const CMILMatrix *
ReinterpretLocalRenderingAsMILMatrix(
    __in_ecount(1) const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *m
    )
{
    C_ASSERT(sizeof(*m) == sizeof( CMILMatrix ));
    return reinterpret_cast<const CMILMatrix *>(m);
}

