//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       imgbmp.cxx
//
//  Contents:   Image filter for .bmp files
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

#ifdef UNIX
#ifndef X_DDRAW_H_
#define X_DDRAW_H_
#include "ddraw.h"
#endif
#endif

#ifndef WIN16
#ifndef X_IMGUTIL_H_
#define X_IMGUTIL_H_
#include "imgutil.h"
#endif
#endif

MtDefine(CImgTaskBmp, Dwn, "CImgTaskBmp")
MtDefine(CImgTaskBmpBmih, CImgTaskBmp, "CImgTaskBmp::_pbmih")
MtDefine(CImgTaskBmpRleBits, CImgTaskBmp, "CImgTaskBmp RLE Bits")

#ifdef BIG_ENDIAN
inline WORD READWINTELWORD(WORD w)
{
  return ( w << 8 | w >> 8 );
}

inline DWORD READWINTELDWORD(DWORD dw)
{
  return READWINTELWORD( (WORD)(dw >> 16 )) | ((DWORD)READWINTELWORD( dw & 0xffff)) << 16;
}
#endif

class CImgTaskBmp : public CImgTask
{
    typedef CImgTask super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgTaskBmp))

    virtual void        Decode(BOOL *pfNonProgressive);

    // Data members

    BITMAPFILEHEADER    _bmfh;
    BITMAPINFOHEADER *  _pbmih;
    RGBQUAD          *  _pargb;
    UINT                _ncolors;
    BYTE *              _pbBits;

};

