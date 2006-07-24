/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/hal/ddraw.c
 * PURPOSE:              DirectDraw HAL Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"




HRESULT 
Hal_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
                   LPDWORD total, LPDWORD free)                                               
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    
    DDHAL_GETAVAILDRIVERMEMORYDATA  mem;

    if (!(This->mDDrawGlobal.lpDDCBtmp->HALDDMiscellaneous.dwFlags & DDHAL_MISCCB32_GETAVAILDRIVERMEMORY)) 
    {       
       return DDERR_NODRIVERSUPPORT;
    }

    mem.lpDD = &This->mDDrawGlobal;    
    mem.ddRVal = DDERR_NOTPALETTIZED;

    if (This->mDDrawGlobal.lpDDCBtmp->HALDDMiscellaneous.GetAvailDriverMemory(&mem) != DDHAL_DRIVER_HANDLED)
    {	
      return DDERR_NODRIVERSUPPORT;
    }

    ddscaps->dwCaps = mem.DDSCaps.dwCaps;
    ddscaps->dwCaps2 = mem.ddsCapsEx.dwCaps2;
    ddscaps->dwCaps3 = mem.ddsCapsEx.dwCaps3;
    ddscaps->dwCaps4 = mem.ddsCapsEx.dwCaps4;
    *total = mem.dwTotal;
    *free = mem.dwFree;
    
    return mem.ddRVal;
}

HRESULT Hal_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,HANDLE h) 
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

    DDHAL_WAITFORVERTICALBLANKDATA WaitVectorData;

    if (!(This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK)) 
    {
        return DDERR_NODRIVERSUPPORT;
    }
      
    WaitVectorData.lpDD = &This->mDDrawGlobal;
    WaitVectorData.dwFlags = dwFlags;
    WaitVectorData.hEvent = (DWORD)h;
    WaitVectorData.ddRVal = DDERR_NOTPALETTIZED;

    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.WaitForVerticalBlank(&WaitVectorData) != DDHAL_DRIVER_HANDLED)
    {
       return DDERR_NODRIVERSUPPORT;
    }

    return WaitVectorData.ddRVal;
}

HRESULT Hal_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

    DDHAL_GETSCANLINEDATA GetScan;

    if (!(This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_GETSCANLINE)) 
    {
        return DDERR_NODRIVERSUPPORT;
    }

    GetScan.lpDD = &This->mDDrawGlobal;
    GetScan.ddRVal = DDERR_NOTPALETTIZED;

    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.GetScanLine(&GetScan) != DDHAL_DRIVER_HANDLED)
    {
       return DDERR_NODRIVERSUPPORT;
    }

    *lpdwScanLine = GetScan.ddRVal;
    return  GetScan.ddRVal;
}





HRESULT Hal_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 iface, DWORD dwWidth, DWORD dwHeight, 
                                                    DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;	
	DDHAL_SETMODEDATA mode;

    if (!(This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_SETMODE)) 
    {
        return DDERR_NODRIVERSUPPORT;
    }
    
    mode.lpDD = &This->mDDrawGlobal;
    mode.ddRVal = DDERR_NODRIVERSUPPORT;

	

    // FIXME : add search for which mode.ModeIndex we should use 
    // FIXME : fill the mode.inexcl; 
    // FIXME : fill the mode.useRefreshRate; 

    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.SetMode(&mode) != DDHAL_DRIVER_HANDLED)
    {
        return DDERR_NODRIVERSUPPORT;
    } 
	   
	return mode.ddRVal;
}


