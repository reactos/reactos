/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMMSTRU1.C
 *  WOW32 16-bit MultiMedia structure conversion support
 *               Contains support for mciSendCommand Thunk message Parms.
 *               Also contains some debug support functions.
 *
 *  History:
 *  Created  14-Jul-1992 by Stephen Estrop (stephene)
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

MODNAME(wmmstru1.c);

#if DBG
int mmDebugLevel = -1;

MCI_MESSAGE_NAMES  mciMessageNames[32] = {
    { MCI_OPEN,         "MCI_OPEN" },
    { MCI_CLOSE,        "MCI_CLOSE" },
    { MCI_ESCAPE,       "MCI_ESCAPE" },
    { MCI_PLAY,         "MCI_PLAY" },
    { MCI_SEEK,         "MCI_SEEK" },
    { MCI_STOP,         "MCI_STOP" },
    { MCI_PAUSE,        "MCI_PAUSE" },
    { MCI_INFO,         "MCI_INFO" },
    { MCI_GETDEVCAPS,   "MCI_GETDEVCAPS" },
    { MCI_SPIN,         "MCI_SPIN" },
    { MCI_SET,          "MCI_SET" },
    { MCI_STEP,         "MCI_STEP" },
    { MCI_RECORD,       "MCI_RECORD" },
    { MCI_SYSINFO,      "MCI_SYSINFO" },
    { MCI_BREAK,        "MCI_BREAK" },
    { MCI_SOUND,        "MCI_SOUND" },
    { MCI_SAVE,         "MCI_SAVE" },
    { MCI_STATUS,       "MCI_STATUS" },
    { MCI_CUE,          "MCI_CUE" },
    { MCI_REALIZE,      "MCI_REALIZE" },
    { MCI_WINDOW,       "MCI_WINDOW" },
    { MCI_PUT,          "MCI_PUT" },
    { MCI_WHERE,        "MCI_WHERE" },
    { MCI_FREEZE,       "MCI_FREEZE" },
    { MCI_UNFREEZE,     "MCI_UNFREEZE" },
    { MCI_LOAD,         "MCI_LOAD" },
    { MCI_CUT,          "MCI_CUT" },
    { MCI_COPY,         "MCI_COPY" },
    { MCI_PASTE,        "MCI_PASTE" },
    { MCI_UPDATE,       "MCI_UPDATE" },
    { MCI_RESUME,       "MCI_RESUME" },
    { MCI_DELETE,       "MCI_DELETE" }
};
#endif

//
// The following are required for the dynamic linking of Multi-Media code
// from within WOW.  They are all defined in wmmedia.c
//

extern FARPROC      mmAPIEatCmdEntry;
extern FARPROC      mmAPIGetParamSize;
extern FARPROC      mmAPISendCmdW;
extern FINDCMDITEM  mmAPIFindCmdItem;

/**********************************************************************\
*
* ThunkMciCommand16
*
* This function converts a 16 bit mci command request into an
* equiverlant 32 bit request.
*
* The ideas behind this function were stolen from ThunkWMMsg16,
* see wmsg16.c and mciDebugOut see mci.c
*
* We return 0 if the thunk was OK, any other value should be used as
* an error code.  If the thunk failed all allocated resources will
* be freed by this function.  If the thunk was sucessful (ie. returned 0)
* UnThunkMciCommand16 MUST be called to free allocated resources.
*
* Here are the assumptions that I have used to perform the thunking:
*
* 1. MCI_OPEN is a special case.
*
* 2. If the message is NOT defined in mmsystem.h then it is treated as a
*    "user" command.  If a user command table is associated with the given
*    device ID we use this command table as an aid to perform the thunking.
*    If a user command table is NOT associated with the device ID the
*    command does NOT GET THUNKED, we return straight away, calling
*    mciSendCommand only to get a relevant error code.
*
* 3. If the command IS defined in mmsystem.h we perfrom a "manual" thunk
*    of the command IF the associated PARMS structure contains ReservedX
*    fields.  We mask out the associated flags as each field is thunked.
*
* 4. If there are any flags left then we use the command table
*    as an aid to perform the thunking.
*
\**********************************************************************/
INT ThunkMciCommand16( MCIDEVICEID DeviceID, UINT OrigCommand, DWORD OrigFlags,
                       DWORD OrigParms, PDWORD pNewParms, LPWSTR *lplpCommand,
                       PUINT puTable )
{


#if DBG
    register    int             i;
                int             n;

    dprintf3(( "ThunkMciCommand16 :" ));
    dprintf5(( " OrigDevice -> %lX", DeviceID ));

    n = sizeof(mciMessageNames) / sizeof(MCI_MESSAGE_NAMES);
    for ( i = 0; i < n; i++ ) {
        if ( mciMessageNames[i].uMsg == OrigCommand ) {
            break;
        }
    }
    dprintf3(( "OrigCommand  -> 0x%lX", (DWORD)OrigCommand ));

    //
    // Special case MCI_STATUS.  I get loads of these from mplayer.
    // I only want to display MCI_STATUS messages if the debug level is
    // set to level 3, that way I won't get swamped with them.
    //
    if ( mciMessageNames[i].uMsg != MCI_STATUS ) {
        if ( i != n ) {
            dprintf2(( "Command Name -> %s", mciMessageNames[i].lpstMsgName ));
        }
        else {
            dprintf2(( "Command Name -> UNKNOWN COMMAND (%x)", OrigCommand ));
        }
    }
    else {
        dprintf3(( "Command Name -> MCI_STATUS" ));
    }

    dprintf5(( "OrigFlags    -> 0x%lX", OrigFlags ));
    dprintf5(( "OrigParms    -> 0x%lX", OrigParms ));
#endif

    //
    // Get some storage for the Mci parameter block, and handle the
    // notify window handle (if supplied).
    //

    if ( (*pNewParms = AllocMciParmBlock( &OrigFlags, OrigParms )) == 0L ) {
        return MCIERR_OUT_OF_MEMORY;
    }

    //
    // We thunk the MCI_OPEN command and all other commands that contain a
    // "ReservedX" field in their PARMS structure here.  We mask out each
    // flag as it is processed, if any flags are left we use the command
    // table to complete the thunk.
    //
    // The following commands have ReservedX fields:
    //      MCI_WINDOW
    //      MCI_SET
    //
    // This means that MOST COMMANDS GET THUNKED VIA THE COMMAND TABLE.
    //
    switch ( OrigCommand ) {

        case MCI_OPEN:
            //
            // MCI_OPEN is a special case message that I don't
            // how to deal with yet.
            //
            ThunkOpenCmd( &OrigFlags, OrigParms, *pNewParms );
            return 0;

            //
            // The next four commands have Reserved padding fields
            // these have to thunked manually.
            //

        case MCI_SET:
            ThunkSetCmd( DeviceID, &OrigFlags, OrigParms, *pNewParms );
            break;

        case MCI_WINDOW:
            ThunkWindowCmd( DeviceID, &OrigFlags, OrigParms, *pNewParms );
            break;

            //
            // Have to special case this command because the command table
            // is not correct.
            //
        case MCI_SETVIDEO:
            ThunkSetVideoCmd( DeviceID, &OrigFlags, OrigParms, *pNewParms );
            break;

            //
            // These two commands don't have any command extensions
            // so we return immediately.
            //
        case MCI_SYSINFO:
            ThunkSysInfoCmd( &OrigFlags, OrigParms, *pNewParms );
            return 0;

        case MCI_BREAK:
            ThunkBreakCmd( &OrigFlags, OrigParms, *pNewParms );
            return 0;
    }

    //
    // Find the command table for the given command ID.
    // We always load the command table this is because the command table is
    // needed for UnThunking.
    //
    *lplpCommand = (*mmAPIFindCmdItem)( DeviceID, NULL, (LPWSTR)OrigCommand,
                                        NULL, puTable );
    //
    // If the command table is not found we return straight away.
    // Note that storage has been allocated for pNewParms and that the
    // MCI_WAIT and MCI_NOTIFY flags have been thunked.
    // We do not return an error here, but call mciSendCommand to
    // let it determine a suitable error code, we must also call
    // UnthunkMciCommand to free the allocated storage.
    //
    if ( *lplpCommand == NULL ) {
        dprintf(( "Command table not found !!" ));
        return 0;
    }
    dprintf4(( "Command table has been loaded -> 0x%lX", *lplpCommand ));

    //
    // If OrigFlags is not equal to 0 we still have work to do !
    // Note that this will be true for the majority of cases.
    //
    if ( OrigFlags ) {

        dprintf3(( "Thunking via command table" ));

        //
        // Now we thunk the command
        //
        return ThunkCommandViaTable( *lplpCommand, OrigFlags, OrigParms,
                                     *pNewParms );
    }

    return 0;

}

