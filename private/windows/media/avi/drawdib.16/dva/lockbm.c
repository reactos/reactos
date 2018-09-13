#include <windows.h>
#include <windowsx.h>
#include "lockbm.h"

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

#ifndef BI_BITFIELDS
    #define BI_BITFIELDS 3
#endif

#ifndef BI_BITMAP
    #define BI_BITMAP   0x4D544942      // 'BITM'
#endif

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

//
//  GDI!GDIInit2()      GDI.403
//
//  this GDI function does the following:
//
//      GetSetBitmapHandle(hbm, 0)  - will return global handle of bitmap
//
//      GetSetBitmapHandle(hbm, h)  - will set global handle to <h>
//
//      GetSetBitmapHandle(hbm, -1) - will set global handle to NULL
//
static HANDLE (FAR PASCAL *GetSetBitmapHandle)(HBITMAP hbm, HANDLE h);

//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
#define muldiv(a,b,c) (UINT)(((DWORD)(UINT)(a) * (DWORD)(UINT)(b)) / (UINT)(c))

//////////////////////////////////////////////////////////////////////////////
//
// CanLockBitmaps()
//
// determime if we can lock bitmaps on the current display device
//
//////////////////////////////////////////////////////////////////////////////
BOOL FAR CanLockBitmaps(void)
{
    UINT w;
    UINT rc;
    HDC  hdc;
    BOOL f;

    static BOOL fCanLockBitmaps = -1;

    if (fCanLockBitmaps == -1)
    {
        w = (UINT)GetVersion();

        w = ((UINT)LOBYTE(w) << 8) | HIBYTE(w);

        hdc = GetDC(NULL);
        rc = GetDeviceCaps(hdc, RASTERCAPS);
        ReleaseDC(NULL, hdc);

        (FARPROC)GetSetBitmapHandle =
            GetProcAddress(GetModuleHandle("GDI"),MAKEINTATOM(403));

        //
        // assume we dont need this on windows 4.0?
        //
        // what about the DIBENG? it does DEVBITS and in win 4.0?
        //
        // if the display handles device bitmaps, dont do this either
        //

        f = GetProfileInt("DrawDib", "Bitmaps", TRUE);

#ifdef DEBUG
        fCanLockBitmaps = f && GetSetBitmapHandle != NULL;
#else
        fCanLockBitmaps = f && /* (w < 0x0400) && */
                          !(rc & RC_DEVBITS) &&
                          GetSetBitmapHandle != NULL;
#endif
    }

    return fCanLockBitmaps;
}

//////////////////////////////////////////////////////////////////////////////
//
// LockBitmap
//
// return a pointer to the bitmap bits
//
//////////////////////////////////////////////////////////////////////////////

LPVOID FAR LockBitmap(HBITMAP hbm)
{
    return GetBitmap(hbm, NULL, 0);
}

//////////////////////////////////////////////////////////////////////////////
//
// GetBitmapDIB
//
//////////////////////////////////////////////////////////////////////////////

