// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Miscellaneous rectangle utility routines
//
//-----------------------------------------------------------------------------

#pragma once


BOOL
AreTransformedRectanglesClose(
    __in_ecount(1) const CMilRectF *pFirstRect,
    __in_ecount(1) const CMilRectF *pSecondRect,
    __in_ecount_opt(1) const CMILMatrix *pTransform,
    __in float closeTolerance
    );

BOOL
RectF_RBFromParallelogramPointsF(
    __in_ecount(4) const MilPoint2F *pPoints,
    __out_ecount_opt(1) MilRectF *pRectF_RB
    );


