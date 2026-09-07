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
//      Definition of methods used to create intermediate brush representations
//      from user-defined state.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

class CBrushTypeUtils
{
public:

    static VOID GetBrushTransform(
        __in_ecount_opt(1) const CMILMatrix *pmatRelative,
        __in_ecount_opt(1) const CMILMatrix *pmatTransform,
        __in_ecount(1) const MilPointAndSizeD *pBoundingBox,
        __out_ecount(1) CMILMatrix *pResultTransform
        );

    static VOID ConvertRelativeTransformToAbsolute(
        __in_ecount(1) const MilPointAndSizeF *pBoundingBox,
        __in_ecount(1) const CMILMatrix *pRelativeTransform,
        __out_ecount(1) CMILMatrix* pConvertedTransform
        );
};


VOID 
AdjustRelativePoint(
    __in const MilPointAndSizeD *pBoundingBox,
    __inout MilPoint2F *pt
    );

VOID 
AdjustRelativeRectangle(
    __in const MilPointAndSizeD *prcBoundingBox,
    __inout MilPointAndSizeD *prcAdjustRectangle
    );