LPVOID FAR GetBitmapDIB(LPBITMAPINFOHEADER lpbi, LPVOID lpBits, LPVOID p, int cb)
{
    IBITMAP FAR *pbm;

    if (lpBits == NULL)
        lpBits = (LPBYTE)lpbi + (int)lpbi->biSize + lpbi->biClrUsed * sizeof(RGBQUAD);

    if (p == NULL || cb < sizeof(BITMAP))
        return lpBits;

    pbm = p;

    if (lpbi->biCompression == 0)
    {
        switch ((int)lpbi->biBitCount + (int)lpbi->biPlanes*256)
        {
            case 0x0101: pbm->bmType = BM_1BIT;  break;
            case 0x0104: pbm->bmType = BM_4BIT;  break;
            case 0x0108: pbm->bmType = BM_8BIT;  break;
            case 0x0110: pbm->bmType = BM_16555; break;
            case 0x0118: pbm->bmType = BM_24BGR; break;
            case 0x0120: pbm->bmType = BM_32BGR; break;
            case 0x0401: pbm->bmType = BM_VGA;   break;
            default: return NULL;
        }
    }
    else if (lpbi->biCompression == BI_BITFIELDS)
    {
        switch ((int)lpbi->biBitCount + (int)lpbi->biPlanes*256)
        {
            //!!! hack: realy should check the bit fields!
            case 0x0110: pbm->bmType = BM_16565; break;
            case 0x0118: pbm->bmType = BM_24RGB; break;
            case 0x0120: pbm->bmType = BM_32RGB; break;
            default: return NULL;
        }
    }
    else
        return NULL;

    pbm->bmWidth        = (int)lpbi->biWidth;
    pbm->bmHeight       = ((int)lpbi->biHeight > 0) ? (int)lpbi->biHeight : -(int)lpbi->biHeight;
    pbm->bmWidthBytes   = (((int)lpbi->biBitCount * (int)lpbi->biWidth + 31)&~31)/8;
    pbm->bmPlanes       = (BYTE)lpbi->biPlanes;
    pbm->bmBitsPixel    = (BYTE)lpbi->biBitCount;
    pbm->bmBits         = lpBits;

    if (cb > sizeof(BITMAP))
    {
        pbm->bmSegmentIndex = 0;
        pbm->bmScanSegment  = pbm->bmHeight;
        pbm->bmFillBytes    = 0;
        pbm->bmBitmapInfo   = (long)lpbi;

        if ((long)lpbi->biHeight < 0)
        {
            pbm->bmNextScan = -pbm->bmWidthBytes;
            pbm->bmOffset   = (long)pbm->bmWidthBytes * (pbm->bmHeight-1);
        }
        else
        {
            pbm->bmNextScan = pbm->bmWidthBytes;
            pbm->bmOffset   = 0;
        }
    }

    return lpBits;
}

#if 0
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

void FAR BitmapXY(IBITMAP FAR *pbm, int x, int y)
{
    UINT t;

    if (pbm->bmFillBytes)
    {
        while (y-- > 0)
        {
            t = (UINT)(pbm->bmOffset & 0xFFFF0000);
            pbm->bmOffset += pbm->bmNextScan;
            if ((UINT)(pbm->bmOffset & 0xFFFF0000) != t)
                pbm->bmOffset += pbm->bmFillBytes;
        }
    }
    else
    {
        pbm->bmOffset += y * (long)pbm->bmNextScan;
    }

    pbm->bmOffset += x * pbm->bmBitsPixel / 8;
}
#endif

//////////////////////////////////////////////////////////////////////////////
//
// GetDIBBitmap
//
//////////////////////////////////////////////////////////////////////////////

LPVOID FAR GetDIBBitmap(HBITMAP hbm, LPBITMAPINFOHEADER lpbi)
{
    UINT wType;
    BITMAP bm;
    UINT ScansPerSeg;
    UINT FillBytes;

    if (hbm)
        GetObject(hbm, sizeof(bm), &bm);

    wType = GetBitmapType();

    if (wType == 0)
        return NULL;

    lpbi->biSize           = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth          = bm.bmWidth;
    lpbi->biHeight         = bm.bmHeight;
    lpbi->biPlanes         = bm.bmPlanes;
    lpbi->biBitCount       = bm.bmBitsPixel;
    lpbi->biCompression    = 0;
    lpbi->biSizeImage      = (DWORD)(bm.bmWidthBytes * bm.bmPlanes) * (DWORD)bm.bmHeight;
    lpbi->biXPelsPerMeter  = 0;
    lpbi->biYPelsPerMeter  = 0;
    lpbi->biClrUsed        = 0;
    lpbi->biClrImportant   = 0;

    switch(wType & BM_TYPE)
    {
        case BM_VGA:
            break;

        case BM_1BIT:
        case BM_4BIT:
        case BM_8BIT:
            break;

        case BM_16555:
            break;

        case BM_24BGR:
        case BM_32BGR:
            break;

        case BM_16565:
            lpbi->biCompression = BI_BITFIELDS;
            ((LPDWORD)(lpbi+1))[0] = 0x00F800;
            ((LPDWORD)(lpbi+1))[1] = 0x0007E0;
            ((LPDWORD)(lpbi+1))[2] = 0x00001F;
            break;

        case BM_24RGB:
        case BM_32RGB:
            lpbi->biCompression = BI_BITFIELDS;
            ((LPDWORD)(lpbi+1))[0] = 0x0000FF;
            ((LPDWORD)(lpbi+1))[1] = 0x00FF00;
            ((LPDWORD)(lpbi+1))[2] = 0xFF0000;
            break;

        default:
            return NULL;
    }

    //
    //  make sure WidthBytes is right, dont forget bitmaps are WORD aligned
    //  and DIBs are DWORD aligned.
    //
    if (bm.bmWidthBytes != ((bm.bmWidth * bm.bmBitsPixel + 31) & ~31)/8)
    {
        if (lpbi->biCompression != 0)
            return NULL;

        lpbi->biCompression = BI_BITMAP;
        lpbi->biXPelsPerMeter  = bm.bmWidthBytes;
    }

    if ((wType & BM_HUGE) && (lpbi->biSizeImage > 64*1024l))
    {
        if (lpbi->biCompression == BI_BITFIELDS)
            return NULL;

        lpbi->biCompression = BI_BITMAP;

        ScansPerSeg = muldiv(64,1024,bm.bmWidthBytes * bm.bmPlanes);
        FillBytes   = (UINT)(64ul*1024 - bm.bmWidthBytes * bm.bmPlanes * ScansPerSeg);

        lpbi->biSizeImage     += FillBytes * (bm.bmHeight / ScansPerSeg);
        lpbi->biXPelsPerMeter  = bm.bmWidthBytes;
        lpbi->biYPelsPerMeter  = FillBytes;
    }

    if (!(wType & BM_BOTTOMTOTOP))
        lpbi->biHeight = -bm.bmHeight;

    return LockBitmap(hbm);
}

