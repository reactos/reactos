/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS
 * FILE:                 lib/ddraw/soft/ddraw.c
 * PURPOSE:              DirectDraw Software Implementation 
 * PROGRAMMER:           Magnus Olsen, Maarten Bosma
 *
 */

#include "rosdraw.h"


HRESULT Hel_DirectDraw_Initialize (LPDIRECTDRAW7 iface)
{
	return DD_OK;
}

HRESULT Hel_DirectDraw_SetCooperativeLevel (LPDIRECTDRAW7 iface)
{
   	return DD_OK;
}

VOID Hel_DirectDraw_Release (LPDIRECTDRAW7 iface) 
{
}


HRESULT Hel_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
				   LPDWORD total, LPDWORD free)	
{
	DX_STUB;
}


HRESULT Hel_DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW7 iface, DWORD dwFlags,HANDLE h) 
{
	DX_STUB;
}

HRESULT Hel_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine)
{
	DX_STUB;
}