/**********************************************************************\
* AllocMciParmBlock
*
* Get some storage for the Mci parameter block.  I always allocate
* MCI_MAX_PARAM_SLOTS * DWORD amount as this allows for any command
* extensions.
*
* As we know that the first dword field is a Window handle
* this field is taken care of here.  Also the MCI_WAIT flag is
* masked out if it is set.
*
\**********************************************************************/
DWORD AllocMciParmBlock( PDWORD pOrigFlags, DWORD OrigParms )
{

    LPMCI_GENERIC_PARMS     lpGenParms;
    PMCI_GENERIC_PARMS16    lpGenParmsOrig;

    UINT                    AllocSize = sizeof(DWORD) * MCI_MAX_PARAM_SLOTS;

    //
    // Get, check and set the required storage.
    //
    lpGenParms = (LPMCI_GENERIC_PARMS)malloc_w( AllocSize );
    if ( lpGenParms == NULL ) {
        return 0L;
    }
    RtlZeroMemory( lpGenParms, AllocSize );
    dprintf4(( "AllocMciParmBlock: Allocated storage -> 0x%lX", lpGenParms ));

    //
    // Look for the notify flag and thunk accordingly
    //
    if ( *pOrigFlags & MCI_NOTIFY ) {


        GETVDMPTR( OrigParms, sizeof(MCI_GENERIC_PARMS16), lpGenParmsOrig );

        dprintf4(( "AllocMciParmBlock: Got MCI_NOTIFY flag." ));

        // Note FETCHWORD of a DWORD below, same as LOWORD(FETCHDWORD(dw)),
        // only faster.
        lpGenParms->dwCallback =
            (DWORD)HWND32( FETCHWORD( lpGenParmsOrig->dwCallback ) );

        FREEVDMPTR( lpGenParmsOrig );

        *pOrigFlags ^= MCI_NOTIFY;
    }

    //
    // If the MCI_WAIT flag is present, mask it out.
    //
    if ( *pOrigFlags & MCI_WAIT ) {
        dprintf4(( "AllocMciParmBlock: Got MCI_WAIT flag." ));
        *pOrigFlags ^= MCI_WAIT;
    }

    return (DWORD)lpGenParms;
}

