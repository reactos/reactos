/*
 * MMSYSTEM - Multimedia Wine Extension ... :-)
 *
 * Copyright (C) the Wine project
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef __WINE_WINE_MMSYSTEM16_H
#define __WINE_WINE_MMSYSTEM16_H

#include "windef.h"
#include "wine/windef16.h"
#include "mmddk.h"

#include "pshpack1.h"

typedef UINT16	MMVERSION16;
typedef UINT16	MCIDEVICEID16;
typedef	UINT16	MMRESULT16;

typedef struct {
    UINT16    wType;		/* indicates the contents of the union */
    union {
	DWORD ms;		/* milliseconds */
	DWORD sample;		/* samples */
	DWORD cb;		/* byte count */
	struct {		/* SMPTE */
	    BYTE hour;		/* hours */
	    BYTE min;		/* minutes */
	    BYTE sec;		/* seconds */
	    BYTE frame;		/* frames  */
	    BYTE fps;		/* frames per second */
	    BYTE dummy;		/* pad */
	} smpte;
	struct {		/* MIDI */
	    DWORD songptrpos;	/* song pointer position */
	} midi;
    } u;
} MMTIME16,  *LPMMTIME16;

typedef struct {
    DWORD   dwDCISize;
    SEGPTR  lpszDCISectionName;
    SEGPTR  lpszDCIAliasName;
} DRVCONFIGINFO16, *LPDRVCONFIGINFO16;

/* GetDriverInfo16 references this structure, so this a struct defined
 * in the Win16 API.
 * GetDriverInfo has been deprecated in Win32.
 */
typedef struct
{
    UINT16       length;
    HDRVR16      hDriver;
    HINSTANCE16  hModule;
    CHAR         szAliasName[128];
} DRIVERINFOSTRUCT16, *LPDRIVERINFOSTRUCT16;

LRESULT   WINAPI DefDriverProc16(DWORD,HDRVR16,UINT16,LPARAM,LPARAM);
HDRVR16   WINAPI OpenDriver16(LPCSTR,LPCSTR,LPARAM);
LRESULT   WINAPI CloseDriver16(HDRVR16,LPARAM,LPARAM);
LRESULT   WINAPI SendDriverMessage16(HDRVR16,UINT16,LPARAM,LPARAM);
HMODULE16 WINAPI GetDriverModuleHandle16(HDRVR16);
HDRVR16   WINAPI GetNextDriver16(HDRVR16,DWORD);
BOOL16    WINAPI GetDriverInfo16(HDRVR16,DRIVERINFOSTRUCT16 *);

typedef void (CALLBACK *LPDRVCALLBACK16) (HDRVR16,UINT16,DWORD,DWORD,DWORD);
typedef LPDRVCALLBACK16 LPWAVECALLBACK16;

UINT16    WINAPI mmsystemGetVersion16(void);
BOOL16    WINAPI sndPlaySound16(LPCSTR,UINT16);

typedef struct {
    WORD	wMid;			/* manufacturer ID */
    WORD	wPid;			/* product ID */
    MMVERSION16	vDriverVersion;		/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];	/* product name (0 terminated string) */
    DWORD	dwFormats;		/* formats supported */
    WORD	wChannels;		/* number of sources supported */
    DWORD	dwSupport;		/* functionality supported by driver */
} WAVEOUTCAPS16, *LPWAVEOUTCAPS16;

typedef struct {
    WORD	wMid;			/* manufacturer ID */
    WORD	wPid;			/* product ID */
    MMVERSION16	vDriverVersion;		/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];	/* product name (0 terminated string) */
    DWORD	dwFormats;		/* formats supported */
    WORD	wChannels;		/* number of channels supported */
} WAVEINCAPS16, *LPWAVEINCAPS16;

typedef struct {
	HWAVE16			hWave;
	LPWAVEFORMATEX		lpFormat;
	DWORD			dwCallback;
	DWORD			dwInstance;
	UINT16			uMappedDeviceID;
        DWORD			dnDevNode;
} WAVEOPENDESC16, *LPWAVEOPENDESC16;

