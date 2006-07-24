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


HRESULT WINAPI 
StartDirectDraw(LPDIRECTDRAW* iface)
{
	IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    DWORD hal_ret;
    DWORD hel_ret;
    DEVMODE devmode;
    HBITMAP hbmp;
    const UINT bmiSize = sizeof(BITMAPINFOHEADER) + 0x10;
    UCHAR *pbmiData;
    BITMAPINFO *pbmi;    
    DWORD *pMasks;
	DWORD Flags;
    
    DX_WINDBG_trace();
	  
	RtlZeroMemory(&This->mDDrawGlobal, sizeof(DDRAWI_DIRECTDRAW_GBL));
	
	/* cObsolete is undoc in msdn it being use in CreateDCA */
	RtlCopyMemory(&This->mDDrawGlobal.cObsolete,&"DISPLAY",7);
	RtlCopyMemory(&This->mDDrawGlobal.cDriverName,&"DISPLAY",7);
	
    /* Same for HEL and HAL */
    This->mcModeInfos = 1;
    This->mpModeInfos = (DDHALMODEINFO*) DxHeapMemAlloc(This->mcModeInfos * sizeof(DDHALMODEINFO));  
   
    if (This->mpModeInfos == NULL)
    {
	   DX_STUB_str("DD_FALSE");
       return DD_FALSE;
    }

    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);

    This->mpModeInfos[0].dwWidth      = devmode.dmPelsWidth;
    This->mpModeInfos[0].dwHeight     = devmode.dmPelsHeight;
    This->mpModeInfos[0].dwBPP        = devmode.dmBitsPerPel;
    This->mpModeInfos[0].lPitch       = (devmode.dmPelsWidth*devmode.dmBitsPerPel)/8;
    This->mpModeInfos[0].wRefreshRate = (WORD)devmode.dmDisplayFrequency;
   
    This->hdc = CreateDCW(L"DISPLAY",L"DISPLAY",NULL,NULL);    

    if (This->hdc == NULL)
    {
	   DX_STUB_str("DDERR_OUTOFMEMORY");
       return DDERR_OUTOFMEMORY ;
    }

    hbmp = CreateCompatibleBitmap(This->hdc, 1, 1);  
    if (hbmp==NULL)
    {
       DxHeapMemFree(This->mpModeInfos);
       DeleteDC(This->hdc);
	   DX_STUB_str("DDERR_OUTOFMEMORY");
       return DDERR_OUTOFMEMORY;
    }
  
    pbmiData = (UCHAR *) DxHeapMemAlloc(bmiSize);
    pbmi = (BITMAPINFO*)pbmiData;

    if (pbmiData==NULL)
    {
       DxHeapMemFree(This->mpModeInfos);       
       DeleteDC(This->hdc);
       DeleteObject(hbmp);
	   DX_STUB_str("DDERR_OUTOFMEMORY");
       return DDERR_OUTOFMEMORY;
    }

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biBitCount = (WORD)devmode.dmBitsPerPel;
    pbmi->bmiHeader.biCompression = BI_BITFIELDS;
    pbmi->bmiHeader.biWidth = 1;
    pbmi->bmiHeader.biHeight = 1;

    GetDIBits(This->hdc, hbmp, 0, 0, NULL, pbmi, 0);
    DeleteObject(hbmp);

    pMasks = (DWORD*)(pbmiData + sizeof(BITMAPINFOHEADER));
    This->mpModeInfos[0].dwRBitMask = pMasks[0];
    This->mpModeInfos[0].dwGBitMask = pMasks[1];
    This->mpModeInfos[0].dwBBitMask = pMasks[2];
    This->mpModeInfos[0].dwAlphaBitMask = pMasks[3];

	DxHeapMemFree(pbmiData);

    /* Startup HEL and HAL */
    RtlZeroMemory(&This->mDDrawGlobal, sizeof(DDRAWI_DIRECTDRAW_GBL));
    RtlZeroMemory(&This->mHALInfo, sizeof(DDHALINFO));
    RtlZeroMemory(&This->mCallbacks, sizeof(DDHAL_CALLBACKS));

    This->mDDrawLocal.lpDDCB = &This->mCallbacks;
    This->mDDrawLocal.lpGbl = &This->mDDrawGlobal;
    This->mDDrawLocal.dwProcessId = GetCurrentProcessId();

    This->mDDrawGlobal.lpDDCBtmp = &This->mCallbacks;
    This->mDDrawGlobal.lpExclusiveOwner = &This->mDDrawLocal;

    hal_ret = StartDirectDrawHal(iface);
	hel_ret = StartDirectDrawHel(iface);    
    if ((hal_ret!=DD_OK) &&  (hel_ret!=DD_OK))
    {
		DX_STUB_str("DDERR_NODIRECTDRAWSUPPORT");
        return DDERR_NODIRECTDRAWSUPPORT; 
    }

    /* 
       Setup HEL or HAL for DD_CALLBACKS 
    */ 
            
    This->mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;
	This->mDdCreatePalette.lpDD = &This->mDDrawGlobal;
	This->mDdCreateSurface.lpDD = &This->mDDrawGlobal;
	This->mDdFlipToGDISurface.lpDD = &This->mDDrawGlobal;
	This->mDdDestroyDriver.lpDD = &This->mDDrawGlobal;
	This->mDdGetScanLine.lpDD = &This->mDDrawGlobal;
    This->mDdSetExclusiveMode.lpDD = &This->mDDrawGlobal;
	This->mDdSetMode.lpDD = &This->mDDrawGlobal;
	This->mDdWaitForVerticalBlank.lpDD = &This->mDDrawGlobal;
	This->mDdSetColorKey.lpDD = &This->mDDrawGlobal;
	
	if (This->devicetype!=1)
	{
		/* both or only hel */
		This->mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HELDD.CanCreateSurface;  	
		This->mDdCreatePalette.CreatePalette = This->mCallbacks.HELDD.CreatePalette;  	
		This->mDdCreateSurface.CreateSurface = This->mCallbacks.HELDD.CreateSurface;  	
		This->mDdDestroyDriver.DestroyDriver = This->mCallbacks.HELDD.DestroyDriver;  	
		This->mDdFlipToGDISurface.FlipToGDISurface = This->mCallbacks.HELDD.FlipToGDISurface; 		
		This->mDdGetScanLine.GetScanLine = This->mCallbacks.HELDD.GetScanLine; 
		This->mDdSetExclusiveMode.SetExclusiveMode = This->mCallbacks.HELDD.SetExclusiveMode; 			
		This->mDdSetMode.SetMode = This->mCallbacks.HELDD.SetMode; 		
		This->mDdWaitForVerticalBlank.WaitForVerticalBlank = This->mCallbacks.HELDD.WaitForVerticalBlank;  	
		// This->mDdSetColorKey.SetColorKey = This->mCallbacks.HELDD.SetColorKey;  
	}
	
    if (This->devicetype!=2)
	{
		Flags = This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags;    
		if (Flags & DDHAL_CB32_CANCREATESURFACE) 
		{
			This->mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HALDD.CanCreateSurface;  
		}
	
		if (Flags & DDHAL_CB32_CREATEPALETTE) 
		{
			This->mDdCreatePalette.CreatePalette = This->mCallbacks.HALDD.CreatePalette;  
		}
	
		if (Flags & DDHAL_CB32_CREATESURFACE) 
		{
			This->mDdCreateSurface.CreateSurface = This->mCallbacks.HALDD.CreateSurface;  
		}
	
		if (Flags & DDHAL_CB32_DESTROYDRIVER) 
		{
			This->mDdDestroyDriver.DestroyDriver = This->mCallbacks.HALDD.DestroyDriver;  
		}
	
		if (Flags & DDHAL_CB32_FLIPTOGDISURFACE) 
		{
			This->mDdFlipToGDISurface.FlipToGDISurface = This->mCallbacks.HALDD.FlipToGDISurface; 
		}
	
		if (Flags & DDHAL_CB32_GETSCANLINE) 
		{
			This->mDdGetScanLine.GetScanLine = This->mCallbacks.HALDD.GetScanLine; 
		}
	
		if (Flags & DDHAL_CB32_SETEXCLUSIVEMODE) 
		{
			This->mDdSetExclusiveMode.SetExclusiveMode = This->mCallbacks.HALDD.SetExclusiveMode; 
		}
	
		if (Flags & DDHAL_CB32_SETMODE) 
		{
			This->mDdSetMode.SetMode = This->mCallbacks.HALDD.SetMode; 
		}
	
		if (Flags & DDHAL_CB32_WAITFORVERTICALBLANK) 
		{
			This->mDdWaitForVerticalBlank.WaitForVerticalBlank = This->mCallbacks.HALDD.WaitForVerticalBlank;  
		}

		if (Flags & DDHAL_CB32_SETCOLORKEY) 
		{
			// This->mDdSetColorKey.SetColorKey = This->mCallbacks.HALDD.SetColorKey;  
		}
	}

	/* 
       Setup HEL or HAL for SURFACE CALLBACK 
    */ 

	// FIXME
    
    /* Setup calback struct so we do not need refill same info again */
    This->mDdCreateSurface.lpDD = &This->mDDrawGlobal;    
    This->mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;  
                  
    return DD_OK;
}


