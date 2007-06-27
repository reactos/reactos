
/* $Id$
 *
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 lib/ddraw/ddraw.c
 * PURPOSE:              DirectDraw Library 
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */


#include "rosdraw.h"

/* PSEH for SEH Support */
#include <pseh/pseh.h>

CRITICAL_SECTION ddcs;

// This function is exported by the dll



/*++
* @name DirectDrawCreateClipper
*
*     The DirectDrawCreateClipper routine <FILLMEIN>.
*
* @param dwFlags
*        <FILLMEIN>.
*
* @param lplpDDClipper
*        <FILLMEIN>.
*
* @param pUnkOuter
*        <FILLMEIN>.
*
* @return  <FILLMEIN>.
*
* @remarks None.
*
*--*/
HRESULT WINAPI DirectDrawCreateClipper (DWORD dwFlags, 
                                        LPDIRECTDRAWCLIPPER* lplpDDClipper, LPUNKNOWN pUnkOuter)
{
    DX_WINDBG_trace();

    return Main_DirectDraw_CreateClipper(NULL, dwFlags, lplpDDClipper, pUnkOuter);
}

/*++
* @name DirectDrawCreate
*
*     The DirectDrawCreate routine <FILLMEIN>.
*
* @param lpGUID
*        <FILLMEIN>.
*
* @param lplpDD
*        <FILLMEIN>.
*
* @param pUnkOuter
*        Alwas set to NULL other wise will  DirectDrawCreate fail it return 
*        errror code CLASS_E_NOAGGREGATION
*
* @return  <FILLMEIN>.
*
* @remarks None.
*
*--*/

HRESULT 
WINAPI 
DirectDrawCreate (LPGUID lpGUID, 
                  LPDIRECTDRAW* lplpDD, 
                  LPUNKNOWN pUnkOuter) 
{    
    HRESULT retVal = DDERR_GENERIC;
    /* 
       remove this when UML digram are in place 
       this api is finish and is working as it should
    */

    DX_WINDBG_trace();
     _SEH_TRY
    {
        /* check see if pUnkOuter is null or not */
        if (pUnkOuter)
        {
            retVal = CLASS_E_NOAGGREGATION; 
        }
        else
        {
            retVal = Create_DirectDraw (lpGUID, (LPDIRECTDRAW*)lplpDD, &IID_IDirectDraw, FALSE);
        }
     }
    _SEH_HANDLE
    {
    }
    _SEH_END;

    return retVal;
}

