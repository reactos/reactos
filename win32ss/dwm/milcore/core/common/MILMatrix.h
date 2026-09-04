// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:
//      Declaration of the general matrix class used by the MIL.
//      This class derives from CBaseMatrix which derives from D3DMATRIX, and
//      adds no additional data to the memory footprint.  This is done to
//      maximize interchangeability between matrix classes and minimize
//      overhead.
//
//-----------------------------------------------------------------------------

#pragma once

MtExtern(CMILMatrix);

class CMILMatrix : public CBaseMatrix
{
    // No extra data members, includin vtables, should ever be added to
    // CMILMatrix
    void _CAssertSize_() { C_ASSERT(sizeof(CMILMatrix) == sizeof(CBaseMatrix)); }

public:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILMatrix));

    // CMILMatrix constructors that expose CBaseMatrix constructors

    CMILMatrix() : CBaseMatrix() {}
    CMILMatrix(bool fInitialize) : CBaseMatrix(fInitialize) {}
    CMILMatrix(__in_ecount(1) const MilMatrix3x2D *pMatrix) : CBaseMatrix(pMatrix) {}
    CMILMatrix(__in_ecount(16) const float * pf) : CBaseMatrix(pf) {}
    CMILMatrix(__in_ecount(1) const CMILMatrix& m) : CBaseMatrix(m) { }
    CMILMatrix( float m00, float m01, float m02, float m03,
                float m10, float m11, float m12, float m13,
                float m20, float m21, float m22, float m23,
                float m30, float m31, float m32, float m33)

                :  CBaseMatrix( m00, m01, m02, m03,
                                m10, m11, m12, m13,
                                m20, m21, m22, m23,
                                m30, m31, m32, m33)
    {
    }

    // D3DMATRIX copy constructor & assignment operator 

// error C4995: 'matrix-type': name was marked as #pragma deprecated
//
// Ignore deprecation of D3DMATRIX type here to provides conversion functionality
#pragma warning (push)
#pragma warning (disable : 4995)

    CMILMatrix(__in_ecount(1) const dxlayer::basetypes<dxlayer::dx_apiset>::matrix_base_t &m)
        : CBaseMatrix(m)
    { 
    }

    CMILMatrix &operator = (__in_ecount(1) const dxlayer::basetypes<dxlayer::dx_apiset>::matrix_base_t &m)
    {
        CBaseMatrix::operator=(m);
        return *this;
    }

    // We continue to support D3DMATRIX (but not D3DXMATRIX)
    CMILMatrix(__in_ecount(1) const D3DMATRIX &m)
        : CBaseMatrix(m)
    {
    }

    CMILMatrix &operator = (__in_ecount(1) const D3DMATRIX  &m)
    {
        CBaseMatrix::operator=(m);
        return *this;
    }

    CMILMatrix& operator*=(const CMILMatrix& mat)
    {
        *this = *this * mat;
        return *this;
    }

    CMILMatrix& operator*=(FLOAT f)
    {
        *this = *this * f;
        return *this;
    }

#pragma warning (pop)


    void SetToIdentity()
    {
        CBaseMatrix::SetToIdentity();
    }

    void InferAffineMatrix(
        __in_ecount(1) const MilPointAndSizeF& destRect,
        __in_ecount(1) const MilPointAndSizeF& srcRect
        );

    // Future Consideration:   Remove InferAffine with two RB rects from CMILMatrix
    //  These rectangles can and should be space specific; so CMILMatrix should
    //  not exposes this variant.
    void InferAffineMatrix(
        __in_ecount(1) CMilRectF const &destRect,
        __in_ecount(1) CMilRectF const &srcRect
        )
    {
        CBaseMatrix::InferAffineMatrix(srcRect, destRect);
    }

    void Transform2DBounds(
        __in_ecount(1) const MilRectF &srcBounds,
        __out_ecount(1) MilRectF &destBounds
        ) const
    {
        CBaseMatrix::Transform2DBounds(srcBounds, destBounds);
    }

    void Transform2DBoundsConservative(
        __in_ecount(1) const MilRectF &srcBounds,
        __out_ecount(1) MilRectF &destBounds
        ) const
    {
        CBaseMatrix::Transform2DBoundsConservative(srcBounds, destBounds);
    }

    static void Transform2DBoundsNullSafe(
        __in_ecount_opt(1) const CMILMatrix *matrix,
        __in_ecount(1) const MilRectF &srcBounds,
        __out_ecount(1) MilRectF &destBounds
        )
    {
        CBaseMatrix::Transform2DBoundsNullSafe(matrix, srcBounds, destBounds);
    }

    BOOL Invert();

    BOOL Invert(
        __in_ecount(1) const CMILMatrix &in
        )
    {
        return CBaseMatrix::Invert(in);
    }

    VOID SetToInverseOfTranslateOrScale(
        __in_ecount(1) const CMILMatrix &in
        )
    {
        CBaseMatrix::SetToInverseOfTranslateOrScale(in);
    }

    VOID Multiply(
        __in_ecount(1) const CMILMatrix& m
        )
    {
        CBaseMatrix::Multiply(m);
    }

    VOID PreMultiply(
        __in_ecount(1) const CMILMatrix& m
        )
    {
        CBaseMatrix::PreMultiply(m);
    }

    VOID SetToMultiplyResult(
        __in_ecount(1) const CMILMatrix &m1,
        __in_ecount(1) const CMILMatrix &m2
        )
    {
        CBaseMatrix::SetToMultiplyResult(m1, m2);
    }


    //+------------------------------------------------------------------------
    //
    //  Member:
    //      ReinterpretBase / ReinterpretBaseForModification
    //
    //  Synopsis:
    //      Promotion casting (down cast) helper.  CBaseMatrix and other
    //      CBaseMatrix based classes can be reinterpreted as CMILMatrix since
    //      all data members are the same.
    //
    //      Reinterpretation for writing shouldn't be regularly needed so that
    //      usage has a qualified name to note the significance.
    //
    //-------------------------------------------------------------------------

    static __returnro const CMILMatrix *ReinterpretBase(
        __in_ecount(1) const CBaseMatrix *pMatrix
        )
    {
        C_ASSERT(sizeof(CMILMatrix) == sizeof(CBaseMatrix));
        return static_cast<const CMILMatrix *>(pMatrix);
    }

    static __ecount(1) CMILMatrix *ReinterpretBaseForModification(
        __in_ecount(1) CBaseMatrix *pMatrix
        )
    {
        C_ASSERT(sizeof(CMILMatrix) == sizeof(CBaseMatrix));
        return static_cast<CMILMatrix *>(pMatrix);
    }

};

extern const CMILMatrix IdentityMatrix;



__out_ecount(1) CMILMatrix *
MILMatrixAdjoint(
    __out_ecount(1) CMILMatrix *pOut,
    __in_ecount(1) const CMILMatrix *pM);


