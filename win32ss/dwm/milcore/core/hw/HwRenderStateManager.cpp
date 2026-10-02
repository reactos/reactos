// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


#include "precomp.hpp"
//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwRenderStateManager declaration
//
//      The purpose of this class is to centralize all states used by our
//      rendering engine so that we can reduce state transitions for better
//      performance.  We also limit the states that can be set so that
//      we can understand and restrict the requirements for our engine.
//
//      This class also maintains a table of render states and will only
//      call D3D on the states that change.  We've measured noticeable
//      performance improvements by avoiding the D3D API overhead.
//
//      NOTE-2004/05/21-chrisra State blocks are not a win.
//
//      Removing the stateblocks to go to setting the states and restoring them
//      for 3D saved about 20% on our scenarios.  If we have to manage more
//      states that may change, but for the time it looks like a big win to keep
//      from using stateblocks.
//

using namespace dxlayer;

MtDefine(CHwRenderStateManager, MILRender, "CHwRenderStateManager");
DeclareTag(tagWireframe, "MIL-HW", "Wireframe");

//+----------------------------------------------------------------------------
//
//  Function:   CHwRenderStateManager::Create
//
//  Synopsis:   Creates the Manager.
//
//-----------------------------------------------------------------------------
__checkReturn HRESULT
CHwRenderStateManager::Create(
    __inout_ecount(1) IDirect3DDevice9 *pD3DDevice,
    DWORD dwMaxBlendStages,
    BOOL fCanHandleBlendFactor,
    BOOL fSupportsScissorRect,
    __range(1,UINT_MAX) DWORD dwMaxStream,
    __range(1,UINT_MAX) DWORD dwAnisotropicFilterLevel,
    __deref_out_ecount(1) CHwRenderStateManager **ppStateManager
    )
{
    HRESULT hr = S_OK;
    CHwRenderStateManager *pNewStateManager = NULL;

    pNewStateManager = new CHwRenderStateManager;
    IFCOOM(pNewStateManager);

    IFC(pNewStateManager->Init(
        pD3DDevice,
        dwMaxBlendStages,
        fCanHandleBlendFactor,
        fSupportsScissorRect,
        dwMaxStream,
        dwAnisotropicFilterLevel
        ));

    pNewStateManager->AddRef();

    *ppStateManager = pNewStateManager;
    pNewStateManager = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pNewStateManager);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   CHwRenderStateManager::Init
