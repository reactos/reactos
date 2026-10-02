// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Contains CSwBitmapCache implementation which supports the
//      CMILCacheableResource interface and can store multiple bitmap
//      realizations.
//

#include "precomp.hpp"

MtDefine(CSwBitmapCache, MILRender, "CSwBitmapCache");

ExternTag(tagLimitBitmapSizeCache);


#if DBG
//+----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapCache::FormatCacheEntry::c_DbgMaxExpectedCacheGrowth
//
//  Synopsis:
//      Arbitrary limit we don't expect caching to exceed.  This is a fudge
//      from the number of prefilter cases (say 5) plus wiggle room (say 2).
//          5 + 2 = 6.
//
//-----------------------------------------------------------------------------

const UINT CSwBitmapCache::FormatCacheEntry::c_DbgMaxExpectedCacheGrowth = 7;
#endif DBG


//+----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapCache::GetBitmapColorSource
//
//  Synopsis:
//      Get a Sw Bitmap Color Source for the given bitmap and context
//

HRESULT
CSwBitmapCache::GetBitmapColorSource(
    __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
    __inout_ecount(1) IWGXBitmap *pBitmap,
    __inout_ecount(1) CSwBitmapColorSource::CacheParameters &oParams,
    __deref_out_ecount(1) CSwBitmapColorSource * &pbcs,
    __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate
    )
{
    HRESULT hr = S_OK;

    CSwBitmapCache *pBitmapCache = NULL;

    IFC(GetCache(
        pBitmap,
        pICacheAlternate,
        &pBitmapCache
        ));

    IFC(pBitmapCache->ChooseBitmapColorSource(pIBitmapSource,
                                              oParams,
                                              pbcs
                                              ));

Cleanup:
    ReleaseInterfaceNoNULL(pBitmapCache);

    RRETURN(hr);
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapCache::GetCache
//
//  Synopsis:
//      Extract a bitmap cache from a resource cache
//
//  Notes:
//      If a bitmap cache doesn't currently exist in the resource cache then
//      one will be created and stored there.
//

HRESULT
CSwBitmapCache::GetCache(
    __inout_ecount_opt(1) IWGXBitmap        *pBitmap,
    __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate,
    __deref_out_ecount(1) CSwBitmapCache    **ppSwBitmapCache
    )
{
    HRESULT hr = S_OK;

    IMILCacheableResource *pICachedResource = NULL;
    CSwBitmapCache *pSwCache = NULL;
    IMILResourceCache *pIResourceCacheNoRef = NULL;
    
    if (!pBitmap || FAILED(pBitmap->QueryInterface(IID_IMILResourceCache, reinterpret_cast<void**>(&pIResourceCacheNoRef))))
    {
        SetInterface(pIResourceCacheNoRef, pICacheAlternate);
    }
    // Undo the ref we just added
    ReleaseInterfaceNoNULL(pIResourceCacheNoRef);

    if (!pIResourceCacheNoRef)
    {
        // No cache is available
        IFC(WGXERR_GENERIC_IGNORE);
    }

    IFC(pIResourceCacheNoRef->GetResource(
        CMILResourceCache::SwRealizationCacheIndex,
        &pICachedResource
        ));

    //
    // Note: May reach here without a pICachedResource on first image
    //       realization.
    //

    if (pICachedResource)
    {
        pSwCache = DYNCAST(CSwBitmapCache, pICachedResource); // Transfer ref
        pICachedResource = NULL;
    }
    else
    {
        pSwCache = new CSwBitmapCache(pBitmap);
        IFCOOM(pSwCache);
        (pSwCache)->AddRef();

        // Try to save bitmap cache in resource cache
        IGNORE_HR(pIResourceCacheNoRef->SetResource(
            CMILResourceCache::SwRealizationCacheIndex,
            pSwCache
            ));
    }

    *ppSwBitmapCache = pSwCache;    // Steal the reference
    pSwCache = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pICachedResource);
    ReleaseInterfaceNoNULL(pSwCache);
    
    RRETURN(hr);
}



//+----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapCache::CSwBitmapCache
//
//  Synopsis:
//      ctor
//

