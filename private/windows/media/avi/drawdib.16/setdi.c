/**************************************************************************
*
*   SETDI.C - contains routines for doing a SetDIBits() into a bitmap.
*
**************************************************************************/

#include <windows.h>
#include <windowsx.h>
#include "lockbm.h"
#include "setdi.h"

#define NAKED

/**************************************************************************
*
*  format conversion functions.
*
*  special functions....
*      copy_8_8    (no translate)
*      dither_8_8  (dither from 8bpp to fixed color device (like VGA, SVGA...)
*
**************************************************************************/

extern CONVERTPROC copy_8_8,      dither_8_8;
extern CONVERTPROC convert_8_8,   convert_8_16,    convert_8_24,    convert_8_32,    convert_8_VGA,  convert_8_565,   convert_8_RGB,   convert_8_RGBX;
extern CONVERTPROC convert_16_8,  convert_16_16,   convert_16_24,   convert_16_32,   convert_16_VGA, convert_16_565,  convert_16_RGB,  convert_16_RGBX;
extern CONVERTPROC convert_24_8,  convert_24_16,   convert_24_24,   convert_24_32,   convert_24_VGA, convert_24_565,  convert_24_RGB,  convert_24_RGBX;
extern CONVERTPROC convert_32_8,  convert_32_16,   convert_32_24,   convert_32_32,   convert_32_VGA, convert_32_565,  convert_32_RGB,  convert_32_RGBX;

static INITPROC init_8_8,   init_8_16,    init_8_24,    init_8_32,    init_8_VGA,  init_8_565,   init_8_RGB,   init_8_RGBX;
static INITPROC init_16_8,  init_16_16,   init_16_24,   init_16_32,   init_16_VGA, init_16_565,  init_16_RGB,  init_16_RGBX;
static INITPROC init_24_8,  init_24_16,   init_24_24,   init_24_32,   init_24_VGA, init_24_565,  init_24_RGB,  init_24_RGBX;
static INITPROC init_32_8,  init_32_16,   init_32_24,   init_32_32,   init_32_VGA, init_32_565,  init_32_RGB,  init_32_RGBX;

       INITPROC    init_setdi;
static CONVERTPROC convert_setdi;

static FREEPROC free_common;

static LPVOID init_dither_8_8(HDC hdc, LPBITMAPINFOHEADER lpbi);

/**************************************************************************
*
*  some conversions we dont do
*
**************************************************************************/

#define convert_8_VGA   NULL
#define convert_16_VGA  NULL
#define convert_24_VGA  NULL
#define convert_32_VGA  NULL

#define convert_8_32   NULL
#define convert_16_32  NULL
#define convert_24_32  NULL
#define convert_32_32  NULL

#define convert_8_RGBX   NULL
#define convert_16_RGBX  NULL
#define convert_24_RGBX  NULL
#define convert_32_RGBX  NULL

#define convert_8_RGB    NULL
#define convert_16_RGB   NULL
#define convert_24_RGB   NULL
#define convert_32_RGB   NULL

#define convert_16_8  NULL      // not now later!
#define convert_24_8  NULL
#define convert_32_8  NULL

/**************************************************************************
*
*  format conversion tables...
*
*  BITMAP types
*
*  8       0
*  16      1
*  24      2
*  32      3
*  VGA     4
*  16 565  5
*  24 RGB  6
*  32 RGB  7
*
**************************************************************************/

static PCONVERTPROC  ConvertProcTable[4][8] = {
    {convert_8_8,   convert_8_16,    convert_8_24,    convert_8_32,    convert_8_VGA,  convert_8_565,   convert_8_RGB,   convert_8_RGBX},
    {convert_16_8,  convert_16_16,   convert_16_24,   convert_16_32,   convert_16_VGA, convert_16_565,  convert_16_RGB,  convert_16_RGBX},
    {convert_24_8,  convert_24_16,   convert_24_24,   convert_24_32,   convert_24_VGA, convert_24_565,  convert_24_RGB,  convert_24_RGBX},
    {convert_32_8,  convert_32_16,   convert_32_24,   convert_32_32,   convert_32_VGA, convert_32_565,  convert_32_RGB,  convert_32_RGBX},
};

