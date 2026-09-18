// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Declaration of the base matrix class used by the MIL.
//      This class derives from D3DMATRIX, and adds no
//      additional data to the memory footprint.  This is done to 
//      maximize interchangeability between matrix classes and minimize
//      overhead.
//
//------------------------------------------------------------------------

#pragma once

#include "factory.hpp"

// error C4995: 'matrix-type': name was marked as #pragma deprecated
//
// Ignore deprecation of non-CBaseMatrix types in this file because
// CBaseMatrix inherits from them, and provides conversion functionality
#pragma warning (push)
#pragma warning (disable : 4995)

template <typename ResultInCoordSpace, typename ResultOutCoordSpace> class CMatrixBuilder;

//--------------------------------------------------------------------------
// Represents a 2D affine transformation matrix
//--------------------------------------------------------------------------

class CBaseMatrix : public dxlayer::matrix
{
    template <typename, typename> friend class CMatrixBuilder;

    // No extra data members, including vtables, should ever be added to
    // CBaseMatrix
    void _CAssertSize_() 
    {
        static_assert(sizeof(CBaseMatrix) == sizeof(dxlayer::basetypes<dxlayer::dx_apiset>::matrix_base_t), "Size is not correct");
    }

public:

    CBaseMatrix() : dxlayer::matrix() {}

    CBaseMatrix(bool fInitialize)
    {
        if (fInitialize)
            SetToIdentity();
    }

    CBaseMatrix(__in_ecount(1) const CBaseMatrix &m) : dxlayer::matrix(m) { }
    CBaseMatrix(const D3DMATRIX& m) : dxlayer::matrix(m) {}
    CBaseMatrix(const D3DMATRIX*& m) : dxlayer::matrix(*m) {}


protected:
    CBaseMatrix(__in_ecount(1) MilMatrix3x2D const *pMatrix);

    CBaseMatrix(__in_ecount(16) const float * pf) : dxlayer::matrix(pf) { }

 

    CBaseMatrix(float m00, float m01, float m02, float m03,
                float m10, float m11, float m12, float m13,
                float m20, float m21, float m22, float m23,
                float m30, float m31, float m32, float m33)
        : dxlayer::matrix( 
            m00, m01, m02, m03,
            m10, m11, m12, m13,
            m20, m21, m22, m23,
            m30, m31, m32, m33)
    {
    }


public:
    inline static __returnro CBaseMatrix const *ReinterpretBase(
        __in_ecount(1) dxlayer::basetypes<dxlayer::dx_apiset>::matrix_base_t const *pm
        )
    {
        C_ASSERT(sizeof(*pm) == sizeof(CBaseMatrix));
        return static_cast<CBaseMatrix const *>(pm);
    }

// Re-enable deprecation of D3D*MATRIX types.
#pragma warning (pop)

    BOOL IsIdentity() const;

    BOOL IsTranslateOrScale() const;

    BOOL IsPureTranslate() const;

    BOOL IsPure2DScale() const;

    BOOL IsPureNonNegative2DScale() const;

    BOOL Is2DAxisAlignedPreserving() const;

    BOOL Is2DAxisAlignedPreservingNonNegativeScale() const;

    BOOL Is2DAxisAlignedPreservingApproximate() const;

    BOOL Is2DAxisAlignedPreservingOrNaN() const;

    BOOL Is2DAffineOrNaN() const;

    BOOL IsIsotropic() const;

    VOID Transform(
        __in_ecount(count) const MilPoint2F *srcPoints,
        __out_ecount(count) MilPoint2F *destPoints,
        __in_range(1,UINT_MAX) UINT count = 1
        ) const;

    VOID TransformAsVectors(
        __in_ecount(count) const MilPoint2F *srcVectors,
        __out_ecount(count) MilPoint2F *destVectors,
        __in UINT count
        ) const;

    REAL GetDeterminant2D() const
    {
        return (_11*_22 - _12*_21);
    }

    REAL GetDeterminant3D() const
    {
        return this->determinant();
    }

    REAL GetUpper3x3Determinant3D() const
    {
        return   _11 * (_22 * _33 - _23 * _32) 
               - _12 * (_21 * _33 - _23 * _31) 
               + _13 * (_21 * _32 - _22 * _31);
    }

    BOOL CompareWithoutOffset(__in_ecount(1) const CBaseMatrix &in) const;

    VOID Scale(REAL scaleX, REAL scaleY);

    VOID Rotate(REAL angle);

    VOID RotateAt2D(FLOAT angle, FLOAT x, FLOAT y);

    VOID Translate(REAL offsetX, REAL offsetY);

    VOID Shear2D(FLOAT xshear, FLOAT yshear);

    VOID SetToZeroMatrix();

    REAL GetM11() const { return _11; }
    REAL GetM12() const { return _12; }
    REAL GetM21() const { return _21; }
    REAL GetM22() const { return _22; }

    VOID SetM11(REAL r) { _11 = r; }
    VOID SetM12(REAL r) { _12 = r; }
    VOID SetM21(REAL r) { _21 = r; }
    VOID SetM22(REAL r) { _22 = r; }

    REAL GetDx() const { return _41; }
    REAL GetDy() const { return _42; }

