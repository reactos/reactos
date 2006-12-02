/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/ddraw.c
 * PURPOSE:              IDirectDraw7 Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

/*
 * IMPLEMENT
 * Status ok
 */

#include "../rosdraw.h"




HRESULT 
WINAPI 
Main_DirectDraw_QueryInterface (LPDIRECTDRAW7 iface, 
								REFIID id, 
								LPVOID *obj) 
{   
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

	DX_WINDBG_trace();
    
	/* fixme 
	   the D3D object cab be optain from here 
	   Direct3D7 
	*/
    if (IsEqualGUID(&IID_IDirectDraw7, id))
    {
		/* DirectDraw7 Vtable */
		This->lpVtbl = &DirectDraw7_Vtable;
        *obj = &This->lpVtbl;
    }
    else
    {
        *obj = NULL;
        return E_NOINTERFACE;
    }
    	
    Main_DirectDraw_AddRef(iface);
    return S_OK;
}
/*
 * IMPLEMENT
 * Status ok
 */
ULONG
WINAPI 
Main_DirectDraw_AddRef (LPDIRECTDRAW7 iface) 
{    
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

	DX_WINDBG_trace();
	   
	if (iface!=NULL)
	{
		This->dwIntRefCnt++;
		This->lpLcl->dwLocalRefCnt++;
		
		if (This->lpLcl->lpGbl != NULL)
		{
			This->lpLcl->lpGbl->dwRefCnt++;			
		}
	}    
    return This->dwIntRefCnt;
}

/*
 * IMPLEMENT
 * Status ok
 */
