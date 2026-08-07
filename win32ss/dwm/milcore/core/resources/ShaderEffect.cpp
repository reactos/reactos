// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      ShaderEffect resource.
//
//-----------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(ShaderEffectResource, MILRender, "ShaderEffect Resource");
MtDefine(CMilShaderEffectDuce, ShaderEffectResource, "CMilShaderEffectDuce");

unsigned g_uBlank = 0x00000000;

WarpPlatform::LockHandle g_LockJitterAccess;

//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::~CMilShaderEffectDuce
//
//-----------------------------------------------------------------------------

CMilShaderEffectDuce::~CMilShaderEffectDuce()
{
    ReleaseInterface(m_pSwShaderEffectBrush);

    FreeSamplerData();

    UnRegisterNotifiers();
}

//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::FreeSamplerData
//
// Synopsis: Release the sampler data.
//
//-----------------------------------------------------------------------------

void
CMilShaderEffectDuce::FreeSamplerData()
{
    // Go through any samplers currently registered, and release them.  Note
    // that we treat the INT array as holding CMilBrush's
    for (UINT32 i = 0; i < m_samplerDataCount; i++)
    {
        SamplerData* pCurrentSamplerData = &(m_pSamplerData[i]);
        CMilBrushDuce *pBrushNoRef = pCurrentSamplerData->GetBrushNoRef();
        UnRegisterNotifier(pBrushNoRef);
        pCurrentSamplerData->Free();
    }    
    WPFFree(ProcessHeap, m_pSamplerData);

    m_pSamplerData = NULL;
    m_samplerDataCount = 0;   
}

//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::ProcessUpdate
//
// Synopsis: 
//      Wraps the codegen'd GeneratedProcessUpdate() and does some additional
//      decoding afterwards.
//
//-----------------------------------------------------------------------------

