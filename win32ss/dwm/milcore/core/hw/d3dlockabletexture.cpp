// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_text
//      $Keywords:
//
//  $Description:
//      Contains CD3DLockableTexture implementation
//
//      Abstract a lockable D3D texture and track it
//       as a D3D resource.
//
//      Also contains CD3DLockableTexturePair implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CD3DLockableTexture, MILRender, "CD3DLockableTexture");
MtDefine(D3DResource_LockableTexture, MILHwMetrics, "Approximate lockable texture size");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::Create
//
//  Synopsis:
//      Create the CD3DLockableTexture
//
//------------------------------------------------------------------------------
HRESULT 
CD3DLockableTexture::Create(
    __inout_ecount(1) CD3DResourceManager *pResourceManager, 
    __in_ecount(1) IDirect3DTexture9 *pD3DTexture,
    __deref_out_ecount(1) CD3DLockableTexture **ppLockableTexture)
{
    HRESULT hr = S_OK;

    *ppLockableTexture = NULL;

    //
    // Create the CD3DLockableTexture
    //

    *ppLockableTexture = new CD3DLockableTexture();
    IFCOOM(*ppLockableTexture);
    (*ppLockableTexture)->AddRef(); // CD3DLockableTexture::ctor sets ref count == 0

    //
    // Call Init
    //

    IFC((*ppLockableTexture)->Init(pResourceManager, pD3DTexture));

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(*ppLockableTexture);
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::CD3DLockableTexture
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CD3DLockableTexture::CD3DLockableTexture()
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::~CD3DLockableTexture
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CD3DLockableTexture::~CD3DLockableTexture()
{
    // InternalDestroy will be called by CD3DTexture destructor.
    //IGNORE_HR(InternalDestroy());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::Init
//
//  Synopsis:
//      Inits the cached bitmap
//
//------------------------------------------------------------------------------
HRESULT 
CD3DLockableTexture::Init(
    __inout_ecount(1) CD3DResourceManager *pResourceManager,
    __in_ecount(1) IDirect3DTexture9 *pD3DTexture
    )
{
    HRESULT hr = S_OK;

    //
    // Let CD3DTexture handle the init
    //

    IFC(CD3DTexture::Init(pResourceManager, pD3DTexture));

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::LockRect
//
//  Synopsis:
//      Delegate to IDirect3DTexture9::LockRect
//
//------------------------------------------------------------------------------
HRESULT 
CD3DLockableTexture::LockRect(
    __out_ecount(1) D3DLOCKED_RECT* pLockedRect, 
    __in_ecount(1) CONST RECT* pRect,
    DWORD dwFlags
    )
{
    HRESULT hr = S_OK;

    Assert((m_sdLevel0.Pool == Device().GetManagedPool()) ||
           (m_sdLevel0.Pool == D3DPOOL_SYSTEMMEM));

    IFC(m_pD3DTexture->LockRect(0, pLockedRect, pRect, dwFlags));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::UnlockRect
//
//  Synopsis:
//      Delegate to IDirect3DTexture9::UnlockRect
//
//------------------------------------------------------------------------------
HRESULT 
CD3DLockableTexture::UnlockRect()
{
    HRESULT hr = S_OK;

    IFC(m_pD3DTexture->UnlockRect(0));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexture::AddDirtyRect
//
//  Synopsis:
//      Delegate to IDirect3DTexture9::AddDirtyRect
//
//------------------------------------------------------------------------------
HRESULT 
CD3DLockableTexture::AddDirtyRect(
    __in_ecount(1) const RECT &rc
    )
{
    HRESULT hr = S_OK;

    IFC(m_pD3DTexture->AddDirtyRect(&rc));

Cleanup:
    RRETURN(hr);
}




//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexturePair::CD3DLockableTexturePair
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CD3DLockableTexturePair::CD3DLockableTexturePair()
{
    m_pTextureMain = NULL;
    m_pTextureAux = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexturePair::~CD3DLockableTexturePair
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CD3DLockableTexturePair::~CD3DLockableTexturePair()
{
    ReleaseInterface(m_pTextureMain);
    ReleaseInterface(m_pTextureAux);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexturePair::InitMain
//
//  Synopsis:
//      Initialize main lockable texture.
//
//------------------------------------------------------------------------------
VOID
CD3DLockableTexturePair::InitMain(
    __in_ecount(1) CD3DLockableTexture *pTexture
    )
{
    Assert(m_pTextureMain == NULL);
    m_pTextureMain = pTexture;
    m_pTextureMain->AddRef();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexturePair::InitAux
//
//  Synopsis:
//      Initialize auxiliary lockable texture.
//
//------------------------------------------------------------------------------
VOID
CD3DLockableTexturePair::InitAux(
    __in_ecount(1) CD3DLockableTexture *pTexture
    )
{
    Assert(m_pTextureAux == NULL);
    m_pTextureAux = pTexture;
    m_pTextureAux->AddRef();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexturePair::Draw
//
//  Synopsis:
//      Render the texture pair using given device. Depending on fUseAux
//      argument, vector alpha values kept in m_pTextureAux will be involved.
//
//------------------------------------------------------------------------------
HRESULT
CD3DLockableTexturePair::Draw(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) const MilPointAndSizeL &rc,
    bool fUseAux
    )
{
    HRESULT hr = S_OK;
    Assert(m_pTextureMain);

    if (fUseAux)
    {
        // vector alpha required, draw in two passes
        Assert(m_pTextureAux);
        IFC(pDevice->RenderTexture(
            m_pTextureAux,
            rc,
            TBM_APPLY_VECTOR_ALPHA
            ));

        IFC(pDevice->RenderTexture(
            m_pTextureMain, 
            rc,
            TBM_ADD_COLORS
            ));
    }
    else
    {
        // regular case, just draw main texture
        IFC(pDevice->RenderTexture(
            m_pTextureMain, 
            rc,
            TBM_DEFAULT
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexturePairLock::Lock
//
//  Synopsis:
//      Prepare one texture for filling with data: lock and clear the rectangle
//      on given texture, that may be either main or auxiliary. Return locked
//      buffer data (pointers and pitch) via LockData structure
//
//------------------------------------------------------------------------------
HRESULT CD3DLockableTexturePairLock::Lock(
    UINT uWidth,
    UINT uHeight,
    __out_ecount(1) LockData &lockData,
    bool fUseAux
    )
{
    Assert(!m_fMainLocked && !m_fAuxLocked);

    HRESULT hr = S_OK;

#if DBG_ANALYSIS
    {
        // Inspect texture sizes and let caller know locked area parameters.
        UINT uWidthMain = 0;
        UINT uHeightMain = 0;
        m_texturePair.m_pTextureMain->GetTextureSize(&uWidthMain, &uHeightMain);

        if (fUseAux)
        {
            UINT uWidthAux = 0;
            UINT uHeightAux = 0;
            m_texturePair.m_pTextureAux->GetTextureSize(&uWidthAux, &uHeightAux);
            Assert(uWidthMain == uWidthAux);
            Assert(uHeightMain == uHeightAux);
        }
    
        // We never attempt to lock outside of texture.
        Assert(uWidth <= uWidthMain);
        Assert(uHeight <= uHeightMain);

        lockData.m_uDbgAnalysisLockedWidth  = uWidth;
        lockData.m_uDbgAnalysisLockedHeight = uHeight;
    }
#endif

    D3DLOCKED_RECT lockedRect;

    // Prepare main texture
    {
        IFC( LockOne(m_texturePair.m_pTextureMain, uWidth, uHeight, &lockedRect) );
        m_fMainLocked = true;
        lockData.pMainBits = static_cast<BYTE*>(lockedRect.pBits);
        lockData.uPitch = static_cast<UINT>(lockedRect.Pitch);
    }

    // Prepare auxiliary texture if necessary
    if (fUseAux)
    {
        IFC( LockOne(m_texturePair.m_pTextureAux, uWidth, uHeight, &lockedRect) );
        m_fAuxLocked = true;
        Assert(lockData.uPitch == lockedRect.Pitch);
        if (lockedRect.Pitch <= 0)
        {
            IFC(WGXERR_INTERNALERROR);
        }
        lockData.pAuxBits = static_cast<BYTE*>(lockedRect.pBits);
    }
    else
    {
        lockData.pAuxBits = NULL;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CD3DLockableTexturePairLock::LockOne
//
//  Synopsis:
//      Prepare one texture for filling with data: lock and clear the rectangle
//      on given texture, that may be either main or auxiliary.
//
//------------------------------------------------------------------------------
HRESULT
CD3DLockableTexturePairLock::LockOne(
    __in_ecount(1) CD3DLockableTexture* pTexture,
    UINT uWidth,
    UINT uHeight,
    __out_ecount(1) D3DLOCKED_RECT* pD3DLockedRect
    )
{
    HRESULT hr = S_OK;

    //
    // Lock the rect and declare it dirty
    //

    CMilRectL rcLock(0, 0, uWidth, uHeight, XYWH_Parameters);
    IFC(pTexture->LockRect(pD3DLockedRect, &rcLock, D3DLOCK_NO_DIRTY_UPDATE));

    IFC(pTexture->AddDirtyRect(rcLock));

    //
    // Clear the rect
    //    

    BYTE *pScanline = (BYTE *)pD3DLockedRect->pBits;
    for (UINT i = 0; i < uHeight; i++)
    {
        ZeroMemory(pScanline, uWidth*sizeof(ARGB));
        pScanline += pD3DLockedRect->Pitch;
    }

Cleanup:
    RRETURN(hr);
}


