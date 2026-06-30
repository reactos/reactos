// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//

#include "precomp.hpp"

// C4356: 'TMilRect<TBase,TBaseRect,unique>::sc_rcEmpty' : static data member cannot be initialized via derived class
#pragma warning(disable:4356)

const CMilRectF::Rect_t CMilRectF::sc_rcEmpty(
    0, 0,
    0, 0,
    LTRB_Parameters
    );

const CMilRectF::Rect_t CMilRectF::sc_rcInfinite(
    -FLT_MAX, -FLT_MAX,
     FLT_MAX,  FLT_MAX,
    LTRB_Parameters
    );


const CMilRectL::Rect_t CMilRectL::sc_rcEmpty(
    0, 0,
    0, 0,
    LTRB_Parameters
    );

const CMilRectL::Rect_t CMilRectL::sc_rcInfinite(
    -LONG_MAX-1, -LONG_MAX-1,
    LONG_MAX, LONG_MAX,
    LTRB_Parameters
    );

const CMilRectU::Rect_t CMilRectU::sc_rcEmpty(
    0, 0,
    0, 0,
    LTRB_Parameters
    );

const CMilRectU::Rect_t CMilRectU::sc_rcInfinite(
    0, 0,
    ULONG_MAX, ULONG_MAX,
    LTRB_Parameters
    );




