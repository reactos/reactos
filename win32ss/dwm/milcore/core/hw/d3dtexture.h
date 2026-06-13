// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains D3D Texture wrapper class declaration
//

class CD3DTexture : public CD3DResource
{
public:
    static void DetermineUsageAndLevels(
        __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
        TextureMipMapLevel eMipMapLevel,
        UINT uTextureWidth,
        UINT uTextureHeight,
        __out_ecount(1) DWORD *pdwUsage,
        __out_ecount(1) UINT *puLevels
        );

    void GetTextureSize(
        __out_ecount(1) UINT *puWidth, 
        __out_ecount(1) UINT *puHeight
        ) const;

    void GetD3DBaseTexture(
        __deref_out_ecount(1) IDirect3DBaseTexture9 **ppd3dBaseTexture
        )
    {
        Assert(IsValid());
        Assert(m_pD3DTexture);

        *ppd3dBaseTexture = m_pD3DTexture;
        (*ppd3dBaseTexture)->AddRef();
    }

    __out_ecount(1) IDirect3DTexture9 *GetD3DTextureNoRef() const
    {
        Assert(IsValid());
        Assert(m_pD3DTexture);

        return m_pD3DTexture;
    }
    
    const D3DSURFACE_DESC& D3DSurface0Desc() const { return m_sdLevel0; }

    __range(1,32) UINT Levels() const { return m_cLevels; }

    HRESULT GetD3DSurfaceLevel(
        UINT Level,
        __deref_out_ecount(1) CD3DSurface **ppSurfaceLevel
        );

    HRESULT GetID3DSurfaceLevel(
        UINT uLevel,
        __deref_out_ecount(1) IDirect3DSurface9 **ppd3dSurfaceLevel
        ) const
    {
        IDirect3DTexture9 *pD3DTextureNoRef = GetD3DTextureNoRef();

        Assert(pD3DTextureNoRef);

        RRETURN(THR(pD3DTextureNoRef->GetSurfaceLevel(
            uLevel,
            ppd3dSurfaceLevel
            )));
    }

    HRESULT UpdateMipmapLevels();

    override bool RequiresDelayedRelease() const
    {
        return true;
    }

protected:
    CD3DTexture();
    ~CD3DTexture();

    HRESULT Init(
        __inout_ecount(1) CD3DResourceManager *pResourceManager,
        __inout_ecount(1) IDirect3DTexture9 *pD3DTexture
        );

    HRESULT InitResource(
        __inout_ecount(1) CD3DResourceManager *pResourceManager,
        __inout_ecount(1) IDirect3DTexture9 *pD3DTexture
        );

private:

    // 
    // CD3DResource methods
    //

    // Should only be called by CD3DResourceManager (destructor is okay, too)
    void ReleaseD3DResources();
    
protected:

    IDirect3DTexture9 *m_pD3DTexture;
    D3DSURFACE_DESC m_sdLevel0;

private:

    __field_range(1,32) DWORD m_cLevels;

    // Cache of surface levels
    __field_ecount_full_opt(m_cLevels) CD3DSurface **m_rgSurfaceLevel;
};