UINT16    WINAPI waveOutGetNumDevs16(void);
UINT16    WINAPI waveOutGetDevCaps16(UINT16,LPWAVEOUTCAPS16,UINT16);
UINT16    WINAPI waveOutGetVolume16(UINT16,DWORD*);
UINT16    WINAPI waveOutSetVolume16(UINT16,DWORD);
UINT16    WINAPI waveOutGetErrorText16(UINT16,LPSTR,UINT16);
UINT16    WINAPI waveOutOpen16(HWAVEOUT16*,UINT16,const LPWAVEFORMATEX,DWORD,DWORD,DWORD);
UINT16    WINAPI waveOutClose16(HWAVEOUT16);
UINT16    WINAPI waveOutPrepareHeader16(HWAVEOUT16,SEGPTR,UINT16);
UINT16    WINAPI waveOutUnprepareHeader16(HWAVEOUT16,SEGPTR,UINT16);
UINT16    WINAPI waveOutWrite16(HWAVEOUT16,WAVEHDR*,UINT16);
UINT16    WINAPI waveOutPause16(HWAVEOUT16);
UINT16    WINAPI waveOutRestart16(HWAVEOUT16);
UINT16    WINAPI waveOutReset16(HWAVEOUT16);
UINT16    WINAPI waveOutBreakLoop16(HWAVEOUT16);
UINT16    WINAPI waveOutGetPosition16(HWAVEOUT16,LPMMTIME16,UINT16);
UINT16    WINAPI waveOutGetPitch16(HWAVEOUT16,DWORD*);
UINT16    WINAPI waveOutSetPitch16(HWAVEOUT16,DWORD);
UINT16    WINAPI waveOutGetPlaybackRate16(HWAVEOUT16,DWORD*);
UINT16    WINAPI waveOutSetPlaybackRate16(HWAVEOUT16,DWORD);
UINT16    WINAPI waveOutGetID16(HWAVEOUT16,UINT16*);
DWORD     WINAPI waveOutMessage16(HWAVEOUT16,UINT16,DWORD,DWORD);
UINT16    WINAPI waveInGetNumDevs16(void);
UINT16    WINAPI waveInGetDevCaps16(UINT16,LPWAVEINCAPS16,UINT16);
UINT16    WINAPI waveInGetErrorText16(UINT16,LPSTR,UINT16);
UINT16    WINAPI waveInOpen16(HWAVEIN16*,UINT16,const LPWAVEFORMATEX,DWORD,DWORD,DWORD);
UINT16    WINAPI waveInClose16(HWAVEIN16);
UINT16    WINAPI waveInPrepareHeader16(HWAVEIN16,SEGPTR,UINT16);
UINT16    WINAPI waveInUnprepareHeader16(HWAVEIN16,SEGPTR,UINT16);
UINT16    WINAPI waveInAddBuffer16(HWAVEIN16,WAVEHDR*,UINT16);
UINT16    WINAPI waveInStart16(HWAVEIN16);
UINT16    WINAPI waveInStop16(HWAVEIN16);
UINT16    WINAPI waveInReset16(HWAVEIN16);
UINT16    WINAPI waveInGetPosition16(HWAVEIN16,LPMMTIME16,UINT16);
UINT16    WINAPI waveInGetID16(HWAVEIN16,UINT16*);
DWORD     WINAPI waveInMessage16(HWAVEIN16,UINT16,DWORD,DWORD);

typedef LPDRVCALLBACK16 LPMIDICALLBACK16;

typedef struct {
    WORD	wMid;		/* manufacturer ID */
    WORD	wPid;		/* product ID */
    MMVERSION16	vDriverVersion;	/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    WORD	wTechnology;	/* type of device */
    WORD	wVoices;	/* # of voices (internal synth only) */
    WORD	wNotes;		/* max # of notes (internal synth only) */
    WORD	wChannelMask;	/* channels used (internal synth only) */
    DWORD	dwSupport;	/* functionality supported by driver */
} MIDIOUTCAPS16, *LPMIDIOUTCAPS16;

typedef struct {
    WORD	wMid;		/* manufacturer ID */
    WORD	wPid;		/* product ID */
    MMVERSION16	vDriverVersion;	/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    DWORD	dwSupport;	/* included in win95 and higher */
} MIDIINCAPS16, *LPMIDIINCAPS16;

typedef struct midihdr16_tag {
    LPSTR	lpData;		/* pointer to locked data block */
    DWORD	dwBufferLength;	/* length of data in data block */
    DWORD	dwBytesRecorded;/* used for input only */
    DWORD	dwUser;		/* for client's use */
    DWORD	dwFlags;	/* assorted flags (see defines) */
    struct midihdr16_tag *lpNext;	/* reserved for driver */
    DWORD	reserved;	/* reserved for driver */
} MIDIHDR16, *LPMIDIHDR16;

typedef struct {
    HMIDI16         hMidi;
    DWORD           dwCallback;
    DWORD           dwInstance;
    UINT16          reserved;
    DWORD           dnDevNode;
    DWORD           cIds;
    MIDIOPENSTRMID  rgIds;
} MIDIOPENDESC16, *LPMIDIOPENDESC16;