ULONG 
WINAPI 
Main_DirectDraw_Release (LPDIRECTDRAW7 iface) 
{    
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

	DX_WINDBG_trace();
	
	if (iface!=NULL)
	{	  	
		This->lpLcl->dwLocalRefCnt--;
        This->dwIntRefCnt--;  

		if (This->lpLcl->lpGbl != NULL)
		{
			This->lpLcl->lpGbl->dwRefCnt--;
		}
            
		if ( This->dwIntRefCnt == 0)
		{
			// set resoltion back to the one in registry
			/*if(This->cooperative_level & DDSCL_EXCLUSIVE)
			{
				ChangeDisplaySettings(NULL, 0);
			}*/
            
			Cleanup(iface);					
            if (This!=NULL)
            {              
			    HeapFree(GetProcessHeap(), 0, This);
            }
		}
    }
    return This->dwIntRefCnt;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT 
WINAPI 
Main_DirectDraw_Compact(LPDIRECTDRAW7 iface) 
{
	/* MSDN say not implement but my question what does it return then */
    DX_WINDBG_trace();
    return DD_OK;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT 
WINAPI 
Main_DirectDraw_CreateClipper(LPDIRECTDRAW7 iface, 
							  DWORD dwFlags, 
                              LPDIRECTDRAWCLIPPER *ppClipper, 
							  IUnknown *pUnkOuter)
{
	//LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
	LPDDRAWI_DDRAWCLIPPER_INT  That; 

    DX_WINDBG_trace();

    if (pUnkOuter!=NULL) 
	{
        return CLASS_E_NOAGGREGATION; 
	}

	
	That = (LPDDRAWI_DDRAWCLIPPER_INT) DxHeapMemAlloc(sizeof(DDRAWI_DDRAWCLIPPER_INT));

    if (That == NULL) 
	{
		return  DDERR_OUTOFMEMORY;  //E_OUTOFMEMORY;
	}
   
    That->lpVtbl = &DirectDrawClipper_Vtable;

    *ppClipper = (LPDIRECTDRAWCLIPPER)That;

	DirectDrawClipper_AddRef((LPDIRECTDRAWCLIPPER)That);

    return DirectDrawClipper_Initialize((LPDIRECTDRAWCLIPPER)That, (LPDIRECTDRAW)iface, dwFlags);
}


HRESULT WINAPI Main_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
                  LPPALETTEENTRY palent, LPDIRECTDRAWPALETTE* ppPalette, LPUNKNOWN pUnkOuter)
{
	DX_WINDBG_trace();

	DX_STUB;		    			    
}


/*
 * stub
 * Status not done
 */
HRESULT WINAPI Main_DirectDraw_CreateSurface (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
                                            LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter) 
{
    
	LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
    LPDDRAWI_DDRAWSURFACE_INT That; 
	
    if (pUnkOuter!=NULL) 
	{
        return CLASS_E_NOAGGREGATION; 
	}

    if (sizeof(DDSURFACEDESC2)!=pDDSD->dwSize && sizeof(DDSURFACEDESC)!=pDDSD->dwSize)
	{
        return DDERR_UNSUPPORTED;
	}
    
    That = (LPDDRAWI_DDRAWSURFACE_INT)DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_INT));
    
    if (That == NULL) 
	{
        return E_OUTOFMEMORY;
	}

	That->lpLcl = (LPDDRAWI_DDRAWSURFACE_LCL)DxHeapMemAlloc(sizeof(DDRAWI_DDRAWSURFACE_LCL));
    
    if (That == NULL) 
	{
        return E_OUTOFMEMORY;
	}

    That->lpVtbl = &DirectDrawSurface7_Vtable;
	*ppSurf = (LPDIRECTDRAWSURFACE7)That;

	That->lpLcl->lpGbl = &ddSurfGbl;
	That->lpLcl->lpGbl->lpDD = &ddgbl;

	Main_DDrawSurface_AddRef((LPDIRECTDRAWSURFACE7)That);
	
	if (This->lpLcl->lpGbl->dsList == NULL)
	{	
		This->lpLcl->lpGbl->dsList = That;
		
	}
      
  
	/* Code from wine cvs 24/7-2006 */

	if (!(pDDSD->dwFlags & DDSD_CAPS))
    {
        /* DVIDEO.DLL does forget the DDSD_CAPS flag ... *sigh* */
        pDDSD->dwFlags |= DDSD_CAPS;
    }
    if (pDDSD->ddsCaps.dwCaps == 0)
    {
        /* This has been checked on real Windows */
        pDDSD->ddsCaps.dwCaps = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
    }

	
    if (pDDSD->ddsCaps.dwCaps & DDSCAPS_ALLOCONLOAD)
    {
        /* If the surface is of the 'alloconload' type, ignore the LPSURFACE field */
        pDDSD->dwFlags &= ~DDSD_LPSURFACE;
    }

	DX_STUB_str("pDDSD->ddsCaps.dwCaps ok");

    if ((pDDSD->dwFlags & DDSD_LPSURFACE) && (pDDSD->lpSurface == NULL))
    {
        /* Frank Herbert's Dune specifies a null pointer for the surface, ignore the LPSURFACE field */       
        pDDSD->dwFlags &= ~DDSD_LPSURFACE;
    }

	DX_STUB_str("pDDSD->dwFlags ok");

	/* own code now */

    /* FIXME 	
	   remove all old code for create a surface 
	   for we need rewrite it 
     */
          
    if (pDDSD->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
	{                         
            DX_STUB_str( "Can not create primary surface");
    }
    else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
    {       
             DX_STUB_str( "Can not create overlay surface");
    }	
    else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_BACKBUFFER)
	{
             DX_STUB_str( "Can not create backbuffer surface");
    }
    else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_TEXTURE)
    {
             DX_STUB_str( "Can not create texture surface");
    }
    else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_ZBUFFER)
    {
             DX_STUB_str( "Can not create zbuffer surface");
    }
    else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_OFFSCREENPLAIN) 
    {
             DX_STUB_str( "Can not create offscreenplain surface");
    }
  
	DX_STUB_str("DDERR_INVALIDSURFACETYPE");
    return DDERR_INVALIDSURFACETYPE;  
   
}


/*
 * stub
 * Status not done
 */
