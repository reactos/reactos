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
//      Implementation for CSwBitmapColorSource which provides source color data
//      to resample color sources
//
//  $Notes:
//      Many parts of this logic are very similar to Hw and it is very possible
//      to extract the common parts to avoid duplication of code.  This was not
//      done because of time constraints and not wanting to add risk to Hw code
//      at the time.  An effort was made to keep the code layout very similar
//      and enable simple comparison for this effort in the future.  When trying
//      to extract common code look to undo second change to this file. That
//      change simply removed a bunch of unused code that would be brough back
//      for a common code set.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


MtDefine(CSwBitmapColorSource, MILRender, "CSwBitmapColorSource");



//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::DeriveFromBitmapAndContext
//
//  Synopsis:
//      Gets a IWGXBitmap from the bitmap source.  The bitmap is realized if it
//      cannot be found in a cache.
//

HRESULT
CSwBitmapColorSource::DeriveFromBitmapAndContext(
    __inout_ecount(1) IWGXBitmapSource *pIBitmap,
    __inout_ecount(1) CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatBitmapToSampleSpace,
    __in_ecount(1) CColorSourceCreator const *pCSCreator,
    bool fPrefilterEnabled,
    REAL rPrefilterThreshold,
    __inout_ecount_opt(1) IMILResourceCache *pICacheAlternate,
    __deref_out_ecount(1) IWGXBitmap **ppBitmap
    )
{
    HRESULT hr = S_OK;

    IWGXBitmap *pIWGXBitmap = NULL;

    // Local bitmap color source in case cache access utterly fails
    CSwBitmapColorSource oSwBitmapColorSourceLocal(NULL);

    CSwBitmapColorSource *pSwBitmapColorSource = NULL;

    //
    // Get realization parameters
    //

    bool fNeedsRealization;
    CSwBitmapColorSource::RealizationParameters oRealizationParams;

    IFC(CSwBitmapColorSource::ComputeRealizationParameters(
        pIBitmap,
        pmatBitmapToSampleSpace,
        pCSCreator,
        fPrefilterEnabled,
        rPrefilterThreshold,
        OUT oRealizationParams,
        OUT fNeedsRealization
        ));

    //
    // Check for IWGXBitmap.  Note that intermediate RTs always produce IWGXBitmaps.
    //

    if (FAILED(pIBitmap->QueryInterface(IID_IWGXBitmap, reinterpret_cast<void**>(&pIWGXBitmap))))
    {
        // Always make a realization if source is not an IWGXBitmap
        fNeedsRealization = true;
    }

    if (fNeedsRealization)
    {
        //
        // Get a color source
        //

        if (FAILED(CSwBitmapCache::GetBitmapColorSource(
             pIBitmap,
             pIWGXBitmap,
             oRealizationParams,
             OUT pSwBitmapColorSource,
             pICacheAlternate
             )))
        {
            pSwBitmapColorSource = &oSwBitmapColorSourceLocal;
        }

        //
        // Set context and bitmap.  They may be the first to be set, the same
        // as currently set, or different than what was set previously.
        //

        pSwBitmapColorSource->SetBitmapAndContext(
            pIBitmap,
            oRealizationParams
            );

        IFC(pSwBitmapColorSource->Realize());

        //
        // Further adjust bitmap to sample space tranform as needed.
        //

        pmatBitmapToSampleSpace->Translate(
            static_cast<REAL>(oRealizationParams.rcSourceContained.left),
            static_cast<REAL>(oRealizationParams.rcSourceContained.top)
            );

        //
        // Update return bitmap
        //

        *ppBitmap = pSwBitmapColorSource->m_pRealizationBitmap; // AddRef below
    }
    else
    {
        // Given source is good enough
        *ppBitmap = pIWGXBitmap;
    }

    (*ppBitmap)->AddRef();

Cleanup:
    // If QI succeeded, we need to remove the extra ref. If it didn't, pIWGXBitmap 
    // is still NULL
    ReleaseInterface(pIWGXBitmap);

    if (pSwBitmapColorSource != &oSwBitmapColorSourceLocal)
    {
        ReleaseInterfaceNoNULL(pSwBitmapColorSource);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::ComputeRealizationSize
//
//  Synopsis:
//      Compose size portion of RealizationParameters structure from the given
//      context
//

void
CSwBitmapColorSource::ComputeRealizationSize(
    __inout_ecount(1) CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatBitmapToSampleSpace,
    bool fPrefilterEnabled,
    REAL rPrefilterThreshold,
    __inout_ecount(1) RealizationParameters &oRealizationParams
    )
{
    Assert(oRealizationParams.uBitmapWidth > 0);
    Assert(oRealizationParams.uBitmapHeight > 0);

    if (fPrefilterEnabled)
    {
        pmatBitmapToSampleSpace->AdjustForPrefiltering(
            oRealizationParams.uBitmapWidth,
            oRealizationParams.uBitmapHeight,
            rPrefilterThreshold,
            OUT &oRealizationParams.uWidth,
            OUT &oRealizationParams.uHeight
            );
    }
    else
    {
        oRealizationParams.uWidth  = oRealizationParams.uBitmapWidth;
        oRealizationParams.uHeight = oRealizationParams.uBitmapHeight;
    }

    Assert(oRealizationParams.uWidth > 0);
    Assert(oRealizationParams.uHeight > 0);

    oRealizationParams.fOnlyContainsSubRectOfSource = false;
    oRealizationParams.rcSourceContained.left = 0;
    oRealizationParams.rcSourceContained.top  = 0;
    oRealizationParams.rcSourceContained.right  = oRealizationParams.uWidth;
    oRealizationParams.rcSourceContained.bottom = oRealizationParams.uHeight;

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::ComputeRealizationParameters
//
//  Synopsis:
//      Compose a RealizationParameters structure from the given context
//

HRESULT
CSwBitmapColorSource::ComputeRealizationParameters(
    __inout_ecount(1) IWGXBitmapSource *pIBitmapSource,
    __inout_ecount(1) CMatrix<CoordinateSpace::RealizationSampling,CoordinateSpace::Device> *pmatBitmapToSampleSpace,
    __in_ecount(1) CColorSourceCreator const *pCSCreator,
    bool fPrefilterEnabled,
    REAL rPrefilterThreshold,
    __out_ecount(1) RealizationParameters &oRealizationParams,
    __out_ecount(1) bool &fNeedsRealization
    )
{
    HRESULT hr = S_OK;

    //
    // Determine texture properties
    //

    MilPixelFormat::Enum fmtBitmapSource;

    //
    // Determine texture format
    //
    // Note: This assumes that any prefiltering will support same formats that
    //       color source creator claims are supported.  For example if
    //       color source creator supports 32bppBGR then CBitmapScaler should
    //       also support it without conversion.
    //

    IFC(pIBitmapSource->GetPixelFormat(&fmtBitmapSource));

    oRealizationParams.fmtTexture =
        pCSCreator->GetSupportedSourcePixelFormat(
            fmtBitmapSource,
            false // ForceAlpha
            );

    fNeedsRealization = (fmtBitmapSource != oRealizationParams.fmtTexture);

    //
    // Determine texture size
    //

    IFC(pIBitmapSource->GetSize(&oRealizationParams.uBitmapWidth,
                                &oRealizationParams.uBitmapHeight));

    ComputeRealizationSize(
        pmatBitmapToSampleSpace,
        fPrefilterEnabled,
        rPrefilterThreshold,
        IN OUT oRealizationParams
        );

    //
    // Determine if realization is even required
    //

    fNeedsRealization = fNeedsRealization
        || (oRealizationParams.rcSourceContained.Width() != oRealizationParams.uBitmapWidth)
        || (oRealizationParams.rcSourceContained.Height() != oRealizationParams.uBitmapHeight);

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::Create
//
//  Synopsis:
//      Creates a Sw bitmap color source
//
//------------------------------------------------------------------------------

HRESULT
CSwBitmapColorSource::Create(
    __in_ecount_opt(1) IWGXBitmap *pBitmap,
    __deref_out_ecount(1) CSwBitmapColorSource ** const ppSwBitmapCS
    )
{
    HRESULT hr = S_OK;

    *ppSwBitmapCS = new CSwBitmapColorSource(pBitmap);
    IFCOOM(*ppSwBitmapCS);
    (*ppSwBitmapCS)->AddRef();

Cleanup:
    if (FAILED(hr))
    {
        Assert(*ppSwBitmapCS == NULL);
    }
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::CSwBitmapColorSource
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------

CSwBitmapColorSource::CSwBitmapColorSource(
    __in_ecount_opt(1) IWGXBitmap *pBitmap
    ) :
    m_pBitmap(pBitmap)
{
    m_uRealizationWidth = UINT_MAX;   // Unreasonable->invalid default
    m_uRealizationHeight = UINT_MAX;  // Unreasonable->invalid default
    m_pRealizationBitmap = NULL;
    m_pIBitmapSource = NULL;
    m_uCachedUniquenessToken = 0;
    m_fValidRealization = false;
    
#if DBG
    // Set the source here to enable an assertion in SetBitmapAndContext that
    // the bitmap source doesn't change when there is an IWGXBitmap.
    m_pIBitmapSourceDbg = pBitmap;
#endif
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::~CSwBitmapColorSource
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------

CSwBitmapColorSource::~CSwBitmapColorSource()
{
    ReleaseInterfaceNoNULL(m_pRealizationBitmap);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::CheckValidRealization
//
//  Synopsis:
//      Validates m_fValidRealization, indicating whether the current texture
//      contains a useful realization of the current device independent bitmap.
//

void
CSwBitmapColorSource::CheckValidRealization()
{
    if (m_pBitmap && m_fValidRealization)
    {
        UINT uBitmapUniquenessToken;
        m_pBitmap->GetUniquenessToken(&uBitmapUniquenessToken);
        if (m_uCachedUniquenessToken != uBitmapUniquenessToken)
        {
            m_fValidRealization = false;
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::SetBitmapAndContext
//
//  Synopsis:
//      Set the current context and bitmap this color source is to realize
//

void
CSwBitmapColorSource::SetBitmapAndContext(
    __in_ecount(1) IWGXBitmapSource *pBitmapSource,
    __in_ecount(1) const RealizationParameters &oRealizationParams
    )
{
    CheckValidRealization();

#if DBG
    if (m_pIBitmapSourceDbg != pBitmapSource)
    {
        // Current caching prevents the source from changing except for the
        // initial call.  See CSwBitmapCache::ChooseBitmapColorSource's cache
        // destruction for more.  (The disable cache destruction you need to
        // make this whole block work under free, disable this assert, set
        // m_fValidRealization to false, replace m_pIBitmapSourceDbg with
        // m_pIBitmapSource, and remove appropriate m_pIBitmapSource sets.
        Assert(m_pIBitmapSourceDbg == NULL);

        // source should never change if this is associated with a IWGXBitmap
        Assert(!m_pBitmap);

        // If the source is changing we should have been fully invalidated
        Assert(!m_fValidRealization);

        m_pIBitmapSourceDbg = pBitmapSource;
        // No Reference held for m_pIBitmapSourceDbg
        //m_pIBitmapSourceDbg->AddRef();
    }
#endif DBG

    m_pIBitmapSource = pBitmapSource;
    // No Reference held for m_pIBitmapSource
    //m_pIBitmapSource->AddRef();

    m_fmtTexture = oRealizationParams.fmtTexture;

    m_uPrefilterWidth = oRealizationParams.uWidth;
    m_uPrefilterHeight = oRealizationParams.uHeight;

    m_rcPrefilteredBitmap = oRealizationParams.rcSourceContained;

    m_uRealizationWidth = oRealizationParams.rcSourceContained.Width();
    m_uRealizationHeight = oRealizationParams.rcSourceContained.Height();

    m_uBitmapWidth = oRealizationParams.uBitmapWidth;
    m_uBitmapHeight = oRealizationParams.uBitmapHeight;

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::CreateTexture
//
//  Synopsis:
//      Creates the texture (realization bitmap)
//

HRESULT
CSwBitmapColorSource::CreateTexture(
    )
{
    HRESULT hr = S_OK;

    Assert(m_pRealizationBitmap == NULL);

    Assert(m_fmtTexture != MilPixelFormat::Undefined);
    Assert(m_uRealizationWidth);
    Assert(m_uRealizationHeight);

    m_pRealizationBitmap = new CSystemMemoryBitmap();
    IFCOOM(m_pRealizationBitmap);
    m_pRealizationBitmap->AddRef();

    IFC(m_pRealizationBitmap->Init(
        m_uRealizationWidth,
        m_uRealizationHeight,
        m_fmtTexture,
        FALSE
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::FillTexture
//
//  Synopsis:
//      Copies the bitmap samples over to the texture (realization bitmap)
//

HRESULT
CSwBitmapColorSource::FillTexture(
    )
{
    HRESULT hr = S_OK;

    IWICBitmapSource *pIWICBitmapSourceNoRef = NULL;
    IWICBitmapSource *pIWGXWrapperBitmapSource = NULL;
    IWGXBitmapSource *pIWICWrapperBitmapSource = NULL;
    IWICBitmapScaler *pIWICScaler = NULL;
    IWICImagingFactory *pIWICFactory = NULL;
    IWICFormatConverter *pConverter = NULL;

    IFC(WrapInClosestBitmapInterface(m_pIBitmapSource, &pIWGXWrapperBitmapSource));
    pIWICBitmapSourceNoRef = pIWGXWrapperBitmapSource; // No ref changes
    
    //
    // Add a bitmap scaler, if needed.
    //

    #if DBG
    {
        UINT uWidth, uHeight;
        Assert(SUCCEEDED(pIWICBitmapSourceNoRef->GetSize(&uWidth, &uHeight)));
        Assert(m_uBitmapWidth  == uWidth);
        Assert(m_uBitmapHeight == uHeight);
    }
    #endif

    Assert(m_uBitmapWidth <= INT_MAX);
    Assert(m_uBitmapHeight <= INT_MAX);
    Assert(m_uPrefilterWidth <= INT_MAX);
    Assert(m_uPrefilterHeight <= INT_MAX);

    if (   (m_uBitmapWidth  != m_uPrefilterWidth)
        || (m_uBitmapHeight != m_uPrefilterHeight))
    {
        IFC(WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION_WPF, &pIWICFactory));
        IFC(pIWICFactory->CreateBitmapScaler(&pIWICScaler));

        IFC(pIWICScaler->Initialize(pIWICBitmapSourceNoRef,
                                    m_uPrefilterWidth,
                                    m_uPrefilterHeight,
                                    WICBitmapInterpolationModeFant));

        pIWICBitmapSourceNoRef = pIWICScaler;  // No ref changes
    }

    //
    // Convert all pixel formats to a format appropriate for rendering.
    //

    MilPixelFormat::Enum fmtBitmap;
    IFC(m_pIBitmapSource->GetPixelFormat(&fmtBitmap));

    if (fmtBitmap != m_fmtTexture)
    {
        if (!pIWICFactory)
        {
            IFC(WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION_WPF, &pIWICFactory));
        }

        IFC(pIWICFactory->CreateFormatConverter(&pConverter));
        IFC(pConverter->Initialize(
            pIWICBitmapSourceNoRef,
            MilPfToWic(m_fmtTexture),
            WICBitmapDitherTypeNone,
            NULL,
            0.0f,
            WICBitmapPaletteTypeCustom
            ));    
        pIWICBitmapSourceNoRef = pConverter;  // No ref changes
    }

    //
    // Validate size
    //

    #if DBG
    {
        UINT uWidth, uHeight;
        Assert(SUCCEEDED(pIWICBitmapSourceNoRef->GetSize(&uWidth, &uHeight)));
        Assert(m_uPrefilterWidth == uWidth);
        Assert(m_uPrefilterHeight == uHeight);
    }
    #endif

    if (   (m_uRealizationWidth  < (m_rcPrefilteredBitmap.right - m_rcPrefilteredBitmap.left))
        || (m_uRealizationHeight < (m_rcPrefilteredBitmap.bottom - m_rcPrefilteredBitmap.top)))
    {
        RIP("Source bitmap rect is larger than destination.");
        IFC(WGXERR_INTERNALERROR);
    }

    IFC(WrapInClosestBitmapInterface(pIWICBitmapSourceNoRef, &pIWICWrapperBitmapSource));

    IFC(FillTextureWithTransformedSource(
        pIWICWrapperBitmapSource
        ));

Cleanup:
    ReleaseInterfaceNoNULL(pIWGXWrapperBitmapSource);
    ReleaseInterfaceNoNULL(pIWICFactory);
    ReleaseInterfaceNoNULL(pIWICScaler);
    ReleaseInterfaceNoNULL(pConverter);
    ReleaseInterfaceNoNULL(pIWICWrapperBitmapSource);

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Function:
//      ScaleIntervalToPrefiltered
//
//  Synopsis:
//      Adjust interval for a uOriginalSize sized domain to a prefiltered,
//      uPrefilterSize sized domain
//
//      Rounding always expands the interval to include more.
//
//------------------------------------------------------------------------------

extern VOID
ScaleIntervalToPrefiltered(
    __inout_ecount(1) UINT &uStart,
    __inout_ecount(1) UINT &uEnd,
    UINT uOriginalSize,
    UINT uPrefilterSize
    );


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::FillTextureWithTransformedSource
//
//  Synopsis:
//      Copies the bitmap samples over to the texture.  The incoming source must
//      be in the format of the texture and it should already have a prefilter
//      transformation applied if necessary.
//

HRESULT
CSwBitmapColorSource::FillTextureWithTransformedSource(
    __in_ecount(1) IWGXBitmapSource *pIBitmapSource
    )
{
    HRESULT hr = S_OK;

    const MilRectU *rgDirtyRects = NULL;
    UINT cDirtyRects = 0;

    //
    // Get the list of dirty rects
    //

    CMilRectU rcTempLocalStorage; // must stay alive as long as we use rgDirtyRects
    GetDirtyRects(
        &rgDirtyRects,
        &cDirtyRects,
        &rcTempLocalStorage
        );
    
    //
    // Iterate through rectangles that need updated
    //

    for (UINT i = 0; i < cDirtyRects; i++)
    {
        CMilRectU rc(rgDirtyRects[i]);

        Assert(rc.right <= m_uBitmapWidth);
        Assert(rc.bottom <= m_uBitmapHeight);

        //
        // Adjust rect as needed if there is prefiltering
        //

        if (m_uBitmapWidth != m_uPrefilterWidth)
        {
            ScaleIntervalToPrefiltered(
                IN OUT rc.left,
                IN OUT rc.right,
                m_uBitmapWidth,
                m_uPrefilterWidth
                );
        }

        if (m_uBitmapHeight != m_uPrefilterHeight)
        {
            ScaleIntervalToPrefiltered(
                IN OUT rc.top,
                IN OUT rc.bottom,
                m_uBitmapHeight,
                m_uPrefilterHeight
                );
        }


        Assert(rc.right <= m_uPrefilterWidth);
        Assert(rc.bottom <= m_uPrefilterHeight);

#if NEVER
        EventWriteBitmapCopyInfo(rc.right-rc.left, rc.bottom - rc.top);
#endif

        //
        // Clip to portion of source stored in destination
        //

        if (rc.Intersect(m_rcPrefilteredBitmap))
        {
            //
            // Update realization bitmap
            //

            IFC(m_pRealizationBitmap->UnsafeUpdateFromSource(
                pIBitmapSource,
                rc,
                rc.left - m_rcPrefilteredBitmap.left,
                rc.top - m_rcPrefilteredBitmap.top
                ));
        }
        // continue with next dirty rect
    }


Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::GetDirtyRects
//
//  Synopsis:
//      Gets an array of dirty rects from the bitmap.  If there are no dirty
//      rects, this method will return one rectangle the entire size of the
//      original bitmap.
//

VOID
CSwBitmapColorSource::GetDirtyRects(
    __deref_out_ecount(*pcDirtyRects) const MilRectU **const prgDirtyRects,
    __out_ecount(1) UINT *pcDirtyRects,
    __inout_ecount(1) CMilRectU *prcTempLocalStorage
    )
{
    //
    // Check for dirty rects if we are associated with an IWGXBitmap
    //
    
    if (m_pBitmap)
    {
        if (!m_pBitmap->GetDirtyRects(OUT prgDirtyRects,
                                      OUT pcDirtyRects,
                                      IN OUT &m_uCachedUniquenessToken
                                      ))
        {
            *pcDirtyRects = 0;
        }
    }
    else
    {
        *pcDirtyRects = 0;
    }

    //
    // Zero length list means the entire realization surface is invalid.
    //
    // Note that GetDirtyRects may have returned true and 0 as the dirty rect
    // count indicating that cached uniqueness matched bitmap's uniqueness, but
    // in it is expected that before reaching this code an exact uniqueness
    // match has already been checked.  So an exact match here is simply a
    // "coincidence", but really the entire cache needs re-realized.  This will
    // happen upon first allocation of the realization surface
    // (m_pRealizationBitmap).
    //
    if (*pcDirtyRects == 0)
    {
        //
        // Note: do not use prefiltered size since we adjust for that later.
        //

        prcTempLocalStorage->left = 0;
        prcTempLocalStorage->top = 0;
        prcTempLocalStorage->right = m_uBitmapWidth;
        prcTempLocalStorage->bottom = m_uBitmapHeight;

        *prgDirtyRects = prcTempLocalStorage;
        *pcDirtyRects = 1;
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::IsValid
//
//  Synopsis:
//      Determine if this is valid; simply check if Sw resource is present
//

bool
CSwBitmapColorSource::IsValid() const
{
    return (m_pRealizationBitmap != NULL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::IsOpaque
//
//  Synopsis:
//      Does the source contain alpha?  This method tells you.
//
//------------------------------------------------------------------------------

bool
CSwBitmapColorSource::IsOpaque(
    ) const
{
    return !HasAlphaChannel(m_fmtTexture);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwBitmapColorSource::Realize
//
//  Synopsis:
//      Create or get a realization of the current device independent bitmap. 
//      If already in the cache, just make sure the current realization still
//      works in this context.
//

HRESULT
CSwBitmapColorSource::Realize(
    )
{
    HRESULT hr = S_OK;

    Assert(m_pIBitmapSource);

#if DBG
    if (m_pRealizationBitmap)
    {
        //
        // Check if existing texture is valid and if it has enough texels
        // required for handling realization
        //

        UINT uWidth, uHeight;

        VerifySUCCEEDED(m_pRealizationBitmap->GetSize(&uWidth, &uHeight));

        Assert(m_uRealizationWidth == uWidth);
        Assert(m_uRealizationHeight == uHeight);
    }
#endif

    if (!m_pRealizationBitmap)
    {
        //
        // Create a new texture
        //

        IFC(CreateTexture());

        // Anytime a new texture is allocated, a realization is needed.
        m_fValidRealization = false;
        if (m_pBitmap)
        {
            // Set cached uniqueness to current which will avoid possibility of
            // getting non-zero length dirty list from IWGXBitmap::GetDirtyRects. 
            // This will result in a zero length dirty rect list which will
            // then be detected as a need for complete realization.  See
            // CSwBitmapColorSource::GetDirtyRects.
            m_pBitmap->GetUniquenessToken(&m_uCachedUniquenessToken);
        }
    }

    if (!m_fValidRealization)
    {
        //
        // Populate the texture
        //

        IFC(FillTexture());

        // Successful population means there is a valid realization.
        m_fValidRealization = true;
    }

Cleanup:
    RRETURN(hr);
}