static PINITPROC  InitProcTable[4][8] = {
    {init_8_8,   init_8_16,    init_8_24,    init_8_32,    init_8_VGA,  init_8_565,   init_8_RGB,   init_8_RGBX},
    {init_16_8,  init_16_16,   init_16_24,   init_16_32,   init_16_VGA, init_16_565,  init_16_RGB,  init_16_RGBX},
    {init_24_8,  init_24_16,   init_24_24,   init_24_32,   init_24_VGA, init_24_565,  init_24_RGB,  init_24_RGBX},
    {init_32_8,  init_32_16,   init_32_24,   init_32_32,   init_32_VGA, init_32_565,  init_32_RGB,  init_32_RGBX},
};

/**************************************************************************
**************************************************************************/

#define RGB555(r,g,b) (\
            (((WORD)(r) >> 3) << 10) |  \
            (((WORD)(g) >> 3) << 5)  |  \
            (((WORD)(b) >> 3) << 0)  )

#define RGB565(r,g,b) (\
            (((WORD)(r) >> 3) << 11) |  \
            (((WORD)(g) >> 2) << 5)  |  \
            (((WORD)(b) >> 3) << 0)  )

/**************************************************************************
**************************************************************************/

#ifdef DEBUG
static
#else
__inline
#endif

LONG BitmapXY(IBITMAP *pbm, int x, int y)
{
    LONG offset = pbm->bmOffset;

//!!! wrong!!! but y for bitmaps is always zero....
//  if (pbm->bmFillBytes)
//      offset += (y / pbm->bmScanSegment) * pbm->bmFillBytes;

    offset += y * (long)pbm->bmNextScan;
    offset += x * pbm->bmBitsPixel / 8;

    return offset;
}

/**************************************************************************
* @doc INTERNAL SetBitmapBegin
*
* @api BOOL | SetBitmapBegin | prepare to do a SetDIBits() into a bitmap
*
* @rdesc Returns TRUE if success.
*
**************************************************************************/

BOOL FAR SetBitmapBegin(
    PSETDI   psd,
    HDC      hdc,               //
    HBITMAP  hbm,               //  bitmap to set into
    LPBITMAPINFOHEADER lpbi,    //  --> BITMAPINFO of source
    UINT     DibUsage)
{
    BITMAP bm;

    SetBitmapEnd(psd);  // free and old stuff

    GetObject(hbm, sizeof(bm), &bm);

    psd->hbm     = hbm;
//  psd->hdc     = hdc;
//  psd->hpal    = hpal;
    psd->DibUsage= DibUsage;

    psd->color_convert = NULL;
    psd->convert = NULL;
    psd->size = sizeof(SETDI);

    if (!GetBitmapDIB(lpbi, NULL, &psd->bmSrc, sizeof(psd->bmSrc)))
        return FALSE;

    //
    // make sure we can lock the bitmap
    //
    if (GetBitmap(hbm, &psd->bmDst, sizeof(psd->bmDst)) &&
        psd->bmDst.bmFillBytes <= 0 &&
        psd->bmSrc.bmType > 0 && psd->bmSrc.bmType <= 4 &&
        psd->bmDst.bmType > 0 && psd->bmDst.bmType <= 8)
    {
        psd->init    = InitProcTable[psd->bmSrc.bmType-1][psd->bmDst.bmType-1];
        psd->convert = ConvertProcTable[psd->bmSrc.bmType-1][psd->bmDst.bmType-1];
        psd->free    = free_common;
    }

    //
    // if we cant convert ourself try SetDIBits()
    //
    if (psd->convert == NULL)
    {
        psd->convert = convert_setdi;
        psd->init    = init_setdi;
        psd->free    = NULL;
    }

    if (psd->init)
    {
        psd->hdc = hdc;
        if (!psd->init(psd))
        {
            psd->hdc = 0;
            psd->size = 0;
            psd->convert = NULL;
            return FALSE;
        }
        psd->hdc  = NULL;
        psd->hpal = NULL;
    }

    return TRUE;
}