UINT16     WINAPI midiOutGetNumDevs16(void);
UINT16     WINAPI midiOutGetDevCaps16(UINT16,LPMIDIOUTCAPS16,UINT16);
UINT16     WINAPI midiOutGetVolume16(UINT16,DWORD*);
UINT16     WINAPI midiOutSetVolume16(UINT16,DWORD);
UINT16     WINAPI midiOutGetErrorText16(UINT16,LPSTR,UINT16);
UINT16     WINAPI midiOutOpen16(HMIDIOUT16*,UINT16,DWORD,DWORD,DWORD);
UINT16     WINAPI midiOutClose16(HMIDIOUT16);
UINT16     WINAPI midiOutPrepareHeader16(HMIDIOUT16,SEGPTR,UINT16);
UINT16     WINAPI midiOutUnprepareHeader16(HMIDIOUT16,SEGPTR,UINT16);
UINT16     WINAPI midiOutShortMsg16(HMIDIOUT16,DWORD);
UINT16     WINAPI midiOutLongMsg16(HMIDIOUT16,MIDIHDR16*,UINT16);
UINT16     WINAPI midiOutReset16(HMIDIOUT16);
UINT16     WINAPI midiOutCachePatches16(HMIDIOUT16,UINT16,WORD*,UINT16);
UINT16     WINAPI midiOutCacheDrumPatches16(HMIDIOUT16,UINT16,WORD*,UINT16);
UINT16     WINAPI midiOutGetID16(HMIDIOUT16,UINT16*);
DWORD      WINAPI midiOutMessage16(HMIDIOUT16,UINT16,DWORD,DWORD);
UINT16     WINAPI midiInGetNumDevs16(void);
UINT16     WINAPI midiInGetDevCaps16(UINT16,LPMIDIINCAPS16,UINT16);
UINT16     WINAPI midiInGetErrorText16(UINT16,LPSTR,UINT16);
UINT16     WINAPI midiInOpen16(HMIDIIN16*,UINT16,DWORD,DWORD,DWORD);
UINT16     WINAPI midiInClose16(HMIDIIN16);
UINT16     WINAPI midiInPrepareHeader16(HMIDIIN16,SEGPTR,UINT16);
UINT16     WINAPI midiInUnprepareHeader16(HMIDIIN16,SEGPTR,UINT16);
UINT16     WINAPI midiInAddBuffer16(HMIDIIN16,MIDIHDR16*,UINT16);
UINT16     WINAPI midiInStart16(HMIDIIN16);
UINT16     WINAPI midiInStop16(HMIDIIN16);
UINT16     WINAPI midiInReset16(HMIDIIN16);
UINT16     WINAPI midiInGetID16(HMIDIIN16,UINT16*);
DWORD      WINAPI midiInMessage16(HMIDIIN16,UINT16,DWORD,DWORD);
MMRESULT16 WINAPI midiStreamClose16(HMIDISTRM16 hms);
MMRESULT16 WINAPI midiStreamOpen16(HMIDISTRM16*,LPUINT16,DWORD,DWORD,DWORD,DWORD);
MMRESULT16 WINAPI midiStreamOut16(HMIDISTRM16,LPMIDIHDR16,UINT16);
MMRESULT16 WINAPI midiStreamPause16(HMIDISTRM16);
MMRESULT16 WINAPI midiStreamPosition16(HMIDISTRM16,LPMMTIME16,UINT16);
MMRESULT16 WINAPI midiStreamProperty16(HMIDISTRM16,LPBYTE,DWORD);
MMRESULT16 WINAPI midiStreamRestart16(HMIDISTRM16);
MMRESULT16 WINAPI midiStreamStop16(HMIDISTRM16);

typedef struct {
    WORD	wMid;			/* manufacturer ID */
    WORD	wPid;			/* product ID */
    MMVERSION16	vDriverVersion;		/* version of the driver */
    CHAR	szPname[MAXPNAMELEN];	/* product name (NULL terminated string) */
    WORD	wTechnology;		/* type of device */
    DWORD	dwSupport;		/* functionality supported by driver */
} AUXCAPS16, *LPAUXCAPS16;

typedef void (CALLBACK *LPTIMECALLBACK16)(UINT16,UINT16,DWORD,DWORD,DWORD);

typedef struct {
    UINT16	wPeriodMin;	/* minimum period supported  */
    UINT16	wPeriodMax;	/* maximum period supported  */
} TIMECAPS16,*LPTIMECAPS16;

typedef struct {
    WORD wMid;                  /* manufacturer ID */
    WORD wPid;                  /* product ID */
    char szPname[MAXPNAMELEN];	/* product name (NULL terminated string) */
    UINT16 wXmin;               /* minimum x position value */
    UINT16 wXmax;               /* maximum x position value */
    UINT16 wYmin;               /* minimum y position value */
    UINT16 wYmax;               /* maximum y position value */
    UINT16 wZmin;               /* minimum z position value */
    UINT16 wZmax;               /* maximum z position value */
    UINT16 wNumButtons;         /* number of buttons */
    UINT16 wPeriodMin;          /* minimum message period when captured */
    UINT16 wPeriodMax;          /* maximum message period when captured */
                                /* win95,nt4 additions: */
    UINT16 wRmin;		/* minimum r position value */
    UINT16 wRmax;		/* maximum r position value */
    UINT16 wUmin;		/* minimum u (5th axis) position value */
    UINT16 wUmax;		/* maximum u (5th axis) position value */
    UINT16 wVmin;		/* minimum v (6th axis) position value */
    UINT16 wVmax;		/* maximum v (6th axis) position value */
    UINT16 wCaps;		/* joystick capabilites */
    UINT16 wMaxAxes;		/* maximum number of axes supported */
    UINT16 wNumAxes;		/* number of axes in use */
    UINT16 wMaxButtons;		/* maximum number of buttons supported */
    CHAR szRegKey[MAXPNAMELEN]; /* registry key */
    CHAR szOEMVxD[MAX_JOYSTICKOEMVXDNAME]; /* OEM VxD in use */
} JOYCAPS16, *LPJOYCAPS16;

typedef struct {
    UINT16 wXpos;                 /* x position */
    UINT16 wYpos;                 /* y position */
    UINT16 wZpos;                 /* z position */
    UINT16 wButtons;              /* button states */
} JOYINFO16, *LPJOYINFO16;

