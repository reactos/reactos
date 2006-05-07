/* $Id: surface_hel.c 21519 2006-04-08 21:05:49Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/soft/surface.c
 * PURPOSE:              DirectDraw Software Implementation 
 * PROGRAMMER:           Magnus Olsen
 *
 */

#include "rosdraw.h"


extern  DDPIXELFORMAT pixelformats[];
extern DWORD pixelformatsCount;

/*
 * Direct Draw Callbacks 
 */
 
DWORD CALLBACK  HelDdDestroyDriver(LPDDHAL_DESTROYDRIVERDATA lpDestroyDriver)
{
   	DX_STUB;
}

DWORD CALLBACK  HelDdCreateSurface(LPDDHAL_CREATESURFACEDATA lpCreateSurface)
{
   	DX_STUB;
}

/*
DWORD CALLBACK  HelDdSetColorKey(LPDDHAL_SETCOLORKEYDATA lpSetColorKey)
{
   	DX_STUB;
}
*/

DWORD CALLBACK  HelDdSetMode(LPDDHAL_SETMODEDATA SetMode)
{
   	DX_STUB;
}

DWORD CALLBACK  HelDdWaitForVerticalBlank(LPDDHAL_WAITFORVERTICALBLANKDATA lpWaitForVerticalBlank)
{
   	DX_STUB;
}

DWORD CALLBACK  HelDdCanCreateSurface(LPDDHAL_CANCREATESURFACEDATA lpCanCreateSurface)
{
   	DX_STUB;
}

DWORD CALLBACK  HelDdCreatePalette(LPDDHAL_CREATEPALETTEDATA lpCreatePalette)
{
   	DX_STUB;
}

DWORD CALLBACK  HelDdGetScanLine(LPDDHAL_GETSCANLINEDATA lpGetScanLine)
{
   	DX_STUB;
}

DWORD CALLBACK  HelDdSetExclusiveMode(LPDDHAL_SETEXCLUSIVEMODEDATA lpSetExclusiveMode)
{
	 DX_WINDBG_trace();
	 DX_STUB_str("Not implement yet, return DD_OK for not bsod\n");
	 lpSetExclusiveMode->ddRVal = DD_OK;
	 
	 return DDHAL_DRIVER_HANDLED;
}

DWORD CALLBACK  HelDdFlipToGDISurface(LPDDHAL_FLIPTOGDISURFACEDATA lpFlipToGDISurface)
{
   	DX_STUB;
}

