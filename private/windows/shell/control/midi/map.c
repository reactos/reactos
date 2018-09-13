/*****************************************************************************
    map.c


    midi mapper run-time

    Copyright (c) Microsoft Corporation 1990-1991. All rights reserved

*****************************************************************************/

#include <windows.h>
#include <string.h>
#include <mmsystem.h>
#if defined(WIN32)
#include <port1632.h>
#endif
#include "hack.h"
#include <mmddk.h>
#include "midimap.h"
#include "midi.h"
#include "extern.h"
#include "mmreg.h"

#define ISSTATUS(bData)     ((bData) & 0x80)
#define FILTERCHANNEL(bStatus)  ((BYTE)((bStatus) & 0xf0))
#define FILTERSTATUS(bStatus)   ((BYTE)((bStatus) & 0x0f))

#define STATUS_NOTEOFF      0x80
#define STATUS_NOTEON       0x90
#define STATUS_POLYPHONICKEY    0xa0
#define STATUS_CONTROLCHANGE    0xb0
#define STATUS_PROGRAMCHANGE    0xc0
#define STATUS_CHANNELPRESSURE  0xd0
#define STATUS_PITCHBEND    0xe0

#define STATUS_SYS      0xf0
#define STATUS_SYSEX        0xf0
#define STATUS_QFRAME       0xf1
#define STATUS_SONGPOINTER  0xf2
#define STATUS_SONGSELECT   0xf3
#define STATUS_F4       0xf4
#define STATUS_F5       0xf5
#define STATUS_TUNEREQUEST  0xf6
#define STATUS_EOX      0xf7
#define STATUS_TIMINGCLOCK  0xf8
#define STATUS_F9       0xf9
#define STATUS_START        0xfa
#define STATUS_CONTINUE     0xfb
#define STATUS_STOP     0xfc
#define STATUS_FD       0xfd
#define STATUS_ACTIVESENSING    0xfe
#define STATUS_SYSTEMRESET  0xff

#define CONTROL_VOLUME      0x07

#define MIDIDATABUFFER      512

#define STATE_MAPNAILED     0x0001
#define STATE_DATANAILED    0x0002
#define STATE_CODENAILED    0x0004

/*****************************************************************************

    local structures

*****************************************************************************/

typedef unsigned char huge *     HPBYTE;

#define DEV_PREPARED    0x0001

typedef struct mididev_tag {
    WORD    wDeviceID;
    WORD    wChannel;
    WORD    fwFlags;
    HMIDIOUT   hMidi;
} MIDIDEV;
typedef MIDIDEV *PMIDIDEV;

/*****************************************************************************

    local data

*****************************************************************************/

/*
 * critical section used to protect the open so that there is no
 * window in which two threads could open simultaneously - otherwise
 * with all these statics there would be a major accident
 */
CRITICAL_SECTION MapperCritSec;




static HGLOBAL        hCurMap;         // handle of current midi map
static WORD           wChannelMask;    // which channels are on
static UINT           uPatchMask;      // which channels have patch maps
static MIDIDEV        mapdevs[16];     // contains device info. for each midi device in the current map.
static MIDIDEV        chnldevs[16];    // map channels to midi devices.
static LPMIDIPATCHMAP lpPMap;          // current patch map
static LPMIDIKEYMAP   lpKMap;          // current key map
static BYTE           curpatch[16];    // what is the currently selected patch for each channel
static BYTE           status;          // virtual running status
static BYTE           bCurrentStatus;  // Current message type
static BYTE           fActiveChannel;  // Channel message to active channel
static BYTE           bCurrentLen;     // Current message length, if any

static DWORD          OpenCallback;    // Open Callback parameter
static DWORD          OpenInstance;    // Open Instance parameter
static DWORD          OpenFlags;       // Open Param2
static HMIDIOUT       hmidiMapper;     // Handle of current mapper device
static LPMIDIHDR      pmidihdrMapper;  // Buffer used for mapped devices
static UINT           ufStateFlags;    // State flags for device
extern BYTE FAR       bMidiLengths[];  // Lengths in lengths.c
extern BYTE FAR       bSysLengths[];   // Lengths in lengths.c

#define MIDILENGTH(bStatus) (bMidiLengths[((bStatus) & 0x70) >> 4])
#define SYSLENGTH(bStatus)  (bSysLengths[(bStatus) & 0x07])

#define lpCurMap ((LPMIDIMAP)hCurMap)  // pointer to current midi map

UINT mapLockCount;

