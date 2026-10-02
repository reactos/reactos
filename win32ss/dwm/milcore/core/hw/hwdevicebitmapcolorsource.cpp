// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
// See the LICENSE file in the project root for more information.


//+-----------------------------------------------------------------------------
//

//
//  $TAG ENGR

//      $Module:    win_mil_graphics_interop
//      $Keywords:
//
//  $Description:
//      Contains CHwDeviceBitmapColorSource implementation
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"


MtDefine(CHwDeviceBitmapColorSource, MILRender, "CHwDeviceBitmapColorSource");


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::CreateForTexture
//
//  Synopsis:
//      Create method which creates a bitmap color source from an existing
//      texture.
//

HRESULT
CHwDeviceBitmapColorSource::CreateForTexture(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) IWGXBitmap *pBitmap,
    MilPixelFormat::Enum fmt,
    __in_ecount(1) CMilRectU const &rcBoundsRequired,
    __inout_ecount(1) CD3DVidMemOnlyTexture *pVidMemTexture,
    __deref_out_ecount(1) CHwDeviceBitmapColorSource **ppHwBitmapCS
    )
{
    WHEN_DBG(Assert(pVidMemTexture->DbgIsAssociatedWithDevice(pDevice)));

    return CHwDeviceBitmapColorSource::CreateInternal(
        pDevice,
        pBitmap,
        fmt,
        rcBoundsRequired,
        pVidMemTexture,
        ppHwBitmapCS,
        NULL                    // pSharedHandle
        );
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::CreateWithSharedHandle
//
//  Synopsis:
//      Create method which creates a bitmap color source containing a new
//      texture. This texture can be referenced by the shared handle.
//
//      This method can be used in WDDM only.
//

HRESULT
CHwDeviceBitmapColorSource::CreateWithSharedHandle(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) IWGXBitmap *pBitmap,
    MilPixelFormat::Enum fmt,
    __in_ecount(1) CMilRectU const &rcBoundsRequired,
    __deref_out_ecount(1) CHwDeviceBitmapColorSource **ppHwBitmapCS,
    __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
    )
{
    return CHwDeviceBitmapColorSource::CreateInternal(
        pDevice,
        pBitmap,
        fmt,
        rcBoundsRequired,
        NULL,                   // pVidMemTexture
        ppHwBitmapCS,
        pSharedHandle
        );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::CreateCommon
//
//  Synopsis:
//      Helper used by this class and derived classes to create a surface
//      and realization description for the texture 
//
//------------------------------------------------------------------------------

/* static */ HRESULT
CHwDeviceBitmapColorSource::CreateCommon(
    __in_ecount(1) const CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) IWGXBitmap *pBitmap,
    MilPixelFormat::Enum fmt,
    __in_ecount(1) const CMilRectU &rcBoundsRequired,
    __in_ecount_opt(1) const CD3DVidMemOnlyTexture *pVidMemTexture,
    __out_ecount(1) CacheParameters &oRealizationDesc,
    __out_ecount(1) D3DSURFACE_DESC &d3dsd,
    __out_ecount(1) UINT &uLevels
    )
{
    HRESULT hr = S_OK;
        
    CHwDeviceBitmapColorSource *pbcs = NULL;
    
    oRealizationDesc.fmtTexture = fmt;

    IFC(pBitmap->GetSize(
        &oRealizationDesc.uWidth,
        &oRealizationDesc.uHeight
        ));

    oRealizationDesc.eMipMapLevel = TMML_One;
    oRealizationDesc.rcSourceContained = rcBoundsRequired;

    oRealizationDesc.dlU.uLength = oRealizationDesc.rcSourceContained.Width<UINT>();
    oRealizationDesc.dlU.eLayout = NaturalTexelLayout;
    oRealizationDesc.dlU.d3dta   = D3DTADDRESS_CLAMP;

    oRealizationDesc.dlV.uLength = oRealizationDesc.rcSourceContained.Height<UINT>();
    oRealizationDesc.dlV.eLayout = NaturalTexelLayout;
    oRealizationDesc.dlV.d3dta   = D3DTADDRESS_CLAMP;

    oRealizationDesc.fOnlyContainsSubRectOfSource =
           (oRealizationDesc.dlU.uLength != oRealizationDesc.uWidth)
        || (oRealizationDesc.dlV.uLength != oRealizationDesc.uHeight);    

    //
    // Underlying texture/surface description is not allowed to change over
    // time.  Compute/get them it now and send to the constructor.
    //

    if (pVidMemTexture)
    {
        d3dsd = pVidMemTexture->D3DSurface0Desc();
        uLevels = pVidMemTexture->Levels();
    }
    else
    {
        GetD3DSDRequired(
            pDevice,
            oRealizationDesc,
            &d3dsd,
            &uLevels
            );

        // Shared texture is read-only source for this module, but requestor will
        // update it with StretchRect; so make it a render target.
        d3dsd.Usage |= D3DUSAGE_RENDERTARGET;
    }

    if (   d3dsd.Width > pDevice->GetMaxTextureWidth()
        || d3dsd.Height > pDevice->GetMaxTextureHeight())
    {
        IFC(WGXERR_MAX_TEXTURE_SIZE_EXCEEDED);
    }

    AssertMinimalTextureDesc(
        pDevice,
        oRealizationDesc.dlU.d3dta,
        oRealizationDesc.dlV.d3dta,
        &d3dsd
        );

Cleanup:
    ReleaseInterfaceNoNULL(pbcs);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::CreateInternal
//
//  Synopsis:
//      Creates a HW device bitmap color source.
//
//      This method is meant to be used in one of two ways:
//      1. To create a bitmap color source using an existing texture
//              pVidMemTexture should be non-NULL
//              pSharedHandle should be NULL
//      2. To create a bitmap color source with a new shared texture (WDDM only)
//              pVidMemTexture should be NULL
//              pSharedHandle should be non-NULL
//
//------------------------------------------------------------------------------

HRESULT
CHwDeviceBitmapColorSource::CreateInternal(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_ecount(1) IWGXBitmap *pBitmap,
    MilPixelFormat::Enum fmt,
    __in_ecount(1) CMilRectU const &rcBoundsRequired,
    __inout_ecount_opt(1) CD3DVidMemOnlyTexture *pVidMemTexture,
    __deref_out_ecount(1) CHwDeviceBitmapColorSource **ppHwBitmapCS,
    __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
    )
{
    // We can't have both existing surface and shared handle request. In the
    // (so far unused) case that neither is requested a new surface with a
    // shared handle will be created, but the shared handle won't be returned
    Assert(!(pVidMemTexture && pSharedHandle));

    HRESULT hr = S_OK;
    CHwDeviceBitmapColorSource *pbcs = NULL;
    CacheParameters oRealizationDesc;
    D3DSURFACE_DESC d3dsd;
    UINT uLevels;

    IFC(CHwDeviceBitmapColorSource::CreateCommon(
        pDevice,
        pBitmap,
        fmt,
        rcBoundsRequired,
        pVidMemTexture,
        OUT oRealizationDesc,
        OUT d3dsd,
        OUT uLevels
        ));

    pbcs = new CHwDeviceBitmapColorSource(
        pDevice,
        NULL,        // color source is read-only and shouldn't be affected by IWGXBitmap changes (e.g. dirty rects)
        fmt, 
        d3dsd, 
        uLevels
        );
    IFCOOM(pbcs);
    pbcs->AddRef();

    IFC(pbcs->Init(
        pBitmap,
        oRealizationDesc,
        pVidMemTexture,
        pSharedHandle
        ));

    *ppHwBitmapCS = pbcs;
    pbcs = NULL;

Cleanup:
    ReleaseInterfaceNoNULL(pbcs);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::CHwDeviceBitmapColorSource
//
//  Synopsis:
//      ctor - delegate to CHwBitmapColorSource ctor; init device bitmap list to
//      none
//
//------------------------------------------------------------------------------

CHwDeviceBitmapColorSource::CHwDeviceBitmapColorSource(
    __in_ecount(1) CD3DDeviceLevel1 *pDevice,
    __in_opt IWGXBitmap *pBitmap,
    MilPixelFormat::Enum fmt,
    __in_ecount(1) const D3DSURFACE_DESC &d3dsd,
    UINT uLevels
    ) :
    CHwBitmapColorSource(pDevice, pBitmap, fmt, d3dsd, uLevels),
    m_hSharedHandle(NULL), m_pSysMemTexture(NULL)
{
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::~CHwDeviceBitmapColorSource
//
//  Synopsis:
//      dtor - clean up device bitmap list
//
//------------------------------------------------------------------------------

CHwDeviceBitmapColorSource::~CHwDeviceBitmapColorSource(
    )
{
    ReleaseInterface(m_pSysMemTexture);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::Init
//
//  Synopsis:
//      Create a device bitmap texture for this color source and initialize
//      basic properties so that it is ready to be used as a source assuming use
//      context matches given CacheParameters.
//
//------------------------------------------------------------------------------

HRESULT
CHwDeviceBitmapColorSource::Init(
    __in_ecount(1) IWGXBitmap *pBitmap,
    __in_ecount(1) CHwBitmapColorSource::CacheParameters const &oRealizationDesc,
    __inout_ecount_opt(1) CD3DVidMemOnlyTexture *pVidMemTexture,
    __deref_opt_inout_ecount(1) HANDLE * const pSharedHandle
    )
{
    HRESULT hr = S_OK;

    Assert(m_pIBitmapSource == NULL);

    IFC(pBitmap->GetSize(&m_uBitmapWidth, &m_uBitmapHeight));

    if (   m_uBitmapWidth > SURFACE_RECT_MAX
        || m_uBitmapHeight > SURFACE_RECT_MAX)
    {
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }

    if (pVidMemTexture)
    {
        m_pVidMemOnlyTexture = pVidMemTexture;
        m_pVidMemOnlyTexture->AddRef();
    }
    else
    {
        IFC(CreateTexture(/* fIsEvictable = */ false, pSharedHandle));
        
        if (pSharedHandle)
        {
            m_hSharedHandle = *pSharedHandle;
        }
    };

    //
    // Set basic context settings that are expected to be set when texture
    // is created and expected to be ready for use.
    //

    SetBitmapAndContextCacheParameters(
        pBitmap,
        oRealizationDesc
        );

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::Realize
//
//  Synopsis:
//      Shared color sources are read-only and therefore always expected to be
//      realized.
//
//------------------------------------------------------------------------------

HRESULT
CHwDeviceBitmapColorSource::Realize(
    )
{
    return S_OK;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::GetPointerToValidSourceRects
//
//  Synopsis:
//      Return list of valid source rects.  List ownership is not given to
//      caller.
//
//------------------------------------------------------------------------------

HRESULT
CHwDeviceBitmapColorSource::GetPointerToValidSourceRects(
    __in_ecount_opt(1) IWGXBitmap *pBitmap,
    __out_ecount(1) UINT &cValidSourceRects,
    __deref_out_ecount_full(cValidSourceRects) CMilRectU const * &rgValidSourceRects
    ) const
{
    HRESULT hr;

    if (   pBitmap
        && (pBitmap->SourceState() == IWGXBitmap::SourceState::DeviceBitmap))
    {
        CDeviceBitmap *pDeviceBitmap =
            DYNCAST(CDeviceBitmap, pBitmap);
        Assert(pDeviceBitmap);

        hr = pDeviceBitmap->GetPointerToValidRectsForSurface(
            OUT cValidSourceRects,
            OUT rgValidSourceRects
            );
    }
    else
    {
        hr = CHwBitmapColorSource::GetPointerToValidSourceRects(
            pBitmap,
            OUT cValidSourceRects,
            OUT rgValidSourceRects
            );
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::IsAdapter
//
//  Synopsis:
//      Returns true if this color source was created for given adapter.
//
//------------------------------------------------------------------------------

bool
CHwDeviceBitmapColorSource::IsAdapter(
    LUID luidAdapter
    ) const
{
    Assert(IsValid());

    return (luidAdapter == m_pDevice->GetD3DAdapterLUID());
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::UpdateValidBounds
//
//  Synopsis:
//      Update the notion of which are of this color source has valid content.
//
//------------------------------------------------------------------------------

void
CHwDeviceBitmapColorSource::UpdateValidBounds(
    __in_ecount(1) CMilRectU const &rcValid
    )
{
    Assert(m_rcPrefilteredBitmap.DoesContain(rcValid));

    m_rcCachedRealizationBounds = rcValid;
    m_rcRequiredRealizationBounds = rcValid;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::CopyPixels
//
//  Synopsis:
//      Copy valid (realized) portions color source to system memory buffer
//      given.
//
//  Notes:
//      Any format conversion request will be passed to
//      CD3DSurface::ReadIntoSysMemBuffer which will pass it on to D3D device's
//      StretchRect.
//
//      This call may be made w/o first going through a corresponding Hw Render
//      Target that will take care entering device context.  Should the device
//      entry require acquisition of a critical section callers will need to
//      reviewed for potential deadlocks, since another device may be entered
//      when this call is made.
//

HRESULT
CHwDeviceBitmapColorSource::CopyPixels(
    __in_ecount(1) const CMilRectU &rcCopy,
    UINT cClipRects,
    __in_ecount_opt(cClipRects) const CMilRectU *rgClipRects,
    MilPixelFormat::Enum fmtOut,
    DBG_ANALYSIS_PARAM_COMMA(UINT cbBufferOut)
    __out_bcount_full(cbBufferOut) BYTE * const pbBufferOut,
    UINT nStrideOut
    )
{
    HRESULT hr = S_OK;

    // This call may be made w/o calling through a Hw RT.  See notes above.
    ENTER_DEVICE_FOR_SCOPE(*m_pDevice);

    CD3DSurface *pSrcSurface = NULL;

    CMilRectU rcValidCopy = m_rcCachedRealizationBounds;

    if (cClipRects == 1)
    {
        rcValidCopy.Intersect(rgClipRects[0]);
        cClipRects = 0;
        rgClipRects = NULL;
    }

    // Make sure copy rect is within valid bounds
    if (rcValidCopy.Intersect(rcCopy))
    {
        // Adjust pbBufferOut by amount inset by intersection with valid bounds
        UINT cbBufferInset, cbLeftInset;
        BYTE BitsPerPixel = GetPixelFormatSize(fmtOut);

        if (BitsPerPixel % BITS_PER_BYTE)
        {
            TraceTag((tagMILWarning,
                      "Call to CHwDeviceBitmapColorSource::CopyPixels requested fraction byte copy"));
            IFC(WGXERR_INVALIDPARAMETER);
        }

        ENTER_USE_CONTEXT_FOR_SCOPE(*m_pDevice);

        IFC(MultiplyUINT(nStrideOut, rcValidCopy.top - rcCopy.top, OUT cbBufferInset));
        IFC(MultiplyUINT(BitsPerPixel / BITS_PER_BYTE, rcValidCopy.left - rcCopy.left, OUT cbLeftInset));
        IFC(AddUINT(cbBufferInset, cbLeftInset, OUT cbBufferInset));

        Assert(cbBufferInset <= cbBufferOut);


        IFC(m_pVidMemOnlyTexture->GetD3DSurfaceLevel(0, OUT &pSrcSurface));

        rcValidCopy.Offset(
            -static_cast<INT>(m_rcPrefilteredBitmap.left),
            -static_cast<INT>(m_rcPrefilteredBitmap.top)
            );

        //
        // By this point the copy rect has been processed into a
        // (0,0)-(INT_MAX,INT_MAX) bound rectangle and should not be empty.
        // This means it may be directly cast to an integer based rectangle.
        //
        Assert(!rcValidCopy.IsEmpty());
        Assert(rcValidCopy.right <= INT_MAX);
        Assert(rcValidCopy.bottom <= INT_MAX);

        IFC(pSrcSurface->ReadIntoSysMemBuffer(
            rcValidCopy,
            cClipRects,
            rgClipRects,
            fmtOut,
            nStrideOut,
            DBG_ANALYSIS_PARAM_COMMA(cbBufferOut - cbBufferInset)
            OUT pbBufferOut + cbBufferInset
            ));
    }

Cleanup:
    ReleaseInterfaceNoNULL(pSrcSurface);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::UpdateSurface
//
//  Synopsis:
//      Copys dirty rects from pISrcSurface to our texture either by sharing
//      a handle or by copying through software. The surfaces will always be
//      on different devices but, in the software case, they may be on different
//      adapters.
//
//------------------------------------------------------------------------------

HRESULT
CHwDeviceBitmapColorSource::UpdateSurface(
    __in UINT cDirtyRects,
    __in_ecount(cDirtyRects) const CMilRectU *prgDirtyRects,
    __in_ecount(1) IDirect3DSurface9 *pISrcSurface
    )
{
    HRESULT hr = S_OK;

    Assert(cDirtyRects > 0);

    ENTER_DEVICE_FOR_SCOPE(*m_pDevice);

    if (m_hSharedHandle)
    {
        IFC(UpdateSurfaceSharedHandle(cDirtyRects, prgDirtyRects, pISrcSurface));
    }
    else
    {
        IFC(UpdateSurfaceSoftware(cDirtyRects, prgDirtyRects, pISrcSurface));
    }

Cleanup:

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::Flush
//
//  Synopsis:
//      Reads a single pixel from pID3DSurface in order to cause a flush. We
//      must first copy to something small that we can read back from.
//
//------------------------------------------------------------------------------
HRESULT
CHwDeviceBitmapColorSource::Flush(
    __in_ecount(1) IDirect3DDevice9 *pID3DDevice,
    __in_ecount(1) IDirect3DSurface9 *pID3DSurface,
    __in_ecount(1) const D3DSURFACE_DESC &desc
    )
{
    HRESULT hr = S_OK;
    // The surface that we will read back from will be 16x16 unless
    // the source is smaller than that. We don't do 1x1 because D3D
    // tells us that some drivers have issues with ultra small surfaces.
    const UINT uFlushWidth = min(16u, desc.Width);
    const UINT uFlushHeight = min(16u, desc.Height);
    const RECT rcCopy = { 0, 0, uFlushWidth, uFlushHeight };
    const RECT rcFlush = { 0, 0, 1, 1 };

    IDirect3DSurface9 *pIFlushSurface = NULL;
    
    IFC(pID3DDevice->CreateRenderTarget(
        uFlushWidth,
        uFlushHeight,
        desc.Format,
        D3DMULTISAMPLE_NONE,
        0,      // multisample quality
        TRUE,   // lockable
        &pIFlushSurface,
        NULL
        ));

    IFC(pID3DDevice->StretchRect(
        pID3DSurface,
        &rcCopy,
        pIFlushSurface,
        &rcCopy,
        D3DTEXF_NONE
        ));

    D3DLOCKED_RECT dontCare;
    IFC(pIFlushSurface->LockRect(
        &dontCare,
        &rcFlush,
        D3DLOCK_READONLY
        ));

Cleanup:
    if (pIFlushSurface)
    {
        IGNORE_HR(pIFlushSurface->UnlockRect());
    }
        
    ReleaseInterface(pIFlushSurface);
    
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::UpdateSurfaceSharedHandle
//
//  Synopsis:
//      Copys dirty rects from pISrcSurface to our texture by sharing
//      a handle. The source and dest will be on different devices but the
//      same video card.
//
//------------------------------------------------------------------------------

HRESULT
CHwDeviceBitmapColorSource::UpdateSurfaceSharedHandle(
    UINT cDirtyRects,
    __in_ecount(cDirtyRects) const CMilRectU *prgDirtyRects,
    __in_ecount(1) IDirect3DSurface9 *pISrcSurface
    )
{
    HRESULT hr = S_OK;

    Assert(m_hSharedHandle);
    
    IDirect3DDevice9 *pID3DSrcDevice = NULL;
    IDirect3DTexture9 *pIDestTexture = NULL;
    IDirect3DSurface9 *pIDestSurface = NULL;
    
    IFC(pISrcSurface->GetDevice(&pID3DSrcDevice));

    const D3DSURFACE_DESC &desc = m_pVidMemOnlyTexture->D3DSurface0Desc();

    IFC(pID3DSrcDevice->CreateTexture(
        desc.Width,
        desc.Height,
        m_pVidMemOnlyTexture->Levels(),
        desc.Usage,
        desc.Format,
        desc.Pool,
        &pIDestTexture,
        &m_hSharedHandle
        ));
    
    IFC(pIDestTexture->GetSurfaceLevel(0, &pIDestSurface));
    
    for (UINT i = 0; i < cDirtyRects; ++i)
    {
        // There is an unsigned -> signed cast going on here, but our texture can't be
        // bigger than INT_MAX and the dirty rect has been checked to be in bounds
        const RECT *prc = reinterpret_cast<const RECT *>(&prgDirtyRects[i]);
        
        IFC(pID3DSrcDevice->StretchRect(
            pISrcSurface, 
            prc, 
            pIDestSurface, 
            prc, 
            D3DTEXF_NONE
            ));
    }

    // Read back from the shared surface on the users device to force
    // a flush of his commands. If we do it on our device or our version
    // of the shared surface, the flush won't happen because D3D doesn't
    // have cross-device object dependency tracking.
    IFC(Flush(pID3DSrcDevice, pIDestSurface, desc));

Cleanup:
    ReleaseInterface(pIDestSurface);
    ReleaseInterface(pIDestTexture);
    ReleaseInterface(pID3DSrcDevice);

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::UpdateSurfaceSoftware
//
//  Synopsis:
//      Copies dirty rects from pISrcSurface to our texture by coping through
//      software.
//
//------------------------------------------------------------------------------

HRESULT
CHwDeviceBitmapColorSource::UpdateSurfaceSoftware(
    UINT cDirtyRects,
    __in_ecount(cDirtyRects) const CMilRectU *prgDirtyRects,
    __in_ecount(1) IDirect3DSurface9 *pISrcSurface
    )
{
    HRESULT hr = S_OK;

    Assert(cDirtyRects > 0);

    bool fUnlockNeeded = false;

    // Create and cache a system memory texture if we don't have one already
    if (!m_pSysMemTexture)
    {
        D3DSURFACE_DESC descSrc;
        IFC(pISrcSurface->GetDesc(&descSrc));

        // Modify the description for system memory use
        descSrc.Usage = 0;
        descSrc.Pool = D3DPOOL_SYSTEMMEM;
        descSrc.MultiSampleType = D3DMULTISAMPLE_NONE;
        descSrc.MultiSampleQuality = 0;
        
        IFC(m_pDevice->CreateLockableTexture(&descSrc, &m_pSysMemTexture));
    }

    // Lock the entire surface with manual dirty rect addition. LockRect assumes a write lock.
    D3DLOCKED_RECT rcLock;
    IFC(m_pSysMemTexture->LockRect(&rcLock, NULL, D3DLOCK_NO_DIRTY_UPDATE));
    fUnlockNeeded = true;

    for (UINT i = 0; i < cDirtyRects; ++i)
    {
        IFC(ReadRenderTargetIntoSysMemBuffer(
            pISrcSurface,
            prgDirtyRects[i],
            m_fmtTexture,
            static_cast<UINT>(rcLock.Pitch),
            DBG_ANALYSIS_PARAM_COMMA(m_uBitmapHeight * rcLock.Pitch)
            static_cast<BYTE *>(rcLock.pBits)
            ));

        // There is an unsigned -> signed cast going on here, but our texture can't be
        // bigger than INT_MAX and the dirty rect has been checked to be in bounds
        IFC(m_pSysMemTexture->AddDirtyRect(reinterpret_cast<const RECT *>(prgDirtyRects)[i]));
    }

    if (fUnlockNeeded)
    {
        IFC(m_pSysMemTexture->UnlockRect());
        fUnlockNeeded = false;
    }

    // This sends the accumlated dirty rects from m_pSysMemTexture to m_pVidMemOnlyTexture
    IFC(m_pDevice->UpdateTexture(
        m_pSysMemTexture->GetD3DTextureNoRef(), 
        m_pVidMemOnlyTexture->GetD3DTextureNoRef()
        ));

Cleanup:
    if (fUnlockNeeded)
    {
        IGNORE_HR(m_pSysMemTexture->UnlockRect());
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CHwDeviceBitmapColorSource::GetValidTransferSurfaceNoRef
//
//  Synopsis:
//      CHwDeviceBitmapColorSource does not have a transfer surface
//
//------------------------------------------------------------------------------

__out_opt CD3DSurface *
CHwDeviceBitmapColorSource::GetValidTransferSurfaceNoRef()
{
    return NULL;
}




