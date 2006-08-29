/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/clipper.c
 * PURPOSE:              IDirectDrawClipper Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"


ULONG WINAPI 
DirectDrawClipper_Release(LPDIRECTDRAWCLIPPER iface) 
{    
    DX_WINDBG_trace();

    IDirectDrawClipperImpl* This = (IDirectDrawClipperImpl*)iface;
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

ULONG WINAPI 
DirectDrawClipper_AddRef (LPDIRECTDRAWCLIPPER iface)
{    
   DX_WINDBG_trace();
      
   IDirectDrawClipperImpl * This = (IDirectDrawClipperImpl*)iface;
   
   ULONG ref=0;
    
   if (iface!=NULL)
   {
       ref = InterlockedIncrement( (PLONG) &This->ref);       
   }    
   return ref;       
}

HRESULT WINAPI 
DirectDrawClipper_Initialize( LPDIRECTDRAWCLIPPER iface, 
                              LPDIRECTDRAW lpDD, 
                              DWORD dwFlags) 
{
   /* FIXME not implment */
   DX_WINDBG_trace();
   DX_STUB_DD_OK;
}

HRESULT WINAPI 
DirectDrawClipper_SetHwnd( LPDIRECTDRAWCLIPPER iface, 
                           DWORD dwFlags, 
                           HWND hWnd) 
{
   /* FIXME not implment */
   DX_WINDBG_trace();
   DX_STUB_DD_OK;
}

HRESULT WINAPI 
DirectDrawClipper_GetClipList( LPDIRECTDRAWCLIPPER iface, 
                               LPRECT lpRect, 
                               LPRGNDATA lpClipList,
                               LPDWORD lpdwSize)
{
   DX_WINDBG_trace();
   DX_STUB;    
}

HRESULT WINAPI 
DirectDrawClipper_SetClipList( LPDIRECTDRAWCLIPPER iface,
                               LPRGNDATA lprgn,
                               DWORD dwFlag) 
{
   DX_WINDBG_trace();
   DX_STUB;    
}

HRESULT WINAPI 
DirectDrawClipper_QueryInterface( LPDIRECTDRAWCLIPPER iface, 
                                  REFIID riid, 
                                  LPVOID* ppvObj) 
{
   DX_WINDBG_trace();
   DX_STUB;    
}

HRESULT WINAPI 
DirectDrawClipper_GetHWnd( LPDIRECTDRAWCLIPPER iface, 
                           HWND* hWndPtr) 
{
   DX_WINDBG_trace();
   DX_STUB;    
}

HRESULT WINAPI 
DirectDrawClipper_IsClipListChanged( LPDIRECTDRAWCLIPPER iface, 
                                     BOOL* lpbChanged) 
{
   DX_WINDBG_trace();
   DX_STUB;    
}

IDirectDrawClipperVtbl DirectDrawClipper_Vtable =
{
    DirectDrawClipper_QueryInterface,
    DirectDrawClipper_AddRef,
    DirectDrawClipper_Release,
    DirectDrawClipper_GetClipList,
    DirectDrawClipper_GetHWnd,
    DirectDrawClipper_Initialize,
    DirectDrawClipper_IsClipListChanged,
    DirectDrawClipper_SetClipList,
    DirectDrawClipper_SetHwnd
};