//
//  Synopsis:   Initializes the size of all the state tables.
//
//-----------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::Init(
    __inout_ecount(1) IDirect3DDevice9 *pD3DDevice,
    DWORD dwMaxBlendStages,
    BOOL fCanHandleBlendFactor,
    BOOL fSupportsScissorRect,
    __range(1,UINT_MAX) DWORD dwMaxStream,
    __range(1,UINT_MAX) DWORD dwAnisotropicFilterLevel
    )
{
    HRESULT hr = S_OK;

    Assert(pD3DDevice && !m_pID3DDevice);

    m_pID3DDevice = pD3DDevice;
    m_pID3DDevice->AddRef();

    IGNORE_HR(m_pID3DDevice->QueryInterface(IID_IDirect3DDevice9Ex, (void**)&m_pID3DDeviceEx));

    m_nMaxTextureBlendStage = dwMaxBlendStages;

    IFC(m_renderStates.Init(
        NUM_D3DRS
        ));

    IFC(m_nonWorldTransforms.Init(
        NUM_D3DNONWORLDTRANSFORMS
        ));

    IFC(m_worldTransform.Init(
        1
        ));

    IFC(m_textureStageStates.Init(
        NUM_D3DTSS*MIL_TEXTURE_STAGE_COUNT
        ));

    IFC(m_samplerStageStates.Init(
        NUM_D3DTSAMPLERSTATES*MIL_SAMPLER_COUNT
        ));

    IFC(m_textures.Init(
        MIL_TEXTURE_STAGE_COUNT
        ));

    IFC(m_stateFVF.Init(
        1
        ));

    IFC(m_stateVertexShader.Init(
        1
        ));

    IFC(m_statePixelShader.Init(
        1
        ));

    //     Add adjustable state table support
    //  Adding redundancy checking for the shader constants adds
    //  about 4.5 KB working set per device. Most shaders won't get
    //  close to using 256 vertex shader constants. The perf team
    //  is fine with this... it's just a little sloppy.
    
    IFC(m_stateVertexShaderFloat4Constants.Init(
        256
        ));

    IFC(m_statePixelShaderFloat4Constants.Init(
        224         // ps_3_0 supports 224 float registers
        ));

    IFC(m_statePixelShaderInt4Constants.Init(
        16         // ps_3_0 supports 16 int registers
        ));

    IFC(m_statePixelShaderBoolConstants.Init(
        16         // ps_3_0 supports 16 bool registers
        ));

    IFC(m_depthStencilSurface.Init(
        1
        ));

    IFC(m_indexStream.Init(
        1
        ));

    IFC(m_streamSourceVertexBuffer.Init(
        1
        ));

    IFC(m_streamSourceVertexStride.Init(
        1
        ));

    IFC(SetDefaultState(
        fCanHandleBlendFactor,
        fSupportsScissorRect,
        dwMaxStream,
        dwAnisotropicFilterLevel
        ));

#if DBG
    SetSupportedTable();
#endif

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:   CHwRenderStateManager::Check2DTransformInVertexShader
//
//  Synopsis:   Check to see if the shader constants being set are overwriting
//              the 2d transform currently stored.
//
//-----------------------------------------------------------------------------
void
CHwRenderStateManager::Check2DTransformInVertexShader(
    UINT nRegisterIndex,
    UINT nRegisterCount
    )
{
    //
    // If the registers being set intersect with the 2d transform range, invalidate
    // the transform.
    //
    if (   nRegisterIndex + nRegisterCount > m_u2DTransformVertexShaderStartRegister
        && nRegisterIndex < m_u2DTransformVertexShaderStartRegister +
                ShaderConstantTraits<ShaderFunctionConstantData::Matrix4x4>::RegisterSize
           )
    {
        m_f2DTransformUsedForVertexShader = FALSE;
    }
}


//+----------------------------------------------------------------------------
//
//  Function:   CHwRenderStateManager::ctor
//
//  Synopsis:   Initializes the memers.
//
//-----------------------------------------------------------------------------
CHwRenderStateManager::CHwRenderStateManager()
{
    m_pID3DDevice = NULL;
    m_pID3DDeviceEx = NULL;
    m_f2DTransformsUsedForFixedFunction = FALSE;
    m_f2DTransformUsedForVertexShader = FALSE;
    m_u2DTransformVertexShaderStartRegister = 0;

    m_nMaxTextureBlendStage = 0;

    m_fClipSet = FALSE;

    m_uDepthStencilSurfaceWidth  = 0;
    m_uDepthStencilSurfaceHeight = 0;

    m_fTexCoordIndicesDefault = false;

    //
    // No need to set m_rcClip since m_fClipSet is FALSE.
    //
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::dtor
//
//  Synopsis:   Release Objects
//
//-------------------------------------------------------------------------
CHwRenderStateManager::~CHwRenderStateManager()
{
    ReleaseInterface(m_pID3DDevice);
    ReleaseInterface(m_pID3DDeviceEx);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::GetTransform
//
//  Synopsis:   Retrieves the transform set.  This function branches based on
//              the transform state required, and will return only 1 type of
//              WORLD transform, and that's D3DTS_WORLD.  It's the only world
//              transform we use, and by avoiding the rest of the world
//              transform table we avoid about 16k of memory.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::GetTransform(
    D3DTRANSFORMSTATETYPE state,
    __out_ecount(1) CMILMatrix * pMatrix
    ) const
{
    HRESULT hr = S_OK;

    if (state < 256)
    {
        IFC(m_nonWorldTransforms.GetState(
            state,
            pMatrix
            ));
    }
    else
    {
        //
        // There are 256 possible world transforms in D3D but since we only use
        // 1, we only support getting the value of 1 of them.
        //

        Assert(state == D3DTS_WORLD);

        IFC(m_worldTransform.GetState(
            0,
            pMatrix
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::ForceSetTexture
//
//  Synopsis:   Sets the texture on the D3D Device and then updates our state
//              settings based on it's success.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::ForceSetTexture(
    DWORD dwStage,
    __in_ecount_opt(1) IDirect3DBaseTexture9 *pTexture
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetTexture(
        dwStage,
        pTexture
        ));

Cleanup:
    m_textures.UpdateState(
        hr,
        dwStage,
        pTexture
        );

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::ForceSetTextureStageState
//
//  Synopsis:   Sets the texture stage state on the D3D Device and then
//              updates our state settings based on it's success.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::ForceSetTextureStageState(
    DWORD dwStage,
    D3DTEXTURESTAGESTATETYPE state,
    DWORD dwValue
    )
{
    HRESULT hr = S_OK;

    if (state == D3DTSS_TEXCOORDINDEX)
    {
        if (dwValue != dwStage)
        {
            m_fTexCoordIndicesDefault = false;
        }
    }

    IFC(m_pID3DDevice->SetTextureStageState(
        dwStage,
        state,
        dwValue
        ));

Cleanup:
    m_textureStageStates.UpdateState(
        hr,
        CalcTextureStageStatePos(dwStage, state),
        dwValue
        );

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::SetDefaultTexCoordIndices
//
//  Synopsis:   Sets the values of D3DTSS_TEXCOORDINDEX for each stage
//              to its default (same as the stage number.)
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::SetDefaultTexCoordIndices(
    )
{
    HRESULT hr = S_OK;

    if (!m_fTexCoordIndicesDefault)
    {
        for (DWORD dwStage = 0; dwStage < m_nMaxTextureBlendStage; ++dwStage)
        {
            IFC(SetTextureStageState(
                dwStage,
                D3DTSS_TEXCOORDINDEX,
                dwStage
                ));
        }

        m_fTexCoordIndicesDefault = true;
    }

Cleanup:

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::ForceSetSamplerState
//
//  Synopsis:   Sets the sampler stage state on the D3D Device and then
//              updates our state settings based on it's success.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::ForceSetSamplerState(
    DWORD dwSampler,
    D3DSAMPLERSTATETYPE state,
    DWORD dwValue
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetSamplerState(
        dwSampler,
        state,
        dwValue
        ));

Cleanup:
    m_samplerStageStates.UpdateState(
        hr,
        CalcSamplerStatePos(dwSampler, state),
        dwValue
        );

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::ForceSetFVF
//
//  Synopsis:   Sets the FVF on the D3D Device and then updates our state
//              settings based on it's success.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::ForceSetFVF(
    DWORD dwFVF
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetFVF(
        dwFVF
        ));

Cleanup:
    m_stateFVF.UpdateState(
        hr,
        0,
        dwFVF
        );

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::ForceSetVertexShader
//
//  Synopsis:   Sets the Vertex Shader on the D3D Device and then updates our
//              state settings based on it's success.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::ForceSetVertexShader(
    __in_ecount_opt(1) IDirect3DVertexShader9 *pVertexShader
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetVertexShader(
        pVertexShader
        ));

Cleanup:
    m_stateVertexShader.UpdateState(
        hr,
        0,
        pVertexShader
        );

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::ForceSetPixelShader
//
//  Synopsis:   Sets the Pixel Shader on the D3D Device and then updates our
//              state settings based on it's success.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::ForceSetPixelShader(
    __in_ecount_opt(1) IDirect3DPixelShader9 *pPixelShader
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetPixelShader(
        pPixelShader
        ));

Cleanup:
    m_statePixelShader.UpdateState(
        hr,
        0,
        pPixelShader
        );

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::ForceSetDepthStencilSurface
//
//  Synopsis:   Sets the depth/stencil buffer on the D3D Device and then updates
//              our state settings based on it's success.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::ForceSetDepthStencilSurface(
    __in_ecount_opt(1) IDirect3DSurface9 *pDepthStencilSurface,
    UINT uWidth,
    UINT uHeight
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetDepthStencilSurface(
        pDepthStencilSurface
        ));

    m_uDepthStencilSurfaceWidth = uWidth;
    m_uDepthStencilSurfaceHeight = uHeight;

Cleanup:
    m_depthStencilSurface.UpdateState(
        hr,
        0,
        pDepthStencilSurface
        );

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::ForceSetRenderState
//
//  Synopsis:   Sets the RenderState on the D3D Device and then updates our
//              state settings based on it's success.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::ForceSetRenderState(
    D3DRENDERSTATETYPE state,
    DWORD dwValue
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetRenderState(
        state,
        dwValue
        ));

Cleanup:
    m_renderStates.UpdateState(
        hr,
        state,
        dwValue
        );

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::ForceSetWorldTransform
//
//  Synopsis:   Sets the World transform on the D3D Device and then updates our
//              state settings based on it's success.
//
//              Since we only use 1 world transform we don't store all 256
//              possible world transforms.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::ForceSetWorldTransform(
    __in_ecount(1) const CBaseMatrix *pMatrix
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetTransform(
        D3DTS_WORLD,
        pMatrix
        ));

Cleanup:
    m_worldTransform.UpdateState(
        hr,
        0,
        *pMatrix
        );

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::ForceSetNonWorldTransform
//
//  Synopsis:   Sets the Non World transform on the D3D Device and then updates
//              our state settings based on it's success.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::ForceSetNonWorldTransform(
    D3DTRANSFORMSTATETYPE state,
    __in_ecount(1) const CBaseMatrix *pMatrix
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetTransform(
        state,
        pMatrix
        ));

