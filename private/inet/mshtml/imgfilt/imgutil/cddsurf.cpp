#include "stdafx.h"
#include "imgutil.h"
#include "ddraw.h"
#include "cddsurf.h"

// Get rid of "synonyms" warning
#pragma warning(disable : 4097)

// Get rid of "unused formal parameters warning"
#pragma warning(disable : 4100)

#undef  DEFINE_GUID
#define DEFINE_GUID(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
        EXTERN_C const GUID name \
                = { l, w1, w2, { b1, b2,  b3,  b4,  b5,  b6,  b7,  b8 } }

DEFINE_GUID( IID_IDirectDrawSurface,		0x6C14DB81,0xA733,0x11CE,0xA5,0x21,0x00,
0x20,0xAF,0x0B,0xE5,0x60 );

DEFINE_GUID( IID_IDirectDrawPalette,		0x6C14DB84,0xA733,0x11CE,0xA5,0x21,0x00,
0x20,0xAF,0x0B,0xE5,0x60 );

RGBQUAD     g_rgbBgColor = { 0xFF, 0xFF, 0xFF, 0 };
RGBQUAD     g_rgbFgColor = { 0x00, 0x00, 0x00, 0 };

PALETTEENTRY g_peVga[16] =
{
    { 0x00, 0x00, 0x00, 0x00 }, // Black
    { 0x80, 0x00, 0x00, 0x00 }, // Dark red
    { 0x00, 0x80, 0x00, 0x00 }, // Dark green
    { 0x80, 0x80, 0x00, 0x00 }, // Dark yellow
    { 0x00, 0x00, 0x80, 0x00 }, // Dark blue
    { 0x80, 0x00, 0x80, 0x00 }, // Dark purple
    { 0x00, 0x80, 0x80, 0x00 }, // Dark aqua
    { 0xC0, 0xC0, 0xC0, 0x00 }, // Light grey
    { 0x80, 0x80, 0x80, 0x00 }, // Dark grey
    { 0xFF, 0x00, 0x00, 0x00 }, // Light red
    { 0x00, 0xFF, 0x00, 0x00 }, // Light green
    { 0xFF, 0xFF, 0x00, 0x00 }, // Light yellow
    { 0x00, 0x00, 0xFF, 0x00 }, // Light blue
    { 0xFF, 0x00, 0xFF, 0x00 }, // Light purple
    { 0x00, 0xFF, 0xFF, 0x00 }, // Light aqua
    { 0xFF, 0xFF, 0xFF, 0x00 }  // White
};

#define Verify(x) (x)

LONG g_lSecondaryObjCount = 0;

#define DecrementSecondaryObjectCount( idCaller ) DecrementSecondaryObjectCount_Actual()

inline void
DecrementSecondaryObjectCount_Actual()
{
    InterlockedDecrement(&g_lSecondaryObjCount);
}

#define IncrementSecondaryObjectCount( idCaller ) IncrementSecondaryObjectCount_Actual()

inline void
IncrementSecondaryObjectCount_Actual()
{
    Verify(InterlockedIncrement(&g_lSecondaryObjCount) > 0);
}

CBaseFT::CBaseFT(CRITICAL_SECTION * pcs)
{
    _ulRefs     = 1;
    _ulAllRefs  = 1;
    _pcs        = pcs;
    IncrementSecondaryObjectCount(10);
}

CBaseFT::~CBaseFT()
{
    DecrementSecondaryObjectCount(10);
}

void CBaseFT::Passivate()
{
}

ULONG CBaseFT::Release()
{
    ULONG ulRefs = (ULONG)InterlockedDecrement((LONG *)&_ulRefs);

    if (ulRefs == 0)
    {
        Passivate();
        SubRelease();
    }

    return(ulRefs);
}

ULONG CBaseFT::SubRelease()
{
    ULONG ulRefs = (ULONG)InterlockedDecrement((LONG *)&_ulAllRefs);

    if (ulRefs == 0)
    {
        delete this;
    }

    return(ulRefs);
}

void CopyColorsFromPaletteEntries(RGBQUAD *prgb, const PALETTEENTRY *ppe,
    UINT uCount)
{
    while (uCount--)
    {
        prgb->rgbRed   = ppe->peRed;
        prgb->rgbGreen = ppe->peGreen;
        prgb->rgbBlue  = ppe->peBlue;
        prgb->rgbReserved = 0;

        prgb++;
        ppe++;
    }
}


void CopyPaletteEntriesFromColors(PALETTEENTRY *ppe, const RGBQUAD *prgb,
    UINT uCount)
{
    while (uCount--)
    {
        ppe->peRed   = prgb->rgbRed;
        ppe->peGreen = prgb->rgbGreen;
        ppe->peBlue  = prgb->rgbBlue;
        ppe->peFlags = 0;

        prgb++;
        ppe++;
    }
}

