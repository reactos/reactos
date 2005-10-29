/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/videoport.c
 * PURPOSE:              IDirectDrawVideoPort, DDVideoPortContainer and IDirectDrawVideoPortNotify Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"


/************* IDirectDrawVideoPort *************/ 

HRESULT WINAPI 
Main_DirectDrawVideoPort_QueryInterface (LPDIRECTDRAWVIDEOPORT iface, REFIID riid, LPVOID* ppvObj)
{
	return E_NOINTERFACE;
}

ULONG WINAPI 
Main_DirectDrawVideoPort_AddRef (LPDIRECTDRAWVIDEOPORT iface) 
{
	return 1;
}

ULONG WINAPI 
Main_DirectDrawVideoPort_Release (LPDIRECTDRAWVIDEOPORT iface)
{
	return 0;
}

HRESULT WINAPI 
Main_DirectDrawVideoPort_Flip (LPDIRECTDRAWVIDEOPORT iface, LPDIRECTDRAWSURFACE lpDDSurface, DWORD dwFlags)
{
	DX_STUB;
}

HRESULT WINAPI 
Main_DirectDrawVideoPort_GetBandwidthInfo (LPDIRECTDRAWVIDEOPORT iface, LPDDPIXELFORMAT lpddpfFormat, DWORD dwWidth, 
										   DWORD dwHeight, DWORD dwFlags, LPDDVIDEOPORTBANDWIDTH lpBandwidth)
{
	DX_STUB;
}

HRESULT WINAPI 
Main_DirectDrawVideoPort_GetColorControls (LPDIRECTDRAWVIDEOPORT iface, LPDDCOLORCONTROL lpColorControl)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawVideoPort_GetInputFormats (LPDIRECTDRAWVIDEOPORT iface, LPDWORD lpNumFormats, 
														 LPDDPIXELFORMAT lpFormats, DWORD dwFlags)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawVideoPort_GetOutputFormats (LPDIRECTDRAWVIDEOPORT iface, LPDDPIXELFORMAT lpInputFormat, 
														  LPDWORD lpNumFormats, LPDDPIXELFORMAT lpFormats, DWORD dwFlags)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawVideoPort_GetFieldPolarity (LPDIRECTDRAWVIDEOPORT iface, LPBOOL lpbFieldPolarity)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawVideoPort_GetVideoLine (LPDIRECTDRAWVIDEOPORT This, LPDWORD lpdwLine)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawVideoPort_GetVideoSignalStatus (LPDIRECTDRAWVIDEOPORT iface, LPDWORD lpdwStatus)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawVideoPort_SetColorControls (LPDIRECTDRAWVIDEOPORT iface, LPDDCOLORCONTROL lpColorControl)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawVideoPort_SetTargetSurface (LPDIRECTDRAWVIDEOPORT iface, LPDIRECTDRAWSURFACE lpDDSurface, 
														  DWORD dwFlags)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawVideoPort_StartVideo (LPDIRECTDRAWVIDEOPORT iface, LPDDVIDEOPORTINFO dwFlags)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawVideoPort_StopVideo (LPDIRECTDRAWVIDEOPORT iface)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawVideoPort_UpdateVideo (LPDIRECTDRAWVIDEOPORT iface, LPDDVIDEOPORTINFO dwFlags)
{
	DX_STUB;
}

HRESULT WINAPI Main_DirectDrawVideoPort_WaitForSync (LPDIRECTDRAWVIDEOPORT iface, DWORD dwFlags, DWORD dwLine, 
													 DWORD dwTimeout)
{
	DX_STUB;
}

/************* IDDVideoPortContainer *************/ 

HRESULT WINAPI Main_DDVideoPortContainer_QueryInterface (LPDDVIDEOPORTCONTAINER iface, REFIID riid, LPVOID* ppvObj)
{
	return E_NOINTERFACE;
}

ULONG WINAPI Main_DDVideoPortContainer_AddRef (LPDDVIDEOPORTCONTAINER iface) 
{
	return 1;
}

ULONG WINAPI Main_DDVideoPortContainer_Release (LPDDVIDEOPORTCONTAINER iface)
{
	return 0;
}