HRESULT WINAPI Main_DirectDraw_DuplicateSurface(LPDIRECTDRAW7 iface, LPDIRECTDRAWSURFACE7 src,
                 LPDIRECTDRAWSURFACE7* dst) 
{
    DX_WINDBG_trace();
    DX_STUB;    
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI Main_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags,
                 LPDDSURFACEDESC2 pDDSD, LPVOID context, LPDDENUMMODESCALLBACK2 callback) 
{
     
	DX_STUB_DD_OK;

 //   IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
 //   DDSURFACEDESC2 desc_callback;
 //   DEVMODE DevMode;   
 //   int iMode=0;
	//
	//RtlZeroMemory(&desc_callback, sizeof(DDSURFACEDESC2));
	//		    	   
 //   desc_callback.dwSize = sizeof(DDSURFACEDESC2);	

 //   desc_callback.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_PITCH;

 //   if (dwFlags & DDEDM_REFRESHRATES)
 //   {
	//    desc_callback.dwFlags |= DDSD_REFRESHRATE;
 //       desc_callback.dwRefreshRate = This->lpLcl->lpGbl->dwMonitorFrequency;
 //   }

 // 
 //    /* FIXME check if the mode are suppretd before sending it back  */

	//memset(&DevMode,0,sizeof(DEVMODE));
	//DevMode.dmSize = (WORD)sizeof(DEVMODE);
	//DevMode.dmDriverExtra = 0;

 //   while (EnumDisplaySettingsEx(NULL, iMode, &DevMode, 0))
 //   {
 //      
	//   if (pDDSD)
	//   {
	//       if ((pDDSD->dwFlags & DDSD_WIDTH) && (pDDSD->dwWidth != DevMode.dmPelsWidth))
	//       continue; 
	//       if ((pDDSD->dwFlags & DDSD_HEIGHT) && (pDDSD->dwHeight != DevMode.dmPelsHeight))
	//	   continue; 
	//       if ((pDDSD->dwFlags & DDSD_PIXELFORMAT) && (pDDSD->ddpfPixelFormat.dwFlags & DDPF_RGB) &&
	//	   (pDDSD->ddpfPixelFormat.dwRGBBitCount != DevMode.dmBitsPerPel))
	//	    continue; 
 //      } 
	//
 //      desc_callback.dwHeight = DevMode.dmPelsHeight;
	//   desc_callback.dwWidth = DevMode.dmPelsWidth;
	//   
 //      if (DevMode.dmFields & DM_DISPLAYFREQUENCY)
 //      {
 //           desc_callback.dwRefreshRate = DevMode.dmDisplayFrequency;
 //      }

	//   if (desc_callback.dwRefreshRate == 0)
	//   {
	//	   DX_STUB_str("dwRefreshRate = 0, we hard code it to value 60");
 //          desc_callback.dwRefreshRate = 60; /* Maybe the valye should be biger */
	//   }

 //     /* above same as wine */
	//  if ((pDDSD->dwFlags & DDSD_PIXELFORMAT) && (pDDSD->ddpfPixelFormat.dwFlags & DDPF_RGB) )
	//  {         
	//		switch(DevMode.dmBitsPerPel)
	//		{
	//			case  8:
	//				 desc_callback.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	//				 desc_callback.ddpfPixelFormat.dwFlags = DDPF_RGB;
	//				 desc_callback.ddpfPixelFormat.dwFourCC = 0;
	//				 desc_callback.ddpfPixelFormat.dwRGBBitCount=8;
	//				 /* FIXME right value */
	//				 desc_callback.ddpfPixelFormat.dwRBitMask = 0xFF0000; /* red bitmask */
	//				 desc_callback.ddpfPixelFormat.dwGBitMask = 0; /* Green bitmask */
	//				 desc_callback.ddpfPixelFormat.dwBBitMask = 0; /* Blue bitmask */
	//				break;

	//			case 15:
	//				desc_callback.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	//				 desc_callback.ddpfPixelFormat.dwFlags = DDPF_RGB;
	//				 desc_callback.ddpfPixelFormat.dwFourCC = 0;
	//				 desc_callback.ddpfPixelFormat.dwRGBBitCount=15;
	//				 /* FIXME right value */
	//				 desc_callback.ddpfPixelFormat.dwRBitMask = 0x7C00; /* red bitmask */
	//				 desc_callback.ddpfPixelFormat.dwGBitMask = 0x3E0; /* Green bitmask */
	//				 desc_callback.ddpfPixelFormat.dwBBitMask = 0x1F; /* Blue bitmask */
	//				break;

	//			case 16: 
	//				 desc_callback.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	//				 desc_callback.ddpfPixelFormat.dwFlags = DDPF_RGB;
	//				 desc_callback.ddpfPixelFormat.dwFourCC = 0;
	//				 desc_callback.ddpfPixelFormat.dwRGBBitCount=16;
	//				 /* FIXME right value */
	//				 desc_callback.ddpfPixelFormat.dwRBitMask = 0xF800; /* red bitmask */
	//				 desc_callback.ddpfPixelFormat.dwGBitMask = 0x7E0; /* Green bitmask */
	//				 desc_callback.ddpfPixelFormat.dwBBitMask = 0x1F; /* Blue bitmask */
	//				break;

	//			case 24: 
	//				 desc_callback.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	//				 desc_callback.ddpfPixelFormat.dwFlags = DDPF_RGB;
	//				 desc_callback.ddpfPixelFormat.dwFourCC = 0;
	//				 desc_callback.ddpfPixelFormat.dwRGBBitCount=24;
	//				 /* FIXME right value */
	//				 desc_callback.ddpfPixelFormat.dwRBitMask = 0xFF0000; /* red bitmask */
	//				 desc_callback.ddpfPixelFormat.dwGBitMask = 0x00FF00; /* Green bitmask */
	//				 desc_callback.ddpfPixelFormat.dwBBitMask = 0x0000FF; /* Blue bitmask */
	//				break;

	//			case 32: 
	//				 desc_callback.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
	//				 desc_callback.ddpfPixelFormat.dwFlags = DDPF_RGB;
	//				 desc_callback.ddpfPixelFormat.dwFourCC = 0;
	//				 desc_callback.ddpfPixelFormat.dwRGBBitCount=8;
	//				 /* FIXME right value */
	//				 desc_callback.ddpfPixelFormat.dwRBitMask = 0xFF0000; /* red bitmask */
	//				 desc_callback.ddpfPixelFormat.dwGBitMask = 0x00FF00; /* Green bitmask */
	//				 desc_callback.ddpfPixelFormat.dwBBitMask = 0x0000FF; /* Blue bitmask */
	//				break;

	//			default:
	//				break;          
	//		}
	//		desc_callback.ddsCaps.dwCaps = 0;
	//	    if (desc_callback.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) 
	//	    {
	//			/* FIXME srt DDCS Caps flag */
	//			desc_callback.ddsCaps.dwCaps |= DDSCAPS_PALETTE;
	//	    }    
	//  }
	//			  
	//  if (DevMode.dmBitsPerPel==15)
 //     {           
	//	  desc_callback.lPitch =  DevMode.dmPelsWidth + (8 - ( DevMode.dmPelsWidth % 8)) % 8;
	//  }
	//  else
	//  {
	//	  desc_callback.lPitch = DevMode.dmPelsWidth * (DevMode.dmBitsPerPel  / 8);
	//      desc_callback.lPitch = desc_callback.lPitch + (8 - (desc_callback.lPitch % 8)) % 8;
	//  }
	//  	 	  	                                                  
 //     if (callback(&desc_callback, context) == DDENUMRET_CANCEL)
 //     {
 //        return DD_OK;       
 //     }
 //      
 //     iMode++; 
 //   }

 //   return DD_OK;
}

