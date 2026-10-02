// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Simple memory allocation for CProgram.
//
//-----------------------------------------------------------------------------
#include "precomp.h"

//+------------------------------------------------------------------------------
//
//  Member:
//      CFlushMemory::Alloc
//
//  Synopsis:
//      Allocate contiguous block in flush memory.
//      New allocated block is not supposed to be ever freed.
//      All the consumed memory will be freed on CFlushMemory::Flush().
//
//-------------------------------------------------------------------------------
__bcount(cbSize) UINT8*
CFlushMemory::Alloc(UINT32 cbSize)
{
    if (cbSize > m_uAvailableSize && !m_fOverflow)
    {
        // Make new chunk
        static const int sc_defaultChunkSize = 0x1000;
        UINT32 cbRequiredSize = sc_defaultChunkSize;
        UINT32 cbActualSize = 0;
        while ((cbRequiredSize - sizeof(CChunk)) < cbSize)
        {
            cbRequiredSize *= 2;
            if (cbRequiredSize == 0)
            {
                // early return to protect against infinit looping
                // when a caller requested really big piece of memory
                m_fOverflow = TRUE;
                return NULL;
            }
        }

        UINT8 *pMemory = NULL;
        if ((cbRequiredSize - sizeof(CChunk)) >= cbSize)
        {
            pMemory = CJitterSupport::MemoryAllocate(cbRequiredSize, cbActualSize);
        }

        if (pMemory)
        {
            WarpAssert(cbActualSize >= cbRequiredSize);
            CChunk *pNewChunk = new(pMemory) CChunk(m_pChunks);
            m_pChunks = pNewChunk;
            m_pAvailableMemory = pMemory + sizeof(CChunk);
            m_uAvailableSize = cbActualSize - sizeof(CChunk);
        }
        else
        {
            m_fOverflow = TRUE;
        }
    }

    if (cbSize > m_uAvailableSize)
    {
        return NULL;
    }
    else
    {
        UINT8* pResult = m_pAvailableMemory;
        m_pAvailableMemory += cbSize;
        m_uAvailableSize -= cbSize;
        return pResult;
    }
}

//+------------------------------------------------------------------------------
//
//  Member:
//      CFlushMemory::Flush
//
//  Synopsis:
//      Free all memory ever consumed.
//
//-------------------------------------------------------------------------------
void
CFlushMemory::Flush()
{
    while (m_pChunks)
    {
        CChunk *pChunk = m_pChunks;
        m_pChunks = pChunk->m_pNext;
        CJitterSupport::MemoryFree((UINT8*)pChunk);
    }

    m_pAvailableMemory = NULL;
    m_uAvailableSize = 0;
    m_fOverflow = FALSE;
}