HRESULT WINAPI Main_DDVideoPortContainer_CreateVideoPort (LPDDVIDEOPORTCONTAINER iface, DWORD dwFlags, LPDDVIDEOPORTDESC pPortDesc, 
														  LPDIRECTDRAWVIDEOPORT* DDVideoPort, IUnknown* pUnkOuter)
{
    DX_STUB;
}    

HRESULT WINAPI Main_DDVideoPortContainer_EnumVideoPorts (LPDDVIDEOPORTCONTAINER iface, DWORD dwFlags, LPDDVIDEOPORTCAPS pCaps, LPVOID pContext, 
														 LPDDENUMVIDEOCALLBACK pEnumVideoCallback)
{
    DX_STUB;
}

HRESULT WINAPI Main_DDVideoPortContainer_GetVideoPortConnectInfo (LPDDVIDEOPORTCONTAINER iface, DWORD PortId, DWORD* pNumEntries, 
																  LPDDVIDEOPORTCONNECT pConnectInfo)
{
    DX_STUB;
}

HRESULT WINAPI Main_DDVideoPortContainer_QueryVideoPortStatus (LPDDVIDEOPORTCONTAINER iface, DWORD PortId, LPDDVIDEOPORTSTATUS pStatus)
{
    DX_STUB;
}

/************* IDirectDrawVideoPortNotify *************/ 

HRESULT WINAPI Main_DDVideoPortNotify_QueryInterface (LPDIRECTDRAWVIDEOPORTNOTIFY iface, REFIID riid, LPVOID* ppvObj)
{
	return E_NOINTERFACE;
}

ULONG WINAPI Main_DDVideoPortNotify_AddRef (LPDIRECTDRAWVIDEOPORTNOTIFY iface)
{
	return 1;
}

ULONG WINAPI Main_DDVideoPortNotify_Release (LPDIRECTDRAWVIDEOPORTNOTIFY iface)
{
	return 0;
}

HRESULT WINAPI Main_DDVideoPortNotify_AcquireNotification (LPDIRECTDRAWVIDEOPORTNOTIFY iface, HANDLE* h, LPDDVIDEOPORTNOTIFY pVideoPortNotify)
{
    DX_STUB;
}

HRESULT WINAPI Main_DDVideoPortNotify_ReleaseNotification (LPDIRECTDRAWVIDEOPORTNOTIFY iface, HANDLE h)
{
    DX_STUB;
}

IDirectDrawVideoPortVtbl DirectDrawVideoPort_Vtable =
{
	Main_DirectDrawVideoPort_QueryInterface,
	Main_DirectDrawVideoPort_AddRef,
	Main_DirectDrawVideoPort_Release,
	Main_DirectDrawVideoPort_Flip, 
	Main_DirectDrawVideoPort_GetBandwidthInfo, 
	Main_DirectDrawVideoPort_GetColorControls, 
	Main_DirectDrawVideoPort_GetInputFormats, 
	Main_DirectDrawVideoPort_GetOutputFormats, 
	Main_DirectDrawVideoPort_GetFieldPolarity, 
	Main_DirectDrawVideoPort_GetVideoLine,
	Main_DirectDrawVideoPort_GetVideoSignalStatus, 
	Main_DirectDrawVideoPort_SetColorControls, 
	Main_DirectDrawVideoPort_SetTargetSurface, 
	Main_DirectDrawVideoPort_StartVideo, 
	Main_DirectDrawVideoPort_StopVideo, 
	Main_DirectDrawVideoPort_UpdateVideo, 
	Main_DirectDrawVideoPort_WaitForSync
};

IDDVideoPortContainerVtbl DDVideoPortContainer_Vtable =
{
    Main_DDVideoPortContainer_QueryInterface,
    Main_DDVideoPortContainer_AddRef,
    Main_DDVideoPortContainer_Release,
    Main_DDVideoPortContainer_CreateVideoPort,
    Main_DDVideoPortContainer_EnumVideoPorts,
    Main_DDVideoPortContainer_GetVideoPortConnectInfo,
    Main_DDVideoPortContainer_QueryVideoPortStatus
};

IDirectDrawVideoPortNotifyVtbl DDVideoPortNotify_Vtable =
{
   Main_DDVideoPortNotify_QueryInterface,
   Main_DDVideoPortNotify_AddRef,
   Main_DDVideoPortNotify_Release,
   Main_DDVideoPortNotify_AcquireNotification,
   Main_DDVideoPortNotify_ReleaseNotification 
};