//////////////////////////////////////////////////////////////////////////////
//
// GetBitmap
//
//////////////////////////////////////////////////////////////////////////////

LPVOID FAR GetBitmap(HBITMAP hbm, LPVOID p, int cb)
{
    HANDLE h;
    DWORD  dwSize;
    IBITMAP FAR *pbm;
    HDC hdc = NULL;
    HBITMAP hbmT;

    if (!CanLockBitmaps())
        return NULL;

    if (hbm == NULL)
        return NULL;

    h = GetSetBitmapHandle(hbm, 0);

    if (h == NULL)
        return NULL;

    pbm = (LPVOID)GlobalLock(h);

    if (IsBadReadPtr(pbm, sizeof(IBITMAP)))
        return NULL;

    //
    // see if it is realy a bitmap.
    //
    if (pbm->bmType != 0)
        return NULL;

    //
    // make sure the bmBits pointer is valid.
    //
    if (pbm->bmBits == NULL)
    {
        hdc = CreateCompatibleDC(NULL);
        hbmT = SelectObject(hdc, hbm);
    }

    dwSize = (DWORD)pbm->bmHeight * (DWORD)pbm->bmWidthBytes;

    if (IsBadHugeWritePtr((LPVOID)pbm->bmBits, dwSize))
    {
        if (hdc)
        {
            SelectObject(hdc, hbmT);
            DeleteDC(hdc);
        }

        return NULL;
    }

    if (p)
    {
        UINT u;

        hmemcpy(p, pbm, min(cb, sizeof(IBITMAP)));
        pbm = p;

        u = GetBitmapType();

        pbm->bmType = u & BM_TYPE;

        if (cb > sizeof(BITMAP))
        {
            pbm->bmBitmapInfo = NULL;
            pbm->bmNextScan = pbm->bmWidthBytes * pbm->bmPlanes;

            if (u & BM_BOTTOMTOTOP)
            {
                pbm->bmOffset = 0;
            }
            else
            {
                pbm->bmOffset   = (long)pbm->bmNextScan * (pbm->bmHeight-1);
                pbm->bmNextScan = -pbm->bmNextScan;
                pbm->bmFillBytes = -pbm->bmFillBytes;
            }

            //
            // see if this particular bitmap is HUGE
            //
            if (!(u & BM_HUGE) || (DWORD)pbm->bmHeight * pbm->bmWidthBytes < 64l*1024)
            {
                pbm->bmFillBytes = 0;
                pbm->bmScanSegment = pbm->bmHeight;
            }
            else
            {
                if (pbm->bmOffset)
                    pbm->bmOffset -= (long)((pbm->bmHeight-1) / pbm->bmScanSegment) * pbm->bmFillBytes;
            }
        }
    }

    if (hdc)
    {
        SelectObject(hdc, hbmT);
        DeleteDC(hdc);
    }

    return (LPVOID)pbm->bmBits;
}

