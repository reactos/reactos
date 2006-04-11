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

const DDPIXELFORMAT pixelformats[] =
{
    /* 8bpp paletted */
     { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, { 24 }, { 0xFF0000 },
      { 0x00FF00 }, { 0x0000FF } },
    /* 15bpp 5/5/5 */
    { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, { 16 }, { 0x7C00 }, { 0x3E0 },
      { 0x1F } },
    /* 16bpp 5/6/5 */
    { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, { 16 }, { 0xF800 }, { 0x7E0 },
      { 0x1F } },
    /* 24bpp 8/8/8 */
    { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, { 24 }, { 0xFF0000 },
      { 0x00FF00 }, { 0x0000FF } },
    /* 32bpp 8/8/8 */
    { sizeof(DDPIXELFORMAT), DDPF_RGB, 0, { 32 }, { 0xFF0000 },
      { 0x00FF00 }, { 0x0000FF } }
};

const DWORD pixelformatsCount = sizeof(pixelformats) / sizeof(DDPIXELFORMAT);

/* more surface format not adding it */
      /* 4 bit  paletted  0 */
      //   sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_PALETTEINDEXED4, 0, 4, 0x00, 0x00, 0x00, 0x00

      ///* 8bpp paletted  1 */
      //{sizeof(DDPIXELFORMAT), DDPF_RGB|DDPF_PALETTEINDEXED8, 0, 8, 0, 0, 0, 0},

      ///* 15bpp 5:5:5 RGB  2 */
      //{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 16, 0x7c00, 0x03e0, 0x001f, 0},

      ///* 15bpp 1:5:5:5 ARGB 3 */
      //{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0, 16, 0x7c00, 0x03e0, 0x001f, 0x8000},

      ///* 16bpp 5:6:5 RGB  4 */
      //{sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 16, 0xf800, 0x07e0, 0x001f, 0}                                 

      ///* 16bpp 4:4:4:4 ARGB 5 */
      //{sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS,´0, 16,       0x0f00,      0x00f0, 0x000f, 0xf000},

      /* 24bpp 8/8/8 RGB 6 */
      //  {sizeof(DDPIXELFORMAT), DDPF_RGB,                    0,  24 ,  0x00FF0000, 0x0000FF00 , 0x000000FF, 0 },

      /* 32bpp 8:8:8 RGB  7 */
      //   {sizeof(DDPIXELFORMAT), DDPF_RGB, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0},                     
 
      /* 32bpp 8:8:8:8 ARGB  8*/
       // {sizeof(DDPIXELFORMAT), DDPF_RGB | DDPF_ALPHAPIXELS, 0, 32, 0x00ff0000, 0x0000ff00, 0x000000ff, 0xff000000}
      



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

	RtlZeroMemory(&This->mDDrawGlobal, sizeof(DDRAWI_DIRECTDRAW_GBL));

	
	/* cObsolete is undoc in msdn it being use in CreateDCA */
	RtlCopyMemory(&This->mDDrawGlobal.cObsolete,&"DISPLAY",7);
	RtlCopyMemory(&This->mDDrawGlobal.cDriverName,&"DISPLAY",7);

		 
                       
    // call software first
    Hal_DirectDraw_Initialize (iface);
        
    Hel_DirectDraw_Initialize (iface); 
        This->mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;
        
    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_CANCREATESURFACE) 
    {
        This->mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HALDD.CanCreateSurface;  
    }
    else
    {
        This->mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HELDD.CanCreateSurface;
    }
        
    This->mDdCreateSurface.lpDD = &This->mDDrawGlobal;
        
    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_CREATESURFACE) 
    {
        This->mDdCreateSurface.CreateSurface = This->mCallbacks.HALDD.CreateSurface;  
    }
    else
    {
        This->mDdCreateSurface.CreateSurface = This->mCallbacks.HELDD.CreateSurface;
    }
                	
    This->mDdCreateSurface.lpDD = &This->mDDrawGlobal;
    This->mDdCreateSurface.CreateSurface = This->mCallbacks.HALDD.CreateSurface;    	
               
    return DD_OK;
}

