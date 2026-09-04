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
//      Backbuffer present class using GDI functions.
//
//  $ENDTAG
//
//------------------------------------------------------------------------------

MtExtern(CSwPresenter32bppGDI);

//+-----------------------------------------------------------------------------
//
//  Class:
//      CReusableBitmapLock
//
//  Synopsis:
//      This class provides CWGXBitmapLock to be reused.  This is necessary because
//      the unlock and release are tightly coupled together.
//

 class CReusableBitmapLock : public CWGXBitmapLock
{
private:
    LONG m_cRef;

public:
    CReusableBitmapLock();

    ULONG STDMETHODCALLTYPE AddRef(void);
    ULONG STDMETHODCALLTYPE Release(void);
};

//+-----------------------------------------------------------------------------
//
//  Class:
//      CSwPresenter32bppGDI
//
//  Synopsis:
//      Implements the ISwPresenter interface using GDI calls. Does convertion
//      to pixel formats different from 32bpp and presentation of the back
//      buffer.
//
//------------------------------------------------------------------------------
class CSwPresenter32bppGDI : public CSwPresenterBase
{
public:
    CSwPresenter32bppGDI(
        __in_ecount(1) CDisplay const *pIdealDisplay,
        MilPixelFormat::Enum fmtBackBuffer
        );
    virtual ~CSwPresenter32bppGDI();

    DECLARE_METERHEAP_CLEAR(ProcessHeap, Mt(CSwPresenter32bppGDI));

    STDMETHOD(Lock)(
        __in_ecount(1) const WICRect *prcLock,
        DWORD flags,
        __deref_out_ecount(1) IWGXBitmapLock **ppILock
        );

    STDMETHOD(Unlock)(
        __in_ecount(1) CWGXBitmapLock *pBitmapLock
        );

    STDMETHOD(CopyPixels)(
        __in_ecount(1) const WICRect *prc,
        UINT cbOutputBufferStride,
        UINT cbOutputBufferSize,
        __out_ecount(cbOutputBufferSize) BYTE *pvPixels
        );

    // CSwPresenterBase methods

    void Init(
        __in_ecount_opt(1) HWND hwnd,
        MilWindowLayerType::Enum eWindowLayerType,
        MilRTInitialization::Flags nFlags
        );

    HRESULT Resize(
        UINT nWidth, 
        UINT nHeight
        );

    HRESULT ScrollBlt(
        __in_ecount(1) CMILSurfaceRect const *prcSource,
        __in_ecount(1) CMILSurfaceRect const *prcDest,
        bool fScrollBackBuffer,
        bool fDeferFrontBufferScroll
    );

    HRESULT Present(
        __in_ecount(1) CMILSurfaceRect const *prcSource,
        __in_ecount(1) CMILSurfaceRect const *prcDest,
        __in RGNDATA *pDirtyRegion
        );

    void FreeResources(
        );

    void SetPosition(POINT ptOrigin);

    void UpdatePresentProperties(
        MilTransparency::Flags transparencyFlags,
        BYTE constantAlpha,
        COLORREF colorKey
        );

private:

    HRESULT CreateBackBuffers(
        __in_ecount(1) HDC hdcFront,
        UINT nWidth, 
        UINT nHeight
        );

    HRESULT GetCompatibleBITMAPINFO(
        __in_ecount(1) HDC hdcFront,
        UINT nWidth, 
        UINT nHeight,
        __out_ecount(1) BITMAPINFO *pbmi
        );

    HRESULT CreateFormatConverter(
        __in_ecount(1) HDC hdcFront,
        __in_ecount(1) const BITMAPINFO *pbmi
        );
    
    HRESULT CSwPresenter32bppGDI::RemoveForegroundWindowScrollArtifacts(
        __in_ecount(1) HDC hdcFront
        );

private:

    // Display to render to
    CDisplay const * const m_pIdealDisplay;

    CMILDeviceContext m_MILDC;
    
    //
    // This is a pointer to the memory of a 32bpp bitmap 
    // that we use for sw rendering.
    //

    void *m_pvRenderBits;
    UINT m_cbRenderBits;
    UINT m_nBufferStride;

    //
    // This is a bitmap compatible with the pixel format of
    // the front buffer. It is possible the condition
    // m_pvDeviceBits == m_pvRenderBits to be true,
    // in which case we render directly into the bits of the 
    // device backbuffer.
    // m_hbmpDeviceBuffer is selected into m_hdcBack.
    //

    HDC m_hdcBack;
    HBITMAP m_hbmpDeviceBuffer;
    void *m_pvDeviceBits;
    UINT m_nDeviceStride;
    HPALETTE m_hSystemPalette;
    HBITMAP m_hbmpPrevSelected;
    MilPixelFormat::Enum m_PresentPixelFormat;

    // In the 16bpp case when there is a color converter, we
    // also need a HDC and HBITMAP for the render bits
    HDC m_hdcRender;
    HBITMAP m_hbmpRenderBuffer;

    //
    // Format converter from the rendering backbuffer to the
    // device one, in case we need one, and its input bitmap
    //

    IWICFormatConverter *m_pConverter;
    CClientMemoryBitmap *m_pConverterInput;
    
    //
    // Window layer type indicating back buffer is presented by calling
    // UpdateLayeredWindow and ULW parameters
    //

    MilWindowLayerType::Enum m_eWindowLayerType;

    //
    // Deferred scrolling for front buffer
    //
    bool m_fHasDeferredScroll;
    CMILSurfaceRect m_sourceScrollRect;
    CMILSurfaceRect m_destinationScrollRect;
    
};




