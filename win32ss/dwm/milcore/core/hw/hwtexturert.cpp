// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//------------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwTextureRenderTarget implementation
//

#include "precomp.hpp"

MtDefine(CHwTextureRenderTarget, MILRender, "CHwTextureRenderTarget");

//+------------------------------------------------------------------------
//
//  Function:  CHwTextureRenderTarget::Create
//
//  Synopsis:  Create the CHwTextureRenderTarget
//
//-------------------------------------------------------------------------
HRESULT
CHwTextureRenderTarget::Create(
    UINT uWidth,
    UINT uHeight,
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
    DisplayId associatedDisplay,
    BOOL fForBlending,
    __deref_out_ecount(1) CHwTextureRenderTarget ** const ppTextureRT
    DBG_STEP_RENDERING_COMMA_PARAM(__inout_ecount_opt(1) CHwDisplayRenderTarget *pDisplayRTParent)
    )
{
    UNREFERENCED_PARAMETER(fForBlending);

    HRESULT hr = S_OK;

    AssertDeviceEntry(*pDevice);

    *ppTextureRT = NULL;

    //
    // Make sure render target format has been tested.
    //

    IFC(pDevice->CheckRenderTargetFormat(
        D3DFMT_A8R8G8B8
        ));

    //
    // Create the CHwTextureRenderTarget
    //

    *ppTextureRT = new CHwTextureRenderTarget(
        pDevice,
        // Right now we always use 32bppPBGRA - we ignore fForBlending and
        // don't support scRGB.
        MilPixelFormat::PBGRA32bpp,
        D3DFMT_A8R8G8B8,
        associatedDisplay
        );
    IFCOOM(*ppTextureRT);
    (*ppTextureRT)->AddRef(); // CHwTextureRenderTarget::ctor sets ref count == 0

    //
    // Call Init
    //

    IFC((*ppTextureRT)->Init(
        uWidth,
        uHeight
        DBG_STEP_RENDERING_COMMA_PARAM(pDisplayRTParent)
        ));

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(*ppTextureRT);
    }
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwTextureRenderTarget::GetSurfaceDescription
//
//  Synopsis:  Computes a Render Target Texture surface description.  This
//             texture will not support wrapping.
//
//-----------------------------------------------------------------------------
HRESULT
CHwTextureRenderTarget::GetSurfaceDescription(
    UINT uWidth,
    UINT uHeight,
    __out_ecount(1) D3DSURFACE_DESC &sdLevel0
    ) const
{
    HRESULT hr = S_OK;

    sdLevel0.Format = m_d3dfmtTargetSurface;
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

    IFC(m_pD3DDevice->GetMinimalTextureDesc(
        &sdLevel0,
        TRUE,
        GMTD_NONPOW2CONDITIONAL_OK | GMTD_IGNORE_FORMAT
        ));

    //
    // Check if dimensions were too big
    //
    if (hr == S_FALSE)
    {
        IFC(WGXERR_UNSUPPORTEDTEXTURESIZE);
    }

    //
    // Check if we changed size
    //
    if (sdLevel0.Width != uWidth || sdLevel0.Height != uHeight)
    {
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }

Cleanup:
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Function:  CHwTextureRenderTarget::CHwTextureRenderTarget
//
//  Synopsis:  ctor
//
//-------------------------------------------------------------------------
CHwTextureRenderTarget::CHwTextureRenderTarget(
    __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
    MilPixelFormat::Enum fmtTarget,
    D3DFORMAT d3dfmtTarget,
    DisplayId associatedDisplay
    ) :
    CHwSurfaceRenderTarget(
        pD3DDevice,
        fmtTarget,
        d3dfmtTarget,
        associatedDisplay
        )
{
    m_pVidMemOnlyTexture = NULL;
    m_pDeviceBitmap = NULL;
    m_fInvalidContents = false;
}

//+------------------------------------------------------------------------
//
//  Function:  CHwTextureRenderTarget::~CHwTextureRenderTarget
//
//  Synopsis:  dtor
//
//-------------------------------------------------------------------------
CHwTextureRenderTarget::~CHwTextureRenderTarget()
{
#if DBG_STEP_RENDERING
    if (m_pDisplayRTParent) { m_pDisplayRTParent->Release(); }
    m_pDisplayRTParent = NULL;
#endif DBG_STEP_RENDERING


    // m_pVidMemOnlyTexture can be NULL when Init fails. It may also be invalid
    // if it has been released before the rt.
    if (m_pVidMemOnlyTexture && m_pVidMemOnlyTexture->IsValid())
    {
        // Now that the render target is no longer rendering  to the underlying bitmap, 
        // it is safe to evict it.  Even though this CHwTextureRenderTarget is going 
        // out of scope, the underlying texture may not due to a call to 
        // GetBitmapSource.        
        ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);
        
        m_pVidMemOnlyTexture->SetAsEvictable();
    }

    ReleaseInterfaceNoNULL(m_pVidMemOnlyTexture);
    ReleaseInterfaceNoNULL(m_pDeviceBitmap);
}