typedef struct {
    WORD         wMid;                  /* manufacturer id */
    WORD         wPid;                  /* product id */
    MMVERSION16  vDriverVersion;        /* version of the driver */
    CHAR         szPname[MAXPNAMELEN];  /* product name */
    DWORD        fdwSupport;            /* misc. support bits */
    DWORD        cDestinations;         /* count of destinations */
} MIXERCAPS16,*LPMIXERCAPS16;

typedef struct tMIXEROPENDESC16
{
	HMIXEROBJ16		hmx;
        LPVOID			pReserved0;
	DWORD			dwCallback;
	DWORD			dwInstance;
} MIXEROPENDESC16, *LPMIXEROPENDESC16;

typedef struct {
    DWORD	cbStruct;		/* size of MIXERLINE structure */
    DWORD	dwDestination;		/* zero based destination index */
    DWORD	dwSource;		/* zero based source index (if source) */
    DWORD	dwLineID;		/* unique line id for mixer device */
    DWORD	fdwLine;		/* state/information about line */
    DWORD	dwUser;			/* driver specific information */
    DWORD	dwComponentType;	/* component type line connects to */
    DWORD	cChannels;		/* number of channels line supports */
    DWORD	cConnections;		/* number of connections [possible] */
    DWORD	cControls;		/* number of controls at this line */
    CHAR	szShortName[MIXER_SHORT_NAME_CHARS];
    CHAR	szName[MIXER_LONG_NAME_CHARS];
    struct {
	DWORD	dwType;			/* MIXERLINE_TARGETTYPE_xxxx */
	DWORD	dwDeviceID;		/* target device ID of device type */
	WORD	wMid;			/* of target device */
	WORD	wPid;			/*      " */
	MMVERSION16	vDriverVersion;	/*      " */
	CHAR	szPname[MAXPNAMELEN];	/*      " */
    } Target;
} MIXERLINE16, *LPMIXERLINE16;

typedef struct {
    DWORD		cbStruct;           /* size in bytes of MIXERCONTROL */
    DWORD		dwControlID;        /* unique control id for mixer device */
    DWORD		dwControlType;      /* MIXERCONTROL_CONTROLTYPE_xxx */
    DWORD		fdwControl;         /* MIXERCONTROL_CONTROLF_xxx */
    DWORD		cMultipleItems;     /* if MIXERCONTROL_CONTROLF_MULTIPLE set */
    CHAR		szShortName[MIXER_SHORT_NAME_CHARS];
    CHAR		szName[MIXER_LONG_NAME_CHARS];
    union {
	struct {
	    LONG	lMinimum;	/* signed minimum for this control */
	    LONG	lMaximum;	/* signed maximum for this control */
	} DUMMYSTRUCTNAME;
	struct {
	    DWORD	dwMinimum;	/* unsigned minimum for this control */
	    DWORD	dwMaximum;	/* unsigned maximum for this control */
	} DUMMYSTRUCTNAME1;
	DWORD       	dwReserved[6];
    } Bounds;
    union {
	DWORD		cSteps;		/* # of steps between min & max */
	DWORD		cbCustomData;	/* size in bytes of custom data */
	DWORD		dwReserved[6];	/* !!! needed? we have cbStruct.... */
    } Metrics;
} MIXERCONTROL16, *LPMIXERCONTROL16;

typedef struct {
    DWORD	cbStruct;	/* size in bytes of MIXERLINECONTROLS */
    DWORD	dwLineID;	/* line id (from MIXERLINE.dwLineID) */
    union {
	DWORD	dwControlID;	/* MIXER_GETLINECONTROLSF_ONEBYID */
	DWORD	dwControlType;	/* MIXER_GETLINECONTROLSF_ONEBYTYPE */
    } DUMMYUNIONNAME;
    DWORD	cControls;	/* count of controls pmxctrl points to */
    DWORD	cbmxctrl;	/* size in bytes of _one_ MIXERCONTROL */
    SEGPTR	pamxctrl;	/* pointer to first MIXERCONTROL array */
} MIXERLINECONTROLS16, *LPMIXERLINECONTROLS16;

typedef struct {
    DWORD	cbStruct;	/* size in bytes of MIXERCONTROLDETAILS */
    DWORD	dwControlID;	/* control id to get/set details on */
    DWORD	cChannels;	/* number of channels in paDetails array */
    union {
        HWND16	hwndOwner;	/* for MIXER_SETCONTROLDETAILSF_CUSTOM */
        DWORD	cMultipleItems;	/* if _MULTIPLE, the number of items per channel */
    } DUMMYUNIONNAME;
    DWORD	cbDetails;	/* size of _one_ details_XX struct */
    LPVOID	paDetails;	/* pointer to array of details_XX structs */
} MIXERCONTROLDETAILS16,*LPMIXERCONTROLDETAILS16;

typedef struct {
    DWORD	dwParam1;
    DWORD	dwParam2;
    CHAR	szName[MIXER_LONG_NAME_CHARS];
} MIXERCONTROLDETAILS_LISTTEXT16,*LPMIXERCONTROLDETAILS_LISTTEXT16;

typedef LRESULT (CALLBACK *LPMMIOPROC16)(LPSTR lpmmioinfo,UINT16 uMessage,
					 LPARAM lParam1,LPARAM lParam2);

