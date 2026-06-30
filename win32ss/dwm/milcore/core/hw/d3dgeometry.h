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
//      D3D Geometry classes
//
//      Holds Vertex Buffer Classes
//
//  $ENDTAG
//
//  Classes:
//      CD3DVertexBuffer
//      CD3DVertexBufferDUV2
//      CD3DVertexBufferDUV6
//
//------------------------------------------------------------------------------

// We use 16-bit indices, so we can't have more than this many vertices
// We use 0xffff as a special value in the tessellator, so set max vertices
// to 0xfffe.
#define MAXRENDERVERTICES 0xfffe

MtExtern(D3DVertexBufferVertices);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DVertexBuffer
//
//  Synopsis:
//      Dynamic Vertex Buffer designed to take in vertices from a tessellator
//      and automatically assign the vertices with Diffuse color and texture
//      coordinates.
//
//------------------------------------------------------------------------------
class CD3DVertexBuffer
{
public:
    CD3DVertexBuffer(__out_range(==, this->m_uVertexStride) UINT uVertexStride);
    virtual ~CD3DVertexBuffer();

    __range(==, this->m_uVertexStride) UINT GetVertexStride() const { return m_uVertexStride; }

    __bcount(this->m_uVertexStride * this->m_uNumVertices) void *GetVertices() { return m_pVertices; }
    __range(==, this->m_uNumVertices) DWORD GetNumVertices() const { return m_uNumVertices; }

    void Clear() { m_uNumVertices = 0; }

protected:
    __field_bcount_part(m_uVertexStride * m_uCapVertices, m_uVertexStride * m_uNumVertices)
    void *m_pVertices;

    __field_range(<=, m_uCapVertices)
    UINT m_uNumVertices;     // amount of vertices already allocated

    UINT const m_uVertexStride;

    __field_range(<=, UINT_MAX/m_uVertexStride)
    UINT m_uCapVertices;     // vertex buffer capacity

protected:    
    HRESULT GrowVertexBufferSize(__in_range(==,4) UINT uGrow);

    //
    // Before this function is called, the memory should already have been allocated
    // with GrowVertexBufferSize.
    //
    MIL_FORCEINLINE void ReserveMemoryForVertices(
        __deref_bcount(this->m_uVertexStride * uNum) void **ppVertex,
        __in_range(<=, this->m_uCapVertices-this->m_uNumVertices)
        __out_range(==, this->m_uNumVertices-pre(this->m_uNumVertices))
        UINT uNum
        )
    {
        *ppVertex = &(static_cast<BYTE *>(m_pVertices))[m_uVertexStride*m_uNumVertices];

        m_uNumVertices += uNum;
    }


    HRESULT GetMultipleVertices(
        __deref_bcount(this->m_uVertexStride * uNumNewVertices) void **ppStartVertex,
        __in_range(4,4) UINT uNumNewVertices
        );
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DVertexBufferDUV2
//
//  Synopsis:
//      Enherits from CD3DVertexBuffer with a specific vertex format. Designed
//      for using diffuse color and 2 texture stages.
//
//------------------------------------------------------------------------------
class CD3DVertexBufferDUV2 : public CD3DVertexBuffer
{
public:
    CD3DVertexBufferDUV2() : CD3DVertexBuffer(sizeof(CD3DVertexXYZDUV2)) { }

    // additional functions

    MIL_FORCEINLINE HRESULT GetNewVertices(
        __in_range(==,4) UINT uNumNewVertices,
        __deref_ecount(uNumNewVertices) CD3DVertexXYZDUV2 **ppVertices)
    {
        Assert(m_uVertexStride == sizeof(**ppVertices));
        return GetMultipleVertices((void **)ppVertices, uNumNewVertices);
    }

protected:
    MILMatrix3x2 m_matTransforms[2];
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DVertexBufferDUV6
//
//  Synopsis:
//      Enherits from CD3DVertexBuffer with a specific vertex format. Designed
//      for using diffuse color and 6 texture stages.
//
//------------------------------------------------------------------------------
class CD3DVertexBufferDUV6 : public CD3DVertexBuffer
{
public:
    CD3DVertexBufferDUV6() : CD3DVertexBuffer(sizeof(CD3DVertexXYZDUV6)) { }

    // additional functions

    MIL_FORCEINLINE HRESULT GetNewVertices(
        __in_range(==,4) UINT uNumNewVertices,
        __deref_ecount(uNumNewVertices) CD3DVertexXYZDUV6 **ppVertices)
    {
        Assert(m_uVertexStride == sizeof(**ppVertices));
        return GetMultipleVertices((void **)ppVertices, uNumNewVertices);
    }
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CD3DVertexBufferXYZNDSUV4
//
//  Synopsis:
//      Enherits from CD3DVertexBufferXYZNDSUV4 with a specific vertex format.
//      Designed for using 3D data with normals, diffuse color, specular color,
//      and 4 texture stages.
//
//------------------------------------------------------------------------------
class CD3DVertexBufferXYZNDSUV4 : public CD3DVertexBuffer
{
public:
    CD3DVertexBufferXYZNDSUV4() : CD3DVertexBuffer(sizeof(CD3DVertexXYZNDSUV4)) { }

    // additional functions

    MIL_FORCEINLINE HRESULT GetNewVertices(
        __in_range(==,4) UINT uNumNewVertices,
        __deref_ecount(uNumNewVertices) OUT CD3DVertexXYZNDSUV4 **ppVertices)
    {
        Assert(m_uVertexStride == sizeof(**ppVertices));
        return GetMultipleVertices((void **)ppVertices, uNumNewVertices);
    }
};



