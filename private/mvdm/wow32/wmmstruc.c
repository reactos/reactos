/*++
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1991, Microsoft Corporation
 *
 *  WMMSTRUC.C
 *
 *
 *  MultiMedia Structure copying functions (modelled after WSTRUC.C by jeffpar)
 *
 *  For input structures, there are GETxxxx16 macros;  for output structures
 *  there are PUTxxxx16 macros. Most or all of these macros will simply call
 *  the corresponding function below.
 *
 *
 *  WOW32 16-bit MultiMedia structure conversion support
 *
 *  History:
 *  Created  13-Feb-1992 by Mike Tricker (miketri)
 *  Changed  16-Jul-1992 by Mike Tricker (miketri) Sorted out the Caps structure copies
 *  Changed  08-Oct-1992 by StephenE Made the thunks safe on MIPS
 *
 *  Basically doing a GETVDMPTR of a null pointer is bad on MIPS, so is
 *  trying to get a pointer to zero bytes.  On Intel these GETXXX macros
 *  don't really do anything.
 *
--*/

#include "precomp.h"
#pragma hdrstop

#if 0
MODNAME(wmmstruc.c);




/**********************************************************************\
 * getmmtime16
 *
 * Thunks an MMTIME structure from 16 bit to 32 bit space.
 *
 * Used by:
 *          waveOutGetPosition
 *          waveInGetPosition
 *          timeGetSystemTime
 *
\**********************************************************************/
ULONG getmmtime16 (VPMMTIME16 vpmmt, LPMMTIME lpmmt)
{
    register PMMTIME16 pmmt16;

#ifdef MIPS_COMPILER_PACKING_BUG
    MMGETOPTPTR(vpmmt, 8, pmmt16);
#else
    MMGETOPTPTR(vpmmt, sizeof(MMTIME16), pmmt16);
#endif

    if ( pmmt16 == NULL ) {
        dprintf1(( "getmmtime16 MMGETOPTPTR returned a NULL pointer" ));
        return MMSYSERR_INVALPARAM;
    }

    lpmmt->wType = (UINT)FETCHWORD(pmmt16->wType);

    switch ( lpmmt->wType ) {
    case TIME_MS:
        lpmmt->u.ms = FETCHDWORD(pmmt16->u.ms);
        break;

    case TIME_SAMPLES:
        lpmmt->u.sample = FETCHDWORD(pmmt16->u.sample);
        break;

    case TIME_BYTES:
        lpmmt->u.cb = FETCHDWORD(pmmt16->u.cb);
        break;

    case TIME_SMPTE:
        lpmmt->u.smpte.hour  = pmmt16->u.smpte.hour;
        lpmmt->u.smpte.min   = pmmt16->u.smpte.min;
        lpmmt->u.smpte.sec   = pmmt16->u.smpte.sec;
        lpmmt->u.smpte.frame = pmmt16->u.smpte.frame;
        lpmmt->u.smpte.fps   = pmmt16->u.smpte.fps;
        lpmmt->u.smpte.dummy = pmmt16->u.smpte.dummy;
        break;

    case TIME_MIDI:
        lpmmt->u.midi.songptrpos = FETCHDWORD(pmmt16->u.midi.songptrpos);
        break;
    }

    FREEVDMPTR(pmmt16);
    return MMSYSERR_NOERROR;
}

