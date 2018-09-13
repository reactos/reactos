#include <windows.h>

#define _INC_VFW
#define VFWAPI FAR PASCAL _loadds

#include "dva.h"
#include "lockbm.h"

/****************************************************************************
 ***************************************************************************/

extern BOOL vga_get_surface(HDC, int, DVASURFACEINFO FAR *);
extern BOOL ati_get_surface(HDC, int, DVASURFACEINFO FAR *);
extern BOOL dib_get_surface(HDC, int, DVASURFACEINFO FAR *);
extern BOOL thun_get_surface(HDC,int, DVASURFACEINFO FAR *);
extern BOOL vlb_get_surface(HDC, int, DVASURFACEINFO FAR *);

#define GetDS() SELECTOROF((LPVOID)&ScreenSel)
static short ScreenSel;

static BOOL InitSurface(DVASURFACEINFO FAR *pdva);
static BOOL TestSurface(DVASURFACEINFO FAR *pdva);
static void SetSelLimit(UINT sel, DWORD limit);

/****************************************************************************
 ***************************************************************************/

void FAR PASCAL DVAInit()
{
}

/****************************************************************************
 ***************************************************************************/

void FAR PASCAL DVATerm()
{
    //
    // free screen alias
    //
    if (ScreenSel)
    {
        SetSelLimit(ScreenSel, 0);
        FreeSelector(ScreenSel);
        ScreenSel = 0;
    }
}

/****************************************************************************
 ***************************************************************************/

BOOL FAR PASCAL _loadds DVAGetSurface(HDC hdc, int nSurface, DVASURFACEINFO FAR *pdva)
{
    int i;

#ifdef USESTUPIDESCAPE
    i = Escape(hdc, DVAGETSURFACE,sizeof(int),(LPCSTR)&nSurface,(LPVOID)pdva);
#else
    i = 0;
#endif
    
    //
    // should this be a function table? list?
    //
    if (i <= 0 &&
#ifdef BADDIBENGINESTUFF
        !dib_get_surface(hdc, nSurface, pdva) &&
#endif  
	!ati_get_surface(hdc, nSurface, pdva) &&
#ifdef DEBUG
        !vlb_get_surface(hdc, nSurface, pdva) &&
        !thun_get_surface(hdc, nSurface, pdva) &&
#endif
        !vga_get_surface(hdc, nSurface, pdva))

        return FALSE;

    return InitSurface(pdva);
}

#if 0

/****************************************************************************
 ***************************************************************************/

HDVA FAR PASCAL DVAOpenSurface(HDC hdc, int nSurface)
{
    PDVA pdva;

    pdva = (PDVA)GlobalAllocPtr(GHND|GMEM_SHARE, sizeof(DVASURFACEINFO));

    if (pdva == NULL)
        return NULL;

    if (!DVAGetSurface(hdc, nSurface, pdva) ||
        !pdva->OpenSurface(pdva->lpSurface))
    {
        GlobalFreePtr(pdva);
        return NULL;
    }

    return pdva;
}

/****************************************************************************
 ***************************************************************************/

void FAR PASCAL DVACloseSurface(HDVA pdva)
{
    if (pdva == NULL)
        return;

    pdva->CloseSurface(pdva->lpSurface);

    GlobalFreePtr(pdva);
}

#endif

/****************************************************************************
 ***************************************************************************/

BOOL CALLBACK default_open_surface(LPVOID pv)
{
    return TRUE;
}

/****************************************************************************
 ***************************************************************************/

void CALLBACK default_close_surface(LPVOID pv)
{
}

/****************************************************************************
 ***************************************************************************/

BOOL CALLBACK default_begin_access(LPVOID pv, int x, int y, int dx, int dy)
{
    return TRUE;
}

/****************************************************************************
 ***************************************************************************/

void CALLBACK default_end_access(LPVOID pv)
{
}


/****************************************************************************
 ***************************************************************************/

UINT CALLBACK default_show_surface(LPVOID pv, HWND hwnd, LPRECT src, LPRECT dst)
{
    return 1;
}

/****************************************************************************
 ***************************************************************************/

static BOOL InitSurface(DVASURFACEINFO FAR *pdva)
{
    LPBITMAPINFOHEADER lpbi;

    if (pdva->Version != 0x0100)
        return FALSE;

    lpbi = &pdva->BitmapInfo;

    if (lpbi->biSize != sizeof(BITMAPINFOHEADER))
        return FALSE;

    if (lpbi->biPlanes != 1)
        return FALSE;

    //
    // make the pointer a 16:16 pointer
    //
    if (pdva->offSurface >= 0x10000 &&
        !(pdva->Flags & DVAF_1632_ACCESS))
    {
        if (ScreenSel == NULL)
            ScreenSel = AllocSelector(GetDS());

        if (pdva->selSurface != 0)
            pdva->offSurface += GetSelectorBase(pdva->selSurface);

        SetSelectorBase(ScreenSel,pdva->offSurface);
        SetSelLimit(ScreenSel,lpbi->biSizeImage-1);

        pdva->offSurface = 0;
        pdva->selSurface = ScreenSel;
    }

    //
    // fill in defaults.
    //
    if (pdva->OpenSurface == NULL)
        pdva->OpenSurface = default_open_surface;

    if (pdva->CloseSurface == NULL)
        pdva->CloseSurface = default_close_surface;

    if (pdva->ShowSurface == NULL)
        pdva->ShowSurface = default_show_surface;

    if (pdva->BeginAccess == NULL)
    {
        pdva->BeginAccess = default_begin_access;
        pdva->EndAccess   = default_end_access;
    }

    //
    // only test RGB surfaces.
    //
    if (lpbi->biCompression == 0 ||
        lpbi->biCompression == BI_BITFIELDS ||
        lpbi->biCompression == BI_1632)
    {
        if (!TestSurface(pdva))
            return FALSE;
    }

    //
    // set BI_1632 if needed
    //
    if (pdva->Flags & DVAF_1632_ACCESS)
    {
        lpbi->biCompression = BI_1632;
    }

    return TRUE;
}

