// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CD3DTextureSurface implementation
//
//      Provides basic abstraction of a D3D surface which is part of a D3D
//      texture.
//

#include "precomp.hpp"

MtDefine(CD3DTextureSurface, MILRender, "CD3DTextureSurface");
MtDefine(D3DResource_TextureSurface, MILHwMetrics, "Approximate surface sizes");

//+------------------------------------------------------------------------
//
//  Member:    CD3DTextureSurface::Create
//
//  Synopsis:  Create a CD3DTextureSurface object to wrap a D3D texture
//
//-------------------------------------------------------------------------
HRESULT 
CD3DTextureSurface::Create(
    __inout_ecount(1) CD3DResourceManager *pResourceManager,
    __inout_ecount(1) IDirect3DSurface9 *pID3DSurface,
    __deref_out_ecount(1) CD3DSurface **ppSurface
    )
{
    HRESULT hr = S_OK;

    CD3DTextureSurface *pTextureSurface = NULL;

    //
    // Create the D3D surface wrapper
    //

    pTextureSurface = new CD3DTextureSurface(pID3DSurface);
    IFCOOM(pTextureSurface);
    pTextureSurface->AddRef(); // CD3DTextureSurface::ctor sets ref count == 0

    //
    // Call init
    //

    IFC(pTextureSurface->Init(pResourceManager));

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(pTextureSurface);
    }

    *ppSurface = pTextureSurface;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DTextureSurface::CD3DTextureSurface
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CD3DTextureSurface::CD3DTextureSurface(
    __inout_ecount(1) IDirect3DSurface9 * const pD3DSurface
    )
    : CD3DSurface(pD3DSurface)
{
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DTextureSurface::Init
//
//  Synopsis:  Inits the texture surface wrapper
//
//-------------------------------------------------------------------------
HRESULT 
CD3DTextureSurface::Init(
    __inout_ecount(1) CD3DResourceManager *pResourceManager
    )
{
    HRESULT hr = S_OK;

    //
    // Compute the size of the resource
    //

    IFC(m_pD3DSurface->GetDesc(&m_d3dsd));

    //
    // Init the base class, use a zero size because texture wrapper will
    // already register with a size acounting for all levels
    //

    CD3DResource::Init(pResourceManager, 0);

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:    CD3DTextureSurface::ReleaseD3DResources
//
//  Synopsis:  Release the surface.
//
//             This method may only be called by CD3DResourceManager because
//             there are various restrictions around when a call to
//             ReleaseD3DResources is okay.
//
//-------------------------------------------------------------------------
void 
CD3DTextureSurface::ReleaseD3DResources()
{
    // This resource should have been marked invalid already or at least be out
    // of use.
    Assert(!m_fResourceValid || (m_cRef == 0));
    Assert(IsValid() == m_fResourceValid);

    // This context is protected so it is safe to release the D3D resource
    ReleaseInterface((*const_cast<IDirect3DSurface9 **>(&m_pD3DSurface)));

    return;
}