/////////////////////////////////////////////////////////////////////////////
//
//  SetPixel
//
//  some cards cant't seam to do SetPixel right it is amazing they work at all
//
/////////////////////////////////////////////////////////////////////////////

static void SetPixelX(HDC hdc, int x, int y, COLORREF rgb)
{
    RECT rc;

    rc.left = x;
    rc.top  = y;
    rc.right = x+1;
    rc.bottom = y+1;

    SetBkColor(hdc, rgb);
    ExtTextOut(hdc, 0, 0, ETO_OPAQUE, &rc, NULL, 0, NULL);
}

#define SetPixel SetPixelX

///////////////////////////////////////////////////////////////////////////////
//
//  GetSurfaceType
//
///////////////////////////////////////////////////////////////////////////////

#define BCODE _based(_segname("_CODE"))

static BYTE  BCODE bits8[]   = {0x00,0xF9,0xFA,0xFC,0xFF};
static WORD  BCODE bits555[] = {0x0000,0x7C00,0x03E0,0x001F,0x7FFF};
static WORD  BCODE bits5551[]= {0x8000,0xFC00,0x83E0,0x801F,0xFFFF};
static WORD  BCODE bits565[] = {0x0000,0xF800,0x07E0,0x001F,0xFFFF};
static BYTE  BCODE bitsBGR[] = {0x00,0x00,0x00, 0x00,0x00,0xFF, 0x00,0xFF,0x00, 0xFF,0x00,0x00, 0xFF,0xFF,0xFF};
static BYTE  BCODE bitsRGB[] = {0x00,0x00,0x00, 0xFF,0x00,0x00, 0x00,0xFF,0x00, 0x00,0x00,0xFF, 0xFF,0xFF,0xFF};
static DWORD BCODE bitsRGBX[]= {0x000000, 0x0000FF, 0x00FF00, 0xFF0000, 0xFFFFFF};
static DWORD BCODE bitsBGRX[]= {0x000000, 0xFF0000, 0x00FF00, 0x0000FF, 0xFFFFFF};

void FAR TestSurfaceType(HDC hdc, int x, int y)
{
    PatBlt(hdc, x, y, 5, 1, BLACKNESS);

    SetPixel(hdc, x+0, y, RGB(000,000,000));
    SetPixel(hdc, x+1, y, RGB(255,000,000));
    SetPixel(hdc, x+2, y, RGB(000,255,000));
    SetPixel(hdc, x+3, y, RGB(000,000,255));
    SetPixel(hdc, x+4, y, RGB(255,255,255));

    GetPixel(hdc, x, y);
}

