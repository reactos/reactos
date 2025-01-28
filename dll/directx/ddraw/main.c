/*
 * COPYRIGHT:            See COPYING in the top level directory
 * PROJECT:              ReactOS kernel
 * FILE:                 dll/directx/ddraw/main.c
 * PURPOSE:              DirectDraw Library
 * PROGRAMMER:           Magnus Olsen (greatlrd)
 *
 */


#include "rosdraw.h"
HMODULE hDllModule = 0;

CRITICAL_SECTION ddcs;

// This function is exported by the dll

typedef struct
{
    LPVOID lpCallback;
    LPVOID lpContext;
} DirectDrawEnumerateProcData;

BOOL
CALLBACK
TranslateCallbackA(GUID *lpGUID,
                   LPSTR lpDriverDescription,
                   LPSTR lpDriverName,
                   LPVOID lpContext,
                   HMONITOR hm)
{
        DirectDrawEnumerateProcData *pEPD = (DirectDrawEnumerateProcData*)lpContext;
        return ((LPDDENUMCALLBACKA) pEPD->lpCallback)(lpGUID, lpDriverDescription, lpDriverName, pEPD->lpContext);
}

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
*        Always set to NULL otherwise DirectDrawCreate will fail and return
*        error code CLASS_E_NOAGGREGATION
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
       remove this when UML diagram is in place
       this api is finished and is working as it should
    */

    DX_WINDBG_trace();
     _SEH2_TRY
    {
        /* check if pUnkOuter is null or not */
        if (pUnkOuter)
        {
            retVal = CLASS_E_NOAGGREGATION;
        }
        else
        {
            retVal = Create_DirectDraw (lpGUID, (LPDIRECTDRAW*)lplpDD, &IID_IDirectDraw, FALSE);
        }
     }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

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
*        Always set to NULL otherwise DirectDrawCreateEx will fail and return
*        error code CLASS_E_NOAGGREGATION
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
        remove this when UML diagram is in place
        this api is finished and is working as it should
    */
    DX_WINDBG_trace();

     _SEH2_TRY
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
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    _SEH2_END;

    return retVal;
}

HRESULT
WINAPI
DirectDrawEnumerateA( LPDDENUMCALLBACKA lpCallback,
                     LPVOID lpContext)
{
     HRESULT retValue;
     DirectDrawEnumerateProcData epd;

     DX_WINDBG_trace();

     epd.lpCallback = (LPVOID) lpCallback;
     epd.lpContext = lpContext;

     if (!IsBadCodePtr((LPVOID)lpCallback))
     {
         return DirectDrawEnumerateExA((LPDDENUMCALLBACKEXA)TranslateCallbackA, &epd, DDENUM_NONDISPLAYDEVICES);
     }
     else
     {
         retValue = DDERR_INVALIDPARAMS;
     }
     return retValue;
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
    HKEY hKey;
    DWORD cbData = 0;
    DWORD Value = 0;
    LONG rc;
    BOOL  EnumerateAttachedSecondaries = FALSE;
    DWORD privateDWFlags = 0;
    CHAR strMsg[RC_STRING_MAX_SIZE];
    HRESULT retVal = DDERR_INVALIDPARAMS;

    DX_WINDBG_trace();

    if ((IsBadCodePtr((LPVOID)lpCallback) == 0) &&
       ((dwFlags & ~(DDENUM_NONDISPLAYDEVICES |
                    DDENUM_DETACHEDSECONDARYDEVICES |
                    DDENUM_ATTACHEDSECONDARYDEVICES)) == 0))
    {
        LoadStringA(hDllModule, STR_PRIMARY_DISPLAY, (LPSTR)&strMsg, RC_STRING_MAX_SIZE);

        rc = RegOpenKeyA(HKEY_LOCAL_MACHINE, REGSTR_PATH_DDHW, &hKey);
        if (rc == ERROR_SUCCESS)
        {
            /* Enumerate Attached Secondaries */
            cbData = sizeof(DWORD);
            rc = RegQueryValueExA(hKey, "EnumerateAttachedSecondaries", NULL, NULL, (LPBYTE)&Value, &cbData);
            if (rc == ERROR_SUCCESS)
            {
                if (Value != 0)
                {
                    EnumerateAttachedSecondaries = TRUE;
                    privateDWFlags = DDENUM_ATTACHEDSECONDARYDEVICES;
                }
            }
            RegCloseKey(hKey);
        }

        /* Call the user supplied callback function */
        rc = lpCallback(NULL, strMsg, "display", lpContext, NULL);

        /* If the callback function returns DDENUMRET_CANCEL, we will stop enumerating devices */
        if(rc == DDENUMRET_CANCEL)
        {
            retVal = DD_OK;
        }
        else
        {
            // not finished
            retVal = DDERR_UNSUPPORTED;
        }
    }

    return retVal;
}