/**********************************************************************\
* ThunkOpenCmd
*
* Thunk the Open mci command parms.
\**********************************************************************/
DWORD ThunkOpenCmd( PDWORD pOrigFlags, DWORD OrigParms, DWORD pNewParms )
{
    //
    // The purpose of this union is to aid the creation of a 32 bit
    // Open Parms structure that is suitable for all known MCI devices.
    //
    typedef union  {
        MCI_OPEN_PARMS          OpenParms;
        MCI_WAVE_OPEN_PARMS     OpenWaveParms;
        MCI_ANIM_OPEN_PARMS     OpenAnimParms;  // Note: Animation and
        MCI_OVLY_OPEN_PARMS     OpenOvlyParms;  // overlay parms are identical
    } MCI_ALL_OPEN_PARMS, *PMCI_ALL_OPEN_PARMS;

    //
    // The following pointers will be used to point to
    // the original 16-bit Parms structure.
    //
    PMCI_OPEN_PARMS16         lpOpenParms16;
    PMCI_WAVE_OPEN_PARMS16    lpOpenWaveParms16;

    //
    // Note: MCI_ANIM_OPEN_PARMS16 and MCI_OVLY_OPEN_PARMS16 structures are
    // identical.
    //
    PMCI_ANIM_OPEN_PARMS16    lpOpenAnimParms16;

    //
    // pOp will point to the 32 bit open Parms structure after
    // we have finished all the thunking.
    //
    PMCI_ALL_OPEN_PARMS       pOp = (PMCI_ALL_OPEN_PARMS)pNewParms;

    //
    // We first do the fields that are common to all open requests.
    // Set up the VDM ptr for lpOpenParms16 to point to OrigParms
    //
    GETVDMPTR( OrigParms, sizeof(MCI_OPEN_PARMS16), lpOpenParms16 );

    //
    // Now scan our way thru all the known MCI_OPEN flags, thunking as
    // necessary.
    //
    // Start at the Device Type field
    //
    if ( *pOrigFlags & MCI_OPEN_TYPE ) {
        if ( *pOrigFlags & MCI_OPEN_TYPE_ID ) {

            dprintf4(( "ThunkOpenCmd: Got MCI_OPEN_TYPE_ID flag." ));
            pOp->OpenParms.lpstrDeviceType =
                (LPSTR)( FETCHDWORD( lpOpenParms16->lpstrDeviceType ) );
            dprintf5(( "lpstrDeviceType -> %ld", pOp->OpenParms.lpstrDeviceType ));

            *pOrigFlags ^= (MCI_OPEN_TYPE | MCI_OPEN_TYPE_ID);
        }
        else {
            dprintf4(( "ThunkOpenCmd: Got MCI_OPEN_TYPE flag" ));
            GETPSZPTR( lpOpenParms16->lpstrDeviceType,
                       pOp->OpenParms.lpstrDeviceType );
            dprintf5(( "lpstrDeviceType -> %s", pOp->OpenParms.lpstrDeviceType ));
            dprintf5(( "lpstrDeviceType -> 0x%lX", pOp->OpenParms.lpstrDeviceType ));

            *pOrigFlags ^= MCI_OPEN_TYPE;
        }
    }

    //
    // Now do the Element Name field
    //
    if ( *pOrigFlags & MCI_OPEN_ELEMENT ) {
        if ( *pOrigFlags & MCI_OPEN_ELEMENT_ID ) {

            dprintf4(( "ThunkOpenCmd: Got MCI_OPEN_ELEMENT_ID flag" ));
            pOp->OpenParms.lpstrElementName =
                (LPSTR)( FETCHDWORD( lpOpenParms16->lpstrElementName ) );
            dprintf5(( "lpstrElementName -> %ld", pOp->OpenParms.lpstrElementName ));

            *pOrigFlags ^= (MCI_OPEN_ELEMENT | MCI_OPEN_ELEMENT_ID);
        }
        else {
            dprintf4(( "ThunkOpenCmd: Got MCI_OPEN_ELEMENT flag" ));
            GETPSZPTR( lpOpenParms16->lpstrElementName,
                       pOp->OpenParms.lpstrElementName );
            dprintf5(( "lpstrElementName -> %s", pOp->OpenParms.lpstrElementName ));
            dprintf5(( "lpstrElementName -> 0x%lX", pOp->OpenParms.lpstrElementName ));

            *pOrigFlags ^= MCI_OPEN_ELEMENT;
        }
    }

    //
    // Now do the Alias Name field
    //
    if ( *pOrigFlags & MCI_OPEN_ALIAS  ) {
        dprintf4(( "ThunkOpenCmd: Got MCI_OPEN_ALIAS flag" ));
        GETPSZPTR( lpOpenParms16->lpstrAlias, pOp->OpenParms.lpstrAlias );
        dprintf5(( "lpstrAlias -> %s", pOp->OpenParms.lpstrAlias ));
        dprintf5(( "lpstrAlias -> 0x%lX", pOp->OpenParms.lpstrAlias ));

        *pOrigFlags ^= MCI_OPEN_ALIAS;
    }

    //
    // Clear the MCI_OPEN_SHAREABLE flag if it is set
    //
    if ( *pOrigFlags & MCI_OPEN_SHAREABLE ) {
        dprintf4(( "ThunkOpenCmd: Got MCI_OPEN_SHAREABLE flag." ));
        *pOrigFlags ^= MCI_OPEN_SHAREABLE;
    }

    //
    // Free the VDM pointer before returning
    //
    FREEVDMPTR( lpOpenParms16 );

    //
    // If we don't have any extended flags I can return now
    //
    if ( *pOrigFlags == 0 ) {
        return (DWORD)pOp;
    }

    //
    // If there are any flags left then these are intended for an extended
    // form of MCI open.  Three different forms are known, these being:
    //      MCI_ANIM_OPEN_PARMS
    //      MCI_OVLY_OPEN_PARMS
    //      MCI_WAVE_OPEN_PARMS
    //
    // If I could tell what sort of device I had I could thunk the
    // extensions with no problems, but we don't have a device ID yet
    // so I can't figure out what sort of device I have without parsing
    // the parameters that I already know about.
    //
    // But, I am in luck; MCI_WAVE_OPEN_PARMS has one extended parameter
    // dwBufferSeconds which has a MCI_WAVE_OPEN_BUFFER flag associated with
    // it.  This field is also a DWORD in the other two parms structures.
    //

    if ( *pOrigFlags & MCI_WAVE_OPEN_BUFFER ) {
        //
        // Set up the VDM ptr for lpOpenWaveParms16 to point to OrigParms
        //
        GETVDMPTR( OrigParms, sizeof(MCI_WAVE_OPEN_PARMS16),
                   lpOpenWaveParms16 );

        dprintf4(( "ThunkOpenCmd: Got MCI_WAVE_OPEN_BUFFER flag." ));
        pOp->OpenWaveParms.dwBufferSeconds =
                FETCHDWORD( lpOpenWaveParms16->dwBufferSeconds );
        dprintf5(( "dwBufferSeconds -> %ld", pOp->OpenWaveParms.dwBufferSeconds ));

        //
        // Free the VDM pointer before returning
        //
        FREEVDMPTR( lpOpenWaveParms16 );

        *pOrigFlags ^= MCI_WAVE_OPEN_BUFFER;
    }

    //
    // Now look for MCI_ANIM_OPEN_PARM and MCI_OVLY_OPEN_PARMS extensions.
    //
    if ( (*pOrigFlags & MCI_ANIM_OPEN_PARENT)
      || (*pOrigFlags & MCI_ANIM_OPEN_WS) ) {

        //
        // Set up the VDM ptr for lpOpenAnimParms16 to point to OrigParms
        //
        GETVDMPTR( OrigParms, sizeof(MCI_ANIM_OPEN_PARMS16),
                   lpOpenAnimParms16 );

        //
        // Check MCI_ANIN_OPEN_PARENT flag, this also checks
        // the MCI_OVLY_OPEN_PARENT flag too.
        //
        if ( *pOrigFlags & MCI_ANIM_OPEN_PARENT ) {
            dprintf4(( "ThunkOpenCmd: Got MCI_Xxxx_OPEN_PARENT flag." ));
            pOp->OpenAnimParms.hWndParent =
                HWND32(FETCHWORD(lpOpenAnimParms16->hWndParent) );

            *pOrigFlags ^= MCI_ANIM_OPEN_PARENT;
        }

        //
        // Check MCI_ANIN_OPEN_WS flag, this also checks
        // the MCI_OVLY_OPEN_WS flag too.
        //
        if ( *pOrigFlags & MCI_ANIM_OPEN_WS ) {
            dprintf4(( "ThunkOpenCmd: Got MCI_Xxxx_OPEN_WS flag." ));
            pOp->OpenAnimParms.dwStyle =
                FETCHDWORD( lpOpenAnimParms16->dwStyle );
            dprintf5(( "dwStyle -> %ld", pOp->OpenAnimParms.dwStyle ));

            *pOrigFlags ^= MCI_ANIM_OPEN_WS;
        }

        //
        // Free the VDM pointer before returning
        //
        FREEVDMPTR( lpOpenAnimParms16 );
    }

    //
    // Check the MCI_ANIN_OPEN_NOSTATIC flag
    //
    if ( *pOrigFlags & MCI_ANIM_OPEN_NOSTATIC ) {
        dprintf4(( "ThunkOpenCmd: Got MCI_ANIM_OPEN_NOSTATIC flag." ));
        *pOrigFlags ^= MCI_ANIM_OPEN_NOSTATIC;
    }

    return (DWORD)pOp;
}

