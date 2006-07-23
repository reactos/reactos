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


HRESULT Hel_DirectDraw_GetAvailableVidMem(LPDIRECTDRAW7 iface, LPDDSCAPS2 ddscaps,
				   LPDWORD total, LPDWORD free)	
{
	IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	*total = HEL_GRAPHIC_MEMORY_MAX;
    *free = This->HELMemoryAvilable;
	return DD_OK;
}

HRESULT Hel_DirectDraw_GetScanLine(LPDIRECTDRAW7 iface, LPDWORD lpdwScanLine)
{
	DX_STUB;
}

HRESULT Hel_DirectDraw_FlipToGDISurface(LPDIRECTDRAW7 iface) 
{
	DX_STUB;
}
