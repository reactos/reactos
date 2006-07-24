/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/ddraw.c
 * PURPOSE:              IDirectDraw7 Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"

/*
 * IMPLEMENT
 * Status ok
 */
HRESULT 
WINAPI 
Main_DirectDraw_QueryInterface (LPDIRECTDRAW7 iface, 
								REFIID id, 
								LPVOID *obj) 
{
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    
    if (IsEqualGUID(&IID_IDirectDraw7, id))
    {
        *obj = &This->lpVtbl;
    }
    else if (IsEqualGUID(&IID_IDirectDraw, id))
    {
        *obj = &This->lpVtbl_v1;
    }
    else if (IsEqualGUID(&IID_IDirectDraw2, id))
    {
        *obj = &This->lpVtbl_v2;
    }
    else if (IsEqualGUID(&IID_IDirectDraw4, id))
    {
        *obj = &This->lpVtbl_v4;
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
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
	ULONG ref=0;
    
	if (iface!=NULL)
	{
        ref = InterlockedIncrement(  (PLONG) &This->mDDrawLocal.dwLocalRefCnt);       
	}    
    return ref;
}

/*
 * IMPLEMENT
 * Status ok
 */
ULONG 
WINAPI 
Main_DirectDraw_Release (LPDIRECTDRAW7 iface) 
{
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
	ULONG ref=0;

	if (iface!=NULL)
	{	  	
		ref = InterlockedDecrement( (PLONG) &This->mDDrawLocal.dwLocalRefCnt);
            
		if (ref == 0)
		{
			// set resoltion back to the one in registry
			if(This->cooperative_level & DDSCL_EXCLUSIVE)
			{
				ChangeDisplaySettings(NULL, 0);
			}
            
			Cleanup(iface);					
            if (This!=NULL)
            {              
			    HeapFree(GetProcessHeap(), 0, This);
            }
		}
    }
    return ref;
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
    DX_WINDBG_trace();

    if (pUnkOuter!=NULL) 
        return DDERR_INVALIDPARAMS; 

    IDirectDrawClipperImpl* That; 
    That = (IDirectDrawClipperImpl*)HeapAlloc(GetProcessHeap(), 0, sizeof(IDirectDrawClipperImpl));

    if (That == NULL) 
        return E_OUTOFMEMORY;

    ZeroMemory(That, sizeof(IDirectDrawClipperImpl));

    That->lpVtbl = &DirectDrawClipper_Vtable;
    That->ref = 1;
    *ppClipper = (LPDIRECTDRAWCLIPPER)That;

    return That->lpVtbl->Initialize (*ppClipper, (LPDIRECTDRAW)iface, dwFlags);
}


HRESULT WINAPI Main_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
                  LPPALETTEENTRY palent, LPDIRECTDRAWPALETTE* ppPalette, LPUNKNOWN pUnkOuter)
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
	IDirectDrawPaletteImpl* That; 
	DX_WINDBG_trace();

    if (pUnkOuter!=NULL) 
	{
        return CLASS_E_NOAGGREGATION; 
	}
    
	if(!This->cooperative_level)
    {
        return DDERR_NOCOOPERATIVELEVELSET;
    }

	if (This->mDdCreatePalette.CreatePalette == NULL)
	{
		return DDERR_NODRIVERSUPPORT;
	}

    That = (IDirectDrawPaletteImpl*)DxHeapMemAlloc(sizeof(IDirectDrawPaletteImpl));
    if (That == NULL) 
	{
        return E_OUTOFMEMORY;
	}

    That->lpVtbl = &DirectDrawPalette_Vtable;    
    *ppPalette = (LPDIRECTDRAWPALETTE)That;

	That->DDPalette.dwRefCnt = 1;
    That->DDPalette.lpDD_lcl = &This->mDDrawLocal;
	That->DDPalette.dwProcessId = GetCurrentProcessId();
    That->DDPalette.lpColorTable = palent;

    if (dwFlags & DDPCAPS_1BIT)
	{
		That->DDPalette.dwFlags |= DDRAWIPAL_2 ;
	}
	if (dwFlags & DDPCAPS_2BIT)
	{
		That->DDPalette.dwFlags |= DDRAWIPAL_4 ;
	}
	if (dwFlags & DDPCAPS_4BIT)
	{
		That->DDPalette.dwFlags |= DDRAWIPAL_16 ;
	}
	if (dwFlags & DDPCAPS_8BIT)
	{
		That->DDPalette.dwFlags |= DDRAWIPAL_256 ;
	}

	if (dwFlags & DDPCAPS_ALPHA)
	{
		That->DDPalette.dwFlags |= DDRAWIPAL_ALPHA;
	}
	if (dwFlags & DDPCAPS_ALLOW256)
	{
		That->DDPalette.dwFlags |= DDRAWIPAL_ALLOW256 ;
	}
	/* FIXME see 
	   http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wceddk5/html/wce50lrfddrawiddrawpalettegbl.asp
     
	if (dwFlags & DDPCAPS_8BITENTRIES)
	{
		That->DDPalette.dwFlags |= 0;
	}
		
	if (dwFlags & DDPCAPS_INITIALIZE)
	{
		That->DDPalette.dwFlags |= 0;
	}
	if (dwFlags & DDPCAPS_PRIMARYSURFACE)
	{
		That->DDPalette.dwFlags |= 0;
	}
	if (dwFlags & DDPCAPS_PRIMARYSURFACELEFT)
	{
		That->DDPalette.dwFlags |= 0;
	}
	if (dwFlags & DDPCAPS_VSYNC)
	{
		That->DDPalette.dwFlags |= 0;
	}
    */


	/*  We need fill in this right 
	   That->DDPalette.hHELGDIPalette/dwReserved1 
       That->DDPalette.dwContentsStamp 
       That->DDPalette.dwSaveStamp 
     */
	
	This->mDdCreatePalette.lpDDPalette = &That->DDPalette;
	This->mDdCreatePalette.lpColorTable = palent;
	
	if (This->mDdCreatePalette.CreatePalette(&This->mDdCreatePalette) == DDHAL_DRIVER_HANDLED);
    {				
		if (This->mDdSetMode.ddRVal == DD_OK)
		{
			Main_DirectDraw_AddRef(iface);
			return That->lpVtbl->Initialize (*ppPalette, (LPDIRECTDRAW)iface, dwFlags, palent);
		}
	}

	return  DDERR_NODRIVERSUPPORT;	 		    			    
}




