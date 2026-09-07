// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_lighting
//      $Keywords:
//
//  $Description:
//      Contains the CHw3DGeometryRenderer implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------
#include "precomp.hpp"
using namespace dxlayer;

MtDefine(CHw3DGeometryRenderer, MILRender, "CHw3DGeometryRenderer");
MtDefine(CHwD3DVertexBuffer, MILRender, "CHw3DGeometryRenderer");
MtDefine(CHwD3DIndexBuffer, MILRender, "CHw3DGeometryRenderer");

MtDefine(D3DResource_VertexBuffer, MILHwMetrics, "Approximate vertex buffer size");
MtDefine(D3DResource_IndexBuffer, MILHwMetrics, "Approximate index buffer size");

MtExtern(D3DVertexBuffer);
MtExtern(D3DIndexBuffer);

template <class T>
struct VertexDataTypeTraits;

template <>
struct VertexDataTypeTraits<vector3>
{
    enum 
    { 
        D3DFVF = D3DFVF_NORMAL,
        MILVF = MILVFAttrNormal
    };
};

template <>
struct VertexDataTypeTraits<DWORD>
{
    enum 
    { 
        D3DFVF = D3DFVF_DIFFUSE,
        MILVF = MILVFAttrDiffuse
    };
};

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DBufferSpaceLocator::ctor
//
//  Synopsis:
//      Initializes class.
//
//------------------------------------------------------------------------------
CHwD3DBufferSpaceLocator::CHwD3DBufferSpaceLocator(
    UINT uNumBytes
    )
{
    m_uBufferByteCapacity = uNumBytes;
    m_uCurrentByteInBuffer = 0;
    m_uNumBytesInLatestChunk = 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DBufferSpaceLocator::AdvanceToNextChunk
//
//  Synopsis:
//      Advances the buffer position in order to hold the number of elements
//      required.  If there isn't enough space at the end it will discard the
//      buffer and start it over at the beginning.
//
//------------------------------------------------------------------------------
void
CHwD3DBufferSpaceLocator::AdvanceToNextChunk(
    UINT cElementsRequired,
    UINT uElementSize,
    __out_ecount(1) DWORD * const pdwLockFlags,
    __out_ecount(1) UINT * const puStartElement
    )
{
    Assert(cElementsRequired*uElementSize <= m_uBufferByteCapacity);

    //
    // Move the current position forward
    //
    // We need to move to a byte value that's a multiple of the new element size.
    //
    // To do that we take the CurrentByteValue and add (uElementSize - CurrentByteValue%uElementSize).
    //
    m_uCurrentByteInBuffer += m_uNumBytesInLatestChunk;

    if (m_uCurrentByteInBuffer%uElementSize != 0)
    {
        m_uCurrentByteInBuffer += uElementSize - m_uCurrentByteInBuffer%uElementSize;
    }

    m_uNumBytesInLatestChunk = cElementsRequired*uElementSize;

    m_uNumBytesPerElementInLatestChunk = uElementSize;

    //
    // If there's enough space from the vertex buffer position to the end of the
    // buffer to hold the vertices, append these vertices to the end of the
    // previous.  Otherwise discard the current buffer and begin writing at the
    // beginning.
    //

    if (m_uNumBytesInLatestChunk + m_uCurrentByteInBuffer <= m_uBufferByteCapacity)
    {
        *pdwLockFlags = D3DLOCK_NOOVERWRITE;
    }
    else
    {
        *pdwLockFlags = D3DLOCK_DISCARD;
        m_uCurrentByteInBuffer = 0;
    }

    Assert(m_uCurrentByteInBuffer%m_uNumBytesPerElementInLatestChunk == 0);

    *puStartElement = m_uCurrentByteInBuffer/m_uNumBytesPerElementInLatestChunk;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DBufferSpaceLocator::GetNextUsableSize
//
//  Synopsis:
//      Retrieves the next usable size.  Will try to return the unused portion
//      at the end of the buffer, but if there aren't at least 3 elements, it
//      returns the full size of the buffer.
//
//------------------------------------------------------------------------------
UINT
CHwD3DBufferSpaceLocator::GetNextUsableNumberOfElements(
    UINT uElementSize
    ) const
{
    UINT uRemaining = GetNumberOfElementsAfterCurrentChunk(uElementSize);

    //
    //
    // This could have an impact on our performance scenarios.  Moving usable
    // size to something more realistic...like 90 or so...will probably change
    // performance for the better since sending a few triangles at a time is
    // highly inefficient.
    //
        
    if (uRemaining < 3)
    {
        uRemaining = m_uBufferByteCapacity/uElementSize;
    }

    //
    // It's possible that we could have a non-multiple of 3 elements available
    // in the buffer.  But depending on the rendering technique used we may
    // need it to be a multiple of 3.  To avoid the problem for now we just
    // always make sure it's a multiple of 3.  This is sub-optimal.
    //
    return (uRemaining - uRemaining%3);
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DVertexBuffer::Create
//
//  Synopsis:
//      Initializes class.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT
CHwD3DVertexBuffer::Create(
    __inout_ecount(1) CD3DResourceManager *pResourceManager, 
    __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
    UINT uCapacity,
    __deref_out_ecount(1) CHwD3DVertexBuffer ** const ppVertexBuffer
    )
{
    HRESULT hr = S_OK;
    CHwD3DVertexBuffer *pNewVertexBuffer = NULL;
    
    *ppVertexBuffer = NULL;

    pNewVertexBuffer = new CHwD3DVertexBuffer(uCapacity);
    IFCOOM(pNewVertexBuffer);

    pNewVertexBuffer->AddRef();

    IFC(pNewVertexBuffer->Init(
        pResourceManager,
        pD3DDevice
        ));

    *ppVertexBuffer = pNewVertexBuffer;
    pNewVertexBuffer = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewVertexBuffer);
    
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DVertexBuffer::ctor
//
//  Synopsis:
//      Initializes class.
//
//------------------------------------------------------------------------------
CHwD3DVertexBuffer::CHwD3DVertexBuffer(
    UINT uCapacity
    )
    : CHwD3DBufferSpaceLocator(uCapacity)
{
    m_pVertexBuffer = NULL;
    m_fLocked = false;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DVertexBuffer::dtor
//
//  Synopsis:
//      Releases the vertex buffer.
//
//------------------------------------------------------------------------------
CHwD3DVertexBuffer::~CHwD3DVertexBuffer()
{
    ReleaseInterfaceNoNULL(m_pVertexBuffer);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DVertexBuffer::Init
//
//  Synopsis:
//      Initializes the buffer to the appropriate size.
//
//------------------------------------------------------------------------------
HRESULT
CHwD3DVertexBuffer::Init(
    __inout_ecount(1) CD3DResourceManager *pResourceManager, 
    __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice
    )
{
    HRESULT hr = S_OK;
    
    IFC(pD3DDevice->CreateVertexBuffer(
        GetCapacity(),
        D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
        0,
        D3DPOOL_DEFAULT,
        &m_pVertexBuffer
        ));

    CD3DResource::Init(
        pResourceManager,
        GetCapacity()
        );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DVertexBuffer::ReleaseD3DResources
//
//  Synopsis:
//      Release the vertex buffer.
//
//      This method may only be called by CD3DResourceManager because there are
//      restrictions around when a call to ReleaseD3DResources is okay.
//
//------------------------------------------------------------------------------
void
CHwD3DVertexBuffer::ReleaseD3DResources()
{
    // This resource should have been marked invalid already or at least be out
    // of use.
    Assert(!m_fResourceValid || (m_cRef == 0));
    Assert(IsValid() == m_fResourceValid);
    Assert(m_fLocked == false);

    ReleaseInterface(m_pVertexBuffer);

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DVertexBuffer::Lock
//
//  Synopsis:
//      Locks the vertex buffer.
//
//------------------------------------------------------------------------------
HRESULT
CHwD3DVertexBuffer::Lock(
    UINT cVertices,
    UINT uVertexStride,
    __deref_out_bcount_part(cVertices * uVertexStride, 0) void ** const ppLockedVertices,
    __out_ecount(1) UINT * const puStartVertex
    )
{
    HRESULT hr = S_OK;
    DWORD dwLockFlags = 0;
    
    Assert(m_fLocked == false);

    if (cVertices > GetMaximumCapacity(uVertexStride))
    {
        // We are larger than the buffer size, so fail.  This will cause us
        // to fall back to DrawPrimitiveUP

        IFC(WGXERR_INSUFFICIENTBUFFER);
    }

    AdvanceToNextChunk(
        cVertices,
        uVertexStride,
        &dwLockFlags,
        puStartVertex
        );

    IFC(m_pVertexBuffer->Lock(
        GetCurrentBytePos(),
        GetNumBytesInLastChunk(),
        ppLockedVertices,
        dwLockFlags
        ));

    //
    // Some D3D HALs can return success and a NULL address.  D3D runtime will
    // happily accept the NULL address, add the lock offset, and return the bad
    // address.  So, check for this hole and interpret it as failure.
    //
    // On Win7 it appears that there are situations where we can just
    // get the NULL address back directly even though the call succeeds, perhaps 
    // due to change in the D3D runtime, so check for that case as well.
    //
    // Warning: When using a checked D3D runtime and D3DLOCK_DISCARD flag, D3D
    //          will blindly memset the buffer causing an access violation --
    //          See d3d9!CDriverVertexBuffer::Lock
    //

    if (   reinterpret_cast<UINT_PTR>(*ppLockedVertices) == static_cast<UINT_PTR>(GetCurrentBytePos())
        || *ppLockedVertices == NULL)
    {
        IGNORE_HR(m_pVertexBuffer->Unlock());
        *ppLockedVertices = NULL;
        IFC(D3DERR_DRIVERINTERNALERROR);
    }

    m_fLocked = true;

Cleanup:
    RRETURN(hr);
}
    
//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DVertexBuffer::Unlock
//
//  Synopsis:
//      Unlocks the vertex buffer.
//
//------------------------------------------------------------------------------
HRESULT
CHwD3DVertexBuffer::Unlock(
    UINT cVerticesUsed
    )
{
    HRESULT hr = S_OK;
    
    Assert(m_fLocked == true);

    IFC(m_pVertexBuffer->Unlock());

    ReportNumberOfElementsUsedInLastChunk(cVerticesUsed);

    m_fLocked = false;

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DIndexBuffer::Create
//
//  Synopsis:
//      Create call.
//
//------------------------------------------------------------------------------
__checkReturn HRESULT 
CHwD3DIndexBuffer::Create(
    __inout_ecount(1) CD3DResourceManager *pResourceManager, 
    __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
    UINT uCapacity,
    __deref_out_ecount(1) CHwD3DIndexBuffer ** const ppIndexBuffer
    )
{
    HRESULT hr = S_OK;
    CHwD3DIndexBuffer *pNewIndexBuffer = NULL;

    pNewIndexBuffer = new CHwD3DIndexBuffer(uCapacity);
    IFCOOM(pNewIndexBuffer);

    pNewIndexBuffer->AddRef();

    IFC(pNewIndexBuffer->Init(
        pResourceManager,
        pD3DDevice
        ));

    *ppIndexBuffer = pNewIndexBuffer;
    pNewIndexBuffer = NULL;
    
Cleanup:
    ReleaseInterfaceNoNULL(pNewIndexBuffer);
    
    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DIndexBuffer::ctor
//
//  Synopsis:
//      Initializes class.
//
//------------------------------------------------------------------------------
CHwD3DIndexBuffer::CHwD3DIndexBuffer(
    UINT uCapacity
    )
    : CHwD3DBufferSpaceLocator(uCapacity)
{
    m_pIndexBuffer = NULL;
    m_fLocked = false;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DIndexBuffer::dtor
//
//  Synopsis:
//      Releases the vertex buffer.
//
//------------------------------------------------------------------------------
CHwD3DIndexBuffer::~CHwD3DIndexBuffer()
{
    ReleaseInterfaceNoNULL(m_pIndexBuffer);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DIndexBuffer::Init
//
//  Synopsis:
//      Initializes the buffer to the appropriate size.
//
//------------------------------------------------------------------------------
HRESULT
CHwD3DIndexBuffer::Init(
    __inout_ecount(1) CD3DResourceManager *pResourceManager, 
    __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice
    )
{
    HRESULT hr = S_OK;
    
    IFC(pD3DDevice->CreateIndexBuffer(
        GetCapacity(),
        D3DUSAGE_WRITEONLY | D3DUSAGE_DYNAMIC,
        D3DFMT_INDEX16,
        D3DPOOL_DEFAULT,
        &m_pIndexBuffer
        ));

    CD3DResource::Init(
        pResourceManager,
        GetCapacity()
        );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DIndexBuffer::ReleaseD3DResources
//
//  Synopsis:
//      Release the vertex buffer.
//
//      This method may only be called by CD3DResourceManager because there are
//      restrictions around when a call to ReleaseD3DResources is okay.
//
//------------------------------------------------------------------------------
void
CHwD3DIndexBuffer::ReleaseD3DResources()
{
    // This resource should have been marked invalid already or at least be out
    // of use.
    Assert(!m_fResourceValid || (m_cRef == 0));
    Assert(IsValid() == m_fResourceValid);
    Assert(m_fLocked == false);

    ReleaseInterface(m_pIndexBuffer);

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DIndexBuffer::Lock
//
//  Synopsis:
//      Locks the index buffer.
//
//------------------------------------------------------------------------------
HRESULT
CHwD3DIndexBuffer::Lock(
    UINT cIndices,
    __deref_out_ecount_part(cIndices, 0) WORD ** const ppwLockedIndices,
    __out_ecount(1) UINT *puStartIndex
    )
{
    HRESULT hr = S_OK;
    DWORD dwLockFlags = 0;
    
    Assert(m_fLocked == false);

    if (cIndices > GetMaximumCapacity(sizeof(WORD)))
    {
        // We are larger than the buffer size, so fail.  This will cause us
        // to fall back to DrawPrimitiveUP

        IFC(WGXERR_INSUFFICIENTBUFFER);
    }

    AdvanceToNextChunk(
        cIndices,
        sizeof(WORD),
        &dwLockFlags,
        puStartIndex
        );

    IFC(m_pIndexBuffer->Lock(
        GetCurrentBytePos(),
        GetNumBytesInLastChunk(),
        reinterpret_cast<void **>(ppwLockedIndices),
        dwLockFlags
        ));

    //
    // Some D3D HALs can return success and a NULL address.  D3D runtime will
    // happily accept the NULL address, add the lock offset, and return the bad
    // address.  So, check for this hole and interpret it as failure.
    //
    // On Win7 it appears that there are situations where we can just
    // get the NULL address back directly even though the call succeeds, perhaps 
    // due to change in the D3D runtime, so check for that case as well.
    //
    // Warning: When using a checked D3D runtime and D3DLOCK_DISCARD flag, D3D
    //          will blindly memset the buffer causing an access violation --
    //          See d3d9!CDriverVertexBuffer::Lock
    //

    if (   reinterpret_cast<UINT_PTR>(*ppwLockedIndices) == static_cast<UINT_PTR>(GetCurrentBytePos())
        || *ppwLockedIndices == NULL)
    {
        IGNORE_HR(m_pIndexBuffer->Unlock());
        *ppwLockedIndices = NULL;
        IFC(D3DERR_DRIVERINTERNALERROR);
    }

    m_fLocked = true;

Cleanup:
    RRETURN(hr);
}
    
//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DIndexBuffer::Unlock
//
//  Synopsis:
//      Unlocks the index buffer.
//
//------------------------------------------------------------------------------
HRESULT
CHwD3DIndexBuffer::Unlock()
{
    HRESULT hr = S_OK;
    
    Assert(m_fLocked == true);

    IFC(m_pIndexBuffer->Unlock());

    m_fLocked = false;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwD3DIndexBuffer::CopyFromInputBuffer
//
//  Synopsis:
//      Copies indices over to the d3d index buffer.
//
//      Note: We cannot reorder the indices any differently than their
//            order in the input stream.  Doing so would render triangles
//            in different orders and violate our rendering rules.
//
//------------------------------------------------------------------------------
HRESULT 
CHwD3DIndexBuffer::CopyFromInputBuffer(
    __in_ecount(cIndices) const UINT *puIndexStream,
    UINT cIndices,
    __out_ecount(1) UINT * const puStartIndex
    )
{
    HRESULT hr = S_OK;
    WORD *pwLockedIndices = NULL;
    
    IFC(Lock(
        cIndices,
        &pwLockedIndices,
        puStartIndex
        ));

    //
    // Copy indices directly into the buffer.
    //

    for (UINT i = 0; i < cIndices; i++)
    {
        pwLockedIndices[i] = static_cast<WORD>(puIndexStream[i]);

        // Verify that the truncation from UINT to WORD above did not
        // loose any information.
        Assert(pwLockedIndices[i] == puIndexStream[i]);
    }

Cleanup:
    if (m_fLocked)
    {
        HRESULT hr2 = THR(Unlock());

        if (SUCCEEDED(hr))
        {
            hr = hr2;
        }
    }
    
    RRETURN(hr);
}





//+-----------------------------------------------------------------------------
//
//  Member:
//      CHw3DGeometryRenderer::ctor
//
//  Synopsis:
//      Initializes the CHw3DGeometryRenderer data
//
//------------------------------------------------------------------------------
template <class TDiffuseOrNormal>
CHw3DGeometryRenderer<TDiffuseOrNormal>::CHw3DGeometryRenderer(
    __in_ecount(1) CMILLightData * const pLightData,
    __in_ecount(1) CD3DDeviceLevel1 * const pDeviceNoRef
    ) : m_pLightData(pLightData), m_pDeviceNoRef(pDeviceNoRef)
{
    m_pInputPositionStream = NULL;
    m_pInputDiffuseOrNormalStream = NULL;
    m_pInputTextureCoordinateStream = NULL;
    m_pInputIndexStream = NULL;

    m_cInputVertices = 0;
    m_cInputIndices = 0;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHw3DGeometryRenderer::SetStreams
//
//  Synopsis:
//      Sets the input for rendering.  Will determine if it can be rendered as
//      an indexed or non-indexed primitive.
//
//------------------------------------------------------------------------------

template <class TDiffuseOrNormal>
void
CHw3DGeometryRenderer<TDiffuseOrNormal>::SetArrays(
    __in_ecount(cVertices) const vector3 *pvec3PositionStream,
    __in_ecount_opt(cVertices) const TDiffuseOrNormal *ptDiffuseOrNormalStream,
    __in_ecount(cVertices) const vector2 *pvec2TextureCoordinateStream,
    UINT cVertices,
    __in_ecount_opt(cIndices) const UINT *puIndexStream,
    UINT cIndices
    )
{
    Assert(m_pInputPositionStream == NULL);
    Assert(m_pInputTextureCoordinateStream == NULL);
    Assert(m_pInputIndexStream == NULL);

    Assert(cIndices % 3 == 0);
    Assert(cVertices > 0);

    m_pInputPositionStream = pvec3PositionStream;
    m_pInputDiffuseOrNormalStream = ptDiffuseOrNormalStream;
    m_pInputTextureCoordinateStream = pvec2TextureCoordinateStream;
    m_cInputVertices = cVertices;

    m_pInputIndexStream = puIndexStream;
    m_cInputIndices = cIndices;

    m_uRenderedIndices = 0;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHw3DGeometryRenderer::PrepareIndexed
//
//  Synopsis:
//      Prepares the class for rendering in indexed primitive mode.
//
//------------------------------------------------------------------------------
template <class TDiffuseOrNormal>
HRESULT
CHw3DGeometryRenderer<TDiffuseOrNormal>::PrepareIndexed(
    __out_ecount(1) UINT * const puStartVertex,
    __out_ecount(1) UINT * const puStartIndex,
    __out_ecount(1) UINT * const pcPrimitives,
    __out_ecount(1) bool * const pfNeedsToRender,
    __in_ecount(1) CHwD3DVertexBuffer *pVertexBuffer,
    __in_ecount(1) CHwD3DIndexBuffer *pIndexBuffer
    )
{
    HRESULT hr = S_OK;

    void *pLockedOutputVertices = NULL;

    UINT cIndicesToCopy = 0;
    UINT cVerticesToCopy = 0;
    
    Assert(m_cInputIndices % 3 == 0);

    *pfNeedsToRender = true;

    //
    // If we haven't loaded the vertices into the buffer, do that first.
    //

    if (m_uRenderedIndices == 0)
    {
        //
        // Grab vertices in the vertex buffer
        //

        IFC(pVertexBuffer->Lock(
            m_cInputVertices,
            GetVertexStride(),
            &pLockedOutputVertices,
            puStartVertex
            ));

        //
        // Copy our stream data into the vertex buffer.
        //
        CopyVerticesIntoBuffer(
            pLockedOutputVertices,
            m_cInputVertices
            );

        cVerticesToCopy = m_cInputVertices;
    }

    //
    // Now since this is the first attempted render for the indexed
    // primitive we see if we should see if there is space at the end of the
    // indices for us to tag our indices off of.
    //
    cIndicesToCopy = pIndexBuffer->GetNextUsableNumberOfElements(sizeof(WORD));

    //
    // Check to see if we've got more space than we need.
    //
    if (cIndicesToCopy > RemainingIndices())
    {
        cIndicesToCopy = RemainingIndices();

        //
        // If there aren't any indices left to copy, we're done.
        //
        if (cIndicesToCopy == 0)
        {
            *pfNeedsToRender = false;
            goto Cleanup;
        }
    }

    //
    // Number of indices should always be a multiple of 3 since we're rendering
    // triangles.
    //
    Assert(cIndicesToCopy%3 == 0);

    IFC(pIndexBuffer->CopyFromInputBuffer(
        &m_pInputIndexStream[m_uRenderedIndices],
        cIndicesToCopy,
        puStartIndex
        ));


    m_uRenderedIndices += cIndicesToCopy;

    *pcPrimitives = cIndicesToCopy/3;

Cleanup:
    if (pVertexBuffer->Locked())
    {
        HRESULT hr2 = THR(pVertexBuffer->Unlock(m_cInputVertices));
        
        if (SUCCEEDED(hr))
        {
            hr = hr2;
        }
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHw3DGeometryRenderer::PrepareNonIndexed
//
//  Synopsis:
//      Prepares the class for rendering in non-indexed primitive mode.
//
//------------------------------------------------------------------------------
template <class TDiffuseOrNormal>
HRESULT
CHw3DGeometryRenderer<TDiffuseOrNormal>::PrepareNonIndexed(
    __out_ecount(1) UINT * const puStartVertex,
    __out_ecount(1) UINT * const pcPrimitives,
    __out_ecount(1) bool * const pfNeedsToRender,
    __in_ecount(1) CHwD3DVertexBuffer *pVertexBuffer
    )
{
    HRESULT hr = S_OK;
    
    UINT cVerticesToCopy = 0;
    void *pLockedOutputVertices = NULL;

    *pfNeedsToRender = true;

    //
    // Since we have a mesh that's too big to fit all in one render, it makes
    // sense to fill the vertex buffer completely and render that.  But if 
    // there's still space in the existing buffer on our first filling we would 
    // rather use that first to avoid a discard.
    //
    cVerticesToCopy = pVertexBuffer->GetNextUsableNumberOfElements(GetVertexStride());

    //
    // Check to see if we've got more space than we need.
    //
    if (cVerticesToCopy > RemainingIndices())
    {
        cVerticesToCopy = RemainingIndices();

        //
        // If we're down to 0 vertices left, it means we've fully rendered the
        // mesh.
        //
        if (cVerticesToCopy == 0)
        {
            *pfNeedsToRender = false;
            goto Cleanup;
        }
    }

    //
    // Grab the vertices in the buffer.
    //
    IFC(pVertexBuffer->Lock(
        cVerticesToCopy,
        GetVertexStride(),
        &pLockedOutputVertices,
        puStartVertex
        ));

    //
    // Copy the indexed triangles into the vertex buffer.
    //
    CopyIndexOrderedVerticesIntoBuffer(
        pLockedOutputVertices,
        m_uRenderedIndices,
        cVerticesToCopy
        );

    //
    // We're rendering non-indexed, so the number of vertices we render
    // sould be a multiple of 3 since we're rendering triangles.
    //
    Assert(cVerticesToCopy%3 == 0);

    *pcPrimitives = cVerticesToCopy/3;

    m_uRenderedIndices += cVerticesToCopy;

Cleanup:
    if (pVertexBuffer->Locked())
    {
        HRESULT hr2 = THR(pVertexBuffer->Unlock(cVerticesToCopy));
        
        if (SUCCEEDED(hr))
        {
            hr = hr2;
        }
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHw3DGeometryRenderer::CopyVerticesIntoBuffer
//
//  Synopsis:
//      Copies triangle vertices into the vertex buffer.
//
//      Note: We cannot reorder the vertices any differently than their
//            natural order in the inputstreams.  Doing so would cause
//            indices to refer to the wrong vertices.
//
//------------------------------------------------------------------------------
template <class TDiffuseOrNormal>
void
CHw3DGeometryRenderer<TDiffuseOrNormal>::CopyVerticesIntoBuffer(
    __out_xcount(cVerticesToCopy*GetVertexStride()) void * const pCardVertexBuffer,
    UINT cVerticesToCopy
    )
{
    DWORD dwVertexStride = GetVertexStride();
    
    Assert(m_pInputPositionStream);
    Assert(m_pInputTextureCoordinateStream);
    Assert(m_pInputIndexStream);

    Assert(cVerticesToCopy <= m_cInputVertices);

    //
    // Gather pointers to each of the subelements of the vertex
    //

    BYTE *pbPosition        = reinterpret_cast<BYTE *>(pCardVertexBuffer);
    BYTE *pbDiffuseOrNormal = pbPosition + sizeof(vector3);   
    BYTE *pbTextureCoord    = pbDiffuseOrNormal + sizeof(TDiffuseOrNormal);

    //
    // Input Light values are optional, if there aren't any then we default to Solid White
    //

    const TDiffuseOrNormal *ptInputDiffuseOrNormalArray = NULL;

    //
    // A multiplier for the vertex index.  This let's us use either an array of light
    // values, or a single value.
    //
    DWORD dwLightVertexIndexMult;

    if (m_pInputDiffuseOrNormalStream)
    {
        ptInputDiffuseOrNormalArray = m_pInputDiffuseOrNormalStream;
        dwLightVertexIndexMult = 1;
    }
    else
    {
        ptInputDiffuseOrNormalArray = &m_defaultDiffuseOrNormal;
        dwLightVertexIndexMult = 0;
    }

    //
    // Iterate through all the vertices, combining the streams together into
    // single packed vertices.
    //
    for (UINT i = 0; i < cVerticesToCopy; i++)
    {
        auto *pvec3Position = reinterpret_cast<vector3*>(pbPosition);
        TDiffuseOrNormal *ptDiffuseOrNormal = reinterpret_cast<TDiffuseOrNormal *>(pbDiffuseOrNormal);

        auto *pvec2TextureCoord 
            = reinterpret_cast<vector2 *>(pbTextureCoord);

        //
        // Pack seperate streams into vertex
        //
        *pvec3Position     = m_pInputPositionStream[i];
        *ptDiffuseOrNormal = ptInputDiffuseOrNormalArray[i*dwLightVertexIndexMult];
        *pvec2TextureCoord = m_pInputTextureCoordinateStream[i];

        pbPosition        += dwVertexStride;
        pbDiffuseOrNormal += dwVertexStride;
        pbTextureCoord    += dwVertexStride;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHw3DGeometryRenderer::CopyIndexOrderedVerticesIntoBuffer
//
//  Synopsis:
//      Copies triangle vertices into the vertex buffer based on the indexed
//      ordering.  This is done so we can render the 3d vertices without
//      indexing information.
//
//      When there isn't an index stream, we're implicitly using an index stream
//      of 1,2,3,...
//
//      Note: We cannot reorder the vertices any differently than their
//            order defined by the index buffer.  Doing so would render
//            triangles in different orders and violate our rendering
//            rules.
//
//------------------------------------------------------------------------------
template <class TDiffuseOrNormal>
void
CHw3DGeometryRenderer<TDiffuseOrNormal>::CopyIndexOrderedVerticesIntoBuffer(
    __out_xcount(cIndicesToCopy*GetVertexStride()) void * const pCardVertexBuffer,
    UINT uInputIndexStart,
    UINT cIndicesToCopy
    )
{
    UINT uCurrentVertex;
    DWORD dwVertexStride = GetVertexStride();

    Assert(uInputIndexStart < m_cInputIndices);
    Assert(cIndicesToCopy <= m_cInputIndices);
    Assert(cIndicesToCopy % 3 == 0);
    Assert(uInputIndexStart % 3 == 0);

    //
    // Gather pointers to each of the subelements of the vertex
    //
    BYTE *pbPosition        = reinterpret_cast<BYTE *>(pCardVertexBuffer);
    BYTE *pbDiffuseOrNormal = pbPosition + sizeof(vector3);   
    BYTE *pbTextureCoord    = pbDiffuseOrNormal + sizeof(TDiffuseOrNormal);

    //
    // Input Light values are optional, if there aren't any then we default to Solid White
    //

    const TDiffuseOrNormal *ptInputDiffuseOrNormalArray = NULL;

    //
    // A multiplier for the vertex index.  This let's us use either an array of light
    // values, or a single value.
    //
    DWORD dwLightVertexIndexMult;

    if (m_pInputDiffuseOrNormalStream)
    {
        ptInputDiffuseOrNormalArray = m_pInputDiffuseOrNormalStream;
        dwLightVertexIndexMult = 1;
    }
    else
    {
        ptInputDiffuseOrNormalArray = &m_defaultDiffuseOrNormal;
        dwLightVertexIndexMult = 0;
    }

    //
    // Iterate through all the indices, determine which vertex it is refering to
    // and combine that vertex's elements into a single packed vertex.
    //
    for (UINT i = 0; i < cIndicesToCopy; i++)
    {
        if (m_pInputIndexStream != NULL)
        {
            uCurrentVertex = m_pInputIndexStream[uInputIndexStart + i];
        }
        else
        {
            uCurrentVertex = uInputIndexStart + i;
        }

        auto *pvec3Position = reinterpret_cast<vector3 *>(pbPosition);
        TDiffuseOrNormal *ptDiffuseOrNormal  = reinterpret_cast<TDiffuseOrNormal *>(pbDiffuseOrNormal);

        auto *pvec2TextureCoord 
            = reinterpret_cast<vector2 *>(pbTextureCoord);

        //
        // Pack seperate streams into vertex
        //
        *pvec3Position     = m_pInputPositionStream[uCurrentVertex];
        *ptDiffuseOrNormal = ptInputDiffuseOrNormalArray[uCurrentVertex*dwLightVertexIndexMult];
        *pvec2TextureCoord = m_pInputTextureCoordinateStream[uCurrentVertex];

        pbPosition        += dwVertexStride;
        pbDiffuseOrNormal += dwVertexStride;
        pbTextureCoord    += dwVertexStride;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHw3DGeometryRenderer::SendDeviceState
//
//  Synopsis:
//      Sets state on the device for rendering.
//
//------------------------------------------------------------------------------
template <class TDiffuseOrNormal>
HRESULT 
CHw3DGeometryRenderer<TDiffuseOrNormal>::SendDeviceState(
    bool fIndexed,
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) const CHwD3DVertexBuffer *pVertexBuffer,
    __in_ecount(1) const CHwD3DIndexBuffer *pIndexBuffer
    )
{
    HRESULT hr = S_OK;

    //
    // Invalidate FVF so it will be reset next time
    //
    DWORD dwFVF = D3DFVF_XYZ | VertexDataTypeTraits<TDiffuseOrNormal>::D3DFVF | D3DFVF_TEX1;

    IFC(pDevice->SetFVF(dwFVF));

    //
    // NOTE-2004/09/21-chrisra Sending only 1 stream is more performant.
    //
    // Perf testing showed that packing the data into a single stream is more
    // performant than keeping the data in seperate streams down to the card.
    //

    IFC(pDevice->SetStreamSource(
        pVertexBuffer->GetD3DBuffer(),
        GetVertexStride()
        ));
        
    if (fIndexed)
    {
        IFC(pDevice->SetIndices(
            pIndexBuffer->GetD3DBuffer()
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHw3DGeometryRenderer::Render
//
//  Synopsis:
//      Renders the mesh.
//
//------------------------------------------------------------------------------
template <class TDiffuseOrNormal>
HRESULT
CHw3DGeometryRenderer<TDiffuseOrNormal>::Render(
    __in_ecount(1) const CMILMesh3D *pMesh3D,
    __in_bcount_opt(cbDiffuseColorsOrNormals) const TDiffuseOrNormal *ptDiffuseColorsOrNormals,
    size_t cbDiffuseColorsOrNormals,
    __in_ecount(1) const TDiffuseOrNormal &defaultDiffuseOrNormal,
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice
    )
{
    HRESULT hr = S_OK;

#if DBG
    UINT dbgNumDrawPrims = 0;
#endif

    CHwD3DVertexBuffer *pVertexBuffer = NULL;
    CHwD3DIndexBuffer  *pIndexBuffer = NULL;
    
    UINT uVertexStart;
    UINT uIndexStart;
    UINT uNumPrimitives;
    bool fIndexed = false;

    m_defaultDiffuseOrNormal = defaultDiffuseOrNormal;

    pVertexBuffer = pDevice->Get3DVertexBuffer();
    pIndexBuffer = pDevice->Get3DIndexBuffer();

    //
    // If the vertex count exceeds the max buffer size, we choose not to draw as indexed
    // because perhaps one of the indices would point to a vertex that we weren't able
    // to copy in. I think we could actually make this happen if we scanned a "window"
    // of indices to get the "index bounds" and break to DrawPrim whenever the index
    // bounds hits the vertex capacity but you can imagine situations where the 
    // performance would be terrible.
    //
    if (pMesh3D->GetNumVertices() > pVertexBuffer->GetMaximumCapacity(GetVertexStride()) || 
        pMesh3D->GetNumIndices() == 0)
    {
        fIndexed = false;
    }
    else
    {
        fIndexed = true;
    }

    const vector3 *pvec3Positions;
    const vector2 *pvec2TexCoords;
    const UINT *puIndices;
    size_t dontCare; // don't care because we pass num verts, num indices to SetArrays
                     // but we'll assert in case the impl changes

    pMesh3D->GetPositions(pvec3Positions, dontCare);
    Assert(dontCare / sizeof(pvec3Positions[0]) == pMesh3D->GetNumVertices());
    pMesh3D->GetIndices(puIndices, dontCare);
    Assert(dontCare / sizeof(puIndices[0]) == pMesh3D->GetNumIndices());
    pMesh3D->GetTextureCoordinates(pvec2TexCoords, dontCare);
    Assert(dontCare / sizeof(pvec2TexCoords[0]) == pMesh3D->GetNumVertices());

    // If a mesh has no indices, we will draw a triangle every three vertices. A mesh 
    // with no indices is like a mesh with an index array of [0, 1, 2, ..., numVerts - 1].
    // Thus, in the non-indexed case, we will say that we have "numVerts" indices and
    // then later in CopyIndexedOrderedVerticesIntoBuffer we will be sure not to
    // index into puIndices.
    UINT uNumIndices = pMesh3D->GetNumIndices();
    if (uNumIndices == 0)
    {
        Assert(puIndices == NULL);
        uNumIndices = pMesh3D->GetNumVertices();
    }

    SetArrays(
        pvec3Positions, 
        ptDiffuseColorsOrNormals,
        pvec2TexCoords,
        pMesh3D->GetNumVertices(), 
        puIndices, 
        uNumIndices
        );
    
    IFC(SendDeviceState(
        fIndexed,
        pDevice,
        pVertexBuffer,
        pIndexBuffer
        ));

    if (fIndexed)
    {
        bool fNeedsToRender;

        for (;;)
        {
            IFC(PrepareIndexed(
                &uVertexStart,
                &uIndexStart, 
                &uNumPrimitives,
                &fNeedsToRender,
                pVertexBuffer,
                pIndexBuffer
                ));

            if (!fNeedsToRender)
            {
                break;
            }
            
            IFC(pDevice->DrawIndexedTriangleList(
                uVertexStart,
                0,
                pMesh3D->GetNumVertices(),
                uIndexStart,
                uNumPrimitives
                ));

#if DBG
            ++dbgNumDrawPrims;
#endif     
        }
            
    }
    else
    {
        bool fNeedsToRender;
        
        for (;;)
        {
            IFC(PrepareNonIndexed(
                &uVertexStart,
                &uNumPrimitives,
                &fNeedsToRender,
                pVertexBuffer
                ));

            if (!fNeedsToRender)
            {
                break;
            }

            IFC(pDevice->DrawTriangleList(
                uVertexStart,
                uNumPrimitives
                ));

#if DBG
            ++dbgNumDrawPrims;
#endif     
        }
    }

Cleanup:    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHw3DGeometryRenderer::GetPerVertexDataType
//
//  Synopsis:
//      Return vertex fields that are generated when this is used
//
//------------------------------------------------------------------------------
template <class TDiffuseOrNormal>
void
CHw3DGeometryRenderer<TDiffuseOrNormal>::GetPerVertexDataType(
    __out_ecount(1) MilVertexFormat &mvfFullyGenerated
    ) const
{
    mvfFullyGenerated = MILVFAttrXYZ | VertexDataTypeTraits<TDiffuseOrNormal>::MILVF | MILVFAttrUV1;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHw3DGeometryRenderer::SendGeometry
//
//  Synopsis:
//      The Hw3DGeometryRenderer doesn't send geometry to a geometrysink. It
//      will just return S_OK.
//
//------------------------------------------------------------------------------
template <class TDiffuseOrNormal>
HRESULT
CHw3DGeometryRenderer<TDiffuseOrNormal>::SendGeometry(
    IN IGeometrySink *pGeomSink
    )
{
    UNREFERENCED_PARAMETER(pGeomSink);

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHw3DGeometryRenderer::SendGeometryModifiers
//
//  Synopsis:
//      Add a blend diffuse colors operation to the pipeline.
//
//------------------------------------------------------------------------------
template <class TDiffuseOrNormal>
HRESULT
CHw3DGeometryRenderer<TDiffuseOrNormal>::SendGeometryModifiers(
    __inout_ecount(1) CHwPipelineBuilder *pPipelineBuilder
    )
{
    UNREFERENCED_PARAMETER(pPipelineBuilder);

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHw3DGeometryRenderer::SendLighting
//
//  Synopsis:
//      Creates a Lighting color source and adds it to the pipeline.
//
//------------------------------------------------------------------------------
template <class TDiffuseOrNormal>
HRESULT
CHw3DGeometryRenderer<TDiffuseOrNormal>::SendLighting(
    __inout_ecount(1) CHwPipelineBuilder *pPipelineBuilder
    )
{
    HRESULT hr = S_OK;
    CHwLightingColorSource *pLightingColorSource = NULL;

    IFC(CHwLightingColorSource::Create(
        m_pLightData,
        &pLightingColorSource
        ));

    IFC(pPipelineBuilder->Add_Lighting(
        pLightingColorSource
        ));        

Cleanup:
    ReleaseInterfaceNoNULL(pLightingColorSource);

    RRETURN(hr);
}

// Forcing the compiler to generate code to avoid linker errors...
template CHw3DGeometryRenderer<DWORD>;
template CHw3DGeometryRenderer<vector3>;





