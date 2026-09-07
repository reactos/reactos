// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+----------------------------------------------------------------------------
//

//
//  Description:     
//      Contains CHwBitmapCache declaration
//

MtExtern(CHwBitmapCache);
MtExtern(CHwBitmapCache_FormatCacheEntry);
MtExtern(CHwBitmapCache_CacheEntryList);
MtExtern(D3DResource_HwBitmapCache);

//+----------------------------------------------------------------------------
//
//  Class:     CHwBitmapCache
//
//  Synopsis:  This class provides a cache of one or more bitmap color sources
//
//             This class is cached itself in an IMILResourceCache and then
//             provides a cache of CHwBitmapColorSources.  This cache can hold
//             multiple color source realizations and they are stored
//             heirarchically according to the caching properties exposed in
//             CHwBitmapColorSource::CacheParameters.
//
//             This class inherits from CD3DResource even though it doesn't
//             directly hold on to an D3D resource because it is cached and
//             needs to be cleaned up when the device (which maintains the
//             cache index) is destroyed.
//

class CHwBitmapCache :
    public CD3DResource, public CMILCacheableResource
{
public:

    static HRESULT RetrieveFromBitmapSource(
        __inout_ecount(1) IWGXBitmapSource *pBitmapSource,
        __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
        __deref_out_ecount_opt(1) IWGXBitmap **ppBitmapNoRef,
        __deref_out_ecount_opt(1) CHwBitmapCache **ppHwBitmapCache
        );

    static HRESULT GetBitmapColorSource(
        __inout_ecount(1) CD3DDeviceLevel1 *pDevice,
        __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
        __inout_ecount_opt(1) IWGXBitmap *pBitmap,
        __inout_ecount(1) CHwBitmapColorSource::CacheParameters &oParams,
        __in_ecount(1) const CHwBitmapColorSource::CacheContextParameters &oContextCacheParameters,
        __inout_ecount_opt(1) CHwBitmapCache *pHwBitmapCacheFromBitmap,
        __deref_out_ecount(1) CHwBitmapColorSource * &pbcs,
        __deref_out_ecount_opt(1) CHwBitmapColorSource * &pbcsWithReusableRealizationSource,
        __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate
        );

    void TryForDeviceBitmapOrLastUsedBitmapColorSource(
        __in_ecount(1) const CHwBitmapColorSource::CacheContextParameters &oContextCacheParameters,
        __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> &rcRealizationBounds,
        __in_ecount(1) CMILBrushBitmap *pBitmapBrush,
        __deref_out_ecount_opt(1) CHwBitmapColorSource * &pbcs,
        __deref_out_ecount_opt(1) CHwBitmapColorSource * &pReusableRealizationSourcesList
        );

    static HRESULT GetCache(
        __inout_ecount(1) CD3DDeviceLevel1      *pDevice,
        __inout_ecount_opt(1) IWGXBitmap        *pBitmap,
        __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate,
        bool fSetResourceRequired,
        __deref_out_ecount(1) CHwBitmapCache    **ppHwBitmapCache
        );

public:

    DEFINE_RESOURCE_REF_COUNT_BASE

    // 
    // CD3DResource methods
    //

    // Should only be called by CD3DResourceManager
    void ReleaseD3DResources();

    //
    // CMILCacheableResource methods
    //

    // the compiler isn't smart enough to see CD3DResource's
    // IsValid() implementation as an override for 
    // IMILCacheableResource's pure virtual
    override bool IsValid() const
    {
        return CD3DResource::IsValid();
    }

    HRESULT CreateSharedColorSource(
        MilPixelFormat::Enum fmt,
        __in_ecount(1) CMilRectU const &rcBoundsRequired,
        __deref_inout_ecount(1) CHwDeviceBitmapColorSource * &pbcs,
        __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
        );

    HRESULT CreateColorSourceForTexture(
        MilPixelFormat::Enum fmt,
        __in_ecount(1) CMilRectU const &rcBoundsRequired,
        __inout_ecount(1) CD3DVidMemOnlyTexture *pVidMemTexture,
        __deref_out_ecount(1) CHwDeviceBitmapColorSource **ppbcs
        );

    HRESULT CreateBitBltColorSource(
        MilPixelFormat::Enum fmt,
        __in_ecount(1) CMilRectU const &rcBoundsRequired,
        bool fIsDependent,
        __deref_inout_ecount(1) CHwDeviceBitmapColorSource * &pbcs
        );

private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwBitmapCache));

    CHwBitmapCache(__in_ecount(1) IWGXBitmap *pBitmap, __in_ecount(1) CD3DDeviceLevel1 *pDevice);
    ~CHwBitmapCache();

    HRESULT ChooseBitmapColorSource(
        __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
        __inout_ecount(1) CHwBitmapColorSource::CacheParameters &oParams,
        __in_ecount(1) const CHwBitmapColorSource::CacheContextParameters &oContextCacheParameters,    
        __deref_out_ecount_opt(1) CHwBitmapColorSource * &pbcs,
        __deref_out_ecount_opt(1) CHwBitmapColorSource * &pbcsWithReusableRealizationSources
        );

    void TryForDeviceBitmapColorSource(
        __in_ecount(1) const CHwBitmapColorSource::CacheContextParameters &oContextCacheParameters,
        __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> &rcRealizationBounds,
        __in_ecount(1) CMILBrushBitmap *pBitmapBrush,
        __deref_out_ecount_opt(1) CHwBitmapColorSource * &pbcs
        );

    void TryForLastUsedBitmapColorSource(
        __in_ecount(1) const CHwBitmapColorSource::CacheContextParameters &oContextCacheParameters,
        __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> &rcRealizationBounds,
        __in_ecount(1) CMILBrushBitmap *pBitmapBrush,
        __deref_out_ecount_opt(1) CHwBitmapColorSource * &pbcs,
        __deref_out_ecount_opt(1) CHwBitmapColorSource * &pReusableRealizationSourcesList
        );

    void AddDeviceBitmapColorSourcesToReusableList(
        __deref_inout_ecount_opt(1) CHwBitmapColorSource * &pbcsWithReusableRealizationSources
        ) const;

