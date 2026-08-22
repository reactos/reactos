// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Definition of CMatrixBuilder - transform matrix building class.
//
//------------------------------------------------------------------------


#pragma once

//+----------------------------------------------------------------------------
//
//  Class:
//      CMatrixBuilder
//
//  Synopsis:
//      Helper class to combine a series of matrices and assert that coordinate
//      spaces are correctly combined.
//
//      Method names are the same or similar to CMILMatrix methods for ease of
//      migration.
//
//-----------------------------------------------------------------------------

template <typename ResultInCoordSpace, typename ResultOutCoordSpace>
class CMatrixBuilder
{
    typedef CMatrix<typename ResultInCoordSpace, typename ResultOutCoordSpace> ResultMatrix_t;

public:

    CMatrixBuilder(__ecount(1) ResultMatrix_t &matTarget)
    : m_matTarget(matTarget)
    DBG_ANALYSIS_COMMA_PARAM(m_eDbgCurrentInCoordSpaceId(ResultInCoordSpace::Id))
    DBG_ANALYSIS_COMMA_PARAM(m_eDbgCurrentOutCoordSpaceId(ResultOutCoordSpace::Id))
    DBG_ANALYSIS_COMMA_PARAM(m_fDbgFailedBuild(false))
    {
    }

    ~CMatrixBuilder()
    {
        Assert(m_fDbgFailedBuild || m_eDbgCurrentInCoordSpaceId == ResultInCoordSpace::Id);
        Assert(m_fDbgFailedBuild || m_eDbgCurrentOutCoordSpaceId == ResultOutCoordSpace::Id);
    }

    template <typename CoordSpace>
    void SetToIdentity()
    {
        Assert(m_eDbgCurrentInCoordSpaceId == ResultInCoordSpace::Id); /*  Not required, but expected. */
        Assert(m_eDbgCurrentOutCoordSpaceId == ResultOutCoordSpace::Id); /*  Not required, but expected. */
        m_matTarget.SetToIdentity();
        WHEN_DBG_ANALYSIS(m_eDbgCurrentInCoordSpaceId = CoordSpace::Id);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = CoordSpace::Id);
    }

    template <typename OutCoordSpace>
    void SetTo(__in_ecount(1) const CMatrix<ResultInCoordSpace, OutCoordSpace> &matCopyFrom)
    {
        Assert(m_eDbgCurrentInCoordSpaceId == ResultInCoordSpace::Id); /*  Not required, but expected. */
        Assert(m_eDbgCurrentOutCoordSpaceId == ResultOutCoordSpace::Id); /*  Not required, but expected. */
        m_matTarget = matCopyFrom;
        WHEN_DBG_ANALYSIS(m_eDbgCurrentInCoordSpaceId = ResultInCoordSpace::Id);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = OutCoordSpace::Id);
    }

    template <typename InCoordSpace>
    void SetTo(__in_ecount(1) const CMatrix<InCoordSpace, ResultOutCoordSpace> &matCopyFrom)
    {
        Assert(m_eDbgCurrentInCoordSpaceId == ResultInCoordSpace::Id); /*  Not required, but expected. */
        Assert(m_eDbgCurrentOutCoordSpaceId == ResultOutCoordSpace::Id); /*  Not required, but expected. */
        m_matTarget = matCopyFrom;
        WHEN_DBG_ANALYSIS(m_eDbgCurrentInCoordSpaceId = InCoordSpace);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = ResultOutCoordSpace::Id);
    }

    template <typename InCoordSpace, typename OutCoordSpace>
    void InferAffineMatrix(
        __in_ecount(1) const CRectF<InCoordSpace> &rcInSpace,
        __in_ecount(3) const MilPoint2F *rgptOutSpace
        )
    {
        Assert(m_eDbgCurrentInCoordSpaceId == ResultInCoordSpace::Id); /*  Not required, but expected. */
        Assert(m_eDbgCurrentOutCoordSpaceId == ResultOutCoordSpace::Id); /*  Not required, but expected. */
        m_matTarget.InferAffineMatrix(rcInSpace, rgptOutSpace);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentInCoordSpaceId = InCoordSpace::Id);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = OutCoordSpace::Id);
    }

    template <typename InCoordSpace, typename OutCoordSpace>
    void AppendMultiply(__in_ecount(1) const CMatrix<InCoordSpace, OutCoordSpace> &matAppend)
    {
        Assert(m_eDbgCurrentOutCoordSpaceId == InCoordSpace::Id);
        Assert(m_eDbgCurrentInCoordSpaceId == ResultInCoordSpace::Id); /*  Not required, but expected. */
        m_matTarget.Multiply(matAppend);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = OutCoordSpace::Id);
    }

    template <typename InCoordSpace, typename OutCoordSpace>
    void PrependMultiply(__in_ecount(1) CMatrix<InCoordSpace, OutCoordSpace> &matAppend)
    {
        Assert(m_eDbgCurrentInCoordSpaceId == OutCoordSpace::Id);
        m_matTarget.PreMultiply(matAppend);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentInCoordSpaceId = InCoordSpace::Id);
    }

    BOOL Invert()
    {
        Assert(m_eDbgCurrentInCoordSpaceId != CoordinateSpaceId::Invalid);
        Assert(m_eDbgCurrentOutCoordSpaceId != CoordinateSpaceId::Invalid);
        BOOL fSuccess = m_matTarget.Invert(m_matTarget);
        if (fSuccess)
        {
            WHEN_DBG_ANALYSIS(CoordinateSpaceId eDbgInOrg = m_eDbgCurrentInCoordSpaceId);
            WHEN_DBG_ANALYSIS(m_eDbgCurrentInCoordSpaceId = m_eDbgCurrentOutCoordSpaceId);
            WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = eDbgInOrg);
        }
        return fSuccess;
    }

    void DbgSetFailedBuild()
    {
        WHEN_DBG_ANALYSIS(m_fDbgFailedBuild = true);
    }

    template <typename InCoordSpace, typename OutCoordSpace>
    void DbgAppendIdentityMultiply()
    {
        Assert(m_eDbgCurrentOutCoordSpaceId == InCoordSpace::Id);
        WHEN_DBG_ANALYSIS(m_eDbgCurrentOutCoordSpaceId = OutCoordSpace::Id);
    }

private:

    CBaseMatrix &m_matTarget;

#if DBG_ANALYSIS
    CoordinateSpaceId::Enum m_eDbgCurrentInCoordSpaceId;
    CoordinateSpaceId::Enum m_eDbgCurrentOutCoordSpaceId;
    bool m_fDbgFailedBuild;
#endif
};

