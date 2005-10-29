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
 

	/* point to it self */
	This->DirectDrawGlobal.lp16DD = &This->DirectDrawGlobal;

	/* get the object */
	if(!DdCreateDirectDrawObject (&This->DirectDrawGlobal, This->hdc))
		return DDERR_INVALIDPARAMS;

			
	/* alloc all the space */
	This->DirectDrawGlobal.lpDDCBtmp = (LPDDHAL_CALLBACKS)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DDHAL_CALLBACKS));		
	This->DirectDrawGlobal.lpD3DHALCallbacks = (ULONG_PTR)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY, sizeof(D3DHAL_CALLBACKS));	
	This->DirectDrawGlobal.lpD3DGlobalDriverData = (ULONG_PTR)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(D3DHAL_GLOBALDRIVERDATA));			
	

	/* Fill in some info */
	This->HalInfo.lpD3DGlobalDriverData = This->DirectDrawGlobal.lpD3DGlobalDriverData;
	This->HalInfo.lpD3DHALCallbacks = This->DirectDrawGlobal.lpD3DHALCallbacks;
	This->HalInfo.lpDDCallbacks = &This->DirectDrawGlobal.lpDDCBtmp->HALDD;
	This->HalInfo.lpDDExeBufCallbacks = &This->DirectDrawGlobal.lpDDCBtmp->HALDDExeBuf;
	This->HalInfo.lpDDPaletteCallbacks = &This->DirectDrawGlobal.lpDDCBtmp->HALDDPalette;
	This->HalInfo.lpDDSurfaceCallbacks = &This->DirectDrawGlobal.lpDDCBtmp->HALDDSurface;

	/* FIXME The insate is not right we need the info that the three NULL return */
	
	/* query all kinds of infos from the driver */
	if(!DdQueryDirectDrawObject (
		&This->DirectDrawGlobal, 
		&This->HalInfo, 
		This->HalInfo.lpDDCallbacks,
		This->HalInfo.lpDDSurfaceCallbacks,
		This->HalInfo.lpDDPaletteCallbacks,
		(LPD3DHAL_CALLBACKS)This->DirectDrawGlobal.lpD3DHALCallbacks,
		(LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData,
		This->HalInfo.lpDDExeBufCallbacks, 
		NULL, 
		NULL, 
		NULL ))
	{
		OutputDebugString(L"First DdQueryDirectDrawObject failed");
		return 1;
	}

	/* FIXME The insate is not right we need the info that the three NULL return and use it here*/

	/* Fixme shall we selected the pixel type before we call the DdQueryDirectDrawObject second time 
	  and fill in more info ?? */

	This->HalInfo.vmiData.pvmList = HeapAlloc(GetProcessHeap(), 0, sizeof(VIDMEM) * This->HalInfo.vmiData.dwNumHeaps);
	This->DirectDrawGlobal.lpdwFourCC = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD) * This->HalInfo.ddCaps.dwNumFourCCCodes);
	This->DirectDrawGlobal.lpZPixelFormats = HeapAlloc(GetProcessHeap(), 0, sizeof(DDPIXELFORMAT) * This->DirectDrawGlobal.dwNumZPixelFormats);
	((LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData)->lpTextureFormats = HeapAlloc(GetProcessHeap(), 0, sizeof(DDSURFACEDESC) * ((LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData)->dwNumTextureFormats);

	if(!DdQueryDirectDrawObject (
		&This->DirectDrawGlobal, 
		&This->HalInfo, 
		This->HalInfo.lpDDCallbacks,
		This->HalInfo.lpDDSurfaceCallbacks,
		This->HalInfo.lpDDPaletteCallbacks,
		(LPD3DHAL_CALLBACKS)This->DirectDrawGlobal.lpD3DHALCallbacks,
		(LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData,
		This->HalInfo.lpDDExeBufCallbacks, 
		((LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData)->lpTextureFormats, 
		This->DirectDrawGlobal.lpdwFourCC, 
		This->HalInfo.vmiData.pvmList 
		))
	{
		OutputDebugString(L"Second DdQueryDirectDrawObject failed");
		return 1;
	}
		
	/* Copy HalInfo to DirectDrawGlobal (Not complete)*/
	
	RtlCopyMemory(&This->DirectDrawGlobal.vmiData,&This->HalInfo.vmiData,sizeof(VIDMEMINFO));
	RtlCopyMemory(&This->DirectDrawGlobal.ddCaps,&This->HalInfo.ddCaps,sizeof(DDCORECAPS));
	This->DirectDrawGlobal.dwMonitorFrequency = This->HalInfo.dwMonitorFrequency;
        
    /* have not check where it should go into yet
       This->HalInfo.GetDriverInfo datatype LPDDHAL_GETDRIVERINFO	
    */

    This->DirectDrawGlobal.dwModeIndex = This->HalInfo.dwModeIndex;

	/* have not check where it should go into yet
	   This->HalInfo.lpdwFourCC datatype LPDWORD			
     */

    This->DirectDrawGlobal.dwNumModes =  This->HalInfo.dwNumModes;
    This->DirectDrawGlobal.lpModeInfo =  This->HalInfo.lpModeInfo;

	/* have not check where it should go into yet
	   This->HalInfo.dwFlags data type DWORD
	*/

	/* Unsure which of these two for lpPDevice 
      This->DirectDrawGlobal.dwPDevice = This->HalInfo.lpPDevice;
	  This->lpDriverHandle = This->HalInfo.lpPDevice;
	*/

    This->DirectDrawGlobal.hInstance = This->HalInfo.hInstance;    
    
	/* have not check where it should go into yet
       This->lpD3DGlobalDriverData datatype ULONG_PTR
       This->lpD3DHALCallbacks datatype  ULONG_PTR
    */ 
	 RtlCopyMemory(&This->DirectDrawGlobal.lpDDCBtmp->HALDDExeBuf,&This->HalInfo.lpDDExeBufCallbacks,sizeof(DDHAL_DDEXEBUFCALLBACKS));

	
	 
	/************************************************************************/
	/* Set up the rest of the callbacks all callbacks we get back from      */
	/* gdi32.dll is user mode                                               */
	/************************************************************************/

	/* Todo add a check see if HalInfo.GetDriverInfo is supported or not */

	 /* Do not trust msdn what it say about dwContext it is not in use for 
	    windows nt, it is in use for all os, and it always pont to 
		DirectDrawGlobal.hDD                                             */

	DDHAL_GETDRIVERINFODATA DriverInfo;
	memset(&DriverInfo,0, sizeof(DDHAL_GETDRIVERINFODATA));
	DriverInfo.dwSize = sizeof(DDHAL_GETDRIVERINFODATA);
	DriverInfo.dwContext = DirectDrawGlobal.hDD; 

	


	/* Get ColorControlCallbacks  */	
	DriverInfo.guidInfo = GUID_ColorControlCallbacks;
	DriverInfo.lpvData = &This->DirectDrawGlobal.lpDDCBtmp->HALDDColorControl;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDCOLORCONTROLCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);

	
	/* Get the GUID_D3DCallbacks callback */

	/* Problem with include files
	DDHAL_DDMISCELLANEOUSCALLBACKS  misc;
	DriverInfo.guidInfo = GUID_D3DCallbacks;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);*/
	
	/* Get the D3DCallbacks2 */
	/* do we need alloc memmory for This->DirectDrawGlobal.lpD3DHALCallbacks2 */
	DriverInfo.guidInfo = GUID_D3DCallbacks2;
	DriverInfo.lpvData =  &This->DirectDrawGlobal.lpD3DHALCallbacks2;
	DriverInfo.dwExpectedSize = sizeof(D3DHAL_CALLBACKS2);
	This->HalInfo.GetDriverInfo( &DriverInfo);

	
	/* Get the D3DCallbacks3 */	
	/*
	DriverInfo.guidInfo = GUID_D3DCallbacks3;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	*/

	/* Get the misc callback */
	/* Problem with include files	
	DriverInfo.guidInfo = GUID_D3DCaps;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	*/

	/* Get the D3DExtendedCaps  */
	/*
	DriverInfo.guidInfo = GUID_D3DExtendedCaps;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
    */

	/* Get the D3DParseUnknownCommandCallback  */
	/*
	DriverInfo.guidInfo = GUID_D3DParseUnknownCommandCallback;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
    */

	/* Get the GetHeapAlignment  */	
	/*
	DriverInfo.guidInfo = GUID_GetHeapAlignment;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	*/
	
	/* Get the KernelCallbacks  */	
	DriverInfo.guidInfo = GUID_KernelCallbacks;
	DriverInfo.lpvData = &This->DirectDrawGlobal.lpDDCBtmp->HALDDKernel;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDKERNELCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	

	/* Get the KernelCaps  */
	/* Need Alloc memmory for lpDDKernelCaps ?? */
	DriverInfo.guidInfo = GUID_KernelCaps;
	DriverInfo.lpvData = &This->DirectDrawGlobal.lpDDKernelCaps;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDKERNELCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	

	/* Get the MiscellaneousCallbacks  */	
	DriverInfo.guidInfo = GUID_MiscellaneousCallbacks;
	DriverInfo.lpvData = &This->DirectDrawGlobal.lpDDCBtmp->HALDDMiscellaneous;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	

	/* Get the Miscellaneous2Callbacks  */
	/*
	DriverInfo.guidInfo = GUID_Miscellaneous2Callbacks;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
    */

	/* Get the MotionCompCallbacks  */		
	DriverInfo.guidInfo = GUID_MotionCompCallbacks;
	DriverInfo.lpvData = &This->DirectDrawGlobal.lpDDCBtmp->HALDDMotionComp;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMOTIONCOMPCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	

    /* Get the NonLocalVidMemCaps  */
	/*
	DriverInfo.guidInfo = GUID_NonLocalVidMemCaps;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	*/

    /* Get the NTCallbacks  */
	/*
	DriverInfo.guidInfo = GUID_NTCallbacks;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	*/
	
	/* Get the NTPrivateDriverCaps  */	
	/*
	DriverInfo.guidInfo = GUID_NTPrivateDriverCaps;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	*/
	
	/* Get the UpdateNonLocalHeap  */	
	/*
	DriverInfo.guidInfo = GUID_UpdateNonLocalHeap;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	*/
	
	/* Get the VideoPortCallbacks  */    
	/*  Problem with include files
	DriverInfo.guidInfo = GUID_VideoPortCallbacks;
	DriverInfo.lpvData = &This->DirectDrawGlobal.lpDDCBtmp->HALDDVideoPort;
	DriverInfo.dwExpectedSize = sizeof(HALDDVideoPort);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	*/
	
	
	/* Get the VideoPortCaps  */
	/*
	DriverInfo.guidInfo = GUID_VideoPortCaps;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	*/

	/* Get the ZPixelFormats */
	/*
	DriverInfo.guidInfo = GUID_ZPixelFormats;
	DriverInfo.lpvData = &misc;
	DriverInfo.dwExpectedSize = sizeof(DDHAL_DDMISCELLANEOUSCALLBACKS);
	This->HalInfo.GetDriverInfo( &DriverInfo);
	*/
	
	
	/* FIXME add setup for one call ot get memmory */


	/* now all insate of HAL is done and we hopply do not have forget anything*/

	return DD_OK;
}

HRESULT Hal_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface)
{
   	return DD_OK;
}


VOID Hal_DirectDraw_Release (LPDIRECTDRAW7 iface) 
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	DdDeleteDirectDrawObject (&This->DirectDrawGlobal);

	if(This->HalInfo.vmiData.pvmList)
		HeapFree(GetProcessHeap(), 0, This->HalInfo.vmiData.pvmList);
	if(This->DirectDrawGlobal.lpdwFourCC)
		HeapFree(GetProcessHeap(), 0, This->DirectDrawGlobal.lpdwFourCC);
	if(This->DirectDrawGlobal.lpZPixelFormats)
		HeapFree(GetProcessHeap(), 0, This->DirectDrawGlobal.lpZPixelFormats);
	if(((LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData)->lpTextureFormats)
		HeapFree(GetProcessHeap(), 0, ((LPD3DHAL_GLOBALDRIVERDATA)This->DirectDrawGlobal.lpD3DGlobalDriverData)->lpTextureFormats);			
	
	if(This->DirectDrawGlobal.lpDDCBtmp)
		HeapFree(GetProcessHeap(), 0, This->DirectDrawGlobal.lpDDCBtmp);
	if(This->DirectDrawGlobal.lpD3DHALCallbacks)
		HeapFree(GetProcessHeap(), 0, (PVOID)This->DirectDrawGlobal.lpD3DHALCallbacks);
	if(This->DirectDrawGlobal.lpD3DGlobalDriverData)
		HeapFree(GetProcessHeap(), 0, (PVOID)This->DirectDrawGlobal.lpD3DGlobalDriverData);
}
