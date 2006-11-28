/* $Id: main.c 21434 2006-04-01 19:12:56Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              DirectDraw Library 
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#include <windows.h>
#include "rosdraw.h"
#include "d3dhal.h"
#include "ddrawgdi.h"

DDRAWI_DIRECTDRAW_GBL ddgbl;
DDRAWI_DDRAWSURFACE_GBL ddSurfGbl;


HRESULT WINAPI 
StartDirectDraw(LPDIRECTDRAW* iface, LPGUID lpGuid)
{
	LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    DWORD hal_ret;
    DWORD hel_ret;
    DEVMODE devmode;
    HBITMAP hbmp;
    const UINT bmiSize = sizeof(BITMAPINFOHEADER) + 0x10;
    UCHAR *pbmiData;
    BITMAPINFO *pbmi;    
    DWORD *pMasks;	
	INT devicetypes = 0;
    
    DX_WINDBG_trace();
	  
	RtlZeroMemory(&ddgbl, sizeof(DDRAWI_DIRECTDRAW_GBL));
    
	ddgbl.lpDDCBtmp = (LPDDHAL_CALLBACKS) DxHeapMemAlloc(sizeof(DDHAL_CALLBACKS));  
	if (ddgbl.lpDDCBtmp == NULL)
	{
	   DX_STUB_str("Out of memmory");
       return DD_FALSE;
	}

	This->lpLcl->lpDDCB = ddgbl.lpDDCBtmp;
	
	
    /* Same for HEL and HAL */
    This->lpLcl->lpGbl->lpModeInfo = (DDHALMODEINFO*) DxHeapMemAlloc(1 * sizeof(DDHALMODEINFO));  
	
    if (This->lpLcl->lpGbl->lpModeInfo == NULL)
    {
	   DX_STUB_str("DD_FALSE");
       return DD_FALSE;
    }

    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);

    This->lpLcl->lpGbl->lpModeInfo[0].dwWidth      = devmode.dmPelsWidth;
    This->lpLcl->lpGbl->lpModeInfo[0].dwHeight     = devmode.dmPelsHeight;
    This->lpLcl->lpGbl->lpModeInfo[0].dwBPP        = devmode.dmBitsPerPel;
    This->lpLcl->lpGbl->lpModeInfo[0].lPitch       = (devmode.dmPelsWidth*devmode.dmBitsPerPel)/8;
    This->lpLcl->lpGbl->lpModeInfo[0].wRefreshRate = (WORD)devmode.dmDisplayFrequency;
   

	if (lpGuid == NULL) 
	{
		devicetypes = 1;

		/* Create HDC for default, hal and hel driver */
		This->lpLcl->hDC =  (ULONG_PTR) CreateDCW(L"DISPLAY",L"DISPLAY",NULL,NULL);    

		/* cObsolete is undoc in msdn it being use in CreateDCA */
	    RtlCopyMemory(&ddgbl.cObsolete,&"DISPLAY",7);
	    RtlCopyMemory(&ddgbl.cDriverName,&"DISPLAY",7);
	}

	else if (lpGuid == (LPGUID) DDCREATE_HARDWAREONLY) 
	{
		devicetypes = 2;

		/* Create HDC for default, hal and hel driver */
		This->lpLcl->hDC = (ULONG_PTR)CreateDCW(L"DISPLAY",L"DISPLAY",NULL,NULL);    

		/* cObsolete is undoc in msdn it being use in CreateDCA */
	    RtlCopyMemory(&ddgbl.cObsolete,&"DISPLAY",7);
	    RtlCopyMemory(&ddgbl.cDriverName,&"DISPLAY",7);
	}

	else if (lpGuid == (LPGUID) DDCREATE_EMULATIONONLY) 
	{
		devicetypes = 3;

		/* Create HDC for default, hal and hel driver */
		This->lpLcl->hDC = (ULONG_PTR) CreateDCW(L"DISPLAY",L"DISPLAY",NULL,NULL);    

		/* cObsolete is undoc in msdn it being use in CreateDCA */
	    RtlCopyMemory(&ddgbl.cObsolete,&"DISPLAY",7);
	    RtlCopyMemory(&ddgbl.cDriverName,&"DISPLAY",7);
	}
	else
	{
		/* FIXME : need getting driver from the GUID that have been pass in from
		           the register. we do not support that yet 
	    */
		devicetypes = 4;
		This->lpLcl->hDC = (ULONG_PTR) NULL ;
	}

    if ( (HDC)This->lpLcl->hDC == NULL)
    {
	   DX_STUB_str("DDERR_OUTOFMEMORY");
       return DDERR_OUTOFMEMORY ;
    }

    hbmp = CreateCompatibleBitmap((HDC) This->lpLcl->hDC, 1, 1);  
    if (hbmp==NULL)
    {
       DxHeapMemFree(This->lpLcl->lpGbl->lpModeInfo);
       DeleteDC((HDC) This->lpLcl->hDC);
	   DX_STUB_str("DDERR_OUTOFMEMORY");
       return DDERR_OUTOFMEMORY;
    }
  
    pbmiData = (UCHAR *) DxHeapMemAlloc(bmiSize);
    pbmi = (BITMAPINFO*)pbmiData;

    if (pbmiData==NULL)
    {
       DxHeapMemFree(This->lpLcl->lpGbl->lpModeInfo);       
       DeleteDC((HDC) This->lpLcl->hDC);
       DeleteObject(hbmp);
	   DX_STUB_str("DDERR_OUTOFMEMORY");
       return DDERR_OUTOFMEMORY;
    }

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biBitCount = (WORD)devmode.dmBitsPerPel;
    pbmi->bmiHeader.biCompression = BI_BITFIELDS;
    pbmi->bmiHeader.biWidth = 1;
    pbmi->bmiHeader.biHeight = 1;

    GetDIBits((HDC) This->lpLcl->hDC, hbmp, 0, 0, NULL, pbmi, 0);
    DeleteObject(hbmp);

    pMasks = (DWORD*)(pbmiData + sizeof(BITMAPINFOHEADER));
    This->lpLcl->lpGbl->lpModeInfo[0].dwRBitMask = pMasks[0];
    This->lpLcl->lpGbl->lpModeInfo[0].dwGBitMask = pMasks[1];
    This->lpLcl->lpGbl->lpModeInfo[0].dwBBitMask = pMasks[2];
    This->lpLcl->lpGbl->lpModeInfo[0].dwAlphaBitMask = pMasks[3];

	DxHeapMemFree(pbmiData);

    /* Startup HEL and HAL */
   // RtlZeroMemory(&ddgbl, sizeof(DDRAWI_DIRECTDRAW_GBL));
	
	This->lpLcl->lpDDCB = This->lpLcl->lpGbl->lpDDCBtmp;	  
	This->lpLcl->dwProcessId = GetCurrentProcessId();

    

    hal_ret = StartDirectDrawHal(iface);
	hel_ret = StartDirectDrawHel(iface);    
    if ((hal_ret!=DD_OK) &&  (hel_ret!=DD_OK))
    {
		DX_STUB_str("DDERR_NODIRECTDRAWSUPPORT");
        return DDERR_NODIRECTDRAWSUPPORT; 
    }

	This->lpLcl->hDD = This->lpLcl->lpGbl->hDD;


	/* Fixme the mix betwin hel and hal */
	This->lpLcl->lpDDCB->cbDDCallbacks.dwSize = sizeof(This->lpLcl->lpDDCB->cbDDCallbacks);

	if (This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_CANCREATESURFACE)
	{		
		This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_CANCREATESURFACE;
		This->lpLcl->lpDDCB->cbDDCallbacks.CanCreateSurface = This->lpLcl->lpDDCB->HALDD.CanCreateSurface;
	}
	else if (This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_CANCREATESURFACE)
	{		    
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_CANCREATESURFACE;
		     This->lpLcl->lpDDCB->cbDDCallbacks.CanCreateSurface = This->lpLcl->lpDDCB->HELDD.CanCreateSurface;
	}
	if (This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_CREATESURFACE)
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_CREATESURFACE;
		This->lpLcl->lpDDCB->cbDDCallbacks.CreateSurface = This->lpLcl->lpDDCB->HALDD.CreateSurface;
	}
	else if (This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_CREATESURFACE)
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_CREATESURFACE;
		     This->lpLcl->lpDDCB->cbDDCallbacks.CreateSurface = This->lpLcl->lpDDCB->HELDD.CreateSurface;
	}
	if (This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_CREATEPALETTE)
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_CREATEPALETTE;
		This->lpLcl->lpDDCB->cbDDCallbacks.CreatePalette = This->lpLcl->lpDDCB->HALDD.CreatePalette;
	}
	else if (This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_CREATEPALETTE)
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_CREATEPALETTE;
		     This->lpLcl->lpDDCB->cbDDCallbacks.CreatePalette = This->lpLcl->lpDDCB->HELDD.CreatePalette;
	}
	if (This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_DESTROYDRIVER)
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_DESTROYDRIVER;
		This->lpLcl->lpDDCB->cbDDCallbacks.DestroyDriver = This->lpLcl->lpDDCB->HALDD.DestroyDriver;
	}
	else if (This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_DESTROYDRIVER)
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_DESTROYDRIVER;
		     This->lpLcl->lpDDCB->cbDDCallbacks.DestroyDriver = This->lpLcl->lpDDCB->HELDD.DestroyDriver;
	}
	if (This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_FLIPTOGDISURFACE)
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_FLIPTOGDISURFACE;
		This->lpLcl->lpDDCB->cbDDCallbacks.FlipToGDISurface = This->lpLcl->lpDDCB->HALDD.FlipToGDISurface;
	}
	else if (This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_FLIPTOGDISURFACE)
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_FLIPTOGDISURFACE;
			 This->lpLcl->lpDDCB->cbDDCallbacks.FlipToGDISurface = This->lpLcl->lpDDCB->HELDD.FlipToGDISurface;
	}
	if (This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_GETSCANLINE)
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_GETSCANLINE;
		This->lpLcl->lpDDCB->cbDDCallbacks.GetScanLine = This->lpLcl->lpDDCB->HALDD.GetScanLine;
	}
	else if (This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_GETSCANLINE)
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_GETSCANLINE;
		     This->lpLcl->lpDDCB->cbDDCallbacks.GetScanLine = This->lpLcl->lpDDCB->HELDD.GetScanLine;
	}
	if (This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_SETCOLORKEY)
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_SETCOLORKEY;
		This->lpLcl->lpDDCB->cbDDCallbacks.SetColorKey = This->lpLcl->lpDDCB->HALDD.SetColorKey;
	}
	else if (This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_SETCOLORKEY)
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_SETCOLORKEY;
		     This->lpLcl->lpDDCB->cbDDCallbacks.SetColorKey = This->lpLcl->lpDDCB->HELDD.SetColorKey;
	}
	if (This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_SETEXCLUSIVEMODE)
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_SETEXCLUSIVEMODE;
		This->lpLcl->lpDDCB->cbDDCallbacks.SetExclusiveMode = This->lpLcl->lpDDCB->HALDD.SetExclusiveMode;
	}
	else if (This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_SETEXCLUSIVEMODE)
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_SETEXCLUSIVEMODE;
			 This->lpLcl->lpDDCB->cbDDCallbacks.SetExclusiveMode = This->lpLcl->lpDDCB->HELDD.SetExclusiveMode;
	}
	if (This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_SETMODE)
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_SETMODE;
		This->lpLcl->lpDDCB->cbDDCallbacks.SetMode = This->lpLcl->lpDDCB->HALDD.SetMode;
	}
	else if (This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_SETMODE)
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_SETMODE;
			 This->lpLcl->lpDDCB->cbDDCallbacks.SetMode = This->lpLcl->lpDDCB->HELDD.SetMode;
	}
	if (This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK)
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_WAITFORVERTICALBLANK;
		This->lpLcl->lpDDCB->cbDDCallbacks.WaitForVerticalBlank = This->lpLcl->lpDDCB->HALDD.WaitForVerticalBlank;
	}
	else if (This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK)
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags = DDHAL_CB32_WAITFORVERTICALBLANK;
		 	 This->lpLcl->lpDDCB->cbDDCallbacks.WaitForVerticalBlank = This->lpLcl->lpDDCB->HELDD.WaitForVerticalBlank;
	}



		
		                                 
								
								
	

	
	/* Fill some basic info for Surface */
	ddSurfGbl.lpDD = &ddgbl;

		    	   	
    return DD_OK;
}