/*++
* @name DirectDrawCreateEx
*
*     The DirectDrawCreateEx routine <FILLMEIN>.
*
* @param lpGUID
*        <FILLMEIN>.
*
* @param lplpDD
*        <FILLMEIN>.
*
* @param pUnkOuter
*        Alwas set to NULL other wise will  DirectDrawCreateEx fail it return 
*        errror code CLASS_E_NOAGGREGATION
*
* @return  <FILLMEIN>.
*
* @remarks None.
*
*--*/
HRESULT 
WINAPI 
DirectDrawCreateEx(LPGUID lpGUID,
                   LPVOID* lplpDD,
                   REFIID id, 
                   LPUNKNOWN pUnkOuter)
{
    HRESULT retVal = DDERR_GENERIC;
    /* 
        remove this when UML digram are in place 
        this api is finish and is working as it should
    */
    DX_WINDBG_trace();

     _SEH_TRY
    {
        /* check see if pUnkOuter is null or not */
        if (pUnkOuter)
        {
            /* we are using same error code as MS*/
            retVal = CLASS_E_NOAGGREGATION; 
        }/* Is it a DirectDraw 7 Request or not */
        else if (!IsEqualGUID(id, &IID_IDirectDraw7)) 
        {
            retVal = DDERR_INVALIDPARAMS;
        }
        else
        {
            retVal = Create_DirectDraw (lpGUID, (LPDIRECTDRAW*)lplpDD, id, FALSE);
        }

        /* Create our DirectDraw interface */
    }
    _SEH_HANDLE
    {
    }
    _SEH_END;

    return retVal;
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

   summuer the msdn 

    The buffer start with D3DHAL_DP2COMMAND struct afer that it follow either one struct or no struct at at all
    example for command D3DDP2OP_VIEWPORTINFO

    then lpCmd will look like this 
    ----------------------------------------
    | struct                 | Pos         |
    ----------------------------------------
    | D3DHAL_DP2COMMAND      | 0x00 - 0x03 |
    ---------------------------------------
    | D3DHAL_DP2VIEWPORTINFO | 0x04 - xxxx |
    ---------------------------------------

    to calc end of the lpCmd buffer in this exmaple 
    D3DHAL_DP2COMMAND->wStateCount * sizeof(D3DHAL_DP2VIEWPORTINFO);
    now you got number of bytes but we need add the size of D3DHAL_DP2COMMAND 
    to get this right. the end should be 
    sizeof(D3DHAL_DP2COMMAND) + ( D3DHAL_DP2COMMAND->wStateCount * sizeof(D3DHAL_DP2VIEWPORTINFO));
    to get the xxxx end positions. 
 */

/*++
* @name D3DParseUnknownCommand
*
*     The D3DParseUnknownCommand routine    <FILLMEIN>.
*
* @param lpCmd
*       Is a typcast to LPD3DHAL_DP2COMMAND struct 
*       typedef struct _D3DHAL_DP2COMMAND
*       {
*           BYTE  bCommand;
*           BYTE  bReserved;
*           union
*           {
*               WORD  wPrimitiveCount;
*               WORD  wStateCount;
*           };
*       } D3DHAL_DP2COMMAND, *LPD3DHAL_DP2COMMAND;
* 
*       lpCmd->bCommand 
*       only accpect D3DDP2OP_VIEWPORTINFO, and undocument command 0x0D
*       rest of the command will be return error code for.
*
        Command 0x0D
*       dp2command->bReserved 
*       is how big struect we got in wStateCount or how many wStateCount we got
*       do not known more about it, no info in msdn about it either.
*
*       Command  D3DDP2OP_VIEWPORTINFO
*        <FILLMEIN>.
*
* @param lpRetCmd
*        <FILLMEIN>.
*
* @return  <FILLMEIN>.
*
* @remarks 
    
*
*--*/

HRESULT WINAPI 
D3DParseUnknownCommand( LPVOID lpCmd, 
                        LPVOID *lpRetCmd)
{
    DWORD retCode = DD_OK;
    LPD3DHAL_DP2COMMAND dp2command = lpCmd;
         
    /* prevent it crash if null pointer are being sent */
    if ( (lpCmd == NULL) || (lpRetCmd == NULL) )
    {
        return E_FAIL;
    }
    
    *lpRetCmd = lpCmd;

    switch (dp2command->bCommand)
    {
       /* check for vaild command, only 3 command is vaild */
       case D3DDP2OP_VIEWPORTINFO:
           *(PBYTE)lpRetCmd += ((dp2command->wStateCount * sizeof(D3DHAL_DP2VIEWPORTINFO)) + sizeof(D3DHAL_DP2COMMAND));
           break;

       case D3DDP2OP_WINFO:
           *(PBYTE)lpRetCmd += (dp2command->wStateCount * sizeof(D3DHAL_DP2WINFO)) + sizeof(D3DHAL_DP2COMMAND);
           break;

       case 0x0d: /* Undocumented in MSDN */
           *(PBYTE)lpRetCmd += ((dp2command->wStateCount * dp2command->bReserved) + sizeof(D3DHAL_DP2COMMAND));
           break;

       
       /* set the error code */
       default:
               
           if ( (dp2command->bCommand <= D3DDP2OP_INDEXEDTRIANGLELIST) || // dp2command->bCommand  <= with 0 to 3
              (dp2command->bCommand == D3DDP2OP_RENDERSTATE) ||  // dp2command->bCommand  == with 8
              (dp2command->bCommand >= D3DDP2OP_LINELIST) )  // dp2command->bCommand  >= with 15 to 255
           {
               /* set error code for command 0 to 3, 8 and 15 to 255 */
               retCode = E_FAIL; 
           }
           else
           {   /* set error code for 4 - 7, 9 - 12, 14  */
               retCode = D3DERR_COMMAND_UNPARSED; 
           }
            
    }

    return retCode;
}


VOID 
WINAPI 
AcquireDDThreadLock()
{
   EnterCriticalSection(&ddcs);
}

VOID 
WINAPI  
ReleaseDDThreadLock()
{
   LeaveCriticalSection(&ddcs);
}

BOOL APIENTRY 
DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{
  BOOL retStatus;
  switch(ul_reason_for_call)
  {
     case DLL_PROCESS_DETACH:
           //DeleteCriticalSection( &ddcs );
           retStatus = TRUE;
           break;

     case DLL_PROCESS_ATTACH:
        //DisableThreadLibraryCalls( hModule );
        //InitializeCriticalSection( &ddcs );
        //EnterCriticalSection( &ddcs );
        //LeaveCriticalSection( &ddcs ); 
        retStatus = FALSE;
        break;

    default:
        retStatus = TRUE;
         break;
  }
  return retStatus;

}
