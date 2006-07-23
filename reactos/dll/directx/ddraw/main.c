
/* $Id$
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


/*
 * IMPLEMENT
 * Status this api is finish and is 100% correct 
 */

HRESULT 
WINAPI 
DirectDrawCreate (LPGUID lpGUID, 
				  LPDIRECTDRAW* lplpDD, 
				  LPUNKNOWN pUnkOuter) 
{   
	/* check see if pUnkOuter is null or not */
	if (pUnkOuter)
	{
		/* we do not use same error code as MS, ms use 0x8004110 */
		return DDERR_INVALIDPARAMS; 
	}
	
	return Create_DirectDraw (lpGUID, lplpDD, &IID_IDirectDraw, FALSE);
}

/*
 * IMPLEMENT
 * Status this api is finish and is 100% correct 
 */

HRESULT 
WINAPI 
DirectDrawCreateEx(LPGUID lpGUID, 
				   LPVOID* lplpDD, 
				   REFIID id, 
				   LPUNKNOWN pUnkOuter)
{    	
	/* check see if pUnkOuter is null or not */
	if (pUnkOuter)
	{
		/* we do not use same error code as MS, ms use 0x8004110 */
		return DDERR_INVALIDPARAMS; 
	}
	
	/* Is it a DirectDraw 7 Request or not */
	if (!IsEqualGUID(id, &IID_IDirectDraw7)) 
	{
	  return DDERR_INVALIDPARAMS;
	}

    return Create_DirectDraw (lpGUID, (LPDIRECTDRAW*)lplpDD, id, TRUE);
}

/*
 * UNIMPLEMENT 
 */

HRESULT 
WINAPI 
DirectDrawEnumerateA(
                     LPDDENUMCALLBACKA lpCallback, 
                     LPVOID lpContext)
{
    DX_STUB;
}


/*
 * UNIMPLEMENT 
 */

HRESULT WINAPI DirectDrawEnumerateW(LPDDENUMCALLBACKW lpCallback, 
                                    LPVOID lpContext)
{
    DX_STUB;
}

/*
 * UNIMPLEMENT 
 */

HRESULT 
WINAPI 
DirectDrawEnumerateExA(LPDDENUMCALLBACKEXA lpCallback, 
                       LPVOID lpContext, 
                       DWORD dwFlags)
{
    DX_STUB;
}

/*
 * UNIMPLEMENT 
 */

HRESULT 
WINAPI 
DirectDrawEnumerateExW(LPDDENUMCALLBACKEXW lpCallback, 
                       LPVOID lpContext, 
                       DWORD dwFlags)
{
     DX_STUB;
}

/*
   See http://msdn.microsoft.com/library/default.asp?url=/library/en-us/
       Display_d/hh/Display_d/d3d_21ac30ea-9803-401e-b541-6b08af79653d.xml.asp

   for more info about this command 

 */

/*
 * UNIMPLEMENT 
 * Status FIXME need be implement and this code is realy need be tested
 */

HRESULT 
WINAPI 
D3DParseUnknownCommand( LPVOID lpvCommands, 
                        LPVOID *lplpvReturnedCommand)
{
    LPD3DHAL_DP2COMMAND cmd = lpvCommands;

    DWORD retCode = D3DERR_COMMAND_UNPARSED; 

    *lplpvReturnedCommand = lpvCommands;
     
    if (cmd->bCommand > D3DDP2OP_TRIANGLESTRIP)
    {
        retCode = DD_FALSE;

        if (cmd->bCommand == D3DDP2OP_VIEWPORTINFO)
        {
            /* FIXME */
            retCode = DD_OK; 
        }

        if (cmd->bCommand == D3DDP2OP_WINFO)
        {
            /* FIXME */
            retCode = DD_OK; 
        }     
    }
    else if (cmd->bCommand == D3DDP2OP_TRIANGLESTRIP)  
    {
        /* FIXME */
        retCode = DD_OK; 
    }
  
    if ((cmd->bCommand <= D3DDP2OP_INDEXEDTRIANGLELIST) || (cmd->bCommand == D3DDP2OP_RENDERSTATE))
    {
        retCode = DD_FALSE;
    }

    return retCode;
}

 

