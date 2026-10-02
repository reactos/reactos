// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwSolidColorTextureSourcePool implementation
//

#include "precomp.hpp"

MtDefine(CHwSolidColorTextureSourcePool, MILRender, "CHwSolidColorTextureSourcePool");

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::ctor
//
//  Synopsis:  Initializes the member data.
//
//-----------------------------------------------------------------------------
CHwSolidColorTextureSourcePool::CHwSolidColorTextureSourcePool()
{
    m_pD3DDeviceNoRef = NULL;
    m_uNumTexturesOpen = 0;
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::dtor
//
//  Synopsis:  Releases all solid color textures being held onto.
//
//-----------------------------------------------------------------------------
CHwSolidColorTextureSourcePool::~CHwSolidColorTextureSourcePool()
{
    for (UINT i = 0; i < m_rgTextures.GetCount(); i++)
    {
        ReleaseInterfaceNoNULL(m_rgTextures[i]);
    }
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::Init
//
//  Synopsis:  Initializes the pool with the given device.
//
//-----------------------------------------------------------------------------
HRESULT
CHwSolidColorTextureSourcePool::Init(__in_ecount(1) CD3DDeviceLevel1 *pD3DDevice)
{
    HRESULT hr = S_OK;

    Assert(m_pD3DDeviceNoRef == NULL);

    m_pD3DDeviceNoRef = pD3DDevice;

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::RetrieveTexture
//
//  Synopsis:  Given a color, retrieves a texture filled with that color
//
//-----------------------------------------------------------------------------
HRESULT
CHwSolidColorTextureSourcePool::RetrieveTexture(
    __in_ecount(1) const MilColorF &color,
    __deref_out_ecount(1) CHwSolidColorTextureSource **ppTexture
    )
{
    HRESULT hr = S_OK;

    // If we've run out of textures to populate add another one

    if (m_uNumTexturesOpen >= m_rgTextures.GetCount())
    {
        IFC(AddTexture());
    }


    // Update the chosen texture with the desired color

    m_rgTextures[m_uNumTexturesOpen]->SetColor(color);


    // Assign the output texture

    *ppTexture = m_rgTextures[m_uNumTexturesOpen];
    (*ppTexture)->AddRef();


    // Increment the current number we have open in the pool

    ++m_uNumTexturesOpen;

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwSolidColorTextureSource::AddTexture
//
//  Synopsis:  Adds another solid color texture to the pool
//
//-----------------------------------------------------------------------------
HRESULT
CHwSolidColorTextureSourcePool::AddTexture()
{
    HRESULT hr = S_OK;
    CHwSolidColorTextureSource *pTexture = NULL;

    IFC(CHwSolidColorTextureSource::Create(
        m_pD3DDeviceNoRef,
        &pTexture
        ));

   
    //
    // Steal the reference
    // 
    IFC(m_rgTextures.Add(pTexture)); 
    pTexture = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pTexture);

    RRETURN(hr);
}




