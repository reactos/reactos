// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      Definition of a matrix transform class. This class derives from
//      CBaseMatrix, and adds no additional data to the memory footprint in
//      retail builds.  This is done to maximize interchangeability between
//      matrix classes and minimize overhead.  In checked/analyzed builds one
//      member is added to track Out coordinate space and Asser proper use at
//      runtime and during analysis.
//
//-----------------------------------------------------------------------------

#pragma once

//+----------------------------------------------------------------------------
//
//  Class:
//      CMultiOutSpaceMatrix<InCoordSpace>
//
//  Synopsis:
//      Dynamic CMatrix representation.  It always transforms from a fixed In
//      coordinate space, but the Out space can change as the matrix is modifed.
//
//      Proper manipulation and use of matrix in relation to its Out coordinate
//      space are checked via runtime Asserts.
//
//-----------------------------------------------------------------------------

template <typename InCoordSpace>
class CMultiOutSpaceMatrix : public CBaseMatrix
{
public:

    CMultiOutSpaceMatrix()
    DBG_ANALYSIS_PARAM(: m_eDbgCurrentOutCoordSpaceId(CoordinateSpaceId::Invalid))
    {
    }

    CMultiOutSpaceMatrix(bool fInitialize)
    DBG_ANALYSIS_PARAM(: m_eDbgCurrentOutCoordSpaceId(CoordinateSpaceId::Invalid))
    {
        if (fInitialize) { SetToIdentity(); }
    }

    template <typename OutCoordSpace>
    CMultiOutSpaceMatrix(
        __in_ecount(1) const CMatrix<InCoordSpace, OutCoordSpace> &matCopyFrom
        )
        : CBaseMatrix(matCopyFrom)
          DBG_ANALYSIS_COMMA_PARAM(m_eDbgCurrentOutCoordSpaceId(OutCoordSpace::Id))
    {
    }

#if !DBG_ANALYSIS
    template <typename OutCoordSpace>
    static CMultiOutSpaceMatrix const &ReinterpretCMatrixAsCMultiOutSpaceMatrix(
        __in_ecount(1) CMatrix<InCoordSpace, OutCoordSpace> const &matRef
        )
    {
        C_ASSERT(sizeof(CMultiOutSpaceMatrix) == sizeof(matRef));
        return reinterpret_cast<CMultiOutSpaceMatrix const &>(matRef);
    }
#endif

    template <typename OutCoordSpace>
    operator const typename CMatrix<InCoordSpace,OutCoordSpace> &() const
    {
        Assert(m_eDbgCurrentOutCoordSpaceId == OutCoordSpace::Id);
        return *reinterpret_cast<CMatrix<InCoordSpace,OutCoordSpace> const *>(this);
    }

    // cast operator for assignment
    //
    // Note: this does not assert current Out space, but sets it.  Ideally,
    //       there would be an explicit set of Out space, but that adds too much
    //       clutter to code and can be quite difficult in certain situations
    //       with the matrix being an passed as an out parameter.
    template <typename OutCoordSpace>
    operator typename CMatrix<InCoordSpace,OutCoordSpace> &()
    {
#if DBG_ANALYSIS
        if (m_eDbgCurrentOutCoordSpaceId != OutCoordSpace::Id)
        {
            m_eDbgCurrentOutCoordSpaceId = OutCoordSpace::Id;
        }
#endif
        return *reinterpret_cast<CMatrix<InCoordSpace,OutCoordSpace> *>(this);
    }

    operator typename CMultiOutSpaceMatrix<CoordinateSpace::Variant> &()
    {
        return *reinterpret_cast<CMultiOutSpaceMatrix<CoordinateSpace::Variant> *>(this);
    }

#if DBG_ANALYSIS
    CoordinateSpaceId::Enum DbgCurrentCoordinateSpaceId() const
    {
        Assert(m_eDbgCurrentOutCoordSpaceId != CoordinateSpaceId::Invalid);
        return m_eDbgCurrentOutCoordSpaceId;
    }
#endif