UINT FAR PASCAL modGetDevCaps(LPMIDIOUTCAPSW lpCaps, UINT uSize);
UINT FAR PASCAL modCachePatches(UINT msg, UINT uBank, LPPATCHARRAY lpPatchArray, UINT uFlags);
static UINT FAR PASCAL midiReadCurrentSetup(LPMIDIOPENDESC lpOpen, DWORD dwParam2);

LRESULT FAR PASCAL DriverProc(DWORD, HDRVR, UINT, LPARAM, LPARAM);

static void NEAR PASCAL modShortData(LPBYTE pbData);
static void NEAR PASCAL modLongData(HPBYTE pbData, DWORD dDataLength);
static void NEAR PASCAL modTranslateEvent(LPBYTE pbData, BYTE bStart, BYTE bLength);

DWORD FAR PASCAL _loadds modMessage(UINT id, UINT msg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);
static void PASCAL FAR CallbackNotification(UINT message, DWORD dwParam);
static void NEAR PASCAL modSendLongData(UINT uMessageLength, BOOL fBroadcast, BOOL force);
static  BOOL NEAR PASCAL modHeaderDone(void);


static  void PASCAL NEAR ReleaseResources(void);

static UINT PASCAL NEAR TranslateError(MMAPERR mmaperr)
{
    switch (mmaperr) {
    case MMAPERR_INVALIDPORT:
	    return MIDIERR_NODEVICE;
    case MMAPERR_MEMORY:
	return MMSYSERR_NOMEM;
    case MMAPERR_INVALIDSETUP:
	return MIDIERR_INVALIDSETUP;
    }
    return MIDIERR_NOMAP;
}

#if defined(WIN16)

static  BOOL NEAR PASCAL GlobalNail(
    HGLOBAL hSegment,
    UINT    uFlag)
{
    if (GlobalWire(hSegment)) {
	if (GlobalPageLock(hSegment)) {
	    ufStateFlags |= uFlag;
	    return TRUE;
	}
	GlobalUnWire(hSegment);
    }
    return FALSE;
}

static  void NEAR PASCAL GlobalUnNail(
    HGLOBAL hSegment,
    UINT    uFlag)
{
    if (ufStateFlags & uFlag) {
	GlobalPageUnlock(hSegment);
	GlobalUnWire(hSegment);
	ufStateFlags &= ~uFlag;
    }
}
#endif //WIN16

static  void PASCAL NEAR ReleaseResources(void)
{
    WORD    wDevice;

#ifdef WIN16
    GlobalUnNail((HGLOBAL)HIWORD((DWORD)(LPVOID)&hCurMap), STATE_DATANAILED);
    GlobalUnNail(hCurMap, STATE_MAPNAILED);
    GlobalUnNail((HGLOBAL)HIWORD(DriverProc), STATE_CODENAILED);
#endif // WIN16

    for (wDevice = 0; (wDevice < 16) && (mapdevs[wDevice].wDeviceID != (WORD)(-1)); wDevice++) {
	if (mapdevs[wDevice].hMidi) {
	    midiOutReset(mapdevs[wDevice].hMidi);
	    if (mapdevs[wDevice].fwFlags & DEV_PREPARED) {
		midiOutUnprepareHeader(mapdevs[wDevice].hMidi, pmidihdrMapper, sizeof(MIDIHDR));
		mapdevs[wDevice].fwFlags &= ~DEV_PREPARED;
	    }
	    midiOutClose(mapdevs[wDevice].hMidi);
	    mapdevs[wDevice].hMidi = NULL;
	    mapdevs[wDevice].wDeviceID = (WORD)(-1);
	}
    }
    if (hCurMap) {
	GlobalFree(hCurMap);
	hCurMap = NULL;
    }
    if (pmidihdrMapper) {
	HGLOBAL hmem = GlobalHandle( pmidihdrMapper );
	GlobalUnlock( hmem );
	GlobalFree( hmem );
	pmidihdrMapper = NULL;
    }
}

static  UINT PASCAL FAR CloseMidiDevice(
    void)
{
    ReleaseResources();
    CallbackNotification(MOM_CLOSE, 0);
    return 0;
}

static  void PASCAL FAR CallbackNotification(
    UINT    message,
    DWORD   dwParam)
{
    if (OpenCallback)
	DriverCallback( OpenCallback
		      , HIWORD(OpenFlags) | DCB_NOSWITCH
		      , (HANDLE)hmidiMapper
		      , message
		      , OpenInstance
		      , dwParam
		      , 0
		      );
}

