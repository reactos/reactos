/*---------------------------------------------------------------------*\
*
*  WOW v1.0
*
*  Copyright (c) 1991, Microsoft Corporation
*
*  WMMEDIA3.C
*  WOW32 16-bit MultiMedia API support
*
*  Contains:
*       Aux sound support apis
*       Joystick support apis
*
*
*  History:
*  Created 21-Jan-1992 by Mike Tricker (MikeTri), after jeffpar
*  Changed 15-Jul-1992 by Mike Tricker (MikeTri), fixing GetDevCaps calls
*          26-Jul-1992 by Stephen Estrop (StephenE) thunks for mciSendCommand
*          30-Jul-1992 by Mike Tricker (MikeTri), fixing Wave/Midi/MMIO
*          03-Aug-1992 by Mike Tricker (MikeTri), added proper error handling
*          08-Oct-1992 by StephenE spawn from the original wmmedia.c
*
\*---------------------------------------------------------------------*/



//
// We define NO_STRICT so that the compiler doesn't moan and groan when
// I use the FARPROC type for the Multi-Media api loading.
//
#define NO_STRICT
#define OEMRESOURCE

#include "precomp.h"
#pragma hdrstop

#if 0

MODNAME(wmmedia1.c);

#if DBG
int mmTraceAux    = 0;
int mmTraceJoy    = 0;
#endif

/* ---------------------------------------------------------------------
** Auxiliary Sound APIs
** ---------------------------------------------------------------------
*/

/**********************************************************************\
*
* WMM32auxGetNumDevs
*
* This function retrieves the number of auxiliary output devices present in the
* system.
*
\**********************************************************************/
ULONG FASTCALL WMM32auxGetNumDevs(PVDMFRAME pFrame)
{
    ULONG ul;
    static   FARPROC            mmAPI = NULL;

    GET_MULTIMEDIA_API( "auxGetNumDevs", mmAPI, MMSYSERR_NODRIVER );

    UNREFERENCED_PARAMETER(pFrame);

    trace_aux(( "auxGetNumDevs()" ));
    ul = GETWORD16( (*mmAPI)() );
    trace_aux(( "-> %ld\n", ul ));

    RETURN(ul);
}

