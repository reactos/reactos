/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMMSTRU2.C
 *  WOW32 16-bit MultiMedia structure conversion support
 *               Contains support for mciSendCommand UnThunk message Parms.
 *
 *  History:
 *  Created  17-Jul-1992 by Stephen Estrop (stephene)
 *
--*/

//
// We define NO_STRICT so that the compiler doesn't moan and groan when
// I use the FARPROC type for the Multi-Media api loading.
//
#define NO_STRICT

#include "precomp.h"
#pragma hdrstop

#if 0

MODNAME(wmmstru2.c);

//
// The following are required for the dynamic linking of Multi-Media code
// from within WOW.  They are all defined in wmmedia.c
//

extern FARPROC      mmAPIEatCmdEntry;
extern FARPROC      mmAPIGetParamSize;
extern FARPROC      mmAPIUnlockCmdTable;
extern FARPROC      mmAPISendCmdW;

/**********************************************************************\
*
* UnThunkMciCommand16
*
* This function "unthunks" a 32 bit mci send command request.
*
* The ideas behind this function were stolen from UnThunkWMMsg16,
* see wmsg16.c
*
\**********************************************************************/
INT UnThunkMciCommand16( MCIDEVICEID devID, UINT OrigCommand, DWORD OrigFlags,
                         DWORD OrigParms, DWORD NewParms, LPWSTR lpCommand,
                         UINT uTable )
{
    BOOL        fReturnValNotThunked = FALSE;

#if DBG
    static      LPSTR   f_name = "UnThunkMciCommand16: ";
    register    int     i;
                int     n;

    dprintf3(( "UnThunkMciCommand :" ));
    n = sizeof(mciMessageNames) / sizeof(MCI_MESSAGE_NAMES);
    for ( i = 0; i < n; i++ ) {
        if ( mciMessageNames[i].uMsg == OrigCommand ) {
            break;
        }
    }
    dprintf3(( "OrigCommand -> %lX", (DWORD)OrigCommand ));
    dprintf3(( "       Name -> %s", i != n ? mciMessageNames[i].lpstMsgName : "Unkown Name" ));

    dprintf5(( "  OrigFlags -> %lX", OrigFlags ));
    dprintf5(( "  OrigParms -> %lX", OrigParms ));
    dprintf5(( "   NewParms -> %lX", NewParms ));

    //
    // If NewParms is 0 we shouldn't be here, I haven't got an assert
    // macro, but the following we do the same thing.
    //
    if ( NewParms == 0 ) {
        dprintf(( "%scalled with NewParms == NULL !!", f_name ));
        dprintf(( "Call StephenE NOW !!" ));
        DebugBreak();
    }
#endif

    //
    // We have to do a manual unthunk of MCI_SYSINFO because the
    // command table is not consistent.  As a command table should be
    // available now we can load it and then use it to unthunk MCI_OPEN.
    //
    switch ( OrigCommand ) {

    case MCI_OPEN:
        UnThunkOpenCmd( OrigFlags, OrigParms, NewParms );
        break;

    case MCI_SYSINFO:
        UnThunkSysInfoCmd( OrigFlags, OrigParms, NewParms );
        break;

    case MCI_STATUS:
        UnThunkStatusCmd( devID, OrigFlags, OrigParms, NewParms );
        break;

    default:
        fReturnValNotThunked = TRUE;
        break;
    }

    //
    // Do we have a command table ?  It is possible that we have
    // a custom command but we did not find a custom command table, in which
    // case we should just free the pNewParms storage.
    //
    if ( lpCommand != NULL ) {

        //
        // We now parse the custom command table to see if there is a
        // return field in the parms structure.
        //
        dprintf3(( "%sUnthunking via command table", f_name ));
        UnThunkCommandViaTable( lpCommand, OrigFlags, OrigParms,
                                NewParms, fReturnValNotThunked );

        //
        // Now we have finished with the command table we should unlock it.
        //
        dprintf4(( "%sUnlocking custom command table", f_name ));
        (*mmAPIUnlockCmdTable)( uTable );
    }

    //
    // All that needs to be done now is to free the storage
    // that was allocated during the ThunkXxxCmd function.
    //
    dprintf4(( "%sFreeing storage.", f_name ));
    free_w( (PBYTE)NewParms );
    return 0;
}