/***************************************************************************
 * @doc INTERNAL
 *
 * @api LRESULT | DriverProc | The entry point for an installable driver.
 *
 * @parm DWORD | dwDriverId | For most messages, <p dwDriverId> is the DWORD
 *     value that the driver returns in response to a <m DRV_OPEN> message.
 *     Each time that the driver is opened, through the <f DrvOpen> API,
 *     the driver receives a <m DRV_OPEN> message and can return an
 *     arbitrary, non-zero value. The installable driver interface
 *     saves this value and returns a unique driver handle to the
 *     application. Whenever the application sends a message to the
 *     driver using the driver handle, the interface routes the message
 *     to this entry point and passes the corresponding <p dwDriverId>.
 *     This mechanism allows the driver to use the same or different
 *     identifiers for multiple opens but ensures that driver handles
 *     are unique at the application interface layer.
 *
 *     The following messages are not related to a particular open
 *     instance of the driver. For these messages, the dwDriverId
 *     will always be zero.
 *
 *         DRV_LOAD, DRV_FREE, DRV_ENABLE, DRV_DISABLE, DRV_OPEN
 *
 * @parm HDRVR  | hDriver | This is the handle returned to the
 *     application by the driver interface.
 *
 * @parm UINT | wMessage | The requested action to be performed. Message
 *     values below <m DRV_RESERVED> are used for globally defined messages.
 *     Message values from <m DRV_RESERVED> to <m DRV_USER> are used for
 *     defined driver protocols. Messages above <m DRV_USER> are used
 *     for driver specific messages.
 *
 * @parm LPARAM | lParam1 | Data for this message.  Defined separately for
 *     each message
 *
 * @parm LPARAM | lParam2 | Data for this message.  Defined separately for
 *     each message
 *
 * @rdesc Defined separately for each message.
 ***************************************************************************/

LRESULT FAR PASCAL DriverProc(DWORD dwDriverID, HDRVR hDriver, UINT wMessage, LPARAM lParam1, LPARAM lParam2)
{
    //
    //  NOTE DS is not valid here.
    //
    switch (wMessage) {

	case DRV_LOAD:
	    InitializeCriticalSection(&MapperCritSec);
	    return (LRESULT)TRUE;

	case DRV_FREE:
	    DeleteCriticalSection(&MapperCritSec);
	    return (LRESULT)TRUE;

	case DRV_OPEN:
	case DRV_CLOSE:
	    return (LRESULT)TRUE;
	case DRV_INSTALL:
	case DRV_REMOVE:
	    return (LRESULT)DRVCNF_RESTART;

	default:
	    return DefDriverProc(dwDriverID, hDriver, wMessage,lParam1,lParam2);
	}
}

DWORD FAR PASCAL _loadds modMessage(UINT id, UINT msg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2)
{
int         i;
DWORD       dResult;

    // this driver only supports one device
    if (id != 0)
	return MMSYSERR_BADDEVICEID;

    switch (msg) {

       case MODM_GETNUMDEVS:
	    return 1;

	case MODM_GETDEVCAPS:
	    return modGetDevCaps((LPMIDIOUTCAPSW)dwParam1, LOWORD(dwParam2));

	case MODM_OPEN:
	    EnterCriticalSection(&MapperCritSec);
	    if( hCurMap || mapLockCount ) {
		dResult = MMSYSERR_ALLOCATED;
	    } else {
		dResult =  midiReadCurrentSetup((LPMIDIOPENDESC)dwParam1, dwParam2);
	    }
	    LeaveCriticalSection(&MapperCritSec);
	    return dResult;

	case MODM_CLOSE:
	    EnterCriticalSection(&MapperCritSec);
	    dResult = CloseMidiDevice();
	    LeaveCriticalSection(&MapperCritSec);
	    return dResult;

	case MODM_CACHEPATCHES:
	case MODM_CACHEDRUMPATCHES:
	    return modCachePatches(msg, HIWORD(dwParam2), (LPPATCHARRAY)dwParam1, LOWORD(dwParam2));

///////////////////////////////////////////////////////////////////////////
//
//  INTERRUPT TIME CODE
//
//  MODM_LONGDATA, MODM_DATA, and MODM_RESET are callable at interupt time!
//
///////////////////////////////////////////////////////////////////////////

	case MODM_DATA:
	    modShortData((LPBYTE)&dwParam1);
	    return 0;

	case MODM_LONGDATA:
	    modLongData( (HPBYTE)((LPMIDIHDR)dwParam1)->lpData
		       , ((LPMIDIHDR)dwParam1)->dwBufferLength
		       );
	    ((LPMIDIHDR)dwParam1)->dwFlags |= MHDR_DONE;
	    CallbackNotification(MOM_DONE, dwParam1);
	    return 0;

///////////////////////////////////////////////////////////////////////////

	case MODM_PREPARE:
	case MODM_UNPREPARE:
	    return MMSYSERR_NOTSUPPORTED;

	//case MODM_RESET:
	//case MODM_GETVOLUME:
	//case MODM_SETVOLUME:

	default:
	    //
	    //  !!!this is fucked if a map goes to multiple physical devices
	    //  we return the *last* dResult, this is
	    //  totally random for some messages (like MODM_GETVOLUME).

	    // pass the message on un-translated to all mapped physical
	    // devices.
	    //
	    for (dResult = 0, i = 0; i < 16 && mapdevs[i].hMidi; i++)
		switch (msg) {
		//
		// Avoid nasty overlaps with open devices
		//
		case MODM_GETVOLUME:
		    dResult = midiOutGetVolume((HMIDIOUT)(mapdevs[i].wDeviceID), (LPDWORD)dwParam1);
		    break;

		case MODM_SETVOLUME:
		    dResult = midiOutSetVolume((HMIDIOUT)(mapdevs[i].wDeviceID), dwParam1);
		    break;

		default:
		    dResult = midiOutMessage(mapdevs[i].hMidi, msg, dwParam1, dwParam2);
		    break;
		}

	    return dResult;
    }
}