/**********************************************************************\
 * Thunks an MMTIME structure from 32 bit back to 16 bit space.
 *
 * Used by:
 *          waveOutGetPosition
 *          waveInGetPosition
 *          timeGetSystemTime
\**********************************************************************/
ULONG putmmtime16 (VPMMTIME16 vpmmt, LPMMTIME lpmmt)
{
    register PMMTIME16 pmmt16;


#ifdef MIPS_COMPILER_PACKING_BUG
    MMGETOPTPTR(vpmmt, 8, pmmt16);
#else
    MMGETOPTPTR(vpmmt, sizeof(MMTIME16), pmmt16);
#endif

    if ( pmmt16 == NULL ) {
        dprintf1(( "putmmtime16 MMGETOPTPTR returned a NULL pointer" ));
        return MMSYSERR_INVALPARAM;
    }

    STOREWORD(pmmt16->wType, (WORD)lpmmt->wType);

    switch ( pmmt16->wType ) {

    case TIME_MS:
        STOREDWORD(pmmt16->u.ms, lpmmt->u.ms);
        dprintf2(( "Time in MS is %x", lpmmt->u.ms ));
        break;

    case TIME_SAMPLES:
        STOREDWORD(pmmt16->u.sample, lpmmt->u.sample);
        dprintf2(( "Time in samples is %x", lpmmt->u.sample ));
        break;

    case TIME_BYTES:
        STOREDWORD(pmmt16->u.cb, lpmmt->u.cb);
        dprintf2(( "Time in bytes is %x", lpmmt->u.cb ));
        break;

    case TIME_SMPTE:
        pmmt16->u.smpte.hour  = lpmmt->u.smpte.hour;
        pmmt16->u.smpte.min   = lpmmt->u.smpte.min;
        pmmt16->u.smpte.sec   = lpmmt->u.smpte.sec;
        pmmt16->u.smpte.frame = lpmmt->u.smpte.frame;
        pmmt16->u.smpte.fps   = lpmmt->u.smpte.fps;
        pmmt16->u.smpte.dummy = lpmmt->u.smpte.dummy;
        break;

    case TIME_MIDI:
        STOREDWORD(pmmt16->u.midi.songptrpos, lpmmt->u.midi.songptrpos);
        dprintf2(( "Time in midi is %x", lpmmt->u.midi.songptrpos ));
        break;
    }

#ifdef MIPS_COMPILER_PACKING_BUG
    FLUSHVDMPTR(vpmmt, 8, pmmt16);
#else
    FLUSHVDMPTR(vpmmt, sizeof(MMTIME16), pmmt16);
#endif
    FREEVDMPTR(pmmt16);

    return MMSYSERR_NOERROR;

}

/**********************************************************************\
* Thunks a WAVEHDR structure from 16 bit to 32 bit space.
*
* Used by:
*          waveOutPrepareHeader
*          waveOutUnprepareHeader
*          waveOutWrite
*          waveInPrepareHeader
*          waveInUnprepareHeader
*          waveInAddBuffer
*
* Returns a 32 bit pointer to the 16 bit wave header.  This wave header
* should have been locked down by wave(In|Out)PrepareHeader.  Therefore,
* it is to store this pointer for use during the WOM_DONE callback message.
*
* With the WAVEHDR and MIDIHDR structs I am assured by Robin that the ->lpNext
* field is only used by the driver, and is therefore in 32 bit space. It
* therefore doesn't matter what gets passed back and forth (I hope !).
*
\**********************************************************************/
PWAVEHDR16 getwavehdr16( VPWAVEHDR16 vpwhdr, LPWAVEHDR lpwhdr )
{
    register PWAVEHDR16 pwhdr16;

    MMGETOPTPTR(vpwhdr, sizeof(WAVEHDR16), pwhdr16);
    if ( pwhdr16 == NULL ) {
        dprintf1(( "getwavehdr16 MMGETOPTPTR returned an invalid pointer" ));
        return NULL;
    }

    if ( HIWORD(FETCHDWORD( pwhdr16->lpData )) != 0 ) {
        GETMISCPTR(pwhdr16->lpData, lpwhdr->lpData);
    }
    else {
        dprintf1(( "getwavehdr16 passed an invalid pointer to data" ));
        lpwhdr->lpData = (VPSTR)NULL;
    }

    lpwhdr->dwBufferLength  = FETCHDWORD(pwhdr16->dwBufferLength);
    dprintf4(( "getwavehdr16: buffer length = %X", lpwhdr->dwBufferLength ));

    lpwhdr->dwBytesRecorded = FETCHDWORD(pwhdr16->dwBytesRecorded);
    lpwhdr->dwUser          = FETCHDWORD(pwhdr16->dwUser);
    lpwhdr->dwFlags         = FETCHDWORD(pwhdr16->dwFlags);
    lpwhdr->dwLoops         = FETCHDWORD(pwhdr16->dwLoops);
    lpwhdr->lpNext          = (PWAVEHDR)FETCHDWORD(pwhdr16->lpNext);
    lpwhdr->reserved        = FETCHDWORD(pwhdr16->reserved);

    return pwhdr16;
}

