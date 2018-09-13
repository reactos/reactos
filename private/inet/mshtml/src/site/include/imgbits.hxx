#ifndef I_IMGBITS_HXX_
#define I_IMGBITS_HXX_
#pragma INCMSG("--- Beg 'imgbits.hxx'")

MtExtern(CImgBitsDIB);

#ifdef _MAC
class CICCProfile;
#include "pixmap.h"
#define MAXCOLORMAPSIZE     256
#define LPPIXMAP CPixMap*
#define LPPROFILE CICCProfile*
#endif

class CImgBits
{
public:
    virtual ~CImgBits() {};
    
    virtual void StretchBlt(HDC hdc, RECT *prcDest, RECT *prcSrc, DWORD dwROP, DWORD dwFlags) = 0;
    virtual LONG CbTotal() = 0;
    virtual void Optimize() = 0;
    virtual HRESULT SaveAsBmp(IStream * pStm, BOOL fFileHeader) = 0;
    virtual void *GetBits() { return NULL; }
    virtual BOOL IsTransparent() { return TRUE; }
    
    void StretchBltOffset(HDC hdc, RECT * prcDst, RECT * prcSrc, LONG xOffset, LONG yOffset, DWORD dwRop, DWORD dwFlags);
    
    long Width() { return _xWidth; }
    long Height() { return _yHeight; }
    
    virtual long Depth() = 0;
    virtual long Colors() = 0;
    virtual void GetColors(long iColorFirst, long cColors, RGBQUAD *prgb) = 0;

protected:
    long _xWidth;
    long _yHeight;
};


#define STACK 0.0

class CImgBitsDIB : public CImgBits
{
public:

    DECLARE_MEMCLEAR_NEW_DELETE(Mt(CImgBitsDIB));

    CImgBitsDIB() {};
    CImgBitsDIB(float f);
    virtual ~CImgBitsDIB() { FreeMemoryNoClear(); }
    
    // From CImgBits
    
    virtual void StretchBlt(HDC hdc, RECT *prcDest, RECT *prcSrc, DWORD dwROP, DWORD dwFlags);
    virtual HRESULT SaveAsBmp(IStream * pStm, BOOL fFileHeader);
    virtual void Optimize();
    virtual LONG CbTotal();
    
    // DIB specific functions : when building a bitmap
    // (1) allocate the dib to the right size
    // (2) fill in the Dib header and color table, and bits
    // (3) for transparency, allocate a mask and fill in the bits
    //     or specify a tranparent color
    // (4) when partially drawable, SetValidLines to set
    //     the number of raster lines that are drawable
    // (5) when done, to speed up the DIB, halftone the colors
    //     so that they'll be converted to match the device
    //     palette

    HRESULT AllocDIB(LONG iBitCount, LONG xWidth, LONG yHeight, RGBQUAD *argbTable, LONG cTable, LONG lTrans, BOOL fOpaque);
    HRESULT AllocDIBSection(LONG iBitCount, LONG xWidth, LONG yHeight, RGBQUAD *argbTable, LONG cTable, LONG lTrans);
    HRESULT AllocDIBSectionFromInfo(BITMAPINFO * pbmi, BOOL fPal);
    HRESULT AllocCopyBitmap(HBITMAP hbm, BOOL fPalColors, LONG lIndex);

    void FreeMemoryNoClear();
    void FreeMemory();

    virtual void *GetBits()
    {
        Assert(_pvImgBits);
        return _pvImgBits;
    }
    virtual long Depth()
    {
        return _iBitCount;
    }
    virtual long Colors()
    {
        return _cColors;
    }
    virtual void GetColors(long iFirst, long cColors, RGBQUAD *prgb);
    
    virtual BOOL IsTransparent()
    {
        return _pvMaskBits || _iTrans >= 0;
    }
    
    HBITMAP GetHbm()
    {
        Assert(_hbmImg);
        return _hbmImg;
    }
    HBITMAP GetHbmMask()
    {
        Assert(_hbmMask);
        return _hbmMask;
    }
    void *GetMaskBits()
    {
        return _pvMaskBits;
    }
    LONG CbLine()
    {
        return(((_xWidth * (_iBitCount == 15 ? 16 : _iBitCount) + 31) & ~31) / 8);
    }
    LONG CbLineMask()
    {
        return(((_xWidth + 31) & ~31) / 8);
    }
    LONG TransIndex()
    {
        return _iTrans;
    }

    void SetMirrorStatus (BOOL bMirrorStatus)
    {
      _fNeedMirroring = bMirrorStatus;
    }
    void SetTransIndex(LONG lIndex);
    void SetValidLines(LONG yLines);

    HRESULT AllocMask();
    HRESULT AllocMaskOnly(LONG _xWid, LONG _yHei);
    
    HRESULT ComputeTransMask(LONG yFirst, LONG cLines, LONG lIndex, LONG lReplace);

#ifdef _MAC
    void ReleaseBits()
    {
        ((CPixMap*)_pvImgBits)->Unlock();
    }
    void ReleaseMaskBits()
    {
        ((CPixMap*)_pvMaskBits)->Unlock();
    }
    HRESULT MapBits();
    void ApplyProfile(LPPROFILE Profile);
#endif

private:
    HRESULT AllocMaskSection();
    
    void *_pvImgBits;
    BITMAPINFOHEADER *_pbmih; // NULL unless custom color table is attached
    
    LONG _yHeightValid;
    LONG _iBitCount;
    LONG _cColors;
    
    void *_pvMaskBits;

    HBITMAP _hbmImg;    // DIBsections, used on NT only so we can use MaskBlt
    HBITMAP _hbmMask;

    COLORREF _crSolid;
    
    short _iTrans;
    BOOL _fPalColors     : 1;
    BOOL _fSolid         : 1;
    BOOL _fNeedMirroring : 1;
#ifdef _MAC
    char    _IndexMap[MAXCOLORMAPSIZE];     //  indexes shifted colormap
    Boolean     fHasColorTable;
#endif
};


#pragma INCMSG("--- End 'imgbits.hxx'")
#else
#pragma INCMSG("*** Dup 'imgbits.hxx'")
#endif