/*****************************************************************************
 * @doc EXTERNAL  MIDI
 *
 * @api UINT | modGetDevCaps | This function returns the mappers device caps
 *
 * @parm LPMIDIOUTCAPS | lpCaps | Specifies a far pointer to a <t MIDIOUTCAPS>
 *   structure.  This structure is filled with information about the
 *   capabilities of the device.
 *
 * @parm UINT | wSize | Specifies the size of the <t MIDIOUTCAPS> structure.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.  Possible error returns are:
 *   @flag MMSYSERR_BADDEVICEID | Specified device ID is out of range.
 *   @flag MMSYSERR_NODRIVER | The driver was not installed.
 *
 ****************************************************************************/
UINT FAR PASCAL modGetDevCaps(LPMIDIOUTCAPSW lpCaps, UINT uSize)
{
MIDIOUTCAPSW mc;
int i;

    if (uSize != 0) {

	i=LoadStringW( hLibInst,
		       IDS_MIDIMAPPER,
		       mc.szPname,
		       sizeof(lpCaps->szPname) / sizeof(WCHAR) );

	mc.wMid = MM_MICROSOFT;
	mc.wPid = MM_MIDI_MAPPER;
	mc.vDriverVersion = 0x0100;
	mc.wTechnology = MOD_MAPPER;
	mc.wVoices = 0;
	mc.wNotes = 0;
	mc.wChannelMask = wChannelMask;  // 0 if mapper not opened yet
	mc.dwSupport = MIDICAPS_CACHE;

	_fmemcpy((LPSTR)lpCaps, (LPSTR)&mc, min(uSize, sizeof(mc)));
    }

return 0;
}

static void PASCAL NEAR TranslatePatchArray(
	LPPATCHARRAY    lpSource,
	LPPATCHARRAY    lpDest,
	BOOL    fToMaps)
{
	int     i;

	_fmemset(lpDest, 0, sizeof(PATCHARRAY));
	for (i = 0; i < 16; i++) {
		UINT    curmask;
		int     j;

		curmask = 1 << i;
		if (uPatchMask & curmask) {
			lpPMap = (LPMIDIPATCHMAP)((LPSTR)lpCurMap + lpCurMap->chMap[i].oPMap);
			if (fToMaps)
				for (j = 0; j < MIDIPATCHSIZE; j++)
					lpDest[LOBYTE(lpPMap->wPMap[j])] |= (lpSource[j] & curmask) ? curmask : 0;
			else
				for (j = 0; j < MIDIPATCHSIZE; j++)
					lpDest[j] |= (lpSource[LOBYTE(lpPMap->wPMap[j])] & curmask) ? curmask : 0;
		} else
			for (j = 0; j < MIDIPATCHSIZE; j++)
				lpDest[j] |= (lpSource[j] & curmask) ? curmask : 0;
	}
}

