//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:       imgbits.cxx
//
//  Contents:   CImbBits, CImgBitsDIB
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

#ifndef X_PUTIL_HXX_
#define X_PUTIL_HXX_
#include "putil.hxx"
#endif

#ifndef X_DITHERS_H_
#define X_DITHERS_H_
#include "dithers.h"
#endif

#ifndef X_IMGBITS_HXX_
#define X_IMGBITS_HXx_
#include "imgbits.hxx"
#endif

MtDefine(CImgBitsDIB, DIBSection, "Bitmaps - CImgBitsDIB")
MtDefine(CImgBitsDIB_pvImg, DIBSection, "Bitmaps - bits")
MtDefine(CImgBitsDIB_pbmih, DIBSection, "Bitmaps - headers")
MtExtern(DIBSection)

#ifdef _WIN64
DeclareTag(tagUseMaskBlt,    "Dwn", "Img: Use MaskBlt on Win64");
#endif

DeclareTag(tagNoTransMask,   "Dwn", "Img: Don't compute TransMask");
DeclareTag(tagForceRawDIB,   "Dwn", "Img: Force use of raw DIB");
DeclareTag(tagIgnorePalette, "Dwn", "Img: Ignore nonhalftone palette");
DeclareTag(tagAssertRgbBlt,  "Dwn", "Img: Assert on RGB mode palette blt");

DeclareTag(tagForceRgbBlt,   "Dwn", "Img: Force RGB mode palette plt");
#define MASK565_0   0x0000F800
#define MASK565_1   0x000007E0
#define MASK565_2   0x0000001F

void
CImgBitsDIB::FreeMemoryNoClear()
{
    if (_hbmImg)
    {
        #ifdef PERFMETER
        MtAdd(Mt(DIBSection), -1, CbLine() * _yHeight);
        #endif

        Verify(DeleteObject(_hbmImg));
        _pvImgBits = NULL;
    }
    
    if (_hbmMask)
    {
        #ifdef PERFMETER
        MtAdd(Mt(DIBSection), -1, CbLineMask() * _yHeight);
        #endif

        Verify(DeleteObject(_hbmMask));
        _pvMaskBits = NULL;
    }

    MemFree(_pvImgBits);
    MemFree(_pvMaskBits);
    MemFree(_pbmih);
}

void
CImgBitsDIB::FreeMemory()
{
    FreeMemoryNoClear();
    _hbmImg = NULL;
    _hbmMask = NULL;
    _pvImgBits = NULL;
    _pvMaskBits = NULL;
    _pbmih = NULL;
    _cColors = 0;
}

static const RGBQUAD g_rgbWhite = { 255, 255, 255, 0 };
static const RGBQUAD g_rgbBlack = { 0, 0, 0, 0 };

CImgBitsDIB::CImgBitsDIB(float f)
{
    memset(this, 0, sizeof(CImgBitsDIB));
}

HRESULT
CImgBitsDIB::AllocCopyBitmap(HBITMAP hbm, BOOL fPalColors, LONG lTrans)
{
    HRESULT hr;
    
    LONG iBitCount;
    struct {
        BITMAPINFOHEADER bmih;
        union {
            RGBQUAD rgb[256];
            WORD windex[256];
            DWORD dwbc[3];
        };
    } header;

    HDC hdc;

    hdc = GetMemoryDC();
    if (!hdc)
        return E_OUTOFMEMORY;
                
    memset(&header, 0, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 256);
    header.bmih.biSize = sizeof(BITMAPINFOHEADER);
    
    GetDIBits(hdc, hbm, 0, 0, NULL, (BITMAPINFO *)&header, fPalColors ? DIB_PAL_COLORS : DIB_RGB_COLORS);
    
    // GetDIBits(hdc, hbm, 0, 0, NULL, (BITMAPINFO *)&header, fPalColors ? DIB_PAL_COLORS : DIB_RGB_COLORS);
    // A second call to GetDIBits should get the color table if any, but it doesn't work on Win95, so we use
    // GetDIBColorTable instead (dbau)

    if (header.bmih.biBitCount <= 8)
    {
        HBITMAP hbmSav;
        hbmSav = (HBITMAP)SelectObject(hdc, hbm);
        GetDIBColorTable(hdc, 0, 1 << header.bmih.biBitCount, header.rgb);
        SelectObject(hdc, hbmSav);
    }
    
    iBitCount = header.bmih.biBitCount;
    if (iBitCount == 16)
    {
        if (header.bmih.biCompression != BI_BITFIELDS ||
            header.dwbc[0] != MASK565_0 || header.dwbc[1] != MASK565_1 || header.dwbc[2] != MASK565_2)
        {
            iBitCount = 15;
        }
    }

    BOOL fColorTable = (!fPalColors && iBitCount <= 8);
    if (fColorTable)
    {
        hr = THR(AllocDIB(iBitCount, header.bmih.biWidth, header.bmih.biHeight, header.rgb, 1 << iBitCount, lTrans, lTrans == -1));
    }
    else
    {
        hr = THR(AllocDIB(iBitCount, header.bmih.biWidth, header.bmih.biHeight, NULL, 0, -1, TRUE));
    }
    
    GetDIBits(hdc, hbm, 0, header.bmih.biHeight, GetBits(), (BITMAPINFO *)&header, fPalColors ? DIB_PAL_COLORS : DIB_RGB_COLORS);

    ReleaseMemoryDC(hdc);

    return S_OK;
}

