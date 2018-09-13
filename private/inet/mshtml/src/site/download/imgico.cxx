//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       imgico.cxx
//
//  Contents:   Image filter for .ico files
//
//  Created by: dli on 06/12/98
//
//-------------------------------------------------------------------------
#include "headers.hxx"

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

MtDefine(CImgTaskIcon, Dwn, "CImgTaskIcon")
MtDefine(CImgTaskIconBmih, CImgTaskIcon, "CImgTaskIcon::_pbmih")
MtDefine(CImgTaskIconJunk, CImgTaskIcon, "CImgTaskIcon::bJunk")

// The following data structures are standard icon file structures
typedef struct tagICONDIRENTRY
{
    BYTE    cx;
    BYTE    cy;
    BYTE    nColors;
    BYTE    iUnused;
    WORD    xHotSpot;
    WORD    yHotSpot;
    DWORD   cbDIB;
    DWORD   offsetDIB;
} ICONDIRENTRY;

typedef struct tagICONDIR
{
    WORD iReserved;
    WORD iResourceType;
    WORD cresIcons;
} ICONDIR;

// decoder for icons
class CImgTaskIcon : public CImgTask
{
    typedef CImgTask super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgTaskIcon))

    // CImgTask methods

    virtual void Decode(BOOL *pfNonProgressive);

    // Data members
    RGBQUAD          *  _pargb;
    BYTE *              _pbBitsXOR;
    BYTE *              _pbBitsAND;
    UINT                _ncolors;
    BITMAPINFOHEADER *  _pbmih;
};