HRESULT
WINAPI
DirectDrawEnumerateW(LPDDENUMCALLBACKW lpCallback,
                                    LPVOID lpContext)
{
    DX_WINDBG_trace();

    return DDERR_UNSUPPORTED;
}

HRESULT
WINAPI
DirectDrawEnumerateExW(LPDDENUMCALLBACKEXW lpCallback,
                       LPVOID lpContext,
                       DWORD dwFlags)
{
    DX_WINDBG_trace();

    return DDERR_UNSUPPORTED;
}




/*
   See http://msdn.microsoft.com/library/default.asp?url=/library/en-us/Display_d/hh/Display_d/d3d_21ac30ea-9803-401e-b541-6b08af79653d.xml.asp (DEAD_LINK)

   for more info about this command see msdn documentation

    The buffer start with D3DHAL_DP2COMMAND struct afer that follows either one struct or
    no struct at at all
    example for command D3DDP2OP_VIEWPORTINFO

    then lpCmd will look like this
    ----------------------------------------
    | struct                 | Pos         |
    ----------------------------------------
    | D3DHAL_DP2COMMAND      | 0x00 - 0x03 |
    ---------------------------------------
    | D3DHAL_DP2VIEWPORTINFO | 0x04 - xxxx |
    ---------------------------------------

    to calculate the end of the lpCmd buffer in this example
    D3DHAL_DP2COMMAND->wStateCount * sizeof(D3DHAL_DP2VIEWPORTINFO);
    now you got number of bytes but we need to add the size of D3DHAL_DP2COMMAND
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
*       Is a typecast to LPD3DHAL_DP2COMMAND struct
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
*       only accept D3DDP2OP_VIEWPORTINFO, and undocumented command 0x0D
*       anything else will result in an error
*
        Command 0x0D
*       dp2command->bReserved
*       is the size of the struct we got in wStateCount or how many wStateCount we got
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

    DX_WINDBG_trace();

    /* prevent it crash if null pointer are being sent */
    if ( (lpCmd == NULL) || (lpRetCmd == NULL) )
    {
        return E_FAIL;
    }

    *lpRetCmd = lpCmd;

    switch (dp2command->bCommand)
    {
       /* check for valid command, only 3 commands are valid */
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
               retCode = DDERR_INVALIDPARAMS;
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
    DX_WINDBG_trace();

    EnterCriticalSection(&ddcs);
}

VOID
WINAPI
ReleaseDDThreadLock()
{
    DX_WINDBG_trace();

    LeaveCriticalSection(&ddcs);
}


BOOL APIENTRY
DllMain( HMODULE hModule, DWORD  ul_reason_for_call, LPVOID lpReserved )
{

    hDllModule = hModule;

    DX_WINDBG_trace();


  switch(ul_reason_for_call)
  {
     case DLL_PROCESS_DETACH:
           DeleteCriticalSection( &ddcs );
           break;

     case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls( hModule );
        InitializeCriticalSection( &ddcs );
        EnterCriticalSection( &ddcs );
        LeaveCriticalSection( &ddcs );
        break;

    default:
         break;
  }

  return TRUE;
}