#define MASK565_0   0x0000F800
#define MASK565_1   0x000007E0
#define MASK565_2   0x0000001F

HBITMAP ImgCreateDib(LONG xWid, LONG yHei, BOOL fPal, int cBitsPerPix,
    int cEnt, PALETTEENTRY * ppe, BYTE ** ppbBits, int * pcbRow)
{
    struct {
        BITMAPINFOHEADER bmih;
        union {
            RGBQUAD argb[256];
            WORD aw[256];
            DWORD adw[3];
        } u;
    } bmi;
    int i;

    if (cBitsPerPix != 8)
        fPal = FALSE;

    bmi.bmih.biSize          = sizeof(BITMAPINFOHEADER);
    bmi.bmih.biWidth         = xWid;
    bmi.bmih.biHeight        = yHei;
    bmi.bmih.biPlanes        = 1;
    bmi.bmih.biBitCount      = (WORD)((cBitsPerPix == 15) ? 16 : cBitsPerPix);
    bmi.bmih.biCompression   = (cBitsPerPix == 16) ? BI_BITFIELDS : BI_RGB;
    bmi.bmih.biSizeImage     = 0;
    bmi.bmih.biXPelsPerMeter = 0;
    bmi.bmih.biYPelsPerMeter = 0;
    bmi.bmih.biClrUsed       = 0;
    bmi.bmih.biClrImportant  = 0;

    if (cBitsPerPix == 1)
    {
        bmi.bmih.biClrUsed = 2;

        if (cEnt > 2)
            cEnt = 2;

        if (cEnt > 0)
        {
            bmi.bmih.biClrImportant = cEnt;
            CopyColorsFromPaletteEntries(bmi.u.argb, ppe, cEnt);
        }
        else
        {
            bmi.u.argb[0] = g_rgbBgColor;
            bmi.u.argb[1] = g_rgbFgColor;
        }
    }
    else if (cBitsPerPix == 4)
    {
        bmi.bmih.biClrUsed = 16;

        if (cEnt > 16)
            cEnt = 16;

        if (cEnt > 0)
        {
            bmi.bmih.biClrImportant = cEnt;
            CopyColorsFromPaletteEntries(bmi.u.argb, ppe, cEnt);
        }
        else
        {
            bmi.bmih.biClrImportant = 16;
            CopyColorsFromPaletteEntries(bmi.u.argb, g_peVga, 16);
        }
    }
    else if (cBitsPerPix == 8)
    {
        if (fPal)
        {
            bmi.bmih.biClrUsed = 256;

            for (i = 0; i < 256; ++i)
                bmi.u.aw[i] = (WORD)i;
        }
        else
        {
            if (cEnt > 0 && cEnt < 256)
            {
                bmi.bmih.biClrUsed = cEnt;
                bmi.bmih.biClrImportant = cEnt;
            }
            else
                bmi.bmih.biClrUsed = 256;

            if (cEnt && ppe)
            {
                CopyColorsFromPaletteEntries(bmi.u.argb, ppe, cEnt);
            }
        }
    }
    else if (cBitsPerPix == 16)
    {
        bmi.u.adw[0] = MASK565_0;
        bmi.u.adw[1] = MASK565_1;
        bmi.u.adw[2] = MASK565_2;
    }

    return ImgCreateDibFromInfo((BITMAPINFO *)&bmi, fPal ? DIB_PAL_COLORS : DIB_RGB_COLORS, ppbBits, pcbRow);
}


HBITMAP ImgCreateDibFromInfo(BITMAPINFO * pbmi, UINT wUsage, BYTE ** ppbBits, int * pcbRow)
{
    HDC 	hdcMem = NULL;
    HBITMAP	hbm = NULL;
    BYTE * 	pbBits;
    int 	cbRow;
    LONG    xWid, yHei;
    int 	cBitsPerPix;

    xWid = pbmi->bmiHeader.biWidth;
    yHei = pbmi->bmiHeader.biHeight;
    cBitsPerPix = pbmi->bmiHeader.biBitCount;
    
	cbRow = ((xWid * cBitsPerPix + 31) & ~31) / 8;

    if (pcbRow)
    {
        *pcbRow = cbRow;
    }

    hdcMem = CreateCompatibleDC(NULL);

    if (hdcMem == NULL)
        goto Cleanup;

    hbm = CreateDIBSection(hdcMem, pbmi, wUsage, (void **)&pbBits, NULL, 0);

    if (hbm && ppbBits)
    {
        *ppbBits = pbBits;
    }

Cleanup:
    if (hdcMem)
        DeleteDC(hdcMem);

    return(hbm);
}

