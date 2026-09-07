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
//      Software Render Target (RT) for screen rendering. This RT is always
//      software rasterized.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

#include "precomp.hpp"

MtDefine(CSwPresenter32bppGDI, MILRender, "CSwPresenter32bppGDI");
MtDefine(MSwBackBuffer, MILRawMemory, "MSwBackBuffer");

DeclareTag(tagMILDisableDithering, "CSwRenderTargetHWND", "Disable MILRender dithering");

//+-----------------------------------------------------------------------------
//
//  Name:
//      CReusableBitmapLock::CReusableBitmapLock
//
//  Synopsis:
//      Initializes the ref count on the reusable bitmap lock.
//
//------------------------------------------------------------------------------

CReusableBitmapLock::CReusableBitmapLock()
    : CWGXBitmapLock()
{
    m_cRef = 0;
}

//+-----------------------------------------------------------------------------
//
//  Name:
//      CReusableBitmapLock::AddRef
//
//  Synopsis:
//      Increments the ref count on this object.
//
//------------------------------------------------------------------------------

ULONG
CReusableBitmapLock::AddRef()
{
    return InterlockedIncrement(&m_cRef);
}

//+-----------------------------------------------------------------------------
//
//  Name:
//      CReusableBitmapLock::Release
//
//  Synopsis:
//      Decrements the ref count of the object and releases the lock, but not
//      the object when the count goes to 0.
//
//------------------------------------------------------------------------------

ULONG
CReusableBitmapLock::Release()
{
    AssertConstMsgW(
        m_cRef != 0,
        L"Attempt to release an object with 0 references! Possible memory leak."
        );

    ULONG cRef = InterlockedDecrement(&m_cRef);

    if(0 == cRef)
    {
        // We do an unlock instead of release.
        Unlock();
    }

    return cRef;
}