    template <typename CurrentOutCoordSpace, typename NewOutCoordSpace>
    void DbgChangeToSpace()
    {
        Assert(m_eDbgCurrentOutCoordSpaceId == CurrentOutCoordSpace::Id);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = NewOutCoordSpace::Id);
    }


    void SetToIdentity()
    {
        CBaseMatrix::SetToIdentity();
        WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = InCoordSpace::Id);
    }

    template <typename CommonCoordSpace, typename OutCoordSpace>
    void SetToMultiplyResult(
        __in_ecount(1) const CMatrix<InCoordSpace,CommonCoordSpace> &m1,
        __in_ecount(1) const CMatrix<CommonCoordSpace,OutCoordSpace>   &m2
        )
    {
        CBaseMatrix::SetToMultiplyResult(m1, m2);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = OutCoordSpace::Id);
    }

    template <typename CommonCoordSpace>
    void SetToMultiplyResult(
        __in_ecount(1) const CMatrix<InCoordSpace,CommonCoordSpace> &m1,
        __in_ecount(1) const CMultiOutSpaceMatrix<CommonCoordSpace>    &m2
        )
    {
        CBaseMatrix::SetToMultiplyResult(m1, m2);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = m2.DbgCurrentCoordinateSpaceId());
    }


    template <typename CommonCoordSpace, typename OutCoordSpace>
    void AppendMultiply(__in_ecount(1) const CMatrix<CommonCoordSpace, OutCoordSpace> &matAppend)
    {
        Assert(m_eDbgCurrentOutCoordSpaceId == CommonCoordSpace::Id);
        CBaseMatrix::Multiply(matAppend);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = OutCoordSpace::Id);
    }

    template <typename CommonCoordSpace>
    void AppendMultiply(__in_ecount(1) const CMultiOutSpaceMatrix<CommonCoordSpace> &matAppend)
    {
        Assert(m_eDbgCurrentOutCoordSpaceId == CommonCoordSpace::Id);
        CBaseMatrix::Multiply(matAppend/*.Base()*/);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = matAppend.DbgCurrentCoordinateSpaceId());
    }


    template <typename OutCoordSpace>
    void Transform2DBounds(
        __in_ecount(1) const CRectF<InCoordSpace> &rcInSpace,
        __out_ecount(1) CRectF<OutCoordSpace> &rcOutSpace
        ) const
    {
        Assert(m_eDbgCurrentOutCoordSpaceId == OutCoordSpace::Id);
        CBaseMatrix::Transform2DBounds(rcInSpace, rcOutSpace);
    }

    void Transform2DBounds(
        __in_ecount(1) const CRectF<InCoordSpace> &rcInSpace,
        __out_ecount(1) CMultiSpaceRectF<CoordinateSpace::PageInPixels,CoordinateSpace::Device> &rcOutSpace
        ) const
    {
#if DBG_ANALYSIS
        if (m_eDbgCurrentOutCoordSpaceId == CoordinateSpace::Device::Id)
        {
            Transform2DBounds(rcInSpace, rcOutSpace.Device());
        }
        else
#endif
        {
            Transform2DBounds(rcInSpace, rcOutSpace.PageInPixels());
        }
    }

    template <typename OutCoordSpace>
    void Transform2DBoundsConservative(
        __in_ecount(1) const CRectF<InCoordSpace> &rcInSpace,
        __out_ecount(1) CRectF<OutCoordSpace> &rcOutSpace
        ) const
    {
        Assert(m_eDbgCurrentOutCoordSpaceId == OutCoordSpace::Id);
        CBaseMatrix::Transform2DBoundsConservative(rcInSpace, rcOutSpace);
    }


    VOID ComputePrefilteringDimensions(
        __range(>=, 1) UINT uRealizationWidth,
        __range(>=, 1) UINT uRealizationHeight,
        REAL rShrinkFactorThreshold,
        __deref_out_range(>=, 1) UINT &uDesiredWidth,
        __deref_out_range(>=, 1) UINT &uDesiredHeight
        ) const
    {
        C_ASSERT(InCoordSpace::Id == CoordinateSpaceId::RealizationSampling);
        Assert(   (m_eDbgCurrentOutCoordSpaceId == CoordinateSpaceId::Device)
               || (m_eDbgCurrentOutCoordSpaceId == CoordinateSpaceId::IdealSampling));

        CBaseMatrix::ComputePrefilteringDimensions(
            uRealizationWidth,
            uRealizationHeight,
            rShrinkFactorThreshold,
            uDesiredWidth,
            uDesiredHeight
            );
    }

private:

    DBG_ANALYSIS_PARAM(CoordinateSpaceId::Enum m_eDbgCurrentOutCoordSpaceId;)

};


//+----------------------------------------------------------------------------
//
//  Function:
//      ReinterpretLocalRenderingAsBaseSampling
//
//  Synopsis:
//      Helper method to reinterpret LocalRendering coordinate space as
//      BaseSampling coordinate space.  This is a common operation for 2D
//      rendering primitives.  Use of helper is preferred over direct
//      reinterpret_cast, becacuse reinterpret_cast is dangerous and at least
//      here sizes can be asserted.
//
//-----------------------------------------------------------------------------

MIL_FORCEINLINE __returnro const CMultiOutSpaceMatrix<CoordinateSpace::BaseSampling> &
ReinterpretLocalRenderingAsBaseSampling(
    __in_ecount(1) const CMultiOutSpaceMatrix<CoordinateSpace::LocalRendering> &m
    )
{
    C_ASSERT(sizeof(m) == sizeof( CMultiOutSpaceMatrix<CoordinateSpace::BaseSampling> ));
    return reinterpret_cast<const CMultiOutSpaceMatrix<CoordinateSpace::BaseSampling> &>(m);
}


