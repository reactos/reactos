// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------
//

//
//  Description:
//      Implementation of the base matrix class used by the MIL.
//      This class derives from D3DMATRIX, and adds no
//      additional data to the memory footprint.  This is done to 
//      maximize interchangeability between matrix classes and minimize
//      overhead.
//

#include "precomp.hpp"
#include <math.h>

using namespace dxlayer;

MtExtern(matrix_t_get_scaling);


CBaseMatrix::CBaseMatrix(__in_ecount(1) MilMatrix3x2D const *pMatrix)
{
    if (pMatrix != NULL)
    {
        this->_11 = (FLOAT) pMatrix->S_11;
        this->_12 = (FLOAT) pMatrix->S_12;
        this->_13 = 0.0f;
        this->_14 = 0.0f;
        this->_21 = (FLOAT) pMatrix->S_21;
        this->_22 = (FLOAT) pMatrix->S_22;
        this->_23 = 0.0f;
        this->_24 = 0.0f;
        this->_31 = 0.0f;
        this->_32 = 0.0f;
        this->_33 = 1.0f;
        this->_34 = 0.0f;
        this->_41 = (FLOAT) pMatrix->DX;
        this->_42 = (FLOAT) pMatrix->DY;
        this->_43 = 0.0f;
        this->_44 = 1.0f;
    }
    else
    {
        SetToIdentity();
    }
}

void CBaseMatrix::SetToIdentity()
{
    reset_to_identity();
}

BOOL CBaseMatrix::IsIdentity() const
{
    return is_identity() ? TRUE : FALSE;
}

/**************************************************************************\
*
* Function Description:
*
*   Is the current transform a scale and/or translation?
*
\**************************************************************************/
BOOL CBaseMatrix::IsTranslateOrScale() const
{
    return (            _12 == 0 && _13 == 0 && _14 == 0 &&
            _21 == 0 &&             _23 == 0 && _24 == 0 &&
            _31 == 0 && _32 == 0 &&             _34 == 0);
}

/**************************************************************************\
*
* Function Description:
*
*   Is the current transform only a translation?
*
\**************************************************************************/
BOOL CBaseMatrix::IsPureTranslate() const
{
    // if the upper 3x4 is identity we are a simple translate
    return ((_11 == 1.0 && _12 == 0   && _13 == 0   && _14 == 0) &&
            (_21 == 0   && _22 == 1.0 && _23 == 0   && _24 == 0) &&
            (_31 == 0   && _32 == 0   && _33 == 1.0 && _34 == 0));
}

/**************************************************************************\
*
* Function Description:
*
*   Is the current transform only a 2D scale matrix?
*
\**************************************************************************/
BOOL CBaseMatrix::IsPure2DScale() const
{
    return ((              _12 == 0   && _13 == 0   && _14 == 0) &&
            (_21 == 0   &&               _23 == 0   && _24 == 0) &&
            (_31 == 0   && _32 == 0   && _33 == 1.0 && _34 == 0) &&
            (_41 == 0   && _42 == 0   && _43 == 0   && _44 == 1.0));
}

/**************************************************************************\
*
* Function Description:
*
*   Is the current transform only a 2D scale matrix with positive scale
*   factors?
*
\**************************************************************************/
BOOL CBaseMatrix::IsPureNonNegative2DScale() const
{
    return IsPure2DScale() && !(_11 < 0) && !(_22 < 0);
}

/**************************************************************************\
*
* Function Description:
*
*   Does the transformation preserve circles?
*
\**************************************************************************/
BOOL CBaseMatrix::IsIsotropic() const
{
    return IsCloseReal(_11, _22) && IsCloseReal(_12, -_21);
}


/**************************************************************************\
*
* Function Description:
*
*   Does the current transform preserve the axis aligned property of the
*   lines it transforms?
*
*   Note: This only works with 2D vectors and 2D transformation matrices.
*
\**************************************************************************/
BOOL CBaseMatrix::Is2DAxisAlignedPreserving() const
{
    return ((_12 == 0 && _21 == 0) ||
            (_11 == 0 && _22 == 0));
}

/**************************************************************************\
*
* Function Description:
*
*   Does the current transform preserve the axis aligned property of the
*   lines it transforms, and is the scale on both axes non-negative?
*
\**************************************************************************/
BOOL CBaseMatrix::Is2DAxisAlignedPreservingNonNegativeScale() const
{
    return Is2DAxisAlignedPreserving() && !(_11 < 0) && !(_22 < 0);
}

/**************************************************************************\
*
* Function Description:
*
*   Does the current transform preserve the axis aligned property of the
*   lines it transforms, and is the scale on both axes non-negative?
*   This function does fuzzy comparisons to determine the axis aligned properties
*   are true.
*
\**************************************************************************/
BOOL CBaseMatrix::Is2DAxisAlignedPreservingApproximate() const
{
    return  ((IsCloseReal(_12, 0) && IsCloseReal(_21, 0)) ||
             (IsCloseReal(_11, 0) && IsCloseReal(_22, 0)));
}