/**************************************************************************
* @doc INTERNAL SetBitmapColorChange
*
* @api BOOL | SetBitmapColorChange | re-init the color conversion
*
* @rdesc Returns TRUE if success.
*
**************************************************************************/

void FAR SetBitmapColorChange(PSETDI psd, HDC hdc, HPALETTE hpal)
{
    if (psd->size != sizeof(SETDI))
        return;

    if (hdc == NULL)
        return;

    if (psd->free)              //!!! ack?
        psd->free(psd);

    psd->hdc  = hdc;
    psd->hpal = hpal;

    if (psd->init)
        psd->init(psd);

    psd->hdc  = NULL;
    psd->hpal = NULL;
}

/**************************************************************************
* @doc INTERNAL SetBitmapEnd
*
* @api void | SetBitmapEnd | clean out a SETDI structure
*
**************************************************************************/

void FAR SetBitmapEnd(PSETDI psd)
{
    if (psd->size != sizeof(SETDI))
        return;

    if (psd->free)
        psd->free(psd);

    psd->size = 0;
    psd->convert = NULL;
    psd->init = NULL;
    psd->free = NULL;
}

/**************************************************************************
* @doc INTERNAL SetBitmap
*
* @api BOOL | SetBitmap | convert DIB bits to bitmaps bits.
*
**************************************************************************/

BOOL FAR SetBitmap(PSETDI psd, int DstX, int DstY, int DstDX, int DstDY, LPVOID lpBits, int SrcX, int SrcY, int SrcDX, int SrcDY)
{
    if (psd->size != sizeof(SETDI))
        return FALSE;

    psd->convert(
        psd->bmDst.bmBits,                  // --> dst.
        BitmapXY(&psd->bmDst, DstX, DstY),  // offset to start at
        psd->bmDst.bmNextScan,              // dst_next_scan.
        psd->bmDst.bmFillBytes,             // fill bytes
        lpBits,                             // --> Src.
        BitmapXY(&psd->bmSrc, SrcX, SrcY),  // offset to start at
        psd->bmSrc.bmNextScan,              // Src_next_scan.
        DstDX,
        DstDY,
        psd->color_convert);

    return TRUE;
}

/**************************************************************************
*
*   cleanup stuff
*
**************************************************************************/

static BOOL free_common(PSETDI psd)
{
    //
    // clean up what we did
    //
    if (psd->color_convert != NULL)
        GlobalFreePtr(psd->color_convert);

    psd->color_convert = NULL;

    return TRUE;
}

/**************************************************************************
*
* GetPaletteTranslate
*
*   get the palette to physical translate table.
*
*   does this by calling GDI, this should always work.
*   this only works on a palette device.
*
**************************************************************************/

BOOL GetPaletteTranslate(HDC hdc, LPBYTE pb)
{
    int  i;
    int  n;
    DWORD rgb;
    DWORD *prgb;

    prgb = (DWORD *)LocalAlloc(LPTR, 256 * sizeof(DWORD));

    if (prgb == NULL)
	return TRUE;

    GetSystemPaletteEntries(hdc, 0, 256,(PALETTEENTRY FAR *)prgb);

    for (n=0; n<256; n++)           //!!! is this needed.
	prgb[n] &= 0x00FFFFFF;

    for (i=0; i<256; i++)
    {
        //
        // GDI will figure out what physical color this palette
        // index is mapped to.
        //
        rgb = GetNearestColor(hdc, PALETTEINDEX(i)) & 0x00FFFFFF;

        //
        // quick check for identity map.
        //
	if (prgb[i] == rgb)
        {
            pb[i] = (BYTE)i;
            continue;
        }

        //
        // now we have to find the rgb in the physical palette
        //
        for (n=0; n<256; n++)
	    if (prgb[n] == rgb)
                break;

        //
        // our search should never fail, because GDI gave us a RGB
        // in the palette.
        //
        if (n == 256)   //!!! should never happen
            n = 0;

        pb[i] = (BYTE)n;
    }

    LocalFree((HLOCAL)prgb);

    return TRUE;
}