typedef struct {
        DWORD		dwFlags;	/* general status flags */
        FOURCC		fccIOProc;	/* pointer to I/O procedure */
        LPMMIOPROC16	pIOProc;	/* pointer to I/O procedure */
        UINT16		wErrorRet;	/* place for error to be returned */
        HTASK16		hTask;		/* alternate local task */
        /* fields maintained by MMIO functions during buffered I/O */
        LONG		cchBuffer;	/* size of I/O buffer (or 0L) */
        HPSTR		pchBuffer;	/* start of I/O buffer (or NULL) */
        HPSTR		pchNext;	/* pointer to next byte to read/write */
        HPSTR		pchEndRead;	/* pointer to last valid byte to read */
        HPSTR		pchEndWrite;	/* pointer to last byte to write */
        LONG		lBufOffset;	/* disk offset of start of buffer */
        /* fields maintained by I/O procedure */
        LONG		lDiskOffset;	/* disk offset of next read or write */
        DWORD		adwInfo[3];	/* data specific to type of MMIOPROC */
        /* other fields maintained by MMIO */
        DWORD		dwReserved1;	/* reserved for MMIO use */
        DWORD		dwReserved2;	/* reserved for MMIO use */
        HMMIO16		hmmio;		/* handle to open file */
} MMIOINFO16, *LPMMIOINFO16;

typedef UINT16 (CALLBACK *YIELDPROC16)(UINT16,DWORD);

UINT16		WINAPI auxGetNumDevs16(void);
UINT16		WINAPI auxGetDevCaps16 (UINT16,LPAUXCAPS16,UINT16);
UINT16		WINAPI auxSetVolume16(UINT16,DWORD);
UINT16		WINAPI auxGetVolume16(UINT16,LPDWORD);
DWORD		WINAPI auxOutMessage16(UINT16,UINT16,DWORD,DWORD);
MMRESULT16	WINAPI timeGetSystemTime16(LPMMTIME16,UINT16);
MMRESULT16	WINAPI timeSetEvent16(UINT16,UINT16,LPTIMECALLBACK16,DWORD,UINT16);
MMRESULT16	WINAPI timeKillEvent16(UINT16);
MMRESULT16	WINAPI timeGetDevCaps16(LPTIMECAPS16,UINT16);
MMRESULT16	WINAPI timeBeginPeriod16(UINT16);
MMRESULT16	WINAPI timeEndPeriod16(UINT16);
MMRESULT16	WINAPI joyGetDevCaps16 (UINT16,LPJOYCAPS16,UINT16);
UINT16		WINAPI joyGetNumDevs16(void);
MMRESULT16	WINAPI joyGetPos16(UINT16,LPJOYINFO16);
MMRESULT16	WINAPI joyGetPosEx16(UINT16,LPJOYINFOEX);
MMRESULT16	WINAPI joyGetThreshold16(UINT16,UINT16*);
MMRESULT16	WINAPI joyReleaseCapture16(UINT16);
MMRESULT16	WINAPI joySetCapture16(HWND16,UINT16,UINT16,BOOL16);
MMRESULT16	WINAPI joySetThreshold16(UINT16,UINT16);
UINT16		WINAPI mixerGetNumDevs16(void);
UINT16		WINAPI mixerOpen16(LPHMIXER16,UINT16,DWORD,DWORD,DWORD);
UINT16		WINAPI mixerClose16(HMIXER16);
DWORD		WINAPI mixerMessage16(HMIXER16,UINT16,DWORD,DWORD);
UINT16		WINAPI mixerGetDevCaps16(UINT16,LPMIXERCAPS16,UINT16);
UINT16		WINAPI mixerGetLineInfo16(HMIXEROBJ16,LPMIXERLINE16,DWORD);
UINT16		WINAPI mixerGetID16(HMIXEROBJ16,LPUINT16,DWORD);
UINT16		WINAPI mixerGetLineControls16(HMIXEROBJ16,LPMIXERLINECONTROLS16,DWORD);
UINT16		WINAPI mixerGetControlDetails16(HMIXEROBJ16,LPMIXERCONTROLDETAILS16,DWORD);
UINT16		WINAPI mixerSetControlDetails16(HMIXEROBJ16,LPMIXERCONTROLDETAILS16,DWORD);
LPMMIOPROC16	WINAPI mmioInstallIOProc16(FOURCC,LPMMIOPROC16,DWORD);
FOURCC 		WINAPI mmioStringToFOURCC16(LPCSTR,UINT16);
HMMIO16		WINAPI mmioOpen16(LPSTR,MMIOINFO16*,DWORD);
UINT16 		WINAPI mmioRename16(LPCSTR,LPCSTR,MMIOINFO16*,DWORD);
MMRESULT16 	WINAPI mmioClose16(HMMIO16,UINT16);
LONG 		WINAPI mmioRead16(HMMIO16,HPSTR,LONG);
LONG 		WINAPI mmioWrite16(HMMIO16,HPCSTR,LONG);
LONG 		WINAPI mmioSeek16(HMMIO16,LONG,INT16);
MMRESULT16	WINAPI mmioGetInfo16(HMMIO16,MMIOINFO16*,UINT16);
MMRESULT16 	WINAPI mmioSetInfo16(HMMIO16,const MMIOINFO16*,UINT16);
UINT16 		WINAPI mmioSetBuffer16(HMMIO16,LPSTR,LONG,UINT16);
UINT16 		WINAPI mmioFlush16(HMMIO16,UINT16);
UINT16 		WINAPI mmioAdvance16(HMMIO16,MMIOINFO16*,UINT16);
LONG 		WINAPI mmioSendMessage16(HMMIO16,UINT16,LPARAM,LPARAM);
UINT16		WINAPI mmioDescend16(HMMIO16,MMCKINFO*,const MMCKINFO*,UINT16);
UINT16		WINAPI mmioAscend16(HMMIO16,MMCKINFO*,UINT16);
UINT16		WINAPI mmioCreateChunk16(HMMIO16,MMCKINFO*,UINT16);
DWORD		WINAPI mciSendCommand16(UINT16,UINT16,DWORD,DWORD);
DWORD		WINAPI mciSendString16(LPCSTR,LPSTR,UINT16,HWND16);
UINT16		WINAPI mciGetDeviceID16(LPCSTR);
UINT16		WINAPI mciGetDeviceIDFromElementID16(DWORD,LPCSTR);
BOOL16		WINAPI mciGetErrorString16 (DWORD,LPSTR,UINT16);
BOOL16		WINAPI mciSetYieldProc16(UINT16,YIELDPROC16,DWORD);
HTASK16		WINAPI mciGetCreatorTask16(UINT16);
YIELDPROC16	WINAPI mciGetYieldProc16(UINT16,DWORD*);
DWORD 		WINAPI mciGetDriverData16(UINT16 uDeviceID);
BOOL16		WINAPI mciSetDriverData16(UINT16 uDeviceID, DWORD dwData);
UINT16		WINAPI mciDriverYield16(UINT16 uDeviceID);
BOOL16		WINAPI mciDriverNotify16(HWND16 hwndCallback, UINT16 uDeviceID,
					  UINT16 uStatus);
