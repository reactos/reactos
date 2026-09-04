// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics
//      $Keywords:
//
//  $Description:
//      Declarations and implementation of CDelayComputedBounds.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Class:
//      CDelayComputedBounds<CoordinateSpace>
//
//  Synopsis:
//      Templated class that stores a bounds rectangle in templated coordinate
//      space as a rectangle in another space and a transform that may be used
//      to produce the needed bounds on demand.
//
//------------------------------------------------------------------------------

template <typename ResultCoordSpace>
class CDelayComputedBounds
{
public:
    CDelayComputedBounds()
    {
    #if DBG_ANALYSIS
        m_fDbgInitialized = false;
    #endif
    }

    template <typename GivenCoordSpace>
    void SetBoundsRectAndInverseTransform(
        __in_ecount(1) CRectF<GivenCoordSpace> const &rcBounds,
        __in_ecount(1) CMatrix<ResultCoordSpace,GivenCoordSpace> const *pmatResultToGiven
        )
    {
        C_ASSERT(GivenCoordSpace::Id != CoordinateSpaceId::Invalid);
        m_rcGivenBounds = rcBounds.ReinterpretAsVariant();
        m_pmatResultToGiven = &(pmatResultToGiven->ReinterpretAsVariantOut());
        m_fResultComputed = false;
    #if DBG_ANALYSIS
        m_fDbgInitialized = true;
    #endif
    }

    bool GetBounds(
        __out_ecount(1) CRectF<ResultCoordSpace> &rcResultBounds
        ) const
    {
        Assert(m_fDbgInitialized);

        if (!m_fResultComputed)
        {
            MILMatrix3x2 matGivenToResult;

            if (matGivenToResult.SetInverse(
                m_pmatResultToGiven->m[0][0],
                m_pmatResultToGiven->m[0][1],
                m_pmatResultToGiven->m[1][0],
                m_pmatResultToGiven->m[1][1],
                m_pmatResultToGiven->GetDx(),
                m_pmatResultToGiven->GetDy()
                ))
            {
                // Compute bounds and cache
                matGivenToResult.Transform2DBounds(
                    IN m_rcGivenBounds,
                    OUT m_rcResultBounds
                    );
                m_fResultComputed = true;
            }
        }

        if (m_fResultComputed)
        {
            rcResultBounds = m_rcResultBounds;
        }

        return m_fResultComputed;
    }

private:
    CRectF<CoordinateSpace::Variant> m_rcGivenBounds;
    CMatrix<ResultCoordSpace,CoordinateSpace::Variant> const *m_pmatResultToGiven;
    mutable bool m_fResultComputed;
    mutable CRectF<ResultCoordSpace> m_rcResultBounds;

#if DBG_ANALYSIS
    bool m_fDbgInitialized;
#endif
};