HRESULT
CImgBitsDIB::AllocMaskSection()
{
    HRESULT hr = S_OK;
    HDC hdcMem;
    
    struct {
        BITMAPINFOHEADER bmih;
        union {
            RGBQUAD argb[2];
        } u;
    } bmi;
    
    memset(&bmi, 0, sizeof(bmi));

    bmi.bmih.biSize          = sizeof(BITMAPINFOHEADER);
    bmi.bmih.biWidth         = _xWidth;
    bmi.bmih.biHeight        = _yHeight;
    bmi.bmih.biPlanes        = 1;
    bmi.bmih.biBitCount      = 1;
    bmi.u.argb[1] = g_rgbWhite;
    
    hdcMem = GetMemoryDC();

    if (hdcMem == NULL)
        goto OutOfMemory;

    _hbmMask = CreateDIBSection(hdcMem, (BITMAPINFO *)&bmi, DIB_RGB_COLORS, &_pvMaskBits, NULL, 0);
    if (!_hbmMask || !_pvMaskBits)
        goto OutOfMemory;
        
    #ifdef PERFMETER
    MtAdd(Mt(DIBSection), +1, CbLineMask() * _yHeight);
    #endif


Cleanup:
    if (hdcMem)
        ReleaseMemoryDC(hdcMem);

    RRETURN(hr);

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

HRESULT
CImgBitsDIB::AllocDIBSection(LONG iBitCount, LONG xWidth, LONG yHeight, RGBQUAD *argbTable, LONG cTable, LONG lTrans)
{
    HRESULT hr;
    struct {
        BITMAPINFOHEADER bmih;
        union {
            RGBQUAD argb[256];
            WORD aw[256];
            DWORD adw[3];
        } u;
    } bmi;
    int i;
    BOOL fPal;
    LONG cTableAlloc;

    fPal = (iBitCount == 8 && !argbTable);
    
    if ((iBitCount > 8 && lTrans > 0) || (lTrans > (1 << iBitCount)))
    {
        lTrans = -1;
    }

    bmi.bmih.biSize          = sizeof(BITMAPINFOHEADER);
    bmi.bmih.biWidth         = xWidth;
    bmi.bmih.biHeight        = yHeight;
    bmi.bmih.biPlanes        = 1;
    bmi.bmih.biBitCount      = (iBitCount == 15) ? 16 : iBitCount;
    bmi.bmih.biCompression   = (iBitCount == 16) ? BI_BITFIELDS : BI_RGB;
    bmi.bmih.biSizeImage     = 0;
    bmi.bmih.biXPelsPerMeter = 0;
    bmi.bmih.biYPelsPerMeter = 0;
    bmi.bmih.biClrUsed       = 0;
    bmi.bmih.biClrImportant  = 0;

    cTableAlloc = cTable;
    if (lTrans >= cTable)
        cTableAlloc = lTrans + 1;

    if (iBitCount == 1)
    {
        bmi.bmih.biClrUsed = 2;

        if (cTable > 2)
            cTable = 2;

        if (cTable > 0)
        {
            bmi.bmih.biClrImportant = cTableAlloc;
            memcpy(bmi.u.argb, argbTable, cTable * sizeof(RGBQUAD));
        }
        else
        {
            bmi.u.argb[0] = g_rgbBlack;
            bmi.u.argb[1] = g_rgbWhite;
        }
    }
    else if (iBitCount == 4)
    {
        bmi.bmih.biClrUsed = 16;

        if (cTable > 16)
            cTable = 16;

        if (cTable > 0)
        {
            bmi.bmih.biClrImportant = cTableAlloc;
            memcpy(bmi.u.argb, argbTable, cTable * sizeof(RGBQUAD));
        }
        else
        {
            bmi.bmih.biClrImportant = 16;
            CopyColorsFromPaletteEntries(bmi.u.argb, g_peVga, 16);
        }
    }
    else if (iBitCount == 8)
    {
        if (fPal)
        {
            bmi.bmih.biClrUsed = 256;

            for (i = 0; i < 256; ++i)
                bmi.u.aw[i] = (WORD)i;
        }
        else
        {
            if (cTable > 256)
                cTable = 256;
                
            if (cTable > 0)
            {
                bmi.bmih.biClrUsed = cTableAlloc;
                bmi.bmih.biClrImportant = bmi.bmih.biClrUsed;
                memcpy(bmi.u.argb, argbTable, cTable * sizeof(RGBQUAD));

                if (lTrans >= 0)
                {
                    bmi.u.argb[lTrans] = g_rgbWhite;
                }
            }
            else
            {
                bmi.bmih.biClrUsed = 256;
                _fPalColors = TRUE;
            }
        }
    }
    else if (iBitCount == 16)
    {
        bmi.u.adw[0] = MASK565_0;
        bmi.u.adw[1] = MASK565_1;
        bmi.u.adw[2] = MASK565_2;
    }

    hr = THR(AllocDIBSectionFromInfo((BITMAPINFO *)&bmi, fPal));
    
    _iTrans = lTrans;

    RRETURN(hr);
}

HRESULT
CImgBitsDIB::AllocDIBSectionFromInfo(BITMAPINFO * pbmi, BOOL fPal)
{
    HDC     hdcMem = NULL;
    HRESULT hr = S_OK;

    Assert(!_pvImgBits);
    Assert(!_hbmImg);

    _xWidth = pbmi->bmiHeader.biWidth;
    _yHeight = pbmi->bmiHeader.biHeight;
    _iBitCount = pbmi->bmiHeader.biBitCount;
    _yHeightValid = _yHeight;
    _iTrans = -1;
    _cColors = pbmi->bmiHeader.biClrUsed;

    if (_iBitCount == 16 &&
        (pbmi->bmiHeader.biCompression != BI_BITFIELDS ||
            ((DWORD*)(pbmi->bmiColors))[0] != MASK565_0 ||
            ((DWORD*)(pbmi->bmiColors))[1] != MASK565_1 ||
            ((DWORD*)(pbmi->bmiColors))[2] != MASK565_2))
    {
        _iBitCount = 15;
    }
    
    Assert(_iBitCount == 1 || _iBitCount == 4 || _iBitCount == 8 ||
           _iBitCount == 15 || _iBitCount == 16 || _iBitCount == 24 || _iBitCount == 32);
           
    Assert(_xWidth > 0 && _yHeight > 0);

    hdcMem = GetMemoryDC();

    if (hdcMem == NULL)
        goto OutOfMemory;

    _hbmImg = CreateDIBSection(hdcMem, pbmi, fPal ? DIB_PAL_COLORS : DIB_RGB_COLORS, &_pvImgBits, NULL, 0);

    if (!_hbmImg || !_pvImgBits)
        goto OutOfMemory;

    #ifdef PERFMETER
    MtAdd(Mt(DIBSection), +1, CbLine() * _yHeight);
    #endif

    // Fill the bits with garbage so that the client doesn't assume that
    // the DIB gets created cleared (on WinNT it does, on Win95 it doesn't).

    #if DBG==1 && !defined(WIN16)
    int c;
    for (c = CbLine() * _yHeight; --c >= 0; ) ((BYTE *)_pvImgBits)[c] = (BYTE)c;
    #endif

Cleanup:
    if (hdcMem)
        ReleaseMemoryDC(hdcMem);

    RRETURN(hr);

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}

HRESULT
CImgBitsDIB::AllocDIB(LONG iBitCount, LONG xWidth, LONG yHeight, RGBQUAD *argbTable, LONG cTable, LONG lTrans, BOOL fOpaque)
{
    Assert(!_pvImgBits);
    Assert(!_pvMaskBits);
    Assert(iBitCount == 1 || iBitCount == 4 || iBitCount == 8 || iBitCount == 15 || iBitCount == 16 || iBitCount == 24 || iBitCount == 32);
    Assert(!!argbTable == !!cTable);
    Assert(!argbTable || iBitCount <= 8);
    Assert(xWidth >= 0 && yHeight >= 0);

#if DBG == 1
    if (!IsTagEnabled(tagForceRawDIB))
#endif
    if (g_dwPlatformID == VER_PLATFORM_WIN32_NT
#ifdef UNIX
        || g_dwPlatformID == VER_PLATFORM_WIN32_UNIX
#endif
        )
    {
        LONG cbImgSize = xWidth * yHeight * iBitCount / 8;

        // On NT, we'd rather use HBITMAPs for big images and transparent images
        // (raw DIBs are for small opaque images)


#ifndef UNIX // This is a hack for Unix, before we fix BitBlt api        
        if (cbImgSize > 10240 || !fOpaque)
#endif
            RRETURN(AllocDIBSection(iBitCount, xWidth, yHeight, argbTable, cTable, lTrans));
    }
    
    _xWidth = xWidth;
    _yHeight = yHeight;
    _iBitCount = iBitCount;
    _yHeightValid = _yHeight;

    if ((iBitCount > 8 && lTrans > 0) || (lTrans > (1 << iBitCount)))
    {
        lTrans = -1;
    }

    _iTrans = lTrans;

    if (iBitCount == 15)
        iBitCount = 16;

    _pvImgBits = MemAlloc(Mt(CImgBitsDIB_pvImg), CbLine() * yHeight);
    if (!_pvImgBits)
        goto OutOfMemory;
    
    // need a header only with color table; otherwise, one is cobbled up while blitting
    if (argbTable)
    {
        // size of the color table or masks: depends on bit count

        LONG cTableCopy;
        LONG cTableAlloc;

        cTableCopy = cTable;
        
        if (cTableCopy > (1 << iBitCount))
            cTableCopy = (1 << iBitCount);

        cTableAlloc = (lTrans >= cTableCopy) ? lTrans + 1 : cTableCopy;

        _cColors = cTableAlloc;

        // Dang it.  Some popular display drivers ignore the biClrUsed field and assume 4bpp
        // bitmaps have 16 color entries and 8bpp have 256 color entries.  If we don't give them
        // what they expect it can lead to random faults / hangs when accessing beyond our 
        // shortened color table touches non-present memory.
        //
        // lTrans could by 16 or 256, requiring 17 or 257 entries so watch out for that...

        if (iBitCount == 4)
            cTableAlloc = (lTrans >= 16) ? lTrans + 1 : 16;
        else if (iBitCount == 8)
            cTableAlloc = (lTrans >= 256) ? lTrans + 1 : 256;

        _pbmih = (BITMAPINFOHEADER *)MemAllocClear(Mt(CImgBitsDIB_pbmih), sizeof(BITMAPINFOHEADER) + cTableAlloc * sizeof(RGBQUAD));
        if (!_pbmih)
            goto OutOfMemory;
            
        _pbmih->biSize = sizeof(BITMAPINFOHEADER);
        _pbmih->biWidth = xWidth;
        _pbmih->biHeight = yHeight;
        _pbmih->biPlanes = 1;
        _pbmih->biBitCount = iBitCount;
        _pbmih->biClrUsed = cTableCopy;
        _pbmih->biClrImportant = cTableCopy;

        memcpy(_pbmih + 1, argbTable, cTableCopy * sizeof(RGBQUAD));

        if (lTrans >= 0)
            ((RGBQUAD *)(_pbmih + 1))[lTrans] = g_rgbWhite;
    }
    else
    {
        if (iBitCount <= 8)
            _fPalColors = TRUE;

        _cColors = 1 << iBitCount;
    }
    
    return S_OK;

OutOfMemory:
    MemFree(_pvImgBits);
    _pvImgBits = NULL;

    MemFree(_pbmih);
    _pbmih = NULL;
    
    return E_OUTOFMEMORY;
}

HRESULT
CImgBitsDIB::AllocMask()
{
    Assert(_pvImgBits);
    Assert(!_pvMaskBits);

    // if a DIBSection is being used, use the same for the mask
    if (_hbmImg)
        return AllocMaskSection();
    
    _pvMaskBits = MemAlloc(Mt(CImgBitsDIB_pvImg), CbLineMask() * _yHeight);
    if (!_pvMaskBits)
        goto OutOfMemory;

    return S_OK;

OutOfMemory:
    return E_OUTOFMEMORY;
}

HRESULT
CImgBitsDIB::AllocMaskOnly(LONG xWidth, LONG yHeight)
{
    Assert(!_pvImgBits);
    Assert(!_pvMaskBits);
    
    _xWidth = xWidth;
    _yHeight = yHeight;
    _yHeightValid = yHeight;
    
    _pvMaskBits = MemAlloc(Mt(CImgBitsDIB_pvImg), CbLineMask() * yHeight);
    if (!_pvMaskBits)
        goto OutOfMemory;

    return S_OK;

OutOfMemory:
    return E_OUTOFMEMORY;
}

void
CImgBitsDIB::GetColors(long iFirst, long cColors, RGBQUAD *prgb)
{
    if (_hbmImg)
    {
        HDC hdcMem;
        HBITMAP hbmSav;
        hdcMem = GetMemoryDC();
        hbmSav = (HBITMAP)SelectObject(hdcMem, _hbmImg);
        GetDIBColorTable(hdcMem, iFirst, cColors, prgb);
        SelectObject(hdcMem, hbmSav);
        ReleaseMemoryDC(hdcMem);
    }
    else
    {
        if (iFirst < 0)
        {
            memset(prgb, 0, sizeof(RGBQUAD) * -iFirst);
            prgb -= iFirst;
            cColors += iFirst;
            iFirst = 0;
        }
        
        if (iFirst + cColors > _cColors)
        {
            memset(prgb + _cColors, 0, sizeof(RGBQUAD) * (iFirst + cColors - _cColors));
            cColors = _cColors - iFirst;
        }
        
        if (_fPalColors)
        {
            memcpy(prgb, g_rgbHalftone + iFirst, sizeof(RGBQUAD) * cColors);
        }
        else
        {
            memcpy(prgb, (RGBQUAD *)(_pbmih + 1) + iFirst, sizeof(RGBQUAD) * cColors);
        }
    }
}
    
void
CImgBitsDIB::SetTransIndex(LONG lIndex)
{
    Assert(_iBitCount <= 8);
    Assert(lIndex >= -1 && lIndex < (1 << _iBitCount));
    
    _iTrans = lIndex;

    if (lIndex >= 0)
    {
        if (_hbmImg)
        {
            HDC hdcMem;
            HBITMAP hbmSav;

            hdcMem = GetMemoryDC();
            if (!hdcMem)
                return;

            hbmSav = (HBITMAP)SelectObject(hdcMem, _hbmImg);
            SetDIBColorTable(hdcMem, lIndex, 1, &g_rgbWhite);
            SelectObject(hdcMem, hbmSav);
            ReleaseMemoryDC(hdcMem);
        }
        else if (_pbmih)
        {
            ((RGBQUAD *)(_pbmih + 1))[_iTrans] = g_rgbWhite;
        }
    }
}

void
CImgBitsDIB::SetValidLines(LONG yLines)
{
    if (yLines >= 0)
        _yHeightValid = yLines;
    else
        _yHeightValid = _yHeight;
}



// Used to StretchBlt an image whose bits are offset from the coordinates by the given amount

void
CImgBits::StretchBltOffset(HDC hdc, RECT * prcDst, RECT * prcSrc, LONG xOffset, LONG yOffset, DWORD dwRop, DWORD dwFlags)
{
    RECT rcSrcOffset;

    rcSrcOffset.left    = prcSrc->left    - xOffset;
    rcSrcOffset.top     = prcSrc->top     - yOffset;
    rcSrcOffset.right   = prcSrc->right   - xOffset;
    rcSrcOffset.bottom  = prcSrc->bottom  - yOffset;

    StretchBlt(hdc, prcDst, &rcSrcOffset, dwRop, dwFlags);
}

// Draw the src rect of the specified image into the dest rect of the hdc

void
CImgBitsDIB::StretchBlt(HDC hdc, RECT * prcDst, RECT * prcSrc, DWORD dwRop, DWORD dwFlags)
{
    HDC         hdcDib = NULL;
    HBITMAP     hbmSav = NULL;
    int         xDst            = prcDst->left;
    int         yDst            = prcDst->top;
    int         xDstWid         = prcDst->right - xDst;
    int         yDstHei         = prcDst->bottom - yDst;
    int         xSrc            = prcSrc->left;
    int         ySrc            = prcSrc->top;
    int         xSrcWid         = prcSrc->right - xSrc;
    int         ySrcHei         = prcSrc->bottom - ySrc;
    int         yUseHei         = _yHeight;

#if DBG == 1
    if (IsTagEnabled(tagIgnorePalette))
    {
        dwFlags &= ~DRAWIMAGE_NHPALETTE;
    }
    
    if (!IsTagEnabled(tagForceRgbBlt))
    {
        dwFlags |= DRAWIMAGE_NHPALETTE;
    }
#endif

    // Cases in which there is nothing to draw

    if ((!_pvImgBits && !_pvMaskBits && !_fSolid) || _yHeightValid == 0)
        return;
        
    if (    xDstWid <= 0 || xSrcWid <= 0 || _xWidth <= 0
        ||  yDstHei <= 0 || ySrcHei <= 0 || _yHeight <= 0)
        return;

    if (dwRop != SRCCOPY && (_pvMaskBits || _iTrans >= 0))
        return;

    // Step 1: Limit the source and dest rectangles to the visible area only.

    if (_yHeightValid > 0 && _yHeightValid < _yHeight)
        yUseHei = _yHeightValid;

    if (xSrc < 0)
    {
        xDst += MulDivQuick(-xSrc, xDstWid, xSrcWid);
        xDstWid = prcDst->right - xDst;
        xSrcWid += xSrc;
        xSrc = 0;        
        if (xDstWid <=0 || xSrcWid <= 0)
            return;
    }
    if (ySrc < 0)
    {
        yDst += MulDivQuick(-ySrc, yDstHei, ySrcHei);
        yDstHei = prcDst->bottom - yDst;
        ySrcHei += ySrc;
        ySrc = 0;
        if (yDstHei <=0 || ySrcHei <= 0)
            return;
    }
    if (xSrc + xSrcWid > _xWidth)
    {
        xDstWid = MulDivQuick(xDstWid, _xWidth - xSrc, xSrcWid);
        xSrcWid = _xWidth - xSrc;
        if (xDstWid <= 0 || xSrcWid <= 0)
            return;
    }
    if (ySrc + ySrcHei > yUseHei)
    {
        yDstHei = MulDivQuick(yDstHei, yUseHei - ySrc, ySrcHei);
        ySrcHei = yUseHei - ySrc;
        if (yDstHei <= 0 || ySrcHei <= 0)
            return;
    }
    // For the mirrored case, we need flip then offset.
    if(_fNeedMirroring)
    {
        // We need to handle clipping correctly and give a right-to-left tiling effect.
        // Let's take the "opposite" slice of the source.
        // The maximum will be the whole image.
        xSrc = - xSrc +_xWidth - xSrcWid;
        xDst += xDstWid - 1;
        xDstWid = - xDstWid;

    }    
    // Optimization: if solid, just patblt the color
    
    if (_fSolid)
    {
        // Turn on the palette relative bit for palettized devices in order to ensure that dithering
        // doesn't happen here.  The main reason is that is looks ugly, but more importantly we aren't
        // prepared to seam multiple copies of the image so that the dithering looks smooth.

        extern COLORREF g_crPaletteRelative;
        PatBltBrush(hdc, xDst, yDst, xDstWid, yDstHei, PATCOPY, _crSolid | g_crPaletteRelative);
        return;
    }

    SetStretchBltMode(hdc, COLORONCOLOR);

    // Step 2: For tranparent images, use mask to whiten drawing area

    if (_pvMaskBits || _iTrans >= 0)
    {
        if (dwFlags & DRAWIMAGE_NOTRANS)
            goto NoTransparency;
            
        if (GetDeviceCaps(hdc, TECHNOLOGY) == DT_RASPRINTER)
        {
            // No transparency for printers that we know lie about their support for transparency.
            
            int iEscapeFunction = POSTSCRIPT_PASSTHROUGH;
            THREADSTATE *   pts = GetThreadState();

            if (pts && (pts->dwPrintMode & PRINTMODE_NO_TRANSPARENCY)
                    || Escape(hdc, QUERYESCSUPPORT, sizeof(int), (LPCSTR) &iEscapeFunction, NULL))
            {
                // Skip transparency unless we are a mask-only image
                if (_pvImgBits || !_pvMaskBits)
                    goto NoTransparency;
            }
        }
    
        if (_pvMaskBits)
        {
            // 1-bit mask case

            if (_hbmMask)
            {
                // We have an HBITMAP, not just bits
                
                Assert(!hdcDib && !hbmSav);
                
                hdcDib = GetMemoryDC();
                if (!hdcDib)
                    goto Cleanup;

#ifdef _WIN64
//$ WIN64: MaskBlt is broken in build 2019.  Remove this when its working again.
#if DBG==1
                if (!IsTagEnabled(tagUseMaskBlt)) ; else
#else
                if (g_Zero.ach[0] == 0) ; else
#endif
#endif

                // Special case: use MaskBlt for the whole thing on NT
                if (g_dwPlatformID == VER_PLATFORM_WIN32_NT && 
                    xSrcWid == xDstWid && ySrcHei == yDstHei && _hbmImg)
                {
                    hbmSav = (HBITMAP)SelectObject(hdcDib, _hbmImg);

                    MaskBlt(hdc, xDst, yDst, xDstWid, yDstHei,
                         hdcDib, xSrc, ySrc, _hbmMask, xSrc, ySrc, 0xAACC0020);

                    goto Cleanup;
                }

                hbmSav = (HBITMAP)SelectObject(hdcDib, _hbmMask);
                
                if (!_pvImgBits)
                {
                    // a mask-only one-bit image: just draw the "1" bits as black
                    ::StretchBlt(hdc, xDst, yDst, xDstWid, yDstHei,
                            hdcDib, xSrc, ySrc, xSrcWid, ySrcHei, MERGEPAINT);
                }
                else
                {
                    // transparent mask: draw the "1" bits as white
                    ::StretchBlt(hdc, xDst, yDst, xDstWid, yDstHei,
                            hdcDib, xSrc, ySrc, xSrcWid, ySrcHei, SRCPAINT);
                }
            }
            else
            {
                // We have just bits, not an HBITMAP
                
                struct
                {
                    BITMAPINFOHEADER bmih;
                    union
                    {
                        RGBQUAD         rgb[2];
                        WORD            windex[2];
                    };
                } bmiMask;

                // construct mask header
                memset(&bmiMask, 0, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD) * 2);
                bmiMask.bmih.biSize = sizeof(BITMAPINFOHEADER);
                bmiMask.bmih.biWidth = _xWidth;
                bmiMask.bmih.biHeight = _yHeight;
                bmiMask.bmih.biPlanes = 1;
                bmiMask.bmih.biBitCount = 1;

                if (!_pvImgBits)
                {
                    // a mask-only one-bit image: just draw the "1" bits as black
                    bmiMask.rgb[0] = g_rgbWhite;
                    StretchDIBits(hdc, xDst, yDst, xDstWid, yDstHei,
                                       xSrc, _yHeight - ySrc - ySrcHei, xSrcWid, ySrcHei, _pvMaskBits, (BITMAPINFO *)&bmiMask, DIB_RGB_COLORS, SRCAND);
                }
                else if (_iBitCount <= 8 && _fPalColors && !(dwFlags & DRAWIMAGE_NHPALETTE))
                {
                    // this setup only occurs on an 8-bit palettized display; we can use DIB_PAL_COLORS
                    bmiMask.windex[1] = 255;
                    StretchDIBits(hdc, xDst, yDst, xDstWid, yDstHei,
                                       xSrc, _yHeight - ySrc - ySrcHei, xSrcWid, ySrcHei, _pvMaskBits, (BITMAPINFO *)&bmiMask, DIB_PAL_COLORS, SRCPAINT);
                }
                else
                {
#if DBG == 1
                    if (IsTagEnabled(tagAssertRgbBlt))
                    {
                        AssertSz(_iBitCount > 8 || !_fPalColors, "Using DIB_RGB_COLORS for mask on image dithered to halftone palette");
                    }
#endif

                    bmiMask.rgb[1] = g_rgbWhite;
                    StretchDIBits(hdc, xDst, yDst, xDstWid, yDstHei,
                                       xSrc, _yHeight - ySrc - ySrcHei, xSrcWid, ySrcHei, _pvMaskBits, (BITMAPINFO *)&bmiMask, DIB_RGB_COLORS, SRCPAINT);
                }
            }
        }
        else
        {
            // 1- 4- or 8-bit mask case (with _iTrans)
            long cTable = 1 << _iBitCount;
            
            Assert(_iTrans >= 0);
            Assert(_iTrans < cTable);
            Assert(_iBitCount <= 8);

            if (_hbmImg)
            {
                // We have an HBITMAP, not just bits
                
                RGBQUAD argbOld[256];
                RGBQUAD argbNew[256];
                
                memset(argbNew, 0, sizeof(RGBQUAD) * cTable);
                argbNew[_iTrans] = g_rgbWhite;
                
                Assert(!hdcDib && !hbmSav);

                hdcDib = GetMemoryDC();
                if (!hdcDib)
                    goto Cleanup;

                hbmSav = (HBITMAP)SelectObject(hdcDib, _hbmImg);

                // HBM case: we need to change the color table, which can only be done one-at-a time
                g_csImgTransBlt.Enter();

                Verify(GetDIBColorTable(hdcDib, 0, cTable, argbOld) > 0);
                Verify(SetDIBColorTable(hdcDib, 0, cTable, argbNew) == (unsigned)cTable);

                ::StretchBlt(hdc, xDst, yDst, xDstWid, yDstHei,
                        hdcDib, xSrc, ySrc, xSrcWid, ySrcHei, MERGEPAINT);
                        
                Verify(SetDIBColorTable(hdcDib, 0, cTable, argbOld) == (unsigned)cTable);

                g_csImgTransBlt.Leave();
            }
            else
            {
                // We have just bits, not an HBITMAP
                
                struct
                {
                    BITMAPINFOHEADER bmih;
                    RGBQUAD          rgb[256];
                } bmiMask;

                // construct mask header
                memset(&bmiMask, 0, sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * cTable));
                bmiMask.bmih.biSize = sizeof(BITMAPINFOHEADER);
                bmiMask.bmih.biWidth = _xWidth;
                bmiMask.bmih.biHeight = _yHeight;
                bmiMask.bmih.biPlanes = 1;
                bmiMask.bmih.biBitCount = _iBitCount;
                bmiMask.rgb[_iTrans] = g_rgbWhite;

                StretchDIBits(hdc, xDst, yDst, xDstWid, yDstHei,
                                   xSrc, _yHeight - ySrc - ySrcHei, xSrcWid, ySrcHei, _pvImgBits, (BITMAPINFO *)&bmiMask, DIB_RGB_COLORS, MERGEPAINT);
            }
        }
        
        // prepare for transparent blt: area to be painted is now whitened, so AND-blt on top of it
        dwRop = SRCAND;
    }