UINT16		WINAPI mciLoadCommandResource16(HINSTANCE16 hInstance,
					 LPCSTR lpResName, UINT16 uType);
BOOL16		WINAPI mciFreeCommandResource16(UINT16 uTable);

HINSTANCE16	WINAPI mmTaskCreate16(SEGPTR spProc, HINSTANCE16 *lphMmTask, DWORD dwPmt);
void    	WINAPI mmTaskBlock16(HINSTANCE16 hInst);
LRESULT 	WINAPI mmTaskSignal16(HTASK16 ht);
void    	WINAPI mmTaskYield16(void);
LRESULT 	WINAPI mmThreadCreate16(FARPROC16 fpThreadAddr, LPHANDLE16 lpHndl,
					 DWORD dwPmt, DWORD dwFlags);
void 		WINAPI mmThreadSignal16(HANDLE16 hndl);
void    	WINAPI mmThreadBlock16(HANDLE16 hndl);
HANDLE16 	WINAPI mmThreadGetTask16(HANDLE16 hndl);
BOOL16   	WINAPI mmThreadIsValid16(HANDLE16 hndl);
BOOL16  	WINAPI mmThreadIsCurrent16(HANDLE16 hndl);

BOOL16		WINAPI DriverCallback16(DWORD dwCallBack, UINT16 uFlags, HANDLE16 hDev,
					 WORD wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);

typedef struct {
    DWORD	dwCallback;
    WORD	wDeviceID;
    WORD	wReserved0;
    SEGPTR	lpstrDeviceType;
    SEGPTR	lpstrElementName;
    SEGPTR	lpstrAlias;
} MCI_OPEN_PARMS16, *LPMCI_OPEN_PARMS16;

typedef struct {
    DWORD   dwCallback;
    SEGPTR  lpstrReturn;
    DWORD   dwRetSize;
} MCI_INFO_PARMS16, *LPMCI_INFO_PARMS16;

typedef struct {
    DWORD	dwCallback;
    SEGPTR	lpstrReturn;
    DWORD	dwRetSize;
    DWORD	dwNumber;
    WORD	wDeviceType;
    WORD	wReserved0;
} MCI_SYSINFO_PARMS16, *LPMCI_SYSINFO_PARMS16;

typedef struct {
    DWORD	dwCallback;
    UINT16	nVirtKey;
    WORD	wReserved0;
    HWND16	hwndBreak;
    WORD	wReserved1;
} MCI_BREAK_PARMS16, *LPMCI_BREAK_PARMS16;

typedef struct {
    DWORD	dwCallback;
    LPCSTR	lpfilename;
} MCI_LOAD_PARMS16, *LPMCI_LOAD_PARMS16;

typedef struct {
    DWORD	dwCallback;
    SEGPTR	lpstrCommand;
} MCI_VD_ESCAPE_PARMS16, *LPMCI_VD_ESCAPE_PARMS16;

typedef struct {
    UINT16			wDeviceID;		/* device ID */
    SEGPTR			lpstrParams;		/* parameter string for entry in SYSTEM.INI */
    UINT16			wCustomCommandTable;	/* custom command table (0xFFFF if none)
							 * filled in by the driver */
    UINT16			wType;			/* driver type (filled in by the driver) */
} MCI_OPEN_DRIVER_PARMS16, *LPMCI_OPEN_DRIVER_PARMS16;