#define IFCW32_CHECKOOGDI_CHECKHWND(uniq, expr) \
    do \
    { \
        volatile bool fTryAgain = false; \
        do \
        { \
            IFW32GOTO_CHECKOUTOFHANDLES(GR_GDIOBJECTS, CheckWindowValidity##uniq, (expr)); \
            if (UNCONDITIONAL_EXPR(false)) \
            { \
            CheckWindowValidity##uniq: \
                if (hr == WGXERR_WIN32ERROR) \
                { \
                    Assert(m_MILDC.GetHWND()); \
                    \
                    if (IsWindow(m_MILDC.GetHWND())) \
                    { \
                        MilInstrumentationBreak(MILINSTRUMENTATIONFLAGS_NOBREAKUNLESSKDPRESENT); \
                        if (fTryAgain) \
                        { \
                             continue; \
                        } \
                    } \
                } \
                \
                goto Cleanup; \
            } \
        } while (UNCONDITIONAL_EXPR(false)); \
    } while (UNCONDITIONAL_EXPR(false))

//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::CSwPresenter32bppGDI
//
//  Synopsis:
//      Clear out all state.
//
//------------------------------------------------------------------------------

CSwPresenter32bppGDI::CSwPresenter32bppGDI(
    __in_ecount(1) CDisplay const *pIdealDisplay,
    MilPixelFormat::Enum fmtBackBuffer
    )
    : CSwPresenterBase(fmtBackBuffer),
    m_pIdealDisplay(pIdealDisplay),
    m_PresentPixelFormat(MilPixelFormat::DontCare),
    m_eWindowLayerType(MilWindowLayerType::NotLayered)
{
    m_pIdealDisplay->AddRef();

    Assert(   m_RenderPixelFormat == MilPixelFormat::BGR32bpp
           || m_RenderPixelFormat == MilPixelFormat::PBGRA32bpp
           || m_RenderPixelFormat == MilPixelFormat::RGB128bppFloat
           || m_RenderPixelFormat == MilPixelFormat::PRGBA128bppFloat);

    Assert(m_pLock == NULL);
}

//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::~CSwPresenter32bppGDI
//
//  Synopsis:
//      Free all state.
//
//------------------------------------------------------------------------------

CSwPresenter32bppGDI::~CSwPresenter32bppGDI()
{
    Assert(!m_fLocked);

    FreeResources();

    ReleaseInterfaceNoNULL(m_pIdealDisplay);
}

//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::Lock
//
//  Synopsis:
//      Hand out portion of the allocated backbuffer.
//
//------------------------------------------------------------------------------

HRESULT CSwPresenter32bppGDI::Lock(
    __in_ecount(1) const WICRect *prcLock,
    DWORD flags,
    __deref_out_ecount(1) IWGXBitmapLock **ppILock
    )
{
    HRESULT hr = S_OK;

    Assert (!m_fLocked);

    WICRect rcLock;
    WICRect rcBack = {0, 0, m_nWidth, m_nHeight};

    if (prcLock == NULL)
        rcLock = rcBack;
    else
        IntersectRect(&rcLock, prcLock, &rcBack);

    if (rcLock.Width <= 0  &&  rcLock.Height <= 0)
    {
        MIL_THR(WGXERR_WRONGSTATE);
    }
    else
    {
        Assert (rcLock.Width > 0);
        Assert (rcLock.Height > 0);

        Assert (rcLock.X >= 0);
        Assert (rcLock.Y >= 0);
        Assert (static_cast<UINT>(rcLock.X + rcLock.Width) <= m_nWidth);
        Assert (static_cast<UINT>(rcLock.Y + rcLock.Height) <= m_nHeight);

        INT_PTR iBitLeft = rcLock.X * GetPixelFormatSize(m_RenderPixelFormat);

        BYTE *pbBits = static_cast<BYTE *>(m_pvRenderBits) +
            static_cast<INT_PTR>(rcLock.Y) * m_nBufferStride +
            (iBitLeft / 8);

        if (m_pLock == NULL)
        {
            m_pLock = new CReusableBitmapLock();

            if (m_pLock == NULL)
            {
                MIL_THR(E_OUTOFMEMORY);
            }
        }        

        if (SUCCEEDED(hr))
        {
            MIL_THR(m_pLock->Init(
                this,
                rcLock.Width,
                rcLock.Height,
                m_RenderPixelFormat,
                m_nBufferStride,
                GetRequiredBufferSize(m_RenderPixelFormat, m_nBufferStride, &rcLock),
                pbBits,
                flags));

            Assert(SUCCEEDED(hr));

            *ppILock = m_pLock;
            (*ppILock)->AddRef();

            m_fLocked = TRUE;
        }

    }

  return hr;
}

//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::Unlock
//
//  Synopsis:
//      Return the locked memory.
//
//------------------------------------------------------------------------------

HRESULT CSwPresenter32bppGDI::Unlock(
    __in_ecount(1) CWGXBitmapLock *pBitmapLock
    )
{
    HRESULT hr = S_OK;

    Assert (m_fLocked);

    m_fLocked = FALSE;
    return hr;
}

//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::CopyPixels
//
//------------------------------------------------------------------------------

HRESULT CSwPresenter32bppGDI::CopyPixels(
    __in_ecount(1) const WICRect *prc,
    UINT cbOutputBufferStride,
    UINT cbOutputBufferSize,
    __out_ecount(cbOutputBufferSize) BYTE *pbPixels
    )
{
    HRESULT hr = S_OK;

    WICRect rcCopy;
    WICRect rcBack = {0, 0, m_nWidth, m_nHeight};

    if (prc == NULL)
    {
        rcCopy = rcBack;
    }
    else if (!IntersectRect(
        OUT &rcCopy,
        prc,
        &rcBack
        ))
    {
        goto Cleanup;
    }

    Assert(rcCopy.Width > 0);
    Assert(rcCopy.Height > 0);

    INT_PTR iBitLeft = rcCopy.X * GetPixelFormatSize(m_RenderPixelFormat);

    const BYTE *pbSurface = static_cast<BYTE *>(m_pvRenderBits) +
        static_cast<INT_PTR>(rcCopy.Y) * m_nBufferStride +
        (iBitLeft / 8);

    UINT cbCopyStride;
    IFC(HrCalcByteAlignedScanlineStride(rcCopy.Width, m_RenderPixelFormat, cbCopyStride));

    //
    // Make sure we don't copy over the end of each scanline as well as the
    // end of the buffer.
    //
    
    ULONGLONG cbOutputBufferUsed =
          static_cast<ULONGLONG>(cbCopyStride)
        + (  static_cast<ULONGLONG>(rcCopy.Height - 1)
           * static_cast<ULONGLONG>(cbOutputBufferStride)
          );

    if (   cbCopyStride > cbOutputBufferStride
        || cbOutputBufferUsed > cbOutputBufferSize
           )
    {
        IFC(E_INVALIDARG);
    }

#if ANALYSIS
    const BYTE *pbAnalysisPixelsOrig = pbPixels;
#endif

    for(INT i=0; i<rcCopy.Height; i++)
    {
    #if ANALYSIS
        Assert(pbPixels + cbCopyStride <= pbAnalysisPixelsOrig + cbOutputBufferSize);
    #endif
        GpMemcpy(pbPixels, pbSurface, cbCopyStride);
        pbSurface += m_nBufferStride;
        pbPixels += cbOutputBufferStride;
    }

Cleanup:
    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::Init
//
//  Synopsis:
//      Initializes the presenter to a specific HWND and/or device.
//
//------------------------------------------------------------------------------
void
CSwPresenter32bppGDI::Init(
    __in_ecount_opt(1) HWND hwnd,               // HWND that is to be rendered to
    MilWindowLayerType::Enum eWindowLayerType,  // Win32 window layer type
    MilRTInitialization::Flags nFlags           // Initialization flags
    )
{
    m_MILDC.Init(hwnd, nFlags);

    m_eWindowLayerType = eWindowLayerType;
}

//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::Resize
//
//  Synopsis:
//      The caller requests a change to the front buffer. Make sure that we have
//      a equally sized back buffer allocated.
//
//------------------------------------------------------------------------------
HRESULT CSwPresenter32bppGDI::Resize(
    UINT nWidth,
    UINT nHeight
    )
{
    HRESULT hr = S_OK;
    HDC hdcFront = NULL;

    FreeResources();

    IFC(m_MILDC.BeginRendering(&hdcFront));

    IFC(CreateBackBuffers(hdcFront, nWidth, nHeight) );

    m_nWidth = nWidth;
    m_nHeight = nHeight;

Cleanup:

    if (NULL != hdcFront)
    {
        m_MILDC.EndRendering(hdcFront);
    }

    RRETURN(hr);
}



//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::ScrollBlt
//
//  Synopsis:
//      Does a surface to surface blt on the front buffer, using the supplied
//      source and destination rects. The rects must be of equal size.
//      Responsibility for remembering and re-rendering the "exposed" areas
//      from the blt belongs to the caller.
//
//------------------------------------------------------------------------------

HRESULT CSwPresenter32bppGDI::ScrollBlt(
    __in_ecount(1) CMILSurfaceRect const *prcSource,
    __in_ecount(1) CMILSurfaceRect const *prcDest,
    bool fScrollBackBuffer,
    bool fDeferFrontBufferScroll
    )
{
    HRESULT hr = S_OK;

    CMILSurfaceRect surfaceSize;

    FreAssert((prcSource->Width() == prcDest->Width()) && (prcSource->Height() == prcDest->Height()));
    FreAssert(m_eWindowLayerType == MilWindowLayerType::NotLayered);

    surfaceSize.top = surfaceSize.left = 0;
    surfaceSize.right = m_nWidth;
    surfaceSize.bottom = m_nHeight;

    FreAssert(surfaceSize.DoesContain(*prcSource) && surfaceSize.DoesContain(*prcDest));

    Assert(m_hSystemPalette == NULL);

    if (fScrollBackBuffer)
    {
        //
        // We must perform the same operation on our own back buffer
        // to ensure it's not stale
        //
        IFCW32_CHECKSAD(::BitBlt(m_hdcBack,
                                 prcDest->left,
                                 prcDest->top,
                                 prcDest->Width(),
                                 prcDest->Height(),
                                 m_hdcBack,
                                 prcSource->left,
                                 prcSource->top,
                                 SRCCOPY));        

        //        
        // We must also scroll our render buffer if it's separate from our back buffer,
        // which will happen when we are presenting to a display of bit depth 16 (which
        // is common over TS).
        //
        if (m_hdcRender)
        {
            Assert(m_pvDeviceBits != m_pvRenderBits);
            IFCW32_CHECKSAD(::BitBlt(m_hdcRender,
                                     prcDest->left,
                                     prcDest->top,
                                     prcDest->Width(),
                                     prcDest->Height(),
                                     m_hdcRender,
                                     prcSource->left,
                                     prcSource->top,
                                     SRCCOPY));        
                
        }
    }

    if (fDeferFrontBufferScroll)
    {
        Assert(!m_fHasDeferredScroll);
        m_fHasDeferredScroll = true;
        m_sourceScrollRect = *prcSource;
        m_destinationScrollRect = *prcDest;        
    }
    else
    {
        HDC hdcFront = NULL;
        IFC(m_MILDC.BeginRendering(&hdcFront));

        // Do the scroll
        IFCW32_CHECKSAD(::BitBlt(hdcFront,
                                 prcDest->left,
                                 prcDest->top,
                                 prcDest->Width(),
                                 prcDest->Height(),
                                 hdcFront,
                                 prcSource->left,
                                 prcSource->top,
                                 SRCCOPY));
        
        m_MILDC.EndRendering(hdcFront);
    }    

Cleanup:
    if (FAILED(hr))
    {
        //
        // There can be a variety of failure codes returned when a window is
        // destroyed while we are trying to draw to it with GDI.  To simplify
        // the callers error handling check for an invalid window and return
        // a failure code indicating such.  Otherwise just return whatever 
        // we could discern so far.
        //
        if (!IsWindow(m_MILDC.GetHWND()))
        {
            MIL_THR(HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE));
        }
    }
    
    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::RemoveForegroundWindowScrollArtifacts
//
//  Synopsis:
//      ScrollBlt scrolls from a source rect to a destination rect using BitBlt
//      on the front buffer. If any part of the source rect is covered by a
//      foreground window (e.g. Task Manager), the BitBlt scrolls the pixels of
//      the foreground window as well, which creates display artifacts. This
//      method removes the artifacts by doing another BitBlt from our back
//      buffer to the front buffer in the affected areas.
//
//------------------------------------------------------------------------------

HRESULT CSwPresenter32bppGDI::RemoveForegroundWindowScrollArtifacts(
    __in_ecount(1) HDC hdcFront
    )
{
    HRESULT hr = S_OK;

    HRGN presentedRegion = NULL;
    HRGN coveredScrollSourceRegion = NULL;
    RECT coveredScrollSourceBounds;
    POINT topLeft, bottomRight;
    int retValue;

    HWND hwnd = WindowFromDC(hdcFront);
    wpf::util::DpiAwarenessScope<HWND> dpiScope(hwnd);
    if (hwnd == NULL)
    {
        goto Cleanup;
    }

    //
    // 1. Find the uncovered, presented region (Note: GetRandomRgn returns
    //    screen coordinates).
    //
    IFCW32(presentedRegion = CreateRectRgn(0, 0, 0, 0));
    retValue = GetRandomRgn(hdcFront, presentedRegion, SYSRGN);
    switch (retValue)
    {
        case -1:
            IFC(HRESULT_FROM_WIN32(GetLastError()));
            goto Cleanup;

        case 0:
            // Region is null, nothing to do
            goto Cleanup;

        default:
            break;
    }

    //
    // 2. Find the source scroll region (need to translate to screen
    //    coordinates).
    //
    topLeft.y = m_sourceScrollRect.top;
    topLeft.x = m_sourceScrollRect.left;
    IFCW32(ClientToScreen(hwnd, &topLeft));
    bottomRight.y = m_sourceScrollRect.bottom;
    bottomRight.x = m_sourceScrollRect.right;
    IFCW32(ClientToScreen(hwnd, &bottomRight));
    IFCW32(coveredScrollSourceRegion = CreateRectRgn(topLeft.x, 
                                                     topLeft.y,
                                                     bottomRight.x,
                                                     bottomRight.y));

    //
    // 3. Take the difference to find the covered areas in the source scroll
    //    region.
    //
    retValue = CombineRgn(coveredScrollSourceRegion,
                          coveredScrollSourceRegion,
                          presentedRegion,
                          RGN_DIFF);
    switch (retValue)
    {
        case ERROR:
            IFC(HRESULT_FROM_WIN32(GetLastError()));
            goto Cleanup;

        case NULLREGION:
            // No areas were covered, nothing to do
            goto Cleanup;

        default:
            break;
    }

    //
    // 4. Find the artifacts (where the covered regions were scrolled to) and
    //    take the bounding box.
    //
    IFCW32(OffsetRgn(coveredScrollSourceRegion,
                     m_destinationScrollRect.left - m_sourceScrollRect.left,
                     m_destinationScrollRect.top - m_sourceScrollRect.top) != ERROR);
    IFCW32(GetRgnBox(coveredScrollSourceRegion, &coveredScrollSourceBounds));

    //
    // 5. BitBlt over the artifacts (BitBlt params are in client coordinates).
    //
    topLeft.x = coveredScrollSourceBounds.left;
    topLeft.y = coveredScrollSourceBounds.top;
    IFCW32(ScreenToClient(hwnd, &topLeft));
    bottomRight.x = coveredScrollSourceBounds.right;
    bottomRight.y = coveredScrollSourceBounds.bottom;
    IFCW32(ScreenToClient(hwnd, &bottomRight));
    IFCW32_CHECKSAD(::BitBlt(hdcFront,
                             topLeft.x,
                             topLeft.y,
                             bottomRight.x - topLeft.x,
                             bottomRight.y - topLeft.y,
                             m_hdcBack,
                             topLeft.x,
                             topLeft.y,
                             SRCCOPY));

Cleanup:

    if (presentedRegion != NULL)
    {
        DeleteObject(presentedRegion);
    }
    if (coveredScrollSourceRegion != NULL)
    {
        DeleteObject(coveredScrollSourceRegion);
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::Present
//
//  Synopsis:
//      Presents the pixels from the specified rectangles onto the front buffer.
//
//------------------------------------------------------------------------------

HRESULT CSwPresenter32bppGDI::Present(
    __in_ecount(1) CMILSurfaceRect const *prcSource,
    __in_ecount(1) CMILSurfaceRect const *prcDest,
    __in RGNDATA *pDirtyRegion
    )
{
    HRESULT hr = S_OK;

    Assert(prcSource->left == prcDest->left);
    Assert(prcSource->top  == prcDest->top);

    Assert(prcSource->Width() == prcDest->Width());
    Assert(prcSource->Height() == prcDest->Height());
    Assert(m_hdcBack != NULL);    

    //
    // We don't ever actually present different source and destination, but we conceptually 
    // could, and it might be desirable in future to keep this behavior to do accelerated
    // hardware scrolling. So keeping this ability for now.
    //
    Assert( (prcSource->left == prcDest->left) && 
            (prcSource->right == prcDest->right) && 
            (prcSource->top == prcDest->top) && 
            (prcSource->bottom == prcDest->bottom)); 


    {
        //
        // Check to see if we need to recolor our software surface before presenting
        // This is used to help developers debug performance issues
        //
        if (   g_pMediaControl != NULL
            && g_pMediaControl->GetDataPtr()->RecolorSoftwareRendering
            )
        {
            ARGB * pBits = reinterpret_cast<ARGB *>(
                static_cast<BYTE *>(m_pvRenderBits)
                + prcSource->top * m_nBufferStride
                + prcSource->left * sizeof(ARGB)
                );

            g_pMediaControl->TintARGBBitmap(
                pBits,
                prcSource->right - prcSource->left,
                prcSource->bottom - prcSource->top,
                m_nBufferStride
                );
        }
    }

    HDC hdcFront = NULL;
    HPALETTE hPalOld = 0;

    if (   m_eWindowLayerType != MilWindowLayerType::ApplicationManagedLayer
        || m_hSystemPalette != NULL)
    {
        IFC(m_MILDC.BeginRendering(&hdcFront));

        if (m_hSystemPalette != NULL)
        {
            IFCW32(hPalOld = SelectPalette(hdcFront, m_hSystemPalette, TRUE));
            IFCW32(GDI_ERROR != RealizePalette(hdcFront));
        }
    }

    if (m_pConverter != NULL)
    {
        WICRect rcUpdate = {
            prcSource->left,
            prcSource->top,
            prcSource->right - prcSource->left,
            prcSource->bottom - prcSource->top
        };

        IFC(m_pConverter->CopyPixels(
            &rcUpdate,
            m_nDeviceStride,
            GetRequiredBufferSize(m_PresentPixelFormat, m_nDeviceStride, &rcUpdate),
            static_cast<BYTE *>(m_pvDeviceBits)
             + rcUpdate.Y * m_nDeviceStride
             + rcUpdate.X * (GetPixelFormatSize(m_PresentPixelFormat) / 8)
            ));
    }

    // Perform deferred scroll if there is one
    if (m_fHasDeferredScroll)
    {
        IFC(ScrollBlt(&m_sourceScrollRect, &m_destinationScrollRect, false, false));
        IFC(RemoveForegroundWindowScrollArtifacts(hdcFront));
        m_fHasDeferredScroll = false;
    }
    
    if (m_eWindowLayerType == MilWindowLayerType::ApplicationManagedLayer)
    {
        SIZE sz = { m_nWidth, m_nHeight };
        POINT ptSrc = { 0, 0 };

        Assert((hdcFront == NULL) == (m_hSystemPalette == NULL));

        hr = UpdateLayeredWindowEx(
            m_MILDC.GetHWND(),
            hdcFront,   // NULL if no m_hSystemPalette -> use default palette
            &m_MILDC.GetPosition(),
            &sz,
            m_hdcBack,
            &ptSrc,
            m_MILDC.GetColorKey(),
            &m_MILDC.GetBlendFunction(),
            m_MILDC.GetULWFlags(),
            prcSource);
        if (hr == HRESULT_FROM_WIN32(ERROR_GEN_FAILURE))
        {
            // This could be because sz is different from the size of the
            // HWND, and ULW_EX_NORESIZE was specified (by GetULWFlags)
            RECT rc = {};
            if (GetWindowRect(m_MILDC.GetHWND(), &rc) != 0)
            {
                auto width = static_cast<UINT>(std::labs(rc.right - rc.left));
                auto height = static_cast<UINT>(std::labs(rc.bottom - rc.top));

                if (m_nWidth != width || m_nHeight != height)
                {
                    // This mismatch between the window's actual size and our book-keeping in the render-thread 
                    // is generally due to the fact that the UI thread is yet to catch-up to a size-change
                    // notification (for e.g., WM_SIZE) and update itself, and then communicate
                    // that information back to the render thread. If we ignore this failure, the UI thread 
                    // will catch-up and a subsequent render-pass (and the corresponding UpdateLayeredWindowEx
                    // call within) would succeed.
                    // 
                    // This is probably the result of rapid changes in the window's size. 
                    hr = S_OK;
                }
            }
        }
        IFC(hr);
    }
    else
    {
        if (pDirtyRegion)
        {
            RECT *prcList = reinterpret_cast<RECT *>(pDirtyRegion->Buffer);

            for (DWORD i = 0; i < pDirtyRegion->rdh.nCount; i++)
            {                    
                // Present the individual dirty regions of the back buffer to the hwnd.
                IFCW32_CHECKSAD(::BitBlt(hdcFront,
                                         prcList[i].left,
                                         prcList[i].top,
                                         prcList[i].right - prcList[i].left,
                                         prcList[i].bottom - prcList[i].top,
                                         m_hdcBack,
                                         prcList[i].left,
                                         prcList[i].top,
                                         SRCCOPY));
            }
        }
        else
        {
            // Present the whole rectangle of the back buffer to the hwnd.
            IFCW32_CHECKSAD(::BitBlt(hdcFront,
                                     prcDest->left,
                                     prcDest->top,
                                     prcDest->Width(),
                                     prcDest->Height(),
                                     m_hdcBack,
                                     prcSource->left,
                                     prcSource->top,
                                     SRCCOPY));
        }        
    }

Cleanup:

    if (FAILED(hr))
    {
        //
        // There can be a variety of failure codes returned when a window is
        // destroyed while we are trying to draw to it with GDI.  To simplify
        // the callers error handling check for an invalid window and return
        // a failure code indicating such.  Otherwise just return whatever 
        // we could discern so far.
        //
        if (!IsWindow(m_MILDC.GetHWND()))
        {
            MIL_THR(HRESULT_FROM_WIN32(ERROR_INVALID_WINDOW_HANDLE));
        }
    }

    if (hdcFront != NULL)
    {
        if (hPalOld != NULL)
        {
            IGNORE_W32(0, SelectPalette(hdcFront, hPalOld, TRUE));
        }

        m_MILDC.EndRendering(hdcFront);
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::GetCompatibleBITMAPINFO
//
//  Synopsis:
//      Initialize the bitmapinfo structure for the backbuffer. Note we create
//      it top down by passing in a negative height. This is so that we don't
//      render on it upside-down
//
//------------------------------------------------------------------------------

HRESULT CSwPresenter32bppGDI::GetCompatibleBITMAPINFO(
    __in_ecount(1) HDC hdc,
    UINT nWidth,
    UINT nHeight,
    __out_ecount(1) BITMAPINFO *pbmi
    )
{
    HRESULT hr = S_OK;
    HBITMAP hbm = (HBITMAP)0;

    Assert (pbmi != NULL);

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);

    //
    // Unknown Win32 errors w/ valid HWND
    //  If the error is unspecified Win32 error and the window is valid then
    //  break when KD is present.  Set local variable fTryAgain to true, so the
    //  call will be made a second time to route out the true error.
    //
    //IFCW32_CHECKOOH(GR_GDIOBJECTS, hbm = CreateCompatibleBitmap(hdc, 1, 1));
    IFCW32_CHECKOOGDI_CHECKHWND(CCB, hbm = CreateCompatibleBitmap(hdc, 1, 1));

    IFCW32(GetDIBits(hdc, hbm, 0, 0, NULL, pbmi, DIB_RGB_COLORS));

    if (pbmi->bmiHeader.biBitCount <= 8)
    {
        //
        // We will let gdi do the bit manipulations
        // in this case. Since we will use the system
        // palette it will not do any color ops.
        //

        pbmi->bmiHeader.biBitCount = 8;
        pbmi->bmiHeader.biCompression = BI_RGB;

        PALETTEENTRY palEntries[256];
        GpMemset(palEntries, 0, sizeof(palEntries));

        int nCount = TW32(0, GetSystemPaletteEntries(hdc, 0, 256, palEntries));
        if (nCount > 0)
        {
            for (int n = 0; n < nCount; n++)
            {
                pbmi->bmiColors[n].rgbRed = palEntries[n].peRed;
                pbmi->bmiColors[n].rgbGreen = palEntries[n].peGreen;
                pbmi->bmiColors[n].rgbBlue = palEntries[n].peBlue;
                pbmi->bmiColors[n].rgbReserved = 0;
            }
        }
        else
        {
            //
            // We do not have a palette to do a better job than GDI
            //

            pbmi->bmiHeader.biBitCount = 32;
            pbmi->bmiHeader.biCompression = BI_RGB;
        }
    }
    else if (pbmi->bmiHeader.biBitCount == 16)
    {
        DWORD redMask = 0;
        DWORD greenMask = 0;
        DWORD blueMask = 0;

        DWORD *masks = reinterpret_cast<DWORD*>(&pbmi->bmiColors[0]);

        if ( pbmi->bmiHeader.biCompression == BI_BITFIELDS )
        {
            // Call a second time to get the color masks.
            // It's a GetDIBits Win32 "feature".

            IFCW32(GetDIBits(
                    hdc,
                    hbm,
                    0,
                    pbmi->bmiHeader.biHeight,
                    NULL,
                    pbmi,
                    DIB_RGB_COLORS
                    ));
        }
        else
        {
            pbmi->bmiHeader.biCompression = BI_BITFIELDS;

            masks[0] = 0x00007c00;
            masks[1] = 0x000003e0;
            masks[2] = 0x0000001f;
        }

        redMask = masks[0];
        greenMask = masks[1];
        blueMask = masks[2];

        if ((redMask   != 0x00007c00) ||
            (greenMask != 0x000003e0) ||
            (blueMask  != 0x0000001f))
        {
            if ((redMask   != 0x0000f800) ||
                (greenMask != 0x000007e0) ||
                (blueMask  != 0x0000001f))
            {
                //
                // We cannot convert from 32bpp to this 16bpp format,
                // so we will let gdi do it.
                //

                pbmi->bmiHeader.biBitCount = 32;
                pbmi->bmiHeader.biCompression = BI_RGB;
            }
        }
    }
    else
    {
        //
        // In all these cases we will resort to 32bpp backbuffer
        // and let gdi do some reasonable job of displaying
        // such a bitmap.
        //

        pbmi->bmiHeader.biBitCount = 32;
        pbmi->bmiHeader.biCompression = BI_RGB;
    }


#if DBG
    if (IsTagEnabled(tagMILDisableDithering))
    {
        pbmi->bmiHeader.biBitCount = 32;
        pbmi->bmiHeader.biCompression = BI_RGB;
    }
#endif

    pbmi->bmiHeader.biWidth = nWidth;
    pbmi->bmiHeader.biHeight = -(INT)nHeight;
    pbmi->bmiHeader.biPlanes = 1;
    pbmi->bmiHeader.biSizeImage = 0;
    pbmi->bmiHeader.biXPelsPerMeter = 10000;
    pbmi->bmiHeader.biYPelsPerMeter = 10000;
    pbmi->bmiHeader.biClrUsed = 0;
    pbmi->bmiHeader.biClrImportant = 0;

Cleanup:

    if (hbm != 0)
    {
        IGNORE_W32(0, DeleteObject(hbm));
    }

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::CreateFormatConverter
//
//  Synopsis:
//      Creates a converter to convert from the rendering buffer to the device
//      buffer.
//
//------------------------------------------------------------------------------

HRESULT CSwPresenter32bppGDI::CreateFormatConverter(
    __in_ecount(1) HDC hdcFront,
    __in_ecount(1) const BITMAPINFO *pbmi
    )
{
    HRESULT hr = S_OK;
    IWICPalette *pIPalette = NULL;
    IWICImagingFactory *pIWICFactory = NULL;
    IWICBitmapSource *pWrapperBitmapSource = NULL;

    Assert (m_pvRenderBits == NULL);

    //
    // Allocate memory for the rendering backbuffer
    //

    IFC(UIntMult(m_nBufferStride, abs(pbmi->bmiHeader.biHeight), &m_cbRenderBits));

    //
    // For the 16 bpp case, we need to create a DIB section for the rendering bits with
    // an associated DC. This is so that we can call BitBlt on it to scroll the bits to 
    // match the other scrolls when the accelerated scrolling optimization is taking place
    //
    // CreateDIBSection doesn't allow the caller to specify a pre-allocated buffer (except
    // through file handle mappings which we don't want), so for this case we need to let
    // CreateDIBSection allocate our bits, and then the DIB section is responsible for 
    // them.
    //
    if (pbmi->bmiHeader.biBitCount == 16)
    {
        //
        // Need to create a DIB section that matches our rendering bits. We already have
        // DIB section for the back buffer (32 bits).
        //
        BITMAPINFO bitmapInfoCopy;
        memcpy(&bitmapInfoCopy, pbmi, sizeof(BITMAPINFO));

        // Setup BITMAPINFO
        bitmapInfoCopy.bmiHeader.biBitCount = 32;
        bitmapInfoCopy.bmiHeader.biCompression = BI_RGB;

        // Create a DC
        IFCW32_CHECKOOGDI_CHECKHWND(CCDC, m_hdcRender = CreateCompatibleDC(hdcFront));

        // Create DIB section
        IFCW32_CHECKOOH(GR_GDIOBJECTS, m_hbmpRenderBuffer = CreateDIBSection(
            hdcFront,
            &bitmapInfoCopy,
            DIB_RGB_COLORS,
            &m_pvRenderBits,
            NULL,
            0
            ));        

        IFCOOM(m_pvRenderBits);
        
        // Select DIB into DC
        IFCW32((HBITMAP)SelectObject(
                            m_hdcRender,
                            m_hbmpRenderBuffer
                            ));
    }
    else
    {
        m_pvRenderBits = GpMalloc(
            Mt(MSwBackBuffer),
            m_cbRenderBits
            );
        
        IFCOOM(m_pvRenderBits);
    }

    m_pConverterInput = new CClientMemoryBitmap;
    IFCOOM(m_pConverterInput);
    m_pConverterInput->AddRef();

    //
    // If we are unable to blend to the desktop, then the defined behavior for
    // back buffers with alpha is to ignore the alpha channel and assume all
    // values are opaque.  Since we are always using premultiplied formats when
    // there is an alpha channel this means the premultiplied R, G, and B
    // values will be used for any non-opaque pixels.  To this end we
    // initialize a CClientMemoryBitmap to think it has the no-alpha channel
    // form of any premultiplied format; so that the format converter ignores
    // the alpha channel.
    //
    //  Conversion table
    //
    //    True back     |____ Intermediate back buffer format ________
    //    buffer format |  32bppPARGB (Blend)  |  ?bppRGB (No Blend)
    //  ----------------+----------------------+----------------------
    //      32bppPARGB  |  No conversion       |   32RGB -> ?RGB
    //     128bppPABGR  |  128PABGR->32PARGB   |  128BGR -> ?RGB
    //      32bppRGB    |     N/A              |   32RGB -> ?RGB
    //     128bppBGR    |     N/A              |  128BGR -> ?RGB
    //

    MilPixelFormat::Enum fmtBackBuffer = m_RenderPixelFormat;

    if (m_eWindowLayerType != MilWindowLayerType::ApplicationManagedLayer)
    {
        if (fmtBackBuffer == MilPixelFormat::PBGRA32bpp)
        {
            fmtBackBuffer = MilPixelFormat::BGR32bpp;
        }
        else if (fmtBackBuffer == MilPixelFormat::PRGBA128bppFloat)
        {
            fmtBackBuffer = MilPixelFormat::RGB128bppFloat;
        }
        else
        {
            Assert(   fmtBackBuffer == MilPixelFormat::BGR32bpp
                   || fmtBackBuffer == MilPixelFormat::RGB128bppFloat);
        }

        Assert(!HasAlphaChannel(fmtBackBuffer));
    }

    IFC(m_pConverterInput->HrInit(
            pbmi->bmiHeader.biWidth,
            abs(pbmi->bmiHeader.biHeight),
            fmtBackBuffer,
            m_cbRenderBits,
            m_pvRenderBits,
            m_nBufferStride
            ));

    IFC(WICCreateImagingFactory_Proxy(WINCODEC_SDK_VERSION_WPF, &pIWICFactory));
    IFC(pIWICFactory->CreateFormatConverter(&m_pConverter));

    IFC(WrapInClosestBitmapInterface(m_pConverterInput, &pWrapperBitmapSource));

    if (pbmi->bmiHeader.biBitCount == 8)
    {
        m_PresentPixelFormat = MilPixelFormat::Indexed8bpp;

        IFC(pIWICFactory->CreatePalette(&pIPalette));
        
        MilColorB colors[256];
        for (int i = 0; i < 256; i++)
        {
            colors[i] =
                MIL_COLOR(
                    0xFF,
                    pbmi->bmiColors[i].rgbRed,
                    pbmi->bmiColors[i].rgbGreen,
                    pbmi->bmiColors[i].rgbBlue);
        }

        IFC(pIPalette->InitializeCustom(colors, 256));

        IFC(m_pConverter->Initialize(
            pWrapperBitmapSource,
            MilPfToWic(m_PresentPixelFormat),
            WICBitmapDitherTypeErrorDiffusion,
            pIPalette,
            0.0,
            WICBitmapPaletteTypeCustom
            ));

        BYTE lp_buf[sizeof(LOGPALETTE) + (sizeof(PALETTEENTRY) * 255)];
        LOGPALETTE *lp = (LOGPALETTE *) lp_buf;

        lp->palVersion = 0x300;
        lp->palNumEntries = (WORD) TW32(0, GetSystemPaletteEntries(
            hdcFront,
            0,
            256,
            &lp->palPalEntry[0]));
        if (lp->palNumEntries != 0)
        {
            IFCW32_CHECKOOH(GR_GDIOBJECTS, m_hSystemPalette = CreatePalette(lp));
        }

    }
    else if (pbmi->bmiHeader.biBitCount == 16)
    {
        const DWORD *masks = reinterpret_cast<const DWORD*>(&pbmi->bmiColors[0]);
        DWORD redMask = masks[0];
        DWORD greenMask = masks[1];
        DWORD blueMask = masks[2];

        if ((redMask   == 0x00007c00) &&
            (greenMask == 0x000003e0) &&
            (blueMask  == 0x0000001f))
        {
            m_PresentPixelFormat = MilPixelFormat::BGR16bpp555;
        }
        else if ((redMask == 0x0000f800) &&
                (greenMask == 0x000007e0) &&
                (blueMask  == 0x0000001f))
        {
            m_PresentPixelFormat = MilPixelFormat::BGR16bpp565;
        }
        else
        {
            RIP("unexpected mask");
        }

        IFC(m_pConverter->Initialize(
            pWrapperBitmapSource,
            MilPfToWic(m_PresentPixelFormat),
            WICBitmapDitherTypeErrorDiffusion,
            NULL,
            0.0,
            WICBitmapPaletteTypeCustom
            ));
    }
    else if (pbmi->bmiHeader.biBitCount == 32)
    {
        m_PresentPixelFormat = (m_eWindowLayerType == MilWindowLayerType::ApplicationManagedLayer ?
                               MilPixelFormat::PBGRA32bpp :
                               MilPixelFormat::BGR32bpp);
        
        IFC(m_pConverter->Initialize(
            pWrapperBitmapSource,
            MilPfToWic(m_PresentPixelFormat),
            WICBitmapDitherTypeNone,
            NULL,
            0.0,
            WICBitmapPaletteTypeCustom
            ));
    }
    else
    {
        RIP("Unexpected bit depth");
    }

    hr = HrCalcDWordAlignedScanlineStride(
        pbmi->bmiHeader.biWidth,
        m_PresentPixelFormat, m_nDeviceStride);

Cleanup:
    ReleaseInterfaceNoNULL(pWrapperBitmapSource);
    ReleaseInterfaceNoNULL(pIWICFactory);
    ReleaseInterfaceNoNULL(pIPalette);

    // Other resources are released by caller calling FreeResources

    RRETURN(hr);
}

//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::CreateBackBuffers
//
//  Synopsis:
//      Creates the render and device backbuffers.
//
//------------------------------------------------------------------------------

HRESULT CSwPresenter32bppGDI::CreateBackBuffers(
    __in_ecount(1) HDC hdcFront,
    UINT nWidth,
    UINT nHeight
    )
{
    HRESULT hr = S_OK;

    BYTE bmi_buf[sizeof(BITMAPINFO) + (sizeof(RGBQUAD) * 255)];
    BITMAPINFO *pbmi = (BITMAPINFO *) bmi_buf;
    GpMemset(bmi_buf, 0, sizeof(bmi_buf));

    Assert(m_hdcBack == NULL);
    Assert(m_hbmpDeviceBuffer == NULL);

    Assert(m_hdcRender == NULL);
    Assert(m_hbmpRenderBuffer == NULL);

    //
    // Note regarding Win32 failures originating from this code:
    //  When this presenter is targeting a window and that window is destroyed
    //  GDI may return failure, but not set last error.  It is easy to check
    //  for this case with a call to IsWindow.  That is left to the caller to
    //  do.  The caller should be checking if its windows has become invalid
    //  whenever it sees a serious error, because what do rendering errors
    //  matter if the window has been destroyed?
    //
    //  See windows changelist # 143918 which added such a check to UCE.
    //

    //
    // Unknown Win32 errors w/ valid HWND
    //  If the error is unspecified Win32 error and the window is valid then
    //  break when KD is present.  Set local variable fTryAgain to true, so the
    //  call will be made a second time to route out the true error.
    //
    //IFCW32_CHECKOOH(GR_GDIOBJECTS, m_hdcBack = CreateCompatibleDC(hdcFront));
    {
        SET_MILINSTRUMENTATION_FLAGS(MILINSTRUMENTATIONFLAGS_BREAKANDCAPTURE | MILINSTRUMENTATIONFLAGS_BREAKINCLUDELIST);
        BEGIN_MILINSTRUMENTATION_HRESULT_LIST
            MILINSTRUMENTATION_DEFAULTOOMHRS
        END_MILINSTRUMENTATION_HRESULT_LIST
        IFCW32_CHECKOOGDI_CHECKHWND(CCDC, m_hdcBack = CreateCompatibleDC(hdcFront));
    }// End Instrumentation

    IFC(GetCompatibleBITMAPINFO(hdcFront, nWidth, nHeight, pbmi));

    //
    // Determine if we are blending to the desktop.  Requirements:
    //  1) Layered window updated via UpdateLayeredWindow
    //  2) Display is more than 8bpp - GDI ignores blending otherwise
    //  3) Back buffer format has alpha
    //
    // Update ULW parameters to reflect the blend
    //
    Assert(   (m_RenderPixelFormat == MilPixelFormat::BGR32bpp)
           || (m_RenderPixelFormat == MilPixelFormat::RGB128bppFloat)
           || (m_RenderPixelFormat == MilPixelFormat::PBGRA32bpp)
           || (m_RenderPixelFormat == MilPixelFormat::PRGBA128bppFloat));

    if (   m_eWindowLayerType == MilWindowLayerType::ApplicationManagedLayer
        && (m_pIdealDisplay->GetBitsPerPixel() > 8)
        && (   (m_RenderPixelFormat == MilPixelFormat::PBGRA32bpp)
            || (m_RenderPixelFormat == MilPixelFormat::PRGBA128bppFloat))
       )
    {
        //
        // Make sure to use a 32bpp device buffer
        //

        pbmi->bmiHeader.biBitCount = 32;
        pbmi->bmiHeader.biCompression = BI_RGB;
    }
    {
        SET_MILINSTRUMENTATION_FLAGS(MILINSTRUMENTATIONFLAGS_BREAKANDCAPTURE | MILINSTRUMENTATIONFLAGS_BREAKINCLUDELIST);
        BEGIN_MILINSTRUMENTATION_HRESULT_LIST
            MILINSTRUMENTATION_DEFAULTOOMHRS
        END_MILINSTRUMENTATION_HRESULT_LIST
        IFCW32_CHECKOOH(GR_GDIOBJECTS, m_hbmpDeviceBuffer = CreateDIBSection(
            hdcFront,
            pbmi,
            DIB_RGB_COLORS,
            &m_pvDeviceBits,
            NULL,
            0
            ));
    }// End Instrumentation

    IFC(HrCalcDWordAlignedScanlineStride(nWidth, m_RenderPixelFormat, m_nBufferStride));

    //
    // Conversion is not needed when we are going from and to a 32bpp format.
    // (Note this is independent of whether we are blending or not.  See
    // CreateFormatConverter for more details.)
    //

    if (   (pbmi->bmiHeader.biBitCount == 32)
        && (   (m_RenderPixelFormat == MilPixelFormat::BGR32bpp)
            || (m_RenderPixelFormat == MilPixelFormat::PBGRA32bpp))
       )
    {
        m_PresentPixelFormat = m_RenderPixelFormat;

        m_pvRenderBits = m_pvDeviceBits;

        IFC(HrGetRequiredBufferSize(
                m_RenderPixelFormat,
                m_nBufferStride,
                nWidth,
                nHeight,
                &m_cbRenderBits));

        m_nDeviceStride = 0;
    }
    else
    {
        IFC(CreateFormatConverter(hdcFront, pbmi));
    }

    IFCW32(m_hbmpPrevSelected = (HBITMAP)SelectObject(
        m_hdcBack,
        m_hbmpDeviceBuffer
        ));

    // If we're in 16 bit mode, must have a hbmp and hdc
    // for render bits too.
    Assert(    (pbmi->bmiHeader.biBitCount != 16)
            || (   (m_hbmpRenderBuffer != NULL)
                && (m_hdcRender != NULL)));

Cleanup:

    if (FAILED(hr))
    {
        FreeResources();
    }

    RRETURN(hr);
}


//+-----------------------------------------------------------------------------
//
//  Name:
//      CSwPresenter32bppGDI::FreeResources
//
//  Synopsis:
//      Frees all resources allocated by this class.
//
//------------------------------------------------------------------------------

void CSwPresenter32bppGDI::FreeResources()
{
    if (m_pConverter != NULL)
    {
        m_pConverter->Release();
        m_pConverter = NULL;
    }

    if (m_pConverterInput != NULL)
    {
        m_pConverterInput->Release();
        m_pConverterInput = NULL;
    }

    if (m_hdcRender !=  nullptr)
    {
        Assert(m_hbmpRenderBuffer != NULL);
        
        // Deselect m_hbmpRenderBuffer from m_hdcRender
        IGNORE_W32(0, SelectObject(m_hdcRender, NULL));

        if (m_hbmpRenderBuffer)
        {
            IGNORE_W32(0, DeleteObject(m_hbmpRenderBuffer));
            m_pvRenderBits = NULL;
            m_cbRenderBits = 0;            
        }

        IGNORE_W32(0, DeleteDC(m_hdcRender));

        m_hdcRender = (HDC)NULL;
        m_hbmpRenderBuffer = (HBITMAP)NULL;
    }

    if ((m_pvRenderBits != m_pvDeviceBits) && (m_pvRenderBits != NULL))
    {
        GpFree(m_pvRenderBits);
        m_pvRenderBits = NULL;
        m_cbRenderBits = 0;
    }
    
    if (m_hdcBack)
    {
        if (m_hbmpPrevSelected)
        {
            IGNORE_W32(0, SelectObject(m_hdcBack, m_hbmpPrevSelected));
        }

        if (m_hbmpDeviceBuffer)
        {
            IGNORE_W32(0, DeleteObject(m_hbmpDeviceBuffer));
        }

        if (m_hSystemPalette)
        {
            IGNORE_W32(0, DeleteObject(m_hSystemPalette));
        }

        IGNORE_W32(0, DeleteDC(m_hdcBack));

        m_hdcBack = (HDC)NULL;
        m_hbmpDeviceBuffer = (HBITMAP)NULL;
        m_hbmpPrevSelected = (HBITMAP)NULL;
        m_hSystemPalette = (HPALETTE)NULL;

        // This protects us in the case that FreeResources 
        // gets called more than once sequentially.        
        if (m_pvRenderBits == m_pvDeviceBits)
        {
            m_pvRenderBits = NULL;
        }            
        
        m_pvDeviceBits = NULL;
        m_nDeviceStride = 0;

        m_nWidth = 0;
        m_nHeight = 0;
    }

    if (m_pLock != NULL)
    {
        delete m_pLock;
        m_pLock = NULL;
    }
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwPresenter32bppGDI::SetPosition
//
//  Synopsis:
//      Remember Present position for when UpdateLayeredWindowEx is called.
//
//------------------------------------------------------------------------------

void
CSwPresenter32bppGDI::SetPosition(POINT ptOrigin)
{
    m_MILDC.SetPosition(ptOrigin);
}

//+-----------------------------------------------------------------------------
//
//  Member:
//      CSwPresenter32bppGDI::UpdatePresentProperties
//
//  Synopsis:
//      Remember Present transparency properties for when UpdateLayeredWindowEx
//      is called.
//
//------------------------------------------------------------------------------

void
CSwPresenter32bppGDI::UpdatePresentProperties(
    MilTransparency::Flags transparencyFlags,
    BYTE constantAlpha,
    COLORREF colorKey
    )
{
    m_MILDC.SetLayerProperties(transparencyFlags, constantAlpha, colorKey, m_pIdealDisplay);
}





