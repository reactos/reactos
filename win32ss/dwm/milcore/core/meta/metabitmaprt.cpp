// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_targets
//      $Keywords:
//
//  $Description:
//      CMetaBitmapRenderTarget implementation
//
//      This is a multiple or meta Render Target for rendering on multiple
//      offscreen surfaces.  This is also a meta Bitmap Source that holds
//      references to IWGXBitmapSources specific to the sub Render Targets.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CMetaBitmapRenderTarget, MILRender, "CMetaBitmapRenderTarget");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::operator new
//
//  Synopsis:
//      Allocate memory for one rendertarget, plus additional memory for each
//      sub-render target
//
//------------------------------------------------------------------------------
void * __cdecl CMetaBitmapRenderTarget::operator new(
    size_t cb,  // bytes of memory to allocate for render target
    size_t cRTs // number of sub-render targets
    )
{
    HRESULT hr = S_OK;
    void *pBuffer = NULL;

    size_t cbBufferSize = 0;

    IFC(SizeTMult(cRTs, sizeof(MetaData), &cbBufferSize));
    IFC(SizeTAdd(cbBufferSize, cb, &cbBufferSize));

    pBuffer = WPFAlloc(
        ProcessHeap,
        Mt(CMetaBitmapRenderTarget),
        cbBufferSize
        );

Cleanup:
    return pBuffer;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::Create
//
//  Synopsis:
//      Create a CMetaBitmapRenderTarget
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaBitmapRenderTarget::Create(
    UINT uWidth,                        // Width of render targets
    UINT uHeight,                       // Height of render targets
    UINT cRTs,                          // Count of render targets
    __in_ecount(cRTs)
    MetaData *pMetaData,                // Array of render target data
    __in_ecount(1) CDisplaySet const *pDisplaySet,
    IntermediateRTUsage usageInfo,
    MilRTInitialization::Flags dwFlags,
    __deref_out_ecount(1) CMetaBitmapRenderTarget **ppMetaRT  // Location to return object pointer
    )
{
    HRESULT hr = S_OK;

    *ppMetaRT = NULL;

    //
    // Create the CMetaBitmapRenderTarget
    //

    *ppMetaRT = new(cRTs) CMetaBitmapRenderTarget(cRTs, pDisplaySet);
     IFCOOM(*ppMetaRT);
     (*ppMetaRT)->AddRef(); // CMetaBitmapRenderTarget::ctor sets ref count == 0

     //
     // Call Init
     //

     IFC((*ppMetaRT)->Init(
         uWidth,
         uHeight,
         usageInfo,
         dwFlags,
         pMetaData
         ));

Cleanup:
    if (FAILED(hr))
    {
        ReleaseInterface(*ppMetaRT);
    }
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::CMetaBitmapRenderTarget
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------

#pragma warning( push )
#pragma warning( disable : 4355 )

CMetaBitmapRenderTarget::CMetaBitmapRenderTarget(
    UINT cMaxRTs,
    __inout_ecount(1) CDisplaySet const *pDisplaySet
    )
    : CMetaRenderTarget(
        reinterpret_cast<MetaData *>(reinterpret_cast<BYTE *>(this) + sizeof(*this)),
        cMaxRTs,
        pDisplaySet
        )
{
}

#pragma warning( pop )

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::~CMetaBitmapRenderTarget
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------
CMetaBitmapRenderTarget::~CMetaBitmapRenderTarget()
{
    for (UINT i = 0; i < m_cRT; i++)
    {
        ReleaseInterface(m_rgMetaData[i].pIRTBitmap);
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::HrFindInterface
//
//  Synopsis:
//      HrFindInterface implementation
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaBitmapRenderTarget::HrFindInterface(
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
        else if (riid == IID_IWGXBitmapSource)
        {
            *ppvObject = static_cast<IWGXBitmapSource*>(this);

            hr = S_OK;
        }
        else if (riid == IID_CMetaBitmapRenderTarget)
        {
            *ppvObject = this;

            hr = S_OK;
        }
        else
        {
            hr = CMetaRenderTarget::HrFindInterface(riid, ppvObject);
        }
    }

    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::Init
//
//  Synopsis:
//      Inits the meta render target and allocates the required resources
//
//------------------------------------------------------------------------------
HRESULT
CMetaBitmapRenderTarget::Init(
    UINT uWidth,
    UINT uHeight,
    IntermediateRTUsage usageInfo,
    MilRTInitialization::Flags dwFlags,
    __in_xcount(m_cRTs) MetaData const *pMetaData
    )
{
    HRESULT hr = S_OK;

    //
    // Intialize basic members
    //

    m_uWidth = uWidth;
    m_uHeight = uHeight;

    //
    // Create bitmap RTs for each RT and remember
    // their source bitmaps.
    //

    Assert(m_cRT*sizeof(m_rgMetaData[0]) ==
           RtlCompareMemoryUlong(m_rgMetaData, m_cRT*sizeof(m_rgMetaData[0]), 0));

    // The width and height are converted to floats when clipping,
    // make sure we don't expect values TOO big as input.
    if (uWidth > SURFACE_RECT_MAX || uHeight > SURFACE_RECT_MAX)
    {
        IFC(WGXERR_UNSUPPORTEDTEXTURESIZE);
    }

    for (UINT i=0; i < m_cRT && SUCCEEDED(hr); i++)
    {
        //
        // Initialize the cache index to invalid. We only want the meta data
        // object which has a non-null pIRTBitmap to have a valid cache index.
        //
        m_rgMetaData[i].cacheIndex = CMILResourceCache::InvalidToken;

        #if DBG
        m_rgMetaData[i].uIndexOfRealRTBitmap = UINT_MAX;
        #endif
        
        if (pMetaData[i].fEnable)
        {
            ULONG curCacheIndex = pMetaData[i].pInternalRT->GetRealizationCacheIndex();
            if (curCacheIndex == CMILResourceCache::InvalidToken)
            {
                curCacheIndex = CMILResourceCache::SwRealizationCacheIndex;
            }


            // Initialize the index 'pointer' to itself.
            m_rgMetaData[i].uIndexOfRealRTBitmap = i;

            // Future Consideration: We may want to revisit this sharing code.  If it is cheap to share
            //     textures (i.e. they have the same underlying video card and we have 9EX
            //     devices) we may want to enable that.  Currently this is only used in software
            //     mode.
            //
            // Search for bitmap RTs created for other displays that we could
            // share. We allow sharing of RTs when the displays share a device.
            // [We use the cache index to differentiate devices.] We also allow
            // sharing of software bitmap render targets- we don't bother to
            // create hardware render targets when we already have a software
            // render target avaliable for use.
            //
            for (UINT j = 0; j < i; j++)
            {
                if (   m_rgMetaData[j].cacheIndex == curCacheIndex
                    || m_rgMetaData[j].cacheIndex == CMILResourceCache::SwRealizationCacheIndex
                   )
                {
                    //
                    // Found a match. There is no need to create two identical
                    // render targets so we will 'point' back to the matching
                    // one.
                    //
                    m_rgMetaData[i].uIndexOfRealRTBitmap = j;
                    break;
                }
            }

            if (m_rgMetaData[i].uIndexOfRealRTBitmap != i)
            {
                m_rgMetaData[i].cacheIndex = CMILResourceCache::InvalidToken;
                Assert(m_rgMetaData[i].fEnable == FALSE);
            }
            else
            {
                MIL_THR(pMetaData[i].pInternalRT->CreateRenderTargetBitmap(
                    uWidth,
                    uHeight,
                    usageInfo,
                    dwFlags,
                    &m_rgMetaData[i].pIRTBitmap
                    ));
    
                if (SUCCEEDED(hr))
                {
                #if DBG
                    if (pMetaData[i].pInternalRT->GetRealizationCacheIndex() == CMILResourceCache::InvalidToken)
                    {
                        //
                        // Assert that CreateRenderTargetBitmap created a
                        // software RT. This justifies setting curCacheIndex to
                        // CMILResourceCache::SwRealizationCacheIndex above
                        //

                        Assert(curCacheIndex == CMILResourceCache::SwRealizationCacheIndex);
                        
                        // The DYNCAST does the assert
                        DYNCAST(CSwRenderTargetBitmap, m_rgMetaData[i].pIRTBitmap);
                    }
                #endif

                    hr = THR(m_rgMetaData[i].pIRTBitmap->QueryInterface(
                        IID_IRenderTargetInternal,
                        (void **)&(m_rgMetaData[i].pInternalRT)
                        ));
                }

                if (SUCCEEDED(hr))
                {
                    //
                    // Enable rendering to the new RT upon success
                    //
                    m_rgMetaData[i].fEnable = TRUE;

                    //
                    // Set the cache index to the cache index of the newly
                    // created bitmap render target. It is important not to use
                    // the cache index of the original render target, as
                    // hardware render targets are allowed to create software
                    // render targets.
                    //
                    m_rgMetaData[i].cacheIndex = m_rgMetaData[i].pInternalRT->GetRealizationCacheIndex();
    
                    // Set the bounds either way
                    Assert(m_rgMetaData[i].rcLocalDeviceRenderBounds.left == 0);
                    Assert(m_rgMetaData[i].rcLocalDeviceRenderBounds.top == 0);
                    m_rgMetaData[i].rcLocalDeviceRenderBounds.right  = static_cast<LONG>(uWidth);
                    m_rgMetaData[i].rcLocalDeviceRenderBounds.bottom = static_cast<LONG>(uHeight);
                    m_rgMetaData[i].rcLocalDevicePresentBounds =
                        m_rgMetaData[i].rcLocalDeviceRenderBounds;
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//+========================================================================
// IMILRenderTarget methods
//=========================================================================

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::GetBounds
//
//  Synopsis:
//      Return accumulated bounds of all render targets
//
//------------------------------------------------------------------------------

STDMETHODIMP_(VOID) CMetaBitmapRenderTarget::GetBounds(
    __out_ecount(1) MilRectF * const pBounds
    )
{
    CMilRectF rcAcc;

    rcAcc.SetEmpty();

    for (UINT idx = 0; idx < m_cRT; idx++)
    {
        // Acuumulate bounds of all RTs as long as there is an RT
        if (m_rgMetaData[idx].pInternalRT)
        {
            m_rgMetaData[idx].pInternalRT->GetBounds(pBounds);

            rcAcc.Union(*pBounds);
        }
    }

    *pBounds = rcAcc;

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::Clear
//
//  Synopsis:
//      Clear the surface to a given color.
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaBitmapRenderTarget::Clear(
    __in_ecount_opt(1) const MilColorF *pColor,
    __in_ecount_opt(1) const CAliasedClip *pAliasedClip
    )
{
    return CMetaRenderTarget::Clear(pColor, pAliasedClip);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::Begin3D, End3D
//
//  Synopsis:
//      Assert state then delegate to base class
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaBitmapRenderTarget::Begin3D(
    __in_ecount(1) MilRectF const &rcBounds,
    MilAntiAliasMode::Enum AntiAliasMode,
    bool fUseZBuffer,
    FLOAT rZ
    )
{
    return CMetaRenderTarget::Begin3D(rcBounds, AntiAliasMode, fUseZBuffer, rZ);
}

STDMETHODIMP
CMetaBitmapRenderTarget::End3D(
    )
{
    return CMetaRenderTarget::End3D();
}

//+========================================================================
// IMILRenderTargetBitmap methods
//=========================================================================

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::GetBitmapSource
//
//  Synopsis:
//      Return a bitmap source interface to the internal meta bitmap that holds
//      separate RT specific bitmaps
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaBitmapRenderTarget::GetBitmapSource(
    __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
    )
{
    Assert(m_rgMetaData);

    *ppIBitmapSource = this;
    (*ppIBitmapSource)->AddRef();

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::GetCacheableBitmapSource
//
//  Synopsis:
//      Return a bitmap source interface to the internal meta bitmap that holds
//      separate RT specific bitmaps
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaBitmapRenderTarget::GetCacheableBitmapSource(
    __deref_out_ecount(1) IWGXBitmapSource ** const ppIBitmapSource
    )
{   
    // To implement GetCacheableBitmapSource, we would need to go into each 
    // surface bitmap & ensure those bitmaps are cacheable. This functionality 
    // isn't currently supported because CMetaBitmapRenderTarget is not used
    // by any callers of GetCacheableBitmapSource.
    *ppIBitmapSource = NULL;
    RIP("CMetaBitmapRenderTarget::GetCacheableBitmapSource isn't implemented");
        
    RRETURN(E_NOTIMPL);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::GetBitmap
//
//  Synopsis:
//      Not implemented
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaBitmapRenderTarget::GetBitmap(
    __deref_out_ecount(1) IWGXBitmap ** const ppIBitmap
    )
{
    RRETURN(WGXERR_NOTIMPLEMENTED);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::GetNumQueuedPresents
//
//  Synopsis:
//      Forwards call to the CMetaRenderTarget member.
//
//------------------------------------------------------------------------------
HRESULT
CMetaBitmapRenderTarget::GetNumQueuedPresents(
    __out_ecount(1) UINT *puNumQueuedPresents
    )
{
    RRETURN(CMetaRenderTarget::GetNumQueuedPresents(puNumQueuedPresents));
}

//+========================================================================
// IWGXBitmapSource methods
//=========================================================================

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::GetSize
//
//  Synopsis:
//      Get pixel dimensions of bitmap
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaBitmapRenderTarget::GetSize(
    __out_ecount(1) UINT *puWidth,
    __out_ecount(1) UINT *puHeight
    )
{
    *puWidth = m_uWidth;
    *puHeight = m_uHeight;

    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::GetPixelFormat
//
//  Synopsis:
//      Get pixel format of bitmap
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaBitmapRenderTarget::GetPixelFormat(
    __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
    )
{
    RRETURN(E_ACCESSDENIED);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::GetResolution
//
//  Synopsis:
//      Not implemented
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaBitmapRenderTarget::GetResolution(
    __out_ecount(1) double *pDpiX,
    __out_ecount(1) double *pDpiY
    )
{
    RRETURN(E_ACCESSDENIED);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::CopyPalette
//
//  Synopsis:
//      Not implemented
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaBitmapRenderTarget::CopyPalette(
    __inout_ecount(1) IWICPalette *pIPalette
    )
{
    RRETURN(E_ACCESSDENIED);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::CopyPixels
//
//  Synopsis:
//      Access via CopyPixels method is not supported
//
//------------------------------------------------------------------------------
STDMETHODIMP
CMetaBitmapRenderTarget::CopyPixels(
    __in_ecount_opt(1) const MILRect *prc,
    UINT cbStride,
    UINT cbBufferSize,
    __out_ecount(cbBufferSize) BYTE *pvPixels
    )
{
    RRETURN(E_ACCESSDENIED);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::GetSubRenderTargetNoRef
//
//  Synopsis:
//      Walks the internal render targets, finding the one that matches the
//      cache index and display id.
//
//      The display id is optional, but if it exists it overrides the cache
//      index as a lookup mechanism.
//
//      Returns an error if no rendertarget was found.
//

HRESULT
CMetaBitmapRenderTarget::GetCompatibleSubRenderTargetNoRef(
    IMILResourceCache::ValidIndex uOptimalRealizationCacheIndex,
    DisplayId targetDestination,
    __deref_out_ecount(1) IMILRenderTargetBitmap **ppIRenderTargetNoRef
    )
{
    HRESULT hr = S_OK;

    IMILRenderTargetBitmap *pIRenderTargetNoRef = GetCompatibleSubRenderTargetNoRefInternal(
        uOptimalRealizationCacheIndex,
        targetDestination
        );

    if (pIRenderTargetNoRef == NULL)
    {
        RIPW(L"No internal intermediate render target found matching realization cache index!");
        MIL_THR(WGXERR_INTERNALERROR);
    }

    *ppIRenderTargetNoRef = pIRenderTargetNoRef;

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CMetaBitmapRenderTarget::GetCompatibleSubRenderTargetNoRefInternal
//
//  Synopsis:
//      Walks the internal render targets, finding the one that matches the
//      cache index and display id.
//
//      The display id is optional, but if it exists it overrides the cache
//      index as a lookup mechanism.
//

__out_ecount_opt(1) IMILRenderTargetBitmap *
CMetaBitmapRenderTarget::GetCompatibleSubRenderTargetNoRefInternal(
    IMILResourceCache::ValidIndex uOptimalRealizationCacheIndex,
    DisplayId targetDestination
    )
{

    // PREfast should catch violations, but doesn't seem to today - so assert
    Assert(uOptimalRealizationCacheIndex != CMILResourceCache::InvalidToken);

    IMILRenderTargetBitmap *pIRenderTargetNoRef = NULL;

    if (targetDestination.IsNone())
    {
        IMILResourceCache::ValidIndex uRealizationCacheIndexToLookFor =
            uOptimalRealizationCacheIndex;
    
    LookAgain:
        for (UINT i = 0; i < m_cRT; i++)
        {
            if (m_rgMetaData[i].fEnable)
            {
                UINT uRTRealizationIndex =
                    m_rgMetaData[i].pInternalRT->GetRealizationCacheIndex();
    
                // Meta bitmap RT should never be created with sub RT that has an
                // invalid cache index.
                Assert(uRTRealizationIndex != CMILResourceCache::InvalidToken);
    
                if (uRTRealizationIndex == uRealizationCacheIndexToLookFor)
                {
                    pIRenderTargetNoRef = m_rgMetaData[i].pIRTBitmap;
                }
            }
        }
    
        if (   pIRenderTargetNoRef == NULL
            && uRealizationCacheIndexToLookFor != CMILResourceCache::SwRealizationCacheIndex
           )
        {
            //
            // We were hoping to find a hardware intermediate, but no such
            // intermediate exists. Look for a software intermediate instead.
            //
    
            uRealizationCacheIndexToLookFor = CMILResourceCache::SwRealizationCacheIndex;
            goto LookAgain;
        }
    }
    else
    {
        UINT idx;
        Verify(SUCCEEDED(m_pDisplaySet->GetDisplayIndexFromDisplayId(
            targetDestination,
            OUT idx
            )));
        idx = m_rgMetaData[idx].uIndexOfRealRTBitmap;
        Assert(idx < m_cRT);
        Assert(m_rgMetaData[idx].fEnable);
        Assert(m_rgMetaData[idx].pIRTBitmap);

        pIRenderTargetNoRef = m_rgMetaData[idx].pIRTBitmap;
    }

    return pIRenderTargetNoRef;
}



