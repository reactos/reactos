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

MtDefine(CImgTaskWmf, Dwn, "CImgTaskWmf")
MtDefine(CImgTaskWmfBuf, CImgTaskWmf, "CImgTaskWmf Decode Buffer")

/* placeable metafile header */
#include "pshpack1.h"

#ifdef UNIX
inline WORD READWINTELWORD(BYTE* b)
{
    return ((*b) | ((WORD)*(b+1) << 8));
}

inline DWORD READWINTELDWORD(BYTE* p)
{
    return (READWINTELWORD(p+2) << 16 | READWINTELWORD(p));
}
#endif

typedef struct tagSRECT {
    short    left;
    short    top;
    short    right;
    short    bottom;
} SRECT;

typedef struct {
        DWORD   key;
        WORD    hmf;
        SRECT   bbox;
        WORD    inch;
        DWORD   reserved;
        WORD    checksum;
}ALDUSMFHEADER;
#include "poppack.h"

class CImgTaskWmf : public CImgTask
{

public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgTaskWmf))

    typedef CImgTask super;

    // CImgTask methods

    virtual void Decode(BOOL *pfNonProgressive);

    // CImgTaskWmf methods

    void ReadImage();

};

#ifdef UNIX
#define SIZEOF_METAHEADER (5*sizeof(WORD) + 2*sizeof(DWORD))
#endif

void CImgTaskWmf::ReadImage()
{
    METAHEADER mh;
    LPBYTE pbBuf = NULL;
    CImgBitsDIB *pibd = NULL;
    HDC hdcDst = NULL;
    HMETAFILE hmf = NULL;
    HBITMAP hbmSav = NULL;
    ULONG ulSize;
    HRESULT hr;
    RGBQUAD argb[256];

    // Get the metafile header so we know how big it is
#ifndef UNIX
    if (!Read((unsigned char *)&mh, sizeof(mh)))
        return;
#else
    METAHEADER mhTmp;
    if (!Read((unsigned char *)&mhTmp, SIZEOF_METAHEADER))
        return;

    BYTE * p = (BYTE*)&mhTmp;

    mh.mtType = READWINTELWORD(p );
    mh.mtHeaderSize = READWINTELWORD(p += sizeof(WORD));
    mh.mtVersion = READWINTELWORD( p += sizeof(WORD));
    mh.mtSize = READWINTELDWORD(p += sizeof(WORD));
    mh.mtNoObjects = READWINTELWORD( p += sizeof(DWORD));
    mh.mtMaxRecord = READWINTELDWORD(p += sizeof(WORD));
    mh.mtNoParameters = READWINTELWORD( p += sizeof(DWORD)); 
#endif

    // allocate a buffer to hold it
    ulSize = mh.mtSize * sizeof(WORD);
    pbBuf = (LPBYTE)MemAlloc(Mt(CImgTaskWmfBuf), ulSize);
    if (!pbBuf)
        return;

    // copy the header into the front of the buffer
#ifdef UNIX // Needs to use raw data
    memcpy(pbBuf, &mhTmp, SIZEOF_METAHEADER);
#else
    memcpy(pbBuf, &mh, sizeof(METAHEADER));
#endif
    
    // read the metafile into memory after the header
#ifndef UNIX
    if (!Read(pbBuf + sizeof(METAHEADER), 
            ulSize - sizeof(METAHEADER)))
#else
    if (!Read(pbBuf + SIZEOF_METAHEADER, 
            ulSize - SIZEOF_METAHEADER))
#endif
        goto Cleanup;

    // convert the buffer into a metafile handle
    hmf = SetMetaFileBitsEx(ulSize, pbBuf);
    if (!hmf)
        goto Cleanup;

    // Free the metafile buffer
    MemFree(pbBuf);
    pbBuf = NULL;

    // Use the halftone palette for the color table
    CopyColorsFromPaletteEntries(argb, g_lpHalftone.ape, 256);
    memcpy(_ape, g_lpHalftone.ape, sizeof(PALETTEENTRY) * 256);

    pibd = new CImgBitsDIB();
    if (!pibd)
        goto Cleanup;

    hr = pibd->AllocDIBSection(8, _xWid, _yHei, argb, 256, 255);
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

    SaveDC(hdcDst);

    SetMapMode(hdcDst, MM_ANISOTROPIC);
    SetViewportExtEx(hdcDst, _xWid, _yHei, (SIZE *)NULL);
    PlayMetaFile(hdcDst, hmf);

    RestoreDC(hdcDst, -1);

    _pImgBits = pibd;
    pibd = NULL;
        
    _ySrcBot = -1;

Cleanup:
    if (hbmSav)
        SelectObject(hdcDst, hbmSav);
    if (hdcDst)
        ReleaseMemoryDC(hdcDst);
    if (pibd)
        delete pibd;
    if (hmf)
        DeleteMetaFile(hmf);
    if (pbBuf)
        MemFree(pbBuf);
        
    return;
}

void CImgTaskWmf::Decode(BOOL *pfNonProgressive)
{
    ALDUSMFHEADER amfh;

    *pfNonProgressive = TRUE;

    // KENSY: What do I do about the color table?
    // KENSY: scale according to DPI of screen
#ifndef UNIX
    if (!Read((unsigned char *)&amfh, sizeof(amfh)))
        goto Cleanup;
#else
    {
    ALDUSMFHEADER amfhTmp;
    
    if (!Read((unsigned char *)&amfhTmp, 3*sizeof(WORD)+2*sizeof(DWORD)+sizeof(SRECT)))
        goto Cleanup;

    BYTE *p = (BYTE*)&amfhTmp;

    amfh.key =            READWINTELDWORD(p);
    amfh.hmf =            READWINTELWORD(p += sizeof(DWORD));
    amfh.bbox.left =   READWINTELWORD(p += sizeof(WORD));
    amfh.bbox.top =    READWINTELWORD( p += sizeof(WORD));
    amfh.bbox.right =  READWINTELWORD( p += sizeof(WORD));
    amfh.bbox.bottom = READWINTELWORD( p += sizeof(WORD));
    amfh.inch =           READWINTELWORD( p += sizeof(WORD));
    amfh.reserved =    READWINTELDWORD( p += sizeof(WORD));
    amfh.checksum =    READWINTELWORD( p += sizeof(DWORD));
    }
#endif

    _xWid = abs(MulDiv(amfh.bbox.right - amfh.bbox.left, 96, amfh.inch));
    _yHei = abs(MulDiv(amfh.bbox.bottom - amfh.bbox.top, 96, amfh.inch));
    
    // Post WHKNOWN
    OnSize(_xWid, _yHei, -1);

    ReadImage();

Cleanup:
    return;
}

CImgTask * NewImgTaskWmf()
{
    return(new CImgTaskWmf);
}
