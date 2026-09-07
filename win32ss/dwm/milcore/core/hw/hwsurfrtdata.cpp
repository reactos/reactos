// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_d3d
//      $Keywords:
//
//  $Description:
//      CHwSurfaceRenderTargetSharedData implementation.
//      Contains costly data that we want to share between hw surface render targets.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

// Names of the Stock Shaders available in our Resource File
// NOTE: This MUST be in the same order as the StockShader enum definition 
//       in shaderutils.h or there will be a mismatch on load
WCHAR const *g_rgstrStockShaderNames[] = {
    _T("SS_RadialGradientCenteredShader2D"),
    _T("SS_RadialGradientNonCenteredShader2D"),
    };

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::ctor
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CHwSurfaceRenderTargetSharedData::CHwSurfaceRenderTargetSharedData()
{
    m_pswFallback = NULL;
    m_pD3DDevice = NULL;
    m_pDrawBitmapScratchBrush = NULL;
    m_pHwDestinationTexturePoolBGR = NULL;
    m_pHwDestinationTexturePoolPBGRA = NULL;
    m_pHwShaderCache = NULL;
    m_pScratchHwBoxColorSource = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::dtor
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CHwSurfaceRenderTargetSharedData::~CHwSurfaceRenderTargetSharedData()
{
    delete m_pswFallback;

    ReleaseInterfaceNoNULL(m_pDrawBitmapScratchBrush);
    ReleaseInterfaceNoNULL(m_pHwShaderCache);
    ReleaseInterfaceNoNULL(m_pScratchHwBoxColorSource);

    for (UINT i = 0; i < m_dynpColorComponentSources.GetCount(); i++)
    {
        ReleaseInterfaceNoNULL(m_dynpColorComponentSources[i]);
    }

    ReleaseInterfaceNoNULL(m_pHwDestinationTexturePoolBGR);
    ReleaseInterfaceNoNULL(m_pHwDestinationTexturePoolPBGRA);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::InitSharedData
//
//  Synopsis:
//      Init our shared data
//
//------------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTargetSharedData::InitSharedData(
    __in_ecount(1) CD3DDeviceLevel1 *pD3DDevice
    )
{
    HRESULT hr = S_OK;

    Assert(m_pD3DDevice == NULL);

    // Don't addref to avoid a cycle.  Since this object is
    // owned by the device, there are no lifetime issues.
    m_pD3DDevice = pD3DDevice;

    IFC(m_poolHwBrushes.Init(pD3DDevice));

    IFC(m_solidColorTextureSourcePool.Init(pD3DDevice));

    IFC(CHwDestinationTexturePool::Create(
        pD3DDevice,
        &m_pHwDestinationTexturePoolBGR
        ));;

    IFC(CHwDestinationTexturePool::Create(
        pD3DDevice,
        &m_pHwDestinationTexturePoolPBGRA
        ));;
    
    IFC(InitColorComponentSources());

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::InitColorComponentSources
//
//  Synopsis:
//      Initializes the ColorComponent Sources that we're going to use
//
//------------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTargetSharedData::InitColorComponentSources()
{
    HRESULT hr = S_OK;
    CHwColorComponentSource *pColorComponent = NULL;

    DWORD dwSourceEnum = static_cast<DWORD>(CHwColorComponentSource::Diffuse);

    while (dwSourceEnum < static_cast<DWORD>(CHwColorComponentSource::Total))
    {
        CHwColorComponentSource::VertexComponent eLocation = 
            static_cast<CHwColorComponentSource::VertexComponent>(dwSourceEnum);

        IFC(CHwColorComponentSource::Create(
            eLocation,
            &pColorComponent
            ));

        IFC(m_dynpColorComponentSources.Add(pColorComponent));

        pColorComponent = NULL;

        dwSourceEnum++;
    }

Cleanup:
    ReleaseInterfaceNoNULL(pColorComponent);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::ResetPerPrimitiveResourceUsage
//
//  Synopsis:
//      Release any per-primitive resource accumulations.
//
//  Notes:
//      This should be called between rendering primitives that may realize
//      pooled resources.
//
//------------------------------------------------------------------------------

void
CHwSurfaceRenderTargetSharedData::ResetPerPrimitiveResourceUsage(
    )
{
    m_solidColorTextureSourcePool.Clear();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::DerivePipelineShader
//
//  Synopsis:
//      Create a shader class from the shader fragments.
//

HRESULT 
CHwSurfaceRenderTargetSharedData::DerivePipelineShader(
    __in_ecount(uNumPipelineItems) HwPipelineItem const * rgShaderPipelineItems,
    UINT uNumPipelineItems,
    __deref_out_ecount(1) CHwPipelineShader ** const ppHwShader
    )
{
    HRESULT hr = S_OK;

    PCSTR pHLSLSource = NULL;
    UINT cbHLSLSource = 0;

    IDirect3DPixelShader9 *pPixelShader = NULL;
    IDirect3DVertexShader9 *pVertexShader = NULL;

    //
    // Generate the shader source
    //

    IFC(ConvertHwShaderFragmentsToHLSL(
        rgShaderPipelineItems,
        uNumPipelineItems,
        OUT pHLSLSource,
        OUT cbHLSLSource
        ));

    IFC(m_pD3DDevice->CompilePipelineVertexShader(
        pHLSLSource,
        cbHLSLSource,
        &pVertexShader
        ));

    IFC(m_pD3DDevice->CompilePipelinePixelShader(
        pHLSLSource,
        cbHLSLSource,
        &pPixelShader
        ));

    IFC(CHwPipelineShader::Create(
        rgShaderPipelineItems,
        uNumPipelineItems,
        m_pD3DDevice,
        pVertexShader,
        pPixelShader,
        ppHwShader
        DBG_COMMA_PARAM(pHLSLSource)
        ));

Cleanup:
    ReleaseInterfaceNoNULL(pVertexShader);
    ReleaseInterfaceNoNULL(pPixelShader);

    WPFFree(ProcessHeap, const_cast<PSTR>(pHLSLSource));

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::GetHwShaderCache
//
//  Synopsis:
//      Retrieve the shader cache.
//

HRESULT
CHwSurfaceRenderTargetSharedData::GetHwShaderCache(
    __deref_out_ecount(1) CHwShaderCache ** const ppCache
    )
{
    HRESULT hr = S_OK;

    if (m_pHwShaderCache == NULL)
    {
        IFC(CHwShaderCache::Create(
            m_pD3DDevice,
            &m_pHwShaderCache
            ));
    }

    *ppCache = m_pHwShaderCache;
    m_pHwShaderCache->AddRef();

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::GetCachedBrush
//
//  Synopsis:
//      Get a HW cached brush. Returns NULL if the brush is not found in the
//      cache.
//
//------------------------------------------------------------------------------
HRESULT CHwSurfaceRenderTargetSharedData::GetCachedBrush(
    __in_ecount(1) CMILBrush *pBrush,
    __in_ecount(1) CHwBrushContext const &hwBrushContext,
    __deref_out_ecount_opt(1) CHwBrush ** const ppHwCachedBrush
    )
{
    HRESULT hr = S_OK;

    *ppHwCachedBrush = NULL;

    //
    // Check cache for linear & radial gradient brushes
    //

    if (   pBrush->GetType() != BrushGradientLinear
        && pBrush->GetType() != BrushGradientRadial
       )
    {
        goto Cleanup;
    }

    CMILBrushGradient *pBrushGradientNoRef = DYNCAST(CMILBrushGradient, pBrush);
    Assert(pBrushGradientNoRef);

    //
    // Check for caching
    //
    // First make sure valid cache index has been acquired
    //

    if (m_uCacheIndex == CMILResourceCache::InvalidToken)
    {
        goto Cleanup;
    }

    IMILCacheableResource *pICachedResource = NULL;

    IFC(pBrushGradientNoRef->GetResource(
        m_uCacheIndex,
        &pICachedResource
        ));

    // GetResource can return NULL indicating that it successfully
    // found that nothing is stored.
    if (!pICachedResource)
    {
        goto Cleanup;
    }

    {
        // Cast to cached type and steal reference
        CHwCacheablePoolBrush *pCachedBrush =
            DYNCAST(CHwCacheablePoolBrush, pICachedResource);
        Assert(pCachedBrush);

        //
        // Get a realization for the current context
        //

        hr = THR(pCachedBrush->SetBrushAndContext(
            pBrushGradientNoRef,
            hwBrushContext
            ));

        // If the realization, failed then this brush needs to be
        // removed from the cache.
        if (FAILED(hr))
        {
            IGNORE_HR(pBrushGradientNoRef->SetResource(m_uCacheIndex, NULL));
            // Release reference obtained from GetResource
            pCachedBrush->Release();
        }
        else
        {
            *ppHwCachedBrush = pCachedBrush;  // Transfer reference
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::DeriveHWBrush
//
//  Synopsis:
//      Get a HW brush capable of realizing the given device independent brush
//      in the given context.
//
//  Note:
//      only one reference to a HW Brush is allowed at one time; do not try to
//      derive a second HW Brush before releasing the first
//

HRESULT
CHwSurfaceRenderTargetSharedData::DeriveHWBrush(
    __in_ecount(1) CMILBrush *pBrush,
    __in_ecount(1) CHwBrushContext const &hwBrushContext,
    __deref_out_ecount(1) CHwBrush ** const ppHwBrush
    )
{
    HRESULT hr = S_OK;

    *ppHwBrush = NULL;

    if (SUCCEEDED(GetCachedBrush(
        pBrush,
        hwBrushContext,
        ppHwBrush
        ))
        && *ppHwBrush
        )
    {
        goto Cleanup;
    }

    //
    // If unable to get a brush from the cache, try the pool.
    //

    IFC(m_poolHwBrushes.GetHwBrush(
        pBrush,
        hwBrushContext,
        ppHwBrush
        ));

Cleanup:
    //
    // Check results
    //

    if (SUCCEEDED(hr))
    {
        Assert(*ppHwBrush);
    }
    else
    {
        Assert(!*ppHwBrush);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::DeriveHWTexturedColorSource
//
//  Synopsis:
//      Get a HW textured color source capable of realizing the given device
//      independent brush in the given context.
//

HRESULT CHwSurfaceRenderTargetSharedData::DeriveHWTexturedColorSource(
    __in_ecount(1) CMILBrush *pBrush,
    __in_ecount(1) CHwBrushContext const &hwBrushContext,
    __deref_out_ecount(1) CHwTexturedColorSource ** const ppHwTexturedColorSource
    )
{
    HRESULT hr = S_OK;

    CHwBrush *pHwLinearGradientBrush = NULL;

    *ppHwTexturedColorSource = NULL;

    switch (pBrush->GetType())
    {
    case BrushSolid:
    {
        //
        // Get the color source from the solid color texture pool
        //

        MilColorF solidColor;
        const CMILBrushSolid *pSolidBrushNoRef = DYNCAST(CMILBrushSolid, pBrush);
        CHwSolidColorTextureSource *pHwSolidColorTextureSource = NULL;
        
        Assert(pSolidBrushNoRef);
        pSolidBrushNoRef->GetColor(&solidColor);

        IFC(m_solidColorTextureSourcePool.RetrieveTexture(
            solidColor,
            &pHwSolidColorTextureSource
            ));

        *ppHwTexturedColorSource = pHwSolidColorTextureSource; // steal ref
        break;
    }
    case BrushGradientLinear:
    case BrushGradientRadial:
    {
        //
        // Derive a primary color source for the linear or radial gradient
        // and grab the color source from it.
        //

        //
        // We derive a linear gradient hw brush for both linear
        // and radial gradients. Both should be realized as a 1D texture
        //

        //
        // It is not okay to use DeriveHWBrush for any brush types
        // other than linear gradient brushes. This is because all other brushes
        // use a scratch brush. The scratch brush, if retrieved twice, cannot be used
        // to do two conflicting operations.
        // Linear gradient brushes are retrieved from the cache or the pool--not from
        // a reused scratch location--so they do not suffer from this problem.
        //

        IFC(DeriveHWBrush(
            pBrush,
            hwBrushContext,
            &pHwLinearGradientBrush
            ));

        CHwLinearGradientBrush *pHwLinearGradientBrushNoRef =
            DYNCAST(CHwLinearGradientBrush, pHwLinearGradientBrush);
        Assert(pHwLinearGradientBrushNoRef);

        IFC(pHwLinearGradientBrushNoRef->GetHwTexturedColorSource(
            ppHwTexturedColorSource
            ));
        break;
    }
    case BrushBitmap:
    {
        CMILBrushBitmap *pBitmapBrush =
            DYNCAST(CMILBrushBitmap, pBrush);
        Assert(pBitmapBrush);
    
        IFC(CHwBitmapColorSource::DeriveFromBrushAndContext(
            m_pD3DDevice,
            pBitmapBrush,
            hwBrushContext,
            ppHwTexturedColorSource
            ));
        break;
    }
    default:
        IFC(E_NOTIMPL);
        break;
    }

    Assert(*ppHwTexturedColorSource);

Cleanup:
    ReleaseInterfaceNoNULL(pHwLinearGradientBrush);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::GetColorComponentSource
//
//  Synopsis:
//      Gets a HwColorComponentSource that satisfies the specified parameters.
//
//------------------------------------------------------------------------------
void
CHwSurfaceRenderTargetSharedData::GetColorComponentSource(
    CHwColorComponentSource::VertexComponent eComponent,
    __deref_out_ecount(1) CHwColorComponentSource ** const ppColorComponentSource
    )
{
    Assert(   eComponent == CHwColorComponentSource::Diffuse
           || eComponent == CHwColorComponentSource::Specular
              );

    *ppColorComponentSource = m_dynpColorComponentSources[eComponent];
    (*ppColorComponentSource)->AddRef();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::DeriveHWShader
//
//  Synopsis:
//      Get a HW shader capable of realizing it's device independent brushs in
//      the given context.
//

HRESULT
CHwSurfaceRenderTargetSharedData::DeriveHWShader(
    __in_ecount(1) CMILShader *pShader,
    __in_ecount(1) CHwBrushContext const &hwBrushContext,
    __deref_out_ecount(1) CHwShader ** const ppHwShader
    )
{
    HRESULT hr = S_OK;

    CBrushRealizer *pBrushRealizer = NULL;
    
    *ppHwShader = NULL;

    // We're beginning a new shader which means that we don't have to hold onto
    // any of the texture sources.  We can begin to reuse them.

    m_solidColorTextureSourcePool.Clear();

    switch (pShader->GetType())
    {
    case ShaderDiffuse:
    case ShaderSpecular:
    case ShaderEmissive:
        {
            CMILShaderBrush *pMILShader = DYNCAST(CMILShaderBrush, pShader);
            CMILBrush *pMILBrush = NULL;
            IMILEffectList *pEffectList = NULL;

            Assert(pMILShader);

            // Grab the CMILBrush from the Shader
            {
                IFC(pMILShader->GetSurfaceSource(&pBrushRealizer));

                //
                // The 3D rendering pipeline doesn't have the support for NULL brushes,
                // so we use a solid color brush that's transparent and pass that
                // down.
                //
                // We have to render even if all brushes are transparent, because we
                // have to populate the zbuffer.
                //

                pMILBrush = pBrushRealizer->GetRealizedBrushNoRef(true /* fConvertNULLToTransparent */);
                Assert(pMILBrush);

                IFC(pBrushRealizer->GetRealizedEffectsNoRef(&pEffectList));
            }

            CHwBrush *pHwBrush = NULL;

            IFC(DeriveHWBrush(
                pMILBrush,
                hwBrushContext,
                &pHwBrush
                ));

            switch (pShader->GetType())
            {
            case ShaderDiffuse:
                {
                    CHwDiffuseShader *pDiffuseShader = NULL;

                    IFC(CHwDiffuseShader::Create(
                        m_pD3DDevice,
                        pHwBrush,
                        pEffectList,
                        hwBrushContext,
                        &pDiffuseShader
                        ));

                    *ppHwShader = pDiffuseShader;
                }
                break;

            case ShaderSpecular:
                {
                    CHwSpecularShader *pSpecularShader = NULL;

                    IFC(CHwSpecularShader::Create(
                        m_pD3DDevice,
                        pHwBrush,
                        pEffectList,
                        hwBrushContext,
                        &pSpecularShader
                        ));

                    *ppHwShader = pSpecularShader;
                }
                break;

            case ShaderEmissive:
                {
                    CHwEmissiveShader *pEmissiveShader = NULL;

                    IFC(CHwEmissiveShader::Create(
                        m_pD3DDevice,
                        pHwBrush,
                        pEffectList,
                        hwBrushContext,
                        &pEmissiveShader
                        ));

                    *ppHwShader = pEmissiveShader;
                }
                break;

            default:
                NO_DEFAULT("Has pShader->GetType() changed?");
            }
        }
        break;
        
    default:
        NO_DEFAULT("Has pShader->GetType() changed?");
    }
    
Cleanup:
    ReleaseInterfaceNoNULL(pBrushRealizer);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::GetHwDestinationTexture
//
//  Synopsis:
//      Gets a texture containing the Destination Surface
//
//------------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTargetSharedData::GetHwDestinationTexture(
    __in_ecount(1) CHwSurfaceRenderTarget *pHwSurfaceRenderTarget,
    __in_ecount(1) CMILSurfaceRect const &rcDestRect,
    __in_ecount_opt(crgSubDestCopyRects) CMILSurfaceRect const *prgSubDestCopyRects,
    UINT crgSubDestCopyRects,
    __deref_out_ecount(1) CHwDestinationTexture ** const ppHwDestinationTexture
    )
{
    HRESULT hr = S_OK;

    CHwDestinationTexture *pHwDestinationTexture = NULL;

    MilPixelFormat::Enum fmtRT;
    IFC(pHwSurfaceRenderTarget->GetPixelFormat(&fmtRT));

    // Separate pools are kept for different render target formats.  This prevents
    // a cache being thrashed when both RT formats are used in a single frame.
    if (fmtRT == MilPixelFormat::PBGRA32bpp)
    {
        IFC(m_pHwDestinationTexturePoolPBGRA->GetHwDestinationTexture(
            &pHwDestinationTexture
            ));
    }
    else
    {
        // HW texture pooling is currently used by clip/opacity only, so
        // the only formats expected are the two supported for back buffers/intermediates
        // since the HwDestinationTexture format is matched to the RT format.
        Assert(fmtRT == MilPixelFormat::BGR32bpp);
        IFC(m_pHwDestinationTexturePoolBGR->GetHwDestinationTexture(
            &pHwDestinationTexture
            ));        
    }

    IFC(pHwDestinationTexture->SetContents(
        pHwSurfaceRenderTarget,
        rcDestRect,
        prgSubDestCopyRects,
        crgSubDestCopyRects
        ));

    *ppHwDestinationTexture = pHwDestinationTexture;
    pHwDestinationTexture = NULL; // steal ref

Cleanup:
    ReleaseInterfaceNoNULL(pHwDestinationTexture);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::GetCachedHwBoxColorSource
//
//  Synopsis:
//      Gets a cached box color source and sets its context.
//
//------------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTargetSharedData::GetScratchHwBoxColorSource(
    __in_ecount(1) MILMatrix3x2 const *pMatXSpaceToSourceClip,
    __deref_out_ecount(1) CHwBoxColorSource ** const ppTextureSource
    )
{
    HRESULT hr = S_OK;

    if (m_pScratchHwBoxColorSource)
    {
#if DBG
        Assert(!DbgHasMultipleReferences(m_pScratchHwBoxColorSource));
#endif
    }
    else
    {
        IFC(CHwBoxColorSource::Create(
            m_pD3DDevice,
            &m_pScratchHwBoxColorSource
            ));
    }

    m_pScratchHwBoxColorSource->SetContext(
        pMatXSpaceToSourceClip
        );

    *ppTextureSource = m_pScratchHwBoxColorSource;
    m_pScratchHwBoxColorSource->AddRef();

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::GetScratchDrawBitmapBrush
//
//  Synopsis:
//      Lazily allocate and return a scratch bitmap brush for the DrawBitmap
//      call.
//
//------------------------------------------------------------------------------
HRESULT CHwSurfaceRenderTargetSharedData::GetScratchDrawBitmapBrushNoAddRef(
    __deref_out_ecount(1) CMILBrushBitmap ** const ppDrawBitmapScratchBrushNoAddRef
    )
{
    HRESULT hr = S_OK;

    if (m_pDrawBitmapScratchBrush)
    {
#if DBG
        Assert(!DbgHasMultipleReferences(m_pDrawBitmapScratchBrush));
#endif
    }
    else
    {
        IFC(CMILBrushBitmap::Create(&m_pDrawBitmapScratchBrush));
    }

    *ppDrawBitmapScratchBrushNoAddRef = m_pDrawBitmapScratchBrush;

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwSurfaceRenderTargetSharedData::GetSoftwareFallback
//
//  Synopsis:
//      Lazily allocate and return SW fallback resource
//
//------------------------------------------------------------------------------
HRESULT
CHwSurfaceRenderTargetSharedData::GetSoftwareFallback(
    __deref_out_ecount(1) CHwSoftwareFallback ** const ppSoftwareFallback,
    HRESULT hrReasonForFallback
    )
{
    HRESULT hr = S_OK;

    *ppSoftwareFallback = NULL;

    if (!m_pswFallback)
    {
        m_pswFallback = new CHwSoftwareFallback;
        IFCOOM(m_pswFallback);

        hr = THR(m_pswFallback->Init(m_pD3DDevice));
        if (FAILED(hr))
        {
            delete m_pswFallback;
            m_pswFallback = NULL;
            goto Cleanup;
        }
    }

    
    if (ETW_ENABLED_CHECK(TRACE_LEVEL_INFORMATION))
    {
        if (hrReasonForFallback == D3DERR_OUTOFVIDEOMEMORY)
        {
            EventWriteUnexpectedSoftwareFallback(UnexpectedSWFallback_OutOfVideoMemory);
        }
        else if (hrReasonForFallback == E_NOTIMPL ||
                 hrReasonForFallback == WGXERR_DEVICECANNOTRENDERTEXT)
        {
            // SW Fallback reasons is likely expected- don't log it.
            //  there are some unexpcted cases where we return E_NOTIMPL.
            // It would be nice to log those as well, perhaps by changing the return code.
        }
        else
        {
            EventWriteUnexpectedSoftwareFallback(UnexpectedSWFallback_UnexpectedPrimitiveFallback);
        }
    }

    *ppSoftwareFallback = m_pswFallback;

Cleanup:
    RRETURN(hr);
}