HRESULT WINAPI Main_DirectDraw_CreateSurface (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
                                            LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter) 
{
    DX_WINDBG_trace();

    DxSurf *surf;

	IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

    IDirectDrawSurfaceImpl* That; 


	if (pUnkOuter!=NULL)       
    {
		DX_STUB_str("Fail Main_DirectDraw_CreateSurface CLASS_E_NOAGGREGATION");
        return CLASS_E_NOAGGREGATION; 
    }

	if( sizeof(DDSURFACEDESC2)!=pDDSD->dwSize && sizeof(DDSURFACEDESC)!=pDDSD->dwSize)
	{
		DX_STUB_str("Fail Main_DirectDraw_CreateSurface DDERR_UNSUPPORTED1");
	    return DDERR_UNSUPPORTED;
	}

	if( This == NULL)
	{
		DX_STUB_str("Fail Main_DirectDraw_CreateSurface DDERR_UNSUPPORTED2");
	    return DDERR_UNSUPPORTED;
	}
    
	if (This->mDDrawGlobal.dsList == NULL)
	{
		/* Fail alloc memmory at startup */
		DX_STUB_str("Fail Main_DirectDraw_CreateSurface E_OUTOFMEMORY1");
	    return E_OUTOFMEMORY;
	}
      
	if (pDDSD->ddsCaps.dwCaps == 0)
    {        
        pDDSD->ddsCaps.dwCaps = DDSCAPS_LOCALVIDMEM | DDSCAPS_VIDEOMEMORY;
    }

	if (pDDSD->ddsCaps.dwCaps & DDSCAPS_ALLOCONLOAD)
    {       
        pDDSD->dwFlags &= ~DDSD_LPSURFACE;
    }

	if ((pDDSD->dwFlags & DDSD_LPSURFACE) && (pDDSD->lpSurface == NULL))
    {             
         pDDSD->dwFlags &= ~DDSD_LPSURFACE;
    }

	That = (IDirectDrawSurfaceImpl*)DxHeapMemAlloc(sizeof(IDirectDrawSurfaceImpl));
    if (That == NULL) 
	{
		DX_STUB_str("Fail Main_DirectDraw_CreateSurface E_OUTOFMEMORY2");
        return E_OUTOFMEMORY;
	}

	surf = (DxSurf*)DxHeapMemAlloc(sizeof(DxSurf));
    if (surf == NULL) 
	{
		DX_STUB_str("Fail Main_DirectDraw_CreateSurface E_OUTOFMEMORY3");
        DxHeapMemFree(That);
        return E_OUTOFMEMORY;
	}

    // the nasty com stuff
    That->lpVtbl = &DirectDrawSurface7_Vtable;
    That->lpVtbl_v3 = &DDRAW_IDDS3_Thunk_VTable;
	*ppSurf = (LPDIRECTDRAWSURFACE7)That;        

	/* setup some stuff */
    That->Owner = (IDirectDrawImpl *)This;
    That->Owner->mDDrawGlobal.dsList->dwIntRefCnt =1;
	That->Surf = surf;

    /* we alwasy set to use the DirectDrawSurface7_Vtable as internel */
    That->Owner->mDDrawGlobal.dsList->lpVtbl = (PVOID) &DirectDrawSurface7_Vtable;
             
    if (pDDSD->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {              
           memcpy(&That->Surf->mddsdPrimary,pDDSD,sizeof(DDSURFACEDESC));
           That->Surf->mddsdPrimary.dwSize      = sizeof(DDSURFACEDESC);          
           This->mDdCanCreateSurface.bIsDifferentPixelFormat = FALSE; 
           This->mDdCanCreateSurface.lpDDSurfaceDesc = (DDSURFACEDESC*)&That->Surf->mddsdPrimary; 

           if (This->mDdCanCreateSurface.CanCreateSurface(&This->mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
           {  
			  DX_STUB_str("Fail Main_DirectDraw_CreateSurface DDERR_NOTINITIALIZED1");
			  DxHeapMemFree(That);
			  DxHeapMemFree(surf);
              return DDERR_NOTINITIALIZED;
           }

           if (This->mDdCanCreateSurface.ddRVal != DD_OK)
           {
			  DX_STUB_str("Fail Main_DirectDraw_CreateSurface DDERR_NOTINITIALIZED2");
			  DxHeapMemFree(That);
			  DxHeapMemFree(surf);
              return DDERR_NOTINITIALIZED;
           }

           memset(&That->Surf->mPrimaryMore,   0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
           That->Surf->mPrimaryMore.dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

           memset(&That->Surf->mPrimaryLocal,  0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
           That->Surf->mPrimaryLocal.lpGbl = &That->Owner->mPrimaryGlobal;
           That->Surf->mPrimaryLocal.lpSurfMore = &That->Surf->mPrimaryMore;
           That->Surf->mPrimaryLocal.dwProcessId = GetCurrentProcessId();
	   
           /*
              FIXME Check the flags if we shall create a primaresurface for overlay or something else 
              Examine windows which flags are being set for we assume this is right unsue I think
           */
           //That->Surf->mPrimaryLocal.dwFlags = DDRAWISURF_PARTOFPRIMARYCHAIN|DDRAWISURF_HASOVERLAYDATA;

		   /*
		       FIXME we are now supproting DDSURFACEDESC2 in rosdraw.h we need make use of it it
		    */
           That->Surf->mPrimaryLocal.ddsCaps.dwCaps = That->Surf->mddsdPrimary.ddsCaps.dwCaps;
           That->Surf->mpPrimaryLocals[0] = &That->Surf->mPrimaryLocal;
          
           This->mDdCreateSurface.lpDDSurfaceDesc = (DDSURFACEDESC*)&That->Surf->mddsdPrimary;
           This->mDdCreateSurface.lplpSList = That->Surf->mpPrimaryLocals;
           This->mDdCreateSurface.dwSCnt = This->mDDrawGlobal.dsList->dwIntRefCnt ; 

            
           if (This->mDdCreateSurface.CreateSurface(&This->mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
           {
			  DX_STUB_str("Fail Main_DirectDraw_CreateSurface DDERR_NOTINITIALIZED3");
			  DxHeapMemFree(That);
			  DxHeapMemFree(surf);
              return DDERR_NOTINITIALIZED;
           }

           if (This->mDdCreateSurface.ddRVal != DD_OK) 
           {   
              DX_STUB_str("Fail Main_DirectDraw_CreateSurface ERROR");
			  DxHeapMemFree(That);
			  DxHeapMemFree(surf);
              return This->mDdCreateSurface.ddRVal;
           }
                     
            
           /* FIXME fill in this if they are avali
              DDSD_BACKBUFFERCOUNT 
              DDSD_CKDESTBLT 
              DDSD_CKDESTOVERLAY 
              DDSD_CKSRCBLT 
              DDSD_CKSRCOVERLAY 
              DDSD_LINEARSIZE 
              DDSD_LPSURFACE 
              DDSD_MIPMAPCOUNT 
              DDSD_ZBUFFERBITDEPTH
           */

           That->Surf->mddsdPrimary.dwFlags = DDSD_CAPS + DDSD_PIXELFORMAT;
           RtlCopyMemory(&That->Surf->mddsdPrimary.ddpfPixelFormat,&This->mDDrawGlobal.vmiData.ddpfDisplay,sizeof(DDPIXELFORMAT));
           //RtlCopyMemory(&That->Surf->mddsdPrimary.ddsCaps,&This->mDDrawGlobal.ddCaps,sizeof(DDCORECAPS));
           //RtlCopyMemory(&pDDSD->ddckCKDestOverlay,&This->mDDrawGlobal.ddckCKDestOverlay,sizeof(DDCOLORKEY));
           //RtlCopyMemory(&pDDSD->ddckCKSrcOverlay,&This->mDDrawGlobal.ddckCKSrcOverlay,sizeof(DDCOLORKEY));

           if (This->mDDrawGlobal.vmiData.dwDisplayHeight != 0)
           {
              That->Surf->mddsdPrimary.dwFlags += DDSD_HEIGHT ;
              That->Surf->mddsdPrimary.dwHeight  = This->mDDrawGlobal.vmiData.dwDisplayHeight;
           }

           if (This->mDDrawGlobal.vmiData.dwDisplayWidth != 0)
           {
              That->Surf->mddsdPrimary.dwFlags += DDSD_WIDTH ;
              That->Surf->mddsdPrimary.dwWidth = This->mDDrawGlobal.vmiData.dwDisplayWidth; 
           }

           if (This->mDDrawGlobal.vmiData.lDisplayPitch != 0)
           {
              That->Surf->mddsdPrimary.dwFlags += DDSD_PITCH ;           
              That->Surf->mddsdPrimary.lPitch  = This->mDDrawGlobal.vmiData.lDisplayPitch;
           }

           if ( This->mDDrawGlobal.dwMonitorFrequency != 0)
           {
              That->Surf->mddsdPrimary.dwFlags += DDSD_REFRESHRATE ;           
              That->Surf->mddsdPrimary.dwRefreshRate = This->mDDrawGlobal.dwMonitorFrequency;
           }
          
           if (This->mDDrawGlobal.vmiData.ddpfDisplay.dwAlphaBitDepth != 0)
           {
             That->Surf->mddsdPrimary.dwFlags += DDSD_ALPHABITDEPTH ;
             That->Surf->mddsdPrimary.dwAlphaBitDepth = This->mDDrawGlobal.vmiData.ddpfDisplay.dwAlphaBitDepth;
           }

           That->Surf->mpInUseSurfaceLocals[0] = &That->Surf->mPrimaryLocal;

		   DX_STUB_str("Fail Main_DirectDraw_CreateSurface OK");
           return DD_OK;

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
  
	DxHeapMemFree(That);
	DxHeapMemFree(surf);
    return DDERR_INVALIDSURFACETYPE;  
   
}






/*
 * IMPLEMENT
 * Status this api is finish and is 100% correct 
 */
HRESULT 
WINAPI 
Main_DirectDraw_Initialize (LPDIRECTDRAW7 iface, LPGUID lpGUID)
{       
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
       
	if (iface==NULL) 
	{
		return DDERR_NOTINITIALIZED;
	}

	if (This->InitializeDraw == TRUE)
	{
        return DDERR_ALREADYINITIALIZED;
	}
	else
	{
     This->InitializeDraw = TRUE;
    }

    return DD_OK;
}






/*
 * IMPLEMENT
 * Status 
 * not finish yet but is working fine 
 * it prevent memmory leaks at exit
 */






HRESULT WINAPI Main_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface, HWND hwnd, DWORD cooplevel)
{
    // TODO:                                                            
    // - create a scaner that check which driver we should get the HDC from    
    //   for now we always asume it is the active dirver that should be use.
    // - allow more Flags

    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    DDHAL_SETEXCLUSIVEMODEDATA SetExclusiveMode;
    
    // check the parameters
    if ((This->cooperative_level == cooplevel) && ((HWND)This->mDDrawGlobal.lpExclusiveOwner->hWnd  == hwnd))
        return DD_OK;
    
    if (This->cooperative_level)
        return DDERR_EXCLUSIVEMODEALREADYSET;

    if ((cooplevel&DDSCL_EXCLUSIVE) && !(cooplevel&DDSCL_FULLSCREEN))
        return DDERR_INVALIDPARAMS;

    if (cooplevel&DDSCL_NORMAL && cooplevel&DDSCL_FULLSCREEN)
        return DDERR_INVALIDPARAMS;

    // set the data
    This->mDDrawGlobal.lpExclusiveOwner->hWnd = (ULONG_PTR) hwnd;
    This->mDDrawGlobal.lpExclusiveOwner->hDC  = (ULONG_PTR)GetDC(hwnd);

	
	/* FIXME : fill the  mDDrawGlobal.lpExclusiveOwner->dwLocalFlags right */
	//mDDrawGlobal.lpExclusiveOwner->dwLocalFlags

    This->cooperative_level = cooplevel;

    if ((This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_SETEXCLUSIVEMODE)) 
    {       
        DX_STUB_str("HAL \n");
        SetExclusiveMode.SetExclusiveMode = This->mDDrawGlobal.lpDDCBtmp->HALDD.SetExclusiveMode;                            
    }
    else
    {
        DX_STUB_str("HEL \n");
        SetExclusiveMode.SetExclusiveMode = This->mDDrawGlobal.lpDDCBtmp->HELDD.SetExclusiveMode;
    }
             
    SetExclusiveMode.lpDD = &This->mDDrawGlobal;
    SetExclusiveMode.ddRVal = DDERR_NOTPALETTIZED;
    SetExclusiveMode.dwEnterExcl = This->cooperative_level;
     
    if (SetExclusiveMode.SetExclusiveMode(&SetExclusiveMode) != DDHAL_DRIVER_HANDLED)
    {
        return DDERR_NODRIVERSUPPORT;
    }

    return SetExclusiveMode.ddRVal;               
}

HRESULT WINAPI Main_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 iface, DWORD dwWidth, DWORD dwHeight, 
                                                                DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
	//BOOL dummy = TRUE;	
		
	DX_WINDBG_trace_res((int)dwWidth, (int)dwHeight, (int)dwBPP );
	/* FIXME check the refresrate if it same if it not same do the mode switch */
	if ((This->mDDrawGlobal.vmiData.dwDisplayHeight == dwHeight) && 
		(This->mDDrawGlobal.vmiData.dwDisplayWidth == dwWidth)  && 
		(This->mDDrawGlobal.vmiData.ddpfDisplay.dwRGBBitCount == dwBPP))  
		{
          
		  return DD_OK;
		}

	if (This->mDdSetMode.SetMode == NULL )
	{
		return DDERR_NODRIVERSUPPORT;
	}

	
	This->mDdSetMode.ddRVal = DDERR_NODRIVERSUPPORT;

    // FIXME : fill the mode.inexcl; 
    // FIXME : fill the mode.useRefreshRate; 
    
	/* FIXME 
	   we hardcoding modIndex list we should 
	   try getting it from ReactOS instead and compare it 
	   for now a small hack for do, using VBE 3.0 mode
	   index table todo it. 
	*/

	/* 320x200   15, 16, 32 */
	if ((dwHeight == 200) && (dwWidth == 320)  && (dwBPP == 15))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x10d;
	}

	if ((dwHeight == 200) && (dwWidth == 320)  && (dwBPP == 16))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x10e;
	}

	if ((dwHeight == 200) && (dwWidth == 320)  && (dwBPP == 32))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x10f;
	}


	/* 640x400   8 */
	if ((dwHeight == 400) && (dwWidth == 640)  && (dwBPP == 8))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x100;
	}

    /* 640x480   8, 15, 16 , 32*/
	if ((dwHeight == 480) && (dwWidth == 640)  && (dwBPP == 8))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x101;
	}

	if ((dwHeight == 480) && (dwWidth == 640)  && (dwBPP == 15))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x110;
	}

	if ((dwHeight == 480) && (dwWidth == 640)  && (dwBPP == 16))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x111;
	}

	if ((dwHeight == 480) && (dwWidth == 640)  && (dwBPP == 32))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x112;
	}

	/* 800x600  4, 8, 15, 16 , 32*/
	if ((dwHeight == 600) && (dwWidth == 800)  && (dwBPP == 4))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x102;
	}
	
	if ((dwHeight == 600) && (dwWidth == 800)  && (dwBPP == 8))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x103;
	}

	if ((dwHeight == 600) && (dwWidth == 800)  && (dwBPP == 15))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x113;
	}

	if ((dwHeight == 600) && (dwWidth == 800)  && (dwBPP == 16))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x114;
	}

	if ((dwHeight == 600) && (dwWidth == 800)  && (dwBPP == 32))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x115;
	}

    /* 1024x768 8, 15, 16 , 32*/

	if ((dwHeight == 768) && (dwWidth == 1024)  && (dwBPP == 4))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x104;
	}

	if ((dwHeight == 768) && (dwWidth == 1024)  && (dwBPP == 8))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x105;
	}

	if ((dwHeight == 768) && (dwWidth == 1024)  && (dwBPP == 15))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x116;
	}
	if ((dwHeight == 768) && (dwWidth == 1024)  && (dwBPP == 16))  
	{          
	    This->mDdSetMode.dwModeIndex  = 0x117;
	}

	if ((dwHeight == 768) && (dwWidth == 1024)  && (dwBPP == 32))  
	{          
	    This->mDdSetMode.dwModeIndex   = 0x118;
	}

	/* not coding for 1280x1024 */
	

	if (This->mDdSetMode.SetMode(&This->mDdSetMode)==DDHAL_DRIVER_HANDLED);
    {
		
		//if (This->mDdSetMode.ddRVal == DD_OK)
	    //{
	    //	// DdReenableDirectDrawObject(&This->mDDrawGlobal, &dummy);
	    //	/* FIXME fill the This->DirectDrawGlobal.vmiData right */
	    //}

		return This->mDdSetMode.ddRVal;
	}
	return  DDERR_NODRIVERSUPPORT;	 		    	
}