Cleanup:
    m_nonWorldTransforms.UpdateState(
        hr,
        state,
        *pMatrix
        );

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::SetPixelShaderConstantF
//
//  Synopsis:   Sets the shader constant on the device.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::SetPixelShaderConstantF(
    UINT nRegisterIndex,
    __in_ecount(4*nRegisterCount) const float * prConstantData,
    UINT nRegisterCount
    )
{
    HRESULT hr = S_OK;

    // Shader constants are typically one register or a matrix (four). In the 
    // matrix case, it's rare that only one line changes so to reduce calls to
    // D3D we will set all four with one call as soon as we find a difference
    bool fAnyDontMatch = false;
    for (UINT i = 0; i < nRegisterCount; ++i)
    {
        const vector4 &pd3dVector4 = reinterpret_cast<const vector4 &>(prConstantData[4 * i]);
        if (!m_statePixelShaderFloat4Constants.IsStateSet(nRegisterIndex + i, pd3dVector4))
        {
            fAnyDontMatch = true;
            break;
        }
    }

    if (fAnyDontMatch)
    {
        IFC(ForceSetPixelShaderConstantF(
            nRegisterIndex,
            prConstantData,
            nRegisterCount
            ));
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CHwRenderStateManager::ForceSetPixelShaderConstantF(
    UINT nRegisterIndex,
    __in_ecount(4*nRegisterCount) const float * prConstantData,
    UINT nRegisterCount
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetPixelShaderConstantF(
        nRegisterIndex,
        prConstantData,
        nRegisterCount
        ));

Cleanup:
    for (UINT i = 0; i < nRegisterCount; ++i)
    {
        const vector4 &pd3dVector4 = reinterpret_cast<const vector4&>(prConstantData[4 * i]);
        m_statePixelShaderFloat4Constants.UpdateState(
            hr, 
            nRegisterIndex + i, 
            pd3dVector4
            );
    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::SetPixelShaderConstantI
//
//  Synopsis:   Sets the shader constant on the device.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::SetPixelShaderConstantI(
    UINT nRegisterIndex,
    __in_ecount(4) const int * pConstantData
    )
{
    HRESULT hr = S_OK;

    bool fDoesNotMatch = false;

    // Even though there are 4 elements, they are all the same, so only cache the first one.
    const int &int4 = reinterpret_cast<const int &>(pConstantData[0]);
    if (!m_statePixelShaderInt4Constants.IsStateSet(nRegisterIndex, int4))
    {
        fDoesNotMatch = true;
    }

    if (fDoesNotMatch)
    {
        IFC(ForceSetPixelShaderConstantI(
            nRegisterIndex,
            pConstantData
            ));
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CHwRenderStateManager::ForceSetPixelShaderConstantI(
    UINT nRegisterIndex,
    __in_ecount(4) const int * pConstantData
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetPixelShaderConstantI(
        nRegisterIndex,
        pConstantData,
        1
        ));

Cleanup:
    const int &int4 = reinterpret_cast<const int &>(pConstantData[0]);
    m_statePixelShaderInt4Constants.UpdateState(
        hr, 
        nRegisterIndex, 
        int4
        );
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::SetPixelShaderConstantB
//
//  Synopsis:   Sets the shader constant on the device.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::SetPixelShaderConstantB(
    UINT nRegisterIndex,
    BOOL constantData
    )
{
    HRESULT hr = S_OK;

    bool fDoesNotMatch = false;
    if (!m_statePixelShaderBoolConstants.IsStateSet(nRegisterIndex, constantData))
    {
        fDoesNotMatch = true;
    }

    if (fDoesNotMatch)
    {
        IFC(ForceSetPixelShaderConstantB(
            nRegisterIndex,
            constantData
            ));
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CHwRenderStateManager::ForceSetPixelShaderConstantB(
    UINT nRegisterIndex,
    BOOL constantData
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetPixelShaderConstantB(
        nRegisterIndex,
        &constantData,
        1
        ));

Cleanup:
    m_statePixelShaderBoolConstants.UpdateState(
        hr, 
        nRegisterIndex, 
        constantData
        );
    RRETURN(hr);
}
    
//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::SetVertexShaderConstantF
//
//  Synopsis:   Sets the shader constant on the device.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::SetVertexShaderConstantF(
    UINT nRegisterIndex,
    __in_ecount(4*nRegisterCount) const float * prConstantData,
    UINT nRegisterCount
    )
{
    HRESULT hr = S_OK;

    //     Remove this
    //  This was a trick to keep 2D from sending the transforms multiple
    //  times. It should no longer be necessary but I don't want to break
    //  something right now.
    Check2DTransformInVertexShader(
        nRegisterIndex,
        nRegisterCount
        );

    // See comment is SetPixelShaderConstantF for fAnyDontMatch explanation
    bool fAnyDontMatch = false;
    for (UINT i = 0; i < nRegisterCount; ++i)
    {
        const vector4 &pd3dVector4 = reinterpret_cast<const vector4 &>(prConstantData[4 * i]);
        if (!m_stateVertexShaderFloat4Constants.IsStateSet(nRegisterIndex + i, pd3dVector4))
        {
            fAnyDontMatch = true;
            break;
        }
    }

    if (fAnyDontMatch)
    {
        IFC(ForceSetVertexShaderConstantF(
            nRegisterIndex,
            prConstantData,
            nRegisterCount
            ));
    }

Cleanup:
    RRETURN(hr);
}

HRESULT
CHwRenderStateManager::ForceSetVertexShaderConstantF(
    UINT nRegisterIndex,
    __in_ecount(4*nRegisterCount) const float * prConstantData,
    UINT nRegisterCount
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetVertexShaderConstantF(
        nRegisterIndex,
        prConstantData,
        nRegisterCount
        ));

Cleanup:
    for (UINT i = 0; i < nRegisterCount; ++i)
    {
        const vector4 &pd3dVector4 = reinterpret_cast<const vector4 &>(prConstantData[4 * i]);
        m_stateVertexShaderFloat4Constants.UpdateState(
            hr, 
            nRegisterIndex + i, 
            pd3dVector4
            );
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:     CD3DRenderState::InvalidateScissorRect
//
//  Synopsis:   Invalidates the current scissor rect so that next time
//              SetScissorRect is called, the rect will actually be set
//              in the device.
//
//      2004/05/21 chrisra
//          Ported it from CD3DRenderStateManager
//------------------------------------------------------------------------------
void
CHwRenderStateManager::InvalidateScissorRect()
{
    m_renderStates.SetToUnknown(D3DRS_SCISSORTESTENABLE);
    ZeroMemory(&m_rcScissorRect, sizeof(m_rcScissorRect));
}

//+-----------------------------------------------------------------------------
//
//  Member:     CD3DRenderState::ScissorRectChanged
//
//  Synopsis:   Notifies the render state that the scissor rect has changed
//              somehow (i.e. via SetRenderTarget) without SetScissorRect
//              being called.
//
//      2004/05/21 chrisra
//          Ported it from CD3DRenderStateManager
//------------------------------------------------------------------------------
void
CHwRenderStateManager::ScissorRectChanged(
    __in_ecount(1) const MilPointAndSizeL *prc
    )
{
    RtlCopyMemory(&m_rcScissorRect, prc, sizeof(m_rcScissorRect));
}

//+-----------------------------------------------------------------------------
//
//  Member:     CD3DRenderState::SetScissorRect
//
//  Synopsis:   Sets the scissor rect on the device, or disables it if
//              prc == NULL
//
//      2004/05/21 chrisra
//          Ported it from CD3DRenderStateManager
//------------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::SetScissorRect(
    __in_ecount_opt(1) const MilPointAndSizeL *prc
    )
{
    HRESULT hr = S_OK;

    EnableStatus eEnable = (prc != NULL) ? ES_ENABLED : ES_DISABLED;
    DWORD dwScissorStateFALSE = FALSE;

    Assert(prc == NULL ||
           prc->Width > 0 &&
           prc->Height > 0);

    // Set the scissor rect if we are enabling scissor rects
    // for the first time or if we are changing the rectangle

    //
    // Note that we theoretically shouldn't have to call SetScissorRect
    // again when re-enabling scissor rect if the rectangle doesn't
    // change.  However, the NV35 (with driver 6.14.10.4480) resets the scissor
    // rect in this case.  The 9700 doesn't reset the scissor rect.
    //
    // In any case, setting the scissor rect when we go from disabled to
    // enabled certainly doesn't hurt.
    //

    if (eEnable == ES_ENABLED &&
        (m_renderStates.IsStateSet(D3DRS_SCISSORTESTENABLE, dwScissorStateFALSE) ||
          !RtlEqualMemory(prc, &m_rcScissorRect, sizeof(*prc))))
    {
        RECT rcScissor;

        rcScissor.left = prc->X;
        rcScissor.top = prc->Y;
        rcScissor.right = prc->X + prc->Width;
        rcScissor.bottom = prc->Y + prc->Height;

        Assert(rcScissor.left < rcScissor.right &&
               rcScissor.top < rcScissor.bottom);

        MIL_THR(m_pID3DDevice->SetScissorRect(&rcScissor));
        if (SUCCEEDED(hr))
        {
            m_rcScissorRect = *prc;
        }
        else
        {
            IGNORE_HR(SetRenderState(
                D3DRS_SCISSORTESTENABLE,
                FALSE
                ));

            goto Cleanup;
        }
    }

    IFC(SetRenderState(
        D3DRS_SCISSORTESTENABLE,
        eEnable
        ));

Cleanup:
    if (FAILED(hr))
    {
        InvalidateScissorRect();
    }
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::IsDepthStencilSurfaceSmallerThan
//
//  Synopsis:   Checks if we have a depth/stencil buffer set on the d3ddevice
//              that has any dimension smaller than this given.
//
//              Note: returns true if the set state is unknown
//
//-------------------------------------------------------------------------
bool
CHwRenderStateManager::IsDepthStencilSurfaceSmallerThan(
    UINT uWidth,
    UINT uHeight
    ) const
{
    IDirect3DSurface9 *pD3DSurface = NULL;

    bool fIsSmaller;

    if (FAILED(m_depthStencilSurface.GetStateNoAddRef(0, &pD3DSurface)))
    {
        // Actual state is unknown - err on side of safety and return true
        fIsSmaller = true;
    }
    else if (pD3DSurface == NULL)
    {
        // No surface is set and therefore not smaller
        fIsSmaller = false;
    }
    else
    {
        // Check actual size
        fIsSmaller = (   (m_uDepthStencilSurfaceWidth < uWidth)
                      || (m_uDepthStencilSurfaceHeight < uHeight));
    }

    return fIsSmaller;
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::Define2DTransforms
//
//  Synopsis:   Caches the transforms that should be used for 2D rendering.
//              Also sets the 2DTransformsUsed flag to FALSE so we know that
//              we are not using the 2D transforms currently defined.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::Define2DTransforms(
    __in_ecount(1) const CMatrix<CoordinateSpace::DeviceHPC,CoordinateSpace::D3DHomogeneousClipIPC> *pProjection
    )
{
    HRESULT hr = S_OK;

    m_mat2DProjectionTransform = *pProjection;

    m_f2DTransformsUsedForFixedFunction = FALSE;
    m_f2DTransformUsedForVertexShader = FALSE;

    m_u2DTransformVertexShaderStartRegister = 0xffffffff;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::Set2DTransformForFixedFunction
//
//  Synopsis:   Makes sure we are using the transforms defined to be 2D.
//              This will be called eventually from ensurestate in the
//              HardwareSurfaceRenderTarget.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::Set2DTransformForFixedFunction()
{
    HRESULT hr = S_OK;

    if (!m_f2DTransformsUsedForFixedFunction)
    {
        IFC(ForceSetWorldTransform(
            &IdentityMatrix
            ));

        IFC(ForceSetNonWorldTransform(
            D3DTS_VIEW,
            &IdentityMatrix
            ));

        IFC(ForceSetNonWorldTransform(
            D3DTS_PROJECTION,
            &m_mat2DProjectionTransform
            ));

        m_f2DTransformsUsedForFixedFunction = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::Set2DTransformForVertexShader
//
//  Synopsis:   Makes sure the transforms defined for 2D are properly set
//              in the vertex shader at the appropriate register.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::Set2DTransformForVertexShader(
    UINT uStartRegister
    )
{
    HRESULT hr = S_OK;

    if (   !m_f2DTransformUsedForVertexShader
        || uStartRegister != m_u2DTransformVertexShaderStartRegister
           )
    {
        CMILMatrix matWorldToProjection;
        CMILMatrix matShaderTransform;

#if DBG
        CMILMatrix matDbgWorld;
        CMILMatrix matDbgView;

        //
        // We expect the world and view transform to be identity.
        //

        IFC(GetTransform(
            D3DTS_WORLD,
            &matDbgWorld
            ));

        IFC(GetTransform(
            D3DTS_VIEW,
            &matDbgView
            ));

        Assert(matDbgWorld.IsIdentity());
        Assert(matDbgView.IsIdentity());
#endif

        IFC(GetTransform(
            D3DTS_PROJECTION,
            &matWorldToProjection
            ));

        //
        // D3D HLSL interprets transforms differently then fixed function,
        // so we need to transpose the matrix.
        //
        matShaderTransform = matWorldToProjection.transpose();

        IFC(ForceSetVertexShaderConstantF(
            uStartRegister,
            reinterpret_cast<const float *>(&matShaderTransform),
            4
            ));

        m_u2DTransformVertexShaderStartRegister = uStartRegister;
        m_f2DTransformUsedForVertexShader = TRUE;
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:     CHwRenderStateManager::Set3DTransformForVertexShader
//
//  Synopsis:   Makes sure the transforms defined for 3D are properly set
//              in the vertex shader at the appropriate register.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::Set3DTransformForVertexShader(
    UINT uStartRegister
    )
{
    HRESULT hr = S_OK;
    CMILMatrix matResult, matWorldView;

    // NOTE: must be in the same order as the Get3DTransforms shader
    //       pipeline item

    const UINT uNumRegisters = 4;
    UINT uRegisterIndex = uStartRegister;

    // Calculate WorldView
    {
        CMILMatrix matWorld, matView; 
        
        IFC(GetTransform(
            D3DTS_WORLD,
            &matWorld
            ));

        IFC(GetTransform(
            D3DTS_VIEW,
            &matView
            ));    

        matWorldView = matWorld * matView;
    }

    // Send WorldView
    {
        matResult = matWorldView.transpose();

        IFC(SetVertexShaderConstantF(
            uRegisterIndex,
            reinterpret_cast<const float *>(&matResult),
            uNumRegisters
            ));  

        uRegisterIndex += uNumRegisters;
    }

    // Send WorldViewProj
    {
        CMILMatrix matProj;
        
        IFC(GetTransform(
            D3DTS_PROJECTION,
            &matProj
            ));

        matResult = matWorldView.multiply_transpose(matProj);
        
        IFC(SetVertexShaderConstantF(
            uRegisterIndex,
            reinterpret_cast<const float *>(&matResult),
            uNumRegisters
            )); 

        uRegisterIndex += uNumRegisters;
    }

    // Send normal transform matrix.
    // Computed with more description in CMILMesh3D::PrecomputeLighting
    {
        MILMatrixAdjoint(&matResult, &matWorldView);
        matResult *= (matWorldView.GetDeterminant3D() < 0.f) ? -1.f : 1.f;

        // Since we normally transpose before sending a matrix,
        // NOT transposing gives us what we want

        IFC(SetVertexShaderConstantF(
            uRegisterIndex,
            reinterpret_cast<const float *>(&matResult),
            uNumRegisters
            ));
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:    CD3DRenderState::DisableTextureStage
//
//  Synopsis:  Disable a given texture stage.
//
//------------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::DisableTextureStage(
    DWORD dwStage     // The stage to disable. Ranges from 0 up to *and
                      // including* the maximum stage number. (This way, calling
                      // code can always call DisableTextureStage, without
                      // checking if it's actually using all available texture
                      // stages).
    )
{
    HRESULT hr = S_OK;

    Assert(dwStage <= m_nMaxTextureBlendStage);

    if (dwStage < m_nMaxTextureBlendStage)
    {
        IFC(SetTextureStageStateInline(dwStage, D3DTSS_COLOROP, D3DTOP_DISABLE));
    }

Cleanup:
    RRETURN(hr);
}

// error C4995: 'D3DMATRIX': name was marked as #pragma deprecated
//
// Ignore deprecation of D3DMATRIX for this prototype because
// it is defined in the interface this class is implementing
#pragma warning (push)
#pragma warning (disable : 4995)
//+------------------------------------------------------------------------
//
//  Function:   CHwRenderStateManager::SetTransform
//
//  Synopsis:   Sets the transform, branching on whether it is a world
//              transform.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::SetTransform(
    D3DTRANSFORMSTATETYPE state,
    __in_ecount(1) const D3DMATRIX *pMatrix
    )
{
    HRESULT hr = S_OK;

    if (state < 256)
    {
        CBaseMatrix baseMatrix(pMatrix);
        IFC(SetNonWorldTransform(state, &baseMatrix));
    }
    else
    {
        //
        // We only support 1 out of the 256 transforms D3D supports.
        //
        Assert(state == D3DTS_WORLD);

        CBaseMatrix baseMatrix(pMatrix);
        IFC(SetWorldTransform(&baseMatrix));
    }

Cleanup:
    RRETURN(hr);
}
#pragma warning (pop)

//+------------------------------------------------------------------------
//
//  Function:   CHwRenderStateManager::SetViewport
//
//  Synopsis:   Sets the viewport on the device.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::SetViewport(
    __in_ecount(1) const MilPointAndSizeL *prcViewport
    )
{
    HRESULT hr = S_OK;
    D3DVIEWPORT9 vp;

    vp.X      = prcViewport->X;
    vp.Y      = prcViewport->Y;
    vp.Width  = prcViewport->Width;
    vp.Height = prcViewport->Height;
    vp.MinZ   = 0.0f;
    vp.MaxZ   = 1.0f;

    IFC(m_pID3DDevice->SetViewport(
        &vp
        ));

Cleanup:
    m_rcViewport = *prcViewport;

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CHwRenderStateManager::ForceSetStreamSource
//
//  Synopsis:   Sets the stream on the device.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::ForceSetStreamSource(
    __in_ecount_opt(1) IDirect3DVertexBuffer9 *pStream,
    UINT uVertexStride
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetStreamSource(
        0,
        pStream,
        0,
        uVertexStride
        ));

Cleanup:
    m_streamSourceVertexBuffer.UpdateState(
        hr,
        0,
        pStream
        );

    m_streamSourceVertexStride.UpdateState(
        hr,
        0,
        uVertexStride
        );

    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CHwRenderStateManager::ForceSetIndices
//
//  Synopsis:   Sets the index stream on the device.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::ForceSetIndices(
    __in_ecount_opt(1) IDirect3DIndexBuffer9 *pStream
    )
{
    HRESULT hr = S_OK;

    IFC(m_pID3DDevice->SetIndices(
        pStream
        ));

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:   CHwRenderStateManager::SetDefaultState
//
//  Synopsis:   Sets the default values for all the state on the D3D Device.
//
//-------------------------------------------------------------------------
HRESULT
CHwRenderStateManager::SetDefaultState(
    BOOL fCanHandleBlendFactor,
    BOOL fSupportsScissorRect,
    __range(1,UINT_MAX) DWORD dwMaxStream,
    __range(1,UINT_MAX) DWORD dwAnisotropicFilterLevel
    )
{
    HRESULT hr = S_OK;
    DWORD dwStage;

    // Set blend factor into any known state
    //    m_rgdwD3DRenderStates[D3DRS_BLENDFACTOR] = 1; // to appease assertion

    if (fCanHandleBlendFactor)
    {
        IFC(ForceSetRenderState(D3DRS_BLENDFACTOR,0));
    }

    //
    // RENDER STATES
    //

    // Keep these in enumeration order. (This makes it easier to check if we've
    // missed something).

    IFC(ForceSetRenderState(D3DRS_ZENABLE, D3DZB_FALSE));
    IFC(ForceSetRenderState(D3DRS_ZWRITEENABLE, FALSE));

    if (IsTagEnabled(tagWireframe))
    {
        IFC(ForceSetRenderState(D3DRS_FILLMODE, D3DFILL_WIREFRAME));
    }
    else
    {
        IFC(ForceSetRenderState(D3DRS_FILLMODE, D3DFILL_SOLID));
    }


    IFC(ForceSetRenderState(D3DRS_SHADEMODE, D3DSHADE_GOURAUD));

    // D3D default: TRUE

    IFC(ForceSetRenderState(D3DRS_ALPHATESTENABLE, FALSE));

    IFC(ForceSetRenderState(D3DRS_LASTPIXEL, FALSE));

    IFC(ForceSetRenderState(D3DRS_ANTIALIASEDLINEENABLE, FALSE));

    // Managed by SetAlphaBlendMode: D3DRS_SRCBLEND, D3DRS_DESTBLEND

    // D3D default: D3DCULL_CCW
    //   We need to disable culling, because e.g. scaling by a negative
    //   number is a valid GDI+ transform, and this would invert the
    //   vertex order.

    IFC(ForceSetRenderState(D3DRS_CULLMODE, D3DCULL_NONE));

    IFC(ForceSetRenderState(D3DRS_ZFUNC, D3DCMP_LESSEQUAL));

    // Ignored: D3DRS_ALPHAREF, D3DRS_ALPHAFUNC (D3DRS_ALPHATESTENABLE
    // defaults to FALSE).

    //   Enable dithering?
    //  We don't enable dithering yet.

    IFC(ForceSetRenderState(D3DRS_DITHERENABLE, FALSE));

    // Managed by SetAlphaBlendMode: D3DRS_ALPHABLENDENABLE

    IFC(ForceSetRenderState(D3DRS_FOGENABLE, FALSE));

    // Ignored: D3DRS_ZVISIBLE.
    // Reason: "Not supported" (MSDN).

    // Ignored: D3DRS_FOGCOLOR, D3DRS_FOGTABLEMODE, D3DRS_FOGSTART,
    //   D3DRS_FOGEND, D3DRS_FOGDENSITY.
    // Reason: D3DRS_FOGENABLE defaults to FALSE.

    IFC(ForceSetRenderState(D3DRS_DEPTHBIAS, 0));

    // Ignored: D3DRS_RANGEFOGENABLE
    // Reason: D3DRS_FOGENABLE defaults to FALSE.

    IFC(ForceSetRenderState(D3DRS_STENCILENABLE, FALSE));

    IFC(ForceSetRenderState(D3DRS_STENCILREF, 0));

    // RenderState: D3DRS_STENCILFUNC
    // D3D Default: D3DCMP_ALWAYS
    // Reason: When we use the stencil buffer for clipping our reference
    //         value is 0, and we fill the stencil with 1 wherever we want
    //         to render.  So we use NOTEQUAL as our comparison function
    //         since we want to render when 0 != 1.

    IFC(ForceSetRenderState(D3DRS_STENCILFUNC,  D3DCMP_NOTEQUAL));
    IFC(ForceSetRenderState(D3DRS_STENCILFAIL,  D3DSTENCILOP_KEEP));
    IFC(ForceSetRenderState(D3DRS_STENCILZFAIL, D3DSTENCILOP_KEEP));
    IFC(ForceSetRenderState(D3DRS_STENCILPASS,  D3DSTENCILOP_KEEP));
    IFC(ForceSetRenderState(D3DRS_STENCILMASK,  0xffffffff));

    // RenderState: D3DRS_STENCILWRITEMASK
    // D3D Default: 0xffffffff
    // Reason: We currently use the software rasterizer to output spans
    //         for clipping using the stencil buffer.  We never use HW to
    //         write to the stencil buffer.

    IFC(ForceSetRenderState(D3DRS_STENCILWRITEMASK, 0x0));

    IFC(ForceSetRenderState(D3DRS_TWOSIDEDSTENCILMODE, FALSE));

    // Ignored: D3DRS_CCW_STENCILFAIL, D3DRS_CCW_STENCILZFAIL,
    //          D3DRS_CCW_STENCILPASS, D3DRS_CCW_STENCILFUNC
    // Reason: D3DRS_TWOSIDEDSTENCILMODE is FALSE

    // Ignored: D3DRS_TEXTUREFACTOR
    // Reason: Our pipelines don't use a texture factor.

    // D3DRS_WRAP0: Dunno. Default, volatile, or ignorable?

    IFC(ForceSetRenderState(D3DRS_CLIPPING, TRUE));

    // Changed: 2/14/2003 chrisra from FALSE to member variable
    // Reason:  Now that we have lighting of 3D objects we need to always
    //          remember what the lighting value is set to in D3D.
    //
    // Note (jordanpa): We do all of our own lighting now so these need to
    //                  be off

    IFC(ForceSetRenderState(D3DRS_LIGHTING, FALSE));
    IFC(ForceSetRenderState(D3DRS_SPECULARENABLE, FALSE));

    // Ignored: D3DRS_FOGVERTEXMODE.
    // Reason: D3DRS_FOGENABLE defaults to FALSE.

    IFC(ForceSetRenderState(D3DRS_COLORVERTEX, TRUE));

    // Ignored: D3DRS_LOCALVIEWER.
    // Reason: We do our own specular lighting

    // D3DRS_NORMALIZENORMALS
    // Changed: 2/14/2003 jordanpa from TRUE to FALSE
    // Reason:  We aren't doing lighting in HW any more but perhaps this
    //          may be a perf boost in some drivers?

    IFC(ForceSetRenderState(D3DRS_NORMALIZENORMALS, FALSE));

    IFC(ForceSetRenderState(D3DRS_DIFFUSEMATERIALSOURCE, D3DMCS_COLOR1));

    IFC(ForceSetRenderState(D3DRS_SPECULARMATERIALSOURCE, D3DMCS_COLOR1));

    // D3DRS_AMBIENTMATERIALSOURCE
    // Changed: 2003/05/14 chrisra From IGNORED to D3DMCS_MATERIAL
    // Reason: Now that we are using ambient lighting for 3D, we explicitly set it to retrieve
    //         from the ambient light source from the material.

    IFC(ForceSetRenderState(D3DRS_AMBIENTMATERIALSOURCE, D3DMCS_MATERIAL));

    IFC(ForceSetDepthStencilSurface(
        NULL,
        0,
        0
        ));

    // D3DRS_AMBIENT
    // Changed: 2003/05/14 chrisra From IGNORED to 0x0
    // Reason: Now that we are using ambient lighting for 3D, we explicitly set the global
    //         ambient light to 0 because we want to only pull ambient light from the
    //         lights in the scene.

    IFC(ForceSetRenderState(D3DRS_AMBIENT, 0x0));
    IFC(ForceSetRenderState(D3DRS_VERTEXBLEND, D3DVBF_DISABLE));
    IFC(ForceSetRenderState(D3DRS_CLIPPLANEENABLE, 0));

    // Ignored: D3DRS_SOFTWAREVERTEXPROCESSING
    //
    //   Enforce our software vertex processing state.
    //  There's a remote possibility that the caller would change it during
    //  interop, and we'd care about the setting (e.g. bad hardware,
    //  bad driver, or precision differences).

    // Ignored: D3DRS_POINTSIZE, D3DRS_POINTSIZE_MIN, D3DRS_POINTSPRITEENABLE,
    //   D3DRS_POINTSCALEENABLE, D3DRS_POINTSCALE_A, D3DRS_POINTSCALE_B,
    //   D3DRS_POINTSCALE_C.
    // Reason: We don't draw point primitives (D3DPT_POINTLIST).

    IFC(ForceSetRenderState(D3DRS_MULTISAMPLEANTIALIAS, TRUE));

    IFC(ForceSetRenderState(D3DRS_MULTISAMPLEMASK, 0xffffffff));

    // Ignored: D3DRS_PATCHEDGESTYLE, D3DRS_PATCHSEGMENTS.
    // Reason: We don't use patches.

    // Ignored: D3DRS_DEBUGMONITORTOKEN.
    // Reason: Doesn't seem to affect us.

    // Ignored: D3DRS_POINTSIZE_MAX.
    // Reason: We don't draw point primitives.

    // Ignored: D3DRS_INDEXEDVERTEXBLENDENABLE.
    // Reason: D3DRS_VERTEXBLEND defaults to D3DVBF_DISABLED. We think that
    //   means this state has no effect, but MSDN isn't very clear.

    IFC(ForceSetRenderState(D3DRS_COLORWRITEENABLE, 0x0000000f));

    // Ignored: D3DRS_TWEENFACTOR.
    // Reason: We don't use tweening.

    IFC(ForceSetRenderState(D3DRS_BLENDOP, D3DBLENDOP_ADD));

    // Ignored: D3DRS_POSITIONDEGREE, D3DRS_NORMALDEGREE.
    // Reason: We don't use patches.

    //   Handle the new D3D9 render states
    //  There are a bunch of new D3D9 state - D3DRS_SCISSORTESTENABLE (174)
    //  through D3DRS_COLORWRITEENABLE3 (192) as of this writing. Find out
    //  more about them, and classify them as default/volatile/ignored.

    // D3D default: FALSE
    // Scissor rect clipping should be turned off until we get a rectangular clip

    if (fSupportsScissorRect)
    {
        IFC(ForceSetRenderState(D3DRS_SCISSORTESTENABLE, FALSE));
    }

    for (dwStage=0; dwStage<MIL_SAMPLER_COUNT; dwStage++)
    {
        IFC(ForceSetTextureStageState(dwStage, D3DTSS_COLOROP, D3DTOP_DISABLE));
    }



    // If there are any stages above MIL_MAX_SAMPLER, they must default to
    // disabled. We rely on this for stage MIL_MAX_SAMPLER+1 (and hence don't
    // really care about the rest).

    for (dwStage=MIL_SAMPLER_COUNT+1; dwStage< m_nMaxTextureBlendStage; dwStage++)
    {
        IFC(ForceSetTextureStageState(
            dwStage,
            D3DTSS_COLOROP,
            D3DTOP_DISABLE
            ));
    }

    //
    // We set the max anisotropic level on all the samplers.  This shouldn't
    // affect our performance when we're not using anisotropic filtering. Since
    // we're only using one level of quality right now, once we set it here we
    // don't have to worry about it again.
    //

    for (dwStage = 0; dwStage < m_nMaxTextureBlendStage; dwStage++)
    {
        IFC(ForceSetSamplerState(
            dwStage,
            D3DSAMP_MAXANISOTROPY,
            dwAnisotropicFilterLevel
            ));
    }

    // TEXTURE STAGE STATES

    // Managed by SetTextureStageOperation: D3DTSS_COLOROP, D3DTSS_COLORARG1,
    // D3DTSS_COLORARG2, D3DTSS_ALPHAOP, D3DTSS_ALPHAARG1, D3DTSS_ALPHAARG2

    // Ignored: D3DTSS_BUMPENVMAT00, D3DTSS_BUMPENVMAT01, D3DTSS_BUMPENVMAT10,
    //   D3DTSS_BUMPENVMAT11.
    // Reason: We don't use bump mapping.

    for (dwStage=0; dwStage< m_nMaxTextureBlendStage; dwStage++)
    {
        IFC(ForceSetTextureStageState(
            dwStage,
            D3DTSS_TEXCOORDINDEX,
            dwStage
            ));
    }

    // Ignored: D3DTSS_BUMPENVLSCALE, D3DTSS_BUMPENVLOFFSET.
    // Reason: We don't use bump mapping.

    // D3D default: D3DTTFF_DISABLE

    for (dwStage=0; dwStage< m_nMaxTextureBlendStage; dwStage++)
    {
        IFC(ForceSetTextureStageState(
            dwStage,
            D3DTSS_TEXTURETRANSFORMFLAGS,
            D3DTTFF_DISABLE
            ));
    }


    //
    // Initialize the transforms to identity
    //
    {
        IFC(ForceSetWorldTransform(
            &IdentityMatrix
            ));

        IFC(ForceSetNonWorldTransform(
            D3DTS_VIEW,
            &IdentityMatrix
            ));

        IFC(ForceSetNonWorldTransform(
            D3DTS_PROJECTION,
            &IdentityMatrix
            ));
    }

    //
    // Because we use ForceSetXXXTransform instead of SetXXXTransform
    // the render state manager must invalidate the 2D transforms with this
    // switch. This method is called by CD3DRenderState::ResetState
    // so we get here after our state has valid values.
    //
    m_f2DTransformsUsedForFixedFunction = FALSE;
    m_f2DTransformUsedForVertexShader = FALSE;

    //
    // Material
    //
    {
        D3DMATERIAL9 material;

        material.Diffuse.r = 1.0f;
        material.Diffuse.g = 1.0f;
        material.Diffuse.b = 1.0f;
        material.Diffuse.a = 1.0f;

        material.Specular.r = 0.0f;
        material.Specular.g = 0.0f;
        material.Specular.b = 0.0f;
        material.Specular.a = 0.0f;

        material.Ambient.r = 0.0f;
        material.Ambient.g = 0.0f;
        material.Ambient.b = 0.0f;
        material.Ambient.a = 0.0f;

        material.Emissive.r = 0.0f;
        material.Emissive.g = 0.0f;
        material.Emissive.b = 0.0f;
        material.Emissive.a = 0.0f;

        //
        // I don't think this statement is correct anymore.
        //
        // Material power is currently the only component in the material that
        // should change rendering.
        //
        material.Power = 40.0f;

        IFC(m_pID3DDevice->SetMaterial(&material));
    }

    // Textures

    // The textures are volatile - but we need a better solution, so that we
    // won't hold references to unused textures for longer than needed.
    // (Workitem #1743).
    //
    // (MSDN also claims a 'resource leak' would be possible, but we can't see
    // how. Maybe it just means the above. Either way, this would avoid it.)

    for (dwStage = 0; dwStage < m_nMaxTextureBlendStage; dwStage++)
    {
        IFC(ForceSetTexture(
            dwStage,
            NULL
            ));
    }

    //
    // Pixel Shader
    //
    IFC(ForceSetPixelShader(NULL));

    //
    // Streams
    //

    IFC(ForceSetStreamSource(
        0,
        NULL
        ));

    for (UINT uStreamNum = 1; uStreamNum < dwMaxStream; uStreamNum++)
    {
        IFC(m_pID3DDevice->SetStreamSource(
            uStreamNum,
            NULL,
            0,
            0
            ));
    }

    IFC(ForceSetIndices(NULL));

    //
    // Scissor Rect
    //
    ZeroMemory(&m_rcScissorRect, sizeof(m_rcScissorRect));

    m_fClipSet = FALSE;

Cleanup:
    RRETURN(hr);
}

#if DBG
//+------------------------------------------------------------------------
//
//  Function:   CHwRenderStateManager::SetSupportedTable
//
//  Synopsis:   Sets the which states are supported for changing.
//
//-------------------------------------------------------------------------
void
CHwRenderStateManager::SetSupportedTable()
{
    /* RENDER STATES */

    //
    // Material States
    //

    m_renderStates.SetSupported(D3DRS_DIFFUSEMATERIALSOURCE);
    m_renderStates.SetSupported(D3DRS_SPECULARMATERIALSOURCE);

    //
    // Fillmode - not sure why this gets called in non-debug
    //

    m_renderStates.SetSupported(D3DRS_FILLMODE);

    //
    // Blend Mode States
    //

    m_renderStates.SetSupported(D3DRS_ALPHABLENDENABLE);
    m_renderStates.SetSupported(D3DRS_SRCBLEND);
    m_renderStates.SetSupported(D3DRS_DESTBLEND);

    //
    // Text
    //

    m_renderStates.SetSupported(D3DRS_BLENDFACTOR);
    m_renderStates.SetSupported(D3DRS_COLORWRITEENABLE);

    //
    // Misc
    //

    m_renderStates.SetSupported(D3DRS_SCISSORTESTENABLE);
    m_depthStencilSurface.SetSupported(0);

    //
    // EnsureState
    //

    m_renderStates.SetSupported(D3DRS_ZENABLE);
    m_renderStates.SetSupported(D3DRS_STENCILENABLE);
    m_renderStates.SetSupported(D3DRS_ZWRITEENABLE);
    m_renderStates.SetSupported(D3DRS_CULLMODE);
    m_renderStates.SetSupported(D3DRS_ZFUNC);
    m_renderStates.SetSupported(D3DRS_MULTISAMPLEANTIALIAS);


    /* TEXTURE STAGE STATES */

    //
    // Texture Stage Operation
    //

    for (UINT uTextureStage = 0;
              uTextureStage < MIL_TEXTURE_STAGE_COUNT;
              uTextureStage++)
    {
        m_textureStageStates.SetSupported(
            CalcTextureStageStatePos(uTextureStage,D3DTSS_COLOROP));

        m_textureStageStates.SetSupported(
            CalcTextureStageStatePos(uTextureStage,D3DTSS_COLORARG1));

        m_textureStageStates.SetSupported(
            CalcTextureStageStatePos(uTextureStage,D3DTSS_COLORARG2));

        m_textureStageStates.SetSupported(
            CalcTextureStageStatePos(uTextureStage,D3DTSS_ALPHAOP));

        m_textureStageStates.SetSupported(
            CalcTextureStageStatePos(uTextureStage,D3DTSS_ALPHAARG1));

        m_textureStageStates.SetSupported(
            CalcTextureStageStatePos(uTextureStage,D3DTSS_ALPHAARG2));

        m_textureStageStates.SetSupported(
            CalcTextureStageStatePos(uTextureStage,D3DTSS_TEXTURETRANSFORMFLAGS));

        m_textureStageStates.SetSupported(
            CalcTextureStageStatePos(uTextureStage,D3DTSS_TEXCOORDINDEX));
    }

    /* TEXTURE SAMPLER STATES */

    //
    // FilterMode
    //

    for (UINT uSamplerState = 0;
              uSamplerState < MIL_SAMPLER_COUNT;
              uSamplerState++)
    {
        m_samplerStageStates.SetSupported(
            CalcSamplerStatePos(uSamplerState,D3DSAMP_MAGFILTER));

        m_samplerStageStates.SetSupported(
            CalcSamplerStatePos(uSamplerState,D3DSAMP_MINFILTER));

        m_samplerStageStates.SetSupported(
            CalcSamplerStatePos(uSamplerState,D3DSAMP_MIPFILTER));

        /* NEW SETTINGS */

        m_samplerStageStates.SetSupported(
            CalcSamplerStatePos(uSamplerState,D3DSAMP_ADDRESSU));

        m_samplerStageStates.SetSupported(
            CalcSamplerStatePos(uSamplerState,D3DSAMP_ADDRESSV));

        m_samplerStageStates.SetSupported(
            CalcSamplerStatePos(uSamplerState,D3DSAMP_BORDERCOLOR));
    }

    /* TEXTURES */

    for (UINT uTextureStage = 0; uTextureStage < MIL_TEXTURE_STAGE_COUNT; uTextureStage++)
    {
        m_textures.SetSupported(uTextureStage);
    }

    /* TRANSFORM STATES */

    m_worldTransform.SetSupported(0);

    m_nonWorldTransforms.SetSupported(D3DTS_VIEW);
    m_nonWorldTransforms.SetSupported(D3DTS_PROJECTION);

    m_nonWorldTransforms.SetSupported(D3DTS_TEXTURE0);
    m_nonWorldTransforms.SetSupported(D3DTS_TEXTURE1);
    m_nonWorldTransforms.SetSupported(D3DTS_TEXTURE2);
    m_nonWorldTransforms.SetSupported(D3DTS_TEXTURE3);
    m_nonWorldTransforms.SetSupported(D3DTS_TEXTURE4);
    m_nonWorldTransforms.SetSupported(D3DTS_TEXTURE5);
    m_nonWorldTransforms.SetSupported(D3DTS_TEXTURE6);
    m_nonWorldTransforms.SetSupported(D3DTS_TEXTURE7);

    //
    // FVF
    //
    m_stateFVF.SetSupported(0);

    //
    // Shaders
    //
    m_stateVertexShader.SetSupported(0);
    m_statePixelShader.SetSupported(0);

    for (UINT i = 0; i < 256; ++i)
    {
        m_stateVertexShaderFloat4Constants.SetSupported(i);
    }
    
    for (UINT i = 0; i < 224; ++i)
    {
        m_statePixelShaderFloat4Constants.SetSupported(i);
    }
    
    for (UINT i = 0; i < 16; ++i)
    {
        m_statePixelShaderInt4Constants.SetSupported(i);
        m_statePixelShaderBoolConstants.SetSupported(i);
    }
    
    //
    // Streams
    //

    m_indexStream.SetSupported(0);
    m_streamSourceVertexBuffer.SetSupported(0);
    m_streamSourceVertexStride.SetSupported(0);
}
#endif






