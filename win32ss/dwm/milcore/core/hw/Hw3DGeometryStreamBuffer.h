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
//      Contains CHwD3DBufferSpaceLocator declaration
//      Contains CHwD3DVertexBuffer declaration
//      Contains CHwD3DIndexBuffer declaration
//      Contains CHw3DGeometryRenderer declaration
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CHw3DGeometryRenderer);
MtExtern(CHwD3DVertexBuffer);
MtExtern(CHwD3DIndexBuffer);
MtExtern(D3DResource_VertexBuffer);
MtExtern(D3DResource_IndexBuffer);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwD3DBufferSpaceLocator
//
//  Synopsis:
//      This class is designed to track the current contents of a buffer and
//      return basic information like the next available piece of buffer memory. 
//      This is necessary because we want to use our buffers intelligently, if
//      there's enough space at the end of the buffer for us to append
//      information, we want to drop our data there before discarding the buffer
//      and starting at the beginning of a new one.
//
//------------------------------------------------------------------------------
class CHwD3DBufferSpaceLocator
{
public:
    UINT GetMaximumCapacity(
        UINT uElementSize
        ) const
    {
        return m_uBufferByteCapacity/uElementSize;
    }

    UINT GetNextUsableNumberOfElements(
        UINT uElementSize
        ) const;

protected:
    CHwD3DBufferSpaceLocator(UINT uNumBytes);

    void AdvanceToNextChunk(
        UINT cElementsRequired,
        UINT uElementSize,
        __out_ecount(1) DWORD * const pdwLockFlags,
        __out_ecount(1) UINT * const puStartElement
        );

    void ReportNumberOfElementsUsedInLastChunk(
        UINT cElementsUsed
        )
    {
        m_uNumBytesInLatestChunk = m_uNumBytesPerElementInLatestChunk*cElementsUsed;
    }

    UINT GetCapacity() const
    {
        return m_uBufferByteCapacity;
    }

    UINT GetCurrentBytePos() const
    {
        return m_uCurrentByteInBuffer;
    }

    UINT GetNumBytesInLastChunk() const
    { 
        return m_uNumBytesInLatestChunk;
    }

private:
    
    UINT GetNumberOfElementsAfterCurrentChunk(
        UINT uElementSize
        ) const
    {
        return (m_uBufferByteCapacity - m_uCurrentByteInBuffer - m_uNumBytesInLatestChunk)/uElementSize;
    }
    

private:
    UINT m_uCurrentByteInBuffer;
    UINT m_uNumBytesInLatestChunk;

    UINT m_uNumBytesPerElementInLatestChunk;

    UINT m_uBufferByteCapacity;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwD3DVertexBuffer
//
//  Synopsis:
//      This class expands on the capabilities of the CHwD3DBufferSpaceLocator
//      class by hanging onto a IDirect3DVertexBuffer9 object and it's
//      locking/unlocking.
//
//------------------------------------------------------------------------------
class CHwD3DVertexBuffer : 
    public CD3DResource,
    public CHwD3DBufferSpaceLocator
{
protected:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwD3DVertexBuffer));

public:
    static __checkReturn HRESULT Create(
        __inout_ecount(1) CD3DResourceManager *pResourceManager, 
        __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        UINT uCapacity,
        __deref_out_ecount(1) CHwD3DVertexBuffer ** const ppVertexBuffer
        );
        
    HRESULT Lock(
        UINT cVertices,
        UINT uVertexStride,
        __deref_out_bcount_part(cVertices * uVertexStride, 0) void ** const ppLockedVertices,
        __out_ecount(1) UINT * const puStartVertex
        );

    bool Locked() const
        { return m_fLocked; }

    HRESULT Unlock(
        UINT cVerticesUsed
        );

    __out_ecount(1) IDirect3DVertexBuffer9 *GetD3DBuffer() const
        { return m_pVertexBuffer; }