// This function is exported by the dll
HRESULT WINAPI DirectDrawCreateClipper (DWORD dwFlags, 
                                        LPDIRECTDRAWCLIPPER* lplpDDClipper, LPUNKNOWN pUnkOuter)
{
    DX_WINDBG_trace();

    return Main_DirectDraw_CreateClipper(NULL, dwFlags, lplpDDClipper, pUnkOuter);
}



HRESULT WINAPI Main_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface) 
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_FLIPTOGDISURFACE) 
    {
        return Hal_DirectDraw_FlipToGDISurface( iface);
    }

    return Hel_DirectDraw_FlipToGDISurface( iface);
}

HRESULT WINAPI Main_DirectDraw_GetCaps(LPDIRECTDRAW7 iface, LPDDCAPS pDriverCaps,
            LPDDCAPS pHELCaps) 
{	
    DX_WINDBG_trace();

	DDSCAPS2 ddscaps;
    DWORD status = DD_FALSE;	
    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;
	
    if (pDriverCaps != NULL) 
    {        
      
      
      
	  Main_DirectDraw_GetAvailableVidMem(iface, 
		                                 &ddscaps,
		                                 &This->mDDrawGlobal.ddCaps.dwVidMemTotal, 
		                                 &This->mDDrawGlobal.ddCaps.dwVidMemFree);	 
	  
	  RtlCopyMemory(pDriverCaps,&This->mDDrawGlobal.ddCaps,sizeof(DDCORECAPS));
	  pDriverCaps->dwSize=sizeof(DDCAPS);
	  
      status = DD_OK;
    }

    if (pHELCaps != NULL) 
    {      	  
	  Main_DirectDraw_GetAvailableVidMem(iface, 
		                                 &ddscaps,
		                                 &This->mDDrawGlobal.ddHELCaps.dwVidMemTotal, 
		                                 &This->mDDrawGlobal.ddHELCaps.dwVidMemFree);	  	 

	  RtlCopyMemory(pDriverCaps,&This->mDDrawGlobal.ddHELCaps,sizeof(DDCORECAPS));
      status = DD_OK;
    }
    
    return status;
}