CDDrawWrapper::CDDrawWrapper(HBITMAP hbmDib)
{
    m_hbmDib = hbmDib;
    GetObject(hbmDib, sizeof(DIBSECTION), &m_dsSurface);
    m_lPitch = ((m_dsSurface.dsBmih.biWidth * m_dsSurface.dsBmih.biBitCount + 31) & ~31) / 8;
    if (m_dsSurface.dsBmih.biHeight > 0)
    {
        m_pbBits = (BYTE *)m_dsSurface.dsBm.bmBits + (m_dsSurface.dsBm.bmHeight - 1) * m_lPitch;
        m_lPitch = -m_lPitch;
    }
    else
    {
        m_pbBits = (BYTE *)m_dsSurface.dsBm.bmBits;
    }

    // left, top already 0
    m_rcSurface.right = m_dsSurface.dsBm.bmWidth;
    m_rcSurface.bottom = m_dsSurface.dsBm.bmHeight;

    // initialize transparent index to -1
    m_ddColorKey.dwColorSpaceLowValue = m_ddColorKey.dwColorSpaceHighValue = (DWORD)-1;
}

CDDrawWrapper::~CDDrawWrapper()
{
}

STDMETHODIMP
CDDrawWrapper::QueryInterface(REFIID riid, void ** ppv)
{
    if (riid == IID_IDirectDrawSurface)
        *ppv = (IUnknown *)(IDirectDrawSurface *)this;
    else if (riid == IID_IDirectDrawPalette)
        *ppv = (IUnknown *)(IDirectDrawPalette *)this;
    else if (riid == IID_IUnknown)
        *ppv = (IUnknown *)(IDirectDrawSurface *)this;
    else    
        *ppv = NULL;

    if (*ppv)
    {
        ((LPUNKNOWN)*ppv)->AddRef();
        return(S_OK);
    }
    else
    {
        return(E_NOINTERFACE);
    }
}

STDMETHODIMP_(ULONG)
CDDrawWrapper::AddRef()
{
    return(super::AddRef());
}

STDMETHODIMP_(ULONG)
CDDrawWrapper::Release()
{
    return(super::Release());
}

STDMETHODIMP CDDrawWrapper::GetColorKey(DWORD dw, LPDDCOLORKEY lpKey)
{
    if (dw != DDCKEY_SRCBLT)
        return E_INVALIDARG;

    if (lpKey == NULL)
        return E_POINTER;

    memcpy(lpKey, &m_ddColorKey, sizeof(DDCOLORKEY));

    return S_OK;
}

STDMETHODIMP CDDrawWrapper::GetPalette(LPDIRECTDRAWPALETTE FAR* ppPal)
{
    if (ppPal == NULL)
        return E_POINTER;
        
    // Return interface to set color table if DIB section has one
    
    if (m_dsSurface.dsBmih.biBitCount <= 8)
    {
        *ppPal = (LPDIRECTDRAWPALETTE)this;
        ((LPUNKNOWN)*ppPal)->AddRef();
        return S_OK;
    }
    else
    {
        *ppPal = NULL;
        return E_NOINTERFACE;
    }
}


STDMETHODIMP CDDrawWrapper::SetColorKey(DWORD dwFlags, LPDDCOLORKEY pDDColorKey)
{
    if (dwFlags != DDCKEY_SRCBLT)
        return E_INVALIDARG;

    if (pDDColorKey == NULL)
        return E_POINTER;

    memcpy(&m_ddColorKey, pDDColorKey, sizeof(DDCOLORKEY));

    return S_OK;
}

STDMETHODIMP CDDrawWrapper::SetEntries(DWORD dwFlags, DWORD dwStart, DWORD dwCount, LPPALETTEENTRY pEntries)
{
    RGBQUAD argb[256];
    DWORD nColors = (DWORD)(1 << m_dsSurface.dsBmih.biBitCount);
    UINT nColorsSet;
    HDC hdc;
    HBITMAP hbm;
    
    if (dwFlags)
        return E_INVALIDARG;

    if (dwStart >= nColors || dwStart + dwCount > nColors)
        return E_INVALIDARG;

    if (pEntries == NULL)
        return E_POINTER;

    CopyColorsFromPaletteEntries(argb, pEntries, dwCount);
    hdc = CreateCompatibleDC(NULL);
    hbm = (HBITMAP)SelectObject(hdc, m_hbmDib);
    nColorsSet = SetDIBColorTable(hdc, (UINT)dwStart, (UINT)dwCount, argb);
    SelectObject(hdc, hbm);
    DeleteDC(hdc);
    
    return nColorsSet ? S_OK : E_FAIL;
}