/**************************************************************************
*
* @doc INTERNAL GetPaletteMap
*
* @api BOOL | GetPhysPaletteMap | gets the physical mapping for a DIB
*
* returns TRUE if the mapping is a 1:1 mapping, FALSE otherwise
*
**************************************************************************/

BOOL GetPhysDibPaletteMap(HDC hdc, LPBITMAPINFOHEADER lpbi, UINT Usage, LPBYTE pb)
{
    int i;
    int n;
    BYTE ab[256];

    //
    // this will give us the palette to physical mapping.
    //
    GetPaletteTranslate(hdc, ab);

    if (Usage == DIB_PAL_COLORS)
    {
        WORD FAR *pw = (WORD FAR *)(lpbi+1);

        for (i=0; i<256; i++)
            pb[i] = ab[pw[i]];
    }
    else
    {
        ;   //!!! should never happen with current code
    }

    //
    // test for 1:1 translate
    //
    n = (int)lpbi->biClrUsed ? (int)lpbi->biClrUsed : 256;

    for (i=0; i<n; i++)
    {
        if (pb[i] != i)
        {
            //
            // some ET4000 drivers have the same color (128,128,128)
            // at index 7 and at index 248.
            //
            // we should detect a identity palette in this case.
            //
            if (i == 248 && pb[i] == 7)
            {
                pb[i] = 248;
                continue;
            }
            break;
        }
    }

    return i == n;
}

/**************************************************************************
*
* @doc INTERNAL
*
* @api void | GetDibPaletteMap | gets the mapping of a DIB color table
* in  foreground palette index's
*
**************************************************************************/

BOOL GetDibPaletteMap(HDC hdc, LPBITMAPINFOHEADER lpbi, UINT Usage, LPBYTE pb)
{
    HBITMAP hbm;
    int i;
    int n;

    LONG biWidth = lpbi->biWidth;
    LONG biHeight = lpbi->biHeight;

    n = (int)lpbi->biClrUsed ? (int)lpbi->biClrUsed : 256;

    for (i=0; i<n; i++)
        pb[i] = i;

    for (; i<256; i++)
        pb[i] = 0;

    if (lpbi->biBitCount != 8)
        return FALSE;

    hbm = CreateCompatibleBitmap(hdc,256,1);

    lpbi->biWidth  = 256;
    lpbi->biHeight = 1;

    SetDIBits(hdc, hbm, 0, 1, pb, (LPBITMAPINFO)lpbi, Usage);
    GetBitmapBits(hbm, 256, pb);
    DeleteObject(hbm);

    lpbi->biWidth  = biWidth;
    lpbi->biHeight = biHeight;

    //
    // test for 1:1 translate
    //
    for (i=0; i<n; i++)
    {
        if (pb[i] != i)
        {
            //
            // some ET4000 drivers have the same color (128,128,128)
            // at index 7 and at index 248.
            //
            // we should detect a identity palette in this case.
            //
            if (i == 248 && pb[i] == 7)
            {
                pb[i] = 248;
                continue;
            }
            break;
        }
    }

    return i == n;
}

/**************************************************************************
*
*   convert for SetDIBits
*
**************************************************************************/