/**********************************************************************\
* ThunkSetCmd
*
* Thunk the ThunkSetCmd mci command parms.
*
* The following are "basic" flags that all devices must support.
*   MCI_SET_AUDIO
*   MCI_SET_DOOR_CLOSED
*   MCI_SET_DOOR_OPEN
*   MCI_SET_TIME_FORMAT
*   MCI_SET_VIDEO
*   MCI_SET_ON
*   MCI_SET_OFF
*
* The following are "extended" flags that "sequencer" devices support.
*   MCI_SEQ_SET_MASTER
*   MCI_SEQ_SET_OFFSET
*   MCI_SEQ_SET_PORT
*   MCI_SEQ_SET_SLAVE
*   MCI_SEQ_SET_TEMPO
*
* The following are "extended" flags that "sequencer" devices support.
*   MCI_WAVE_INPUT
*   MCI_WAVE_OUTPUT
*   MCI_WAVE_SET_ANYINPUT
*   MCI_WAVE_SET_ANYOUTPUT
*   MCI_WAVE_SET_AVGBYTESPERSEC
*   MCI_WAVE_SET_BITSPERSAMPLES
*   MCI_WAVE_SET_BLOCKALIGN
*   MCI_WAVE_SET_CHANNELS
*   MCI_WAVE_SET_FORMAT_TAG
*   MCI_WAVE_SET_SAMPLESPERSEC
*
\**********************************************************************/
DWORD ThunkSetCmd( MCIDEVICEID DeviceID, PDWORD pOrigFlags, DWORD OrigParms,
                   DWORD pNewParms )
{
    //
    // This purpose of this union is to aid the creation of a 32 bit Set
    // Parms structure that is suitable for all known MCI devices.
    //
    typedef union  {
        MCI_SET_PARMS           SetParms;
        MCI_WAVE_SET_PARMS      SetWaveParms;
        MCI_SEQ_SET_PARMS       SetSeqParms;
    } MCI_ALL_SET_PARMS, *PMCI_ALL_SET_PARMS;

    //
    // The following pointers will be used to point to the original
    // 16-bit Parms structure.
    //
    PMCI_SET_PARMS16            lpSetParms16;
    PMCI_WAVE_SET_PARMS16       lpSetWaveParms16;
    PMCI_SEQ_SET_PARMS16        lpSetSeqParms16;

    //
    // pSet will point to the 32 bit Set Parms structure after
    // we have finished all the thunking.
    //
    PMCI_ALL_SET_PARMS          pSet = (PMCI_ALL_SET_PARMS)pNewParms;

    //
    // GetDevCaps is used to determine what sort of device are dealing
    // with.  We need this information to determine if we should use
    // standard, wave or sequencer MCI_SET structure.
    //
    MCI_GETDEVCAPS_PARMS        GetDevCaps;
    DWORD                       dwRetVal;

    //
    // Set up the VDM ptr for lpSetParms16 to point to OrigParms
    //
    GETVDMPTR( OrigParms, sizeof(MCI_SET_PARMS16), lpSetParms16 );

    //
    // First do the fields that are common to all devices.  Thunk the
    // dwAudio field.
    //
    if ( *pOrigFlags & MCI_SET_AUDIO ) {
        dprintf4(( "ThunkSetCmd: Got MCI_SET_AUDIO flag." ));
        pSet->SetParms.dwAudio = FETCHDWORD( lpSetParms16->dwAudio );
        dprintf5(( "dwAudio -> %ld", pSet->SetParms.dwAudio ));

        *pOrigFlags ^= MCI_SET_AUDIO;    // Mask out the flag
    }

    //
    // Thunk the dwTimeFormat field.
    //
    if ( *pOrigFlags & MCI_SET_TIME_FORMAT ) {
        dprintf4(( "ThunkSetCmd: Got MCI_SET_TIME_FORMAT flag." ));
        pSet->SetParms.dwTimeFormat = FETCHDWORD( lpSetParms16->dwTimeFormat );
        dprintf5(( "dwTimeFormat -> %ld", pSet->SetParms.dwTimeFormat ));

        *pOrigFlags ^= MCI_SET_TIME_FORMAT;    // Mask out the flag
    }

    //
    // Mask out the MCI_SET_DOOR_CLOSED
    //
    if ( *pOrigFlags & MCI_SET_DOOR_CLOSED ) {
        dprintf4(( "ThunkSetCmd: Got MCI_SET_DOOR_CLOSED flag." ));
        *pOrigFlags ^= MCI_SET_DOOR_CLOSED;    // Mask out the flag
    }

    //
    // Mask out the MCI_SET_DOOR_OPEN
    //
    if ( *pOrigFlags & MCI_SET_DOOR_OPEN ) {
        dprintf4(( "ThunkSetCmd: Got MCI_SET_DOOR_OPEN flag." ));
        *pOrigFlags ^= MCI_SET_DOOR_OPEN;    // Mask out the flag
    }

    //
    // Mask out the MCI_SET_VIDEO
    //
    if ( *pOrigFlags & MCI_SET_VIDEO ) {
        dprintf4(( "ThunkSetCmd: Got MCI_SET_VIDEO flag." ));
        *pOrigFlags ^= MCI_SET_VIDEO;    // Mask out the flag
    }

    //
    // Mask out the MCI_SET_ON
    //
    if ( *pOrigFlags & MCI_SET_ON ) {
        dprintf4(( "ThunkSetCmd: Got MCI_SET_ON flag." ));
        *pOrigFlags ^= MCI_SET_ON;    // Mask out the flag
    }

    //
    // Mask out the MCI_SET_OFF
    //
    if ( *pOrigFlags & MCI_SET_OFF ) {
        dprintf4(( "ThunkSetCmd: Got MCI_SET_OFF flag." ));
        *pOrigFlags ^= MCI_SET_OFF;    // Mask out the flag
    }

    //
    // Free the VDM pointer as we have finished with it
    //
    FREEVDMPTR( lpSetParms16 );

    //
    // We have done all the standard flags.  If there are any flags
    // still set we must have an extended command.
    //
    if ( *pOrigFlags == 0 ) {
        return (DWORD)pSet;
    }

    //
    // Now we need to determine what type of device we are
    // dealing with.  We can do this by send an MCI_GETDEVCAPS
    // command to the device. (We might as well use the Unicode
    // version of mciSendCommand and avoid another thunk).
    //
    RtlZeroMemory( &GetDevCaps, sizeof(MCI_GETDEVCAPS_PARMS) );
    GetDevCaps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
    dwRetVal = (*mmAPISendCmdW)( DeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM,
                                 (DWORD)&GetDevCaps );

    //
    // What do we do if dwRetCode is not equal to 0 ?  If this is the
    // case it probably means that we have been given a duff device ID,
    // anyway it is pointless to carry on with the thunk so I will clear
    // the *pOrigFlags variable and return.  This means that the 32 bit version
    // of mciSendCommand will get called with only half the message thunked,
    // but as there is probably already a problem with the device or
    // the device ID is duff, mciSendCommand should be able to work out a
    // suitable error code to return to the application.
    //
    if ( dwRetVal ) {
        *pOrigFlags = 0;
        return (DWORD)pSet;
    }
    switch ( GetDevCaps.dwReturn ) {
        case MCI_DEVTYPE_WAVEFORM_AUDIO:
            //
            // Set up the VDM ptr for lpSetWaveParms16 to point to OrigParms
            //
            dprintf3(( "ThunkSetCmd: Got a WaveAudio device." ));
            GETVDMPTR( OrigParms, sizeof(MCI_WAVE_SET_PARMS16),
                       lpSetWaveParms16 );
            //
            // Thunk the wInput field.
            //
            if ( *pOrigFlags & MCI_WAVE_INPUT ) {
                dprintf4(( "ThunkSetCmd: Got MCI_WAVE_INPUT flag." ));
                pSet->SetWaveParms.wInput =
                    FETCHWORD( lpSetWaveParms16->wInput );
                dprintf5(( "wInput -> %u", pSet->SetWaveParms.wInput ));
                *pOrigFlags ^= MCI_WAVE_INPUT;
            }

            //
            // Thunk the wOutput field.
            //
            if ( *pOrigFlags & MCI_WAVE_OUTPUT ) {
                dprintf4(( "ThunkSetCmd: Got MCI_WAVE_OUTPUT flag." ));
                pSet->SetWaveParms.wOutput =
                    FETCHWORD( lpSetWaveParms16->wOutput );
                dprintf5(( "wOutput -> %u", pSet->SetWaveParms.wOutput ));
                *pOrigFlags ^= MCI_WAVE_OUTPUT;
            }

            //
            // Thunk the wFormatTag field.
            //
            if ( *pOrigFlags & MCI_WAVE_SET_FORMATTAG ) {
                dprintf4(( "ThunkSetCmd: Got MCI_WAVE_SET_FORMATTAG flag." ));
                pSet->SetWaveParms.wFormatTag =
                    FETCHWORD( lpSetWaveParms16->wFormatTag );
                dprintf5(( "wFormatTag -> %u", pSet->SetWaveParms.wFormatTag ));
                *pOrigFlags ^= MCI_WAVE_SET_FORMATTAG;
            }

            //
            // Thunk the nChannels field.
            //
            if ( *pOrigFlags & MCI_WAVE_SET_CHANNELS ) {
                dprintf4(( "ThunkSetCmd: Got MCI_WAVE_SET_CHANNELS flag." ));
                pSet->SetWaveParms.nChannels =
                    FETCHWORD( lpSetWaveParms16->nChannels );
                dprintf5(( "nChannels -> %u", pSet->SetWaveParms.nChannels ));
                *pOrigFlags ^= MCI_WAVE_SET_CHANNELS;
            }

            //
            // Thunk the nSamplesPerSec field.
            //
            if ( *pOrigFlags & MCI_WAVE_SET_SAMPLESPERSEC ) {
                dprintf4(( "ThunkSetCmd: Got MCI_WAVE_SET_SAMPLESPERSEC flag." ));
                pSet->SetWaveParms.nSamplesPerSec =
                    FETCHDWORD( lpSetWaveParms16->nSamplesPerSecond );
                dprintf5(( "nSamplesPerSec -> %u", pSet->SetWaveParms.nSamplesPerSec ));
                *pOrigFlags ^= MCI_WAVE_SET_SAMPLESPERSEC;
            }

            //
            // Thunk the nAvgBytesPerSec field.
            //
            if ( *pOrigFlags & MCI_WAVE_SET_AVGBYTESPERSEC ) {
                dprintf4(( "ThunkSetCmd: Got MCI_WAVE_SET_AVGBYTESPERSEC flag." ));
                pSet->SetWaveParms.nAvgBytesPerSec =
                    FETCHDWORD( lpSetWaveParms16->nAvgBytesPerSec );
                dprintf5(( "nAvgBytesPerSec -> %u", pSet->SetWaveParms.nAvgBytesPerSec ));
                *pOrigFlags ^= MCI_WAVE_SET_AVGBYTESPERSEC;
            }

            //
            // Thunk the nBlockAlign field.
            //
            if ( *pOrigFlags & MCI_WAVE_SET_BLOCKALIGN ) {
                dprintf4(( "ThunkSetCmd: Got MCI_WAVE_SET_BLOCKALIGN flag." ));
                pSet->SetWaveParms.nBlockAlign =
                    FETCHWORD( lpSetWaveParms16->nBlockAlign );
                dprintf5(( "nBlockAlign -> %u", pSet->SetWaveParms.nBlockAlign ));
                *pOrigFlags ^= MCI_WAVE_SET_BLOCKALIGN;
            }

            //
            // Thunk the nBitsPerSample field.
            //
            if ( *pOrigFlags & MCI_WAVE_SET_BITSPERSAMPLE ) {
                dprintf4(( "ThunkSetCmd: Got MCI_WAVE_SET_BITSPERSAMPLE flag." ));
                pSet->SetWaveParms.wBitsPerSample =
                    FETCHWORD( lpSetWaveParms16->wBitsPerSample );
                dprintf5(( "wBitsPerSamples -> %u", pSet->SetWaveParms.wBitsPerSample ));
                *pOrigFlags ^= MCI_WAVE_SET_BITSPERSAMPLE;
            }

            FREEVDMPTR( lpSetWaveParms16 );
            break;

        case MCI_DEVTYPE_SEQUENCER:
            //
            // Set up the VDM ptr for lpSetSeqParms16 to point to OrigParms
            //
            dprintf3(( "ThunkSetCmd: Got a Sequencer device." ));
            GETVDMPTR( OrigParms, sizeof(MCI_WAVE_SET_PARMS16),
                       lpSetSeqParms16 );

            //
            // Thunk the dwMaster field.
            //
            if ( *pOrigFlags & MCI_SEQ_SET_MASTER ) {
                dprintf4(( "ThunkSetCmd: Got MCI_SEQ_SET_MASTER flag." ));
                pSet->SetSeqParms.dwMaster =
                    FETCHDWORD( lpSetSeqParms16->dwMaster );
                dprintf5(( "dwMaster -> %ld", pSet->SetSeqParms.dwMaster ));
                *pOrigFlags ^= MCI_SEQ_SET_MASTER;
            }

            //
            // Thunk the dwPort field.
            //
            if ( *pOrigFlags & MCI_SEQ_SET_PORT ) {
                dprintf4(( "ThunkSetCmd: Got MCI_SEQ_SET_PORT flag." ));
                pSet->SetSeqParms.dwPort =
                    FETCHDWORD( lpSetSeqParms16->dwPort );
                dprintf5(( "dwPort -> %ld", pSet->SetSeqParms.dwPort ));
                *pOrigFlags ^= MCI_SEQ_SET_PORT;
            }

            //
            // Thunk the dwOffset field.
            //
            if ( *pOrigFlags & MCI_SEQ_SET_OFFSET ) {
                dprintf4(( "ThunkSetCmd: Got MCI_SEQ_SET_OFFSET flag." ));
                pSet->SetSeqParms.dwOffset=
                    FETCHDWORD( lpSetSeqParms16->dwOffset );
                dprintf5(( "dwOffset -> %ld", pSet->SetSeqParms.dwOffset ));
                *pOrigFlags ^= MCI_SEQ_SET_OFFSET;
            }

            //
            // Thunk the dwSlave field.
            //
            if ( *pOrigFlags & MCI_SEQ_SET_SLAVE ) {
                dprintf4(( "ThunkSetCmd: Got MCI_SEQ_SET_SLAVE flag." ));
                pSet->SetSeqParms.dwSlave =
                    FETCHDWORD( lpSetSeqParms16->dwSlave );
                dprintf5(( "dwSlave -> %ld", pSet->SetSeqParms.dwSlave ));
                *pOrigFlags ^= MCI_SEQ_SET_SLAVE;
            }

            //
            // Thunk the dwTempo field.
            //
            if ( *pOrigFlags & MCI_SEQ_SET_TEMPO ) {
                dprintf4(( "ThunkSetCmd: Got MCI_SEQ_SET_TEMPO flag." ));
                pSet->SetSeqParms.dwTempo =
                    FETCHDWORD( lpSetSeqParms16->dwTempo );
                dprintf5(( "dwTempo -> %ld", pSet->SetSeqParms.dwTempo ));
                *pOrigFlags ^= MCI_SEQ_SET_TEMPO;
            }

            FREEVDMPTR( lpSetSeqParms16 );
            break;
    }

    return (DWORD)pSet;
}

