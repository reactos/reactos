/*
 * PROJECT:     Xbox NV2A accelerated GDI display driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Header — PDEV and shared decls
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#ifndef _XBOXDISP_PCH_
#define _XBOXDISP_PCH_

#include <stdarg.h>
#include <windef.h>
#include <wingdi.h>
#include <winddi.h>
#include <winioctl.h>
#include <ntddvdeo.h>

#include "../../miniport/xboxvmp/nv2a_accel.h"

#define DEVICE_NAME L"xboxdisp"
#define ALLOC_TAG   'DxbX'

typedef struct _PDEV
{
    HANDLE hDriver;
    HDEV   hDevEng;
    HSURF  hSurfEng;
    ULONG  ModeIndex;
    ULONG  ScreenWidth;
    ULONG  ScreenHeight;
    ULONG  ScreenDelta;
    BYTE   BitsPerPixel;
    ULONG  RedMask;
    ULONG  GreenMask;
    ULONG  BlueMask;
    BYTE   PaletteShift;
    PVOID  ScreenPtr;
    HPALETTE     DefaultPalette;
    PALETTEENTRY *PaletteEntries;
    BOOL   AccelAvailable;
    BOOL   AccelHardware;
    DWORD  iDitherFormat;
    PVOID  VramBase;          /* CPU base of the mapped video memory (== ScreenPtr) */
    ULONG  VramLen;           /* mapped video-memory length in bytes */
    ULONG  FbGpuOffset;       /* GPU offset of VramBase */
#define XBOXDISP_HEAP_MAX_SPANS 64
    struct { ULONG Off; ULONG Size; } HeapFree[XBOXDISP_HEAP_MAX_SPANS]; /* free spans, GPU-absolute Off */
    ULONG  HeapSpanCount;
} PDEV, *PPDEV;

/* Per-device-bitmap driver handle (DHSURF) for EngCreateDeviceSurface bitmaps. */
typedef struct _XBOXDISP_DEVBMP
{
    struct _PDEV *ppdev; /* owning device (needed to free the heap span on delete) */
    HSURF  hsurf;
    PVOID  CpuPtr;       /* CPU address of the bitmap's pixels in mapped VRAM */
    ULONG  GpuOffset;    /* absolute GPU offset of the pixels */
    ULONG  Pitch;        /* stride in bytes */
    ULONG  HeapOff;      /* GPU-absolute offset returned by the allocator (for free) */
    ULONG  HeapSize;     /* allocation size (for free) */
    LONG   Width;
    LONG   Height;
} XBOXDISP_DEVBMP, *PXBOXDISP_DEVBMP;

/* enable.c */
BOOL APIENTRY DrvEnableDriver(ULONG, ULONG, PDRVENABLEDATA);
DHPDEV APIENTRY DrvEnablePDEV(DEVMODEW*, LPWSTR, ULONG, HSURF*, ULONG, ULONG*,
                              ULONG, DEVINFO*, HDEV, LPWSTR, HANDLE);
VOID  APIENTRY DrvCompletePDEV(DHPDEV, HDEV);
VOID  APIENTRY DrvDisablePDEV(DHPDEV);

/* surface.c */
HSURF APIENTRY DrvEnableSurface(DHPDEV);
VOID  APIENTRY DrvDisableSurface(DHPDEV);
BOOL  APIENTRY DrvAssertMode(DHPDEV, BOOL);

/* screen.c */
ULONG APIENTRY DrvGetModes(HANDLE, ULONG, DEVMODEW*);
BOOL  IntInitScreenInfo(PPDEV, LPDEVMODEW, PGDIINFO, PDEVINFO);

/* palette.c */
BOOL  APIENTRY DrvSetPalette(DHPDEV, PALOBJ*, FLONG, ULONG, ULONG);
BOOL  IntInitDefaultPalette(PPDEV, PDEVINFO);
BOOL  APIENTRY IntSetPalette(DHPDEV, PPALETTEENTRY, ULONG, ULONG);

/* pointer.c */
ULONG APIENTRY DrvSetPointerShape(SURFOBJ*, SURFOBJ*, SURFOBJ*, XLATEOBJ*,
                                  LONG, LONG, LONG, LONG, RECTL*, FLONG);
VOID  APIENTRY DrvMovePointer(SURFOBJ*, LONG, LONG, RECTL*);

/* accel.c */
BOOL  APIENTRY DrvBitBlt(SURFOBJ*, SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                         RECTL*, POINTL*, POINTL*, BRUSHOBJ*, POINTL*, ROP4);
BOOL  APIENTRY DrvCopyBits(SURFOBJ*, SURFOBJ*, CLIPOBJ*, XLATEOBJ*,
                           RECTL*, POINTL*);
ULONG APIENTRY DrvEscape(SURFOBJ*, ULONG, ULONG, PVOID, ULONG, PVOID);
HBITMAP APIENTRY DrvCreateDeviceBitmap(DHPDEV, SIZEL, ULONG);
VOID  APIENTRY DrvDeleteDeviceBitmap(DHSURF);

/* dd.c (DirectDraw stubs) */
BOOL  APIENTRY DrvEnableDirectDraw(DHPDEV, DD_CALLBACKS*, DD_SURFACECALLBACKS*,
                                   DD_PALETTECALLBACKS*);
VOID  APIENTRY DrvDisableDirectDraw(DHPDEV);

#endif /* _XBOXDISP_PCH_ */
