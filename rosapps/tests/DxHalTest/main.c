#include <stdio.h>
#include "main.h"


// TODO:
//  
// - free memory
// - Check if we ran out of memory (HeapAlloc == NULL)
  HDC hdc;
  DDRAWI_DIRECTDRAW_GBL mDDrawGlobal;
  DDRAWI_DIRECTDRAW_LCL mDDrawLocal;
  DDHALINFO mHALInfo;

  DDHAL_CALLBACKS mCallbacks;
  DDHAL_DDEXEBUFCALLBACKS mD3dBufferCallbacks;
  D3DHAL_CALLBACKS mD3dCallbacks;
  D3DHAL_GLOBALDRIVERDATA mD3dDriverData;

  UINT mcModeInfos;
  DDHALMODEINFO *mpModeInfos;

  UINT mcvmList;
  VIDMEM *mpvmList;

  UINT mcFourCC;
  DWORD *mpFourCC;

  UINT mcTextures;
  DDSURFACEDESC *mpTextures;

  /* Surface */
  DDRAWI_DDRAWSURFACE_GBL mPrimaryGlobal;
  DDRAWI_DDRAWSURFACE_MORE mPrimaryMore;
  DDRAWI_DDRAWSURFACE_LCL mPrimaryLocal;
  DDRAWI_DDRAWSURFACE_LCL *mpPrimaryLocals[1];
  DDRAWI_DDRAWCLIPPER_LCL mPrimaryClipperLocal;
  DDRAWI_DDRAWCLIPPER_GBL mPrimaryClipperGlobal;
  //DDRAWI_DDRAWCLIPPER_INT mPrimaryClipperInterface;
  DDSURFACEDESC mddsdPrimary;
  DDSURFACEDESC mddsdOverlay;

  DDRAWI_DDRAWSURFACE_GBL mOverlayGlobal;
  DDRAWI_DDRAWSURFACE_LCL mOverlayLocal[6];
  DDRAWI_DDRAWSURFACE_LCL *mpOverlayLocals[6];
  DDRAWI_DDRAWSURFACE_MORE mOverlayMore[6];

  DDHAL_BLTDATA                mDdBlt;


typedef struct _DD_GETDRIVERINFODATA {
  VOID  *dhpdev;
  DWORD  dwSize;
  DWORD  dwFlags;
  GUID  guidInfo;
  DWORD  dwExpectedSize;
  PVOID  lpvData;
  DWORD  dwActualSize;
  HRESULT  ddRVal;
  ULONG_PTR  dwContext;
} DD_GETDRIVERINFODATA, *PDD_GETDRIVERINFODATA;


typedef struct _DD_MISCELLANEOUSCALLBACKS {
  DWORD  dwSize;
  DWORD  dwFlags;
  PVOID  GetAvailDriverMemory;
} DD_MISCELLANEOUSCALLBACKS;


