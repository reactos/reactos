// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//-----------------------------------------------------------------------------


#include "precomp.hpp"
using namespace dxlayer;

extern HINSTANCE g_DllInstance;


MtDefine(EffectResource, MILRender, "Effect Resource");
MtDefine(CMilEffectDuce, EffectResource, "CMilEffectDuce");

//+----------------------------------------------------------------------------
//
// CMilEffectDuce::GetLocalSpaceClipBounds
//
// Synopsis: 
//    Called by the composition layer to allow effects to be applied
//    to clipped areas.  By default, the effect will not be clipped
//    to prevent visual artifacts, since an effect can potentially sample
//    from any pixel in a texture, so the whole texture may need to be drawn.
//    If the effect can override this method it should to reduce texture
//    size and eliminate overdraw when drawing outside visible regions.
//
//-----------------------------------------------------------------------------

HRESULT 
CMilEffectDuce::GetLocalSpaceClipBounds(
        __in CRectF<CoordinateSpace::LocalRendering> unclippedBoundsLocalSpace,
        __in CRectF<CoordinateSpace::PageInPixels> clip,
        __in const CMatrix<CoordinateSpace::LocalRendering,CoordinateSpace::PageInPixels> *pWorldTransform,
        __out CRectF<CoordinateSpace::LocalRendering> *pClippedBoundsLocalSpace)
{
    HRESULT hr = S_OK;

    // The default implementation returns the unclipped bounds.
    // A custom effect can create dependencies between any pixels in an image,
    // so by default we must render the entire texture to ensure the visual
    // effect is correct.
    *pClippedBoundsLocalSpace = unclippedBoundsLocalSpace;

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilEffectDuce::GetShaderRenderMode
//
// Synopsis: 
//    Called by the composition layer to determine whether an effect is being
//    forced to run in software or hardware, or is being run with default
//    settings (hardware with automatic software fallback).
//
//-----------------------------------------------------------------------------

ShaderEffectShaderRenderMode::Enum
CMilEffectDuce::GetShaderRenderMode()
{
    return ShaderEffectShaderRenderMode::Auto;
}


//+----------------------------------------------------------------------------
//
// CMilEffectDuce::CreateIntermediateRT
//
// Synopsis: 
//      Creates a temporary texture to render into and read from for
//      intermediate stages of an effect pipeline.
//      Ported from CHwTextureRenderTarget::Init and ::GetSurfaceDescriptor
//
//-----------------------------------------------------------------------------
HRESULT
CMilEffectDuce::CreateIntermediateRT(
    __in CD3DDeviceLevel1 *pD3DDevice, 
    __in UINT uWidth, 
    __in UINT uHeight, 
    __in D3DFORMAT d3dfmtTarget,
    __out CD3DVidMemOnlyTexture **ppVidMemOnlyTexture)
{  
    HRESULT hr = S_OK;

    D3DSURFACE_DESC sdLevel0;
    sdLevel0.Format = d3dfmtTarget;
    sdLevel0.Type = D3DRTYPE_TEXTURE;
    sdLevel0.Usage = D3DUSAGE_RENDERTARGET;
    // We have to use default pool since we don't have drivers
    // that supported DDI management features needed for MANAGED
    // render targets.
    sdLevel0.Pool = D3DPOOL_DEFAULT;
    sdLevel0.MultiSampleType = D3DMULTISAMPLE_NONE;
    sdLevel0.MultiSampleQuality = 0;
    sdLevel0.Width = uWidth;
    sdLevel0.Height = uHeight;

    //
    // Get the required texture characteristics
    //

    IFC(pD3DDevice->GetMinimalTextureDesc(
        &sdLevel0,
        TRUE,
        GMTD_NONPOW2CONDITIONAL_OK | GMTD_IGNORE_FORMAT
        ));

    //
    // Check if dimensions were too big
    //
    if (hr == S_FALSE)
    {
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }

    //
    // Check if we changed size
    //
    if (sdLevel0.Width != uWidth || sdLevel0.Height != uHeight)
    {
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }

    //
    // Create the texture
    //

    IFC(CD3DVidMemOnlyTexture::Create(
        &sdLevel0,          // pSurfDesc
        1,                              // uLevels
        false,                  // fIsEvictable
        pD3DDevice,
        ppVidMemOnlyTexture,
        NULL                    // pSharedHandle
        ));
    
Cleanup:

    if (FAILED(hr))
    {
        ReleaseInterface(*ppVidMemOnlyTexture);
    }

    RRETURN(hr);

}

//-----------------------------------------------------------------------------
//
// CMilEffectDuce::PopulateVertexBuffer
//
// Synopsis:
//    Populates the vertex buffer for drawing into the final destination. 
//
//-----------------------------------------------------------------------------

HRESULT 
CMilEffectDuce::SetupVertexTransform(
    __in const CContextState *pContextState, 
    __in CD3DDeviceLevel1 *pDevice, 
    float destinationWidth, 
    float destinationHeight,
    bool passToFinalDestination
    )
{
    HRESULT hr = S_OK;
    
    CMILMatrix matVertexTransform; // don't initialize

    if (passToFinalDestination)
    {
        //
        // Scaling our (0,0) to (1,1) quad by the size of the texture, the world transform, and the projection.

        // Create a matrix to scale our unit quad to the size of our texture.        
        CMILMatrix matScaleForTextureSize(
                destinationWidth,                 0,    0,    0,
                               0, destinationHeight,    0,    0,
                               0,                 0, 1.0f,    0,
                               0,                 0,    0, 1.0f);

        // Set our vertex transform to scale * world transform.
        matVertexTransform.SetToMultiplyResult(matScaleForTextureSize, pContextState->WorldToDevice);

        // Get the projection matrix saved in the device state.
        CMILMatrix matWorldToProjection;
        IFC(pDevice->GetTransform(
            D3DTS_PROJECTION,
            &matWorldToProjection
            ));

        // Apply transforms in this order since D3D does column multiplication (v * T1 * T2)
        // We apply scale first, then world, and projection last.
        matVertexTransform.Multiply(matWorldToProjection);        
    }
    else
    {
        // Scaling and translating our (0,0) to (1,1) quad to a 
        // (-1 - halfPixelWidth, -1 - halfPixelHeight) to (1 + halfPixelWidth, 1 + halfPixelHeight) quad.
        // A pixel dimension in our scaled quad is 2 / width or height.
        // A half pixel dimension is (2 / dimension) / 2 = 1 / dimension;
        float halfPixelWidth = 1.0f / destinationWidth;
        float halfPixelHeight = 1.0f / destinationHeight;

        // Create a matrix to scale our unit quad up to twice its size and flip its coordinate space over the
        // y-axis (since the vertex in the buffer is oriented in the screen coord space directions, not the default DX one).  
        // We also translate to center the new quad, and to account for the half pixel offset for DX texture sampling.
        matVertexTransform = CMILMatrix(
                           2.0f,                       0,    0,    0,
                              0,                   -2.0f,    0,    0,
                              0,                       0, 1.0f,    0,
            -1 - halfPixelWidth,     1 + halfPixelHeight,    0, 1.0f);
                             //   -(-1 - halfPixelHeight)
    }

    //
    // D3D HLSL interprets transforms differently than we store them elsewhere in the pipeline,
    // so we need to transpose the matrix.
    //
    matVertexTransform = matVertexTransform.transpose();

    // Send our transform to the vertex shader.
    IFC(pDevice->SetVertexShaderConstantF(0, reinterpret_cast<const float *>(&matVertexTransform), 4));

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilEffectDuce::SetSamplerState
//
// Synopsis: 
//    Sets the device sampler state (address mode and sampling mode) for the
//    specified sampler register used by this effect.
//
//-----------------------------------------------------------------------------

HRESULT
CMilEffectDuce::SetSamplerState(
    __in CD3DDeviceLevel1 *pDevice,
    UINT uSamplerRegister,
    bool setAddressMode,
    bool useBilinear
    )
{
    HRESULT hr = S_OK;
    
    // Set the address mode to clamp for effects.
    if (setAddressMode)
    {
        IFC(pDevice->SetSamplerState(
            uSamplerRegister, 
            D3DSAMP_ADDRESSU, 
            D3DTADDRESS_CLAMP));

        IFC(pDevice->SetSamplerState(
            uSamplerRegister, 
            D3DSAMP_ADDRESSV, 
            D3DTADDRESS_CLAMP));
    }

    // Set the sampling mode to bilinear or nearest neighbor.
    if (useBilinear)
    {
        IFC(pDevice->SetSamplerState(
            uSamplerRegister, 
            D3DSAMP_MINFILTER, 
            D3DTEXF_LINEAR));

        IFC(pDevice->SetSamplerState(
            uSamplerRegister, 
            D3DSAMP_MAGFILTER,
            D3DTEXF_LINEAR));
    }
    else
    {
        IFC(pDevice->SetSamplerState(
            uSamplerRegister, 
            D3DSAMP_MINFILTER, 
            D3DTEXF_POINT));

        IFC(pDevice->SetSamplerState(
            uSamplerRegister, 
            D3DSAMP_MAGFILTER,
            D3DTEXF_POINT));
    }
    
Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
// CMilEffectDuce::LockResource <static>
//
// Synopsis:
//     This function is used by derived classes to load precompiled shader
//     resources from our DLL.
//
// Remark:
//     The pointer returned by LockResource is valid until the module 
//     containing the resource is unloaded. It is not necessary to 
//     unlock resources because the system automatically deletes them when 
//     the process that created them terminates.
//-----------------------------------------------------------------------------

HRESULT
CMilEffectDuce::LockResource(
    __in UINT resourceId, 
    __deref_out_bcount(*pSizeInBytes) BYTE **ppResource,
    __out UINT *pSizeInBytes
    )
{
    HRESULT hr = S_OK;
    HGLOBAL hResource = NULL;
    HRSRC hResourceInfo = NULL;

    Assert(*ppResource == NULL);

    IFCW32(hResourceInfo = FindResource(
        g_DllInstance,
        MAKEINTRESOURCE(resourceId),
        RT_RCDATA));

    IFCW32(hResource = LoadResource(g_DllInstance, hResourceInfo));
    IFCW32(*pSizeInBytes = SizeofResource(g_DllInstance, hResourceInfo));

    // This method is nothing more than a cast, so we don't have to worry about error checking here
    *ppResource = reinterpret_cast<BYTE *>(::LockResource(hResource));

    if (*ppResource == NULL)
    {
        IFC(E_FAIL);
    }

Cleanup:
    return hr;
}


