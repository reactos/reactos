// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//---------------------------------------------------------------------------------
//

//
// Module Name:
//
//    rgnutils.cpp
//
//---------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(MUnwrapRegionData, MILRawMemory, "MUnwrapRegionData");

//+--------------------------------------------------------------------------------
//
// Method: 
//      HrgnToRgnData
//
// Synposis: 
//      Extracts the region data from a region handle
//
// Arguments:
//      hRgn - GDI region handle
//
//      ppRgnData - Returned region data - callers must free this memory 
//                  with WPFFree
// Returns: 
//      HRESULT indicating success or failure 
//
//---------------------------------------------------------------------------------
HRESULT
HrgnToRgnData(
    HRGN hrgn, 
    __deref_out RGNDATA **ppRgnData
    )
{
    // Get the region's data size, allocate a temporary buffer
    HRESULT hr = S_OK;
    UINT ccbRegionData;
    RGNDATA *pRegionData = NULL;

    IFCW32(ccbRegionData = GetRegionData(hrgn, 0, NULL));

    pRegionData =  WPFAllocType(
        RGNDATA *, 
        ProcessHeap,
        Mt(MUnwrapRegionData),
        ccbRegionData
        );
    IFCOOM(pRegionData);

    // Retrieve the region data
    IFCW32(GetRegionData(hrgn, ccbRegionData, pRegionData));

    *ppRgnData = pRegionData;
    pRegionData = NULL;

Cleanup:
    // Making sure that memory is recollected in case of error
    if (pRegionData)
    {
        WPFFree(ProcessHeap, pRegionData);
    }
    
    RRETURN(hr);
}

//+--------------------------------------------------------------------------------
//
// Method: 
//      HrgnFromRects
//
// Synposis: 
//      Constructs region from the list of rectangles
//
// Arguments:
//      pRects - ponter to the array of rects
//
//      nCount - number of elements in pRects
//
//      phrgn - Returned region
//
// Returns: 
//      HRESULT indicating success or failure 
//
//---------------------------------------------------------------------------------
HRESULT
HrgnFromRects(
    __in_ecount(nCount) const RECT *pRects,
    UINT nCount,
    __deref_out_ecount(1) HRGN *phrgn
    )
{
    HRESULT hr = S_OK;
    HRGN hrgn = NULL;
    UINT ccbRects = 0;
    UINT ccbRegionData = 0;
    RGNDATA *pRegionData = NULL;
    RECT rcBound = {0};
    
    // Calculate region data size and allocate buffer
    IFC(UIntMult(nCount, sizeof(RECT), &ccbRects));
    IFC(UIntAdd(ccbRects, sizeof(RGNDATA), &ccbRegionData));

    pRegionData = WPFAllocType(
        RGNDATA *, 
        ProcessHeap,
        Mt(MUnwrapRegionData),
        ccbRegionData
        );

    IFCOOM(pRegionData);
    
    // Calculate bounding rect
    if (nCount > 0)
    {
        rcBound = pRects[0];

        for (UINT nRect = 1; nRect < nCount; nRect++)
        {
            if (pRects[nRect].left < rcBound.left) rcBound.left = pRects[nRect].left;
            if (pRects[nRect].top < rcBound.top) rcBound.top = pRects[nRect].top;
            if (pRects[nRect].right  > rcBound.right) rcBound.right = pRects[nRect].right;
            if (pRects[nRect].bottom > rcBound.bottom) rcBound.bottom = pRects[nRect].bottom;
        }
    }
    
    // Poulate region data header
    pRegionData->rdh.dwSize = sizeof(RGNDATAHEADER);
    pRegionData->rdh.iType = RDH_RECTANGLES;
    pRegionData->rdh.nCount = nCount;
    pRegionData->rdh.nRgnSize = ccbRects;
    pRegionData->rdh.rcBound = rcBound;
    
    // Copy rectangles data
    RtlCopyMemory(pRegionData->Buffer, pRects, ccbRects);
    
    // Create region object
    IFCW32(hrgn = ExtCreateRegion(NULL, ccbRegionData, pRegionData));
    
    *phrgn = hrgn;

Cleanup:
    if (pRegionData)
    {
        WPFFree(ProcessHeap, pRegionData);
    }

    RRETURN(hr);
}




