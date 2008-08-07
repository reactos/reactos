/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdiblt.c
 * PURPOSE:         GDI Bit Block Transfer Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtGdiAlphaBlend(IN HDC hdcDst,
                IN LONG DstX,
                IN LONG DstY,
                IN LONG DstCx,
                IN LONG DstCy,
                IN HDC hdcSrc,
                IN LONG SrcX,
                IN LONG SrcY,
                IN LONG SrcCx,
                IN LONG SrcCy,
                IN BLENDFUNCTION BlendFunction,
                IN HANDLE hcmXform)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiBitBlt(IN HDC hdcDst,
            IN INT x,
            IN INT y,
            IN INT cx,
            IN INT cy,
            IN HDC hdcSrc,
            IN INT xSrc,
            IN INT ySrc,
            IN DWORD rop4,
            IN DWORD crBackColor,
            IN FLONG fl)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiMaskBlt(IN HDC hdc,
             IN INT xDst,
             IN INT yDst,
             IN INT cx,
             IN INT cy,
             IN HDC hdcSrc,
             IN INT xSrc,
             IN INT ySrc,
             IN HBITMAP hbmMask,
             IN INT xMask,
             IN INT yMask,
             IN DWORD dwRop4,
             IN DWORD crBackColor)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiPatBlt(IN HDC hdcDst,
            IN INT x,
            IN INT y,
            IN INT cx,
            IN INT cy,
            IN DWORD rop4)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiPolyPatBlt(IN HDC hdc,
                IN DWORD rop4,
                IN PPOLYPATBLT pPoly,
                IN DWORD Count,
                IN DWORD Mode)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiPlgBlt(IN HDC hdcTrg,
            IN LPPOINT pptlTrg,
            IN HDC hdcSrc,
            IN INT xSrc,
            IN INT ySrc,
            IN INT cxSrc,
            IN INT cySrc,
            IN HBITMAP hbmMask,
            IN INT xMask,
            IN INT yMask,
            IN DWORD crBackColor)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiStretchBlt(IN HDC hdcDst,
                IN INT xDst,
                IN INT yDst,
                IN INT cxDst,
                IN INT cyDst,
                IN HDC hdcSrc,
                IN INT xSrc,
                IN INT ySrc,
                IN INT cxSrc,
                IN INT cySrc,
                IN DWORD dwRop,
                IN DWORD dwBackColor)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiTransparentBlt(IN HDC hdcDst,
                    IN INT xDst,
                    IN INT yDst,
                    IN INT cxDst,
                    IN INT cyDst,
                    IN HDC hdcSrc,
                    IN INT xSrc,
                    IN INT ySrc,
                    IN INT cxSrc,
                    IN INT cySrc,
                    IN COLORREF TransColor)
{
    UNIMPLEMENTED;
    return FALSE;
}