NoTransparency:

    // Step 3: Draw the image bits
    
    if (_pvImgBits)
    {
        if (dwFlags & DRAWIMAGE_MASKONLY)
            goto Cleanup;

        if (_hbmImg)
        {
            // We have an HBITMAP, not just bits
            
            if (g_dwPlatformID != VER_PLATFORM_WIN32_WINDOWS
                 || GetDeviceCaps(hdc, TECHNOLOGY) != DT_RASPRINTER)
            {
                // The normal case (not to a Win95 printer): call StretchBlt
                
                if (!hdcDib)
                {
                    hdcDib = GetMemoryDC();
                    if (!hdcDib)
                        goto Cleanup;
                }

                HBITMAP hbmOld;
                
                hbmOld = (HBITMAP)SelectObject(hdcDib, _hbmImg);
                if (!hbmSav)
                    hbmSav = hbmOld;

                ::StretchBlt(hdc, xDst, yDst, xDstWid, yDstHei,
                        hdcDib, xSrc, ySrc, xSrcWid, ySrcHei, dwRop);
            }
            else
            {
                // On a Win95 printer, we need to use StretchDIBits, so extract the bits
                
                DIBSECTION dsPrint;

                if (GetObject(_hbmImg, sizeof(DIBSECTION), &dsPrint))
                {
                    struct
                    {
                        BITMAPINFOHEADER bmih;
                        RGBQUAD argb[256];
                    } bmi;

                    bmi.bmih = dsPrint.dsBmih;
                    GetDIBColorTable(hdcDib, 0, 256, bmi.argb);

                    Assert(bmi.bmih.biHeight > 0);

                    StretchDIBits(hdc, xDst, yDst, xDstWid, yDstHei,
                                  xSrc, _yHeight - ySrc - ySrcHei, xSrcWid, ySrcHei,
                                  dsPrint.dsBm.bmBits, (BITMAPINFO *) &bmi, DIB_RGB_COLORS, dwRop);
                }
            }
        }
        else
        {
            // We have just bits, not an HBITMAP
            
            if (!_pbmih)
            {
                // No color table header: cobble up a standard header [perhaps these should just be globally cached?]
                struct
                {
                    BITMAPINFOHEADER bmih;
                    union
                    {
                        WORD             windex[256];
                        RGBQUAD          rgb[256];
                        DWORD            bfmask[3];
                    };
                } bmi;

                DWORD dwDibMode = DIB_RGB_COLORS;

                // construct mask header
                memset(&bmi, 0, sizeof(BITMAPINFOHEADER) + (_iBitCount > 8 ? sizeof(DWORD) * 3 : sizeof(WORD) * (_iBitCount << 1)));
                bmi.bmih.biSize = sizeof(BITMAPINFOHEADER);
                bmi.bmih.biWidth = _xWidth;
                bmi.bmih.biHeight = _yHeight;
                bmi.bmih.biPlanes = 1;
                bmi.bmih.biBitCount = _iBitCount + (_iBitCount == 15);
                
                if (_iBitCount == 4)
                {
                    // Thanks to Steve Palmer: fix VGA color rendering
                    
                    bmi.bmih.biClrUsed = 16;
                    bmi.bmih.biClrImportant = 16;
                    CopyColorsFromPaletteEntries(bmi.rgb, g_peVga, 16);
                }
                else if (_iBitCount <= 8)
                {
                    if (dwFlags & DRAWIMAGE_NHPALETTE)
                    {
                        // If being drawn on an <= 8-bit surface from a filter, we can make no assumptions about
                        // the selected palette, so use RGB_COLORS
#if DBG == 1
                        if (IsTagEnabled(tagAssertRgbBlt))
                        {
                            AssertSz(0, "Using DIB_RGB_COLORS for image dithered to halftone palette");
                        }
#endif
                        LONG c;

                        c = (1 << (_iBitCount - 1));
                        
                        memcpy(bmi.rgb, g_rgbHalftone, c * sizeof(RGBQUAD));
                        memcpy(bmi.rgb + c, g_rgbHalftone + 256 - c, c * sizeof(RGBQUAD));
                    }
                    else
                    {
                        // internal draw, no color table with _iBitCount <= 8 means that the palette selected into hdc
                        // is our standard 8-bit halftone palette and we can use DIB_PAL_COLORS
                        
                        LONG c;
                        LONG d;
                        WORD *pwi;

                        dwDibMode = DIB_PAL_COLORS;
                        
                        for (c = (1 << (_iBitCount - 1)), pwi = bmi.windex + c; c; *(--pwi) = (--c));
                        for (c = (1 << (_iBitCount - 1)), pwi = bmi.windex + c * 2, d = 256; c; --c, *(--pwi) = (--d));
                    }
                }
                else if (_iBitCount == 16)
                {
                    // sixteen-bit case: fill in the bitfields mask for 565
                    
                    bmi.bmih.biCompression = BI_BITFIELDS;
                    bmi.bfmask[0] = MASK565_0;
                    bmi.bfmask[1] = MASK565_1;
                    bmi.bfmask[2] = MASK565_2;
                }

                StretchDIBits(hdc, xDst, yDst, xDstWid, yDstHei,
                              xSrc, _yHeight - ySrc - ySrcHei, xSrcWid, ySrcHei, _pvImgBits, (BITMAPINFO *)&bmi, dwDibMode, dwRop);
            }
            else
            {
                StretchDIBits(hdc, xDst, yDst, xDstWid, yDstHei,
                              xSrc, _yHeight - ySrc - ySrcHei, xSrcWid, ySrcHei, _pvImgBits, (BITMAPINFO *)_pbmih, DIB_RGB_COLORS, dwRop);
            }
        }
    }

