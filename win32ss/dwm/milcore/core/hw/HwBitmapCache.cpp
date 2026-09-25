// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+---------------------------------------------------------------------------
//

//
//  Description:     
//      Contains CHwBitmapCache implementation
//

#include "precomp.hpp"

MtDefine(CHwBitmapCache, MILRender, "CHwBitmapCache");
MtDefine(CHwBitmapCache_FormatCacheEntry, MILRender, "CHwBitmapCache_FormatCacheEntry");
MtDefine(CHwBitmapCache_CacheEntryList, MILRender, "CHwBitmapCache_CacheEntryList");
MtDefine(D3DResource_HwBitmapCache, MILHwMetrics, "Approximate hw bitmap cache sizes");

DeclareTag(tagLimitBitmapSizeCache, "MIL", "Limit number of uniquely sized bitmap realizations in a cache");


#if DBG
//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::CacheEntryList::c_DbgMaxExpectedCacheGrowth
//
//  Synopsis:  Arbitrary limit we don't expect caching to exceed.  This some
//             fudge of the number of prefilter cases (say 4) times the number
//             of adapters supported by a single device (say 3) plus wiggle
//             room (say 3).  4 * 3 + 3 = 15.  From experience with partially
//             implemented caching logic we've between 18 and 26 entries
//             indicating a problem.
//
//-----------------------------------------------------------------------------
const UINT CHwBitmapCache::CacheEntryList::c_DbgMaxExpectedCacheGrowth = 15;
#endif DBG


//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapCache::RetrieveFromBitmapSource
//
//  Synopsis:
//      Extract an IWGXBitmap and CHwBitmapCache from an 
//      IWGXBitmapSource
//