/**********************************************************************\
*
* WMM32auxGetDevCaps
*
* This function queries a specified auxiliary output device to determine its
* capabilities.
*
\**********************************************************************/
ULONG FASTCALL WMM32auxGetDevCaps(PVDMFRAME pFrame)
{
    ULONG ul;
    AUXCAPS auxcaps;
    register PAUXGETDEVCAPS16 parg16;
    static   FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "auxGetDevCapsA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(AUXGETDEVCAPS16), parg16);

    trace_aux(( "auxGetDevCaps( %x, %x, %x )", INT32(parg16->f1),
                 DWORD32(parg16->f2), UINT32(parg16->f3) ));

    /*
    ** If the size parameter was zero return straight away.  Note that this
    ** is not an error.
    */
    if ( UINT32( parg16->f3 ) == 0 ) {
        ul = MMSYSERR_NOERROR;
    }
    else {
        ul = GETWORD16((*mmAPI)( INT32(parg16->f1), &auxcaps,
                                 sizeof(AUXCAPS) ));
        /*
        ** Don't update the 16 bit structure if the call falied
        **
        */
        if ( ul == MMSYSERR_NOERROR ) {
            ul = PUTAUXCAPS16( parg16->f2, &auxcaps, UINT32(parg16->f3) );
        }
    }
    trace_aux(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32auxGetVolume
*
* This function returns the current volume setting of an auxiliary output
* device.
*
* Does this actually return the value in f2 ? It should...
*
\**********************************************************************/
ULONG FASTCALL WMM32auxGetVolume(PVDMFRAME pFrame)
{
    ULONG ul;
    LPDWORD lpdwVolume;
    register PAUXGETVOLUME16 parg16;
    static   FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "auxGetVolume", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(AUXGETVOLUME16), parg16);
    GETMISCPTR(parg16->f2, lpdwVolume);

    trace_aux(( "auxGetVolume( %x, %x )", INT32(parg16->f1),
                 DWORD32(parg16->f2) ));

    ul = GETWORD16((*mmAPI)( INT32(parg16->f1), lpdwVolume ));
    trace_aux(( "-> %ld\n", ul ));

    FREEMISCPTR(lpdwVolume);
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32auxSetVolume
*
* This function sets the volume of an auxiliary output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32auxSetVolume(PVDMFRAME pFrame)
{
    ULONG ul;
    register PAUXSETVOLUME16 parg16;
    static   FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "auxSetVolume", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(AUXSETVOLUME16), parg16);

    trace_aux(( "auxSetVolume( %x, %x )", INT32(parg16->f1),
                 DWORD32(parg16->f2) ));

    ul = GETWORD16((*mmAPI)( INT32(parg16->f1), DWORD32(parg16->f2) ));
    trace_aux(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32auxOutMessage
*
* This function sends a message to an auxiliary output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32auxOutMessage(PVDMFRAME pFrame)
{
    ULONG ul;
    register PAUXOUTMESSAGE3216 parg16;
    static   FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "auxOutMessage", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(AUXOUTMESSAGE16), parg16);

    trace_aux(( "auxOutMessage( %x, %x, %x, %x )", WORD32(parg16->f1),
                UINT32(parg16->f2), DWORD32(parg16->f3), DWORD32(parg16->f4) ));

    if ( (UINT32(parg16->f2) >= DRV_BUFFER_LOW)
      && (UINT32(parg16->f2) <= DRV_BUFFER_HIGH) ) {

        LPDWORD     lpdwParam1;
        GETMISCPTR(parg16->f3, lpdwParam1);

        ul = GETDWORD16((*mmAPI)( INT32(parg16->f1), UINT32(parg16->f2),
                                  (DWORD)lpdwParam1, DWORD32(parg16->f4) ));
        FREEMISCPTR(lpdwParam1);

    } else {

        ul = GETDWORD16((*mmAPI)( INT32(parg16->f1),
                                  MAKELONG( WORD32(parg16->f2), 0xFFFF ),
                                  DWORD32(parg16->f3),
                                  DWORD32(parg16->f4) ));
    }

    trace_aux(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/* ---------------------------------------------------------------------
** Joystick APIs
** ---------------------------------------------------------------------
*/

/**********************************************************************\
*
* WMM32joyGetNumDevs
*
* This function returns the number of joystick devices supported by the system.
*
*
*
\**********************************************************************/
ULONG FASTCALL WMM32joyGetNumDevs(PVDMFRAME pFrame)
{
    ULONG ul;
    static   FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "joyGetNumDevs", mmAPI, MMSYSERR_NODRIVER );

    UNREFERENCED_PARAMETER(pFrame);

    trace_joy(( "joyGetNumDevs()" ));
    ul = GETWORD16((*mmAPI)());
    trace_joy(( "-> %ld\n", ul ));

    RETURN(ul);
}

/**********************************************************************\
*
* WMM32joyGetDevCaps
*
* This function queries a joystick device to determine its capabilities.
*
\**********************************************************************/
ULONG FASTCALL WMM32joyGetDevCaps(PVDMFRAME pFrame)
{
    ULONG ul;
    JOYCAPS joycaps;
    register PJOYGETDEVCAPS16 parg16;
    static   FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "joyGetDevCapsA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(JOYGETDEVCAPS16), parg16);

    trace_joy(( "joyGetDevCaps( %x, %x, %x )", INT32(parg16->f1),
                DWORD32(parg16->f2), UINT32(parg16->f3) ));

    ul = GETWORD16((*mmAPI)(INT32(parg16->f1), &joycaps, sizeof(JOYCAPS)));

    if ( ul == JOYERR_NOERROR ) {
        ul = PUTJOYCAPS16( parg16->f2, &joycaps, UINT32(parg16->f3) );
    }
    trace_joy(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32joyGetPos
*
* This function queries the position and button activity of a joystick device.
*
\**********************************************************************/
ULONG FASTCALL WMM32joyGetPos(PVDMFRAME pFrame)
{
    ULONG ul;
    JOYINFO joyinfo;
    register PJOYGETPOS16 parg16;
    static   FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "joyGetPos", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(JOYGETPOS16), parg16);
    trace_joy(( "joyGetPosition( %x, %x )", WORD32(parg16->f1),
                DWORD32(parg16->f2) ));

    ul = GETWORD16((*mmAPI)( INT32(parg16->f1), &joyinfo ));

    if ( ul == JOYERR_NOERROR ) {
        ul = PUTJOYINFO16( parg16->f2, &joyinfo );
    }
    trace_joy(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);

    RETURN(ul);
}

/**********************************************************************\
*
* WMM32joySetThreshold
*
* This function sets the movement threshold of a joystick device.
*
\**********************************************************************/
ULONG FASTCALL WMM32joySetThreshold(PVDMFRAME pFrame)
{
    ULONG ul;
    register PJOYSETTHRESHOLD16 parg16;
    static   FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "joySetThreshold", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(JOYSETTHRESHOLD), parg16);

    trace_joy(( "joySetThreshold( %x, %x )", INT32(parg16->f1),
                UINT32(parg16->f2) ));

    ul = GETWORD16((*mmAPI)( INT32(parg16->f1), UINT32(parg16->f2) ));
    trace_joy(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32joyGetThreshold
*
* This function queries the current movement threshold of a joystick device.
*
\**********************************************************************/
ULONG FASTCALL WMM32joyGetThreshold(PVDMFRAME pFrame)
{
    register PJOYGETTHRESHOLD16 parg16;
             ULONG              ul;
             UINT               uThreshold;
             LPWORD             lpwThreshold16;
    static   FARPROC            mmAPI = NULL;

    GET_MULTIMEDIA_API( "joyGetThreshold", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(JOYGETTHRESHOLD16), parg16);
    trace_joy(( "joyGetThreshold( %x, %x )", WORD32(parg16->f1),
                 DWORD32(parg16->f2) ));


    ul = GETWORD16((*mmAPI)( INT32(parg16->f1), &uThreshold ));

    /*
    ** Only copy the threshold back to 16 bit space if the call was sucessful
    **
    */
    if ( ul == JOYERR_NOERROR ) {

        MMGETOPTPTR( parg16->f2, sizeof(WORD), lpwThreshold16 );

        if ( lpwThreshold16 ) {
            STOREWORD  ( *lpwThreshold16, uThreshold );
            FLUSHVDMPTR( DWORD32(parg16->f2), sizeof(WORD), lpwThreshold16 );
            FREEVDMPTR ( lpwThreshold16 );
        }
        else {
            ul = JOYERR_PARMS;
        }
    }

    trace_joy(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}


/**********************************************************************\
*
* WMM32joyReleaseCapture
*
* This function releases the capture set by joySetCapture on the specified
* joystick device
*
\**********************************************************************/
ULONG FASTCALL WMM32joyReleaseCapture(PVDMFRAME pFrame)
{
    ULONG ul;
    register PJOYRELEASECAPTURE16 parg16;
    static   FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "joyReleaseCapture", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(JOYRELEASECAPTURE16), parg16);

    trace_joy(( "joyReleaseCapture( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)( INT32(parg16->f1) ));
    trace_joy(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32joySetCapture
*
* This function causes joystick messages to be sent to the specified window.
*
\**********************************************************************/
ULONG FASTCALL WMM32joySetCapture(PVDMFRAME pFrame)
{
    ULONG ul;
    register PJOYSETCAPTURE16 parg16;
    static   FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "joySetCapture", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(JOYSETCAPTURE), parg16);

    trace_joy(( "joySetCapture( %x, %x, %x, %x )", WORD32(parg16->f1),
                INT32(parg16->f2), UINT32(parg16->f3), BOOL32(parg16->f4) ));

    ul = GETWORD16((*mmAPI)( HWND32(parg16->f1), INT32(parg16->f2),
                             UINT32(parg16->f3), BOOL32(parg16->f4) ));
    trace_joy(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/**********************************************************************\
*
* WMM32joySetCalibration
*
* This function allows the calibration of a joystick device.
*
\**********************************************************************/
ULONG FASTCALL WMM32joySetCalibration(PVDMFRAME pFrame)
{
    register PJOYSETCALIBRATION16 parg16;
    static   FARPROC              mmAPI = NULL;
             ULONG                ul;
             LPWORD               lpwXbase;
             LPWORD               lpwXdelta;
             LPWORD               lpwYbase;
             LPWORD               lpwYdelta;
             LPWORD               lpwZbase;
             LPWORD               lpwZdelta;
             UINT                 uXbase;
             UINT                 uXdelta;
             UINT                 uYbase;
             UINT                 uYdelta;
             UINT                 uZbase;
             UINT                 uZdelta;

    GET_MULTIMEDIA_API( "joySetCapture", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(JOYSETCALIBRATION16), parg16);

    trace_joy(( "joySetCalibration( %x, %x, %x, %x, %x, %x, %x )",
                DWORD32(parg16->f1), UINT32(parg16->f2), UINT32(parg16->f3),
                UINT32(parg16->f4),  UINT32(parg16->f5), UINT32(parg16->f6),
                UINT32(parg16->f7) ));

    MMGETOPTPTR( parg16->f2, sizeof(WORD), lpwXbase );
    if ( lpwXbase == NULL ) {
        goto exit_1;
    }

    MMGETOPTPTR( parg16->f3, sizeof(WORD), lpwXdelta );
    if ( lpwXdelta == NULL ) {
        goto exit_2;
    }

    MMGETOPTPTR( parg16->f4, sizeof(WORD), lpwYbase );
    if ( lpwYbase == NULL ) {
        goto exit_3;
    }

    MMGETOPTPTR( parg16->f5, sizeof(WORD), lpwYdelta );
    if ( lpwYdelta == NULL ) {
        goto exit_4;
    }

    MMGETOPTPTR( parg16->f6, sizeof(WORD), lpwZbase );
    if ( lpwZbase == NULL ) {
        goto exit_5;
    }

    MMGETOPTPTR( parg16->f7, sizeof(WORD), lpwZdelta );
    if ( lpwZdelta == NULL ) {
        goto exit_6;
    }

    uXbase   = FETCHWORD( *lpwXbase  );
    uXdelta  = FETCHWORD( *lpwXdelta );
    uYbase   = FETCHWORD( *lpwYbase  );
    uYdelta  = FETCHWORD( *lpwYdelta );
    uZbase   = FETCHWORD( *lpwZbase  );
    uZdelta  = FETCHWORD( *lpwZdelta );

    ul = GETWORD16((*mmAPI)( DWORD32(parg16->f1), &uXbase, &uXdelta,
                             &uYbase, &uYdelta, &uZbase, &uZdelta ));

    STOREWORD( *lpwXbase,  uXbase  );
    STOREWORD( *lpwXdelta, uXdelta );
    STOREWORD( *lpwYbase,  uYbase  );
    STOREWORD( *lpwYdelta, uYdelta );
    STOREWORD( *lpwZbase,  uZbase  );
    STOREWORD( *lpwZdelta, uZdelta );

    FREEMISCPTR( lpwZdelta );

exit_6:
    FREEMISCPTR( lpwZbase );

exit_5:
    FREEMISCPTR( lpwYdelta );

exit_4:
    FREEMISCPTR( lpwYbase );

exit_3:
    FREEMISCPTR( lpwXdelta );

exit_2:
    FREEMISCPTR( lpwXbase );

exit_1:
    trace_joy(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

#endif


