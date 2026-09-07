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

MtExtern(CSwRenderTargetHWND);
MtExtern(CSwPresenter32bppGDI);

/**************************************************************************
*
*   class ISwPresenter
*
*   This is the interface that a render target will use for the creation
*   and presentation of the back buffer. The implementor of the interface
*   must create a buffer which is always 32bpp, and must convert to a
*   different front buffer if need be.
*
**************************************************************************/

//
// There is only one presenter, CSwPresenter32bppGDI. Having a separate 
// CSwPresenterBase is probably redundant and unnecessary
//

class CSwPresenterBase :
    //  This needs to be an IWGXBitmap so it can be used in SetSurface but
    //  a lock operates on it and our lock implementation, CWGXBitmapLock, 
    //  requires a CWGXBitmap because of the Unlock() method
    public CWGXBitmap
{
protected:

    CSwPresenterBase(
        MilPixelFormat::Enum fmt
        );

    virtual ~CSwPresenterBase()
    {
    }

public:

    // IWGXBitmapSource methods

    STDMETHOD(GetSize)(
        __out_ecount(1) UINT *puiWidth,
        __out_ecount(1) UINT *puiHeight
        );

    STDMETHOD(GetPixelFormat)(
        __out_ecount(1) MilPixelFormat::Enum *pPixelFormat
        );

    STDMETHOD(GetResolution)(
        __out_ecount(1) double *pDpiX,
        __out_ecount(1) double *pDpiY
        );

    STDMETHOD(CopyPalette)(
        __inout_ecount(1) IWICPalette *pIPalette
        );

    STDMETHOD(CopyPixels)(
        __in_ecount_opt(1) const MILRect *prc,
        UINT cbStride,
        UINT cbBufferSize,
        __out_ecount(cbBufferSize) BYTE *pvPixels
        );

    // IWGXBitmap methods

    STDMETHOD(SetPalette)(
        __in_ecount(1) IWICPalette *pIPalette
        );

    STDMETHOD(SetResolution)(
        double dpiX,
        double dpiY
        );

    STDMETHOD(AddDirtyRect)(
        __in_ecount(1) const RECT *prcDirtyRectangle
        );

protected:
    CWGXBitmapLock *m_pLock;
    BOOL m_fLocked;

    UINT m_nWidth;
    UINT m_nHeight;
    const MilPixelFormat::Enum m_RenderPixelFormat;

    MilColorF m_ClearColor;
};



class CSwPresenter32bppGDI;


class CSwRenderTargetHWND :
    public CSwRenderTargetSurface,
    public IRenderTargetHWNDInternal
    DBG_STEP_RENDERING_COMMA_PARAM(public ISteppedRenderingDisplayRT)
{
protected:
    CSwRenderTargetHWND(DisplayId associatedDisplay);
    virtual ~CSwRenderTargetHWND();

public:
    static HRESULT Create(
        __in_opt HWND hwnd,
        MilWindowLayerType::Enum eWindowLayerType,
        __in_ecount(1) CDisplay const *pIdealDisplay,
        DisplayId associatedDisplay,
        UINT nWidth,
        UINT nHeight,
        MilRTInitialization::Flags dwFlags,
        __deref_out_ecount(1) CSwRenderTargetHWND **ppRT
        );

    DECLARE_METERHEAP_ALLOC(ProcessHeap, Mt(CSwRenderTargetHWND));

    // IUnknown.

    DECLARE_COM_BASE;

    //
    // IMILRenderTarget methods
    //

    //
    // IRenderTargetHWNDInternal.
    //

    void SetPosition(POINT ptOrigin) override;

    void UpdatePresentProperties(
        MilTransparency::Flags transparencyFlags,
        BYTE constantAlpha,
        COLORREF colorKey
        ) override;

    STDMETHOD(Present)(
        __in_ecount(1) const RECT *pRect
        );

    STDMETHOD(ScrollBlt) (
        THIS_
        __in_ecount(1) const RECT *prcSource,
        __in_ecount(1) const RECT *prcDest
        );    

    STDMETHOD(InvalidateRect)(
        __in_ecount(1) CMILSurfaceRect const *prc
        )
    {
        return CBaseSurfaceRenderTarget<CSwRenderTargetLayerData>::InvalidateRect(prc);
    }

    STDMETHOD(ClearInvalidatedRects)(
        )
    {
        RRETURN(CBaseSurfaceRenderTarget<CSwRenderTargetLayerData>::ClearInvalidatedRects());
    }
    
    STDMETHOD(Resize)(
        UINT uWidth,
        UINT uHeight
        );

    STDMETHOD(WaitForVBlank)();

    STDMETHOD_(VOID, AdvanceFrame)(
        UINT uFrameNumber
        );

protected:
    STDMETHOD(HrFindInterface)(__in_ecount(1) REFIID riid, __deref_out void **ppv);

#if DBG_STEP_RENDERING
private:
    BOOL m_fDbgClearOnPresent;

    override void ShowSteppedRendering(
        __in LPCTSTR pszRenderDesc,
        __in_ecount(1) const ISteppedRenderingSurfaceRT *pRT
        );
#endif DBG_STEP_RENDERING

private:

    HRESULT Init(
        __in_opt HWND hwnd,
        MilWindowLayerType::Enum eWindowLayerType,
        __in_ecount(1) CDisplay const *pIdealDisplay,
        MilRTInitialization::Flags nFlags
        );

private:

    HWND m_hwnd;

    CSwPresenter32bppGDI *m_pPresenter;
};



