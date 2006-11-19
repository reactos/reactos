/* $Id: kernel.c 24690 2006-11-05 21:19:53Z greatlrd $
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

ULONG WINAPI 
DirectDrawKernel_AddRef ( LPDIRECTDRAWKERNEL iface)
{   
      
   LPDDRAWI_KERNEL_INT This = (LPDDRAWI_KERNEL_INT)iface;
   
   ULONG ref=0;
   DX_WINDBG_trace();
    
   if (iface!=NULL)
   {
       This->dwIntRefCnt++;   
	   ref =  This->dwIntRefCnt;
   }    
   return ref;    
}

ULONG WINAPI 
DirectDrawKernel_Release ( LPDIRECTDRAWKERNEL iface)
{    
    LPDDRAWI_KERNEL_INT This = (LPDDRAWI_KERNEL_INT)iface;
	ULONG ref=0;

	DX_WINDBG_trace();

	if (iface!=NULL)
	{	  	
		This->dwIntRefCnt--;
            
		if (This->dwIntRefCnt == 0)
		{		
		    /* Add here if we need releae some memory pointer before 
             * exists
             */   
		      			
            if (This!=NULL)
            {              
			    HeapFree(GetProcessHeap(), 0, This);
            }
		}

		ref = This->dwIntRefCnt;
    }
    return ref;
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
   
   ULONG ref=0;
   DX_WINDBG_trace();
    
   if (iface!=NULL)
   {
       This->dwIntRefCnt++;   
	   ref =  This->dwIntRefCnt;
   }    
   return ref; 
}

ULONG WINAPI 
DDSurfaceKernel_Release ( LPDIRECTDRAWSURFACEKERNEL iface)
{    
    LPDDRAWI_DDKERNELSURFACE_INT This = (LPDDRAWI_DDKERNELSURFACE_INT)iface;
	ULONG ref=0;

	DX_WINDBG_trace();

	if (iface!=NULL)
	{	  	
		This->dwIntRefCnt--;
            
		if (This->dwIntRefCnt == 0)
		{		
		    /* Add here if we need releae some memory pointer before 
             * exists
             */   
		      			
            if (This!=NULL)
            {              
			    HeapFree(GetProcessHeap(), 0, This);
            }
		}

		ref = This->dwIntRefCnt;
    }
    return ref;
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
