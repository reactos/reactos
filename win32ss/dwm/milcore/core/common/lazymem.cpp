// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+--------------------------------------------------------------------------
//

//
//  Abstract:
//      Defines classes CLazyMemBlock and CGlyphPainterMemory
//      to provide simple memory management for text rendering.
// 
//

#include "precomp.hpp"

MtDefine(MILTempMemory, Mem, "MIL Temp memory");

//+------------------------------------------------------------------------
//
//  Member:
//      CLazyMemBlock::Clean
//
//  Synopsis:
//      Deallocate a block of memory
//
//-------------------------------------------------------------------------
void
CLazyMemBlock::Clean()
{
    if (m_pData)
    {
        WPFFree(ProcessHeap, m_pData);
        m_pData = NULL;
        m_size = 0;
    }
}

//+------------------------------------------------------------------------
//
//  Member:
//      CLazyMemBlock::Reallocate
//
//  Synopsis:
//      Allocate a block of memory
//
//  Returns:
//      Pointer to allocated memory or null on failure.
//              
//-------------------------------------------------------------------------
__allocator __success(return!=NULL) __bcount(size) void*
CLazyMemBlock::Reallocate(
    __out_range(==, this->m_size) UINT size
    )
{
    if (m_pData)
    {
        WPFFree(ProcessHeap, m_pData);
    }
    m_pData = WPFAlloc(ProcessHeap, Mt(MILTempMemory), size);
    m_size = m_pData ? size : 0;
    return m_pData;
}

//+------------------------------------------------------------------------
//
//  Member:
//      CGlyphPainterMemory::Clean
//
//  Synopsis:
//      Deallocate the memory
//              
//-------------------------------------------------------------------------
void
CGlyphPainterMemory::Clean()
{
    m_positions.Clean();
    m_glyphBitmapRefs.Clean();
    m_runBitmap.Clean();
    m_alphaArray.Clean();
}

//+------------------------------------------------------------------------
//
//  Member:
//      CGlyphPainterMemory::CleanHuge
//
//  Synopsis:
//      Deallocate the memory if there is too much allocated
//              
//-------------------------------------------------------------------------
void
CGlyphPainterMemory::CleanHuge()
{
    static const UINT tooMuch = 16384;
    if (m_alphaArray.GetSize() > tooMuch)
    {
        m_alphaArray.Clean();
    }
}