/**********************************************************************\
* UnThunkOpenCmd
*
* UnThunk the Open mci command parms.
\**********************************************************************/
VOID UnThunkOpenCmd( DWORD OrigFlags, DWORD OrigParms, DWORD NewParms )
{

#if DBG
    static  LPSTR   f_name = "UnThunkOpenCmd: ";
#endif

    LPMCI_OPEN_PARMS     lpOpeParms = (LPMCI_OPEN_PARMS)NewParms;
    PMCI_OPEN_PARMS16    lpOpeParms16;
    WORD                 wDevice;

    dprintf4(( "%sCopying Device ID.", f_name ));

    GETVDMPTR( OrigParms, sizeof(MCI_OPEN_PARMS16), lpOpeParms16 );
    wDevice = LOWORD( lpOpeParms->wDeviceID );
    STOREWORD( lpOpeParms16->wDeviceID, wDevice );
    FLUSHVDMPTR( OrigParms, sizeof(MCI_OPEN_PARMS16), lpOpeParms16 );
    FREEVDMPTR( lpOpeParms16 );

    dprintf5(( "wDeviceID -> %u", wDevice ));

    if ( (OrigParms & MCI_OPEN_TYPE) && !(OrigParms & MCI_OPEN_TYPE_ID ) ) {

        dprintf3(( "%sFreeing a STRING pointer", f_name ));
        FREEPSZPTR( lpOpeParms->lpstrDeviceType );
    }

    if ( (OrigParms & MCI_OPEN_ELEMENT)
     && !(OrigParms & MCI_OPEN_ELEMENT_ID ) ) {

        dprintf3(( "%sFreeing a STRING pointer", f_name ));
        FREEPSZPTR( lpOpeParms->lpstrElementName );
    }
}


/**********************************************************************\
* UnThunkSysInfoCmd
*
* UnThunk the SysInfo mci command parms.
\**********************************************************************/
VOID UnThunkSysInfoCmd( DWORD OrigFlags, DWORD OrigParms, DWORD NewParms )
{

#if DBG
    static  LPSTR   f_name = "UnThunkSysInfoCmd: ";
#endif

    LPMCI_SYSINFO_PARMS     lpSysParms = (LPMCI_SYSINFO_PARMS)NewParms;

    //
    // Had better check that we did actually allocate
    // a pointer.
    //
    if ( lpSysParms->lpstrReturn && lpSysParms->dwRetSize ) {

#if DBG
        if ( !(OrigFlags & MCI_SYSINFO_QUANTITY) ) {
            dprintf5(( "lpstrReturn -> %s", lpSysParms->lpstrReturn ));
        }
        else {
            dprintf5(( "lpstrReturn -> %d", *(LPDWORD)lpSysParms->lpstrReturn ));
        }
#endif

        //
        // Free lpSysParms->lpstrReturn;
        //
        dprintf4(( "%sFreeing lpstrReturn", f_name ));
        FREEVDMPTR( lpSysParms->lpstrReturn );
    }
}