void FAR PASCAL convert_setdi(
    LPVOID pd,      // --> dst.
    LONG   dd,      // offset to start at
    LONG   nd,      // dst_next_scan.
    LONG   fd,      // dst fill bytes
    LPVOID ps,      // --> source.
    LONG   ds,      // offset to start at
    LONG   ns,      // src_next_scan.
    LONG   dx,      // pixel count.
    LONG   dy,      // scan count.
    LPVOID pc)      // pixel convert table.
{
    PSETDI psd = (PSETDI)(LONG)pd;
    LPBITMAPINFOHEADER lpbi;

    lpbi = (LPBITMAPINFOHEADER)psd->bmSrc.bmBitmapInfo;

    lpbi->biHeight = dy;

    SetDIBits(
        psd->hdc,
        psd->hbm,
        0,(int)dy,
        ((BYTE _huge *)ps) + ds - dd,
        (LPBITMAPINFO)lpbi,
        psd->DibUsage);

    lpbi->biHeight = psd->bmSrc.bmHeight;
}

/**************************************************************************
*
*   init stuff for SetDIBits
*
**************************************************************************/

BOOL init_setdi(PSETDI psd)
{
    UINT u;
    HDC  hdc;
    LPBYTE p;
    LPBITMAPINFOHEADER lpbi;

    // test to see if SetDIBits() works.
    // !!! we should check for 16 or a 32bit DIB and do the escape.
    // !!! on a palette device we need to build a palette map!!!

    if (psd->bmSrc.bmBitsPixel == 16 ||
        psd->bmSrc.bmBitsPixel == 32)
        return FALSE;

    // convert_setdi will need this.
    psd->bmDst.bmBits = (LPVOID)(UINT)psd;
    psd->bmDst.bmOffset = 0;
    psd->bmDst.bmBitsPixel = psd->bmSrc.bmBitsPixel;

    if (psd->hdc && psd->hpal)
    {
        // map colors to current palette!!!!!!!!!!!!!!!!!!!!!!!!!!

        //set this to be the BITMAPINFO + color map.
        psd->color_convert = 0;
    }

    lpbi = (LPBITMAPINFOHEADER)psd->bmSrc.bmBitmapInfo;
    lpbi->biHeight = 1;

    p = (LPBYTE)GlobalAllocPtr(GHND,psd->bmSrc.bmWidthBytes);

    hdc = GetDC(NULL);

    u = SetDIBits(
        hdc,
        psd->hbm,0,1,p,
        (LPBITMAPINFO)psd->bmSrc.bmBitmapInfo,
        psd->DibUsage);

    ReleaseDC(NULL, hdc);

    lpbi->biHeight = psd->bmSrc.bmHeight;
    GlobalFreePtr(p);

    return u == 1;
}

/**************************************************************************
*
*   init stuff for 8bpp bitmaps
*
**************************************************************************/

static BOOL init_8_8(PSETDI psd)
{
    LPBITMAPINFOHEADER lpbi;

    //
    //  if we are mapping from one DIB to another figure this out
    //
    if (psd->hdc == NULL || psd->bmDst.bmBitmapInfo != 0)
    {
        // we assume this routine will not be used for  color matching
        // from DIB to DIB, so give up.

        psd->convert = copy_8_8;
        return TRUE;
    }

    //
    // we are mapping to a device (HDC)
    //
    // we need to compute a 8-->8 conversion table, from the source colors
    // (in psd->lpbiSrc) to the colors on the device.
    //
    // how we do this depends on weather the device is a palette device or not.
    //

    lpbi = (LPBITMAPINFOHEADER)psd->bmSrc.bmBitmapInfo;

    if (GetDeviceCaps(psd->hdc, RASTERCAPS) & RC_PALETTE)
    {
        if (psd->hpal == NULL)
        {
            // no palette to match to yet
            psd->convert = copy_8_8;
            return TRUE;
        }

        if (psd->color_convert == NULL)
            psd->color_convert = GlobalAllocPtr(GHND, 256);

        //
        //  we can do this one of two ways,
        //
        //  we can always convert to the palette foreground mapping, or
        //
        //  we can convert to the current colors always (using this method
        //  we will need to recompute the xlat table on every palette
        //  change)
        //
        //  lets convert to the current device colors. (this may cause
        //  problems we will check on later...)
        //
#ifdef NAKED
        if (GetPhysDibPaletteMap(psd->hdc, lpbi, psd->DibUsage, psd->color_convert))
#else
        if (GetDibPaletteMap(psd->hdc, lpbi, psd->DibUsage, psd->color_convert))
#endif
            psd->convert = copy_8_8;
        else
            psd->convert = convert_8_8;
    }
    else
    {
        // !!!we should check for solid colors (ie no dither needed) and also
        // check for 1:1 (no translate)

        if (psd->color_convert == NULL)     //!!!
            psd->color_convert = init_dither_8_8(psd->hdc, lpbi);

        psd->convert = dither_8_8;

        //!!! we need to give the device colors to the caller
    }

    return TRUE;
}

