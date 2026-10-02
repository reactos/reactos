// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DTexture implementation
//
//      Provides basic abstraction of a D3D texture and tracks it as a D3D
//      resource.
//

#include "precomp.hpp"

MtDefine(TextureSurfaceCache, MILRender, "CD3DTexture surface cache array");

//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::CD3DTexture
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CD3DTexture::CD3DTexture()
    : m_pD3DTexture(NULL),
      m_rgSurfaceLevel(NULL)
{
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::~CD3DTexture
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CD3DTexture::~CD3DTexture()
{
    if (m_rgSurfaceLevel)
    {
        while (m_cLevels-- > 0)
        {
            ReleaseInterfaceNoNULL(m_rgSurfaceLevel[m_cLevels]);
        }
        WPFFree(ProcessHeap, m_rgSurfaceLevel);
    }

    ReleaseInterfaceNoNULL(m_pD3DTexture);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::Init
//
//  Synopsis:  Inits the texture wrapper
//
//-------------------------------------------------------------------------
HRESULT 
CD3DTexture::Init(
    __inout_ecount(1) CD3DResourceManager *pResourceManager,
    __inout_ecount(1) IDirect3DTexture9 *pD3DTexture
    )
{
    HRESULT hr = S_OK;

    Assert(m_pD3DTexture == NULL);

    //
    // Get texture information
    //

    m_cLevels = pD3DTexture->GetLevelCount();
    if (   (m_cLevels < 1)
        || (m_cLevels > 32))
    {
        IFC(E_FAIL);
    }

    IFC(pD3DTexture->GetLevelDesc(0, &m_sdLevel0));

    //
    // Init the resouce base class
    //

    IFC(InitResource(pResourceManager, pD3DTexture));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::InitResource
//
//  Synopsis:  Inits the CD3DResouce base class
//
//-------------------------------------------------------------------------
HRESULT 
CD3DTexture::InitResource(
    __inout_ecount(1) CD3DResourceManager *pResourceManager,
    __inout_ecount(1) IDirect3DTexture9 *pD3DTexture
    )
{
    HRESULT hr = S_OK;
    D3DSURFACE_DESC sd;
    UINT uPixelSize;
    UINT uResourceSize = 0;

    //
    // Compute the size of the resource
    //

    for (UINT uLevel = 0; uLevel < m_cLevels; uLevel++)
    {
        // Get description at each level
        IFC(pD3DTexture->GetLevelDesc(uLevel, &sd));

        // Lookup pixel size from D3DFORMAT
        uPixelSize = D3DFormatSize(sd.Format);
        if (uPixelSize == 0)
        {
            IFC(D3DERR_WRONGTEXTUREFORMAT);
        }

        // Accumulate each levels size
        uResourceSize += sd.Width * sd.Height * uPixelSize;
    }
    
    //
    // Init the base class
    //

    CD3DResource::Init(pResourceManager, uResourceSize);

    //
    // Save the D3D texture reference 
    //

    m_pD3DTexture = pD3DTexture;
    m_pD3DTexture->AddRef();

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::ReleaseD3DResources
//
//  Synopsis:  Release the texture.
//
//-------------------------------------------------------------------------
void 
CD3DTexture::ReleaseD3DResources()
{
    ReleaseInterface(m_pD3DTexture);

    return;
}


//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::GetTextureSize
//
//  Synopsis:  Return the dimensions of the texture.
//
//-------------------------------------------------------------------------
void
CD3DTexture::GetTextureSize(
    __out_ecount(1) UINT *puWidth, 
    __out_ecount(1) UINT *puHeight
    ) const
{
    Assert(IsValid());

    *puWidth = m_sdLevel0.Width;
    *puHeight = m_sdLevel0.Height;
}

//+----------------------------------------------------------------------------
//
//  Member:    CD3DTexture::GetD3DSurfaceLevel
//
//  Synopsis:  Get a D3D surface wrapper for specified texture surface level
//

HRESULT
CD3DTexture::GetD3DSurfaceLevel(
    UINT Level,
    __deref_out_ecount(1) CD3DSurface ** const ppSurfaceLevel
    )
{
    HRESULT hr = S_OK;

    IDirect3DSurface9 *pID3DSurface = NULL;

    Assert(m_pD3DTexture);

    Device().Use(*this);

    if (!m_rgSurfaceLevel)
    {
        Assert(m_cLevels <= SIZE_T_MAX / sizeof(*m_rgSurfaceLevel));
        size_t const cbArray = sizeof(*m_rgSurfaceLevel) * m_cLevels;
        m_rgSurfaceLevel =
            WPFAllocTypeClear(CD3DSurface **, ProcessHeap, Mt(TextureSurfaceCache), cbArray);
        IFCOOM(m_rgSurfaceLevel);
    }

    if (!m_rgSurfaceLevel[Level])
    {
        //
        // Get specified texture level 
        //

        IFC(m_pD3DTexture->GetSurfaceLevel(
            Level,
            &pID3DSurface
            ));

        //
        // Create the wrapper
        //

        IFC(CD3DTextureSurface::Create(
            DYNCAST(CD3DResourceManager, m_pManager),
            pID3DSurface,
            OUT &m_rgSurfaceLevel[Level]
            ));
    }

    *ppSurfaceLevel = m_rgSurfaceLevel[Level];
    (*ppSurfaceLevel)->AddRef();

Cleanup:
    ReleaseInterfaceNoNULL(pID3DSurface);

    RRETURN(hr);
}


//+------------------------------------------------------------------------
//
//  Function:  CD3DTexture::UpdateMipmapLevels
//
//  Synopsis:  Update nonzero mipmap levels of the texture based on the zero
//             level.
//
//-------------------------------------------------------------------------
HRESULT CD3DTexture::UpdateMipmapLevels()
{
    HRESULT hr = S_OK;

    CD3DSurface *pSurfaceSrc = NULL;
    CD3DSurface *pSurfaceDst = NULL;

    if (m_cLevels > 1)
    {
        if (Device().CanAutoGenMipMap())
        {
            // This is a hint to the device to autogenerate the mipmaps.  Never fails.
            GetD3DTextureNoRef()->GenerateMipSubLevels();
        }
        else
        {
            IFC(GetD3DSurfaceLevel(0, &pSurfaceSrc));
        
            for (DWORD dw = 1; dw < m_cLevels; ++dw)
            {
                IFC(GetD3DSurfaceLevel(dw, &pSurfaceDst));
                IFC(Device().StretchRect(pSurfaceSrc, NULL, pSurfaceDst, NULL, D3DTEXF_LINEAR));

                ReleaseInterface(pSurfaceSrc);
                pSurfaceSrc = pSurfaceDst;
                pSurfaceDst = NULL;
            }

            ReleaseInterface(pSurfaceSrc);
        }
    }

  Cleanup:

    ReleaseInterfaceNoNULL(pSurfaceSrc);
    ReleaseInterfaceNoNULL(pSurfaceDst);
    
    return hr;
}



//+----------------------------------------------------------------------------
//
//  Member:    
//      CD3DTexture::DetermineUsageAndLevels
//
//  Synopsis:  
//      Determines the usage and levels for a texture that might be mipmapped.
//

void
CD3DTexture::DetermineUsageAndLevels(
    __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
    TextureMipMapLevel eMipMapLevel,
    UINT uTextureWidth,
    UINT uTextureHeight,
    __out_ecount(1) DWORD *pdwUsage,
    __out_ecount(1) UINT *puLevels
    )
{
    if (eMipMapLevel == TMML_One)
    {
        *pdwUsage = 0;
        *puLevels = 1;
    }
    else
    {
        Assert(eMipMapLevel == TMML_All);
        Assert(IS_POWER_OF_2(uTextureWidth));
        Assert(IS_POWER_OF_2(uTextureHeight));

        if (pDevice->CanAutoGenMipMap())
        {
            *pdwUsage = D3DUSAGE_AUTOGENMIPMAP;
            //
            // If we're automatically generating mipmaps, we should pass 0 levels.
            //
            *puLevels = 0;

        }
        else
        {
            //  We should (maybe) generate mipmaps even for
            // cards (e.g. Parhelia) that don't support the stretchrect we use.
            Assert(pDevice->CanStretchRectGenMipMap());

            UINT uMaxSize = max(uTextureWidth, uTextureHeight);

            // Must be RT Usage when it is the target of a StretchRect
            *pdwUsage = D3DUSAGE_RENDERTARGET;
            // Request levels all the way down to 1x1
            *puLevels = Log2(uMaxSize) + 1;
        }
    }
}