Cleanup:
    if (hbmSav)
        SelectObject(hdcDib, hbmSav);
    if (hdcDib)
        ReleaseMemoryDC(hdcDib);
}

void
CImgBitsDIB::Optimize()
{
    RGBQUAD rgbSolid;
    
    if (_iTrans >= 0 && !_pvMaskBits)
    {
        if (!!ComputeTransMask(0, _yHeightValid, _iTrans, _iTrans))
            return;
    }

    if (!_pvMaskBits)
    {
        // If we still don't have a mask, it means there were no transparent bits, so dump _iTrans

        _iTrans = -1;

        // check 8-bit images to see if they're solid

        if (_iBitCount == 8 && _pvImgBits)
        {
            DWORD dwTest;
            DWORD *pdw;
            int cdw;
            int cLines;

            dwTest = *(BYTE *)_pvImgBits;
            dwTest = dwTest | dwTest << 8;
            dwTest = dwTest | dwTest << 16;

            pdw = (DWORD *)_pvImgBits;
            
            for (cLines = _yHeight; cLines; cLines -= 1)
            {
                cdw = CbLine() / 4;

                for (;;)
                {
                    if (cdw == 1)
                    {
                        // Assumes little endian; shift in other direction for big endian

#ifndef BIG_ENDIAN                        
                        if ((dwTest ^ *pdw) << (8 * (3 - (0x3 & (_xWidth - 1)))))
#else
                        if ((dwTest ^ *pdw) >> (8 * (3 - (0x3 & (_xWidth - 1)))))
#endif
                            goto NotSolid;

                        pdw += 1;
                        break;
                    }
                    else if (*pdw != dwTest)
                        goto NotSolid;
                        
                    cdw -= 1;
                    pdw += 1;
                }
            }

            // It's solid! So extract the color
            dwTest &= 0xFF;
            
            if (_pbmih)
            {
                rgbSolid = ((RGBQUAD *)(_pbmih + 1))[dwTest];
            }
            else if (_fPalColors)
            {
                rgbSolid = g_rgbHalftone[dwTest];
            }
            else if (_hbmImg)
            {
                HDC hdcMem;
                HBITMAP hbmSav;
                
                hdcMem = GetMemoryDC();
                if (!hdcMem)
                    goto NotSolid;

                hbmSav = (HBITMAP) SelectObject(hdcMem, _hbmImg);
                GetDIBColorTable(hdcMem, dwTest, 1, &rgbSolid);
                SelectObject(hdcMem, hbmSav);
                ReleaseMemoryDC(hdcMem);
            }
            else
            {
                goto NotSolid;
            }

            // And set up the data

            _fSolid = TRUE;
            _crSolid = (rgbSolid.rgbRed) | (rgbSolid.rgbGreen << 8) | (rgbSolid.rgbBlue << 16);
            
            FreeMemory();
        }
    }
    else
    {
        // If we have a mask, check to see if the entire image is transparent; if so, dump the data
        
        int cdw;
        int cLines;
        DWORD *pdw;
        DWORD dwLast;
        BYTE bLast;

        Assert(!(CbLineMask() & 0x3));

        bLast = (0xFF << (7 - (0x7 & (_xWidth - 1))));

        // Assumes little endian; shift in other direction for big endian
#ifndef BIG_ENDIAN
        dwLast = (unsigned)(0x00FFFFFF | (bLast << 24)) >> (8 * (3 - (0x3 & (((7 + _xWidth) >> 3) - 1))));
#else
        dwLast = (unsigned)(0x00FFFFFF | (bLast << 24)) << (8 * (3 - (0x3 & (((7 + _xWidth) >> 3) - 1))));
#endif
        
        pdw = (DWORD *)_pvMaskBits;

        for (cLines = _yHeight; cLines; cLines -= 1)
        {
            cdw = CbLineMask() / 4;

            for (;;)
            {
                if (cdw == 1)
                {
                    // Assumes little endian; shift in other direction for big endian
                    
                    if (*pdw & dwLast)
                        goto NotAllTransparent;

                    pdw += 1;
                    break;
                }
                else if (*pdw)
                    goto NotAllTransparent;
                    
                cdw -= 1;
                pdw += 1;
            }
        }
        
        FreeMemory();
        return;
    }

NotSolid:
NotAllTransparent:
    ;    
}

