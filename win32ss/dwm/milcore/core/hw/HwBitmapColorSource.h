// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//-----------------------------------------------------------------------------
//

//
//  Description:
//      Contains CHwBitmapColorSource declaration
//


MtExtern(CHwBitmapColorSource);

class CHwBrushContext;
class CHwBitmapCache;

//+----------------------------------------------------------------------------
//
//  Class:     CHwBitmapColorSource
//
//  Synopsis:  Provides a bitmap color source for a HW device
//
//-----------------------------------------------------------------------------

class CHwBitmapColorSource : public CHwTexturedColorSource
{
public:

    //+------------------------------------------------------------------------
    //
    //  Enum:       TexelLayout
    //
    //  Synopsis:   Enumeration of possible texel run population options
    //
    //-------------------------------------------------------------------------

    enum TexelLayout
    {

        // Source bits exactly fill texel run
        NaturalTexelLayout,

        // Source is split in half and sent to ends of the texel run
        CenterSplitTexelLayout,

        // Source length is two less than texel run length.  The first, 0, and last, Tn-1,
        // texels are populated by extended the source.  The natural source is placed in
        // texels 1 to Tn-2.
        EdgeWrappedTexelLayout,
        
        EdgeMirroredTexelLayout,

        // Source length is less then texel run length.  The first of texels is
        // populated by the natural source, but all remaining texels are left
        // unset.
        FirstOnlyTexelLayout,

        // Count of different texel layouts
        TotalTexelLayouts

    };

    static bool DoesTexelLayoutHaveBorder(TexelLayout tl)
    {
        switch(tl)
        {
        case EdgeWrappedTexelLayout:
        case EdgeMirroredTexelLayout:
            return true;
        default:
            return false;
        }
    }

    //+------------------------------------------------------------------------
    //
    //  Structure:  DimensionLayout
    //
    //  Synopsis:   This structure contains the properties required for
    //              realizing an individual dimension of a texture
    //
    //              2D textures should have one for U and another for V.
    //
    //-------------------------------------------------------------------------

    struct DimensionLayout
    {
        UINT uLength;                     // Texels
        TexelLayout eLayout;              // Texel Layout
        D3DTEXTUREADDRESS d3dta;          // Texture addressing mode
    };

    //+------------------------------------------------------------------------
    //
    //  Structure:  CacheContextParameters
    //
    //  Synopsis:   This structure contains the information needed to:
    //              1) check at a high level if a color source can be reused
    //              2) provide context params gathering point while deriving
    //
    //-------------------------------------------------------------------------

    struct CacheContextParameters
    {
        CMILBrushBitmap *pBitmapBrushNoRef;
        MilBitmapInterpolationMode::Enum interpolationMode;
        bool fPrefilterEnable;
        MilPixelFormat::Enum fmtRenderTarget;
        UINT nBitmapBrushUniqueness;
        MilBitmapWrapMode::Enum wrapMode;

        //
        // Constructors
        //

        CacheContextParameters(
            bool fInitializeNoMembers
            );

        CacheContextParameters(
            __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
            __in_ecount(1) const CContextState *pContextState,
            __in_ecount(1) CMILBrushBitmap *pBitmapBrush,
            MilPixelFormat::Enum fmtTargetSurface
            );

        CacheContextParameters(
            MilBitmapInterpolationMode::Enum interpolationMode,
            bool fPrefilterEnable,
            MilPixelFormat::Enum fmtRenderTarget,
            MilBitmapWrapMode::Enum wrapMode
            );

        //
        // NOTICE-2005/10/12-chrisra ContextParameters Don't track subregion
        //
        // When a bitmap is realized, we may only realize a subregion into a
        // texture because of texture size limits on the hardware.  But we
        // don't know this until the full realization code has been run.
        //
        // Until the ContextParameters can properly track this information we
        // simply avoid setting a "Last Used" color source in the cache.
        //
    };

    //+------------------------------------------------------------------------
    //
    //  Structure:  Cache*Parameters, CacheParameters
    //
    //  Synopsis:   These structures contains the information needed to:
    //              1) test if one color source can be reused and
    //              2) sort color sources
    //
    //-------------------------------------------------------------------------