private:
    CHwD3DVertexBuffer(UINT uCapacity);
    ~CHwD3DVertexBuffer();

    HRESULT Init(    
        __inout_ecount(1) CD3DResourceManager *pResourceManager, 
        __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice
        );

#if PERFMETER
    virtual PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(D3DResource_VertexBuffer);
    }
#endif

    // 
    // CD3DResource methods
    //

    // Should only be called by CD3DResourceManager (destructor is okay, too)
    void ReleaseD3DResources();

private:
    IDirect3DVertexBuffer9 *m_pVertexBuffer;    
    bool m_fLocked;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHwD3DIndexBuffer
//
//  Synopsis:
//      This class expands on the capabilities of the CHwD3DBufferSpaceLocator
//      class by hanging onto a IDirect3DIndexBuffer9 object and it's
//      locking/unlocking.
//
//------------------------------------------------------------------------------
class CHwD3DIndexBuffer : 
    public CD3DResource,
    public CHwD3DBufferSpaceLocator
{
protected:
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwD3DIndexBuffer));

public:
    static __checkReturn HRESULT Create(
        __inout_ecount(1) CD3DResourceManager *pResourceManager, 
        __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        UINT uCapacity,
        __deref_out_ecount(1) CHwD3DIndexBuffer ** const ppIndexBuffer
        );

public:
    HRESULT Lock(
        UINT cIndices,
        __deref_out_ecount_part(cIndices, 0) WORD ** const ppwLockedIndices,
        __out_ecount(1) UINT *puStartIndex
        );

    HRESULT CopyFromInputBuffer(
        __in_ecount(cIndices) const UINT *puIndexStream,
        UINT cIndices,
        __out_ecount(1) UINT * const puStartIndex
        );

    bool Locked() const
        { return m_fLocked; }

    HRESULT Unlock();

    __out_ecount(1) IDirect3DIndexBuffer9 *GetD3DBuffer() const
        { return m_pIndexBuffer; }

private:
    CHwD3DIndexBuffer(UINT uCapacity);
    ~CHwD3DIndexBuffer();
    
    HRESULT Init(    
        __inout_ecount(1) CD3DResourceManager *pResourceManager, 
        __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice
        );

#if PERFMETER
    virtual PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(D3DResource_IndexBuffer);
    }
#endif

    // 
    // CD3DResource methods
    //

    // Should only be called by CD3DResourceManager (destructor is okay, too)
    void ReleaseD3DResources();

private:
    IDirect3DIndexBuffer9 *m_pIndexBuffer;    
    bool m_fLocked;
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CHw3DGeometryRenderer
//
//  Synopsis:
//      This class is designed to take 3d mesh data in the form of multiple
//      streams (1 each for position, normal, texture coordinate, and index). 
//      The class will then do whatever partitioning and state setting is
//      necessary for rendering.
//
//      Indexed vs NonIndexed
//
//      The class will try to handle the data in an indexed form, but there are
//      situations when it can't.  If we can't fit all the vertices into the
//      vertex buffer, then the indices will refer to vertices off the edge of
//      our buffer.  In this situation we fill the vertex buffer with vertices
//      ordered by the index buffer.  For example if we have a vertex buffer:
//
//      [Vertex 0] [Vertex 1] [Vertex 2] [Vertex 3]
//
//      and an index buffer:
//
//      0 1 2  1 2 3
//
//      Then we would store:
//
//      [Vertex 0] [Vertex 1] [Vertex 2]  [Vertex 1] [Vertex 2] [Vertex 3]
//