/**********************************************************************\
* Thunks a WAVEHDR structure from 32 bit back to 16 bit space.
*
* Used by:
*          waveOutPrepareHeader
*          waveOutUnprepareHeader
*          waveOutWrite
*          waveInPrepareHeader
*          waveInUnprepareHeader
*          waveInAddBuffer
*
*
* With the WAVEHDR and MIDIHDR structs I am assured by Robin that the ->lpNext
* field is only used by the driver, and is therefore in 32 bit space. It
* therefore doesn't matter what gets passed back and forth (I hope !).
*
\**********************************************************************/
VOID putwavehdr16 (VPWAVEHDR16 vpwhdr, LPWAVEHDR lpwhdr)
{
    register PWAVEHDR16 pwhdr16;

    MMGETOPTPTR(vpwhdr, sizeof(WAVEHDR16), pwhdr16);
    if ( pwhdr16 == NULL ) {
        dprintf1(( "getwavehdr16 MMGETOPTPTR returned a NULL pointer" ));
        return;
    }

    STOREDWORD(pwhdr16->dwBufferLength,  lpwhdr->dwBufferLength);
    STOREDWORD(pwhdr16->dwBytesRecorded, lpwhdr->dwBytesRecorded);
    STOREDWORD(pwhdr16->dwUser,          lpwhdr->dwUser);
    STOREDWORD(pwhdr16->dwFlags,         lpwhdr->dwFlags);
    STOREDWORD(pwhdr16->dwLoops,         lpwhdr->dwLoops);
    STOREDWORD(pwhdr16->lpNext,          lpwhdr->lpNext);
    STOREDWORD(pwhdr16->reserved,        lpwhdr->reserved);

    FLUSHVDMPTR(vpwhdr, sizeof(WAVEHDR16), pwhdr16);
    FREEVDMPTR(pwhdr16);
}


/**********************************************************************\
 * Thunks a WAVEOUTCAPS structure from 32 bit back to 16 bit space.
 *
 * Used by:
 *          waveOutGetDevCaps
 *
 * Remember that the ->vDriverVersion is a WORD in 16bit land and a UINT in
 * 32 bit. This applies to WAVEIN/OUTCAPS, MIDIIN/OUTCAPS and AUXCAPS.
 *
\**********************************************************************/
ULONG putwaveoutcaps16 (VPWAVEOUTCAPS16 vpwoc, LPWAVEOUTCAPS lpwoc, UINT uSize)
{
    INT i;
    WAVEOUTCAPS16 Temp;
    PWAVEOUTCAPS16 pwoc16;

    /*
    ** Just in case the app specified a NULL pointer.  We have already
    ** validated that uSize is not zero.
    */

    MMGETOPTPTR( vpwoc, min(uSize, sizeof(WAVEOUTCAPS16)), pwoc16 );
    if ( pwoc16 == NULL ) {
        dprintf1(( "putwaveoutcaps16 MMGETOPTPTR returned a NULL pointer" ));
        return MMSYSERR_INVALPARAM;
    }

    STOREWORD(Temp.wMid, lpwoc->wMid);
    STOREWORD(Temp.wPid, lpwoc->wPid);
    STOREWORD(Temp.vDriverVersion, (WORD)lpwoc->vDriverVersion);

    /*
    ** The product name string should be null terminated, but we want
    ** the whole string anyway, so copy the whole MAXPNAMELEN bytes.
    */
    i = 0;
    while (i < MAXPNAMELEN) {
        Temp.szPname[i] = lpwoc->szPname[i++];
    }

    STOREDWORD(Temp.dwFormats,     lpwoc->dwFormats);
    STOREWORD(Temp.wChannels,      lpwoc->wChannels);
    STOREDWORD(Temp.dwSupport,     lpwoc->dwSupport);

    RtlCopyMemory( (LPVOID)pwoc16, &Temp, min(uSize, sizeof(WAVEOUTCAPS16)) );

    FLUSHVDMPTR(vpwoc, min(uSize, sizeof(WAVEOUTCAPS16)), pwoc16);
    FREEVDMPTR(pwoc16);

    return MMSYSERR_NOERROR;

}