/**********************************************************************\
* UnThunkMciStatus
*
* UnThunk the Status mci command parms.
\**********************************************************************/
VOID UnThunkStatusCmd( MCIDEVICEID devID, DWORD OrigFlags,
                       DWORD OrigParms, DWORD NewParms )
{
#if DBG
    static  LPSTR   f_name = "UnThunkStatusCmd: ";
#endif

    MCI_GETDEVCAPS_PARMS        GetDevCaps;
    DWORD                       dwRetVal;
    DWORD                       dwParm16;
    PDWORD                      pdwOrig16;
    PDWORD                      pdwParm32;
    int                         iReturnType = MCI_INTEGER;

    /*
    ** If the MCI_STATUS_ITEM flag is not specified don't bother
    ** doing any unthunking.
    */
    if ( !(OrigFlags & MCI_STATUS_ITEM) ) {
        return;
    }

    /*
    ** We need to determine what type of device we are
    ** dealing with.  We can do this by send an MCI_GETDEVCAPS
    ** command to the device. (We might as well use the Unicode
    ** version of mciSendCommand and avoid another thunk).
    */
    RtlZeroMemory( &GetDevCaps, sizeof(MCI_GETDEVCAPS_PARMS) );
    GetDevCaps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
    dwRetVal = (*mmAPISendCmdW)( devID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM,
                                 (DWORD)&GetDevCaps );
    /*
    ** If we can't get the DevCaps then we are doomed.
    */
    if ( dwRetVal ) {
        dprintf(("%sFailure to get devcaps", f_name));
        return;
    }

    /*
    ** Determine the dwReturn type.
    */
    switch ( GetDevCaps.dwReturn ) {

    case MCI_DEVTYPE_ANIMATION:
        switch ( ((LPDWORD)NewParms)[2] ) {

        case MCI_ANIM_STATUS_HWND:
            iReturnType = MCI_HWND;
            break;

        case MCI_ANIM_STATUS_HPAL:
            iReturnType = MCI_HPAL;
            break;
        }
        break;

    case MCI_DEVTYPE_OVERLAY:
        if ( ((LPDWORD)NewParms)[2] == MCI_OVLY_STATUS_HWND ) {
            iReturnType = MCI_HWND;
        }
        break;

    case MCI_DEVTYPE_DIGITAL_VIDEO:
        switch ( ((LPDWORD)NewParms)[2] ) {

        case MCI_DGV_STATUS_HWND:
            iReturnType = MCI_HWND;
            break;

        case MCI_DGV_STATUS_HPAL:
            iReturnType = MCI_HPAL;
            break;
        }
        break;
    }


    /*
    ** Thunk the dwReturn value according to the required type
    */
    GETVDMPTR( OrigParms, sizeof( MCI_STATUS_PARMS), pdwOrig16 );
    pdwParm32 = (LPDWORD)((LPBYTE)NewParms + 4);

    switch ( iReturnType ) {
    case MCI_HPAL:
        dprintf4(( "%sFound an HPAL return field", f_name ));
        dwParm16 = MAKELONG( GETHPALETTE16( (HPALETTE)*pdwParm32 ), 0 );
        STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), dwParm16 );
        dprintf5(( "HDC32 -> 0x%lX", *pdwParm32 ));
        dprintf5(( "HDC16 -> 0x%lX", dwParm16 ));
        break;

    case MCI_HWND:
        dprintf4(( "%sFound an HWND return field", f_name ));
        dwParm16 = MAKELONG( GETHWND16( (HWND)*pdwParm32 ), 0 );
        STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), dwParm16 );
        dprintf5(( "HWND32 -> 0x%lX", *pdwParm32 ));
        dprintf5(( "HWND16 -> 0x%lX", dwParm16 ));
        break;

    case MCI_INTEGER:
        dprintf4(( "%sFound an INTEGER return field", f_name ));
        STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), *pdwParm32 );
        dprintf5(( "INTEGER -> %ld", *pdwParm32 ));
        break;

    // no default: all possible cases accounted for
    }

    /*
    ** Free the VDM pointer as we have finished with it
    */
    FLUSHVDMPTR( OrigParms, sizeof( MCI_STATUS_PARMS), pdwOrig16 );
    FREEVDMPTR( pdwOrig16 );

}
/**********************************************************************\
*  UnThunkCommandViaTable
*
* Thunks the return field if there is one and then frees and pointers
* that were got via GETVDMPTR or GETPSZPTR.
\**********************************************************************/
INT UnThunkCommandViaTable( LPWSTR lpCommand, DWORD dwFlags, DWORD OrigParms,
                            DWORD pNewParms, BOOL fReturnValNotThunked )
{

#if DBG
    static  LPSTR   f_name = "UnThunkCommandViaTable: ";
#endif

    LPWSTR  lpFirstParameter;

    UINT    wID;
    DWORD   dwValue;

    UINT    wOffset32, wOffset1stParm32;

    DWORD   dwParm16;
    DWORD   Size;
    PDWORD  pdwOrig16;
    PDWORD  pdwParm32;

    DWORD   dwMask = 1;

    //
    // Calculate the size of this command parameter block in terms
    // of bytes, then get a VDM pointer to the OrigParms.
    //
    Size = GetSizeOfParameter( lpCommand );

    //
    // Skip past command entry
    //
    lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                    (*mmAPIEatCmdEntry)( lpCommand, NULL, NULL ));
    //
    // Get the next entry
    //
    lpFirstParameter = lpCommand;

    //
    // Skip past the DWORD return value
    //
    wOffset1stParm32 = 4;

    lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                    (*mmAPIEatCmdEntry)( lpCommand, &dwValue, &wID ));
    //
    // If it is a return value, skip it
    //
    if ( (wID == MCI_RETURN) && (fReturnValNotThunked) ) {

        GETVDMPTR( OrigParms, Size, pdwOrig16 );
        pdwParm32 = (LPDWORD)((LPBYTE)pNewParms + 4);

        //
        // Look for a string return type, these are a special case.
        //
        switch ( dwValue ) {

        case MCI_STRING:
            dprintf4(( "%sFound a STRING return field", f_name ));
            //
            // Get string pointer and length
            //
            Size = *(LPDWORD)((LPBYTE)pNewParms + 8);

            //
            // Get the 32 bit string pointer
            //
            if ( Size > 0 ) {

                dprintf4(( "%sFreeing a return STRING pointer", f_name ));
                dprintf5(( "STRING -> %s", (LPSTR)*pdwParm32 ));
                FREEVDMPTR( (LPSTR)*pdwParm32 );
            }
            break;

        case MCI_RECT:
            {
                PRECT   pRect32 = (PRECT)((LPBYTE)pNewParms + 4);
                PRECT16 pRect16 = (PRECT16)((LPBYTE)pdwOrig16 + 4);

                dprintf4(( "%sFound a RECT return field", f_name ));
                STORESHORT( pRect16->top,    (SHORT)pRect32->top );
                STORESHORT( pRect16->bottom, (SHORT)pRect32->bottom );
                STORESHORT( pRect16->left,   (SHORT)pRect32->left );
                STORESHORT( pRect16->right,  (SHORT)pRect32->right );
            }
            break;

        case MCI_INTEGER:
            //
            // Get the 32 bit return integer and store it in the
            // 16 bit parameter structure.
            //
            dprintf4(( "%sFound an INTEGER return field", f_name ));
            STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), *pdwParm32 );
            dprintf5(( "INTEGER -> %ld", *pdwParm32 ));
            break;

        case MCI_HWND:
            dprintf4(( "%sFound an HWND return field", f_name ));
            dwParm16 = MAKELONG( GETHWND16( (HWND)*pdwParm32 ), 0 );
            STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), dwParm16 );
            dprintf5(( "HWND32 -> 0x%lX", *pdwParm32 ));
            dprintf5(( "HWND16 -> 0x%lX", dwParm16 ));
            break;

        case MCI_HPAL:
            dprintf4(( "%sFound an HPAL return field", f_name ));
            dwParm16 = MAKELONG( GETHPALETTE16( (HPALETTE)*pdwParm32 ), 0 );
            STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), dwParm16 );
            dprintf5(( "HDC32 -> 0x%lX", *pdwParm32 ));
            dprintf5(( "HDC16 -> 0x%lX", dwParm16 ));
            break;

        case MCI_HDC:
            dprintf4(( "%sFound an HDC return field", f_name ));
            dwParm16 = MAKELONG( GETHDC16( (HDC)*pdwParm32 ), 0 );
            STOREDWORD( *(LPDWORD)((LPBYTE)pdwOrig16 + 4), dwParm16 );
            dprintf5(( "HDC32 -> 0x%lX", *pdwParm32 ));
            dprintf5(( "HDC16 -> 0x%lX", dwParm16 ));
            break;
        }

        //
        // Free the VDM pointer as we have finished with it
        //
        FLUSHVDMPTR( OrigParms, Size, pdwOrig16 );
        FREEVDMPTR( pdwOrig16 );

        //
        // Adjust the offset of the first parameter.
        //
        wOffset1stParm32 = (*mmAPIGetParamSize)( dwValue, wID );

        //
        // Save the new first parameter
        //
        lpFirstParameter = lpCommand;
    }

    //
    // Walk through each flag looking for strings to free
    //
    while ( dwMask != 0 ) {

        //
        // Is this bit set?
        //
        if ( (dwFlags & dwMask) != 0 ) {

            wOffset32 = wOffset1stParm32;
            lpCommand = (LPWSTR)((LPBYTE)lpFirstParameter +
                            (*mmAPIEatCmdEntry)( lpFirstParameter,
                                                &dwValue, &wID ));

            //
            // What parameter uses this bit?
            //
            while ( wID != MCI_END_COMMAND && dwValue != dwMask ) {

                wOffset32 = (*mmAPIGetParamSize)( dwValue, wID );

                if ( wID == MCI_CONSTANT ) {

                    while ( wID != MCI_END_CONSTANT ) {

                        lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                                (*mmAPIEatCmdEntry)( lpCommand, NULL, &wID ));
                    }
                }
                lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                             (*mmAPIEatCmdEntry)( lpCommand, &dwValue, &wID ));
            }

            if ( wID == MCI_STRING ) {
                dprintf4(( "%sFreeing a STRING pointer", f_name ));
                pdwParm32 = (LPDWORD)((LPBYTE)pNewParms + wOffset32);
                FREEPSZPTR( (LPSTR)*pdwParm32 );
            }
        }

        //
        // Go to the next flag
        //
        dwMask <<= 1;
    }

    return 0;
}
#endif
