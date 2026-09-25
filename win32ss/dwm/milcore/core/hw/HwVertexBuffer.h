// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains HW Vertex Buffer and Vertex Buffer Builder declarations
//


MtExtern(CHwTVertexBuffer_Builder);

class CHwConstantColorSource;
class CD3DDeviceVertexBuffer;
class CD3DDeviceIndexBuffer;
class CHwPipeline;
struct CCoverageInterval;

//+------------------------------------------------------------------------
//
//  Enum:       WaffleModeFlags
//
//  Synopsis:   Indicates what sort of waffling we need.
//
//-------------------------------------------------------------------------

enum WaffleModeFlags
{
    WaffleModeNone    = 0,      // No waffling
    WaffleModeEnabled = 1,      // Do some waffling
    WaffleModeFlipX   = 2,      // Additionally, flip X
    WaffleModeFlipY   = 4,      // Also, flip Y
};

//+----------------------------------------------------------------------------
//
//  Class:     CHwVertexBuffer
//
//  Synopsis:  Base class for HW Vertex Buffers
//
//             It provides support for indexed triangles and the common
//             interface for calling vertex buffers.
//
//  Responsibilites:
//    - Accumulate geometry data
//    - Set vertex format on device
//
//  Not responsible for:
//    - Converting partial vertices to fully expanded vertices
//    - Choosing vertex format to use
//
//  Inputs required:
//    - Vertices and triangles from vertex builder
//

class CHwVertexBuffer
{
public:
    
    CHwVertexBuffer()
    {
        m_pBuilder = NULL;
    }
    
protected:

    //+------------------------------------------------------------------------
    //
    //  Member:    Reset
    //
    //  Synopsis:  Mark the beginning of a new list of vertices; the existing
    //             list may be discarded
    //
    //             All derived classes should implement a Reset method,
    //             but it is not required on the base.
    //
    //-------------------------------------------------------------------------

    /*
    void Reset();
    */

    //+------------------------------------------------------------------------
    //
    //  Member:    AddVertices
    //
    //  Synopsis:  Reserve space for consecutive vertices and return start
    //             index
    //
    //             All derivatives should define an AddVertices method that
    //             returns a pointer to a specific type of vertex.
    //
    //-------------------------------------------------------------------------
    /*
    HRESULT AddVertices(
        UINT uCount,
        __deref_out_ecount(uCount) <class TVertex> **ppVertices,
        __out_ecount(1) WORD *pIndexStart
        );
    */

    //+------------------------------------------------------------------------
    //
    //  Member:    AddTriangle
    //
    //  Synopsis:  Add a triangle using the three indices given to the list
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE HRESULT AddTriangle(
        WORD i1,         // In: Index of triangle's first vertex
        WORD i2,         // In: Index of triangle's second vertex
        WORD i3          // In: Index of triangle's third vertex
        );

public:

    //+------------------------------------------------------------------------
    //
    //  Member:    SendVertexFormat
    //
    //  Synopsis:  Send contained vertex format to device
    //
    //-------------------------------------------------------------------------

    virtual HRESULT SendVertexFormat(
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice
        ) const PURE;

    //+------------------------------------------------------------------------
    //
    //  Member:    DrawPrimitive
    //
    //  Synopsis:  Send the geometry data to the device and execute rendering
    //
    //-------------------------------------------------------------------------

    virtual HRESULT DrawPrimitive(
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice
        ) const PURE;

    virtual BOOL IsEmpty() const PURE;

public:
    class Builder;

protected:
    DynArray<WORD> m_rgIndices;     // Dynamic array of indices

public:
    Builder *m_pBuilder;
};


//+----------------------------------------------------------------------------
//
//  Class:     CHwTVertexBuffer<class TVertex>
//
//  Synopsis:  Type specific HW Vertex Buffer
//