UINT FAR GetSurfaceType(LPVOID lpBits)
{
    #define TESTFMT(a,n) \
        if (_fmemcmp(lpBits, (LPVOID)a, sizeof(a)) == 0) return n;

    TESTFMT(bits8,    BM_8BIT);
    TESTFMT(bits555,  BM_16555);
    TESTFMT(bits5551, BM_16555);
    TESTFMT(bits565,  BM_16565);
    TESTFMT(bitsRGB,  BM_24RGB);
    TESTFMT(bitsBGR,  BM_24BGR);
    TESTFMT(bitsRGBX, BM_32RGB);
    TESTFMT(bitsBGRX, BM_32BGR);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
//
//  GetBitmapType
//
//  return the bitmap type that the display driver uses
//
///////////////////////////////////////////////////////////////////////////////

UINT FAR GetBitmapType()
{
    BITMAP bm;
    HBITMAP hbm;
    HBITMAP hbmT;
    HDC hdc;
    UINT u;
    BYTE bits[20*4*2];

    static UINT wBitmapType = 0xFFFF;

    if (wBitmapType != 0xFFFF)
        return wBitmapType;

    //
    // create a test bitmap (<64k)
    //
    hdc = GetDC(NULL);
    hbm = CreateCompatibleBitmap(hdc,20,2);
    ReleaseDC(NULL, hdc);

    hdc = CreateCompatibleDC(NULL);
    hbmT = SelectObject(hdc, hbm);

    GetObject(hbm, sizeof(bm), &bm);
    PatBlt(hdc, 0, 0, bm.bmWidth, bm.bmHeight, BLACKNESS);

    TestSurfaceType(hdc, 0, 0);
    GetBitmapBits(hbm, sizeof(bits), bits);
    u = GetSurfaceType(bits);

    if (u == 0) {
        u = GetSurfaceType(bits + bm.bmWidthBytes);

        if (u)
            u |= BM_BOTTOMTOTOP;
    }

#ifndef WIN32
    if (u) {
        BYTE _huge *pb;
        UINT dy,w;

        //
        // see if bitmap(s) are huge format
        //
        dy = (UINT)(0x10000l/bm.bmWidthBytes) + 1;
        hbm = CreateCompatibleBitmap(hdc,bm.bmWidth,dy);
        DeleteObject(SelectObject(hdc, hbm));
        PatBlt(hdc, 0, 0, bm.bmWidth, dy, BLACKNESS);

        pb = (BYTE _huge *)LockBitmap(hbm);

        if (pb == NULL || OFFSETOF(pb) != 0)
            ; // cant lock bitmaps
        else {
            u |= BM_CANLOCK;

            w = (dy-1) * bm.bmWidthBytes;

            pb[64l*1024] = 0;
            pb[w] = 0;

            if (u & BM_BOTTOMTOTOP)
                SetPixel(hdc, 0, 0, RGB(255,255,255));
            else
                SetPixel(hdc, 0, dy-1, RGB(255,255,255));

            if (pb[64l*1024] != 0 && pb[w] == 0)
                u |= BM_HUGE;
            else if (pb[64l*1024] == 0 && pb[w] != 0)
                ;
            else
                u = 0;
        }
    }
#endif

    SelectObject(hdc, hbmT);
    DeleteObject(hbm);
    DeleteDC(hdc);

    wBitmapType = u;
    return u;
}

//////////////////////////////////////////////////////////////////////////////
//
//  returns the PDevice of the given physical or memory DC
//
//  return the bitmap type that the display driver uses
//
///////////////////////////////////////////////////////////////////////////////

LPVOID FAR GetPDevice(HDC hdc)
{               
    HANDLE h;
    HBITMAP hbm;
    HBITMAP hbmT;
    HDC hdcT=NULL;
    IBITMAP FAR *pbm;
    LPVOID lpPDevice = NULL;

    // GDI.403
    static HANDLE (FAR PASCAL *GdiGetBitmapHandle)(HBITMAP hbm, HANDLE h);

    if (GdiGetBitmapHandle == NULL)
        (FARPROC)GdiGetBitmapHandle = GetProcAddress(GetModuleHandle("GDI"),MAKEINTATOM(403));

    if (GdiGetBitmapHandle == NULL)
        return NULL;

    hbm = CreateBitmap(1,1,1,1,NULL);

    //
    //  first try the passed DC if it is a bitmap/DC
    //
    hbmT = SelectBitmap(hdc, hbm);

    if (hbmT != NULL)
    {
        //
        // it is a memory DC.
        //
        h = GdiGetBitmapHandle(hbmT, 0);
    }
    else
    {
        //
        // it is a physical DC.
        //

        hdcT = CreateCompatibleDC(hdc);
        hbmT = SelectBitmap(hdcT, hbm);

        h = GdiGetBitmapHandle(hbm, 0);
    }

    if (h == NULL)
        goto exit;

    pbm = (IBITMAP FAR *)GlobalLock(h);

    if (IsBadReadPtr(pbm, sizeof(IBITMAP)))
        goto exit;

    if (pbm)
        pbm = (IBITMAP FAR *)pbm->bmlpPDevice;
    else
        pbm = NULL;

    if (IsBadReadPtr(pbm, 2))
        goto exit;

    lpPDevice = (LPVOID)pbm;

exit:
    if (hdcT)
    {
        SelectObject(hdcT, hbmT);
        DeleteObject(hdcT);
    }
    else
    {
        SelectObject(hdc, hbmT);
    }

    DeleteObject(hbm);

    return lpPDevice;
}