/**********************************************************************\
 * Thunks a WAVEINCAPS structure from 32 bit back to 16 bit space.
 *
 * Used by:
 *          waveInGetDevCaps
 *
 * Remember that the ->vDriverVersion is a WORD in 16bit land and a UINT in
 * 32 bit. This applies to WAVEIN/OUTCAPS, MIDIIN/OUTCAPS and AUXCAPS.
 *
\**********************************************************************/
ULONG putwaveincaps16 (VPWAVEINCAPS16 vpwic, LPWAVEINCAPS lpwic, UINT uSize)
{
    INT i;
    WAVEINCAPS16 Temp;
    PWAVEINCAPS16 pwic16;

    /*
    ** Just in case the app specified a NULL pointer.  We have already
    ** validated that uSize is not zero.
    */

    MMGETOPTPTR(vpwic, min(uSize, sizeof(WAVEINCAPS16)), pwic16);
    if ( pwic16 == NULL ) {
        dprintf1(( "putwaveincaps16 MMGETOPTPTR returned a NULL pointer" ));
        return MMSYSERR_INVALPARAM;
    }

    STOREWORD(Temp.wMid,           lpwic->wMid);
    STOREWORD(Temp.wPid,           lpwic->wPid);
    STOREWORD(Temp.vDriverVersion, (WORD)lpwic->vDriverVersion);

    /*
    ** The product name string should be null terminated,
    ** but we want the whole string anyway, so copy the whole
    ** MAXPNAMELEN bytes.
    */
    i = 0;
    while (i < MAXPNAMELEN) {
        Temp.szPname[i] = lpwic->szPname[i++];
    }

    STOREDWORD(Temp.dwFormats,     lpwic->dwFormats);
    STOREWORD(Temp.wChannels,      lpwic->wChannels);

    RtlCopyMemory( (LPVOID)pwic16, &Temp, min(uSize, sizeof(WAVEINCAPS16)) );

    FLUSHVDMPTR(vpwic, min(uSize, sizeof(WAVEINCAPS16)), pwic16);
    FREEVDMPTR(pwic16);

    return MMSYSERR_NOERROR;

}

/**********************************************************************\
 * Thunks a MIDIHDR structure from 16 bit to 32 bit space.
 *
 * Used by:
 *          midiOutLongMsg
 *          midiInAddBuffer
 *          midiOutPrepareHdr
 *          midiOutUnprepareHdr
 *          midiInPrepareHdr
 *          midiInUnprepareHdr
 *
\**********************************************************************/
PMIDIHDR16 getmidihdr16 (VPMIDIHDR16 vpmhdr, LPMIDIHDR lpmhdr)
{
    PMIDIHDR16 pmhdr16;

    MMGETOPTPTR(vpmhdr, sizeof(MIDIHDR16), pmhdr16);
    if ( pmhdr16 == NULL ) {
        dprintf1(( "getmidihdr MMGETOPTPTR returned a NULL pointer" ));
        return NULL;
    }

    if ( HIWORD(FETCHDWORD( pmhdr16->lpData )) != 0 ) {
        GETMISCPTR(pmhdr16->lpData, lpmhdr->lpData);
    }
    else {
        dprintf1(( "getmidihdr16 passed a NULL pointer to data" ));
        lpmhdr->lpData = (VPSTR)NULL;
    }

    lpmhdr->dwBufferLength  = FETCHDWORD(pmhdr16->dwBufferLength);
    dprintf4(( "getmidihdr16: buffer length = %X", lpmhdr->dwBufferLength ));

    lpmhdr->dwBytesRecorded = FETCHDWORD(pmhdr16->dwBytesRecorded);
    lpmhdr->dwUser          = FETCHDWORD(pmhdr16->dwUser);
    lpmhdr->dwFlags         = FETCHDWORD(pmhdr16->dwFlags);
    lpmhdr->lpNext          = (PMIDIHDR)FETCHDWORD(pmhdr16->lpNext);
    lpmhdr->reserved        = FETCHDWORD(pmhdr16->reserved);

    return pmhdr16;
}