CSwBitmapCache::CSwBitmapCache(
    __in_ecount_opt(1) IWGXBitmap *pBitmap
    ) : m_pBitmap(pBitmap)
{
#if DBG
    // We only set the source here to enable an assertion in
    // ChooseBitmapColorSource that the bitmap source doesn't change when there
    // is an IWGXBitmap.  Never AddRef.
    m_pIBitmapSourceNoRef = pBitmap;
#else
    m_pIBitmapSourceNoRef = NULL;
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapCache::~CSwBitmapCache
//
//  Synopsis:
//      dtor
//
//-----------------------------------------------------------------------------

CSwBitmapCache::~CSwBitmapCache()
{
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapCache::ChooseBitmapColorSource
//
//  Synopsis:
//      Select a bitmap color source from this cache that suits the given
//      context creating a new bitmap color source as needed
//

HRESULT
CSwBitmapCache::ChooseBitmapColorSource(
    __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
    __inout_ecount(1) CSwBitmapColorSource::CacheParameters &oParams,
    __out_ecount(1) CSwBitmapColorSource * &pbcs
    )
{
    HRESULT hr = S_OK;

    pbcs = NULL;

    //
    // If the source interface is different then there is no content of value
    // in the cache.  So clean it out.
    //
    // Note that if it becomes valuable to keep the resource around to avoid
    // texture reallocation, then
    //  1) the assertion is still okay and
    //  2) the color source will have to updated to expect changing sources
    //

    if (m_pIBitmapSourceNoRef != pIBitmapSource)
    {
        Assert(!m_pBitmap);

        // No need to destroy if this is the first use
        if (m_pIBitmapSourceNoRef != NULL)
        {
            CleanCache();
        }

        // Remember source association
        m_pIBitmapSourceNoRef = pIBitmapSource;
    }

    FormatCacheEntry &oFormatCachedEntry =
        m_rgFormatCachedEntry[oParams.fmtTexture == MilPixelFormat::PRGBA128bppFloat ? 1 : 0];

    oFormatCachedEntry.GetSetBitmapColorSource(IN OUT oParams, OUT pbcs);

    if (!pbcs)
    {
        IFC(CSwBitmapColorSource::Create(m_pBitmap, &pbcs));

        // Try to place this new color source in the cache
        oFormatCachedEntry.GetSetBitmapColorSource(IN oParams, IN pbcs);
    }

Cleanup:

    RRETURN(hr);
}


//+----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapCache::FormatCacheEntry::FormatCacheEntry
//
//  Synopsis:
//      ctor
//
//-----------------------------------------------------------------------------

CSwBitmapCache::FormatCacheEntry::FormatCacheEntry(
    )
{
    m_fmt = MilPixelFormat::Undefined;
#if DBG
    m_uNextEvictionIndexDbg = 0;
#endif
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapCache::FormatCacheEntry::~FormatCacheEntry
//
//  Synopsis:
//      dtor
//
//-----------------------------------------------------------------------------

CSwBitmapCache::FormatCacheEntry::~FormatCacheEntry(
    )
{
    for (UINT i = 0; i < m_rgSizeLayoutEntry.GetCount(); i++)
    {
        ReleaseInterfaceNoNULL(m_rgSizeLayoutEntry[i].pbcs);
    }
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapCache::FormatCacheEntry::CheckSizeLayoutMatch
//
//  Synopsis:
//      Check if two Size-Layout parameter structures are compatible
//

bool
CSwBitmapCache::FormatCacheEntry::CheckSizeLayoutMatch(
    __in_ecount(1) CSwBitmapColorSource::CacheSizeLayoutParameters &oCachedParams,
    __in_ecount(1) CSwBitmapColorSource::CacheSizeLayoutParameters &oNewParams,
    __out_ecount(1) bool &fNewParamsNotContained
    )
{
    bool fMatch = false;

    //  jordanpa  This means fForceInvalidation is always false so we could delete a bunch of code
    fNewParamsNotContained = false;

    if ( (oCachedParams.uWidth == oNewParams.uWidth) &&
         (oCachedParams.uHeight == oNewParams.uHeight) )
    {
        Assert(oCachedParams.fOnlyContainsSubRectOfSource ==
               oNewParams.fOnlyContainsSubRectOfSource);

        //  jordanpa  If we never want to add sub rect realization, we could delete more code
        AssertConstMsg(
            !oCachedParams.fOnlyContainsSubRectOfSource, 
            "oCachedParams.fOnlyContainsSubRectOfSource is unexpectedly true."
            );

        fMatch = true;
    }

    return fMatch;
}

//+----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapCache::FormatCacheEntry::GetSetBitmapColorSource
//
//  Synopsis:
//      Get/set a color source from/in the cache according to the
//      CacheParameters
//
//      If pbcs is NULL a color source is retrieved.  If pbcs is not NULL that
//      color source is stored in the cache replacing any previous color
//      source.
//

void
CSwBitmapCache::FormatCacheEntry::GetSetBitmapColorSource(
    __inout_ecount(1) CSwBitmapColorSource::CacheParameters &oParams,
    __deref_inout_ecount_opt(1) CSwBitmapColorSource * &pbcs
    )
{
    Assert(oParams.fmtTexture != MilPixelFormat::Undefined);

    //
    // No search for matching format is needed because format caching is
    // divided between sRGB and scRGB prior to this call.
    //

    Assert(m_fmt == oParams.fmtTexture || m_fmt == MilPixelFormat::Undefined);

    m_fmt = oParams.fmtTexture;

    //
    // Search for supporting size entry
    //

    UINT i;

    bool fMatch = false;
    bool fForceInvalidation = false;

    for (i = 0; i < m_rgSizeLayoutEntry.GetCount(); i++)
    {
        bool fNewParamsNotContained;

        if (CheckSizeLayoutMatch(
            m_rgSizeLayoutEntry[i].oSizeParams,
            oParams,
            OUT fNewParamsNotContained
            ))
        {
            fMatch = true;
            fForceInvalidation = fNewParamsNotContained;
            break;
        }
    }

    if (fMatch)
    {
        //
        // Found a match - get or set Bitmap Color Source and update size
        // parameters
        //

        {
            CacheEntry &oSizeEntry = m_rgSizeLayoutEntry[i];

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

                if (   !fForceInvalidation
                    && oSizeEntry.pbcs->IsValid())
                {
                    //
                    // Update oParams (passed in) with cached settings since
                    // those are the settings that will be used.
                    //

                    *static_cast<CSwBitmapColorSource::CacheSizeLayoutParameters *>
                        (&oParams) = oSizeEntry.oSizeParams;

                    pbcs = oSizeEntry.pbcs;
                    pbcs->AddRef();
                }
                else
                {
                    if (fForceInvalidation)
                    {
                        //
                        // Update this entry to be the place holder for the
                        // realization we expect to come through shortly.
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

        if (fForceInvalidation)
        {
            //
            // Walk remaining entries and invalidate matches to avoid polluting
            // cache with too many realizations.
            //
            for (i++; i < m_rgSizeLayoutEntry.GetCount(); i++)
            {
                CacheEntry &oSizeEntry = m_rgSizeLayoutEntry[i];

                bool fNewParamsNotContained;

                if (CheckSizeLayoutMatch(
                        oSizeEntry.oSizeParams,
                        oParams,
                        OUT fNewParamsNotContained
                        )
                    )
                {
                    Assert(fNewParamsNotContained);
                    Assert(oSizeEntry.oSizeParams.fOnlyContainsSubRectOfSource);

                    ReleaseInterfaceNoNULL(oSizeEntry.pbcs);

                    UINT uNewCount = m_rgSizeLayoutEntry.GetCount()-1;
                    if (i != uNewCount)
                    {
                        Assert(i < uNewCount);
                        // Overwrite this element with last
                        m_rgSizeLayoutEntry[i] = m_rgSizeLayoutEntry.Last();
                        // reduce index to process the new entry at this index.
                        i--;
                    }
                    m_rgSizeLayoutEntry.SetCount(uNewCount);
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
            && m_rgSizeLayoutEntry.GetCount() == m_rgSizeLayoutEntry.GetCapacity())
        {
            m_rgSizeLayoutEntry[m_uNextEvictionIndexDbg].oSizeParams = oParams;
            ReplaceInterface(m_rgSizeLayoutEntry[m_uNextEvictionIndexDbg].pbcs, pbcs);
            m_uNextEvictionIndexDbg =
                (m_uNextEvictionIndexDbg + 1) % m_rgSizeLayoutEntry.GetCapacity();
        }
        else
#endif DBG
        {
            CacheEntry *pNewCacheEntry;

            if (SUCCEEDED(THR(m_rgSizeLayoutEntry.AddMultiple(1, &pNewCacheEntry))))
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
            if (m_rgSizeLayoutEntry.GetCount() > c_DbgMaxExpectedCacheGrowth)
            {
                TraceTag((tagMILWarning,
                          "Over %u cached Sw realizations of a bitmap.",
                          c_DbgMaxExpectedCacheGrowth));
            }
#endif DBG
        }
    }

    return;
}



