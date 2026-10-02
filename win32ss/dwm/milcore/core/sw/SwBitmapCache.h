// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Abstract:
//      Contains CSwBitmapCache declaration which supports the
//      IMILCacheableResource interface and can store multiple bitmap
//      realizations.
//

MtExtern(CSwBitmapCache);

class CSwBitmapCache :
    public CMILRefCountBase,
    public CMILCacheableResource
{
public:

    static HRESULT GetBitmapColorSource(
        __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
        __inout_ecount(1) IWGXBitmap *pBitmap,
        __inout_ecount(1) CSwBitmapColorSource::CacheParameters &oParams,
        __deref_out_ecount(1) CSwBitmapColorSource * &pbcs,
        __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate
        );

private:

    static HRESULT GetCache(
        __inout_ecount_opt(1) IWGXBitmap        *pBitmap,
        __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate,
        __deref_out_ecount(1) CSwBitmapCache    **ppSwBitmapCache
        );

public:

    DEFINE_REF_COUNT_BASE

    //
    // CMILCacheableResource methods
    //

    bool IsValid() const override
    {
        return true;
    }

private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CSwBitmapCache));

    CSwBitmapCache(__in_ecount_opt(1) IWGXBitmap *pBitmap);
    ~CSwBitmapCache();

    HRESULT ChooseBitmapColorSource(
        __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
        __inout_ecount(1) CSwBitmapColorSource::CacheParameters &oParams,
        __out_ecount(1) CSwBitmapColorSource * &pbcs
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    CleanCache
    //
    //  Synopsis:  Release all realizations in the cache
    //
    //-------------------------------------------------------------------------

    MIL_FORCEINLINE void CleanCache()
    {
        // Clean cache by destroying the member object and reconstructing
        (&m_rgFormatCachedEntry[0])->~FormatCacheEntry();
        new (&m_rgFormatCachedEntry[0]) FormatCacheEntry();
        (&m_rgFormatCachedEntry[1])->~FormatCacheEntry();
        new (&m_rgFormatCachedEntry[1]) FormatCacheEntry();
    }

private:

    //
    // Caching hierarchy for multiple realizations
    //
    //  The hierarchy is organized so that the least likely to change
    //  properties are checked first.  This enables us to keep the number of
    //  entries low.  Hierarchy of properties order from least to most frequent
    //  changes expected:
    //
    //      Format
    //      Bitmap Size
    //

    //+------------------------------------------------------------------------
    //
    //  Structure:  CacheEntry
    //
    //  Synopsis:   Lowest level entry has a size data and bitmap
    //
    //-------------------------------------------------------------------------

    struct CacheEntry
    {
        CSwBitmapColorSource::CacheSizeLayoutParameters oSizeParams;
        CSwBitmapColorSource *pbcs;
    };

    //+------------------------------------------------------------------------
    //
    //  Structure:  FormatCacheEntry
    //
    //  Synopsis:   Top level entry has a list of entries for a specific format
    //  Synopsis:   Mid level entry has an array of sized entries for a set of
    //              wrap modes
    //
    //-------------------------------------------------------------------------

    class FormatCacheEntry
    {
    public:
        // Place new and delete to allow for in place destroy and reconstruct
        inline void * __cdecl operator new(size_t, void * pv) { return pv; }
        inline void __cdecl operator delete(void *, void *) { }

        FormatCacheEntry();
        ~FormatCacheEntry();
        void GetSetBitmapColorSource(
            __inout_ecount(1) CSwBitmapColorSource::CacheParameters &oParams,
            __deref_inout_ecount_opt(1) CSwBitmapColorSource * &pbcs
            );

    private:

        static bool CheckSizeLayoutMatch(
            __in_ecount(1) CSwBitmapColorSource::CacheSizeLayoutParameters &oCachedParams,
            __in_ecount(1) CSwBitmapColorSource::CacheSizeLayoutParameters &oNewParams,
            __out_ecount(1) bool &fNewParamsNotContained
            );

    private:
        MilPixelFormat::Enum m_fmt;
        DynArrayIA<CacheEntry, 2, true> m_rgSizeLayoutEntry;

#if DBG
        // When tagLimitBitmapSizeCache is enabled this marks the index in the
        // size cache array that should be evicted to make space for a new
        // cache entry.
        UINT m_uNextEvictionIndexDbg;

        static const UINT c_DbgMaxExpectedCacheGrowth;
#endif
    };


    // IWGXBitmap if this cache is for a IWGXBitmap - never referenced
    IWGXBitmap * const m_pBitmap;

    // IWGXBitmapSource this cache was last used with
    //  This is important for the case when the cache is attached to an object
    //  other than the source itself (an alternate cache).  This is what
    //  happens for brushes used with decoder sources.
    IWGXBitmapSource *m_pIBitmapSourceNoRef;

    // Cached bitmaps per color space (sRGB+scRGB)
    FormatCacheEntry m_rgFormatCachedEntry[2];

};