typedef struct {
    DWORD		dwCallback;
    MCIDEVICEID16	wDeviceID;
    WORD		wReserved0;
    SEGPTR		lpstrDeviceType;
    SEGPTR		lpstrElementName;
    SEGPTR		lpstrAlias;
    DWORD		dwBufferSeconds;
} MCI_WAVE_OPEN_PARMS16, *LPMCI_WAVE_OPEN_PARMS16;

typedef struct {
    DWORD	dwCallback;
    DWORD	dwTimeFormat;
    DWORD	dwAudio;
    UINT16	wInput;
    UINT16	wReserved0;
    UINT16	wOutput;
    UINT16	wReserved1;
    UINT16	wFormatTag;
    UINT16	wReserved2;
    UINT16	nChannels;
    UINT16	wReserved3;
    DWORD	nSamplesPerSec;
    DWORD	nAvgBytesPerSec;
    UINT16	nBlockAlign;
    UINT16	wReserved4;
    UINT16	wBitsPerSample;
    UINT16	wReserved5;
} MCI_WAVE_SET_PARMS16, * LPMCI_WAVE_SET_PARMS16;

typedef struct {
    DWORD   dwCallback;
    UINT16  wDeviceID;
    UINT16  wReserved0;
    SEGPTR  lpstrDeviceType;
    SEGPTR  lpstrElementName;
    SEGPTR  lpstrAlias;
    DWORD   dwStyle;
    HWND16  hWndParent;
    UINT16  wReserved1;
} MCI_ANIM_OPEN_PARMS16, *LPMCI_ANIM_OPEN_PARMS16;

typedef struct {
    DWORD	dwCallback;
    HWND16	hWnd;
    WORD	wReserved1;
    WORD	nCmdShow;
    WORD	wReserved2;
    LPCSTR	lpstrText;
} MCI_ANIM_WINDOW_PARMS16, *LPMCI_ANIM_WINDOW_PARMS16;

typedef struct {
    DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
    POINT16 ptOffset;
    POINT16 ptExtent;
#else   /* ifdef MCI_USE_OFFEXT */
    RECT16  rc;
#endif  /* ifdef MCI_USE_OFFEXT */
} MCI_ANIM_RECT_PARMS16, *LPMCI_ANIM_RECT_PARMS16;

typedef struct {
    DWORD   dwCallback;
    RECT16  rc;
    HDC16   hDC;
} MCI_ANIM_UPDATE_PARMS16, *LPMCI_ANIM_UPDATE_PARMS16;

typedef struct {
    DWORD		dwCallback;
    MCIDEVICEID16	wDeviceID;
    WORD		wReserved0;
    LPCSTR		lpstrDeviceType;
    LPCSTR		lpstrElementName;
    LPCSTR		lpstrAlias;
    DWORD		dwStyle;
    HWND16		hWndParent;
    WORD		wReserved1;
} MCI_OVLY_OPEN_PARMS16, *LPMCI_OVLY_OPEN_PARMS16;

typedef struct {
    DWORD	dwCallback;
    HWND16	hWnd;
    WORD	wReserved1;
    UINT16	nCmdShow;
    WORD	wReserved2;
    LPCSTR	lpstrText;
} MCI_OVLY_WINDOW_PARMS16, *LPMCI_OVLY_WINDOW_PARMS16;

typedef struct {
    DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
    POINT16 ptOffset;
    POINT16 ptExtent;
#else   /* ifdef MCI_USE_OFFEXT */
    RECT16  rc;
#endif  /* ifdef MCI_USE_OFFEXT */
} MCI_OVLY_RECT_PARMS16, *LPMCI_OVLY_RECT_PARMS16;

typedef struct {
    DWORD   dwCallback;
    LPCSTR  lpfilename;
    RECT16  rc;
} MCI_OVLY_SAVE_PARMS16, *LPMCI_OVLY_SAVE_PARMS16;

typedef struct {
    DWORD	dwCallback;
    LPCSTR	lpfilename;
    RECT16	rc;
} MCI_OVLY_LOAD_PARMS16, *LPMCI_OVLY_LOAD_PARMS16;

/* from digitalv / 16 bit */
typedef struct {
    DWORD   dwCallback;
    RECT16  rc;
} MCI_DGV_RECT_PARMS16, *LPMCI_DGV_RECT_PARMS16;

typedef struct {
    DWORD   dwCallback;
    LPSTR   lpstrFileName;
    RECT16  rc;
} MCI_DGV_CAPTURE_PARMS16, *LPMCI_DGV_CAPTURE_PARMS16;

typedef struct {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
    RECT16  rc;
    DWORD   dwAudioStream;
    DWORD   dwVideoStream;
} MCI_DGV_COPY_PARMS16, *LPMCI_DGV_COPY_PARMS16;

typedef struct {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
    RECT16  rc;
    DWORD   dwAudioStream;
    DWORD   dwVideoStream;
} MCI_DGV_CUT_PARMS16, * LPMCI_DGV_CUT_PARMS16;