HRESULT WINAPI 
StartDirectDrawHal(LPDIRECTDRAW* iface)
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
    This->mD3dDriverData.lpTextureFormats = (DDSURFACEDESC*) This->mpTextures;

    if (!DdQueryDirectDrawObject(
                                    &This->mDDrawGlobal,
                                    &This->mHALInfo,
                                    &This->mCallbacks.HALDD,
                                    &This->mCallbacks.HALDDSurface,
                                    &This->mCallbacks.HALDDPalette, 
                                    &This->mD3dCallbacks,
                                    &This->mD3dDriverData,
                                    &This->mCallbacks.HALDDExeBuf,
                                    (DDSURFACEDESC*)This->mpTextures,
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

  /* Setup global surface */   
  /*This->mPrimaryGlobal.dwGlobalFlags = DDRAWISURFGBL_ISGDISURFACE;
  This->mPrimaryGlobal.lpDD       = &This->mDDrawGlobal;
  This->mPrimaryGlobal.lpDDHandle = &This->mDDrawGlobal;
  This->mPrimaryGlobal.wWidth  = (WORD)This->mpModeInfos[0].dwWidth;
  This->mPrimaryGlobal.wHeight = (WORD)This->mpModeInfos[0].dwHeight;
  This->mPrimaryGlobal.lPitch  = This->mpModeInfos[0].lPitch;*/

  /* FIXME free it in cleanup */
  This->mDDrawGlobal.dsList = (LPDDRAWI_DDRAWSURFACE_INT)DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_INT)); 
  return DD_OK;
}