/**********************************************************************\
* ThunkSetVideoCmd
*
* Thunk the SetVideo mci command parms.
*
\**********************************************************************/
DWORD ThunkSetVideoCmd( MCIDEVICEID DeviceID, PDWORD pOrigFlags,
                        DWORD OrigParms, DWORD pNewParms )
{

    //
    // The following pointers will be used to point to the original
    // 16-bit Parms structure.
    //
    LPMCI_DGV_SETVIDEO_PARMS        lpSetParms16;

    //
    // pSet will point to the 32 bit SetVideo Parms structure after
    // we have finished all the thunking.
    //
    LPMCI_DGV_SETVIDEO_PARMS        pSet = (LPMCI_DGV_SETVIDEO_PARMS)pNewParms;

    //
    // Set up the VDM ptr for lpSetParms16 to point to OrigParms
    //
    GETVDMPTR( OrigParms, sizeof(MCI_DGV_SETVIDEO_PARMS), lpSetParms16 );

    if ( *pOrigFlags & MCI_DGV_SETVIDEO_ITEM ) {

        dprintf4(( "ThunkSetVideoCmd: Got MCI_DGV_SETVIDEO_ITEM flag." ));
        pSet->dwItem = FETCHDWORD( lpSetParms16->dwItem );
        dprintf5(( "dwItem -> %ld", pSet->dwItem ));
        *pOrigFlags ^= MCI_DGV_SETVIDEO_ITEM;    // Mask out the flag
    }

    if ( *pOrigFlags & MCI_DGV_SETVIDEO_VALUE ) {

        if ( pSet->dwItem == MCI_DGV_SETVIDEO_PALHANDLE ) {

            HPAL16  hpal16;

            dprintf4(( "ThunkSetVideoCmd: Got MCI_DGV_SETVIDEO_PALHANLE." ));

            hpal16 = (HPAL16)LOWORD( FETCHDWORD( lpSetParms16->dwValue ) );
            pSet->dwValue = (DWORD)HPALETTE32( hpal16 );
            dprintf5(( "\t-> 0x%X", hpal16 ));

        }
        else {
            dprintf4(( "ThunkSetVideoCmd: Got an MCI_INTEGER." ));
            pSet->dwValue = FETCHDWORD( lpSetParms16->dwValue );
            dprintf5(( "dwValue -> %ld", pSet->dwValue ));
        }

        *pOrigFlags ^= MCI_DGV_SETVIDEO_VALUE;    // Mask out the flag
    }

    //
    // Turn off the MCI_SET_ON FLAG.
    //
    if ( *pOrigFlags & MCI_SET_ON ) {
        dprintf4(( "ThunkSetVideoCmd: Got MCI_SET_ON flag." ));
        *pOrigFlags ^= MCI_SET_ON;    // Mask out the flag
    }

    //
    // Turn off the MCI_SET_OFF FLAG.
    //
    if ( *pOrigFlags & MCI_SET_OFF ) {
        dprintf4(( "ThunkSetVideoCmd: Got MCI_SET_OFF flag." ));
        *pOrigFlags ^= MCI_SET_OFF;    // Mask out the flag
    }

    return (DWORD)pSet;
}


