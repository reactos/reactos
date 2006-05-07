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


HRESULT Hal_DirectDraw_Initialize (LPDIRECTDRAW7 iface)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	/* HAL Startup process */
    BOOL newmode = FALSE;	
	    	

    /* 
      Startup DX HAL step one of three 
    */
    if (!DdCreateDirectDrawObject(&This->mDDrawGlobal, This->hdc))
    {
       DxHeapMemFree(This->mpModeInfos);	   
       DeleteDC(This->hdc);       
       return DD_FALSE;
    }
	
    // Do not relase HDC it have been map in kernel mode 
    // DeleteDC(hdc);
      
    if (!DdReenableDirectDrawObject(&This->mDDrawGlobal, &newmode))
    {
      DxHeapMemFree(This->mpModeInfos);
      DeleteDC(This->hdc);      
      return DD_FALSE;
    }
           
	
    /*
       Startup DX HAL step two of three 
    */

    if (!DdQueryDirectDrawObject(&This->mDDrawGlobal,
                                 &This->mHALInfo,
                                 &This->mCallbacks.HALDD,
                                 &This->mCallbacks.HALDDSurface,
                                 &This->mCallbacks.HALDDPalette, 
                                 &This->mD3dCallbacks,
                                 &This->mD3dDriverData,
                                 &This->mD3dBufferCallbacks,
                                 NULL,
                                 NULL,
                                 NULL))
    {
      DxHeapMemFree(This->mpModeInfos);
      DeleteDC(This->hdc);      
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    This->mcvmList = This->mHALInfo.vmiData.dwNumHeaps;
    This->mpvmList = (VIDMEM*) DxHeapMemAlloc(sizeof(VIDMEM) * This->mcvmList);
    if (This->mpvmList == NULL)
    {      
      DxHeapMemFree(This->mpModeInfos);
      DeleteDC(This->hdc);      
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    This->mcFourCC = This->mHALInfo.ddCaps.dwNumFourCCCodes;
    This->mpFourCC = (DWORD *) DxHeapMemAlloc(sizeof(DWORD) * This->mcFourCC);
    if (This->mpFourCC == NULL)
    {
      DxHeapMemFree(This->mpvmList);
      DxHeapMemFree(This->mpModeInfos);
      DeleteDC(This->hdc);      
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    This->mcTextures = This->mD3dDriverData.dwNumTextureFormats;
    This->mpTextures = (DDSURFACEDESC*) DxHeapMemAlloc(sizeof(DDSURFACEDESC) * This->mcTextures);
    if (This->mpTextures == NULL)
    {      
      DxHeapMemFree( This->mpFourCC);
      DxHeapMemFree( This->mpvmList);
      DxHeapMemFree( This->mpModeInfos);
      DeleteDC(This->hdc);      
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    This->mHALInfo.vmiData.pvmList = This->mpvmList;
    This->mHALInfo.lpdwFourCC = This->mpFourCC;
    This->mD3dDriverData.lpTextureFormats = This->mpTextures;

    if (!DdQueryDirectDrawObject(
                                    &This->mDDrawGlobal,
                                    &This->mHALInfo,
                                    &This->mCallbacks.HALDD,
                                    &This->mCallbacks.HALDDSurface,
                                    &This->mCallbacks.HALDDPalette, 
                                    &This->mD3dCallbacks,
                                    &This->mD3dDriverData,
                                    &This->mCallbacks.HALDDExeBuf,
                                    This->mpTextures,
                                    This->mpFourCC,
                                    This->mpvmList))
  
    {
      DxHeapMemFree(This->mpTextures);
      DxHeapMemFree(This->mpFourCC);
      DxHeapMemFree(This->mpvmList);
      DxHeapMemFree(This->mpModeInfos);
      DeleteDC(This->hdc);      
	  // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

   /*
      Copy over from HalInfo to DirectDrawGlobal
   */

  // this is wrong, cDriverName need be in ASC code not UNICODE 
  //memcpy(mDDrawGlobal.cDriverName, mDisplayAdapter, sizeof(wchar)*MAX_DRIVER_NAME);

  memcpy(&This->mDDrawGlobal.vmiData, &This->mHALInfo.vmiData,sizeof(VIDMEMINFO));
  memcpy(&This->mDDrawGlobal.ddCaps,  &This->mHALInfo.ddCaps,sizeof(DDCORECAPS));

  This->mHALInfo.dwNumModes = This->mcModeInfos;
  This->mHALInfo.lpModeInfo = This->mpModeInfos;
  This->mHALInfo.dwMonitorFrequency = This->mpModeInfos[0].wRefreshRate;

  This->mDDrawGlobal.dwMonitorFrequency = This->mHALInfo.dwMonitorFrequency;
  This->mDDrawGlobal.dwModeIndex        = This->mHALInfo.dwModeIndex;
  This->mDDrawGlobal.dwNumModes         = This->mHALInfo.dwNumModes;
  This->mDDrawGlobal.lpModeInfo         = This->mHALInfo.lpModeInfo;
  This->mDDrawGlobal.hInstance          = This->mHALInfo.hInstance;    
  
  This->mDDrawGlobal.lp16DD = &This->mDDrawGlobal;
  
  //DeleteDC(This->hdc);

   DDHAL_GETDRIVERINFODATA DriverInfo;
   memset(&DriverInfo,0, sizeof(DDHAL_GETDRIVERINFODATA));
   DriverInfo.dwSize = sizeof(DDHAL_GETDRIVERINFODATA);
   DriverInfo.dwContext = This->mDDrawGlobal.hDD; 

  /* Get the MiscellaneousCallbacks  */    
  DriverInfo.guidInfo = GUID_MiscellaneousCallbacks;
  DriverInfo.lpvData = &This->mDDrawGlobal.lpDDCBtmp->HALDDMiscellaneous;
  DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
  This->mHALInfo.GetDriverInfo(&DriverInfo);

  return DD_OK;
}

VOID 
Hal_DirectDraw_Release (LPDIRECTDRAW7 iface) 
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	if (This->mDDrawGlobal.hDD != 0)
	{
     DdDeleteDirectDrawObject (&This->mDDrawGlobal);
	}

	if (This->mpTextures != NULL)
	{
	  DxHeapMemFree(This->mpTextures);
	}

	if (This->mpFourCC != NULL)
	{
	  DxHeapMemFree(This->mpFourCC);
	}

	if (This->mpvmList != NULL)
	{
	  DxHeapMemFree(This->mpvmList);
	}

	if (This->mpModeInfos != NULL)
	{
	  DxHeapMemFree(This->mpModeInfos);
	}

	if (This->hdc != NULL)
	{
	  DeleteDC(This->hdc);
	}
        
}







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

HRESULT Hal_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface) 
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

    DDHAL_FLIPTOGDISURFACEDATA FlipGdi;

    if (!(This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_FLIPTOGDISURFACE)) 
    {
        return DDERR_NODRIVERSUPPORT;
    }

    FlipGdi.lpDD = &This->mDDrawGlobal;
    FlipGdi.ddRVal = DDERR_NOTPALETTIZED;

    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.FlipToGDISurface(&FlipGdi) != DDHAL_DRIVER_HANDLED)
    {
       return DDERR_NODRIVERSUPPORT;
    }
    
    /* FIXME where should FlipGdi.dwToGDI be fill in */
    return  FlipGdi.ddRVal;    
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