UINT FAR PASCAL modCachePatches(UINT msg, UINT uBank, LPPATCHARRAY lpPatchArray, UINT uFlags)
{
int         i;
PATCHARRAY  patchlist;
PATCHARRAY  retpatchlist;
UINT        uResult = 0;

    TranslatePatchArray(lpPatchArray, patchlist, TRUE);

    // send to drivers
    _fmemset(retpatchlist, 0, sizeof(PATCHARRAY));

    for( i = 0; ((i < 16) && (mapdevs[i].wDeviceID != (WORD)(-1))); i++ ) {
	PATCHARRAY  curpatchlist;
	int     j;

	for (j = 0; j < MIDIPATCHSIZE; j++ )
	    curpatchlist[j] = patchlist[j] & mapdevs[i].wChannel;


	uResult = ( (msg == MODM_CACHEPATCHES)
		  ?  midiOutCachePatches( mapdevs[i].hMidi
					, uBank
					, curpatchlist
					, uFlags
					)
		  : midiOutCacheDrumPatches( mapdevs[i].hMidi
					   , uBank
					   , curpatchlist
					   , uFlags
					   )
		  );

	// combine the returned info
	for (j = 0; j < MIDIPATCHSIZE; j++ )
	    retpatchlist[j] |= (curpatchlist[j] & mapdevs[i].wChannel);
    }

    TranslatePatchArray(retpatchlist, lpPatchArray, FALSE);

    return uResult;
}



///////////////////////////////////////////////////////////////////////////
//
//  INTERRUPT TIME CODE
//
//  MODM_LONGDATA, and MODM_DATA are callable at interupt time!
//
///////////////////////////////////////////////////////////////////////////

static  BOOL NEAR PASCAL modHeaderDone(
    void)
{
    if (pmidihdrMapper->dwFlags & MHDR_DONE)
	return TRUE;
    else
	return FALSE;
}



static  void NEAR PASCAL modSendLongData(
    UINT    uMessageLength,
    BOOL    fBroadcast,
    BOOL    fForce)             // Used on final invocation
{

static BYTE LongBuffer[200];           // Cache the stuff for performance
static DWORD nLongData;                // How much we've got
static BOOL LastBroadcast;             // What we were asked to do last time
static BYTE LastStatus;                // Last status we had

    if (nLongData &&
	(fForce ||
	 FILTERSTATUS(status) != FILTERSTATUS(LastStatus) ||
	 uMessageLength + nLongData > sizeof(LongBuffer) ||
	 LastBroadcast != fBroadcast)) {

	LPBYTE lpSave = pmidihdrMapper->lpData;
	pmidihdrMapper->lpData = LongBuffer;

	pmidihdrMapper->dwBufferLength = nLongData;
	if (LastBroadcast) {
	    WORD    wDevice;

	    for (wDevice = 0; (wDevice < 16) && (mapdevs[wDevice].wDeviceID != (WORD)(-1)); wDevice++) {
		pmidihdrMapper->dwFlags &= ~MHDR_DONE;
		if (MMSYSERR_NOERROR ==
		    midiOutLongMsg(mapdevs[wDevice].hMidi,
				   pmidihdrMapper,
				   sizeof(MIDIHDR))) {
		    while (!modHeaderDone())
			Sleep(1);
		}
	    }
	} else {
	    pmidihdrMapper->dwFlags &= ~MHDR_DONE;
	    if (MMSYSERR_NOERROR ==
		midiOutLongMsg(chnldevs[FILTERSTATUS(LastStatus)].hMidi,
			       pmidihdrMapper,
			       sizeof(MIDIHDR))) {
		while (!modHeaderDone())
		    Sleep(1);
	    }
	}

	pmidihdrMapper->lpData = lpSave;
	nLongData = 0;
    }

    //
    // Pull in our new data
    //
    LastStatus = status;
    LastBroadcast = fBroadcast;
    if (fBroadcast || fActiveChannel) {
	memcpy(LongBuffer + nLongData, pmidihdrMapper->lpData, uMessageLength);
	nLongData += uMessageLength;
    }
}

