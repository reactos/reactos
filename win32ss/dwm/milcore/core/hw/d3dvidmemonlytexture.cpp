// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DVidMemOnlyTexture implementation
//
//      Abstract a POOL_DEFAULT D3D texture and track it
//       as a D3D resource.
//

#include "precomp.hpp"

MtDefine(CD3DVidMemOnlyTexture, MILRender, "CD3DVidMemOnlyTexture");
MtDefine(D3DResource_VidMemOnlyTexture, MILHwMetrics, "Approximate vid-mem-only texture size");

//+------------------------------------------------------------------------
//
//  Function:  CD3DVidMemOnlyTexture::Create
//
//  Synopsis:  Create the CD3DVidMemOnlyTexture
//
//-------------------------------------------------------------------------
HRESULT 
CD3DVidMemOnlyTexture::Create(
    __in_ecount(1) const D3DSURFACE_DESC *pSurfDesc,
    UINT uLevels,
    bool fIsEvictable,
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
    __deref_out_ecount(1) CD3DVidMemOnlyTexture **ppVidMemOnlyTexture,
    __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
    )
{
    HRESULT hr = S_OK;

    IDirect3DTexture9 *pD3DTexture = NULL;
    
    Assert(pSurfDesc->Pool == D3DPOOL_DEFAULT);

    //
    // Allocate the D3D texture
    //

    IFC(pDevice->CreateTexture(
        pSurfDesc,
        uLevels,
        OUT &pD3DTexture,
        IN OUT pSharedHandle
        ));

    //
    // Utilize other Create method
    //

    IFC(Create(
        pD3DTexture,
        fIsEvictable,
        pDevice,
        OUT ppVidMemOnlyTexture
        ));

Cleanup:
    ReleaseInterfaceNoNULL(pD3DTexture);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CD3DVidMemOnlyTexture::Create
//
//  Synopsis:
//      Creates a CD3DVidMemOnlyTexture resource which wraps an existing D3D
//      texture.
//

HRESULT
CD3DVidMemOnlyTexture::Create(
    __inout_ecount(1) IDirect3DTexture9 *pD3DExistingTexture,
    bool fIsEvictable,
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
    __deref_out_ecount(1) CD3DVidMemOnlyTexture **ppVidMemOnlyTexture
    )
{
    HRESULT hr = S_OK;

    CD3DVidMemOnlyTexture *pVidMemOnlyTexture = NULL;

    //
    // Create the CD3DVidMemOnlyTexture
    //

    pVidMemOnlyTexture = new CD3DVidMemOnlyTexture();
    IFCOOM(pVidMemOnlyTexture);
    pVidMemOnlyTexture->AddRef(); // CD3DVidMemOnlyTexture::ctor sets ref count == 0

    //
    // Call Init
    //

    IFC(pVidMemOnlyTexture->Init(pDevice->GetResourceManager(), pD3DExistingTexture));
    
    Assert(pVidMemOnlyTexture->m_sdLevel0.Pool == D3DPOOL_DEFAULT);

    if (fIsEvictable)
    {
        pVidMemOnlyTexture->SetAsEvictable();
    }

    *ppVidMemOnlyTexture = pVidMemOnlyTexture;
    pVidMemOnlyTexture = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pVidMemOnlyTexture);

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DVidMemOnlyTexture::CD3DVidMemOnlyTexture
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CD3DVidMemOnlyTexture::CD3DVidMemOnlyTexture()
{

}

//+------------------------------------------------------------------------
//
//  Function:  CD3DVidMemOnlyTexture::~CD3DVidMemOnlyTexture
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CD3DVidMemOnlyTexture::~CD3DVidMemOnlyTexture()
{
    // InternalDestroy will be called by CD3DTexture destructor.
    //IGNORE_HR(InternalDestroy());
}

//+------------------------------------------------------------------------
//
//  Function:  CD3DVidMemOnlyTexture::Init
//
//  Synopsis:  Inits the texture
//
//-------------------------------------------------------------------------
HRESULT 
CD3DVidMemOnlyTexture::Init(
    __inout_ecount(1) CD3DResourceManager *pResourceManager,
    __inout_ecount(1) IDirect3DTexture9 *pD3DTexture
    )
{
    HRESULT hr = S_OK;

    Assert(pResourceManager);
    Assert(pD3DTexture);

    //
    // Let CD3DTexture handle the init
    //

    IFC(CD3DTexture::Init(pResourceManager, pD3DTexture));

Cleanup:
    RRETURN(hr);
}


