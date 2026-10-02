// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      Implementation of CD3DVertexBuffer
//
//      Buffer provides a simple vertex collector and collection can be sent to
//      device to draw primitives.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#include "precomp.hpp"

MtDefine(D3DVertexBuffers, Mem, "VertexBuffers");
MtDefine(D3DVertexBufferVertices, D3DVertexBuffers, "Storage space for DynVertexBuffer Vertices" );

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DVertexBuffer::ctor
//
//  Synopsis:
//      Constructs empty buffer
//
//------------------------------------------------------------------------------
CD3DVertexBuffer::CD3DVertexBuffer(
    __out_range(==, this->m_uVertexStride) UINT uVertexStride
    )
    : m_pVertices(NULL),
      m_uNumVertices(0),
      m_uVertexStride(uVertexStride),
      m_uCapVertices(0)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DVertexBuffer::dtor
//
//  Synopsis:
//      Free the 2 dynamic arrays
//
//------------------------------------------------------------------------------
CD3DVertexBuffer::~CD3DVertexBuffer()
{
    WPFFree( ProcessHeap, m_pVertices );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DVertexBuffer::GetMultipleVertices
//
//  Synopsis:
//      Ensures there's space for it and then retrieves multiple vertices
//
//------------------------------------------------------------------------------
HRESULT
CD3DVertexBuffer::GetMultipleVertices(
    __deref_bcount(m_iVertexStride * uNumNewVertices) void **ppStartVertex,
    __in_range(4,4) UINT uNumNewVertices
    )
{
    HRESULT hr = S_OK;

    Assert(m_uCapVertices >= m_uNumVertices);
    if (uNumNewVertices >= (m_uCapVertices - m_uNumVertices))
    {
        IFC(GrowVertexBufferSize(uNumNewVertices));
    }

    ReserveMemoryForVertices(ppStartVertex, uNumNewVertices);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DVertexBuffer::GrowVertexBufferSize
//
//  Synopsis:
//      Doubles the allocated size for the vertex buffer, and copies any old
//      data over
//
//------------------------------------------------------------------------------
HRESULT
CD3DVertexBuffer::GrowVertexBufferSize(__in_range(==,4) UINT uGrow)
{
    HRESULT hr = S_OK;

    UINT uNewCap = m_uCapVertices;

    // Assert against a usage pattern which could encounter an overflow.
    // Since uGrow == 4, this also ensures that m_uCapVertices + uGrow doesn't overflow.
    Assert((m_uCapVertices * 2) >= m_uCapVertices);

    do
    {
        uNewCap = ( uNewCap == 0 ) ? 4 : uNewCap*2;
    }
    while ((m_uCapVertices+uGrow) >= uNewCap);
    
    UINT uVertexStride = m_uVertexStride;

    // ensure that the buffer does not get too big
    // 0xFFFF is reserved by the tesselator
    if (uNewCap >= 0xFFFF)
    {
        IFC(E_INVALIDARG);
    }
    
    IFC(WPFRealloc(ProcessHeap,
                        Mt(D3DVertexBufferVertices),
                        (void **)&m_pVertices,
                        uVertexStride * uNewCap ));

    ZeroMemory(((PBYTE)m_pVertices) + m_uCapVertices*m_uVertexStride,
               m_uVertexStride * (uNewCap - m_uCapVertices));

    m_uCapVertices = uNewCap;

    Assert(m_uCapVertices < MAXRENDERVERTICES);

Cleanup:
    RRETURN(hr);
}



