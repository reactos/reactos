// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  Description:  
//
//      The "Copy" scan operation. See scanoperation.h.
//
//      Scan operations for copying a scan. Because the operation doesn't need
//      to interpret the pixel data, we only need one function per pixel size
//      (in bits).
//
//  Notes:
//
//      The destination and source scans must not overlap in memory.
//

#include "precomp.hpp"

//+-----------------------------------------------------------------------------
//
//  Operation Description:
//
//      Copy: Binary operation; copies a scan, to the same destination format.
//
//  Inputs:
//
//      pSOP->m_pvDest:   The destination scan.
//      pSOP->m_pvSrc1:   The source scan.
//      pPP->m_uiCount:    Scan length, in pixels.
//
//  Return Value:
//
//      None
//

// Copy 1bpp

VOID FASTCALL
Copy_1(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    UINT uiCount = pPP->m_uiCount;
    
    GpMemcpy(pSOP->m_pvDest, pSOP->m_pvSrc1, (uiCount + 7) >> 3);
}

// Copy 4bpp

VOID FASTCALL
Copy_4(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    UINT uiCount = pPP->m_uiCount;
    
    GpMemcpy(pSOP->m_pvDest, pSOP->m_pvSrc1, (4*uiCount + 4) >> 3);
}

// Copy 8bpp

VOID FASTCALL
Copy_8(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    UINT uiCount = pPP->m_uiCount;
    
    GpMemcpy(pSOP->m_pvDest, pSOP->m_pvSrc1, uiCount);
}

// Copy 16bpp

VOID FASTCALL
Copy_16(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    UINT uiCount = pPP->m_uiCount;
    
    GpMemcpy(pSOP->m_pvDest, pSOP->m_pvSrc1, 2*uiCount);
}

// Copy 24bpp

VOID FASTCALL
Copy_24(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    UINT uiCount = pPP->m_uiCount;
    
    GpMemcpy(pSOP->m_pvDest, pSOP->m_pvSrc1, 3*uiCount);
}

// Copy 32bpp

VOID FASTCALL
Copy_32(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB, ARGB)
    UINT uiCount = pPP->m_uiCount;
    
    while (uiCount--)
    {
        *pDest++ = *pSrc++;
    }
}

// Copy 48bpp

VOID FASTCALL
Copy_48(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    UINT uiCount = pPP->m_uiCount;
    
    GpMemcpy(pSOP->m_pvDest, pSOP->m_pvSrc1, 6*uiCount);
}

// Copy 64bpp

VOID FASTCALL
Copy_64(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    DEFINE_POINTERS(ARGB64, ARGB64)
    UINT uiCount = pPP->m_uiCount;
    
    while (uiCount--)
    {
        *pDest++ = *pSrc++;
    }
}

// Copy 128bpp

VOID FASTCALL
Copy_128(
    const PipelineParams *pPP,
    const ScanOpParams *pSOP
    )
{
    UINT uiCount = pPP->m_uiCount;
    
    GpMemcpy(pSOP->m_pvDest, pSOP->m_pvSrc1, sizeof(MilColorF)*uiCount);
}





