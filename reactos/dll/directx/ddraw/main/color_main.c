/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/color.c
 * PURPOSE:              IDirectDrawColorControl Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"

ULONG WINAPI
DirectDrawColorControl_AddRef( LPDIRECTDRAWCOLORCONTROL iface)
{   
   DX_WINDBG_trace();
      
   IDirectDrawColorImpl * This = (IDirectDrawColorImpl*)iface;
   
   ULONG ref=0;
    
   if (iface!=NULL)
   {
       ref = InterlockedIncrement( (PLONG) &This->ref);       
   }    
   return ref; 
}

ULONG WINAPI
DirectDrawColorControl_Release( LPDIRECTDRAWCOLORCONTROL iface)
{
    DX_WINDBG_trace();

    IDirectDrawColorImpl* This = (IDirectDrawColorImpl*)iface;
	ULONG ref=0;

	if (iface!=NULL)
	{	  	
		ref = InterlockedDecrement( (PLONG) &This->ref);
            
		if (ref == 0)
		{		
		    /* Add here if we need releae some memory pointer before 
             * exists
             */   
		      			
            if (This!=NULL)
            {              
			    HeapFree(GetProcessHeap(), 0, This);
            }
		}
    }
    return ref;
}

HRESULT WINAPI
DirectDrawColorControl_QueryInterface( LPDIRECTDRAWCOLORCONTROL iface, 
									   REFIID riid, 
                                       LPVOID* ppvObj) 
{
   DX_WINDBG_trace();
   DX_STUB;  
}

HRESULT WINAPI
DirectDrawColorControl_GetColorControls( LPDIRECTDRAWCOLORCONTROL iface, 
                                         LPDDCOLORCONTROL lpColorControl)
{
   DX_WINDBG_trace();
   DX_STUB;  
}

HRESULT WINAPI
DirectDrawColorControl_SetColorControls( LPDIRECTDRAWCOLORCONTROL iface,
                                         LPDDCOLORCONTROL lpColorControl)
{
   DX_WINDBG_trace();
   DX_STUB;  
}

IDirectDrawColorControlVtbl DirectDrawColorControl_Vtable =
{
    DirectDrawColorControl_QueryInterface,
    DirectDrawColorControl_AddRef,
    DirectDrawColorControl_Release,
    DirectDrawColorControl_GetColorControls,
    DirectDrawColorControl_SetColorControls
};
