/* $Id: main.c 21434 2006-04-01 19:12:56Z greatlrd $
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              DirectDraw Library 
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */

#include <windows.h>
#include "rosdraw.h"
#include "d3dhal.h"

VOID 
Cleanup(LPDIRECTDRAW7 iface) 
{
    IDirectDrawImpl* This = (IDirectDrawImpl*)iface;

	if (This->mDDrawGlobal.hDD != 0)
	{
     DdDeleteDirectDrawObject (&This->mDDrawGlobal);
	}

	if (This->mpTextures != NULL)
	{
	  DxHeapMemFree(This->mpTextures);
	}

	if (This->mpFourCC != NULL)
	{
	  DxHeapMemFree(This->mpFourCC);
	}

	if (This->mpvmList != NULL)
	{
	  DxHeapMemFree(This->mpvmList);
	}

	if (This->mpModeInfos != NULL)
	{
	  DxHeapMemFree(This->mpModeInfos);
	}

	if (This->hdc != NULL)
	{
	  DeleteDC(This->hdc);
	}
        
}

