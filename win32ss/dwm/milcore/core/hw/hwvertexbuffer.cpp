// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains HW Vertex Buffer and Builder class implementations
//
//
//  Notes:
//        
//        +--------------------------------------+
//        |                                      |
//        |           Start Stratum              |
//     1  |                                      |
//        |                                      |
//        +--------------------------------------+
//     2  |======================================|
//        +--------------------------------------+
//        |          /   \             / \       |
//        |         /     \           /   \      |
//        |   A    /   B   \    C    /  D  \  E  |
//     3  |       /         \       /       \    |
//        |      /           \     /         \   |
//        |     /             \   /           \  |
//        |    /               \ /             \ |
//        +--------------------------------------+
//        |    \               / \             / |
//        |     \             /   \           /  |
//     4  |  F   \     G     /  H  \    I    / J |
//        |       \         /       \       /    |
//        +--------------------------------------+
//     5  |======================================|
//        +--------------------------------------+
//     6  |======================================|
//        +--------------------------------------+
//        |                                      |
//        |                                      |
//     7  |           Stop Stratum               |
//        |                                      |
//        |                                      |
//        +--------------------------------------+
//        
//  
//  Strata & complement mode.
//  
//  The anti-aliased HW rasterizer produces a series of "strata" where
//  each strata can be a complex span rendered using lines (#'s 2,5,6) or
//  a series of trapezoids (#'s 3 & 4.)  In normal mode the trapezoid
//  regions B,D,G,I are filled in.
//  
//  Complement mode complicates things.  Complex spans are relatively easy
//  because we get the whole line's worth of data at once.  Trapezoids are
//  more complex because we get B,D,G and I separately.  We handle this by
//  tracking the current stratum and finishing the last incomplete
//  trapezoid stratum when a new stratum begins.  Regions E & J finish
//  trapezoid strata.  We also need to add rectangles at the beginning and
//  end of the geometry (start and stop) to fill out the complement
//  region.
//  
//  This is implemented like so:
//  
//    1. Strata are generated from top to bottom without gaps.
//    2. Before drawing any lines or trapezoids call
//       PrepareStratum(a, b, fTrapezoid) where a & b are the extent of
//       the current stratum and fTrapezoid is true if you are drawing
//       a trapezoid.  This will take care of creating the start
//       stratum and/or finishing a trapezoid stratum if necessary.
//    3. When completely done call EndBuildingOutside() which will
//       close a pending trapezoid and/or produce the stop stratum.
//  
//-----------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CHwTVertexBuffer_Builder, MILRender, "CHwTVertexBuffer<TVertex>::Builder");

ExternTag(tagWireframe);

//+----------------------------------------------------------------------------
//
//  Constants to control when we stop waffling because the tiles are too
//  small to make a difference.
//
//  Future Consideration:  can produce an excessive number of triangles.
//   How we mitigate or handle this could be improved.  Right now we stop
//   waffling if the waffle size is less than a quarter-pixel.
//   Two big improvements that could be made are:
//    - multipacking very small textures (but note that we cannot rely
//      on prefiltering to ensure that small screen space means small texture
//      source)
//    - clipping primitives to approximately the screen size
//
//-----------------------------------------------------------------------------

const float c_rMinWaffleWidthPixels = 0.25f;


//+----------------------------------------------------------------------------
//
//  Class:     CHwVertexBuffer and CHwTVertexBuffer<class TVertex>
//
//  Synopsis:  This class accumulates geometry data for a primitive
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:    CHwVertexBuffer::AddTriangle
//
//  Synopsis:  Add a triangle using the three indices given to the list
//