HRESULT
CImgBitsDIB::ComputeTransMask(LONG yFirst, LONG cLines, LONG lTrans, LONG lReplace)
{
    DWORD *         pdw;
    DWORD           dwBits;
    BYTE *          pb;
    int             cb;
    int             cbPad;
    int             x, y, b;
    BYTE            bTrans;

    Assert(_iBitCount == 8);

    #if DBG==1
    if (IsTagEnabled(tagNoTransMask))
        return S_OK;
    #endif

    if (lTrans < 0)
    {
        Assert(!_pvMaskBits);
        return S_OK;
    }

    // negate coordinate system for DIBs

    yFirst = _yHeight - cLines - yFirst;
    
    // Step 1: scan for transparent bits: if none, there's nothing to do (yet)

    bTrans = lTrans;

    if (!_pvMaskBits)
    {
        pb    = (BYTE *)GetBits() + CbLine() * yFirst;
        cbPad = CbLine() - _xWidth;

        for (y = cLines; y-- > 0; pb += cbPad)
            for (x = _xWidth; x-- > 0; )
                if (*pb++ == bTrans)
                {
                    HRESULT hr = AllocMask();
                    if (hr)
                        RRETURN(hr);
                    goto trans;
                }

        return S_OK;
    }

trans:

    // Step 2: allocate and fill in the one-bit mask

    pdw   = (DWORD *)((BYTE *)GetBits() + CbLine() * yFirst);

    pb = (BYTE *)GetMaskBits() + CbLineMask() * yFirst;
    cbPad = CbLineMask() - (_xWidth + 7) / 8;

    for (y = cLines; y-- > 0; pb += cbPad)
    {
        for (x = _xWidth; x > 0; x -= 8)
        {
            dwBits = *pdw++; b = 0;
#ifdef UNIX
            b |= ((BYTE)(dwBits >> 24) != bTrans); b <<=1;
            b |= ((BYTE)(dwBits >> 16) != bTrans); b <<=1;
            b |= ((BYTE)(dwBits >> 8) != bTrans); b <<=1;
            b |= ((BYTE)dwBits != bTrans); b <<= 1;
#else
            b |= ((BYTE)dwBits != bTrans); b <<= 1; dwBits >>= 8;
            b |= ((BYTE)dwBits != bTrans); b <<= 1; dwBits >>= 8;
            b |= ((BYTE)dwBits != bTrans); b <<= 1; dwBits >>= 8;
            b |= ((BYTE)dwBits != bTrans); b <<= 1;
#endif
            if (x <= 4)
                b = (b << 3) | 0xF;
            else
            {
                dwBits = *pdw++;
#ifdef UNIX
                b |= ((BYTE)(dwBits >> 24) != bTrans); b <<= 1;
                b |= ((BYTE)(dwBits >> 16) != bTrans); b <<= 1;
                b |= ((BYTE)(dwBits >> 8 ) != bTrans); b <<= 1;
                b |= ((BYTE)dwBits != bTrans);
#else
                b |= ((BYTE)dwBits != bTrans); b <<= 1; dwBits >>= 8;
                b |= ((BYTE)dwBits != bTrans); b <<= 1; dwBits >>= 8;
                b |= ((BYTE)dwBits != bTrans); b <<= 1; dwBits >>= 8;
                b |= ((BYTE)dwBits != bTrans);
#endif
            }

            *pb++ = (BYTE)b;
        }
    }

    // Step 3: replace the transparent color

    if (lTrans != lReplace)
    {
        for (pb = (BYTE *)GetBits() + CbLine() * yFirst, cb = CbLine() * cLines; cb; pb += 1, cb -= 1)
        {
            if (*pb == bTrans)
                *pb = lReplace;
        }
    }

    return S_OK;
}