static void NEAR PASCAL modTranslateEvent(
    LPBYTE  pbData,
    BYTE    bStart,
    BYTE    bLength)
{
    static BYTE fControlVol;

    if (wChannelMask & (1 << FILTERSTATUS(status))) {
	fActiveChannel = TRUE;
	bCurrentStatus = FILTERCHANNEL(status) + (BYTE)lpCurMap->chMap[FILTERSTATUS(status)].wChannel;
	if (!bStart) {
	    *(pbData++) = bCurrentStatus;
	    bStart++;
	    bLength--;
	    if (!bLength)
		return;
	}
	if (uPatchMask & (1 << FILTERSTATUS(status))) {
	    lpPMap = (LPMIDIPATCHMAP)((LPSTR)lpCurMap + lpCurMap->chMap[FILTERSTATUS(status)].oPMap);
	    switch (FILTERCHANNEL(status)) {
	    case STATUS_NOTEOFF:
	    case STATUS_NOTEON:
	    case STATUS_POLYPHONICKEY:
		if ((bStart > 1) || !lpPMap->okMaps[curpatch[FILTERSTATUS(status)]])
		    break;
		lpKMap = (LPMIDIKEYMAP)((LPSTR)lpPMap + lpPMap->okMaps[curpatch[FILTERSTATUS(status)]]);
		*pbData = lpKMap->bKMap[*pbData];
		break;
	    case STATUS_CONTROLCHANGE:
		if (bStart == 1) {
		    if (*pbData != CONTROL_VOLUME)
			break;
		    pbData++;
		    bStart++;
		    bLength--;
		    fControlVol = TRUE;
		    if (!bLength)
			return;
		}
		*pbData = (BYTE)((DWORD)*pbData * (DWORD)HIBYTE(lpPMap->wPMap[curpatch[FILTERSTATUS(status)]]) / lpPMap->bVMax);
		fControlVol = FALSE;
		break;
	    case STATUS_PROGRAMCHANGE:
		curpatch[FILTERSTATUS(status)] = *pbData;
		*pbData = (BYTE)lpPMap->wPMap[*pbData];
		break;
	    }
	}
    } else
	fActiveChannel = FALSE;
}

static  void NEAR PASCAL modShortData( LPBYTE  pbData)
{
    BOOL    fBroadcast;
    BYTE    bStart = 0;
    BYTE    bLength = 0;

    if (*pbData >= STATUS_TIMINGCLOCK)
	fBroadcast = TRUE;
    else {
	bCurrentLen = 0;
	if (ISSTATUS(*pbData)) {
	    bCurrentStatus = *pbData;
	    if (bCurrentStatus >= STATUS_SYSEX) {
		status = 0;
		fBroadcast = TRUE;
	    } else {
		status = bCurrentStatus;
		bLength = MIDILENGTH(bCurrentStatus);
		fBroadcast = FALSE;
		bStart = 0;
	    }
	} else if (!status)
	    return;
	else {
	    fBroadcast = FALSE;
	    bLength = (BYTE)(MIDILENGTH(status) - 1);
	    bStart = 1;
	}
    }
    if (fBroadcast) {
	WORD    wDevice;

	for (wDevice = 0; (wDevice < 16) && (mapdevs[wDevice].wDeviceID != (WORD)(-1)); wDevice++)
	    midiOutShortMsg(mapdevs[wDevice].hMidi, *(LPDWORD)pbData);
    } else {
	modTranslateEvent(pbData, bStart, bLength);
	if (fActiveChannel)
	    midiOutShortMsg(chnldevs[FILTERSTATUS(status)].hMidi, *(LPDWORD)pbData);
    }
}