void CImgTaskBmp::Decode(BOOL *pfNonProgressive)
{
    BYTE    *pbCompressedBits = NULL;
    HDC     hdcMem;
    HBITMAP hbmSav;
    HBITMAP hbm = NULL;
    BOOL    fCompatibleFormat = TRUE;
    BOOL    fSuccess = FALSE;
    DWORD   dwHeaderSize;
    BOOL    fDither = FALSE;
    BOOL    fCoreHeader = FALSE;
    CImgBitsDIB *pibd = NULL;
    LONG    cbRead = 0;
    HRESULT hr;
    
    *pfNonProgressive = TRUE;

#if !defined(UNIX) && !defined(_MAC)
    if (!Read((BYTE *)&_bmfh, sizeof(BITMAPFILEHEADER)))
#else
    // IEUNIX: Because of alignment problem, we need to read as following.
    if(!Read((BYTE *)&_bmfh.bfType, sizeof(WORD)) ||
        !Read((BYTE *)&_bmfh.bfSize, 3 * sizeof(DWORD)))
#endif
        goto Cleanup;

    cbRead += sizeof(BITMAPFILEHEADER);

#ifdef BIG_ENDIAN
    _bmfh.bfType = READWINTELWORD(_bmfh.bfType);
    _bmfh.bfSize = READWINTELDWORD(_bmfh.bfSize);
    _bmfh.bfReserved1 = READWINTELWORD(_bmfh.bfReserved1);
    _bmfh.bfReserved2 = READWINTELWORD(_bmfh.bfReserved2);
    _bmfh.bfOffBits = READWINTELDWORD(_bmfh.bfOffBits);
#endif

    // BITMAPINFOHEADER is a variable-length structure where the 
    // first field is the header size.  By reading this header and 
    // dynamically allocating the structure we will be compatible
    // with all forms of the header.  

    if (!Read((BYTE *)&dwHeaderSize, sizeof(DWORD)))
        goto Cleanup;

    cbRead += sizeof(DWORD);

#if defined(UNIX) || defined(_MAC)
    dwHeaderSize = READWINTELDWORD(dwHeaderSize);
#endif

    // Validate the header size

    if (dwHeaderSize < sizeof(BITMAPCOREHEADER))
        goto Cleanup;
    else if (dwHeaderSize == sizeof(BITMAPCOREHEADER))
    {
        fCoreHeader = TRUE;
        dwHeaderSize = sizeof(BITMAPINFOHEADER);
    }
    else if (dwHeaderSize < sizeof(BITMAPINFOHEADER))
        goto Cleanup;
    else if (dwHeaderSize > 4096) //arbitrary limit to guard bogus file
        goto Cleanup;
        
    _pbmih = (BITMAPINFOHEADER *)MemAlloc(Mt(CImgTaskBmpBmih), dwHeaderSize + 256 * sizeof(RGBQUAD));
    if (!_pbmih)
        goto Cleanup;
    _pargb = (RGBQUAD *)((BYTE *)_pbmih + dwHeaderSize);
    
    _pbmih->biSize = dwHeaderSize;

    if (!fCoreHeader)
    {
        if (!Read((BYTE *)_pbmih + sizeof(DWORD), dwHeaderSize - sizeof(DWORD)))
            goto Cleanup;
            
        cbRead += dwHeaderSize - sizeof(DWORD);
    }
    else
    {
        BITMAPCOREHEADER bcHeader;

        if (!Read((BYTE *)&bcHeader.bcWidth, sizeof(BITMAPCOREHEADER) - sizeof(DWORD)))
            goto Cleanup;
            
        cbRead += sizeof(BITMAPCOREHEADER) - sizeof(DWORD);

        _pbmih->biWidth = (LONG)bcHeader.bcWidth;
        _pbmih->biHeight = (LONG)bcHeader.bcHeight;
        _pbmih->biPlanes = bcHeader.bcPlanes;
        _pbmih->biBitCount = bcHeader.bcBitCount;
        _pbmih->biCompression = BI_RGB;
        _pbmih->biSizeImage = 0;
        _pbmih->biXPelsPerMeter = 0;
        _pbmih->biYPelsPerMeter = 0;
        _pbmih->biClrUsed = 0;
        _pbmih->biClrImportant = 0;
    }

#ifdef BIG_ENDIAN
    _pbmih->biWidth = (LONG)READWINTELDWORD(_pbmih->biWidth);
    _pbmih->biHeight = (LONG)READWINTELDWORD(_pbmih->biHeight);
    _pbmih->biPlanes = READWINTELWORD(_pbmih->biPlanes);
    _pbmih->biBitCount = READWINTELWORD(_pbmih->biBitCount);
    _pbmih->biCompression = READWINTELDWORD(_pbmih->biCompression);
    _pbmih->biSizeImage = READWINTELDWORD(_pbmih->biSizeImage);
    _pbmih->biXPelsPerMeter = (LONG)READWINTELDWORD(_pbmih->biXPelsPerMeter);
    _pbmih->biYPelsPerMeter = (LONG)READWINTELDWORD(_pbmih->biYPelsPerMeter);
    _pbmih->biClrUsed = READWINTELDWORD(_pbmih->biClrUsed);
    _pbmih->biClrImportant = READWINTELDWORD(_pbmih->biClrImportant);
#endif
    if (_pbmih->biPlanes != 1)
        goto Cleanup;

    if (    _pbmih->biBitCount != 1
        &&  _pbmih->biBitCount != 4
        &&  _pbmih->biBitCount != 8
        &&  _pbmih->biBitCount != 16
        &&  _pbmih->biBitCount != 24
        &&  _pbmih->biBitCount != 32)
        goto Cleanup;

    if (_pbmih->biClrUsed && _pbmih->biClrUsed <= 256)
    {
        _ncolors = _pbmih->biClrUsed;
    }
    else if (_pbmih->biBitCount <= 8)
    {
        _ncolors = 1 << _pbmih->biBitCount;
    }
    else if (_pbmih->biCompression == BI_BITFIELDS)
    {
        _ncolors = 3;       // RGB masks
    }

    if (_ncolors)
    {
        if (!fCoreHeader)
        {
            if (!Read((BYTE *)_pargb, _ncolors * sizeof(RGBQUAD)))
                goto Cleanup;
                
            cbRead += _ncolors * sizeof(RGBQUAD);
        }
        else
        {
            RGBTRIPLE   argbT[256];
            UINT i;

            if (!Read((BYTE *)argbT, _ncolors * sizeof(RGBTRIPLE)))
                goto Cleanup;
                
            cbRead += _ncolors * sizeof(RGBTRIPLE);
    
            for (i = 0; i < _ncolors; ++i)
            {
                _pargb[i].rgbRed = argbT[i].rgbtRed;
                _pargb[i].rgbGreen = argbT[i].rgbtGreen;
                _pargb[i].rgbBlue = argbT[i].rgbtBlue;
                _pargb[i].rgbReserved = 0;
            }
        }

        if (_pbmih->biCompression != BI_BITFIELDS)
            CopyPaletteEntriesFromColors(_ape, _pargb, _ncolors);
    }

    _xWid  = _pbmih->biWidth;
    _yHei = _pbmih->biHeight;

    // Post WHKNOWN
    OnSize(_xWid, _yHei, -1 /* lTrans */);

    // Determine if we are reading a format compatible with the rest
    // of Trident.  
    
    if (_colorMode == 8 && _pbmih->biBitCount == 8 && !_pImgInfo->TstFlags(DWNF_RAWIMAGE))
        fDither = TRUE;
    
    if (_pbmih->biCompression != BI_RGB 
        && _pbmih->biCompression != BI_BITFIELDS)
        fCompatibleFormat = FALSE;

    pibd = new CImgBitsDIB();
    if (!pibd)
        goto Cleanup;

    _pImgBits = pibd;

    if (fCompatibleFormat)
    {
        // for compatible formats: if there are extra fields we do not understand, chop them off.
        // n.b. NT5's BITMAPV5HEADERs point to extra "profile" data past the bitmap that we do not read.
        
        if (_pbmih->biSize > sizeof(BITMAPV4HEADER))
            _pbmih->biSize = sizeof(BITMAPV4HEADER);
        
        hr = THR(pibd->AllocDIBSectionFromInfo((BITMAPINFO *)_pbmih, fDither));
        if (hr)
            goto Cleanup;
    }
    else
    {
        hr = THR(pibd->AllocDIBSection((_pbmih->biBitCount == 16) ? 15 : _pbmih->biBitCount,
                                _xWid, _yHei, _pargb, _ncolors, -1));
        if (hr)
            goto Cleanup;
    }

    hbm = pibd->GetHbm();
    _pbBits = (BYTE *)pibd->GetBits();

    // Before we read the bits, seek to the correct location in the file
    while (_bmfh.bfOffBits > (unsigned)cbRead)
    {
        BYTE abDummy[1024];
        int cbSkip;

        cbSkip = _bmfh.bfOffBits - cbRead;
        
        if (cbSkip > 1024)
            cbSkip = 1024;

        if (!Read(abDummy, cbSkip))
            goto Cleanup;
            
        cbRead += cbSkip;
    }

    if (fCompatibleFormat)
    {
 #ifdef _MAC
        long    row;
        BYTE*   rowStart;
        int     cbRow = pibd->CbLine();

        for (row = _yHei - 1; row >= 0; row--)
        {
            rowStart = _pbBits + (cbRow * row);
            if (!Read(rowStart, cbRow))
                goto Cleanup;
        }
        pibd->ReleaseBits();
        _pbBits = nil;
        pibd->MapBits();
#else
        if (!Read(_pbBits, pibd->CbLine() * _yHei))
            goto Cleanup;
#endif
    }
    else
    {
        DIBSECTION  ds;
        LONG cbNeeded;

        cbNeeded = _pbmih->biSizeImage;

        // If the file continues beyond the end of the bitmap, read extra data in as well.
        
        if ((unsigned)cbNeeded < _bmfh.bfSize - cbRead)
            cbNeeded = _bmfh.bfSize - cbRead;

        Assert(_pbmih->biSizeImage > 0);
        if (_pbmih->biSizeImage == 0)
            goto Cleanup;
            
        pbCompressedBits = (BYTE *)MemAlloc(Mt(CImgTaskBmpRleBits), cbNeeded);
        if (!pbCompressedBits)
            goto Cleanup;

        if (!Read(pbCompressedBits, cbNeeded))
            goto Cleanup;
        
        hdcMem = GetMemoryDC();

        if (hdcMem == NULL)
            goto Cleanup;

        hbmSav = (HBITMAP)SelectObject(hdcMem, hbm);

        // BUGBUG - need to fill image with transparent info so that
        //          RLE jumps are handled correctly.
        
        SetDIBitsToDevice(hdcMem, 0, 0, _xWid, _yHei, 0, 0,
            0, _yHei, pbCompressedBits, (BITMAPINFO *)_pbmih, 
            DIB_RGB_COLORS);

        SelectObject(hdcMem, hbmSav);

        ReleaseMemoryDC(hdcMem);

        // Update the info header to be the uncompressed format
        // in hbm.  Update the color table to the halftone
        // entries (ImgCreateDib caused the color table to be
        // set to our halftone palette.)

        GetObject(hbm, sizeof(DIBSECTION), &ds);
        memcpy(_pbmih, &ds.dsBmih, sizeof(BITMAPINFOHEADER));

        // BUGBUG: Why is this not GetDibColorTable() ??????
        
        memcpy(_ape, g_lpHalftone.ape, _ncolors * sizeof(PALETTEENTRY));
    }

#ifndef _MAC
    if (fDither)
    {
        if (x_Dither(_pbBits, _ape, _xWid, _yHei, _lTrans))
            goto Cleanup;

        hdcMem = GetMemoryDC();

        if (hdcMem == NULL)
            goto Cleanup;

        hbmSav = (HBITMAP)SelectObject(hdcMem, hbm);

        SetDIBColorTable(hdcMem, 0, 256, g_rgbHalftone);

        SelectObject(hdcMem, hbmSav);

        ReleaseMemoryDC(hdcMem);
    }
#endif

    _ySrcBot = -1;
    _yBot = _yHei;
    
    fSuccess = TRUE;
    
Cleanup:
    MemFree(pbCompressedBits);
    MemFree(_pbmih);
    
    if (!fSuccess)
    {
        delete _pImgBits;
        _pImgBits = NULL;
        _pbBits = NULL;
    }
}

CImgTask * NewImgTaskBmp()
{
    return(new CImgTaskBmp);
}
