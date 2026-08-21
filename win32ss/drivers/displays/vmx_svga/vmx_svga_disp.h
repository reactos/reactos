/*
 * PROJECT:         ReactOS
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * FILE:            win32ss/drivers/displays/vmx_svga/vmx_svga_disp.h
 * PURPOSE:         VMware SVGA-II display driver definitions
 */

#pragma once

#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <winioctl.h>
#include <ntddvdeo.h>
#include <debug.h>

#include "../../miniport/vmx_svga/vmx_svga_ioctl.h"

#define DEVICE_NAME L"vmx_svga_disp"
#define ALLOC_TAG 'DXMV'
#define VMX_PDEV_SIGNATURE 0x564D5850

#define VMX_SURFACE_HOOKS (HOOK_BITBLT | HOOK_COPYBITS | HOOK_PAINT | HOOK_LINETO)

typedef struct _PDEV
{
    ULONG Signature;
    HANDLE hDriver;
    HDEV hDevEng;
    HSURF hSurfEng;
    HSURF hBacking;
    SURFOBJ *psoBacking;

    ULONG ModeIndex;
    ULONG ScreenWidth;
    ULONG ScreenHeight;
    ULONG ScreenDelta;
    UCHAR BitsPerPixel;
    PVOID ScreenPtr;
    PVOID VideoRamPtr;

    ULONG RedMask;
    ULONG GreenMask;
    ULONG BlueMask;
    UCHAR PaletteShift;

    PPALETTEENTRY PaletteEntries;
    HPALETTE DefaultPalette;

    ULONG iDitherFormat;
    ULONG MemHeight;
    ULONG MemWidth;
} PDEV, *PPDEV;

DWORD
GetAvailableModes(
    HANDLE hDriver,
    PVIDEO_MODE_INFORMATION *ModeInfo,
    DWORD *ModeInfoSize);

BOOL
IntInitScreenInfo(
    PPDEV ppdev,
    LPDEVMODEW pDevMode,
    PGDIINFO pGdiInfo,
    PDEVINFO pDevInfo);

BOOL
IntInitDefaultPalette(
    PPDEV ppdev,
    PDEVINFO pDevInfo);

BOOL APIENTRY
DrvEnableDriver(
    ULONG iEngineVersion,
    ULONG cj,
    PDRVENABLEDATA pded);

DHPDEV APIENTRY
DrvEnablePDEV(
    IN DEVMODEW *pdm,
    IN LPWSTR pwszLogAddress,
    IN ULONG cPat,
    OUT HSURF *phsurfPatterns,
    IN ULONG cjCaps,
    OUT ULONG *pdevcaps,
    IN ULONG cjDevInfo,
    OUT DEVINFO *pdi,
    IN HDEV hdev,
    IN LPWSTR pwszDeviceName,
    IN HANDLE hDriver);

VOID APIENTRY DrvCompletePDEV(IN DHPDEV dhpdev, IN HDEV hdev);
VOID APIENTRY DrvDisablePDEV(IN DHPDEV dhpdev);
HSURF APIENTRY DrvEnableSurface(IN DHPDEV dhpdev);
VOID APIENTRY DrvDisableSurface(IN DHPDEV dhpdev);
BOOL APIENTRY DrvAssertMode(IN DHPDEV dhpdev, IN BOOL bEnable);
ULONG APIENTRY DrvGetModes(IN HANDLE hDriver, IN ULONG cjSize, OUT DEVMODEW *pdm);
BOOL APIENTRY DrvSetPalette(IN DHPDEV dhpdev, IN PALOBJ *ppalo, IN FLONG fl, IN ULONG iStart, IN ULONG cColors);

ULONG APIENTRY
DrvSetPointerShape(
    IN SURFOBJ *pso,
    IN SURFOBJ *psoMask,
    IN SURFOBJ *psoColor,
    IN XLATEOBJ *pxlo,
    IN LONG xHot,
    IN LONG yHot,
    IN LONG x,
    IN LONG y,
    IN RECTL *prcl,
    IN FLONG fl);

VOID APIENTRY DrvMovePointer(IN SURFOBJ *pso, IN LONG x, IN LONG y, IN RECTL *prcl);

BOOL APIENTRY
DrvBitBlt(
    IN OUT SURFOBJ *psoTrg,
    IN SURFOBJ *psoSrc,
    IN SURFOBJ *psoMask,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN RECTL *prclTrg,
    IN POINTL *pptlSrc,
    IN POINTL *pptlMask,
    IN BRUSHOBJ *pbo,
    IN POINTL *pptlBrush,
    IN ROP4 rop4);

BOOL APIENTRY
DrvCopyBits(
    IN SURFOBJ *psoDest,
    IN SURFOBJ *psoSrc,
    IN CLIPOBJ *pco,
    IN XLATEOBJ *pxlo,
    IN RECTL *prclDest,
    IN POINTL *pptlSrc);

BOOL APIENTRY
DrvPaint(
    IN SURFOBJ *pso,
    IN CLIPOBJ *pco,
    IN BRUSHOBJ *pbo,
    IN POINTL *pptlBrushOrg,
    IN MIX mix);

BOOL APIENTRY
DrvLineTo(
    IN SURFOBJ *pso,
    IN CLIPOBJ *pco,
    IN BRUSHOBJ *pbo,
    IN LONG x1,
    IN LONG y1,
    IN LONG x2,
    IN LONG y2,
    IN RECTL *prclBounds,
    IN MIX mix);