void CImgTaskIcon::Decode(BOOL *pfNonProgressive)
{
    ICONDIR icondir = {0};
    ICONDIRENTRY icoentry = {0};
    BOOL    fDither = FALSE;
    BOOL    fSuccess = FALSE;
    HDC     hdcDib = NULL;
    HDC     hdcMask = NULL;
    int     iIco = 0;
    CImgBitsDIB *pibd = NULL;
    HRESULT hr;

    *pfNonProgressive = TRUE;

    // Read in the icon file header (ICONDIR)
#ifndef UNIX
    if (!Read((BYTE *)&icondir, sizeof(icondir)))
#else
    // IEUNIX: Because of alignment problem, we need to read as following.
    if(!Read((BYTE *)&icondir.iReserved, 3 * sizeof(WORD)))
#endif
        goto Cleanup;

    // If this is not a standard icon, bail
    if ((icondir.iReserved !=0) || (icondir.iResourceType != IMAGE_ICON) || (icondir.cresIcons < 1))
        goto Cleanup;

    // BUGBUG: add logic here to figure out which icon we should load, Trident
    // needs to communicate with us on the size and color depth of the icon intended
    // For now, load the first icon in the .ico file.
    
    //for (iIco = 0; iIco < icondir.cresIcons; iIco++)
    //{           
    if (!Read((BYTE *)&icoentry, sizeof(icoentry)))
        goto Cleanup;
    //if (iIco == 5)
    //break;
    //}
    
    // if ((icoentry.bWidth == width_we_want) && (icoentry.bColorCount == color_count_we_want))
    //     break;
    //}
    // if (iIco == icondir.cresIcons)
    //    goto Cleanup;


    _xWid = icoentry.cx;
    _yHei = icoentry.cy;

    // Post WHKNOWN
    OnSize(_xWid, _yHei, -1 /* lTrans */);

    // NOTE: This is just doing a seek. There is got to be a better way!!!!! 
    if (sizeof(icondir) + sizeof(icoentry) * (iIco+1) < icoentry.offsetDIB)
    {
        DWORD cbJunkSize = icoentry.offsetDIB - sizeof(icondir) - sizeof(icoentry) * (iIco+1);
        BYTE * bJunk = (BYTE *) MemAlloc(Mt(CImgTaskIconJunk), cbJunkSize);
        if ((!bJunk) || (!Read(bJunk, cbJunkSize)))
            goto Cleanup;
        MemFree(bJunk);
    }

    // allocate BITMAPINFO header and color table 
    _pbmih = (BITMAPINFOHEADER *)MemAlloc(Mt(CImgTaskIconBmih), sizeof(BITMAPINFOHEADER) + 256 * sizeof(RGBQUAD));
    if (!_pbmih)
        goto Cleanup;

    // point to the color table
    _pargb = (RGBQUAD *)((BYTE *)_pbmih + sizeof(BITMAPINFOHEADER));

    // read BITMAPINFOHEADER and make sure the size is correct
    if (!Read((BYTE *)_pbmih, sizeof(BITMAPINFOHEADER)) || (_pbmih->biSize != sizeof(BITMAPINFOHEADER)))
        goto Cleanup;

    // bail if it's not 1 plane
    if (_pbmih->biPlanes != 1)
        goto Cleanup;

    // check possible bit counts 
    if (    _pbmih->biBitCount != 1
        &&  _pbmih->biBitCount != 4
        &&  _pbmih->biBitCount != 8
        &&  _pbmih->biBitCount != 16
        &&  _pbmih->biBitCount != 24
        &&  _pbmih->biBitCount != 32)
        goto Cleanup;

    // number of colors
    _ncolors = 0;
    if (_pbmih->biBitCount <= 8)        
        _ncolors = 1 << _pbmih->biBitCount;

    if ((_ncolors > 0) && !Read((BYTE *)_pargb, _ncolors * sizeof(RGBQUAD)))
        goto Cleanup;

    if (_ncolors > 0)
        CopyPaletteEntriesFromColors(_ape, _pargb, _ncolors);
    
    if (_colorMode == 8 && _pbmih->biBitCount == 8 && !_pImgInfo->TstFlags(DWNF_RAWIMAGE))
        fDither = TRUE;

    // _pbmih->biHeight is the sum of heights of the two bitmaps
    // and in an ICO file the two bitmaps namely the color and mask
    // must have the same dimension so
    // _pbmih->biHeight must be twice of _yHei
    Assert(_pbmih->biHeight == 2 * _yHei);
    _pbmih->biHeight = _yHei;

    // create the CImgBitDib Object
    pibd = new CImgBitsDIB();
    if (!pibd)
        goto Cleanup;

    _pImgBits = pibd;
    hr = THR(pibd->AllocDIBSectionFromInfo((BITMAPINFO *)_pbmih, fDither));
    if (hr)
        goto Cleanup;

    _pbBitsXOR = (BYTE *)pibd->GetBits();
    
    // read in the XOR bitmap bits
    if (!Read(_pbBitsXOR, pibd->CbLine() * _yHei))
        goto Cleanup;

    hr = THR(pibd->AllocMask());
    if (hr)
        goto Cleanup;

    _pbBitsAND = (BYTE *)pibd->GetMaskBits();
    // read in the AND bits
    if (!Read(_pbBitsAND, pibd->CbLineMask() * _yHei))
        goto Cleanup;

    // make sure  the total bits we read in is the same as the total size
    // of this icon resource
    // BUGBUG: somehow the icoentry.cbDIB is always bigger ???
    // Assert((DWORD)((cbRowXOR+cbRowAND) * _yHei + sizeof(ICONDIRENTRY) +
    //               _ncolors * sizeof(RGBQUAD)) == icoentry.cbDIB);
    
    // Convert hbmXOR and hbmAND to format Trident recognize 
    hdcDib = CreateCompatibleDC(NULL);
    if (hdcDib)
    {
        hdcMask = CreateCompatibleDC(NULL);
        if (hdcMask)
        {
            HBITMAP hDibOld = SelectBitmap(hdcDib, pibd->GetHbm());
            HBITMAP hMaskOld = SelectBitmap(hdcMask, pibd->GetHbmMask());
            
            // OR _hbmMask onto _hbmDib.  This will make the transparent pixels white.
            BitBlt(hdcDib, 0, 0, _xWid, _yHei, hdcMask, 0, 0, SRCPAINT);

            // invert _hbmMask (0->1, 1->0) so that 0 pixels are transparent and 1 pixels are opaque
            BitBlt(hdcMask, 0, 0, _xWid, _yHei, hdcMask, 0, 0, DSTINVERT);
            
            SelectBitmap(hdcDib, hDibOld);
            SelectBitmap(hdcMask, hMaskOld);
        }
    }

    if (fDither)
    {
        HDC hdcMem;
        HBITMAP hbmSav;
        
        if (x_Dither(_pbBitsXOR, _ape, _xWid, _yHei, _lTrans))
            goto Cleanup;

        hdcMem = GetMemoryDC();

        if (hdcMem == NULL)
            goto Cleanup;

        hbmSav = SelectBitmap(hdcMem, pibd->GetHbm());

        SetDIBColorTable(hdcMem, 0, 256, g_rgbHalftone);

        SelectObject(hdcMem, hbmSav);

        ReleaseMemoryDC(hdcMem);
    }

    _ySrcBot = -1;

    fSuccess = TRUE;

Cleanup:
    if (hdcDib)
        DeleteDC(hdcDib);

    if (hdcMask)
        DeleteDC(hdcMask);

    MemFree(_pbmih);
    
    if (!fSuccess)
    {
        delete _pImgBits;
        _pImgBits = NULL;
        _pbBitsXOR = NULL;
        _pbBitsAND = NULL;
    }

}

CImgTask * NewImgTaskIco()
{
    return(new CImgTaskIcon);
}

#if DBG!=1
#pragma optimize(SPEED_OPTIMIZE_FLAGS, on)
#endif