LONG CImgBitsDIB::CbTotal()
{
    LONG cb = sizeof(CImgBitsDIB);
    
    if (_pvImgBits)
        cb += CbLine() * _yHeight;
    if (_pvMaskBits)
        cb += CbLineMask() * _yHeight;

    if (cb < sizeof(CImgBitsDIB))
        return sizeof(CImgBitsDIB);

    return cb;
}



HRESULT CImgBitsDIB::SaveAsBmp(IStream * pStm, BOOL fFileHeader)
{
    HRESULT hr = S_OK;

    HBITMAP hbmSavDst = NULL;
    HDC hdcDibDst = NULL;
    int adjustedwidth;
    DIBSECTION ds;
    DWORD dw;
    int nColors;
    DWORD dwColors;
    DWORD dwImage;
    int cBitsPerPix;
    CImgBitsDIB *pibd = NULL;
    HBITMAP hbm;

    // 1. Get a DIBSECTION structure even if it means allocating another CImgBitsDIB

    hbm = _hbmImg;

    if (!hbm || 
        (g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS && (_iBitCount == 15 || _iBitCount == 16)) ||
        !GetObject(_hbmImg, sizeof(DIBSECTION), &ds))
    {
        RECT rc;
        int iBitCount;
        RGBQUAD *prgb;
        int nColors;

        if (g_dwPlatformID == VER_PLATFORM_WIN32_WINDOWS && (_iBitCount == 15 || _iBitCount == 16))
        {
            // On Win95, we always convert 15- and 16- bpp to 24-bpp because wallpaper can't handle 15/16.
            iBitCount = 24;
            prgb = NULL;
            nColors = 0;
        }
        else
        {
            // Otherwise, duplicate what we have.
            iBitCount = _iBitCount;
            if (_pbmih && iBitCount <= 8)
            {
                prgb = (RGBQUAD *)(_pbmih + 1);
                nColors = _pbmih->biClrUsed ? _pbmih->biClrUsed : (1 << iBitCount);
            }
            else if (iBitCount == 8)
            {
                prgb = g_rgbHalftone;
                nColors = 256;
            }
            else
            {
                prgb = NULL;
                nColors = 0;
            }
        }
        
        pibd = new CImgBitsDIB();
        if (!pibd)
            goto OutOfMemory;

        hr = THR(pibd->AllocDIBSection(iBitCount, _xWidth, _yHeight, prgb, nColors, _iTrans));
        if (hr)
            goto Cleanup;

        hbm = pibd->GetHbm();

        Assert(hbm);
        
        hdcDibDst = GetMemoryDC();
        if (!hdcDibDst)
            goto OutOfMemory;
            
        rc.left = 0;
        rc.top = 0;
        rc.right = _xWidth;
        rc.bottom = _yHeight;
        
        hbmSavDst = (HBITMAP) SelectObject(hdcDibDst, hbm);

        StretchBlt(hdcDibDst, &rc, &rc, SRCCOPY, DRAWIMAGE_NHPALETTE | DRAWIMAGE_NOTRANS);

        if (!GetObject(hbm, sizeof(DIBSECTION), &ds))
            goto OutOfMemory;
    }


    // 2. Save it out
    
    cBitsPerPix = ds.dsBmih.biBitCount;

    Assert(cBitsPerPix == 1 || cBitsPerPix == 4 ||
        cBitsPerPix == 8 || cBitsPerPix == 16 || cBitsPerPix == 24 || cBitsPerPix == 32);

    adjustedwidth = ((ds.dsBmih.biWidth * cBitsPerPix + 31) & ~31) / 8;

#ifdef UNIX
    // IEBUGBUG - this should be done in mainwin
    if ( ds.dsBmih.biClrUsed > 256 ) 
    {
        ds.dsBmih.biClrUsed = 0;
    }
#endif

    nColors = ds.dsBmih.biClrUsed;
    if ((nColors == 0) && (cBitsPerPix <= 8))
        nColors = 1 << cBitsPerPix;
    
    Assert(ds.dsBmih.biCompression != BI_BITFIELDS || nColors == 0);
    
    dwColors = nColors * sizeof(RGBQUAD) + (ds.dsBmih.biCompression == BI_BITFIELDS ? 3 * sizeof(DWORD) : 0);

    dwImage = ds.dsBmih.biHeight * adjustedwidth;

    if (fFileHeader)
    {
        BITMAPFILEHEADER bf;

        bf.bfType = 0x4D42; // "BM"
        bf.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + dwColors;
        bf.bfSize = bf.bfOffBits + dwImage;
        bf.bfReserved1 = 0;
        bf.bfReserved2 = 0;

        hr = pStm->Write(&bf, sizeof(bf), &dw);
        if (hr)
            goto Cleanup;
    }

    hr = pStm->Write(&(ds.dsBmih), sizeof(BITMAPINFOHEADER), &dw);
    if (hr)
        goto Cleanup;

    if (nColors > 0)
    {
        RGBQUAD argb[256];

        if (!hdcDibDst)
        {
            hdcDibDst = GetMemoryDC();
            if (!hdcDibDst)
                goto OutOfMemory;
                
            hbmSavDst = (HBITMAP) SelectObject(hdcDibDst, hbm);
        }

        GetDIBColorTable(hdcDibDst, 0, min(256, nColors), argb);

        hr = pStm->Write(argb, dwColors, &dw);
        if (hr)
            goto Cleanup;
    }
    else if (ds.dsBmih.biCompression == BI_BITFIELDS)
    {
        hr = pStm->Write(ds.dsBitfields, dwColors, &dw);
        if (hr)
            goto Cleanup;
    }

    hr = pStm->Write(ds.dsBm.bmBits, dwImage, &dw);

Cleanup:
    if (hbmSavDst)
        SelectObject(hdcDibDst, hbmSavDst);
    if (hdcDibDst)
        ReleaseMemoryDC(hdcDibDst);
    if (pibd)
        delete pibd;

    RRETURN(hr);

OutOfMemory:
    hr = E_OUTOFMEMORY;
    goto Cleanup;
}