template <class TDiffuseOrNormal>
class CHw3DGeometryRenderer :
    public IGeometryGenerator,
    public CMILRefCountBase
{
public:
    CHw3DGeometryRenderer(
        __in_ecount(1) CMILLightData * const pLightData,
        __in_ecount(1) CD3DDeviceLevel1 * const pDeviceNoRef = NULL
        );

public:
    HRESULT Render(
        __in_ecount(1) const CMILMesh3D *pMesh3D,
        __in_bcount_opt(cbDiffuseColorsOrNormals) const TDiffuseOrNormal *ptDiffuseColorsOrNormals,
        size_t cbDiffuseColorsOrNormals,
        __in_ecount(1) const TDiffuseOrNormal &defaultDiffuseOrNormal,
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice
        );

    DWORD GetVertexStride() const
    { 
        // XYZ N/D UV
        return sizeof(dxlayer::vector3) 
             + sizeof(TDiffuseOrNormal) 
             + sizeof(dxlayer::vector2); 
    }

    //
    // Geometry Generator Methods
    //

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      GetPerVertexDataType
    //
    //  Synopsis:
    //      Return vertex fields that are generated when this is used
    //
    //--------------------------------------------------------------------------

    override void GetPerVertexDataType(
        __out_ecount(1) MilVertexFormat &mvfFullyGenerated
        ) const;

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      SendGeometry
    //
    //  Synopsis:
    //      The Hw3DGeometryRenderer doesn't send geometry to a geometrysink. 
    //      It will just return S_OK.
    //
    //--------------------------------------------------------------------------

    override HRESULT SendGeometry(
        IN IGeometrySink *pGeomSink
        );

    override HRESULT SendGeometryModifiers(
        __inout_ecount(1) CHwPipelineBuilder *pPipelineBuilder
        );

    override HRESULT SendLighting(
        __inout_ecount(1) CHwPipelineBuilder *pPipelineBuilder
        );

private:
    void CopyVerticesIntoBuffer(
        __out_xcount(cVerticesToCopy*GetVertexStride()) void * const pCardVertexBuffer,
        UINT cVerticesToCopy
        );

    void CopyIndexOrderedVerticesIntoBuffer(
        __out_xcount(cIndicesToCopy*GetVertexStride()) void * const pCardVertexBuffer,
        UINT uInputIndexStart,
        UINT cIndicesToCopy
        );

    UINT RemainingIndices() const 
        { return m_cInputIndices - m_uRenderedIndices; }

    void SetArrays(
        __in_ecount(cVertices) const dxlayer::vector3 *pvec3PositionStream,
        __in_ecount_opt(cVertices) const TDiffuseOrNormal *ptDiffuseOrNormalStream,
        __in_ecount(cVertices) const dxlayer::vector2 *pvec2TextureCoordinateStream,
        UINT cVertices,
        __in_ecount_opt(cIndices) const UINT *puIndexStream,
        UINT cIndices
        );

    HRESULT SendDeviceState(
        bool fIndexed,
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) const CHwD3DVertexBuffer *pVertexBuffer,
        __in_ecount(1) const CHwD3DIndexBuffer *pIndexBuffer
        );

    HRESULT PrepareIndexed(
        __out_ecount(1) UINT * const puStartVertex,
        __out_ecount(1) UINT * const puStartIndex,
        __out_ecount(1) UINT * const pcPrimitives,
        __out_ecount(1) bool * const pfNeedsToRender,
        __in_ecount(1) CHwD3DVertexBuffer *pVertexBuffer,
        __in_ecount(1) CHwD3DIndexBuffer *pIndexBuffer
        );

    HRESULT PrepareNonIndexed(
        __out_ecount(1) UINT * const puStartVertex,
        __out_ecount(1) UINT * const pcPrimitives,
        __out_ecount(1) bool * const pfNeedsToRender,
        __in_ecount(1) CHwD3DVertexBuffer *pVertexBuffer
        );
    
private:
    const dxlayer::vector3 *m_pInputPositionStream;
    const TDiffuseOrNormal *m_pInputDiffuseOrNormalStream;
    const dxlayer::vector2 *m_pInputTextureCoordinateStream;
    const UINT             *m_pInputIndexStream;

    CMILLightData * const m_pLightData;
    CD3DDeviceLevel1 * const m_pDeviceNoRef;

    TDiffuseOrNormal m_defaultDiffuseOrNormal;

    UINT m_cInputVertices;
    UINT m_cInputIndices;

    UINT m_uRenderedIndices;
};






