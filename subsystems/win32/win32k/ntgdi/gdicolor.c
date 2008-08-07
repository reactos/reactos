/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntgdi/gdicolor.c
 * PURPOSE:         GDI Color Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

HANDLE
APIENTRY
NtGdiCreateColorSpace(IN PLOGCOLORSPACEEXW pLogColorSpace)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiSetColorSpace(IN HDC hdc,
                   IN HCOLORSPACE hColorSpace)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiDeleteColorSpace(IN HANDLE hColorSpace)
{
    UNIMPLEMENTED;
    return FALSE;
}

HANDLE
APIENTRY
NtGdiCreateColorTransform(IN HDC hdc,
                          IN LPLOGCOLORSPACEW pLogColorSpaceW,
                          IN OPTIONAL PVOID pvSrcProfile,
                          IN ULONG cjSrcProfile,
                          IN OPTIONAL PVOID pvDestProfile,
                          IN ULONG cjDestProfile,
                          IN OPTIONAL PVOID pvTargetProfile,
                          IN ULONG cjTargetProfile)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtGdiDeleteColorTransform(IN HDC hdc,
                          IN HANDLE hColorTransform)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGetColorAdjustment(IN HDC hdc,
                        OUT PCOLORADJUSTMENT pcaOut)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSetColorAdjustment(IN HDC hdc,
                        IN PCOLORADJUSTMENT pca)
{
    UNIMPLEMENTED;
    return FALSE;
}

COLORREF
APIENTRY
NtGdiGetNearestColor(IN HDC hdc,
                     IN COLORREF cr)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtGdiSetMagicColors(IN HDC hdc,
                    IN PALETTEENTRY peMagic,
                    IN ULONG Index)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiUpdateColors(IN HDC hdc)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiGetDeviceGammaRamp(IN HDC hdc,
                        OUT LPVOID lpGammaRamp)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSetDeviceGammaRamp(IN HDC hdc,
                        IN LPVOID lpGammaRamp)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtGdiSetIcmMode(IN HDC hdc,
                IN ULONG nCommand,
                IN ULONG ulMode)
{
    UNIMPLEMENTED;
    return FALSE;
}