//+------------------------------------------------------------------------
//
//  Member:    CHwTextureRenderTarget::IsValid
//
//  Synopsis:  Returns FALSE when rendering with this render target or any
//             use is no longer allowed.  Mode change is a common cause of
//             of invalidation.
//
//-------------------------------------------------------------------------
bool
CHwTextureRenderTarget::IsValid() const
{
    return m_pVidMemOnlyTexture->IsValid();
}

//+------------------------------------------------------------------------
//
//  Member:    CHwTextureRenderTarget::GetTexture
//
//  Synopsis:  Returns the underlying video memory texture.
//
//-------------------------------------------------------------------------
CD3DVidMemOnlyTexture* 
CHwTextureRenderTarget::GetTextureNoRef()
{
    return m_pVidMemOnlyTexture;
}
//+------------------------------------------------------------------------
//
//  Function:  CHwTextureRenderTarget::HrFindInterface
//
//  Synopsis:  HrFindInterface implementation
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwTextureRenderTarget::HrFindInterface(
    __in_ecount(1) REFIID riid,
    __deref_out void **ppvObject
    )
{
    HRESULT hr = E_INVALIDARG;

    if (ppvObject)
    {
        if (riid == IID_IMILRenderTargetBitmap)
        {
            *ppvObject = static_cast<IMILRenderTargetBitmap*>(this);
        
            hr = S_OK;
        }
        else
        {
            hr = CHwSurfaceRenderTarget::HrFindInterface(riid, ppvObject);
        }
    }

    return hr;
}

//+------------------------------------------------------------------------
//
//  Function:  CHwTextureRenderTarget::Init
//
//  Synopsis:  Inits the texture render target and allocates the required
//             resources
//
//-------------------------------------------------------------------------
HRESULT
CHwTextureRenderTarget::Init(
    UINT uWidth,
    UINT uHeight
    DBG_STEP_RENDERING_COMMA_PARAM(__inout_ecount_opt(1) CHwDisplayRenderTarget *pDisplayRTParent)
    )
{
    HRESULT hr = S_OK;

    //
    // Without a cache index no efficient means is available to use the results
    // as a source. Therefore fail the call and let the caller use software. 
    //
    {
        IMILResourceCache::ValidIndex uUnusedCacheIndex;
        IFC(m_pD3DDevice->GetCacheIndex(&uUnusedCacheIndex));
    }

    //
    // Create the texture
    //

    D3DSURFACE_DESC sdLevel0;

    IFC(GetSurfaceDescription(
        uWidth,
        uHeight,
        OUT sdLevel0
        ));

    //
    // Create the texture
    //

    IFC(CD3DVidMemOnlyTexture::Create(
        &sdLevel0,	    // pSurfDesc
        1,				// uLevels
        false,			// fIsEvictable
        m_pD3DDevice,
        &m_pVidMemOnlyTexture,
        NULL			// pSharedHandle
        ));

    //
    // Remaining CHwSurfaceRenderTarget members
    //

    // None of these should be valid, yet.
    Assert(m_pD3DTargetSurface == NULL);
    Assert(m_uWidth == 0);
    Assert(m_uHeight == 0);

    //
    // Derive the render target - level 0 of texture
    //

    IFC(m_pVidMemOnlyTexture->GetD3DSurfaceLevel(
        0,
        &m_pD3DTargetSurface
        ));

    m_uWidth = uWidth;
    m_uHeight = uHeight;

    IFC(CBaseRenderTarget::Init());

    //
    // Step Rendering code
    //

#if DBG_STEP_RENDERING
    m_pDisplayRTParent = pDisplayRTParent;
    if (m_pDisplayRTParent) { m_pDisplayRTParent->AddRef(); }
#endif DBG_STEP_RENDERING

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(m_pD3DTargetSurface);
    }
    RRETURN(hr);
}


//+========================================================================
// IMILRenderTarget methods
//=========================================================================

//+----------------------------------------------------------------------------
//
//  Member:    CHwTextureRenderTarget::GetBounds
//
//  Synopsis:  Delegate to CHwSurfaceRenderTarget::GetBounds
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(VOID)
CHwTextureRenderTarget::GetBounds(
    __out_ecount(1) MilRectF * const pBounds
    )
{
    return CHwSurfaceRenderTarget::GetBounds(pBounds);
}