static void NEAR PASCAL modLongData(
    HPBYTE  pbData,
    DWORD   dDataLength)
{
    static BYTE bStart;
    UINT    uMessageLength;
    LPBYTE  pbHdrData;

    pbHdrData = pmidihdrMapper->lpData;
    uMessageLength = 0;
    for (; dDataLength;) {
	if (ISSTATUS(*pbData)) {
	    if (bCurrentStatus == STATUS_SYSEX) {
		bCurrentStatus = *pbData;
		if ((bCurrentStatus == STATUS_EOX) || (bCurrentStatus == STATUS_SYSEX) || (bCurrentStatus >= STATUS_TIMINGCLOCK)) {
		    *(pbHdrData++) = bCurrentStatus;
		    dDataLength--;
		    if (bCurrentStatus >= STATUS_TIMINGCLOCK)
			bCurrentStatus = STATUS_SYSEX;
		} else
		    uMessageLength--;
	    } else {
		if (bCurrentLen) {
		    if (status) {
			BYTE    bMessageLength;

			bMessageLength = (BYTE)(MIDILENGTH(status) - bCurrentLen - bStart);
			modTranslateEvent(pmidihdrMapper->lpData - bMessageLength, bStart, bMessageLength);
		    }
		    modSendLongData(uMessageLength, !status, FALSE);
		    pbHdrData = pmidihdrMapper->lpData;
		    uMessageLength = 0;
		}
		*pbHdrData = *(pbData++);
		dDataLength--;
		if (*pbHdrData >= STATUS_TIMINGCLOCK) {
		    modSendLongData(1, TRUE, FALSE);
		    continue;
		}
		bCurrentStatus = *(pbHdrData++);
		if (bCurrentStatus >= STATUS_SYSEX) {
		    status = 0;
		    bCurrentLen = (BYTE)(SYSLENGTH(bCurrentStatus) - 1);
		} else {
		    status = bCurrentStatus;
		    bCurrentLen = (BYTE)(MIDILENGTH(bCurrentStatus) - 1);
		    bStart = 0;
		}
	    }
	} else {
	    *(pbHdrData++) = *(pbData++);
	    dDataLength--;
	    if (bCurrentLen)
		bCurrentLen--;
	    else if (status) {
		bStart = 1;
		bCurrentLen = (BYTE)(MIDILENGTH(status) - 2);
	    }

	}
	uMessageLength++;
	if (!bCurrentLen && ((bCurrentStatus != STATUS_SYSEX) || (uMessageLength == MIDIDATABUFFER))) {
	    if (status) {
		BYTE    bMessageLength;

		bMessageLength = (BYTE)(MIDILENGTH(status) - bStart);
		modTranslateEvent(pmidihdrMapper->lpData - bStart, bStart, bMessageLength);
	    }
	    modSendLongData(uMessageLength, !status, FALSE);
	    pbHdrData = pmidihdrMapper->lpData;
	    uMessageLength = 0;
	}
    }
    if (uMessageLength) {
	if (status) {
	    BYTE    bMessageLength;

	    bMessageLength = (BYTE)(MIDILENGTH(status) - bCurrentLen - bStart);
	    modTranslateEvent(pmidihdrMapper->lpData - bStart, bStart, bMessageLength);
	    bStart += bMessageLength;
	}
	modSendLongData(uMessageLength, !status, FALSE);
    }

    modSendLongData(0, 0, TRUE);
}

