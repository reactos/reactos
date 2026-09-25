// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CHwPixelShaderEffect, CD3DDeviceLevel1, "CHwPixelShaderEffect");
MtDefine(CHwPixelShaderEffect_NotImplementedAndShouldAlwaysBeZero, CHwPixelShaderEffect, "Not implemented and therefore meaningless.");


//+-----------------------------------------------------------------------------
//
//  CHwPixelShaderEffect::Create
//
//  Synopsis:
//      This class on to a D3D specific shader resource.
//
//------------------------------------------------------------------------------

HRESULT 
CHwPixelShaderEffect::Create(
    __in CD3DDeviceLevel1 *pDevice,
    __in_bcount(sizeInBytes) BYTE *pPixelShaderByteCode,
    __in UINT sizeInBytes,
    __out CHwPixelShaderEffect **ppHwPixelShaderEffect)
{
    HRESULT hr = S_OK;

    CHwPixelShaderEffect* pSE = new CHwPixelShaderEffect();
    IFCOOM(pSE);
    pSE->AddRef(); // Resources start out with ref-count 0.

    IFC(pSE->Init(pDevice, pPixelShaderByteCode, sizeInBytes));

    *ppHwPixelShaderEffect = pSE;
    pSE = NULL;

Cleanup:
    ReleaseInterface(pSE);
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  CHwPixelShaderEffect::Init
//
//  Synopsis:
//      Initializes the class by allocating a device specific pixel shader.
//
//------------------------------------------------------------------------------

HRESULT 
CHwPixelShaderEffect::Init(
    __in CD3DDeviceLevel1 *pDevice,
    __in_bcount(sizeInBytes) BYTE *pPixelShaderByteCode,
    __in UINT sizeInBytes)
{
    HRESULT hr = S_OK;

    IFC(pDevice->CreatePixelShader(reinterpret_cast<DWORD*>(pPixelShaderByteCode), &m_pD3DPixelShader));

    CD3DResource::Init(pDevice->GetResourceManager(), sizeInBytes /* this is a guess for the shader size, but that is ok */);

#if DBG
    m_pDbgDeviceNoRef = pDevice;
#endif

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  CHwPixelShaderEffect::SendToDevice
//
//  Synopsis:
//      Set the pixel shader on the device. The device must be the same as the 
//      one against which the pixel shader has been created. 
//------------------------------------------------------------------------------


HRESULT 
CHwPixelShaderEffect::SendToDevice(
    __in CD3DDeviceLevel1 *pDevice)
{
    HRESULT hr = S_OK;

#if DBG
    Assert(m_pDbgDeviceNoRef == pDevice);
#endif

    IFC(pDevice->SetPixelShader(m_pD3DPixelShader));

Cleanup:
    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  CHwPixelShaderEffect::ReleaseD3DResources (override CD3DResource)
//
//  Synopsis:
//     Called by the device to release D3D resources associated with it on 
//     device lost, shutdown, etc.
//      
//------------------------------------------------------------------------------

void
CHwPixelShaderEffect::ReleaseD3DResources()
{
    ReleaseInterface(m_pD3DPixelShader);
}