/* static */ HRESULT
CHwBitmapCache::RetrieveFromBitmapSource(
    __inout_ecount(1) IWGXBitmapSource *pBitmapSource,
    __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
    __deref_out_ecount_opt(1) IWGXBitmap **ppBitmapNoRef,
    __deref_out_ecount_opt(1) CHwBitmapCache **ppHwBitmapCache
    )
{
    HRESULT hr = S_OK;

    IMILResourceCache *pIMILResourceCache = NULL;

    *ppBitmapNoRef        = NULL;
    *ppHwBitmapCache      = NULL;

    hr = pBitmapSource->QueryInterface(IID_IWGXBitmap, reinterpret_cast<void**>(ppBitmapNoRef));

    if (SUCCEEDED(hr))
    {
        // The ppBitmapNoRef out is NoRef so release here to undo the QI AddRef
        (*ppBitmapNoRef)->Release();
    }

    hr = pBitmapSource->QueryInterface(
        IID_IMILResourceCache,
        reinterpret_cast<void **>(&pIMILResourceCache)
        );

    if (SUCCEEDED(hr))
    {
        IMILResourceCache::ValidIndex uCacheIndex;

        hr = pDevice->GetCacheIndex(
            &uCacheIndex
            );

        if (SUCCEEDED(hr))
        {
            IMILCacheableResource *pIMILCachedResource = NULL;

            hr = pIMILResourceCache->GetResource(
                uCacheIndex,
                &pIMILCachedResource
                );

            // Cast to specific type and transfer reference
            *ppHwBitmapCache = DYNCAST(CHwBitmapCache, pIMILCachedResource);
        }
    }
    //
    // We expect success except when using CDummySource
    //
    else
    {
        *ppBitmapNoRef        = NULL;
        *ppHwBitmapCache      = NULL;
        hr = S_OK;
    }

    ReleaseInterface(pIMILResourceCache);

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::GetBitmapColorSource
//
//  Synopsis:  Get a HW Bitmap Color Source for the given bitmap and context
//

HRESULT
CHwBitmapCache::GetBitmapColorSource(
    __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
    __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
    __inout_ecount_opt(1) IWGXBitmap *pBitmap,
    __inout_ecount(1) CHwBitmapColorSource::CacheParameters &oParams,
    __in_ecount(1) const CHwBitmapColorSource::CacheContextParameters &oContextCacheParameters,
    __inout_ecount_opt(1) CHwBitmapCache *pBitmapCache,
    __deref_out_ecount(1) CHwBitmapColorSource * &pbcs,
    __deref_out_ecount_opt(1) CHwBitmapColorSource * &pbcsWithReusableRealizationSource,
    __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate
    )
{
    HRESULT hr = S_OK;

    if (pBitmapCache)
    {
        pBitmapCache->AddRef();
    }
    else
    {
        MIL_THR(GetCache(
            pDevice,
            pBitmap,
            pICacheAlternate,
            /* fSetResourceRequired = */ false,
            &pBitmapCache
            ));
    }

    if (SUCCEEDED(hr))
    {
        IFC(pBitmapCache->ChooseBitmapColorSource(pIBitmapSource,
                                                  oParams,
                                                  oContextCacheParameters,
                                                  pbcs,
                                                  pbcsWithReusableRealizationSource
                                                  ));
    }
    else
    {
        pbcsWithReusableRealizationSource = NULL;

        IFC(CHwBitmapColorSource::Create(
            pDevice,
            pBitmap,
            oParams,
            false,
            &pbcs
            ));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pBitmapCache);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapCache::TryForDeviceBitmapOrLastUsedBitmapColorSource
//
//  Synopsis:
//      See if a device bitmap or the last used color source can be used in given
//      context.
//
//-----------------------------------------------------------------------------
void
CHwBitmapCache::TryForDeviceBitmapOrLastUsedBitmapColorSource(
    __in_ecount(1) const CHwBitmapColorSource::CacheContextParameters &oContextCacheParameters,
    __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> &rcRealizationBounds,
    __in_ecount(1) CMILBrushBitmap *pBitmapBrush,
    __deref_out_ecount_opt(1) CHwBitmapColorSource * & pbcs,
    __deref_out_ecount_opt(1) CHwBitmapColorSource * & pReusableRealizationSourcesList
    )
{
    pbcs = NULL;

    //
    // If there is a device bitmap color source here, then exactly one of them
    // may be returned.
    //
    if (m_pDeviceBitmapColorSource != NULL) 
    {
        TryForDeviceBitmapColorSource(
            oContextCacheParameters,
            rcRealizationBounds,
            pBitmapBrush,
            pbcs
            );
    }

    if (!pbcs)
    {
        TryForLastUsedBitmapColorSource(
            oContextCacheParameters,
            rcRealizationBounds,
            pBitmapBrush,
            pbcs,
            pReusableRealizationSourcesList
            );
    }
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapCache::TryForDeviceBitmapColorSource
//
//  Synopsis:
//      See if a device bitmap color source can be used.
//
//-----------------------------------------------------------------------------
void
CHwBitmapCache::TryForDeviceBitmapColorSource(
    __in_ecount(1) const CHwBitmapColorSource::CacheContextParameters &oContextCacheParameters,
    __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> &rcRealizationBounds,
    __in_ecount(1) CMILBrushBitmap *pBitmapBrush,
    __deref_out_ecount_opt(1) CHwBitmapColorSource * & pbcs
    )
{
    Assert(pbcs == NULL);
    Assert(m_pBitmap == pBitmapBrush->GetTextureNoAddRef());

    if (m_pBitmap->SourceState() == IWGXBitmap::SourceState::DeviceBitmap)
    {
        CDeviceBitmap *pBitmap =
            DYNCAST(CDeviceBitmap, m_pBitmap);
        Assert(pBitmap);

        CMilRectU rcReqBounds;

        if (CHwBitmapColorSource::ComputeMinimumRealizationBounds(
            pBitmap,
            rcRealizationBounds,
            oContextCacheParameters,
            rcReqBounds
            ))
        {
            if (pBitmap->ContainsValidArea(rcReqBounds))
            {
                // We've found a valid DBCS containing the area we need. However, we need to see
                // if the desired wrap mode is possible given the current DBCS.
                
                D3DTEXTUREADDRESS taU, taV;
                CHwTexturedColorSource::ConvertWrapModeToTextureAddressModes(
                    oContextCacheParameters.wrapMode,
                    &taU,
                    &taV
                    );

                UINT uWidth, uHeight;
                if (SUCCEEDED(m_pBitmap->GetSize(&uWidth, &uHeight)))
                {
                    if (   (IS_POWER_OF_2(uWidth) && IS_POWER_OF_2(uHeight))
                         || (m_pDevice->SupportsNonPow2Conditionally() && taU == D3DTADDRESS_CLAMP && taV == D3DTADDRESS_CLAMP)
                         || m_pDevice->SupportsNonPow2Unconditionally()
                       )
                    {
                        pbcs = m_pDeviceBitmapColorSource;
                        pbcs->SetWrapModes(taU, taV);
                    }
                    // else pbcs will be NULL and later we'll create a new BCS that can tile 
                    // correctly and pull from the CDeviceBitmap through software via
                    // IWGXBitmap::Lock() and/or IWGXBitmapSource::CopyPixels()
                }
            }
        }
    }
    else if (m_pDeviceBitmapColorSource)
    {
        //
        // Note brush uniqueness is not checked because device bitmaps must
        // always be cached on a C*Bitmap which means changes to brush
        // uniqueness (new bitmap source selected) don't matter.
        //
        // Other realization context parameters are also ignored in favor of
        // using the device bitmap.  Other context parameters ignored:
        //  - Prefiltering
        //  - MipMapping
        //  - Render Target preferred realization format
        //  - Wrap Mode
        //  - Color key

        //
        // Make sure sufficient area of source is realized.
        //

        if (m_pDeviceBitmapColorSource->CheckRequiredRealizationBounds(
                rcRealizationBounds,
                oContextCacheParameters.interpolationMode,
                oContextCacheParameters.wrapMode,
                CHwBitmapColorSource::RequiredBoundsCheck::CheckCached
                )
           )
        {
            pbcs = m_pDeviceBitmapColorSource;
        }
    }

    if (pbcs)
    {
        pbcs->AddRef();
    }
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::TryForLastUsedBitmapColorSource
//
//  Synopsis:  See if last used color source can be re-used.
//
//-----------------------------------------------------------------------------
void
CHwBitmapCache::TryForLastUsedBitmapColorSource(
    __in_ecount(1) const CHwBitmapColorSource::CacheContextParameters &oContextCacheParameters,
    __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> &rcRealizationBounds,
    __in_ecount(1) CMILBrushBitmap *pBitmapBrush,
    __deref_out_ecount_opt(1) CHwBitmapColorSource * & pbcs,
    __deref_out_ecount_opt(1) CHwBitmapColorSource * & pReusableRealizationSourcesList
    )
{
    Assert(pbcs == NULL);
    Assert(pReusableRealizationSourcesList == NULL);

    if (m_pLastUsedColorSource == NULL)
    {
        return;
    }

    // Check uniqueness to see if bitmap source selection may have changed.
    if (oContextCacheParameters.nBitmapBrushUniqueness != m_oLastUsedCacheParameters.nBitmapBrushUniqueness)
    {
        // The uniqueness count changed, we cannot use the cached color source
        return;
    }

    //
    // We can only re-use textures if prefiltering was not enabled or if we
    // generated mipmaps.
    //
    // Future Consideration:  Could calculate the destination rect for prefiltering, but the source
    // rect code adds complication here that I don't want to deal with now.
    // NOTICE-2006/08/17-JasonHa  Required bounds are checked below and are not
    // a problem.  What would need to be matched is prefilter scale factor.
    // Just let regular cache look up take that into account.
    //

    Assert(   m_oLastUsedCacheParameters.fPrefilterEnable == false
           || DoesUseMipMapping(m_oLastUsedCacheParameters.interpolationMode)
              );

    //
    // If one was prefiltered and the other wasn't, we can't reuse the texture
    // unless mip mapping is required and last used and original bitmap width
    // and height are powers of two.  See
    // CHwBitmapColorSource::ComputeRealizationParameters and
    // CHwBitmapColorSource::ComputeRealizationSize.
    //
    if (m_oLastUsedCacheParameters.fPrefilterEnable != oContextCacheParameters.fPrefilterEnable)
    {
        //
        // Rather than check if mip mapping and if orginal bitmap dimensions
        // are powers of two here (the latter is unlikely), just fallback to
        // regular cache lookup.
        //
        return;
    }

    //
    // If one was mipmapped and the other wasn't, we can't reuse the texture.
    //
    if (   DoesUseMipMapping(oContextCacheParameters.interpolationMode) !=
           DoesUseMipMapping(m_oLastUsedCacheParameters.interpolationMode)
        )
    {
        return;
    }

    Assert(   oContextCacheParameters.fPrefilterEnable == false
           || DoesUseMipMapping(oContextCacheParameters.interpolationMode)
              );

    //
    // If the color source isn't being derived from the same bitmap brush, we can't reuse the
    // texture.
    //
    // Brush address check doesn't guarentee uniqueness
    //  Composition often allocates a new CMILBrushBitmap for each draw and
    //  sets the same number of properties resulting in a regular
    //  uniqueness number and a strong chance that reallocating the
    //  CMILBrushBitmap will reuse the same address and fool this check. 
    //  To counteract that we can simply check the properties that matter. 
    //  Bitmap brush properties that should be checked:
    //      - Wrap Mode
    //      - Color Key
    //
    if (m_oLastUsedCacheParameters.pBitmapBrushNoRef != oContextCacheParameters.pBitmapBrushNoRef)
    {
        return;
    }

    //
    // If the rendertarget format has changed, we may need a new format for the texture.
    //
    if (m_oLastUsedCacheParameters.fmtRenderTarget != oContextCacheParameters.fmtRenderTarget) 
    {
        return;
    }

    //
    // If the wrap modes are different, then we probably need a new
    // realization.  Using the previously setup wrap mode is the symptom of
    //
    if (m_oLastUsedCacheParameters.wrapMode != oContextCacheParameters.wrapMode)
    {
        return;
    }

    //
    // Make sure sufficient area of source can be realized.  It doesn't have to
    // be realized now, but the color source must be large enough to include
    // areas required now.
    //

    if (!m_pLastUsedColorSource->CheckRequiredRealizationBounds(
        rcRealizationBounds,
        oContextCacheParameters.interpolationMode,
        oContextCacheParameters.wrapMode,
        (m_pBitmap && m_pBitmap->SourceState() == IWGXBitmap::SourceState::DeviceBitmap) ?
            CHwBitmapColorSource::RequiredBoundsCheck::CheckPossibleAndUpdateRequired :
            CHwBitmapColorSource::RequiredBoundsCheck::CheckRequired
        ))
    {
        return;
    }

    pbcs = m_pLastUsedColorSource;
    m_pLastUsedColorSource->AddRef();

    if (m_pDeviceBitmapColorSource)
    {
        AddDeviceBitmapColorSourcesToReusableList(
            IN OUT pReusableRealizationSourcesList
            );
    }

    return;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapCache::AddDeviceBitmapColorSourcesToReusableList
//
//  Synopsis:
//      Add each valid device bitmap color source to the reusable realization
//      source list.
//
//-----------------------------------------------------------------------------

void
CHwBitmapCache::AddDeviceBitmapColorSourcesToReusableList(
    __deref_inout_ecount_opt(1) CHwBitmapColorSource * &pbcsWithReusableRealizationSources
    ) const
{
    //
    // Only set as reusable if device bitmap color source is valid.
    //

    if (m_pDeviceBitmapColorSource && m_pDeviceBitmapColorSource->IsValid())
    {
        // Insert this new found color source at beginning of reusable
        // realization source list.
        m_pDeviceBitmapColorSource->AddToReusableRealizationSourceList(
            IN OUT pbcsWithReusableRealizationSources
            );
    }
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::GetCache
//
//  Synopsis:  Extract a bitmap cache from a resource cache
//
//  Notes:     If a bitmap cache doesn't currently exist in the resource cache
//             then one will be created and stored there.
//

HRESULT
CHwBitmapCache::GetCache(
    __inout_ecount(1) CD3DDeviceLevel1      *pDevice,
    __inout_ecount_opt(1) IWGXBitmap        *pBitmap,
    __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate,
    bool fSetResourceRequired,
    __deref_out_ecount(1) CHwBitmapCache    **ppHwBitmapCache
    )
{
    HRESULT hr = S_OK;

    CHwBitmapCache *pHwCache = NULL;
    IMILResourceCache *pIResourceCacheNoRef = NULL;
    IMILResourceCache::ValidIndex uCacheIndex;

    IFC(pDevice->GetCacheIndex(&uCacheIndex));
    
    if (pBitmap == NULL)
    {        
        if (pICacheAlternate)
        {
            //
            // Need to double check this, I don't believe there should be a
            // pHwBitmapCacheFromBitmap if pBitmap == NULL.
            //
    
            pIResourceCacheNoRef = pICacheAlternate;
        }
        else
        {
            //
            // Should we really leave an IFC here?  Seems like it should be an
            // assert.
            //
            IFC(E_NOTIMPL);
        }
    }
    else
    {
        IFC(pBitmap->QueryInterface(IID_IMILResourceCache, reinterpret_cast<void**>(&pIResourceCacheNoRef)));
        // Since pIResourceCacheNoRef is NoRef
        pBitmap->Release();
    }


    {
        IMILCacheableResource *pICachedResource = NULL;

        IFC(pIResourceCacheNoRef->GetResource(
            uCacheIndex,
            &pICachedResource
            ));

        // Cast to specific type and transfer reference
        pHwCache = DYNCAST(CHwBitmapCache, pICachedResource);
    }

    //
    // Check to see if we can reach here without a pICachedResource...appears we
    // can on first image realization...that makes sense...
    //
    if (pHwCache)
    {
        Assert(pHwCache->m_pDevice == pDevice);
    }

    if (!pHwCache)
    {
        pHwCache = new CHwBitmapCache(pBitmap, pDevice);
        IFCOOM(pHwCache);
        (pHwCache)->AddRef();

        // Try to save bitmap cache in resource cache
        MIL_THR(pIResourceCacheNoRef->SetResource(
            uCacheIndex,
            pHwCache
            ));

        if (FAILED(hr))
        {
            if (fSetResourceRequired)
            {
                goto Cleanup;
            }

            // Set result to success since caching success is not required.
            hr = S_OK;
        }
    }

    *ppHwBitmapCache = pHwCache;    // Steal the reference
    pHwCache = NULL;

Cleanup:

    ReleaseInterfaceNoNULL(pHwCache);
    
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::CHwBitmapCache
//
//  Synopsis:  ctor
//

CHwBitmapCache::CHwBitmapCache(
    __in_ecount(1) IWGXBitmap *pBitmap,
    __in_ecount(1) CD3DDeviceLevel1 *pDevice
    ) : m_pBitmap(pBitmap),
        m_pDevice(pDevice),
        m_pDeviceBitmapColorSource(NULL),
        m_oLastUsedCacheParameters(/* fInitializeNoMembers = */ true)
{
    Init(m_pDevice->GetResourceManager(), 0);

#if DBG
    // We only set the source here to enable an assertion in
    // ChooseBitmapColorSource that the bitmap source doesn't change when there
    // is a CWGXBitmap.  Never AddRef.
    m_pIBitmapSource = pBitmap;
#else
    m_pIBitmapSource = NULL;
#endif

    m_pLastUsedColorSource = NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::~CHwBitmapCache
//
//  Synopsis:  dtor
//

CHwBitmapCache::~CHwBitmapCache()
{
    ReleaseInterfaceNoNULL(m_pDeviceBitmapColorSource);
    ReleaseInterfaceNoNULL(m_pLastUsedColorSource);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::ChooseBitmapColorSource
//
//  Synopsis:  Select a bitmap color source from this cache that suits the
//             given conext creating a new bitmap color source as needed
//

HRESULT
CHwBitmapCache::ChooseBitmapColorSource(
    __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
    __inout_ecount(1) CHwBitmapColorSource::CacheParameters &oParams,
    __in_ecount(1) const CHwBitmapColorSource::CacheContextParameters &oContextCacheParameters,    
    __deref_out_ecount_opt(1) CHwBitmapColorSource * &pbcs,
    __deref_out_ecount_opt(1) CHwBitmapColorSource * &pbcsWithReusableRealizationSources
    )
{
    HRESULT hr = S_OK;

    Assert(pIBitmapSource);

    Assert(m_pDevice);

    //
    // Start with no color source and no reusable sources.
    //

    pbcs = NULL;
    pbcsWithReusableRealizationSources = NULL;

    //
    // If the source interface is different then there is no content of value
    // in the cache.  So clean it out.
    //
    // Note that if it become valueable to keep the resource around to avoid
    // texture reallocation, then
    //  1) the assertion is still okay and
    //  2) the color source will have to updated to expect changing sources
    //

    if (m_pIBitmapSource != pIBitmapSource)
    {
        Assert(!m_pBitmap);

        // No need to destroy if this is the first use
        if (m_pIBitmapSource != NULL)
        {
            CleanCache();
        }

        // Remember source association
        m_pIBitmapSource = pIBitmapSource;
    }

    m_oCachedEntryList.GetSetBitmapColorSource(IN OUT oParams, OUT pbcs, IN OUT &pbcsWithReusableRealizationSources);

    //
    // If there are device bitmaps then they can be sources for at least part of the required 
    // realizations. Add the ones that may contribute. The reusable source code can't handle
    // borders.
    //
    if (   m_pDeviceBitmapColorSource 
        && !CHwBitmapColorSource::DoesTexelLayoutHaveBorder(oParams.dlU.eLayout)
        && !CHwBitmapColorSource::DoesTexelLayoutHaveBorder(oParams.dlV.eLayout)
       )
    {
        Assert(m_pIBitmapSource == pIBitmapSource);

        AddDeviceBitmapColorSourcesToReusableList(
            IN OUT pbcsWithReusableRealizationSources
            );
    }

    if (!pbcs)
    {
        bool fCreateAsRenderTarget =
               (pbcsWithReusableRealizationSources != NULL)
            && (   m_pDevice->CanStretchRectFromTextures()
                || pbcsWithReusableRealizationSources->IsARenderTarget());

        IFC(CHwBitmapColorSource::Create(
            m_pDevice,
            m_pBitmap,
            oParams,
            fCreateAsRenderTarget,
            &pbcs
            ));

        // Try to place this new color source in the cache
        m_oCachedEntryList.GetSetBitmapColorSource(IN oParams, IN pbcs, NULL);
    }

    if (m_pLastUsedColorSource != pbcs)
    {
        // We could wait until the replace to do this release. It would depend on
        // whether we oscillated back and forth between cachable and non-cachable
        // color sources. For now, we'll clean up.
        ReleaseInterfaceNoNULL(m_pLastUsedColorSource);

        //
        // Future Consideration:  Could calculate the destination rect for prefiltering, but the source
        // rect code adds complication here that I don't want to deal with now.
        //

        if (   (oContextCacheParameters.fPrefilterEnable == false) 
            || DoesUseMipMapping(oContextCacheParameters.interpolationMode)
           )
        {
            m_pLastUsedColorSource = pbcs;
            m_oLastUsedCacheParameters = oContextCacheParameters;
            m_pLastUsedColorSource->AddRef();
        }
        else
        {
            m_pLastUsedColorSource = NULL;
        }
    }
    else
    {
        //
        // Same bitmap color source was chosen again.  There should be few
        // cases that can cause this, since matches should normally be
        // found in TryForLastUsedBitmapColorSource.  One case that can
        // match the same color source is when one size+layout works for
        // multiple wrap modes.  In that case we just want to make sure to
        // update the last used wrap mode to the most recent since
        // successful TryForLastUsedBitmapColorSource match won't set wrap
        // mode properties on bitmap color source.  One of the reasons the
        // wrap won't be set is because device bitmap color sources are
        // also returned from TryForLastUsedBitmapColorSource and they may
        // not support the given wrap mode.  Instead device bitmap match
        // is chosen in favor of wrap mode match.
        //
        //
        m_oLastUsedCacheParameters.wrapMode = oContextCacheParameters.wrapMode;
    }

Cleanup:

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::ReleaseD3DResources
//
//  Synopsis:  There are no direct D3D resources to release, but we can destroy
//             all cached realizations to free system memory
//
//             We expect that the resource manager is cleaning up all resources
//             when this is called.
//

void
CHwBitmapCache::ReleaseD3DResources()
{
    // Cache should either be unusable or unused when this is called.
    Assert(!m_fResourceValid || m_cRef == 0);
    CleanCache();
    ReleaseInterface(m_pLastUsedColorSource);
    ReleaseInterface(m_pDeviceBitmapColorSource);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapCache::CreateSharedColorSource
//
//  Synopsis:
//      Create a device bitmap color source and keep it readily available. This 
//      method creates a color source with a new texture and returns the handle
//      to that texture.
//
//-----------------------------------------------------------------------------

HRESULT CHwBitmapCache::CreateSharedColorSource(
    MilPixelFormat::Enum fmt,
    __in_ecount(1) CMilRectU const &rcBoundsRequired,
    __deref_inout_ecount(1) CHwDeviceBitmapColorSource * &pbcs,
    __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
    )
{
    HRESULT hr = S_OK;

    // Only one color source at a time
    if (m_pDeviceBitmapColorSource)
    {
        IFC(E_UNEXPECTED);
    }

    IFC(CHwDeviceBitmapColorSource::CreateWithSharedHandle(
        m_pDevice,
        m_pBitmap,
        fmt,
        rcBoundsRequired,
        OUT &pbcs,
        OUT pSharedHandle
        ));

    m_pDeviceBitmapColorSource = pbcs;
    m_pDeviceBitmapColorSource->AddRef();

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapCache::CreateBitBltColorSource
//
//  Synopsis:
//      Create a BitBlt-able device bitmap color source and keep it readily 
//      available.
//
//-----------------------------------------------------------------------------

HRESULT CHwBitmapCache::CreateBitBltColorSource(
    MilPixelFormat::Enum fmt,
    __in_ecount(1) CMilRectU const &rcBoundsRequired,
    bool fIsDependent,
    __deref_inout_ecount(1) CHwDeviceBitmapColorSource * &pbcs
    )
{
    HRESULT hr = S_OK;

    // Really, this should be asserting that it's a CInteropDeviceBitmap
    Assert(m_pBitmap->SourceState() == IWGXBitmap::SourceState::DeviceBitmap);

    // Only one color source at a time
    if (m_pDeviceBitmapColorSource)
    {
        IFC(E_UNEXPECTED);
    }

    IFC(CHwBitBltDeviceBitmapColorSource::Create(
        m_pDevice,
        m_pBitmap,
        fmt,
        rcBoundsRequired,
        fIsDependent,
        OUT &pbcs
        ));

    m_pDeviceBitmapColorSource = pbcs;
    m_pDeviceBitmapColorSource->AddRef();

Cleanup:
    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CHwBitmapCache::CreateColorSourceForTexture
//
//  Synopsis:
//      Create a device bitmap color source and keep it readily available. This 
//      method creates a color source with a preexisting texture.
//
//      To access this color source using other methods that require a shared
//      handle, a NULL handle should be used.  Note that since shared handles
//      are expected to be unique, only one color source may be created in this
//      way.
//
//-----------------------------------------------------------------------------

HRESULT CHwBitmapCache::CreateColorSourceForTexture(
    MilPixelFormat::Enum fmt,
    __in_ecount(1) CMilRectU const &rcBoundsRequired,
    __inout_ecount(1) CD3DVidMemOnlyTexture *pVidMemTexture,
    __deref_out_ecount(1) CHwDeviceBitmapColorSource **ppbcs
    )
{
    HRESULT hr = S_OK;

    // Only one at a time
    if (m_pDeviceBitmapColorSource)
    {
        IFC(E_UNEXPECTED);        }
    
    IFC(CHwDeviceBitmapColorSource::CreateForTexture(
        m_pDevice,
        m_pBitmap,
        fmt,
        rcBoundsRequired,
        pVidMemTexture,
        OUT ppbcs
        ));

    m_pDeviceBitmapColorSource = *ppbcs;
    m_pDeviceBitmapColorSource->AddRef();

Cleanup:
    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::FormatCacheEntry::FormatCacheEntry
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------

CHwBitmapCache::FormatCacheEntry::FormatCacheEntry(
    )
{
    m_fmt = MilPixelFormat::Undefined;
    m_pNext = NULL;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::FormatCacheEntry::~FormatCacheEntry
//
//  Synopsis:  dtor
//
//-----------------------------------------------------------------------------

CHwBitmapCache::FormatCacheEntry::~FormatCacheEntry(
    )
{
    delete m_pNext;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::FormatCacheEntry::GetSetBitmapColorSource
//
//  Synopsis:  Get/set a color source from/in the cache according to the
//             CacheParamters
//
//             If pbcs is NULL a color source is retrieved.  If pbsc is not
//             NULL that color source is stored in the cache replacing any
//             previous color source.
//

void
CHwBitmapCache::FormatCacheEntry::GetSetBitmapColorSource(
    __inout_ecount(1) CHwBitmapColorSource::CacheParameters &oParams,
    __deref_inout_ecount_opt(1) CHwBitmapColorSource * &pbcs,
    __deref_opt_inout_ecount_opt(1) CHwBitmapColorSource ** ppbcsWithReusableRealizationSources
    )
{
    Assert(oParams.fmtTexture != MilPixelFormat::Undefined);

    //
    // Search for supporting format entry
    //

    if (m_fmt != oParams.fmtTexture)
    {
        if (m_fmt == MilPixelFormat::Undefined)
        {
            m_fmt = oParams.fmtTexture;
        }
        else
        {
            if (!m_pNext)
            {
                m_pNext = new FormatCacheEntry();
                if (!m_pNext) goto Cleanup;
            }

            m_pNext->GetSetBitmapColorSource(oParams, pbcs, ppbcsWithReusableRealizationSources);
            goto Cleanup;
        }
    }

    //
    // Search for supporting wrap mode entry
    //

    m_oHeadWrapEntry.GetSetBitmapColorSource(oParams, pbcs, ppbcsWithReusableRealizationSources);

Cleanup:
    return;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::CacheEntryList::CacheEntryList
//
//  Synopsis:  ctor
//
//-----------------------------------------------------------------------------

CHwBitmapCache::CacheEntryList::CacheEntryList(
    )
{
#if DBG
    m_uNextEvictionIndexDbg = 0;
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::CacheEntryList::~CacheEntryList
//
//  Synopsis:  dtor
//
//-----------------------------------------------------------------------------

CHwBitmapCache::CacheEntryList::~CacheEntryList(
    )
{
    for (UINT i = 0; i < m_rgSizeEntry.GetCount(); i++)
    {
        ReleaseInterfaceNoNULL(m_rgSizeEntry[i].pbcs);
    }
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::CacheEntryList::CheckSizeLayoutMatch
//
//  Synopsis:  Check if two Size-Layout paramter structure are compatible
//

CHwBitmapCache::CacheEntryList::SizeLayoutMatch::Enum
CHwBitmapCache::CacheEntryList::CheckSizeLayoutMatch(
    __in_ecount(1) const CHwBitmapColorSource::CacheSizeLayoutParameters &oCachedParams,
    __in_ecount(1) const CHwBitmapColorSource::CacheSizeLayoutParameters &oNewParams
    )
{
    SizeLayoutMatch::Enum eMatch = SizeLayoutMatch::NoMatch;

    if (   (oCachedParams.uWidth == oNewParams.uWidth)
        && (oCachedParams.uHeight == oNewParams.uHeight)
       )
    {
        if (   !oCachedParams.fOnlyContainsSubRectOfSource
            && !CHwBitmapColorSource::DoesTexelLayoutHaveBorder(oCachedParams.dlU.eLayout)
            && !CHwBitmapColorSource::DoesTexelLayoutHaveBorder(oCachedParams.dlV.eLayout)
            && !CHwBitmapColorSource::DoesTexelLayoutHaveBorder(oNewParams.dlU.eLayout)
            && !CHwBitmapColorSource::DoesTexelLayoutHaveBorder(oNewParams.dlV.eLayout)
            )
        {
            //
            // Future Consideration:  Improve perf with reuse of sources with borders.
            // If the code in CHwBitmapColorSource::FillTextureWithTransformedSource was
            // extended to support video memory sources or sinks with border we could
            // remove the DoesTexelLayoutHaveBorder checks.
            //
            // Future Consideration:   Allow reuse with partial sources
            //  Support is in place to effectively reuse partial sources based
            //  on m_rcRequiredRealizationBounds.  What remains to be
            //  investigated is whether matching a partial first before a fully
            //  match is a common scenario and would this be detrimental.
            //
            eMatch = SizeLayoutMatch::ReusableSource;
        }

        //
        // Check for exact or partial overlap match.
        //
        // Exact matches require identical display restrictions.  Partial is
        // also not intersting unless there is an exact restriction match.
        //
        // Exact/partial require the same layout.
        //

        if (   (oCachedParams.dlU.eLayout == oNewParams.dlU.eLayout)
            && (oCachedParams.dlV.eLayout == oNewParams.dlV.eLayout))
        {
            if (oCachedParams.fOnlyContainsSubRectOfSource == oNewParams.fOnlyContainsSubRectOfSource)
            {
                //
                // If the mipmap levels of the cached bitmap are strictly greater than
                // the new one, we're ok.
                //

                //
                // Future Consideration:  Clean up old mipmap texture.
                //
                // Now we should be properly cleaning up the old mipmap texture if it
                // has less levels than the new one we're creating, but currently we
                // don't.
                //
                if (oCachedParams.eMipMapLevel >= oNewParams.eMipMapLevel)
                {
                    if (!oCachedParams.fOnlyContainsSubRectOfSource)
                    {
                        eMatch = SizeLayoutMatch::MeetsAllRequirements;
                    }
                    else
                    {
                        // Match if there is overlap
                        if (oCachedParams.rcSourceContained.DoesIntersect(
                                oNewParams.rcSourceContained
                                ))
                        {
                            // If current doesn't contain the new needs invalidate the
                            // cached realization to avoid polluting the cache.
                            if (!oCachedParams.rcSourceContained.DoesContain(
                                    oNewParams.rcSourceContained
                                    ))
                            {
                                eMatch = SizeLayoutMatch::PartialOverlap;
                            }
                            else
                            {
                                eMatch = SizeLayoutMatch::MeetsAllRequirements;
                            }
                        }
                    }
                }
            }
        }
    }

    return eMatch;
}

//+----------------------------------------------------------------------------
//
//  Member:    CHwBitmapCache::CacheEntryList::GetSetBitmapColorSource
//
//  Synopsis:  Get/set a color source from/in the cache according to the
//             CacheParamters
//
//             If pbcs is NULL a color source is retrieved.  If pbsc is not
//             NULL that color source is stored in the cache replacing any
//             previous color source.
//

void
CHwBitmapCache::CacheEntryList::GetSetBitmapColorSource(
    __inout_ecount(1) CHwBitmapColorSource::CacheParameters &oParams,
    __deref_inout_ecount_opt(1) CHwBitmapColorSource * &pbcs,
    __deref_opt_inout_ecount_opt(1) CHwBitmapColorSource ** ppbcsWithReusableRealizationSources
    )
{
    //
    // Search for supporting size entry
    //

    UINT i;

    SizeLayoutMatch::Enum eMatch = SizeLayoutMatch::NoMatch;

    for (i = 0; i < m_rgSizeEntry.GetCount(); i++)
    {
        eMatch = CheckSizeLayoutMatch(m_rgSizeEntry[i].oSizeParams, oParams);

        if (eMatch > SizeLayoutMatch::NoMatch)
        {
            // Future Consideration:   Consider PartialOverlap as reusable
            //  With introduction of m_rcRequiredRealizationBounds and
            //  supporting logic partially overlapping color sources are now
            //  reusable.  This was confirmed with a change to this routine and
            //  nothing else.  However notepad was the only app tested and it
            //  seemed to invalidate enough of itself on movement that the
            //  gains could be losses.  Better heuristics for when to actually
            //  reuse, when the reuseable source is itself out of date, could
            //  completely solve this.
            if (eMatch >= SizeLayoutMatch::PartialOverlap)
            {
                break;
            }

            Assert(eMatch == SizeLayoutMatch::ReusableSource);

            if (ppbcsWithReusableRealizationSources)
            {
                CHwBitmapColorSource *pbcsWithReusableRealizationSource =
                    m_rgSizeEntry[i].pbcs;

                // Don't check for validity because caller really wants to
                // know if there may ever be a reusable color source.  If
                // later processing made this source available then the
                // caller might be surprised.  Technically it is possible
                // for this entry to be NULL and later become not NULL and
                // reusable; so caller will have to protect against that,
                // but a validity check is still something to avoid here.
                //
                // Future Consideration:   Check for reusable validity
                //  when there are multiple possible sources, if we can
                //  find a case of this that exists.  That will make more
                //  sense when multiple realization sources are allowed or
                //  if reuse of partial intersection is implemented.
                //
                // NOTICE-2006/09/18-JasonHa  IsValid check is made by
                // CheckAndSetReusableSource.  Savings here would be less
                // AddRef/Release and list processing, but it is probably not
                // worth it the complexity of try to pick just one valid
                // source, but one invalid source if none are valid (per note
                // above).
                if (pbcsWithReusableRealizationSource)
                {
                    pbcsWithReusableRealizationSource->AddToReusableRealizationSourceList(
                        IN OUT *ppbcsWithReusableRealizationSources
                        );
                }
            }
        }
    }

    if (eMatch >= SizeLayoutMatch::PartialOverlap)
    {
        //
        // Found a match - get or set Bitmap Color Source and update size
        // parameters
        //

        {
            CacheEntry &oSizeEntry = m_rgSizeEntry[i];

            if (pbcs)
            {
                // Set - Update cache
                oSizeEntry.oSizeParams = oParams;
                ReplaceInterface(oSizeEntry.pbcs, pbcs);
            }
            else if (oSizeEntry.pbcs)
            {
                //
                // Make sure the color source is valid
                //  The color source can become invalid if the resource manager
                //  decided to have it release its resources or realization
                //  failed after it was added to the cache.
                //

                if (   eMatch != SizeLayoutMatch::PartialOverlap
                    && oSizeEntry.pbcs->IsValid())
                {
                    //
                    // Update oParams (passed in) with cached settings since
                    // those are the settings that will be used.
                    //
                    D3DTEXTUREADDRESS d3dtaU = oParams.dlU.d3dta;
                    D3DTEXTUREADDRESS d3dtaV = oParams.dlV.d3dta;

                    *static_cast<CHwBitmapColorSource::CacheSizeLayoutParameters *>
                        (&oParams) = oSizeEntry.oSizeParams;

                    //
                    // Restore two cache settings that should really be a part
                    // of InternalRealizationParameters and should not be
                    // changed.
                    //
                    oParams.dlU.d3dta = d3dtaU;
                    oParams.dlV.d3dta = d3dtaV;

                    pbcs = oSizeEntry.pbcs;
                    pbcs->AddRef();
                }
                else
                {
                    if (eMatch == SizeLayoutMatch::PartialOverlap)
                    {
                        //
                        // Update this entry to be the place holder for the
                        // realization we expect to come through shortlty.
                        //
                        // Future Consideration:   Return entry rather than re-walk cache
                        //  while direct additions could still come through
                        //  GetSet we wouldn't have to worry as much about
                        //  when to update on a Get that doesn't exactly match.
                        //
                        oSizeEntry.oSizeParams = oParams;
                    }

                    oSizeEntry.pbcs->Release();
                    oSizeEntry.pbcs = NULL;
                }
            }
        }

        if (eMatch == SizeLayoutMatch::PartialOverlap)
        {
            //
            // Walk remaining entries and invalidate matches to avoid polluting
            // cache with too many realizations.
            //
            for (i++; i < m_rgSizeEntry.GetCount(); i++)
            {
                CacheEntry &oSizeEntry = m_rgSizeEntry[i];

                eMatch = CheckSizeLayoutMatch(oSizeEntry.oSizeParams, oParams);

                // There should not be any better matches than PartialOverlap.
                Assert(eMatch != SizeLayoutMatch::MeetsAllRequirements);

                if (eMatch == SizeLayoutMatch::PartialOverlap)
                {
                    Assert(oSizeEntry.oSizeParams.fOnlyContainsSubRectOfSource);

                    ReleaseInterfaceNoNULL(oSizeEntry.pbcs);

                    UINT uNewCount = m_rgSizeEntry.GetCount()-1;
                    if (i != uNewCount)
                    {
                        Assert(i < uNewCount);
                        // Overwrite this element with last
                        m_rgSizeEntry[i] = m_rgSizeEntry.Last();
                        // reduce index to process the new entry at this index.
                        i--;
                    }
                    m_rgSizeEntry.SetCount(uNewCount);
                }
            }
        }
    }
    else
    {
        //
        // Try to create a cache location for this type of realization
        //

#if DBG
        if (   IsTagEnabled(tagLimitBitmapSizeCache)
               // Never grow beyond original/default capacity
            && m_rgSizeEntry.GetCount() == m_rgSizeEntry.GetCapacity())
        {
            m_rgSizeEntry[m_uNextEvictionIndexDbg].oSizeParams = oParams;
            ReplaceInterface(m_rgSizeEntry[m_uNextEvictionIndexDbg].pbcs, pbcs);
            m_uNextEvictionIndexDbg =
                (m_uNextEvictionIndexDbg + 1) % m_rgSizeEntry.GetCapacity();
        }
        else
#endif DBG
        {
            CacheEntry *pNewCacheEntry;

            if (SUCCEEDED(THR(m_rgSizeEntry.AddMultiple(1, &pNewCacheEntry))))
            {
                pNewCacheEntry->oSizeParams = oParams;
                pNewCacheEntry->pbcs = pbcs;

                // Add a ref count for the successfully cached bitmap color source
                if (pbcs)
                {
                    pbcs->AddRef();
                }
            }

#if DBG
            if (m_rgSizeEntry.GetCount() > c_DbgMaxExpectedCacheGrowth)
            {
                TraceTag((tagMILWarning,
                          "Over %u cached Hw realizations of a bitmap.",
                          c_DbgMaxExpectedCacheGrowth));
            }
#endif DBG
        }
    }
}