/*
 * IMPLEMENT
 * Status this api is finish and is 100% correct 
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
        ref = InterlockedIncrement( (PLONG) &This->ref);       
	}    
    return ref;
}

/*
 * IMPLEMENT
 * Status 
 * not finish yet but is working fine 
 * it prevent memmory leaks at exit
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
		ref = InterlockedDecrement( (PLONG) &This->ref);
            
		if (ref == 0)
		{
			// set resoltion back to the one in registry
			if(This->cooperative_level & DDSCL_EXCLUSIVE)
			{
				ChangeDisplaySettings(NULL, 0);
			}
            
			Hal_DirectDraw_Release(iface);
			//Hel_DirectDraw_Release(iface);            			
            if (This!=NULL)
            {              
			    HeapFree(GetProcessHeap(), 0, This);
            }
		}
    }
    return ref;
}






HRESULT WINAPI Main_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface, HWND hwnd, DWORD cooplevel)
{
    // TODO:                                                            
    // - create a scaner that check which driver we should get the HDC from    
    //   for now we always asume it is the active dirver that should be use.
    // - allow more Flags

    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

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

    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_SETEXCLUSIVEMODE) 
    {
        return Hal_DirectDraw_SetCooperativeLevel (iface);        
    }

    return Hel_DirectDraw_SetCooperativeLevel(iface);

}

HRESULT WINAPI Main_DirectDraw_SetDisplayMode (LPDIRECTDRAW7 iface, DWORD dwWidth, DWORD dwHeight, 
                                                                DWORD dwBPP, DWORD dwRefreshRate, DWORD dwFlags)
{
    DX_WINDBG_trace();

    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
	BOOL dummy = TRUE;
	DWORD ret;
	
	/* FIXME check the refresrate if it same if it not same do the mode switch */
	if ((This->mDDrawGlobal.vmiData.dwDisplayHeight == dwHeight) && 
		(This->mDDrawGlobal.vmiData.dwDisplayWidth == dwWidth)  && 
		(This->mDDrawGlobal.vmiData.ddpfDisplay.dwRGBBitCount == dwBPP))  
		{
          
		  return DD_OK;
		}

	/* Check use the Hal or Hel for SetMode */
	if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_SETMODE)
	{
		ret = Hal_DirectDraw_SetDisplayMode(iface, dwWidth, dwHeight, dwBPP, dwRefreshRate, dwFlags);   
        
    }    
	else 
	{
		ret = Hel_DirectDraw_SetDisplayMode(iface, dwWidth, dwHeight, dwBPP, dwRefreshRate, dwFlags);
	}
	    
	if (ret == DD_OK)
	{
		DdReenableDirectDrawObject(&This->mDDrawGlobal, &dummy);
		/* FIXME fill the This->DirectDrawGlobal.vmiData right */
	}
  
	return ret;
}




