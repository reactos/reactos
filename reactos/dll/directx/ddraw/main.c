
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


// This function is exported by the dll
HRESULT WINAPI DirectDrawCreateClipper (DWORD dwFlags, 
                                        LPDIRECTDRAWCLIPPER* lplpDDClipper, LPUNKNOWN pUnkOuter)
{
    DX_WINDBG_trace();

    return Main_DirectDraw_CreateClipper(NULL, dwFlags, lplpDDClipper, pUnkOuter);
}

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
		/* we do not use same error code as MS, ms use CLASS_E_NOAGGREGATION  */
		return CLASS_E_NOAGGREGATION; 
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
		/* we do not use same error code as MS, ms use CLASS_E_NOAGGREGATION */
		return CLASS_E_NOAGGREGATION; 
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
D3DParseUnknownCommand( LPVOID lpCmd, 
                        LPVOID *lpRetCmd)
{
    LPD3DHAL_DP2COMMAND dp2command = lpCmd;

    DWORD retCode = D3DERR_COMMAND_UNPARSED; 
    
    /* prevent it crash if null pointer are being sent */
    if ( (lpCmd == NULL) || (lpRetCmd == NULL) )
    {
        return E_FAIL;
    }
    
    *lpRetCmd = lpCmd;
    
    /* check for vaild command, only 3 command is vaild */
    if (dp2command->bCommand == D3DDP2OP_VIEWPORTINFO) 
    {    	
        /* dp2command->wStateCount * sizeof D3DHAL_DP2VIEWPORTINFO + 4 bytes */
        *(PBYTE)lpRetCmd += ((dp2command->wStateCount * sizeof(D3DHAL_DP2VIEWPORTINFO)) + sizeof(ULONG_PTR));
        retCode = 0;
    }                                                                                          
    else if (dp2command->bCommand == D3DDP2OP_WINFO) 
    {	
        /* dp2command->wStateCount * sizeof D3DHAL_DP2WINFO + 4 bytes */
        *(PBYTE)lpRetCmd += (dp2command->wStateCount * sizeof(D3DHAL_DP2WINFO)) + sizeof(ULONG_PTR);                                                          
        retCode = 0;
    } 
	else if (dp2command->bCommand == 0x0d) 
    {	
        /* dp2command->wStateCount * how many wStateCount ? + 4 bytes */
        *(PBYTE)lpRetCmd += ((dp2command->wStateCount * dp2command->bReserved) + sizeof(ULONG_PTR));               
        retCode = 0;
    }
   
	/* set error code for command 0 to 3, 8 and 15 to 255 */    
    else if ( (dp2command->bCommand <= D3DDP2OP_INDEXEDTRIANGLELIST) || // dp2command->bCommand  <= with 0 to 3
              (dp2command->bCommand == D3DDP2OP_RENDERSTATE) ||  // dp2command->bCommand  == with 8
              (dp2command->bCommand >= D3DDP2OP_LINELIST) )  // dp2command->bCommand  >= with 15 to 255
    {
       retCode = E_FAIL; 
    }
      
    return retCode;
}

 

