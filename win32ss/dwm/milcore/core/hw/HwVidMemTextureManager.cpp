// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:        
//      Implmentation for CHwVidMemTextureManager
//


#include "precomp.hpp"


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::CHwVidMemTextureManager
//
//  Synopsis:  
//      ctor
//

CHwVidMemTextureManager::CHwVidMemTextureManager()
{
    Initialize();
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::Initialize
//
//  Synopsis:  
//      Initilizes member variables
//

void CHwVidMemTextureManager::Initialize()
{
    m_pDeviceNoRef = NULL;

    m_pSysMemSurface = NULL;

    m_pVideoMemTexture = NULL;

    ZeroMemory(&m_d3dsdRequiredForVidMem, sizeof(m_d3dsdRequiredForVidMem));
    m_uLevelsForVidMem = 0;

#if DBG
    m_fDBGSysMemSurfaceIsLocked = false;
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::~CHwVidMemTextureManager
//
//  Synopsis:  
//      dtor
//

CHwVidMemTextureManager::~CHwVidMemTextureManager()
{
    Destroy();
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::Destroy
//
//  Synopsis:  
//      destroys memory held onto by this object
//

void CHwVidMemTextureManager::Destroy()
{
#if DBG
    Assert(!m_fDBGSysMemSurfaceIsLocked);
#endif

    ReleaseInterfaceNoNULL(m_pSysMemSurface);
    ReleaseInterfaceNoNULL(m_pVideoMemTexture);
}

//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::HasRealizationParameters
//
//  Synopsis:  
//      Returns whether the realizaton parameters have been set since this
//      class was constructed/destroyed.
//

bool
CHwVidMemTextureManager::HasRealizationParameters() const
{
    return (m_pDeviceNoRef != NULL);
}

//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::SetRealizationParameters
//
//  Synopsis:  
//      Create method
//

void
CHwVidMemTextureManager::SetRealizationParameters(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    D3DFORMAT d3dFormat,
    UINT uWidth,
    UINT uHeight,
    TextureMipMapLevel eMipMapLevel
    DBG_COMMA_PARAM(bool bDbgConditionalNonPowTwoOkay)
    )
{
    Assert(!HasRealizationParameters());

    m_pDeviceNoRef = pDevice;
        
    ComputeTextureDesc(
        d3dFormat,
        uWidth,
        uHeight,
        eMipMapLevel
        DBG_COMMA_PARAM(bDbgConditionalNonPowTwoOkay)
        );
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::PrepareForNewRealization
//
//  Synopsis:  
//      Destroys realizations in this object and sets it up for re-use.
//

void
CHwVidMemTextureManager::PrepareForNewRealization()
{
    Destroy();
    Initialize();
}

//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::ReCreateAndLockSysMemSurface
//
//  Synopsis:  
//      Creates system memory texture and locks it for updating
//

HRESULT
CHwVidMemTextureManager::ReCreateAndLockSysMemSurface(
    __out_ecount(1) D3DLOCKED_RECT *pD3DLockedRect
    )
{
    HRESULT hr = S_OK;

#if DBG
    Assert(!m_fDBGSysMemSurfaceIsLocked);
#endif

    IDirect3DSurface9 *pID3DSysMemSurface = NULL;

    //
    // Create the surface
    //

    if (!IsSysMemSurfaceValid())
    {
        ReleaseInterface(m_pSysMemSurface);

        IFC(m_pDeviceNoRef->CreateSysMemUpdateSurface(
            m_d3dsdRequiredForVidMem.Width,
            m_d3dsdRequiredForVidMem.Height,
            m_d3dsdRequiredForVidMem.Format,
            NULL, // pvPixels
            &pID3DSysMemSurface
            ));

        IFC(CD3DSurface::Create(
            m_pDeviceNoRef->GetResourceManager(),
            pID3DSysMemSurface,
            &m_pSysMemSurface
            ));

        ReleaseInterface(pID3DSysMemSurface); // ownership transfered to m_pSysMemSurface
    }

    //
    // Lock the entire surface
    //

    {
        RECT rcTextureLock;

        rcTextureLock.left = 0;
        rcTextureLock.top = 0;
        rcTextureLock.right = static_cast<LONG>(m_d3dsdRequiredForVidMem.Width);
        rcTextureLock.bottom = static_cast<LONG>(m_d3dsdRequiredForVidMem.Height);

        IFC(m_pSysMemSurface->LockRect(
            pD3DLockedRect,
            &rcTextureLock,
            D3DLOCK_NO_DIRTY_UPDATE
            ));
    }

#if DBG
    m_fDBGSysMemSurfaceIsLocked = true;
#endif

Cleanup:
    ReleaseInterfaceNoNULL(pID3DSysMemSurface);
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::UnlockSysMemSurface
//
//  Synopsis:  
//      Unlocks the system memory texture. Should be called if
//      ReCreateAndLockSysMemSurface passed.
//

HRESULT
CHwVidMemTextureManager::UnlockSysMemSurface(
    )
{
    HRESULT hr = S_OK;
    
    Assert(IsSysMemSurfaceValid());

#if DBG
    Assert(m_fDBGSysMemSurfaceIsLocked);
    // If the unlock fails, callers should not try to unlock again.
    m_fDBGSysMemSurfaceIsLocked = false;
#endif

    IFC(m_pSysMemSurface->UnlockRect());

Cleanup:
    RRETURN(hr);
}



//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::PushBitsToVidMemTexture
//
//  Synopsis:  
//      Create the video memory texture if necessary and send the bits from the
//      system memory texture to it.
//

HRESULT
CHwVidMemTextureManager::PushBitsToVidMemTexture()
{
    HRESULT hr = S_OK;

    IDirect3DSurface9 *pD3DVidMemSurface = NULL;

    Assert(m_pSysMemSurface->IsValid());

    //
    // Check to see if the video memory texture is already valid
    //

    if (   m_pVideoMemTexture
        && !m_pVideoMemTexture->IsValid()
       )
    {
        ReleaseInterface(m_pVideoMemTexture);
    }

    //
    // (Re)create the video memory texture
    //
    if (NULL == m_pVideoMemTexture)
    {
        IFC(CD3DVidMemOnlyTexture::Create(
            &m_d3dsdRequiredForVidMem,
            m_uLevelsForVidMem,
            true, // fIsEvictable
            m_pDeviceNoRef,
            &m_pVideoMemTexture,
            /* HANDLE *pSharedHandle */ NULL
            ));
    }

    //
    // Get the surface from the texture
    //

    IFC(m_pVideoMemTexture->GetID3DSurfaceLevel(
        0, // uLevel
        &pD3DVidMemSurface
        ));

    //
    // Update the video memory surface
    //

    IFC(m_pDeviceNoRef->UpdateSurface(
        m_pSysMemSurface->ID3DSurface(),
        NULL, // pSourceRect (NULL -> update entire surface)
        pD3DVidMemSurface,
        NULL  // pDestinationPoint (NULL -> update entire surface)
        ));

    //
    // We've dirtied the 0 level and on some cards we need to update the other
    // levels of the mipmaps.  On other cards or if we don't have mipmaps
    // this is a no-op.
    //
    IFC(m_pVideoMemTexture->UpdateMipmapLevels());

Cleanup:
    ReleaseInterfaceNoNULL(pD3DVidMemSurface);

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    
//      CHwVidMemTextureManager::ComputeTextureDesc
//
//  Synopsis:  
//      Compute the texture description used for creating the video memory
//      texture. Make sure that there is nothing wrong with it.
//

void
CHwVidMemTextureManager::ComputeTextureDesc(
    D3DFORMAT d3dFormat,
    UINT uWidth,
    UINT uHeight,
    TextureMipMapLevel eMipMapLevel
    DBG_COMMA_PARAM(bool bDbgConditionalNonPowTwoOkay)
    )
{
    m_d3dsdRequiredForVidMem.Format = d3dFormat;
    m_d3dsdRequiredForVidMem.Type = D3DRTYPE_TEXTURE;
    
    CD3DTexture::DetermineUsageAndLevels(
        m_pDeviceNoRef,
        eMipMapLevel,
        uWidth,
        uHeight,
        OUT &m_d3dsdRequiredForVidMem.Usage,
        OUT &m_uLevelsForVidMem
        );

    m_d3dsdRequiredForVidMem.Pool = D3DPOOL_DEFAULT;
    m_d3dsdRequiredForVidMem.MultiSampleType = D3DMULTISAMPLE_NONE;
    m_d3dsdRequiredForVidMem.MultiSampleQuality = 0;
    m_d3dsdRequiredForVidMem.Width = uWidth;
    m_d3dsdRequiredForVidMem.Height = uHeight;

#if DBG
    Assert(
        m_pDeviceNoRef->GetMinimalTextureDesc(
            &m_d3dsdRequiredForVidMem,
            FALSE,
            (GMTD_IGNORE_FORMAT | 
             (bDbgConditionalNonPowTwoOkay ? GMTD_NONPOW2CONDITIONAL_OK : 0)
             )
            ) == S_OK
        );
#endif
}