#if PERFMETER
    virtual PERFMETERTAG GetPerfMeterTag() const
    {
        return Mt(D3DResource_HwBitmapCache);
    }
#endif

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
        (&m_oCachedEntryList)->~FormatCacheEntry();
        new (&m_oCachedEntryList) FormatCacheEntry();
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
    //      Device (should never change - the cache is device associated)
    //      Format
    //      WrappingSupport
    //      Bitmap Size
    //

    //+------------------------------------------------------------------------
    //
    //  Structure:  CacheEntry
    //
    //  Synopsis:   Lowest level entry has a size data and color source
    //
    //-------------------------------------------------------------------------

    struct CacheEntry
    {
        CHwBitmapColorSource::CacheSizeLayoutParameters oSizeParams;
        CHwBitmapColorSource *pbcs;
    };

    //+------------------------------------------------------------------------
    //
    //  Structure:  CacheEntryList
    //
    //  Synopsis:   Mid level entry has an array of sized entries.
    //
    //-------------------------------------------------------------------------

    class CacheEntryList
    {
    public:
        DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwBitmapCache_CacheEntryList));
        CacheEntryList();
        ~CacheEntryList();
        void GetSetBitmapColorSource(
            __inout_ecount(1) CHwBitmapColorSource::CacheParameters &oParams,
            __deref_inout_ecount_opt(1) CHwBitmapColorSource * &pbcs,
            __deref_opt_inout_ecount_opt(1) CHwBitmapColorSource ** ppbcsWithReusableRealizationSources
            );

    private:

        //+--------------------------------------------------------------------
        //
        //  Enum:
        //      SizeLayoutMatch::Enum
        //
        //  Synopsis:
        //      Describes how well two CacheSizeLayoutParameters match.  Note
        //      that values are value ordered such that greater values indicate
        //      a better match.
        //
        //---------------------------------------------------------------------

        struct SizeLayoutMatch {
            enum Enum {
                // NoMatch - neither size nor layout of cached parameters
                // sufficiently matches new parameters to be of use
                NoMatch,

                // ReusableSource - size and layout are such that if color
                // source is valid its previously realized contents can be a
                // useful source to get required color source realized.  For
                // example a non-mip mapped texture of an complete bitmap
                // source is readily usable as a source to populate any other
                // layout, especially if scale factors match.
                ReusableSource,

                // PartialOverlap - layout is a match and size of natural color
                // source (accounts for scaling of bitmap) is a match, but only
                // part of the bitmap is stored in the color source.  That
                // stored portion strictly contains only a part of what is
                // required.  This happens with very large sources and the
                // portion required is scrolled.
                PartialOverlap,

                // MeetsAllRequirements - size and layout match enough for
                // color source to be resused as is.  If only a part of bitmap
                // is stored in color source, then that portion does contain
                // the part of bitmap required.
                MeetsAllRequirements
            };
        };

        static SizeLayoutMatch::Enum CheckSizeLayoutMatch(
            __in_ecount(1) const CHwBitmapColorSource::CacheSizeLayoutParameters &oCachedParams,
            __in_ecount(1) const CHwBitmapColorSource::CacheSizeLayoutParameters &oNewParams
            );

    private:
        DynArrayIA<CacheEntry, 4, true> m_rgSizeEntry;
