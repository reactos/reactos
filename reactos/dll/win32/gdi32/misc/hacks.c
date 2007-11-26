#include "precomp.h"

/* $Id: stubs.c 28709 2007-08-31 15:09:51Z greatlrd $
 *
 * reactos/lib/gdi32/misc/hacks.c
 *
 * GDI32.DLL hacks
 *
 * Api that are hacked but we can not do correct implemtions yetm but using own syscall
 *
 */

/*
 * @implemented
 *
 */
int
STDCALL
GetPolyFillMode(HDC hdc)
{
    /* FIXME do not use reactos own syscall for this,
     * this hack need be remove
     */
     return NtGdiGetPolyFillMode(hdc);
}


/*
 * @implemented
 *
 */
int
STDCALL
GetGraphicsMode(HDC hdc)
{
    /* FIXME do not use reactos own syscall for this,
     * this hack need be remove
     */
     return NtGdiGetGraphicsMode(hdc);
}

/*
 * @implemented
 *
 */
INT
STDCALL
SetDIBitsToDevice(
    HDC hDC,
    int XDest,
    int YDest,
    DWORD Width,
    DWORD Height,
    int XSrc,
    int YSrc,
    UINT StartScan,
    UINT ScanLines,
    CONST VOID *Bits,
    CONST BITMAPINFO *lpbmi,
    UINT ColorUse)
{
    return NtGdiSetDIBitsToDeviceInternal(hDC,
                                          XDest,
                                          YDest,
                                          Width,
                                          Height,
                                          XSrc,
                                          YSrc,
                                          StartScan,
                                          ScanLines,
                                          (LPBYTE)Bits,
                                          (LPBITMAPINFO)lpbmi,
                                          ColorUse,
                                          lpbmi->bmiHeader.biSizeImage,
                                          lpbmi->bmiHeader.biSize,
                                          FALSE,
                                          NULL);
}

/*
 * @implemented
 *
 */
HGDIOBJ
STDCALL
SelectObject(HDC hdc,
             HGDIOBJ hgdiobj)
{
    return NtGdiSelectObject(hdc,hgdiobj);
}


/*
 * @implemented
 *
 */
int
STDCALL
GetMapMode(HDC hdc)
{
    return NtGdiGetMapMode(hdc);
}

/*
 * @implemented
 *
 */
int
STDCALL
GetStretchBltMode(HDC hdc)
{
    return NtGdiGetStretchBltMode(hdc);
}

/*
 * @implemented
 *
 */
BOOL
STDCALL
OffsetViewportOrgEx(HDC hdc,
                    int nXOffset,
                    int nYOffset,
                    LPPOINT lpPoint)
{
    return  NtGdiOffsetViewportOrgEx(hdc, nXOffset, nYOffset, lpPoint);
}

/*
 * @implemented
 *
 */
BOOL
STDCALL
OffsetWindowOrgEx(HDC hdc,
                  int nXOffset,
                  int nYOffset,
                  LPPOINT lpPoint)
{
    return NtGdiOffsetWindowOrgEx(hdc, nXOffset, nYOffset, lpPoint);
}



