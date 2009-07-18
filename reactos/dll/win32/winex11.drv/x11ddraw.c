/*
 * DirectDraw driver interface
 *
 * Copyright 2001 TransGaming Technologies, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "config.h"

#include <stdarg.h>
#include <string.h>
#include <X11/Xlib.h>

#define NONAMELESSUNION
#define NONAMELESSSTRUCT
#include "x11drv.h"

#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "wine/debug.h"

WINE_DEFAULT_DEBUG_CHANNEL(x11drv);

typedef struct _X11DRIVERINFO {
  const GUID *		lpGuid;
  DWORD			dwSize;
  LPVOID		lpvData;
  struct _X11DRIVERINFO*lpNext;
} X11DRIVERINFO,*LPX11DRIVERINFO;

typedef struct _X11DEVICE {
  LPX11DRIVERINFO	lpInfo;
} X11DEVICE,*LPX11DEVICE;

static LPDDRAWI_DDRAWSURFACE_LCL X11DRV_DD_Primary;
static LPDDRAWI_DDRAWSURFACE_GBL X11DRV_DD_PrimaryGbl;
static HWND X11DRV_DD_PrimaryWnd;
static HBITMAP X11DRV_DD_PrimaryDIB;
static Drawable X11DRV_DD_PrimaryDrawable;
static ATOM X11DRV_DD_UserClass;
static UINT X11DRV_DD_GrabMessage;
static WNDPROC X11DRV_DD_GrabOldProcedure;

static void X11DRV_DDHAL_SetPalEntries(Colormap pal, DWORD dwBase, DWORD dwNumEntries,
                                       LPPALETTEENTRY lpEntries);

static void SetPrimaryDIB(HBITMAP hBmp)
{
  X11DRV_DD_PrimaryDIB = hBmp;
  if (hBmp) {
    X11DRV_DD_PrimaryDrawable = X11DRV_get_pixmap( hBmp );
  } else {
    X11DRV_DD_PrimaryDrawable = 0;
  }
}

static LRESULT WINAPI GrabWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
  struct x11drv_thread_data *data = x11drv_thread_data();

  if(message != X11DRV_DD_GrabMessage)
    return CallWindowProcA(X11DRV_DD_GrabOldProcedure, hWnd, message, wParam, lParam);

  TRACE("hwnd=%p, grab=%ld\n", hWnd, wParam);

  if (!data) return 0;

  if (wParam)
  {
    /* find the X11 window that ddraw uses */
    Window win = X11DRV_get_whole_window(hWnd);
    TRACE("X11 window: %ld\n", win);
    if (!win) {
      TRACE("host off desktop\n");
      win = root_window;
    }

    wine_tsx11_lock();
    XGrabPointer(data->display, win, True, 0, GrabModeAsync, GrabModeAsync, win, None, CurrentTime);
    wine_tsx11_unlock();
    data->grab_window = win;
  }
  else
  {
    wine_tsx11_lock();
    XUngrabPointer(data->display, CurrentTime);
    wine_tsx11_unlock();
    data->grab_window = None;
  }

  return 0;
}

static void GrabPointer(BOOL grab)
{
  if(grab) {
    Display *display = thread_display();
    Window window = X11DRV_get_whole_window(GetFocus());
    if(window && display)
    {
      wine_tsx11_lock();
      XSetInputFocus(display, window, RevertToParent, CurrentTime);
      wine_tsx11_unlock();
    }
  }

  if(!X11DRV_DD_GrabMessage)
    X11DRV_DD_GrabMessage = RegisterWindowMessageA("WINE_X11DRV_GRABPOINTER");

  X11DRV_DD_GrabOldProcedure = (WNDPROC)SetWindowLongPtrA(X11DRV_DD_PrimaryWnd,
                                                       GWLP_WNDPROC, (LONG_PTR)GrabWndProc);

  SendMessageW(X11DRV_DD_PrimaryWnd, X11DRV_DD_GrabMessage, grab, 0);

  if(SetWindowLongPtrA(X11DRV_DD_PrimaryWnd, GWLP_WNDPROC,
                    (LONG_PTR)X11DRV_DD_GrabOldProcedure) != (LONG_PTR)GrabWndProc)
    ERR("Window procedure has been changed!\n");
}