/**************************************************************************\
*
* Function Description:
*
*   Does the current transform preserve the axis aligned property of the
*   lines it transforms... or is the matrix NaN?
*
*   Note: This only works with 2D vectors and 2D transformation matrices.
*
\**************************************************************************/
BOOL CBaseMatrix::Is2DAxisAlignedPreservingOrNaN() const
{
    return (   (   IsNaNOrIsEqualTo(_12, 0.0f)
                && IsNaNOrIsEqualTo(_21, 0.0f)
               )
            || (   IsNaNOrIsEqualTo(_11, 0.0f)
                && IsNaNOrIsEqualTo(_22, 0.0f)
               )
           );
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CBaseMatrix::Is2DAffineOrNaN
//
//  Synopsis:  
//      Returns true if the matrix is a 2D affine matrix or if it is NaN.
//

BOOL CBaseMatrix::Is2DAffineOrNaN() const
{
    //    A11 A12    0    0
    //    A21 A22    0    0
    //    0     0    1    0
    //    A41 A42    0    1


    return (        // row 1
               IsNaNOrIsEqualTo(_13, 0.0f)
            && IsNaNOrIsEqualTo(_14, 0.0f)
                    // row 2
            && IsNaNOrIsEqualTo(_23, 0.0f)
            && IsNaNOrIsEqualTo(_24, 0.0f)
                    // row 3
            && IsNaNOrIsEqualTo(_31, 0.0f)
            && IsNaNOrIsEqualTo(_32, 0.0f)
            && IsNaNOrIsEqualTo(_33, 1.0f)
            && IsNaNOrIsEqualTo(_34, 0.0f)
                    // row 4
            && IsNaNOrIsEqualTo(_43, 0.0f)
            && IsNaNOrIsEqualTo(_44, 1.0f)
           );
}

//+----------------------------------------------------------------------------
//
//  Member:  
//      CBaseMatrix::Transform
//
//  Synopsis:  
//      Transform the specified array of points using the current matrix.  
//
//  Notes:
//      The function performs the following computation
//
//      p' = p M:
//                                  ( M11 M12 0 )
//      (px', py', 1) = (px, py, 1) ( M21 M22 0 )
//                                  ( dx  dy  1 )
//
//-----------------------------------------------------------------------------
VOID CBaseMatrix::Transform(
    __in_ecount(count) const MilPoint2F *srcPoints,
        // Array of source points to transform
    __out_ecount(count) MilPoint2F *destPoints,
        // Destination to place transformed points.  
        // Can be the same array as srcPoints BUT
        // otherwise it cannot overlap srcPoints
    __in_range(1,UINT_MAX) UINT count
        // Number of points to transform
    ) const
{
    // make sure src and dest don't overlap unless they are equal
    Assert(
        ( srcPoints + count <= destPoints) || // S_end <= D_begin
        (destPoints + count <= srcPoints)  || // D_end <= S_begin
        srcPoints == destPoints
        );

    while(count != 0)
    {
        // Copy vectors compontents into local variables to allow
        // srcPoints to be the same array as destPoints        
        REAL x = srcPoints->X;
        REAL y = srcPoints->Y;

        destPoints->X = (_11 * x) + (_21 * y) + _41;
        destPoints->Y = (_12 * x) + (_22 * y) + _42;

        destPoints++;
        srcPoints++;
        count--;
    }
}

//+------------------------------------------------------------------------
//
//  Member:  
//      CBaseMatrix::TransformAsVectors
//
//  Synopsis:  
//      Transform the specified array of vectors using the current matrix.  
//
//  Notes:
//      The function performs the following computation
//      
//      v' = v M:
//                                  ( M11 M12 0 )
//      (vx', vy', 0) = (vx, vy, 0) ( M21 M22 0 )
//                                  ( dx  dy  1 )
//
//      Because vectors have a 0 in their W coordinate, the translation
//      portion of the matrix is not applied.    
//
//-------------------------------------------------------------------------  
VOID 
CBaseMatrix::TransformAsVectors(
    __in_ecount(count) const MilPoint2F *srcVectors,
        // Array of source vectors to transform
    __out_ecount(count) MilPoint2F *destVectors,
        // Destination to place transformed vectors.
        // Can be the same array as destVectors.
    __in UINT count
        // Number of vectors to transform
    ) const
{
    // Source & destination must be non-NULL, unless the count is 0
    Assert ((srcVectors && destVectors) || 
            (count == 0));

    REAL x, y;
    
    for (UINT i = 0; i < count; i++)
    {
        // Copy vectors compontents into local variables to allow
        // srcVectors to be the same array as destVectors
        x = srcVectors->X;
        y = srcVectors->Y;
        
        destVectors->X = (_11 * x) + (_21 * y);
        destVectors->Y = (_12 * x) + (_22 * y);        
        
        destVectors++;
        srcVectors++;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CBaseMatrix::Transform2DBoundsHelper
//
//  Synopsis:
//      Convert a bounding rectangle from one coordinate space to another
//      always expanding bounds to ensure any point within source bounds
//      transformed to destination space will fall within output destination
//      bounds.
//
//      This routine works by first converting the corners of the given source
//      rectangle into destination space.  Then with the transformed top-left
//      corner as a starting point each transformed point is accumulated to the
//      destination bounds.
//
//  Notes:
//      This function is a helper function for Transform2DBoundsConservative
//      and Transform2DBounds. Do not call it directly.
//
//      If a NaN is encountered at any time in the calculation, the result may
//      or may not be well-ordered. Furthermore, a well-ordered result does not
//      imply a correct result (NaNs may be present in the result, but only if
//      the top-left corner produces them). To ensure correctness,
//      Transform2DBoundsConservative passes in DoCheckForNaN, which checks for
//      NaNs and sets the destRect to infinity if any exist. For maximum speed,
//      Transform2DBounds passes in DontCheckForNaN, which ignores NaNs.
//
//-----------------------------------------------------------------------------
template<int checkForNaNBehavior>
MIL_FORCEINLINE
void CBaseMatrix::Transform2DBoundsHelper(
    __in_ecount(1) const CMilRectF &srcRect,
    __out_ecount(1) MilRectF &destRect) const
{
    MilPoint2F pt[4];
    pt[0].X = srcRect.left;
    pt[0].Y = srcRect.top;
    pt[1].X = srcRect.right;
    pt[1].Y = srcRect.top;
    pt[2].X = srcRect.left;
    pt[2].Y = srcRect.bottom;
    pt[3].X = srcRect.right;
    pt[3].Y = srcRect.bottom;

    Transform(&pt[0], &pt[0], 4);

    MilPoint2F ptBottomRight = pt[0], ptTopLeft = pt[0];

    //
    // suppress warning "conditional expression is constant".
    // Since we are using template expansion, it is expected that we will
    // condition based on constants. The optimizer will remove the
    // conditional.
    //
    #pragma warning (suppress : 4127)
    if (checkForNaNBehavior == DoCheckForNaN)
    {
        for (int i=0; i<4; i++)
        {
            if (GpIsNaNF(pt[i].X) || GpIsNaNF(pt[i].Y))
            {
                CMilRectF::ReinterpretBaseType(&destRect)->SetInfinite();
                return;
            }
        }
    }

    for (int i=1; i<4; i++)
    {
        if (pt[i].X < ptTopLeft.X)
        {
            ptTopLeft.X = pt[i].X;
        }
        if (pt[i].Y < ptTopLeft.Y)
        {
            ptTopLeft.Y = pt[i].Y;
        }
        if (pt[i].X > ptBottomRight.X)
        {
            ptBottomRight.X = pt[i].X;
        }
        if (pt[i].Y > ptBottomRight.Y)
        {
            ptBottomRight.Y = pt[i].Y;
        }
    }

    destRect.left = ptTopLeft.X;
    destRect.top = ptTopLeft.Y;
    destRect.right = ptBottomRight.X;
    destRect.bottom = ptBottomRight.Y;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CBaseMatrix::Transform2DBounds
//
//  Synopsis:
//      Convert a bounding rectangle from one coordinate space to another
//      always expanding bounds to ensure any point within source bounds
//      transformed to destination space will fall within output destination
//      bounds.
//
//  Notes:
//      This procedure does not handle NaN gracefully. See
//      Transform2DBoundsHelper for details. If NaN-safety is desired, use
//      Transform2DBoundsConservative
//
//-----------------------------------------------------------------------------

void CBaseMatrix::Transform2DBounds(
    __in_ecount(1) const MilRectF &srcRect,
    __out_ecount(1) MilRectF &destRect
    ) const
{
    Transform2DBoundsHelper<DontCheckForNaN>(srcRect, destRect);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CBaseMatrix::Transform2DBoundsConservative
//
//  Synopsis:
//      Finds a bounding rectangle for the transformed points in an input
//      bounding rectangle.  This routine is conservative in that it ALWAYS
//      returns a valid rectangle.  In the event of any numerical failure it
//      returns an infinity rect.
//

void CBaseMatrix::Transform2DBoundsConservative(
    __in_ecount(1) const MilRectF &srcRect,
    __out_ecount(1) MilRectF &destRect
    ) const
{
    Transform2DBoundsHelper<DoCheckForNaN>(srcRect, destRect);
}

BOOL CBaseMatrix::Invert(
    __in_ecount(1) const CBaseMatrix &in
    )
{
    bool success = false;
    try
    {
        this->set(in.inverse());
        success = true;
    }
    catch (const dxlayer_exception&)
    {
        // do nothing
    }

    return success ? TRUE : FALSE;
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CBaseMatrix::SetToInverseOfTranslateOrScale
//
//  Synopsis:  
//      Specialized inverse for simple translate/scale matrix.
//      Advantages: faster than normal Inverse operation.
//                  doesn't degenerate into NaN as easily.
//

VOID
CBaseMatrix::SetToInverseOfTranslateOrScale(
    __in_ecount(1) const CBaseMatrix &in
    )
{
    Assert(in.IsTranslateOrScale());
    
    // y = m*x + b
    // x = (1/m)*y - (b/m)

    // Setting Dx & Dy before M11 & M22 allows *this
    // to be the param.
    SetDx(-in.GetDx() / in.GetM11());
    SetDy(-in.GetDy() / in.GetM22());
    SetM11(1.0f / in.GetM11());
    SetM22(1.0f / in.GetM22());
    
    // Set the rest of the matrix to Identity
                _12 = 0.0f; _13 = 0.0f; _14 = 0.0f;
    _21 = 0.0f;             _23 = 0.0f; _24 = 0.0f;
    _31 = 0.0f; _32 = 0.0f; _33 = 1.0f; _34 = 0.0f;
                            _43 = 0.0f; _44 = 1.0f;
}

VOID CBaseMatrix::Scale(REAL scaleX, REAL scaleY)
{
    MtSetDefault(Mt(matrix_t_get_scaling));
    MatrixAppendScale2D(this, scaleX, scaleY);
}

VOID CBaseMatrix::Rotate(REAL angle)
{
    MatrixAppendRotate2D(this, angle);
}

VOID CBaseMatrix::RotateAt2D(FLOAT angle, FLOAT x, FLOAT y)
{
    MatrixAppendRotateAt2D(this, angle, x, y);
}

VOID CBaseMatrix::Translate(REAL offsetX, REAL offsetY)
{
    MatrixAppendTranslate2D(this, offsetX, offsetY);
}

VOID CBaseMatrix::Shear2D(FLOAT xshear, FLOAT yshear)
{
    float t = 0.0f;

    t = _11;
    _11 += yshear * _21;
    _21 += xshear * t;

    t = _12;
    _12 += yshear * _22;
    _22 += xshear * t;
}

VOID CBaseMatrix::SetToMultiplyResult(
    __in_ecount(1) const CBaseMatrix &m1,
    __in_ecount(1) const CBaseMatrix &m2
    )
{
    this->set(m1 * m2);
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      SetToZeroMatrix
//
//  Synopsis:  
//      Sets matrix to a transformation that transforms all points to (0, 0)
//

void
CBaseMatrix::SetToZeroMatrix()
{
    ZeroMemory(this, sizeof(CBaseMatrix));  
    _44 = 1.0f;
}

//+-----------------------------------------------------------------------
//
//  Member:    
//      CBaseMatrix::SetTranslation
//
//  Synopsis:  
//      Overwrites the x & y translation components of the matrix (_41
//      and _42) with the passed-in translation components.  
//
//------------------------------------------------------------------------
void
CBaseMatrix::SetTranslation(
    REAL xTranslation41,   
        // x translation to overwrite _41 with
    REAL yTranslation42   
        // y translation to overwrite _42 with
        )
{
    _41 = xTranslation41;
    _42 = yTranslation42;
}
//+-----------------------------------------------------------------------
//
//  Member:    CBaseMatrix::GetMaxFactor
//
//  Synopsis:  Get maximum of |Transformed V| / |V| for this matrix
//
//  Return:    The maximum
//
//  Notes: When a vector (x,y) is transformed with the matrix M then the
//  square of its length is:
//                               (x,y) /M11 M12\ /M11 M21\ /x\
//                                     \M21 M22/ \M12 M22/ \y/
//  This is a quadratic form:
//                           f(x, y) = (x, y)/a b\ /x\
//                                           \b c/ \y/
//  where:
//         a = M11*M11 + M12*M12,
//         b = M11*M21 + M12*M22,
//         c = M21*M21 + M22*M22,
//
//  The min and max of its values on a unit vector are eigenvalues
//  of  the matrix /a, b\.
//                 \b, c/
//
//  Eigenvalues are the roots of the characteristic equation:
//
//               |a-x  b | = 0,
//               | b  c-x|
//
//  where | | stands for the determinant. The equation is:
//
//               (a-x)(c-x) - b^2 = 0
//  or
//               x^2 - (a+c)x + ac - b^2 = 0.
//                         _____________________                  ______________
//  The roots are (a+c +-\/(a+c)^2 - 4(ac - b^2) ) / 2 = (a+c +-\/(a-c)^2 + 4b^2 ) / 2
//
//  The maximal factor is the one with greater absolute value
//
//------------------------------------------------------------------------------
REAL
CBaseMatrix::GetMaxFactor() const
{
    REAL r;

    if (IsTranslateOrScale())
    {
        // The min and max scale factors are _11 and _22.
        REAL s = (REAL)fabs(_11);
        r = (REAL)fabs(_22);
        if (s > r)
        {
            r = s;
        }
    }
    else
    {
        REAL a = _11 * _11 + _12 * _12;
        REAL b = _11 * _21 + _12 * _22;
        REAL c = _21 * _21 + _22 * _22;

        r = a - c;
        r = sqrtf(r * r + 4 * b * b);

        // The roots are eigenvalues of a positive semi-definite quadratic form, so they
        // are non-negative.  They are a + c +- r.  For best accuracy we want to get the
        // larger one first. Both a and c are sums of squares, so a + c > 0. For the larger
        // root we therefore need to choose +:

        // Ignore NaNs
        Assert (!(r < 0));
        r = sqrtf((a + c + r) * .5f);
   }

    return r;
}

//+-----------------------------------------------------------------------
//
//  Member:    CBaseMatrix::GetMinFactor
//
//  Synopsis:  Get minimum of |Transformed V| / |V| for this matrix
//
//  Return:    Minimum of |Transformed V| / |V| for this matrix
//
//  See GetMaxFactor for notes.
//
//------------------------------------------------------------------------------
REAL
CBaseMatrix::GetMinFactor() const
{
    REAL r;

    if (IsTranslateOrScale())
    {
        // The min and max scale factors are _11 and _22.
        REAL s = (REAL)fabs(_11);
        r = (REAL)fabs(_22);
        if (s < r)
        {
            r = s;
        }
    }
    else
    {
        REAL a = _11 * _11 + _12 * _12;
        REAL b = _11 * _21 + _12 * _22;
        REAL c = _21 * _21 + _22 * _22;

        r = a - c;
        r = sqrtf(r * r + 4 * b * b);

        // The roots are eigenvalues of a positive semi-definite quadratic form, so they
        // are non-negative.  They are a + c +- r.  For best accuracy we want to get the
        // larger one first. Both a and c are sums of squares, so a + c > 0. For the larger
        // root we therefore need to choose +:

        // To find the smaller eigenvalue we use r1 * r2 = ac - b^2
        // (The product of the eigenvalues is the determinant of the 2x2 matrix.)

        // Ignore NaNs
        Assert (!(r < 0));
        r = sqrtf((a + c + r) * .5f);

        // That's the larger eigenvalue.  Here we find the smaller
        r = (a * c - b * b) / r;
   }

    return r;
}


//+-----------------------------------------------------------------------------
//
//  Member:    GetScaleDimensions
//
//  Synopsis:  Extracts the absolute scale factors, ignoring translation,
//             rotation, flipping, skewing. This is useful in prefiltering an
//             image for display. (The reconstruction filter will handle
//             translation, rotation, flipping, and skewing).
//
//------------------------------------------------------------------------------

VOID
CBaseMatrix::GetScaleDimensions(
    __out_ecount(1) REAL *prScaleX,
    __out_ecount(1) REAL *prScaleY
    ) const
{
    Assert(prScaleX);
    Assert(prScaleY);

    // rScaleX is the length of the transform of the vector (1,0).
    // rScaleY is the length of the transform of the vector (0,1).

    REAL rScaleX = sqrtf(_11 * _11 + _12 * _12);
    REAL rScaleY = sqrtf(_21 * _21 + _22 * _22);


    // Convert NaN to zero.
    if (!(rScaleX == rScaleX))
    {
        TraceTag((tagMILWarning, "rScaleX is NaN"));
        rScaleX = 0;
    }
    if (!(rScaleY == rScaleY))
    {
        TraceTag((tagMILWarning, "rScaleY is NaN"));
        rScaleY = 0;
    }


    // Postconditions

    Assert(rScaleX >= 0);
    Assert(rScaleY >= 0);

    *prScaleX = rScaleX;
    *prScaleY = rScaleY;
}



//+---------------------------------------------------------------------------
//
//  Member:     CBaseMatrix::DecomposeMatrixIntoScaleAndRest
//
//  Synopsis:   Calculates seperate transforms for the scale and non-scale
//              portions of this transform.
//
//----------------------------------------------------------------------------
VOID
CBaseMatrix::DecomposeMatrixIntoScaleAndRest(
    __out_ecount(1) CBaseMatrix *pmatScale,
    __out_ecount(1) CBaseMatrix *pmatRest,
    __out_ecount(1) BOOL *pfCanDecomposeMatrix
    ) const
{
    REAL rScaleX, rScaleY;

    //
    // Calculate the scale portion of the source matrix
    //

    // Determine scale factors of source matrix
    GetScaleDimensions(&rScaleX, &rScaleY);

    // Set scale factors of the scale matrix
    pmatScale->SetToIdentity();
    pmatScale->Scale(rScaleX, rScaleY);

    //
    // Calculate the source matrix with the scale portion removed
    //

    // Set the 'rest' matrix to the inverse scale matrix
    *pmatRest = *pmatScale;

    *pfCanDecomposeMatrix = pmatRest->Invert(*pmatRest);

    // Apply the source matrix to the inverse scale matrix
    if (*pfCanDecomposeMatrix)
    {
        pmatRest->Multiply(*this);
    }
}


//+---------------------------------------------------------------------------
//
//  Member:     CBaseMatrix::AdjustForIPC
//
//  Synopsis:
//      Change the matrix to take into account 1/2 pixel offsets
//      from pixel left-top corner to pixel center.
//
//      Given matrix assumed to be 2-dimensional, i.e. inteded for
//      applying to 2d points using Transform().
//      The effect of applying the adjusted matrix to point (x,y)
//      is equivalent to:
//      1) x += 0.5, y += 0.5
//      2) apply original matrix to (x,y), get (u,v)
//      3) u -= 0.5, v -= 0.5
// 
//----------------------------------------------------------------------------
VOID
CBaseMatrix::AdjustForIPC()
{
    // pre-transform: shift by (1/2, 1/2)
    _41 += (_11 + _21)*0.5f;
    _42 += (_12 + _22)*0.5f;

    // post-transform: shift by (-1/2, -1/2)
    _41 -= 0.5f;
    _42 -= 0.5f;
}

//+----------------------------------------------------------------------------
//
//  Function:  ComputePrefilteredSize
//
//  Synopsis:  Helper routine for ComputePrefilteringDimensions that handles
//             computations for a single dimension
//
//  Notes:     Given a constant source size (S), every scale factor will be
//             bucketed to a threshold interval, which yields a consistent
//             prefiltered size result.  The following table shows the mapping
//             from threshold interval to prefiltered size:
//
//              T = rScaleThreshold = 1/rShrinkFactorThreshold, T <= 1
//                (computed by ComputePrefilteringDimensions)
//              N is an integer
//              S = original size
//
//              Note: ^ is used to indicate an exponent, i.e. 2^3 = 2*2*2 = 8.
//
//              Threshold Interval   Result Prefiltered Size
//              ------------------   --------------------------
//               (1  , Infinity]      S (*)
//               (T^1, 1       ]      ceiling( S * T^0 ) = S (*)
//               (T^2, T^1     ]      ceiling( S * T^1 )
//                 ...                  ...
//               (T^N, T^(N-1) ]      ceiling( S * T^(N-1) )
//               [0  , T^N     ]      1
//
//             * = a prefilter result of S means there is no real prefiltering.
//
//             Ceiling is used in calculing the prefiltered result to keep
//             reconstruction process scaling down.
//
//             N is limited by scale factor at which the prefiltered size
//             becomes 1 or less.  (Prefiltered size is always rounded up to be
//             at least 1.)  The limiting scale factor,
//             T^N, may then be directly calculated by solving:
//
//              ceiling( S * T^N ) <= 1
//
//             This is solved as:
//
//              S * T^N <= 1
//              T^N <= 1/S
//
//             Therefore the interesting interval, being careful that the lower
//             end of the interval is less than or equal to the higher end, for
//             scale factors is ( min( 1/S,T ) , T].
//

VOID
ComputePrefilteredSize(
    __range(1, UINT_MAX) UINT uOriginal,
    REAL rScaleThreshold,
    REAL rScale,
    __deref_out_range(1, UINT_MAX) UINT &uPrefiltered
    )
{
    Assert(uOriginal > 0);
    Assert(rScaleThreshold <= 1.0f);    // Failure is handled with log check
    Assert(rScale >= 0);
    Assert(rScale <= rScaleThreshold);

    //
    // Check for lowest scale case such that result is 1
    //  rScale <= 1/uOriginal => rScale * uOriginal <= 1
    //

    if (rScale * uOriginal <= 1.0f)
    {
        uPrefiltered = 1;
    }
    else
    {
#if NEVER
        //
        // Math Performance Note:
        //
        // If using log to compute the interval and then pow to get the
        // prefiltered number is too costly it maybe be possible to increase
        // performance by iterating through the first couple intervals with
        // just multiplications as shown here.  For further intervals we would
        // need to fallback to the log/pow code to avoid infinite looping when
        // the next interval limit doesn't change and/or getting to that point
        // takes many, many iterations.
        //

        // rShrinkFactorThreshold would need to be passed in.
        Assert(rShrinkFactorThreshold > 1.0f);

        REAL rIntervalThreshold = rScaleThreshold;

        //
        // Increase rScale by 1/rScaleThreshold so that when rScale is no
        // longer less than rIntervalThreshold, the original rScale value is
        // between rIntervalThreshold*rScaleThreshold and rIntervalThreshold.
        // rIntervalThreshold is then the upper limit of the proper threshold
        // interval and can be used directly to compute the prefilter size.
        //

        rScale *= rShrinkFactorThreshold;

        //
        // Search for threshold interval
        //

        while (rScale < rIntervalThreshold)
        {
            // Compute next interval limit
            rIntervalThreshold *= rScaleThreshold;
        }

        //
        // Compute prefiltered result
        //

        uPrefiltered = CFloatFPU::Ceiling(uOriginal * rIntervalThreshold);
#endif

        //
        // Compute integer interval R, where T^(R+1) < rScale <= T^R
        //
        //  R = floor( log(rScale) / log(T) )
        //
        // Then find integer prefilter size from S*(T^R).  The ceiling of
        // S*(T^R) is used to keep reconstruction process scaling down.  If we
        // were to prefilter down and then scale up there would be some slight
        // color bleeding.
        //

        REAL rLogThreshold = logf(rScaleThreshold);
        REAL rExp;

        //
        // If rScaleThreshold >= 1.0 then we have infinite buckets and exact
        // prefiltering.  log(x) >= 0.0, when x >= 1.0.
        //
        // Additionally, if rScaleThreshold is close to 1.0 (rLogThreshold is
        // very small) and rScale is much smaller such that there are very many
        // buckets to get to rScale then use exact prefiltering.
        //
        // Finally, there can never be more useful intervals than there are
        // samples in the original size so just use the exact computation in
        // that case too.  Note there may be a better limit than the original
        // size.
        //

        if (   (rLogThreshold >= 0.0f)
            || (!_finite(rExp = (logf(rScale) / rLogThreshold)))
            || (rExp >= static_cast<REAL>(uOriginal)))
        {
            // Exact prefiltering to scale with round up as described above
            uPrefiltered = CFloatFPU::Ceiling(uOriginal * rScale);
        }
        else
        {
            INT nExp = CFloatFPU::Floor(rExp);

            Assert(nExp > 0);

#if defined(pow)
#pragma push_macro("pow")
#undef pow
#define RESTORE_POW
#endif 
            // The result of std::pow should be safe to static_cast to REAL since 
            // rScaleThreshold <= 1 (this is assert'ed early in the method).
            //
            //   Assert(rScaleThreshold <= 1.0f);    // Failure is handled with log check

            uPrefiltered = CFloatFPU::Ceiling(uOriginal * TOREAL(std::pow(rScaleThreshold, nExp)));
#if defined(RESTORE_POW)
#pragma pop_macro("pow")
#endif 
        }

        // Results that go to 1 should have been handled earlier
        Assert(uPrefiltered > 1);

        // This might be possible due to conversion of a very large uOriginal
        // to a single precision float.
        if (uPrefiltered > uOriginal)
        {
            uPrefiltered = uOriginal;
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Function:  ComputePrefilteringDimensions
//
//  Synopsis:  Given a bitmap of a certain size, calculate the required
//             intermediate size to which the bitmap should be prefiltered.
//

VOID
CBaseMatrix::ComputePrefilteringDimensions(
    __range(>=, 1) UINT uBitmapWidth,
    __range(>=, 1) UINT uBitmapHeight,
    REAL rShrinkFactorThreshold,
    __deref_out_range(>=, 1) UINT &uDesiredWidth,
    __deref_out_range(>=, 1) UINT &uDesiredHeight
    ) const
{
    Assert(uBitmapWidth > 0);
    Assert(uBitmapHeight > 0);
    Assert(rShrinkFactorThreshold >= 1.0f);

    UINT uScaledWidth = uBitmapWidth;
    UINT uScaledHeight = uBitmapHeight;

    //
    // We alter a dimension if the corresponding shrink factor is above the
    // following threshold.
    //

    if (rShrinkFactorThreshold > 0.0f)
    {
        //   Determine appropriate prefiltering bias
        //  Original prefilter code used a bias equal to the shrink factor
        //  threshold, which defaulted to sqrt(2).  This was based on bilinear
        //  reconstruction being 2x2, but never really tested.  When
        //  prefiltering was changed to have buckets/intervals the bias was
        //  left out and results of minimal testing looked good.  Testing needs
        //  to be done to see what is right for rotation, sub-pixel
        //  translation, etc.  It may be that the bias should be computed from
        //  those factors rather than just being a constant.

        // More bias increases the prefilter size and makes the reconstruction
        // filter handle more of the shrink.  Bias should probably be >= 1.
        const REAL rScaleBias = 1.0f;

        const REAL rScaleThreshold = 1.0f/rShrinkFactorThreshold;

        REAL rScaleX, rScaleY;
        GetScaleDimensions(&rScaleX, &rScaleY);

        Assert(rScaleX >= 0);
        Assert(rScaleY >= 0);

        if (rScaleX <= rScaleThreshold)
        {
            ComputePrefilteredSize(uBitmapWidth,
                                   rScaleThreshold,
                                   rScaleX * rScaleBias,
                                   uScaledWidth
                                   );
        }

        if (rScaleY <= rScaleThreshold)
        {
            ComputePrefilteredSize(uBitmapHeight,
                                   rScaleThreshold,
                                   rScaleY * rScaleBias,
                                   uScaledHeight
                                   );
        }
    }

    uDesiredWidth = uScaledWidth;
    uDesiredHeight = uScaledHeight;
}

//+-----------------------------------------------------------------------------
//
//  Function:  AdjustForPrefiltering
//
//  Synopsis:  Given a bitmap of a certain size, calculate the required
//             intermediate size to which the bitmap should be prefiltered;
//             remove the corresponding scale factor from the matrix.
//
//             When this returns, the matrix has been modified if and only if:
//             (*puDesiredWidth != uBitmapWidth) || (*puDesiredHeight != uBitmapHeight)
//

VOID
CBaseMatrix::AdjustForPrefiltering(
    __range(>=, 1) UINT uBitmapWidth,
    __range(>=, 1) UINT uBitmapHeight,
    REAL rShrinkFactorThreshold,
    __deref_out_range(>=, 1) UINT *puDesiredWidth,
    __deref_out_range(>=, 1) UINT *puDesiredHeight
    )
{
    Assert(uBitmapWidth > 0);
    Assert(uBitmapHeight > 0);
    Assert(rShrinkFactorThreshold >= 1.0f);
    Assert(puDesiredWidth);
    Assert(puDesiredHeight);

    UINT uScaledWidth;  // Set by ComputePrefilteringDimensions
    UINT uScaledHeight; // Set by ComputePrefilteringDimensions

    ComputePrefilteringDimensions(
        uBitmapWidth,
        uBitmapHeight,
        rShrinkFactorThreshold,
        OUT uScaledWidth,
        OUT uScaledHeight
        );

    //
    // Adjust the matrix to account for the bitmap scaling
    //

    // Note: It would be wrong to use the rScaleX and rScaleY computed in
    //       ComputePrefilteringDimensions, because of rounding and the
    //       adjustments we make to avoid extremes.

    if (   (uScaledWidth != uBitmapWidth)
        || (uScaledHeight != uBitmapHeight))
    {
        // Prepend the equivalent scale matrix
        
        CBaseMatrix mxScale = dxlayer::matrix::get_scaling(
            (static_cast<REAL>(uBitmapWidth)) / (static_cast<REAL>(uScaledWidth)),
            (static_cast<REAL>(uBitmapHeight)) / (static_cast<REAL>(uScaledHeight)),
            1.0f);

        this->set(mxScale.multiply_by(*this));
    }

    *puDesiredWidth = uScaledWidth;
    *puDesiredHeight = uScaledHeight;
}

//+-----------------------------------------------------------------------------
//
//  Function:  CompareWithoutOffset
//
//  Synopsis:  Compares the two matrices without the offset part
//+-----------------------------------------------------------------------------
BOOL 
CBaseMatrix::CompareWithoutOffset(__in_ecount(1) const CBaseMatrix &in) const
{
    return (_11 == in._11 && _12 == in._12 && 
            _21 == in._21 && _22 == in._22);
}


#if DBG
#include <strsafe.h>

//+-------------------------------------------------------------------------------------------------
//
//  Member:     CBaseMatrix::Dump
//
//  Synopsis:   Debug dump
//
//--------------------------------------------------------------------------------------------------
void
CBaseMatrix::Dump() const
{
    WCHAR wzString[100];

    OutputDebugString(L"CBaseMatrix\n");
    // Debug spew -- ignore failure.
    IGNORE_HR(StringCchPrintfW(wzString, ARRAYSIZE(wzString), L"%f, %f\n", _11, _12));
    OutputDebugString(wzString);

    // Debug spew -- ignore failure.
    IGNORE_HR(StringCchPrintfW(wzString, ARRAYSIZE(wzString), L"%f, %f\n", _21, _22));
    OutputDebugString(wzString);

    // Debug spew -- ignore failure.
    IGNORE_HR(StringCchPrintfW(wzString, ARRAYSIZE(wzString), L"%f, %f\n", _41, _42));
    OutputDebugString(wzString);
}
#endif



VOID CBaseMatrix::Multiply(__in_ecount(1) const CBaseMatrix& m)
{
    auto& self = *this;
    self = self * m;
}

VOID CBaseMatrix::PreMultiply(__in_ecount(1) const CBaseMatrix& m)
{
    auto& self = *this;
    self = m * self;
}




/**************************************************************************\
*
* Function Description:
*
*   Infer an affine transformation matrix from a rectangle-to-rectangle mapping.
*
* Arguments:
*
*   [IN] rcInSpace  - Specifies the source rectangle
*   [IN] rcOutSpace - Specifies the destination rectangle
*
\**************************************************************************/

void CBaseMatrix::InferAffineMatrix(
    __in_ecount(1) const CMilRectF &rcInSpace,
    __in_ecount(1) const CMilRectF &rcOutSpace
    )
{
    SetToIdentity();

    // Division by zero is okay

    double rScaleX = rcOutSpace.UnorderedWidth<double>() / rcInSpace.UnorderedWidth<double>();
    _11 = static_cast<float>(rScaleX);
    _41 = static_cast<float>(rcOutSpace.left - (rScaleX * rcInSpace.left));

    double rScaleY = rcOutSpace.UnorderedHeight<double>() / rcInSpace.UnorderedHeight<double>();
    _22 = static_cast<float>(rScaleY);
    _42 = static_cast<float>(rcOutSpace.top - (rScaleY * rcInSpace.top));
}

/**************************************************************************\
*
* Function Description:
*
*   Infer an affine transformation matrix
*   from a rectangle-to-parallelogram mapping
*
* Arguments:
*
*   [IN] rcInSpace - Specifies the source rectangle
*   [IN] rgptOutSpace - Specifies the destination parallelogram
*       The array must contain at least 3 points.
*       rgptOutSpace[0] <=> top-left corner of the source rectangle
*       rgptOutSpace[1] <=> top-right corner
*       rgptOutSpace[2] <=> bottom-left corner
*
* Reference:
*
*   Digital Image Warping
*   by George Wolberg
*   pp. 50-51
*
\**************************************************************************/

void CBaseMatrix::InferAffineMatrix(
    __in_ecount(1) const CMilRectF &rcInSpace,
    __in_ecount(3) const MilPoint2F *rgptOutSpace
    )
{

    REAL x0, y0, x1, y1, x2, y2;
    REAL u0, v0, u1, v1, u2, v2;
    REAL d;

    x0 = rgptOutSpace[0].X;
    y0 = rgptOutSpace[0].Y;
    x1 = rgptOutSpace[1].X;
    y1 = rgptOutSpace[1].Y;
    x2 = rgptOutSpace[2].X;
    y2 = rgptOutSpace[2].Y;

    u0 = rcInSpace.left;
    v0 = rcInSpace.top;
    u1 = rcInSpace.right;
    v1 = v0;
    u2 = u0;
    v2 = rcInSpace.bottom;

    d = u0*(v1-v2) - v0*(u1-u2) + (u1*v2-u2*v1);

    if (REALABS(d) < REAL_EPSILON)
    {
        TraceTag((tagMILWarning, "Colinear points in InferAffineMatrix"));
    }

    SetToIdentity();

    // Division by zero is okay
    d = TOREAL(1.0) / d;

    REAL t0, t1, t2;

    t0 = v1-v2;
    t1 = v2-v0;
    t2 = v0-v1;
    _11 = d * (x0*t0 + x1*t1 + x2*t2);
    _12 = d * (y0*t0 + y1*t1 + y2*t2);

    t0 = u2-u1;
    t1 = u0-u2;
    t2 = u1-u0;
    _21 = d * (x0*t0 + x1*t1 + x2*t2);
    _22 = d * (y0*t0 + y1*t1 + y2*t2);

    t0 = u1*v2-u2*v1;
    t1 = u2*v0-u0*v2;
    t2 = u2*v1-u1*v0;
    _41 = d * (x0*t0 + x1*t1 + x2*t2);
    _42 = d * (y0*t0 + y1*t1 + y2*t2);
}






extern "C" VOID WINAPI MatrixAppendRotateAt2D(
    __inout_ecount(1) CBaseMatrix *pmat,
    FLOAT angle,
    FLOAT x,
    FLOAT y
    )
{
    // Construct a rotation matrix at the appropriate location by translating
    // (x, y) to the origin, applying a pure rotation matrix and translating
    // back again.
    auto matrix = matrix::get_translation(-x, -y, 0.0f);
    auto tmp = matrix::get_rotation_z(angle);
    matrix = matrix * tmp;
    
    tmp = matrix::get_translation(x, y, 0.0f);
    matrix = matrix * tmp;

    // Append the rotation to the current matrix in place.
    *pmat = *pmat * matrix;
}

extern "C" VOID WINAPI MatrixAppendTranslate2D(
    __inout_ecount(1) CBaseMatrix *pmat,
    FLOAT dx,
    FLOAT dy
    )
{
    pmat->_41 += dx;
    pmat->_42 += dy;
}

extern "C" VOID WINAPI MatrixPrependTranslate2D(
    __inout_ecount(1) CBaseMatrix *pmat,
    FLOAT dx,
    FLOAT dy
    )
{
    auto& mat = *pmat;
    mat = matrix::get_translation(dx, dy, 0.0f) * mat;
}

extern "C" VOID WINAPI MatrixAppendRotate2D(
    __inout_ecount(1) CBaseMatrix *pmat,
    FLOAT angle
    )
{
    auto& mat = *pmat;
    mat = mat * matrix::get_rotation_z(angle);
}

extern "C" VOID WINAPI MatrixAppendScale2D(
    __inout_ecount(1) CBaseMatrix *pmat,
    FLOAT sx,
    FLOAT sy
    )
{
    CBaseMatrix scaling_matrix = matrix::get_scaling(sx, sy, 1.0f);
    *pmat = pmat->multiply_by(scaling_matrix);
}