/**********************************************************************\
*  Thunks a MIDIHDR structure from 32 bit to 16 bit space.
*
*  Used by:
*           midiOutLongMsg
*           midiInAddBuffer
*           midiOutPrepareHdr
*           midiOutUnprepareHdr
*           midiInPrepareHdr
*           midiInUnprepareHdr
*
\**********************************************************************/
VOID putmidihdr16 (VPMIDIHDR16 vpmhdr, LPMIDIHDR lpmhdr)
{
    register PMIDIHDR16 pmhdr16;

    MMGETOPTPTR(vpmhdr, sizeof(MIDIHDR16), pmhdr16);
    if ( pmhdr16 == NULL ) {
        dprintf1(( "putmidihdr MMGETOPTPTR returned a NULL pointer" ));
        return;
    }

    STOREDWORD(pmhdr16->dwBufferLength,  lpmhdr->dwBufferLength);
    STOREDWORD(pmhdr16->dwBytesRecorded, lpmhdr->dwBytesRecorded);
    STOREDWORD(pmhdr16->dwUser,          lpmhdr->dwUser);
    STOREDWORD(pmhdr16->dwFlags,         lpmhdr->dwFlags);
    STOREDWORD(pmhdr16->lpNext,          lpmhdr->lpNext);
    STOREDWORD(pmhdr16->reserved,        lpmhdr->reserved);

    FLUSHVDMPTR(vpmhdr, sizeof(MIDIHDR16), pmhdr16);
    FREEVDMPTR(pmhdr16);
}


/**********************************************************************\
 * Thunks an AUXCAPS structure from 32 bit back to 16 bit space.
 *
 * Used by:
 *          auxGetDevCaps
 *
 * Remember that the ->vDriverVersion is a WORD in 16bit land and a UINT in
 * 32 bit. This applies to WAVEIN/OUTCAPS, MIDIIN/OUTCAPS and AUXCAPS.
 *
\**********************************************************************/
ULONG putauxcaps16 (VPAUXCAPS16 vpauxc, LPAUXCAPS lpauxc, UINT uSize)
{
    INT i;
    AUXCAPS16 Temp;
    PAUXCAPS16 pauxc16;

    /*
    ** Just in case the app specified a NULL pointer.  We have already
    ** validated that uSize is not zero.
    */

    MMGETOPTPTR(vpauxc, min(uSize, sizeof(AUXCAPS16)), pauxc16);
    if ( pauxc16 == NULL ) {
        dprintf1(( "putauxcaps16 MMGETOPTPTR returned a NULL pointer" ));
        return MMSYSERR_INVALPARAM;
    }

    STOREWORD(Temp.wMid,           lpauxc->wMid);
    STOREWORD(Temp.wPid,           lpauxc->wPid);
    STOREWORD(Temp.vDriverVersion, (WORD)lpauxc->vDriverVersion);

    /*
    ** The product name string should be null terminated,
    ** but we want the whole string anyway, so copy the whole
    ** MAXPNAMELEN bytes.
    */

    i = 0;
    while (i < MAXPNAMELEN) {
        Temp.szPname[i] = lpauxc->szPname[i++];
    }

    STOREWORD(Temp.wTechnology,     lpauxc->wTechnology);
    STOREDWORD(Temp.dwSupport,      lpauxc->dwSupport);

    RtlCopyMemory( (LPVOID)pauxc16, &Temp, min(uSize, sizeof(AUXCAPS16)) );

    FLUSHVDMPTR(vpauxc, min(uSize, sizeof(AUXCAPS16)), pauxc16);
    FREEVDMPTR(pauxc16);
    return MMSYSERR_NOERROR;
}

