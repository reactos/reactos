/* $Id: kernel.c 24690 2006-11-05 21:19:53Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS DirectX
 * FILE:                 ddraw/kernel/kernel_stubs.c
 * PURPOSE:              IDirectDrawKernel and IDirectDrawSurfaceKernel Implementation
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"


/***** IDirectDrawKernel ****/

ULONG WINAPI
DirectDrawKernel_AddRef ( LPDIRECTDRAWKERNEL iface)
{
  DX_WINDBG_trace();

   DX_STUB;
}

ULONG WINAPI
DirectDrawKernel_Release ( LPDIRECTDRAWKERNEL iface)
{
  DX_WINDBG_trace();

   DX_STUB;
}

HRESULT WINAPI
DirectDrawKernel_QueryInterface ( LPDIRECTDRAWKERNEL iface,
                                  REFIID riid,
                                  LPVOID* ppvObj)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
DirectDrawKernel_GetKernelHandle ( LPDIRECTDRAWKERNEL iface,
                                   ULONG* handle)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
DirectDrawKernel_ReleaseKernelHandle ( LPDIRECTDRAWKERNEL iface)
{
   DX_WINDBG_trace();
   DX_STUB;
}


ULONG WINAPI
DDSurfaceKernel_AddRef ( LPDIRECTDRAWSURFACEKERNEL iface)
{
   LPDDRAWI_DDKERNELSURFACE_INT This = (LPDDRAWI_DDKERNELSURFACE_INT)iface;

   DX_WINDBG_trace();

    if (iface!=NULL)
    {
        This->dwIntRefCnt++;
        //This->lpLcl->dwLocalRefCnt++;

        //if (This->lpLcl->lpGbl != NULL)
        //{
        //    This->lpLcl->lpGbl->dwRefCnt++;
        //}
    }
    return This->dwIntRefCnt;
}

ULONG WINAPI
DDSurfaceKernel_Release ( LPDIRECTDRAWSURFACEKERNEL iface)
{
    LPDDRAWI_DDKERNELSURFACE_INT This = (LPDDRAWI_DDKERNELSURFACE_INT)iface;

    DX_WINDBG_trace();
    /* FIXME
       This is not right exiame how it should be done
     */
    DX_STUB_str("FIXME This is not right exiame how it should be done\n");
    return This->dwIntRefCnt;
}

HRESULT WINAPI
DDSurfaceKernel_QueryInterface ( LPDIRECTDRAWSURFACEKERNEL iface,
                                 REFIID riid,
                                 LPVOID* ppvObj)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
DDSurfaceKernel_GetKernelHandle ( LPDIRECTDRAWSURFACEKERNEL iface,
                                  ULONG* handle)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
DDSurfaceKernel_ReleaseKernelHandle ( LPDIRECTDRAWSURFACEKERNEL iface)
{
   DX_WINDBG_trace();
   DX_STUB;
}


IDirectDrawKernelVtbl DirectDrawKernel_Vtable =
{
    DirectDrawKernel_QueryInterface,
    DirectDrawKernel_AddRef,
    DirectDrawKernel_Release,
	DirectDrawKernel_GetKernelHandle,
	DirectDrawKernel_ReleaseKernelHandle
};

IDirectDrawSurfaceKernelVtbl DirectDrawSurfaceKernel_Vtable =
{
    DDSurfaceKernel_QueryInterface,
    DDSurfaceKernel_AddRef,
    DDSurfaceKernel_Release,
	DDSurfaceKernel_GetKernelHandle,
	DDSurfaceKernel_ReleaseKernelHandle
};