HRESULT WINAPI Main_DirectDraw_GetDisplayMode(LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD) 
{   
    DX_WINDBG_trace();

    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    if (pDDSD == NULL)
    {
      return DD_FALSE;
    }
    
    
    pDDSD->dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_PITCH | DDSD_PIXELFORMAT | DDSD_REFRESHRATE | DDSD_WIDTH; 
    pDDSD->dwHeight  = This->mDDrawGlobal.vmiData.dwDisplayHeight;
    pDDSD->dwWidth = This->mDDrawGlobal.vmiData.dwDisplayWidth; 
    pDDSD->lPitch  = This->mDDrawGlobal.vmiData.lDisplayPitch;
    pDDSD->dwRefreshRate = This->mDDrawGlobal.dwMonitorFrequency;
    pDDSD->dwAlphaBitDepth = This->mDDrawGlobal.vmiData.ddpfDisplay.dwAlphaBitDepth;

    RtlCopyMemory(&pDDSD->ddpfPixelFormat,&This->mDDrawGlobal.vmiData.ddpfDisplay,sizeof(DDPIXELFORMAT));
    RtlCopyMemory(&pDDSD->ddsCaps,&This->mDDrawGlobal.ddCaps,sizeof(DDCORECAPS));
    
    RtlCopyMemory(&pDDSD->ddckCKDestOverlay,&This->mDDrawGlobal.ddckCKDestOverlay,sizeof(DDCOLORKEY));
    RtlCopyMemory(&pDDSD->ddckCKSrcOverlay,&This->mDDrawGlobal.ddckCKSrcOverlay,sizeof(DDCOLORKEY));

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

HRESULT WINAPI Main_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,
                                                   HANDLE h)
{
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	if (This->mDdWaitForVerticalBlank.WaitForVerticalBlank == NULL) 
    {
        return DDERR_NODRIVERSUPPORT;
    }
   
    This->mDdWaitForVerticalBlank.dwFlags = dwFlags;
    This->mDdWaitForVerticalBlank.hEvent = (DWORD)h;
    This->mDdWaitForVerticalBlank.ddRVal = DDERR_NOTPALETTIZED;
	
    if (This->mDdWaitForVerticalBlank.WaitForVerticalBlank(&This->mDdWaitForVerticalBlank) != DDHAL_DRIVER_HANDLED)
    {
       return DDERR_NODRIVERSUPPORT;
    }

    return This->mDdWaitForVerticalBlank.ddRVal;      
}