int WINAPI WinMain (HINSTANCE hInst, HINSTANCE hPrevInst, 
					LPSTR lpCmdLine, int nCmdShow)
{
	/* get the functions we need */
//	DD_GETDRIVERINFODATA drv;
	HMODULE lib = LoadLibrary("gdi32.dll");
	DdCreateDirectDrawObject = (BOOL (APIENTRY*)(LPDDRAWI_DIRECTDRAW_GBL,   HDC))GetProcAddress(lib, "GdiEntry1");
	DdQueryDirectDrawObject  = (BOOL (APIENTRY*)(LPDDRAWI_DIRECTDRAW_GBL,   LPDDHALINFO,LPDDHAL_DDCALLBACKS,LPDDHAL_DDSURFACECALLBACKS,LPDDHAL_DDPALETTECALLBACKS,LPD3DHAL_CALLBACKS,LPD3DHAL_GLOBALDRIVERDATA,LPDDHAL_DDEXEBUFCALLBACKS,LPDDSURFACEDESC,LPDWORD,LPVIDMEM))GetProcAddress(lib, "GdiEntry2");
    DdAttachSurface          = (BOOL (APIENTRY*)(LPDDRAWI_DDRAWSURFACE_LCL, LPDDRAWI_DDRAWSURFACE_LCL))GetProcAddress(lib, "GdiEntry11");
    DdResetVisrgn            = (BOOL (APIENTRY*)(LPDDRAWI_DDRAWSURFACE_LCL, HWND))GetProcAddress(lib, "GdiEntry6");

    /* HAL Startup process */
    DEVMODE devmode;
    HBITMAP hbmp;
    const UINT bmiSize = sizeof(BITMAPINFOHEADER) + 0x10;
    UCHAR *pbmiData;
    BITMAPINFO *pbmi;
    
    DWORD *pMasks;
    //BOOL newmode = FALSE;
	//DWORD Status; /* for create surface */
	UINT i;
	UINT j;
    	

	printf("This apps showing how to start up directx draw/d3d interface and some other as well\n");
	printf("This code have been releae to some close applactons with my premtions, if any company\n");
    printf("want use part or whole code, you need contact the orginal author to ask for premtions\n");
    printf("This code are release under alot of diffent licen\n");
    printf("All GPL and LGPL project have right use and studing this code.\n");
    printf("This code maybe need more comment to known how stuff working and maybe looking bit mesy\n");
	printf("Bestreagds Magnus Olsen magnus@greatlord.com or greatlord@reactos.org\n");
    printf("Copyright 2006 by Magnus Olsen\n\n");
    printf("This demo showing how to  start dx draw hal and create a primary surface,\n");
	printf("and a overlay sufrace and blt to the primary surface\n");


    /* 
       Get and Create mode info 
    */
    mcModeInfos = 1;
    mpModeInfos = (DDHALMODEINFO*)HeapAlloc(GetProcessHeap(), 
                                                  HEAP_ZERO_MEMORY, 
                                                  mcModeInfos * sizeof(DDHALMODEINFO));  
   
    if (mpModeInfos == NULL)
    {
       printf("Fail to alloc mpModeInfos\n"); 
       return DD_FALSE;
    }


    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &devmode);

    mpModeInfos[0].dwWidth      = devmode.dmPelsWidth;
    mpModeInfos[0].dwHeight     = devmode.dmPelsHeight;
    mpModeInfos[0].dwBPP        = devmode.dmBitsPerPel;
    mpModeInfos[0].lPitch       = (devmode.dmPelsWidth*devmode.dmBitsPerPel)/8;
    mpModeInfos[0].wRefreshRate = (WORD)devmode.dmDisplayFrequency;

    /*
       Setup HDC and mDDrawGlobal right 
    */    
    hdc = CreateDCW(L"DISPLAY",L"DISPLAY",NULL,NULL);    

    if (hdc == NULL)
    {
	  printf("Fail to create HDC\n"); 
      return DD_FALSE;
    }

    /*
      Dectect RGB bit mask 
    */  
    hbmp = CreateCompatibleBitmap(hdc, 1, 1);  
    if (hbmp==NULL)
    {
       HeapFree(GetProcessHeap(), 0, mpModeInfos);
       DeleteDC(hdc);
	   printf("Fail to Create Compatible Bitmap\n"); 
       return DD_FALSE;
    }
  
    pbmiData = (UCHAR *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bmiSize);
    pbmi = (BITMAPINFO*)pbmiData;

    if (pbmiData==NULL)
    {
       HeapFree(GetProcessHeap(), 0, mpModeInfos);
       free(mpModeInfos);
       DeleteDC(hdc);
       DeleteObject(hbmp);
	   printf("Fail to Alloc  pbmiData\n"); 
       return DDERR_UNSUPPORTED;
    }

    pbmi->bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    pbmi->bmiHeader.biBitCount = (WORD)devmode.dmBitsPerPel;
    pbmi->bmiHeader.biCompression = BI_BITFIELDS;
    pbmi->bmiHeader.biWidth = 1;
    pbmi->bmiHeader.biHeight = 1;

    GetDIBits(hdc, hbmp, 0, 0, NULL, pbmi, 0);
    DeleteObject(hbmp);

    pMasks = (DWORD*)(pbmiData + sizeof(BITMAPINFOHEADER));
    mpModeInfos[0].dwRBitMask = pMasks[0];
    mpModeInfos[0].dwGBitMask = pMasks[1];
    mpModeInfos[0].dwBBitMask = pMasks[2];
    mpModeInfos[0].dwAlphaBitMask = pMasks[3];

    HeapFree(GetProcessHeap(), 0, pbmiData);

    /* 
      prepare start up the DX Draw HAL interface now 
    */
   
    memset(&mDDrawGlobal, 0, sizeof(DDRAWI_DIRECTDRAW_GBL));
    memset(&mHALInfo,     0, sizeof(DDHALINFO));
    memset(&mCallbacks,   0, sizeof(DDHAL_CALLBACKS));
	
    /* 
      Startup DX HAL step one of three 
    */
    if (!DdCreateDirectDrawObject(&mDDrawGlobal, hdc))
    {
       HeapFree(GetProcessHeap(), 0, mpModeInfos);       
       DeleteDC(hdc);
       DeleteObject(hbmp);
	   printf("Fail to Create Direct DrawObject\n"); 
       return DD_FALSE;
    }

	mDDrawGlobal.dwRefCnt = 1; //addref / remove ref

    // Do not relase HDC it have been map in kernel mode 
    // DeleteDC(hdc);
  
    /* we need reanable it if screen res have changes, and some bad drv need be reanble very few 
	   to contiune */
	/*
    if (!DdReenableDirectDrawObject(&mDDrawGlobal, &newmode))
    {
      HeapFree(GetProcessHeap(), 0, mpModeInfos);
      DeleteDC(hdc);
      DeleteObject(hbmp);
      return DD_FALSE;
    }*/
  
    /*
      Setup the DirectDraw Local 
    */
  
    mDDrawLocal.lpDDCB = &mCallbacks;
    mDDrawLocal.lpGbl = &mDDrawGlobal;
    mDDrawLocal.dwProcessId = GetCurrentProcessId();

    mDDrawGlobal.lpDDCBtmp = &mCallbacks;
    mDDrawGlobal.lpExclusiveOwner = &mDDrawLocal;
	//mDDrawLocal.dwLocalFlags = DDRAWILCL_DIRECTDRAW7;

	
    /*
       Startup DX HAL step two of three 
    */

    if (!DdQueryDirectDrawObject(&mDDrawGlobal,
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
      HeapFree(GetProcessHeap(), 0, mpModeInfos);
      DeleteDC(hdc);
      DeleteObject(hbmp);
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    mcvmList = mHALInfo.vmiData.dwNumHeaps;
    mpvmList = (VIDMEM*) HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(VIDMEM) * mcvmList);
    if (mpvmList == NULL)
    {      
      HeapFree(GetProcessHeap(), 0, mpModeInfos);
      DeleteDC(hdc);
      DeleteObject(hbmp);
      // FIXME Close DX fristcall and second call
	  printf("Fail to QueryDirect Draw Object frist pass\n"); 
      return DD_FALSE;
    }

    mcFourCC = mHALInfo.ddCaps.dwNumFourCCCodes;
    mpFourCC = (DWORD *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DWORD) * mcFourCC);
    if (mpFourCC == NULL)
    {
      HeapFree(GetProcessHeap(), 0, mpvmList);
      HeapFree(GetProcessHeap(), 0, mpModeInfos);
      DeleteDC(hdc);
      DeleteObject(hbmp);
      // FIXME Close DX fristcall and second call
      return DD_FALSE;
    }

    mcTextures = mD3dDriverData.dwNumTextureFormats;
    mpTextures = (DDSURFACEDESC*)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DDSURFACEDESC) * mcTextures);
    if (mpTextures == NULL)
    {      
      HeapFree(GetProcessHeap(), 0, mpFourCC);
      HeapFree(GetProcessHeap(), 0, mpvmList);
      HeapFree(GetProcessHeap(), 0, mpModeInfos);
      DeleteDC(hdc);
      DeleteObject(hbmp);
      // FIXME Close DX fristcall and second call
	  printf("Fail QueryDirect Draw Object to Alloc mpTextures \n"); 
      return DD_FALSE;
    }

    mHALInfo.vmiData.pvmList = mpvmList;
    mHALInfo.lpdwFourCC = mpFourCC;
    mD3dDriverData.lpTextureFormats = mpTextures;

    if (!DdQueryDirectDrawObject(
                                    &mDDrawGlobal,
                                    &mHALInfo,
                                    &mCallbacks.HALDD,
                                    &mCallbacks.HALDDSurface,
                                    &mCallbacks.HALDDPalette, 
                                    &mD3dCallbacks,
                                    &mD3dDriverData,
                                    &mCallbacks.HALDDExeBuf,
                                    mpTextures,
                                    mpFourCC,
                                    mpvmList))
  
    {
      HeapFree(GetProcessHeap(), 0, mpTextures);
      HeapFree(GetProcessHeap(), 0, mpFourCC);
      HeapFree(GetProcessHeap(), 0, mpvmList);
      HeapFree(GetProcessHeap(), 0, mpModeInfos);
      DeleteDC(hdc);
      DeleteObject(hbmp);
	  printf("Fail to QueryDirect Draw Object second pass\n"); 
      return DD_FALSE;
    }

   /*
      Copy over from HalInfo to DirectDrawGlobal
   */

  // this is wrong, cDriverName need be in ASC code not UNICODE 
  //memcpy(mDDrawGlobal.cDriverName, mDisplayAdapter, sizeof(wchar)*MAX_DRIVER_NAME);

  memcpy(&mDDrawGlobal.vmiData, &mHALInfo.vmiData,sizeof(VIDMEMINFO));
  memcpy(&mDDrawGlobal.ddCaps,  &mHALInfo.ddCaps,sizeof(DDCORECAPS));

  mHALInfo.dwNumModes = mcModeInfos;
  mHALInfo.lpModeInfo = mpModeInfos;
  mHALInfo.dwMonitorFrequency = mpModeInfos[0].wRefreshRate;

  mDDrawGlobal.dwMonitorFrequency = mHALInfo.dwMonitorFrequency;
  mDDrawGlobal.dwModeIndex        = mHALInfo.dwModeIndex;
  mDDrawGlobal.dwNumModes         = mHALInfo.dwNumModes;
  mDDrawGlobal.lpModeInfo         = mHALInfo.lpModeInfo;
  mDDrawGlobal.hInstance          = mHALInfo.hInstance;    
  
  mDDrawGlobal.lp16DD = &mDDrawGlobal;

  

  /* Hal insate is down now */

  /* cleare surface code now*/

  //  memset(&mGlobal, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
  //  memset(&mMore,   0, sizeof(DDRAWI_DDRAWSURFACE_MORE));

   /* mLocal.lpSurfMore = &mMore;
   memset(mMore, 0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
   mMore.dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);*/

   mPrimaryLocal.lpSurfMore = &mPrimaryMore;

  /* cleare surface ends now */

  /* create primare surface now */
  
   memset(&mddsdPrimary,   0, sizeof(DDSURFACEDESC));
   mddsdPrimary.dwSize      = sizeof(DDSURFACEDESC);
   mddsdPrimary.dwFlags     = DDSD_CAPS;
   mddsdPrimary.ddsCaps.dwCaps = DDSCAPS_PRIMARYSURFACE | DDSCAPS_VIDEOMEMORY | DDSCAPS_VISIBLE;

   DDHAL_CANCREATESURFACEDATA   mDdCanCreateSurface;
   mDdCanCreateSurface.lpDD = &mDDrawGlobal;
   mDdCanCreateSurface.CanCreateSurface = mCallbacks.HALDD.CanCreateSurface;
   mDdCanCreateSurface.bIsDifferentPixelFormat = FALSE; //isDifferentPixelFormat;
   mDdCanCreateSurface.lpDDSurfaceDesc = &mddsdPrimary; // pDDSD;
      
   if (mHALInfo.lpDDCallbacks->CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
   {
     printf("Fail to mDdCanCreateSurface DDHAL_DRIVER_NOTHANDLED\n"); 
     return DDERR_NOTINITIALIZED;
   }

   if (mDdCanCreateSurface.ddRVal != DD_OK)
   {
	   printf("Fail to mDdCanCreateSurface mDdCanCreateSurface.ddRVal = %d:%s\n",(int)mDdCanCreateSurface.ddRVal,DDErrorString(mDdCanCreateSurface.ddRVal)); 
       return DDERR_NOTINITIALIZED;
   }

  memset(&mPrimaryGlobal, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
  mPrimaryGlobal.dwGlobalFlags = DDRAWISURFGBL_ISGDISURFACE;
  mPrimaryGlobal.lpDD       = &mDDrawGlobal;
  mPrimaryGlobal.lpDDHandle = &mDDrawGlobal;
  mPrimaryGlobal.wWidth  = (WORD)mpModeInfos[0].dwWidth;
  mPrimaryGlobal.wHeight = (WORD)mpModeInfos[0].dwHeight;
  mPrimaryGlobal.lPitch  = mpModeInfos[0].lPitch;

  memset(&mPrimaryMore,   0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
  mPrimaryMore.dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

   
  //mPrimaryMore. = mpr

  memset(&mPrimaryLocal,  0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
  mPrimaryLocal.lpGbl = &mPrimaryGlobal;
  mPrimaryLocal.lpSurfMore = &mPrimaryMore;
  mPrimaryLocal.dwProcessId = GetCurrentProcessId();
  mPrimaryLocal.dwFlags = DDRAWISURF_PARTOFPRIMARYCHAIN|DDRAWISURF_HASOVERLAYDATA;
  mPrimaryLocal.ddsCaps.dwCaps = mddsdPrimary.ddsCaps.dwCaps;

  mpPrimaryLocals[0] = &mPrimaryLocal;

  DDHAL_CREATESURFACEDATA      mDdCreateSurface;
  mDdCreateSurface.lpDD = &mDDrawGlobal;
  mDdCreateSurface.CreateSurface = mCallbacks.HALDD.CreateSurface;  
  mDdCreateSurface.lpDDSurfaceDesc = &mddsdPrimary;//pDDSD;
  mDdCreateSurface.lplpSList = mpPrimaryLocals; //cSurfaces;
  mDdCreateSurface.dwSCnt = 1 ;  //ppSurfaces;

  if (mHALInfo.lpDDCallbacks->CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
  {
    printf("Fail to mDdCreateSurface  DDHAL_DRIVER_NOTHANDLED \n");
	return DDERR_NOTINITIALIZED;
  }
  

  if (mDdCreateSurface.ddRVal != DD_OK) 
  {   
    printf("Fail to mDdCanCreateSurface mDdCreateSurface.ddRVal = %d:%s\n",(int)mDdCreateSurface.ddRVal,DDErrorString(mDdCreateSurface.ddRVal)); 
    return mDdCreateSurface.ddRVal;
  }

  // -- Setup Clipper ---------------------------------------------------------
  memset(&mPrimaryClipperGlobal, 0, sizeof(DDRAWI_DDRAWCLIPPER_GBL));
  mPrimaryClipperGlobal.dwFlags = DDRAWICLIP_ISINITIALIZED;
  mPrimaryClipperGlobal.dwProcessId = GetCurrentProcessId();
  //mPrimaryClipperGlobal.hWnd = (ULONG_PTR)hwnd; 
  mPrimaryClipperGlobal.hWnd = (ULONG_PTR)GetDesktopWindow();
  mPrimaryClipperGlobal.lpDD = &mDDrawGlobal;
  mPrimaryClipperGlobal.lpStaticClipList = NULL;

  memset(&mPrimaryClipperLocal, 0, sizeof(DDRAWI_DDRAWCLIPPER_LCL));
  mPrimaryClipperLocal.lpGbl = &mPrimaryClipperGlobal;

  //memset(&mPrimaryClipperInterface, 0, sizeof(DDRAWI_DDRAWCLIPPER_INT));
  //mPrimaryClipperInterface.lpLcl = &mPrimaryClipperLocal;
  //mPrimaryClipperInterface.dwIntRefCnt = 1;
  //mPrimaryClipperInterface.lpLink = null;
  //mPrimaryClipperInterface.lpVtbl = null;

  mPrimaryLocal.lpDDClipper = &mPrimaryClipperLocal;
  //mPrimaryMore.lpDDIClipper = &mPrimaryClipperInterface;

  mDdBlt.lpDDDestSurface = mpPrimaryLocals[0];
  
  
  /* create primare surface is down now */

  /* create overlay surface now */
  
  memset(&mddsdOverlay, 0, sizeof(DDSURFACEDESC));
  mddsdOverlay.dwSize = sizeof(DDSURFACEDESC);
  mddsdOverlay.dwFlags = DDSD_CAPS | DDSD_PIXELFORMAT | DDSD_BACKBUFFERCOUNT | DDSD_WIDTH | DDSD_HEIGHT;

  mddsdOverlay.ddsCaps.dwCaps = DDSCAPS_OVERLAY | DDSCAPS_VIDEOMEMORY | DDSCAPS_LOCALVIDMEM | DDSCAPS_COMPLEX | DDSCAPS_FLIP;

  mddsdOverlay.dwWidth = 100;  //pels;
  mddsdOverlay.dwHeight = 100; // lines;
  mddsdOverlay.dwBackBufferCount = 1; //cBuffers;

  mddsdOverlay.ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
  mddsdOverlay.ddpfPixelFormat.dwFlags = DDPF_RGB; 
  mddsdOverlay.ddpfPixelFormat.dwRGBBitCount = 32;
  

   //DDHAL_CANCREATESURFACEDATA   mDdCanCreateSurface;
   mDdCanCreateSurface.lpDD = &mDDrawGlobal;
   mDdCanCreateSurface.CanCreateSurface = mCallbacks.HALDD.CanCreateSurface;
   mDdCanCreateSurface.bIsDifferentPixelFormat = TRUE; //isDifferentPixelFormat;
   mDdCanCreateSurface.lpDDSurfaceDesc = &mddsdOverlay; // pDDSD;
   
   
   if (mHALInfo.lpDDCallbacks->CanCreateSurface(&mDdCanCreateSurface)== DDHAL_DRIVER_NOTHANDLED) 
   {
    // derr(L"DirectDrawImpl[%08x]::__createPrimary Cannot create primary [%08x]", this, rv);
	   
	 printf("Fail to mDdCanCreateSurface DDHAL_DRIVER_NOTHANDLED \n"); 
    return DDERR_NOTINITIALIZED;
   }

   if (mDdCanCreateSurface.ddRVal != DD_OK)
   {
	 printf("Fail to mDdCanCreateSurface mDdCanCreateSurface.ddRVal = %d:%s\n",(int)mDdCanCreateSurface.ddRVal,DDErrorString(mDdCanCreateSurface.ddRVal)); 
     return DDERR_NOTINITIALIZED;
   }

 
  memset(&mOverlayGlobal, 0, sizeof(DDRAWI_DDRAWSURFACE_GBL));
  mOverlayGlobal.dwGlobalFlags = 0;
  mOverlayGlobal.lpDD       = &mDDrawGlobal;
  mOverlayGlobal.lpDDHandle = &mDDrawGlobal;
  mOverlayGlobal.wWidth  = (WORD)mddsdOverlay.dwWidth;
  mOverlayGlobal.wHeight = (WORD)mddsdOverlay.dwHeight;
  mOverlayGlobal.lPitch  = -1;
  mOverlayGlobal.ddpfSurface = mddsdOverlay.ddpfPixelFormat;

  // setup front- and backbuffer surfaces
  UINT cSurfaces = mddsdOverlay.dwBackBufferCount + 1;
  for ( i = 0; i < cSurfaces; i++)
  {
     memset(&mOverlayMore[i], 0, sizeof(DDRAWI_DDRAWSURFACE_MORE));
     mOverlayMore[i].dwSize = sizeof(DDRAWI_DDRAWSURFACE_MORE);

     memset(&mOverlayLocal[i],  0, sizeof(DDRAWI_DDRAWSURFACE_LCL));
    mOverlayLocal[i].lpGbl = &mOverlayGlobal;
    mOverlayLocal[i].lpSurfMore = &mOverlayMore[i];
    mOverlayLocal[i].dwProcessId = GetCurrentProcessId();
    mOverlayLocal[i].dwFlags = (i == 0) ?
      (DDRAWISURF_IMPLICITROOT|DDRAWISURF_FRONTBUFFER):
      (DDRAWISURF_IMPLICITCREATE|DDRAWISURF_BACKBUFFER);

    mOverlayLocal[i].dwFlags |= 
      DDRAWISURF_ATTACHED|DDRAWISURF_ATTACHED_FROM|
      DDRAWISURF_HASPIXELFORMAT|
      DDRAWISURF_HASOVERLAYDATA;

    mOverlayLocal[i].ddsCaps.dwCaps = mddsdOverlay.ddsCaps.dwCaps;
    mpOverlayLocals[i] = &mOverlayLocal[i];
  }

  for ( i = 0; i < cSurfaces; i++)
  {
    j = (i + 1) % cSurfaces;

	
    /*if (!mHALInfo.lpDDSurfaceCallbacks->AddAttachedSurface(mpOverlayLocals[i], mpOverlayLocals[j])) 
	{
     // derr(L"DirectDrawImpl[%08x]::__setupDevice DdAttachSurface(%d, %d) failed", this, i, j);
      return DD_FALSE;
    }*/

	if (!DdAttachSurface(mpOverlayLocals[i], mpOverlayLocals[j])) 
	{
     // derr(L"DirectDrawImpl[%08x]::__setupDevice DdAttachSurface(%d, %d) failed", this, i, j);
		printf("Fail to DdAttachSurface (%d:%d)\n", i, j);
      return DD_FALSE;
    }
	
  }


  // DDHAL_CREATESURFACEDATA      mDdCreateSurface;
  mDdCreateSurface.lpDD = &mDDrawGlobal;
  mDdCreateSurface.CreateSurface = mCallbacks.HALDD.CreateSurface;  
  mDdCreateSurface.lpDDSurfaceDesc = &mddsdOverlay;//pDDSD;
  mDdCreateSurface.lplpSList = mpOverlayLocals; //cSurfaces;
  mDdCreateSurface.dwSCnt = 1 ;  //ppSurfaces;

 if (mHALInfo.lpDDCallbacks->CreateSurface(&mDdCreateSurface) == DDHAL_DRIVER_NOTHANDLED)
  {
	printf("Fail to mDdCreateSurface = DDHAL_DRIVER_HANDLED\n");
	return DDERR_NOTINITIALIZED;
  }
  

  if (mDdCreateSurface.ddRVal != DD_OK) 
  {   
	 printf("Fail to mDdCreateSurface mDdCreateSurface.ddRVal = %d:%s\n",(int)mDdCreateSurface.ddRVal,DDErrorString(mDdCreateSurface.ddRVal)); 
     return mDdCreateSurface.ddRVal;
  }


  DDHAL_UPDATEOVERLAYDATA      mDdUpdateOverlay;
  mDdUpdateOverlay.lpDD = &mDDrawGlobal;
  mDdUpdateOverlay.UpdateOverlay = mCallbacks.HALDDSurface.UpdateOverlay;
  mDdUpdateOverlay.lpDDDestSurface = mpPrimaryLocals[0];
  mDdUpdateOverlay.lpDDSrcSurface = mpOverlayLocals[0];//pDDSurface;
  mDdUpdateOverlay.dwFlags = DDOVER_SHOW;

 /* if (flags & DDOVER_DDFX)
    mDdUpdateOverlay.overlayFX = *pFx;
  copyRect(&mDdUpdateOverlay.rDest, pdst);
  copyRect(&mDdUpdateOverlay.rSrc, psrc);
*/
  
  mDdUpdateOverlay.rDest.top = 0;
  mDdUpdateOverlay.rDest.left = 0;
  mDdUpdateOverlay.rDest.right = 50;
  mDdUpdateOverlay.rDest.bottom = 50;

   mDdUpdateOverlay.rSrc.top = 0;
  mDdUpdateOverlay.rSrc.left = 0;
  mDdUpdateOverlay.rSrc.right = 50;
  mDdUpdateOverlay.rSrc.bottom = 50;

 
 

  if ( mDdUpdateOverlay.UpdateOverlay(&mDdUpdateOverlay) == DDHAL_DRIVER_NOTHANDLED)
  {
	 printf("Fail to mDdBlt = DDHAL_DRIVER_HANDLED\n");
	 return DDERR_NOTINITIALIZED;
  }
  

  if (mDdUpdateOverlay.ddRVal != DD_OK) 
  {   
	  printf("Fail to mDdUpdateOverlay mDdUpdateOverlay.ddRVal = %d:%s\n",(int)mDdUpdateOverlay.ddRVal,DDErrorString(mDdUpdateOverlay.ddRVal)); 
      return mDdUpdateOverlay.ddRVal;
  }
 
  /* blt */
    

  DDRAWI_DDRAWSURFACE_LCL *pDDSurface = mpPrimaryLocals[0];

  if (!DdResetVisrgn(pDDSurface, NULL)) 
  {
   // derr(L"DirectDrawImpl[%08x]::_clear DdResetVisrgn failed", this);
  }

   
  memset(&mDdBlt, 0, sizeof(DDHAL_BLTDATA));
  memset(&mDdBlt.bltFX, 0, sizeof(DDBLTFX));
  mDdBlt.bltFX.dwSize = sizeof(DDBLTFX);

  mDdBlt.lpDD = &mDDrawGlobal;
  mDdBlt.Blt = mCallbacks.HALDDSurface.Blt; 
  mDdBlt.lpDDDestSurface = mpPrimaryLocals[0];
	
  mpPrimaryLocals[0]->hDC = (ULONG_PTR)GetDC(GetDesktopWindow());
  mDdBlt.rDest.top = 50;
  mDdBlt.rDest.bottom = 100;
  mDdBlt.rDest.left = 0;
  mDdBlt.rDest.right = 100;
  mDdBlt.lpDDSrcSurface = NULL;
  mDdBlt.IsClipped = FALSE;    
  mDdBlt.bltFX.dwFillColor = 0xFFFF00;
  mDdBlt.dwFlags = DDBLT_COLORFILL | DDBLT_WAIT;
 // mDdBlt.IsClipped = TRUE;
    
    if (mDdBlt.Blt(&mDdBlt) != DDHAL_DRIVER_HANDLED)
	{
      printf("Fail to mDdBlt = DDHAL_DRIVER_HANDLED\n");
	  return DDHAL_DRIVER_HANDLED;
    }

	

    if (mDdBlt.ddRVal!=DD_OK) 
	{      
		printf("Fail to mDdBlt mDdBlt.ddRVal = %d:%s\n",(int)mDdBlt.ddRVal,DDErrorString(mDdBlt.ddRVal)); 
		return mDdBlt.ddRVal;
    }

  mDdUpdateOverlay.rDest.right = 100;
   if ( mDdUpdateOverlay.UpdateOverlay(&mDdUpdateOverlay) == DDHAL_DRIVER_NOTHANDLED)
  {
    printf("Fail to mDdUpdateOverlay = DDERR_NOTINITIALIZED\n");
	return DDERR_NOTINITIALIZED;
  }

  while(TRUE);

  return DD_OK;


}


