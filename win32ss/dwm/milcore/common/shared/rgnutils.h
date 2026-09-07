// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------------
//

//
// Module Name:
//
//    rgnutils.h
//
//---------------------------------------------------------------------------------

#pragma once

HRESULT
HrgnToRgnData(
    HRGN hrgn, 
    __deref_out RGNDATA **ppRgnData
    );

HRESULT
HrgnFromRects(
    __in_ecount(nCount) const RECT *pRects,
    UINT nCount,
    __deref_out_ecount(1) HRGN *phrgn
    );