HRESULT WINAPI 
StartDirectDrawHel(LPDIRECTDRAW* iface)
{
	IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	This->HELMemoryAvilable = HEL_GRAPHIC_MEMORY_MAX;

    This->mCallbacks.HELDD.dwFlags = DDHAL_CB32_DESTROYDRIVER;
    This->mCallbacks.HELDD.DestroyDriver = HelDdDestroyDriver;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_CREATESURFACE; 
    This->mCallbacks.HELDD.CreateSurface = HelDdCreateSurface;
    
    // DDHAL_CB32_
    //This->mCallbacks.HELDD.SetColorKey = HelDdSetColorKey;
   
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_SETMODE;
    This->mCallbacks.HELDD.SetMode = HelDdSetMode;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_WAITFORVERTICALBLANK;     
    This->mCallbacks.HELDD.WaitForVerticalBlank = HelDdWaitForVerticalBlank;
        
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_CANCREATESURFACE;
    This->mCallbacks.HELDD.CanCreateSurface = HelDdCanCreateSurface;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_CREATEPALETTE;
    This->mCallbacks.HELDD.CreatePalette = HelDdCreatePalette;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_GETSCANLINE;
    This->mCallbacks.HELDD.GetScanLine = HelDdGetScanLine;
    
    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_SETEXCLUSIVEMODE;
    This->mCallbacks.HELDD.SetExclusiveMode = HelDdSetExclusiveMode;

    This->mCallbacks.HELDD.dwFlags += DDHAL_CB32_FLIPTOGDISURFACE;
    This->mCallbacks.HELDD.FlipToGDISurface = HelDdFlipToGDISurface;
   
	return DD_OK;
}

HRESULT 
WINAPI 
Create_DirectDraw (LPGUID pGUID, 
				   LPDIRECTDRAW* pIface, 
				   REFIID id, 
				   BOOL ex)
{   
    IDirectDrawImpl* This = (IDirectDrawImpl*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(IDirectDrawImpl));

	DX_WINDBG_trace();

	if (This == NULL) 
	{
		return E_OUTOFMEMORY;
	}

	ZeroMemory(This,sizeof(IDirectDrawImpl));

	This->lpVtbl = &DirectDraw7_Vtable;
	This->lpVtbl_v1 = &DDRAW_IDirectDraw_VTable;
	This->lpVtbl_v2 = &DDRAW_IDirectDraw2_VTable;
	This->lpVtbl_v4 = &DDRAW_IDirectDraw4_VTable;
	
	*pIface = (LPDIRECTDRAW)This;

	This->devicetype = 0;

	if (pGUID == (LPGUID) DDCREATE_HARDWAREONLY)
	{
		This->devicetype = 1; /* hal only */
	}

	if (pGUID == (LPGUID) DDCREATE_EMULATIONONLY)
	{
	    This->devicetype = 2; /* hel only */
	}
	 
	if(This->lpVtbl->QueryInterface ((LPDIRECTDRAW7)This, id, (void**)&pIface) != S_OK)
	{
		return DDERR_INVALIDPARAMS;
	}

	if (StartDirectDraw((LPDIRECTDRAW*)This) == DD_OK);
    {
		return This->lpVtbl->Initialize ((LPDIRECTDRAW7)This, pGUID);
	}

	return DDERR_INVALIDPARAMS;
}