#if DBG
        // When tagLimitBitmapSizeCache is enabled this marks the index in the
        // size cache array that should be evicted to make space for a new
        // cache entry.
        UINT m_uNextEvictionIndexDbg;

        static const UINT c_DbgMaxExpectedCacheGrowth;
#endif
    };

    //+------------------------------------------------------------------------
    //
    //  Structure:  FormatCacheEntry
    //
    //  Synopsis:   Top level entry has a list of entries for a specific format
    //
    //-------------------------------------------------------------------------

    class FormatCacheEntry
    {
    public:
        DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwBitmapCache_FormatCacheEntry));
        FormatCacheEntry();
        ~FormatCacheEntry();
        void GetSetBitmapColorSource(
            __inout_ecount(1) CHwBitmapColorSource::CacheParameters &oParams,
            __deref_inout_ecount_opt(1) CHwBitmapColorSource * &pbcs,
            __deref_opt_inout_ecount_opt(1) CHwBitmapColorSource ** ppbcsWithReusableRealizationSources
            );

    private:
        MilPixelFormat::Enum m_fmt;
        FormatCacheEntry *m_pNext;
        CacheEntryList m_oHeadWrapEntry;
    };


    // Device with which this cache is associated
    CD3DDeviceLevel1 * const m_pDevice;

    // IWGXBitmap if this cache is for a IWGXBitmap - never referenced
    IWGXBitmap * const m_pBitmap;

    // IWGXBitmapSource this cache was last used with
    //  This is important for the case when the cache is attached to an object
    //  other than the source itself (an alternate cache).  This is what
    //  happens for brushes used with decoder sources.
    IWGXBitmapSource *m_pIBitmapSource;

    // Cached bitmap color sources
    FormatCacheEntry m_oCachedEntryList;

    // Device bitmap read-only
    CHwDeviceBitmapColorSource *m_pDeviceBitmapColorSource;

    // Cache's lookaside for recently used color source.  When a device bitmap
    // surface is used, last is always left NULL.
    CHwBitmapColorSource *m_pLastUsedColorSource;
    CHwBitmapColorSource::CacheContextParameters m_oLastUsedCacheParameters;
};