/**********************************************************************\
 * Thunks a TIMECAPS structure from 32 bit back to 16 bit space.
 *
 * Used by:
 *          timeGetDevCaps
 *
\**********************************************************************/
ULONG puttimecaps16(VPTIMECAPS16 vptimec, LPTIMECAPS lptimec, UINT uSize)
{
    PTIMECAPS16 ptimec16;
    TIMECAPS16  Temp;

    /*
    ** Just in case the app specified a NULL pointer.  We have already
    ** validated that uSize is not zero.
    */

    MMGETOPTPTR(vptimec, min(uSize, sizeof(TIMECAPS16)), ptimec16);
    if ( ptimec16 == NULL ) {
        dprintf1(( "puttimecaps16 MMGETOPTPTR returned a NULL pointer" ));
        return MMSYSERR_INVALPARAM;
    }

    //
    // Under NT, the minimum time period is about 15ms.  But Win3.1 on a 386
    // always returns 1ms.  Encarta doesn't even bother testing the
    // CD-ROM's speed if the minimum period is > 2ms, it just assumes
    // it is too slow.  So here we lie to WOW apps and always tell
    // them 1ms just like Win3.1.
    //      John Vert (jvert) 17-Jun-1993
    //
    STOREWORD( Temp.wPeriodMin, 1 );

    /*
    ** In windows 3.1 the wPeriodMax value is 0xFFFF which is the
    ** max value you can store in a word.  In windows NT the
    ** wPeriodMax is 0xF4240 (1000 seconds).
    **
    ** If we just cast the 32 bit value down to a 16bit value we
    ** end up with 0x4240 which very small compared to real 32 bit
    ** value.
    **
    ** Therefore I will take the minimum of wPeriodMax and 0xFFFF
    ** that way will should remain consistant with Win 3.1 if
    ** wPeriodMax is greater than 0xFFFF.
    */
    STOREWORD(Temp.wPeriodMax, (WORD)min( 0xFFFF, lptimec->wPeriodMax) );

    RtlCopyMemory( (LPVOID)ptimec16, &Temp, min(uSize, sizeof(TIMECAPS16)) );

    FLUSHVDMPTR(vptimec, min(uSize, sizeof(TIMECAPS16)), ptimec16);
    FREEVDMPTR(ptimec16);
    return MMSYSERR_NOERROR;
}