/*
 * stub
 * Status not done
 */
HRESULT WINAPI 
Main_DirectDraw_EnumSurfaces(LPDIRECTDRAW7 iface, DWORD dwFlags,
                 LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
                 LPDDENUMSURFACESCALLBACK7 callback) 
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface) 
{
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
	DDHAL_FLIPTOGDISURFACEDATA mDdFlipToGDISurface;

	DX_WINDBG_trace();
	
	mDdFlipToGDISurface.ddRVal = DDERR_NOTINITIALIZED;
	mDdFlipToGDISurface.dwReserved = 0;
	mDdFlipToGDISurface.dwToGDI = TRUE;
	mDdFlipToGDISurface.FlipToGDISurface = This->lpLcl->lpDDCB->cbDDCallbacks.FlipToGDISurface;

	if (mDdFlipToGDISurface.FlipToGDISurface == NULL)
	{
		return  DDERR_NODRIVERSUPPORT;	 		    	
	}

	if (mDdFlipToGDISurface.FlipToGDISurface(&mDdFlipToGDISurface)==DDHAL_DRIVER_HANDLED);
    {				
		return mDdFlipToGDISurface.ddRVal;
	}

	return  DDERR_NODRIVERSUPPORT;	 		    	   
}
 
/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
            LPDDCAPS pHELCaps) 
{	
    

    DDSCAPS2 ddscaps = {0};
    DWORD status = DD_FALSE;	
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

	DX_WINDBG_trace();
	
    if (pDriverCaps != NULL) 
    {                          
	  Main_DirectDraw_GetAvailableVidMem(iface, 
		                                 &ddscaps,
		                                 &This->lpLcl->lpGbl->ddCaps.dwVidMemTotal, 
		                                 &This->lpLcl->lpGbl->ddCaps.dwVidMemFree);	 
	  
	  RtlCopyMemory(pDriverCaps,&This->lpLcl->lpGbl->ddCaps,sizeof(DDCORECAPS));
	  pDriverCaps->dwSize=sizeof(DDCAPS);
	  
      status = DD_OK;
    }

    if (pHELCaps != NULL) 
    {      	  
	  Main_DirectDraw_GetAvailableVidMem(iface, 
		                                 &ddscaps,
		                                 &This->lpLcl->lpGbl->ddHELCaps.dwVidMemTotal, 
		                                 &This->lpLcl->lpGbl->ddHELCaps.dwVidMemFree);	  	 

	  RtlCopyMemory(pDriverCaps,&This->lpLcl->lpGbl->ddHELCaps,sizeof(DDCORECAPS));
      status = DD_OK;
    }
    
    return status;
}


