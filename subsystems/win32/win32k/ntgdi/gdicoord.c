/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdicoord.c
 * PURPOSE:         GDI Coordinate Space and Transformation Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

BOOL
APIENTRY
NtGdiCombineTransform(OUT LPXFORM pxfDst,
                      IN LPXFORM pxfSrc1,
                      IN LPXFORM pxfSrc2)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiComputeXformCoefficients(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGetTransform(IN HDC hdc,
                  IN DWORD iXform,
                  OUT LPXFORM pxf)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiModifyWorldTransform(IN HDC hdc,
                          IN OPTIONAL LPXFORM pxf,
                          IN DWORD iXform)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiScaleViewportExtEx(IN HDC hdc,
                        IN INT xNum,
                        IN INT xDenom,
                        IN INT yNum,
                        IN INT yDenom,
                        OUT OPTIONAL LPSIZE pszOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiScaleWindowExtEx(IN HDC hdc,
                      IN INT xNum,
                      IN INT xDenom,
                      IN INT yNum,
                      IN INT yDenom,
                      OUT OPTIONAL LPSIZE pszOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSetSizeDevice(IN HDC hdc,
                   IN INT cxVirtualDevice,
                   IN INT cyVirtualDevice)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSetVirtualResolution(IN HDC hdc,
                          IN INT cxVirtualDevicePixel,
                          IN INT cyVirtualDevicePixel,
                          IN INT cxVirtualDeviceMm,
                          IN INT cyVirtualDeviceMm)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiTransformPoints(IN HDC hdc,
                     IN PPOINT pptIn,
                     OUT PPOINT pptOut,
                     IN INT c,
                     IN INT iMode)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiMirrorWindowOrg(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}


BOOL
APIENTRY
NtGdiUpdateTransform(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}
