/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/kernel.c
 * PURPOSE:              IDirectDrawKernel and IDirectDrawSurfaceKernel Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"


/***** IDirectDrawKernel ****/

ULONG WINAPI Main_DirectDrawKernel_AddRef (LPDIRECTDRAWKERNEL iface)
{
   return 1;
}

ULONG WINAPI Main_DirectDrawKernel_Release (LPDIRECTDRAWKERNEL iface)
{
   return 0;
}

HRESULT WINAPI Main_DirectDrawKernel_QueryInterface (LPDIRECTDRAWKERNEL iface, REFIID riid, LPVOID* ppvObj)
{
	return E_NOINTERFACE;
}

HRESULT WINAPI Main_DirectDrawKernel_GetKernelHandle (LPDIRECTDRAWKERNEL iface, ULONG* handle)
{
   DX_STUB;
}

HRESULT WINAPI Main_DirectDrawKernel_ReleaseKernelHandle (LPDIRECTDRAWKERNEL iface)
{
   DX_STUB;
}


/***** IDirectDrawSurfaceKernel ****/

ULONG WINAPI Main_DDSurfaceKernel_AddRef (LPDIRECTDRAWSURFACEKERNEL iface)
{
   return 1;
}

ULONG WINAPI Main_DDSurfaceKernel_Release (LPDIRECTDRAWSURFACEKERNEL iface)
{
   return 0;
}

HRESULT WINAPI Main_DDSurfaceKernel_QueryInterface (LPDIRECTDRAWSURFACEKERNEL iface, REFIID riid, LPVOID* ppvObj)
{
	return E_NOINTERFACE;
}

HRESULT WINAPI Main_DDSurfaceKernel_GetKernelHandle (LPDIRECTDRAWSURFACEKERNEL iface, ULONG* handle)
{
   DX_STUB;
}

HRESULT WINAPI Main_DDSurfaceKernel_ReleaseKernelHandle (LPDIRECTDRAWSURFACEKERNEL iface)
{
   DX_STUB;
}


IDirectDrawKernelVtbl DirectDrawKernel_Vtable =
{
    Main_DirectDrawKernel_QueryInterface,
    Main_DirectDrawKernel_AddRef,
    Main_DirectDrawKernel_Release,
	Main_DirectDrawKernel_GetKernelHandle,
	Main_DirectDrawKernel_ReleaseKernelHandle
};

IDirectDrawSurfaceKernelVtbl DirectDrawSurfaceKernel_Vtable =
{
    Main_DDSurfaceKernel_QueryInterface,
    Main_DDSurfaceKernel_AddRef,
    Main_DDSurfaceKernel_Release,
	Main_DDSurfaceKernel_GetKernelHandle,
	Main_DDSurfaceKernel_ReleaseKernelHandle
};