/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI Main_DirectDraw_GetDisplayMode(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD) 
{       
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

	DX_WINDBG_trace();

    if (pDDSD == NULL)
    {
      return DD_FALSE;
    }
        
    pDDSD->dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE | DDSD_WIDTH; 
	pDDSD->dwHeight  = This->lpLcl->lpGbl->vmiData.dwDisplayHeight;
    pDDSD->dwWidth = This->lpLcl->lpGbl->vmiData.dwDisplayWidth; 
    pDDSD->lPitch  = This->lpLcl->lpGbl->vmiData.lDisplayPitch;
    pDDSD->dwRefreshRate = This->lpLcl->lpGbl->dwMonitorFrequency;
    pDDSD->dwAlphaBitDepth = This->lpLcl->lpGbl->vmiData.ddpfDisplay.dwAlphaBitDepth;

    RtlCopyMemory(&pDDSD->ddpfPixelFormat,&This->lpLcl->lpGbl->vmiData.ddpfDisplay,sizeof(DDPIXELFORMAT));
    RtlCopyMemory(&pDDSD->ddsCaps,&This->lpLcl->lpGbl->ddCaps,sizeof(DDCORECAPS));
    
    RtlCopyMemory(&pDDSD->ddckCKDestOverlay,&This->lpLcl->lpGbl->ddckCKDestOverlay,sizeof(DDCOLORKEY));
    RtlCopyMemory(&pDDSD->ddckCKSrcOverlay,&This->lpLcl->lpGbl->ddckCKSrcOverlay,sizeof(DDCOLORKEY));

    /* have not check where I should get hold of this info yet
	DWORD  dwBackBufferCount;    
    DWORD  dwReserved;
    LPVOID lpSurface;    
    DDCOLORKEY    ddckCKDestBlt;    
    DDCOLORKEY    ddckCKSrcBlt;  
    DWORD         dwTextureStage;
    */
  
    return DD_OK;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI 
Main_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD pNumCodes, LPDWORD pCodes)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI 
Main_DirectDraw_GetGDISurface(LPDIRECTDRAW7 iface, 
                                             LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_GetMonitorFrequency(LPDIRECTDRAW7 iface,LPDWORD freq)
{      
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

	DX_WINDBG_trace();

    if (freq == NULL)
    {
        return DD_FALSE;
    }

	*freq = This->lpLcl->lpGbl->dwMonitorFrequency;
    return DD_OK;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine)
{    		
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
	DDHAL_GETSCANLINEDATA mDdGetScanLine;

	DX_WINDBG_trace();

    *lpdwScanLine = 0;

	mDdGetScanLine.ddRVal = DDERR_NOTINITIALIZED;
	mDdGetScanLine.dwScanLine = 0;
	mDdGetScanLine.GetScanLine = This->lpLcl->lpDDCB->cbDDCallbacks.GetScanLine;
	mDdGetScanLine.lpDD = This->lpLcl->lpGbl;

	if (mDdGetScanLine.GetScanLine == NULL)
	{
		return  DDERR_NODRIVERSUPPORT;	 		    	
	}

	mDdGetScanLine.ddRVal = DDERR_NOTPALETTIZED;
	mDdGetScanLine.dwScanLine = 0;

	if (mDdGetScanLine.GetScanLine(&mDdGetScanLine)==DDHAL_DRIVER_HANDLED);
    {			
		*lpdwScanLine = mDdGetScanLine.dwScanLine;
		return mDdGetScanLine.ddRVal;
	}

	return  DDERR_NODRIVERSUPPORT;	 		    	       
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI 
Main_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7 iface, LPBOOL lpbIsInVB)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT 
WINAPI 
Main_DirectDraw_Initialize (LPDIRECTDRAW7 iface, LPGUID lpGUID)
{          
    //LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

	DX_WINDBG_trace();
       
	if (iface==NULL) 
	{
		return DDERR_NOTINITIALIZED;
	}
	
    return DDERR_ALREADYINITIALIZED;	
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface)
{
   DX_WINDBG_trace();

   ChangeDisplaySettings(NULL, 0);
   return DD_OK;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface, HWND hwnd, DWORD cooplevel)
{
    // TODO:                                                            
    // - create a scaner that check which driver we should get the HDC from    
    //   for now we always asume it is the active dirver that should be use.
    // - allow more Flags

    

 //   LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
 //   DDHAL_SETEXCLUSIVEMODEDATA SetExclusiveMode;

	//DX_WINDBG_trace();
 //   
	//
 //   // check the parameters
 //   if ((HWND)This->lpLcl->lpGbl->lpExclusiveOwner->hWnd  == hwnd)
 //       return DD_OK;
 //   
 //  

 //   if ((cooplevel&DDSCL_EXCLUSIVE) && !(cooplevel&DDSCL_FULLSCREEN))
 //       return DDERR_INVALIDPARAMS;

 //   if (cooplevel&DDSCL_NORMAL && cooplevel&DDSCL_FULLSCREEN)
 //       return DDERR_INVALIDPARAMS;

 //   // set the data
 //   This->lpLcl->lpGbl->lpExclusiveOwner->hWnd = (ULONG_PTR) hwnd;
 //   This->lpLcl->lpGbl->lpExclusiveOwner->hDC  = (ULONG_PTR)GetDC(hwnd);

	//
	///* FIXME : fill the  mDDrawGlobal.lpExclusiveOwner->dwLocalFlags right */
	////mDDrawGlobal.lpExclusiveOwner->dwLocalFlags


 //   SetExclusiveMode.ddRVal = DDERR_NOTPALETTIZED;
	//if ((This->lpLcl->lpGbl->lpDDCBtmp->cbDDCallbacks.dwFlags & DDHAL_CB32_SETEXCLUSIVEMODE)) 
 //   {       
 //      
 //       SetExclusiveMode.SetExclusiveMode = This->lpLcl->lpGbl->lpDDCBtmp->cbDDCallbacks.SetExclusiveMode;   
	//	SetExclusiveMode.lpDD = This->lpLcl->lpGbl;       
 //       SetExclusiveMode.dwEnterExcl = cooplevel;

	//	if (SetExclusiveMode.SetExclusiveMode(&SetExclusiveMode) != DDHAL_DRIVER_HANDLED)
 //       {
 //           return DDERR_NODRIVERSUPPORT;
 //       }
 //   }
 //                  
 //   return SetExclusiveMode.ddRVal;  
	DX_STUB_DD_OK;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 iface, DWORD dwWidth, DWORD dwHeight, 
                                                                DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
   

    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
	BOOL dummy = TRUE;
	DEVMODE DevMode; 	
    int iMode=0;
    int Width=0;
    int Height=0;
    int BPP=0;
	DDHAL_SETMODEDATA mDdSetMode;

	 DX_WINDBG_trace();
	
	/* FIXME check the refresrate if it same if it not same do the mode switch */
	 
	if ((This->lpLcl->lpGbl->vmiData.dwDisplayHeight == dwHeight) && 
		(This->lpLcl->lpGbl->vmiData.dwDisplayWidth == dwWidth)  && 
		(This->lpLcl->lpGbl->vmiData.ddpfDisplay.dwRGBBitCount == dwBPP))  
		{
          
		  return DD_OK;
		}

	mDdSetMode.ddRVal = DDERR_NOTINITIALIZED;
	mDdSetMode.dwModeIndex = 0;
	mDdSetMode.inexcl = 0;
	mDdSetMode.lpDD = This->lpLcl->lpGbl;
	mDdSetMode.useRefreshRate = FALSE;
	mDdSetMode.SetMode = This->lpLcl->lpDDCB->cbDDCallbacks.SetMode;

	if (mDdSetMode.SetMode == NULL)
	{
		return  DDERR_NODRIVERSUPPORT;	 		    	
	}

	/* Check use the Hal or Hel for SetMode */
	// this only for exclusive mode
	/*if(!(This->cooperative_level & DDSCL_EXCLUSIVE))
	{
   		return DDERR_NOEXCLUSIVEMODE;
	}*/

	DevMode.dmSize = (WORD)sizeof(DEVMODE);
	DevMode.dmDriverExtra = 0;

    while (EnumDisplaySettingsEx(NULL, iMode, &DevMode, 0 ) != 0)
    {
	 
       if ((dwWidth == DevMode.dmPelsWidth) && (dwHeight == DevMode.dmPelsHeight) && ( dwBPP == DevMode.dmBitsPerPel))
	   {
		  Width = DevMode.dmPelsWidth;
		  Height = DevMode.dmPelsHeight;
          BPP = DevMode.dmBitsPerPel;		  
          break;   
	   }		   
	   iMode++;		   
    }

	if ((dwWidth != DevMode.dmPelsWidth) || (dwHeight != DevMode.dmPelsHeight) || ( dwBPP != DevMode.dmBitsPerPel))	
	{	
		return DDERR_UNSUPPORTEDMODE;
	}
	
	
	
	mDdSetMode.dwModeIndex = iMode; 
    mDdSetMode.SetMode(&mDdSetMode);
	
	DdReenableDirectDrawObject(This->lpLcl->lpGbl, &dummy);

	/* FIXME fill the This->DirectDrawGlobal.vmiData right */		     
     //This->lpLcl->lpGbl->lpExclusiveOwner->hDC  = (ULONG_PTR)GetDC( (HWND)This->lpLcl->lpGbl->lpExclusiveOwner->hWnd);  
	return mDdSetMode.ddRVal;
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
                                                   HANDLE h)
{
    	
    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;
	DDHAL_WAITFORVERTICALBLANKDATA mDdWaitForVerticalBlank;

	DX_WINDBG_trace();
    
	if (!(This->lpLcl->lpDDCB->cbDDCallbacks.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK))
	{
		return  DDERR_NODRIVERSUPPORT;	 		    	
	}

	if (mDdWaitForVerticalBlank.WaitForVerticalBlank == NULL)
	{
		return  DDERR_NODRIVERSUPPORT;	 		    	
	}

	mDdWaitForVerticalBlank.bIsInVB = DDWAITVB_BLOCKBEGIN ; /* return begin ? */
	mDdWaitForVerticalBlank.ddRVal = DDERR_NOTINITIALIZED;
	mDdWaitForVerticalBlank.dwFlags = dwFlags;
	mDdWaitForVerticalBlank.hEvent = (DWORD)h;
	mDdWaitForVerticalBlank.lpDD = This->lpLcl->lpGbl;
	mDdWaitForVerticalBlank.WaitForVerticalBlank = This->lpLcl->lpDDCB->cbDDCallbacks.WaitForVerticalBlank;

	if (mDdWaitForVerticalBlank.WaitForVerticalBlank(&mDdWaitForVerticalBlank)==DDHAL_DRIVER_HANDLED);
    {					
		return mDdWaitForVerticalBlank.ddRVal;
	}

	return  DDERR_NODRIVERSUPPORT;	 
}

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT WINAPI 
Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
                   LPDWORD total, LPDWORD free)                                               
{        
	DDHAL_GETAVAILDRIVERMEMORYDATA  mem;

    LPDDRAWI_DIRECTDRAW_INT This = (LPDDRAWI_DIRECTDRAW_INT)iface;

	DX_WINDBG_trace();
    
	
	/* Only Hal version exists acodring msdn */
	if (!(This->lpLcl->lpDDCB->cbDDMiscellaneousCallbacks.dwFlags & DDHAL_MISCCB32_GETAVAILDRIVERMEMORY))
	{
		return  DDERR_NODRIVERSUPPORT;	 		    	
	}
	
	mem.lpDD = This->lpLcl->lpGbl;
    mem.ddRVal = DDERR_NOTPALETTIZED;	
    mem.DDSCaps.dwCaps = ddscaps->dwCaps;
    mem.ddsCapsEx.dwCaps2 = ddscaps->dwCaps2;
    mem.ddsCapsEx.dwCaps3 = ddscaps->dwCaps3;
    mem.ddsCapsEx.dwCaps4 = ddscaps->dwCaps4;

	if (This->lpLcl->lpDDCB->cbDDMiscellaneousCallbacks.GetAvailDriverMemory(&mem) == DDHAL_DRIVER_HANDLED);
    {
		if (total !=NULL)
		{
           *total = mem.dwTotal;
		}

        *free = mem.dwFree;
		return mem.ddRVal;
	}

	return  DDERR_NODRIVERSUPPORT;	 
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7 iface, HDC hdc,
                                                LPDIRECTDRAWSURFACE7 *lpDDS)
{  
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface) 
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
                   LPDDDEVICEIDENTIFIER2 pDDDI, DWORD dwFlags)
{    
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_StartModeTest(LPDIRECTDRAW7 iface, LPSIZE pModes,
                  DWORD dwNumModes, DWORD dwFlags)
{
    DX_WINDBG_trace();
    DX_STUB;
}

/*
 * Stub
 * Status todo
 */
HRESULT WINAPI Main_DirectDraw_EvaluateMode(LPDIRECTDRAW7 iface,DWORD a,DWORD* b)
{  
    DX_WINDBG_trace();
    DX_STUB;
}


IDirectDraw7Vtbl DirectDraw7_Vtable =
{
    Main_DirectDraw_QueryInterface,
    Main_DirectDraw_AddRef,
    Main_DirectDraw_Release,
    Main_DirectDraw_Compact,
    Main_DirectDraw_CreateClipper,
    Main_DirectDraw_CreatePalette,
    Main_DirectDraw_CreateSurface,
    Main_DirectDraw_DuplicateSurface,
    Main_DirectDraw_EnumDisplayModes,
    Main_DirectDraw_EnumSurfaces,
    Main_DirectDraw_FlipToGDISurface,
    Main_DirectDraw_GetCaps,
    Main_DirectDraw_GetDisplayMode,
    Main_DirectDraw_GetFourCCCodes,
    Main_DirectDraw_GetGDISurface,
    Main_DirectDraw_GetMonitorFrequency,
    Main_DirectDraw_GetScanLine,
    Main_DirectDraw_GetVerticalBlankStatus,
    Main_DirectDraw_Initialize,
    Main_DirectDraw_RestoreDisplayMode,
    Main_DirectDraw_SetCooperativeLevel,
    Main_DirectDraw_SetDisplayMode,
    Main_DirectDraw_WaitForVerticalBlank,
    Main_DirectDraw_GetAvailableVidMem,
    Main_DirectDraw_GetSurfaceFromDC,
    Main_DirectDraw_RestoreAllSurfaces,
    Main_DirectDraw_TestCooperativeLevel,
    Main_DirectDraw_GetDeviceIdentifier,
    Main_DirectDraw_StartModeTest,
    Main_DirectDraw_EvaluateMode
};
