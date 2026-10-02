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
//      Contains implementation of CDeviceBitmap class
//
//  $ENDTAG
//
//------------------------------------------------------------------------------


#include "precomp.hpp"

MtDefine(CDeviceBitmap, MILRender, "CDeviceBitmap");
MtDefine(MBitmapOfDeviceBitmapsBits, MILRawMemory, "Texels for CDeviceBitmap");

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::Create
//
//  Synopsis:
//      Allocates and initializes a CDeviceBitmap
//
//------------------------------------------------------------------------------

HRESULT
CDeviceBitmap::Create(
    __in_range(0,SURFACE_RECT_MAX) UINT uWidth,
    __in_range(0,SURFACE_RECT_MAX) UINT uHeight,
    MilPixelFormat::Enum pixelFormat,
    __deref_out_ecount(1) CDeviceBitmap * &pBitmapOut
    )
{
    HRESULT hr = S_OK;

    CDeviceBitmap *pBitmap = NULL;

    if (uWidth > SURFACE_RECT_MAX || uHeight > SURFACE_RECT_MAX)
    {
        IFC(E_INVALIDARG);
    }

    pBitmap = new CDeviceBitmap(uWidth, uHeight, pixelFormat);
    IFCOOM(pBitmap);
    pBitmap->AddRef();

Cleanup:
    pBitmapOut = pBitmap; // Transfer reference if non-NULL

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::CDeviceBitmap
//
//  Synopsis:
//      ctor
//
//------------------------------------------------------------------------------

CDeviceBitmap::CDeviceBitmap(
    __in_range(0,SURFACE_RECT_MAX) UINT uWidth,
    __in_range(0,SURFACE_RECT_MAX) UINT uHeight,
    MilPixelFormat::Enum pixelFormat
    ) :
    CWGXBitmap(),
    // Set once PrepareSysMemBufferAsSourcePixels is called.
    m_pPixels(NULL),
    m_uStride(0),
    m_cbBuffer(0),
    m_cbPixel(0),
    m_poDeviceBitmapInfo(NULL)
{
    Assert(IsValidPixelFormat(pixelFormat));

    m_nWidth = uWidth;
    m_nHeight = uHeight;
    m_PixelFormat = pixelFormat;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::~CDeviceBitmap
//
//  Synopsis:
//      dtor
//
//------------------------------------------------------------------------------

CDeviceBitmap::~CDeviceBitmap(
    )
{
    WPFFree(ProcessHeap, m_pPixels);

    if (m_poDeviceBitmapInfo)
    {
        m_poDeviceBitmapInfo->Destruct();
        WPFFree(ProcessHeap, m_poDeviceBitmapInfo);
    }
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::Lock
//
//  Synopsis:
//      Prepare a system memory buffer containing at least bits specifies by
//      lock rect from device bitmaps that are associated and return pointer to
//      that prepared buffer.
//
//------------------------------------------------------------------------------

STDMETHODIMP
CDeviceBitmap::Lock(
    __in_ecount_opt(1) const WICRect *prcLock,
    DWORD dwFlags,
    __deref_out_ecount(1) IWGXBitmapLock **ppILock
    )
{
    HRESULT hr = S_OK;

    CMilRectL rcLock;

    if (dwFlags != MilBitmapLock::Read)
    {
        IFC(WGXERR_UNSUPPORTED_OPERATION);
    }

    IFC(HrCheckPixelRect(prcLock, &rcLock));

    Assert(IsValidPixelFormat(m_PixelFormat));

    if (!m_pPixels)
    {
        IFC(PrepareSysMemBufferAsSourcePixels());
    }

    // HrCheckPixelRect ensures the lock rect has only positives and is not
    // empty
    Assert(rcLock.left >= 0);
    Assert(rcLock.top >= 0);
    Assert(!rcLock.IsEmpty());

    CMilRectU const &rcLockU = reinterpret_cast<CMilRectU &>(rcLock);

    IFC(EnsureUpdatedSysMemBuffer(rcLockU));

    // This inset calculation should not overflow since rcLock is already
    // trimmed by surface bounds by HrCheckPixelRect.
    Assert(rcLockU.top < m_nHeight);
    Assert(rcLockU.left < m_nWidth);
    size_t cbInset =
        rcLockU.top * m_uStride
        + rcLockU.left * m_cbPixel;

    IFC(HrLock(
        rcLock,
        m_PixelFormat,
        m_uStride,
        static_cast<UINT>(m_cbBuffer - cbInset),
        static_cast<BYTE *>(m_pPixels) + cbInset,
        dwFlags,
        ppILock
        ));

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::SetDeviceBitmapColorSource
//
//  Synopsis:
//      Associate given id, shared handle and bitmap color source.  Create space
//      tracking valid and dirty information for the device bitmap.
//
//------------------------------------------------------------------------------

HRESULT CDeviceBitmap::SetDeviceBitmapColorSource(
    __in_ecount_opt(1) HANDLE hShared,
    __inout_ecount(1) CHwDeviceBitmapColorSource *pbcs
    )
{
    HRESULT hr = S_OK;

    // Take this oppurtunity to remove any device bitmaps that are now
    // invalid.
    CleanupInvalidSource();

    if (m_poDeviceBitmapInfo)
    {
        // Release prior resources
        m_poDeviceBitmapInfo->Destruct();
    }
    else
    {
        // First time setting, so create
        m_poDeviceBitmapInfo = 
            WPFAllocType(DeviceBitmapInfo *,
                         ProcessHeap, Mt(CDeviceBitmap),
                         sizeof(DeviceBitmapInfo));
        IFCOOM(m_poDeviceBitmapInfo);  
    }

    m_poDeviceBitmapInfo->Construct(
        hShared,
        pbcs
        );

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::AddUpdateRect
//
//  Synopsis:
//      Add a dirty rect to the device bitmap which we will use later on to
//      update the device bitmaps of other displays.
//
//      NOTE: Does not return HRESULT because the caller can't handle
//            failure. In the rare event of a failure, the update rect
//            will be dropped.
//
//------------------------------------------------------------------------------

void
CDeviceBitmap::AddUpdateRect(
    __in_ecount(1) const CMilRectU &rcUpdate
    )
{
    Assert(rcUpdate.right <= static_cast<INT>(m_nWidth));
    Assert(rcUpdate.bottom <= static_cast<INT>(m_nHeight));

    CMILSurfaceRect const &rcDirty =
        reinterpret_cast<CMILSurfaceRect const &>(rcUpdate);

    HRGN hrgnUpdate = NULL;

    bool fContained = false;

    //
    // Add or subtract the new update rect from the valid regions
    //

    if (m_poDeviceBitmapInfo)
    {
        HRESULT hr;

        //
        // Bitmap level dirty rects need tracked when a realization is
        // required that is not a direct use of the device bitmap color
        // source.  This may mean transfer through system memory or
        // vid-mem to vid-mem transfers on a single GPU.  Rather than
        // try to detect all the cases that require dirty rects, track
        // them all the time.  On failure the entire bitmap is marked
        // as dirty so the error can be safely ignored.
        //
        // NOTICE-2006/09/25-JasonHa  Dirty marked fully for resticted
        // content.  This means complex realizations, including
        // realizations that aren't targeting the required display,
        // will upload the zeroed out system memory buffer repeatedly.
        // The best way to fix this is to understand the restiction at
        // the composition level and not recompose.  That is
        // non-trivial and separating a sense of restricted content is
        // not dirty from regular content being dirty is non-trival.
        // Therefore suffer through with the general dirty marking and
        // re-upload.
        //
        IGNORE_HR(AddDirtyRect(&rcDirty));

        IFC(m_poDeviceBitmapInfo->AddValidRect(
            rcUpdate,
            IN OUT hrgnUpdate,
            OUT fContained
            ));

        m_poDeviceBitmapInfo->AssertRgnBoundsMatch();
    }

Cleanup:
    if (hrgnUpdate)
    {
        ::DeleteObject(hrgnUpdate);
    }

    return;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::ContainsValidArea
//
//  Synopsis:
//      Determines if the valid region contains rcArea
//
//------------------------------------------------------------------------------

bool
CDeviceBitmap::ContainsValidArea(
    __in_ecount(1) CMilRectU const &rcArea) const
{
    if (m_poDeviceBitmapInfo)
    {
        if (m_poDeviceBitmapInfo->DoesIntersectValid(rcArea))
        {
            Assert(m_poDeviceBitmapInfo->m_pbcs != NULL);
            return true;
        }
    }

    return false;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::HasContributorFromDifferentAdapter
//
//  Synopsis:
//      Return true if there is valid content from an adapter different than the
//      given one.
//
//------------------------------------------------------------------------------

bool
CDeviceBitmap::HasContributorFromDifferentAdapter(
    LUID luidAdapter
    )
{
    CleanupInvalidSource();

    if (m_poDeviceBitmapInfo)
    {
        if (   !m_poDeviceBitmapInfo->m_rcValid.IsEmpty()
            && !m_poDeviceBitmapInfo->m_pbcs->IsAdapter(luidAdapter))
        {
            return true;
        }
    }

    return false;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::HasValidDeviceBitmap
//
//  Synopsis:
//      Returns whether this bitmap contains valid, unrestricted device bitmaps
//
//      If this bitmap contains any restricted contents whatsoever, the method
//      will return false.
//
//------------------------------------------------------------------------------

bool CDeviceBitmap::HasValidDeviceBitmap()
{
    CleanupInvalidSource();

    if (m_poDeviceBitmapInfo)
    {
        if (!m_poDeviceBitmapInfo->m_rcValid.IsEmpty())
        {
            return true;
        }
    }

    return false;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::GetPointerToValidRectsForSurface
//
//  Synopsis:
//      Return array of rectangles representing areas with valid content in the
//      given device bitmap.  Ownership of array is not given to caller.
//
//------------------------------------------------------------------------------

HRESULT
CDeviceBitmap::GetPointerToValidRectsForSurface(
    __out_ecount(1) UINT &cValidRects,
    __deref_out_ecount_full(cValidRects) CMilRectU const * &rgValidContents
    )
{
    HRESULT hr = S_OK;

    Assert(m_poDeviceBitmapInfo);
    Assert(!m_poDeviceBitmapInfo->m_rcValid.IsEmpty());

    IFC(m_poDeviceBitmapInfo->GetPointerToValidRects(
        OUT cValidRects,
        OUT rgValidContents
        ));

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::GetDeviceColorSourceNoRef
//
//  Synopsis:
//      Returns the DBCS without addref-ing
//
//------------------------------------------------------------------------------

__out_opt CHwDeviceBitmapColorSource *
CDeviceBitmap::GetDeviceColorSourceNoRef()
{
    CleanupInvalidSource();

    return m_poDeviceBitmapInfo ? m_poDeviceBitmapInfo->m_pbcs : NULL;
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::PrepareSysMemBufferAsSourcePixels
//
//  Synopsis:
//      Allocate a system memory buffer of the stored pixel format.
//
//------------------------------------------------------------------------------

HRESULT
CDeviceBitmap::PrepareSysMemBufferAsSourcePixels(
    )
{
    Assert(!m_pPixels);

    HRESULT hr = S_OK;

    UINT BitsPerPixelRequested = GetPixelFormatSize(m_PixelFormat);

    if (BitsPerPixelRequested % BITS_PER_BYTE)
    {
        TraceTag((tagMILWarning,
                  "Call to CDeviceBitmap::PrepareSysMemBufferAsSourcePixels requested fraction byte buffer"));
        IFC(WGXERR_UNSUPPORTEDPIXELFORMAT);
    }

    //
    // Allocate buffer and save buffer properties
    //

    m_cbPixel = BitsPerPixelRequested / BITS_PER_BYTE;

    // Calculate stride in exact bytes to increase chance of wrapping system
    // memory buffer as target when pulling bits from D3D surface.
    IFC(MultiplyUINT(m_nWidth, m_cbPixel, m_uStride));

    IFC(MultiplyUINT(m_nHeight, m_uStride, m_cbBuffer));

    IFC(HrAlloc(
        Mt(MBitmapOfDeviceBitmapsBits),
        m_cbBuffer,
        &m_pPixels
        ));

    // When valid contents aren't available for some portions, the buffer will
    // not be updated.  So start with a zero-ed out buffer.
    ZeroMemory(m_pPixels, m_cbBuffer);

    //
    // Make device bitmap is marked as not copied to system memory buffer.
    //

    if (m_poDeviceBitmapInfo)
    {
        m_poDeviceBitmapInfo->m_rcCopiedToSysMemBuffer.SetEmpty();
        m_poDeviceBitmapInfo->m_rcDirty.SetEmpty();
        m_poDeviceBitmapInfo->m_fSysMemBufferStale = !m_poDeviceBitmapInfo->m_rcValid.IsEmpty();
    }

Cleanup:
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::EnsureUpdatedSysMemBuffer
//
//  Synopsis:
//      Ensure that system memory buffer covered by given rectangle has up to
//      date copy of device bitmap content.
//
//------------------------------------------------------------------------------

HRESULT
CDeviceBitmap::EnsureUpdatedSysMemBuffer(
    __in_ecount(1) CMilRectU const &rc
    )
{
    HRESULT hr = S_OK;

    Assert(m_pPixels);

    CleanupInvalidSource();

    if (m_poDeviceBitmapInfo)
    {
        if (m_poDeviceBitmapInfo->m_fSysMemBufferStale)
        {
            CMilRectU rcCopy(rc);

            if (rcCopy.Intersect(m_poDeviceBitmapInfo->m_rcValid))
            {
                bool fChangeCopiedArea =
                    !m_poDeviceBitmapInfo->m_rcCopiedToSysMemBuffer.DoesContain(rcCopy);

                bool fUpdateSysMemBuffer;

                // Check if update needed due to changing copy area.
                if (fChangeCopiedArea)
                {
                    fUpdateSysMemBuffer = true;
                }
                // If no update needed due to changing copy area, ...
                else
                {
                    // Check if update is needed because an area is dirty. 
                    // Update area is limited to dirty area.
                    fUpdateSysMemBuffer = rcCopy.Intersect(m_poDeviceBitmapInfo->m_rcDirty);
                }

                if (fUpdateSysMemBuffer)
                {
                    // This inset calculation should not overflow since
                    // rcCopy is already trimmed by surface bounds.
                    Assert(rcCopy.top < m_nHeight);
                    Assert(rcCopy.left < m_nWidth);
                    size_t cbInset =
                        rcCopy.top * m_uStride
                        + rcCopy.left * m_cbPixel;

                    //
                    // Determine copy limiting list (clip list), if any.  Clip
                    // to valid content areas, otherwise copy could clobber
                    // content from other sources.
                    //

                    UINT cClipRects = 0;
                    CMilRectU const *rgClipRects = NULL;

                    if (m_poDeviceBitmapInfo->m_hrgnValid)
                    {
                        IFC(m_poDeviceBitmapInfo->EnsureRgnData());

                        cClipRects = m_poDeviceBitmapInfo->m_pRgnData->rdh.nCount;
                        Assert(cClipRects > 1);
                        rgClipRects = reinterpret_cast<CMilRectU const *>
                            (m_poDeviceBitmapInfo->m_pRgnData->Buffer);
                    }

                    //
                    // Copy the pixels into buffer
                    //

                    IFC(m_poDeviceBitmapInfo->m_pbcs->CopyPixels(
                        rcCopy,
                        cClipRects,
                        rgClipRects,
                        m_PixelFormat,
                        DBG_ANALYSIS_PARAM_COMMA(static_cast<UINT>(m_cbBuffer - cbInset))
                        static_cast<BYTE *>(m_pPixels) + cbInset,
                        m_uStride
                        ));

                    //
                    // Update system memory buffer tracking information
                    //

                    UINT cDirtyRemaining;
                    CMilRectU rcDirtyRemaining;

                    if (fChangeCopiedArea)
                    {
                        // Copy area is changing and thus remaining dirty areas
                        // is considered incalculable.
                        cDirtyRemaining = UINT_MAX;
                    }
                    else
                    {
                        cDirtyRemaining =
                            m_poDeviceBitmapInfo->m_rcDirty.CalculateSubtractionRectangles(
                                IN rcCopy,
                                OUT &rcDirtyRemaining,
                                1
                                );
                    }

                    if (cDirtyRemaining == 1)
                    {
                        Assert(!fChangeCopiedArea);
                        m_poDeviceBitmapInfo->m_rcDirty = rcDirtyRemaining;
                    }
                    else
                    {
                        if (cDirtyRemaining != 0)
                        {
                            Assert(cDirtyRemaining > 1);
                            // Set rcCopiedToSysMemBuffer to exactly what was
                            // copied.
                            m_poDeviceBitmapInfo->m_rcCopiedToSysMemBuffer = rcCopy;
                        }

                        m_poDeviceBitmapInfo->m_rcDirty.SetEmpty();

                        if (rcCopy.DoesContain(m_poDeviceBitmapInfo->m_rcValid))
                        {
                            m_poDeviceBitmapInfo->m_fSysMemBufferStale = false;
                        }
                    }
                }
            }
        }
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::CleanupInvalidSource
//
//  Synopsis:
//      Release information about device bitmap with an invalid surface.
//
//  Notes:
//      Most likely reason for a device bitmap to become invalid is mode change.
//      Out of video memory is also possible.
//
//------------------------------------------------------------------------------

void
CDeviceBitmap::CleanupInvalidSource(
    )
{
    if (m_poDeviceBitmapInfo)
    {
        if (!m_poDeviceBitmapInfo->m_pbcs->IsValid())
        {
            m_poDeviceBitmapInfo->Destruct();
            WPFFree(ProcessHeap, m_poDeviceBitmapInfo);
            
            m_poDeviceBitmapInfo = NULL;
        }
    }
}

//+-----------------------------------------------------------------------------
//
//  Structure:
//      CDeviceBitmap::DeviceBitmapInfo
//
//  Synopsis:
//      Tracks information relating to a single device bitmap and provides some
//      methods to manage the information.
//
//------------------------------------------------------------------------------

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::DeviceBitmapInfo::Construct
//
//  Synopsis:
//      Contructor
//
//------------------------------------------------------------------------------

void
CDeviceBitmap::DeviceBitmapInfo::Construct(
    __in_ecount_opt(1) HANDLE hShared,
    __inout_ecount(1) CHwDeviceBitmapColorSource *pbcs
    )
{
    Assert(pbcs->IsValid());

    m_hShared = hShared;
    m_pbcs = pbcs;
    m_rcValid.SetEmpty();
    m_hrgnValid = NULL;
    m_pRgnData = NULL;
    m_rcCopiedToSysMemBuffer.SetEmpty();
    m_rcDirty.SetEmpty();

    m_fSysMemBufferStale = false;

    // Color source is now referenced locally
    pbcs->AddRef();
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::DeviceBitmapInfo::Destruct
//
//  Synopsis:
//      Destructor
//
//------------------------------------------------------------------------------

void
CDeviceBitmap::DeviceBitmapInfo::Destruct()
{
    ReleaseInterface(m_pbcs);
    if (m_hrgnValid)
    {
        ::DeleteObject(m_hrgnValid);
    }
    WPFFree(ProcessHeap, m_pRgnData);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::DeviceBitmapInfo::AddValidRect
//
//  Synopsis:
//      Add the given rcValid to this surface's collection of valid contents.
//
//      fContained will be returned true if given rectangle did not impact
//      collection of valid contents.  But this is not a strict result;
//      sometimes false may be returned even when the rectangle was actually
//      already contained.
//
//------------------------------------------------------------------------------

HRESULT
CDeviceBitmap::DeviceBitmapInfo::AddValidRect(
    __in_ecount(1) CMilRectU const &rcValid,
    __deref_inout_opt HRGN &hrgnNewValid,
    __out_ecount(1) bool &fContained
    )
{
    HRESULT hr = S_OK;

    Assert(fContained == false);

    CMilRectU rcDirtyCopied = rcValid;
    if (rcDirtyCopied.Intersect(m_rcCopiedToSysMemBuffer))
    {
        m_rcDirty.Union(rcDirtyCopied);
    }

    m_fSysMemBufferStale = true;

    //
    // Combine the new update rect into the valid region of that surface.
    //

    //
    // If old valid region for the surface is a simple rect, attempt to
    // avoid region manipulation for simple containment cases.
    //
    if (!m_hrgnValid)
    {
        Assert(!m_pRgnData);

        if (m_rcValid.DoesContain(rcValid))
        {
            //
            // If old valid rect for that surface fully contains new
            // update rect then no further processing is required.
            //
            fContained = true;
        }
        else if (rcValid.DoesContain(m_rcValid))
        {
            //
            // If new update rect fully contains the old rect, replace
            // the old valid rect with the new one.
            //
            m_rcValid = rcValid;
        }
        else
        {
            //
            // Generic case - combine the old and new rects into a region.
            // Create a region from existing rect and then use complex add
            // logic below.
            //
            // NOTICE-2006/09/12-JasonHa  Aligned case w/o gap is simple
            // union result and can be detected w/o making kernel calls.
            //

            IFCW32(m_hrgnValid = ::CreateRectRgnIndirect(reinterpret_cast<RECT const *>(&m_rcValid)));
        }
    }

    //
    // Check for unprocessed complex case.
    //
    if (m_hrgnValid)
    {
        Assert(!fContained);

        // Independent of result, the Rgn Data will be stale; so clear it.
        WPFFree(ProcessHeap, m_pRgnData);
        m_pRgnData = NULL;

        int iResult;

        //
        // Generic case - combine new update into the stored region.
        //
        if (hrgnNewValid == NULL)
        {
            IFCW32(hrgnNewValid = ::CreateRectRgnIndirect(reinterpret_cast<RECT const *>(&rcValid)));
        }

        IFCW32(iResult = ::CombineRgn(
            m_hrgnValid,
            m_hrgnValid,
            hrgnNewValid,
            RGN_OR
            ));
        Assert(iResult != NULLREGION);

        //
        // Store region bounds on the surface info struct for
        // potential calcualtion optimization in the future.
        //
        m_rcValid.Union(rcValid);

#if DBG_ANALYSIS
        {
            CMilRectL rc;
            if (GetRgnBox(m_hrgnValid, &rc))
            {
                Assert(m_rcValid.IsEquivalentTo(reinterpret_cast<CMilRectU const &>(rc)));
            }
        }
#endif

        //
        // Check if the result of the combine is a simple rect.
        //
        if (iResult == SIMPLEREGION)
        {
            ::DeleteObject(m_hrgnValid);
            m_hrgnValid = NULL;
        }
    }

    if (!fContained)
    {
        //
        // Valid area may have changed so mark system memory buffer as stale. 
        // This is important for restricted content case.  See comments above.
        //
        m_fSysMemBufferStale = true;

        //
        // Update device bitmap color source's notion of its valid content bounds.
        //
        m_pbcs->UpdateValidBounds(
            m_rcValid
            );
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::DeviceBitmapInfo::DoesIntersectValid
//
//  Synopsis:
//      Returns true if given rectangle intersects any part of this surfaces
//      valid content region.
//
//------------------------------------------------------------------------------

bool
CDeviceBitmap::DeviceBitmapInfo::DoesIntersectValid(
    __in_ecount(1) CMilRectU const &rc
    ) const
{
    return (   m_rcValid.DoesIntersect(rc)
            && (   (m_hrgnValid == NULL)
                || RectInRegion(
                    m_hrgnValid,
                    &reinterpret_cast<CMilRectL const &>(rc)
                   )
               )
           );
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::DeviceBitmapInfo::EnsureRgnData
//
//  Synopsis:
//      Make sure a rgn data representation of complex valid area is allocated
//      and up to date.  This should only be called when there is already an
//      HRGN.
//
//------------------------------------------------------------------------------

HRESULT
CDeviceBitmap::DeviceBitmapInfo::EnsureRgnData(
    )
{
    HRESULT hr = S_OK;

    Assert(m_hrgnValid);

    if (!m_pRgnData)
    {
        IFC(HrgnToRgnData(m_hrgnValid, &m_pRgnData));
    }

    AssertRgnBoundsMatch();

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::DeviceBitmapInfo::GetPointerToValidRects
//
//  Synopsis:
//      Return an array of rectangles representing the valid contents of this
//      surface.  Ownership of the list is not given to caller.
//
//------------------------------------------------------------------------------

HRESULT
CDeviceBitmap::DeviceBitmapInfo::GetPointerToValidRects(
    __out_ecount(1) UINT &cValidRects,
    __deref_out_ecount_full(cValidRects) CMilRectU const * &rgValidRects
    )
{
    HRESULT hr = S_OK;

    if (m_hrgnValid)
    {
        IFC(EnsureRgnData());

        rgValidRects =
            reinterpret_cast<CMilRectU const *>(m_pRgnData->Buffer);
        cValidRects = m_pRgnData->rdh.nCount;
    }
    else
    {
        rgValidRects = &m_rcValid;
        cValidRects = 1;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::TryCreateDependentDeviceColorSource
//
//  Synopsis:
//      Does nothing. Vanilla CDeviceBitmap do not support pulling from
//      their color source. CInteropDeviceBitmap does.
//
//------------------------------------------------------------------------------

bool
CDeviceBitmap::TryCreateDependentDeviceColorSource(
    __in const LUID &luidNewDevice,
    __in CHwBitmapCache *pNewCache
    )
{
    return false;
}


//+-----------------------------------------------------------------------------
//
//  Member:
//      CDeviceBitmap::DeviceBitmapInfo::AssertRgnBoundsMatch
//
//  Synopsis:
//      Assert that when rgn data exists, its bounds match m_rcValid.
//
//------------------------------------------------------------------------------

void
CDeviceBitmap::DeviceBitmapInfo::AssertRgnBoundsMatch(
    ) const
{
#if DBG_ANALYSIS
    if (m_pRgnData)
    {
        Assert(m_rcValid.IsEquivalentTo(
            reinterpret_cast<CMilRectU const &>
            (m_pRgnData->rdh.rcBound)
            ));
    }
#endif
}