static BOOL init_16_8(PSETDI psd)
{
    return FALSE;       // we dont handle dither yet!
}

static BOOL init_24_8(PSETDI psd)
{
    return FALSE;       // we dont handle dither yet!
}

static BOOL init_32_8(PSETDI psd)
{
    return FALSE;       // we dont handle dither yet!
}

/**************************************************************************
*
*   init stuff for 16bpp bitmaps
*
**************************************************************************/

static BOOL init_8_16(PSETDI psd)
{
    WORD FAR*pw;
    int i;
    int n;
    LPRGBQUAD prgb;
    LPBITMAPINFOHEADER lpbi;

    lpbi = (LPBITMAPINFOHEADER)psd->bmSrc.bmBitmapInfo;

    if (psd->color_convert == NULL)     //!!!
        psd->color_convert = GlobalAllocPtr(GHND, 256*2);

    n = (lpbi->biClrUsed == 0) ? 256 : (int)lpbi->biClrUsed;
    prgb = (LPRGBQUAD)((LPBYTE)lpbi + (int)lpbi->biSize);
    pw = psd->color_convert;

    for (i=0; i<n; i++)
        pw[i] = RGB555(prgb[i].rgbRed, prgb[i].rgbGreen, prgb[i].rgbBlue);

    for (; i<256; i++)
        pw[i] = 0;

    return TRUE;
}

static BOOL init_16_16(PSETDI psd)
{
    return TRUE;
}

static BOOL init_24_16(PSETDI psd)
{
    return TRUE;
}

static BOOL init_32_16(PSETDI psd)
{
    return TRUE;
}

/**************************************************************************
*
*   init stuff for 24bpp bitmaps
*
**************************************************************************/

static BOOL init_8_24(PSETDI psd)
{
    DWORD FAR*pd;
    int i;
    int n;
    LPRGBQUAD prgb;
    LPBITMAPINFOHEADER lpbi;

    lpbi = (LPBITMAPINFOHEADER)psd->bmSrc.bmBitmapInfo;

    if (psd->color_convert == NULL)     //!!!
        psd->color_convert = GlobalAllocPtr(GHND, 256*4);

    n = (lpbi->biClrUsed == 0) ? 256 : (int)lpbi->biClrUsed;
    prgb = (LPRGBQUAD)((LPBYTE)lpbi + (int)lpbi->biSize);
    pd = psd->color_convert;

    for (i=0; i<n; i++)
        pd[i] = RGB(prgb[i].rgbBlue, prgb[i].rgbGreen, prgb[i].rgbRed);

    for (; i<256; i++)
        pd[i] = 0;

    return TRUE;
}

static BOOL init_16_24(PSETDI psd)
{
    return TRUE;
}

static BOOL init_24_24(PSETDI psd)
{
    return TRUE;
}

static BOOL init_32_24(PSETDI psd)
{
    return TRUE;
}

