/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdibmp.c
 * PURPOSE:         GDI Bitmap Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtGdiCheckBitmapBits(IN HDC hdc,
                     IN HANDLE hColorTransform,
                     IN PVOID pvBits,
                     IN ULONG bmFormat,
                     IN DWORD dwWidth,
                     IN DWORD dwHeight,
                     IN DWORD dwStride,
                     OUT PBYTE paResults)
{
    UNIMPLEMENTED;
    return FALSE;
}

HBITMAP
APIENTRY
NtGdiClearBitmapAttributes(IN HBITMAP hbm,
                           IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return NULL;
}

HBITMAP
APIENTRY
NtGdiCreateBitmap(IN INT cx,
                  IN INT cy,
                  IN UINT cPlanes,
                  IN UINT cBPP,
                  IN OPTIONAL LPBYTE pjInit)
{
    UNIMPLEMENTED;
    return NULL;
}

HBITMAP
APIENTRY
NtGdiCreateCompatibleBitmap(IN HDC hdc,
                            IN INT cx,
                            IN INT cy)
{
    UNIMPLEMENTED;
    return NULL;
}

HBITMAP
APIENTRY
NtGdiCreateDIBitmapInternal(IN HDC hdc,
                            IN INT cx,
                            IN INT cy,
                            IN DWORD fInit,
                            IN OPTIONAL LPBYTE pjInit,
                            IN OPTIONAL LPBITMAPINFO pbmi,
                            IN DWORD iUsage,
                            IN UINT cjMaxInitInfo,
                            IN UINT cjMaxBits,
                            IN FLONG f,
                            IN HANDLE hcmXform)
{
    UNIMPLEMENTED;
    return NULL;
}

HBITMAP
APIENTRY
NtGdiCreateDIBSection(IN HDC hdc,
                      IN OPTIONAL HANDLE hSectionApp,
                      IN DWORD dwOffset,
                      IN LPBITMAPINFO pbmi,
                      IN DWORD iUsage,
                      IN UINT cjHeader,
                      IN FLONG fl,
                      IN ULONG_PTR dwColorSpace,
                      OUT PVOID *ppvBits)
{
    UNIMPLEMENTED;
    return NULL;
}

LONG
APIENTRY
NtGdiGetBitmapBits(IN HBITMAP hbm,
                   IN ULONG cjMax,
                   OUT OPTIONAL PBYTE pjOut)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiGetBitmapDimension(IN HBITMAP hbm,
                        OUT LPSIZE psize)
{
    UNIMPLEMENTED;
    return FALSE;
}

ULONG_PTR
APIENTRY
NtGdiGetColorSpaceforBitmap(IN HBITMAP hsurf)
{
    UNIMPLEMENTED;
    return 0;
}

HDC
APIENTRY
NtGdiGetDCforBitmap(IN HBITMAP hsurf)
{
    UNIMPLEMENTED;
    return NULL;
}

INT
APIENTRY
NtGdiGetDIBitsInternal(IN HDC hdc,
                       IN HBITMAP hbm,
                       IN UINT iStartScan,
                       IN UINT cScans,
                       OUT OPTIONAL LPBYTE pBits,
                       IN OUT LPBITMAPINFO pbmi,
                       IN UINT iUsage,
                       IN UINT cjMaxBits,
                       IN UINT cjMaxInfo)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiMonoBitmap(IN HBITMAP hbm)
{
    UNIMPLEMENTED;
    return FALSE;
}

HBITMAP
APIENTRY
NtGdiSelectBitmap(IN HDC hdc,
                  IN HBITMAP hbm)
{
    UNIMPLEMENTED;
    return NULL;
}

HBITMAP
APIENTRY
NtGdiSetBitmapAttributes(IN HBITMAP hbm,
                         IN DWORD dwFlags)
{
    UNIMPLEMENTED;
    return NULL;
}

LONG
APIENTRY
NtGdiSetBitmapBits(IN HBITMAP hbm,
                   IN ULONG cj,
                   IN PBYTE pjInit)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiSetBitmapDimension(IN HBITMAP hbm,
                        IN INT cx,
                        IN INT cy,
                        OUT OPTIONAL LPSIZE psizeOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

INT
APIENTRY
NtGdiSetDIBitsToDeviceInternal(IN HDC hdcDest,
                               IN INT xDst,
                               IN INT yDst,
                               IN DWORD cx,
                               IN DWORD cy,
                               IN INT xSrc,
                               IN INT ySrc,
                               IN DWORD iStartScan,
                               IN DWORD cNumScan,
                               IN LPBYTE pInitBits,
                               IN LPBITMAPINFO pbmi,
                               IN DWORD iUsage,
                               IN UINT cjMaxBits,
                               IN UINT cjMaxInfo,
                               IN BOOL bTransformCoordinates,
                               IN OPTIONAL HANDLE hcmXform)
{
    UNIMPLEMENTED;
    return 0;
}

INT
APIENTRY
NtGdiStretchDIBitsInternal(IN HDC hdc,
                           IN INT xDst,
                           IN INT yDst,
                           IN INT cxDst,
                           IN INT cyDst,
                           IN INT xSrc,
                           IN INT ySrc,
                           IN INT cxSrc,
                           IN INT cySrc,
                           IN OPTIONAL LPBYTE pjInit,
                           IN LPBITMAPINFO pbmi,
                           IN DWORD dwUsage,
                           IN DWORD dwRop4,
                           IN UINT cjMaxInfo,
                           IN UINT cjMaxBits,
                           IN HANDLE hcmXform)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtGdiGetPixel(IN HDC hdc,
              IN INT x,
              IN INT y)
{
    UNIMPLEMENTED;
    return 0;
}

COLORREF
APIENTRY
NtGdiSetPixel(IN HDC hdcDst,
              IN INT x,
              IN INT y,
              IN COLORREF crColor)
{
    UNIMPLEMENTED;
    return 0;
}

HBITMAP
APIENTRY
NtGdiGetObjectBitmapHandle(IN HBRUSH hbr,
                           OUT UINT *piUsage)
{
    UNIMPLEMENTED;
    return NULL;
}