    VOID SetDx(REAL dx) { _41 = dx; }
    VOID SetDy(REAL dy) { _42 = dy; }

    void SetTranslation(
        REAL xTranslation41,   
        REAL yTranslation42
        );

    REAL GetMaxFactor() const;
    REAL GetMinFactor() const;

    VOID AdjustForIPC();

    VOID GetScaleDimensions(
        __out_ecount(1) REAL *prScaleX,
        __out_ecount(1) REAL *prScaleY
        ) const;

    VOID DecomposeMatrixIntoScaleAndRest(
        __out_ecount(1) CBaseMatrix *pmatScale,
        __out_ecount(1) CBaseMatrix *pmatRest,
        __out_ecount(1) BOOL *pfCanDecomposeMatrix) const;
    
    VOID SetToMultiplyResult(
        __in_ecount(1) const CBaseMatrix &m1,
        __in_ecount(1) const CBaseMatrix &m2
        );
#ifdef DBG
    VOID Dump() const;
#endif

protected:

    void SetToIdentity();

    // Transform 2D rectangle bounds using the matrix v' = v M:
    //
    //                                  ( M11 M12 0 )
    //      (vx', vy', 1) = (vx, vy, 1) ( M21 M22 0 )
    //                                  ( dx  dy  1 )
    //
    // for each corner and produce a bounding rectangle for those results.
    //
    // Since Transform2DBounds works by transforming each corner individually
    // it expects that incoming bounds fall within reasonable floating point
    // limits.  For example a X,Y Width,Height based rectange should not allow
    // X+Width or Y+Height to exceed FLT_MAX.

    void Transform2DBounds(
        __in_ecount(1) const MilRectF &srcBounds,
        __out_ecount(1) MilRectF &destBounds
        ) const;

    void Transform2DBoundsConservative(
        __in_ecount(1) const MilRectF &srcBounds,
        __out_ecount(1) MilRectF &destBounds
        ) const;

    static void Transform2DBoundsNullSafe(
        __in_ecount_opt(1) const CBaseMatrix *matrix,
        __in_ecount(1) const MilRectF &srcBounds,
        __out_ecount(1) MilRectF &destBounds
        )
    {
        if (matrix)
        {
            matrix->Transform2DBounds(srcBounds,destBounds);
        }
        else
        {
            destBounds = srcBounds;
        }
    }

    BOOL Invert(
        __in_ecount(1) const CBaseMatrix &in
        );

    VOID SetToInverseOfTranslateOrScale(
        __in_ecount(1) const CBaseMatrix &in
        );

    //
    // Not for use by CMatrix
    //

    VOID Multiply(
        __in_ecount(1) const CBaseMatrix& m
        );

    VOID PreMultiply(
        __in_ecount(1) const CBaseMatrix& m
        );

    VOID ComputePrefilteringDimensions(
        __range(>=, 1) UINT uRealizationWidth,
        __range(>=, 1) UINT uRealizationHeight,
        REAL rShrinkFactorThreshold,
        __deref_out_range(1, UINT_MAX) UINT &uDesiredWidth,
        __deref_out_range(1, UINT_MAX) UINT &uDesiredHeight
        ) const;

    VOID AdjustForPrefiltering(
        __range(>=, 1) UINT uRealizationWidth,
        __range(>=, 1) UINT uRealizationHeight,
        REAL rShrinkFactorThreshold,
        __deref_out_range(>=, 1) UINT *puDesiredWidth,
        __deref_out_range(>=, 1) UINT *puDesiredHeight
        );

    //
    // Not for use by CMILMatrix
    //

    void InferAffineMatrix(
        __in_ecount(1) const CMilRectF &rcInSpace,
        __in_ecount(1) const CMilRectF &rcOutSpace
        );

    void InferAffineMatrix(
        __in_ecount(1) const CMilRectF &rcInSpace,
        __in_ecount(3) const MilPoint2F *rgptOutSpace
        );

private:
    enum
    {
        DoCheckForNaN,
        DontCheckForNaN
    };

    template<int CheckForNaNBehavior>
    MIL_FORCEINLINE
    void Transform2DBoundsHelper(
        __in_ecount(1) const CMilRectF &srcRect,
        __out_ecount(1) MilRectF &destRect) const;
};



extern "C" {

VOID WINAPI MatrixAppendRotateAt2D(
    __inout_ecount(1) CBaseMatrix *pmat,
    FLOAT angle,
    FLOAT x,
    FLOAT y
    );

VOID WINAPI MatrixAppendTranslate2D(
    __inout_ecount(1) CBaseMatrix *pmat,
    FLOAT dx,
    FLOAT dy
    );

VOID WINAPI MatrixPrependTranslate2D(
    __inout_ecount(1) CBaseMatrix *pmat,
    FLOAT dx,
    FLOAT dy
    );

VOID WINAPI MatrixAppendRotate2D(
    __inout_ecount(1) CBaseMatrix *pmat,
    FLOAT angle
    );

VOID WINAPI MatrixAppendScale2D(
    __inout_ecount(1) CBaseMatrix *pmat,
    FLOAT sx,
    FLOAT sy
    );
}


