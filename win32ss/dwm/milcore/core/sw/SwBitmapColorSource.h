// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_software
//      $Keywords:
//
//  $Description:
//      Definition for CSwBitmapColorSource which provides source color data to
//      resample color sources
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


MtExtern(CSwBitmapColorSource);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CSwBitmapColorSource
//
//  Synopsis:
//      Provides a bitmap color source for a HW device
//
//------------------------------------------------------------------------------

class CSwBitmapColorSource
    : public CMILRefCountBase
{
public:

    //+-------------------------------------------------------------------------
    //
    //  Structure:
    //      Cache*Parameters, CacheParameters
    //
    //  Synopsis:
    //      These structures contains the information needed to:
    //      1) test if one color source can be reused and
    //      2) sort color sources
    //
    //--------------------------------------------------------------------------

    struct CacheContextParameters
    {
        bool fPrefilterEnable;
        CColorSourceCreator const *pCSCreator;
    };

    struct CacheFormatParameters
    {
        MilPixelFormat::Enum fmtTexture;  // Pixel format
    };

    struct CacheSizeLayoutParameters
    {
        UINT uWidth;                // Width of color source (natural)
        UINT uHeight;               // Height of color source (natural)
        bool fOnlyContainsSubRectOfSource;
        CMilRectU rcSourceContained;
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

    //+-------------------------------------------------------------------------
    //
    //  Structure:
    //      InternalRealizationParameters
    //
    //  Synopsis:
    //      This structures contains the information needed to realize a texture
    //      for a certain context when combined with the caching parameters in
    //      CacheParamters
    //
    //--------------------------------------------------------------------------

    struct InternalRealizationParameters
    {
        //
        // Properties relevant to realization, but that don't affect caching
        //

        UINT uBitmapWidth, uBitmapHeight;   // Size of original source bitmap
    };

    //+-------------------------------------------------------------------------
    //
    //  Structure:
    //      RealizationParameters
    //
    //  Synopsis:
    //      This structures contains the information needed to:
    //      1) test if one realization can be reused,
    //      2) create a new realization, and/or
    //      3) set the context for a realization
    //
    //--------------------------------------------------------------------------

    struct RealizationParameters :
        public CacheParameters,
        public InternalRealizationParameters
    {
    };

public:

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      DeriveFromBitmapAndContext
    //
    //  Synopsis:
    //      Gets a bitmap from the bitmap brush data.  The bitmap is realized
    //      if it cannot be found in a cache.
    //
    //--------------------------------------------------------------------------

    static HRESULT DeriveFromBitmapAndContext(
        __inout_ecount(1) IWGXBitmapSource *pIBitmap,
        __inout_ecount(1) CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatBitmapToSampleSpace,
        __in_ecount(1) CColorSourceCreator const *pCSCreator,
        bool fPrefilterEnabled,
        REAL rPrefilterThreshold,
        __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate,
        __deref_out_ecount(1) IWGXBitmap **ppBitmap
        );


    static HRESULT Create(
        __in_ecount_opt(1) IWGXBitmap *pBitmap,
        __deref_out_ecount(1) CSwBitmapColorSource ** const ppSwBitmapCS
        );

private:

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CSwBitmapColorSource));

    CSwBitmapColorSource(
        __in_ecount_opt(1) IWGXBitmap *pBitmap
        );
    ~CSwBitmapColorSource();

public:

    bool IsValid() const;

    //
    // CSwTexturedColorSource methods (if such a class existed)
    //

    /*override*/ bool IsOpaque(
        ) const;

    /*override*/ HRESULT Realize(
        );


private:

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      ComputeRealizationSize
    //
    //  Synopsis:
    //      Compose size portion of RealizationParameters structure from the
    //      given context
    //
    //--------------------------------------------------------------------------

    static void ComputeRealizationSize(
        __inout_ecount(1) CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatBitmapToSampleSpace,
        bool fPrefilterEnabled,
        REAL rPrefilterThreshold,
        __inout_ecount(1) RealizationParameters &oRealizationParams
        );

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      ComputeRealizationParameters
    //
    //  Synopsis:
    //      Compose a RealizationParameters structure from the given context
    //
    //--------------------------------------------------------------------------

    static HRESULT ComputeRealizationParameters(
        __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
        __inout_ecount(1) CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatBitmapToSampleSpace,
        __in_ecount(1) CColorSourceCreator const *pCSCreator,
        bool fPrefilterEnabled,
        REAL rPrefilterThreshold,
        __out_ecount(1) RealizationParameters &oRealizationParams,
        __out_ecount(1) bool &fNeedsRealization
        );

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      CheckValidRealization
    //
    //  Synopsis:
    //      Validates m_fValidRealization, indicating whether the current
    //      texture contains a useful realization of the current device
    //      independent bitmap.
    //
    //--------------------------------------------------------------------------

    void CheckValidRealization();

    //+-------------------------------------------------------------------------
    //
    //  Member:
    //      SetBitmapAndContext
    //
    //  Synopsis:
    //      Set the current context and bitmap this color source is to realize
    //
    //--------------------------------------------------------------------------

    void SetBitmapAndContext(
        __in_ecount(1) IWGXBitmapSource *pBitmapSource,
        __in_ecount(1) const RealizationParameters &oRealizationParams
        );


    HRESULT CreateTexture();

    HRESULT FillTexture();
    
    HRESULT FillTextureWithTransformedSource(
        __in_ecount(1) IWGXBitmapSource *pIBitmapSource
        );
    
    VOID GetDirtyRects(
        __deref_out_ecount(*pcDirtyRects) const MilRectU **const prgDirtyRects,
        __out_ecount(1) UINT *pcDirtyRects,
        __inout_ecount(1) CMilRectU *prcTempLocalStorage
        );

private:

    IWGXBitmapSource *m_pIBitmapSource; // The current device independent
                                        // bitmap being realized

    IWGXBitmap * const m_pBitmap;  // IWGXBitmap- if m_pIBitmapSource is a IWGXBitmap this variable
                                   // refers to that same bitmap.

    MilPixelFormat::Enum m_fmtTexture;    // Precise pixel format inc. premul type
    UINT m_uPrefilterWidth;         // Width for prefiltered source
    UINT m_uPrefilterHeight;        // Height for prefiltered source

    CMilRectU m_rcPrefilteredBitmap; // Area of prefiltered source used to
                                        // populate color source

    UINT m_uRealizationWidth;   // Width of realization
    UINT m_uRealizationHeight;  // Height of realization

    CSystemMemoryBitmap *m_pRealizationBitmap;  // Currently allocated/cached texture


    UINT m_uBitmapWidth;            // Width of original source
    UINT m_uBitmapHeight;           // Height of original source

    UINT m_uCachedUniquenessToken;  // Uniqueness token if realized for IWGXBitmap

    bool m_fValidRealization;   // True if the current texture contains a
                                // useful realization of the current device
                                // independent bitmap

#if DBG
private:

    IWGXBitmapSource *m_pIBitmapSourceDbg;  // same as m_pIBitmapSource, but
                                            // initialized in constructor, used
                                            // for assertions
#endif

};



