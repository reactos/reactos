//+------------------------------------------------------------------------
//
//  Microsoft Forms
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       imgwmf.cxx
//
//  Contents:   Image filter for .wmf files
//
//-------------------------------------------------------------------------

#include "headers.hxx"

#ifndef X_IMG_HXX_
#define X_IMG_HXX_
#include "img.hxx"
#endif

#ifndef X_IMGBITS_HXX_
#define X_IMGBITS_HXX_
#include "imgbits.hxx"
#endif

MtDefine(CImgTaskEmf, Dwn, "CImgTaskEmf")
MtDefine(CImgTaskEmfBuf, CImgTaskEmf, "CImgTaskEmf Decode Buffer")

class CImgTaskEmf : public CImgTask
{
    typedef CImgTask super;

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgTaskEmf))

    virtual void Decode(BOOL *pfNonProgressive);
};

void CImgTaskEmf::Decode(BOOL *pfNonProgressive)
{
    ULONG ulSize;
    LPBYTE pbBuf = NULL;
    HENHMETAFILE hmf = NULL;
    CImgBitsDIB *pibd = NULL;
    HDC hdcDst = NULL;
    HBITMAP hbmSav = NULL;
    RECT rc;
    ENHMETAHEADER    emh;
    UINT nColors;
    HRESULT hr;
    RGBQUAD argb[256];

    *pfNonProgressive = TRUE;

    // read in the header
    if (!Read(&emh, sizeof(emh)))
       return;

    _xWid = emh.rclBounds.right - emh.rclBounds.left;
    _yHei = emh.rclBounds.bottom - emh.rclBounds.top;
    
    // Post WHKNOWN
    OnSize(_xWid, _yHei, _lTrans);

    // allocate a buffer to hold metafile
    ulSize = emh.nBytes;
    pbBuf = (LPBYTE)MemAlloc(Mt(CImgTaskEmfBuf), ulSize);
    if (!pbBuf)
        return;

    // copy the header into the buffer
    memcpy(pbBuf, &emh, sizeof(emh));

    // read the metafile into memory after the header
    if (!Read(pbBuf + sizeof(emh), ulSize - sizeof(emh)))
        goto Cleanup;

    // convert the buffer into a metafile handle
    hmf = SetEnhMetaFileBits(ulSize, pbBuf);
    if (!hmf)
        goto Cleanup;

    // Free the metafile buffer
    MemFree(pbBuf);
    pbBuf = NULL;

    // Get the palette from the metafile if there is one, otherwise use the 
    // halftone palette.
    nColors = GetEnhMetaFilePaletteEntries(hmf, 256, _ape);
    if (nColors == 0)
    {
        memcpy(_ape, g_lpHalftone.ape, sizeof(_ape));
        nColors = 256;        
    }

    CopyColorsFromPaletteEntries(argb, _ape, nColors);

    pibd = new CImgBitsDIB();
    if (!pibd)
        goto Cleanup;

    hr = pibd->AllocDIBSection(8, _xWid, _yHei, argb, nColors, 255);
    if (hr)
        goto Cleanup;

    _lTrans = 255;

    Assert(pibd->GetBits() && pibd->GetHbm());

    memset(pibd->GetBits(), (BYTE)_lTrans, pibd->CbLine() * _yHei);

    // Render the metafile into the bitmap
    
    hdcDst = GetMemoryDC();
    if (!hdcDst)
        goto Cleanup;
        
    hbmSav = (HBITMAP)SelectObject(hdcDst, pibd->GetHbm());

    rc.left = rc.top = 0;
    rc.right = _xWid;
    rc.bottom = _yHei;
    PlayEnhMetaFile(hdcDst, hmf, &rc);

    _ySrcBot = -1;
    
    _pImgBits = pibd;
    pibd = NULL;

Cleanup:
    if (hbmSav)
        SelectObject(hdcDst, hbmSav);
    if (hdcDst)
        ReleaseMemoryDC(hdcDst);
    if (pibd)
        delete pibd;
    if (hmf)        
        DeleteEnhMetaFile(hmf);
    if (pbBuf)
        MemFree(pbBuf);
    return;
}

CImgTask * NewImgTaskEmf()
{
    return(new CImgTaskEmf);
}