HRESULT WINAPI 
StartDirectDrawHal(LPDIRECTDRAW* iface)
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
	DDHAL_GETDRIVERINFODATA DriverInfo;

    DDHALINFO mHALInfo;
    DDHAL_CALLBACKS mCallbacks;
    DDHAL_DDEXEBUFCALLBACKS mD3dBufferCallbacks;
    D3DHAL_CALLBACKS mD3dCallbacks;
    D3DHAL_GLOBALDRIVERDATA mD3dDriverData;
    UINT mcvmList;
    VIDMEM *mpvmList;

    UINT mcFourCC;
    DWORD *mpFourCC;
        UINT mcTextures;
    DDSURFACEDESC *mpTextures;

	/* HAL Startup process */
    BOOL newmode = FALSE;	
	    	

    RtlZeroMemory(&mHALInfo, sizeof(DDHALINFO));
    RtlZeroMemory(&mCallbacks, sizeof(DDHAL_CALLBACKS));

    /* 
      Startup DX HAL step one of three 
    */
	if (!DdCreateDirectDrawObject(This->lpLcl->lpGbl, (HDC)This->lpLcl->hDC))
    {
       DxHeapMemFree(This->lpLcl->lpGbl->lpModeInfo);	   
       DeleteDC((HDC)This->lpLcl->hDC);       
       return DD_FALSE;
    }
	
    // Do not relase HDC it have been map in kernel mode 
    // DeleteDC(hdc);
      
    if (!DdReenableDirectDrawObject(This->lpLcl->lpGbl, &newmode))
    {
      DxHeapMemFree(This->lpLcl->lpGbl->lpModeInfo);
      DeleteDC((HDC)This->lpLcl->hDC);      
      return DD_FALSE;
    }
           
	
    /*
       Startup DX HAL step two of three 
    */

    if (!DdQueryDirectDrawObject(This->lpLcl->lpGbl,
                                 &mHALInfo,
                                 &mCallbacks.HALDD,
                                 &mCallbacks.HALDDSurface,
                                 &mCallbacks.HALDDPalette, 
                                 &mD3dCallbacks,
                                 &mD3dDriverData,
                                 &mD3dBufferCallbacks,
                                 NULL,
                                 NULL,
                                 NULL))
    {
      DxHeapMemFree(This->lpLcl->lpGbl->lpModeInfo);
      DeleteDC((HDC)This->lpLcl->hDC);      
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    mcvmList = mHALInfo.vmiData.dwNumHeaps;
    mpvmList = (VIDMEM*) DxHeapMemAlloc(sizeof(VIDMEM) * mcvmList);
    if (mpvmList == NULL)
    {      
      DxHeapMemFree(This->lpLcl->lpGbl->lpModeInfo);
      DeleteDC((HDC)This->lpLcl->hDC);     
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    mcFourCC = mHALInfo.ddCaps.dwNumFourCCCodes;
    mpFourCC = (DWORD *) DxHeapMemAlloc(sizeof(DWORD) * mcFourCC);
    if (mpFourCC == NULL)
    {
      DxHeapMemFree(mpvmList);
      DxHeapMemFree(This->lpLcl->lpGbl->lpModeInfo);
      DeleteDC((HDC)This->lpLcl->hDC);      
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    mcTextures = mD3dDriverData.dwNumTextureFormats;
    mpTextures = (DDSURFACEDESC*) DxHeapMemAlloc(sizeof(DDSURFACEDESC) * mcTextures);
    if (mpTextures == NULL)
    {      
      DxHeapMemFree( mpFourCC);
      DxHeapMemFree( mpvmList);
      DxHeapMemFree( This->lpLcl->lpGbl->lpModeInfo);
      DeleteDC((HDC)This->lpLcl->hDC);     
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    mHALInfo.vmiData.pvmList = mpvmList;
    mHALInfo.lpdwFourCC = mpFourCC;
    mD3dDriverData.lpTextureFormats = (DDSURFACEDESC*) mpTextures;

    if (!DdQueryDirectDrawObject(
                                    This->lpLcl->lpGbl,
                                    &mHALInfo,
                                    &mCallbacks.HALDD,
                                    &mCallbacks.HALDDSurface,
                                    &mCallbacks.HALDDPalette, 
                                    &mD3dCallbacks,
                                    &mD3dDriverData,
                                    &mCallbacks.HALDDExeBuf,
                                    (DDSURFACEDESC*)mpTextures,
                                    mpFourCC,
                                    mpvmList))
  
    {
      DxHeapMemFree(mpTextures);
      DxHeapMemFree(mpFourCC);
      DxHeapMemFree(mpvmList);
      DxHeapMemFree(This->lpLcl->lpGbl->lpModeInfo);
      DeleteDC((HDC)This->lpLcl->hDC);      
	  // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

   /*
      Copy over from HalInfo to DirectDrawGlobal
   */

  // this is wrong, cDriverName need be in ASC code not UNICODE 
  //memcpy(mDDrawGlobal.cDriverName, mDisplayAdapter, sizeof(wchar)*MAX_DRIVER_NAME);

  memcpy(&ddgbl.vmiData, &mHALInfo.vmiData,sizeof(VIDMEMINFO));
  memcpy(&ddgbl.ddCaps,  &mHALInfo.ddCaps,sizeof(DDCORECAPS));
  
  mHALInfo.dwNumModes = 1;
  mHALInfo.lpModeInfo = This->lpLcl->lpGbl->lpModeInfo;
  mHALInfo.dwMonitorFrequency = This->lpLcl->lpGbl->lpModeInfo[0].wRefreshRate;

  This->lpLcl->lpGbl->dwMonitorFrequency = mHALInfo.dwMonitorFrequency;
  This->lpLcl->lpGbl->dwModeIndex        = mHALInfo.dwModeIndex;
  This->lpLcl->lpGbl->dwNumModes         = mHALInfo.dwNumModes;
  This->lpLcl->lpGbl->lpModeInfo         = mHALInfo.lpModeInfo;
  This->lpLcl->lpGbl->hInstance          = mHALInfo.hInstance;    
  
  This->lpLcl->lpGbl->lp16DD = This->lpLcl->lpGbl;
  
   
   memset(&DriverInfo,0, sizeof(DDHAL_GETDRIVERINFODATA));
   DriverInfo.dwSize = sizeof(DDHAL_GETDRIVERINFODATA);
   DriverInfo.dwContext = This->lpLcl->lpGbl->hDD; 

  /* Get the MiscellaneousCallbacks  */    
  DriverInfo.guidInfo = GUID_MiscellaneousCallbacks;
  DriverInfo.lpvData = &ddgbl.lpDDCBtmp->HALDDMiscellaneous;
  DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
  mHALInfo.GetDriverInfo(&DriverInfo);

  
  return DD_OK;
}

HRESULT WINAPI 
StartDirectDrawHel(LPDIRECTDRAW* iface)
{
	LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
	    
	This->lpLcl->lpDDCB->HELDD.CanCreateSurface     = HelDdCanCreateSurface;	
	This->lpLcl->lpDDCB->HELDD.CreateSurface        = HelDdCreateSurface;		
	This->lpLcl->lpDDCB->HELDD.CreatePalette        = HelDdCreatePalette;
	This->lpLcl->lpDDCB->HELDD.DestroyDriver        = HelDdDestroyDriver;
	//This->lpLcl->lpDDCB->HELDD.FlipToGDISurface     = HelDdFlipToGDISurface
	This->lpLcl->lpDDCB->HELDD.GetScanLine          = HelDdGetScanLine;
	// This->lpLcl->lpDDCB->HELDD.SetColorKey          = HelDdSetColorKey;
	This->lpLcl->lpDDCB->HELDD.SetExclusiveMode     = HelDdSetExclusiveMode;
	This->lpLcl->lpDDCB->HELDD.SetMode              = HelDdSetMode;
	This->lpLcl->lpDDCB->HELDD.WaitForVerticalBlank = HelDdWaitForVerticalBlank;

	This->lpLcl->lpDDCB->HELDD.dwFlags =  DDHAL_CB32_CANCREATESURFACE     |
		                                  DDHAL_CB32_CREATESURFACE        |
										  DDHAL_CB32_CREATEPALETTE        |
		                                  DDHAL_CB32_DESTROYDRIVER        | 
										  // DDHAL_CB32_FLIPTOGDISURFACE     |
										  DDHAL_CB32_GETSCANLINE          |
										  // DDHAL_CB32_SETCOLORKEY          |
										  DDHAL_CB32_SETEXCLUSIVEMODE     | 
										  DDHAL_CB32_SETMODE              |                                                   
										  DDHAL_CB32_WAITFORVERTICALBLANK ;


	This->lpLcl->lpDDCB->HELDD.dwSize = sizeof(This->lpLcl->lpDDCB->HELDD);

	/*
	This->lpLcl->lpDDCB->HELDDSurface.AddAttachedSurface = HelDdSurfAddAttachedSurface;
	This->lpLcl->lpDDCB->HELDDSurface.Blt = HelDdSurfBlt;
	This->lpLcl->lpDDCB->HELDDSurface.DestroySurface = HelDdSurfDestroySurface;
	This->lpLcl->lpDDCB->HELDDSurface.Flip = HelDdSurfFlip;
	This->lpLcl->lpDDCB->HELDDSurface.GetBltStatus = HelDdSurfGetBltStatus;
	This->lpLcl->lpDDCB->HELDDSurface.GetFlipStatus = HelDdSurfGetFlipStatus;
	This->lpLcl->lpDDCB->HELDDSurface.Lock = HelDdSurfLock;
	This->lpLcl->lpDDCB->HELDDSurface.reserved4 = HelDdSurfreserved4;
	This->lpLcl->lpDDCB->HELDDSurface.SetClipList = HelDdSurfSetClipList;
	This->lpLcl->lpDDCB->HELDDSurface.SetColorKey = HelDdSurfSetColorKey;
	This->lpLcl->lpDDCB->HELDDSurface.SetOverlayPosition = HelDdSurfSetOverlayPosition;
	This->lpLcl->lpDDCB->HELDDSurface.SetPalette = HelDdSurfSetPalette;
	This->lpLcl->lpDDCB->HELDDSurface.Unlock = HelDdSurfUnlock;
	This->lpLcl->lpDDCB->HELDDSurface.UpdateOverlay = HelDdSurfUpdateOverlay;
    */

	/*
	This->lpLcl->lpDDCB->HELDDPalette.DestroyPalette  = HelDdPalDestroyPalette; 
	This->lpLcl->lpDDCB->HELDDPalette.SetEntries = HelDdPalSetEntries;
	This->lpLcl->lpDDCB->HELDDPalette.dwSize = sizeof(This->lpLcl->lpDDCB->HELDDPalette);
	*/

	/*
	This->lpLcl->lpDDCB->HELDDExeBuf.CanCreateExecuteBuffer = HelDdExeCanCreateExecuteBuffer;
	This->lpLcl->lpDDCB->HELDDExeBuf.CreateExecuteBuffer = HelDdExeCreateExecuteBuffer;
	This->lpLcl->lpDDCB->HELDDExeBuf.DestroyExecuteBuffer = HelDdExeDestroyExecuteBuffer;
	This->lpLcl->lpDDCB->HELDDExeBuf.LockExecuteBuffer = HelDdExeLockExecuteBuffer;
	This->lpLcl->lpDDCB->HELDDExeBuf.UnlockExecuteBuffer = HelDdExeUnlockExecuteBuffer;
	*/
										  										  	   
	return DD_OK;
}

HRESULT 
WINAPI 
Create_DirectDraw (LPGUID pGUID, 
				   LPDIRECTDRAW* pIface, 
				   REFIID id, 
				   BOOL ex)
{   	
    LPDDRAWI_DIRECTDRAW_INT This;

	
	DX_WINDBG_trace();
		
	This = DxHeapMemAlloc(sizeof(DDRAWI_DIRECTDRAW_INT));
	if (This == NULL) 
	{
		return E_OUTOFMEMORY;
	}

	This->lpLcl = DxHeapMemAlloc(sizeof(DDRAWI_DIRECTDRAW_INT));

	if (This->lpLcl == NULL)
	{
		/* FIXME cleanup */
		return DDERR_OUTOFMEMORY;
	}

	This->lpLcl->lpGbl = &ddgbl;
			 
	*pIface = (LPDIRECTDRAW)This;

	if(Main_DirectDraw_QueryInterface((LPDIRECTDRAW7)This, id, (void**)&pIface) != S_OK)
	{		
		return DDERR_INVALIDPARAMS;
	}

	if (StartDirectDraw((LPDIRECTDRAW*)This, pGUID) == DD_OK);
    {

		return DD_OK;
	}
	
	return DDERR_INVALIDPARAMS;
}



