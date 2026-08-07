// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Class CGlyphRunStorage implementation.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.h"

MtDefine(CGlyphRunStorage, CGlyphRunResource, "CGlyphRunStorage");

volatile LONG g_uiCMILGlyphRunCount = 0;

//=========================================================== CGlyphRunStorage

CGlyphRunStorage::~CGlyphRunStorage()
{
    ReleaseInterface(m_pIDWriteFont);
    CGlyphRunResource::ResetFontFaceCache();

    WPFFree(ProcessHeap, m_pGlyphIndices);
    
    InterlockedDecrement(&g_uiCMILGlyphRunCount);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CGlyphRunStorage::InitStorage
//
//  Synopsis:
//      Initialize CGlyphRunStorage instance using transportation data
//
//------------------------------------------------------------------------------
HRESULT
CGlyphRunStorage::InitStorage(
    MILCMD_GLYPHRUN_CREATE const* pPacket,
    UINT cbSize
    )
{
    HRESULT hr = S_OK;

    if (   (cbSize < sizeof (MILCMD_GLYPHRUN_CREATE))
        || (pPacket->GlyphCount == 0)
        || !(pPacket->MuSize >= 0)
       )
    {
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }

    m_usGlyphCount      = pPacket->GlyphCount;
    m_glyphRunFlags     = pPacket->GlyphRunFlags;
    m_origin            = pPacket->Origin;
    m_muSize            = pPacket->MuSize;
    m_bidiLevel         = pPacket->BidiLevel;
    m_measuringMethod   = static_cast<DWRITE_MEASURING_MODE>
                                (pPacket->DWriteTextMeasuringMethod);

    //
    // Note: IDWriteFont was already AddRef'd for us on the UI thread to ensure that
    // it survived to this point. We own the reference now.
    //
    m_pIDWriteFont  = reinterpret_cast<IDWriteFont*>(pPacket->pIDWriteFont);

    // Cast our bounds rect from double -> float.
    MilRectFFromMilPointAndSizeD(m_boundingRect, *reinterpret_cast<const MilPointAndSizeD*>(&pPacket->ManagedBounds));

    {   // Check arrays for possible corruption.
        // Calculate array sizes along the way.

        const BYTE* pStart = reinterpret_cast<const BYTE*>(pPacket); // beginning of packet

        const BYTE* pIndices = pStart + sizeof(MILCMD_GLYPHRUN_CREATE); // beginning of packet data
        
        UINT uGlyphCount = static_cast<UINT>(m_usGlyphCount);

        UINT cbIndices = sizeof(USHORT) * uGlyphCount;    
        UINT cbAdvances = sizeof(float) * uGlyphCount;
        UINT cbOffsets = HasOffsets() ? sizeof(float) * (2 * uGlyphCount) : 0;
        
        const BYTE* pAdvances = pIndices + cbIndices;  // end of m_pGlyphIndices origin

        const BYTE* pOffsets = pAdvances + cbAdvances;  // end of m_pGlyphAdvances origin

        UINT cbVarData = 0;
        IFC(UIntAdd(cbAdvances, cbIndices, &cbVarData));
        IFC(UIntAdd(cbVarData, cbOffsets, &cbVarData));

        const BYTE* pEnd = pStart + cbSize; // end of packet;

        if (!(pStart <= pIndices && pIndices <= pAdvances && pAdvances <= pOffsets && pOffsets <= pEnd))
        {
            Assert(false);
            IFC(WGXERR_UCE_MALFORMEDPACKET);
        }

        // The variable sized arrays in the packet may not be properly aligned
        // Compute the padding needed to to align them
        UINT cbIndicesPadded = GetPaddedByteCount(cbIndices);    
        UINT cbAdvancesPadded = GetPaddedByteCount(cbAdvances);
        
        UINT cbVarDataPadded = 0;
        IFC(UIntAdd(cbAdvancesPadded, cbIndicesPadded, &cbVarDataPadded));
        IFC(UIntAdd(cbVarDataPadded, cbOffsets, &cbVarDataPadded));
        
        BYTE *pData = (BYTE*)WPFAllocClear(
            ProcessHeap,
            Mt(CGlyphRunStorage),
            cbVarDataPadded
            );
        IFCOOM(pData);

        {   // split the data buffer
            m_pGlyphIndices = reinterpret_cast<USHORT*>(pData);
            memcpy(m_pGlyphIndices, pIndices, cbIndices);
            
            m_pGlyphAdvances = reinterpret_cast<float*>(pData + cbIndicesPadded);
            memcpy(m_pGlyphAdvances, pAdvances, cbAdvances);
            
            if (HasOffsets())
            {
                m_pGlyphOffsets = reinterpret_cast<float*>(pData + cbIndicesPadded + cbAdvancesPadded);
                memcpy(m_pGlyphOffsets, pOffsets, cbOffsets);
            }
            else
            {
                m_pGlyphOffsets = NULL;
            }
        }
    }

Cleanup:
    if (FAILED(hr))
    {
        // If anything went bad, make the glyph run empty.
        m_usGlyphCount = 0;
    }
    return hr;
}