    struct CacheFormatParameters
    {
        MilPixelFormat::Enum fmtTexture;  // Pixel format
    };

    struct CacheSizeLayoutParameters
    {
        UINT uWidth;                // Width of color source (natural)
        UINT uHeight;               // Height of color source (natural)
        bool fOnlyContainsSubRectOfSource;
        TextureMipMapLevel eMipMapLevel;
        CMilRectU rcSourceContained;
        DimensionLayout dlU, dlV;   // Layouts for each texture dimension
    };

    //
    // Collection of properties relevant to choosing/caching a color source
    //

    struct CacheParameters :
        public CacheFormatParameters,
        public CacheSizeLayoutParameters
    {
    };

private:

    //+------------------------------------------------------------------------
    //
    //  Structure:  InternalRealizationParameters
    //
    //  Synopsis:   This structures contains the information needed to realize
    //              a texture for a certain context when combined with the
    //              caching parameters in CacheParamters
    //
    //-------------------------------------------------------------------------

    struct InternalRealizationParameters
    {
        //
        // Properties relevant to use and realization updates, but that don't
        // affect caching
        //

        MilBitmapInterpolationMode::Enum interpolationMode;
        UINT uBitmapWidth, uBitmapHeight;   // Size of original source bitmap
        MilBitmapWrapMode::Enum wrapMode;

        // Indicates whether minimum required realization has been computed in
        // response to need to meet texture limits with results stored in
        // rcSourceContained.  If not computations can be made to limit
        // realization of costly realization processes like copying from video
        // memory.
        bool fMinimumRealizationRectRequiredComputed;
    };

    //+------------------------------------------------------------------------
    //
    //  Structure:  RealizationParameters
    //
    //  Synopsis:   This structures contains the information needed to:
    //              1) test if one realization can be reused,
    //              2) create a new realization, and/or
    //              3) set the context for a realization
    //
    //-------------------------------------------------------------------------

    struct RealizationParameters :
        public CacheParameters,
        public InternalRealizationParameters
    {
    };

public:

    //+----------------------------------------------------------------------------
    //
    //  Member:    CHwBitmapColorSource::DeriveFromBrushAndContext
    //
    //  Synopsis:  Gets a CHwTexturedColorSource from the bitmap brush
    //
    //-----------------------------------------------------------------------------

    static HRESULT DeriveFromBrushAndContext(
        __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        __in_ecount(1) CMILBrushBitmap *pBitmapBrush,
        __in_ecount(1) const CHwBrushContext &hwBrushContext,
        __deref_out_ecount(1) CHwTexturedColorSource **ppHwTexturedColorSource
        );

    //+----------------------------------------------------------------------------
    //
    //  Member:  CHwBitmapColorSource::DeriveFromBitmapAndContext
    //
    //  Synopsis:  Gets a CHwTexturedColorSource from the bitmap data
    //             The color source is created if it cannot be found in a cache
    //
    //-----------------------------------------------------------------------------

    static HRESULT DeriveFromBitmapAndContext(
        __inout_ecount(1) CD3DDeviceLevel1 *pD3DDevice,
        __inout_ecount(1) IWGXBitmapSource *pIBitmap,
        __inout_ecount_opt(1) IWGXBitmap *pBitmap,
        __inout_ecount_opt(1) CHwBitmapCache *pHwBitmapCacheFromBitmap,
        __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> const &rcRealizationBounds,
        __in_ecount(1) const CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> *pmatBitmapToIdealRealization,
        __in_ecount(1) const BitmapToXSpaceTransform *pBitmapToXSpaceTransform,
        REAL rPrefilterThreshold,
        BOOL fCanFallback,
        __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate,
        __in_ecount(1) CHwBitmapColorSource::CacheContextParameters &oContextCacheParameters,
        __deref_out_ecount(1) CHwTexturedColorSource **ppHwTexturedColorSource
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    CHwBitmapColorSource::ComputeMinimumRealizationBounds
    //
    //  Synopsis:  Compute minimum realization bounds for
    //             RealizationParameters structure from the given context
    //
    //-------------------------------------------------------------------------

    static bool ComputeMinimumRealizationBounds(
        __in_ecount(1) IWGXBitmapSource *pIBitmap,
        __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> const &rcBounds,
        __in_ecount(1) CacheContextParameters const &oCacheContextParameters,
        __out_ecount(1) CMilRectU &rcMinBounds
        );

    static HRESULT Create(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount_opt(1) IWGXBitmap *pBitmap,
        __in_ecount(1) const CHwBitmapColorSource::CacheParameters &oRealizationDesc,
        bool fCreateAsRenderTarget,
        __deref_out_ecount(1) CHwBitmapColorSource **ppHwBitmapCS
        );

protected:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CHwBitmapColorSource));

    CHwBitmapColorSource(
        __in_ecount(1) CD3DDeviceLevel1 *pDevice,
        __in_ecount_opt(1) IWGXBitmap *pBitmap,
        MilPixelFormat::Enum fmt,
        __in_ecount(1) const D3DSURFACE_DESC &d3dsd,
        UINT uLevels
        );
    ~CHwBitmapColorSource();

