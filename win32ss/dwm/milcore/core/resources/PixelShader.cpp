// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      PixelShader resource.
//
//-----------------------------------------------------------------------------


#include "precomp.hpp"

MtDefine(PixelShaderResource, MILRender, "PixelShader Resource");
MtDefine(CMilPixelShaderDuce, PixelShaderResource, "CMilPixelShaderDuce");
MtDefine(CMilPixelShaderDuceCompiler, PixelShaderResource, "CMilPixelShaderDuceCompiler");

extern HINSTANCE g_DllInstance;

BYTE* CMilPixelShaderDuce::m_pPassThroughShaderBytecodeData;
UINT  CMilPixelShaderDuce::m_cbPassThroughShaderBytecodeSize;

//-----------------------------------------------------------------------------
//
// CMilPixelShaderDuce::Create
//
//-----------------------------------------------------------------------------
HRESULT 
CMilPixelShaderDuce::Create(
    __in_ecount(1) CComposition *pComposition,
    __in ShaderEffectShaderRenderMode::Enum shaderEffectShaderRenderMode,
    __in UINT cbBytecodeSize,
    __in_bcount(cbBytecodeSize) BYTE* pBytecode, 
    __deref_out CMilPixelShaderDuce **ppOut)
{
    HRESULT hr = S_OK;

    CMilPixelShaderDuce* pShader = new CMilPixelShaderDuce();
    IFCOOM(pShader);
    pShader->AddRef();

    IFC(pShader->Initialize(pComposition, shaderEffectShaderRenderMode, cbBytecodeSize, pBytecode));

    *ppOut = pShader;  // Transitioning ref to out argument
    pShader = NULL;

Cleanup:
    ReleaseInterface(pShader);

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilPixelShaderDuce::CMilPixelShaderDuce
//
//-----------------------------------------------------------------------------
CMilPixelShaderDuce::CMilPixelShaderDuce()
{
}

//-----------------------------------------------------------------------------
//
// CMilPixelShaderDuce::Initialize
//
//-----------------------------------------------------------------------------
HRESULT
CMilPixelShaderDuce::Initialize(
    __inout_ecount(1) CComposition *pComposition,
    __in ShaderEffectShaderRenderMode::Enum shaderEffectShaderRenderMode,
    __in UINT cbBytecodeSize, 
    __in BYTE* pBytecode)
{
    HRESULT hr = S_OK;

    m_pComposition = pComposition;
    m_data.m_ShaderRenderMode = shaderEffectShaderRenderMode;
    m_data.m_cbPixelShaderBytecodeSize = cbBytecodeSize;

    // Allocate memory for copy of data
    IFC(HrAlloc(
        Mt(CMilPixelShaderDuce),
        m_data.m_cbPixelShaderBytecodeSize,
        reinterpret_cast<void**>(&m_data.m_pPixelShaderBytecodeData)
        ));

    // Copy data
    RtlCopyMemory(m_data.m_pPixelShaderBytecodeData, pBytecode, m_data.m_cbPixelShaderBytecodeSize);

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilPixelShaderDuce::~CMilPixelShaderDuce
//
//-----------------------------------------------------------------------------

CMilPixelShaderDuce::~CMilPixelShaderDuce()
{
    ReleaseInterface(m_pSwPixelShaderCompiler);
    delete m_hwPixelShaderEffectCache;
    UnRegisterNotifiers();
}

//+----------------------------------------------------------------------------
//
// CMilPixelShaderDuce::GetShaderRenderMode
//
// Synopsis: 
//    Called by the composition layer to determine whether an effect is being
//    forced to run in software or hardware, or is being run with default
//    settings (hardware with automatic software fallback).
//
//-----------------------------------------------------------------------------
ShaderEffectShaderRenderMode::Enum
CMilPixelShaderDuce::GetShaderRenderMode() const
{
   return m_data.m_ShaderRenderMode;
}

//+----------------------------------------------------------------------------
//
// CMilPixelShaderDuce::GetHwPixelShaderEffectFromCache
//
//    Gets the cached pixel shader for the specified cacheIndex. If no cached
//    pixel shader is available a new cached shader is created from the
//    specified byte code.
//
//  Arguments:
//
//  Return: 
//
//    E_INVALIDARG: cacheIndex out of range.
//
//    E_OUTOFMEMORY: Not enough memory to allocate the hw specific pixel shader.
// 
//    D3DERR_OUTOFVIDEOMEMORY: Not enough video memory to allocate a the video
//       memory for holding the pixel shader. 
//
//-----------------------------------------------------------------------------

HRESULT 
CMilPixelShaderDuce::GetHwPixelShaderEffectFromCache(
    __in CD3DDeviceLevel1 *pDevice,   
    __out CHwPixelShaderEffect **ppPixelShaderEffect)
{
    HRESULT hr = S_OK;
    IMILCacheableResource *pResource = NULL;

    if (m_hwPixelShaderEffectCache == NULL)
    {
        // No hw shader cache for device specific IDirect3DPixelShader9 objects has been 
        // allocated  yet.  This means the bytecode hasn't yet been compiled.

        if (m_data.m_CompileSoftwareShader)
        {
            m_ignoreHwShader = false;

            // First compile it for software if we might ever run this in software.
            // This also serves to validate that this is a good shader that we can
            // run in hardware as well.
            CPixelShaderCompiler* pSwPixelShaderCompiler;
            IFC(GetSwPixelShader(&pSwPixelShaderCompiler));
            ReleaseInterface(pSwPixelShaderCompiler);
        }

        // Allocate cache and then use it to cache hw pixel shaders per d3d device.
        m_hwPixelShaderEffectCache = new CMILSimpleResourceCache();
        IFCOOM(m_hwPixelShaderEffectCache);
    }

    //
    // Have a resource cache - check if there is a hw shader effect cached for the pDevice.
    CMILResourceCache::ValidIndex deviceCacheIndex = CMILResourceCache::InvalidToken;

    // Get the unique device cache index for pDevice. 
    IFC(pDevice->GetCacheIndex(&deviceCacheIndex));

    // Use the unique device cache index to lookup a device specific shader
    IFC(m_hwPixelShaderEffectCache->GetResource(deviceCacheIndex, &pResource));

    if (pResource == NULL)
    {
        // No cached pixel shader found. Create the pixel shader and cache it.

        CHwPixelShaderEffect *pEffect = NULL;

        if (!m_ignoreHwShader && m_data.m_cbPixelShaderBytecodeSize != 0)
        {
            // If we have byte code try to create a hw pixel shader.

            hr = CHwPixelShaderEffect::Create(pDevice,
                reinterpret_cast<BYTE*>(m_data.m_pPixelShaderBytecodeData),
                m_data.m_cbPixelShaderBytecodeSize,                                     
                &pEffect);

            if (FAILED(hr))
            {
                if (hr == E_OUTOFMEMORY || hr == D3DERR_OUTOFVIDEOMEMORY)
                {
                    IFC(hr);
                }

                // Setup backchannel notification.
                m_pComposition->SetPendingBadShaderNotification();

                // Use pass through shader instead.
                IFC(EnsurePassThroughShaderResourceRead());
                IFC(CHwPixelShaderEffect::Create(pDevice,
                                                 m_pPassThroughShaderBytecodeData,
                                                 m_cbPassThroughShaderBytecodeSize,
                                                 &pEffect));
            }
        }
        else
        {
            IFC(EnsurePassThroughShaderResourceRead());
            IFC(CHwPixelShaderEffect::Create(pDevice,
                 m_pPassThroughShaderBytecodeData,
                 m_cbPassThroughShaderBytecodeSize,
                 &pEffect));
        }

        pResource = pEffect;

        // Cache the shader at pDevice's cache index "deviceCacheIndex".
        IFC(m_hwPixelShaderEffectCache->SetResource(deviceCacheIndex, pResource));
    }

    *ppPixelShaderEffect = static_cast<CHwPixelShaderEffect*>(pResource);
    pResource = NULL; // Transitioning ref count to out argument. 

  Cleanup:
    ReleaseInterface(pResource);
    
    RRETURN(hr);    
}

//-----------------------------------------------------------------------------
//
// CMilPixelShaderDuce::SetupShader
//
// Synopsis: 
//      Loads and caches the compiled shader binaries, if necessary.
//
// Remarks: 
//      Behaviors:
//         * If no shader is specified, nothing will be rendered. (NULL Shader).
//-----------------------------------------------------------------------------

HRESULT 
CMilPixelShaderDuce::SetupShader(
    __in CD3DDeviceLevel1* pDevice    
    )
{
    HRESULT hr = S_OK;
    CHwPixelShaderEffect *pPixelShaderEffect = NULL;

    // If dirty, force re-creation of the shader.
    IFC(GetHwPixelShaderEffectFromCache(pDevice, &pPixelShaderEffect));

    IFC(pPixelShaderEffect->SendToDevice(pDevice));

    SetDirty(FALSE);

Cleanup:
    ReleaseInterface(pPixelShaderEffect);

    RRETURN(hr);    
}

//-----------------------------------------------------------------------------
//
// CMilPixelShaderDuce::GetSwPixelShader
//
//-----------------------------------------------------------------------------


HRESULT 
CMilPixelShaderDuce::GetSwPixelShader(__deref_out CPixelShaderCompiler **ppPixelShaderCompiler)
{
    HRESULT hr = S_OK;

    // No meter heaps in the pixeljit code, so wrap it all up in our default meter here.
    #ifdef DEBUG
    CSetDefaultMeter mtDefault(Mt(CMilPixelShaderDuce));
    #endif
    
    if (m_pSwPixelShaderCompiler == NULL)
    {
        // Prevent meter warnings from being raised from software compiler. 
        MtSetDefault(Mt(CMilPixelShaderDuceCompiler));

        if (m_data.m_cbPixelShaderBytecodeSize != 0)
        {
            hr = CPixelShaderCompiler::Create(m_data.m_pPixelShaderBytecodeData, m_data.m_cbPixelShaderBytecodeSize, OUT &m_pSwPixelShaderCompiler);

            if (FAILED(hr))
            {
                if (hr == E_OUTOFMEMORY || hr == D3DERR_OUTOFVIDEOMEMORY)
                {
                    IFC(hr);
                }
 
                // Setup backchannel notification.
                m_pComposition->SetPendingBadShaderNotification();
 
                // Use pass through shader instead.
                IFC(EnsurePassThroughShaderResourceRead());
                IFC(CPixelShaderCompiler::Create(reinterpret_cast<void *>(m_pPassThroughShaderBytecodeData), m_cbPassThroughShaderBytecodeSize, OUT &m_pSwPixelShaderCompiler));

                //
                // Ignore the hardware shader set in m_data.
                // It's possible that we got to this method from GetHwPixelShaderEffectFromCache,
                // so unless we explicitly ignore the shader we will pass the invalid shader to DX anyway.
                //
                m_ignoreHwShader = true;
            }
        }
        else
        {
            // If the user code did not send us a shader, treat it as a pass through or identity shader.
            IFC(EnsurePassThroughShaderResourceRead());
            IFC(CPixelShaderCompiler::Create(reinterpret_cast<void *>(m_pPassThroughShaderBytecodeData), m_cbPassThroughShaderBytecodeSize, OUT &m_pSwPixelShaderCompiler));
        }
    }

    *ppPixelShaderCompiler = m_pSwPixelShaderCompiler;
    m_pSwPixelShaderCompiler->AddRef();

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilPixelShaderDuce::OnChanged
//
//-----------------------------------------------------------------------------

BOOL 
CMilPixelShaderDuce::OnChanged(
    CMilSlaveResource *pSender,
    NotificationEventArgs::Flags e
    )
{
    ReleaseInterface(m_pSwPixelShaderCompiler);

    // Delete hw cache... will force recreation of a hwshader next time
    // requested. 
    delete m_hwPixelShaderEffectCache;
    m_hwPixelShaderEffectCache = NULL;
    
    return TRUE; // Bubble changed notification.
}


//-----------------------------------------------------------------------------
//
// CMilPixelShaderDuce::EnsurePassThroughShaderResourceRead()
//
//-----------------------------------------------------------------------------
HRESULT
CMilPixelShaderDuce::EnsurePassThroughShaderResourceRead()
{
    HRESULT hr = S_OK;

    if (m_pPassThroughShaderBytecodeData == NULL)
    {
        IFC(CMilEffectDuce::LockResource(PS_PassThroughShaderEffect,
                                         &m_pPassThroughShaderBytecodeData,
                                         &m_cbPassThroughShaderBytecodeSize));
    }

Cleanup:
    RRETURN(hr);
}


//-----------------------------------------------------------------------------
//
// CMilPixelShaderDuce::GetShaderMajorVersion()
//
//-----------------------------------------------------------------------------
byte
CMilPixelShaderDuce::GetShaderMajorVersion()
{
    if (m_data.m_pPixelShaderBytecodeData != NULL)
    {
        DWORD *version = (DWORD *)m_data.m_pPixelShaderBytecodeData;
        return (*version >> 8) & 0xFF;
    }
    else
    {
        // ps_2_0 by default
        return 2;
    }
}


