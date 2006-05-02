/* $Id: palette_main.c 21434 2006-04-01 19:12:56Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/main/palette.c
 * PURPOSE:              IDirectDrawPalette Implementation 
 * PROGRAMMER:           Maarten Bosma
 *
 */

#include "rosdraw.h"

ULONG WINAPI
DirectDrawPalette_Release( LPDIRECTDRAWPALETTE iface)
{
    DX_WINDBG_trace();

    IDirectDrawPaletteImpl* This = (IDirectDrawPaletteImpl*)iface;
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
DirectDrawPalette_AddRef( LPDIRECTDRAWPALETTE iface) 
{
   DX_WINDBG_trace();
      
   IDirectDrawPaletteImpl * This = (IDirectDrawPaletteImpl*)iface;
   
   ULONG ref=0;
    
   if (iface!=NULL)
   {
       ref = InterlockedIncrement( (PLONG) &This->ref);       
   }    
   return ref;    
}

HRESULT WINAPI
DirectDrawPalette_Initialize( LPDIRECTDRAWPALETTE iface,
				              LPDIRECTDRAW ddraw, 
                              DWORD dwFlags,
				              LPPALETTEENTRY palent)
{
   DX_WINDBG_trace();
   DX_STUB;  
}

HRESULT WINAPI
DirectDrawPalette_GetEntries( LPDIRECTDRAWPALETTE iface, 
                              DWORD dwFlags,
				              DWORD dwStart, DWORD dwCount,
				              LPPALETTEENTRY palent)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
DirectDrawPalette_SetEntries( LPDIRECTDRAWPALETTE iface, 
                              DWORD dwFlags,
				              DWORD dwStart,
                              DWORD dwCount,
				              LPPALETTEENTRY palent)
{
   DX_WINDBG_trace();
   DX_STUB;
}
HRESULT WINAPI
DirectDrawPalette_GetCaps( LPDIRECTDRAWPALETTE iface, 
                           LPDWORD lpdwCaps)
{
   DX_WINDBG_trace();
   DX_STUB;
}

HRESULT WINAPI
DirectDrawPalette_QueryInterface( LPDIRECTDRAWPALETTE iface,
				                  REFIID refiid, 
                                  LPVOID *obj)
{
   DX_WINDBG_trace();
   DX_STUB;
}

IDirectDrawPaletteVtbl DirectDrawPalette_Vtable =
{
    DirectDrawPalette_QueryInterface,
    DirectDrawPalette_AddRef,
    DirectDrawPalette_Release,
    DirectDrawPalette_GetCaps,
    DirectDrawPalette_GetEntries,
    DirectDrawPalette_Initialize,
    DirectDrawPalette_SetEntries
};
