/* $Id: gamma.c 24690 2006-11-05 21:19:53Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/gamma.c
 * PURPOSE:              IDirectDrawGamma Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"

ULONG WINAPI
DirectDrawGammaControl_AddRef( LPDIRECTDRAWGAMMACONTROL iface)
{         
   LPDDRAWI_DDGAMMACONTROL_INT This = (LPDDRAWI_DDGAMMACONTROL_INT)iface;
    
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
DirectDrawGammaControl_Release( LPDIRECTDRAWGAMMACONTROL iface)
{    
    LPDDRAWI_DDGAMMACONTROL_INT This = (LPDDRAWI_DDGAMMACONTROL_INT)iface;
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
DirectDrawGammaControl_QueryInterface( LPDIRECTDRAWGAMMACONTROL iface, 
                                       REFIID riid,
				                       LPVOID *ppObj)
{
   DX_WINDBG_trace();
   DX_STUB;  
}

HRESULT WINAPI
DirectDrawGammaControl_GetGammaRamp( LPDIRECTDRAWGAMMACONTROL iface, 
                                     DWORD dwFlags, 
                                     LPDDGAMMARAMP lpGammaRamp)
{
   DX_WINDBG_trace();
   DX_STUB;  
}

HRESULT WINAPI
DirectDrawGammaControl_SetGammaRamp( LPDIRECTDRAWGAMMACONTROL iface, 
                                     DWORD dwFlags, 
                                     LPDDGAMMARAMP lpGammaRamp)
{
   DX_WINDBG_trace();
   DX_STUB;  
}

IDirectDrawGammaControlVtbl DirectDrawGammaControl_Vtable =
{
    DirectDrawGammaControl_QueryInterface,
    DirectDrawGammaControl_AddRef,
    DirectDrawGammaControl_Release,
    DirectDrawGammaControl_GetGammaRamp,
    DirectDrawGammaControl_SetGammaRamp
};
