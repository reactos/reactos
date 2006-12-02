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
	  

	if (This->lpLink == NULL)
	{
		RtlZeroMemory(&ddgbl, sizeof(DDRAWI_DIRECTDRAW_GBL));
	    
		if (ddgbl.lpDDCBtmp == NULL)
		{
			ddgbl.lpDDCBtmp = (LPDDHAL_CALLBACKS) DxHeapMemAlloc(sizeof(DDHAL_CALLBACKS));  
			if (ddgbl.lpDDCBtmp == NULL)
			{
				DX_STUB_str("Out of memmory");
				return DD_FALSE;
			}
		}
		
		
	}

	/* 
	   Visual studio think this code is a break point if we call 
	   second time to this function, press on continue in visual
	   studio the program will work. No real bug. gcc 3.4.5 genreate 
	   code that look like MS visual studio break point. 
     */

	This->lpLcl->lpDDCB = ddgbl.lpDDCBtmp;

    /* Same for HEL and HAL */

	if (ddgbl.lpModeInfo == NULL)
	{
		ddgbl.lpModeInfo = (DDHALMODEINFO*) DxHeapMemAlloc(1 * sizeof(DDHALMODEINFO));  
		if (ddgbl.lpModeInfo == NULL)
		{
			DX_STUB_str("DD_FALSE");
			return DD_FALSE;
		}		   
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
    
    hel_ret = StartDirectDrawHel(iface);
	hal_ret = StartDirectDrawHal(iface);

    if ((hal_ret!=DD_OK) &&  (hel_ret!=DD_OK))
    {
		DX_STUB_str("DDERR_NODIRECTDRAWSUPPORT");
        return DDERR_NODIRECTDRAWSUPPORT; 
    }

	This->lpLcl->hDD = This->lpLcl->lpGbl->hDD;

	/* Mix the DDCALLBACKS */	
	This->lpLcl->lpDDCB->cbDDCallbacks.dwSize = sizeof(This->lpLcl->lpDDCB->cbDDCallbacks);

	if ((This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_CANCREATESURFACE) && (devicetypes !=3))
	{		
		This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_CANCREATESURFACE;
		This->lpLcl->lpDDCB->cbDDCallbacks.CanCreateSurface = This->lpLcl->lpDDCB->HALDD.CanCreateSurface;
	}
	else if ((This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_CANCREATESURFACE) && (devicetypes !=2))
	{		    
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_CANCREATESURFACE;
		     This->lpLcl->lpDDCB->cbDDCallbacks.CanCreateSurface = This->lpLcl->lpDDCB->HELDD.CanCreateSurface;
	}
	if ((This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_CREATESURFACE) && (devicetypes !=3))
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_CREATESURFACE;
		This->lpLcl->lpDDCB->cbDDCallbacks.CreateSurface = This->lpLcl->lpDDCB->HALDD.CreateSurface;
	}
	else if ((This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_CREATESURFACE) && (devicetypes !=2))
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_CREATESURFACE;
		     This->lpLcl->lpDDCB->cbDDCallbacks.CreateSurface = This->lpLcl->lpDDCB->HELDD.CreateSurface;
	}
	if ((This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_CREATEPALETTE) && (devicetypes !=3))
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_CREATEPALETTE;
		This->lpLcl->lpDDCB->cbDDCallbacks.CreatePalette = This->lpLcl->lpDDCB->HALDD.CreatePalette;
	}
	else if ((This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_CREATEPALETTE) && (devicetypes !=2))
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_CREATEPALETTE;
		     This->lpLcl->lpDDCB->cbDDCallbacks.CreatePalette = This->lpLcl->lpDDCB->HELDD.CreatePalette;
	}
	if ((This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_DESTROYDRIVER) && (devicetypes !=3))
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_DESTROYDRIVER;
		This->lpLcl->lpDDCB->cbDDCallbacks.DestroyDriver = This->lpLcl->lpDDCB->HALDD.DestroyDriver;
	}
	else if ((This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_DESTROYDRIVER) && (devicetypes !=2))
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_DESTROYDRIVER;
		     This->lpLcl->lpDDCB->cbDDCallbacks.DestroyDriver = This->lpLcl->lpDDCB->HELDD.DestroyDriver;
	}
	if ((This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_FLIPTOGDISURFACE) && (devicetypes !=3))
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_FLIPTOGDISURFACE;
		This->lpLcl->lpDDCB->cbDDCallbacks.FlipToGDISurface = This->lpLcl->lpDDCB->HALDD.FlipToGDISurface;
	}
	else if ((This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_FLIPTOGDISURFACE) && (devicetypes !=2))
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_FLIPTOGDISURFACE;
			 This->lpLcl->lpDDCB->cbDDCallbacks.FlipToGDISurface = This->lpLcl->lpDDCB->HELDD.FlipToGDISurface;
	}
	if ((This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_GETSCANLINE) && (devicetypes !=3))
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_GETSCANLINE;
		This->lpLcl->lpDDCB->cbDDCallbacks.GetScanLine = This->lpLcl->lpDDCB->HALDD.GetScanLine;
	}
	else if ((This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_GETSCANLINE) && (devicetypes !=2))
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_GETSCANLINE;
		     This->lpLcl->lpDDCB->cbDDCallbacks.GetScanLine = This->lpLcl->lpDDCB->HELDD.GetScanLine;
	}
	if ((This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_SETCOLORKEY) && (devicetypes !=3))
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_SETCOLORKEY;
		This->lpLcl->lpDDCB->cbDDCallbacks.SetColorKey = This->lpLcl->lpDDCB->HALDD.SetColorKey;
	}
	else if ((This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_SETCOLORKEY) && (devicetypes !=2))
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_SETCOLORKEY;
		     This->lpLcl->lpDDCB->cbDDCallbacks.SetColorKey = This->lpLcl->lpDDCB->HELDD.SetColorKey;
	}
	if ((This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_SETEXCLUSIVEMODE) && (devicetypes !=3))
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_SETEXCLUSIVEMODE;
		This->lpLcl->lpDDCB->cbDDCallbacks.SetExclusiveMode = This->lpLcl->lpDDCB->HALDD.SetExclusiveMode;
	}
	else if ((This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_SETEXCLUSIVEMODE) && (devicetypes !=2))
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_SETEXCLUSIVEMODE;
			 This->lpLcl->lpDDCB->cbDDCallbacks.SetExclusiveMode = This->lpLcl->lpDDCB->HELDD.SetExclusiveMode;
	}
	if ((This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_SETMODE) && (devicetypes !=3))
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_SETMODE;
		This->lpLcl->lpDDCB->cbDDCallbacks.SetMode = This->lpLcl->lpDDCB->HALDD.SetMode;
	}
	else if ((This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_SETMODE) && (devicetypes !=2))
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_SETMODE;
			 This->lpLcl->lpDDCB->cbDDCallbacks.SetMode = This->lpLcl->lpDDCB->HELDD.SetMode;
	}
	if ((This->lpLcl->lpDDCB->HALDD.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK) && (devicetypes !=3))
	{
        This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_WAITFORVERTICALBLANK;
		This->lpLcl->lpDDCB->cbDDCallbacks.WaitForVerticalBlank = 
			This->lpLcl->lpDDCB->HALDD.WaitForVerticalBlank;
	}
	else if ((This->lpLcl->lpDDCB->HELDD.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK) && (devicetypes !=2))
	{
             This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags |= DDHAL_CB32_WAITFORVERTICALBLANK;
		 	 This->lpLcl->lpDDCB->cbDDCallbacks.WaitForVerticalBlank = 
				 This->lpLcl->lpDDCB->HELDD.WaitForVerticalBlank;
	}

	/* Mix the DDSURFACE CALLBACKS */
	This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwSize = sizeof(This->lpLcl->lpDDCB->cbDDSurfaceCallbacks);

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_ADDATTACHEDSURFACE) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_ADDATTACHEDSURFACE;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.AddAttachedSurface =  
			  This->lpLcl->lpDDCB->HALDDSurface.AddAttachedSurface;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_ADDATTACHEDSURFACE) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_ADDATTACHEDSURFACE;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.AddAttachedSurface =  
			  This->lpLcl->lpDDCB->HELDDSurface.AddAttachedSurface;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_BLT) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_BLT;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.Blt =  
			  This->lpLcl->lpDDCB->HALDDSurface.Blt;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_BLT) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_BLT;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.Blt =  
			  This->lpLcl->lpDDCB->HELDDSurface.Blt;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_DESTROYSURFACE) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_DESTROYSURFACE;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.DestroySurface =  
			  This->lpLcl->lpDDCB->HALDDSurface.DestroySurface;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_DESTROYSURFACE) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_DESTROYSURFACE;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.DestroySurface =  
			  This->lpLcl->lpDDCB->HELDDSurface.DestroySurface;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_FLIP) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_FLIP;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.Flip =  
			  This->lpLcl->lpDDCB->HALDDSurface.Flip;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_FLIP) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_FLIP;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.Flip =  
			  This->lpLcl->lpDDCB->HELDDSurface.Flip;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_GETBLTSTATUS) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_GETBLTSTATUS;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.GetBltStatus =  
			  This->lpLcl->lpDDCB->HALDDSurface.GetBltStatus;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_GETBLTSTATUS) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_GETBLTSTATUS;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.GetBltStatus =  
			  This->lpLcl->lpDDCB->HELDDSurface.GetBltStatus;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_GETFLIPSTATUS) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_GETFLIPSTATUS;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.GetFlipStatus =  
			  This->lpLcl->lpDDCB->HALDDSurface.GetFlipStatus;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_GETFLIPSTATUS) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_GETFLIPSTATUS;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.GetFlipStatus =  
			  This->lpLcl->lpDDCB->HELDDSurface.GetFlipStatus;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_LOCK) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_LOCK;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.Lock =  
			  This->lpLcl->lpDDCB->HALDDSurface.Lock;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_LOCK) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_LOCK;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.Lock =  
			  This->lpLcl->lpDDCB->HELDDSurface.Lock;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_RESERVED4) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_RESERVED4;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.reserved4 =  
			  This->lpLcl->lpDDCB->HALDDSurface.reserved4;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_RESERVED4) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_RESERVED4;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.reserved4 =  
			  This->lpLcl->lpDDCB->HELDDSurface.reserved4;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_SETCLIPLIST) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_SETCLIPLIST;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.SetClipList =  
			  This->lpLcl->lpDDCB->HALDDSurface.SetClipList;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_SETCLIPLIST) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_SETCLIPLIST;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.SetClipList =  
			  This->lpLcl->lpDDCB->HELDDSurface.SetClipList;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_SETCOLORKEY) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_SETCOLORKEY;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.SetColorKey =  
			  This->lpLcl->lpDDCB->HALDDSurface.SetColorKey;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_SETCOLORKEY) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_SETCOLORKEY;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.SetColorKey =  
			  This->lpLcl->lpDDCB->HELDDSurface.SetColorKey;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_SETOVERLAYPOSITION) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_SETOVERLAYPOSITION;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.SetOverlayPosition =  
			  This->lpLcl->lpDDCB->HALDDSurface.SetOverlayPosition;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_SETOVERLAYPOSITION) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_SETOVERLAYPOSITION;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.SetOverlayPosition =  
			  This->lpLcl->lpDDCB->HELDDSurface.SetOverlayPosition;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_SETPALETTE) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_SETPALETTE;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.SetPalette =  
			  This->lpLcl->lpDDCB->HALDDSurface.SetPalette;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_SETPALETTE) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_SETPALETTE;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.SetPalette =  
			  This->lpLcl->lpDDCB->HELDDSurface.SetPalette;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_UNLOCK) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_UNLOCK;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.Unlock =  
			  This->lpLcl->lpDDCB->HALDDSurface.Unlock;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_UNLOCK) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_UNLOCK;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.Unlock =  
			  This->lpLcl->lpDDCB->HELDDSurface.Unlock;
	}

	if ((This->lpLcl->lpDDCB->HALDDSurface.dwFlags & DDHAL_SURFCB32_UPDATEOVERLAY) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_UPDATEOVERLAY;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.UpdateOverlay =  
			  This->lpLcl->lpDDCB->HALDDSurface.UpdateOverlay;
	}
	else if ((This->lpLcl->lpDDCB->HELDDSurface.dwFlags & DDHAL_SURFCB32_UPDATEOVERLAY) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.dwFlags |= DDHAL_SURFCB32_UPDATEOVERLAY;

		This->lpLcl->lpDDCB->cbDDSurfaceCallbacks.UpdateOverlay =  
			  This->lpLcl->lpDDCB->HELDDSurface.UpdateOverlay;
	}

	/*  Mix the DDPALETTE CALLBACKS */	
	This->lpLcl->lpDDCB->HALDDPalette.dwSize = sizeof(This->lpLcl->lpDDCB->HALDDPalette);

	if ((This->lpLcl->lpDDCB->HALDDPalette.dwFlags & DDHAL_PALCB32_DESTROYPALETTE) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDPaletteCallbacks.dwFlags |= DDHAL_PALCB32_SETENTRIES;

		This->lpLcl->lpDDCB->cbDDPaletteCallbacks.DestroyPalette =  
			  This->lpLcl->lpDDCB->HALDDPalette.DestroyPalette;
	}
	else if ((This->lpLcl->lpDDCB->HELDDPalette.dwFlags & DDHAL_PALCB32_DESTROYPALETTE) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDPaletteCallbacks.dwFlags |= DDHAL_PALCB32_DESTROYPALETTE;

		This->lpLcl->lpDDCB->cbDDPaletteCallbacks.DestroyPalette =  
			This->lpLcl->lpDDCB->HELDDPalette.DestroyPalette;
	}

	if ((This->lpLcl->lpDDCB->HALDDPalette.dwFlags & DDHAL_PALCB32_SETENTRIES) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDPaletteCallbacks.dwFlags |= DDHAL_PALCB32_SETENTRIES;

		This->lpLcl->lpDDCB->cbDDPaletteCallbacks.SetEntries =  
			  This->lpLcl->lpDDCB->HALDDPalette.SetEntries;
	}
	else if ((This->lpLcl->lpDDCB->HELDDPalette.dwFlags & DDHAL_PALCB32_SETENTRIES) && (devicetypes !=2))
	{
		This->lpLcl->lpDDCB->cbDDPaletteCallbacks.dwFlags |= DDHAL_PALCB32_SETENTRIES;

		This->lpLcl->lpDDCB->cbDDPaletteCallbacks.SetEntries =  
			  This->lpLcl->lpDDCB->HELDDPalette.SetEntries;
	}

	/* Mix the DDExeBuf CALLBACKS */
	This->lpLcl->lpDDCB->cbDDExeBufCallbacks.dwSize = sizeof(This->lpLcl->lpDDCB->cbDDExeBufCallbacks);
	
	if ((This->lpLcl->lpDDCB->HALDDExeBuf.dwFlags & DDHAL_EXEBUFCB32_CANCREATEEXEBUF) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDExeBufCallbacks.CanCreateExecuteBuffer =
			   This->lpLcl->lpDDCB->HALDDExeBuf.CanCreateExecuteBuffer;
		This->lpLcl->lpDDCB->cbDDExeBufCallbacks.dwFlags |= DDHAL_EXEBUFCB32_CANCREATEEXEBUF;
	}
	else if ((This->lpLcl->lpDDCB->HELDDExeBuf.dwFlags & DDHAL_EXEBUFCB32_CANCREATEEXEBUF) && (devicetypes !=2))
	{		     
		     This->lpLcl->lpDDCB->cbDDExeBufCallbacks.CanCreateExecuteBuffer = 
		         This->lpLcl->lpDDCB->HELDDExeBuf.CanCreateExecuteBuffer;
			 This->lpLcl->lpDDCB->cbDDExeBufCallbacks.dwFlags |= DDHAL_EXEBUFCB32_CANCREATEEXEBUF;
	}

	if ((This->lpLcl->lpDDCB->HALDDExeBuf.dwFlags & DDHAL_EXEBUFCB32_CREATEEXEBUF) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDExeBufCallbacks.CreateExecuteBuffer =
			   This->lpLcl->lpDDCB->HALDDExeBuf.CreateExecuteBuffer;
		This->lpLcl->lpDDCB->cbDDExeBufCallbacks.dwFlags |= DDHAL_EXEBUFCB32_CREATEEXEBUF;
	}
	else if ((This->lpLcl->lpDDCB->HELDDExeBuf.dwFlags & DDHAL_EXEBUFCB32_CREATEEXEBUF) && (devicetypes !=2))
	{		     
		     This->lpLcl->lpDDCB->cbDDExeBufCallbacks.CreateExecuteBuffer = 
		         This->lpLcl->lpDDCB->HELDDExeBuf.CreateExecuteBuffer;
			 This->lpLcl->lpDDCB->cbDDExeBufCallbacks.dwFlags |= DDHAL_EXEBUFCB32_CREATEEXEBUF;
	}

	if ((This->lpLcl->lpDDCB->HALDDExeBuf.dwFlags & DDHAL_EXEBUFCB32_DESTROYEXEBUF) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDExeBufCallbacks.DestroyExecuteBuffer =
			   This->lpLcl->lpDDCB->HALDDExeBuf.DestroyExecuteBuffer;
		This->lpLcl->lpDDCB->cbDDExeBufCallbacks.dwFlags |= DDHAL_EXEBUFCB32_DESTROYEXEBUF;
	}
	else if ((This->lpLcl->lpDDCB->HELDDExeBuf.dwFlags & DDHAL_EXEBUFCB32_DESTROYEXEBUF) && (devicetypes !=2))
	{		     
		     This->lpLcl->lpDDCB->cbDDExeBufCallbacks.DestroyExecuteBuffer = 
		         This->lpLcl->lpDDCB->HELDDExeBuf.DestroyExecuteBuffer;
			 This->lpLcl->lpDDCB->cbDDExeBufCallbacks.dwFlags |= DDHAL_EXEBUFCB32_DESTROYEXEBUF;
	}

	if ((This->lpLcl->lpDDCB->HALDDExeBuf.dwFlags & DDHAL_EXEBUFCB32_LOCKEXEBUF) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDExeBufCallbacks.LockExecuteBuffer =
			   This->lpLcl->lpDDCB->HALDDExeBuf.LockExecuteBuffer;
		This->lpLcl->lpDDCB->cbDDExeBufCallbacks.dwFlags |= DDHAL_EXEBUFCB32_LOCKEXEBUF;
	}
	else if ((This->lpLcl->lpDDCB->HELDDExeBuf.dwFlags & DDHAL_EXEBUFCB32_LOCKEXEBUF) && (devicetypes !=2))
	{		     
		     This->lpLcl->lpDDCB->cbDDExeBufCallbacks.LockExecuteBuffer = 
		         This->lpLcl->lpDDCB->HELDDExeBuf.LockExecuteBuffer;
			 This->lpLcl->lpDDCB->cbDDExeBufCallbacks.dwFlags |= DDHAL_EXEBUFCB32_LOCKEXEBUF;
	}

	if ((This->lpLcl->lpDDCB->HALDDExeBuf.dwFlags & DDHAL_EXEBUFCB32_UNLOCKEXEBUF) && (devicetypes !=3))
	{
		This->lpLcl->lpDDCB->cbDDExeBufCallbacks.UnlockExecuteBuffer =
			   This->lpLcl->lpDDCB->HALDDExeBuf.UnlockExecuteBuffer;
		This->lpLcl->lpDDCB->cbDDExeBufCallbacks.dwFlags |= DDHAL_EXEBUFCB32_UNLOCKEXEBUF;
	}
	else if ((This->lpLcl->lpDDCB->HELDDExeBuf.dwFlags & DDHAL_EXEBUFCB32_UNLOCKEXEBUF) && (devicetypes !=2))
	{		     
		     This->lpLcl->lpDDCB->cbDDExeBufCallbacks.UnlockExecuteBuffer = 
		         This->lpLcl->lpDDCB->HELDDExeBuf.UnlockExecuteBuffer;
			 This->lpLcl->lpDDCB->cbDDExeBufCallbacks.dwFlags |= DDHAL_EXEBUFCB32_UNLOCKEXEBUF;
	}
	
	/* Fill some basic info for Surface */
	ddSurfGbl.lpDD = &ddgbl;

	/* FIXME 
	   We need setup this also 
	   This->lpLcl->lpDDCB->cbDDColorControlCallbacks
	   This->lpLcl->lpDDCB->cbDDKernelCallbacks
	   This->lpLcl->lpDDCB->cbDDMiscellaneousCallbacks
	   This->lpLcl->lpDDCB->cbDDMotionCompCallbacks
	   This->lpLcl->lpDDCB->cbDDVideoPortCallbacks
	*/

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

	return DD_OK;
	
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


    /* FIXME 
	   The 3d and private data are not save at moment 
	   
	   we need lest the private data being setup
	   for some driver are puting kmode memory there
	   the memory often contain the private struct +
	   surface, see MS DDK how MS example driver using 
	   it

	   the 3d interface are not so improten if u do not
	   want the 3d, and we are not writing 3d code yet
	   so we be okay for now. 
	 */

  
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
	This->lpLcl->lpDDCB->HELDD.FlipToGDISurface     = HelDdFlipToGDISurface;
	This->lpLcl->lpDDCB->HELDD.GetScanLine          = HelDdGetScanLine;
	This->lpLcl->lpDDCB->HELDD.SetColorKey          = HelDdSetColorKey;
	This->lpLcl->lpDDCB->HELDD.SetExclusiveMode     = HelDdSetExclusiveMode;
	This->lpLcl->lpDDCB->HELDD.SetMode              = HelDdSetMode;
	This->lpLcl->lpDDCB->HELDD.WaitForVerticalBlank = HelDdWaitForVerticalBlank;

	

	This->lpLcl->lpDDCB->HELDD.dwFlags =  DDHAL_CB32_CANCREATESURFACE     |
		                                  DDHAL_CB32_CREATESURFACE        |
										  DDHAL_CB32_CREATEPALETTE        |
		                                  DDHAL_CB32_DESTROYDRIVER        | 
										  DDHAL_CB32_FLIPTOGDISURFACE     |
										  DDHAL_CB32_GETSCANLINE          |
										  DDHAL_CB32_SETCOLORKEY          |
										  DDHAL_CB32_SETEXCLUSIVEMODE     | 
										  DDHAL_CB32_SETMODE              |                                                   
										  DDHAL_CB32_WAITFORVERTICALBLANK ;

	This->lpLcl->lpDDCB->HELDD.dwSize = sizeof(This->lpLcl->lpDDCB->HELDD);
	
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
    
	This->lpLcl->lpDDCB->HELDDSurface.dwFlags = DDHAL_SURFCB32_ADDATTACHEDSURFACE |
		                                        DDHAL_SURFCB32_BLT                | 
												DDHAL_SURFCB32_DESTROYSURFACE     | 
												DDHAL_SURFCB32_FLIP               | 
												DDHAL_SURFCB32_GETBLTSTATUS       | 
												DDHAL_SURFCB32_GETFLIPSTATUS      | 
												DDHAL_SURFCB32_LOCK               | 
												DDHAL_SURFCB32_RESERVED4          | 
												DDHAL_SURFCB32_SETCLIPLIST        | 
												DDHAL_SURFCB32_SETCOLORKEY        | 
												DDHAL_SURFCB32_SETOVERLAYPOSITION | 
												DDHAL_SURFCB32_SETPALETTE         | 
												DDHAL_SURFCB32_UNLOCK             |
												DDHAL_SURFCB32_UPDATEOVERLAY;

    This->lpLcl->lpDDCB->HELDDSurface.dwSize = sizeof(This->lpLcl->lpDDCB->HELDDSurface);
		                                      
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
	LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)*pIface;
    
	DX_WINDBG_trace();

	if (!IsEqualGUID(&IID_IDirectDraw7, id))
    {
		return DDERR_INVALIDDIRECTDRAWGUID;
    }

	if (This == NULL)
	{
		/* We do not have any DirectDraw interface alloc */
	    This = DxHeapMemAlloc(sizeof(DDRAWI_DIRECTDRAW_INT));
	    if (This == NULL) 
	    {
		    return DDERR_OUTOFMEMORY;
	    }
	} 
	else
	{
		/* We got the DirectDraw interface alloc and we need create the link */

		LPDDRAWI_DIRECTDRAW_INT  newThis;	
		newThis = DxHeapMemAlloc(sizeof(DDRAWI_DIRECTDRAW_INT));
		if (newThis == NULL) 
	    {
		    return DDERR_OUTOFMEMORY;
	    }
        
		/* we need check the GUID lpGUID what type it is */
		if (pGUID != DDCREATE_HARDWAREONLY)
		{
			if (pGUID !=NULL)
			{
				This = newThis;
				return DDERR_INVALIDDIRECTDRAWGUID;		
			}
		}

		newThis->lpLink = This;
		This = newThis;		
	}

	This->lpLcl = DxHeapMemAlloc(sizeof(DDRAWI_DIRECTDRAW_INT));

	if (This->lpLcl == NULL)
	{
		/* FIXME cleanup */
		return DDERR_OUTOFMEMORY;
	}
	

    /* 
	   We need manual fill this struct member we can not trust on 
	   the heap zero the struct if you play to much with directdraw 
	   in Windows 2000. This is a small workaround of one of directdraw
	   bugs
     */

	/*
	   FIXME 
	   read dwAppHackFlags flag from the system register instead for hard code it
	 */
	This->lpLcl->dwAppHackFlags = 0; 
	This->lpLcl->dwErrorMode = 0;
	This->lpLcl->dwHotTracking = 0;
	This->lpLcl->dwIMEState = 0;
	This->lpLcl->dwLocalFlags = DDRAWILCL_DIRECTDRAW7;
	This->lpLcl->dwLocalRefCnt = 0;
	This->lpLcl->dwObsolete1 = 0;
	This->lpLcl->dwPreferredMode = 0;
	This->lpLcl->dwProcessId = 0;
	This->lpLcl->dwUnused0 = 0;
	This->lpLcl->hD3DInstance = NULL;
	This->lpLcl->hDC = 0;	
	This->lpLcl->hDDVxd = 0;
	This->lpLcl->hFocusWnd = 0;
	This->lpLcl->hGammaCalibrator = 0;
	This->lpLcl->hWnd = 0;
	This->lpLcl->hWndPopup = 0;	
	This->lpLcl->lpCB = NULL;
	This->lpLcl->lpDDCB = NULL;
	This->lpLcl->lpDDMore = 0;
	This->lpLcl->lpGammaCalibrator = 0;
	This->lpLcl->lpGbl = &ddgbl;
	This->lpLcl->lpPrimary = NULL;
	This->lpLcl->pD3DIUnknown = NULL;
	This->lpLcl->pUnkOuter = NULL;
			 
	*pIface = (LPDIRECTDRAW)This;

	if(Main_DirectDraw_QueryInterface((LPDIRECTDRAW7)This, id, (void**)&pIface) != S_OK)
	{		
		return DDERR_INVALIDPARAMS;
	}
	
	if (StartDirectDraw((LPDIRECTDRAW*)This, pGUID) == DD_OK);
    {
        This->lpLcl->hDD = ddgbl.hDD;
		return DD_OK;
	}
	
	return DDERR_INVALIDPARAMS;
}