/****************************************************************************
 ***************************************************************************/
#pragma optimize("", off)
static void SetSelLimit(UINT sel, DWORD limit)
{
    if (limit >= 1024*1024l)
        limit = ((limit+4096) & ~4095) - 1;

    _asm
    {
        mov     ax,0008h            ; DPMI set limit
        mov     bx,sel
        mov     dx,word ptr limit[0]
        mov     cx,word ptr limit[2]
        int     31h
    }
}
#pragma optimize("", on)

/****************************************************************************
 ***************************************************************************/

#define ASM66 _asm _emit 0x66 _asm
#define DB    _asm _emit

#pragma optimize("", off)
static BYTE ReadByte(PDVA pdva, LPVOID lpBits, DWORD dw)
{
    BYTE b=42;

    DVABeginAccess(pdva, 0, 0, 1024, 1024);

    _asm {
        ASM66   xor     bx,bx
                les     bx,lpBits
        ASM66   add     bx,word ptr dw
                mov     ax,es
        ASM66   lsl     ax,ax
        ASM66   cmp     bx,ax
                ja      exit
        DB 26h ;mov     al,es:[ebx]
        DB 67h
        DB 8Ah
        DB 03h
                mov b,al
exit:
    }

    DVAEndAccess(pdva);

    return b;
}
#pragma optimize("", on)

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

/****************************************************************************
 ***************************************************************************/

static BOOL TestSurface(DVASURFACEINFO FAR *pdva)
{
    HDC hdc;
    int x,y,h,w,wb;
    COLORREF rgb,rgb0,rgb1,rgb2,rgb3,rgb4;
    DWORD dw;
    BYTE  b0,b1;
    UINT  uType=0;
    LPBITMAPINFOHEADER lpbi;
    LPVOID lpBits;
    HCURSOR hcur;

    if (!pdva->OpenSurface(pdva->lpSurface))
        return FALSE;

    lpbi = DVAGetSurfaceFmt(pdva);
    lpBits = DVAGetSurfacePtr(pdva);

    h = abs((int)lpbi->biHeight);
    w = (int)lpbi->biWidth;
    wb = (w * ((UINT)lpbi->biBitCount/8) + 3) & ~3;
    dw = (DWORD)(UINT)(h-1) * (DWORD)(UINT)wb;

    if ((int)lpbi->biHeight < 0)
        y = 0;
    else
        y = h-1;

#ifdef XDEBUG
    x = (int)lpbi->biWidth - 5;
    ((LPBYTE)lpBits) += x * (UINT)lpbi->biBitCount/8;
#else
    x = 0;
#endif

    hcur = SetCursor(NULL);
    hdc = GetDC(NULL);

    rgb = GetPixel(hdc, x, h-1-y);
    SetPixel(hdc, x, h-1-y, RGB(0,0,0));       GetPixel(hdc, x, h-1-y); b0 = ReadByte(pdva, lpBits, dw);
    SetPixel(hdc, x, h-1-y, RGB(255,255,255)); GetPixel(hdc, x, h-1-y); b1 = ReadByte(pdva, lpBits, dw);
    SetPixel(hdc, x, h-1-y,rgb);

    if (b0 != 0x00 || b1 == 0x00)
        goto done;

    rgb0 = GetPixel(hdc, x+0, y);
    rgb1 = GetPixel(hdc, x+1, y);
    rgb2 = GetPixel(hdc, x+2, y);
    rgb3 = GetPixel(hdc, x+3, y);
    rgb4 = GetPixel(hdc, x+4, y);

    TestSurfaceType(hdc, x, y);

    DVABeginAccess(pdva, x, y, 5, 1);
    uType = GetSurfaceType(lpBits);
    DVAEndAccess(pdva);

    SetPixel(hdc, x+0, y,rgb0);
    SetPixel(hdc, x+1, y,rgb1);
    SetPixel(hdc, x+2, y,rgb2);
    SetPixel(hdc, x+3, y,rgb3);
    SetPixel(hdc, x+4, y,rgb4);

done:
    ReleaseDC(NULL, hdc);
    SetCursor(hcur);

    pdva->CloseSurface(pdva->lpSurface);

    switch (uType)
    {
        case BM_8BIT:
            break;

        case BM_16555:
            ((LPDWORD)(lpbi+1))[0] = 0x007C00;
            ((LPDWORD)(lpbi+1))[1] = 0x0003E0;
            ((LPDWORD)(lpbi+1))[2] = 0x00001F;
            break;

        case BM_24BGR:
        case BM_32BGR:
            ((LPDWORD)(lpbi+1))[0] = 0xFF0000;
            ((LPDWORD)(lpbi+1))[1] = 0x00FF00;
            ((LPDWORD)(lpbi+1))[2] = 0x0000FF;
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
    }

    return uType != 0;
}
