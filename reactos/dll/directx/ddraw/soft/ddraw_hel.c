/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/soft/ddraw.c
 * PURPOSE:              DirectDraw Software Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"




VOID Hel_DirectDraw_Release (LPDIRECTDRAW7 iface) 
{
}


HRESULT Hel_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
				   LPDWORD total, LPDWORD free)	
{
	IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	*total = HEL_GRAPHIC_MEMORY_MAX;
    *free = This->HELMemoryAvilable;
	return DD_OK;
}


HRESULT Hel_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,HANDLE h) 
{
	DX_STUB;
}

HRESULT Hel_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine)
{
	DX_STUB;
}

HRESULT Hel_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface) 
{
	DX_STUB;
}

HRESULT Hel_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 iface, DWORD dwWidth, DWORD dwHeight, 
													DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	// this only for exclusive mode
	if(!(This->cooperative_level & DDSCL_EXCLUSIVE))
   		return DDERR_NOEXCLUSIVEMODE;

	// change the resolution using normal WinAPI function
	DEVMODE mode;
	mode.dmSize = sizeof(mode);
	mode.dmPelsWidth = dwWidth;
	mode.dmPelsHeight = dwHeight;
	mode.dmBitsPerPel = dwBPP;
    
	//mode.dmDisplayFrequency = dwRefreshRate;
	mode.dmFields = 0;

    DX_STUB_str("in hel");

	if(dwWidth)
		mode.dmFields |= DM_PELSWIDTH;
	if(dwHeight)
		mode.dmFields |= DM_PELSHEIGHT;
	if(dwBPP)
		mode.dmFields |= DM_BITSPERPEL;
    /*
	if(dwRefreshRate)
		mode.dmFields |= DM_DISPLAYFREQUENCY;
    */
	if (ChangeDisplaySettings(&mode, CDS_FULLSCREEN) != DISP_CHANGE_SUCCESSFUL)
		return DDERR_UNSUPPORTEDMODE;
	
    
	// TODO: reactivate ddraw object, maximize window, set it in foreground 
	// and set excluive mode (if implemented by the driver)

	/* FIXME fill the DirectDrawGlobal right the modeindex old and new */

	//if(dwWidth)
	//	This->Height = dwWidth;
	//if(dwHeight)
	//	This->Width = dwHeight;
	//if(dwBPP)
	//	This->Bpp = dwBPP;

	return DD_OK;
}