/**********************************************************************\
* ThunkSysInfoCmd
*
* Thunk the SysInfo mci command parms.
\**********************************************************************/
DWORD ThunkSysInfoCmd( PDWORD pOrigFlags, DWORD OrigParms, DWORD pNewParms )
{
    //
    // lpSysInfoParms16 points to the 16 bit parameter block
    // passed to us by the WOW application.
    //
    PMCI_SYSINFO_PARMS16    lpSysInfoParms16;

    //
    // pSys will point to the 32 bit SysInfo Parms structure after
    // we have finished all the thunking.
    //
    PMCI_SYSINFO_PARMS      pSys = (PMCI_SYSINFO_PARMS)pNewParms;

    //
    // Set up the VDM ptr for lpSysInfoParms16 to point to OrigParms
    //
    GETVDMPTR( OrigParms, sizeof(MCI_SYSINFO_PARMS16), lpSysInfoParms16 );

    //
    // Thunk the dwRetSize, dwNumber and wDeviceType parameters.
    //
    pSys->dwRetSize = FETCHDWORD( lpSysInfoParms16->dwRetSize );
    dprintf5(( "dwRetSize -> %ld", pSys->dwRetSize ));

    pSys->dwNumber = FETCHDWORD( lpSysInfoParms16->dwNumber );
    dprintf5(( "dwNumber -> %ld", pSys->dwNumber ));

    pSys->wDeviceType = (DWORD)FETCHWORD( lpSysInfoParms16->wDeviceType );
    dprintf5(( "wDeviceType -> %ld", pSys->wDeviceType ));

    //
    // Thunk lpstrReturn
    //
    if ( pSys->dwRetSize > 0 ) {
        GETVDMPTR( lpSysInfoParms16->lpstrReturn, pSys->dwRetSize,
                   pSys->lpstrReturn );
        dprintf5(( "lpstrReturn -> 0x%lX", lpSysInfoParms16->lpstrReturn ));
    }
    else {
        dprintf1(( "ThunkSysInfoCmd: lpstrReturn is 0 bytes long !!!" ));

        /* lpstrReturn has been set to NULL by RtlZeroMemory above */
    }

    //
    // Free the VDM pointer as we have finished with it
    //
    FREEVDMPTR( lpSysInfoParms16 );
    return (DWORD)pSys;

}

/**********************************************************************\
* ThunkBreakCmd
*
* Thunk the Break mci command parms.
\**********************************************************************/
DWORD ThunkBreakCmd( PDWORD pOrigFlags, DWORD OrigParms, DWORD pNewParms )
{
    //
    // lpBreakParms16 points to the 16 bit parameter block
    // passed to us by the WOW application.
    //
    PMCI_BREAK_PARMS16  lpBreakParms16;

    //
    // pBrk will point to the 32 bit Break Parms structure after
    // we have finished all the thunking.
    //
    PMCI_BREAK_PARMS    pBrk = (PMCI_BREAK_PARMS)pNewParms;

    //
    // Set up the VDM ptr for lpBreakParms16 to point to OrigParms
    //
    GETVDMPTR( OrigParms, sizeof(MCI_BREAK_PARMS16), lpBreakParms16 );

    //
    // Check for the MCI_BREAK_KEY flag
    //
    if ( *pOrigFlags & MCI_BREAK_KEY ) {
        dprintf4(( "ThunkBreakCmd: Got MCI_BREAK_KEY flag." ));
        pBrk->nVirtKey = (int)FETCHWORD( lpBreakParms16->nVirtKey );
        dprintf5(( "nVirtKey -> %d", pBrk->nVirtKey ));
    }

    //
    // Check for the MCI_BREAK_HWND flag
    //
    if ( *pOrigFlags & MCI_BREAK_HWND ) {
        dprintf4(( "ThunkBreakCmd: Got MCI_BREAK_HWND flag." ));
        pBrk->hwndBreak = HWND32(FETCHWORD(lpBreakParms16->hwndBreak));
    }

    //
    // Free the VDM pointer as we have finished with it
    //
    FREEVDMPTR( lpBreakParms16 );
    return (DWORD)pBrk;

}