MIL_FORCEINLINE
HRESULT
CHwVertexBuffer::AddTriangle(
    WORD i1,         // In: Index of triangle's first vertex
    WORD i2,         // In: Index of triangle's second vertex
    WORD i3          // In: Index of triangle's third vertex
    )
{
    HRESULT hr = S_OK;

    // Asserting indices < max vertex requires a debug only pure virtual method
    // which is too much of a functionality change between retail and debug.
    //
    //
    // Assert(i1 < GetNumTriListVertices());
    // Assert(i2 < GetNumTriListVertices());
    // Assert(i3 < GetNumTriListVertices());

    WORD *pIndices;

    IFC(m_rgIndices.AddMultiple(3, &pIndices));

    pIndices[0] = i1;
    pIndices[1] = i2;
    pIndices[2] = i3;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::AddTriangle
//
//  Synopsis:  Add a triangle using given three points to the list
//
//-----------------------------------------------------------------------------
template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::AddTriangle(
    __in_ecount(1) const PointXYA &v0,
    __in_ecount(1) const PointXYA &v1,
    __in_ecount(1) const PointXYA &v2)
{
    HRESULT hr = S_OK;
    
    TVertex *pVertices;
    hr = AddNonIndexedTriListVertices(3,&pVertices);

    if (hr == E_OUTOFMEMORY)
    {
        DebugBreak ();
    }
    IFC(hr);
    
    pVertices[0].ptPt.X = v0.x;
    pVertices[0].ptPt.Y = v0.y;
    pVertices[0].Diffuse = reinterpret_cast<const DWORD &>(v0.a);
    pVertices[1].ptPt.X = v1.x;
    pVertices[1].ptPt.Y = v1.y;
    pVertices[1].Diffuse = reinterpret_cast<const DWORD &>(v1.a);
    pVertices[2].ptPt.X = v2.x;
    pVertices[2].ptPt.Y = v2.y;
    pVertices[2].Diffuse = reinterpret_cast<const DWORD &>(v2.a);

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::AddLine
//
//  Synopsis:  Add a nominal width line using given two points to the list
//
//-----------------------------------------------------------------------------
template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::AddLine(
    __in_ecount(1) const PointXYA &v0,
    __in_ecount(1) const PointXYA &v1
    )
{
    HRESULT hr = S_OK;

    TVertex *pVertices;
    TVertex rgScratchVertices[2];

    Assert(!(v0.y != v1.y));
    
    bool fUseTriangles = v0.y < m_pBuilder->GetViewportTop() + 1;

    if (fUseTriangles)
    {
        pVertices = rgScratchVertices;
    }
    else
    {
        IFC(AddLineListVertices(2, &pVertices));
    }
    
    pVertices[0].ptPt.X = v0.x;
    pVertices[0].ptPt.Y = v0.y;
    pVertices[0].Diffuse = reinterpret_cast<const DWORD &>(v0.a);
    pVertices[1].ptPt.X = v1.x;
    pVertices[1].ptPt.Y = v1.y;
    pVertices[1].Diffuse = reinterpret_cast<const DWORD &>(v1.a);

    if (fUseTriangles)
    {
        IFC(AddLineAsTriangleStrip(pVertices,pVertices+1));
    }
    
  Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::AddTriListVertices
//
//  Synopsis:  Reserve space for consecutive vertices and return start index
//

template <class TVertex>
MIL_FORCEINLINE
HRESULT
CHwTVertexBuffer<TVertex>::AddTriListVertices(
    UINT uDelta,
    __deref_ecount(uDelta) TVertex **ppVertices,
    __out_ecount(1) WORD *pwIndexStart
    )
{
    HRESULT hr = S_OK;

    Assert(ppVertices);

    UINT uCount = static_cast<UINT>(m_rgVerticesTriList.GetCount());
    if (uCount > SHRT_MAX)
    {
        IFC(WGXERR_INVALIDPARAMETER);
    }
    UINT newCount = uDelta + uCount;

    if (newCount > SHRT_MAX)
    {
        IFC(m_pBuilder->FlushReset());
        uCount = 0;
        newCount = uDelta;
    }

    if (newCount > m_rgVerticesTriList.GetCapacity())
    {
        IFC(m_rgVerticesTriList.ReserveSpace(uDelta));
    }

    m_rgVerticesTriList.SetCount(newCount);
    *pwIndexStart = static_cast<WORD>(uCount);
    *ppVertices = &m_rgVerticesTriList[uCount];

  Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::AddTriStripVertices
//
//  Synopsis:  Reserve space for consecutive vertices
//

template <class TVertex>
MIL_FORCEINLINE
HRESULT
CHwTVertexBuffer<TVertex>::AddTriStripVertices(
    UINT uCount,
    __deref_ecount(uCount) TVertex **ppVertices
    )
{
    HRESULT hr = S_OK;

    Assert(ppVertices);
#if DBG
    if (uCount != 6)
    {
        // Make a note that we added a tristrip using other than
        // 6 elements.
        m_fDbgNonLineSegmentTriangleStrip = true;
    }
#endif

    UINT Count = static_cast<UINT>(m_rgVerticesTriStrip.GetCount());
    UINT newCount = Count + uCount;

    if (newCount > m_rgVerticesTriStrip.GetCapacity())
    {
        IFC(m_rgVerticesTriStrip.ReserveSpace(uCount));
    }

    m_rgVerticesTriStrip.SetCount(newCount);
    *ppVertices = &m_rgVerticesTriStrip[Count];

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::AddNonIndexedTriListVertices
//
//  Synopsis:  Reserve space for triangle list vertices.
//

template <class TVertex>
MIL_FORCEINLINE
HRESULT
CHwTVertexBuffer<TVertex>::AddNonIndexedTriListVertices(
    UINT uCount,
    __deref_ecount(uCount) TVertex **ppVertices
    )
{
    HRESULT hr = S_OK;

    UINT Count = static_cast<UINT>(m_rgVerticesNonIndexedTriList.GetCount());
    UINT newCount = Count + uCount;

    if (newCount > m_rgVerticesNonIndexedTriList.GetCapacity())
    {
        IFC(m_rgVerticesNonIndexedTriList.ReserveSpace(uCount));
    }

    m_rgVerticesNonIndexedTriList.SetCount(newCount);
    *ppVertices = &m_rgVerticesNonIndexedTriList[Count];

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::AddLineListVertices
//
//  Synopsis:  Reserve space for consecutive vertices
//

template <class TVertex>
MIL_FORCEINLINE
HRESULT
CHwTVertexBuffer<TVertex>::AddLineListVertices(
    UINT uCount,
    __deref_ecount(uCount) TVertex **ppVertices
    )
{
    HRESULT hr = S_OK;

    Assert(ppVertices);

    UINT Count = static_cast<UINT>(m_rgVerticesLineList.GetCount());
    UINT newCount = Count + uCount;

    if (newCount > m_rgVerticesLineList.GetCapacity())
    {
        IFC(m_rgVerticesLineList.ReserveSpace(uCount));
    }

    m_rgVerticesLineList.SetCount(newCount);
    *ppVertices = &m_rgVerticesLineList[Count];

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Class:     CHwVertexBuffer::Builder
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:    CHwVertexBuffer::Builder::Create
//
//  Synopsis:  Choose the appropriate final vertex format and instantiate the
//             matching vertex builder
//

HRESULT
CHwVertexBuffer::Builder::Create(
    MilVertexFormat vfIn,
    MilVertexFormat vfOut,
    MilVertexFormatAttribute mvfaAntiAliasScaleLocation,
    __in_ecount_opt(1) CHwPipeline *pPipeline,
    __in_ecount_opt(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) CBufferDispenser *pBufferDispenser,
    __deref_out_ecount(1) CHwVertexBuffer::Builder **ppVertexBufferBuilder
    )
{
    HRESULT hr = S_OK;

    Assert(ppVertexBufferBuilder);

    *ppVertexBufferBuilder = NULL;

    if (!(vfOut & ~CHwTVertexBuffer<CD3DVertexXYZDUV2>::Builder::GetOutVertexFormat()))
    {
        CHwTVertexBuffer<CD3DVertexXYZDUV2> *pVB = pDevice->GetVB_XYZDUV2();
        CHwTVertexBuffer<CD3DVertexXYZDUV2>::Builder *pVBB = NULL;

        IFC(CHwTVertexBuffer<CD3DVertexXYZDUV2>::Builder::Create(
            pVB,
            vfIn,
            vfOut,
            mvfaAntiAliasScaleLocation,
            pBufferDispenser,
            &pVBB
            ));
        
        *ppVertexBufferBuilder = pVBB;
    }
    else if (!(vfOut & ~CHwTVertexBuffer<CD3DVertexXYZDUV8>::Builder::GetOutVertexFormat()))
    {
        CHwTVertexBuffer<CD3DVertexXYZDUV8> *pVB = pDevice->GetVB_XYZRHWDUV8();
        CHwTVertexBuffer<CD3DVertexXYZDUV8>::Builder *pVBB = NULL;

        IFC(CHwTVertexBuffer<CD3DVertexXYZDUV8>::Builder::Create(
            pVB,
            vfIn,
            vfOut,
            mvfaAntiAliasScaleLocation,
            pBufferDispenser,
            &pVBB
            ));

        *ppVertexBufferBuilder = pVBB;
    }
    else
    {
        // NOTE-2004/03/22-chrisra Adding another vertexbuffer type requires updating enum
        //
        // If we add another buffer builder type kMaxVertexBuilderSize enum in hwvertexbuffer.h file
        // needs to be updated to reflect possible changes to the maximum size of buffer builders.
        //
        IFC(E_NOTIMPL);
    }

    // Store the pipeline, if any, which this VBB can use to spill the vertex buffer to if it
    // overflows.
    (**ppVertexBufferBuilder).m_pPipelineNoRef = pPipeline;
    (**ppVertexBufferBuilder).m_pDeviceNoRef = pDevice;


Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwVertexBuffer::Builder::TransferUIntIndicesAsWords
//
//  Synopsis:  Move indices from one buffer to another while converting them
//             from UINT to WORD.
//
//-----------------------------------------------------------------------------
void
CHwVertexBuffer::Builder::TransferUIntIndicesAsWords(
    __in_ecount(cIndices) const UINT *rguInputIndices,
    __out_ecount_full(cIndices) WORD *rgwOutputIndices,
    __range(1, UINT_MAX) UINT cIndices
    )
{
    // Align write pointer to at least four bytes
    UINT_PTR uOutAddress =
        reinterpret_cast<UINT_PTR>(static_cast<void *>(rgwOutputIndices));
    Assert( !(uOutAddress & 0x1) );
    if (uOutAddress & 0x2)
    {
        *rgwOutputIndices++ = static_cast<WORD>(*rguInputIndices++);
        cIndices--;
    }

    // Write and many double words as possible
    while (cIndices > 1)
    {
        DWORD dwTwoWordIndices = *rguInputIndices++ & 0xFFFF;
        dwTwoWordIndices |= *rguInputIndices++ << 16;
        *reinterpret_cast<DWORD *>(rgwOutputIndices) = dwTwoWordIndices;
        rgwOutputIndices += 2;
        cIndices -= 2;
    }

    // Write any remaining single index
    if (cIndices)
    {
        *rgwOutputIndices = static_cast<WORD>(*rguInputIndices);
    }
}




//+----------------------------------------------------------------------------
//
//  Class:     THwTVertexMappings<class TVertex>
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Member:    THwTVertexMappings<TVertex>::THwTVertexMappings
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------

template <class TVertex>
CHwTVertexMappings<TVertex>::CHwTVertexMappings()
 :
    m_mvfMapped(MILVFAttrNone)
{
    for (int i = 0; i < ARRAY_SIZE(m_rgWaffleMode); ++i)
    {
        m_rgWaffleMode[i] = WaffleModeNone;
    }
    m_fAreWaffling = false;

    m_matPos2DTransform.SetIdentity();
}


//+----------------------------------------------------------------------------
//
//  Member:    THwTVertexMappings<TVertex>::SetPositionTransform
//
//  Synopsis:  Sets the position transform that needs to be applied.
//
//-----------------------------------------------------------------------------
template <class TVertex>
void 
CHwTVertexMappings<TVertex>::SetPositionTransform(
    __in_ecount(1) const MILMatrix3x2 &matPositionTransform
    )
{
    m_matPos2DTransform = matPositionTransform;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexMappings<TVertex>::SetConstantMapping
//
//  Synopsis:  Remember the static color for the given vertex field
//

template <class TVertex>
HRESULT
CHwTVertexMappings<TVertex>::SetConstantMapping(
    MilVertexFormatAttribute mvfaLocation,
    __in_ecount(1) const CHwConstantColorSource *pConstCS
    )
{
    HRESULT hr = S_OK;

    Assert(!(m_mvfMapped & mvfaLocation));
    pConstCS->GetColor(m_colorStatic);
    m_mvfMapped |= mvfaLocation;    // Remember this field has been mapped

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  GetMILVFAttributeOfTextureCoord
//
//  Synopsis:  Compute MilVertexFormatAttribute for a texture coordinate index
//

MIL_FORCEINLINE
MilVertexFormat
GetMILVFAttributeOfTextureCoord(
    DWORD dwCoordIndex
    )
{
    return MILVFAttrUV1 << dwCoordIndex;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexMappings<TVertex>::SetTextureMapping
//
//  Synopsis:  Remember the the transformation for generating texture
//             coordinates at the given index
//

template <class TVertex>
HRESULT
CHwTVertexMappings<TVertex>::SetTextureMapping(
    DWORD dwDestinationCoordIndex,
    DWORD dwSourceCoordIndex,
    __in_ecount(1) const MILMatrix3x2 *pmatDevicePointToTextureUV
    )
{
    HRESULT hr = S_OK;

    // The array size is not accessible to this class.  The assert is left here
    // for anyone debugging this code to check.
//    Assert(dwDestinationCoordIndex < ARRAY_SIZE(m_rgmatPointToUV));

    // Compute single bit of UV location from coord index
    MilVertexFormat mvfLocation =
        GetMILVFAttributeOfTextureCoord(dwDestinationCoordIndex);

    Assert(!(m_mvfMapped & mvfLocation));

    // Only mappings using matrix transforms from the position is supported
    if (dwSourceCoordIndex != DWORD_MAX) IFC(E_NOTIMPL);
    if (!pmatDevicePointToTextureUV) IFC(E_NOTIMPL);

    m_rgmatPointToUV[dwDestinationCoordIndex] = *pmatDevicePointToTextureUV;
    
    m_mvfMapped |= mvfLocation;     // Remember this field has been mapped

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexMappings<TVertex>::SetWaffling
//
//  Synopsis:  Remember the waffling parameters for the
//             coordinates at the given index
//

template <class TVertex>
HRESULT
CHwTVertexMappings<TVertex>::SetWaffling(
    DWORD dwCoordIndex,
    __in_ecount(1) const CMilPointAndSizeF *pSubrect,
    WaffleModeFlags waffleMode
    )
{
    HRESULT hr = S_OK;

    m_rgSubrect[dwCoordIndex] = *pSubrect;
    m_rgWaffleMode[dwCoordIndex] = waffleMode;

    if (waffleMode & WaffleModeEnabled)
    {
        m_fAreWaffling = true;
    }
    else
    {
        m_fAreWaffling = false;
        for (int i = 0; i < NUM_OF_VERTEX_TEXTURE_COORDS(TVertex); ++i)
        {
            if (m_rgWaffleMode[i] & WaffleModeEnabled)
            {
                m_fAreWaffling = true;
                break;
            }
        }
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexMappings<TVertex>::PointToUV
//
//  Synopsis:  Helper function to populate the texture coordinates at the given
//             index using the given point
//

template <class TVertex>
MIL_FORCEINLINE void
CHwTVertexMappings<TVertex>::PointToUV(
    __in_ecount(1) const MilPoint2F &ptIn,
    __bound UINT uIndex,
    __out_ecount(1) TVertex *pvOut
    )
{
    m_rgmatPointToUV[uIndex].TransformPoint(
        &pvOut->ptTx[uIndex],
        ptIn.X,
        ptIn.Y
        );
}





//+----------------------------------------------------------------------------
//
//  Class:     CHwTVertexBuffer<TVertex>::Builder
//
//-----------------------------------------------------------------------------

//+----------------------------------------------------------------------------
//
//  Table:      CHwTVertexBuffer<TVertex>::Builder::PFN_ExpandVertices
//
//  Synopsis:   Lookup table of optimized vertex expansion methods
//
//-----------------------------------------------------------------------------

template <class TVertex>
const typename CHwTVertexBuffer<TVertex>::Builder::PFN_ExpandVertices
CHwTVertexBuffer<TVertex>::Builder::sc_pfnExpandVerticesTable[8*2] =
{

//
// [pfx_parse] - workaround for PREfix parse problems
//
#ifndef _PREFIX_

    // No Falloff computations
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrNone, MILVFAttrNone>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrZ,    MILVFAttrNone>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrNone | MILVFAttrDiffuse, MILVFAttrNone>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrZ    | MILVFAttrDiffuse, MILVFAttrNone>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrNone | MILVFAttrUV1, MILVFAttrNone>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrZ    | MILVFAttrUV1, MILVFAttrNone>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrNone | MILVFAttrDiffuse | MILVFAttrUV1, MILVFAttrNone>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrZ    | MILVFAttrDiffuse | MILVFAttrUV1, MILVFAttrNone>,

    // AntiAliasing via Alpha Falloff
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrNone, MILVFAttrDiffuse>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrZ,    MILVFAttrDiffuse>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrNone | MILVFAttrDiffuse, MILVFAttrDiffuse>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrZ    | MILVFAttrDiffuse, MILVFAttrDiffuse>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrNone | MILVFAttrUV1, MILVFAttrDiffuse>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrZ    | MILVFAttrUV1, MILVFAttrDiffuse>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrNone | MILVFAttrDiffuse | MILVFAttrUV1, MILVFAttrDiffuse>,
    &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrZ    | MILVFAttrDiffuse | MILVFAttrUV1, MILVFAttrDiffuse>,

#endif // !_PREFIX_

};


//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::GetOutVertexFormat
//
//  Synopsis:  Return MIL vertex format covered by specific builders
//
//-----------------------------------------------------------------------------

template <>
MilVertexFormat
CHwTVertexBuffer<CD3DVertexXYZDUV2>::Builder::GetOutVertexFormat()
{
    return (MILVFAttrXYZ | MILVFAttrDiffuse | MILVFAttrUV2);
}

template <>
MilVertexFormat
CHwTVertexBuffer<CD3DVertexXYZDUV8>::Builder::GetOutVertexFormat()
{
    return (MILVFAttrXYZ | MILVFAttrDiffuse | MILVFAttrUV8);
}

template <>
MilVertexFormat
CHwTVertexBuffer<CD3DVertexXYZDUV6>::Builder::GetOutVertexFormat()
{
    return (MILVFAttrXYZ | MILVFAttrDiffuse | MILVFAttrUV6);
}

template <>
MilVertexFormat
CHwTVertexBuffer<CD3DVertexXYZNDSUV4>::Builder::GetOutVertexFormat()
{
    return (MILVFAttrXYZ |
            MILVFAttrNormal |
            MILVFAttrDiffuse |
            MILVFAttrSpecular |
            MILVFAttrUV4);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::Create
//
//  Synopsis:  Instantiate a specific type of vertex builder
//

template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::Create(
    __in_ecount(1) CHwTVertexBuffer<TVertex> *pVertexBuffer,
    MilVertexFormat mvfIn,
    MilVertexFormat mvfOut,
    MilVertexFormatAttribute mvfaAntiAliasScaleLocation,
    __inout_ecount(1) CBufferDispenser *pBufferDispenser,
    __deref_out_ecount(1) typename CHwTVertexBuffer<TVertex>::Builder **ppVertexBufferBuilder
    )
{
    HRESULT hr = S_OK;

    Assert(ppVertexBufferBuilder);

    *ppVertexBufferBuilder = NULL;

    CHwTVertexBuffer<TVertex>::Builder *pVertexBufferBuilder;

    pVertexBufferBuilder =
        new(pBufferDispenser) CHwTVertexBuffer<TVertex>::Builder(pVertexBuffer);
    IFCOOM(pVertexBufferBuilder);

    IFC(pVertexBufferBuilder->SetupConverter(
        mvfIn,
        mvfOut,
        mvfaAntiAliasScaleLocation
        ));

    *ppVertexBufferBuilder = pVertexBufferBuilder;
    pVertexBufferBuilder = NULL;

Cleanup:
    if (pVertexBufferBuilder)
    {
        delete pVertexBufferBuilder;
    }
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::Builder
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------

template <class TVertex>
CHwTVertexBuffer<TVertex>::Builder::Builder(
    __in_ecount(1) CHwTVertexBuffer<TVertex> *pVertexBuffer
    )
{
    Assert(pVertexBuffer);

    m_pVB = pVertexBuffer;


    m_rgoPrecomputedTriListVertices = NULL;
    m_cPrecomputedTriListVertices = 0;

    m_rguPrecomputedTriListIndices = NULL;
    m_cPrecomputedTriListIndices = 0;

    // These two track the Y extent of the shape this builder is producing.
    m_rCurStratumTop = +FLT_MAX;
    m_rCurStratumBottom = -FLT_MAX;
    m_fNeedOutsideGeometry = false;
    m_fNeedInsideGeometry = true;

    m_rLastTrapezoidRight = -FLT_MAX;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::SetupConverter
//
//  Synopsis:  Choose the appropriate conversion method
//

template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::SetupConverter(
    MilVertexFormat mvfIn,
    MilVertexFormat mvfOut,
    MilVertexFormatAttribute mvfaAntiAliasScaleLocation
    )
{
    HRESULT hr = S_OK;

    m_mvfIn = mvfIn;

#if DBG
    m_mvfDbgOut = mvfOut;
#endif

    m_mvfGenerated = mvfOut & ~m_mvfIn;
    m_mvfaAntiAliasScaleLocation = mvfaAntiAliasScaleLocation;

    Assert(!(m_mvfGenerated & MILVFAttrXY));

    m_pfnExpandVertices = NULL;

    const MilVertexFormat mvfFastSupport =
        MILVFAttrZ | MILVFAttrDiffuse | MILVFAttrUV1;

    if ((m_mvfGenerated & ~mvfFastSupport) == 0)
    {
        UINT uConvIndex =
              ((m_mvfGenerated & MILVFAttrZ)        ? 1 : 0)
            + ((m_mvfGenerated & MILVFAttrDiffuse)  ? 2 : 0)
            + ((m_mvfGenerated & MILVFAttrUV1)      ? 4 : 0);

        Assert(uConvIndex < ARRAY_SIZE(sc_pfnExpandVerticesTable)/2);

        uConvIndex += ((m_mvfaAntiAliasScaleLocation & MILVFAttrDiffuse) ?
                       ARRAY_SIZE(sc_pfnExpandVerticesTable)/2 : 0);

        m_pfnExpandVertices = sc_pfnExpandVerticesTable[uConvIndex];
    }
    else if (m_mvfGenerated == (MILVFAttrZ | MILVFAttrUV8))
    {
        if (mvfIn == MILVFAttrXY)
        {
            m_pfnExpandVertices = &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrZ | MILVFAttrUV8, MILVFAttrNone>;
        }
        else if (   (mvfIn == (MILVFAttrXY | MILVFAttrDiffuse)) 
                 && (mvfaAntiAliasScaleLocation == MILVFAttrDiffuse)
                )
        {
            m_pfnExpandVertices = &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesFast<MILVFAttrZ | MILVFAttrUV8, MILVFAttrDiffuse>;
        }
    }
    else if (m_mvfGenerated & (MILVFAttrNormal | MILVFAttrSpecular))
    {
        m_pfnExpandVertices = &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesInvalid;
        IFC(E_NOTIMPL);
    }

    if (!m_pfnExpandVertices)
    {
        m_pfnExpandVertices = &CHwTVertexBuffer<TVertex>::Builder::ExpandVerticesGeneral;
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::BuilderRenderPrecomputedIndexedTriangles
//
//  Synopsis:  Render the pre calculated triangles.  I couldn't think of a better location
//             for this.  It will probably be more apparent after we change everything to
//             triangle strips and write directly into hw memory.
//
//-----------------------------------------------------------------------------
template <class TVertex>
HRESULT 
CHwTVertexBuffer<TVertex>::Builder::RenderPrecomputedIndexedTriangles(
    __range(1, SHORT_MAX) UINT cVertices,
    __in_ecount(cVertices) const TVertex *rgoVertices,
    __range(1, UINT_MAX) UINT cIndices,
    __in_ecount(cIndices) const UINT *rguIndices
    )
{
    HRESULT hr = S_OK;

    CHwD3DVertexBuffer *pVertexBufferNoRef = NULL;
    CHwD3DIndexBuffer *pIndexBufferNoRef = NULL;

    UINT uStartIndex = 0;
    UINT uStartVertex = 0;

    TVertex *poLockedVertices = NULL;
    WORD *pwLockedIndices = NULL;

    bool fVertexBufferLocked = false;
    bool fIndexBufferLocked = false;

    pVertexBufferNoRef = m_pDeviceNoRef->Get3DVertexBuffer();
    pIndexBufferNoRef = m_pDeviceNoRef->Get3DIndexBuffer();

    IFC(pVertexBufferNoRef->Lock(
        cVertices,
        sizeof(rgoVertices[0]),
        reinterpret_cast<void **>(&poLockedVertices),
        &uStartVertex
        ));
    fVertexBufferLocked = true;

    IFC(pIndexBufferNoRef->Lock(
        cIndices,
        &pwLockedIndices,
        &uStartIndex
        ));
    fIndexBufferLocked = true;

    TransferAndExpandVerticesGeneral(
        cVertices,
        rgoVertices,
        poLockedVertices,
        true
        );

    TransferUIntIndicesAsWords(
        rguIndices,
        pwLockedIndices,
        cIndices
        );

    IFC(pVertexBufferNoRef->Unlock(
        cVertices
        ));
    fVertexBufferLocked = false;

    IFC(pIndexBufferNoRef->Unlock());
    fIndexBufferLocked = false;

    IFC(m_pDeviceNoRef->SetStreamSource(
        pVertexBufferNoRef->GetD3DBuffer(),
        sizeof(TVertex)
        ));

    IFC(m_pDeviceNoRef->SetIndices(
        pIndexBufferNoRef->GetD3DBuffer()
        ));

    IFC(m_pDeviceNoRef->DrawIndexedTriangleList(
        uStartVertex,
        0,
        cVertices,
        uStartIndex,
        cIndices/3
        ));

Cleanup:
    if (fVertexBufferLocked)
    {
        IGNORE_HR(pVertexBufferNoRef->Unlock(cVertices));
    }

    if (fIndexBufferLocked)
    {
        IGNORE_HR(pVertexBufferNoRef->Unlock(cVertices));
    }

    RRETURN(hr);
}

                                                                  
//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::SetTransformMapping
//
//  Synopsis:  Delegate mapping sets to CHwTVertexMappings
//
//-----------------------------------------------------------------------------

template <class TVertex>
void
CHwTVertexBuffer<TVertex>::Builder::SetTransformMapping(
    __in_ecount(1) const MILMatrix3x2 &mat2DPositionTransform
    )
{
    m_map.SetPositionTransform(mat2DPositionTransform);
}
                                                                    
template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::SetConstantMapping(
    MilVertexFormatAttribute mvfaLocation,
    __in_ecount(1) const CHwConstantColorSource *pConstCS
    )
{
    HRESULT hr = S_OK;

    IFC(m_map.SetConstantMapping(mvfaLocation, pConstCS));

Cleanup:
    RRETURN(hr);
}

template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::SetTextureMapping(
    DWORD dwDestinationCoordIndex,
    DWORD dwSourceCoordIndex,
    __in_ecount(1) const MILMatrix3x2 *pmatDevicePointToTextureUV
    )
{
    HRESULT hr = S_OK;

    IFC(m_map.SetTextureMapping(
        dwDestinationCoordIndex,
        dwSourceCoordIndex,
        pmatDevicePointToTextureUV
        ));
    
Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::SetWaffling
//
//  Synopsis:  Delegate texture waffling sets to CHwTVertexMappings
//
//-----------------------------------------------------------------------------

template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::SetWaffling(
    DWORD dwCoordIndex,
    __in_ecount(1) const CMilPointAndSizeF *pSubrect,
    WaffleModeFlags waffleMode
    )
{
    HRESULT hr = S_OK;

    IFC(m_map.SetWaffling(
            dwCoordIndex,
            pSubrect,
            waffleMode
            ));
    
Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::FinalizeMappings
//
//  Synopsis:  Complete setup of vertex mappings
//

template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::FinalizeMappings(
    )
{
    HRESULT hr = S_OK;

    //
    // Set default Z if required.
    //

    if (m_mvfGenerated & MILVFAttrZ)
    {
        if (!(m_map.m_mvfMapped & MILVFAttrZ))
        {
            m_map.m_vStatic.Z = 0.5f;
        }
    }

    //
    // If AA falloff is not going to scale the diffuse color and it is
    // generated then see if the color is constant such that we can do any
    // complex conversions just once here instead of in every iteration of the
    // expansion loop.  If AA falloff is going to scale the diffuse color then
    // we can still optimize for the falloff = 1.0 case by precomputing that
    // color now and checking for 1.0 during generation.  Such a precomputation
    // has shown significant to performance.
    //

    if (m_mvfGenerated & MILVFAttrDiffuse)
    {
        if (m_map.m_mvfMapped & MILVFAttrDiffuse)
        {

            // Assumes diffuse color is constant
            m_map.m_vStatic.Diffuse =
                Convert_MilColorF_scRGB_To_Premultiplied_MilColorB_sRGB(&m_map.m_colorStatic);
        }
        else
        {
            // Set default Diffuse value: White
            m_map.m_vStatic.Diffuse = MIL_COLOR(0xFF,0xFF,0xFF,0xFF);
        }
    }

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::ViewportToPackedCoordinates
//
//  Synopsis:  Transform the normalized viewport texture coordinates into
//             normalized packed coordinates suitable for sending to the HW
//             This involves 1. Wrapping, and 2. Remapping
//             (see waffling-and-packing.txt Sec COORDINATE SYSTEMS)
//             NB: The first parameter - uGroupCount - is the count of complete
//             groups of uGroupSize present in pVertex.
//
//-----------------------------------------------------------------------------
template <class TVertex>
void
CHwTVertexBuffer<TVertex>::Builder::ViewportToPackedCoordinates(
    __range(1,UINT_MAX / uGroupSize) UINT uGroupCount,           // Number of vertices
    __inout_ecount(uGroupCount * uGroupSize) TVertex *pVertex,   // Pointer to vertices to process
    __range(2,6) UINT uGroupSize,                                // Number of vertices in a primitive
                                                                 // i.e. 2 for lines, 3 for tris
    /*__range(0,NUM_OF_VERTEX_TEXTURE_COORDS(TVertex)-1)*/ __bound UINT uIndex
                                                                 // Index of texture coord to process
    )
{
    Assert(uIndex < NUM_OF_VERTEX_TEXTURE_COORDS(TVertex));

    // uGroupSize must be 2, 3 or 6 
    Assert(uGroupSize == 2 || uGroupSize == 3 || uGroupSize == 6);

    bool flipX = (m_map.m_rgWaffleMode[uIndex] & WaffleModeFlipX) != 0;
    bool flipY = (m_map.m_rgWaffleMode[uIndex] & WaffleModeFlipY) != 0;

    // Take each group (e.g. two points making a line segment or three making a triangle) of texture
    // coordinates and find their centroid.  Then compute the integer i,j to subtract to bring the^
    // centroid into the [0,1) x [0,1) base tile.  And subtract it, of course.

    TVertex *pv = pVertex;

    for (UINT i = 0; i < uGroupCount; i++, pv += uGroupSize)
    {
        float x = 0;
        float y = 0;

        for (UINT j = 0; j < uGroupSize; ++j)
        {
            x += pv[j].ptTx[uIndex].X;
            y += pv[j].ptTx[uIndex].Y;
        }
        x = static_cast<float>(GpFloorSat(x/uGroupSize));
        y = static_cast<float>(GpFloorSat(y/uGroupSize));

        // Use comparison with zero not 1 here because of negatives.
        bool flipThisX = flipX && ((int)x)%2 != 0;
        bool flipThisY = flipY && ((int)y)%2 != 0;

        for (UINT j = 0; j < uGroupSize; ++j)
        {
            pv[j].ptTx[uIndex].X -= x;
            if (flipThisX)
            {
                pv[j].ptTx[uIndex].X = 1 - pv[j].ptTx[uIndex].X;
            }
            pv[j].ptTx[uIndex].Y -= y;
            if (flipThisY)
            {
                pv[j].ptTx[uIndex].Y = 1 - pv[j].ptTx[uIndex].Y;
            }
        }
    }

    // Then transform the base tile to the subrect of the actual texture which corresponds to the
    // base tile.  Generally this will be the rect inset by 1 pixel on each side to account for the
    // extra border of texels we added to make sampling work right with our tiling "by hand."
    const CMilPointAndSizeF rcTexture = m_map.m_rgSubrect[uIndex];
    for (UINT i = 0; i < (uGroupCount * uGroupSize); ++i)
    {
        float &x = pVertex[i].ptTx[uIndex].X;
        float &y = pVertex[i].ptTx[uIndex].Y;

        x = rcTexture.X + x * rcTexture.Width;
        y = rcTexture.Y + y * rcTexture.Height;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::ViewportToPackedCoordinates
//
//  Synopsis:  Transform the normalized viewport texture coordinates into
//             normalized packed coordinates suitable for sending to the HW
//             This function calls the other overload for each set of texture
//             coordinates that need it.
//             NB: The first parameter - uGroupCount - is the count of complete
//             groups of uGroupSize present in pVertex.
//
//-----------------------------------------------------------------------------
template <class TVertex>
void
CHwTVertexBuffer<TVertex>::Builder::ViewportToPackedCoordinates(
    __range(1,UINT_MAX / uGroupSize) UINT uGroupCount,           // Number of vertices
    __inout_ecount(uGroupCount * uGroupSize) TVertex *pVertex,   // Pointer to vertices to process
    __range(2,6) UINT uGroupSize                                 // Number of vertices in a primitive
                                                                 // i.e. 2 for lines, 3 for tris
    )
{
    Assert(uGroupSize == 2 || uGroupSize == 3 || uGroupSize == 6);

    // Future Consideration:   Check out perf for multiple texture waffling
    //  as iterating through vertex list for each texture coordinate can be
    //  costly for decent amounts of vertices.  When waffling just one set of
    //  texture coordinates this organization is definitely the fastest.
    for (int i = 0; i < NUM_OF_VERTEX_TEXTURE_COORDS(TVertex); ++i)
    {
        DWORD dwMask = MILVFAttrUV1 << i;
        if ((m_mvfGenerated & dwMask) && (m_map.m_rgWaffleMode[i] & WaffleModeEnabled))
        {
            ViewportToPackedCoordinates(uGroupCount, pVertex, uGroupSize, i);
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::SetOutsideBounds
//
//
//  Synopsis:  Enables rendering geometry for areas outside the shape but
//             within the bounds.  These areas will be created with
//             zero alpha.
//

template <class TVertex>
void
CHwTVertexBuffer<TVertex>::Builder::SetOutsideBounds(
    __in_ecount_opt(1) const CMILSurfaceRect *prcOutsideBounds,
    bool fNeedInside
    )
{
    // Waffling and outside bounds is not currently implemented.  It's
    // not difficult to do but currently there is no need.
    Assert(!(AreWaffling() && prcOutsideBounds));

    if (prcOutsideBounds)
    {
        m_rcOutsideBounds = *prcOutsideBounds;
        m_fNeedOutsideGeometry = true;
        m_fNeedInsideGeometry = fNeedInside;
    }
    else
    {
        m_fNeedOutsideGeometry = false;
        m_fNeedInsideGeometry = true;
    }
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::BeginBuilding
//
//  Synopsis:  Prepare for a new primitive by resetting the vertex buffer
//

template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::BeginBuilding(
    )
{
    HRESULT hr = S_OK;

    m_fHasFlushed = false;
    m_pVB->Reset(this);

    // We need to know the viewport that this vertex buffer will be applied
    // to because a horizontal line through the first row of the viewport
    // can be incorrectly clipped.
    // This assumes that we've already set the viewport & we won't use
    // the vertex buffer with any other viewport.
    MilPointAndSizeL rcViewport = m_pDeviceNoRef->GetViewport();
    m_iViewportTop = rcViewport.Y;

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::AddVertex
//
//  Synopsis:  Add a vertex to the vertex buffer
//
//             Remember just the given vertex information now and convert later
//             in a single, more optimal pass.
//

template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::AddVertex(
    __in_ecount(1) const MilPoint2F &ptPosition,
        // Vertex coordinates
    __out_ecount(1) WORD *pIndex
        // The index of the new vertex
    )
{
    HRESULT hr = S_OK;

    Assert(!NeedOutsideGeometry());
    Assert(m_mvfIn == MILVFAttrXY);

    TVertex *pVertex;

    IFC(m_pVB->AddTriListVertices(1, &pVertex, pIndex));

    pVertex->ptPt = ptPosition;

    //  store coverage as a DWORD instead of float

    pVertex->Diffuse = FLOAT_ONE;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::AddIndexedVertices, IGeometrySink
//
//  Synopsis:  Add a fully computed, indexed vertex to the vertex buffer
//

template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::AddIndexedVertices(
    UINT cVertices,
        // In: number of vertices                                                       
    __in_bcount(cVertices*uVertexStride) const void *pVertexBufferNoRef,
        // In: vertex buffer containing the vertices                                    
    UINT uVertexStride,
        // In: size of each vertex                                                      
    MilVertexFormat mvfFormat,
        // In: format of each vertex                                                    
    UINT cIndices,
        // In: Number of indices                                                        
    __in_ecount(cIndices) const UINT *puIndexBuffer
        // In: index buffer                                                             
    )
{
    Assert(m_mvfIn & (MILVFAttrXYZ | MILVFAttrDiffuse | MILVFAttrUV2));
    Assert(mvfFormat == (MILVFAttrXYZ | MILVFAttrDiffuse | MILVFAttrUV2));

    Assert(uVertexStride == sizeof(TVertex));

    m_rgoPrecomputedTriListVertices = reinterpret_cast<const TVertex *>(pVertexBufferNoRef);
    m_cPrecomputedTriListVertices = cVertices;

    m_rguPrecomputedTriListIndices = puIndexBuffer;
    m_cPrecomputedTriListIndices = cIndices;

    return S_OK;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::AddTriangle
//
//  Synopsis:  Add a triangle to the vertex buffer
//

template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::AddTriangle(
    DWORD i1,                    // In: Index of triangle's first vertex
    DWORD i2,                    // In: Index of triangle's second vertex
    DWORD i3                     // In: Index of triangle's third vertex
    )
{
    HRESULT hr = S_OK;

    Assert(!NeedOutsideGeometry());

    if (AreWaffling())
    {
        TVertex *pVertex;
        UINT uNumVertices;
        m_pVB->GetTriListVertices(&pVertex, &uNumVertices);

        Assert(i1 < uNumVertices);
        Assert(i2 < uNumVertices);
        Assert(i3 < uNumVertices);

        PointXYA rgPoints[3];
        rgPoints[0].x = pVertex[i1].ptPt.X;
        rgPoints[0].y = pVertex[i1].ptPt.Y;
        rgPoints[0].a = 1;
        rgPoints[1].x = pVertex[i2].ptPt.X;
        rgPoints[1].y = pVertex[i2].ptPt.Y;
        rgPoints[1].a = 1;
        rgPoints[2].x = pVertex[i3].ptPt.X;
        rgPoints[2].y = pVertex[i3].ptPt.Y;
        rgPoints[2].a = 1;
        
        TriangleWaffler<PointXYA> wafflers[NUM_OF_VERTEX_TEXTURE_COORDS(TVertex) * 2];
        TriangleWaffler<PointXYA>::ISink *pWaffleSinkNoRef = BuildWafflePipeline(wafflers);
        IFC(pWaffleSinkNoRef->AddTriangle(rgPoints[0], rgPoints[1], rgPoints[2]));
    }
    else
    {
        IFC(m_pVB->AddTriangle(
                static_cast<WORD>(i1),
                static_cast<WORD>(i2),
                static_cast<WORD>(i3)
                ));
    }
    
Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::NeedCoverageGeometry
//
//  Synopsis:  Returns true if the coverage value needs to be rendered
//             based on NeedInsideGeometry() and NeedOutsideGeometry()
//
//             Two cases where we don't need to generate geometry:
//              1. NeedInsideGeometry is false, and coverage is c_nShiftSizeSquared.
//              2. NeedOutsideGeometry is false and coverage is 0
//
//-----------------------------------------------------------------------------
template <class TVertex>
MIL_FORCEINLINE bool
CHwTVertexBuffer<TVertex>::Builder::NeedCoverageGeometry(
    INT nCoverage
    ) const
{
    return    (NeedInsideGeometry()  || nCoverage != c_nShiftSizeSquared)
           && (NeedOutsideGeometry() || nCoverage != 0);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::AddComplexScan
//
//  Synopsis:  Add a coverage span to the vertex buffer
//
//-----------------------------------------------------------------------------
template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::AddComplexScan(
    INT nPixelY,
        // In: y coordinate in pixel space
    __in_ecount(1) const CCoverageInterval *pIntervalSpanStart
        // In: coverage segments
    )
{
    HRESULT hr = S_OK;
    TVertex *pVertex = NULL;

    IFC(PrepareStratum(static_cast<float>(nPixelY),
                  static_cast<float>(nPixelY+1),
                  false /* Not a trapezoid. */ ));

    float rPixelY = float(nPixelY) + 0.5f;

    LineWaffler<PointXYA> wafflers[NUM_OF_VERTEX_TEXTURE_COORDS(TVertex) * 2];

    // Use sink for waffling & the first line fix up (aka the complicated cases.)
    ILineSink<PointXYA> *pLineSink = NULL;

    if (AreWaffling())
    {
        bool fWafflersUsed;
        pLineSink = BuildWafflePipeline(wafflers, OUT fWafflersUsed);
        if (!fWafflersUsed)
        {
            pLineSink = NULL;
        }
    }
    
    // Use triangles instead of lines, for lines too close to the top of the viewport
    // because lines are clipped (before rasterization) against a viewport that only
    // includes half of the top pixel row.  Waffling will take care of this separately.
    if (!pLineSink && rPixelY < GetViewportTop() + 1)
    {
        pLineSink = m_pVB;
    }

    //
    // Output all segments if creating outside geometry, otherwise only output segments
    // with non-zero coverage.
    //

    if (!pLineSink)
    {
        UINT nSegmentCount = 0;

        for (const CCoverageInterval *pIntervalSpanTemp = pIntervalSpanStart;
             pIntervalSpanTemp->m_nPixelX != INT_MAX;
             pIntervalSpanTemp = pIntervalSpanTemp->m_pNext
             )
        {
            if (NeedCoverageGeometry(pIntervalSpanTemp->m_nCoverage))
            {
                ++nSegmentCount;
            }
        }

        //
        // Add vertices
        //
        if (nSegmentCount)
        {
            IFC(m_pVB->AddLineListVertices(nSegmentCount*2, &pVertex));
        }
    }

    //
    // Having allocated space (if not using sink), now let's actually output the vertices.
    //

    while (pIntervalSpanStart->m_nPixelX != INT_MAX)
    {
        Assert(pIntervalSpanStart->m_pNext != NULL);

        //
        // Output line list segments
        //
        // Note that line segments light pixels by going through the the
        // "diamond" interior of a pixel.  While we could accomplish this
        // by going from left edge to right edge of pixel, D3D10 uses the
        // convention that the LASTPIXEL is never lit.  We respect that now
        // by setting D3DRS_LASTPIXEL to FALSE and use line segments that
        // start in center of first pixel and end in center of one pixel
        // beyond last.
        //
        // Since our top left corner is integer, we add 0.5 to get to the
        // pixel center.
        //
        if (NeedCoverageGeometry(pIntervalSpanStart->m_nCoverage))
        {
            float rCoverage = static_cast<float>(pIntervalSpanStart->m_nCoverage)/static_cast<float>(c_nShiftSizeSquared);
            
            LONG iBegin = pIntervalSpanStart->m_nPixelX;
            LONG iEnd = pIntervalSpanStart->m_pNext->m_nPixelX;
            if (NeedOutsideGeometry())
            {
                // Intersect the interval with the outside bounds to create
                // start and stop lines.  The scan begins (ends) with an
                // interval starting (ending) at -inf (+inf).

                // The given geometry is not guaranteed to be within m_rcOutsideBounds but
                // the additional inner min and max (in that order) produce empty spans
                // for intervals not intersecting m_rcOutsideBounds.
                //
                // We could cull here but that should really be done by the geometry
                // generator.

                iBegin = max(iBegin, min(iEnd, m_rcOutsideBounds.left));
                iEnd = min(iEnd, max(iBegin, m_rcOutsideBounds.right));
            }
            float rPixelXBegin = float(iBegin) + 0.5f;
            float rPixelXEnd = float(iEnd) + 0.5f;

            //
            // Output line (linelist or tristrip) for a pixel
            //

            if (pLineSink)
            {
                PointXYA v0,v1;
                v0.x = rPixelXBegin;
                v0.y = rPixelY;
                v0.a = rCoverage;

                v1.x = rPixelXEnd;
                v1.y = rPixelY;
                v1.a = rCoverage;

                IFC(pLineSink->AddLine(v0,v1));
            }
            else
            {
                DWORD dwDiffuse = ReinterpretFloatAsDWORD(rCoverage);

                pVertex[0].ptPt.X = rPixelXBegin;
                pVertex[0].ptPt.Y = rPixelY;
                pVertex[0].Diffuse = dwDiffuse;

                pVertex[1].ptPt.X = rPixelXEnd;
                pVertex[1].ptPt.Y = rPixelY;
                pVertex[1].Diffuse = dwDiffuse;

                // Advance output vertex pointer
                pVertex += 2;
            }
        }

        //
        // Advance coverage buffer
        //

        pIntervalSpanStart = pIntervalSpanStart->m_pNext;
    }

Cleanup:
    RRETURN(hr);
}
//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::AddLineAsTriangleStrip
//
//  Synopsis:  Adds a horizontal line as a triangle strip to work around
//             issue in D3D9 where horizontal lines with y = 0 may not render.
//
//              Line clipping in D3D9
//             This behavior will change in D3D10 and this work-around will no
//             longer be needed.  (Pixel center conventions will also change.)
//              
//-----------------------------------------------------------------------------
template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::AddLineAsTriangleStrip(
    __in_ecount(1) const TVertex *pBegin, // Begin
    __in_ecount(1) const TVertex *pEnd    // End
    )
{
    HRESULT hr = S_OK;
    TVertex *pVertex;

    // Collect pertinent data from vertices.
    Assert(pBegin->ptPt.Y == pEnd->ptPt.Y);
    Assert(pBegin->Diffuse == pEnd->Diffuse);

    // Offset begin and end X left by 0.5 because the line starts on the first
    // pixel center and ends on the center of the pixel after the line segment.
    const float x0 = pBegin->ptPt.X - 0.5f;
    const float x1 = pEnd->ptPt.X - 0.5f;
    const float y = pBegin->ptPt.Y;
    const DWORD dwDiffuse = pBegin->Diffuse;

    //
    // Add the vertices
    //

    IFC(AddTriStripVertices(6, &pVertex));

    //
    // Duplicate the first vertex.  Assuming that the previous two
    // vertices in the tristrip are coincident then the first three
    // vertices here create degenerate triangles.  If this is the
    // beginning of the strip the first two vertices fill the pipe,
    // the third creates a degenerate vertex.  In either case the
    // fourth creates the first triangle in our quad.
    // 
    pVertex[0].ptPt.X = x0;
    pVertex[0].ptPt.Y = y  - 0.5f;
    pVertex[0].Diffuse = dwDiffuse;
    
    // Offset two vertices up and two down to form a 1-pixel-high quad.
    // Order is TL-BL-TR-BR.
    pVertex[1].ptPt.X = x0;
    pVertex[1].ptPt.Y = y  - 0.5f;
    pVertex[1].Diffuse = dwDiffuse;
    pVertex[2].ptPt.X = x0;
    pVertex[2].ptPt.Y = y  + 0.5f;
    pVertex[2].Diffuse = dwDiffuse;
    pVertex[3].ptPt.X = x1;
    pVertex[3].ptPt.Y = y  - 0.5f;
    pVertex[3].Diffuse = dwDiffuse;
    pVertex[4].ptPt.X = x1;
    pVertex[4].ptPt.Y = y  + 0.5f;
    pVertex[4].Diffuse = dwDiffuse;
    
    //
    // Duplicate the last vertex. This creates a degenerate triangle
    // and sets up the next tristrip to create three more degenerate
    // triangles.
    // 
    pVertex[5].ptPt.X = x1;
    pVertex[5].ptPt.Y = y  + 0.5f;
    pVertex[5].Diffuse = dwDiffuse;

  Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::AddParallelogram
//
//  Synopsis:  This function adds the coordinates of a parallelogram to the vertex strip buffer. 
//
//  Parameter: rgPosition contains four coordinates of the parallelogram. Coordinates should have 
//              a winding order
//
//-----------------------------------------------------------------------------
template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::AddParallelogram(
        __in_ecount(4)  const MilPoint2F *rgPosition
        )
{
    HRESULT hr = S_OK;

    if (AreWaffling())
    {
        PointXYA rgPoints[4];
        for (int i = 0; i < 4; ++i)
        {
            rgPoints[i].x = rgPosition[i].X;
            rgPoints[i].y = rgPosition[i].Y;
            rgPoints[i].a = 1;
        }
        TriangleWaffler<PointXYA> wafflers[NUM_OF_VERTEX_TEXTURE_COORDS(TVertex) * 2];
        TriangleWaffler<PointXYA>::ISink *pWaffleSinkNoRef = BuildWafflePipeline(wafflers);
        IFC(pWaffleSinkNoRef->AddTriangle(rgPoints[0], rgPoints[1], rgPoints[3]));
        IFC(pWaffleSinkNoRef->AddTriangle(rgPoints[3], rgPoints[1], rgPoints[2]));
    }
    else
    {
        TVertex *pVertex;
  
        //
        // Add the vertices
        //

        IFC(m_pVB->AddTriStripVertices(6, &pVertex));

        //
        // Duplicate the first vertex. This creates 2 degenerate triangles: one connecting
        // the previous rect to this one and another between vertices 0 and 1.
        //

        pVertex[0].ptPt = rgPosition[0];
        pVertex[0].Diffuse = FLOAT_ONE;

        pVertex[1].ptPt = rgPosition[0];
        pVertex[1].Diffuse = FLOAT_ONE;
    
        pVertex[2].ptPt = rgPosition[1];
        pVertex[2].Diffuse = FLOAT_ONE;

        pVertex[3].ptPt = rgPosition[3];
        pVertex[3].Diffuse = FLOAT_ONE;

        pVertex[4].ptPt = rgPosition[2];
        pVertex[4].Diffuse = FLOAT_ONE;

        //
        // Duplicate the last vertex. This creates 2 degenerate triangles: one
        // between vertices 4 and 5 and one connecting this Rect to the
        // next one.
        //

        pVertex[5].ptPt = rgPosition[2];
        pVertex[5].Diffuse = FLOAT_ONE;
    }
        
  Cleanup:
    RRETURN(hr);
}
    
//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::BuildWafflePipeline<TWaffler>
//
//  Synopsis:  Builds a pipeline of wafflers into the provided array of wafflers.
//             And returns a pointer (not to be deleted) to the input sink
//             of the waffle pipeline.
//             the final result is sinked int m_pVB.
//
//-----------------------------------------------------------------------------

template<class TVertex>
template<class TWaffler>
__out_ecount(1) typename TWaffler::ISink *
CHwTVertexBuffer<TVertex>::Builder::BuildWafflePipeline(
        __out_xcount(NUM_OF_VERTEX_TEXTURE_COORDS(TVertex) * 2) TWaffler *wafflers,
        __out_ecount(1) bool &fWafflersUsed
    ) const
{
    UINT count = 0;

    for (int i = 0; i < NUM_OF_VERTEX_TEXTURE_COORDS(TVertex); ++i)
    {
        if (m_map.m_rgWaffleMode[i] != 0)
        {
            const MILMatrix3x2 &pMatWaffle = m_map.m_rgmatPointToUV[i];

            // Each column ([a,b,c] transpose) of this matrix specifies a waffler that
            // partitions the plane into regions between the lines:
            //                    ax + by + c = k
            // for every integer k.
            //
            // If this partition width is substantially less than a pixel we have
            // serious problems with waffling generating too many triangles for
            // doubtful visual effect so we don't perform a waffling with width less
            // than c_rMinWaffleWidthPixels.  So we need to know the width of the partition
            // regions:
            //
            // Changing c just translates the partition so let's assume c = 0.
            // The line ax + by = 0 goes through the origin and the line ax + by
            // = 1 is adjacent to it in the partition.  The distance between
            // these lines is also the distance from ax + by = 1 to the origin.
            // Using Lagrange multipliers we can determine that this distance
            // is
            //                     1/sqrt(a*a+b*b).
            // We want to avoid waffling if this is less than c_rMinWaffleWidthPixels
            // or equivalently:
            //   1/sqrt(a*a+b*b) < c_rMinWaffleWidthPixels
            //     sqrt(a*a+b*b) > 1/c_rMinWaffleWidthPixels
            //          a*a+b*b  > 1/(c_rMinWaffleWidthPixels*c_rMinWaffleWidthPixels)
            //          

            const float c_rMaxWaffleMagnitude = 1/(c_rMinWaffleWidthPixels*c_rMinWaffleWidthPixels);
            
            float mag0 = pMatWaffle.m_00*pMatWaffle.m_00+pMatWaffle.m_10*pMatWaffle.m_10;
            if (mag0 < c_rMaxWaffleMagnitude)
            {
                wafflers[count].Set(pMatWaffle.m_00, pMatWaffle.m_10, pMatWaffle.m_20, wafflers+count+1);
                ++count;
            }

            float mag1 = pMatWaffle.m_01*pMatWaffle.m_01+pMatWaffle.m_11*pMatWaffle.m_11;
            if (mag1 < c_rMaxWaffleMagnitude)
            {
                wafflers[count].Set(pMatWaffle.m_01, pMatWaffle.m_11, pMatWaffle.m_21, wafflers+count+1);
                ++count;
            }
        }
    }

    if (count)
    {
        fWafflersUsed = true;
        // As the last step in the chain we send the triangles to our vertex buffer.
        wafflers[count-1].SetSink(m_pVB);
        return &wafflers[0];
    }
    else
    {
        fWafflersUsed = false;
        // If we built no wafflers then sink straight into the vertex buffer.
        return m_pVB;
    }
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::IsEmpty
//
//  Synopsis:  Does our VB have any triangles/lines?
//
//-----------------------------------------------------------------------------
template <class TVertex>
BOOL
CHwTVertexBuffer<TVertex>::Builder::IsEmpty()
{
    return m_pVB->IsEmpty();
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::AddTrapezoid
//
//  Synopsis:  Add a trapezoid to the vertex buffer
//
//
//      left edge       right edge
//      ___+_________________+___      <<< top edge
//     /  +  /             \  +  \
//    /  +  /               \  +  \
//   /  +  /                 \  +  \
//  /__+__/___________________\__+__\  <<< bottom edge
//    + ^^                        +
//      delta
//

template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::AddTrapezoid(
    float rPixelYTop,              // In: y coordinate of top of trapezoid
    float rPixelXTopLeft,          // In: x coordinate for top left
    float rPixelXTopRight,         // In: x coordinate for top right
    float rPixelYBottom,           // In: y coordinate of bottom of trapezoid
    float rPixelXBottomLeft,       // In: x coordinate for bottom left
    float rPixelXBottomRight,      // In: x coordinate for bottom right
    float rPixelXLeftDelta,        // In: trapezoid expand radius for left edge
    float rPixelXRightDelta        // In: trapezoid expand radius for right edge
    )
{
    HRESULT hr = S_OK;

    if (AreWaffling())
    {
        IFC(AddTrapezoidWaffle(
                rPixelYTop,
                rPixelXTopLeft,
                rPixelXTopRight,
                rPixelYBottom,
                rPixelXBottomLeft,
                rPixelXBottomRight,
                rPixelXLeftDelta,
                rPixelXRightDelta));
    }
    else
    {
        IFC(AddTrapezoidStandard(
                rPixelYTop,
                rPixelXTopLeft,
                rPixelXTopRight,
                rPixelYBottom,
                rPixelXBottomLeft,
                rPixelXBottomRight,
                rPixelXLeftDelta,
                rPixelXRightDelta));
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::AddTrapezoidStandard
//
//  Synopsis:  See AddTrapezoid.  This doesn't do waffling & uses tri strips.
//

template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::AddTrapezoidStandard(
    float rPixelYTop,              // In: y coordinate of top of trapezoid
    float rPixelXTopLeft,          // In: x coordinate for top left
    float rPixelXTopRight,         // In: x coordinate for top right
    float rPixelYBottom,           // In: y coordinate of bottom of trapezoid
    float rPixelXBottomLeft,       // In: x coordinate for bottom left
    float rPixelXBottomRight,      // In: x coordinate for bottom right
    float rPixelXLeftDelta,        // In: trapezoid expand radius for left edge
    float rPixelXRightDelta        // In: trapezoid expand radius for right edge
    )
{
    HRESULT hr = S_OK;
    TVertex *pVertex;

    IFC(PrepareStratum(
        rPixelYTop,
        rPixelYBottom,
        true, /* Trapezoid */
        min(rPixelXTopLeft, rPixelXBottomLeft),
        max(rPixelXTopRight, rPixelXBottomRight)
        ));
    
    //
    // Add the vertices
    //

    UINT cVertices = 8;
    bool fNeedOutsideGeometry = NeedOutsideGeometry();
    bool fNeedInsideGeometry = NeedInsideGeometry();

    if (!fNeedOutsideGeometry)
    {
        // For duplicates at beginning and end required to skip outside
        // geometry.
        cVertices += 2;
    }
       
    if (!fNeedInsideGeometry)
    {
        // For duplicates in middle required to skip inside geometry.
        cVertices += 2;
    }

    IFC(m_pVB->AddTriStripVertices(cVertices, &pVertex));

    if (!fNeedOutsideGeometry)
    {
        //
        // Duplicate the first vertex. This creates 2 degenerate triangles: one connecting
        // the previous trapezoid to this one and another between vertices 0 and 1.
        //

        pVertex->ptPt.X = rPixelXTopLeft - rPixelXLeftDelta;
        pVertex->ptPt.Y = rPixelYTop;
        pVertex->Diffuse = FLOAT_ZERO;
        ++pVertex;
    }

    //
    // Fill in the strip vertices
    //

    pVertex->ptPt.X = rPixelXTopLeft - rPixelXLeftDelta;
    pVertex->ptPt.Y = rPixelYTop;
    pVertex->Diffuse = FLOAT_ZERO;
    ++pVertex;

    pVertex->ptPt.X = rPixelXBottomLeft - rPixelXLeftDelta;
    pVertex->ptPt.Y = rPixelYBottom;
    pVertex->Diffuse = FLOAT_ZERO;
    ++pVertex;

    pVertex->ptPt.X = rPixelXTopLeft + rPixelXLeftDelta;
    pVertex->ptPt.Y = rPixelYTop;
    pVertex->Diffuse = FLOAT_ONE;
    ++pVertex;

    pVertex->ptPt.X = rPixelXBottomLeft + rPixelXLeftDelta;
    pVertex->ptPt.Y = rPixelYBottom;
    pVertex->Diffuse = FLOAT_ONE;
    ++pVertex;

    if (!fNeedInsideGeometry)
    {
        // Don't create inside geometry.
        pVertex->ptPt.X = rPixelXBottomLeft + rPixelXLeftDelta;
        pVertex->ptPt.Y = rPixelYBottom;
        pVertex->Diffuse = FLOAT_ONE;
        ++pVertex;
        
        pVertex->ptPt.X = rPixelXTopRight - rPixelXRightDelta;
        pVertex->ptPt.Y = rPixelYTop;
        pVertex->Diffuse = FLOAT_ONE;
        ++pVertex;
    }

    pVertex->ptPt.X = rPixelXTopRight - rPixelXRightDelta;
    pVertex->ptPt.Y = rPixelYTop;
    pVertex->Diffuse = FLOAT_ONE;
    ++pVertex;

    pVertex->ptPt.X = rPixelXBottomRight - rPixelXRightDelta;
    pVertex->ptPt.Y = rPixelYBottom;
    pVertex->Diffuse = FLOAT_ONE;
    ++pVertex;

    pVertex->ptPt.X = rPixelXTopRight + rPixelXRightDelta;
    pVertex->ptPt.Y = rPixelYTop;
    pVertex->Diffuse = FLOAT_ZERO;
    ++pVertex;

    pVertex->ptPt.X = rPixelXBottomRight + rPixelXRightDelta;
    pVertex->ptPt.Y = rPixelYBottom;
    pVertex->Diffuse = FLOAT_ZERO;
    ++pVertex;

    if (!fNeedOutsideGeometry)
    {
        //
        // Duplicate the last vertex. This creates 2 degenerate triangles: one
        // between vertices 8 and 9 and one connecting this trapezoid to the
        // next one.
        //

        pVertex->ptPt.X = rPixelXBottomRight + rPixelXRightDelta;
        pVertex->ptPt.Y = rPixelYBottom;
        pVertex->Diffuse = FLOAT_ZERO;
        ++pVertex;
    }

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::AddTrapezoidWaffle
//
//  Synopsis:  See AddTrapezoid.  This adds a waffled trapezoid.
//
//-----------------------------------------------------------------------------
template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::AddTrapezoidWaffle(
    float rPixelYTop,              // In: y coordinate of top of trapezoid
    float rPixelXTopLeft,          // In: x coordinate for top left
    float rPixelXTopRight,         // In: x coordinate for top right
    float rPixelYBottom,           // In: y coordinate of bottom of trapezoid
    float rPixelXBottomLeft,       // In: x coordinate for bottom left
    float rPixelXBottomRight,      // In: x coordinate for bottom right
    float rPixelXLeftDelta,        // In: trapezoid expand radius for left edge
    float rPixelXRightDelta        // In: trapezoid expand radius for right edge
    )
{
    HRESULT hr = S_OK;

    // We have 2 (u & v) wafflers per texture coordinate that need waffling.
    TriangleWaffler<PointXYA> wafflers[NUM_OF_VERTEX_TEXTURE_COORDS(TVertex) * 2];
    bool fWafflersUsed = false;

    TriangleWaffler<PointXYA>::ISink *pWaffleSinkNoRef = BuildWafflePipeline(wafflers, OUT fWafflersUsed);

    PointXYA vertices[8];

    //
    // Fill in the strip vertices
    //

    // Nonstandard coverage mapping and waffling are not supported at the same time.
    Assert(!NeedOutsideGeometry());

    vertices[0].x = rPixelXTopLeft - rPixelXLeftDelta;
    vertices[0].y = rPixelYTop;
    vertices[0].a = 0;

    vertices[1].x = rPixelXBottomLeft - rPixelXLeftDelta;
    vertices[1].y = rPixelYBottom;
    vertices[1].a = 0;

    vertices[2].x = rPixelXTopLeft + rPixelXLeftDelta;
    vertices[2].y = rPixelYTop;
    vertices[2].a = 1;

    vertices[3].x = rPixelXBottomLeft + rPixelXLeftDelta;
    vertices[3].y = rPixelYBottom;
    vertices[3].a = 1;

    vertices[4].x = rPixelXTopRight - rPixelXRightDelta;
    vertices[4].y = rPixelYTop;
    vertices[4].a = 1;

    vertices[5].x = rPixelXBottomRight - rPixelXRightDelta;
    vertices[5].y = rPixelYBottom;
    vertices[5].a = 1;

    vertices[6].x = rPixelXTopRight + rPixelXRightDelta;
    vertices[6].y = rPixelYTop;
    vertices[6].a = 0;

    vertices[7].x = rPixelXBottomRight + rPixelXRightDelta;
    vertices[7].y = rPixelYBottom;
    vertices[7].a = 0;

    // Send the triangles in the strip through the waffle pipeline.
    for (int i = 0; i < 6; ++i)
    {
        IFC(pWaffleSinkNoRef->AddTriangle(vertices[i+1], vertices[i], vertices[i+2]));
    }

Cleanup:
    RRETURN(hr);
}
    
//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::PrepareStratumSlow
//
//  Synopsis:  Call before producing a new stratum (complex span or trapezoid.)
//             Handles several tasks:
//               1. Producing between top of complement geometry & the 1st
//                  stratum or when a gap between strata occurs (because
//                  the geometry is not closed and has horizontal gaps.)
//                  Passing in FLT_MAX for rStratumTop and rStratumBottom
//                  Fills the gap between the last stratum and the bottom
//                  of the outside.
//               2. Begins and/or ends the triangle strip corresponding to
//                  a trapezoid row.
//               3. Updates status vars m_rCurStratumTop & m_rCurStratumBottom
//
//  Note:      Call PrepareStratum which inlines the check for NeedOutsideGeometry()
//             If NeedOutsideGeometry is false PrepareStratum() does nothing.
//             This (slow) version asserts NeedOutsideGeometry()
//
//-----------------------------------------------------------------------------
template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::PrepareStratumSlow(
    float rStratumTop,
    float rStratumBottom,
    bool fTrapezoid,
    float rTrapezoidLeft,
    float rTrapezoidRight
    )
{
    HRESULT hr = S_OK;
    
    Assert(!(rStratumTop > rStratumBottom));
    Assert(NeedOutsideGeometry());

    // There's only once case where a stratum can go "backwards"
    // and that's when we're done building & calling from
    // EndBuildingOutside
        
    float fEndBuildingOutside = rStratumBottom == OutsideBottom() &&
                                rStratumTop == OutsideBottom();

    if (fEndBuildingOutside)
    {
        Assert(!fTrapezoid);
    }
    else
    {
        Assert(!(rStratumBottom < m_rCurStratumBottom));
    }
    
    if (   fEndBuildingOutside
        || rStratumBottom != m_rCurStratumBottom)
    {
        
        // New stratum starting now.  Two things to do
        //  1. Close out current trapezoid stratum if necessary.
        //  2. Begin new trapezoid stratum if necessary.
        
        if (m_rCurStratumTop != FLT_MAX)
        {
            // End current trapezoid stratum.

            TVertex *pVertex;
            IFC(m_pVB->AddTriStripVertices(3, &pVertex));

            // we do not clip trapezoids so RIGHT boundary
            // of the stratus can be outside of m_rcOutsideBounds.
            
            float rOutsideRight = max(OutsideRight(), m_rLastTrapezoidRight);

            pVertex->ptPt.X = rOutsideRight;
            pVertex->ptPt.Y = m_rCurStratumTop;
            pVertex->Diffuse = FLOAT_ZERO;
            ++pVertex;

            pVertex->ptPt.X = rOutsideRight;
            pVertex->ptPt.Y = m_rCurStratumBottom;
            pVertex->Diffuse = FLOAT_ZERO;
            ++pVertex;

            // Duplicate last vertex in row
            pVertex->ptPt.X = rOutsideRight;
            pVertex->ptPt.Y = m_rCurStratumBottom;
            pVertex->Diffuse = FLOAT_ZERO;
        }
        // Compute the gap between where the last stratum ended and where
        // this one begins.
        float flGap = rStratumTop - m_rCurStratumBottom;

        if (flGap > 0)
        {
            // The "special" case of a gap at the beginning is caught here
            // using the sentinel initial value of m_rCurStratumBottom.

            float flRectTop = m_rCurStratumBottom == -FLT_MAX
                              ? OutsideTop()
                              : m_rCurStratumBottom;
            float flRectBot = static_cast<float>(rStratumTop);

            // Produce rectangular for any horizontal intervals in the
            // outside bounds that have no generated geometry.
            Assert(m_rCurStratumBottom != -FLT_MAX || m_rCurStratumTop == FLT_MAX);

            TVertex *pVertex;
            IFC(m_pVB->AddTriStripVertices(6, &pVertex));
            
            // Duplicate first vertex.
            pVertex->ptPt.X = OutsideLeft();
            pVertex->ptPt.Y = flRectTop;
            pVertex->Diffuse = FLOAT_ZERO;
            ++pVertex;

            pVertex->ptPt.X = OutsideLeft();
            pVertex->ptPt.Y = flRectTop;
            pVertex->Diffuse = FLOAT_ZERO;
            ++pVertex;

            pVertex->ptPt.X = OutsideLeft();
            pVertex->ptPt.Y = flRectBot;
            pVertex->Diffuse = FLOAT_ZERO;
            ++pVertex;

            pVertex->ptPt.X = OutsideRight();
            pVertex->ptPt.Y = flRectTop;
            pVertex->Diffuse = FLOAT_ZERO;
            ++pVertex;

            pVertex->ptPt.X = OutsideRight();
            pVertex->ptPt.Y = flRectBot;
            pVertex->Diffuse = FLOAT_ZERO;
            ++pVertex;
        
            pVertex->ptPt.X = OutsideRight();
            pVertex->ptPt.Y = flRectBot;
            pVertex->Diffuse = FLOAT_ZERO;
            ++pVertex;
        }

        if (fTrapezoid)
        {
            // Begin new trapezoid stratum.
            
            TVertex *pVertex;
            IFC(m_pVB->AddTriStripVertices(3, &pVertex));

            // we do not clip trapezoids so left boundary
            // of the stratus can be outside of m_rcOutsideBounds.
            
            float rOutsideLeft = min(OutsideLeft(), rTrapezoidLeft);

            // Duplicate first vertex.
            pVertex->ptPt.X = rOutsideLeft;
            pVertex->ptPt.Y = rStratumTop;
            pVertex->Diffuse = FLOAT_ZERO;
            ++pVertex;
            
            pVertex->ptPt.X = rOutsideLeft;
            pVertex->ptPt.Y = rStratumTop;
            pVertex->Diffuse = FLOAT_ZERO;
            ++pVertex;

            pVertex->ptPt.X = rOutsideLeft;
            pVertex->ptPt.Y = rStratumBottom;
            pVertex->Diffuse = FLOAT_ZERO;
        }
    }
    
    if (fTrapezoid)
    {
        m_rLastTrapezoidRight = rTrapezoidRight;
    }

    m_rCurStratumTop = fTrapezoid ? rStratumTop : FLT_MAX;
    m_rCurStratumBottom = rStratumBottom;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::EndBuildingOutside
//
//  Synopsis:  Finish creating outside geometry.
//             1. If no geometry was created then just fill bounds.
//             2. Otherwise:
//                 A. End last trapezoid row
//                 B. Produce stop stratum
// 
//-----------------------------------------------------------------------------
template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::EndBuildingOutside()
{
    return PrepareStratum(
        OutsideBottom(),
        OutsideBottom(),
        false /* Not a trapezoid. */
        );
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::EndBuilding
//
//  Synopsis:  Expand all vertices to the full required format and return
//             vertex buffer.
//
//-----------------------------------------------------------------------------
template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::EndBuilding(
    __deref_opt_out_ecount(1) CHwVertexBuffer **ppVertexBuffer
    )
{
    HRESULT hr = S_OK;

    IFC(EndBuildingOutside());
    
    ExpandVertices();

    if (ppVertexBuffer)
    {
        *ppVertexBuffer = m_pVB;
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::ExpandVertices
//
//  Synopsis:  Expand all vertices to the full required format now that all
//             vertices have been received
//

template <class TVertex>
void
CHwTVertexBuffer<TVertex>::Builder::ExpandVertices()
{
    if (m_cPrecomputedTriListVertices)
    {
        Assert(m_pVB->GetNumTriListVertices() == 0);
        Assert(m_pVB->GetNumTriStripVertices() == 0);
        Assert(m_pVB->GetNumLineListVertices() == 0);

        Assert(m_cPrecomputedTriListIndices > 0);

#if DBG
        Assert(m_mvfIn == m_mvfDbgOut);
#endif

        Assert(!AreWaffling());
    }
    else
    {
        // Indexed triangle lists are not used with waffling.
        Assert(   !AreWaffling()
               || m_pVB->m_rgIndices.GetCount() == 0);
        
        if (   m_pVB->GetNumTriListVertices() > 0
            && !AreWaffling())
        {
            TVertex *pVertices;
            UINT uNumVertices;

            m_pVB->GetTriListVertices(&pVertices, &uNumVertices);

            (this->*m_pfnExpandVertices)(
                uNumVertices,
                pVertices
                );
        }
    
        if (m_pVB->GetNumNonIndexedTriListVertices() > 0)
        {
            TVertex *pVertices;
            UINT uNumVertices;

            m_pVB->GetNonIndexedTriListVertices(&pVertices, &uNumVertices);

            (this->*m_pfnExpandVertices)(
                uNumVertices,
                pVertices
                );
    
            if (AreWaffling())
            {
                // Assert that there are an intergral quantitiy of groups of size 3.
                Assert(m_pVB->GetNumNonIndexedTriListVertices() % 3 == 0);

                ViewportToPackedCoordinates(uNumVertices / 3, pVertices, 3);
            }
        }
    
        if (m_pVB->GetNumTriStripVertices() > 0)
        {
            TVertex *pVertices;
            UINT uNumVertices;

            m_pVB->GetTriStripVertices(&pVertices, &uNumVertices);

            (this->*m_pfnExpandVertices)(
                uNumVertices,
                pVertices
                );
    
            if (AreWaffling())
            {
                // When we are waffling we only use tri strips for AddLineAsTriangleStrip so we know that
                // there are 6 vertices in each triangle strip.
                #if DBG
                Assert(!m_pVB->m_fDbgNonLineSegmentTriangleStrip);
                #endif

                // Assert that there are an intergral quantitiy of groups of size 6.
                Assert(uNumVertices % 6 == 0);

                ViewportToPackedCoordinates(uNumVertices / 6, pVertices, 6);
            }
        }
    
        if (m_pVB->GetNumLineListVertices() > 0)
        {
            TVertex *pVertices;
            UINT uNumVertices;

            m_pVB->GetLineListVertices(&pVertices, &uNumVertices);

            (this->*m_pfnExpandVertices)(
                uNumVertices,
                pVertices
                );
    
            if (AreWaffling())
            {
                // Assert that there are an intergral quantitiy of groups of size 2.
                Assert(uNumVertices % 2 == 0);

                ViewportToPackedCoordinates(uNumVertices / 2, pVertices, 2);
            }
        }
    }
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwTVertexBuffer<TVertex>::Builder::FlushInternal
//
//  Synopsis:  Send any pending state and geometry to the device.
//             If the optional argument is NULL then reset the
//             vertex buffer.
//             If the optional argument is non-NULL AND we have
//             not yet flushed the vertex buffer return the vertex
//             buffer.
//
//             These semantics allow the VB to be re-used for multipass
//             rendering if a single buffer sufficed for all of the geometry.
//             Otherwise multipass has to use a slower algorithm.
//
//-----------------------------------------------------------------------------
template <class TVertex>
HRESULT
CHwTVertexBuffer<TVertex>::Builder::FlushInternal(
    __deref_opt_out_ecount_opt(1) CHwVertexBuffer **ppVertexBuffer
    )
{
    HRESULT hr = S_OK;

    if (m_pPipelineNoRef)
    {
        // We use the pointer to the pipeline to ask it to send
        // the state if it hasn't been sent already.  Therefore after sending
        // we null it.
        IFC(m_pPipelineNoRef->RealizeColorSourcesAndSendState(m_pVB));
        m_pPipelineNoRef = NULL;
    }

    IFC(EndBuilding(NULL));

    if (m_rgoPrecomputedTriListVertices)
    {
        IFC(RenderPrecomputedIndexedTriangles(
            m_cPrecomputedTriListVertices,
            m_rgoPrecomputedTriListVertices,
            m_cPrecomputedTriListIndices,
            m_rguPrecomputedTriListIndices
            ));
    }
    else
    {
        IFC(m_pVB->DrawPrimitive(m_pDeviceNoRef));
    }

  Cleanup:
    if (ppVertexBuffer)
    {
        if (!m_fHasFlushed)
        {
            *ppVertexBuffer = m_pVB;
        }
    }
    else
    {
        m_fHasFlushed = true;
        m_pVB->Reset(this);

        m_rgoPrecomputedTriListVertices = NULL;
        m_cPrecomputedTriListVertices = 0;

        m_rguPrecomputedTriListIndices = NULL;
        m_cPrecomputedTriListIndices = 0;
    }
    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwTVertexBuffer<TVertex>::Builder::TransferAndOrExpandVerticesInline
//
//  Synopsis:  Expand vertices from the basic pre-generated data to the full
//             required format.  Input and output buffers may be the same or
//             different.
//
//             This method is forced inline as a template to generate optimized
//             and general conversion routines.  It should never be called
//             directly, but rather through a wrapper method like
//             ExpandVerticesGeneral and ExpandVerticesFast.
//

const DWORD FLOAT_ZERO = 0x00000000;
const DWORD FLOAT_ONE  = 0x3f800000;

template <class TVertex>
MIL_FORCEINLINE
void
CHwTVertexBuffer<TVertex>::Builder::TransferAndOrExpandVerticesInline(
    __range(1,UINT_MAX) UINT uCount,
    __in_ecount(uCount) TVertex const * pInputVertex_,
    __out_ecount(uCount) TVertex * pOutputVertex,
    MilVertexFormat mvfGenerated,
    MilVertexFormatAttribute mvfaScaleByFalloff,
    bool fInputOutputAreSameBuffer,
    bool fTransformPosition
    )
{
    Assert(mvfGenerated == m_mvfGenerated);
    Assert(fTransformPosition || m_map.m_matPos2DTransform.IsIdentity());

    UINT uDiffuse;
    UINT uDiffuse00AA00GG = 0;
    UINT uDiffuse00RR00BB = 0;
    UINT uBlendedDiffuseCache = 0;
    UINT uBlendedDiffuseCacheFalloff = 0;

    // Create a reference to the pointer that will be used for input vertices.
    // A reference is used make the compiler only track a single pointer
    // through this routine in the pIn == pOut case as an optimization.
    TVertex const * &pInputVertex = *(fInputOutputAreSameBuffer ? &pOutputVertex : &pInputVertex_);

    //
    // Set the diffuse color and components we need for fast blending
    //

    if (m_map.m_mvfMapped & MILVFAttrDiffuse)
    {
        uDiffuse = m_map.m_vStatic.Diffuse;

        //
        // If we are going to need to compute falloffs often, then
        // compute these useful components of the diffuse color
        //

        if (mvfaScaleByFalloff & MILVFAttrDiffuse)
        {
            GpCC color;

            color.argb = uDiffuse;

            uDiffuse00AA00GG = (color.a << 16) | color.g;
            uDiffuse00RR00BB = (color.r << 16) | color.b;
        }
    }
    else
    {
        //
        // In this case, there is no source diffuse, so we setup
        // the card to blend with white
        //

        uDiffuse = 0xffffffff;
        if (mvfaScaleByFalloff & MILVFAttrDiffuse)
        {
            uDiffuse00AA00GG = 0x00ff00ff;
            uDiffuse00RR00BB = 0x00ff00ff;
        }
    }

    //
    // Expand vertices
    //

#if DBG
    BOOL fDbgIsPixelZoomMode = DbgIsPixelZoomMode();
#endif

    for (;;)
    {
        //
        // Assign the position.
        //

        //
        // NOTICE-2005/12/15-chrisra The pos transform must be applied first.
        //
        // We generate the texture coordinates based on the 2d position, which
        // requires this transform to be applied to the point first.
        //
        if (fTransformPosition)
        {
            m_map.m_matPos2DTransform.TransformPoint(
                pOutputVertex->ptPt,
                pInputVertex->ptPt
                );
        }
        else if (!fInputOutputAreSameBuffer)
        {
            pOutputVertex->ptPt = pInputVertex->ptPt;
        }

#if DBG
        if (fDbgIsPixelZoomMode)
        {
            extern POINT g_dbgMousePosition;

            pOutputVertex->ptPt.X = (pOutputVertex->ptPt.X - float(g_dbgMousePosition.x)) * c_dbgPixelZoomModeScale;
            pOutputVertex->ptPt.Y = (pOutputVertex->ptPt.Y - float(g_dbgMousePosition.y)) * c_dbgPixelZoomModeScale;

            if (IsTagEnabled(tagWireframe))
            {
                // Force diffuse to one so that we can see wireframe edges
                pOutputVertex->Diffuse = FLOAT_ONE;
            }
        }
#endif

        const MilPoint2F *pptPoint = &pOutputVertex->ptPt;
        Assert((ULONG_PTR)pptPoint == (ULONG_PTR)pOutputVertex);


        if (mvfGenerated & MILVFAttrZ)
        {
            pOutputVertex->Z = m_map.m_vStatic.Z;
        }
        else if (!fInputOutputAreSameBuffer)
        {
            pOutputVertex->Z = pInputVertex->Z;
        }

        __if_exists (TVertex::Normal)
        {
            if (mvfGenerated & MILVFAttrNormal)
            {
                pOutputVertex->Normal = m_map.m_vStatic.Normal;
            }
            else if (!fInputOutputAreSameBuffer)
            {
                pOutputVertex->Normal = pInputVertex->Normal;
            }
        }

        if (mvfGenerated & MILVFAttrDiffuse)
        {
            if (mvfaScaleByFalloff & MILVFAttrDiffuse)
            {
                DWORD dwFalloff = pInputVertex->Diffuse;
                float rFalloff = *reinterpret_cast<float *>(&dwFalloff);

                Assert(rFalloff >= 0.0f);
                Assert(rFalloff <= 1.0f);

                // Check for simple, completely transparent case to
                // avoid expensive floating point operations
                if (dwFalloff == FLOAT_ZERO)
                {
                    pOutputVertex->Diffuse = MIL_COLOR(0,0,0,0);
                }
                else if (dwFalloff == FLOAT_ONE)
                {
                    pOutputVertex->Diffuse = uDiffuse;
                }
                else if (dwFalloff == uBlendedDiffuseCacheFalloff)
                {
                    // We often get consequetive pixels with the same coverage, so we
                    // fast path this case.  Note that the most common occurence of this reuse
                    // is during a Trapezoidal AA complex scan.

                    pOutputVertex->Diffuse = uBlendedDiffuseCache;
                }
                else
                {
                    //
                    //  modify the pipeline to pass integer coverage
                    //
                    // We can pass coverage as an integer between 0 and 256 so that we can avoid all the
                    // conversions.
                    //

                    UINT uCoverage = CFloatFPU::SmallRound(rFalloff*256.0f);

                    //
                    // Blending computation will overflow for uiCoverage > 255, so we have
                    // to handle this case explicitly
                    //

                    if (uCoverage > 255)
                    {
                        pOutputVertex->Diffuse = uDiffuse;
                    }
                    else
                    {
                        //
                        // Multiply the falloff by the diffuse color
                        //

                        UINT uBlendedDiffuseAA00GG00 = ((uDiffuse00AA00GG * uCoverage + 0x00800080) & 0xff00ff00);
                        UINT uBlendedDiffuse00RR00BB = ((uDiffuse00RR00BB * uCoverage + 0x00800080) >> 8) & 0x00ff00ff;
                        uBlendedDiffuseCache = uBlendedDiffuseAA00GG00 | uBlendedDiffuse00RR00BB;
                        uBlendedDiffuseCacheFalloff = dwFalloff;

                        pOutputVertex->Diffuse = uBlendedDiffuseCache;
                    }
                }
            }
            else
            {
                // No falloff (no PPAA)
                pOutputVertex->Diffuse = uDiffuse;
            }
        }
        else if (!fInputOutputAreSameBuffer)
        {
            pOutputVertex->Diffuse = pInputVertex->Diffuse;
        }

        __if_exists (TVertex::Specular)
        {
            if (mvfGenerated & MILVFAttrSpecular)
            {
                pOutputVertex->Specular = m_map.m_vStatic.Specular;
            }
            else if (!fInputOutputAreSameBuffer)
            {
                pOutputVertex->Specular = pInputVertex->Specular;
            }
        }

        //
        // NOTICE-2005/12/15-chrisra UV Transform applied after pos transform
        //
        // The texture coordinate transforms are applied to the 2d position in
        // device space, which means we have to apply these to the 2d point
        // after all other 2d transforms have been applied (Exception is the
        // final projection transform).
        //
        __if_exists (TVertex::UV0)
        {
            if (mvfGenerated & MILVFAttrUV1)
            {
                m_map.PointToUV(*pptPoint, 0, pOutputVertex);
            }
            else if (!fInputOutputAreSameBuffer)
            {
                pOutputVertex->UV0(pInputVertex->UV0());
            }
        }

        __if_exists (TVertex::UV1)
        {
            if (mvfGenerated & (MILVFAttrUV2 & ~MILVFAttrUV1))
            {
                m_map.PointToUV(*pptPoint, 1, pOutputVertex);
            }
            else if (!fInputOutputAreSameBuffer)
            {
                pOutputVertex->UV1(pInputVertex->UV1());
            }
        }

        __if_exists (TVertex::UV2)
        {
            if (mvfGenerated & (MILVFAttrUV3 & ~MILVFAttrUV2))
            {
                m_map.PointToUV(*pptPoint, 2, pOutputVertex);
            }
            else if (!fInputOutputAreSameBuffer)
            {
                pOutputVertex->UV2(pInputVertex->UV2());
            }
        }

        __if_exists (TVertex::UV3)
        {
            if (mvfGenerated & (MILVFAttrUV4 & ~MILVFAttrUV3))
            {
                m_map.PointToUV(*pptPoint, 3, pOutputVertex);
            }
            else if (!fInputOutputAreSameBuffer)
            {
                pOutputVertex->UV3(pInputVertex->UV3());
            }
        }

        __if_exists (TVertex::UV4)
        {
            if (mvfGenerated & (MILVFAttrUV5 & ~MILVFAttrUV4))
            {
                m_map.PointToUV(*pptPoint, 4, pOutputVertex);
            }
            else if (!fInputOutputAreSameBuffer)
            {
                pOutputVertex->UV4(pInputVertex->UV4());
            }
        }

        __if_exists (TVertex::UV5)
        {
            if (mvfGenerated & (MILVFAttrUV6 & ~MILVFAttrUV5))
            {
                m_map.PointToUV(*pptPoint, 5, pOutputVertex);
            }
            else if (!fInputOutputAreSameBuffer)
            {
                pOutputVertex->UV5(pInputVertex->UV5());
            }
        }

        __if_exists (TVertex::UV6)
        {
            if (mvfGenerated & (MILVFAttrUV7 & ~MILVFAttrUV6))
            {
                m_map.PointToUV(*pptPoint, 6, pOutputVertex);
            }
            else if (!fInputOutputAreSameBuffer)
            {
                pOutputVertex->UV6(pInputVertex->UV6());
            }
        }

        __if_exists (TVertex::UV7)
        {
            if (mvfGenerated & (MILVFAttrUV8 & ~MILVFAttrUV7))
            {
                m_map.PointToUV(*pptPoint, 7, pOutputVertex);
            }
            else if (!fInputOutputAreSameBuffer)
            {
                pOutputVertex->UV7(pInputVertex->UV7());
            }
        }

        //
        // Check for more vertices
        //

        if (--uCount <= 0) break;

        //
        // Advance
        //

        pInputVertex++;

        // When buffers are the same pOutputVertex just refers to pInputVertex,
        // so advancing both would be a mistake for that case.
        if (!fInputOutputAreSameBuffer)
        {
            pOutputVertex++;
        }
    }
}


// 4505: unreferenced local function has been removed
//   These will show up as errors in very bizarre way including references to
//   shared\dynarray.h and the particular methods in
//   CHwTVertexBuffer<TVertex>::Builder, but what you won't find is a reference
//   to this line of explicit instantiation.  Proper placing of this was done
//   by trial and error :)
#pragma warning(disable : 4505)

// Explicit template instantiation
template class CHwTVertexBuffer<CD3DVertexXYZDUV2>;
template class CHwTVertexBuffer<CD3DVertexXYZDUV8>;