HRESULT
CMilShaderEffectDuce::ProcessUpdate(
    __in_ecount(1) CMilSlaveHandleTable* pHandleTable,
    __in_ecount(1) const MILCMD_SHADEREFFECT* pCmd,
    __in_bcount(cbPayload) LPCVOID pPayload,
    UINT cbPayload)
{
    HRESULT hr = S_OK;

    FreeSamplerData();

    // Do the main update processing
    IFC(GeneratedProcessUpdate(pHandleTable, pCmd, pPayload, cbPayload));

    // Get the sampler data count and cap at MAX_TEXTURE_STAGE_CONFIGURATIONS.
    m_samplerDataCount = 
        min((UINT32)(m_data.m_cbDependencyPropertySamplerValuesSize / sizeof(UINT32)), 
            (UINT32)SHADEREFFECT_MAX_TEXTURE_STAGE_CONFIGURATIONS);

    // Verify that we have enough info entries.
    UINT32 numInfos = 
        min((UINT32)(m_data.m_cbShaderSamplerRegistrationInfoSize / (2 * sizeof(UINT32))),
            (UINT32)SHADEREFFECT_MAX_TEXTURE_STAGE_CONFIGURATIONS);
    if (numInfos != m_samplerDataCount)
    {
        RIP("Invalid handle.");
        IFC(WGXERR_UCE_MALFORMEDPACKET);
    }    

    // Organize our input data in a more structured form.
    if (m_samplerDataCount > 0)
    {
        const int *info = m_data.m_pShaderSamplerRegistrationInfoData;

        UINT sampleDataSizeInBytes = m_samplerDataCount * sizeof(SamplerData);
        m_pSamplerData = (SamplerData*)WPFAlloc(ProcessHeap, Mt(CMilShaderEffectDuce), sampleDataSizeInBytes);
        IFCOOM(m_pSamplerData);
        ZeroMemory(m_pSamplerData, sampleDataSizeInBytes);

        for (UINT32 i = 0; i < m_samplerDataCount; i++)
        {
            DWORD samplerRegister = 0;
            DWORD samplingMode = 0;


            IFC(IntToDWord(*info, &samplerRegister));
            info++;
            
            IFC(IntToDWord(*info, &samplingMode));
            info++;        
            
            HMIL_RESOURCE handle = *(reinterpret_cast<const HMIL_RESOURCE*>(&(m_data.m_pDependencyPropertySamplerValuesData[i])));

            CMilBrushDuce *pBrush = static_cast<CMilBrushDuce*>(pHandleTable->GetResource(handle, TYPE_BRUSH));
            Assert(pBrush);

            IFC(RegisterNotifier(pBrush));

            m_pSamplerData[i].Init(samplerRegister, samplingMode, pBrush);
        }
    }

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::GetShaderRenderMode
//
// Synopsis: 
//    Called by the composition layer to determine whether an effect is being
//    forced to run in software or hardware, or is being run with default
//    settings (hardware with automatic software fallback).  For shader effects
//    this can be set on the custom pixel shader directly in managed code.
//
//-----------------------------------------------------------------------------

ShaderEffectShaderRenderMode::Enum
CMilShaderEffectDuce::GetShaderRenderMode()
{
    return m_data.m_pPixelShader->GetShaderRenderMode();
}

//+----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::TransformBoundsForInflation
//
// Synopsis: 
//    Called by the composition layer to account for potential bounds
//    transformations by  effects.  The shader effect inflates the bounds
//    in each direction by the padding.
//
//-----------------------------------------------------------------------------

HRESULT 
CMilShaderEffectDuce::TransformBoundsForInflation(__inout CMilRectF *bounds)
{
    HRESULT hr = S_OK;

    bounds->top -= static_cast<float>(m_data.m_TopPadding);    
    bounds->left -= static_cast<float>(m_data.m_LeftPadding);
    bounds->bottom += static_cast<float>(m_data.m_BottomPadding);
    bounds->right += static_cast<float>(m_data.m_RightPadding);

    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::ApplyEffect
//
// Synopsis: 
//      Renders ShaderEffect into the currently selected render target
//      on the device.
//
//-----------------------------------------------------------------------------

HRESULT 
CMilShaderEffectDuce::ApplyEffect(
    __in CContextState *pContextState, 
    __in CHwSurfaceRenderTarget *pDestRT,
    __in CMILMatrix *pScaleTransform,
    __in CD3DDeviceLevel1 *pDevice, 
    UINT uIntermediateWidth,
    UINT uIntermediateHeight,
    __in_opt CHwTextureRenderTarget *pImplicitInput
    )
{
    HRESULT hr = S_OK;
    CD3DVidMemOnlyTexture* pImplicitInputTextureNoRef = NULL;

    if (pImplicitInput != NULL)
    {
        pImplicitInputTextureNoRef = pImplicitInput->GetTextureNoRef();
    }
    
    //
    // Send effect-supplied shader constants to the device.
    IFC(SendShaderSamplersHw(
        pContextState, 
        pDevice, 
        pDestRT, 
        pImplicitInputTextureNoRef, 
        static_cast<float>(uIntermediateWidth), 
        static_cast<float>(uIntermediateHeight), 
        pScaleTransform));

    IFC(pDevice->PrepareShaderEffectPipeline(GetShaderMajorVersion() == 3));

    //
    // EnsureState will configure our device for rendering into the destination pDestRt.
    //
    IFC(pDestRT->EnsureState(pContextState));

    //
    // Load the pixel shader and set it on the device. 

    IFC(m_data.m_pPixelShader->SetupShader(pDevice));

    //
    // Populate the vertex buffer for rendering into the final destination buffer. 
    //   
    IFC(SetupVertexTransform(
            pContextState, 
            pDevice, 
            static_cast<float>(uIntermediateWidth), 
            static_cast<float>(uIntermediateHeight),
            true /* drawing into final destination texture */));

    //
    // Send effect-supplied shader constants to the device.
    IFC(SendShaderConstantsHw(pDevice));


    //
    // Send down the destination size into a shader constant if requested.
    if ((m_data.m_DdxUvDdyUvRegisterIndex >= 0) && 
        (m_data.m_DdxUvDdyUvRegisterIndex < PIXELSHADER_CONSTANTS_MAX))
    {
        MilPoint2F downRightXY[2] = { { 1.0f, 0 }, { 0, 1.0f} }; // downRightXY maps to ddx(uv), ddy(uv)
        CMILMatrix matTextureToLocal;
        CMILMatrix matBitmapToBaseSamplingSpace;
        CMilRectF sourceTextureRect(0, 0, 1.0f, 1.0f, LTRB_Parameters);
        CMilRectF rectangleShapeRect(0, 0, static_cast<float>(uIntermediateWidth), static_cast<float>(uIntermediateHeight), XYWH_Parameters);

        // WARNING: InferAffineMatrix is defined on multiple classes with which all map the implementation to a
        // common CBaseMatrix::InferAffineMatrix. However each implementation treats the arguments in different order!!!        
        matTextureToLocal.InferAffineMatrix(rectangleShapeRect, sourceTextureRect);
        matBitmapToBaseSamplingSpace.SetToMultiplyResult(matTextureToLocal, pContextState->WorldToDevice);

        CMILMatrix deviceHPCToWorldHPC = *(reinterpret_cast<const CMILMatrix*>(&matBitmapToBaseSamplingSpace));
        if (deviceHPCToWorldHPC.Invert())
        {
            deviceHPCToWorldHPC.TransformAsVectors(downRightXY, OUT downRightXY, 2);
        }
        else
        {
            IFC(WGXERR_NONINVERTIBLEMATRIX);                
        }

        {
            float registerValue[4];
            registerValue[0] = downRightXY[0].X;
            registerValue[1] = downRightXY[0].Y;
            registerValue[2] = downRightXY[1].X;
            registerValue[3] = downRightXY[1].Y;
            
            IFC(pDevice->SetPixelShaderConstantF(m_data.m_DdxUvDdyUvRegisterIndex, &(registerValue[0]), 1));
        }
    }

    // 
    // Draw into the final destination texture. 
    
    IFC(pDevice->SetAlphaBlendMode(&CD3DRenderState::sc_abmSrcOverPremultiplied));

    IFC(pDevice->DrawTriangleStrip(0, 2));

    IFC(ResetTextureStagesHw(pDevice));

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::ResetTextureStagesHw
//
//-----------------------------------------------------------------------------

HRESULT 
CMilShaderEffectDuce::ResetTextureStagesHw(
    __in CD3DDeviceLevel1 *pDevice
    )
{
    HRESULT hr = S_OK;
    
    for (UINT32 i = 0; i < m_samplerDataCount; i++)
    {
        const SamplerData* pCurrentSamplerData = &(m_pSamplerData[i]);
        IFC(pDevice->SetTexture(pCurrentSamplerData->GetSamplerRegister(), NULL));
    }

Cleanup:
    RRETURN(hr);
}
//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::SendShaderSamplersSw
//
// Synopsis: 
//      Prepares the intermediates for the sw pass.
//
//-----------------------------------------------------------------------------


HRESULT 
CMilShaderEffectDuce::SendShaderSamplersSw(
    __in CContextState *pContextState,
    __in CSwRenderTargetSurface *pDestRT,
    __in_opt IWGXBitmap *pImplicitInputTexture,
    float implicitInputWidth,
    float implicitInputHeight,
    __in const CMILMatrix *pScaleTransform)
{
    //
    // OVERVIEW: 
    //   This method will produce the intermediate textures for the software shader
    //   pass. The textures are then used during the PrepareSoftwarePass call that the 
    //   software rasterizer makes to populate the CPixelShaderState structure. 
    //
    
    
    HRESULT hr = S_OK;


    //
    // This for loop is responsible for the first stage (see above). 
    //
    
    for (UINT32 i = 0; i < m_samplerDataCount; i++)
    {
        SamplerData* pCurrentSamplerData = &(m_pSamplerData[i]);
        CMilBrushDuce *pBrushNoRef = pCurrentSamplerData->GetBrushNoRef();

        if (pBrushNoRef == NULL)
        {
            continue;
        }
        else if (pBrushNoRef->IsOfType(TYPE_IMPLICITINPUTBRUSH))
        {
            // If the shader uses the implicit input, it should have been realized.
            Assert(pImplicitInputTexture);
            pCurrentSamplerData->SetSwTexture(pImplicitInputTexture);
        }
        else if (pBrushNoRef->IsOfType(TYPE_VISUALBRUSH) || pBrushNoRef->IsOfType(TYPE_IMAGEBRUSH))
        {                
            IWGXBitmap *pTexture = NULL;
            IFC(PrepareTileBrushSamplerSw(
                pContextState, 
                pBrushNoRef, 
                pDestRT, 
                implicitInputWidth, 
                implicitInputHeight, 
                pScaleTransform, 
                OUT &pTexture
                ));                
            
            pCurrentSamplerData->SetSwTexture(pTexture);
            ReleaseInterface(pTexture);
        }
        else if (pBrushNoRef->IsOfType(TYPE_BITMAPCACHEBRUSH))
        {
            CMilBitmapCacheBrushDuce *pCacheBrush = DYNCAST(CMilBitmapCacheBrushDuce, pBrushNoRef);
            IWGXBitmap *pTexture = NULL;
            Assert(pCacheBrush);

            IFC(PrepareCacheBrushSamplerSw(
                pCacheBrush,
                pDestRT,
                OUT &pTexture
                ));
            
            pCurrentSamplerData->SetSwTexture(pTexture);
            ReleaseInterface(pTexture);
        }
        else
        {
            AssertMsg(false, "Non NULL brushes must be either TYPE_IMPLICITINPUTBRUSH, TYPE_VISUALBRUSH or TYPE_IMAGEBRUSH");
        }
    }
   
Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::PrepareCacheBrushSamplerSw
//
//-----------------------------------------------------------------------------

HRESULT 
CMilShaderEffectDuce::PrepareCacheBrushSamplerSw(
    __in CMilBitmapCacheBrushDuce *pBrush,
    __in CSwRenderTargetSurface *pDestRT, 
    __deref_out_opt IWGXBitmap** ppBrushSwTexture
    )
{
    HRESULT hr = S_OK;
    
    IMILRenderTargetBitmap *pCacheRTB = NULL;
    CMetaBitmapRenderTarget *pMetaRTB = NULL;
    IMILRenderTargetBitmap *pRTBNoRef = NULL;
    IWGXBitmap *pSwTexture = NULL;
    
    IFC(pBrush->GetRenderTargetBitmap(
        GetCompositionDeviceNoRef(), 
        pDestRT, 
        &pCacheRTB
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Device)
        ));

    if (pCacheRTB != NULL)
    {
        // If we are running the effect in software, our cache might be in hardware
        // or software.  If it's in hardware, it must be a meta bitmap RT.
        MIL_THR(pCacheRTB->QueryInterface(
            IID_CMetaBitmapRenderTarget,
            reinterpret_cast<void **>(&pMetaRTB)
            ));

        // If we're caching in hardware, find a specific hardware render target from which
        // we'll put bits from video memory into system memory.
        if (SUCCEEDED(hr))
        {
            IFC(pMetaRTB->GetCompatibleSubRenderTargetNoRef( 
                CMILResourceCache::SwRealizationCacheIndex,
                pDestRT->GetDisplayId(),
                &pRTBNoRef
                ));
        }
        else
        {
            hr = S_OK;
            
            // If we're caching in software we can just get a bitmap directly from our software
            // render target.
            pRTBNoRef = pCacheRTB;
        }
        
        IFC(pRTBNoRef->GetBitmap(&pSwTexture));
    }

    *ppBrushSwTexture = pSwTexture; // Pass ref out, or null
    pSwTexture = NULL;
    
Cleanup:
    ReleaseInterface(pCacheRTB);
    ReleaseInterface(pMetaRTB);
    ReleaseInterface(pSwTexture);
    
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::SetTileBrushSamplerSw
//
//-----------------------------------------------------------------------------

HRESULT 
CMilShaderEffectDuce::PrepareTileBrushSamplerSw(
    __in CContextState *pContextState,
    __in CMilBrushDuce *pBrush, 
    __in CSwRenderTargetSurface *pDestRT,
    float implicitInputWidth,
    float implicitInputHeight,
    __in const CMILMatrix *pScaleTransform,
    __deref_out IWGXBitmap** ppBrushSwTexture
    )
{
    HRESULT hr = S_OK;
   
    IMILRenderTargetBitmap *pRenderTargetBitmap = NULL;
    IWGXBitmap *pSwTexture = NULL;

    IFC(DrawIntoIntermediate(
        pContextState, 
        pBrush, 
        pDestRT, 
        implicitInputWidth, 
        implicitInputHeight, 
        pScaleTransform,
        &pRenderTargetBitmap
        ));

    IFC(pRenderTargetBitmap->GetBitmap(&pSwTexture));

    *ppBrushSwTexture = pSwTexture;
    pSwTexture = NULL; // Transferring ref to out argument.

Cleanup:
    ReleaseInterface(pRenderTargetBitmap);
    ReleaseInterface(pSwTexture);
    
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::SendShaderSamplersHw
//
// Synopsis: 
//      Establishes the shader samplers on the device
//
//-----------------------------------------------------------------------------

HRESULT
CMilShaderEffectDuce::SendShaderSamplersHw(
    __in CContextState *pContextState,
    __in CD3DDeviceLevel1 *pDevice,
    __in CHwSurfaceRenderTarget *pDestRT,
    __in_opt CD3DVidMemOnlyTexture* pImplicitInputTexture,
    float implicitInputWidth,
    float implicitInputHeight,
    __in const CMILMatrix *pScaleTransform
    )
{
    //
    // OVERVIEW: 
    // Setting up the inputs for an effect happens for HW in two stages. The first stage produces all the input 
    // textures, the second stage the calls DX texture stage methods to set the states. This has to happen in
    // two stages because realizing textures might involve rendering (for example for a VisualBrush) which 
    // might use the texture stages. 
    //
    
    
    HRESULT hr = S_OK;


    //
    // This for loop is responsible for the first stage (see above). 
    //
    
    for (UINT32 i = 0; i < m_samplerDataCount; i++)
    {
        SamplerData* pCurrentSamplerData = &(m_pSamplerData[i]);
        CMilBrushDuce *pBrushNoRef = pCurrentSamplerData->GetBrushNoRef();

        if (pBrushNoRef == NULL)
        {
            continue; 
        }
        else if (pBrushNoRef->IsOfType(TYPE_IMPLICITINPUTBRUSH))
        {
            // If the shader uses the implicit input, it should have been realized.
            Assert(pImplicitInputTexture);
            pCurrentSamplerData->SetD3DTexture(pImplicitInputTexture);
        }
        else if (pBrushNoRef->IsOfType(TYPE_VISUALBRUSH) || pBrushNoRef->IsOfType(TYPE_IMAGEBRUSH))
        {                
            CD3DVidMemOnlyTexture* pTexture = NULL;
            IFC(PrepareTileBrushSamplerHw(
                pContextState, 
                pBrushNoRef, 
                pDestRT, 
                implicitInputWidth, 
                implicitInputHeight, 
                pScaleTransform, 
                OUT &pTexture
                ));                
            
            pCurrentSamplerData->SetD3DTexture(pTexture);
            ReleaseInterface(pTexture);
        }
        else if (pBrushNoRef->IsOfType(TYPE_BITMAPCACHEBRUSH))
        {
            CMilBitmapCacheBrushDuce *pCacheBrush = DYNCAST(CMilBitmapCacheBrushDuce, pBrushNoRef);
            CD3DVidMemOnlyTexture* pTexture = NULL;
            Assert(pCacheBrush);

            IFC(PrepareCacheBrushSamplerHw(
                pCacheBrush, 
                pDevice, 
                pDestRT, 
                OUT &pTexture
                ));
            
            pCurrentSamplerData->SetD3DTexture(pTexture);
            ReleaseInterface(pTexture);
        }
        else
        {
            AssertMsg(false, "Non NULL brushes must be either TYPE_IMPLICITINPUTBRUSH, TYPE_VISUALBRUSH or TYPE_IMAGEBRUSH");
        }
    }

    //
    // This for loop is responsible for the second phase, that is configuring the HW texture stages. 
    //
    for (UINT32 i = 0; i < m_samplerDataCount; i++)
    {
        SamplerData* pCurrentSamplerData = &(m_pSamplerData[i]);
        
        IFC(pDevice->SetTexture(
            pCurrentSamplerData->GetSamplerRegister(), 
            pCurrentSamplerData->GetD3DTextureNoRef()
            ));

        // Set the sampling mode for this sampler to bilinear or nearest neighbor.
        DWORD samplingMode = pCurrentSamplerData->GetSamplingMode();
        AssertMsg(samplingMode == EFFECT_SAMPLING_MODE_NEAREST_NEIGHBOR || 
                  samplingMode == EFFECT_SAMPLING_MODE_BILINEAR ||
                  samplingMode == EFFECT_SAMPLING_MODE_AUTO, 
                  "SamplingMode has an unsupported value.");
        
        // Ensure the address mode is set to clamp to match PixelJIT implementation.
        // Set the sampling mode for this sampler as specified.
        IFC(SetSamplerState(
            pDevice, 
            pCurrentSamplerData->GetSamplerRegister(), 
            true,     // set address mode to clamp
            samplingMode != EFFECT_SAMPLING_MODE_NEAREST_NEIGHBOR // use nearest neighbor if specified, otherwise use bilinear
            ));  
    }
    
Cleanup:
    for (UINT32 i = 0; i < m_samplerDataCount; i++)
    {
        SamplerData* pCurrentSamplerData = &(m_pSamplerData[i]);
        pCurrentSamplerData->SetD3DTexture(NULL);
    }
    
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::PrepareCacheBrushSamplerHw
//
//-----------------------------------------------------------------------------

HRESULT 
CMilShaderEffectDuce::PrepareCacheBrushSamplerHw(
    __in CMilBitmapCacheBrushDuce *pBrush,
    __in CD3DDeviceLevel1 *pDevice,
    __in CHwSurfaceRenderTarget *pDestRT, 
    __deref_out CD3DVidMemOnlyTexture** ppBrushTexture
    )
{
    HRESULT hr = S_OK;
    
    IMILRenderTargetBitmap *pCacheRTB = NULL;
    IRenderTargetInternal *pIRT = NULL;
    IMILRenderTargetBitmap *pHwRTBNoRef = NULL;
    CD3DVidMemOnlyTexture *pTexture = NULL;

    
    IFC(pBrush->GetRenderTargetBitmap(
        GetCompositionDeviceNoRef(), 
        pDestRT, 
        &pCacheRTB
        DBG_ANALYSIS_COMMA_PARAM(CoordinateSpaceId::Device)
        ));

    if (pCacheRTB == NULL)
    {
        // If our cache brush has no texture (because, say, it isn't pointing at a Visual)
        // we still need a texture to supply to the shader.  Create a 1x1 transparent black
        // hw texture.
        IntermediateRTUsage rtUsage;
        rtUsage.flags = IntermediateRTUsage::ForBlending;
        rtUsage.wrapMode = MilBitmapWrapMode::Extend;

        IFC(pDestRT->CreateRenderTargetBitmap(
            1, 
            1, 
            rtUsage,  
            MilRTInitialization::ForceCompatible, 
            OUT &pCacheRTB
            ));

        IFC(pCacheRTB->QueryInterface(
            IID_IRenderTargetInternal,
            reinterpret_cast<void **>(&pIRT)));

        //
        // Clear the render target to blank
        //
        {
            MilColorF colBlank = {0, 0, 0, 0};
            IFC(pIRT->Clear(&colBlank));
        }

        CHwTextureRenderTarget *pHwRTNoRef = DYNCAST(CHwTextureRenderTarget, pIRT);
        Assert(pHwRTNoRef);
        pTexture = pHwRTNoRef->GetTextureNoRef();
    }
    else
    {
        // If we are running an effect in hardware, we know that we're caching in hardware
        // for certain.  Cached hardware textures are always meta bitmap RTs.
        CMetaBitmapRenderTarget *pMetaRTNoRef = DYNCAST(CMetaBitmapRenderTarget, pCacheRTB);
        Assert(pMetaRTNoRef);
        IFC(pMetaRTNoRef->GetCompatibleSubRenderTargetNoRef( 
            pDevice->GetRealizationCacheIndex(),
            pDestRT->GetDisplayId(),
            &pHwRTBNoRef
            ));

        CHwTextureRenderTarget *pHwRTNoRef = DYNCAST(CHwTextureRenderTarget, pHwRTBNoRef);
        Assert(pHwRTNoRef);
        pTexture = pHwRTNoRef->GetTextureNoRef();
    }

    pTexture->AddRef();
    *ppBrushTexture = pTexture; // Pass ref out
    
Cleanup:
    ReleaseInterface(pCacheRTB);
    ReleaseInterface(pIRT);
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::PrepareVisualBrushSamplerHw
//
//-----------------------------------------------------------------------------

HRESULT 
CMilShaderEffectDuce::PrepareTileBrushSamplerHw(
    __in CContextState *pContextState,
    __in CMilBrushDuce *pBrush, 
    __in CHwSurfaceRenderTarget *pDestRT, 
    float implicitInputWidth,
    float implicitInputHeight,
    __in const CMILMatrix *pScaleTransform,
    __deref_out CD3DVidMemOnlyTexture** ppBrushTexture
    )
{
    HRESULT hr = S_OK;
    IMILRenderTargetBitmap *pRenderTargetBitmap = NULL;
    CHwTextureRenderTarget *pTextureRTNoRef = NULL;

    IFC(DrawIntoIntermediate(
        pContextState, 
        pBrush, 
        pDestRT, 
        implicitInputWidth, 
        implicitInputHeight, 
        pScaleTransform,
        OUT &pRenderTargetBitmap));
 

    // Since we create this off a hw render target, we know we can simply cast it.
    pTextureRTNoRef = static_cast<CHwTextureRenderTarget*>(pRenderTargetBitmap);    

    (*ppBrushTexture) =  pTextureRTNoRef->GetTextureNoRef();
    (*ppBrushTexture)->AddRef();
    

Cleanup:
    ReleaseInterface(pRenderTargetBitmap);

    RRETURN(hr);
}


//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::DrawIntoIntermediate
//
//-----------------------------------------------------------------------------

HRESULT 
CMilShaderEffectDuce::DrawIntoIntermediate(
    __in CContextState *pContextState,
    __in CMilBrushDuce *pBrush, 
    __in IRenderTargetInternal *pDestRT, 
    float implicitInputWidth,
    float implicitInputHeight,
    __in const CMILMatrix *pScaleTransform,
    __deref_out IMILRenderTargetBitmap** ppTexture
    )
{
    HRESULT hr = S_OK;
    IMILRenderTargetBitmap *pRenderTargetBitmap = NULL;
    IRenderTargetInternal *pRenderTargetBitmapInternal = NULL;
    CBrushRealizer *pBrushRealizer = NULL;
    IntermediateRTUsage rtUsage;
    CContextState contextState;
    // ContextStates are initialized assumg PageInPixel space which is typically converted by the Meta RT layer into device space. 
    // Since we are operating here below the Meta RT, we need to change the default coordinate space to Device which is what we 
    // actual operate in. 
    contextState.WorldToDevice.DbgChangeToSpace<CoordinateSpace::PageInPixels,CoordinateSpace::Device>();
    // Note that the render state is inherited here, so we realize the secondary input with the context's bitmap scaling mode, etc.
    contextState.RenderState = pContextState->RenderState;
    contextState.AliasedClip = CAliasedClip(&CMilRectF::sc_rcInfinite);

    BrushContext brushContext;
    brushContext.pBrushDeviceNoRef = GetCompositionDeviceNoRef();
    brushContext.fBrushIsUsedFor3D = false;
    brushContext.fRealizeProceduralBrushesAsIntermediates = FALSE;
    brushContext.pRenderTargetCreator = NULL;

    // Double precision is always good karma...
    MilPointAndSizeD boundingBox = { 0, 0, static_cast<double>(implicitInputWidth), static_cast<double>(implicitInputHeight) };
    brushContext.rcWorldBrushSizingBounds = boundingBox;
    brushContext.rcWorldSpaceBounds = CMilRectF::sc_rcInfinite;

    brushContext.pContentBounder = NULL;
    IFC(CContentBounder::Create(brushContext.pBrushDeviceNoRef, &brushContext.pContentBounder));
   
    UINT textureWidth = static_cast<UINT>(implicitInputWidth);
    UINT textureHeight = static_cast<UINT>(implicitInputHeight);

    rtUsage.flags = IntermediateRTUsage::ForBlending;
    rtUsage.wrapMode = MilBitmapWrapMode::Extend;

    IFC(pDestRT->CreateRenderTargetBitmap(
        textureWidth, 
        textureHeight, 
        rtUsage,  
        MilRTInitialization::ForceCompatible, 
        OUT &pRenderTargetBitmap));

    EventWriteWClientCreateIRT(this, this->GetCompositionDeviceNoRef()->GetCurrentResourceNoRef(), IRT_ShaderEffect_Input);

    IFC(pRenderTargetBitmap->QueryInterface(
        IID_IRenderTargetInternal,
        reinterpret_cast<void **>(&pRenderTargetBitmapInternal)));

    //
    // Clear the render target to blank
    //
    {
        MilColorF colBlank = {0, 0, 0, 0};
        IFC(pRenderTargetBitmapInternal->Clear(&colBlank));
    }
    
    //
    // Retrieve the brush realizations.
    //

    IFC(pBrush->GetRealizer(&brushContext, &pBrushRealizer));

    IFC(pRenderTargetBitmapInternal->DrawInfinitePath(
        &contextState,
        &brushContext,
        pBrushRealizer));

    *ppTexture = pRenderTargetBitmap;
    pRenderTargetBitmap = NULL; //Transferring reference. 

    
Cleanup:
    ReleaseInterface(pRenderTargetBitmap);
    ReleaseInterface(pRenderTargetBitmapInternal);
    delete brushContext.pContentBounder;

    if (pBrushRealizer)
    {
        // FreeRealizationResources must be called to release resources that are 
        // not supposed to outlast a single primitive. (Not calling this method
        // leads in checked builds to a hard to track down assert).
        pBrushRealizer->FreeRealizationResources();
    }

    ReleaseInterface(pBrushRealizer);

    RRETURN(hr);
}



//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::SendShaderConstantsHw
//
// Synopsis: 
//      Establishes the shader constants on the device
//
//-----------------------------------------------------------------------------

HRESULT
CMilShaderEffectDuce::SendShaderConstantsHw(
    __inout CD3DDeviceLevel1 *pDevice
    )
{
    HRESULT hr = S_OK;

    // Floating point values
    float *pFloatValues  = m_data.m_pDependencyPropertyFloatValuesData;
    UINT32 floatCount = m_data.m_cbShaderConstantFloatRegistersSize / sizeof(short);
    const short *pFloatRegisterIndices = m_data.m_pShaderConstantFloatRegistersData;
    for (UINT32 i = 0; i < floatCount; i++)
    {
        IFC(pDevice->SetPixelShaderConstantF(*pFloatRegisterIndices++, pFloatValues, 1));
        pFloatValues += 4;
    }

    // Int values
    int *pIntValues = m_data.m_pDependencyPropertyIntValuesData;
    UINT32 intCount = m_data.m_cbShaderConstantIntRegistersSize / sizeof(short);
    const short *pIntRegisterIndices = m_data.m_pShaderConstantIntRegistersData;
    for (UINT32 i = 0; i < intCount; i++)
    {
        IFC(pDevice->SetPixelShaderConstantI(*pIntRegisterIndices++, pIntValues));
        pIntValues += 4;
    }

    // Bool values
    BOOL *pBoolValues  = m_data.m_pDependencyPropertyBoolValuesData;
    UINT32 boolCount = m_data.m_cbShaderConstantBoolRegistersSize / sizeof(short);
    const short *pBoolRegisterIndices = m_data.m_pShaderConstantBoolRegistersData;
    for (UINT32 i = 0; i < boolCount; i++)
    {
        IFC(pDevice->SetPixelShaderConstantB(*pBoolRegisterIndices++, *pBoolValues));
        pBoolValues++;
    }

Cleanup:
    RRETURN(hr);
}

//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::ApplyEffectSw
//
//-----------------------------------------------------------------------------

HRESULT 
CMilShaderEffectDuce::ApplyEffectSw(
    __in CContextState *pContextState,
    __in CSwRenderTargetSurface *pDestRT,
    __in CMILMatrix *pScaleTransform, 
    UINT uIntermediateWidth,
    UINT uIntermediateHeight,
    __in_opt IWGXBitmap *pImplicitInput
    )
{
    HRESULT hr = S_OK;

    if (m_pSwShaderEffectBrush == NULL)
    {
        IFC(CMILBrushShaderEffect::Create(this, OUT &m_pSwShaderEffectBrush));
    }

    {
        // Create a local shader effect brush.
        LocalMILObject<CImmediateBrushRealizer> shaderEffectBrush;
        CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::BaseSampling> matTextureToLocal;
        CRectangle rectangle;
        CRectF<CoordinateSpace::RealizationSampling> sourceTextureRect(0, 0, 1.0f, 1.0f, LTRB_Parameters);

        // This code is about to pass the textureToLocal transform all the way down into the sw rasterizer
        // and back up into this code. There is no need to do this, since the rasterizer does not make any use
        // of it. Cleanup this code by moving this calculation into PrepareSoftwarePass.        
        { 
            CRectF<CoordinateSpace::BaseSampling> rectangleShapeRect(0, 0, static_cast<float>(uIntermediateWidth), static_cast<float>(uIntermediateHeight), XYWH_Parameters);
            rectangle.Set(rectangleShapeRect, 0.0f /* radius */);

            // WARNING: InferAffineMatrix is defined on multiple classes with which all map the implementation to a
            // common CBaseMatrix::InferAffineMatrix. However each implementation treats the arguments in different order!!!
            matTextureToLocal.InferAffineMatrix(sourceTextureRect, rectangleShapeRect);
        }

        m_destinationWidthSw = static_cast<float>(uIntermediateWidth);
        m_destinationHeightSw = static_cast<float>(uIntermediateHeight);

        IFC(SendShaderSamplersSw(
            pContextState,
            pDestRT,
            pImplicitInput,
            m_destinationWidthSw,
            m_destinationHeightSw,
            pScaleTransform));
       
        IFC(m_pSwShaderEffectBrush->ConfigurePass(matTextureToLocal));
        shaderEffectBrush.SetMILBrush(m_pSwShaderEffectBrush, NULL, true /* skip meta fix ups */);
        IFC(pDestRT->DrawPath(pContextState, NULL, &rectangle, NULL, NULL, &shaderEffectBrush));
    }   

Cleanup:

    // Release all rendering resources that we accumulated for the sw pass.

    for (UINT32 i = 0; i < m_samplerDataCount; i++)
    {
        SamplerData *pSamplerData = &(m_pSamplerData[i]);
        pSamplerData->SetSwTexture(NULL);
        pSamplerData->SetSwTextureLock(NULL);
    }
    
    RRETURN(hr);
}


//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::PrepareSoftwarePass
//
// Description: This method is called by the software pipeline to setup the 
//      pixel shader state and get a prepare color method.
//  
//-----------------------------------------------------------------------------

HRESULT 
CMilShaderEffectDuce::PrepareSoftwarePass(
        __in const CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::DeviceHPC> *pRealizationSamplingToDevice,
        __inout CPixelShaderState *pPixelShaderState, 
        __deref_out CPixelShaderCompiler **ppPixelShaderCompiler
        )
{
    HRESULT hr = S_OK;
    IWGXBitmapLock *pSwTextureLock = NULL;

    // Floating point values
    float *pFloatValues  = m_data.m_pDependencyPropertyFloatValuesData;
    UINT registerCount = m_data.m_cbShaderConstantFloatRegistersSize / sizeof(short);
    const short *pRegisterIndices = m_data.m_pShaderConstantFloatRegistersData;

    for (UINT i = 0; i < registerCount; i++)
    {
        UINT registerIndex = pRegisterIndices[i];
        if (registerIndex >= PIXELSHADER_CONSTANTS_MAX)
        {
            IFC(E_INVALIDARG);
        }
        
        float* pRegister = pPixelShaderState->m_rgShaderConstants[registerIndex];
        CopyMemory(pRegister, pFloatValues, sizeof(float) * 4);
        pFloatValues += 4;
    }

    //
    // Configure sampler state
    //

    for (UINT32 i = 0; i < m_samplerDataCount; i++)
    {
        UINT bufferSizeInBytes; // size of the buffer in bytes.
        unsigned *pBits = NULL;
        unsigned width = 0;
        unsigned height = 0;
        SamplerData *pCurrentSamplerData = &(m_pSamplerData[i]);
        IWGXBitmap *pSwTexture = pCurrentSamplerData->GetSwTextureNoRef();
        DWORD registerIndex = pCurrentSamplerData->GetSamplerRegister();

        if (pSwTexture == NULL) // This indicates we are operating on a NULL brush.
        {
            width = 1;
            height = 1;
            pBits = &g_uBlank;
        }
        else
        {
            IFC(pSwTexture->GetSize(OUT &width, OUT &height));

            // Try to acquire a lock for the software texture.
            {
                WICRect lockRect = { 0, 0, width, height };
                IFC(pSwTexture->Lock(&lockRect, MilBitmapLock::Read, &pSwTextureLock));
                
                // Store reference to lock in SamplerData struct to keep the locked bits alive
                // while the sw code executes the PixelShader. Note: if this function fails, 
                // ApplyEffectSw will cleanup the SamplerData struct and release the lock.
                pCurrentSamplerData->SetSwTextureLock(pSwTextureLock);        
            }
        
            IFC(pSwTextureLock->GetDataPointer(&bufferSizeInBytes, reinterpret_cast<byte**>(&pBits)));
        }
                
        pPixelShaderState->m_samplers[registerIndex].m_nWidth = width;
        pPixelShaderState->m_samplers[registerIndex].m_nHeight = height;
        pPixelShaderState->m_samplers[registerIndex].m_pargbSource = pBits;

        DWORD samplingMode = pCurrentSamplerData->GetSamplingMode();
        AssertMsg(samplingMode == EFFECT_SAMPLING_MODE_NEAREST_NEIGHBOR || 
                  samplingMode == EFFECT_SAMPLING_MODE_BILINEAR ||
                  samplingMode == EFFECT_SAMPLING_MODE_AUTO, 
                  "SamplingMode has an unsupported value.");

        // Use nearest neighbor unless we specify bilinear since it's faster.
        pPixelShaderState->m_samplers[registerIndex].m_nUseBilinear = (samplingMode == EFFECT_SAMPLING_MODE_BILINEAR) ? 1 : 0;

        // Release local reference.
        ReleaseInterface(pSwTextureLock); 
    }


    {
        //
        // Texture sample space is the space that is described by mapping the bounding box of the shape that is being rasterized to 
        // Rect{x=0, y=0, width=1.0f, height=1.0f} (Think DX UV coordinates for textures). 
        // 
        // For example:
        //   X--------------------    X is the origin of the screen
        //   |
        //   |
        //
        //        A-----------|
        //        |   SSSS    |
        //        | SSSSSSSS  |
        //        |SSSSSSSSSSS|            S marks the filled shape.
        //        | SSSSSSSSS |
        //        |  SSSSSSS  |            The shape's bbox top-left corner is A and the bottom-right corner is B
        //        | SSSSSSSSS |
        //        |-----------B
        //
        //   The goal is to map the bbox(A, B) into Rect{x=0, y=0, w=1, h=1}.
        //
        //   The transform that takes (0, 0, 1, 1) to device space is TS * WorldTransform where the WorldTransform is passed
        //   in the context state (and effectivly positions the shape, potentially rotated) and TS is the transform that maps
        //   Rect{x=0, y=0, w=1, h=1} into bbox(A, B) in local shape space. The WorldTransform then takes this space to device space.
        //   Since TS depends on the shape being drawn, TS * WorldTransform is calculated higher up in the call stack and passed 
        //   to this code as the argument pRelizationSamplingToDevice. 
        //
        //   The pixel jit uses two inputs to traverse input textures. The first input is the offset to A in Rect{x=0, y=0, w=1, h=1} space. 
        //   It is used to set up the initial UV coordinate value for the shader. This offset is calculated by inverting pRealizationSamplingToDevice 
        //   and transforming the (0, 0) point through the result. The second input required by the pixel jit is the delta vector for a step
        //   in the x direction and a step in the y direction in destination space. The pixel jit uses those to progress the UV coordinates 
        //   passed to the shader while rendering a scan line. The delta vectors are computed by inverting pRealizationSamplingToDevice and using the
        //   result to transform the unit vectors (1, 0) and (0, 1). 
        //            
        //   The initial UV coordinates (startUV) are therefore computed as:
        //
        //      startX: x coordinate of the first pixel being rasterized.
        //      startY: y coordinate of the first pixel being rasterized.
        //
        //      startUV = (offset.X + startX * dux + startY * duY, offsetY + startX * dvx + startY * dvy)
        //
        //   The offset is set in the m_rgOffsetUV member and the dux, duy, dvx, dvy are set in the m_rgDeltaUVDownRight array of the pixel
        //   shader state structure. 
        //
        MilPoint2F downRightXY[2] = { { 1.0f, 0 }, { 0, 1.0f} }; // downRightXY maps to ddx(uv), ddy(uv)
        MilPoint2F start = {0, 0};
        
        CMILMatrix deviceHPCToWorldHPC = *(reinterpret_cast<const CMILMatrix*>(pRealizationSamplingToDevice));
        if (deviceHPCToWorldHPC.Invert())
        {
            deviceHPCToWorldHPC.TransformAsVectors(downRightXY, OUT downRightXY, 2);
            deviceHPCToWorldHPC.Transform(&start, OUT &start, 1);
        }
        else
        {
            IFC(WGXERR_NONINVERTIBLEMATRIX);                
        }

        pPixelShaderState->m_rgOffsetUV[0] = start.X;
        pPixelShaderState->m_rgOffsetUV[1] = start.Y;
            
        pPixelShaderState->m_rgDeltaUVDownRight[0] = downRightXY[0].X;
        pPixelShaderState->m_rgDeltaUVDownRight[1] = downRightXY[0].Y;
        pPixelShaderState->m_rgDeltaUVDownRight[2] = downRightXY[1].X;
        pPixelShaderState->m_rgDeltaUVDownRight[3] = downRightXY[1].Y;
    }


    //
    // Configure ddx/ddy registers if requested by user.
    //
    
    if ((m_data.m_DdxUvDdyUvRegisterIndex >= 0) && 
        (m_data.m_DdxUvDdyUvRegisterIndex < PIXELSHADER_CONSTANTS_MAX))
    {
        float* pRegister = pPixelShaderState->m_rgShaderConstants[m_data.m_DdxUvDdyUvRegisterIndex];
        CopyMemory(pRegister, pPixelShaderState->m_rgDeltaUVDownRight, sizeof(float) * 4);
    }

    //
    // Compile the pixel shader.
    //


    if (m_data.m_pPixelShader != NULL)
    {
        IFC(m_data.m_pPixelShader->GetSwPixelShader(ppPixelShaderCompiler));
    }
    else
    {
        *ppPixelShaderCompiler = NULL;
    }

Cleanup:
    // In case of failure, ApplyEffectSw will cleanup the locks taken by this method. 

    RRETURN(hr);    
}


//-----------------------------------------------------------------------------
//
// CMilShaderEffectDuce::OnChanged
//
//-----------------------------------------------------------------------------

BOOL 
CMilShaderEffectDuce::OnChanged(
    CMilSlaveResource *pSender, 
    NotificationEventArgs::Flags e
    )
{
    SetDirty(TRUE);
    
    return TRUE; // Bubble changed notification.
}

//-----------------------------------------------------------------------------
//
// Description: Returns true if the custom shader references the implicit
//              input.
//  
//-----------------------------------------------------------------------------

bool
CMilShaderEffectDuce::UsesImplicitInput()
{
    for (UINT32 i = 0; i < m_samplerDataCount; i++)
    {
        SamplerData* pCurrentSamplerData = &(m_pSamplerData[i]);
        const CMilBrushDuce *pBrushNoRef = pCurrentSamplerData->GetBrushNoRef();

        if (pBrushNoRef != NULL && pBrushNoRef->IsOfType(TYPE_IMPLICITINPUTBRUSH))
        {
            return true;
        }
    }

    return false;
}

//-----------------------------------------------------------------------------
//
// Description: Returns the major version of the pixel shader
//  
//-----------------------------------------------------------------------------

byte
CMilShaderEffectDuce::GetShaderMajorVersion()
{
    if (m_data.m_pPixelShader != NULL)
    {
        return m_data.m_pPixelShader->GetShaderMajorVersion();
    }
    else
    {
        // ps_2_0 by default
        return 2;
    }
}




