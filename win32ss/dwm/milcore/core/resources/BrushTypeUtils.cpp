// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_brush
//      $Keywords:
//
//  $Description:
//      Implementation of methods used to create intermediate brush
//      representations from user-defined state.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CBrushTypeUtils, MILRender, "CBrushTypeUtils");

/*++

Routine Description:

    Obtains immediate (realized) value of the brush transform.
    We derive the brush transform from converting the relative
    transform to absolute space using the bounding box and combining
    it with the absolute transform.  Per spec, the relative transform
    is applied before the absolute transform.  This allows users to do
    things like rotating about the center of a shape using the relative
    transform and then offseting it by a constant amount amoungst all shapes
    being filled using the absolute transform.

Return Value:

    HRESULT

--*/

VOID
CBrushTypeUtils::GetBrushTransform(
    __in_ecount_opt(1) const CMILMatrix *pmatRelative,
        // Current value of user-specified Brush.RelativeTransform property
    __in_ecount_opt(1) const CMILMatrix *pmatTransform,
        // Current value of user-specified Brush.Transform property
    __in_ecount(1) const MilPointAndSizeD *pBoundingBox,
        // Bounding box the relative transform is relative to
    __out_ecount(1) CMILMatrix *pResultTransform
        // Output combined transform
    )
{

    // Assert required parameters
    Assert(pBoundingBox && pResultTransform);

    // fResultSet specifies whether or not the result matrix has been set.
    // We use this knowledge to avoid unncessary matrix operations when
    // the relative and/or absolute transforms aren't set.
    BOOL fResultSet = FALSE;

    //
    // Apply the relative transform
    //

    if (pmatRelative)       
    {
        // Handle relative transforms applied to degenerate shapes.  This equality
        // check has been added to maintain parity with the previous InferAffineMatrix
        // implementation.  But we need to handle dimensions close to zero, in addition
        // to zero.
        if ( (pBoundingBox->Width != 0.0) &&
             (pBoundingBox->Height != 0.0))
        {
            // Bounding box relative coordinates are relative to
            MilPointAndSizeF absoluteBounds;

            MilPointAndSizeFFromMilPointAndSizeD(&absoluteBounds, pBoundingBox);

            // Calculate matrix that transforms absolute coordinates by the relative transform
            ConvertRelativeTransformToAbsolute(
                &absoluteBounds,
                pmatRelative,
                pResultTransform
                );
            
            fResultSet = TRUE;
        }
    }

    //
    // Apply the absolute transform, if one was specified
    //

    if (pmatTransform)
    {
        if (fResultSet)
        {
            // Append the absolute transform to the relative transform if
            // a relative transform was set
            pResultTransform->Multiply(*pmatTransform);
        }
        else
        {
            // Copy the absolute transform directly to the out-param if no
            // relative transform was specified
            *pResultTransform = *pmatTransform;
        }
        fResultSet = TRUE;
    }

    // Set the matrix to identity if the matrix hasn't been initialized because
    // no transforms were specified
    if (!fResultSet)
    {
        pResultTransform->SetToIdentity();
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CBrushTypeUtils::ConvertRelativeTransformToAbsolute
//
//  Synopsis:
//      Given the relative transform & bounding box it's relative to, this
//      function calculates an absolute derivation of the relative tranform.
//
//      The relative transform is modified to take absolute coordinates as
//      input, transform those coordinates by the user-specified relative
//      transform, and then output absolute coordinates.
//
//  Notes:
//      This function is an optimized equivalent of the following operations:
//
//      MilRectF relativeBounds = {0.0, 0.0, 1.0, 1.0};
//      pResultTransform->InferAffineMatrix(/*from*/ pBoundingBox, /*to*/ relativeBounds);
//      pResultTransform->Multiply(*pmatRelative);
//      relativeToAbsolute.InferAffineMatrix(/*from*/ relativeBounds, /*to*/ pBoundingBox);
//      pResultTransform->Multiply(relativeToAbsolute);
//
//      To avoid inferring 2 rectangle mappings, & performing 2 full matrix
//      multiplications, the resultant math performed by these 4 operations was
//      expanded out, and terms which cancel or always evaluate to 0 were
//      removed.  As a final optimization, this function assumes (and asserts)
//      that the input relative transform only has 6 elements set to
//      non-identity values.
//
//------------------------------------------------------------------------------
VOID
CBrushTypeUtils::ConvertRelativeTransformToAbsolute(
    __in_ecount(1) const MilPointAndSizeF *pBoundingBox,            
        // Bounds that the relative transform is relative to.  The unit square in the relative 
        // coordinate space is mapped to these bounds in the absolute coordinate space.
    __in_ecount(1) const CMILMatrix *pRelativeTransform,
        // User-specified relative transform
    __out_ecount(1) CMILMatrix* pConvertedTransform
        // Relative transform that has been modified to take absolute coordinates as input,
        // transform those coordinates by the relative transform, and then output absolute
        // coordinates.
    )
{
    // Copy commonly used variables to the stack for quicker access (and to make the implementation 
    // more readable)
    REAL X = pBoundingBox->X;
    REAL Y = pBoundingBox->Y;
    REAL W = pBoundingBox->Width;
    REAL H = pBoundingBox->Height;

    // Precompute divides that are needed more than once.
    REAL rHeightDividedByWidth = H / W;
    REAL rWidthDividedByHeight = W / H;

    // Guard that entries other than _11, _12, _21, _22, _41, & _42 are still identity
    //
    // Only these 6 matrix entries can be set at our API, which is assumed by this 
    // implementation.  Doing so allows us to dramatically reduce the number of 
    // calculations performed by this function.
    //
    // We allow NaN through these Asserts as NaN can pop up through matrix multiplies.
    // Unfortunately for matrix multiplies, NaN * 0 == NaN.
    //
    Assert (IsNaNOrIsEqualTo(pRelativeTransform->_13, 0.0f));
    Assert (IsNaNOrIsEqualTo(pRelativeTransform->_14, 0.0f));
    Assert (IsNaNOrIsEqualTo(pRelativeTransform->_23, 0.0f));
    Assert (IsNaNOrIsEqualTo(pRelativeTransform->_24, 0.0f));
    Assert (IsNaNOrIsEqualTo(pRelativeTransform->_31, 0.0f));
    Assert (IsNaNOrIsEqualTo(pRelativeTransform->_32, 0.0f));
    Assert (IsNaNOrIsEqualTo(pRelativeTransform->_33, 1.0f));
    Assert (IsNaNOrIsEqualTo(pRelativeTransform->_34, 0.0f));
    Assert (IsNaNOrIsEqualTo(pRelativeTransform->_43, 0.0f));
    Assert (IsNaNOrIsEqualTo(pRelativeTransform->_44, 1.0f));

    //
    // Calculate the first vector
    //

    pConvertedTransform->_11 = pRelativeTransform->_11;
    pConvertedTransform->_12 = pRelativeTransform->_12 * rHeightDividedByWidth;
    pConvertedTransform->_13 = 0.0f;
    pConvertedTransform->_14 = 0.0f;

    //
    // Calculate the second vector
    //
        
    pConvertedTransform->_21 = pRelativeTransform->_21 * rWidthDividedByHeight;
    pConvertedTransform->_22 = pRelativeTransform->_22;
    pConvertedTransform->_23 = 0.0f;
    pConvertedTransform->_24 = 0.0f;

    //
    // Calculate the third vector
    // 
    
    pConvertedTransform->_31 = 0.0f;
    pConvertedTransform->_32 = 0.0f;
    pConvertedTransform->_33 = 1.0f;
    pConvertedTransform->_34 = 0.0f;

    //
    // Calculate fourth vector
    //

    pConvertedTransform->_41 = 
        pRelativeTransform->_41 * W -
        pRelativeTransform->_11 * X -
        pRelativeTransform->_21 * Y * rWidthDividedByHeight +        
        X;

    pConvertedTransform->_42 = 
        pRelativeTransform->_42 * H -
        pRelativeTransform->_12 * X * rHeightDividedByWidth -
        pRelativeTransform->_22 * Y +
        Y;

    pConvertedTransform->_43 = 0.0f;
    pConvertedTransform->_44 = 1.0f;
}
    

/*++

Routine Description:

    Calculates an absolute point from a relative point and
    bounding box.

Arguments:
    pBoundingBox: Bounding box pt is relative to
    pt: IN:  Relative point OUT: Absolute Point

Return Value:

    void

--*/

VOID
AdjustRelativePoint(
    __in const MilPointAndSizeD *pBoundingBox,
    __inout MilPoint2F *pt
    )
{
    Assert(pt);

    // Must have bounding box
    Assert(pBoundingBox);

    // Relative points are defined as a decimal perctange of a bounding box
    // dimension.  Any given coordinate, "A", will reside within the bounding
    // box over the range 0.0 <= A <= 1.0.  E.g., if pt->X is 0.5, then the
    // absolute X coordinate is half the width of the bounding box,
    // or: pBoundingBox->X + 0.5 * pBoundingBox->Width.
    // Likewise, if pt->X is defined as 3.1, then the absolute coordinate is
    // 3.1 times the width of the bounding box + the bounding box's X coordinate

    // Calculate absolute point
    pt->X = static_cast<float>(pBoundingBox->X) + pt->X * static_cast<float>(pBoundingBox->Width);
    pt->Y = static_cast<float>(pBoundingBox->Y) + pt->Y * static_cast<float>(pBoundingBox->Height);
}

/*++

Routine Description:

    Calculates an absolute rectangle from a relative rectangle and
    bounding box.

Arguments:
    pBoundingBox: Bounding box pt is relative to
    pRelativeRectangle: IN:  Relative Rectangle OUT: Absolute Rectangle

Return Value:

    void

--*/

VOID 
AdjustRelativeRectangle(
    __in const MilPointAndSizeD *prcBoundingBox,
    __inout MilPointAndSizeD *prcAdjustRectangle
    )
{
    Assert(prcBoundingBox && prcAdjustRectangle);

    if (IsRectEmptyOrInvalid(prcBoundingBox) || IsRectEmptyOrInvalid(prcAdjustRectangle))
    {
        *prcAdjustRectangle = MilEmptyPointAndSizeD;
    }
    else
    {
        prcAdjustRectangle->X = prcBoundingBox->X + (prcAdjustRectangle->X * prcBoundingBox->Width);
        prcAdjustRectangle->Y = prcBoundingBox->Y + (prcAdjustRectangle->Y * prcBoundingBox->Height);
    
        prcAdjustRectangle->Width = prcAdjustRectangle->Width * prcBoundingBox->Width;
        prcAdjustRectangle->Height = prcAdjustRectangle->Height * prcBoundingBox->Height;
    }
}