HRESULT WINAPI Main_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
                   LPDWORD total, LPDWORD free)                                               
{    
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	
    if (This->mDDrawGlobal.lpDDCBtmp->HALDDMiscellaneous.dwFlags & DDHAL_MISCCB32_GETAVAILDRIVERMEMORY) 
    {
        return Hal_DirectDraw_GetAvailableVidMem (iface,ddscaps,total,free);
    }

    return Hel_DirectDraw_GetAvailableVidMem (iface,ddscaps,total,free);
}

HRESULT WINAPI Main_DirectDraw_GetMonitorFrequency(LPDIRECTDRAW7 iface,LPDWORD freq)
{  
    DX_WINDBG_trace();

    IDirectDrawImpl *This = (IDirectDrawImpl *)iface;

    if (freq == NULL)
    {
        return DD_FALSE;
    }

    *freq = This->mDDrawGlobal.dwMonitorFrequency;
    return DD_OK;
}

HRESULT WINAPI Main_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine)
{    
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_GETSCANLINE) 
    {
        return Hal_DirectDraw_GetScanLine( iface,  lpdwScanLine);         
    }

    return Hel_DirectDraw_GetScanLine( iface,  lpdwScanLine);
}

HRESULT WINAPI Main_DirectDraw_RestoreDisplayMode(LPDIRECTDRAW7 iface)
{
   DX_WINDBG_trace();

   ChangeDisplaySettings(NULL, 0);
   return DD_OK;
}

