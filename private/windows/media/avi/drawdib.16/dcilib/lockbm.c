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
//  determine the physical format of a framebuffer by seting pixels and
//  reading them back to see what we got.
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
    static HANDLE (FAR PASCAL *Gdi403)(HBITMAP hbm, HANDLE h);

    if (Gdi403 == NULL)
        (FARPROC)Gdi403 = GetProcAddress(GetModuleHandle("GDI"),MAKEINTATOM(403));

    if (Gdi403 == NULL)
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
        h = Gdi403(hbmT, 0);
    }
    else
    {
        //
        // it is a physical DC.
        //

        hdcT = CreateCompatibleDC(hdc);
        hbmT = SelectBitmap(hdcT, hbm);

        h = Gdi403(hbm, 0);
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