template <class TVertex>
class CHwTVertexBuffer : public CHwVertexBuffer,
                         public ITriangleSink<PointXYA>,
                         public ILineSink<PointXYA>
{
#if DBG
public:
    
    CHwTVertexBuffer()
    {
        m_fDbgNonLineSegmentTriangleStrip = false;
    }
#endif

protected:

    //+------------------------------------------------------------------------
    //
    //  Member:    Reset
    //
    //  Synopsis:  Mark the beginning of a new list of vertices; the existing
    //             list is discarded
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE void Reset(
        __in_ecount(1) Builder *pVBB
        )
    {
#if DBG
        m_fDbgNonLineSegmentTriangleStrip = false;
#endif
        m_rgIndices.SetCount(0);
        m_rgVerticesTriList.SetCount(0);
        m_rgVerticesTriStrip.SetCount(0);
        m_rgVerticesLineList.SetCount(0);
        m_rgVerticesNonIndexedTriList.SetCount(0);

        m_pBuilder = pVBB;
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    AddNonIndexedTriListVertices
    //
    //  Synopsis:  Reserve space for consecutive vertices and return start
    //             index
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE HRESULT AddNonIndexedTriListVertices(
        UINT uCount,
        __deref_ecount(uCount) TVertex **ppVertices
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    AddTriListVertices
    //
    //  Synopsis:  Reserve space for consecutive vertices and return start
    //             index
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE HRESULT AddTriListVertices(
        UINT uDelta,
        __deref_ecount(uDelta) TVertex **ppVertices,
        __out_ecount(1) WORD *pwIndexStart
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    AddTriStripVertices
    //
    //  Synopsis:  Reserve space for consecutive vertices and return start
    //             index
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE HRESULT AddTriStripVertices(
        UINT uCount,
        __deref_ecount(uCount) TVertex **ppVertices
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    AddLineListVertices
    //
    //  Synopsis:  Reserve space for consecutive vertices and return start
    //             index
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE HRESULT AddLineListVertices(
        UINT uCount,
        __deref_ecount(uCount) TVertex **ppVertices
        );

public:

    //+------------------------------------------------------------------------
    //
    //  Member:    AddLine implements ILineSink<PointXYA>
    //
    //  Synopsis:  Add a line given two points with x, y, & alpha.
    //
    //-------------------------------------------------------------------------
    HRESULT AddLine(
        __in_ecount(1) const PointXYA &v0,
        __in_ecount(1) const PointXYA &v1
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    AddTriangle implements ITriangleSink<PointXYA>
    //
    //  Synopsis:  Add a triangle given three points with x, y, & alpha.
    //
    //-------------------------------------------------------------------------

    HRESULT AddTriangle(
        __in_ecount(1) const PointXYA &v0,
        __in_ecount(1) const PointXYA &v1,
        __in_ecount(1) const PointXYA &v2
        );

    // Re-introduce parent AddTriangle(WORD,WORD,WORD) into this scope.
    using CHwVertexBuffer::AddTriangle;
    
    //+------------------------------------------------------------------------
    //
    //  Member:    AddLineAsTriangleStrip
    //
    //  Synopsis:  Add a horizontal line using a trinagle strip
    //
    //-------------------------------------------------------------------------
    HRESULT AddLineAsTriangleStrip(
        __in_ecount(1) const TVertex *pBegin, // Begin
        __in_ecount(1) const TVertex *pEnd    // End
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    SendVertexFormat
    //
    //  Synopsis:  Send contained vertex format to device
    //
    //-------------------------------------------------------------------------

    HRESULT SendVertexFormat(
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice
        ) const
    {
        RRETURN(THR(pDevice->SetFVF(TVertex::Format)));
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    DrawPrimitive
    //
    //  Synopsis:  Send the geometry data to the device and execute rendering
    //
    //-------------------------------------------------------------------------

    HRESULT DrawPrimitive(
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice
        ) const
    {
        HRESULT hr = S_OK;

        Assert(pDevice);

        //
        // Draw the indexed triangle lists.  We might have indexed tri list
        // vertices but not indices if we are aliased and waffling.
        //

        if (m_rgVerticesTriList.GetCount() > 0
            && m_rgIndices.GetCount() > 0)
        {
            Assert(m_rgIndices.GetCount() % 3 == 0);

            IFC(pDevice->DrawIndexedTriangleListUP(
                m_rgVerticesTriList.GetCount(),
                m_rgIndices.GetCount() / 3, // primitive count
                m_rgIndices.GetDataBuffer(),
                m_rgVerticesTriList.GetDataBuffer(),
                sizeof(TVertex)
                ));
        }
        
        //
        // Draw the non-indexed triangle lists
        //

        if (m_rgVerticesNonIndexedTriList.GetCount() > 0)
        {
            Assert(m_rgVerticesNonIndexedTriList.GetCount() %3 == 0);
            
            IFC(pDevice->DrawPrimitiveUP(
                D3DPT_TRIANGLELIST,
                m_rgVerticesNonIndexedTriList.GetCount() / 3, // primitive count
                m_rgVerticesNonIndexedTriList.GetDataBuffer(),
                sizeof(TVertex)
                ));
        }

        //
        // Draw the triangle strips
        //

        if (m_rgVerticesTriStrip.GetCount() > 0)
        {
            // A tri strip should have at least 5 vertices including duplicate vertices 
            // at the beginning and end to make degenerate vertices
            Assert(m_rgVerticesTriStrip.GetCount() > 4);

            TVertex *pVertex = static_cast<TVertex *>(m_rgVerticesTriStrip.GetDataBuffer());
            UINT uVertexCount = m_rgVerticesTriStrip.GetCount(); 

            //Check that the tri strip does contain vertces at start and end for the degenerated triangles. 
            Assert(pVertex);
            Assert(pVertex[0].ptPt.Y == pVertex[1].ptPt.Y);
            Assert(pVertex[0].ptPt.X == pVertex[1].ptPt.X); 
            Assert(pVertex[uVertexCount -1].ptPt.Y == pVertex[uVertexCount -2].ptPt.Y);
            Assert(pVertex[uVertexCount -1].ptPt.X == pVertex[uVertexCount -2].ptPt.X); 

            //Remove degenerated triangles from starting and the ending of the vertex buffer. 
            pVertex++;

            IFC(pDevice->DrawPrimitiveUP(
                D3DPT_TRIANGLESTRIP,
                uVertexCount - 4, // primitive count
                pVertex,
                sizeof(TVertex)
                ));
        }

        //
        // Draw the line lists
        //

        if (m_rgVerticesLineList.GetCount() > 0)
        {
            IFC(pDevice->DrawPrimitiveUP(
                D3DPT_LINELIST,
                m_rgVerticesLineList.GetCount() / 2, // primitive count
                m_rgVerticesLineList.GetDataBuffer(),
                sizeof(TVertex)
                ));
        }

    Cleanup:
        RRETURN(hr);
    }

protected:
    //+------------------------------------------------------------------------
    //
    //  Member:    GetNumTriListVertices
    //
    //  Synopsis:  Return current number of vertices
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE DWORD GetNumTriListVertices() const
    {
        return m_rgVerticesTriList.GetCount();
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    GetTriListVertices
    //
    //  Synopsis:  Return pointer to beginning of vertex list and their count
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE void GetTriListVertices(
        __deref_out_ecount_full(*puNumVertices) TVertex **ppVertices,
        __out_ecount(1) UINT * puNumVertices
        )
    {
        *ppVertices = m_rgVerticesTriList.GetDataBuffer();
        *puNumVertices = m_rgVerticesTriList.GetCount();
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    GetNumNonIndexedTriListVertices
    //
    //  Synopsis:  Return current number of vertices
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE DWORD GetNumNonIndexedTriListVertices() const
    {
        return m_rgVerticesNonIndexedTriList.GetCount();
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    GetNonIndexedTriListVertices
    //
    //  Synopsis:  Return pointer to beginning of vertex list and their count
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE void GetNonIndexedTriListVertices(
        __deref_out_ecount_full(*puNumVertices) TVertex **ppVertices,
        __out_ecount(1) UINT * puNumVertices
        )
    {
        *ppVertices = m_rgVerticesNonIndexedTriList.GetDataBuffer();
        *puNumVertices = m_rgVerticesNonIndexedTriList.GetCount();
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    GetNumTriStripVertices
    //
    //  Synopsis:  Return current number of vertices
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE DWORD GetNumTriStripVertices() const
    {
        return m_rgVerticesTriStrip.GetCount();
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    GetTriStripVertices
    //
    //  Synopsis:  Return pointer to beginning of vertex list and their count
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE void GetTriStripVertices(
        __deref_out_ecount_full(*puNumVertices) TVertex **ppVertices,
        __out_ecount(1) UINT *puNumVertices
        )
    {
        *ppVertices = m_rgVerticesTriStrip.GetDataBuffer();
        *puNumVertices = m_rgVerticesTriStrip.GetCount();
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    GetNumLineListVertices
    //
    //  Synopsis:  Return current number of vertices
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE DWORD GetNumLineListVertices() const
    {
        return m_rgVerticesLineList.GetCount();
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    GetLineListVertices
    //
    //  Synopsis:  Return pointer to beginning of vertex list and their count
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE void GetLineListVertices(
        __deref_out_ecount_full(*puNumVertices) TVertex **ppVertices,
        __out_ecount(1) UINT * puNumVertices
        )
    {
        *ppVertices = m_rgVerticesLineList.GetDataBuffer();
        *puNumVertices = m_rgVerticesLineList.GetCount();
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    GetLineListVertices
    //
    //  Synopsis:  Return pointer to beginning of vertex list
    //
    //-------------------------------------------------------------------------

    virtual BOOL IsEmpty() const
    {
        return 
               (m_rgIndices.GetCount() == 0)
            && (m_rgVerticesLineList.GetCount() == 0)
            && (m_rgVerticesTriStrip.GetCount() == 0)
            && (m_rgVerticesNonIndexedTriList.GetCount() == 0);
    }

public:
    class Builder;

private:
    // Dynamic array of vertices for which all allocations are zeroed.
    DynArray<TVertex, true> m_rgVerticesTriList;             // Indexed triangle list vertices
    DynArray<TVertex, true> m_rgVerticesNonIndexedTriList;   // Non-indexed triangle list vertices
    DynArray<TVertex, true> m_rgVerticesTriStrip;            // Triangle strip vertices
    DynArray<TVertex, true> m_rgVerticesLineList;            // Linelist vertices

#if DBG
    // In debug make a note if we add a triangle strip that doesn't have 6 vertices
    // so that we can ensure that we only waffle 6-vertex tri strips.
    bool m_fDbgNonLineSegmentTriangleStrip;
#endif
};


//+----------------------------------------------------------------------------
//
//  Class:     CHwVertexBuffer::Builder
//
//  Synopsis:  Base vertex builder class
//
//  Responsibilities:
//    - Given ordered basic vertex information expand/convert/pass-thru
//      to vertex buffer  (Basic vertex information is minimal vertex
//      information sent from the caller that may or may not have been
//      passed thru a tessellator.)
//    - Choosing vertex format from a minimal required vertex format
//
//  Not responsible for:
//    - Allocating space in vertex buffer
//
//  Inputs required:
//    - Key and data to translate input basic vertex info to full vertex data
//    - Vertex info from tessellation (or other Geometry Generator)
//    - Vertex buffer to send output to
//

class CHwVertexBuffer::Builder : public IGeometrySink
{
public:

    static HRESULT Create(
        MilVertexFormat vfIn,
        MilVertexFormat vfOut,
        MilVertexFormatAttribute vfaAntiAliasScaleLocation,
        __in_ecount_opt(1) CHwPipeline *pPipeline,
        __in_ecount_opt(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) CBufferDispenser *pBufferDispenser,
        __deref_out_ecount(1) CHwVertexBuffer::Builder **ppVertexBufferBuilder
        );

    virtual ~Builder()
    {
#if DBG
        Assert(!m_fDbgDestroyed);
        m_fDbgDestroyed = true;
#endif DBG
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    SetConstantMapping
    //
    //  Synopsis:  Use this method to specify that the given color source for
    //             the given vertex destination is constant (won't differ per
    //             vertex)
    //
    //-------------------------------------------------------------------------

    virtual HRESULT SetConstantMapping(
        MilVertexFormatAttribute mvfaDestination,
        __in_ecount(1) const CHwConstantColorSource *pConstCS
        ) PURE;

    //+------------------------------------------------------------------------
    //
    //  Member:    SetTextureMapping
    //
    //  Synopsis:  Use this method to specify how to generate texture
    //             coordinates at the given destination index
    //
    //-------------------------------------------------------------------------

    virtual HRESULT SetTextureMapping(
        DWORD dwDestinationCoordIndex,
        DWORD dwSourceCoordIndex,
        __in_ecount(1) const MILMatrix3x2 *pmatDevicePointToTextureUV
        ) PURE;

    virtual HRESULT SetWaffling(
        DWORD dwCoordIndex,
        __in_ecount(1) const CMilPointAndSizeF *pSubrect,
        WaffleModeFlags waffleMode
        ) PURE;

    //+------------------------------------------------------------------------
    //
    //  Member:    FinalizeMappings
    //
    //  Synopsis:  Use this method to let builder know that all mappings have
    //             been sent
    //
    //-------------------------------------------------------------------------

    virtual HRESULT FinalizeMappings(
        ) PURE;

    //+------------------------------------------------------------------------
    //
    //  Member:    SetOutsideBounds
    //
    //  Synopsis:  Enables rendering zero-alpha geometry outside of the input
    //             shape but within the given bounding rectangle, if fNeedInside
    //             isn't true then it doesn't render geometry with full alpha.
    //
    //-------------------------------------------------------------------------
    virtual void SetOutsideBounds(
        __in_ecount_opt(1) const CMILSurfaceRect *prcBounds,
        bool fNeedInside
        ) PURE;

    //+------------------------------------------------------------------------
    //
    //  Member:    HasOutsideBounds
    //
    //  Synopsis:  Returns true if outside bounds have been set.
    //
    //-------------------------------------------------------------------------
    virtual bool HasOutsideBounds() const PURE;

    //+------------------------------------------------------------------------
    //
    //  Member:    BeginBuilding
    //
    //  Synopsis:  This method lets the builder know it should start from a
    //             clean slate
    //
    //-------------------------------------------------------------------------

    virtual HRESULT BeginBuilding(
        ) PURE;

    //+------------------------------------------------------------------------
    //
    //  Member:    EndBuilding
    //
    //  Synopsis:  Use this method to let the builder know that all of the
    //             vertex data has been sent
    //
    //-------------------------------------------------------------------------

    virtual HRESULT EndBuilding(
        __deref_opt_out_ecount(1) CHwVertexBuffer **ppVertexBuffer
        ) PURE;

    //+------------------------------------------------------------------------
    //
    //  Member:    FlushReset
    //
    //  Synopsis:  Send pending state and geometry to the device and reset
    //             the vertex buffer.
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE HRESULT FlushReset()
    {
        return FlushInternal(NULL);
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    FlushTryGetVertexBuffer
    //
    //  Synopsis:  Send pending state and geometry to the device and
    //             return the vertex buffer if there was not another flush
    //             (since the last BeginBuilding.)
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE HRESULT FlushTryGetVertexBuffer(
        __out_ecount(1) CHwVertexBuffer **ppVertexBuffer
        )
    {
        return FlushInternal(ppVertexBuffer);
    }
    
    //+------------------------------------------------------------------------
    //
    //  Member:    GetViewportTop
    //
    //  Synopsis:  Returns the top of the viewport last time BeginBuilding
    //             was called.
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE INT GetViewportTop() const
    {
        return m_iViewportTop;
    }
        
    //
    // Currently all CHwVertexBuffer::Builder are supposed to be allocated via
    // a CBufferDispenser.
    //

    DECLARE_BUFFERDISPENSER_DELETE

protected:

    CHwVertexBuffer::Builder()
    {
        m_mvfIn = MILVFAttrNone;

#if DBG
        m_mvfDbgOut = MILVFAttrNone;
#endif

        m_mvfaAntiAliasScaleLocation = MILVFAttrNone;

        m_pPipelineNoRef = NULL;
        m_pDeviceNoRef = NULL;
        
#if DBG
        m_fDbgDestroyed = false;
#endif DBG
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    FlushInternal
    //
    //  Synopsis:  Send any pending state and geometry to the device.
    //             If the optional argument is NULL then reset the
    //             vertex buffer.
    //             If the optional argument is non-NULL AND we have
    //             not yet flushed the vertex buffer return the vertex
    //             buffer.
    //
    //-------------------------------------------------------------------------

    virtual HRESULT FlushInternal(
        __deref_opt_out_ecount_opt(1) CHwVertexBuffer **ppVertexBuffer
        ) PURE;


    static void TransferUIntIndicesAsWords(
        __in_ecount(cIndices) const UINT *rguInputIndices,
        __out_ecount_full(cIndices) WORD *rgwOutputIndices,
        __range(1, UINT_MAX) UINT cIndices
        );

    CHwPipeline *m_pPipelineNoRef;
    CD3DDeviceLevel1 *m_pDeviceNoRef;

    INT m_iViewportTop;

    MilVertexFormat m_mvfIn;         // Vertex fields that are pre-generated

#if DBG
    MilVertexFormat m_mvfDbgOut;     // Output format of the vertex
#endif

    MilVertexFormat m_mvfGenerated;  // Vertex fields that are dynamically
                                     // generated by this builder

    MilVertexFormatAttribute m_mvfaAntiAliasScaleLocation;  // Vertex field that
                                                            // contains PPAA
                                                            // falloff factor

#if DBG
private:

    bool m_fDbgDestroyed;     // Used to check single Release pattern

#endif DBG

};


//+----------------------------------------------------------------------------
//
//  Class:     CHwTVertexMappings<class TVertex>
//
//  Synopsis:  Helper class that knows how to populate a vertex from the
//             incoming basic per vertex data, like just X and Y
//
//-----------------------------------------------------------------------------

template <class TVertex>
class CHwTVertexMappings
{
public:

    CHwTVertexMappings();

    void SetPositionTransform(
        __in_ecount(1) const MILMatrix3x2 &matPositionTransform
        );

    HRESULT SetConstantMapping(
        MilVertexFormatAttribute mvfaDestination,
        __in_ecount(1) const CHwConstantColorSource *pConstCS
        );

    HRESULT SetTextureMapping(
        DWORD dwDestinationCoordIndex,
        DWORD dwSourceCoordIndex,
        __in_ecount(1) const MILMatrix3x2 *pmatDevicePointToTextureUV
        );

    HRESULT SetWaffling(
        DWORD dwCoordIndex,
        __in_ecount(1) const CMilPointAndSizeF *pSubrect,
        WaffleModeFlags waffleMode
        );
        
    void PointToUV(
        __in_ecount(1) const MilPoint2F &ptIn,
        __bound UINT uIndex,
        __out_ecount(1) TVertex *pvOut
        );
    
    MIL_FORCEINLINE bool AreWaffling() const
    {
        return m_fAreWaffling;
    }

private:
    static const size_t s_numOfVertexTextureCoords
        = NUM_OF_VERTEX_TEXTURE_COORDS(TVertex);
public:

    MilVertexFormat m_mvfMapped;

    MilColorF m_colorStatic;

    MILMatrix3x2 m_matPos2DTransform;

    MILMatrix3x2 m_rgmatPointToUV[s_numOfVertexTextureCoords];
    CMilPointAndSizeF m_rgSubrect[s_numOfVertexTextureCoords];
    WaffleModeFlags m_rgWaffleMode[s_numOfVertexTextureCoords];

    bool m_fAreWaffling;

    TVertex m_vStatic;
};


//+----------------------------------------------------------------------------
//
//  Class:     CHwTVertexBuffer<class TVertex>::Builder
//
//  Synopsis:  Implements CHwVertexBuffer::Builder for a particular vertex
//             format
//
//-----------------------------------------------------------------------------

template <class TVertex>
class CHwTVertexBuffer<TVertex>::Builder : public CHwVertexBuffer::Builder
{
public:

    static MilVertexFormat GetOutVertexFormat();

    static HRESULT Create(
        __in_ecount(1) CHwTVertexBuffer<TVertex> *pVertexBuffer,
        MilVertexFormat mvfIn,
        MilVertexFormat mvfOut,
        MilVertexFormatAttribute mvfaAntiAliasScaleLocation,
        __inout_ecount(1) CBufferDispenser *pBufferDispenser,
        __deref_out_ecount(1) typename CHwTVertexBuffer<TVertex>::Builder **ppVertexBufferBuilder
        );

    HRESULT SetConstantMapping(
        MilVertexFormatAttribute mvfaDestination,
        __in_ecount(1) const CHwConstantColorSource *pConstCS
        );

    void SetTransformMapping(
        __in_ecount(1) const MILMatrix3x2 &mat2DTransform
        );

    HRESULT SetTextureMapping(
        DWORD dwDestinationCoordIndex,
        DWORD dwSourceCoordIndex,
        __in_ecount(1) const MILMatrix3x2 *pmatDevicePointToTextureUV
        );

    override HRESULT SetWaffling(
        DWORD dwCoordIndex,
        __in_ecount(1) const CMilPointAndSizeF *pSubrect,
        WaffleModeFlags waffleMode
        );

    HRESULT FinalizeMappings(
        );

    override void SetOutsideBounds(
        __in_ecount_opt(1) const CMILSurfaceRect *prcBounds,
        bool fNeedInside
        );

    override bool HasOutsideBounds() const
    {
        return NeedOutsideGeometry();
    }

    HRESULT BeginBuilding(
        );

    HRESULT AddVertex(
        __in_ecount(1) const MilPoint2F &ptPosition,
            // In: Vertex coordinates
        __out_ecount(1) WORD *pIndex
            // Out: The index of the new vertex
        );

    HRESULT AddIndexedVertices(
        UINT cVertices,                                                  // In: number of vertices                                                                                            
        __in_bcount(cVertices*uVertexStride) const void *pVertexBuffer,  // In: vertex buffer containing the vertices                                                                         
        UINT uVertexStride,                                              // In: size of each vertex                                                                                           
        MilVertexFormat mvfFormat,                                       // In: format of each vertex                                                                                         
        UINT cIndices,                                                   // In: Number of indices                                                                                             
        __in_ecount(cIndices) const UINT *puIndexBuffer                  // In: index buffer                                                             
        );

    HRESULT AddTriangle(
        DWORD i1,                    // In: Index of triangle's first vertex
        DWORD i2,                    // In: Index of triangle's second vertex
        DWORD i3                     // In: Index of triangle's third vertex
        );

    HRESULT AddComplexScan(
        INT nPixelY,
            // In: y coordinate in pixel space
        __in_ecount(1) const CCoverageInterval *pIntervalSpanStart
            // In: coverage segments
        );

   HRESULT AddParallelogram(
        __in_ecount(4)  const MilPoint2F *rgPosition
        );
   
    HRESULT AddTrapezoid(
        float rPixelYTop,               // In: y coordinate of top of trapezoid
        float rPixelXTopLeft,           // In: x coordinate for top left
        float rPixelXTopRight,          // In: x coordinate for top right
        float rPixelYBottom,            // In: y coordinate of bottom of trapezoid
        float rPixelXBottomLeft,        // In: x coordinate for bottom left
        float rPixelXBottomRight,       // In: x coordinate for bottom right
        float rPixelXLeftDelta,         // In: trapezoid expand radius for left edge
        float rPixelXRightDelta         // In: trapezoid expand radius for right edge
        );

    BOOL IsEmpty();

    HRESULT EndBuilding(
        __deref_opt_out_ecount(1) CHwVertexBuffer **ppVertexBuffer
        );

    override HRESULT FlushInternal(
        __deref_opt_out_ecount_opt(1) CHwVertexBuffer **ppVertexBuffer
        );
            
private:

    // Helpers that do AddTrapezoid.  Same parameters
    HRESULT AddTrapezoidStandard( float, float, float, float, float, float, float, float );
    HRESULT AddTrapezoidWaffle( float, float, float, float, float, float, float, float );

    // Helpers that handle extra shapes in trapezoid mode.
    MIL_FORCEINLINE HRESULT PrepareStratum(
        float rStratumTop,
        float rStratumBottom,
        bool fTrapezoid,
        float rTrapezoidLeft = 0,
        float rTrapezoidRight = 0
        )
    {
        return NeedOutsideGeometry()
            ? PrepareStratumSlow(
                rStratumTop,
                rStratumBottom,
                fTrapezoid,
                rTrapezoidLeft,
                rTrapezoidRight
                )
            : S_OK;
    }
    
    HRESULT PrepareStratumSlow(
        float rStratumTop,
        float rStratumBottom,
        bool fTrapezoid,
        float rTrapezoidLeft,
        float rTrapezoidRight
        );
    
    // Wrap up building of outside geometry.
    HRESULT EndBuildingOutside();

    DECLARE_BUFFERDISPENSER_NEW(CHwTVertexBuffer<TVertex>::Builder,
                                Mt(CHwTVertexBuffer_Builder));

    Builder(
        __in_ecount(1) CHwTVertexBuffer<TVertex> *pVertexBuffer
    );

    HRESULT SetupConverter(
        MilVertexFormat mvfIn,
        MilVertexFormat mvfOut,
        MilVertexFormatAttribute mvfaAntiAliasScaleLocation
        );

    HRESULT RenderPrecomputedIndexedTriangles(
        __range(1, SHORT_MAX) UINT cVertices,
        __in_ecount(cVertices) const TVertex *rgoVertices,
        __range(1, UINT_MAX) UINT cIndices,
        __in_ecount(cIndices) const UINT *rguIndices
        );


    // Expands all vertices in the buffer.
    void ExpandVertices();
    
    // Has never been successfully used to declare a method or derived type...
/*    typedef void (CHwTVertexBuffer<TVertex>::Builder::FN_ExpandVertices)(
        UINT uCount,
        TVertex *pVertex
        );*/

    // error C2143: syntax error : missing ';' before '*'
//    typedef FN_ExpandVertices *PFN_ExpandVertices;

    typedef void (CHwTVertexBuffer<TVertex>::Builder::* PFN_ExpandVertices)(
        __range(1,UINT_MAX) UINT uCount,
        __inout_ecount_full(uCount) TVertex *rgVertices
        );

    //
    // Table of vertex expansion routines for common expansion cases:
    //  - There are entries for Z, Diffuse, and one set texture coordinates for
    //    a total of eight combinations.
    //  - Additionally there is a second set of entries for anti-aliasing
    //    falloff applied thru diffuse.
    //

    static const PFN_ExpandVertices sc_pfnExpandVerticesTable[8*2];

    MIL_FORCEINLINE
    void TransferAndOrExpandVerticesInline(
        __range(1,UINT_MAX) UINT uCount,
        __in_ecount(uCount) TVertex const * rgInputVertices,
        __out_ecount(uCount) TVertex *rgOutputVertices,
        MilVertexFormat mvfOut,
        MilVertexFormatAttribute mvfaScaleByFalloff,
        bool fInputOutputAreSameBuffer,
        bool fTransformPosition
        );

    // FN_ExpandVertices ExpandVerticesFast
    template <MilVertexFormat mvfOut, MilVertexFormatAttribute mvfaScaleByFalloff>
    void ExpandVerticesFast(
        __range(1,UINT_MAX) UINT uCount,
        __inout_ecount_full(uCount) TVertex *rgVertices
        )
    {
        TransferAndOrExpandVerticesInline(
            uCount, 
            rgVertices, 
            rgVertices, 
            mvfOut, 
            mvfaScaleByFalloff,
            true, // => fInputOutputAreSameBuffer
            false // => fTransformPosition
            );
    }

    // error C2146: syntax error : missing ';' before identifier 'ExpandVerticesGeneral'
    // error C2501: 'CHwTVertexBufferBuilder<TVertex>::FN_ExpandVertices' : missing storage-class or type specifiers
//    FN_ExpandVertices ExpandVerticesGeneral
//    typename FN_ExpandVertices ExpandVerticesGeneral
    // error C4346: 'CHwTVertexBufferBuilder<TVertex>::FN_ExpandVertices' : dependent name is not a type
//    CHwTVertexBufferBuilder<TVertex>::FN_ExpandVertices ExpandVerticesGeneral
    // Can't define methos here (unless not parameters are used).
//    typename CHwTVertexBufferBuilder<TVertex>::FN_ExpandVertices ExpandVerticesGeneral
    // FN_ExpandVertices ExpandVerticesGeneral
    void ExpandVerticesGeneral(
        __range(1,UINT_MAX) UINT uCount,
        __inout_ecount_full(uCount) TVertex *rgVertices
        )
    {
        TransferAndOrExpandVerticesInline(
            uCount, 
            rgVertices,
            rgVertices,
            m_mvfGenerated, 
            m_mvfaAntiAliasScaleLocation,
            true, // => fInputOutputAreSameBuffer
            false // => fTransformPosition
            );
    }

    void TransferAndExpandVerticesGeneral(
        __range(1,UINT_MAX) UINT uCount,
        __in_ecount(uCount) TVertex const *rgInputVertices,
        __out_ecount_full(uCount) TVertex *rgOutputVertices,
        bool fTransformPosition
        )
    {
        TransferAndOrExpandVerticesInline(
            uCount, 
            rgInputVertices,
            rgOutputVertices,
            m_mvfGenerated, 
            m_mvfaAntiAliasScaleLocation,
            false,              // => fInputOutputAreSameBuffer
            fTransformPosition  // => fTransformPosition
            );
    }

    // FN_ExpandVertices ExpandVerticesInvalid
    void ExpandVerticesInvalid(
        __range(1,UINT_MAX) UINT uCount,
        __inout_ecount_full(uCount) TVertex *rgVertices
        )
    {
        RIP("Invalid ExpandVertices routine.");
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    NeedCoverageGeometry
    //
    //  Synopsis:  True if we should create geometry for a particular
    //             coverage value.
    //
    //-------------------------------------------------------------------------
    bool NeedCoverageGeometry(INT nCoverage) const;

    //+------------------------------------------------------------------------
    //
    //  Member:    NeedOutsideGeometry
    //
    //  Synopsis:  True if we should create geometry with zero alpha for
    //             areas outside the input geometry but within a given
    //             bounding box.
    //
    //-------------------------------------------------------------------------
    MIL_FORCEINLINE bool NeedOutsideGeometry() const
    {
        return m_fNeedOutsideGeometry;
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    NeedInsideGeometry
    //
    //  Synopsis:  True if we should create geometry for areas completely
    //             withing the input geometry (i.e. alpha 1.)  Should only
    //             be false if NeedOutsideGeometry is true.
    //
    //-------------------------------------------------------------------------
    MIL_FORCEINLINE bool NeedInsideGeometry() const
    {
        Assert(m_fNeedOutsideGeometry || m_fNeedInsideGeometry);
        return m_fNeedInsideGeometry;
    }

    //+------------------------------------------------------------------------
    //
    //  Member:    ReinterpretFloatAsDWORD
    //
    //  Synopsis:  Quicky helper to convert a float to a DWORD bitwise.
    //
    //-------------------------------------------------------------------------
    static MIL_FORCEINLINE DWORD ReinterpretFloatAsDWORD(float c)
    {
        return reinterpret_cast<DWORD &>(c);
    }

private:
    MIL_FORCEINLINE bool AreWaffling() const
    {
        return m_map.AreWaffling();
    }
   
    void ViewportToPackedCoordinates(
        __range(1,UINT_MAX / uGroupSize) UINT uGroupCount,        
        __inout_ecount(uGroupCount * uGroupSize) TVertex *pVertex,
        __range(2,6) UINT uGroupSize,
        /*__range(0,NUM_OF_VERTEX_TEXTURE_COORDS(TVertex)-1)*/ __bound UINT uIndex
        );
    
    void ViewportToPackedCoordinates(
        __range(1,UINT_MAX / uGroupSize) UINT uGroupCount,
        __inout_ecount(uGroupCount * uGroupSize) TVertex *pVertex,
        __range(2,6) UINT uGroupSize
        );

    template<class TWaffler>
    __out_ecount(1) typename TWaffler::ISink *
    BuildWafflePipeline(
        __out_xcount(NUM_OF_VERTEX_TEXTURE_COORDS(TVertex) * 2) TWaffler *wafflers,
        __out_ecount(1) bool &fWafflersUsed
        ) const;

    
    template<class TWaffler>
    typename TWaffler::ISink *
    BuildWafflePipeline(
        __out_xcount(NUM_OF_VERTEX_TEXTURE_COORDS(TVertex) * 2) TWaffler *wafflers
        ) const
    {
        bool fNotUsed;
        return BuildWafflePipeline(wafflers, fNotUsed);
    }

    CHwTVertexBuffer<TVertex> *m_pVB;

    PFN_ExpandVertices m_pfnExpandVertices;  // Method for expanding vertices

    TVertex const *m_rgoPrecomputedTriListVertices;
    UINT m_cPrecomputedTriListVertices;

    UINT const *m_rguPrecomputedTriListIndices;
    UINT m_cPrecomputedTriListIndices;

    CHwTVertexMappings<TVertex> m_map;

    // This is true if we had to flush the pipeline as we were getting
    // geometry rather than just filling up a single vertex buffer.
    bool m_fHasFlushed;

    // The next two members control the generation of the zero-alpha geometry
    // outside the input geometry.
    bool m_fNeedOutsideGeometry;
    bool m_fNeedInsideGeometry;
    CMILSurfaceRect m_rcOutsideBounds; // Bounds for creation of outside geometry

    // Helpful m_rcOutsideBounds casts.
    float OutsideLeft() const { return static_cast<float>(m_rcOutsideBounds.left); }
    float OutsideRight() const { return static_cast<float>(m_rcOutsideBounds.right); }
    float OutsideTop() const { return static_cast<float>(m_rcOutsideBounds.top); }
    float OutsideBottom() const { return static_cast<float>(m_rcOutsideBounds.bottom); }

    // This interval (if we are doing outside) shows the location
    // of the current stratum.  It is initialized to [FLT_MAX, -FLT_MAX].
    //
    // If the current stratum is a complex span then
    // m_rCurStratumBottom is set to the bottom of the stratum and
    // m_rCurStratumTop is set to FLT_MAX.
    //
    // If the current stratum is a trapezoidal one, then
    // m_rCurStratumBottom is its bottom and m_rCurStratumTop is its
    // top.
    float m_rCurStratumTop;
    float m_rCurStratumBottom;

    // If the current stratum is a trapezoidal one, following var stores
    // right boundary of the last trapezoid handled by PrepareStratum.
    // We need it to cloze the stratus properly.
    float m_rLastTrapezoidRight;
};

//+------------------------------------------------------------------------
//
//  Enum:      kMaxVertexBuilderSize
//
//  Synopsis:  Keeps track of the largest amount of space required for a
//             vertex builder.
//
//-------------------------------------------------------------------------

// The MAX macro has been phased out and replaced by the function max, but here a function
// call is not allowed.  To avoid misuse elsewhere, the BAD_MAX macro is defined ad-hoc here.
//
// NOTE-2005/04/13-chrisra BAD_MAX is also defined in hwsurfrt.cpp
//
#define BAD_MAX(a, b) ((a) >= (b) ? (a) : (b))
enum
{
    kMaxVertexBuilderSize =
        BAD_MAX(MAX_SPACE_FOR_TYPE(CHwTVertexBuffer<CD3DVertexXYZDUV8>::Builder),
                MAX_SPACE_FOR_TYPE(CHwTVertexBuffer<CD3DVertexXYZDUV2>::Builder))
};
// This macro should not be used elsewhere.
#undef BAD_MAX