public:

    //+------------------------------------------------------------------------
    //
    //  Member:
    //      CheckRequiredRealizationBounds
    //
    //  Synopsis:
    //      Return true if color source has a realization of required sampling
    //      bounds.
    //
    //-------------------------------------------------------------------------

    struct RequiredBoundsCheck
    {
        enum Enum {
            // Check only that m_rcRequiredRealizationBounds covers required
            // bounds.  This is the common situation with system memory
            // sources.  m_rcPrefilteredBitmap should always be realized.
            CheckRequired,

            // Check that m_rcCachedRealizationBounds covers required bounds.
            // This is used for shared bitmap color source.  As they are
            // read-only they only satisfy requirements when their cached
            // bounds cover required bounds.
            CheckCached,

            // Check that m_rcPrefilteredBitmap can hold a realization of
            // required bounds.  If it can then set
            // m_rcRequiredRealizationBounds to the current requirements. This
            // is useful with shared surface sources to limit updates to just
            // what is needed and avoid copies from video memory that could be
            // fairly slow.
            CheckPossibleAndUpdateRequired,
        };
    };

    bool CheckRequiredRealizationBounds(
        __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> &rcRealizationBounds,
        MilBitmapInterpolationMode::Enum interpolationMode,
        MilBitmapWrapMode::Enum wrapMode,
        RequiredBoundsCheck::Enum eCheckRequest
        );

    override bool IsValid() const;

    //
    // CHwColorSource methods
    //

    override bool IsOpaque(
        ) const;

    override virtual HRESULT Realize(
        );

    override HRESULT SendDeviceStates(
        DWORD dwStage,
        DWORD dwSampler
        );

    override HRESULT SendVertexMapping(
        __inout_ecount_opt(1) CHwVertexBuffer::Builder *pVertexBuilder,
        MilVertexFormatAttribute mvfaLocation
        );


    //
    // Simple property accessor methods
    //

    bool IsARenderTarget() const
    {
        return m_d3dsdRequired.Usage & D3DUSAGE_RENDERTARGET;
    }

    MilPixelFormat::Enum Format() const
    {
        return m_fmtTexture;
    }

    //
    // Property setting methods
    //

    void AddToReusableRealizationSourceList(
        __deref_inout_ecount_inopt(1) CHwBitmapColorSource * &pbcsReusableList
        );

    //
    // Maintenance methods
    //

protected:

    static VOID GetD3DSDRequired(
        __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
        __in_ecount(1) const CacheParameters &oRealizationParams,
        __out_ecount(1) D3DSURFACE_DESC *pd3dsdRequired,
        __out_ecount(1) UINT *puLevels
        );

    HRESULT CreateTexture(
        bool fIsEvictable,
        __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
        );

    void SetBitmapAndContextCacheParameters(
        __in_ecount(1) IWGXBitmapSource *pBitmapSource,
        __in_ecount(1) const CacheParameters &oRealizationParams
        );

    virtual HRESULT GetPointerToValidSourceRects(
        __in_ecount_opt(1) IWGXBitmap *pBitmap,
        __out_ecount(1) UINT &cValidSourceRects,
        __deref_out_ecount_full(cValidSourceRects) CMilRectU const * &rgValidSourceRects
        ) const;

    //+------------------------------------------------------------------------
    //
    //  Member:
    //      CHwBitmapColorSource::IsRealizationValid
    //
    //  Synopsis:
    //      Checks whether cached content is realized for current requirements
    //      and state of source, if source is a bitmap whose contents may
    //      change.
    //
    //-------------------------------------------------------------------------

    bool IsRealizationValid() const;