static DWORD PASCAL X11DRV_DDHAL_DestroyDriver(LPDDHAL_DESTROYDRIVERDATA data)
{
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DWORD PASCAL X11DRV_DDHAL_CreateSurface(LPDDHAL_CREATESURFACEDATA data)
{
  if (data->lpDDSurfaceDesc->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE) {
    X11DRV_DD_Primary = *data->lplpSList;
    X11DRV_DD_PrimaryWnd = X11DRV_DD_Primary->lpSurfMore->lpDDRAWReserved;
    X11DRV_DD_PrimaryGbl = X11DRV_DD_Primary->lpGbl;
    SetPrimaryDIB((HBITMAP)GET_LPDDRAWSURFACE_GBL_MORE(X11DRV_DD_PrimaryGbl)->hKernelSurface);
    X11DRV_DD_UserClass = GlobalFindAtomA("WINE_DDRAW");
    if (dxgrab) GrabPointer(TRUE);
  }
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_NOTHANDLED;
}

static DWORD PASCAL X11DRV_DDHAL_CreatePalette(LPDDHAL_CREATEPALETTEDATA data)
{
  FIXME("stub\n");
  /* only makes sense to do anything if the X server is running at 8bpp,
   * which few people do nowadays */
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DDHAL_DDCALLBACKS hal_ddcallbacks = {
  sizeof(DDHAL_DDCALLBACKS),
  0x3ff, /* all callbacks are 32-bit */
  X11DRV_DDHAL_DestroyDriver,
  X11DRV_DDHAL_CreateSurface,
  NULL, /* SetColorKey */
  NULL, /* SetMode */
  NULL, /* WaitForVerticalBlank */
  NULL, /* CanCreateSurface */
  X11DRV_DDHAL_CreatePalette,
  NULL, /* GetScanLine */
  NULL, /* SetExclusiveMode */
  NULL  /* FlipToGDISurface */
};

static DWORD PASCAL X11DRV_DDHAL_DestroySurface(LPDDHAL_DESTROYSURFACEDATA data)
{
  if (data->lpDDSurface == X11DRV_DD_Primary) {
    if (dxgrab) GrabPointer(FALSE);
    X11DRV_DD_Primary = NULL;
    X11DRV_DD_PrimaryWnd = 0;
    X11DRV_DD_PrimaryGbl = NULL;
    SetPrimaryDIB(0);
    X11DRV_DD_UserClass = 0;
  }
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DWORD PASCAL X11DRV_DDHAL_SetPalette(LPDDHAL_SETPALETTEDATA data)
{
  if (data->lpDDPalette && data->lpDDPalette->u1.dwReserved1) {
    if (data->lpDDSurface == X11DRV_DD_Primary) {
      FIXME("stub\n");
      /* we should probably find the ddraw window (maybe data->lpDD->lpExclusiveOwner->hWnd),
       * and attach the palette to it
       *
       * Colormap pal = data->lpDDPalette->u1.dwReserved1;
       */
    }
  }
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DDHAL_DDSURFACECALLBACKS hal_ddsurfcallbacks = {
  sizeof(DDHAL_DDSURFACECALLBACKS),
  0x3fff, /* all callbacks are 32-bit */
  X11DRV_DDHAL_DestroySurface,
  NULL, /* Flip */
  NULL, /* SetClipList */
  NULL, /* Lock */
  NULL, /* Unlock */
  NULL, /* Blt */
  NULL, /* SetColorKey */
  NULL, /* AddAttachedSurface */
  NULL, /* GetBltStatus */
  NULL, /* GetFlipStatus */
  NULL, /* UpdateOverlay */
  NULL, /* SetOverlayPosition */
  NULL, /* reserved4 */
  X11DRV_DDHAL_SetPalette
};

static DWORD PASCAL X11DRV_DDHAL_DestroyPalette(LPDDHAL_DESTROYPALETTEDATA data)
{
  Colormap pal = data->lpDDPalette->u1.dwReserved1;
  if (pal)
  {
      wine_tsx11_lock();
      XFreeColormap(gdi_display, pal);
      wine_tsx11_unlock();
  }
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DWORD PASCAL X11DRV_DDHAL_SetPaletteEntries(LPDDHAL_SETENTRIESDATA data)
{
  X11DRV_DDHAL_SetPalEntries(data->lpDDPalette->u1.dwReserved1,
			     data->dwBase, data->dwNumEntries,
			     data->lpEntries);
  data->ddRVal = DD_OK;
  return DDHAL_DRIVER_HANDLED;
}

static DDHAL_DDPALETTECALLBACKS hal_ddpalcallbacks = {
  sizeof(DDHAL_DDPALETTECALLBACKS),
  0x3, /* all callbacks are 32-bit */
  X11DRV_DDHAL_DestroyPalette,
  X11DRV_DDHAL_SetPaletteEntries
};

static X11DEVICE x11device = {
  NULL
};

static DWORD PASCAL X11DRV_DDHAL_GetDriverInfo(LPDDHAL_GETDRIVERINFODATA data)
{
  LPX11DRIVERINFO info = x11device.lpInfo;
  while (info) {
    if (IsEqualGUID(&data->guidInfo, info->lpGuid)) {
      DWORD dwSize = info->dwSize;
      data->dwActualSize = dwSize;
      if (data->dwExpectedSize < dwSize) dwSize = data->dwExpectedSize;
      memcpy(data->lpvData, info->lpvData, dwSize);
      data->ddRVal = DD_OK;
      return DDHAL_DRIVER_HANDLED;
    }
    info = info->lpNext;
  }
  data->ddRVal = DDERR_CURRENTLYNOTAVAIL;
  return DDHAL_DRIVER_HANDLED;
}

static DDHALINFO hal_info = {
  sizeof(DDHALINFO),
  &hal_ddcallbacks,
  &hal_ddsurfcallbacks,
  &hal_ddpalcallbacks,
  {	/* vmiData */
   0	 /* fpPrimary */
  },
  {	/* ddCaps (only stuff the HAL implements here) */
   sizeof(DDCORECAPS),							/* dwSize */
   DDCAPS_GDI | DDCAPS_PALETTE,						/* dwCaps */
   DDCAPS2_CERTIFIED | DDCAPS2_NONLOCALVIDMEM | DDCAPS2_NOPAGELOCKREQUIRED | DDCAPS2_CANRENDERWINDOWED |
   DDCAPS2_WIDESURFACES | DDCAPS2_PRIMARYGAMMA | DDCAPS2_FLIPNOVSYNC,   /* dwCaps2 */
   0,									/* dwCKeyCaps */
   0,									/* dwFXCaps */
   0,									/* dwFXAlphaCaps */
   DDPCAPS_8BIT | DDPCAPS_PRIMARYSURFACE,				/* dwPalCaps */
   0,									/* dwSVCaps */
   0,									/* dwAlphaBltConstBitDepths */
   0,									/* dwAlphaBltPixelBitDepths */
   0,									/* dwAlphaBltSurfaceBitDepths */
   0,									/* dwAlphaOverlayBltConstBitDepths */
   0,									/* dwAlphaOverlayBltPixelBitDepths */
   0,									/* dwAlphaOverlayBltSurfaceBitDepths */
   0,									/* dwZBufferBitDepths */
   16*1024*1024,							/* dwVidMemTotal */
   16*1024*1024,							/* dwVidMemFree */
   0,									/* dwMaxVisibleOverlays */
   0,									/* dwCurrVisibleOverlays */
   0,									/* dwNumFourCCCodes */
   0,									/* dwAlignBoundarySrc */
   0,									/* dwAlignSizeSrc */
   0,									/* dwAlignBoundaryDest */
   0,									/* dwAlignSizeDest */
   0,									/* dwAlignStrideAlign */
   {0},									/* dwRops */
   {									/* ddsCaps */
    DDSCAPS_BACKBUFFER | DDSCAPS_FLIP | DDSCAPS_FRONTBUFFER |
    DDSCAPS_OFFSCREENPLAIN | DDSCAPS_PALETTE | DDSCAPS_PRIMARYSURFACE |
    DDSCAPS_SYSTEMMEMORY | DDSCAPS_TEXTURE | DDSCAPS_VIDEOMEMORY |
    DDSCAPS_VISIBLE | DDSCAPS_LOCALVIDMEM | DDSCAPS_NONLOCALVIDMEM	/* dwCaps */
   },
   0,									/* dwMinOverlayStretch */
   0,									/* dwMaxOverlayStretch */
   0,									/* dwMinLiveVideoStretch */
   0,									/* dwMaxLiveVideoStretch */
   0,									/* dwMinHwCodecStretch */
   0,									/* dwMaxHwCodecStretch */
   0,									/* dwReserved1 */
   0,									/* dwReserved2 */
   0,									/* dwReserved2 */
   0,									/* dwSVBCaps */
   0,									/* dwSVBCKeyCaps */
   0,									/* dwSVBFXCaps */
   {0},									/* dwSVBRops */
   0,									/* dwVSBCaps */
   0,									/* dwVSBCKeyCaps */
   0,									/* dwVSBFXCaps */
   {0},									/* dwVSBRops */
   0,									/* dwSSBCaps */
   0,									/* dwSSBCKeyCaps */
   0,									/* dwSSBFXCaps */
   {0},									/* dwSSBRops */
   0,									/* dwMaxVideoPorts */
   0,									/* dwCurrVideoPorts */
   0									/* dwSVBCaps */
  },
  0,	/* dwMonitorFrequency */
  X11DRV_DDHAL_GetDriverInfo,
  0,	/* dwModeIndex */
  NULL,	/* lpdwFourCC */
  0,	/* dwNumModes */
  NULL,	/* lpModeInfo */
  DDHALINFO_ISPRIMARYDISPLAY | DDHALINFO_MODEXILLEGAL | DDHALINFO_GETDRIVERINFOSET, /* dwFlags */
  &x11device,
  0,	/* hInstance */
  0,	/* lpD3DGlobalDriverData */
  0,	/* lpD3DHALCallbacks */
  NULL	/* lpDDExeBufCallbacks */
};

static LPDDHALDDRAWFNS ddraw_fns;
static DWORD ddraw_ver;

static void X11DRV_DDHAL_SetInfo(void)
{
  (ddraw_fns->lpSetInfo)(&hal_info, FALSE);
}

INT X11DRV_DCICommand(INT cbInput, const DCICMD *lpCmd, LPVOID lpOutData)
{
  TRACE("(%d,(%d,%d,%d),%p)\n", cbInput, lpCmd->dwCommand,
	lpCmd->dwParam1, lpCmd->dwParam2, lpOutData);

  switch (lpCmd->dwCommand) {
  case DDNEWCALLBACKFNS:
    ddraw_fns = (LPDDHALDDRAWFNS)lpCmd->dwParam1;
    return TRUE;
  case DDVERSIONINFO:
    {
      LPDDVERSIONDATA lpVer = lpOutData;
      ddraw_ver = lpCmd->dwParam1;
      if (!lpVer) break;
      /* well, whatever... the DDK says so */
      lpVer->dwHALVersion = DD_RUNTIME_VERSION;
    }
    return TRUE;
  case DDGET32BITDRIVERNAME:
    {
      LPDD32BITDRIVERDATA lpData = (LPDD32BITDRIVERDATA)lpOutData;
      /* here, we could ask ddraw to load a separate DLL, that
       * would contain the 32-bit ddraw HAL */
      strcpy(lpData->szName,"x11drv");
      /* the entry point named here should initialize our hal_info
       * with 32-bit entry points (ignored for now) */
      strcpy(lpData->szEntryPoint,"DriverInit");
      lpData->dwContext = 0;
    }
    return TRUE;
  case DDCREATEDRIVEROBJECT:
    {
      LPDWORD lpInstance = lpOutData;

      /* FIXME: get x11drv's hInstance */
      X11DRV_Settings_CreateDriver(&hal_info);

      (ddraw_fns->lpSetInfo)(&hal_info, FALSE);
      *lpInstance = hal_info.hInstance;
    }
    return TRUE;
  }
  return 0;
}

void X11DRV_DDHAL_SwitchMode(DWORD dwModeIndex, LPVOID fb_addr, LPVIDMEM fb_mem)
{
  LPDDHALMODEINFO info = &hal_info.lpModeInfo[dwModeIndex];

  hal_info.dwModeIndex        = dwModeIndex;
  hal_info.dwMonitorFrequency = info->wRefreshRate;
  hal_info.vmiData.fpPrimary  = (FLATPTR)fb_addr;
  hal_info.vmiData.dwDisplayWidth  = info->dwWidth;
  hal_info.vmiData.dwDisplayHeight = info->dwHeight;
  hal_info.vmiData.lDisplayPitch   = info->lPitch;
  hal_info.vmiData.ddpfDisplay.dwSize = info->dwBPP ? sizeof(hal_info.vmiData.ddpfDisplay) : 0;
  hal_info.vmiData.ddpfDisplay.dwFlags = (info->wFlags & DDMODEINFO_PALETTIZED) ? DDPF_PALETTEINDEXED8 : 0;
  hal_info.vmiData.ddpfDisplay.u1.dwRGBBitCount = (info->dwBPP > 24) ? 24 : info->dwBPP;
  hal_info.vmiData.ddpfDisplay.u2.dwRBitMask = info->dwRBitMask;
  hal_info.vmiData.ddpfDisplay.u3.dwGBitMask = info->dwGBitMask;
  hal_info.vmiData.ddpfDisplay.u4.dwBBitMask = info->dwBBitMask;
  hal_info.vmiData.dwNumHeaps = fb_mem ? 1 : 0;
  hal_info.vmiData.pvmList = fb_mem;

  X11DRV_DDHAL_SetInfo();
}

static void X11DRV_DDHAL_SetPalEntries(Colormap pal, DWORD dwBase, DWORD dwNumEntries,
                                       LPPALETTEENTRY lpEntries)
{
  XColor c;
  unsigned int n;

  if (pal) {
    wine_tsx11_lock();
    c.flags = DoRed|DoGreen|DoBlue;
    c.pixel = dwBase;
    for (n=0; n<dwNumEntries; n++,c.pixel++) {
      c.red   = lpEntries[n].peRed   << 8;
      c.green = lpEntries[n].peGreen << 8;
      c.blue  = lpEntries[n].peBlue  << 8;
      XStoreColor(gdi_display, pal, &c);
    }
    XFlush(gdi_display); /* update display immediately */
    wine_tsx11_unlock();
  }
}