/**********************************************************************\
 * Thunks a MIDIINCAPS structure from 32 bit back to 16 bit space.
 *
 * Used by:
 *          midiInGetDevCaps
 *
 * OK - heres the scoop:
 *
 * Robin observed that it is valid (in theory) to copy back more bytes than
 * JUST those contained in the MIDIINCAPS16 structure...
 *   Unfortunately that would probably blow this lot clean out of the water, so
 * we aren't going to worry about that possibility for now.
 *  We will thunk the ENTIRE structure (not least 'cos we ALWAYS request it in
 * the 32 bit call), then copy the required number, <= sizeof(MIDIINCAPS16)
 * back to the 16 bit app.
 *
 * pTemp is the pointer to our local copy of the complete MIDIOUTCAPS16
\**********************************************************************/
ULONG putmidiincaps16 (VPMIDIINCAPS16 vpmic, LPMIDIINCAPS lpmic, UINT uSize)
{

    INT i;
    MIDIINCAPS16 Temp;
    PMIDIINCAPS16 pmic16;

    /*
    ** Just in case the app specified a NULL pointer.  We have already
    ** validated that uSize is not zero.
    */

    MMGETOPTPTR(vpmic, min(uSize, sizeof(MIDIINCAPS16)), pmic16);
    if ( pmic16 == NULL ) {
        dprintf1(( "putmidiincaps16 MMGETOPTPTR returned a NULL pointer" ));
        return MMSYSERR_INVALPARAM;
    }

    STOREWORD(Temp.wMid,           lpmic->wMid);
    STOREWORD(Temp.wPid,           lpmic->wPid);
    STOREWORD(Temp.vDriverVersion, (WORD)lpmic->vDriverVersion);

    /*
    ** The product name string should be null terminated, but we want the whole
    ** string anyway, so copy the whole MAXPNAMELEN bytes.
    */
    i = 0;
    while (i < MAXPNAMELEN) {
        Temp.szPname[i] = lpmic->szPname[i++];
    }

    RtlCopyMemory( (LPVOID)pmic16, &Temp, min(uSize, sizeof(MIDIINCAPS16)) );

    FLUSHVDMPTR(vpmic, min(uSize, sizeof(MIDIINCAPS16)), pmic16);
    FREEVDMPTR(pmic16);
    return MMSYSERR_NOERROR;
}



/**********************************************************************\
 * Thunks a MIDIOUTCAPS structure from 32 bit back to 16 bit space.
 *
 *
 * OK - heres the scoop:
 *
 * Robin observed that it is valid (in theory) to copy back more bytes than
 * JUST those contained in the MIDIOUTCAPS16 structure...
 *   Unfortunately that would probably blow this lot clean out of the water, so
 * we aren't going to worry about that possibility for now.
 *  We will thunk the ENTIRE structure (not least 'cos we ALWAYS request it in
 * the 32 bit call), then copy the required number, <= sizeof(MIDIOUTCAPS16)
 * back to the 16 bit app.
 *
 * pTemp is the pointer to our local copy of the complete MIDIOUTCAPS16
 *
 * Used by:
 *          midiOutGetDevCaps
 *
\**********************************************************************/
ULONG putmidioutcaps16 (VPMIDIOUTCAPS16 vpmoc, LPMIDIOUTCAPS lpmoc, UINT uSize)
{

    INT i;
    MIDIOUTCAPS16 Temp;
    PMIDIOUTCAPS16 pmoc16;

    /*
    ** Just in case the app specified a NULL pointer.  We have already
    ** validated that uSize is not zero.
    */

    MMGETOPTPTR(vpmoc, min(uSize, sizeof(MIDIINCAPS16)), pmoc16);
    if ( pmoc16 == NULL ) {
        dprintf1(( "putmidioutcaps16 MMGETOPTPTR returned a NULL pointer" ));
        return MMSYSERR_INVALPARAM;
    }

    STOREWORD(Temp.wMid,           lpmoc->wMid);
    STOREWORD(Temp.wPid,           lpmoc->wPid);
    STOREWORD(Temp.vDriverVersion, (WORD)lpmoc->vDriverVersion);

    /*
    ** The product name string should be null terminated, but we want the whole
    ** string anyway, so copy the whole MAXPNAMELEN bytes.
    */
    i = 0;
    while (i < MAXPNAMELEN) {
        Temp.szPname[i] = lpmoc->szPname[i++];
    }

    STOREWORD(Temp.wTechnology,  lpmoc->wTechnology);
    STOREWORD(Temp.wVoices,      lpmoc->wVoices);
    STOREWORD(Temp.wNotes,       lpmoc->wNotes);
    STOREWORD(Temp.wChannelMask, lpmoc->wChannelMask);
    STOREDWORD(Temp.dwSupport,   lpmoc->dwSupport);

    RtlCopyMemory( (LPVOID)pmoc16, &Temp, min(uSize, sizeof(MIDIOUTCAPS16)) );

    FLUSHVDMPTR(vpmoc, min(uSize, sizeof(MIDIOUTCAPS16)), pmoc16);
    FREEVDMPTR(pmoc16);
    return MMSYSERR_NOERROR;
}




