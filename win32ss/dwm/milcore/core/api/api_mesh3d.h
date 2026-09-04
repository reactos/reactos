// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


/*=========================================================================*\



    File: api_mesh3d.h

    Module Name: MIL

\*=========================================================================*/

MtExtern(CMILMesh3D);

/*=========================================================================*\
    CMILMesh3D - MIL Mesh3D Primitive
\*=========================================================================*/

class CMILMesh3D :
    public CMILObject,
    public IMILMesh3D
{
private:
    CMILMesh3D(
        __in_ecount_opt(1) CMILFactory *pFactory,
        UINT cVertices,
        UINT cIndices,
        __out_ecount(1) HRESULT *result
        );
    
    virtual ~CMILMesh3D();

public:
    DECLARE_MIL_OBJECT
    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CMILMesh3D));

    static __checkReturn HRESULT Create(
        __in_ecount_opt(1) CMILFactory *pFactory, 
        UINT cVertices,
        UINT cIndices,
        __deref_out_ecount(1) IMILMesh3D **ppIMesh3D
        );

    STDMETHOD(PrecomputeLighting)(
        THIS_
        __in_ecount(1) const CMILMatrix *pmatWorldTransform,
        __in_ecount(1) const CMILMatrix *pmatViewTransform,
        __in_ecount(1) CMILLightData *pLightData
        );

    STDMETHOD(CopyPositionsFrom)(
        THIS_
        __in_bcount(cbSize) const dxlayer::vector3 *pVertexPositions,
        size_t cbSize
        );

    STDMETHOD(CopyNormalsFrom)(
        THIS_
        __in_bcount_opt(cbSize) const dxlayer::vector3 *pVertexNormals,
        size_t cbSize
        );

    STDMETHOD(CopyTextureCoordinatesFrom)(
        THIS_
        __in_bcount_opt(cbSize) const dxlayer::vector2 *pVertexTextureCoordinates,
        size_t cbSize
        );

    STDMETHOD(CopyIndicesFrom)(
        THIS_
        __in_bcount(cbSize) const UINT *rgIndices,
        size_t cbSize
        );
    
    STDMETHOD(CloneMesh)(
        THIS_
        __deref_out_ecount(1) IMILMesh3D **ppIMesh3D
        );

    STDMETHOD_(VOID, NotifyPositionChange)(
        THIS_
        BOOL fCalculateNormals
        );

    STDMETHOD_(VOID, NotifyIndicesChange)(
        THIS_
        BOOL fCalculateNormals
        );

    STDMETHOD(GetBounds)(
        __out_ecount(1) MilPointAndSize3F *pboxBounds
        );

    STDMETHOD_(UINT, GetNumVertices)(THIS_) const
        { return m_cVertices; }

    STDMETHOD_(void, GetPositions)(
        __deref_outro_bcount(cbSize) const dxlayer::vector3* &buffer, 
        __out_ecount(1) size_t &cbSize
        ) const
    {
        buffer = m_pvec3Vertices;
        cbSize = m_cbPositions;
    }

    STDMETHOD_(void, GetNormals)(
        __deref_outro_bcount(cbSize) const dxlayer::vector3* &buffer, 
        __out_ecount(1) size_t &cbSize
        ) const
    {
        buffer = m_pvec3Normals;
        cbSize = m_cbNormals;
    }

    STDMETHOD_(void, GetTextureCoordinates)(
        __deref_outro_bcount(cbSize) const dxlayer::vector2* &buffer, 
        __out_ecount(1) size_t &cbSize
        ) const
    {
        buffer = m_pvec2TextureCoordinates;
        cbSize = m_cbTextureCoordinates;
    }

    STDMETHOD_(void, GetIndices)(
        __deref_outro_bcount(cbSize) const UINT* &buffer, 
        __out_ecount(1) size_t &cbSize
        ) const
    {
        buffer = m_puIndices;
        cbSize = m_cbIndices;
    }

    STDMETHOD_(void, GetDiffuseColors)(
        __deref_outro_bcount(cbSize) const DWORD* &buffer, 
        __out_ecount(1) size_t &cbSize
        ) const
    {
        buffer = m_pdwDiffuseColors;
        cbSize = m_cbDiffuseColors;
    }

    STDMETHOD_(void, GetSpecularColors)(
        __deref_outro_bcount(cbSize) const DWORD* &buffer, 
        __out_ecount(1) size_t &cbSize
        ) const
    {
        buffer = m_pdwSpecularColors;
        cbSize = m_cbSpecularColors;
    }
    
    STDMETHOD(SetPosition)(
        UINT index,
        __in_ecount(1) const dxlayer::vector3 &position
        ) 
    {
        HRESULT hr = E_INVALIDARG;
        
        if (index < m_cVertices)
        {
            m_pvec3Vertices[index] = position;
            hr = S_OK;
        }

        RRETURN(THR(hr));
    }

    // NON-API Functions:

    UINT GetNumIndices() const
        { return m_cIndices; }

    HRESULT GetTextureCoordinateBounds(
        __out_ecount(1) CRectF<CoordinateSpace::BaseSampling> *prcTextureCoordinateBounds
        );

    STDMETHOD(CopyTextureCoordinatesFromDoubles)(
        __in_bcount_opt(cbSize) const MilPoint2D *pVertexTextureCoordinates,
        size_t cbSize
        );

    void InvalidateColorCache()
    {
        m_fIsColorCacheValid = false;
    }

private:
    void CalculateNormals();

    HRESULT CalculateBounds();

    UINT *m_puIndices;
    dxlayer::vector3 *m_pvec3Vertices;
    dxlayer::vector3 *m_pvec3Normals;
    dxlayer::vector2 *m_pvec2TextureCoordinates;

    DWORD *m_pdwDiffuseColors;
    DWORD *m_pdwSpecularColors;

    UINT m_cVertices;
    UINT m_cIndices;

    CMilPointAndSize3F m_boxBounds;

    CRectF<CoordinateSpace::BaseSampling> m_rcTextureCoordinateBounds;
    
    BOOL m_fBoundsValid;

    size_t m_cbPositions;
    size_t m_cbNormals;
    size_t m_cbTextureCoordinates;
    size_t m_cbIndices;
    size_t m_cbDiffuseColors;
    size_t m_cbSpecularColors;

    bool m_fIsColorCacheValid;

#ifdef DBG
    BOOL m_fDBGPositionsSet;
    BOOL m_fDBGIndicesSet;
#endif
};


