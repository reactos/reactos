// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Coverage buffer implementation
//

#include "precomp.hpp"

MtDefine(CoverageIntervalBuffer, MILRawMemory, "CoverageIntervalBuffer");

//-------------------------------------------------------------------------
//
//  Function:   CCoverageBuffer::Initialize
//
//  Synopsis:   Set the coverage buffer to a valid initial state
// 
//-------------------------------------------------------------------------
VOID
CCoverageBuffer::Initialize()
{
    m_pIntervalBufferBuiltin.m_interval[0].m_nPixelX = INT_MIN;
    m_pIntervalBufferBuiltin.m_interval[0].m_nCoverage = 0;
    m_pIntervalBufferBuiltin.m_interval[0].m_pNext = &m_pIntervalBufferBuiltin.m_interval[1];

    m_pIntervalBufferBuiltin.m_interval[1].m_nPixelX = INT_MAX;
    m_pIntervalBufferBuiltin.m_interval[1].m_nCoverage = 0xdeadbeef;
    m_pIntervalBufferBuiltin.m_interval[1].m_pNext = NULL;

    m_pIntervalBufferBuiltin.m_pNext = NULL;
    m_pIntervalBufferCurrent = &m_pIntervalBufferBuiltin;

    m_pIntervalStart = &m_pIntervalBufferBuiltin.m_interval[0];
    m_pIntervalNew = &m_pIntervalBufferBuiltin.m_interval[2];
    m_pIntervalEndMinus4 = &m_pIntervalBufferBuiltin.m_interval[INTERVAL_BUFFER_NUMBER - 4];
}

//-------------------------------------------------------------------------
//
//  Function:   CCoverageBuffer::Destroy
//
//  Synopsis:   Free all allocated buffers
// 
//-------------------------------------------------------------------------
VOID
CCoverageBuffer::Destroy()
{
    // Free the linked-list of allocations (skipping 'm_pIntervalBufferBuiltin',
    // which is built into the class):

    CCoverageIntervalBuffer *pIntervalBuffer = m_pIntervalBufferBuiltin.m_pNext;
    while (pIntervalBuffer != NULL)
    {
        CCoverageIntervalBuffer *pIntervalBufferNext = pIntervalBuffer->m_pNext;
        GpFree(pIntervalBuffer);
        pIntervalBuffer = pIntervalBufferNext;
    }
}

//-------------------------------------------------------------------------
//
//  Function:   CCoverageBuffer::Reset
//
//  Synopsis:   Reset the coverage buffer
// 
//-------------------------------------------------------------------------
VOID
CCoverageBuffer::Reset()
{
    // Reset our coverage structure.  Point the head back to the tail,
    // and reset where the next new entry will be placed:

    m_pIntervalBufferBuiltin.m_interval[0].m_pNext = &m_pIntervalBufferBuiltin.m_interval[1];

    m_pIntervalBufferCurrent = &m_pIntervalBufferBuiltin;
    m_pIntervalNew = &m_pIntervalBufferBuiltin.m_interval[2];
    m_pIntervalEndMinus4 = &m_pIntervalBufferBuiltin.m_interval[INTERVAL_BUFFER_NUMBER - 4];
}

//-------------------------------------------------------------------------
//
//  Function:   CCoverageBuffer::Grow
//
//  Synopsis:   
//      Grow our interval buffer.
//
//-------------------------------------------------------------------------
HRESULT 
CCoverageBuffer::Grow(
    __deref_out_ecount(1) CCoverageInterval **ppIntervalNew, 
    __deref_out_ecount(1) CCoverageInterval **ppIntervalEndMinus4
    )
{
    HRESULT hr = S_OK;
    CCoverageIntervalBuffer *pIntervalBufferNew = m_pIntervalBufferCurrent->m_pNext;

    if (!pIntervalBufferNew)
    {
        pIntervalBufferNew = static_cast<CCoverageIntervalBuffer*>(GpMalloc(
             Mt(CoverageIntervalBuffer),
             sizeof(CCoverageIntervalBuffer)
             ));

        IFCOOM(pIntervalBufferNew);

        pIntervalBufferNew->m_pNext = NULL;
        m_pIntervalBufferCurrent->m_pNext = pIntervalBufferNew;
    }

    m_pIntervalBufferCurrent = pIntervalBufferNew;

    m_pIntervalNew = &pIntervalBufferNew->m_interval[2];
    m_pIntervalEndMinus4 = &pIntervalBufferNew->m_interval[INTERVAL_BUFFER_NUMBER - 4];

    *ppIntervalNew = m_pIntervalNew;
    *ppIntervalEndMinus4 = m_pIntervalEndMinus4;

Cleanup:
    RRETURN(hr);
}





