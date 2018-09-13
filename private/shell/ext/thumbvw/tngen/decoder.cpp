#include "stdafx.h"
#include "ctngen.h"

LPDIRECTDRAW g_lpDD=NULL;

HBITMAP ImgCreateDib(LONG xWid, LONG yHei, BOOL fPal, int cBitsPerPix,
    int cEnt, PALETTEENTRY * ppe, BYTE ** ppbBits, int * pcbRow);

void DrawImage(HDC hwnd, FILTERINFO *pFilter, RECT *prc);

RGBQUAD g_rgbBgColor = { 0, 0, 0, 0 };
RGBQUAD g_rgbFgColor = { 255, 255, 255, 0 };

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

#define Assert(x)
#define ReleaseMemoryDC(x)              DeleteObject(x)
#define GetMemoryDC()                   CreateCompatibleDC(NULL)
#define MulDivQuick                     MulDiv
#define Verify(x)                       x



HRESULT decoderDDrawInitialize()
{
    HRESULT retVal;

    //
    // DirectDraw initialization fails on systems with 16 colors.  In such cases, we don't want to
    // pop up the "You must be running in 256 color mode or higher" message because thumbnail generation
    // can happen in the background.  Instead we will silently fail.
    //

    UINT uMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    //
    // Can get an exception because ddraw.dll is delay loaded
    //

    _try
    {
        if (((retVal=DirectDrawCreate(NULL, &g_lpDD, NULL)) == DD_OK) &&
            ((retVal=g_lpDD->SetCooperativeLevel(NULL, DDSCL_NORMAL)) == DD_OK))
        {
            retVal = ERROR_SUCCESS;
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        //
        // Most likely we failed to load ddraw.dll.  Fail gracefully.
        //
        retVal = E_FAIL;
    }

    //
    // Restore previous error mode
    //
    
    SetErrorMode(uMode);
    
    return retVal;
}
HRESULT decoderInitialize(VOID)
{
    return ERROR_SUCCESS;
}

VOID decoderUninitialize(VOID)
{
    if (g_lpDD)
    {
        g_lpDD->Release();
        g_lpDD = NULL;
    }
    
    return;
}

STDMETHODIMP CThumbnailFCNContainer::InitializeEventSink(VOID)
{
    ZeroMemory(&m_Filter, sizeof(m_Filter));
        ZeroMemory(&m_rcProg, sizeof(m_rcProg));
        m_pDDrawSurface=NULL;
        m_dwLastTick=0;
        return ERROR_SUCCESS;
}

STDMETHODIMP CThumbnailFCNContainer::OnBeginDecode(DWORD* pdwEvents, 
   ULONG* pnFormats, GUID** ppFormats)
{
    GUID* pFormats;

    if (pdwEvents != NULL)
    {
        *pdwEvents = 0;
    }
    if (pnFormats != NULL)
    {
        *pnFormats = 0;
    }
    if (ppFormats != NULL)
    {
        *ppFormats = NULL;
    }
    if (pdwEvents == NULL)
    {
        return (E_POINTER);
    }
    if (pnFormats == NULL)
    {
        return (E_POINTER);
    }
    if (ppFormats == NULL)
    {
        return (E_POINTER);
    }

    pFormats = (GUID*)CoTaskMemAlloc(6*sizeof(GUID));
    if(pFormats == NULL)
    {
        return (E_OUTOFMEMORY);
    }
        
    pFormats[0] = BFID_RGB_24;
    pFormats[1] = BFID_INDEXED_RGB_8;
    pFormats[2] = BFID_RGB_555;
    pFormats[3] = BFID_RGB_565;
    pFormats[4] = BFID_INDEXED_RGB_4;
    pFormats[5] = BFID_INDEXED_RGB_1;
    *pnFormats = 6;

    *ppFormats = pFormats;
    *pdwEvents = IMGDECODE_EVENT_PALETTE|IMGDECODE_EVENT_BITSCOMPLETE;

    *pdwEvents |= IMGDECODE_EVENT_USEDDRAW;

    m_Filter.dwEvents = *pdwEvents;

    m_dwLastTick = GetTickCount();

    return (S_OK);
}

STDMETHODIMP CThumbnailFCNContainer::OnBitsComplete()
{
    return (S_OK);
}

STDMETHODIMP CThumbnailFCNContainer::OnDecodeComplete(HRESULT hrStatus)
{
    // Deliver NULL data for failures cases
    deliverDecompressedImage(NULL, 0, 0);

    if (FAILED(hrStatus))
    {
        // Release ddraw surface if allocated
	if (m_pDDrawSurface) 
	{
	    m_pDDrawSurface.Release();
	    m_pDDrawSurface = NULL;
	}

        return S_OK; // Decode failed for some reason--abort.
    }
    
    if (m_pDDrawSurface != NULL)
    {
        DDSURFACEDESC    ddsd;
        DDCOLORKEY      ddKey;
        RECT    rcDest;
        LPDIRECTDRAWPALETTE    pPal;
        PALETTEENTRY    ape[256];
        int    cbRow;
        BYTE *pbSrc, *pbDst;
        LONG i;

        // HACK

        m_pDDrawSurface->GetColorKey(DDCKEY_SRCBLT, &ddKey);
        m_Filter._lTrans = ddKey.dwColorSpaceLowValue;

        rcDest.left = rcDest.top = 0;
        rcDest.right = m_Filter._xWidth;
        rcDest.bottom = m_Filter._yHeight;
        ddsd.dwSize = sizeof(ddsd);
        if (SUCCEEDED(m_pDDrawSurface->Lock(&rcDest, &ddsd, DDLOCK_SURFACEMEMORYPTR | DDLOCK_WAIT, NULL)))
        {
            if (SUCCEEDED(m_pDDrawSurface->GetPalette(&pPal)))
            {
                pPal->GetEntries(0, 0, 256, ape);
                pPal->Release();
            }

            m_Filter._hbmDib = ImgCreateDib(
                m_Filter._xWidth,
                -m_Filter._yHeight,
                m_Filter._colorMode == 8 && m_Filter.m_nBitsPerPixel == 8, 
                m_Filter.m_nBitsPerPixel,
                (m_Filter.m_nBitsPerPixel <= 4) ? 0 : 256, 
                ape,
                &m_Filter.m_pbBits, 
                &cbRow);

            pbSrc = (BYTE *)ddsd.lpSurface;
            pbDst = m_Filter.m_pbBits;

            if (pbSrc && pbDst)
            {
                for (i = 0; i < m_Filter._yHeight; ++i)
                {
                    memcpy(pbDst, pbSrc, cbRow);
                    pbDst += cbRow;
                    pbSrc += ddsd.lPitch;
                }

                deliverDecompressedImage(m_Filter._hbmDib, m_Filter._xWidth, m_Filter._yHeight);
            }
            m_pDDrawSurface->Unlock(ddsd.lpSurface);
        }

        m_pDDrawSurface.Release();
        m_pDDrawSurface = NULL;
    }

    return S_OK; // Always return S_OK
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

HBITMAP ImgCreateDib(LONG xWid, LONG yHei, BOOL fPal, int cBitsPerPix,
    int cEnt, PALETTEENTRY * ppe, BYTE ** ppbBits, int * pcbRow)
{
    HDC hdcMem = NULL;
    HBITMAP hbm = NULL;
    struct {
        BITMAPINFOHEADER bmih;
        union {
            RGBQUAD argb[256];
            WORD aw[256];
        } u;
    } bmi;
    BYTE * pbBits;
    int cbRow;
    int i;
    
    Assert(cBitsPerPix == 1 || cBitsPerPix == 4 ||
        cBitsPerPix == 8 || cBitsPerPix == 24 || cBitsPerPix == 32);
    Assert(xWid > 0 && yHei > 0);

    bmi.bmih.biSize          = sizeof(BITMAPINFOHEADER);
    bmi.bmih.biWidth         = xWid;
    bmi.bmih.biHeight        = yHei;
    bmi.bmih.biPlanes        = 1;
    bmi.bmih.biBitCount      = (WORD)cBitsPerPix;
    bmi.bmih.biCompression   = BI_RGB;
    bmi.bmih.biSizeImage     = 0;
    bmi.bmih.biXPelsPerMeter = 0;
    bmi.bmih.biYPelsPerMeter = 0;
    bmi.bmih.biClrUsed       = 0;
    bmi.bmih.biClrImportant  = 0;

        fPal = FALSE;

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

        cbRow = (((xWid + 7) / 8) + 3) & ~3;
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

        cbRow = (((xWid + 1) / 2) + 3) & ~3;
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

        cbRow = (xWid + 3) & ~3;
    }
    else if (cBitsPerPix == 24)
    {
        cbRow = ((xWid * 3) + 3) & ~3;
    }
    else
    {
        Assert(cBitsPerPix == 32);
        cbRow = ((xWid * 4) + 3) & ~3;
    }

    if (pcbRow)
    {
        *pcbRow = cbRow;
    }

    hdcMem = GetMemoryDC();

    if (hdcMem == NULL)
        goto Cleanup;

    hbm = CreateDIBSection(hdcMem, (BITMAPINFO *)&bmi,
            fPal ? DIB_PAL_COLORS : DIB_RGB_COLORS,
            (void **)&pbBits, NULL, 0);

    if (hbm && ppbBits)
    {
        *ppbBits = pbBits;
    }

    // Fill the bits with garbage so that the client doesn't assume that
    // the DIB gets created cleared (on WinNT it does, on Win95 it doesn't).

    #if DBG==1
    if (hbm && pbBits)
        for (int c = cbRow * yHei; --c >= 0; ) pbBits[c] = (BYTE)c;
    #endif

Cleanup:
    if (hdcMem)
        ReleaseMemoryDC(hdcMem);

    return(hbm);
}

#define LINEBYTES(_wid,_bits) ((((_wid)*(_bits) + 31) / 32) * 4)

STDMETHODIMP CThumbnailFCNContainer::GetSurface(LONG nWidth, LONG nHeight, 
    REFGUID bfid, ULONG nPasses, DWORD dwHints, IUnknown ** ppSurface)
{
    DDSURFACEDESC ddsd;
    HRESULT retVal;

    if (!g_lpDD)
    {
        retVal = decoderDDrawInitialize();
        if (FAILED(retVal))
        {
            return retVal;
        }
    }

    (void)nPasses;
    (void)dwHints;
    
    if (ppSurface != NULL)
    {
        *ppSurface = NULL;
    }
    if (ppSurface == NULL)
    {
        return (E_POINTER);
    }

    ddsd.dwSize = sizeof(DDSURFACEDESC);
    ddsd.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PIXELFORMAT;
    ddsd.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN | DDSCAPS_SYSTEMMEMORY;
    ddsd.dwHeight = nHeight;
    ddsd.dwWidth = nWidth;

    ddsd.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);

    if (IsEqualGUID(bfid, BFID_INDEXED_RGB_8))
    {
        m_Filter.m_nBitsPerPixel = 8;

        ddsd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED8 | DDPF_RGB;
        ddsd.ddpfPixelFormat.dwRGBBitCount = 8;
        ddsd.ddpfPixelFormat.dwRBitMask = 0;
        ddsd.ddpfPixelFormat.dwGBitMask = 0;
        ddsd.ddpfPixelFormat.dwBBitMask = 0;
        ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;
    }
    else if (IsEqualGUID(bfid, BFID_RGB_24))
    {
        m_Filter.m_nBitsPerPixel = 24;

        ddsd.ddpfPixelFormat.dwFlags = DDPF_RGB;
        ddsd.ddpfPixelFormat.dwRGBBitCount = 24;
        ddsd.ddpfPixelFormat.dwRBitMask = 0x00FF0000L;
        ddsd.ddpfPixelFormat.dwGBitMask = 0x0000FF00L;
        ddsd.ddpfPixelFormat.dwBBitMask = 0x000000FFL;
    }
    else if (IsEqualGUID(bfid, BFID_INDEXED_RGB_4))
    {
        m_Filter.m_nBitsPerPixel = 4;
    
        ddsd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED4 | DDPF_RGB;
        ddsd.ddpfPixelFormat.dwRGBBitCount = 4;
        ddsd.ddpfPixelFormat.dwRBitMask = 0;
        ddsd.ddpfPixelFormat.dwGBitMask = 0;
        ddsd.ddpfPixelFormat.dwBBitMask = 0;
        ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;
    }
    else if (IsEqualGUID(bfid, BFID_INDEXED_RGB_1))
    {
        m_Filter.m_nBitsPerPixel = 1;
    
        ddsd.ddpfPixelFormat.dwFlags = DDPF_PALETTEINDEXED1 | DDPF_RGB;
        ddsd.ddpfPixelFormat.dwRGBBitCount = 1;
        ddsd.ddpfPixelFormat.dwRBitMask = 0;
        ddsd.ddpfPixelFormat.dwGBitMask = 0;
        ddsd.ddpfPixelFormat.dwBBitMask = 0;
        ddsd.ddpfPixelFormat.dwRGBAlphaBitMask = 0;
    }
    else
    {
        return (E_NOINTERFACE);
    }

    m_Filter._xWidth = nWidth;
    m_Filter._yHeight = nHeight;
    m_Filter.m_nPitch = -LONG( LINEBYTES( m_Filter._xWidth, 
    m_Filter.m_nBitsPerPixel ) );
    m_Filter.m_pbBits = NULL;
    m_Filter.m_pbFirstScanLine = NULL;

    // Don't create surfaces that require more than 4M (MAX_IMAGE_SIZE) of memory.
    // The computation below could still overflow for really large surfaces, but dividing
    // by 8 (bits per byte) before multiplying by the BPP allows us to handle reasonably
    // large images.
    if ((((nWidth * nHeight) / 8) * m_Filter.m_nBitsPerPixel) > MAX_IMAGE_SIZE)
    {
        return E_OUTOFMEMORY;
    }

    if (FAILED(g_lpDD->CreateSurface(&ddsd, &m_pDDrawSurface, NULL)))
        return (E_OUTOFMEMORY);

    // If this is a palette surface create/attach a palette to it.

    if (m_Filter.m_nBitsPerPixel == 8)
    {
    PALETTEENTRY ape[256];
        LPDIRECTDRAWPALETTE pDDPalette;

    g_lpDD->CreatePalette(DDPCAPS_8BIT | DDPCAPS_ALLOW256, ape, &pDDPalette, NULL);
        m_pDDrawSurface->SetPalette(pDDPalette);
        pDDPalette->Release();
    }

//    m_Filter.m_pDDrawSurface = m_pDDrawSurface;
//    m_pDDrawSurface->AddRef();

    *ppSurface = (IUnknown *)m_pDDrawSurface;
    (*ppSurface)->AddRef();
    
    return S_OK;
}

STDMETHODIMP CThumbnailFCNContainer::OnPalette()
{
    LONG nColors;
    RGBQUAD argb[256];
    HDC hdcMem;
    
    if (m_pDDrawSurface)
    {
        LPDIRECTDRAWPALETTE pDDPalette;
        PALETTEENTRY        ape[256];
        HRESULT             hResult;

        hResult = m_pDDrawSurface->GetPalette(&pDDPalette);
        if (SUCCEEDED(hResult))
        {
            pDDPalette->GetEntries(0, 0, 256, ape);
            pDDPalette->Release();

            CopyColorsFromPaletteEntries(argb, ape, 256);
        }
    }
    
    return (S_OK);
}

STDMETHODIMP CThumbnailFCNContainer::OnProgress(RECT* pBounds, BOOL bComplete)
{
    return (S_OK);
}