/**************************************************************************
*
*   init stuff for 32bpp bitmaps
*
**************************************************************************/

static BOOL init_8_32(PSETDI psd)
{
    return FALSE;
////return init_8_24(psd);
}

static BOOL init_16_32(PSETDI psd)
{
    return FALSE;
}

static BOOL init_24_32(PSETDI psd)
{
    return FALSE;
}

static BOOL init_32_32(PSETDI psd)
{
    return FALSE;
}

/**************************************************************************
*
*   init stuff for VGA bitmaps
*
**************************************************************************/

static BOOL init_8_VGA(PSETDI psd)
{
    return FALSE;
}

static BOOL init_16_VGA(PSETDI psd)
{
    return FALSE;
}

static BOOL init_24_VGA(PSETDI psd)
{
    return FALSE;
}

static BOOL init_32_VGA(PSETDI psd)
{
    return FALSE;
}

/**************************************************************************
*
*   init stuff for RGB 565 bitmaps
*
**************************************************************************/

static BOOL init_8_565(PSETDI psd)
{
    WORD FAR*pw;
    int i;
    int n;
    LPRGBQUAD prgb;
    LPBITMAPINFOHEADER lpbi;

    lpbi = (LPBITMAPINFOHEADER)psd->bmSrc.bmBitmapInfo;

    if (psd->color_convert == NULL)     //!!!
        psd->color_convert = GlobalAllocPtr(GHND, 256*2);

    n = (lpbi->biClrUsed == 0) ? 256 : (int)lpbi->biClrUsed;
    prgb = (LPRGBQUAD)((LPBYTE)lpbi + (int)lpbi->biSize);
    pw = psd->color_convert;

    for (i=0; i<n; i++)
        pw[i] = RGB565(prgb[i].rgbRed, prgb[i].rgbGreen, prgb[i].rgbBlue);

    for (; i<256; i++)
        pw[i] = 0;

    return TRUE;
}

static BOOL init_16_565(PSETDI psd)
{
    return TRUE;
}

static BOOL init_24_565(PSETDI psd)
{
    return TRUE;
}

static BOOL init_32_565(PSETDI psd)
{
    return TRUE;
}

/**************************************************************************
*
*   init stuff for RGB 24bpp bitmaps
*
**************************************************************************/

static BOOL init_8_RGB(PSETDI psd)
{
    DWORD FAR *pd;
    int i;
    int n;
    LPRGBQUAD prgb;
    LPBITMAPINFOHEADER lpbi;

    lpbi = (LPBITMAPINFOHEADER)psd->bmSrc.bmBitmapInfo;

    if (psd->color_convert == NULL)     //!!!
        psd->color_convert = GlobalAllocPtr(GHND, 256*4);

    n = (lpbi->biClrUsed == 0) ? 256 : (int)lpbi->biClrUsed;
    prgb = (LPRGBQUAD)((LPBYTE)lpbi + (int)lpbi->biSize);
    pd = psd->color_convert;

    for (i=0; i<n; i++)
        pd[i] = RGB(prgb[i].rgbRed, prgb[i].rgbGreen, prgb[i].rgbBlue);

    for (; i<256; i++)
        pd[i] = 0;

    return TRUE;
}

static BOOL init_16_RGB(PSETDI psd)
{
    return FALSE;
}

static BOOL init_24_RGB(PSETDI psd)
{
    return FALSE;
}

static BOOL init_32_RGB(PSETDI psd)
{
    return FALSE;
}

/**************************************************************************
*
*   init stuff for RGB 32bpp bitmaps
*
**************************************************************************/

static BOOL init_8_RGBX(PSETDI psd)
{
    return init_8_RGB(psd);
}

static BOOL init_16_RGBX(PSETDI psd)
{
    return FALSE;
}

static BOOL init_24_RGBX(PSETDI psd)
{
    return FALSE;
}