//+------------------------------------------------------------------------
//
//  Member:    CHwTextureRenderTarget::Clear
//
//  Synopsis:  Delegate to CHwSurfaceRenderTarget
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwTextureRenderTarget::Clear(
    __in_ecount_opt(1) const MilColorF *pColor,
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip
    )
{
    return CHwSurfaceRenderTarget::Clear(pColor, pAliasedClip);
}

//+------------------------------------------------------------------------
//
//  Member:    CHwTextureRenderTarget::Begin3D
//
//  Synopsis:  Delegate to CHwSurfaceRenderTarget
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwTextureRenderTarget::Begin3D(
    __in_ecount(1) MilRectF const &rcBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    bool fUseZBuffer,
    FLOAT rZ
    )
{
    return CHwSurfaceRenderTarget::Begin3D(rcBounds, AntiAliasMode, fUseZBuffer, rZ);
}

//+------------------------------------------------------------------------
//
//  Member:    CHwTextureRenderTarget::End3D
//
//  Synopsis:  Delegate to CHwSurfaceRenderTarget if enabled
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwTextureRenderTarget::End3D(
    )
{
    return CHwSurfaceRenderTarget::End3D();
}


//+========================================================================
// IMILRenderTargetBitmap methods
//=========================================================================

//+----------------------------------------------------------------------------
//
//  Function:  CHwTextureRenderTarget::GetBitmapSource
//
//  Synopsis:  Return a bitmap source interface that enables access to the
//             "cached" bitmap
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CHwTextureRenderTarget::GetBitmap(
    __deref_out_ecount(1) IWGXBitmap ** const ppIBitmap
    )
{
    HRESULT hr = S_OK;
	
    CDeviceBitmap *pDeviceBitmap = NULL;
    CHwBitmapCache *pBitmapCache = NULL;
    CHwDeviceBitmapColorSource *pDeviceBitmapColorSource = NULL;

    CMilRectU rcSurfBounds(0, 0, m_uWidth, m_uHeight, LTRB_Parameters);
    
    if (m_pDeviceBitmap == NULL)
    {
        ENTER_DEVICE_FOR_SCOPE(*m_pD3DDevice);

        IFC(CDeviceBitmap::Create(
            m_uWidth,
            m_uHeight,
            m_fmtTarget,
            OUT pDeviceBitmap
            ));
    
        //
        // Check for a bitmap cache.  Create and store one if it doesn't exist.
        //
        // Normally caching is optional, but in this case it is required, since
        // failure caching means we won't have access to the device bitmap
        // surface later when it is used as source.
        //
    
        IFC(CHwBitmapCache::GetCache(
            m_pD3DDevice,
            pDeviceBitmap,
            NULL,
            true, 			// fSetResourceRequired
            &pBitmapCache
            ));
    
        // Create the color source and put it in the cache
        IFC(pBitmapCache->CreateColorSourceForTexture(
            m_fmtTarget,
            rcSurfBounds,           // rcBoundsRequired
            m_pVidMemOnlyTexture,
            OUT &pDeviceBitmapColorSource
            ));
    
        // Add the color source to the bitmap's collection of color sources
        IFC(pDeviceBitmap->SetDeviceBitmapColorSource(
            NULL,                   // hShared
            pDeviceBitmapColorSource
            ));

        // Let the bitmap know that the device bitmap color source contains
        // fully updated (non-dirty) bits. The surface is considered fully
        // updated since this call to GetBitmapSource signals that rendering is
        // now complete.
        pDeviceBitmap->AddUpdateRect(
            rcSurfBounds
            );

        m_fInvalidContents = false;
        
        m_pDeviceBitmap = pDeviceBitmap;
        pDeviceBitmap = NULL; // steal ref
    }

    // If we've drawn into this texture, update the cached device bitmap.
    if (m_fInvalidContents)
    {
        m_pDeviceBitmap->AddUpdateRect(
            rcSurfBounds
            );
        
        m_fInvalidContents = false;
    }
    
    *ppIBitmap = m_pDeviceBitmap;
    m_pDeviceBitmap->AddRef();

Cleanup:
    ReleaseInterfaceNoNULL(pDeviceBitmap);
    ReleaseInterfaceNoNULL(pBitmapCache);
    ReleaseInterfaceNoNULL(pDeviceBitmapColorSource);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Function:  CHwTextureRenderTarget::GetCacheableBitmapSource
//
//  Synopsis:  Return a bitmap source interface that enables access to the
//             "cached" bitmap.
//
//             The interface we return is cachable because it does not hold on
//             the D3D device.
//
//-----------------------------------------------------------------------------
STDMETHODIMP
CHwTextureRenderTarget::GetCacheableBitmapSource(
    __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
    )
{
    RRETURN(GetBitmapSource(ppIBitmapSource));
}

//+------------------------------------------------------------------------
//
//  Function:  CHwTextureRenderTarget::GetBitmapSource
//
//  Synopsis:  Returns a device bitmap.
//
//-------------------------------------------------------------------------
STDMETHODIMP
CHwTextureRenderTarget::GetBitmapSource(
    __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
    )
{
    HRESULT hr = S_OK;
    
    IWGXBitmap *pIBitmap = NULL;
    IFC(GetBitmap(&pIBitmap));

    // Transfer ref
    *ppIBitmapSource = pIBitmap;
    pIBitmap = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pIBitmap);
    RRETURN(hr);
}

