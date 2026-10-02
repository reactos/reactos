// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains declarations for generic render utility routines.
//
//------------------------------------------------------------------------------

#pragma once

class CPlainPen;

//
// Round num to the closest power of 2 that is
// equal or greater than num
//
#define ROUNDTOPOW2_UPPER_BOUND 0x80000000  // = 1 << 31 and is understood by PREfast
extern UINT RoundToPow2(__in_range(1,ROUNDTOPOW2_UPPER_BOUND) UINT num);


//
// Return smallest N such that 2^(N+1) > ui
//
extern __range(0,32) UINT Log2(UINT ui);
float Distance( MilPoint2F pt1, MilPoint2F pt2 );

//
// Check if the filtermode uses mipmapping.
// 
bool DoesUseMipMapping(MilBitmapInterpolationMode::Enum interpolationMode);

HRESULT SetPenDoubleDashArray(
    __inout_ecount(1) CPlainPen *pPen,
    __in_ecount_opt(cDash) double *rgDashDouble,
    UINT cDash
    );

UINT GetPaddedByteCount(UINT cbSize);


