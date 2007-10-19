/*
 *  DirectDraw GDI32.dll interface definitions
 *  Copyright (C) 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <d3dhal.h>

/* Define the real export names */
#define DdCreateDirectDrawObject            GdiEntry1
#define DdQueryDirectDrawObject             GdiEntry2
#define DdDeleteDirectDrawObject            GdiEntry3
#define DdCreateSurfaceObject               GdiEntry4
#define DdDeleteSurfaceObject               GdiEntry5
#define DdResetVisrgn                       GdiEntry6
#define DdGetDC                             GdiEntry7
#define DdReleaseDC                         GdiEntry8
#define DdCreateDIBSection                  GdiEntry9
#define DdReenableDirectDrawObject          GdiEntry10
#define DdAttachSurface                     GdiEntry11
#define DdUnattachSurface                   GdiEntry12
#define DdQueryDisplaySettingsUniqueness    GdiEntry13
#define DdGetDxHandle                       GdiEntry14
#define DdSetGammaRamp                      GdiEntry15
#define DdSwapTextureHandles                GdiEntry16


BOOL
STDCALL
DdCreateDirectDrawObject(
    LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
    HDC hdc
);

BOOL
STDCALL
DdQueryDirectDrawObject(
    LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
    LPDDHALINFO pHalInfo,
    LPDDHAL_DDCALLBACKS pDDCallbacks,
    LPDDHAL_DDSURFACECALLBACKS pDDSurfaceCallbacks,
    LPDDHAL_DDPALETTECALLBACKS pDDPaletteCallbacks,
    LPD3DHAL_CALLBACKS pD3dCallbacks,
    LPD3DHAL_GLOBALDRIVERDATA pD3dDriverData,
    LPDDHAL_DDEXEBUFCALLBACKS pD3dBufferCallbacks,
    LPDDSURFACEDESC pD3dTextureFormats,
    LPDWORD pdwFourCC,
    LPVIDMEM pvmList
);

BOOL
STDCALL
DdDeleteDirectDrawObject(
    LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal
);

BOOL
STDCALL
DdCreateSurfaceObject(
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
    BOOL bPrimarySurface
);

BOOL
STDCALL
DdDeleteSurfaceObject(
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal
);

BOOL
STDCALL
DdResetVisrgn(
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
    HWND hWnd
);

HDC
STDCALL
DdGetDC(
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal,
    LPPALETTEENTRY pColorTable
);

BOOL
STDCALL
DdReleaseDC(
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceLocal
);

HBITMAP
STDCALL
DdCreateDIBSection(
    HDC hdc,
    CONST BITMAPINFO *pbmi,
    UINT iUsage,
    VOID **ppvBits,
    HANDLE hSectionApp,
    DWORD dwOffset
);

BOOL
STDCALL
DdReenableDirectDrawObject(
    LPDDRAWI_DIRECTDRAW_GBL pDirectDrawGlobal,
    BOOL *pbNewMode
);

BOOL
STDCALL
DdAttachSurface(
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceFrom,
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceTo
);

VOID
STDCALL
DdUnattachSurface(
    LPDDRAWI_DDRAWSURFACE_LCL pSurface,
    LPDDRAWI_DDRAWSURFACE_LCL pSurfaceAttached
);

ULONG
STDCALL
DdQueryDisplaySettingsUniqueness(VOID);

HANDLE
STDCALL
DdGetDxHandle(
    LPDDRAWI_DIRECTDRAW_LCL pDDraw,
    LPDDRAWI_DDRAWSURFACE_LCL pSurface,
    BOOL bRelease
);

BOOL
STDCALL
DdSetGammaRamp(
    LPDDRAWI_DIRECTDRAW_LCL pDDraw,
    HDC hdc,
    LPVOID lpGammaRamp
);

DWORD
STDCALL
DdSwapTextureHandles(
    LPDDRAWI_DIRECTDRAW_LCL pDDraw,
    LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl1,
    LPDDRAWI_DDRAWSURFACE_LCL pDDSLcl2
);