typedef struct {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
    RECT16  rc;
    DWORD   dwAudioStream;
    DWORD   dwVideoStream;
} MCI_DGV_DELETE_PARMS16, * LPMCI_DGV_DELETE_PARMS16;

typedef MCI_DGV_RECT_PARMS16 MCI_DGV_FREEZE_PARMS16, * LPMCI_DGV_FREEZE_PARMS16;

typedef struct  {
    DWORD   dwCallback;
    LPSTR   lpstrReturn;
    DWORD   dwRetSize;
    DWORD   dwItem;
} MCI_DGV_INFO_PARMS16, * LPMCI_DGV_INFO_PARMS16;

typedef struct {
    DWORD   dwCallback;
    LPSTR   lpstrReturn;
    DWORD   dwLength;
    DWORD   dwNumber;
    DWORD   dwItem;
    LPSTR   lpstrAlgorithm;
} MCI_DGV_LIST_PARMS16, *LPMCI_DGV_LIST_PARMS16;

typedef MCI_LOAD_PARMS16  MCI_DGV_LOAD_PARMS16 , * LPMCI_DGV_LOAD_PARMS16;

typedef struct {
    DWORD   dwCallback;
    UINT16  wDeviceID;
    UINT16  wReserved0;
    LPSTR   lpstrDeviceType;
    LPSTR   lpstrElementName;
    LPSTR   lpstrAlias;
    DWORD   dwStyle;
    HWND16  hWndParent;
    UINT16  wReserved1;
} MCI_DGV_OPEN_PARMS16, *LPMCI_DGV_OPEN_PARMS16;

typedef struct {
    DWORD   dwCallback;
    DWORD   dwTo;
    RECT16  rc;
    DWORD   dwAudioStream;
    DWORD   dwVideoStream;
} MCI_DGV_PASTE_PARMS16, * LPMCI_DGV_PASTE_PARMS16;

typedef MCI_DGV_RECT_PARMS16 MCI_DGV_PUT_PARMS16, * LPMCI_DGV_PUT_PARMS16;

typedef struct {
    DWORD       dwCallback;
    DWORD       dwItem;
    LPSTR       lpstrName;
    DWORD       lpstrAlgorithm;
    DWORD       dwHandle;
} MCI_DGV_QUALITY_PARMS16, *LPMCI_DGV_QUALITY_PARMS16;

typedef struct {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
    RECT16  rc;
    DWORD   dwAudioStream;
    DWORD   dwVideoStream;
} MCI_DGV_RECORD_PARMS16, * LPMCI_DGV_RECORD_PARMS16;

typedef struct {
    DWORD   dwCallback;
    LPSTR   lpstrPath;
    DWORD   dwSize;
} MCI_DGV_RESERVE_PARMS16, *LPMCI_DGV_RESERVE_PARMS16A;

typedef struct {
    DWORD   dwCallback;
    LPSTR   lpstrFileName;
    RECT16  rc;
} MCI_DGV_RESTORE_PARMS16, *LPMCI_DGV_RESTORE_PARMS16;

typedef struct {
    DWORD   dwCallback;
    LPSTR   lpstrFileName;
    RECT16  rc;
} MCI_DGV_SAVE_PARMS16, *LPMCI_DGV_SAVE_PARMS16;

typedef struct {
    DWORD   dwCallback;
    DWORD   dwItem;
    DWORD   dwValue;
    DWORD   dwOver;
    LPSTR   lpstrAlgorithm;
    LPSTR   lpstrQuality;
} MCI_DGV_SETAUDIO_PARMS16, *LPMCI_DGV_SETAUDIO_PARMS16;

typedef struct {
    DWORD   dwCallback;
    DWORD   dwItem;
    DWORD   dwValue;
    DWORD   dwOver;
    LPSTR   lpstrAlgorithm;
    LPSTR   lpstrQuality;
    DWORD   dwSourceNumber;
} MCI_DGV_SETVIDEO_PARMS16, *LPMCI_DGV_SETVIDEO_PARMS16;

typedef struct {
    DWORD   dwCallback;
    DWORD   dwReturn;
    DWORD   dwItem;
    DWORD   dwTrack;
    SEGPTR  lpstrDrive;
    DWORD   dwReference;
} MCI_DGV_STATUS_PARMS16, *LPMCI_DGV_STATUS_PARMS16;

typedef struct {
    DWORD   dwCallback;
    RECT16  rc;
    HDC16   hDC;
    UINT16  wReserved0;
} MCI_DGV_UPDATE_PARMS16, * LPMCI_DGV_UPDATE_PARMS16;

typedef MCI_DGV_RECT_PARMS16 MCI_DGV_UNFREEZE_PARMS16, * LPMCI_DGV_UNFREEZE_PARMS16;

typedef MCI_DGV_RECT_PARMS16 MCI_DGV_WHERE_PARMS16, * LPMCI_DGV_WHERE_PARMS16;

typedef struct {
    DWORD   dwCallback;
    HWND16  hWnd;
    UINT16  wReserved1;
    UINT16  nCmdShow;
    UINT16  wReserved2;
    LPSTR   lpstrText;
} MCI_DGV_WINDOW_PARMS16, *LPMCI_DGV_WINDOW_PARMS16;

#include "poppack.h"

#endif  /* __WINE_WINE_MMSYSTEM16_H */