/*
 * IMPLEMENT
 * Status this api is finish and is 100% correct 
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

HRESULT WINAPI Main_DirectDraw_CreateSurface (LPDIRECTDRAW7 iface, LPDDSURFACEDESC2 pDDSD,
                                            LPDIRECTDRAWSURFACE7 *ppSurf, IUnknown *pUnkOuter) 
{
    DX_WINDBG_trace();

    DxSurf *surf;

    if (pUnkOuter!=NULL) 
        return DDERR_INVALIDPARAMS; 

    if(sizeof(DDSURFACEDESC2)!=pDDSD->dwSize && sizeof(DDSURFACEDESC)!=pDDSD->dwSize)
        return DDERR_UNSUPPORTED;

    // the nasty com stuff
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

    IDirectDrawSurfaceImpl* That; 

    That = (IDirectDrawSurfaceImpl*)HeapAlloc(GetProcessHeap(), 0, sizeof(IDirectDrawSurfaceImpl));
    
    if (That == NULL) 
	{
        return E_OUTOFMEMORY;
	}
    ZeroMemory(That, sizeof(IDirectDrawSurfaceImpl));
    
    surf = (DxSurf*)HeapAlloc(GetProcessHeap(), 0, sizeof(DxSurf));

    if (surf == NULL) 
	{
        // FIXME Free memmory at exit
        return E_OUTOFMEMORY;
	}
    
 
    That->lpVtbl = &DirectDrawSurface7_Vtable;
    That->lpVtbl_v3 = &DDRAW_IDDS3_Thunk_VTable;
	*ppSurf = (LPDIRECTDRAWSURFACE7)That;

    // FIXME free This->mDDrawGlobal.dsList  on release 
    This->mDDrawGlobal.dsList = (LPDDRAWI_DDRAWSURFACE_INT)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 
                                                                            sizeof(DDRAWI_DDRAWSURFACE_INT));        
    That->owner = (IDirectDrawImpl *)This;
    That->owner->mDDrawGlobal.dsList->dwIntRefCnt =1;

    /* we alwasy set to use the DirectDrawSurface7_Vtable as internel */
    That->owner->mDDrawGlobal.dsList->lpVtbl = (PVOID) &DirectDrawSurface7_Vtable;
   
   
    That->Surf = surf;

    // UINT i;
    //IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    //IDirectDrawSurfaceImpl* That = ppSurf;        

    
	
  
          
    if (pDDSD->ddsCaps.dwCaps & DDSCAPS_PRIMARYSURFACE)
    {   
           
           memcpy(&That->Surf->mddsdPrimary,pDDSD,sizeof(DDSURFACEDESC));
           That->Surf->mddsdPrimary.dwSize      = sizeof(DDSURFACEDESC);          
           This->mDdCanCreateSurface.bIsDifferentPixelFormat = FALSE; 
           This->mDdCanCreateSurface.lpDDSurfaceDesc = &That->Surf->mddsdPrimary; 

           if (This->mDdCanCreateSurface.CanCreateSurface(&This->mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
           {         
              return DDERR_NOTINITIALIZED;
           }

           if (This->mDdCanCreateSurface.ddRVal != DD_OK)
           {
              return DDERR_NOTINITIALIZED;
           }

           memset(&That->Surf->mPrimaryGlobal, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
           That->Surf->mPrimaryGlobal.dwGlobalFlags = DDRAWISURFGBL_ISGDISURFACE;
           That->Surf->mPrimaryGlobal.lpDD       = &This->mDDrawGlobal;
           That->Surf->mPrimaryGlobal.lpDDHandle = &This->mDDrawGlobal;
           That->Surf->mPrimaryGlobal.wWidth  = (WORD)This->mpModeInfos[0].dwWidth;
           That->Surf->mPrimaryGlobal.wHeight = (WORD)This->mpModeInfos[0].dwHeight;
           That->Surf->mPrimaryGlobal.lPitch  = This->mpModeInfos[0].lPitch;

           memset(&That->Surf->mPrimaryMore,   0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
           That->Surf->mPrimaryMore.dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

           memset(&That->Surf->mPrimaryLocal,  0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
           That->Surf->mPrimaryLocal.lpGbl = &That->Surf->mPrimaryGlobal;
           That->Surf->mPrimaryLocal.lpSurfMore = &That->Surf->mPrimaryMore;
           That->Surf->mPrimaryLocal.dwProcessId = GetCurrentProcessId();
	   
           /*
              FIXME Check the flags if we shall create a primaresurface for overlay or something else 
              Examine windows which flags are being set for we assume this is right unsue I think
           */
           //That->Surf->mPrimaryLocal.dwFlags = DDRAWISURF_PARTOFPRIMARYCHAIN|DDRAWISURF_HASOVERLAYDATA;
           That->Surf->mPrimaryLocal.ddsCaps.dwCaps = That->Surf->mddsdPrimary.ddsCaps.dwCaps;
           That->Surf->mpPrimaryLocals[0] = &That->Surf->mPrimaryLocal;

          

           This->mDdCreateSurface.lpDDSurfaceDesc = &That->Surf->mddsdPrimary;
           This->mDdCreateSurface.lplpSList = That->Surf->mpPrimaryLocals;
           This->mDdCreateSurface.dwSCnt = This->mDDrawGlobal.dsList->dwIntRefCnt ; 

            
           if (This->mDdCreateSurface.CreateSurface(&This->mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
           {
              return DDERR_NOTINITIALIZED;
           }

           if (This->mDdCreateSurface.ddRVal != DD_OK) 
           {   
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
           RtlCopyMemory(&That->Surf->mddsdPrimary.ddsCaps,&This->mDDrawGlobal.ddCaps,sizeof(DDCORECAPS));
    
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

           return DD_OK;

        }
        else if (pDDSD->ddsCaps.dwCaps & DDSCAPS_OVERLAY)
        {
           // DX_STUB_str( "Can not create overlay surface");
           // memset(&That->Surf->mddsdOverlay, 0, sizeof(DDSURFACEDESC));
           // memcpy(&That->Surf->mddsdOverlay,pDDSD,sizeof(DDSURFACEDESC));
           // That->Surf->mddsdOverlay.dwSize = sizeof(DDSURFACEDESC);
           //That->Surf->mddsdOverlay.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_BACKBUFFERCOUNT | DDSD_WIDTH | DDSD_HEIGHT;

           //That->Surf->mddsdOverlay.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

           //That->Surf->mddsdOverlay.dwWidth = 100;  //pels;
           //That->Surf->mddsdOverlay.dwHeight = 100; // lines;
           //That->Surf->mddsdOverlay.dwBackBufferCount = 1; //cBuffers;

           //That->Surf->mddsdOverlay.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
           //That->Surf->mddsdOverlay.ddpfPixelFormat.dwFlags = DDPF_RGB; 
           //That->Surf->mddsdOverlay.ddpfPixelFormat.dwRGBBitCount = 32;
     
           //mDdCanCreateSurface.lpDD = &This->mDDrawGlobal;
           //mDdCanCreateSurface.CanCreateSurface = This->mCallbacks.HALDD.CanCreateSurface;
           //mDdCanCreateSurface.bIsDifferentPixelFormat = TRUE; //isDifferentPixelFormat;
           //mDdCanCreateSurface.lpDDSurfaceDesc = &That->Surf->mddsdOverlay; // pDDSD;
   
   
           //if (This->mHALInfo.lpDDCallbacks->CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
           //{    
           //   return DDERR_NOTINITIALIZED;
           //}

           //if (mDdCanCreateSurface.ddRVal != DD_OK)
           //{
           //   return DDERR_NOTINITIALIZED;
           //}

           //memset(&That->Surf->mOverlayGlobal, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
           //That->Surf->mOverlayGlobal.dwGlobalFlags = 0;
           //That->Surf->mOverlayGlobal.lpDD       = &This->mDDrawGlobal;
           //That->Surf->mOverlayGlobal.lpDDHandle = &This->mDDrawGlobal;
           //That->Surf->mOverlayGlobal.wWidth  = (WORD)That->Surf->mddsdOverlay.dwWidth;
           //That->Surf->mOverlayGlobal.wHeight = (WORD)That->Surf->mddsdOverlay.dwHeight;
           //That->Surf->mOverlayGlobal.lPitch  = -1;
           //That->Surf->mOverlayGlobal.ddpfSurface = That->Surf->mddsdOverlay.ddpfPixelFormat;

           //// setup front- and backbuffer surfaces
           //UINT cSurfaces = That->Surf->mddsdOverlay.dwBackBufferCount + 1;
           //for (i = 0; i < cSurfaces; i++)
           //{
           //   memset(&That->Surf->mOverlayMore[i], 0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
           //   That->Surf->mOverlayMore[i].dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

           //   memset(&That->Surf->mOverlayLocal[i],  0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
           //   That->Surf->mOverlayLocal[i].lpGbl = &That->Surf->mOverlayGlobal;
           //   That->Surf->mOverlayLocal[i].lpSurfMore = &That->Surf->mOverlayMore[i];
           //   That->Surf->mOverlayLocal[i].dwProcessId = GetCurrentProcessId();
           //   That->Surf->mOverlayLocal[i].dwFlags = (i == 0) ?
           //   (DDRAWISURF_IMPLICITROOT|DDRAWISURF_FRONTBUFFER):
           //   (DDRAWISURF_IMPLICITCREATE|DDRAWISURF_BACKBUFFER);

           //   That->Surf->mOverlayLocal[i].dwFlags |= DDRAWISURF_ATTACHED|DDRAWISURF_ATTACHED_FROM| DDRAWISURF_HASPIXELFORMAT| DDRAWISURF_HASOVERLAYDATA;

           //   That->Surf->mOverlayLocal[i].ddsCaps.dwCaps = That->Surf->mddsdOverlay.ddsCaps.dwCaps;
           //   That->Surf->mpOverlayLocals[i] = &That->Surf->mOverlayLocal[i];
           //}

           //for (i = 0; i < cSurfaces; i++)
           //{
           //   UINT j = (i + 1) % cSurfaces;	
           //   if (!DdAttachSurface(That->Surf->mpOverlayLocals[i], That->Surf->mpOverlayLocals[j])) 
           //   {
           //      // derr(L"DirectDrawImpl[%08x]::__setupDevice DdAttachSurface(%d, %d) failed", this, i, j);
           //     return DD_FALSE;
           //   }	
           //}
  
           //mDdCreateSurface.lpDD = &This->mDDrawGlobal;
           //mDdCreateSurface.CreateSurface = This->mCallbacks.HALDD.CreateSurface;  
           //mDdCreateSurface.lpDDSurfaceDesc = &That->Surf->mddsdOverlay;//pDDSD;
           //mDdCreateSurface.lplpSList = That->Surf->mpOverlayLocals; //cSurfaces;
           //mDdCreateSurface.dwSCnt = 1 ;  //ppSurfaces;

           //if (This->mHALInfo.lpDDCallbacks->CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
           //{
           //   return DDERR_NOTINITIALIZED;
           //}
  
           //if (mDdCreateSurface.ddRVal != DD_OK) 
           //{   
           //   return mDdCreateSurface.ddRVal;
           //}
  
           //DDHAL_UPDATEOVERLAYDATA      mDdUpdateOverlay;
           //mDdUpdateOverlay.lpDD = &This->mDDrawGlobal;
           //mDdUpdateOverlay.UpdateOverlay = This->mCallbacks.HALDDSurface.UpdateOverlay;
           //mDdUpdateOverlay.lpDDDestSurface = That->Surf->mpPrimaryLocals[0];
           //mDdUpdateOverlay.lpDDSrcSurface = That->Surf->mpOverlayLocals[0];//pDDSurface;
           //mDdUpdateOverlay.dwFlags = DDOVER_SHOW;
  
           //mDdUpdateOverlay.rDest.top = 0;
           //mDdUpdateOverlay.rDest.left = 0;
           //mDdUpdateOverlay.rDest.right = 50;
           //mDdUpdateOverlay.rDest.bottom = 50;

           //mDdUpdateOverlay.rSrc.top = 0;
           //mDdUpdateOverlay.rSrc.left = 0;
           //mDdUpdateOverlay.rSrc.right = 50;
           //mDdUpdateOverlay.rSrc.bottom = 50;
	 
           //if (mDdUpdateOverlay.UpdateOverlay(&mDdUpdateOverlay) == DDHAL_DRIVER_NOTHANDLED)
           //{
           //   return DDERR_NOTINITIALIZED;
           //}
  
           //if (mDdUpdateOverlay.ddRVal != DD_OK) 
           //{   
           //   return mDdUpdateOverlay.ddRVal;
           //}

           return DD_OK;
           return DDERR_INVALIDSURFACETYPE;
 
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
  
    return DDERR_INVALIDSURFACETYPE;  
   
}

HRESULT WINAPI Main_DirectDraw_CreateClipper(LPDIRECTDRAW7 iface, DWORD dwFlags, 
                                             LPDIRECTDRAWCLIPPER *ppClipper, IUnknown *pUnkOuter)
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

// This function is exported by the dll
HRESULT WINAPI DirectDrawCreateClipper (DWORD dwFlags, 
                                        LPDIRECTDRAWCLIPPER* lplpDDClipper, LPUNKNOWN pUnkOuter)
{
    DX_WINDBG_trace();

    return Main_DirectDraw_CreateClipper(NULL, dwFlags, lplpDDClipper, pUnkOuter);
}

HRESULT WINAPI Main_DirectDraw_CreatePalette(LPDIRECTDRAW7 iface, DWORD dwFlags,
                  LPPALETTEENTRY palent, LPDIRECTDRAWPALETTE* ppPalette, LPUNKNOWN pUnkOuter)
{
    DX_WINDBG_trace();

    if (pUnkOuter!=NULL) 
        return DDERR_INVALIDPARAMS; 

    IDirectDrawPaletteImpl* That; 
    That = (IDirectDrawPaletteImpl*)HeapAlloc(GetProcessHeap(), 0, sizeof(IDirectDrawPaletteImpl));

    if (That == NULL) 
        return E_OUTOFMEMORY;

    ZeroMemory(That, sizeof(IDirectDrawPaletteImpl));

    That->lpVtbl = &DirectDrawPalette_Vtable;
    That->ref = 1;
    *ppPalette = (LPDIRECTDRAWPALETTE)That;

       return That->lpVtbl->Initialize (*ppPalette, (LPDIRECTDRAW)iface, dwFlags, palent);
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

    if (This->mDDrawGlobal.lpDDCBtmp->HALDD.dwFlags & DDHAL_CB32_WAITFORVERTICALBLANK) 
    {
        return Hal_DirectDraw_WaitForVerticalBlank( iface,  dwFlags, h);        
    }

    return Hel_DirectDraw_WaitForVerticalBlank( iface,  dwFlags, h);        
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

HRESULT WINAPI Main_DirectDraw_Compact(LPDIRECTDRAW7 iface) 
{
    DX_WINDBG_trace();
    DX_STUB;
}

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
    
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;
    DDSURFACEDESC2 desc_callback;
    DEVMODE DevMode;   
    int iMode=0;

    if (pDDSD!=NULL)
    {
        // FIXME fill in pDDSD  
    }

    RtlZeroMemory(&desc_callback, sizeof(DDSURFACEDESC2));
    desc_callback.dwSize = sizeof(DDSURFACEDESC2);

    desc_callback.dwFlags = DDSD_HEIGHT|DDSD_WIDTH|DDSD_PIXELFORMAT|DDSD_CAPS | DDSD_PITCH;

    if (dwFlags & DDEDM_REFRESHRATES)
    {
	    desc_callback.dwFlags |= DDSD_REFRESHRATE;
        desc_callback.dwRefreshRate = This->mDDrawGlobal.dwMonitorFrequency;
    }

  
    /// FIXME check if the mode are suppretd before sending it back 

    while (EnumDisplaySettingsEx(NULL, iMode, &DevMode, 0))
    {
       
	   if (pDDSD)
	   {
	       if ((pDDSD->dwFlags & DDSD_WIDTH) && (pDDSD->dwWidth != DevMode.dmPelsWidth))
	       continue; 
	       if ((pDDSD->dwFlags & DDSD_HEIGHT) && (pDDSD->dwHeight != DevMode.dmPelsHeight))
		   continue; 
	       if ((pDDSD->dwFlags & DDSD_PIXELFORMAT) && (pDDSD->ddpfPixelFormat.dwFlags & DDPF_RGB) &&
		   (pDDSD->ddpfPixelFormat.dwRGBBitCount != DevMode.dmBitsPerPel))
		    continue; 
       } 

	
       desc_callback.dwHeight = DevMode.dmPelsHeight;
	   desc_callback.dwWidth = DevMode.dmPelsWidth;

       if (DevMode.dmFields & DM_DISPLAYFREQUENCY)
       {
            desc_callback.dwRefreshRate = DevMode.dmDisplayFrequency;
       }
         
      switch(DevMode.dmBitsPerPel)
      {
        case  8:
            memcpy(&desc_callback.ddpfPixelFormat,&pixelformats[0],sizeof(DDPIXELFORMAT));
            break;

        case 15:
            memcpy(&desc_callback.ddpfPixelFormat,&pixelformats[1],sizeof(DDPIXELFORMAT));
            break;

        case 16: 
            memcpy(&desc_callback.ddpfPixelFormat,&pixelformats[2],sizeof(DDPIXELFORMAT));
            break;


       case 24: 
            memcpy(&desc_callback.ddpfPixelFormat,&pixelformats[3],sizeof(DDPIXELFORMAT));
            break;

       case 32: 
            memcpy(&desc_callback.ddpfPixelFormat,&pixelformats[4],sizeof(DDPIXELFORMAT));
            break;

        default:
            break;          
      }
                        
       if (desc_callback.ddpfPixelFormat.dwRGBBitCount==15)
       {           
            desc_callback.lPitch =  DevMode.dmPelsWidth + (8 - ( DevMode.dmPelsWidth % 8)) % 8;
       }
       else
       {
           desc_callback.lPitch = DevMode.dmPelsWidth * (desc_callback.ddpfPixelFormat.dwRGBBitCount / 8);
           desc_callback.lPitch =  desc_callback.lPitch + (8 - (desc_callback.lPitch % 8)) % 8;
       }
           
       desc_callback.ddsCaps.dwCaps = 0;
       if (desc_callback.ddpfPixelFormat.dwFlags & DDPF_PALETTEINDEXED8) 
       {
           desc_callback.ddsCaps.dwCaps |= DDSCAPS_PALETTE;
       }
      

       if (callback(&desc_callback, context) == DDENUMRET_CANCEL)
       {

           return DD_OK;       
       }
       
      iMode++; 
    }

    return DD_OK;
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