/**********************************************************************\
* ThunkWindowCmd
*
* Thunk the mci Window command parms.
\**********************************************************************/
DWORD ThunkWindowCmd( MCIDEVICEID DeviceID, PDWORD pOrigFlags, DWORD OrigParms,
                      DWORD pNewParms )
{
    //
    // lpAni will point to the 32 bit Anim Window Parms
    // structure after we have finished all the thunking.
    //
    PMCI_ANIM_WINDOW_PARMS      lpAni = (PMCI_ANIM_WINDOW_PARMS)pNewParms;

    //
    // lpAniParms16 point to the 16 bit parameter block
    // passed to us by the WOW application.
    //
    PMCI_ANIM_WINDOW_PARMS16    lpAniParms16;

    //
    // GetDevCaps is used to determine what sort of device are dealing
    // with.  We need this information to determine if we should use
    // overlay or animation MCI_WINDOW structure.
    //
    MCI_GETDEVCAPS_PARMS        GetDevCaps;
    DWORD                       dwRetVal;

    //
    // Now we need to determine what type of device we are
    // dealing with.  We can do this by send an MCI_GETDEVCAPS
    // command to the device. (We might as well use the Unicode
    // version of mciSendCommand and avoid another thunk).
    //
    RtlZeroMemory( &GetDevCaps, sizeof(MCI_GETDEVCAPS_PARMS) );
    GetDevCaps.dwItem = MCI_GETDEVCAPS_DEVICE_TYPE;
    dwRetVal = (*mmAPISendCmdW)( DeviceID, MCI_GETDEVCAPS, MCI_GETDEVCAPS_ITEM,
                                (DWORD)&GetDevCaps );
    //
    // What do we do if dwRetCode is not equal to 0 ?  If this is the
    // case it probably means that we have been given a duff device ID,
    // anyway it is pointless to carry on with the thunk so I will clear
    // the *pOrigFlags variable and return.  This means that the 32 bit version
    // of mciSendCommand will get called with only half the message thunked,
    // but as there is probably already a problem with the device or
    // the device ID is duff, mciSendCommand should be able to work out a
    // suitable error code to return to the application.
    //
    if ( dwRetVal ) {
        *pOrigFlags = 0;
        return pNewParms;
    }

    //
    // Do we have an Animation or Overlay device type ?
    // Because Animation and Overlay have identical flags and
    // parms structures they can share the same code.
    //
    if ( GetDevCaps.dwReturn == MCI_DEVTYPE_ANIMATION
      || GetDevCaps.dwReturn == MCI_DEVTYPE_OVERLAY
      || GetDevCaps.dwReturn == MCI_DEVTYPE_DIGITAL_VIDEO ) {

        //
        // Set up the VDM ptr for lpWineParms16 to point to OrigParms
        //
        GETVDMPTR( OrigParms, sizeof(MCI_ANIM_WINDOW_PARMS16),
                   lpAniParms16 );

        //
        // Check for the MCI_ANIM_WINDOW_TEXT
        //
        if ( *pOrigFlags & MCI_ANIM_WINDOW_TEXT ) {
            dprintf4(( "ThunkWindowCmd: Got MCI_Xxxx_WINDOW_TEXT flag." ));

            GETPSZPTR( lpAniParms16->lpstrText, lpAni->lpstrText );

            dprintf5(( "lpstrText -> %s", lpAni->lpstrText ));
            dprintf5(( "lpstrText -> 0x%lX", lpAni->lpstrText ));
            *pOrigFlags ^= MCI_ANIM_WINDOW_TEXT;

        }

        //
        // Check for the MCI_ANIM_WINDOW_HWND flag
        //
        if ( *pOrigFlags & MCI_ANIM_WINDOW_HWND ) {
            dprintf4(( "ThunkWindowCmd: Got MCI_Xxxx_WINDOW_HWND flag." ));
            lpAni->hWnd = HWND32( FETCHWORD( lpAniParms16->hWnd ) );
            dprintf5(( "hWnd -> 0x%lX", lpAni->hWnd ));
            *pOrigFlags ^= MCI_ANIM_WINDOW_HWND;
        }

        //
        // Check for the MCI_ANIM_WINDOW_STATE flag
        //
        if ( *pOrigFlags & MCI_ANIM_WINDOW_STATE ) {
            dprintf4(( "ThunkWindowCmd: Got MCI_Xxxx_WINDOW_STATE flag." ));
            lpAni->nCmdShow = FETCHWORD( lpAniParms16->nCmdShow );
            dprintf5(( "nCmdShow -> 0x%lX", lpAni->nCmdShow ));
            *pOrigFlags ^= MCI_ANIM_WINDOW_STATE;
        }

        //
        // Check for the MCI_ANIM_WINDOW_DISABLE_STRETCH flag
        //
        if ( *pOrigFlags & MCI_ANIM_WINDOW_DISABLE_STRETCH ) {
            dprintf4(( "ThunkWindowCmd: Got MCI_Xxxx_WINDOW_DISABLE_STRETCH flag." ));
            *pOrigFlags ^= MCI_ANIM_WINDOW_DISABLE_STRETCH;
        }

        //
        // Check for the MCI_ANIM_WINDOW_ENABLE_STRETCH flag
        //
        if ( *pOrigFlags & MCI_ANIM_WINDOW_ENABLE_STRETCH ) {
            dprintf4(( "ThunkWindowCmd: Got MCI_Xxxx_WINDOW_ENABLE_STRETCH flag." ));
            *pOrigFlags ^= MCI_ANIM_WINDOW_ENABLE_STRETCH;
        }

        //
        // Free the VDM pointer as we have finished with it
        //
        FREEVDMPTR( lpAniParms16 );
        return (DWORD)lpAni;

    }

    return pNewParms;
}


/**********************************************************************\
*  ThunkCommandViaTable
*
\**********************************************************************/
INT ThunkCommandViaTable( LPWSTR lpCommand, DWORD dwFlags, DWORD OrigParms,
                          DWORD pNewParms )
{

#if DBG
    static  LPSTR   f_name = "ThunkCommandViaTable: ";
#endif

    LPWSTR  lpFirstParameter;

    UINT    wID;
    DWORD   dwValue;

    UINT    wOffset16, wOffset1stParm16;
    UINT    wOffset32, wOffset1stParm32;

    UINT    wParamSize;

    DWORD   dwParm16;
    PDWORD  pdwOrig16;
    PDWORD  pdwParm32;

    DWORD   dwMask = 1;

    //
    // Calculate the size of this command parameter block in terms
    // of bytes, then get a VDM pointer to the OrigParms.
    //
    GETVDMPTR( OrigParms, GetSizeOfParameter( lpCommand ), pdwOrig16 );
    dprintf3(( "%s16 bit Parms -> %lX", f_name, pdwOrig16 ));

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
    wOffset1stParm32 = wOffset1stParm16 = 4;

    lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                    (*mmAPIEatCmdEntry)( lpCommand, &dwValue, &wID ));
    //
    // If it is a return value, skip it
    //
    if ( wID == MCI_RETURN ) {

        //
        // Look for a string return type, these are a special case.
        //
        if ( dwValue == MCI_STRING ) {

            DWORD   dwStrlen;
            LPSTR   *lplpStr;

            //
            // Get string pointer and length
            //
            dwParm16 = FETCHDWORD(*(LPDWORD)((LPBYTE)pdwOrig16 + 4));
            dwStrlen = FETCHDWORD(*(LPDWORD)((LPBYTE)pdwOrig16 + 8));

            //
            // Copy string pointer
            //
            lplpStr = (LPSTR *)((LPBYTE)pNewParms + 4);
            if ( dwStrlen > 0 ) {
                GETVDMPTR( dwParm16, dwStrlen, *lplpStr );
                dprintf5(( "%sReturn string -> 0x%lX", f_name, *lplpStr ));
                dprintf5(( "%sReturn length -> 0x%lX", f_name, dwStrlen ));
            }

            //
            // Copy string length
            //
            pdwParm32 = (LPDWORD)((LPBYTE)pNewParms + 8);
            *pdwParm32 = dwStrlen;
        }

        //
        // Adjust the offset of the first parameter.  Remember that RECTS
        // are a different size in 16-bit world.
        //
        wParamSize = (*mmAPIGetParamSize)( dwValue, wID );
        wOffset1stParm16 += (dwValue == MCI_RECT ? sizeof(RECT16) : wParamSize);
        wOffset1stParm32 += wParamSize;

        //
        // Save the new first parameter
        //
        lpFirstParameter = lpCommand;
    }

    //
    // Walk through each flag
    //
    while ( dwMask != 0 ) {

        //
        // Is this bit set?
        //
        if ( (dwFlags & dwMask) != 0 ) {

            wOffset16 = wOffset1stParm16;
            wOffset32 = wOffset1stParm32;
            lpCommand = (LPWSTR)((LPBYTE)lpFirstParameter +
                                         (*mmAPIEatCmdEntry)( lpFirstParameter,
                                                              &dwValue, &wID ));

            //
            // What parameter uses this bit?
            //
            while ( wID != MCI_END_COMMAND && dwValue != dwMask ) {

                wParamSize = (*mmAPIGetParamSize)( dwValue, wID );
                wOffset16 += (wID == MCI_RECT ? sizeof( RECT16 ) : wParamSize);
                wOffset32 += wParamSize;

                if ( wID == MCI_CONSTANT ) {

                    while ( wID != MCI_END_CONSTANT ) {

                        lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                                (*mmAPIEatCmdEntry)( lpCommand, NULL, &wID ));
                    }
                }
                lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                             (*mmAPIEatCmdEntry)( lpCommand, &dwValue, &wID ));
            }

            if ( wID != MCI_END_COMMAND ) {

                //
                // Thunk the argument if there is one.  The argument is at
                // wOffset16 from the start of OrigParms.
                // This offset is in bytes.
                //
                dprintf5(( "%sOffset 16 -> 0x%lX", f_name, wOffset16 ));
                dprintf5(( "%sOffset 32 -> 0x%lX", f_name, wOffset32 ));

                if ( wID != MCI_FLAG ) {
                    dwParm16 = FETCHDWORD(*(LPDWORD)((LPBYTE)pdwOrig16 + wOffset16));
                    pdwParm32 = (LPDWORD)((LPBYTE)pNewParms + wOffset32);
                }

                switch ( wID ) {

                    case MCI_STRING:
                        {
                            LPSTR   str16 = (LPSTR)dwParm16;
                            LPSTR   str32 = (LPSTR)*pdwParm32;
                            dprintf4(( "%sGot STRING flag -> 0x%lX", f_name, dwMask ));
                            GETPSZPTR( str16, str32 );
                            dprintf5(( "%s\t-> 0x%lX", f_name, *pdwParm32 ));
                            dprintf5(( "%s\t-> %s", f_name, *pdwParm32 ));
                        }
                        break;

                    case MCI_HWND:
                        {
                            HWND16  hwnd16;
                            dprintf4(( "%sGot HWND flag -> 0x%lX", f_name, dwMask ));
                            hwnd16 = (HWND16)LOWORD( dwParm16 );
                            *pdwParm32 = (DWORD)HWND32( hwnd16 );
                            dprintf5(( "\t-> 0x%X", hwnd16 ));
                        }
                        break;

                    case MCI_HPAL:
                        {
                            HPAL16  hpal16;
                            dprintf4(( "%sGot HPAL flag -> 0x%lX", f_name, dwMask ));
                            hpal16 = (HPAL16)LOWORD( dwParm16 );
                            *pdwParm32 = (DWORD)HPALETTE32( hpal16 );
                            dprintf5(( "\t-> 0x%X", hpal16 ));
                        }
                        break;

                    case MCI_HDC:
                        {
                            HDC16   hdc16;
                            dprintf4(( "%sGot HDC flag -> 0x%lX", f_name, dwMask ));
                            hdc16 = (HDC16)LOWORD( dwParm16 );
                            *pdwParm32 = (DWORD)HDC32( hdc16 );
                            dprintf5(( "\t-> 0x%X", hdc16 ));
                        }
                        break;

                    case MCI_RECT:
                        {
                            PRECT16 pRect16 = (PRECT16)((LPBYTE)pdwOrig16 + wOffset16);
                            PRECT   pRect32 = (PRECT)pdwParm32;

                            dprintf4(( "%sGot RECT flag -> 0x%lX", f_name, dwMask ));
                            pRect32->top    = (LONG)pRect16->top;
                            pRect32->bottom = (LONG)pRect16->bottom;
                            pRect32->left   = (LONG)pRect16->left;
                            pRect32->right  = (LONG)pRect16->right;
                        }
                        break;

                    case MCI_CONSTANT:
                    case MCI_INTEGER:
                        dprintf4(( "%sGot INTEGER flag -> 0x%lX", f_name, dwMask ));
                        *pdwParm32 = dwParm16;
                        dprintf5(( "\t-> 0x%lX", dwParm16 ));
                        break;
                }
            }
        }

        //
        // Go to the next flag
        //
        dwMask <<= 1;
    }

    //
    // Free the VDM pointer as we have finished with it
    //
    FREEVDMPTR( pdwOrig16 );
    return 0;
}