private:

    //+------------------------------------------------------------------------
    //
    //  Member:    CalcTextureTransform
    //
    //  Synopsis:  Sets the matrix which transforms points from device space
    //             to source space.
    //
    //-------------------------------------------------------------------------

    HRESULT CalcTextureTransform(
        __in_ecount(1) const BitmapToXSpaceTransform *pBitmapToXSpaceTransform
        );

    //+------------------------------------------------------------------------
    //
    //  Member:
    //      CHwBitmapColorSource::IsRealizationCurrent
    //
    //  Synopsis:
    //      Checks whether cached content is current with source, independent
    //      of whether enough area of required realization is present.
    //
    //-------------------------------------------------------------------------

    bool IsRealizationCurrent() const;

    //+------------------------------------------------------------------------
    //
    //  Member:    SetBitmapAndContext
    //
    //  Synopsis:  Set the current context and bitmap this color source is to
    //             realize
    //
    //-------------------------------------------------------------------------

    HRESULT SetBitmapAndContext(
        __in_ecount(1) IWGXBitmapSource *pBitmapSource,
        __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> const &rcRealizationBounds,
        __in_ecount(1) const BitmapToXSpaceTransform *pBitmapToXSpaceTransform,
        __in_ecount(1) const RealizationParameters &oRealizationParams,
        __in_ecount_opt(1) CHwBitmapColorSource *pbcsWithReusableRealizationSource
        );


    bool DoPrefilterDimensionsMatch(
        UINT uWidth,
        UINT uHeight
        ) const;

    void CheckAndSetReusableSource(
        __inout_ecount(1) CHwBitmapColorSource *pbcsWithReusableRealizationSource
        );

    void CheckAndSetReusableSources(
        __inout_ecount_opt(1) CHwBitmapColorSource *pbcsWithReusableRealizationSources
        );

    void ReleaseRealizationSources(
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    CHwBitmapColorSource::ComputeMinimumRealizationBounds
    //
    //  Synopsis:  Compute minimum realization bounds for
    //             RealizationParameters structure from the given context
    //
    //-------------------------------------------------------------------------

    static bool ComputeMinimumRealizationBounds(
        __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> const &rcBounds,
        __in_ecount(1) InternalRealizationParameters const &oRealizationParams,
        __inout_ecount(1) CMilRectU &rcMinBounds
        );

    static void InitializeSubRectParameters(
        __inout_ecount(1) RealizationParameters &oRealizationParams
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    CHwBitmapColorSource::ComputeRealizationSize
    //
    //  Synopsis:  Compose size portion of RealizationParameters structure from
    //             the given context
    //
    //-------------------------------------------------------------------------

    static HRESULT ComputeRealizationSize(
        UINT uMaxTextureWidth,
        UINT uMaxTextureHeight,
        __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> const &rcRealizationBounds,
        __in_ecount(1) const CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> *pmatBitmapToIdealRealization,
        MilBitmapWrapMode::Enum wrapMode,
        BOOL fPrefilterEnabled,
        REAL rPrefilterThreshold,
        BOOL fCanFallback,
        __inout_ecount(1) RealizationParameters &oRealizationParams
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    CHwBitmapColorSource::ComputeRealizationParameters
    //
    //  Synopsis:  Compose a RealizationParameters structure from the given
    //             context
    //
    //-------------------------------------------------------------------------

    static HRESULT ComputeRealizationParameters(
        __in_ecount(1) CD3DDeviceLevel1 const *pDevice,
        __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
        __inout_ecount(1) CDelayComputedBounds<CoordinateSpace::RealizationSampling> const &rcRealizationBounds,
        __in_ecount(1) const CMultiOutSpaceMatrix<CoordinateSpace::RealizationSampling> *pmatBitmapToIdealRealization,
        MilPixelFormat::Enum fmtRenderTarget,
        MilBitmapWrapMode::Enum wrapMode,
        MilBitmapInterpolationMode::Enum interpolationMode,
        BOOL fPrefilterEnabled,
        REAL rPrefilterThreshold,
        BOOL fCanFallback,
        __out_ecount(1) RealizationParameters &oRealizationParams
        );

    //+------------------------------------------------------------------------
    //
    //  Member:    AdjustLayoutForConditionalNonPowerOfTwo
    //
    //  Synopsis:  Adjust the given natural layout accommodate conditional
    //             non-power of two support
    //
    //-------------------------------------------------------------------------

    static HRESULT AdjustLayoutForConditionalNonPowerOfTwo(
        IN OUT DimensionLayout &dl,
        IN UINT uMaxLength
        );

    static HRESULT ReconcileLayouts(
        __inout_ecount(1) RealizationParameters &rp,
        UINT uMaxWidth,
        UINT uMaxHeight
        );

    void UpdateBorders(
        __in_ecount(1) const CMilRectU *rcDirty,
        UINT cbStep,
        UINT cbStride,
        UINT cbBufferSize,
        __inout_bcount(cbBufferSize) BYTE *pvPixels
        );

    HRESULT FillTexture();

    HRESULT FillTextureWithTransformedSource(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        BOOL fBitmapSourceIsCBitmap
        );
    
    __success(true) bool GetDirtyRects(
        __deref_out_ecount(*pcDirtyRects) MilRectU const ** const prgDirtyRects,
        __deref_in_range(0,0) __deref_out_range(0,5) UINT * const pcDirtyRects,
        __out_ecount(1) UINT * const puNewestUniquenessToken
        ) const;

    HRESULT PrepareToPushSourceBitsToVidMem(
        BOOL fBitmapSourceIsCBitmap,
        __deref_out_ecount_opt(1) IWGXBitmapLock **ppILock,
        __out_ecount(1) bool *pfShouldCopySourceToSysMemSurface,
        __deref_out_ecount(1) IDirect3DSurface9** ppD3DSysMemSurface
        DBG_COMMA_PARAM(__in_ecount(1) IWGXBitmapSource *pIDBGBitmapSource)
        );

    __range(0, cDirtyRects)
    UINT ComputePrefilteredDirtyRects(
        __in_ecount(cDirtyRects) const MilRectU *rgDirtyRects,
        __in_range(1,5) UINT cDirtyRects,
        __out_ecount_part(cDirtyRects,return) CMilRectU *rgDestDirtyRects
        );

    typedef DynArray<CMilRectU> * PDynCMilRectUArray;

    HRESULT UpdateFromReusableSource(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        BOOL fBitmapSourceIsCBitmap,
        __in_ecount(1) CHwBitmapColorSource *pbcsSource,
        __in_range(>=,1) UINT cDirtyRects,
        __in_ecount(cDirtyRects) CMilRectU *rgDirtyRects,
        __out_ecount(1) UINT *pcRemaining,
        __deref_out_ecount_full(*pcRemaining) CMilRectU **prgRemainingRects,
        UINT const cDirtyRectRemainingBuffers,
        __in_ecount(cDirtyRectRemainingBuffers) PDynCMilRectUArray const *rgprgDirtyRectsRemaining,
        __deref_in_range(0,cDirtyRectRemainingBuffers-1) __deref_out_range(0,cDirtyRectRemainingBuffers-1) UINT *puActiveOutputArrayIndex
        );

    HRESULT PushTheSourceBitsToVideoMemory(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource,
        __in_range(>=,1) UINT cDirtyRects,
        __inout_ecount(cDirtyRects) CMilRectU *rgDestDirtyRects,
        __inout_ecount(1) IDirect3DSurface9 *pD3DSysMemSurface,
        bool fCopySourceToSysMemSurface
        );
    
    HRESULT GetSysMemUpdateSurfaceSource(
        __in_opt void *pvReferenceBits,
        UINT uWidth,
        UINT uHeight,
        bool fCanCreateFromBits,
        __deref_out_ecount(1) IDirect3DSurface9** ppD3DSysMemSurface
        );

protected:

    static VOID AssertMinimalTextureDesc(
        __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
        D3DTEXTUREADDRESS taU,
        D3DTEXTUREADDRESS taV,
        __in_ecount(1) const D3DSURFACE_DESC *d3dsdRequired
        )
    #if DBG
        ;
    #else
    {
        UNREFERENCED_PARAMETER(pDevice);
        UNREFERENCED_PARAMETER(taU);
        UNREFERENCED_PARAMETER(taV);
        UNREFERENCED_PARAMETER(d3dsdRequired);
    }
    #endif


private:

    #if DBG
        void AssertSysMemSurfaceDescriptionNotChanged(
            __in_ecount(1) IDirect3DSurface9 *pD3DSysMemSurface,
            UINT Width,
            UINT Height
            );

        void AssertSysMemTextureDescriptionNotChanged(
            __in_ecount(1) IDirect3DTexture9 *pD3DSysMemTexture
            );
    #else
        MIL_FORCEINLINE void AssertSysMemSurfaceDescriptionNotChanged(
            __in_ecount(1) IDirect3DSurface9 *pD3DSysMemSurface,
            UINT Width,
            UINT Height
            )
        {
            UNREFERENCED_PARAMETER(pD3DSysMemSurface);
            UNREFERENCED_PARAMETER(Width);
            UNREFERENCED_PARAMETER(Height);
        }

        MIL_FORCEINLINE void AssertSysMemTextureDescriptionNotChanged(
            __in_ecount(1) IDirect3DTexture9 *pD3DSysMemTexture
            )
        {
            UNREFERENCED_PARAMETER(pD3DSysMemTexture);
        }
    #endif

protected:

    IWGXBitmapSource *m_pIBitmapSource; // The current device independent
                                        // bitmap being realized

    CMilRectU m_rcPrefilteredBitmap;  // Area of prefiltered source used to
                                         // populate color source

    UINT m_uBitmapWidth;                // Width of original source
    UINT m_uBitmapHeight;               // Height of original source

    CD3DVidMemOnlyTexture *m_pVidMemOnlyTexture;  // Currently allocated/cached
                                                  // texture (if using a pool-default texture)

    UINT m_uCachedUniquenessToken;  // Uniqueness token if realized for IWGXBitmap

    CMilRectU m_rcCachedRealizationBounds;   // Area of realization that
                                                // contains valid content in
                                                // sync with
                                                // m_uCachedUniquenessToken

    CMilRectU m_rcRequiredRealizationBounds;  // Area that must be realized
                                                 // during call to Realize

    IWGXBitmap * const m_pBitmap;  // IWGXBitmap- if m_pIBitmapSource is a IWGXBitmap this variable
                                   // refers to that same bitmap.

    MilPixelFormat::Enum const m_fmtTexture;    // Precise pixel format inc. premul type

private:

    D3DSURFACE_DESC const m_d3dsdRequired;  // Description of surface needed to
                                            // realize this bitmap

    UINT const m_uLevels;                   // Number of mip-map levels to
                                            // create texture with

    void *m_pvReferencedSystemBits;       // pointer to the bits that the bitmap had during the last lock.
    IDirect3DSurface9* m_pD3DSysMemRefSurface; // cache of system memory surface that references the bitmap bits

    CHwBitmapColorSource *m_pbcsRealizationSources;     // A list of realized
                                                        // Hw color source that
                                                        // can be used to
                                                        // update this color
                                                        // source.

    UINT m_uPrefilterWidth;         // Width for prefiltered source
    UINT m_uPrefilterHeight;        // Height for prefiltered source

    TexelLayout m_tlU, m_tlV;   // Layout we're using for the hardware texture

#if DBG
private:

    IWGXBitmapSource *m_pIBitmapSourceDBG;  // same as m_pIBitmapSource, but
                                            // initialized in constructor, used
                                            // for assertions
#endif

};