/**********************************************************************\
 * Thunks a JOYCAPS structure from 32 bit back to 16 bit space.
 *
 * Used by:
 *          joyGetDevCaps
 *
\**********************************************************************/
ULONG putjoycaps16 (VPJOYCAPS16 vpjoyc, LPJOYCAPS lpjoyc, UINT uSize)
{
    INT i;
    JOYCAPS16 Temp;
    PJOYCAPS16 pjoyc16;

    /*
    ** Just in case the app specified a NULL pointer.  We have already
    ** validated that uSize is not zero.
    */

    MMGETOPTPTR(vpjoyc, min(uSize, sizeof(JOYCAPS16)), pjoyc16);
    if ( pjoyc16 == NULL ) {
        dprintf1(( "putjoycaps16 MMGETOPTPTR returned a NULL pointer" ));
        return JOYERR_PARMS;
    }

    STOREWORD(Temp.wMid,           lpjoyc->wMid);
    STOREWORD(Temp.wPid,           lpjoyc->wPid);

    /*
    ** The product name string should be null terminated,
    ** but we want the whole string anyway, so copy the
    ** whole MAXPNAMELEN bytes.
    */

    i = 0;
    while (i < MAXPNAMELEN) {
        Temp.szPname[i] = lpjoyc->szPname[i++];
    }

    STOREWORD(Temp.wXmin,       lpjoyc->wXmin);
    STOREWORD(Temp.wXmax,       lpjoyc->wXmax);
    STOREWORD(Temp.wYmin,       lpjoyc->wYmin);
    STOREWORD(Temp.wYmax,       lpjoyc->wYmax);
    STOREWORD(Temp.wZmin,       lpjoyc->wZmin);
    STOREWORD(Temp.wZmax,       lpjoyc->wZmax);
    STOREWORD(Temp.wNumButtons, lpjoyc->wNumButtons);
    STOREWORD(Temp.wPeriodMin,  lpjoyc->wPeriodMin);
    STOREWORD(Temp.wPeriodMax,  lpjoyc->wPeriodMax);

    RtlCopyMemory( (LPVOID)pjoyc16, &Temp, min(uSize, sizeof(JOYCAPS16)) );

    FLUSHVDMPTR(vpjoyc, min(uSize, sizeof(JOYCAPS16) ), pjoyc16);
    FREEVDMPTR(pjoyc16);
    return JOYERR_NOERROR;
}


/**********************************************************************\
 * Thunks a JOYINFO structure from 32 bit back to 16 bit space.
 *
 * Used by:
 *          joyGetPos
 *
\**********************************************************************/
ULONG putjoyinfo16 (VPJOYINFO16 vpjoyi, LPJOYINFO lpjoyi)
{
    PJOYINFO16 pjoyi16;

    /*
    ** Protect against NULL pointers
    */

    MMGETOPTPTR(vpjoyi, sizeof(JOYINFO16), pjoyi16);
    if ( pjoyi16 == NULL ) {
        dprintf1(( "putjoyinfo16 MMGETOPTPTR returned a NULL pointer" ));
        return JOYERR_PARMS;
    }

    STOREWORD(pjoyi16->wXpos,    lpjoyi->wXpos);
    STOREWORD(pjoyi16->wYpos,    lpjoyi->wYpos);
    STOREWORD(pjoyi16->wZpos,    lpjoyi->wZpos);
    STOREWORD(pjoyi16->wButtons, lpjoyi->wButtons);

    FLUSHVDMPTR(vpjoyi, sizeof(JOYINFO16), pjoyi16);
    FREEVDMPTR(pjoyi16);
    return JOYERR_NOERROR;
}
#endif