STDMETHODIMP CDDrawWrapper::GetEntries(DWORD dwFlags, DWORD dwStart, DWORD dwCount, LPPALETTEENTRY pEntries)
{
    RGBQUAD argb[256];
    DWORD nColors = (DWORD)(1 << m_dsSurface.dsBmih.biBitCount);
    UINT nColorsGet;
    HDC hdc;
    HBITMAP hbm;
    
    if (dwFlags)
        return E_INVALIDARG;

    if (dwStart >= nColors || dwStart + dwCount > nColors)
        return E_INVALIDARG;

    if (pEntries == NULL)
        return E_POINTER;

    hdc = CreateCompatibleDC(NULL);
    hbm = (HBITMAP)SelectObject(hdc, m_hbmDib);
    nColorsGet = GetDIBColorTable(hdc, (UINT)dwStart, (UINT)dwCount, argb);
    SelectObject(hdc, hbm);
    DeleteDC(hdc);

    if (nColorsGet)
        CopyPaletteEntriesFromColors(pEntries, argb, dwCount);

    return nColorsGet ? S_OK : E_FAIL;
}


STDMETHODIMP CDDrawWrapper::Lock(LPRECT pRect, LPDDSURFACEDESC pSurfaceDesc, DWORD dwFlags, HANDLE hEvent)
{
    RECT    rcClip;
    
    if (pRect == NULL || pSurfaceDesc == NULL)
        return E_POINTER;

    if (pSurfaceDesc->dwSize != sizeof(DDSURFACEDESC))
        return E_INVALIDARG;

    if (hEvent)
        return E_INVALIDARG;

    IntersectRect(&rcClip, pRect, &m_rcSurface);
    if (!EqualRect(&rcClip, pRect))
        return E_INVALIDARG;
        
    pSurfaceDesc->dwFlags = DDSD_WIDTH | DDSD_HEIGHT | DDSD_PITCH;
    pSurfaceDesc->dwWidth = m_dsSurface.dsBm.bmWidth;
    pSurfaceDesc->dwHeight = m_dsSurface.dsBm.bmHeight;
    pSurfaceDesc->lPitch = m_lPitch;

    pSurfaceDesc->lpSurface = (LPVOID)(m_pbBits 
                                + pRect->top * m_lPitch 
                                + ((pRect->left * m_dsSurface.dsBmih.biBitCount) / 8));
     
    return S_OK;
}


STDMETHODIMP CDDrawWrapper::Unlock(LPVOID pBits)
{
    return S_OK;
}

// The remainder of these methods are not needed by the plugin filters

STDMETHODIMP CDDrawWrapper::AddAttachedSurface(LPDIRECTDRAWSURFACE lpdds)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::AddOverlayDirtyRect(LPRECT lprc)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::Blt(LPRECT lprcDest, LPDIRECTDRAWSURFACE lpdds, LPRECT lprcSrc, DWORD dw, LPDDBLTFX lpfx)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::BltBatch(LPDDBLTBATCH lpBlt, DWORD dwCount, DWORD dwFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::BltFast(DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpdds, LPRECT lprcSrc, DWORD dwTrans)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::DeleteAttachedSurface(DWORD dwFlags, LPDIRECTDRAWSURFACE lpdds)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::EnumAttachedSurfaces(LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfn)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::EnumOverlayZOrders(DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfn)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::Flip(LPDIRECTDRAWSURFACE lpdds, DWORD dwFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::GetAttachedSurface(LPDDSCAPS lpCaps, LPDIRECTDRAWSURFACE FAR * lpdds)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::GetBltStatus(DWORD dw)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::GetCaps(LPDDSCAPS lpCaps)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::GetClipper(LPDIRECTDRAWCLIPPER FAR* lpClipper)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::GetDC(HDC FAR * lphdc)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::GetFlipStatus(DWORD dw)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::GetOverlayPosition(LPLONG lpl1, LPLONG lpl2)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::GetPixelFormat(LPDDPIXELFORMAT pPixelFormat)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::GetSurfaceDesc(LPDDSURFACEDESC pSurfaceDesc)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::Initialize(LPDIRECTDRAW pDD, LPDDSURFACEDESC pSurfaceDesc)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::IsLost()
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::ReleaseDC(HDC hdc)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::Restore()
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::SetClipper(LPDIRECTDRAWCLIPPER pClipper)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::SetOverlayPosition(LONG x, LONG y)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::SetPalette(LPDIRECTDRAWPALETTE pDDPal)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::UpdateOverlay(LPRECT prc, LPDIRECTDRAWSURFACE pdds, LPRECT prc2, DWORD dw, LPDDOVERLAYFX pfx)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::UpdateOverlayDisplay(DWORD dw)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::UpdateOverlayZOrder(DWORD dw, LPDIRECTDRAWSURFACE pdds)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::GetCaps(LPDWORD lpdw)
{
    return E_NOTIMPL;
}

STDMETHODIMP CDDrawWrapper::Initialize(LPDIRECTDRAW lpdd, DWORD dwCount, LPPALETTEENTRY pEntries)
{
    return E_NOTIMPL;
}