/********************************** Stubs **********************************/



HRESULT WINAPI Main_DirectDraw_DuplicateSurface(LPDIRECTDRAW7 iface, LPDIRECTDRAWSURFACE7 src,
                 LPDIRECTDRAWSURFACE7* dst) 
{
    DX_WINDBG_trace();
    DX_STUB;    
}

HRESULT WINAPI Main_DirectDraw_EnumDisplayModes(LPDIRECTDRAW7 iface, DWORD dwFlags,
                 LPDDSURFACEDESC2 pDDSD, LPVOID context, LPDDENUMMODESCALLBACK2 callback) 
{
    DX_WINDBG_trace();
    DX_STUB;    
}

HRESULT WINAPI Main_DirectDraw_EnumSurfaces(LPDIRECTDRAW7 iface, DWORD dwFlags,
                 LPDDSURFACEDESC2 lpDDSD2, LPVOID context,
                 LPDDENUMSURFACESCALLBACK7 callback) 
{
    DX_WINDBG_trace();
    DX_STUB;
}


HRESULT WINAPI Main_DirectDraw_GetFourCCCodes(LPDIRECTDRAW7 iface, LPDWORD pNumCodes, LPDWORD pCodes)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_GetGDISurface(LPDIRECTDRAW7 iface, 
                                             LPDIRECTDRAWSURFACE7 *lplpGDIDDSSurface)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW7 iface, LPBOOL status)
{
    DX_WINDBG_trace();
    DX_STUB;
}


                                                   
HRESULT WINAPI Main_DirectDraw_GetSurfaceFromDC(LPDIRECTDRAW7 iface, HDC hdc,
                                                LPDIRECTDRAWSURFACE7 *lpDDS)
{  
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_RestoreAllSurfaces(LPDIRECTDRAW7 iface)
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_TestCooperativeLevel(LPDIRECTDRAW7 iface) 
{
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_GetDeviceIdentifier(LPDIRECTDRAW7 iface,
                   LPDDDEVICEIDENTIFIER2 pDDDI, DWORD dwFlags)
{    
    DX_WINDBG_trace();
    DX_STUB;
}

HRESULT WINAPI Main_DirectDraw_StartModeTest(LPDIRECTDRAW7 iface, LPSIZE pModes,
                  DWORD dwNumModes, DWORD dwFlags)
{
    DX_WINDBG_trace();
    DX_STUB;
}

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