static BOOL init_32_RGBX(PSETDI psd)
{
    return FALSE;
}

/**************************************************************************
*
*  init_dither_8_8
*
*  initialize a dither table that maps a 8 bit color to the device's dither
*
*  pel = dither_table[y&7][pel][x&7]
*
**************************************************************************/

static LPVOID init_dither_8_8(HDC hdc, LPBITMAPINFOHEADER lpbi)
{
    HBRUSH   hbr;
    HDC      hdcMem;
//  HDC      hdc;
    HBITMAP  hbm;
    HBITMAP  hbmT;
    int      i;
    int      nColors;
    LPRGBQUAD prgb;
    LPVOID   lpDitherTable;

    struct {
        BITMAPINFOHEADER bi;
        RGBQUAD rgb[256];
    }   dib;

    lpDitherTable = GlobalAllocPtr(GHND, 256*8*8);

    if (lpDitherTable == NULL)
        return (LPVOID)-1;

    hdc = GetDC(NULL);
    hdcMem = CreateCompatibleDC(hdc);

    hbm = CreateCompatibleBitmap(hdc, 256*8, 8);
    hbmT = SelectObject(hdcMem, hbm);

    if ((nColors = (int)lpbi->biClrUsed) == 0)
        nColors = 1 << (int)lpbi->biBitCount;

    prgb = (LPRGBQUAD)(lpbi+1);

    for (i=0; i<nColors; i++)
    {
        hbr = CreateSolidBrush(RGB(prgb[i].rgbRed,prgb[i].rgbGreen,prgb[i].rgbBlue));
        hbr = SelectObject(hdcMem, hbr);
        PatBlt(hdcMem, i*8, 0, 8, 8, PATCOPY);
        hbr = SelectObject(hdcMem, hbr);
        DeleteObject(hbr);
    }

#ifdef XDEBUG
    for (i=0; i<16; i++)
        BitBlt(hdc,0,i*8,16*8,8,hdcMem,i*(16*8),0,SRCCOPY);
#endif

    dib.bi.biSize           = sizeof(BITMAPINFOHEADER);
    dib.bi.biPlanes         = 1;
    dib.bi.biBitCount       = 8;
    dib.bi.biWidth          = 256*8;
    dib.bi.biHeight         = 8;
    dib.bi.biCompression    = BI_RGB;
    dib.bi.biSizeImage      = 256*8*8;
    dib.bi.biXPelsPerMeter  = 0;
    dib.bi.biYPelsPerMeter  = 0;
    dib.bi.biClrUsed        = 0;
    dib.bi.biClrImportant   = 0;
    GetDIBits(hdc, hbm, 0, 8, lpDitherTable, (LPBITMAPINFO)&dib, DIB_RGB_COLORS);

    SelectObject(hdcMem, hbmT);
    DeleteDC(hdcMem);
    DeleteObject(hbm);
    ReleaseDC(NULL, hdc);

    return (LPVOID)lpDitherTable;
}

#ifdef WIN32 // Provide some dummy entry points as a temporary measure for NT
void FAR PASCAL convert_16_16
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL convert_16_24
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL convert_16_565
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL convert_24_16
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL convert_24_24
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL convert_24_565
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL convert_32_16
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL convert_32_24
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL convert_32_565
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL convert_8_16
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL convert_8_24
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL convert_8_565
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL convert_8_8
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL copy_8_8
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
void FAR PASCAL dither_8_8
       (LPVOID pd,              // --> dst.
        LONG   dd,              // offset to start at
        LONG   nd,              // dst_next_scan.
        LONG   fd,              // dst fill bytes
        LPVOID ps,              // --> source.
        LONG   ds,              // offset to start at
        LONG   ns,              // src_next_scan.
        LONG   dx,              // pixel count.
        LONG   dy,              // scan count.
        LPVOID pc)              // pixel convert table.
{
    return;
}
#endif