/**********************************************************************\
*  GetSizeOfParameter
*
\**********************************************************************/
UINT GetSizeOfParameter( LPWSTR lpCommand )
{

#if DBG
    static  LPSTR   f_name = "GetSizeOfParameter";
#endif

    UINT    wOffset;
    UINT    wID;
    DWORD   dwValue;

    //
    // Skip past command entry
    //
    lpCommand = (LPWSTR)((LPBYTE)lpCommand + (*mmAPIEatCmdEntry)( lpCommand,
                                                                  NULL, NULL ));
    //
    // Skip past the DWORD return value
    //
    wOffset = 4;

    //
    // Get the first parameter slot entry
    //
    lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                    (*mmAPIEatCmdEntry)( lpCommand, &dwValue, &wID ));
    //
    // If it is a return value, skip it
    //
    if ( wID == MCI_RETURN ) {

        //
        // Don't forget that RECT's are smaller in 16-bit world.
        // Other parameters are OK though
        //
        if ( dwValue == MCI_RECT ) {
            wOffset += sizeof( RECT16 );
        }
        else {
            wOffset += (*mmAPIGetParamSize)( dwValue, wID );
        }

        //
        // Get first proper entry that is not a return field.
        //
        lpCommand = (LPWSTR)((LPBYTE)lpCommand +
                        (*mmAPIEatCmdEntry)( lpCommand, &dwValue, &wID ));
    }

    //
    // What parameter uses this bit?
    //
    while ( wID != MCI_END_COMMAND ) {

        //
        // Don't forget that RECT's are smaller in 16-bit world.
        // Other parameters are OK though
        //
        if ( wID == MCI_RECT ) {
            wOffset += sizeof( RECT16 );
        }
        else {
            wOffset += (*mmAPIGetParamSize)( dwValue, wID );
        }

        //
        // If we have a constant we need to skip the entries in
        // the command table.
        //
        if ( wID == MCI_CONSTANT ) {

            while ( wID != MCI_END_CONSTANT ) {

                lpCommand = (LPWSTR)((LPBYTE)lpCommand
                    + (*mmAPIEatCmdEntry)( lpCommand, NULL, &wID ));
            }
        }

        //
        // Get the next entry
        //
        lpCommand = (LPWSTR)((LPBYTE)lpCommand
                     + (*mmAPIEatCmdEntry)( lpCommand, &dwValue, &wID ));
    }

    dprintf4(( "%sSizeof Cmd Params -> %u bytes", f_name, wOffset ));
    return wOffset;
}


#if DBG
/*--------------------------------------------------------------------*\
                      MCI WOW DEBUGGING FUNCTIONS
\*--------------------------------------------------------------------*/

/**********************************************************************\
 * wow32MciDebugOutput
 *
 * Output a formated message to the debug terminal.
 *
\**********************************************************************/
VOID wow32MciDebugOutput( LPSTR lpszFormatStr, ... )
{
    CHAR buf[256];
    UINT n;
    va_list va;

    va_start(va, lpszFormatStr);
    n = vsprintf(buf, lpszFormatStr, va);
    va_end(va);

    buf[n++] = '\n';
    buf[n] = 0;
    OutputDebugString(buf);
}

/**********************************************************************\
 * wow32MciSetDebugLevel
 *
 * Query and set the debug level.
\**********************************************************************/
VOID wow32MciSetDebugLevel( VOID )
{

    int DebugLevel;

    //
    // First see if a specific WOW32MCI key has been defined.
    // If one hasn't been defined DebugLevel will be set to '999'.
    //
    DebugLevel = (int)GetProfileInt( "MMDEBUG", "WOW32MCI", 999 );

    //
    // If DebugLevel == '999' then an WOW32MCI key has not been defined,
    // so try a "WOW32" key.  This time if the key has not been defined
    // set the debug level to 0, ie. no debugging info should be
    // displayed.
    //
    if ( DebugLevel == 999 ) {
        DebugLevel = (int)GetProfileInt( "MMDEBUG", "WOW32", 0 );
    }

    mmDebugLevel = DebugLevel;
}
#endif
#endif
