/***********************************************************************\

*
*  WOW v1.0
*
*  Copyright (c) 1991, Microsoft Corporation
*
*  WMMEDIA2.C
*  WOW32 16-bit MultiMedia API support
*
*  Contains:
*       Midi IN apis
*       Midi OUT apis
*       Wave IN apis
*       Wave OUT apis
*
*
*  History:
*  Created 21-Jan-1992 by Mike Tricker (MikeTri), after jeffpar
*  Changed 15-Jul-1992 by Mike Tricker (MikeTri), fixing GetDevCaps calls
*          26-Jul-1992 by Stephen Estrop (StephenE) thunks for mciSendCommand
*          30-Jul-1992 by Mike Tricker (MikeTri), fixing Wave/Midi/MMIO
*          03-Aug-1992 by Mike Tricker (MikeTri), added proper error handling
*          08-Oct-1992 by StephenE spawned from the original wmmedia.c
*
\***********************************************************************/


//
// We define NO_STRICT so that the compiler doesn't moan and groan when
// I use the FARPROC type for the Multi-Media api loading.
//
#define NO_STRICT
#define OEMRESOURCE

#include "precomp.h"
#pragma hdrstop

#if 0


MODNAME(wmmedia2.c);

// A 32 bit ptr to the 16 bit callback data
extern PCALLBACK_DATA       pCallBackData;
extern CRITICAL_SECTION     mmCriticalSection;

#if DBG
/*
** AllocCount maintains a count of the number XXM_DONE messages that
** we expect to receive before the device is closed.  When the device is
** closed this count should be zero.
**
*/
int AllocWaveCount = 0;
int AllocMidiCount = 0;
int mmTraceWave    = 0;
int mmTraceMidi    = 0;
#endif


/* ---------------------------------------------------------------------
** MIDI Output API's
** ---------------------------------------------------------------------
*/

/**********************************************************************\
*
* WMM32midiOutGetNumDevs
*
* This function retrieves the number of MIDI output devices present
* in the system.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutGetNumDevs(PVDMFRAME pFrame)
{
    static  FARPROC mmAPI = NULL;
            ULONG   ul;

    UNREFERENCED_PARAMETER(pFrame);

    GET_MULTIMEDIA_API( "midiOutGetNumDevs", mmAPI, MMSYSERR_NODRIVER );

    trace_midi(( "midiOutGetNumDevs()" ));
    ul = GETWORD16((*mmAPI)() );
    trace_midi(( "-> %ld\n", ul ));

    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiOutGetDevCaps
*
* This function queries a specified MIDI output device to determine its
* capabilities.
*
*
*
*  We will now change things...
*
* Step 1: get the ENTIRE Caps structure, irrespective of the number of bytes
*         requested
*         (previously I was getting the requested number of bytes via
*         parg16->f3 (plus 2 'cos of the WORD -> UINT change for version -
*         which was wrong anyway...) )
*
* Step 2: thunk the ENTIRE structure in to a 16 bit local variable
*
* Step 3: RtlCopyMemory the REQUESTED number of bytes from the local copy
*         to the "real" structure within the app
*
* Thanks to RCBS for sorting me out once again !
*
*
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutGetDevCaps(PVDMFRAME pFrame)

{
    register PMIDIOUTGETDEVCAPS16   parg16;
    static   FARPROC                mmAPI = NULL;
             ULONG                  ul;
             MIDIOUTCAPS            midioutcaps;

    GET_MULTIMEDIA_API( "midiOutGetDevCapsA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTGETDEVCAPS16), parg16);

    trace_midi(( "midiOutGetDevCaps( %x, %x, %x )", INT32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));


    /*
    ** If the size parameter was zero return straight away.  Note that this
    ** is not an error.
    */
    if ( UINT32( parg16->f3 ) == 0 ) {
        ul = MMSYSERR_NOERROR;
    }
    else {

        ul = GETWORD16((*mmAPI)( INT32(parg16->f1), &midioutcaps,
                                 sizeof(MIDIOUTCAPS) ));
        /*
        ** This must now thunk the ENTIRE structure, then copy parg16->f3
        ** bytes onto the "real" structure in the app, but only if the call
        ** returned success.
        */
        if ( ul == MMSYSERR_NOERROR ) {
            ul = PUTMIDIOUTCAPS16(parg16->f2, &midioutcaps, UINT32(parg16->f3));
        }
    }
    trace_midi(( "-> %ld\n", ul ));


    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiOutGetErrorText
*
* This function retrieves a textual description of the error
* identified by the specified error number.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutGetErrorText(PVDMFRAME pFrame)
{
    register PMIDIOUTGETERRORTEXT16 parg16;
    static   FARPROC                mmAPI = NULL;
             ULONG                  ul = MMSYSERR_NOERROR;
             PSZ                    pszText;

    GET_MULTIMEDIA_API( "midiOutGetErrorTextA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTGETERRORTEXT16), parg16);

    trace_midi(( "midiOutGetErrorText( %x, %x, %x )", UINT32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));

    /*
    ** Test against a zero length string and a NULL pointer.  If 0 is passed
    ** as the buffer length then the manual says we should return
    ** MMSYSERR_NOERR.  MMGETOPTPTR only returns a pointer if parg16->f2 is
    ** not NULL.
    */
    MMGETOPTPTR( parg16->f2, UINT32(parg16->f3), pszText );
    if ( pszText != NULL ) {

        ul = GETWORD16((*mmAPI)( UINT32(parg16->f1), pszText,
                                 UINT32(parg16->f3) ));

        FLUSHVDMPTR(DWORD32(parg16->f2), UINT32(parg16->f3), pszText);
        FREEVDMPTR(pszText);
    }
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/***********************************************************************\
*
* WMM32midiOutOpen
*
* This function opens a specified MIDI output device for playback.
*
\***********************************************************************/
ULONG FASTCALL WMM32midiOutOpen(PVDMFRAME pFrame)
{
    register PMIDIOUTOPEN16 parg16;
    static   FARPROC        mmAPI = NULL;

    ULONG         ul = MMSYSERR_NOERROR;
    UINT          uDevID;
    PINSTANCEDATA pInstanceData;
    LPHMIDIOUT    lpHMidiOut;        // pointer to handle in 16 bit app space
    HMIDIOUT      Hand32;            // 32bit handle

    GET_MULTIMEDIA_API( "midiOutOpen", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTOPEN16), parg16);

    trace_midi(( "midiOutOpen( %x, %x, %x, %x, %x )",
                 DWORD32( parg16->f1 ), INT32  ( parg16->f2 ),
                 DWORD32( parg16->f3 ), DWORD32( parg16->f4 ),
                 DWORD32( parg16->f5 ) ));

    /*
    ** Get the device ID. We use INT32 here not UINT32 to make sure that
    ** negative values (such as MIDI_MAPPER (-1)) get thunked correctly.
    */
    uDevID = (UINT)INT32(parg16->f2);

    /*
    ** Map the 16 bit pointer is one was specified.
    */
    MMGETOPTPTR( parg16->f1, sizeof(HMIDIOUT16), lpHMidiOut );
    if ( lpHMidiOut ) {

        /*
        ** Create InstanceData block to be used by our callback routine.
        **
        ** NOTE: Although we malloc it here we don't free it.
        ** This is not a mistake - it must not be freed before the
        ** callback routine has used it - so it does the freeing.
        **
        ** If the malloc fails we bomb down to the bottom,
        ** set ul to MMSYSERR_NOMEM and exit gracefully.
        **
        ** We always have a callback functions.  This is to ensure that
        ** the MIDIHDR structure keeps getting copied back from
        ** 32 bit space to 16 bit, as it contains flags which
        ** applications are liable to keep checking.
        */
        if ( pInstanceData = malloc_w(sizeof(INSTANCEDATA)) ) {

            dprintf2(( "WM32midiOutOpen: Allocated instance buffer at %8X",
                       pInstanceData ));
            pInstanceData->dwCallback         = DWORD32(parg16->f3);
            pInstanceData->dwCallbackInstance = DWORD32(parg16->f4);
            pInstanceData->dwFlags            = DWORD32(parg16->f5);

            ul = GETWORD16((*mmAPI)( &Hand32, uDevID,
                                     (DWORD)W32CommonDeviceOpen,
                                     (DWORD)pInstanceData,
                                     CALLBACK_FUNCTION ));

        }
        else {
            ul = MMSYSERR_NOMEM;
        }

        /*
        ** If the call returns success update the 16 bit handle,
        ** otherwise don't, and free the memory we malloc'd earlier, as
        ** the callback that would have freed it will never get callled.
        */
        if ( ul == MMSYSERR_NOERROR ) {

                HMIDIOUT16 Hand16 = GETHMIDIOUT16(Hand32);

                trace_midi(( "Handle -> %x", Hand16 ));

                STOREWORD  ( *lpHMidiOut, Hand16 );
                FLUSHVDMPTR( DWORD32(parg16->f1), sizeof(HMIDIOUT16),
                             lpHMidiOut );
        }

        /*
        ** We only free the memory if we actually allocated any
        */
        else if ( pInstanceData ) {

            free_w(pInstanceData);
        }

        /*
        ** Regardless of sucess or failure we need to free the pointer
        ** to the 16 bit MidiIn handle.
        */
        FREEVDMPTR ( lpHMidiOut );

    }
    else {

        ul = MMSYSERR_INVALPARAM;
    }
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/**********************************************************************\
*
* WMM32midiOutClose
*
* This function closes the specified MIDI output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutClose(PVDMFRAME pFrame)
{
    register PMIDIOUTCLOSE16 parg16;
    static   FARPROC         mmAPI = NULL;
             ULONG           ul;

    GET_MULTIMEDIA_API( "midiOutClose", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTCLOSE16), parg16);

    trace_midi(( "midiOutClose( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16( (*mmAPI)(HMIDIOUT32(parg16->f1) ));
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/**********************************************************************\
*
* WMM32midiOutPrepareHeader
*
* This function prepares the specified midiform header.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutPrepareHeader(PVDMFRAME pFrame)
{
    register PMIDIOUTPREPAREHEADER3216 parg16;
    static   FARPROC                   mmAPI = NULL;
             ULONG                     ul;
             MIDIHDR                   midihdr;


    GET_MULTIMEDIA_API( "midiOutPrepareHeader", mmAPI, MMSYSERR_NODRIVER );
    GETARGPTR(pFrame, sizeof(MIDIOUTPREPAREHEADER3216), parg16);
    trace_midi(( "midiOutPrepareHeader( %x %x %x)", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), WORD32( parg16->f3 ) ));

    GETMIDIHDR16(parg16->f2, &midihdr);

    ul = GETWORD16((*mmAPI)( HMIDIOUT32(parg16->f1),
                             &midihdr, WORD32(parg16->f3) ) );
    /*
    ** Only update the 16 bit structure if the call returns success
    **
    */
    if ( !ul ) {
        PUTMIDIHDR16(parg16->f2, &midihdr);
    }

    trace_midi(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiOutUnprepareHeader
*
* This function prepares the specified midiform header.
* This function cleans up the preparation performed by midiOutPrepareHeader.
* The function must be called after the device driver has finished with a
* data block. You must call this function before freeing the data buffer.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutUnprepareHeader(PVDMFRAME pFrame)
{
    register PMIDIOUTUNPREPAREHEADER3216  parg16;
    static   FARPROC                      mmAPI = NULL;
             ULONG                        ul;
             MIDIHDR                      midihdr;

    GET_MULTIMEDIA_API( "midiOutUnprepareHeader", mmAPI, MMSYSERR_NODRIVER );
    GETARGPTR(pFrame, sizeof(MIDIOUTUNPREPAREHEADER3216), parg16);
    trace_midi(( "midiOutUnprepareHeader( %x %x %x)", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), WORD32( parg16->f3 ) ));

    GETMIDIHDR16(parg16->f2, &midihdr);

    ul = GETWORD16((*mmAPI)( HMIDIOUT32(parg16->f1),
                             &midihdr, WORD32(parg16->f3) ) );
    /*
    ** Only update the 16 bit structure if the call returns success
    */
    if (!ul) {
        PUTMIDIHDR16(parg16->f2, &midihdr);
    }

    trace_midi(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}


/**********************************************************************\
*
* WMM32midiOutShortMsg
*
* This function sends a short MIDI message to the specified MIDI output device.
* Use this function to send any MIDI message except for system exclusive
* messages.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutShortMsg(PVDMFRAME pFrame)
{
    register PMIDIOUTSHORTMSG16 parg16;
    static   FARPROC            mmAPI = NULL;
             ULONG              ul;

    GET_MULTIMEDIA_API( "midiOutShortMsg", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTSHORTMSG16), parg16);

    trace_midi(( "midiOutShortMsg( %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));

    ul = GETWORD16((*mmAPI)( HMIDIOUT32(parg16->f1), DWORD32(parg16->f2) ));
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiOutLongMsg
*
* This function sends a long MIDI message to the specified MIDI output
* device.  Use this function to send system exclusive messages or to
* send a buffer filled with short messages.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutLongMsg(PVDMFRAME pFrame)
{
    register PMIDIOUTLONGMSG16  parg16;
    static   FARPROC            mmAPI = NULL;
             ULONG              ul;
             PMIDIHDR32         pMidihdr32;

    GET_MULTIMEDIA_API( "midiOutLongMsg", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTLONGMSG16), parg16);

    trace_midi(( "midiOutLongMsg( %x, %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));

    /*
    ** If the given size of the MIDIHDR structure is too small
    ** or the lphdr is invalid return an error
    **
    */
    if ( UINT32(parg16->f3) < sizeof(MIDIHDR16)
      || HIWORD( DWORD32(parg16->f2) ) == 0 ) {

        ul = MMSYSERR_INVALPARAM;
    }
    else {
        if ( pMidihdr32 = malloc_w(sizeof(MIDIHDR32)) ) {

            PMIDIHDR   lpwhdr;
#if DBG
            AllocMidiCount++;
            dprintf2(( "M>> %8X (%d)", pMidihdr32, AllocMidiCount ));
#endif
            /* Copy across the midi header stuff.  Note that lpwhdr (a
            ** 32 bit ptr to a 32 bit midi header) is used to make the
            ** pointer stuff a bit less hairy.
            **
            ** pMidihdr32->pMidihdr32 is a 32 bit ptr to a 16 bit midi header
            ** pMidihdr32->pMidihdr16 is a 16 bit ptr to a 16 bit midi header
            ** pMidihdr32->Midihdr    is a 32 bit midi header
            */
            lpwhdr = &(pMidihdr32->Midihdr);
            pMidihdr32->pMidihdr16 = (PMIDIHDR16)DWORD32(parg16->f2);
            pMidihdr32->pMidihdr32 = GETMIDIHDR16(DWORD32(parg16->f2), lpwhdr);

            /*
            ** GETMIDIHDR16 can return NULL, in which case we should set
            ** lpwhdr to NULL too and call midiOutLongMessage only to get the
            ** correct error code.
            */
            if ( pMidihdr32->pMidihdr32 == NULL ) {
                lpwhdr = NULL;
            }

            ul = GETWORD16( (*mmAPI)( HMIDIOUT32(parg16->f1), lpwhdr,
                                      UINT32(parg16->f3) ) );
            /*
            ** If the call fails we need to free the memory we malloc'd
            ** above, as the callback that would have freed it will never
            ** get called.
            **
            */
            if ( ul == MMSYSERR_NOERROR ) {

                /*
                ** Make sure we reflect any changes that midiOutLongMessage did
                ** to the MIDIHDR back to 16 bit land.
                **
                ** This is important because some apps poll the
                ** MHDR_DONE bit !!
                */
                COPY_MIDIOUTHDR16_FLAGS( pMidihdr32->pMidihdr32,
                                         pMidihdr32->Midihdr );
            }
            else {
#if DBG
                AllocMidiCount--;
                dprintf2(( "M<< \t%8X (%d)", pMidihdr32,
                            AllocMidiCount ));
#endif
                free_w( pMidihdr32 );
            }
        }
        else {
            ul = MMSYSERR_NOMEM;
        }
    }
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiOutReset
*
* This function turns off all notes on al MIDI channels for the specified
* MIDI output deice.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutReset(PVDMFRAME pFrame)
{
    register PMIDIOUTRESET16   parg16;
    static   FARPROC           mmAPI = NULL;
             ULONG             ul;

    GET_MULTIMEDIA_API( "midiOutReset", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTRESET16), parg16);

    trace_midi(( "midiOutReset( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)( HMIDIOUT32(parg16->f1) ));
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiOutGetVolume
*
* This function returns the current volume setting of a MIDI output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutGetVolume(PVDMFRAME pFrame)
{
    register PMIDIOUTGETVOLUME16 parg16;
    static   FARPROC             mmAPI = NULL;
             ULONG               ul;
             LPDWORD             lpdwVolume;
             DWORD               dwVolume;

    GET_MULTIMEDIA_API( "midiOutGetVolume", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTGETVOLUME16), parg16);

    trace_midi(( "midiOutGetVolume( %x, %x )", INT32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));

    ul = GETWORD16((*mmAPI)( INT32(parg16->f1), &dwVolume ));
    if ( ul == MMSYSERR_NOERROR ) {

        MMGETOPTPTR( parg16->f2, sizeof(DWORD), lpdwVolume);

        if ( lpdwVolume ) {
            STOREDWORD ( *lpdwVolume, dwVolume );
            FLUSHVDMPTR( DWORD32(parg16->f2), sizeof(DWORD), lpdwVolume );
            FREEVDMPTR ( lpdwVolume );
        }
        else {
            ul = MMSYSERR_INVALPARAM;
        }
    }
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiOutSetVolume
*
* This function sets the volume of a MIDI output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutSetVolume(PVDMFRAME pFrame)
{
    register PMIDIOUTSETVOLUME16 parg16;
    static   FARPROC             mmAPI = NULL;
             ULONG               ul;

    GET_MULTIMEDIA_API( "midiOutSetVolume", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTSETVOLUME16), parg16);

    trace_midi(( "midiOutSetVolume( %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));
    ul = GETWORD16((*mmAPI)( INT32(parg16->f1), DWORD32(parg16->f2) ));
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiOutCachePatches
*
* This function requests that an internal MIDI synthesizer device preload a
* specified set of patches. Some synthesizers are not capable of keeping all
* patches loaded simultaneously and must load data from disk when they receive
* MIDI program change messages. Caching patches ensures specified patches are
* immediately available.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutCachePatches(PVDMFRAME pFrame)
{
    register PMIDIOUTCACHEPATCHES16 parg16;
    static   FARPROC                mmAPI = NULL;
             ULONG                  ul = MMSYSERR_INVALPARAM;
             LPPATCHARRAY           lppa1;

    GET_MULTIMEDIA_API( "midiOutCachePatches", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTCACHEPATCHES16), parg16);

    trace_midi(( "midiOutCachePatches( %x, %x, %x, %x )", WORD32( parg16->f1 ),
                 UINT32( parg16->f2 ), DWORD32( parg16->f3 ),
                 UINT32( parg16->f4 ) ));

    /*
    ** GETMISCPTR checks that parg16->f3 is not zero so we need not bother.
    */
    GETMISCPTR( DWORD32( parg16->f3 ), lppa1 );

    if ( lppa1 ) {

        ul = GETWORD16((*mmAPI)( HMIDIOUT32(parg16->f1), UINT32(parg16->f2),
                                 lppa1, UINT32(parg16->f4) ));
        FREEMISCPTR( lppa1 );

    }
    else  {
        dprintf1(( "WMM32midiOutCachePatches passed a NULL pointer" ));
    }
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiOutCacheDrumPatches
*
* This function requests that an internal MIDI synthesizer device preload a
* specified set of key-based percussion patches. Some synthesizers are not
* capable of keeping all percussion patches loaded simultaneously. Caching
* patches ensures specified patches are immediately available.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutCacheDrumPatches(PVDMFRAME pFrame)
{
    register PMIDIOUTCACHEDRUMPATCHES16 parg16;
    static   FARPROC                    mmAPI = NULL;
             ULONG                      ul = MMSYSERR_INVALPARAM;
             LPKEYARRAY                 lpka1;

    GET_MULTIMEDIA_API( "midiOutCacheDrumPatches", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTCACHEDRUMPATCHES16), parg16);
    trace_midi(( "midiOutCacheDrumPatches( %x, %x, %x, %x )",
                 WORD32( parg16->f1 ), UINT32( parg16->f2 ),
                 DWORD32( parg16->f3 ), UINT32( parg16->f4 ) ));

    GETMISCPTR( DWORD32( parg16->f3 ), lpka1 );
    if ( lpka1 ) {

        ul = GETWORD16((*mmAPI)( HMIDIOUT32(parg16->f1), UINT32(parg16->f2),
                                 lpka1, UINT32(parg16->f4) ) );
        FREEMISCPTR(lpka1);
    }
    else  {
        dprintf1(( "WMM32midiOutCacheDrumPatches passed a NULL pointer" ));
    }
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiOutGetID
*
* This function gets the device ID for a MIDI output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutGetID(PVDMFRAME pFrame)
{
    register PMIDIOUTGETID16    parg16;
    static   FARPROC            mmAPI = NULL;
             ULONG              ul;
             UINT               dwDeviceID32;
             LPWORD             lpwDeviceID16;

    GET_MULTIMEDIA_API( "midiOutGetID", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTGETID16), parg16);

    trace_midi(( "midiOutGetID( %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));

    ul = GETWORD16((*mmAPI)( (HMIDIOUT)HMIDIOUT32(parg16->f1), &dwDeviceID32 ));

    /*
    ** Only copy the ID back to 16 bit space if the call was sucessful
    **
    */
    if ( ul == MMSYSERR_NOERROR ) {

        MMGETOPTPTR( parg16->f2, sizeof(WORD), lpwDeviceID16 );

        if ( lpwDeviceID16 ) {
            STOREWORD  ( *lpwDeviceID16, dwDeviceID32 );
            FLUSHVDMPTR( DWORD32(parg16->f2), sizeof(WORD), lpwDeviceID16 );
            FREEVDMPTR ( lpwDeviceID16 );
        }
        else {
            ul = MMSYSERR_INVALPARAM;
        }
    }
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiOutMessage
*
* This function sends a message to the specified MIDI output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiOutMessage(PVDMFRAME pFrame)
{
    register PMIDIOUTMESSAGE3216 parg16;
    static   FARPROC             mmAPI = NULL;
             ULONG               ul;

    GET_MULTIMEDIA_API( "midiOutMessage", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIOUTMESSAGE16), parg16);

    trace_midi(( "midiOutMessage( %x, %x, %x, %x )",
                 WORD32( parg16->f1 ),   UINT32( parg16->f2 ),
                 DWORD32( parg16->f3 ), DWORD32( parg16->f4 ) ));

    if ( (UINT32(parg16->f2) >= DRV_BUFFER_LOW)
      && (UINT32(parg16->f2) <= DRV_BUFFER_HIGH) ) {

        LPDWORD     lpdwParam1;
        GETMISCPTR(parg16->f3, lpdwParam1);

        ul = GETDWORD16((*mmAPI)( HMIDIOUT32(parg16->f1), UINT32(parg16->f2),
                                  (DWORD)lpdwParam1, DWORD32(parg16->f4)));
        FREEMISCPTR(lpdwParam1);

    } else {
        ul = GETDWORD16((*mmAPI)( HMIDIOUT32(parg16->f1),
                                  MAKELONG( WORD32(parg16->f2), 0xFFFF ),
                                  DWORD32(parg16->f3), DWORD32(parg16->f4) ));
    }

    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/* ---------------------------------------------------------------------
** MIDI Input API's
** ---------------------------------------------------------------------
*/


/**********************************************************************\
*
* WMM32midiInGetNumDevs
*
* This function retrieves the number of MIDI input devices in the system.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiInGetNumDevs(PVDMFRAME pFrame)
{
    static  FARPROC mmAPI = NULL;
            ULONG   ul;

    GET_MULTIMEDIA_API( "midiInGetNumDevs", mmAPI, MMSYSERR_NODRIVER );

    UNREFERENCED_PARAMETER(pFrame);

    trace_midi(( "midiInGetNumDevs()" ));
    ul = GETWORD16((*mmAPI)() );
    trace_midi(( "-> %ld\n", ul ));

    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiInGetDevCaps
*
* This function queries a specified MIDI input device to determine its
* capabilities.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiInGetDevCaps(PVDMFRAME pFrame)
{
    ULONG ul;
    MIDIINCAPS midiincaps1;
    register PMIDIINGETDEVCAPS16 parg16;
    static   FARPROC             mmAPI = NULL;

    GET_MULTIMEDIA_API( "midiInGetDevCapsA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIINGETDEVCAPS16), parg16);

    trace_midi(( "midiInGetDevCaps( %x, %x, %x )", INT32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));
    if ( UINT32( parg16->f3 ) == 0 ) {
        ul = MMSYSERR_NOERROR;
    }
    else {

        ul = GETWORD16((*mmAPI)(INT32(parg16->f1), &midiincaps1,
                                sizeof(MIDIINCAPS) ) );
        /*
        ** This must now thunk the ENTIRE structure, then copy parg16->f3
        ** bytes onto the "real" structure in the app, but only if the
        ** call returned success.
        */
        if ( ul == MMSYSERR_NOERROR ) {
            ul = PUTMIDIINCAPS16( parg16->f2, &midiincaps1, UINT32(parg16->f3) );
        }
    }
    trace_midi(( "-> %ld\n", ul ));


    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiInGetErrorText
*
* This function retrieves a textual description of the error identified by the
* specified error number.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiInGetErrorText(PVDMFRAME pFrame)
{
    register PMIDIINGETERRORTEXT16  parg16;
             ULONG                  ul = MMSYSERR_NOERROR;
             PSZ                    pszText;
    static   FARPROC                mmAPI = NULL;

    GET_MULTIMEDIA_API( "midiInGetErrorTextA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR( pFrame, sizeof(MIDIINGETERRORTEXT16), parg16 );

    trace_midi(( "midiInGetErrorText( %x, %x, %x )", UINT32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));
    /*
    ** Test against a zero length string and a NULL pointer.  If 0 is passed
    ** as the buffer length then the manual says we should return
    ** MMSYSERR_NOERR.
    */
    MMGETOPTPTR( parg16->f2, UINT32(parg16->f3), pszText );
    if ( pszText != NULL ) {

        ul = GETWORD16( (*mmAPI)( UINT32(parg16->f1), pszText,
                                  UINT32(parg16->f3) ) );

        FLUSHVDMPTR( DWORD32(parg16->f2), UINT32(parg16->f3), pszText);
        FREEVDMPTR( pszText );
    }
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}



/**********************************************************************\
*
* WMM32midiInOpen
*
* This function opens a specified MIDI input device.
*
\***********************************************************************/
ULONG FASTCALL WMM32midiInOpen(PVDMFRAME pFrame)
{
    ULONG           ul=0;
    UINT            uDevID;
    PINSTANCEDATA   pInstanceData;
    LPHMIDIIN       lpHMidiIn;      // pointer to handle in 16 bit app space
    HMIDIIN         Hand32;         // 32bit handle
    register PMIDIINOPEN16 parg16;
    static   FARPROC       mmAPI = NULL;

    GET_MULTIMEDIA_API( "midiInOpen", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIINOPEN16), parg16);

    trace_midi(( "midiInOpen( %x, %x, %x, %x, %x )",
                 DWORD32( parg16->f1 ), INT32  ( parg16->f2 ),
                 DWORD32( parg16->f3 ), DWORD32( parg16->f4 ),
                 DWORD32( parg16->f5 ) ));

    /*
    ** Get the device ID. We use INT32 here not UINT32 to make sure that
    ** negative values (such as MIDI_MAPPER (-1)) get thunked correctly.
    */
    uDevID = (UINT)INT32(parg16->f2);

    /*
    ** Map the 16 bit pointer is one was specified.
    */
    MMGETOPTPTR( parg16->f1, sizeof(HMIDIIN), lpHMidiIn );
    if ( lpHMidiIn ) {

        /*
        ** Create InstanceData block to be used by our callback routine.
        **
        ** NOTE: Although we malloc it here we don't free it.
        **       This is not a mistake - it must not be freed before the
        **       callback routine has used it - so it does the freeing.
        **
        ** If the malloc fails we bomb down to the bottom,
        ** set ul to MMSYSERR_NOMEM and exit gracefully.
        **
        ** We always have a callback functions.  This is to ensure that
        ** the MIDIHDR structure keeps getting copied back from
        ** 32 bit space to 16 bit, as it contains flags which
        ** applications are liable to keep checking.
        */
        if ( pInstanceData = malloc_w(sizeof(INSTANCEDATA) ) ) {

            dprintf2(( "WM32midiInOpen: Allocated instance buffer at %8X",
                       pInstanceData ));
            pInstanceData->dwCallback         = DWORD32(parg16->f3);
            pInstanceData->dwCallbackInstance = DWORD32(parg16->f4);
            pInstanceData->dwFlags            = DWORD32(parg16->f5);

            ul = GETWORD16((*mmAPI)( &Hand32, uDevID,
                                     (DWORD)W32CommonDeviceOpen,
                                     (DWORD)pInstanceData,
                                     CALLBACK_FUNCTION ));

        }
        else {

            ul = (ULONG)MMSYSERR_NOMEM;
        }

        /*
        ** If the call returns success update the 16 bit handle,
        ** otherwise don't, and free the memory we malloc'd earlier, as
        ** the callback that would have freed it will never get callled.
        */
        if ( ul == MMSYSERR_NOERROR ) {

            HMIDIIN16 Hand16 = GETHMIDIIN16(Hand32);

            trace_midi(( "Handle -> %x", Hand16 ));
            STOREWORD  ( *lpHMidiIn, Hand16 );
            FLUSHVDMPTR( DWORD32(parg16->f1), sizeof(HMIDIIN), lpHMidiIn );
        }

        /*
        ** We only free the memory if we actually allocated any and the
        ** 32 bit call failed.
        */
        else if ( pInstanceData ) {

            free_w(pInstanceData);
        }

        /*
        ** Regardless of sucess or failure we need to free the pointer
        ** to the 16 bit MidiIn handle.
        */
        FREEVDMPTR ( lpHMidiIn );

    }
    else {
        ul = MMSYSERR_INVALPARAM;
    }

    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiInClose
*
* This function closes the specified MIDI input device.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiInClose(PVDMFRAME pFrame)
{
    ULONG ul;
    register PMIDIINCLOSE16 parg16;
    static   FARPROC        mmAPI = NULL;

    GET_MULTIMEDIA_API( "midiInClose", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIINCLOSE16), parg16);

    trace_midi(( "midiInClose( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)(HMIDIIN32(parg16->f1) ) );
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/**********************************************************************\
*
* WMM32midiInPrepareHeader
*
* This function prepares the specified midiform header.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiInPrepareHeader(PVDMFRAME pFrame)
{
    register PMIDIOUTPREPAREHEADER3216 parg16;
    static   FARPROC                   mmAPI = NULL;
             ULONG                     ul;
             MIDIHDR                   midihdr;


    GET_MULTIMEDIA_API( "midiInPrepareHeader", mmAPI, MMSYSERR_NODRIVER );
    GETARGPTR(pFrame, sizeof(MIDIOUTPREPAREHEADER3216), parg16);
    trace_midi(( "midiInPrepareHeader( %x %x %x)", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), WORD32( parg16->f3 ) ));

    GETMIDIHDR16(parg16->f2, &midihdr);

    ul = GETWORD16((*mmAPI)( HMIDIOUT32(parg16->f1),
                             &midihdr, WORD32(parg16->f3) ) );
    /*
    ** Only update the 16 bit structure if the call returns success
    **
    */
    if ( !ul ) {
        PUTMIDIHDR16(parg16->f2, &midihdr);
    }

    trace_midi(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiInUnprepareHeader
*
* This function prepares the specified midiform header.
* This function cleans up the preparation performed by midiInPrepareHeader.
* The function must be called after the device driver has finished with a
* data block. You must call this function before freeing the data buffer.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiInUnprepareHeader(PVDMFRAME pFrame)
{
    register PMIDIOUTUNPREPAREHEADER3216  parg16;
    static   FARPROC                      mmAPI = NULL;
             ULONG                        ul;
             MIDIHDR                      midihdr;

    GET_MULTIMEDIA_API( "midiInUnprepareHeader", mmAPI, MMSYSERR_NODRIVER );
    GETARGPTR(pFrame, sizeof(MIDIOUTUNPREPAREHEADER3216), parg16);
    trace_midi(( "midiInUnprepareHeader( %x %x %x)", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), WORD32( parg16->f3 ) ));

    GETMIDIHDR16(parg16->f2, &midihdr);

    ul = GETWORD16((*mmAPI)( HMIDIOUT32(parg16->f1),
                             &midihdr, WORD32(parg16->f3) ) );
    /*
    ** Only update the 16 bit structure if the call returns success
    */
    if (!ul) {
        PUTMIDIHDR16(parg16->f2, &midihdr);
    }

    trace_midi(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiInAddBuffer
*
* This function sends an input buffer to a specified opened MIDI input device.
* When the buffer is filled, it is sent back to the application. Input buffers
* are used only for system exclusive messages.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiInAddBuffer(PVDMFRAME pFrame)
{
    ULONG ul;
    PMIDIHDR32 pMidihdr32;
    register PMIDIINADDBUFFER16 parg16;
    static   FARPROC            mmAPI = NULL;

    GET_MULTIMEDIA_API( "midiInAddBuffer", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIINADDBUFFER16), parg16);

    trace_midi(( "midiInAddBuffer( %x, %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));

    /*
    ** If the given size of the MIDIHDR structure is too small
    ** or the lphdr is invalid return an error
    **
    */
    if ( UINT32(parg16->f3) < sizeof(MIDIHDR16)
      || HIWORD( DWORD32(parg16->f2) ) == 0 ) {

        ul = (ULONG)MMSYSERR_INVALPARAM;
    }
    else  {
        if (pMidihdr32 = malloc_w(sizeof(MIDIHDR32) ) ) {

            PMIDIHDR   lpwhdr;
#if DBG
            AllocMidiCount++;
            dprintf2(( "M>> %8X (%d)", pMidihdr32, AllocMidiCount ));
#endif
            /* Copy across the midi header stuff.  Note that lpwhdr (a
            ** 32 bit ptr to a 32 bit midi header) is used to make the
            ** pointer stuff a bit less hairy.
            **
            ** pMidihdr32->Midihdr    is a 32 bit midi header
            ** pMidihdr32->pMidihdr16 is a 16 bit ptr to a 16 bit midi header
            ** pMidihdr32->pMidihdr32 is a 32 bit ptr to a 16 bit midi header
            */
            lpwhdr = &(pMidihdr32->Midihdr);
            pMidihdr32->pMidihdr16 = (PMIDIHDR16)DWORD32(parg16->f2);
            pMidihdr32->pMidihdr32 = GETMIDIHDR16(DWORD32(parg16->f2), lpwhdr);

            /*
            ** GETMIDIHDR16 can return NULL, in which case we should set
            ** lpwhdr to NULL too and call midiInAddBuffer only to get the
            ** correct error code.
            */
            if ( pMidihdr32->pMidihdr32 == NULL ) {
                lpwhdr = NULL;
            }

            ul = GETWORD16((*mmAPI)( HMIDIIN32(parg16->f1), lpwhdr,
                                     UINT32(parg16->f3) ));
            /*
            ** If the call fails we need to free the memory we malloc'd
            ** above, as the callback that would have freed it will never
            ** get called.
            **
            */
            if ( ul == MMSYSERR_NOERROR ) {

                /*
                ** Make sure we reflect any changes that midiInAddBuffer did
                ** to the MIDIHDR back to 16 bit land.
                **
                ** This is important because some apps poll the
                ** MHDR_DONE bit !!
                */
                COPY_MIDIINHDR16_FLAGS( pMidihdr32->pMidihdr32,
                                        pMidihdr32->Midihdr );
            }
            else {
#if DBG
                AllocMidiCount--;
                dprintf2(( "M<< \t%8X (%d)", pMidihdr32,
                            AllocMidiCount ));
#endif
                free_w( pMidihdr32 );
            }
        }
        else {
            ul = (ULONG)MMSYSERR_NOMEM;
        }
    }
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiInStart
*
* This function starts MIDI input on the specified MIDI input device.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiInStart(PVDMFRAME pFrame)
{
    ULONG ul;
    register PMIDIINSTART16 parg16;
    static   FARPROC        mmAPI = NULL;

    GET_MULTIMEDIA_API( "midiInStart", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIINSTART16), parg16);

    trace_midi(( "midiInStart( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)(HMIDIIN32(parg16->f1) ) );
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiInStop
*
* This function terminates MIDI input on the specified MIDI input device.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiInStop(PVDMFRAME pFrame)
{
    ULONG ul;
    register PMIDIINSTOP16 parg16;
    static   FARPROC        mmAPI = NULL;

    GET_MULTIMEDIA_API( "midiInStop", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIINSTOP16), parg16);

    trace_midi(( "midiInStop( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)(HMIDIIN32(parg16->f1) ) );
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiInReset
*
* This function stops input on a given MIDI input device and marks all pending
* input buffers as done.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiInReset(PVDMFRAME pFrame)
{
    ULONG ul;
    register PMIDIINRESET16 parg16;
    static   FARPROC        mmAPI = NULL;

    GET_MULTIMEDIA_API( "midiInReset", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIINRESET16), parg16);

    trace_midi(( "midiInReset( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)(HMIDIIN32(parg16->f1) ) );
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiInGetID
*
* This function gets the device ID for a MIDI input device.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiInGetID(PVDMFRAME pFrame)
{
    register PMIDIINGETID16 parg16;
    static   FARPROC        mmAPI = NULL;
             ULONG          ul;
             UINT           dwDeviceID32;
             LPWORD         lpwDeviceID16;

    GET_MULTIMEDIA_API( "midiInGetID", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIINGETID16), parg16);

    trace_midi(( "midiInGetID( %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));

    ul = GETWORD16((*mmAPI)( HMIDIIN32(parg16->f1), &dwDeviceID32 ));

    /*
    ** Only copy the ID back to 16 bit space if the call was sucessful
    **
    */
    if ( ul == MMSYSERR_NOERROR ) {

        MMGETOPTPTR( parg16->f2, sizeof(WORD), lpwDeviceID16 );

        if ( lpwDeviceID16 ) {
            STOREWORD  ( *lpwDeviceID16, dwDeviceID32 );
            FLUSHVDMPTR( DWORD32(parg16->f2), sizeof(WORD), lpwDeviceID16 );
            FREEVDMPTR ( lpwDeviceID16 );
        }
        else {
            ul = MMSYSERR_INVALPARAM;
        }
    }
    trace_midi(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32midiInMessage
*
* This function sends a message to the specified MIDI input device.
*
\**********************************************************************/
ULONG FASTCALL WMM32midiInMessage(PVDMFRAME pFrame)
{
    ULONG ul;
    register PMIDIINMESSAGE3216 parg16;
    static   FARPROC          mmAPI = NULL;

    GET_MULTIMEDIA_API( "midiInMessage", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(MIDIINMESSAGE16), parg16);

    trace_midi(( "midiInMessage( %x, %x, %x, %x )",
                 WORD32( parg16->f1 ),   UINT32( parg16->f2 ),
                 DWORD32( parg16->f3 ), DWORD32( parg16->f4 ) ));

    if ( (UINT32(parg16->f2) >= DRV_BUFFER_LOW)
      && (UINT32(parg16->f2) <= DRV_BUFFER_HIGH) ) {

        LPDWORD     lpdwParam1;
        GETMISCPTR(parg16->f3, lpdwParam1);

        ul = GETDWORD16((*mmAPI)( HMIDIIN32(parg16->f1), UINT32(parg16->f2),
                                  (DWORD)lpdwParam1, DWORD32(parg16->f4)));
        FREEMISCPTR(lpdwParam1);

    } else {
        ul = GETDWORD16((*mmAPI)( HMIDIIN32(parg16->f1),
                                  MAKELONG( WORD32(parg16->f2), 0xFFFF ),
                                  DWORD32(parg16->f3), DWORD32(parg16->f4) ));
    }

    trace_midi(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}


/* ---------------------------------------------------------------------
** WAVE Output API's
** ---------------------------------------------------------------------
*/

/**********************************************************************\
*
* WMM32waveOutGetNumDevs
*
* This function retrieves the number of waveform output devices present in the
* system.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutGetNumDevs(PVDMFRAME pFrame)
{
    static  FARPROC mmAPI = NULL;
            ULONG   ul;

    GET_MULTIMEDIA_API( "waveOutGetNumDevs", mmAPI, MMSYSERR_NODRIVER );

    trace_wave(( "waveOutGetNumDevs()" ));
    ul = GETWORD16((*mmAPI)() );
    trace_wave(( "-> %ld\n", ul ));

    RETURN(ul);

    UNREFERENCED_PARAMETER(pFrame);
}

/**********************************************************************\
*
* WMM32waveOutGetDevCaps
*
* This function queries a specified waveform device to determine its
* capabilities.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutGetDevCaps(PVDMFRAME pFrame)
{
    register PWAVEOUTGETDEVCAPS16 parg16;
    static   FARPROC              mmAPI = NULL;
             ULONG                ul;
             WAVEOUTCAPS          waveoutcaps;

    GET_MULTIMEDIA_API( "waveOutGetDevCapsA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTGETDEVCAPS16), parg16);

    trace_wave(( "waveOutGetDevCaps( %x, %x, %x )", INT32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));

    /*
    ** If the size parameter was zero return straight away.  Note that this
    ** is not an error.
    */
    if ( UINT32( parg16->f3 ) == 0 ) {
        ul = MMSYSERR_NOERROR;
    }
    else {

        ul = GETWORD16((*mmAPI)( INT32(parg16->f1), &waveoutcaps,
                                 sizeof(WAVEOUTCAPS) ));
        /*
        ** Don't update the 16 bit structure if the call failed
        **
        */
        if ( ul == MMSYSERR_NOERROR ) {
            ul = PUTWAVEOUTCAPS16(parg16->f2, &waveoutcaps, UINT32(parg16->f3));
        }
    }

    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);

    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutGetErrorText
*
* This function retrieves a textual description of the error identified by the
* specified error number.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutGetErrorText(PVDMFRAME pFrame)
{
    register    PWAVEOUTGETERRORTEXT16  parg16;
    static      FARPROC                 mmAPI = NULL;
                ULONG                   ul = MMSYSERR_NOERROR;
                PSZ                     pszText;

    GET_MULTIMEDIA_API( "waveOutGetErrorTextA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTGETERRORTEXT16), parg16);

    trace_wave(( "waveOutGetErrorText( %x, %x, %x )", UINT32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));

    /*
    ** Test against a zero length string and a NULL pointer.  If 0 is passed
    ** as the buffer length then the manual says we should return
    ** MMSYSERR_NOERR.  MMGETOPTPTR only returns a pointer if parg16->f2 is
    ** not NULL.
    */
    MMGETOPTPTR( parg16->f2, UINT32(parg16->f3), pszText );
    if ( pszText != NULL ) {

        ul = GETWORD16( (*mmAPI)( UINT32(parg16->f1), pszText,
                                  UINT32(parg16->f3) ) );

        FLUSHVDMPTR( DWORD32(parg16->f2), UINT32(parg16->f3), pszText);
        FREEVDMPTR(pszText);
    }
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutOpen
*
* This function opens a specified waveform output device for playback.
*
* As of November 1992 we map the 16 bit Wave Format data directly to the
* the 32 bit side, no thunking of the parameters is performed.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutOpen(PVDMFRAME pFrame)
{
    ULONG           ul = MMSYSERR_NOERROR;
    UINT            uDevID;
    PINSTANCEDATA   pInstanceData = NULL;
    DWORD           dwFlags;
    PWAVEFORMAT16   lpWaveformData;
    LPHWAVEOUT      lphWaveOut = NULL; // pointer to handle in 16 bit app space
    HWAVEOUT        Hand32;            // 32bit handle
    register PWAVEOUTOPEN16 parg16;
    static   FARPROC        mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveOutOpen", mmAPI, MMSYSERR_NODRIVER );


    GETARGPTR(pFrame, sizeof(WAVEOUTOPEN16), parg16);

    trace_wave(( "waveOutOpen( %x, %x, %x, %x, %x, %x )",
                 DWORD32( parg16->f1 ), INT32  ( parg16->f2 ),
                 DWORD32( parg16->f3 ), DWORD32( parg16->f4 ),
                 DWORD32( parg16->f5 ), DWORD32( parg16->f6 ) ));

    /*
    ** Get the device ID. We use INT32 here not UINT32 to make sure that
    ** negative values (such as WAVE_MAPPER (-1)) get thunked correctly.
    ** Also, get the flags to be used.
    */
    uDevID = (UINT)INT32(parg16->f2);
    dwFlags = DWORD32(parg16->f6);


    /*
    ** Get a pointer to the WAVEFORMAT structure.  Because the format of this
    ** structure is exactly the same in 32 and 16 bit land I will use
    ** GETMISCPTR to get a generic pointer to the data.  The stuff being
    ** pointed to could be full of unaligned WORDs, but the 32 bit code
    ** would have to cope with this anyway.
    */
    GETMISCPTR( parg16->f3, lpWaveformData );
    if ( lpWaveformData == (PWAVEFORMAT16)NULL ) {
        ul = (ULONG)MMSYSERR_INVALPARAM;
        goto exit_function;
    }


    /*
    ** We don't need a callback routine when the WAVE_FORMAT_QUERY flag
    ** is specified.
    */
    if ( !(dwFlags & WAVE_FORMAT_QUERY) ) {

        /*
        ** Map the 16 bit pointer is one was specified.
        */
        MMGETOPTPTR( parg16->f1, sizeof(HWAVEOUT), lphWaveOut );
        if ( lphWaveOut == NULL ) {

            ul = MMSYSERR_INVALPARAM;
        }

        /*
        ** Create InstanceData block to be used by our callback routine.
        **
        ** NOTE: Although we malloc it here we don't free it.
        ** This is not a mistake - it must not be freed before the
        ** callback routine has used it - so it does the freeing.
        **
        ** If the malloc fails we bomb down to the bottom,
        ** set ul to MMSYSERR_NOMEM and exit gracefully.
        **
        ** We always have a callback functions.  This is to ensure that
        ** the WAVEHDR structure keeps getting copied back from
        ** 32 bit space to 16 bit, as it contains flags which
        ** applications are liable to keep checking.
        */
        else if ( pInstanceData = malloc_w(sizeof(INSTANCEDATA) ) ) {

            DWORD dwNewFlags = CALLBACK_FUNCTION;

            dprintf2(( "WM32waveOutOpen: Allocated instance buffer at %8X",
                       pInstanceData ));
            pInstanceData->dwCallback         = DWORD32(parg16->f4);;
            pInstanceData->dwCallbackInstance = DWORD32(parg16->f5);
            pInstanceData->dwFlags            = dwFlags;

            dwNewFlags |= (dwFlags & WAVE_ALLOWSYNC);

            ul = GETWORD16((*mmAPI)( &Hand32, uDevID,
                                     (LPWAVEFORMAT)lpWaveformData,
                                     (DWORD)W32CommonDeviceOpen,
                                     (DWORD)pInstanceData, dwNewFlags ));
            /*
            ** If the call returns success update the 16 bit handle,
            ** otherwise don't, and free the memory we malloc'd earlier, as
            ** the callback that would have freed it will never get callled.
            */
            if ( ul == MMSYSERR_NOERROR ) {

                HWAVEOUT16 Hand16 = GETHWAVEOUT16(Hand32);

                trace_wave(( "Handle -> %x", Hand16 ));

                STOREWORD  ( *lphWaveOut, Hand16);
                FLUSHVDMPTR( DWORD32(parg16->f1), sizeof(HWAVEOUT),
                             lphWaveOut );
            }
            else {

                dprintf2(( "WM32waveOutOpen: Freeing instance buffer at %8X",
                           pInstanceData ));
                free_w(pInstanceData);
            }

            /*
            ** Regardless of sucess or failure we need to free the pointer
            ** to the 16 bit WaveOut handle.
            */
            FREEVDMPTR ( lphWaveOut );
        }
        else {
            ul = (ULONG)MMSYSERR_NOMEM;
        }
    }
    else {
        ul = GETWORD16((*mmAPI)( NULL, uDevID, (LPWAVEFORMAT)lpWaveformData,
                                 DWORD32(parg16->f4), DWORD32(parg16->f5),
                                 dwFlags ));
    }

    /*
    ** Regardless of sucess or failure we need to free the pointer to the
    ** 16 bit WaveFormatData.
    */
    FREEMISCPTR( lpWaveformData );


exit_function:
    trace_wave(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutClose
*
* This function closes the specified waveform output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutClose(PVDMFRAME pFrame)
{
    register PWAVEOUTCLOSE16 parg16;
    static   FARPROC         mmAPI = NULL;
             ULONG           ul;

    GET_MULTIMEDIA_API( "waveOutClose", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTCLOSE16), parg16);

    trace_wave(( "waveOutClose( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)( HWAVEOUT32(parg16->f1) ));
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutPrepareHeader
*
* This function prepares the specified waveform header.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutPrepareHeader(PVDMFRAME pFrame)
{
    register PWAVEOUTPREPAREHEADER3216 parg16;
    static   FARPROC                   mmAPI = NULL;
             ULONG                     ul;
             WAVEHDR                   wavehdr;


    GET_MULTIMEDIA_API( "waveOutPrepareHeader", mmAPI, MMSYSERR_NODRIVER );
    GETARGPTR(pFrame, sizeof(WAVEOUTPREPAREHEADER3216), parg16);
    trace_wave(( "waveOutPrepareHeader( %x %x %x)", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), WORD32( parg16->f3 ) ));

    GETWAVEHDR16(parg16->f2, &wavehdr);

    ul = GETWORD16((*mmAPI)( HWAVEOUT32(parg16->f1),
                             &wavehdr, WORD32(parg16->f3) ) );
    /*
    ** Only update the 16 bit structure if the call returns success
    **
    */
    if ( !ul ) {
        PUTWAVEHDR16(parg16->f2, &wavehdr);
    }

    trace_wave(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutUnprepareHeader
*
* This function prepares the specified waveform header.
* This function cleans up the preparation performed by waveOutPrepareHeader.
* The function must be called after the device driver has finished with a
* data block. You must call this function before freeing the data buffer.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutUnprepareHeader(PVDMFRAME pFrame)
{
    register PWAVEOUTUNPREPAREHEADER3216  parg16;
    static   FARPROC                      mmAPI = NULL;
             ULONG                        ul;
             WAVEHDR                      wavehdr;

    GET_MULTIMEDIA_API( "waveOutUnprepareHeader", mmAPI, MMSYSERR_NODRIVER );
    GETARGPTR(pFrame, sizeof(WAVEOUTUNPREPAREHEADER3216), parg16);
    trace_wave(( "waveOutUnprepareHeader( %x %x %x)", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), WORD32( parg16->f3 ) ));

    GETWAVEHDR16(parg16->f2, &wavehdr);

    ul = GETWORD16((*mmAPI)( HWAVEOUT32(parg16->f1),
                             &wavehdr, WORD32(parg16->f3) ) );
    /*
    ** Only update the 16 bit structure if the call returns success
    */
    if (!ul) {
        PUTWAVEHDR16(parg16->f2, &wavehdr);
    }

    trace_wave(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutWrite
*
* This function sends a data block to the specified waveform output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutWrite(PVDMFRAME pFrame)
{
    register    PWAVEOUTWRITE16 parg16;
    static      FARPROC         mmAPI = NULL;
                ULONG           ul;
                PWAVEHDR32      pWavehdr32;

    GET_MULTIMEDIA_API( "waveOutWrite", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTWRITE16), parg16);

    trace_wave(( "waveOutWrite( %x, %x, %x)", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));

    /*
    ** If the given size of the WAVEHDR structure is too small
    ** or the lphdr is invalid return an error
    **
    */
    if ( UINT32(parg16->f3) < sizeof(WAVEHDR16)
      || HIWORD( DWORD32(parg16->f2) ) == 0 ) {

        ul = (ULONG)MMSYSERR_INVALPARAM;
    }
    else {

        if ( pWavehdr32 = malloc_w( sizeof(WAVEHDR32) ) ) {

            PWAVEHDR   lpwhdr;  /* used to simplify ptr stuff later on */
#if DBG
            AllocWaveCount++;
            dprintf2(( "W>> %8X (%d)", pWavehdr32, AllocWaveCount ));
#endif

            /* Copy across the wave header stuff.  Note that lpwhdr (a
            ** 32 bit ptr to a 32 bit wave header) is used to make the
            ** pointer stuff a bit less hairy.
            **
            ** pWavehdr32->Wavehdr    is a 32 bit wave header
            ** pWavehdr32->pWavehdr16 is a 16 bit ptr to a 16 bit wave header
            ** pWavehdr32->pWavehdr32 is a 32 bit ptr to a 16 bit wave header
            */
            lpwhdr = &(pWavehdr32->Wavehdr);
            pWavehdr32->pWavehdr16 = (PWAVEHDR16)DWORD32(parg16->f2);
            pWavehdr32->pWavehdr32 = GETWAVEHDR16(DWORD32(parg16->f2), lpwhdr);

            /*
            ** GETWAVEHDR16 can return NULL, in which case we should set
            ** lpwhdr to NULL too and call waveOutWrite only to get the
            ** correct error code.
            */
            if ( pWavehdr32->pWavehdr32 == NULL ) {
                lpwhdr = NULL;
            }

            ul = GETWORD16( (*mmAPI)( HWAVEOUT32(parg16->f1),
                                      lpwhdr, UINT32(parg16->f3) ) );

            /* If the call fails we need to free the memory we malloc'd
            ** above, as the callback that would have freed it will never
            ** get called.
            */
            if ( ul == MMSYSERR_NOERROR ) {

                /* Make sure we reflect any changes that waveOutWrite did
                ** to the WAVEHDR back to 16 bit land.
                **
                ** This is important because some apps (waveedit) poll the
                ** WHDR_DONE bit !!
                */
                COPY_WAVEOUTHDR16_FLAGS( pWavehdr32->pWavehdr32,
                                         pWavehdr32->Wavehdr );
            }
            else {
#if DBG
                AllocWaveCount--;
                dprintf2(( "W<< \t%8X (%d)", pWavehdr32,
                            AllocWaveCount ));
#endif
                free_w( pWavehdr32 );
            }
        }
        else {
            ul = (ULONG)MMSYSERR_NOMEM;
        }
    }

    trace_wave(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutPause
*
* This function pauses playback on a specified waveform output device. The
* current playback position is saved. Use wavOutRestart to resume playback from
* the current playback position.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutPause(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWAVEOUTPAUSE16 parg16;
    static   FARPROC         mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveOutPause", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTPAUSE16), parg16);

    trace_wave(( "waveOutGetPause( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)( HWAVEOUT32(parg16->f1) ));
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutRestart
*
* This function restarts a paused waveform output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutRestart(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWAVEOUTRESTART16 parg16;
    static   FARPROC           mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveOutRestart", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTRESTART16), parg16);

    trace_wave(( "waveOutRestart( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)( HWAVEOUT32(parg16->f1) ));
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutReset
*
* This function stops playback on a given waveform output device and resets the
* current position to 0. All pending playback buffers are marked as done and
* returned to the application.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutReset(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWAVEOUTRESET16 parg16;
    static   FARPROC         mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveOutReset", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTRESET16), parg16);

    trace_wave(( "waveOutReset( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16( (*mmAPI)( HWAVEOUT32(parg16->f1) ));
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutGetPosition
*
* This function retrieves the current palyback position of the specified
* waveform output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutGetPosition(PVDMFRAME pFrame)
{
    register    PWAVEOUTGETPOSITION16   parg16;
    static      FARPROC                 mmAPI = NULL;
                MMTIME                  mmtime;
                ULONG                   ul = MMSYSERR_INVALPARAM;

    GET_MULTIMEDIA_API( "waveOutGetPosition", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTGETPOSITION16), parg16);

    trace_wave(( "waveOutGetPosition( %x, %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));

    /*
    ** If the given size of the MMTIME structure is too small return an error
    **
    ** There is a problem here on MIPS.  For some reason the MIPS
    ** compiler thinks a MMTIME16 structure is 10 bytes big.  We
    ** have a pragma in wowmmed.h to align this structure on byte
    ** boundaries therefore I guess this is a compiler bug!
    **
    ** If the input structure is not large enough we return immediately
    */

#ifdef MIPS_COMPILER_PACKING_BUG
    if ( UINT32(parg16->f3) >= 8 ) {
#else
    if ( UINT32(parg16->f3) >= sizeof(MMTIME16) ) {
#endif

        ul = GETMMTIME16( parg16->f2, &mmtime);
        if ( ul == MMSYSERR_NOERROR ) {

            ul = GETWORD16((*mmAPI)( HWAVEOUT32(parg16->f1),
                                     &mmtime, sizeof(MMTIME) ));
            /*
            ** Only update the 16 bit structure if the call returns success
            **
            */
            if ( ul == MMSYSERR_NOERROR ) {
                ul = PUTMMTIME16( parg16->f2, &mmtime);
            }
        }
    }

    trace_wave(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}


/**********************************************************************\
*
* WMM32waveOutGetPitch
*
* This function queries the current pitch setting of a waveform output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutGetPitch(PVDMFRAME pFrame)
{
    register PWAVEOUTGETPITCH16 parg16;
    static   FARPROC            mmAPI = NULL;
             ULONG              ul = MMSYSERR_INVALPARAM;
             LPDWORD            lpdwPitch;
             DWORD              dwPitch;

    GET_MULTIMEDIA_API( "waveOutGetPitch", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTGETPITCH16), parg16);
    trace_wave(( "waveOutGetPitch( %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));

    ul = GETWORD16((*mmAPI)( HWAVEOUT32(parg16->f1), &dwPitch ));

    if ( ul == MMSYSERR_NOERROR ) {

        MMGETOPTPTR( parg16->f2, sizeof(DWORD), lpdwPitch);

        if ( lpdwPitch ) {
            STOREDWORD ( *lpdwPitch, dwPitch );
            FLUSHVDMPTR( DWORD32(parg16->f2), sizeof(DWORD), lpdwPitch );
            FREEVDMPTR ( lpdwPitch );
        }
        else {
            ul = MMSYSERR_INVALPARAM;
        }
    }

    trace_wave(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutSetPitch
*
* This function sets the pitch of a waveform output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutSetPitch(PVDMFRAME pFrame)
{
    register PWAVEOUTSETPITCH16 parg16;
    static   FARPROC            mmAPI = NULL;
             ULONG              ul;

    GET_MULTIMEDIA_API( "waveOutSetPitch", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTSETPITCH16), parg16);
    trace_wave(( "waveOutSetPitch( %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));

    ul = GETWORD16((*mmAPI)( HWAVEOUT32(parg16->f1), DWORD32(parg16->f2) ));

    trace_wave(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutGetVolume
*
* This function queries the current volume setting of a waveform output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutGetVolume(PVDMFRAME pFrame)
{
    register PWAVEOUTGETVOLUME16    parg16;
    static   FARPROC                mmAPI = NULL;
             ULONG                  ul;
             LPDWORD                lpdwVolume;
             DWORD                  dwVolume;

    GET_MULTIMEDIA_API( "waveOutGetVolume", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTGETVOLUME16), parg16);

    trace_wave(( "waveOutGetVolume( %x, %x )", INT32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));

    ul = GETWORD16((*mmAPI)( INT32(parg16->f1), &dwVolume ));
    if ( ul == MMSYSERR_NOERROR ) {

        MMGETOPTPTR( parg16->f2, sizeof(DWORD), lpdwVolume);

        if ( lpdwVolume ) {
            STOREDWORD ( *lpdwVolume, dwVolume );
            FLUSHVDMPTR( DWORD32(parg16->f2), sizeof(DWORD), lpdwVolume );
            FREEVDMPTR ( lpdwVolume );
        }
        else {
            ul = MMSYSERR_INVALPARAM;
        }
    }
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutSetVolume
*
* This function sets the volume of a waveform output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutSetVolume(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWAVEOUTSETVOLUME16 parg16;
    static   FARPROC             mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveOutSetVolume", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTSETVOLUME16), parg16);

    trace_wave(( "waveOutSetVolume( %x, %x )", INT32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));
    ul = GETWORD16((*mmAPI)( INT32(parg16->f1), DWORD32(parg16->f2) ));
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutGetPlaybackRate
*
* This function queries the current playback rate setting of a waveform output
* device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutGetPlaybackRate(PVDMFRAME pFrame)
{
    register PWAVEOUTGETPLAYBACKRATE16  parg16;
    static   FARPROC                    mmAPI = NULL;
             ULONG                      ul;
             LPDWORD                    lpdwRate;
             DWORD                      dwRate;

    GET_MULTIMEDIA_API( "waveOutGetPlaybackRate", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTGETPLAYBACKRATE16), parg16);
    trace_wave(( "waveOutGetPlaybackRate( %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));

    ul = GETWORD16((*mmAPI)( HWAVEOUT32(parg16->f1), &dwRate ));
    if ( ul == MMSYSERR_NOERROR ) {

       MMGETOPTPTR( parg16->f2, sizeof(DWORD), lpdwRate );

       if ( lpdwRate ) {

           STOREDWORD ( *lpdwRate, dwRate );
           FLUSHVDMPTR( DWORD32(parg16->f2), sizeof(DWORD), lpdwRate );
           FREEVDMPTR ( lpdwRate );
       }
    }
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutSetPlaybackRate
*
* This function sets the playback rate of a waveform output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutSetPlaybackRate(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWAVEOUTSETPLAYBACKRATE16 parg16;
    static   FARPROC                   mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveOutSetPlaybackRate", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTSETPLAYBACKRATE16), parg16);

    trace_wave(( "waveOutSetPlaybackRate( %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));
    ul = GETWORD16((*mmAPI)( HWAVEOUT32(parg16->f1), DWORD32(parg16->f2) ));
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}



/**********************************************************************\
*
* WMM32waveOutBreakLoop
*
* This function breaks a loop on a given waveform output device and allows
* playback to continue with the next block in the driver list.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutBreakLoop(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWAVEOUTBREAKLOOP16 parg16;
    static   FARPROC             mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveOutBreakLoop", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTBREAKLOOP16), parg16);

    trace_wave(( "waveOutBreakLoop( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)( HWAVEOUT32(parg16->f1) ));
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutGetID
*
* This function gets the device ID for a waveform output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutGetID(PVDMFRAME pFrame)
{
    register    PWAVEOUTGETID16 parg16;
    static      FARPROC         mmAPI = NULL;
                ULONG           ul;
                UINT            dwDeviceID32;
                LPWORD          lpwDeviceID16;

    GET_MULTIMEDIA_API( "waveOutGetID", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTGETID16), parg16);

    trace_wave(( "waveOutGetID( %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));

    ul = GETWORD16((*mmAPI)( HWAVEOUT32(parg16->f1), &dwDeviceID32 ));

    /*
    ** Only copy the ID back to 16 bit space if the call was sucessful
    **
    */
    if ( ul == MMSYSERR_NOERROR ) {

        MMGETOPTPTR( parg16->f2, sizeof(WORD), lpwDeviceID16 );

        if ( lpwDeviceID16 ) {
            STOREWORD  ( *lpwDeviceID16, dwDeviceID32 );
            FLUSHVDMPTR( DWORD32(parg16->f2), sizeof(WORD), lpwDeviceID16 );
            FREEVDMPTR ( lpwDeviceID16 );
        }
        else {
            ul = MMSYSERR_INVALPARAM;
        }
    }

    trace_wave(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveOutMessage
*
* This function send a message to a waveform output device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveOutMessage(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWAVEOUTMESSAGE3216 parg16;
    static   FARPROC             mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveOutMessage", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEOUTMESSAGE16), parg16);

    trace_wave(( "waveOutMessage( %x, %x, %x, %x )",
                 WORD32( parg16->f1 ),   UINT32( parg16->f2 ),
                 DWORD32( parg16->f3 ), DWORD32( parg16->f4 ) ));

    if ( (UINT32(parg16->f2) >= DRV_BUFFER_LOW)
      && (UINT32(parg16->f2) <= DRV_BUFFER_HIGH) ) {

        LPDWORD     lpdwParam1;
        GETMISCPTR(parg16->f3, lpdwParam1);

        ul = GETDWORD16((*mmAPI)( HWAVEOUT32(parg16->f1), UINT32(parg16->f2),
                                  (DWORD)lpdwParam1, DWORD32(parg16->f4) ));
        FREEMISCPTR(lpdwParam1);

    } else {
        ul = GETDWORD16((*mmAPI)( HWAVEOUT32(parg16->f1),
                                  MAKELONG( WORD32(parg16->f2), 0xFFFF ),
                                  DWORD32(parg16->f3), DWORD32(parg16->f4) ));
    }

    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/* ---------------------------------------------------------------------
** WAVE Input API's
** ---------------------------------------------------------------------
*/

/**********************************************************************\
*
* WMM32waveInGetNumDevs
*
* This function returns the number of waveform input devices.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInGetNumDevs(PVDMFRAME pFrame)
{
            ULONG   ul;
    static  FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveInGetNumDevs", mmAPI, MMSYSERR_NODRIVER );

    UNREFERENCED_PARAMETER(pFrame);

    trace_wave(( "waveInGetNumDevs()" ));
    ul = GETWORD16((*mmAPI)() );
    trace_wave(( "-> %ld\n", ul ));

    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveInGetDevCaps
*
* This function queries a specified waveform input device to determine its
* capabilities.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInGetDevCaps(PVDMFRAME pFrame)
{
    ULONG ul;
    WAVEINCAPS waveincaps1;
    register PWAVEINGETDEVCAPS16 parg16;
    static  FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveInGetDevCapsA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEINGETDEVCAPS16), parg16);

    trace_wave(( "waveInGetDevCaps( %x, %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));

    /*
    ** If the size parameter was zero return straight away.  Note that this
    ** is not an error.
    */
    if ( UINT32( parg16->f3 ) == 0 ) {
        ul = MMSYSERR_NOERROR;
    }
    else {

        ul = GETWORD16((*mmAPI)(INT32(parg16->f1), &waveincaps1,
                                sizeof(WAVEINCAPS) ) );
        /*
        ** Don't update the 16 bit structure if the call failed
        **
        */
        if ( ul == MMSYSERR_NOERROR ) {
            ul = PUTWAVEINCAPS16(parg16->f2, &waveincaps1, UINT32(parg16->f3));
        }
    }
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);

    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveInGetErrorText
*
* This function retrieves a textual description of the error identified by the
* specified error number.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInGetErrorText(PVDMFRAME pFrame)
{
    ULONG ul = MMSYSERR_NOERROR;
    PSZ pszText;
    register PWAVEINGETERRORTEXT16 parg16;
    static  FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveInGetErrorTextA", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEINGETERRORTEXT16), parg16);

    trace_wave(( "waveInGetErrorText( %x, %x, %x )", UINT32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));

    /*
    ** Test against a zero length string and a NULL pointer.  If 0 is passed
    ** as the buffer length then the manual says we should return
    ** MMSYSERR_NOERR.  MMGETOPTPTR only returns a pointer if parg16->f2 is
    ** not NULL.
    */
    MMGETOPTPTR( parg16->f2, UINT32(parg16->f3), pszText );
    if ( pszText != NULL ) {

        ul = GETWORD16((*mmAPI)( UINT32(parg16->f1), pszText,
                                 UINT32(parg16->f3) ));

        FLUSHVDMPTR( DWORD32(parg16->f2), UINT32(parg16->f3), pszText);
        FREEVDMPTR(pszText);
    }

    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveInOpen
*
* This function opens a specified waveform input device for recording.
*
* As of November 1992 we map the 16 bit Wave Format data directly to the
* the 32 bit side, no thunking of the parameters is performed.
*
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInOpen(PVDMFRAME pFrame)
{
    ULONG         ul=0;
    UINT          uDevID;
    PINSTANCEDATA pInstanceData = NULL;
    DWORD         dwFlags;
    PWAVEFORMAT16 lpWaveformData;
    LPHWAVEIN     lphWaveIn;         // pointer to handle in 16 bit app space
    HWAVEIN       Hand32;            // 32bit handle
    register PWAVEINOPEN16 parg16;
    static   FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveInOpen", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEINOPEN16), parg16);

    trace_wave(( "waveInOpen( %x, %x, %x, %x, %x, %x )",
                 DWORD32( parg16->f1 ), INT32  ( parg16->f2 ),
                 DWORD32( parg16->f3 ), DWORD32( parg16->f4 ),
                 DWORD32( parg16->f5 ), DWORD32( parg16->f6 ) ));

    /*
    ** Get the device ID. We use INT32 here not UINT32 to make sure that
    ** negative values (such as WAVE_MAPPER (-1)) get thunked correctly.
    */
    uDevID = (UINT)INT32(parg16->f2);

    /*
    ** Get the flags to be used.
    */
    dwFlags = DWORD32(parg16->f6);

    /*
    ** Get a pointer to the WAVEFORMAT structure.  Because the format of this
    ** structure is exactly the same in 32 and 16 bit land I will use
    ** GETMISCPTR to get a generic pointer to the data.  The stuff being
    ** pointed to could be full of unaligned WORDs, but the 32 bit code
    ** would have to cope with this anyway.
    */
    GETMISCPTR( DWORD32(parg16->f3), lpWaveformData );
    if ( lpWaveformData == (PWAVEFORMAT16)NULL ) {
        ul = (ULONG)MMSYSERR_INVALPARAM;
        goto exit_function;
    }

    /*
    ** We don't need a callback routine when the WAVE_FORMAT_QUERY flag
    ** is specified.
    */
    if ( !( dwFlags & WAVE_FORMAT_QUERY ) ) {

        /*
        ** Map the 16 bit pointer is one was specified.
        */
        MMGETOPTPTR( parg16->f1, sizeof(HWAVEIN), lphWaveIn );
        if ( lphWaveIn == NULL ) {

            ul = MMSYSERR_INVALPARAM;
        }

        /*
        ** Create InstanceData block to be used by our callback routine.
        **
        ** NOTE: Although we malloc it here we don't free it.
        ** This is not a mistake - it must not be freed before the
        ** callback routine has used it - so it does the freeing.
        **
        ** If the malloc fails we bomb down to the bottom,
        ** set ul to MMSYSERR_NOMEM and exit gracefully.
        **
        ** We always have a callback functions.  This is to ensure that
        ** the WAVEHDR structure keeps getting copied back from
        ** 32 bit space to 16 bit, as it contains flags which
        ** applications are liable to keep checking.
        */
        else if ( pInstanceData = malloc_w(sizeof(INSTANCEDATA) ) ) {

            DWORD   dwNewFlags = CALLBACK_FUNCTION;

            dprintf2(( "WM32waveInOpen: Allocated instance buffer at %8X",
                       pInstanceData ));
            pInstanceData->dwCallback         = DWORD32(parg16->f4);;
            pInstanceData->dwCallbackInstance = DWORD32(parg16->f5);
            pInstanceData->dwFlags            = dwFlags;

            dwNewFlags |= (dwFlags & WAVE_ALLOWSYNC);

            ul = GETWORD16( (*mmAPI)( &Hand32, uDevID,
                                      (LPWAVEFORMAT)lpWaveformData,
                                      (DWORD)W32CommonDeviceOpen,
                                      (DWORD)pInstanceData, dwNewFlags ) );
            /*
            ** If the call returns success update the 16 bit handle,
            ** otherwise don't, and free the memory we malloc'd earlier, as
            ** the callback that would have freed it will never get callled.
            */
            if ( ul == MMSYSERR_NOERROR ) {

                HWAVEIN16 Hand16 = GETHWAVEIN16(Hand32);

                trace_wave(( "Handle -> %x", Hand16 ));
                STOREWORD  ( *lphWaveIn, Hand16 );
                FLUSHVDMPTR( DWORD32(parg16->f1), sizeof(HWAVEIN), lphWaveIn );
            }
            else {

                free_w(pInstanceData);
            }

            /*
            ** Regardless of sucess or failure we need to free the pointer
            ** to the 16 bit WaveIn handle.
            */
            FREEVDMPTR ( lphWaveIn );
        }
        else {
            ul = (ULONG)MMSYSERR_NOMEM;
        }
    }
    else {

        ul = GETWORD16( (*mmAPI)( NULL, uDevID, (LPWAVEFORMAT)lpWaveformData,
                                  DWORD32(parg16->f4),
                                  DWORD32(parg16->f5), dwFlags) );
    }

    /*
    ** Regardless of sucess or failure we need to free the pointer to the
    ** 16 bit WaveFormatData.
    */
    FREEMISCPTR( lpWaveformData );


exit_function:
    trace_wave(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveInClose
*
* This function closes the specified waveform input device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInClose(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWAVEINCLOSE16 parg16;
    static  FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveInClose", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEINCLOSE16), parg16);

    trace_wave(( "waveInClose( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)( HWAVEIN32(parg16->f1) ));
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}


/**********************************************************************\
*
* WMM32waveInPrepareHeader
*
* This function prepares the specified waveform header.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInPrepareHeader(PVDMFRAME pFrame)
{
    register PWAVEINPREPAREHEADER3216 parg16;
    static   FARPROC                  mmAPI = NULL;
             ULONG                    ul;
             WAVEHDR                  wavehdr;


    GET_MULTIMEDIA_API( "waveInPrepareHeader", mmAPI, MMSYSERR_NODRIVER );
    GETARGPTR(pFrame, sizeof(WAVEINPREPAREHEADER3216), parg16);
    trace_wave(( "waveInPrepareHeader( %x %x %x)", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), WORD32( parg16->f3 ) ));

    GETWAVEHDR16(parg16->f2, &wavehdr);

    ul = GETWORD16((*mmAPI)( HWAVEIN32(parg16->f1),
                             &wavehdr, WORD32(parg16->f3) ) );
    /*
    ** Only update the 16 bit structure if the call returns success
    **
    */
    if ( !ul ) {
        PUTWAVEHDR16(parg16->f2, &wavehdr);
    }

    trace_wave(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveInUnprepareHeader
*
* This function prepares the specified waveform header.
* This function cleans up the preparation performed by waveInPrepareHeader.
* The function must be called after the device driver has finished with a
* data block. You must call this function before freeing the data buffer.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInUnprepareHeader(PVDMFRAME pFrame)
{
    register PWAVEINUNPREPAREHEADER3216  parg16;
    static   FARPROC                     mmAPI = NULL;
             ULONG                       ul;
             WAVEHDR                     wavehdr;

    GET_MULTIMEDIA_API( "waveInUnprepareHeader", mmAPI, MMSYSERR_NODRIVER );
    GETARGPTR(pFrame, sizeof(WAVEINUNPREPAREHEADER3216), parg16);
    trace_wave(( "waveInUnprepareHeader( %x %x %x)", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), WORD32( parg16->f3 ) ));

    GETWAVEHDR16(parg16->f2, &wavehdr);

    ul = GETWORD16((*mmAPI)( HWAVEIN32(parg16->f1),
                             &wavehdr, WORD32(parg16->f3) ) );
    /*
    ** Only update the 16 bit structure if the call returns success
    */
    if (!ul) {
        PUTWAVEHDR16(parg16->f2, &wavehdr);
    }

    trace_wave(( "-> %ld\n", ul ));
    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveInAddBuffer
*
* This function sends an input buffer to a waveform input device.
* When the buffer is filled it is sent back to the application.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInAddBuffer(PVDMFRAME pFrame)
{
    ULONG ul;
    PWAVEHDR32 pWavehdr32;
    register PWAVEINADDBUFFER16 parg16;
    static  FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveInAddBuffer", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEINADDBUFFER16), parg16);

    trace_wave(( "waveInAddBuffer( %x, %x, %x)", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));

    /*
    ** If the given size of the WAVEHDR structure is too small
    ** or the lphdr is invalid return an error
    **
    */
    if ( UINT32(parg16->f3) < sizeof(WAVEHDR16)
      || HIWORD( DWORD32(parg16->f2) ) == 0 ) {

        ul = (ULONG)MMSYSERR_INVALPARAM;
    }
    else {
        if (pWavehdr32 = malloc_w(sizeof(WAVEHDR32) ) ) {

            PWAVEHDR   lpwhdr;  /* used to simplify ptr stuff later on */
#if DBG
            AllocWaveCount++;
            dprintf2(( "W>> %8X (%d)", pWavehdr32, AllocWaveCount ));
#endif

            /* Copy across the wave header stuff.  Note that lpwhdr (a
            ** 32 bit ptr to a 32 bit wave header) is used to make the
            ** pointer stuff a bit less hairy.
            **
            ** pWavehdr32->Wavehdr    is a 32 bit wave header
            ** pWavehdr32->pWavehdr16 is a 16 bit ptr to a 16 bit wave header
            ** pWavehdr32->pWavehdr32 is a 32 bit ptr to a 16 bit wave header
            */
            lpwhdr = &(pWavehdr32->Wavehdr);
            pWavehdr32->pWavehdr16 = (PWAVEHDR16)DWORD32(parg16->f2);
            pWavehdr32->pWavehdr32 = GETWAVEHDR16(DWORD32(parg16->f2), lpwhdr);

            /*
            ** GETWAVEHDR16 can return NULL, in which case we should set
            ** lpwhdr to NULL too and call waveInAddBuffer only to get the
            ** correct error code.
            */
            if ( pWavehdr32->pWavehdr32 == NULL ) {
                lpwhdr = NULL;
            }

            ul = GETWORD16( (*mmAPI)( HWAVEIN32(parg16->f1),
                                      lpwhdr, UINT32(parg16->f3) ) );
            /*
            ** If the call fails we need to free the memory we malloc'd
            ** above, as the callback that would have freed it will never
            ** get called.
            **
            */
            if ( ul == MMSYSERR_NOERROR ) {

                /*
                ** Make sure we reflect any changes that waveInAddBuffer did
                ** to the WAVEHDR back to 16 bit land.
                **
                ** This is important because some apps (waveedit) poll the
                ** WHDR_DONE bit !!
                */
                COPY_WAVEINHDR16_FLAGS( pWavehdr32->pWavehdr32,
                                        pWavehdr32->Wavehdr );
            }
            else {
#if DBG
                AllocWaveCount--;
                dprintf2(( "W<< \t%8X (%d)", pWavehdr32,
                            AllocWaveCount ));
#endif
                free_w( pWavehdr32 );
            }
        }
        else {
            ul = (ULONG)MMSYSERR_NOMEM;
        }
    }
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveInStart
*
* This function starts input on the specified waveform input device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInStart(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWAVEINSTART16 parg16;
    static  FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveInStart", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEINSTART16), parg16);

    trace_wave(( "waveInStart( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)( HWAVEIN32(parg16->f1) ));
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveInStop
*
* This function stops waveform input.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInStop(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWAVEINSTOP16 parg16;
    static  FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveInStop", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEINSTOP16), parg16);

    trace_wave(( "waveInStop( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)( HWAVEIN32(parg16->f1) ));
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveInReset
*
* This function stops input on a given waveform input device and resets the
* current position to 0. All pending buffers are marked as done and returned
* to the application.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInReset(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWAVEINRESET16 parg16;
    static  FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveInReset", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEINRESET16), parg16);

    trace_wave(( "waveInReset( %x )", WORD32( parg16->f1 ) ));
    ul = GETWORD16((*mmAPI)( HWAVEIN32(parg16->f1) ));
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveInGetPosition
*
* This function retrieves the current input position of the specified waveform
* input device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInGetPosition(PVDMFRAME pFrame)
{
    register PWAVEINGETPOSITION16   parg16;
             MMTIME                 mmtime;
             ULONG                  ul = MMSYSERR_INVALPARAM;
    static  FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveInGetPosition", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEINGETPOSITION16), parg16);

    trace_wave(( "waveInGetPosition( %x, %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ), UINT32( parg16->f3 ) ));

    /*
    ** If the given size of the MMTIME structure is too small return an error
    **
    ** There is a problem here on MIPS.  For some reason the MIPS
    ** compiler thinks a MMTIME16 structure is 10 bytes big.  We
    ** have a pragma in wowmmed.h to align this structure on byte
    ** boundaries therefore I guess this is a compiler bug!
    **
    ** If the input structure is not large enough we return immediately
    */
#ifdef MIPS_COMPILER_PACKING_BUG
    if ( UINT32(parg16->f3) >= 8 ) {
#else
    if ( UINT32(parg16->f3) >= sizeof(MMTIME16) ) {
#endif

        ul = GETMMTIME16( parg16->f2, &mmtime );
        if ( ul == MMSYSERR_NOERROR ) {

            ul = GETWORD16((*mmAPI)( HWAVEIN32(parg16->f1),
                                     &mmtime, sizeof(MMTIME) ));
            /*
            ** Only update the 16 bit structure if the call returns success
            **
            */
            if ( ul == MMSYSERR_NOERROR ) {
                ul = PUTMMTIME16( parg16->f2, &mmtime );
            }
        }
    }
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveInGetID
*
* This function gets the device ID for a waveform input device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInGetID(PVDMFRAME pFrame)
{
    register PWAVEINGETID16 parg16;
             ULONG          ul;
             UINT           dwDeviceID32;
             LPWORD         lpwDeviceID16;
    static  FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveInGetID", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEINGETID16), parg16);
    trace_wave(( "waveInGetID( %x, %x )", WORD32( parg16->f1 ),
                 DWORD32( parg16->f2 ) ));

    ul = GETWORD16((*mmAPI)( HWAVEIN32(parg16->f1), &dwDeviceID32 ));

    /*
    ** Only copy the ID back to 16 bit space if the call was sucessful
    **
    */
    if ( ul == MMSYSERR_NOERROR ) {

        MMGETOPTPTR( parg16->f2, sizeof(WORD), lpwDeviceID16 );

        if ( lpwDeviceID16 ) {
            STOREWORD  ( *lpwDeviceID16, dwDeviceID32 );
            FLUSHVDMPTR( DWORD32(parg16->f2), sizeof(WORD), lpwDeviceID16 );
            FREEVDMPTR ( lpwDeviceID16 );
        }
        else {
            ul = MMSYSERR_INVALPARAM;
        }
    }
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* WMM32waveInMessage
*
* This function sends a message to a waveform input device.
*
\**********************************************************************/
ULONG FASTCALL WMM32waveInMessage(PVDMFRAME pFrame)
{
    ULONG ul;
    register PWAVEINMESSAGE3216 parg16;
    static  FARPROC mmAPI = NULL;

    GET_MULTIMEDIA_API( "waveInMessage", mmAPI, MMSYSERR_NODRIVER );

    GETARGPTR(pFrame, sizeof(WAVEINMESSAGE16), parg16);


    trace_wave(( "waveInMessage( %x, %x, %x, %x )", WORD32( parg16->f1 ),
                 UINT32( parg16->f2 ), DWORD32( parg16->f3 ),
                 DWORD32( parg16->f4 ) ));

    if ( (UINT32(parg16->f2) >= DRV_BUFFER_LOW)
      && (UINT32(parg16->f2) <= DRV_BUFFER_HIGH) ) {

        LPDWORD     lpdwParam1;
        GETMISCPTR(parg16->f3, lpdwParam1);

        ul = GETDWORD16((*mmAPI)( HWAVEIN32(parg16->f1), UINT32(parg16->f2),
                                  (DWORD)lpdwParam1, DWORD32(parg16->f4) ));
        FREEMISCPTR(lpdwParam1);

    } else {
        ul = GETDWORD16((*mmAPI)( HWAVEIN32(parg16->f1),
                                  MAKELONG( WORD32(parg16->f2), 0xFFFF ),
                                  DWORD32(parg16->f3), DWORD32(parg16->f4) ));
    }
    trace_wave(( "-> %ld\n", ul ));

    FREEARGPTR(parg16);
    RETURN(ul);
}

/**********************************************************************\
*
* W32CommonDeviceOpen
*
* This routine is the callback which is ALWAYS called by wave and midi
* functions.  This is done to ensure that the XXXXHDR structure keeps
* getting copied back from 32 bit space to 16 bit, as it contains flags
* which the application is liable to keep checking.
*
* The way this whole business works is that the wave/midi data stays in 16
* bit space, but the XXXXHDR is copied to the 32 bit side, with the
* address of the data thunked accordingly so that Robin's device driver
* can still get at the data but we don't have the performance penalty of
* copying it back and forth all the time, not least because it is liable
* to be rather large...
*
* It also handles the tidying up of memory which is reserved to store
* the XXXXHDR, and the instance data (HWND/Callback address; instance
* data; flags) which the xxxxOpen calls pass to this routine, enabling
* it to forward messages or call callback as required.
*
* This routine handles all the messages that get sent from Robin's
* driver, and in fact thunks them back to the correct 16 bit form.  In
* theory there should be no MM_ format messages from the 16 bit side, so
* I can zap 'em out of WMSG16.  However the 32 bit side should thunk the
* mesages correctly and forward them to the 16 bit side and thence to
* the app.
*
* However...  I discover that somewhere in the system the wParam Msg
* parameter (which is 32 bits in Win32 and 16 bits in Win16) is having
* the top 16 bits trashed (zeroed actually).  As I pass a 32 bit handle
* in it, to be thunked in WMDISP32 back to a 16 bit handle, the loss of
* the top 16 bits is not conducive to correct thunking.  Soooo, no more
* thunking in WMDISP32 - I do it all here and Post the correct 16 bit
* message.
*
* Hence WMTBL32 (the 32 bit message switch table) contains the
* NoThunking entry for all the MM_ messages - I'll do 'em myself thank
* you.
*
*
* For the MM_WIM_DATA and MM_WOM_DONE message dwParam1 points to the
* following data struture.
*
*    P32HDR  is a 32 bit pointer to the original 16 bit header
*    P16HDR  is a 16 bit far pointer to the original 16 bit header
*
*    If we need to refernece the original header we must do via the
*    P32HDR pointer.
*
*                   +---------+
*                   | P32HDR  +----->+---------+
*                   +---------+      | 16 bit  |
*                   | P16HDR  +----->|         |    This is the original
*    dwParam1 ----->+---------+      |  Wave   |    wave header passed to
*                   | 32 bit  |      | Header  |    us by the Win 16 app.
*    This is the 32 |         |      |         |
*    bit wave       |  Wave   |      +---------+
*    header that we | Header  |
*    thunked at     |         |
*    earlier.       +---------+
*
*
* We must ensure that the 32 bit structure is completely hidden from the
* 16 bit application, ie. the 16 bit app only see's the wave header that it
* passed to us earlier.
*
*
* NOTE: dwParam2 is junk
*
*
\**********************************************************************/
VOID W32CommonDeviceOpen( HANDLE handle, UINT uMsg, DWORD dwInstance,
                        DWORD dwParam1, DWORD dwParam2 )
{
    PWAVEHDR32      pWavehdr32;
    PMIDIHDR32      pMidihdr32;
    PINSTANCEDATA   pInstanceData;
    WORD            Handle;


    switch (uMsg) {

        /* ------------------------------------------------------------
        ** MIDI INPUT MESSAGES
        ** ------------------------------------------------------------
        */

    case MM_MIM_LONGDATA:
        /*
        ** This message is sent to a window when an input buffer has been
        ** filled with MIDI system-exclusive data and is being returned to
        ** the application.
        */

    case MM_MIM_LONGERROR:
        /*
        ** This message is sent to a window when an invalid MIDI
        ** system-exclusive message is received.
        */
        pMidihdr32 = (PMIDIHDR32)( (PBYTE)dwParam1 - (sizeof(PMIDIHDR16) * 2) );
        WOW32ASSERT( pMidihdr32 );
        COPY_MIDIINHDR16_FLAGS( pMidihdr32->pMidihdr32, pMidihdr32->Midihdr );
        dwParam1 = (DWORD)pMidihdr32->pMidihdr16;


    case MM_MIM_DATA:
        /*
        ** This message is sent to a window when a MIDI message is
        ** received by a MIDI input device.
        */

    case MM_MIM_ERROR:
        /*
        ** This message is sent to a window when an invalid MIDI message
        ** is received.
        */

    case MM_MIM_OPEN:
        /*
        ** This message is sent to a window when a MIDI input device is opened.
        ** We process this message the same way as MM_MIM_CLOSE (see below)
        */

    case MM_MIM_CLOSE:
        /*
        ** This message is sent to a window when a MIDI input device is
        ** closed. The device handle is no longer valid once this message
        ** has been sent.
        */
        Handle = GETHMIDIIN16(handle);
        break;



        /* ------------------------------------------------------------
        ** MIDI OUTPUT MESSAGES
        ** ------------------------------------------------------------
        */

    case MM_MOM_DONE:
        /*
        ** This message is sent to a window when the specified
        ** system-exclusive buffer has been played and is being returned to
        ** the application.
        */
        pMidihdr32 = (PMIDIHDR32)( (PBYTE)dwParam1 - (sizeof(PMIDIHDR16) * 2) );
        WOW32ASSERT( pMidihdr32 );
        COPY_MIDIOUTHDR16_FLAGS( pMidihdr32->pMidihdr32, pMidihdr32->Midihdr );
        dwParam1 = (DWORD)pMidihdr32->pMidihdr16;


    case MM_MOM_OPEN:
        /*
        ** This message is sent to a window when a MIDI output device is opened.
        ** We process this message the same way as MM_MOM_CLOSE (see below)
        */

    case MM_MOM_CLOSE:
        /*
        ** This message is sent to a window when a MIDI output device is
        ** closed. The device handle is no longer valid once this message
        ** has been sent.
        */
        Handle = GETHMIDIOUT16(handle);
        break;



        /* ------------------------------------------------------------
        ** WAVE INPUT MESSAGES
        ** ------------------------------------------------------------
        */

    case MM_WIM_DATA:
        /*
        ** This message is sent to a window when waveform data is present
        ** in the input buffer and the buffer is being returned to the
        ** application.  The message can be sent either when the buffer
        ** is full, or after the waveInReset function is called.
        */
        pWavehdr32 = (PWAVEHDR32)( (PBYTE)dwParam1 - (sizeof(PWAVEHDR16) * 2));
        WOW32ASSERT( pWavehdr32 );
        COPY_WAVEINHDR16_FLAGS( pWavehdr32->pWavehdr32, pWavehdr32->Wavehdr );
        dwParam1 = (DWORD)pWavehdr32->pWavehdr16;

    case MM_WIM_OPEN:
        /*
        ** This message is sent to a window when a waveform input
        ** device is opened.
        **
        ** We process this message the same way as MM_WIM_CLOSE (see below)
        */

    case MM_WIM_CLOSE:
        /*
        ** This message is sent to a window when a waveform input device is
        ** closed.  The device handle is no longer valid once the message has
        ** been sent.
        */
        Handle = GETHWAVEIN16(handle);
        break;



        /* ------------------------------------------------------------
        ** WAVE OUTPUT MESSAGES
        ** ------------------------------------------------------------
        */

    case MM_WOM_DONE:
        /*
        ** This message is sent to a window when the specified output
        ** buffer is being returned to the application. Buffers are returned
        ** to the application when they have been played, or as the result of
        ** a call to waveOutReset.
        */
        pWavehdr32 = (PWAVEHDR32)( (PBYTE)dwParam1 - (sizeof(PWAVEHDR16) * 2));
        WOW32ASSERT( pWavehdr32 );
        COPY_WAVEOUTHDR16_FLAGS( pWavehdr32->pWavehdr32, pWavehdr32->Wavehdr );
        dwParam1 = (DWORD)pWavehdr32->pWavehdr16;

    case MM_WOM_OPEN:
        /*
        ** This message is sent to a window when a waveform output device
        ** is opened.
        **
        ** We process this message the same way as MM_WOM_CLOSE (see below)
        */

    case MM_WOM_CLOSE:
        /*
        ** This message is sent to a window when a waveform output device
        ** is closed.  The device handle is no longer valid once the
        ** message has been sent.
        */
        Handle = GETHWAVEOUT16(handle);
        break;

#if DBG
    default:
        dprintf(( "Unknown message received in CallBack function " ));
        dprintf(( "best call StephenE or MikeTri" ));
        return;
#endif

    }


    /*
    ** Now make the CallBack, or PostMessage call depending
    ** on the flags passed to original (wave|midi)(In|Out)Open call.
    */
    pInstanceData = (PINSTANCEDATA)dwInstance;
    WOW32ASSERT( pInstanceData );

    switch (pInstanceData->dwFlags & CALLBACK_TYPEMASK)  {

    case CALLBACK_WINDOW:
        dprintf2(( "WINDOW callback identified" ));
        PostMessage( HWND32( LOWORD(pInstanceData->dwCallback) ),
                     uMsg, Handle, dwParam1 );
        break;


    case CALLBACK_TASK:
    case CALLBACK_FUNCTION: {

        DWORD   dwFlags;

        if ( (pInstanceData->dwFlags & CALLBACK_TYPEMASK) == CALLBACK_TASK ) {
            dprintf2(( "TASK callback identified" ));
            dwFlags = DCB_TASK;
        }
        else {
            dprintf2(( "FUNCTION callback identified" ));
            dwFlags = DCB_FUNCTION;
        }

        WOW32DriverCallback( pInstanceData->dwCallback,
                             dwFlags,
                             Handle,
                             LOWORD( uMsg ),
                             pInstanceData->dwCallbackInstance,
                             dwParam1,
                             dwParam2 );

        }
        break;
    }

    /*
    ** Now, free up any storage that was allocated during the waveOutOpen
    ** and waveInOpen.  This should only be freed during the MM_WOM_CLOSE or
    ** MM_WIM_CLOSE message.
    **
    ** Also, free up any storage that was allocated during the waveOutWrite
    ** and waveInAddBuffer call.  This should only be freed during the
    ** MM_WOM_DONE or MM_WIM_DATA message.
    */
    switch (uMsg) {

    case MM_MIM_CLOSE:
    case MM_MOM_CLOSE:
    case MM_WIM_CLOSE:
    case MM_WOM_CLOSE:
        dprintf2(( "W32CommonDeviceOpen: Freeing device open buffer at %X",
                    pInstanceData ));
        dprintf2(( "Alloc Midi count = %d", AllocMidiCount ));
        dprintf2(( "Alloc Wave count = %d", AllocWaveCount ));
        free_w( pInstanceData );
        FREEHWAVEIN16( Handle );
        break;

    case MM_WIM_DATA:
    case MM_WOM_DONE:
#       if DBG
            AllocWaveCount--;
            dprintf2(( "W<< \t%8X (%d)", pWavehdr32, AllocWaveCount ));
#       endif
        free_w( pWavehdr32 );
        break;

    case MM_MIM_LONGDATA:
    case MM_MIM_LONGERROR:
    case MM_MOM_DONE:
#       if DBG
            AllocMidiCount--;
            dprintf2(( "M<< \t%8X (%d)", pMidihdr32, AllocMidiCount ));
#       endif
        free_w( pMidihdr32 );
        break;
    }

}

#endif