//+------------------------------------------------------------------------
//
//  Member:  CHwTextureRenderTarget::GetNumQueuedPresents
//
//  Synopsis:  Forward the call to the CMetaRenderTarget member.
//
//-------------------------------------------------------------------------
HRESULT
CHwTextureRenderTarget::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    RRETURN(CHwSurfaceRenderTarget::GetNumQueuedPresents(
        puNumQueuedPresents
        ));
}

//
// IRenderTargetInternal overrides.
//
// Since we might re-use this texture over multiple frames for visual caching,
// we need to invalidate the cached DeviceBitmap source whenever we update the 
// texture's contents.  See GetBitmapSource().  We then delegate the drawing 
// calls to the base case.
//
STDMETHODIMP 
CHwTextureRenderTarget::DrawBitmap(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) IWGXBitmapSource *pIBitmap,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    m_fInvalidContents = true;
    RRETURN(CHwSurfaceRenderTarget::DrawBitmap(
        pContextState,
        pIBitmap,
        pIEffect));
}

STDMETHODIMP 
CHwTextureRenderTarget::DrawMesh3D(
    __inout_ecount(1) CContextState* pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) CMILMesh3D* pMesh3D,
    __inout_ecount_opt(1) CMILShader* pShader,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    m_fInvalidContents = true;
    RRETURN(CHwSurfaceRenderTarget::DrawMesh3D(
        pContextState,
        pBrushContext,
        pMesh3D,
        pShader,
        pIEffect));
}

STDMETHODIMP 
CHwTextureRenderTarget::DrawPath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount_opt(1) BrushContext *pBrushContext,
    __inout_ecount(1) IShapeData *pPath,
    __inout_ecount_opt(1) CPlainPen *pPen,
    __inout_ecount_opt(1) CBrushRealizer *pStrokeBrush,
    __inout_ecount_opt(1) CBrushRealizer *pFillBrush
    )
{
    m_fInvalidContents = true;
    RRETURN(CHwSurfaceRenderTarget::DrawPath(
        pContextState,
        pBrushContext,
        pPath,
        pPen,
        pStrokeBrush,
        pFillBrush));
}

STDMETHODIMP 
CHwTextureRenderTarget::DrawInfinitePath(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount(1) BrushContext *pBrushContext,
    __inout_ecount(1) CBrushRealizer *pFillBrush
    )
{
    m_fInvalidContents = true;
    RRETURN(CHwSurfaceRenderTarget::DrawInfinitePath(
        pContextState,
        pBrushContext,
        pFillBrush));
}
    
STDMETHODIMP 
CHwTextureRenderTarget::ComposeEffect(
    __inout_ecount(1) CContextState *pContextState,
    __in_ecount(1) CMILMatrix *pScaleTransform,
    __inout_ecount(1) CMilEffectDuce* pEffect,
    UINT uIntermediateWidth,
    UINT uIntermediateHeight,
    __in_opt IMILRenderTargetBitmap* pImplicitInput
    )
{
    m_fInvalidContents = true;
    RRETURN(CHwSurfaceRenderTarget::ComposeEffect(
        pContextState,
        pScaleTransform,
        pEffect,
        uIntermediateWidth,
        uIntermediateHeight,
        pImplicitInput
        ));
}

STDMETHODIMP 
CHwTextureRenderTarget::DrawGlyphs(
    __inout_ecount(1) DrawGlyphsParameters &pars
    )
{
    m_fInvalidContents = true;
    RRETURN(CHwSurfaceRenderTarget::DrawGlyphs(pars));
}

STDMETHODIMP 
CHwTextureRenderTarget::DrawVideo(
    __inout_ecount(1) CContextState *pContextState,
    __inout_ecount_opt(1) IAVSurfaceRenderer *pSurfaceRenderer,
    __inout_ecount_opt(1) IWGXBitmapSource *pIBitmapSource,
    __inout_ecount_opt(1) IMILEffectList *pIEffect
    )
{
    m_fInvalidContents = true;
    RRETURN(CHwSurfaceRenderTarget::DrawVideo(
        pContextState,
        pSurfaceRenderer,
        pIBitmapSource,
        pIEffect));
}


