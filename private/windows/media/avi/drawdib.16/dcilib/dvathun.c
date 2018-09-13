/****************************************************************************
 ***************************************************************************/

#include <windows.h>
#include "dva.h"
#include "lockbm.h"

extern BOOL FAR PASCAL _loadds vga_open_surface(LPVOID pv);
extern void FAR PASCAL _loadds vga_close_surface(LPVOID pv);
extern BOOL FAR PASCAL _loadds vga_begin_access(LPVOID pv, int x, int y, int dx, int dy);
extern void FAR PASCAL _loadds vga_end_access(LPVOID pv);

/****************************************************************************
 ***************************************************************************/

BOOL thun_get_surface(HDC hdc, int nSurface, DVASURFACEINFO FAR *pdva)
{
    DWORD SizeImage;
    IBITMAP FAR *pbm;
    UINT sel;
    DWORD off;
    LPBITMAPINFOHEADER lpbi;

    if (nSurface != 0)
        return FALSE;

    pbm = GetPDevice(hdc);

    if (pbm == NULL || pbm->bmType == 0)
        return FALSE;

    if (pbm->bmType != 0xFFFF)
        return FALSE;

    sel = ((WORD FAR *)&pbm->bmBits)[1];
    off = ((WORD FAR *)&pbm->bmBits)[0];

    SizeImage = (DWORD)(UINT)pbm->bmWidthBytes * (DWORD)(UINT)pbm->bmHeight;

    if (GetSelectorLimit(sel) != 0x3FFFFF)
        return FALSE;

    lpbi = &pdva->BitmapInfo;

    lpbi->biSize            = sizeof(BITMAPINFOHEADER);
    lpbi->biWidth           = pbm->bmWidthBytes/(pbm->bmBitsPixel/8);
    lpbi->biHeight          = -(int)pbm->bmHeight;
    lpbi->biPlanes          = pbm->bmPlanes;
    lpbi->biBitCount        = pbm->bmBitsPixel;
    lpbi->biCompression     = 0;
    lpbi->biSizeImage       = SizeImage;
    lpbi->biXPelsPerMeter   = pbm->bmWidthBytes;
    lpbi->biYPelsPerMeter   = 0;
    lpbi->biClrUsed         = 0;
    lpbi->biClrImportant    = 0;

    pdva->selSurface   = sel;
    pdva->offSurface   = off;
    pdva->Version      = 0x0100;
    pdva->Flags        = 0;
    pdva->lpSurface    = 0;
    (FARPROC)pdva->OpenSurface  = (FARPROC)vga_open_surface;
    (FARPROC)pdva->CloseSurface = (FARPROC)vga_close_surface;
    (FARPROC)pdva->BeginAccess  = (FARPROC)vga_begin_access;
    (FARPROC)pdva->EndAccess    = (FARPROC)vga_end_access;
    (FARPROC)pdva->ShowSurface  = (FARPROC)NULL;

    return TRUE;
}