/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api BOOL | mapLock | This function prevents anyone from opening the
 *   mapper.
 *
 * @rdesc Returns TRUE if successful and FALSE otherwise (i.e. the mapper is
 *   already open.
 *
 * @comm This is a private function for the control panel applet to call
 *   while a setup is being edited.  There is a lock count kept - you must
 *   call mapUnlock once for each call to mapLock.
 ****************************************************************************/
BOOL FAR PASCAL mapLock(VOID)
{
    // if someone has the mapper open, return FALSE
    if( hCurMap )
	return FALSE;

    mapLockCount++;
    return TRUE;
}

/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api VOID | mapUnlock | This function unlocks the mapper if it's locked.
 *
 * @rdesc There is no return value.
 *
 * @comm This is a private function for the control panel applet to call
 *   while a setup is being edited.  There is a lock count kept but
 *   underflow will not generate an error, and lock count will remain 0.
 ****************************************************************************/
VOID FAR PASCAL mapUnlock(VOID)
{
    if( mapLockCount )
	mapLockCount--;
    return;
}

/*****************************************************************************
 * @doc INTERNAL  MIDI
 *
 * @api UINT | midiReadCurrentSetup | This function reads in the current setup.
 *
 * @parm DWORD | dwParam1 | The first DWORD from the <f midiOutOpen> call.
 *
 * @parm DWORD | dwParam2 | The second DWORD from the <f midiOutOpen> call.
 *
 * @rdesc Returns zero if the function was successful.  Otherwise, it returns
 *   an error number.
 ****************************************************************************/
static UINT FAR PASCAL midiReadCurrentSetup(LPMIDIOPENDESC lpOpen, DWORD dwParam2)
{
int      i,j;
WORD     devid;
MMAPERR  mmaperr;
DWORD    dwSize;
UINT     uResult;
char     szCurSetup[MMAP_MAXNAME];
WORD     wChan;

    // get current setup
    mmaperr = mapGetCurrentSetup(szCurSetup, MMAP_MAXNAME);
    if (mmaperr != MMAPERR_SUCCESS)
	return TranslateError(mmaperr);

    dwSize = mapGetSize(MMAP_SETUP, szCurSetup);
    if (dwSize < (DWORD)MMAPERR_MAXERROR)
	return TranslateError((UINT)dwSize);

    // allocate memory
    hCurMap = GlobalAlloc(GMEM_MOVEABLE, dwSize);
    if( !hCurMap )
	return MMSYSERR_NOMEM;

    hCurMap = (HGLOBAL)GlobalLock(hCurMap);

    mmaperr = mapRead (MMAP_SETUP, szCurSetup, lpCurMap);
    if( mmaperr != MMAPERR_SUCCESS ) {
	ReleaseResources();
	return TranslateError(mmaperr);
    }

    // initialize channel and patch masks
    wChannelMask = 0;
    uPatchMask = 0;

    // initialize device list
    for (i = 0; i < 16; i++) {
	mapdevs[i].wDeviceID = (WORD)(-1);
	mapdevs[i].hMidi = NULL;
	mapdevs[i].fwFlags = 0;
    }

    // go through each source channel
    for( wChan = 0; wChan < 16; wChan++ ) {
	if( ((lpCurMap->chMap[wChan]).dwFlags) & MMAP_ACTIVE ) {

	    // set channel mask
	    wChannelMask |= (1 << wChan);

	    // set patch mask
	    if( ((lpCurMap->chMap[wChan]).dwFlags) & MMAP_PATCHMAP )
		uPatchMask |= (1 << wChan);

	    // map device id
	    devid = lpCurMap->chMap[wChan].wDeviceID;

	    // save driver and device ids for channel messages
	    chnldevs[wChan].wDeviceID = devid;

	    // brain dead algorithm for device list
	    // wChannel will have the channel mask for the device
	    for( j = 0; j < 16; j++ ) {
		if( mapdevs[j].wDeviceID == devid ) {
		    mapdevs[j].wChannel |= 0x0001 << wChan;
		    break;     // from for loop
		}
		if( mapdevs[j].wDeviceID == (WORD)(-1) ) {
		    mapdevs[j].wDeviceID = devid;
		    mapdevs[j].wChannel = (WORD)(1 << wChan);  // first channel
		    break;
		}
	    }
	}
    }

    // create a long message buffer for translation of long messages.
    {  HANDLE hMem;
       hMem = GlobalAlloc( GMEM_MOVEABLE | GMEM_ZEROINIT
			 , (LONG)(sizeof(MIDIHDR) + MIDIDATABUFFER)
			 );
       if (hMem)
       pmidihdrMapper = ( hMem ? (LPMIDIHDR)GlobalLock(hMem) : NULL);
    }
    if (!pmidihdrMapper) {
	ReleaseResources();
	return MMSYSERR_NOMEM;
    }
    pmidihdrMapper->lpData = (LPSTR)(pmidihdrMapper + 1);
    pmidihdrMapper->dwBufferLength = MIDIDATABUFFER;

    // open all devices in new map
    for( i = 0; ((i < 16) && (mapdevs[i].wDeviceID != (WORD)(-1)) ); i++ ) {
	uResult = midiOutOpen(&mapdevs[i].hMidi,
			       mapdevs[i].wDeviceID,
			       0,
			       0,
			       dwParam2 & ~CALLBACK_TYPEMASK);

	if(uResult != 0 ){  // if any opens fail, return now
	    ReleaseResources();
	    return uResult;
	}

    uResult = midiOutPrepareHeader(mapdevs[i].hMidi, pmidihdrMapper, sizeof(MIDIHDR));
    if (uResult) {
	    ReleaseResources();
	    return uResult;
	}
	mapdevs[i].fwFlags |= DEV_PREPARED;

	for( j = 0; j < 16; j++ ) {
	    if( mapdevs[i].wDeviceID == chnldevs[j].wDeviceID )
	       chnldevs[j].hMidi = mapdevs[i].hMidi;
	}
    }

    OpenCallback = lpOpen->dwCallback;
    OpenInstance = lpOpen->dwInstance;
    OpenFlags = dwParam2;
    hmidiMapper = (HMIDIOUT)lpOpen->hMidi;
    status = 0;
    bCurrentLen = 0;
    bCurrentStatus = 0;

#if defined(WIN16)
    if (  GlobalNail((HGLOBAL)HIWORD(DriverProc), STATE_CODENAILED)
       && GlobalNail(hCurMap, STATE_MAPNAILED)
       && GlobalNail((HGLOBAL)HIWORD((DWORD)(LPVOID)&hCurMap), STATE_DATANAILED)
       )
    {
#endif //WIN16
       CallbackNotification(MOM_OPEN, 0);
       return MMSYSERR_NOERROR;
#if defined(WIN16)
    }
    ReleaseResources();
    return MMSYSERR_NOMEM;
#endif //WIN16
}
