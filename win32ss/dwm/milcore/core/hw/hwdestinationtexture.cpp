// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_effects
//      $Keywords:
//
//  $Description:
//      Contains CHwDestinationTexture implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"
using namespace dxlayer;

MtDefine(CHwDestinationTexture, MILRender, "CHwDestinationTexture");
MtDefine(D3DResource_DestinationTexture, MILHwMetrics, "Approximate destination texture sizes");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::Create
//
//  Synopsis:
//      Creates the Destination Texture
//
//------------------------------------------------------------------------------
__checkReturn HRESULT CHwDestinationTexture::Create(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) CHwDestinationTexturePool *pPoolManager,
    __deref_out_ecount(1) CHwDestinationTexture **ppHwDestinationTexture
    )
{
    HRESULT hr = S_OK;

    *ppHwDestinationTexture = new CHwDestinationTexture(
                                    pDevice,
                                    pPoolManager
                                    );
    
    IFCOOM(*ppHwDestinationTexture);

    (*ppHwDestinationTexture)->AddRef();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::CHwDestinationTexture
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------
CHwDestinationTexture::CHwDestinationTexture(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) CHwDestinationTexturePool *pPoolManager
    )
    : CMILPoolResource(pPoolManager),
      CHwTexturedColorSource(pDevice)
{
    m_pBackgroundTexture = NULL;

    ZeroMemory(&m_backgroundTextureInfo, sizeof(m_backgroundTextureInfo));
    m_fmtTexture = MilPixelFormat::Undefined;
    m_uTextureWidth = 0;
    m_uTextureHeight = 0;

    m_uCopyWidthTextureSpace = 0;
    m_uCopyHeightTextureSpace = 0;
    m_uCopyOffsetXTextureSpace = 0;
    m_uCopyOffsetYTextureSpace = 0;
    m_fValidRealization = false;

    m_pHwSurfaceRenderTarget = NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::CHwDestinationTexture
//
//  Synopsis:
//      dctor
//
//------------------------------------------------------------------------------
CHwDestinationTexture::~CHwDestinationTexture()
{
    ReleaseInterfaceNoNULL(m_pBackgroundTexture);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::Contains
//
//  Synopsis:
//      Checks to see if this destination texture can be used for the render
//      parameters supplied.
//
//------------------------------------------------------------------------------
HRESULT 
CHwDestinationTexture::Contains(
    __in_ecount(1) const CHwSurfaceRenderTarget *pHwTargetSurface,
    __in_ecount(1) const CMILSurfaceRect &rcDestRect,
    __out_ecount(1) bool *pDestinationTextureContainsNewTexture
    ) const
{
    HRESULT hr = S_OK;
    UINT uRTWidth = 0;
    UINT uRTHeight = 0;

    Assert(pHwTargetSurface == m_pHwSurfaceRenderTarget);
    
    CMILSurfaceRect rcNewSource;

    IFC(pHwTargetSurface->GetSize(
        &uRTWidth,
        &uRTHeight
        ));

    CHwDestinationTexture::CalculateSourceRect(
        uRTWidth, 
        uRTHeight, 
        rcDestRect, 
        &rcNewSource
        );

    if (   !m_rcSource.DoesContain(rcNewSource)  )
    {
        *pDestinationTextureContainsNewTexture = false;
    }
    else
    {
        *pDestinationTextureContainsNewTexture = true;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::IsOpaque
//
//  Synopsis:
//      Does the source contain alpha?  This method tells you.
//
//------------------------------------------------------------------------------

bool
CHwDestinationTexture::IsOpaque(
    ) const
{
    return !HasAlphaChannel(m_fmtTexture);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::Realize
//
//  Synopsis:
//      Quick out because this should have been already realized during a call
//      to Update.  Here only to satisfy CHwColorSource interface.
//
//------------------------------------------------------------------------------

HRESULT
CHwDestinationTexture::Realize(
    )
{
    Assert(m_fValidRealization);
    Assert(m_pBackgroundTexture);

    return S_OK;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::SendDeviceStates
//
//  Synopsis:
//      Send related texture states to the device
//

HRESULT
CHwDestinationTexture::SendDeviceStates(
    DWORD dwStage,
    DWORD dwSampler
    )
{
    HRESULT hr = S_OK;

    Assert(m_fValidRealization);
    Assert(m_pBackgroundTexture);

    IFC(CHwTexturedColorSource::SendDeviceStates(
        dwStage,
        dwSampler
        ));

    IFC(m_pDevice->SetTexture(dwSampler, m_pBackgroundTexture));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::TransformDeviceSpaceBoundsToClippedDeviceSpaceBounds
//
//  Synopsis:
//      Returns the bounds, clipping to the
//      rendertarget dimensions in device space.
//
//------------------------------------------------------------------------------
HRESULT
CHwDestinationTexture::TransformDeviceSpaceBoundsToClippedDeviceSpaceBounds(
    __in_ecount(1) const CMILSurfaceRect &rcContentBoundsDeviceSpace,
    __out_ecount(1) CMILSurfaceRect *prcInflatedContentBoundsDeviceSpace
    ) const
{
    HRESULT hr = S_OK;
    UINT uRTWidth = 0;
    UINT uRTHeight = 0;

    IFC(m_pHwSurfaceRenderTarget->GetSize(
        &uRTWidth,
        &uRTHeight
        ));

    CHwDestinationTexture::CalculateSourceRect(
        uRTWidth, 
        uRTHeight,
        rcContentBoundsDeviceSpace,
        prcInflatedContentBoundsDeviceSpace
        );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::TransformDeviceSpaceBoundsToTextureSpaceBounds
//
//  Synopsis:
//      Transforms the device space bounds to texture space bounds.
//
//------------------------------------------------------------------------------
void 
CHwDestinationTexture::TransformDeviceSpaceBoundsToTextureSpaceBounds(
    __in_ecount(1) const CMILSurfaceRect &rcBoundsDeviceSpace,
    __out_ecount(1) CMILSurfaceRect *rcBoundsTextureSpace
    ) const
{
    rcBoundsTextureSpace->left  = 
          (rcBoundsDeviceSpace.left 
        - static_cast<LONG>(m_backgroundTextureInfo.offsetDeviceSpace.x));
    
    rcBoundsTextureSpace->top    = 
          (rcBoundsDeviceSpace.top    
        - static_cast<LONG>(m_backgroundTextureInfo.offsetDeviceSpace.y));

    rcBoundsTextureSpace->right = 
          (rcBoundsDeviceSpace.right 
        - static_cast<LONG>(m_backgroundTextureInfo.offsetDeviceSpace.x));
    
    rcBoundsTextureSpace->bottom = 
          (rcBoundsDeviceSpace.bottom 
        - static_cast<LONG>(m_backgroundTextureInfo.offsetDeviceSpace.y));
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::TransformDeviceSpaceToTextureCoordinates
//
//  Synopsis:
//      Takes device space bounds and returns the texture coordinates those
//      bounds map to inside the texture.
//
//------------------------------------------------------------------------------
void 
CHwDestinationTexture::TransformDeviceSpaceToTextureCoordinates(
    __in_ecount(1) const CMilRectL &rcBounds,
    __out_ecount(1) CMilRectF *prcTextureCoordinateBounds
    ) const
{
    //
    // If there is no texture, then we haven't been updated.
    //
    Assert(m_pBackgroundTexture);
    
    prcTextureCoordinateBounds->left = 
          (rcBounds.left - m_backgroundTextureInfo.offsetDeviceSpace.x)
        * m_backgroundTextureInfo.textureSpaceMult.x;
    
    prcTextureCoordinateBounds->top  = 
          (rcBounds.top  - m_backgroundTextureInfo.offsetDeviceSpace.y)
        * m_backgroundTextureInfo.textureSpaceMult.y;
    
    prcTextureCoordinateBounds->right  = 
          (rcBounds.right  - m_backgroundTextureInfo.offsetDeviceSpace.x)
        * m_backgroundTextureInfo.textureSpaceMult.x;
    
    prcTextureCoordinateBounds->bottom = 
          (rcBounds.bottom - m_backgroundTextureInfo.offsetDeviceSpace.y)
        * m_backgroundTextureInfo.textureSpaceMult.y;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::CalculateSourceRect
//
//  Synopsis:
//      Calculates the source rect needed to render the specified dest rect.
//
//------------------------------------------------------------------------------
void 
CHwDestinationTexture::CalculateSourceRect(
    UINT uRTWidth,
    UINT uRTHeight,
    __in_ecount(1) const CMILSurfaceRect &rcDestRect,
    __out_ecount(1) CMILSurfaceRect *pSourceRect
    )
{
    CMilRectL rcSourceIdeal;

    // The dest rect should be within the render target bounds
    Assert(rcDestRect.left >= 0);
    Assert(static_cast<UINT>(rcDestRect.right) <= uRTWidth);
    Assert(rcDestRect.top >= 0);
    Assert(static_cast<UINT>(rcDestRect.bottom) <= uRTHeight);

    rcSourceIdeal = rcDestRect;

    // Calculate the Source Rectangle bound to the rendertarget size

    pSourceRect->left = max(rcSourceIdeal.left, 0L);
    pSourceRect->top  = max(rcSourceIdeal.top, 0L);
    pSourceRect->left = min(pSourceRect->left, static_cast<LONG>(uRTWidth));
    pSourceRect->top  = min(pSourceRect->top, static_cast<LONG>(uRTHeight));

    pSourceRect->right  = max(rcSourceIdeal.right, 0L);
    pSourceRect->bottom = max(rcSourceIdeal.bottom, 0L);
    pSourceRect->right  = min(pSourceRect->right, static_cast<LONG>(uRTWidth));
    pSourceRect->bottom = min(pSourceRect->bottom, static_cast<LONG>(uRTHeight));

}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::SetContents
//
//  Synopsis:
//      Calculates the size required, recreates the texture if necessary, and
//      realizes it.
//
//------------------------------------------------------------------------------
HRESULT
CHwDestinationTexture::SetContents(
    __inout_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface,
    __in_ecount(1) const CMILSurfaceRect &rcDestRect,
    __in_ecount_opt(crgSubDestCopyRects) const CMILSurfaceRect *prgSubDestCopyRects,
    UINT crgSubDestCopyRects
    )
{
    HRESULT hr = S_OK;
    MilPixelFormat::Enum fmtRT;
    UINT uRTWidth;
    UINT uRTHeight;

    IFC(pHwTargetSurface->GetSize(&uRTWidth, &uRTHeight));
    IFC(pHwTargetSurface->GetPixelFormat(&fmtRT));

    CHwDestinationTexture::CalculateSourceRect(
        uRTWidth,
        uRTHeight,
        rcDestRect,
        &m_rcSource
        );

    m_pHwSurfaceRenderTarget = pHwTargetSurface;

    //
    // Our Destination Size is the appropriate shrink factor to 1 
    // (or 1 to 1/ShrinkFactor) mapping of source.
    //
    m_uCopyWidthTextureSpace  = (m_rcSource.right - m_rcSource.left);
    m_uCopyHeightTextureSpace = (m_rcSource.bottom - m_rcSource.top);

    Assert(m_uCopyWidthTextureSpace > 0);
    Assert(m_uCopyHeightTextureSpace > 0);

    //
    // Create a texure if:
    //   1. We don't have one
    //   2. The format is wrong
    //   3. The current one is too small
    //
    if (   !m_pBackgroundTexture
        || !m_pBackgroundTexture->IsValid()
        || m_fmtTexture != fmtRT
        || m_uTextureWidth < m_uCopyWidthTextureSpace
        || m_uTextureHeight < m_uCopyHeightTextureSpace
        )
    {
        //  If you hit this Assert you have a potential perf problem.
        //   Current use of this class creates a texture pool per format- this will trigger
        //   if you try to use the same pool for different texture formats.  Setting contents
        //   to a texture of a different format than the one already cached will cause
        //   a re-allocation of texture, which defeats the caching mechanism whenever multiple 
        //   formats are drawn in a single frame.  Furthermore, the new texture is allocated at 
        //   the PREVIOUS texture's size if it's larger, which is not what a consumer of this 
        //   class would expect.
        Assert(m_pBackgroundTexture == NULL || m_fmtTexture == fmtRT);
                
        ReleaseInterface(m_pBackgroundTexture);
        
        m_uTextureWidth = max(m_uTextureWidth, m_uCopyWidthTextureSpace);
        m_uTextureHeight = max(m_uTextureHeight, m_uCopyHeightTextureSpace);

        // If our dimensions are valid create a texture
        if (m_uTextureWidth > 0 && m_uTextureHeight > 0)
        {
            D3DSURFACE_DESC sdLevel0;
            
            // We're recreating the texture, so we'll need to re-realize
            m_fValidRealization = false;

            // Future Consideration:  May not always need a RenderTarget usage

            PopulateSurfaceDesc(
                PixelFormatToD3DFormat(fmtRT),
                D3DPOOL_DEFAULT,
                D3DUSAGE_RENDERTARGET,
                m_uTextureWidth,
                m_uTextureHeight,
                &sdLevel0
                );

            // No need call GetMinimalTextureDesc since this surface is a
            // representation of the target surface which has already been
            // successfully validated.
#if DBG

            D3DSURFACE_DESC sd = sdLevel0;
            Assert(SUCCEEDED(m_pDevice->GetMinimalTextureDesc(
                &sd, 
                FALSE, 
                GMTD_IGNORE_FORMAT | GMTD_NONPOW2CONDITIONAL_OK
                )));
            
            Assert(sdLevel0.Width == sd.Width);
            Assert(sdLevel0.Height == sd.Height);

#endif

            //
            // IMPORTANT: Do not create an evictable video
            //            memory texture here. Doing so may
            //            break BeginLayer/EndLayer in low
            //            memory situations. See comment in
            //            CHwRenderTargetLayerData
            //

            IFC(CD3DVidMemOnlyTexture::Create(
                &sdLevel0,
                1,      // levels
                false,  // fIsEvictable
                m_pDevice,
                &m_pBackgroundTexture,
                /* HANDLE *pSharedHandle */ NULL
                ));

            //
            // Set filter and wrap modes.
            //

            SetFilterAndWrapModes(
                MilBitmapInterpolationMode::NearestNeighbor, 
                D3DTADDRESS_CLAMP, 
                D3DTADDRESS_CLAMP
                );
            
            // Remember format
            m_fmtTexture = fmtRT;
        }
    }


#if DBG
    //
    // At this point we cannot expect the surface to contain anything useful
    // because it could have just been created.
    // Set the entire contents to something invalid
    //
    IGNORE_HR(DbgSetContentsInvalid(
        m_pDevice
        ));
#endif

    //
    // Determine which region of the texture to copy the destination into
    // If the source rect is aligned to an edge of the render target, the copied
    // region must be justified to the same edge of the texture. This allows
    // us to use clamping to sample off the edge of the texture when necessary.
    // If the texture is not on the edge of render target we default to
    // justifying the copied region to the top left corner of the texture.
    //
    
    if (static_cast<UINT>(m_rcSource.right) >= uRTWidth)
    {
        Assert(m_uTextureWidth >= m_uCopyWidthTextureSpace);
        m_uCopyOffsetXTextureSpace = (m_uTextureWidth - m_uCopyWidthTextureSpace);
    }
    else
    {
        m_uCopyOffsetXTextureSpace = 0;
    }

    if (static_cast<UINT>(m_rcSource.bottom) >= uRTHeight)
    {
        Assert(m_uTextureHeight >= m_uCopyHeightTextureSpace);
        m_uCopyOffsetYTextureSpace = (m_uTextureHeight - m_uCopyHeightTextureSpace);
    }
    else
    {
        m_uCopyOffsetYTextureSpace = 0;
    }

    //
    // store the parameters to the pixel shader
    // in the background texture info struct
    //

    m_backgroundTextureInfo.offsetDeviceSpace = vector2(
        static_cast<float>(m_rcSource.left) - m_uCopyOffsetXTextureSpace,
        static_cast<float>(m_rcSource.top) - m_uCopyOffsetYTextureSpace
        );

    m_backgroundTextureInfo.textureSpaceMult = vector2(
        1.0f/static_cast<float>(m_uTextureWidth),
        1.0f/static_cast<float>(m_uTextureHeight)
        );


    {
        BitmapToXSpaceTransform bitmapToDevice;
    #if DBG
        bitmapToDevice.dbgXSpaceDefinition = XSpaceIsSampleSpace;
    #endif
        bitmapToDevice.matBitmapSpaceToXSpace.SetToIdentity();
    #if DBG_ANALYSIS
        bitmapToDevice.matBitmapSpaceToXSpace.DbgChangeToSpace
            <CoordinateSpace::RealizationSampling,CoordinateSpace::Device>();
    #endif
        bitmapToDevice.matBitmapSpaceToXSpace.SetDx(m_backgroundTextureInfo.offsetDeviceSpace.x);
        bitmapToDevice.matBitmapSpaceToXSpace.SetDy(m_backgroundTextureInfo.offsetDeviceSpace.y);


        IFC(CalcTextureTransform(
            &bitmapToDevice,
            m_uTextureWidth,
            m_uTextureHeight
            ));
    }

    if (prgSubDestCopyRects)
    {
        Assert(crgSubDestCopyRects > 0);
        
        for (UINT i = 0; i < crgSubDestCopyRects; i++)
        {
            CMILSurfaceRect rcSource;
            CMILSurfaceRect rcDest;

            if (!prgSubDestCopyRects[i].IsEmpty())
            {
                IFC(TransformDeviceSpaceBoundsToClippedDeviceSpaceBounds(
                    prgSubDestCopyRects[i],
                    &rcSource
                    ));

                TransformDeviceSpaceBoundsToTextureSpaceBounds(
                    rcSource, 
                    &rcDest
                    );

                IFC(UpdateSourceRect(
                    rcSource,
                    rcDest,
                    pHwTargetSurface
                    ));
            }
        }
    }
    else
    {
        CMILSurfaceRect rcDest(
            m_uCopyOffsetXTextureSpace,
            m_uCopyOffsetYTextureSpace,
            m_uCopyWidthTextureSpace,
            m_uCopyHeightTextureSpace,
            XYWH_Parameters
            );
            
        IFC(UpdateSourceRect(
            m_rcSource,
            rcDest,
            pHwTargetSurface
            ));
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::UpdateSourceRect
//
//  Synopsis:
//      Realizes the rendertarget surface
//
//------------------------------------------------------------------------------
HRESULT
CHwDestinationTexture::UpdateSourceRect(
    __in_ecount(1) CMILSurfaceRect &rcSource,
    __in_ecount(1) CMILSurfaceRect &rcDest,
    __in_ecount(1) CHwSurfaceRenderTarget *pHwTargetSurface
    )
{
    HRESULT hr = S_OK;

    Assert(m_pBackgroundTexture != NULL);

    // Our dimensions could be invalid here, we need to check

    // We CAN reach here with invalid dimensions and a valid texture.
    // We don't have to test source dimensions, because Dest was generated
    //  from them

    if (m_uTextureWidth > 0  &&
        m_uTextureHeight > 0
        )
    {
        IFC(pHwTargetSurface->PopulateDestinationTexture(
            &rcSource,
            &rcDest,
            m_pBackgroundTexture->GetD3DTextureNoRef()
            ));
    }

    m_fValidRealization = true;

Cleanup:

    RRETURN(hr);
}

#if DBG

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDestinationTexture::DbgSetContentsInvalid
//
//  Synopsis:
//      Sets the contents of this texture to some strange color
//
//------------------------------------------------------------------------------
HRESULT
CHwDestinationTexture::DbgSetContentsInvalid(
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice
    )
{
    HRESULT hr = S_OK;

    IDirect3DSurface9 *pDestSurface = NULL;


    // get the destination surface
    {
        IDirect3DTexture9 *pDestTextureNoRef = m_pBackgroundTexture->GetD3DTextureNoRef();
    
        IFC(pDestTextureNoRef->GetSurfaceLevel(
            0,
            &pDestSurface
            ));
    }

    // fill to some kind of purple
    D3DCOLOR fillColor = D3DCOLOR_ARGB(255, 255, 0, 128);

    IFC(pDevice->ColorFill(
        pDestSurface,
        NULL,
        fillColor
        ));
 
Cleanup:
    ReleaseInterfaceNoNULL(pDestSurface);

    RRETURN(hr);
}
#endif





