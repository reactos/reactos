/*==========================================================================
 *
 *  mmsystem.h -- Include file for Multimedia API's
 *
 *  Version 4.00
 *
 *  Copyright (C) 1992-1996 Microsoft Corporation.  All Rights Reserved.
 *
 *--------------------------------------------------------------------------
 *
 *  Define:         Prevent inclusion of:
 *  --------------  --------------------------------------------------------
 *  MMNODRV         Installable driver support
 *  MMNOSOUND       Sound support
 *  MMNOWAVE        Waveform support
 *  MMNOMIDI        MIDI support
 *  MMNOAUX         Auxiliary audio support
 *  MMNOMIXER       Mixer support
 *  MMNOTIMER       Timer support
 *  MMNOJOY         Joystick support
 *  MMNOMCI         MCI support
 *  MMNOMMIO        Multimedia file I/O support
 *  MMNOMMSYSTEM    General MMSYSTEM functions
 *
 *==========================================================================
 */

#ifndef _INC_MMSYSTEM
#define _INC_MMSYSTEM   /* #defined if mmsystem.h has been included */

#ifdef _WIN32
#include <pshpack1.h>
#else
#ifndef RC_INVOKED
#pragma pack(1)
#endif
#endif

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

#ifdef _WIN32
#ifndef _WINMM_
#define	WINMMAPI	DECLSPEC_IMPORT
#else
#define	WINMMAPI
#endif
#define _loadds
#define _huge
#else
#define	WINMMAPI
#endif

#ifdef _WIN32_VXD

#define NEAR
#define FAR

#define DECLARE_HANDLE(x) typedef WORD x
#define WINAPI

typedef DWORD LPSTR;
typedef DWORD LPVOID;
typedef DWORD LPCSTR;
typedef DWORD LPCWSTR;
typedef DWORD LRESULT;
typedef DWORD LPARAM;
typedef DWORD WPARAM;

typedef short WCHAR;

#endif

/****************************************************************************

		    General constants and data types

****************************************************************************/

/* general constants */
#define MAXPNAMELEN      32     /* max product name length (including NULL) */
#define MAXERRORLENGTH   256    /* max error text length (including NULL) */
#define MAX_JOYSTICKOEMVXDNAME 260 /* max oem vxd name length (including NULL) */

/*
 *  Microsoft Manufacturer and Product ID's (these have been moved to
 *  MMREG.H for Windows 4.00 and above).
 */
#if (WINVER <= 0x030A)
#ifndef MM_MICROSOFT
#define MM_MICROSOFT            1   /* Microsoft Corporation */
#endif

#ifndef MM_MIDI_MAPPER
#define MM_MIDI_MAPPER          1   /* MIDI Mapper */
#define MM_WAVE_MAPPER          2   /* Wave Mapper */
#define MM_SNDBLST_MIDIOUT      3   /* Sound Blaster MIDI output port */
#define MM_SNDBLST_MIDIIN       4   /* Sound Blaster MIDI input port */
#define MM_SNDBLST_SYNTH        5   /* Sound Blaster internal synthesizer */
#define MM_SNDBLST_WAVEOUT      6   /* Sound Blaster waveform output */
#define MM_SNDBLST_WAVEIN       7   /* Sound Blaster waveform input */
#define MM_ADLIB                9   /* Ad Lib-compatible synthesizer */
#define MM_MPU401_MIDIOUT      10   /* MPU401-compatible MIDI output port */
#define MM_MPU401_MIDIIN       11   /* MPU401-compatible MIDI input port */
#define MM_PC_JOYSTICK         12   /* Joystick adapter */
#endif
#endif

/* general data types */

#ifdef _WIN32
typedef UINT        MMVERSION;  /* major (high byte), minor (low byte) */
#else
typedef UINT        VERSION;    /* major (high byte), minor (low byte) */
#endif
typedef UINT        MMRESULT;   /* error return code, 0 means no error */
				/* call as if(err=xxxx(...)) Error(err); else */
#define _MMRESULT_

typedef UINT FAR   *LPUINT;

/* MMTIME data structure */
typedef struct mmtime_tag
{
    UINT            wType;      /* indicates the contents of the union */
    union
    {
	DWORD       ms;         /* milliseconds */
	DWORD       sample;     /* samples */
	DWORD       cb;         /* byte count */
	DWORD       ticks;      /* ticks in MIDI stream */

	/* SMPTE */
	struct
	{
	    BYTE    hour;       /* hours */
	    BYTE    min;        /* minutes */
	    BYTE    sec;        /* seconds */
	    BYTE    frame;      /* frames  */
	    BYTE    fps;        /* frames per second */
	    BYTE    dummy;      /* pad */
#ifdef _WIN32
	    BYTE    pad[2];
#endif
	} smpte;

	/* MIDI */
	struct
	{
	    DWORD songptrpos;   /* song pointer position */
	} midi;
    } u;
} MMTIME, *PMMTIME, NEAR *NPMMTIME, FAR *LPMMTIME;

/* types for wType field in MMTIME struct */
#define TIME_MS         0x0001  /* time in milliseconds */
#define TIME_SAMPLES    0x0002  /* number of wave samples */
#define TIME_BYTES      0x0004  /* current byte offset */
#define TIME_SMPTE      0x0008  /* SMPTE time */
#define TIME_MIDI       0x0010  /* MIDI time */
#define TIME_TICKS      0x0020  /* Ticks within MIDI stream */

/*
 *
 *
 */
#define MAKEFOURCC(ch0, ch1, ch2, ch3)                              \
		((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
		((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))

/****************************************************************************

		    Multimedia Extensions Window Messages

****************************************************************************/

#define MM_JOY1MOVE         0x3A0           /* joystick */
#define MM_JOY2MOVE         0x3A1
#define MM_JOY1ZMOVE        0x3A2
#define MM_JOY2ZMOVE        0x3A3
#define MM_JOY1BUTTONDOWN   0x3B5
#define MM_JOY2BUTTONDOWN   0x3B6
#define MM_JOY1BUTTONUP     0x3B7
#define MM_JOY2BUTTONUP     0x3B8

#define MM_MCINOTIFY        0x3B9           /* MCI */

#define MM_WOM_OPEN         0x3BB           /* waveform output */
#define MM_WOM_CLOSE        0x3BC
#define MM_WOM_DONE         0x3BD

#define MM_WIM_OPEN         0x3BE           /* waveform input */
#define MM_WIM_CLOSE        0x3BF
#define MM_WIM_DATA         0x3C0

#define MM_MIM_OPEN         0x3C1           /* MIDI input */
#define MM_MIM_CLOSE        0x3C2
#define MM_MIM_DATA         0x3C3
#define MM_MIM_LONGDATA     0x3C4
#define MM_MIM_ERROR        0x3C5
#define MM_MIM_LONGERROR    0x3C6

#define MM_MOM_OPEN         0x3C7           /* MIDI output */
#define MM_MOM_CLOSE        0x3C8
#define MM_MOM_DONE         0x3C9

/* these are also in msvideo.h */
#ifndef MM_DRVM_OPEN
 #define MM_DRVM_OPEN       0x3D0           /* installable drivers */
 #define MM_DRVM_CLOSE      0x3D1
 #define MM_DRVM_DATA       0x3D2
 #define MM_DRVM_ERROR      0x3D3
#endif

/* these are used by msacm.h */
#define MM_STREAM_OPEN	    0x3D4
#define MM_STREAM_CLOSE	    0x3D5
#define MM_STREAM_DONE	    0x3D6
#define MM_STREAM_ERROR	    0x3D7

#if(WINVER >= 0x0400)
#define MM_MOM_POSITIONCB   0x3CA           /* Callback for MEVT_POSITIONCB */

#ifndef MM_MCISIGNAL
 #define MM_MCISIGNAL        0x3CB
#endif

#define MM_MIM_MOREDATA      0x3CC          /* MIM_DONE w/ pending events */

#endif /* WINVER >= 0x0400 */
#define MM_MIXM_LINE_CHANGE     0x3D0       /* mixer line change notify */
#define MM_MIXM_CONTROL_CHANGE  0x3D1       /* mixer control change notify */

/****************************************************************************

		String resource number bases (internal use)

****************************************************************************/

#define MMSYSERR_BASE          0
#define WAVERR_BASE            32
#define MIDIERR_BASE           64
#define TIMERR_BASE            96
#define JOYERR_BASE            160
#define MCIERR_BASE            256
#define MIXERR_BASE            1024

#define MCI_STRING_OFFSET      512
#define MCI_VD_OFFSET          1024
#define MCI_CD_OFFSET          1088
#define MCI_WAVE_OFFSET        1152
#define MCI_SEQ_OFFSET         1216

/****************************************************************************

			General error return values

****************************************************************************/

/* general error return values */
#define MMSYSERR_NOERROR      0                    /* no error */
#define MMSYSERR_ERROR        (MMSYSERR_BASE + 1)  /* unspecified error */
#define MMSYSERR_BADDEVICEID  (MMSYSERR_BASE + 2)  /* device ID out of range */
#define MMSYSERR_NOTENABLED   (MMSYSERR_BASE + 3)  /* driver failed enable */
#define MMSYSERR_ALLOCATED    (MMSYSERR_BASE + 4)  /* device already allocated */
#define MMSYSERR_INVALHANDLE  (MMSYSERR_BASE + 5)  /* device handle is invalid */
#define MMSYSERR_NODRIVER     (MMSYSERR_BASE + 6)  /* no device driver present */
#define MMSYSERR_NOMEM        (MMSYSERR_BASE + 7)  /* memory allocation error */
#define MMSYSERR_NOTSUPPORTED (MMSYSERR_BASE + 8)  /* function isn't supported */
#define MMSYSERR_BADERRNUM    (MMSYSERR_BASE + 9)  /* error value out of range */
#define MMSYSERR_INVALFLAG    (MMSYSERR_BASE + 10) /* invalid flag passed */
#define MMSYSERR_INVALPARAM   (MMSYSERR_BASE + 11) /* invalid parameter passed */
#define MMSYSERR_HANDLEBUSY   (MMSYSERR_BASE + 12) /* handle being used */
						   /* simultaneously on another */
						   /* thread (eg callback) */
#define MMSYSERR_INVALIDALIAS (MMSYSERR_BASE + 13) /* specified alias not found */
#define MMSYSERR_BADDB        (MMSYSERR_BASE + 14) /* bad registry database */
#define MMSYSERR_KEYNOTFOUND  (MMSYSERR_BASE + 15) /* registry key not found */
#define MMSYSERR_READERROR    (MMSYSERR_BASE + 16) /* registry read error */
#define MMSYSERR_WRITEERROR   (MMSYSERR_BASE + 17) /* registry write error */
#define MMSYSERR_DELETEERROR  (MMSYSERR_BASE + 18) /* registry delete error */
#define MMSYSERR_VALNOTFOUND  (MMSYSERR_BASE + 19) /* registry value not found */
#define MMSYSERR_NODRIVERCB   (MMSYSERR_BASE + 20) /* driver does not call DriverCallback */
#define MMSYSERR_LASTERROR    (MMSYSERR_BASE + 20) /* last error in range */

#if (WINVER < 0x030a) || defined(_WIN32)
DECLARE_HANDLE(HDRVR);
#endif /* ifdef WINVER < 0x030a */

#ifndef MMNODRV

/****************************************************************************

			Installable driver support

****************************************************************************/

#ifdef _WIN32
typedef struct DRVCONFIGINFOEX {
    DWORD   dwDCISize;
    LPCWSTR  lpszDCISectionName;
    LPCWSTR  lpszDCIAliasName;
    DWORD    dnDevNode;
} DRVCONFIGINFOEX, *PDRVCONFIGINFOEX, NEAR *NPDRVCONFIGINFOEX, FAR *LPDRVCONFIGINFOEX;

#else
typedef struct DRVCONFIGINFOEX {
    DWORD   dwDCISize;
    LPCSTR  lpszDCISectionName;
    LPCSTR  lpszDCIAliasName;
    DWORD    dnDevNode;
} DRVCONFIGINFOEX, *PDRVCONFIGINFOEX, NEAR *NPDRVCONFIGINFOEX, FAR *LPDRVCONFIGINFOEX;
#endif

#if (WINVER < 0x030a) || defined(_WIN32)

#ifndef DRV_LOAD

/* Driver messages */
#define DRV_LOAD                0x0001
#define DRV_ENABLE              0x0002
#define DRV_OPEN                0x0003
#define DRV_CLOSE               0x0004
#define DRV_DISABLE             0x0005
#define DRV_FREE                0x0006
#define DRV_CONFIGURE           0x0007
#define DRV_QUERYCONFIGURE      0x0008
#define DRV_INSTALL             0x0009
#define DRV_REMOVE              0x000A
#define DRV_EXITSESSION         0x000B
#define DRV_POWER               0x000F
#define DRV_RESERVED            0x0800
#define DRV_USER                0x4000

/* LPARAM of DRV_CONFIGURE message */
#ifdef _WIN32
typedef struct tagDRVCONFIGINFO {
    DWORD   dwDCISize;
    LPCWSTR  lpszDCISectionName;
    LPCWSTR  lpszDCIAliasName;
} DRVCONFIGINFO, *PDRVCONFIGINFO, NEAR *NPDRVCONFIGINFO, FAR *LPDRVCONFIGINFO;
#else
typedef struct tagDRVCONFIGINFO {
    DWORD   dwDCISize;
    LPCSTR  lpszDCISectionName;
    LPCSTR  lpszDCIAliasName;
} DRVCONFIGINFO, *PDRVCONFIGINFO, NEAR *NPDRVCONFIGINFO, FAR *LPDRVCONFIGINFO;
#endif

/* Supported return values for DRV_CONFIGURE message */
#define DRVCNF_CANCEL           0x0000
#define DRVCNF_OK               0x0001
#define DRVCNF_RESTART          0x0002

/* installable driver function prototypes */
#ifdef _WIN32
 #ifndef _WIN32_VXD

typedef LRESULT (CALLBACK* DRIVERPROC)(DWORD, HDRVR, UINT, LPARAM, LPARAM);

WINMMAPI LRESULT   WINAPI CloseDriver(HDRVR hDriver, LONG lParam1, LONG lParam2);
WINMMAPI HDRVR     WINAPI OpenDriver(LPCWSTR szDriverName, LPCWSTR szSectionName, LONG lParam2);
WINMMAPI LRESULT   WINAPI SendDriverMessage(HDRVR hDriver, UINT message, LONG lParam1, LONG lParam2);
WINMMAPI HMODULE   WINAPI DrvGetModuleHandle(HDRVR hDriver);
WINMMAPI HMODULE   WINAPI GetDriverModuleHandle(HDRVR hDriver);
WINMMAPI LRESULT   WINAPI DefDriverProc(DWORD dwDriverIdentifier, HDRVR hdrvr, UINT uMsg, LPARAM lParam1, LPARAM lParam2);
 #endif /* ifndef _WIN32_VXD */
#else
LRESULT   WINAPI DrvClose(HDRVR hdrvr, LPARAM lParam1, LPARAM lParam2);
HDRVR     WINAPI DrvOpen(LPCSTR szDriverName, LPCSTR szSectionName, LPARAM lParam2);
LRESULT   WINAPI DrvSendMessage(HDRVR hdrvr, UINT uMsg, LPARAM lParam1, LPARAM lParam2);
HINSTANCE WINAPI DrvGetModuleHandle(HDRVR hdrvr);
LRESULT   WINAPI DrvDefDriverProc(DWORD dwDriverIdentifier, HDRVR hdrvr, UINT uMsg, LPARAM lParam1, LPARAM lParam2);
#define DefDriverProc DrvDefDriverProc
#endif /* ifdef _WIN32 */
#endif /* DRV_LOAD */
#endif /* ifdef (WINVER < 0x030a) || defined(_WIN32) */

#if (WINVER >= 0x030a)
/* return values from DriverProc() function */
#define DRV_CANCEL             DRVCNF_CANCEL
#define DRV_OK                 DRVCNF_OK
#define DRV_RESTART            DRVCNF_RESTART

#endif /* ifdef WINVER >= 0x030a */

#define DRV_MCI_FIRST          DRV_RESERVED
#define DRV_MCI_LAST           (DRV_RESERVED + 0xFFF)

#endif  /* ifndef MMNODRV */

/****************************************************************************

			  Driver callback support

****************************************************************************/

/* flags used with waveOutOpen(), waveInOpen(), midiInOpen(), and */
/* midiOutOpen() to specify the type of the dwCallback parameter. */

#define CALLBACK_TYPEMASK   0x00070000l    /* callback type mask */
#define CALLBACK_NULL       0x00000000l    /* no callback */
#define CALLBACK_WINDOW     0x00010000l    /* dwCallback is a HWND */
#define CALLBACK_TASK       0x00020000l    /* dwCallback is a HTASK */
#define CALLBACK_FUNCTION   0x00030000l    /* dwCallback is a FARPROC */
#ifdef _WIN32
#define CALLBACK_THREAD     (CALLBACK_TASK)/* thread ID replaces 16 bit task */
#define CALLBACK_EVENT      0x00050000l    /* dwCallback is an EVENT Handle */
#endif

#ifndef _WIN32_VXD 

typedef void (CALLBACK DRVCALLBACK)(HDRVR hdrvr, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);

typedef DRVCALLBACK FAR *LPDRVCALLBACK;
#ifdef _WIN32
typedef DRVCALLBACK     *PDRVCALLBACK;
#endif

#endif   /* ifndef _WIN32_VXD */ 

#ifndef MMNOMMSYSTEM
#ifndef _WIN32_VXD
/****************************************************************************

		    General MMSYSTEM support

****************************************************************************/

#if (WINVER <= 0x030A)
WINMMAPI UINT WINAPI mmsystemGetVersion(void);
#endif
#ifdef _WIN32
#define OutputDebugStr  OutputDebugString
#else
void WINAPI OutputDebugStr(LPCSTR);
#endif

#endif  /* ifndef _WIN32_VXD */
#endif  /* ifndef MMNOMMSYSTEM */

#ifndef MMNOSOUND
#ifndef _WIN32_VXD
/****************************************************************************

			    Sound support

****************************************************************************/

#ifdef _WIN32

WINMMAPI BOOL WINAPI sndPlaySoundA(LPCSTR pszSound, UINT fuSound);
WINMMAPI BOOL WINAPI sndPlaySoundW(LPCWSTR pszSound, UINT fuSound);
#ifdef UNICODE
#define sndPlaySound  sndPlaySoundW
#else
#define sndPlaySound  sndPlaySoundA
#endif // !UNICODE

#else
BOOL WINAPI sndPlaySound(LPCSTR pszSound, UINT fuSound);
#endif

/*
 *  flag values for fuSound and fdwSound arguments on [snd]PlaySound
 */
#define SND_SYNC            0x0000  /* play synchronously (default) */
#define SND_ASYNC           0x0001  /* play asynchronously */
#define SND_NODEFAULT       0x0002  /* silence (!default) if sound not found */
#define SND_MEMORY          0x0004  /* pszSound points to a memory file */
#define SND_LOOP            0x0008  /* loop the sound until next sndPlaySound */
#define SND_NOSTOP          0x0010  /* don't stop any currently playing sound */

#define SND_NOWAIT	0x00002000L /* don't wait if the driver is busy */
#define SND_ALIAS       0x00010000L /* name is a registry alias */
#define SND_ALIAS_ID	0x00110000L /* alias is a predefined ID */
#define SND_FILENAME    0x00020000L /* name is file name */
#define SND_RESOURCE    0x00040004L /* name is resource name or atom */
#if(WINVER >= 0x0400)
#define SND_PURGE           0x0040  /* purge non-static events for task */
#define SND_APPLICATION     0x0080  /* look for application specific association */
#endif /* WINVER >= 0x0400 */

#define SND_ALIAS_START	0           /* alias base */

#ifdef _WIN32
#define	sndAlias(ch0, ch1)	(SND_ALIAS_START + (DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8))

#define SND_ALIAS_SYSTEMASTERISK        sndAlias('S', '*')
#define SND_ALIAS_SYSTEMQUESTION        sndAlias('S', '?')
#define SND_ALIAS_SYSTEMHAND            sndAlias('S', 'H')
#define SND_ALIAS_SYSTEMEXIT            sndAlias('S', 'E')
#define SND_ALIAS_SYSTEMSTART           sndAlias('S', 'S')
#define SND_ALIAS_SYSTEMWELCOME         sndAlias('S', 'W')
#define SND_ALIAS_SYSTEMEXCLAMATION     sndAlias('S', '!')
#define SND_ALIAS_SYSTEMDEFAULT         sndAlias('S', 'D')

WINMMAPI BOOL WINAPI PlaySoundA(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound);
WINMMAPI BOOL WINAPI PlaySoundW(LPCWSTR pszSound, HMODULE hmod, DWORD fdwSound);
#ifdef UNICODE
#define PlaySound  PlaySoundW
#else
#define PlaySound  PlaySoundA
#endif // !UNICODE

#else
BOOL WINAPI PlaySound(LPCSTR pszSound, HMODULE hmod, DWORD fdwSound);
#endif

#endif  /* ifndef _WIN32_VXD */
#endif  /* ifndef MMNOSOUND */

#ifndef MMNOWAVE
/****************************************************************************

			Waveform audio support

****************************************************************************/

/* waveform audio error return values */
#define WAVERR_BADFORMAT      (WAVERR_BASE + 0)    /* unsupported wave format */
#define WAVERR_STILLPLAYING   (WAVERR_BASE + 1)    /* still something playing */
#define WAVERR_UNPREPARED     (WAVERR_BASE + 2)    /* header not prepared */
#define WAVERR_SYNC           (WAVERR_BASE + 3)    /* device is synchronous */
#define WAVERR_LASTERROR      (WAVERR_BASE + 3)    /* last error in range */

/* waveform audio data types */
DECLARE_HANDLE(HWAVE);
DECLARE_HANDLE(HWAVEIN);
DECLARE_HANDLE(HWAVEOUT);
#ifndef _WIN32_VXD
typedef HWAVEIN FAR *LPHWAVEIN;
typedef HWAVEOUT FAR *LPHWAVEOUT;
typedef DRVCALLBACK WAVECALLBACK;
typedef WAVECALLBACK FAR *LPWAVECALLBACK;
#endif

/* wave callback messages */
#define WOM_OPEN        MM_WOM_OPEN
#define WOM_CLOSE       MM_WOM_CLOSE
#define WOM_DONE        MM_WOM_DONE
#define WIM_OPEN        MM_WIM_OPEN
#define WIM_CLOSE       MM_WIM_CLOSE
#define WIM_DATA        MM_WIM_DATA

/* device ID for wave device mapper */
#define WAVE_MAPPER     ((UINT)-1)

/* flags for dwFlags parameter in waveOutOpen() and waveInOpen() */
#define  WAVE_FORMAT_QUERY         0x0001
#define  WAVE_ALLOWSYNC            0x0002
#if(WINVER >= 0x0400)
#define  WAVE_MAPPED               0x0004
#define  WAVE_FORMAT_DIRECT        0x0008
#define  WAVE_FORMAT_DIRECT_QUERY  (WAVE_FORMAT_QUERY | WAVE_FORMAT_DIRECT)
#endif /* WINVER >= 0x0400 */

/* wave data block header */
typedef struct wavehdr_tag {
    LPSTR       lpData;                 /* pointer to locked data buffer */
    DWORD       dwBufferLength;         /* length of data buffer */
    DWORD       dwBytesRecorded;        /* used for input only */
    DWORD       dwUser;                 /* for client's use */
    DWORD       dwFlags;                /* assorted flags (see defines) */
    DWORD       dwLoops;                /* loop control counter */
    struct wavehdr_tag FAR *lpNext;     /* reserved for driver */
    DWORD       reserved;               /* reserved for driver */
} WAVEHDR, *PWAVEHDR, NEAR *NPWAVEHDR, FAR *LPWAVEHDR;

/* flags for dwFlags field of WAVEHDR */
#define WHDR_DONE       0x00000001  /* done bit */
#define WHDR_PREPARED   0x00000002  /* set if this header has been prepared */
#define WHDR_BEGINLOOP  0x00000004  /* loop start block */
#define WHDR_ENDLOOP    0x00000008  /* loop end block */
#define WHDR_INQUEUE    0x00000010  /* reserved for driver */

/* waveform output device capabilities structure */
#ifdef _WIN32

typedef struct tagWAVEOUTCAPSA {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    MMVERSION vDriverVersion;      /* version of the driver */
    CHAR    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    DWORD   dwFormats;             /* formats supported */
    WORD    wChannels;             /* number of sources supported */
    WORD    wReserved1;            /* packing */
    DWORD   dwSupport;             /* functionality supported by driver */
} WAVEOUTCAPSA, *PWAVEOUTCAPSA, *NPWAVEOUTCAPSA, *LPWAVEOUTCAPSA;
typedef struct tagWAVEOUTCAPSW {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    MMVERSION vDriverVersion;      /* version of the driver */
    WCHAR   szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    DWORD   dwFormats;             /* formats supported */
    WORD    wChannels;             /* number of sources supported */
    WORD    wReserved1;            /* packing */
    DWORD   dwSupport;             /* functionality supported by driver */
} WAVEOUTCAPSW, *PWAVEOUTCAPSW, *NPWAVEOUTCAPSW, *LPWAVEOUTCAPSW;
#ifdef UNICODE
typedef WAVEOUTCAPSW WAVEOUTCAPS;
typedef PWAVEOUTCAPSW PWAVEOUTCAPS;
typedef NPWAVEOUTCAPSW NPWAVEOUTCAPS;
typedef LPWAVEOUTCAPSW LPWAVEOUTCAPS;
#else
typedef WAVEOUTCAPSA WAVEOUTCAPS;
typedef PWAVEOUTCAPSA PWAVEOUTCAPS;
typedef NPWAVEOUTCAPSA NPWAVEOUTCAPS;
typedef LPWAVEOUTCAPSA LPWAVEOUTCAPS;
#endif // UNICODE

#else
typedef struct waveoutcaps_tag {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    VERSION vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    DWORD   dwFormats;             /* formats supported */
    WORD    wChannels;             /* number of sources supported */
    DWORD   dwSupport;             /* functionality supported by driver */
} WAVEOUTCAPS, *PWAVEOUTCAPS, NEAR *NPWAVEOUTCAPS, FAR *LPWAVEOUTCAPS;
#endif

/* flags for dwSupport field of WAVEOUTCAPS */
#define WAVECAPS_PITCH          0x0001   /* supports pitch control */
#define WAVECAPS_PLAYBACKRATE   0x0002   /* supports playback rate control */
#define WAVECAPS_VOLUME         0x0004   /* supports volume control */
#define WAVECAPS_LRVOLUME       0x0008   /* separate left-right volume control */
#define WAVECAPS_SYNC           0x0010
#define WAVECAPS_SAMPLEACCURATE 0x0020
#define WAVECAPS_DIRECTSOUND    0x0040

/* waveform input device capabilities structure */
#ifdef _WIN32

typedef struct tagWAVEINCAPSA {
    WORD    wMid;                    /* manufacturer ID */
    WORD    wPid;                    /* product ID */
    MMVERSION vDriverVersion;        /* version of the driver */
    CHAR    szPname[MAXPNAMELEN];    /* product name (NULL terminated string) */
    DWORD   dwFormats;               /* formats supported */
    WORD    wChannels;               /* number of channels supported */
    WORD    wReserved1;              /* structure packing */
} WAVEINCAPSA, *PWAVEINCAPSA, *NPWAVEINCAPSA, *LPWAVEINCAPSA;
typedef struct tagWAVEINCAPSW {
    WORD    wMid;                    /* manufacturer ID */
    WORD    wPid;                    /* product ID */
    MMVERSION vDriverVersion;        /* version of the driver */
    WCHAR   szPname[MAXPNAMELEN];    /* product name (NULL terminated string) */
    DWORD   dwFormats;               /* formats supported */
    WORD    wChannels;               /* number of channels supported */
    WORD    wReserved1;              /* structure packing */
} WAVEINCAPSW, *PWAVEINCAPSW, *NPWAVEINCAPSW, *LPWAVEINCAPSW;
#ifdef UNICODE
typedef WAVEINCAPSW WAVEINCAPS;
typedef PWAVEINCAPSW PWAVEINCAPS;
typedef NPWAVEINCAPSW NPWAVEINCAPS;
typedef LPWAVEINCAPSW LPWAVEINCAPS;
#else
typedef WAVEINCAPSA WAVEINCAPS;
typedef PWAVEINCAPSA PWAVEINCAPS;
typedef NPWAVEINCAPSA NPWAVEINCAPS;
typedef LPWAVEINCAPSA LPWAVEINCAPS;
#endif // UNICODE

#else
typedef struct waveincaps_tag {
    WORD    wMid;                    /* manufacturer ID */
    WORD    wPid;                    /* product ID */
    VERSION vDriverVersion;          /* version of the driver */
    char    szPname[MAXPNAMELEN];    /* product name (NULL terminated string) */
    DWORD   dwFormats;               /* formats supported */
    WORD    wChannels;               /* number of channels supported */
} WAVEINCAPS, *PWAVEINCAPS, NEAR *NPWAVEINCAPS, FAR *LPWAVEINCAPS;
#endif

/* defines for dwFormat field of WAVEINCAPS and WAVEOUTCAPS */
#define WAVE_INVALIDFORMAT     0x00000000       /* invalid format */
#define WAVE_FORMAT_1M08       0x00000001       /* 11.025 kHz, Mono,   8-bit  */
#define WAVE_FORMAT_1S08       0x00000002       /* 11.025 kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_1M16       0x00000004       /* 11.025 kHz, Mono,   16-bit */
#define WAVE_FORMAT_1S16       0x00000008       /* 11.025 kHz, Stereo, 16-bit */
#define WAVE_FORMAT_2M08       0x00000010       /* 22.05  kHz, Mono,   8-bit  */
#define WAVE_FORMAT_2S08       0x00000020       /* 22.05  kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_2M16       0x00000040       /* 22.05  kHz, Mono,   16-bit */
#define WAVE_FORMAT_2S16       0x00000080       /* 22.05  kHz, Stereo, 16-bit */
#define WAVE_FORMAT_4M08       0x00000100       /* 44.1   kHz, Mono,   8-bit  */
#define WAVE_FORMAT_4S08       0x00000200       /* 44.1   kHz, Stereo, 8-bit  */
#define WAVE_FORMAT_4M16       0x00000400       /* 44.1   kHz, Mono,   16-bit */
#define WAVE_FORMAT_4S16       0x00000800       /* 44.1   kHz, Stereo, 16-bit */

/* OLD general waveform format structure (information common to all formats) */
typedef struct waveformat_tag {
    WORD    wFormatTag;        /* format type */
    WORD    nChannels;         /* number of channels (i.e. mono, stereo, etc.) */
    DWORD   nSamplesPerSec;    /* sample rate */
    DWORD   nAvgBytesPerSec;   /* for buffer estimation */
    WORD    nBlockAlign;       /* block size of data */
} WAVEFORMAT, *PWAVEFORMAT, NEAR *NPWAVEFORMAT, FAR *LPWAVEFORMAT;

/* flags for wFormatTag field of WAVEFORMAT */
#define WAVE_FORMAT_PCM     1

/* specific waveform format structure for PCM data */
typedef struct pcmwaveformat_tag {
    WAVEFORMAT  wf;
    WORD        wBitsPerSample;
} PCMWAVEFORMAT, *PPCMWAVEFORMAT, NEAR *NPPCMWAVEFORMAT, FAR *LPPCMWAVEFORMAT;

#ifndef _WAVEFORMATEX_
#define _WAVEFORMATEX_

/*
 *  extended waveform format structure used for all non-PCM formats. this
 *  structure is common to all non-PCM formats.
 */
typedef struct tWAVEFORMATEX
{
    WORD        wFormatTag;         /* format type */
    WORD        nChannels;          /* number of channels (i.e. mono, stereo...) */
    DWORD       nSamplesPerSec;     /* sample rate */
    DWORD       nAvgBytesPerSec;    /* for buffer estimation */
    WORD        nBlockAlign;        /* block size of data */
    WORD        wBitsPerSample;     /* number of bits per sample of mono data */
    WORD        cbSize;             /* the count in bytes of the size of */
				    /* extra information (after cbSize) */
} WAVEFORMATEX, *PWAVEFORMATEX, NEAR *NPWAVEFORMATEX, FAR *LPWAVEFORMATEX;
typedef const WAVEFORMATEX FAR *LPCWAVEFORMATEX;

#endif /* _WAVEFORMATEX_ */

#ifndef _WIN32_VXD
/* waveform audio function prototypes */
WINMMAPI UINT WINAPI waveOutGetNumDevs(void);

#ifdef _WIN32

WINMMAPI MMRESULT WINAPI waveOutGetDevCapsA(UINT uDeviceID, LPWAVEOUTCAPSA pwoc, UINT cbwoc);
WINMMAPI MMRESULT WINAPI waveOutGetDevCapsW(UINT uDeviceID, LPWAVEOUTCAPSW pwoc, UINT cbwoc);
#ifdef UNICODE
#define waveOutGetDevCaps  waveOutGetDevCapsW
#else
#define waveOutGetDevCaps  waveOutGetDevCapsA
#endif // !UNICODE

#else
WINMMAPI MMRESULT WINAPI waveOutGetDevCaps(UINT uDeviceID, LPWAVEOUTCAPS pwoc, UINT cbwoc);
#endif

#if (WINVER >= 0x0400)
WINMMAPI MMRESULT WINAPI waveOutGetVolume(HWAVEOUT hwo, LPDWORD pdwVolume);
WINMMAPI MMRESULT WINAPI waveOutSetVolume(HWAVEOUT hwo, DWORD dwVolume);
#else
WINMMAPI MMRESULT WINAPI waveOutGetVolume(UINT uId, LPDWORD pdwVolume);
WINMMAPI MMRESULT WINAPI waveOutSetVolume(UINT uId, DWORD dwVolume);
#endif

#ifdef _WIN32

WINMMAPI MMRESULT WINAPI waveOutGetErrorTextA(MMRESULT mmrError, LPSTR pszText, UINT cchText);
WINMMAPI MMRESULT WINAPI waveOutGetErrorTextW(MMRESULT mmrError, LPWSTR pszText, UINT cchText);
#ifdef UNICODE
#define waveOutGetErrorText  waveOutGetErrorTextW
#else
#define waveOutGetErrorText  waveOutGetErrorTextA
#endif // !UNICODE

#else
MMRESULT WINAPI waveOutGetErrorText(MMRESULT mmrError, LPSTR pszText, UINT cchText);
#endif

WINMMAPI MMRESULT WINAPI waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID,
    LPCWAVEFORMATEX pwfx, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);

WINMMAPI MMRESULT WINAPI waveOutClose(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
WINMMAPI MMRESULT WINAPI waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
WINMMAPI MMRESULT WINAPI waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh);
WINMMAPI MMRESULT WINAPI waveOutPause(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutRestart(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutReset(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutBreakLoop(HWAVEOUT hwo);
WINMMAPI MMRESULT WINAPI waveOutGetPosition(HWAVEOUT hwo, LPMMTIME pmmt, UINT cbmmt);
WINMMAPI MMRESULT WINAPI waveOutGetPitch(HWAVEOUT hwo, LPDWORD pdwPitch);
WINMMAPI MMRESULT WINAPI waveOutSetPitch(HWAVEOUT hwo, DWORD dwPitch);
WINMMAPI MMRESULT WINAPI waveOutGetPlaybackRate(HWAVEOUT hwo, LPDWORD pdwRate);
WINMMAPI MMRESULT WINAPI waveOutSetPlaybackRate(HWAVEOUT hwo, DWORD dwRate);
WINMMAPI MMRESULT WINAPI waveOutGetID(HWAVEOUT hwo, LPUINT puDeviceID);

#if (WINVER >= 0x030a)
#ifdef _WIN32
WINMMAPI MMRESULT WINAPI waveOutMessage(HWAVEOUT hwo, UINT uMsg, DWORD dw1, DWORD dw2);
#else
DWORD WINAPI waveOutMessage(HWAVEOUT hwo, UINT uMsg, DWORD dw1, DWORD dw2);
#endif
#endif /* ifdef WINVER >= 0x030a */

WINMMAPI UINT WINAPI waveInGetNumDevs(void);

#ifdef _WIN32

WINMMAPI MMRESULT WINAPI waveInGetDevCapsA(UINT uDeviceID, LPWAVEINCAPSA pwic, UINT cbwic);
WINMMAPI MMRESULT WINAPI waveInGetDevCapsW(UINT uDeviceID, LPWAVEINCAPSW pwic, UINT cbwic);
#ifdef UNICODE
#define waveInGetDevCaps  waveInGetDevCapsW
#else
#define waveInGetDevCaps  waveInGetDevCapsA
#endif // !UNICODE

#else
MMRESULT WINAPI waveInGetDevCaps(UINT uDeviceID, LPWAVEINCAPS pwic, UINT cbwic);
#endif

#ifdef _WIN32

WINMMAPI MMRESULT WINAPI waveInGetErrorTextA(MMRESULT mmrError, LPSTR pszText, UINT cchText);
WINMMAPI MMRESULT WINAPI waveInGetErrorTextW(MMRESULT mmrError, LPWSTR pszText, UINT cchText);
#ifdef UNICODE
#define waveInGetErrorText  waveInGetErrorTextW
#else
#define waveInGetErrorText  waveInGetErrorTextA
#endif // !UNICODE

#else
MMRESULT WINAPI waveInGetErrorText(MMRESULT mmrError, LPSTR pszText, UINT cchText);
#endif

WINMMAPI MMRESULT WINAPI waveInOpen(LPHWAVEIN phwi, UINT uDeviceID,
    LPCWAVEFORMATEX pwfx, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);

WINMMAPI MMRESULT WINAPI waveInClose(HWAVEIN hwi);
WINMMAPI MMRESULT WINAPI waveInPrepareHeader(HWAVEIN hwi, LPWAVEHDR pwh, UINT cbwh);
WINMMAPI MMRESULT WINAPI waveInUnprepareHeader(HWAVEIN hwi, LPWAVEHDR pwh, UINT cbwh);
WINMMAPI MMRESULT WINAPI waveInAddBuffer(HWAVEIN hwi, LPWAVEHDR pwh, UINT cbwh);
WINMMAPI MMRESULT WINAPI waveInStart(HWAVEIN hwi);
WINMMAPI MMRESULT WINAPI waveInStop(HWAVEIN hwi);
WINMMAPI MMRESULT WINAPI waveInReset(HWAVEIN hwi);
WINMMAPI MMRESULT WINAPI waveInGetPosition(HWAVEIN hwi, LPMMTIME pmmt, UINT cbmmt);
WINMMAPI MMRESULT WINAPI waveInGetID(HWAVEIN hwi, LPUINT puDeviceID);

#if (WINVER >= 0x030a)
#ifdef _WIN32
WINMMAPI MMRESULT WINAPI waveInMessage(HWAVEIN hwi, UINT uMsg, DWORD dw1, DWORD dw2);
#else
DWORD WINAPI waveInMessage(HWAVEIN hwi, UINT uMsg, DWORD dw1, DWORD dw2);
#endif
#endif /* ifdef WINVER >= 0x030a */

#endif /* ifndef _WIN32_VXD */

#endif  /* ifndef MMNOWAVE */

#ifndef MMNOMIDI
/****************************************************************************

			    MIDI audio support

****************************************************************************/

/* MIDI error return values */
#define MIDIERR_UNPREPARED    (MIDIERR_BASE + 0)   /* header not prepared */
#define MIDIERR_STILLPLAYING  (MIDIERR_BASE + 1)   /* still something playing */
#define MIDIERR_NOMAP         (MIDIERR_BASE + 2)   /* no configured instruments */
#define MIDIERR_NOTREADY      (MIDIERR_BASE + 3)   /* hardware is still busy */
#define MIDIERR_NODEVICE      (MIDIERR_BASE + 4)   /* port no longer connected */
#define MIDIERR_INVALIDSETUP  (MIDIERR_BASE + 5)   /* invalid MIF */
#define MIDIERR_BADOPENMODE   (MIDIERR_BASE + 6)   /* operation unsupported w/ open mode */
#define MIDIERR_DONT_CONTINUE (MIDIERR_BASE + 7)   /* thru device 'eating' a message */
#define MIDIERR_LASTERROR     (MIDIERR_BASE + 7)   /* last error in range */

/* MIDI audio data types */
DECLARE_HANDLE(HMIDI);
DECLARE_HANDLE(HMIDIIN);
DECLARE_HANDLE(HMIDIOUT);
DECLARE_HANDLE(HMIDISTRM);
#ifndef _WIN32_VXD
typedef HMIDI FAR *LPHMIDI;
typedef HMIDIIN FAR *LPHMIDIIN;
typedef HMIDIOUT FAR *LPHMIDIOUT;
typedef HMIDISTRM FAR *LPHMIDISTRM;
typedef DRVCALLBACK MIDICALLBACK;
typedef MIDICALLBACK FAR *LPMIDICALLBACK;
#define MIDIPATCHSIZE   128
typedef WORD PATCHARRAY[MIDIPATCHSIZE];
typedef WORD FAR *LPPATCHARRAY;
typedef WORD KEYARRAY[MIDIPATCHSIZE];
typedef WORD FAR *LPKEYARRAY;
#endif  /* ifndef _WIN32_VXD */

/* MIDI callback messages */
#define MIM_OPEN        MM_MIM_OPEN
#define MIM_CLOSE       MM_MIM_CLOSE
#define MIM_DATA        MM_MIM_DATA
#define MIM_LONGDATA    MM_MIM_LONGDATA
#define MIM_ERROR       MM_MIM_ERROR
#define MIM_LONGERROR   MM_MIM_LONGERROR
#define MOM_OPEN        MM_MOM_OPEN
#define MOM_CLOSE       MM_MOM_CLOSE
#define MOM_DONE        MM_MOM_DONE

#if(WINVER >= 0x0400)
#define MIM_MOREDATA      MM_MIM_MOREDATA
#define MOM_POSITIONCB    MM_MOM_POSITIONCB
#endif /* WINVER >= 0x0400 */

/* device ID for MIDI mapper */
#define MIDIMAPPER     ((UINT)-1)
#define MIDI_MAPPER    ((UINT)-1)

#if(WINVER >= 0x0400)
/* flags for dwFlags parm of midiInOpen() */
#define MIDI_IO_STATUS      0x00000020L
#endif /* WINVER >= 0x0400 */

/* flags for wFlags parm of midiOutCachePatches(), midiOutCacheDrumPatches() */
#define MIDI_CACHE_ALL      1
#define MIDI_CACHE_BESTFIT  2
#define MIDI_CACHE_QUERY    3
#define MIDI_UNCACHE        4

/* MIDI output device capabilities structure */
#ifdef _WIN32

typedef struct tagMIDIOUTCAPSA {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    MMVERSION vDriverVersion;      /* version of the driver */
    CHAR    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    WORD    wTechnology;           /* type of device */
    WORD    wVoices;               /* # of voices (internal synth only) */
    WORD    wNotes;                /* max # of notes (internal synth only) */
    WORD    wChannelMask;          /* channels used (internal synth only) */
    DWORD   dwSupport;             /* functionality supported by driver */
} MIDIOUTCAPSA, *PMIDIOUTCAPSA, *NPMIDIOUTCAPSA, *LPMIDIOUTCAPSA;
typedef struct tagMIDIOUTCAPSW {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    MMVERSION vDriverVersion;      /* version of the driver */
    WCHAR   szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    WORD    wTechnology;           /* type of device */
    WORD    wVoices;               /* # of voices (internal synth only) */
    WORD    wNotes;                /* max # of notes (internal synth only) */
    WORD    wChannelMask;          /* channels used (internal synth only) */
    DWORD   dwSupport;             /* functionality supported by driver */
} MIDIOUTCAPSW, *PMIDIOUTCAPSW, *NPMIDIOUTCAPSW, *LPMIDIOUTCAPSW;
#ifdef UNICODE
typedef MIDIOUTCAPSW MIDIOUTCAPS;
typedef PMIDIOUTCAPSW PMIDIOUTCAPS;
typedef NPMIDIOUTCAPSW NPMIDIOUTCAPS;
typedef LPMIDIOUTCAPSW LPMIDIOUTCAPS;
#else
typedef MIDIOUTCAPSA MIDIOUTCAPS;
typedef PMIDIOUTCAPSA PMIDIOUTCAPS;
typedef NPMIDIOUTCAPSA NPMIDIOUTCAPS;
typedef LPMIDIOUTCAPSA LPMIDIOUTCAPS;
#endif // UNICODE

#else
typedef struct midioutcaps_tag {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    VERSION vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    WORD    wTechnology;           /* type of device */
    WORD    wVoices;               /* # of voices (internal synth only) */
    WORD    wNotes;                /* max # of notes (internal synth only) */
    WORD    wChannelMask;          /* channels used (internal synth only) */
    DWORD   dwSupport;             /* functionality supported by driver */
} MIDIOUTCAPS, *PMIDIOUTCAPS, NEAR *NPMIDIOUTCAPS, FAR *LPMIDIOUTCAPS;
#endif

/* flags for wTechnology field of MIDIOUTCAPS structure */
#define MOD_MIDIPORT    1  /* output port */
#define MOD_SYNTH       2  /* generic internal synth */
#define MOD_SQSYNTH     3  /* square wave internal synth */
#define MOD_FMSYNTH     4  /* FM internal synth */
#define MOD_MAPPER      5  /* MIDI mapper */

/* flags for dwSupport field of MIDIOUTCAPS structure */
#define MIDICAPS_VOLUME          0x0001  /* supports volume control */
#define MIDICAPS_LRVOLUME        0x0002  /* separate left-right volume control */
#define MIDICAPS_CACHE           0x0004
#if(WINVER >= 0x0400)
#define MIDICAPS_STREAM          0x0008  /* driver supports midiStreamOut directly */
#endif /* WINVER >= 0x0400 */

/* MIDI input device capabilities structure */
#ifdef _WIN32

typedef struct tagMIDIINCAPSA {
    WORD        wMid;                   /* manufacturer ID */
    WORD        wPid;                   /* product ID */
    MMVERSION   vDriverVersion;         /* version of the driver */
    CHAR        szPname[MAXPNAMELEN];   /* product name (NULL terminated string) */
#if (WINVER >= 0x0400)
    DWORD   dwSupport;             /* functionality supported by driver */
#endif
} MIDIINCAPSA, *PMIDIINCAPSA, *NPMIDIINCAPSA, *LPMIDIINCAPSA;
typedef struct tagMIDIINCAPSW {
    WORD        wMid;                   /* manufacturer ID */
    WORD        wPid;                   /* product ID */
    MMVERSION   vDriverVersion;         /* version of the driver */
    WCHAR       szPname[MAXPNAMELEN];   /* product name (NULL terminated string) */
#if (WINVER >= 0x0400)
    DWORD   dwSupport;             /* functionality supported by driver */
#endif
} MIDIINCAPSW, *PMIDIINCAPSW, *NPMIDIINCAPSW, *LPMIDIINCAPSW;
#ifdef UNICODE
typedef MIDIINCAPSW MIDIINCAPS;
typedef PMIDIINCAPSW PMIDIINCAPS;
typedef NPMIDIINCAPSW NPMIDIINCAPS;
typedef LPMIDIINCAPSW LPMIDIINCAPS;
#else
typedef MIDIINCAPSA MIDIINCAPS;
typedef PMIDIINCAPSA PMIDIINCAPS;
typedef NPMIDIINCAPSA NPMIDIINCAPS;
typedef LPMIDIINCAPSA LPMIDIINCAPS;
#endif // UNICODE

#else
typedef struct midiincaps_tag {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    VERSION vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
#if (WINVER >= 0x0400)
    DWORD   dwSupport;             /* functionality supported by driver */
#endif
} MIDIINCAPS, *PMIDIINCAPS, NEAR *NPMIDIINCAPS, FAR *LPMIDIINCAPS;
#endif

/* MIDI data block header */
typedef struct midihdr_tag {
    LPSTR       lpData;               /* pointer to locked data block */
    DWORD       dwBufferLength;       /* length of data in data block */
    DWORD       dwBytesRecorded;      /* used for input only */
    DWORD       dwUser;               /* for client's use */
    DWORD       dwFlags;              /* assorted flags (see defines) */
    struct midihdr_tag FAR *lpNext;   /* reserved for driver */
    DWORD       reserved;             /* reserved for driver */
#if (WINVER >= 0x0400)
    DWORD       dwOffset;             /* Callback offset into buffer */
    DWORD       dwReserved[8];        /* Reserved for MMSYSTEM */
#endif
} MIDIHDR, *PMIDIHDR, NEAR *NPMIDIHDR, FAR *LPMIDIHDR;

#if(WINVER >= 0x0400)
typedef struct midievent_tag
{
    DWORD       dwDeltaTime;          /* Ticks since last event */
    DWORD       dwStreamID;           /* Reserved; must be zero */
    DWORD       dwEvent;              /* Event type and parameters */
    DWORD       dwParms[1];           /* Parameters if this is a long event */
} MIDIEVENT;

typedef struct midistrmbuffver_tag
{
    DWORD       dwVersion;                  /* Stream buffer format version */
    DWORD       dwMid;                      /* Manufacturer ID as defined in MMREG.H */
    DWORD       dwOEMVersion;               /* Manufacturer version for custom ext */
} MIDISTRMBUFFVER;
#endif /* WINVER >= 0x0400 */

/* flags for dwFlags field of MIDIHDR structure */
#define MHDR_DONE       0x00000001       /* done bit */
#define MHDR_PREPARED   0x00000002       /* set if header prepared */
#define MHDR_INQUEUE    0x00000004       /* reserved for driver */
#define MHDR_ISSTRM     0x00000008       /* Buffer is stream buffer */

#if(WINVER >= 0x0400)
/* */
/* Type codes which go in the high byte of the event DWORD of a stream buffer */
/* */
/* Type codes 00-7F contain parameters within the low 24 bits */
/* Type codes 80-FF contain a length of their parameter in the low 24 */
/* bits, followed by their parameter data in the buffer. The event */
/* DWORD contains the exact byte length; the parm data itself must be */
/* padded to be an even multiple of 4 bytes long. */
/* */

#define MEVT_F_SHORT        0x00000000L
#define MEVT_F_LONG         0x80000000L
#define MEVT_F_CALLBACK     0x40000000L

#define MEVT_EVENTTYPE(x)   ((BYTE)(((x)>>24)&0xFF))
#define MEVT_EVENTPARM(x)   ((DWORD)((x)&0x00FFFFFFL))

#define MEVT_SHORTMSG       ((BYTE)0x00)    /* parm = shortmsg for midiOutShortMsg */
#define MEVT_TEMPO          ((BYTE)0x01)    /* parm = new tempo in microsec/qn     */
#define MEVT_NOP            ((BYTE)0x02)    /* parm = unused; does nothing         */

/* 0x04-0x7F reserved */

#define MEVT_LONGMSG        ((BYTE)0x80)    /* parm = bytes to send verbatim       */
#define MEVT_COMMENT        ((BYTE)0x82)    /* parm = comment data                 */
#define MEVT_VERSION        ((BYTE)0x84)    /* parm = MIDISTRMBUFFVER struct       */

/* 0x81-0xFF reserved */

#define MIDISTRM_ERROR      (-2)

/* */
/* Structures and defines for midiStreamProperty */
/* */
#define MIDIPROP_SET        0x80000000L
#define MIDIPROP_GET        0x40000000L

/* These are intentionally both non-zero so the app cannot accidentally */
/* leave the operation off and happen to appear to work due to default */
/* action. */

#define MIDIPROP_TIMEDIV    0x00000001L
#define MIDIPROP_TEMPO      0x00000002L

typedef struct midiproptimediv_tag
{
    DWORD       cbStruct;
    DWORD       dwTimeDiv;
} MIDIPROPTIMEDIV, FAR *LPMIDIPROPTIMEDIV;

typedef struct midiproptempo_tag
{
    DWORD       cbStruct;
    DWORD       dwTempo;
} MIDIPROPTEMPO, FAR *LPMIDIPROPTEMPO;

#endif /* WINVER >= 0x0400 */

#ifndef _WIN32_VXD

/* MIDI function prototypes */
WINMMAPI UINT WINAPI midiOutGetNumDevs(void);
#if(WINVER >= 0x0400)
WINMMAPI MMRESULT WINAPI midiStreamOpen(LPHMIDISTRM phms, LPUINT puDeviceID, DWORD cMidi, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);
WINMMAPI MMRESULT WINAPI midiStreamClose(HMIDISTRM hms);

WINMMAPI MMRESULT WINAPI midiStreamProperty(HMIDISTRM hms, LPBYTE lppropdata, DWORD dwProperty);
WINMMAPI MMRESULT WINAPI midiStreamPosition(HMIDISTRM hms, LPMMTIME lpmmt, UINT cbmmt);

WINMMAPI MMRESULT WINAPI midiStreamOut(HMIDISTRM hms, LPMIDIHDR pmh, UINT cbmh);
WINMMAPI MMRESULT WINAPI midiStreamPause(HMIDISTRM hms);
WINMMAPI MMRESULT WINAPI midiStreamRestart(HMIDISTRM hms);
WINMMAPI MMRESULT WINAPI midiStreamStop(HMIDISTRM hms);

#ifdef _WIN32
WINMMAPI MMRESULT WINAPI midiConnect(HMIDI hmi, HMIDIOUT hmo, LPVOID pReserved);
WINMMAPI MMRESULT WINAPI midiDisconnect(HMIDI hmi, HMIDIOUT hmo, LPVOID pReserved);
#endif
#endif /* WINVER >= 0x0400 */

#ifdef _WIN32

WINMMAPI MMRESULT WINAPI midiOutGetDevCapsA(UINT uDeviceID, LPMIDIOUTCAPSA pmoc, UINT cbmoc);
WINMMAPI MMRESULT WINAPI midiOutGetDevCapsW(UINT uDeviceID, LPMIDIOUTCAPSW pmoc, UINT cbmoc);
#ifdef UNICODE
#define midiOutGetDevCaps  midiOutGetDevCapsW
#else
#define midiOutGetDevCaps  midiOutGetDevCapsA
#endif // !UNICODE

#else
MMRESULT WINAPI midiOutGetDevCaps(UINT uDeviceID, LPMIDIOUTCAPS pmoc, UINT cbmoc);
#endif

#if (WINVER >= 0x0400)
WINMMAPI MMRESULT WINAPI midiOutGetVolume(HMIDIOUT hmo, LPDWORD pdwVolume);
WINMMAPI MMRESULT WINAPI midiOutSetVolume(HMIDIOUT hmo, DWORD dwVolume);
#else
WINMMAPI MMRESULT WINAPI midiOutGetVolume(UINT uId, LPDWORD pdwVolume);
WINMMAPI MMRESULT WINAPI midiOutSetVolume(UINT uId, DWORD dwVolume);
#endif

#ifdef _WIN32

WINMMAPI MMRESULT WINAPI midiOutGetErrorTextA(MMRESULT mmrError, LPSTR pszText, UINT cchText);
WINMMAPI MMRESULT WINAPI midiOutGetErrorTextW(MMRESULT mmrError, LPWSTR pszText, UINT cchText);
#ifdef UNICODE
#define midiOutGetErrorText  midiOutGetErrorTextW
#else
#define midiOutGetErrorText  midiOutGetErrorTextA
#endif // !UNICODE

#else
WINMMAPI MMRESULT WINAPI midiOutGetErrorText(MMRESULT mmrError, LPSTR pszText, UINT cchText);
#endif

WINMMAPI MMRESULT WINAPI midiOutOpen(LPHMIDIOUT phmo, UINT uDeviceID,
    DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);
WINMMAPI MMRESULT WINAPI midiOutClose(HMIDIOUT hmo);
WINMMAPI MMRESULT WINAPI midiOutPrepareHeader(HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh);
WINMMAPI MMRESULT WINAPI midiOutUnprepareHeader(HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh);
WINMMAPI MMRESULT WINAPI midiOutShortMsg(HMIDIOUT hmo, DWORD dwMsg);
WINMMAPI MMRESULT WINAPI midiOutLongMsg(HMIDIOUT hmo, LPMIDIHDR pmh, UINT cbmh);
WINMMAPI MMRESULT WINAPI midiOutReset(HMIDIOUT hmo);
WINMMAPI MMRESULT WINAPI midiOutCachePatches(HMIDIOUT hmo, UINT uBank, LPWORD pwpa, UINT fuCache);
WINMMAPI MMRESULT WINAPI midiOutCacheDrumPatches(HMIDIOUT hmo, UINT uPatch, LPWORD pwkya, UINT fuCache);
WINMMAPI MMRESULT WINAPI midiOutGetID(HMIDIOUT hmo, LPUINT puDeviceID);

#if (WINVER >= 0x030a)
#ifdef _WIN32
WINMMAPI MMRESULT WINAPI midiOutMessage(HMIDIOUT hmo, UINT uMsg, DWORD dw1, DWORD dw2);
#else
DWORD WINAPI midiOutMessage(HMIDIOUT hmo, UINT uMsg, DWORD dw1, DWORD dw2);
#endif
#endif /* ifdef WINVER >= 0x030a */

WINMMAPI UINT WINAPI midiInGetNumDevs(void);

#ifdef _WIN32

WINMMAPI MMRESULT WINAPI midiInGetDevCapsA(UINT uDeviceID, LPMIDIINCAPSA pmic, UINT cbmic);
WINMMAPI MMRESULT WINAPI midiInGetDevCapsW(UINT uDeviceID, LPMIDIINCAPSW pmic, UINT cbmic);
#ifdef UNICODE
#define midiInGetDevCaps  midiInGetDevCapsW
#else
#define midiInGetDevCaps  midiInGetDevCapsA
#endif // !UNICODE
WINMMAPI MMRESULT WINAPI midiInGetErrorTextA(MMRESULT mmrError, LPSTR pszText, UINT cchText);
WINMMAPI MMRESULT WINAPI midiInGetErrorTextW(MMRESULT mmrError, LPWSTR pszText, UINT cchText);
#ifdef UNICODE
#define midiInGetErrorText  midiInGetErrorTextW
#else
#define midiInGetErrorText  midiInGetErrorTextA
#endif // !UNICODE

#else
MMRESULT WINAPI midiInGetDevCaps(UINT uDeviceID, LPMIDIINCAPS pmic, UINT cbmic);
WINMMAPI MMRESULT WINAPI midiInGetErrorText(MMRESULT mmrError, LPSTR pszText, UINT cchText);
#endif

WINMMAPI MMRESULT WINAPI midiInOpen(LPHMIDIIN phmi, UINT uDeviceID,
	DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);
WINMMAPI MMRESULT WINAPI midiInClose(HMIDIIN hmi);
WINMMAPI MMRESULT WINAPI midiInPrepareHeader(HMIDIIN hmi, LPMIDIHDR pmh, UINT cbmh);
WINMMAPI MMRESULT WINAPI midiInUnprepareHeader(HMIDIIN hmi, LPMIDIHDR pmh, UINT cbmh);
WINMMAPI MMRESULT WINAPI midiInAddBuffer(HMIDIIN hmi, LPMIDIHDR pmh, UINT cbmh);
WINMMAPI MMRESULT WINAPI midiInStart(HMIDIIN hmi);
WINMMAPI MMRESULT WINAPI midiInStop(HMIDIIN hmi);
WINMMAPI MMRESULT WINAPI midiInReset(HMIDIIN hmi);
WINMMAPI MMRESULT WINAPI midiInGetID(HMIDIIN hmi, LPUINT puDeviceID);

#if (WINVER >= 0x030a)
#ifdef _WIN32
WINMMAPI MMRESULT WINAPI midiInMessage(HMIDIIN hmi, UINT uMsg, DWORD dw1, DWORD dw2);
#else
DWORD WINAPI midiInMessage(HMIDIIN hmi, UINT uMsg, DWORD dw1, DWORD dw2);
#endif
#endif /* ifdef WINVER >= 0x030a */

#endif /* ifndef _WIN32_VXD */

#endif  /* ifndef MMNOMIDI */

#ifndef MMNOAUX
/****************************************************************************

			Auxiliary audio support

****************************************************************************/

/* device ID for aux device mapper */
#define AUX_MAPPER     ((UINT)-1)

/* Auxiliary audio device capabilities structure */
#ifdef _WIN32

typedef struct tagAUXCAPSA {
    WORD        wMid;                /* manufacturer ID */
    WORD        wPid;                /* product ID */
    MMVERSION   vDriverVersion;      /* version of the driver */
    CHAR        szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    WORD        wTechnology;         /* type of device */
    WORD        wReserved1;          /* padding */
    DWORD       dwSupport;           /* functionality supported by driver */
} AUXCAPSA, *PAUXCAPSA, *NPAUXCAPSA, *LPAUXCAPSA;
typedef struct tagAUXCAPSW {
    WORD        wMid;                /* manufacturer ID */
    WORD        wPid;                /* product ID */
    MMVERSION   vDriverVersion;      /* version of the driver */
    WCHAR       szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    WORD        wTechnology;         /* type of device */
    WORD        wReserved1;          /* padding */
    DWORD       dwSupport;           /* functionality supported by driver */
} AUXCAPSW, *PAUXCAPSW, *NPAUXCAPSW, *LPAUXCAPSW;
#ifdef UNICODE
typedef AUXCAPSW AUXCAPS;
typedef PAUXCAPSW PAUXCAPS;
typedef NPAUXCAPSW NPAUXCAPS;
typedef LPAUXCAPSW LPAUXCAPS;
#else
typedef AUXCAPSA AUXCAPS;
typedef PAUXCAPSA PAUXCAPS;
typedef NPAUXCAPSA NPAUXCAPS;
typedef LPAUXCAPSA LPAUXCAPS;
#endif // UNICODE

#else
typedef struct auxcaps_tag {
    WORD    wMid;                  /* manufacturer ID */
    WORD    wPid;                  /* product ID */
    VERSION vDriverVersion;        /* version of the driver */
    char    szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    WORD    wTechnology;           /* type of device */
    DWORD   dwSupport;             /* functionality supported by driver */
} AUXCAPS, *PAUXCAPS, NEAR *NPAUXCAPS, FAR *LPAUXCAPS;
#endif

/* flags for wTechnology field in AUXCAPS structure */
#define AUXCAPS_CDAUDIO    1       /* audio from internal CD-ROM drive */
#define AUXCAPS_AUXIN      2       /* audio from auxiliary input jacks */

/* flags for dwSupport field in AUXCAPS structure */
#define AUXCAPS_VOLUME          0x0001  /* supports volume control */
#define AUXCAPS_LRVOLUME        0x0002  /* separate left-right volume control */

#ifndef _WIN32_VXD

/* auxiliary audio function prototypes */
WINMMAPI UINT WINAPI auxGetNumDevs(void);
#ifdef _WIN32

WINMMAPI MMRESULT WINAPI auxGetDevCapsA(UINT uDeviceID, LPAUXCAPSA pac, UINT cbac);
WINMMAPI MMRESULT WINAPI auxGetDevCapsW(UINT uDeviceID, LPAUXCAPSW pac, UINT cbac);
#ifdef UNICODE
#define auxGetDevCaps  auxGetDevCapsW
#else
#define auxGetDevCaps  auxGetDevCapsA
#endif // !UNICODE

#else
MMRESULT WINAPI auxGetDevCaps(UINT uDeviceID, LPAUXCAPS pac, UINT cbac);
#endif
WINMMAPI MMRESULT WINAPI auxSetVolume(UINT uDeviceID, DWORD dwVolume);
WINMMAPI MMRESULT WINAPI auxGetVolume(UINT uDeviceID, LPDWORD pdwVolume);

#if (WINVER >= 0x030a)
#ifdef _WIN32
WINMMAPI MMRESULT WINAPI auxOutMessage(UINT uDeviceID, UINT uMsg, DWORD dw1, DWORD dw2);
#else
DWORD WINAPI auxOutMessage(UINT uDeviceID, UINT uMsg, DWORD dw1, DWORD dw2);
#endif
#endif /* ifdef WINVER >= 0x030a */

#endif /* ifndef _WIN32_VXD */

#endif  /* ifndef MMNOAUX */

#ifndef MMNOMIXER
#ifndef _WIN32_VXD
/****************************************************************************

			    Mixer Support

****************************************************************************/

DECLARE_HANDLE(HMIXEROBJ);
typedef HMIXEROBJ FAR *LPHMIXEROBJ;

DECLARE_HANDLE(HMIXER);
typedef HMIXER     FAR *LPHMIXER;

#define MIXER_SHORT_NAME_CHARS   16
#define MIXER_LONG_NAME_CHARS    64

/* */
/*  MMRESULT error return values specific to the mixer API */
/* */
/* */
#define MIXERR_INVALLINE            (MIXERR_BASE + 0)
#define MIXERR_INVALCONTROL         (MIXERR_BASE + 1)
#define MIXERR_INVALVALUE           (MIXERR_BASE + 2)
#define MIXERR_LASTERROR            (MIXERR_BASE + 2)

#define MIXER_OBJECTF_HANDLE    0x80000000L
#define MIXER_OBJECTF_MIXER     0x00000000L
#define MIXER_OBJECTF_HMIXER    (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIXER)
#define MIXER_OBJECTF_WAVEOUT   0x10000000L
#define MIXER_OBJECTF_HWAVEOUT  (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_WAVEOUT)
#define MIXER_OBJECTF_WAVEIN    0x20000000L
#define MIXER_OBJECTF_HWAVEIN   (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_WAVEIN)
#define MIXER_OBJECTF_MIDIOUT   0x30000000L
#define MIXER_OBJECTF_HMIDIOUT  (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIDIOUT)
#define MIXER_OBJECTF_MIDIIN    0x40000000L
#define MIXER_OBJECTF_HMIDIIN   (MIXER_OBJECTF_HANDLE|MIXER_OBJECTF_MIDIIN)
#define MIXER_OBJECTF_AUX       0x50000000L

WINMMAPI UINT WINAPI mixerGetNumDevs(void);

#ifdef _WIN32

typedef struct tagMIXERCAPSA {
    WORD            wMid;                   /* manufacturer id */
    WORD            wPid;                   /* product id */
    MMVERSION       vDriverVersion;         /* version of the driver */
    CHAR            szPname[MAXPNAMELEN];   /* product name */
    DWORD           fdwSupport;             /* misc. support bits */
    DWORD           cDestinations;          /* count of destinations */
} MIXERCAPSA, *PMIXERCAPSA, *LPMIXERCAPSA;
typedef struct tagMIXERCAPSW {
    WORD            wMid;                   /* manufacturer id */
    WORD            wPid;                   /* product id */
    MMVERSION       vDriverVersion;         /* version of the driver */
    WCHAR           szPname[MAXPNAMELEN];   /* product name */
    DWORD           fdwSupport;             /* misc. support bits */
    DWORD           cDestinations;          /* count of destinations */
} MIXERCAPSW, *PMIXERCAPSW, *LPMIXERCAPSW;
#ifdef UNICODE
typedef MIXERCAPSW MIXERCAPS;
typedef PMIXERCAPSW PMIXERCAPS;
typedef LPMIXERCAPSW LPMIXERCAPS;
#else
typedef MIXERCAPSA MIXERCAPS;
typedef PMIXERCAPSA PMIXERCAPS;
typedef LPMIXERCAPSA LPMIXERCAPS;
#endif // UNICODE

#else
typedef struct tMIXERCAPS {
    WORD            wMid;                   /* manufacturer id */
    WORD            wPid;                   /* product id */
    VERSION         vDriverVersion;         /* version of the driver */
    char            szPname[MAXPNAMELEN];   /* product name */
    DWORD           fdwSupport;             /* misc. support bits */
    DWORD           cDestinations;          /* count of destinations */
} MIXERCAPS, *PMIXERCAPS, FAR *LPMIXERCAPS;
#endif

#ifdef _WIN32

WINMMAPI MMRESULT WINAPI mixerGetDevCapsA(UINT uMxId, LPMIXERCAPSA pmxcaps, UINT cbmxcaps);
WINMMAPI MMRESULT WINAPI mixerGetDevCapsW(UINT uMxId, LPMIXERCAPSW pmxcaps, UINT cbmxcaps);
#ifdef UNICODE
#define mixerGetDevCaps  mixerGetDevCapsW
#else
#define mixerGetDevCaps  mixerGetDevCapsA
#endif // !UNICODE

#else
MMRESULT WINAPI mixerGetDevCaps(UINT uMxId, LPMIXERCAPS pmxcaps, UINT cbmxcaps);
#endif

WINMMAPI MMRESULT WINAPI mixerOpen(LPHMIXER phmx, UINT uMxId, DWORD dwCallback, DWORD dwInstance, DWORD fdwOpen);

WINMMAPI MMRESULT WINAPI mixerClose(HMIXER hmx);

WINMMAPI DWORD WINAPI mixerMessage(HMIXER hmx, UINT uMsg, DWORD dwParam1, DWORD dwParam2);

#ifdef _WIN32

typedef struct tagMIXERLINEA {
    DWORD       cbStruct;               /* size of MIXERLINE structure */
    DWORD       dwDestination;          /* zero based destination index */
    DWORD       dwSource;               /* zero based source index (if source) */
    DWORD       dwLineID;               /* unique line id for mixer device */
    DWORD       fdwLine;                /* state/information about line */
    DWORD       dwUser;                 /* driver specific information */
    DWORD       dwComponentType;        /* component type line connects to */
    DWORD       cChannels;              /* number of channels line supports */
    DWORD       cConnections;           /* number of connections [possible] */
    DWORD       cControls;              /* number of controls at this line */
    CHAR        szShortName[MIXER_SHORT_NAME_CHARS];
    CHAR        szName[MIXER_LONG_NAME_CHARS];
    struct {
	DWORD   dwType;                 /* MIXERLINE_TARGETTYPE_xxxx */
	DWORD   dwDeviceID;             /* target device ID of device type */
	WORD    wMid;                   /* of target device */
	WORD    wPid;                   /*      " */
	MMVERSION vDriverVersion;       /*      " */
	CHAR    szPname[MAXPNAMELEN];   /*      " */
    } Target;
} MIXERLINEA, *PMIXERLINEA, *LPMIXERLINEA;
typedef struct tagMIXERLINEW {
    DWORD       cbStruct;               /* size of MIXERLINE structure */
    DWORD       dwDestination;          /* zero based destination index */
    DWORD       dwSource;               /* zero based source index (if source) */
    DWORD       dwLineID;               /* unique line id for mixer device */
    DWORD       fdwLine;                /* state/information about line */
    DWORD       dwUser;                 /* driver specific information */
    DWORD       dwComponentType;        /* component type line connects to */
    DWORD       cChannels;              /* number of channels line supports */
    DWORD       cConnections;           /* number of connections [possible] */
    DWORD       cControls;              /* number of controls at this line */
    WCHAR       szShortName[MIXER_SHORT_NAME_CHARS];
    WCHAR       szName[MIXER_LONG_NAME_CHARS];
    struct {
	DWORD   dwType;                 /* MIXERLINE_TARGETTYPE_xxxx */
	DWORD   dwDeviceID;             /* target device ID of device type */
	WORD    wMid;                   /* of target device */
	WORD    wPid;                   /*      " */
	MMVERSION vDriverVersion;       /*      " */
	WCHAR   szPname[MAXPNAMELEN];   /*      " */
    } Target;
} MIXERLINEW, *PMIXERLINEW, *LPMIXERLINEW;
#ifdef UNICODE
typedef MIXERLINEW MIXERLINE;
typedef PMIXERLINEW PMIXERLINE;
typedef LPMIXERLINEW LPMIXERLINE;
#else
typedef MIXERLINEA MIXERLINE;
typedef PMIXERLINEA PMIXERLINE;
typedef LPMIXERLINEA LPMIXERLINE;
#endif // UNICODE

#else
typedef struct tMIXERLINE {
    DWORD       cbStruct;               /* size of MIXERLINE structure */
    DWORD       dwDestination;          /* zero based destination index */
    DWORD       dwSource;               /* zero based source index (if source) */
    DWORD       dwLineID;               /* unique line id for mixer device */
    DWORD       fdwLine;                /* state/information about line */
    DWORD       dwUser;                 /* driver specific information */
    DWORD       dwComponentType;        /* component type line connects to */
    DWORD       cChannels;              /* number of channels line supports */
    DWORD       cConnections;           /* number of connections [possible] */
    DWORD       cControls;              /* number of controls at this line */
    char        szShortName[MIXER_SHORT_NAME_CHARS];
    char        szName[MIXER_LONG_NAME_CHARS];
    struct {
	DWORD   dwType;                 /* MIXERLINE_TARGETTYPE_xxxx */
	DWORD   dwDeviceID;             /* target device ID of device type */
	WORD    wMid;                   /* of target device */
	WORD    wPid;                   /*      " */
	VERSION vDriverVersion;         /*      " */
	char    szPname[MAXPNAMELEN];   /*      " */
    } Target;
} MIXERLINE, *PMIXERLINE, FAR *LPMIXERLINE;
#endif

/* */
/*  MIXERLINE.fdwLine */
/* */
/* */
#define MIXERLINE_LINEF_ACTIVE              0x00000001L
#define MIXERLINE_LINEF_DISCONNECTED        0x00008000L
#define MIXERLINE_LINEF_SOURCE              0x80000000L

/* */
/*  MIXERLINE.dwComponentType */
/* */
/*  component types for destinations and sources */
/* */
/* */
#define MIXERLINE_COMPONENTTYPE_DST_FIRST       0x00000000L
#define MIXERLINE_COMPONENTTYPE_DST_UNDEFINED   (MIXERLINE_COMPONENTTYPE_DST_FIRST + 0)
#define MIXERLINE_COMPONENTTYPE_DST_DIGITAL     (MIXERLINE_COMPONENTTYPE_DST_FIRST + 1)
#define MIXERLINE_COMPONENTTYPE_DST_LINE        (MIXERLINE_COMPONENTTYPE_DST_FIRST + 2)
#define MIXERLINE_COMPONENTTYPE_DST_MONITOR     (MIXERLINE_COMPONENTTYPE_DST_FIRST + 3)
#define MIXERLINE_COMPONENTTYPE_DST_SPEAKERS    (MIXERLINE_COMPONENTTYPE_DST_FIRST + 4)
#define MIXERLINE_COMPONENTTYPE_DST_HEADPHONES  (MIXERLINE_COMPONENTTYPE_DST_FIRST + 5)
#define MIXERLINE_COMPONENTTYPE_DST_TELEPHONE   (MIXERLINE_COMPONENTTYPE_DST_FIRST + 6)
#define MIXERLINE_COMPONENTTYPE_DST_WAVEIN      (MIXERLINE_COMPONENTTYPE_DST_FIRST + 7)
#define MIXERLINE_COMPONENTTYPE_DST_VOICEIN     (MIXERLINE_COMPONENTTYPE_DST_FIRST + 8)
#define MIXERLINE_COMPONENTTYPE_DST_LAST        (MIXERLINE_COMPONENTTYPE_DST_FIRST + 8)

#define MIXERLINE_COMPONENTTYPE_SRC_FIRST       0x00001000L
#define MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED   (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 0)
#define MIXERLINE_COMPONENTTYPE_SRC_DIGITAL     (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 1)
#define MIXERLINE_COMPONENTTYPE_SRC_LINE        (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 2)
#define MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE  (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 3)
#define MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 4)
#define MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 5)
#define MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE   (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 6)
#define MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER   (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 7)
#define MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT     (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 8)
#define MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY   (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 9)
#define MIXERLINE_COMPONENTTYPE_SRC_ANALOG      (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 10)
#define MIXERLINE_COMPONENTTYPE_SRC_LAST        (MIXERLINE_COMPONENTTYPE_SRC_FIRST + 10)

/* */
/*  MIXERLINE.Target.dwType */
/* */
/* */
#define MIXERLINE_TARGETTYPE_UNDEFINED      0
#define MIXERLINE_TARGETTYPE_WAVEOUT        1
#define MIXERLINE_TARGETTYPE_WAVEIN         2
#define MIXERLINE_TARGETTYPE_MIDIOUT        3
#define MIXERLINE_TARGETTYPE_MIDIIN         4
#define MIXERLINE_TARGETTYPE_AUX            5

#ifdef _WIN32

WINMMAPI MMRESULT WINAPI mixerGetLineInfoA(HMIXEROBJ hmxobj, LPMIXERLINEA pmxl, DWORD fdwInfo);
WINMMAPI MMRESULT WINAPI mixerGetLineInfoW(HMIXEROBJ hmxobj, LPMIXERLINEW pmxl, DWORD fdwInfo);
#ifdef UNICODE
#define mixerGetLineInfo  mixerGetLineInfoW
#else
#define mixerGetLineInfo  mixerGetLineInfoA
#endif // !UNICODE

#else
MMRESULT WINAPI mixerGetLineInfo(HMIXEROBJ hmxobj, LPMIXERLINE pmxl, DWORD fdwInfo);
#endif

#define MIXER_GETLINEINFOF_DESTINATION      0x00000000L
#define MIXER_GETLINEINFOF_SOURCE           0x00000001L
#define MIXER_GETLINEINFOF_LINEID           0x00000002L
#define MIXER_GETLINEINFOF_COMPONENTTYPE    0x00000003L
#define MIXER_GETLINEINFOF_TARGETTYPE       0x00000004L

#define MIXER_GETLINEINFOF_QUERYMASK        0x0000000FL

WINMMAPI MMRESULT WINAPI mixerGetID(HMIXEROBJ hmxobj, UINT FAR *puMxId, DWORD fdwId);

/* */
/*  MIXERCONTROL */
/* */
/* */
#ifdef _WIN32

typedef struct tagMIXERCONTROLA {
    DWORD           cbStruct;           /* size in bytes of MIXERCONTROL */
    DWORD           dwControlID;        /* unique control id for mixer device */
    DWORD           dwControlType;      /* MIXERCONTROL_CONTROLTYPE_xxx */
    DWORD           fdwControl;         /* MIXERCONTROL_CONTROLF_xxx */
    DWORD           cMultipleItems;     /* if MIXERCONTROL_CONTROLF_MULTIPLE set */
    CHAR            szShortName[MIXER_SHORT_NAME_CHARS];
    CHAR            szName[MIXER_LONG_NAME_CHARS];
    union {
	struct {
	    LONG    lMinimum;           /* signed minimum for this control */
	    LONG    lMaximum;           /* signed maximum for this control */
	};
	struct {
	    DWORD   dwMinimum;          /* unsigned minimum for this control */
	    DWORD   dwMaximum;          /* unsigned maximum for this control */
	};
	DWORD       dwReserved[6];
    } Bounds;
    union {
	DWORD       cSteps;             /* # of steps between min & max */
	DWORD       cbCustomData;       /* size in bytes of custom data */
	DWORD       dwReserved[6];      /* !!! needed? we have cbStruct.... */
    } Metrics;
} MIXERCONTROLA, *PMIXERCONTROLA, *LPMIXERCONTROLA;
typedef struct tagMIXERCONTROLW {
    DWORD           cbStruct;           /* size in bytes of MIXERCONTROL */
    DWORD           dwControlID;        /* unique control id for mixer device */
    DWORD           dwControlType;      /* MIXERCONTROL_CONTROLTYPE_xxx */
    DWORD           fdwControl;         /* MIXERCONTROL_CONTROLF_xxx */
    DWORD           cMultipleItems;     /* if MIXERCONTROL_CONTROLF_MULTIPLE set */
    WCHAR           szShortName[MIXER_SHORT_NAME_CHARS];
    WCHAR           szName[MIXER_LONG_NAME_CHARS];
    union {
	struct {
	    LONG    lMinimum;           /* signed minimum for this control */
	    LONG    lMaximum;           /* signed maximum for this control */
	};
	struct {
	    DWORD   dwMinimum;          /* unsigned minimum for this control */
	    DWORD   dwMaximum;          /* unsigned maximum for this control */
	};
	DWORD       dwReserved[6];
    } Bounds;
    union {
	DWORD       cSteps;             /* # of steps between min & max */
	DWORD       cbCustomData;       /* size in bytes of custom data */
	DWORD       dwReserved[6];      /* !!! needed? we have cbStruct.... */
    } Metrics;
} MIXERCONTROLW, *PMIXERCONTROLW, *LPMIXERCONTROLW;
#ifdef UNICODE
typedef MIXERCONTROLW MIXERCONTROL;
typedef PMIXERCONTROLW PMIXERCONTROL;
typedef LPMIXERCONTROLW LPMIXERCONTROL;
#else
typedef MIXERCONTROLA MIXERCONTROL;
typedef PMIXERCONTROLA PMIXERCONTROL;
typedef LPMIXERCONTROLA LPMIXERCONTROL;
#endif // UNICODE

#else
typedef struct tMIXERCONTROL {
    DWORD           cbStruct;           /* size in bytes of MIXERCONTROL */
    DWORD           dwControlID;        /* unique control id for mixer device */
    DWORD           dwControlType;      /* MIXERCONTROL_CONTROLTYPE_xxx */
    DWORD           fdwControl;         /* MIXERCONTROL_CONTROLF_xxx */
    DWORD           cMultipleItems;     /* if MIXERCONTROL_CONTROLF_MULTIPLE set */
    char            szShortName[MIXER_SHORT_NAME_CHARS];
    char            szName[MIXER_LONG_NAME_CHARS];
    union {
	struct {
	    LONG    lMinimum;           /* signed minimum for this control */
	    LONG    lMaximum;           /* signed maximum for this control */
	};
	struct {
	    DWORD   dwMinimum;          /* unsigned minimum for this control */
	    DWORD   dwMaximum;          /* unsigned maximum for this control */
	};
	DWORD       dwReserved[6];
    } Bounds;
    union {
	DWORD       cSteps;             /* # of steps between min & max */
	DWORD       cbCustomData;       /* size in bytes of custom data */
	DWORD       dwReserved[6];      /* !!! needed? we have cbStruct.... */
    } Metrics;
} MIXERCONTROL, *PMIXERCONTROL, FAR *LPMIXERCONTROL;
#endif

/* */
/*  MIXERCONTROL.fdwControl */
/* */
/* */
#define MIXERCONTROL_CONTROLF_UNIFORM   0x00000001L
#define MIXERCONTROL_CONTROLF_MULTIPLE  0x00000002L
#define MIXERCONTROL_CONTROLF_DISABLED  0x80000000L

/* */
/*  MIXERCONTROL_CONTROLTYPE_xxx building block defines */
/* */
/* */
#define MIXERCONTROL_CT_CLASS_MASK          0xF0000000L
#define MIXERCONTROL_CT_CLASS_CUSTOM        0x00000000L
#define MIXERCONTROL_CT_CLASS_METER         0x10000000L
#define MIXERCONTROL_CT_CLASS_SWITCH        0x20000000L
#define MIXERCONTROL_CT_CLASS_NUMBER        0x30000000L
#define MIXERCONTROL_CT_CLASS_SLIDER        0x40000000L
#define MIXERCONTROL_CT_CLASS_FADER         0x50000000L
#define MIXERCONTROL_CT_CLASS_TIME          0x60000000L
#define MIXERCONTROL_CT_CLASS_LIST          0x70000000L

#define MIXERCONTROL_CT_SUBCLASS_MASK       0x0F000000L

#define MIXERCONTROL_CT_SC_SWITCH_BOOLEAN   0x00000000L
#define MIXERCONTROL_CT_SC_SWITCH_BUTTON    0x01000000L

#define MIXERCONTROL_CT_SC_METER_POLLED     0x00000000L

#define MIXERCONTROL_CT_SC_TIME_MICROSECS   0x00000000L
#define MIXERCONTROL_CT_SC_TIME_MILLISECS   0x01000000L

#define MIXERCONTROL_CT_SC_LIST_SINGLE      0x00000000L
#define MIXERCONTROL_CT_SC_LIST_MULTIPLE    0x01000000L

#define MIXERCONTROL_CT_UNITS_MASK          0x00FF0000L
#define MIXERCONTROL_CT_UNITS_CUSTOM        0x00000000L
#define MIXERCONTROL_CT_UNITS_BOOLEAN       0x00010000L
#define MIXERCONTROL_CT_UNITS_SIGNED        0x00020000L
#define MIXERCONTROL_CT_UNITS_UNSIGNED      0x00030000L
#define MIXERCONTROL_CT_UNITS_DECIBELS      0x00040000L /* in 10ths */
#define MIXERCONTROL_CT_UNITS_PERCENT       0x00050000L /* in 10ths */

/* */
/*  Commonly used control types for specifying MIXERCONTROL.dwControlType */
/* */

#define MIXERCONTROL_CONTROLTYPE_CUSTOM         (MIXERCONTROL_CT_CLASS_CUSTOM | MIXERCONTROL_CT_UNITS_CUSTOM)
#define MIXERCONTROL_CONTROLTYPE_BOOLEANMETER   (MIXERCONTROL_CT_CLASS_METER | MIXERCONTROL_CT_SC_METER_POLLED | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_SIGNEDMETER    (MIXERCONTROL_CT_CLASS_METER | MIXERCONTROL_CT_SC_METER_POLLED | MIXERCONTROL_CT_UNITS_SIGNED)
#define MIXERCONTROL_CONTROLTYPE_PEAKMETER      (MIXERCONTROL_CONTROLTYPE_SIGNEDMETER + 1)
#define MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER  (MIXERCONTROL_CT_CLASS_METER | MIXERCONTROL_CT_SC_METER_POLLED | MIXERCONTROL_CT_UNITS_UNSIGNED)
#define MIXERCONTROL_CONTROLTYPE_BOOLEAN        (MIXERCONTROL_CT_CLASS_SWITCH | MIXERCONTROL_CT_SC_SWITCH_BOOLEAN | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_ONOFF          (MIXERCONTROL_CONTROLTYPE_BOOLEAN + 1)
#define MIXERCONTROL_CONTROLTYPE_MUTE           (MIXERCONTROL_CONTROLTYPE_BOOLEAN + 2)
#define MIXERCONTROL_CONTROLTYPE_MONO           (MIXERCONTROL_CONTROLTYPE_BOOLEAN + 3)
#define MIXERCONTROL_CONTROLTYPE_LOUDNESS       (MIXERCONTROL_CONTROLTYPE_BOOLEAN + 4)
#define MIXERCONTROL_CONTROLTYPE_STEREOENH      (MIXERCONTROL_CONTROLTYPE_BOOLEAN + 5)
#define MIXERCONTROL_CONTROLTYPE_BUTTON         (MIXERCONTROL_CT_CLASS_SWITCH | MIXERCONTROL_CT_SC_SWITCH_BUTTON | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_DECIBELS       (MIXERCONTROL_CT_CLASS_NUMBER | MIXERCONTROL_CT_UNITS_DECIBELS)
#define MIXERCONTROL_CONTROLTYPE_SIGNED         (MIXERCONTROL_CT_CLASS_NUMBER | MIXERCONTROL_CT_UNITS_SIGNED)
#define MIXERCONTROL_CONTROLTYPE_UNSIGNED       (MIXERCONTROL_CT_CLASS_NUMBER | MIXERCONTROL_CT_UNITS_UNSIGNED)
#define MIXERCONTROL_CONTROLTYPE_PERCENT        (MIXERCONTROL_CT_CLASS_NUMBER | MIXERCONTROL_CT_UNITS_PERCENT)
#define MIXERCONTROL_CONTROLTYPE_SLIDER         (MIXERCONTROL_CT_CLASS_SLIDER | MIXERCONTROL_CT_UNITS_SIGNED)
#define MIXERCONTROL_CONTROLTYPE_PAN            (MIXERCONTROL_CONTROLTYPE_SLIDER + 1)
#define MIXERCONTROL_CONTROLTYPE_QSOUNDPAN      (MIXERCONTROL_CONTROLTYPE_SLIDER + 2)
#define MIXERCONTROL_CONTROLTYPE_FADER          (MIXERCONTROL_CT_CLASS_FADER | MIXERCONTROL_CT_UNITS_UNSIGNED)
#define MIXERCONTROL_CONTROLTYPE_VOLUME         (MIXERCONTROL_CONTROLTYPE_FADER + 1)
#define MIXERCONTROL_CONTROLTYPE_BASS           (MIXERCONTROL_CONTROLTYPE_FADER + 2)
#define MIXERCONTROL_CONTROLTYPE_TREBLE         (MIXERCONTROL_CONTROLTYPE_FADER + 3)
#define MIXERCONTROL_CONTROLTYPE_EQUALIZER      (MIXERCONTROL_CONTROLTYPE_FADER + 4)
#define MIXERCONTROL_CONTROLTYPE_SINGLESELECT   (MIXERCONTROL_CT_CLASS_LIST | MIXERCONTROL_CT_SC_LIST_SINGLE | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_MUX            (MIXERCONTROL_CONTROLTYPE_SINGLESELECT + 1)
#define MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT (MIXERCONTROL_CT_CLASS_LIST | MIXERCONTROL_CT_SC_LIST_MULTIPLE | MIXERCONTROL_CT_UNITS_BOOLEAN)
#define MIXERCONTROL_CONTROLTYPE_MIXER          (MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT + 1)
#define MIXERCONTROL_CONTROLTYPE_MICROTIME      (MIXERCONTROL_CT_CLASS_TIME | MIXERCONTROL_CT_SC_TIME_MICROSECS | MIXERCONTROL_CT_UNITS_UNSIGNED)
#define MIXERCONTROL_CONTROLTYPE_MILLITIME      (MIXERCONTROL_CT_CLASS_TIME | MIXERCONTROL_CT_SC_TIME_MILLISECS | MIXERCONTROL_CT_UNITS_UNSIGNED)

/* */
/*  MIXERLINECONTROLS */
/* */
#ifdef _WIN32

typedef struct tagMIXERLINECONTROLSA {
    DWORD           cbStruct;       /* size in bytes of MIXERLINECONTROLS */
    DWORD           dwLineID;       /* line id (from MIXERLINE.dwLineID) */
    union {
	DWORD       dwControlID;    /* MIXER_GETLINECONTROLSF_ONEBYID */
	DWORD       dwControlType;  /* MIXER_GETLINECONTROLSF_ONEBYTYPE */
    };
    DWORD           cControls;      /* count of controls pmxctrl points to */
    DWORD           cbmxctrl;       /* size in bytes of _one_ MIXERCONTROL */
    LPMIXERCONTROLA pamxctrl;       /* pointer to first MIXERCONTROL array */
} MIXERLINECONTROLSA, *PMIXERLINECONTROLSA, *LPMIXERLINECONTROLSA;
typedef struct tagMIXERLINECONTROLSW {
    DWORD           cbStruct;       /* size in bytes of MIXERLINECONTROLS */
    DWORD           dwLineID;       /* line id (from MIXERLINE.dwLineID) */
    union {
	DWORD       dwControlID;    /* MIXER_GETLINECONTROLSF_ONEBYID */
	DWORD       dwControlType;  /* MIXER_GETLINECONTROLSF_ONEBYTYPE */
    };
    DWORD           cControls;      /* count of controls pmxctrl points to */
    DWORD           cbmxctrl;       /* size in bytes of _one_ MIXERCONTROL */
    LPMIXERCONTROLW pamxctrl;       /* pointer to first MIXERCONTROL array */
} MIXERLINECONTROLSW, *PMIXERLINECONTROLSW, *LPMIXERLINECONTROLSW;
#ifdef UNICODE
typedef MIXERLINECONTROLSW MIXERLINECONTROLS;
typedef PMIXERLINECONTROLSW PMIXERLINECONTROLS;
typedef LPMIXERLINECONTROLSW LPMIXERLINECONTROLS;
#else
typedef MIXERLINECONTROLSA MIXERLINECONTROLS;
typedef PMIXERLINECONTROLSA PMIXERLINECONTROLS;
typedef LPMIXERLINECONTROLSA LPMIXERLINECONTROLS;
#endif // UNICODE

#else
typedef struct tMIXERLINECONTROLS {
    DWORD           cbStruct;       /* size in bytes of MIXERLINECONTROLS */
    DWORD           dwLineID;       /* line id (from MIXERLINE.dwLineID) */
    union {
	DWORD       dwControlID;    /* MIXER_GETLINECONTROLSF_ONEBYID */
	DWORD       dwControlType;  /* MIXER_GETLINECONTROLSF_ONEBYTYPE */
    };
    DWORD           cControls;      /* count of controls pmxctrl points to */
    DWORD           cbmxctrl;       /* size in bytes of _one_ MIXERCONTROL */
    LPMIXERCONTROL  pamxctrl;       /* pointer to first MIXERCONTROL array */
} MIXERLINECONTROLS, *PMIXERLINECONTROLS, FAR *LPMIXERLINECONTROLS;
#endif

/* */
/* */
/* */
#ifdef _WIN32

WINMMAPI MMRESULT WINAPI mixerGetLineControlsA(HMIXEROBJ hmxobj, LPMIXERLINECONTROLSA pmxlc, DWORD fdwControls);
WINMMAPI MMRESULT WINAPI mixerGetLineControlsW(HMIXEROBJ hmxobj, LPMIXERLINECONTROLSW pmxlc, DWORD fdwControls);
#ifdef UNICODE
#define mixerGetLineControls  mixerGetLineControlsW
#else
#define mixerGetLineControls  mixerGetLineControlsA
#endif // !UNICODE

#else
MMRESULT WINAPI mixerGetLineControls(HMIXEROBJ hmxobj, LPMIXERLINECONTROLS pmxlc, DWORD fdwControls);
#endif

#define MIXER_GETLINECONTROLSF_ALL          0x00000000L
#define MIXER_GETLINECONTROLSF_ONEBYID      0x00000001L
#define MIXER_GETLINECONTROLSF_ONEBYTYPE    0x00000002L

#define MIXER_GETLINECONTROLSF_QUERYMASK    0x0000000FL

typedef struct tMIXERCONTROLDETAILS {
    DWORD           cbStruct;       /* size in bytes of MIXERCONTROLDETAILS */
    DWORD           dwControlID;    /* control id to get/set details on */
    DWORD           cChannels;      /* number of channels in paDetails array */
    union {
	HWND        hwndOwner;      /* for MIXER_SETCONTROLDETAILSF_CUSTOM */
	DWORD       cMultipleItems; /* if _MULTIPLE, the number of items per channel */
    };
    DWORD           cbDetails;      /* size of _one_ details_XX struct */
    LPVOID          paDetails;      /* pointer to array of details_XX structs */
} MIXERCONTROLDETAILS, *PMIXERCONTROLDETAILS, FAR *LPMIXERCONTROLDETAILS;

/* */
/*  MIXER_GETCONTROLDETAILSF_LISTTEXT */
/* */
/* */
#ifdef _WIN32

typedef struct tagMIXERCONTROLDETAILS_LISTTEXTA {
    DWORD           dwParam1;
    DWORD           dwParam2;
    CHAR            szName[MIXER_LONG_NAME_CHARS];
} MIXERCONTROLDETAILS_LISTTEXTA, *PMIXERCONTROLDETAILS_LISTTEXTA, *LPMIXERCONTROLDETAILS_LISTTEXTA;
typedef struct tagMIXERCONTROLDETAILS_LISTTEXTW {
    DWORD           dwParam1;
    DWORD           dwParam2;
    WCHAR           szName[MIXER_LONG_NAME_CHARS];
} MIXERCONTROLDETAILS_LISTTEXTW, *PMIXERCONTROLDETAILS_LISTTEXTW, *LPMIXERCONTROLDETAILS_LISTTEXTW;
#ifdef UNICODE
typedef MIXERCONTROLDETAILS_LISTTEXTW MIXERCONTROLDETAILS_LISTTEXT;
typedef PMIXERCONTROLDETAILS_LISTTEXTW PMIXERCONTROLDETAILS_LISTTEXT;
typedef LPMIXERCONTROLDETAILS_LISTTEXTW LPMIXERCONTROLDETAILS_LISTTEXT;
#else
typedef MIXERCONTROLDETAILS_LISTTEXTA MIXERCONTROLDETAILS_LISTTEXT;
typedef PMIXERCONTROLDETAILS_LISTTEXTA PMIXERCONTROLDETAILS_LISTTEXT;
typedef LPMIXERCONTROLDETAILS_LISTTEXTA LPMIXERCONTROLDETAILS_LISTTEXT;
#endif // UNICODE

#else
typedef struct tMIXERCONTROLDETAILS_LISTTEXT {
    DWORD           dwParam1;
    DWORD           dwParam2;
    char            szName[MIXER_LONG_NAME_CHARS];
} MIXERCONTROLDETAILS_LISTTEXT, *PMIXERCONTROLDETAILS_LISTTEXT, FAR *LPMIXERCONTROLDETAILS_LISTTEXT;
#endif

/* */
/*  MIXER_GETCONTROLDETAILSF_VALUE */
/* */
/* */
typedef struct tMIXERCONTROLDETAILS_BOOLEAN {
    LONG            fValue;
}       MIXERCONTROLDETAILS_BOOLEAN,
      *PMIXERCONTROLDETAILS_BOOLEAN,
 FAR *LPMIXERCONTROLDETAILS_BOOLEAN;

typedef struct tMIXERCONTROLDETAILS_SIGNED {
    LONG            lValue;
}       MIXERCONTROLDETAILS_SIGNED,
      *PMIXERCONTROLDETAILS_SIGNED,
 FAR *LPMIXERCONTROLDETAILS_SIGNED;

typedef struct tMIXERCONTROLDETAILS_UNSIGNED {
    DWORD           dwValue;
}       MIXERCONTROLDETAILS_UNSIGNED,
      *PMIXERCONTROLDETAILS_UNSIGNED,
 FAR *LPMIXERCONTROLDETAILS_UNSIGNED;

#ifdef _WIN32

WINMMAPI MMRESULT WINAPI mixerGetControlDetailsA(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);
WINMMAPI MMRESULT WINAPI mixerGetControlDetailsW(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);
#ifdef UNICODE
#define mixerGetControlDetails  mixerGetControlDetailsW
#else
#define mixerGetControlDetails  mixerGetControlDetailsA
#endif // !UNICODE

#else
MMRESULT WINAPI mixerGetControlDetails(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);
#endif

#define MIXER_GETCONTROLDETAILSF_VALUE      0x00000000L
#define MIXER_GETCONTROLDETAILSF_LISTTEXT   0x00000001L

#define MIXER_GETCONTROLDETAILSF_QUERYMASK  0x0000000FL

WINMMAPI MMRESULT WINAPI mixerSetControlDetails(HMIXEROBJ hmxobj, LPMIXERCONTROLDETAILS pmxcd, DWORD fdwDetails);

#define MIXER_SETCONTROLDETAILSF_VALUE      0x00000000L
#define MIXER_SETCONTROLDETAILSF_CUSTOM     0x00000001L

#define MIXER_SETCONTROLDETAILSF_QUERYMASK  0x0000000FL

#endif /* ifndef _WIN32_VXD */
#endif /* ifndef MMNOMIXER */

#ifndef MMNOTIMER
#ifndef _WIN32_VXD
/****************************************************************************

			    Timer support

****************************************************************************/

/* timer error return values */
#define TIMERR_NOERROR        (0)                  /* no error */
#define TIMERR_NOCANDO        (TIMERR_BASE+1)      /* request not completed */
#define TIMERR_STRUCT         (TIMERR_BASE+33)     /* time struct size */

/* timer data types */
typedef void (CALLBACK TIMECALLBACK)(UINT uTimerID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2);

typedef TIMECALLBACK FAR *LPTIMECALLBACK;

/* flags for fuEvent parameter of timeSetEvent() function */
#define TIME_ONESHOT    0x0000   /* program timer for single event */
#define TIME_PERIODIC   0x0001   /* program for continuous periodic event */

#ifdef _WIN32
#define TIME_CALLBACK_FUNCTION      0x0000  /* callback is function */
#define TIME_CALLBACK_EVENT_SET     0x0010  /* callback is event - use SetEvent */
#define TIME_CALLBACK_EVENT_PULSE   0x0020  /* callback is event - use PulseEvent */
#endif

/* timer device capabilities data structure */
typedef struct timecaps_tag {
    UINT    wPeriodMin;     /* minimum period supported  */
    UINT    wPeriodMax;     /* maximum period supported  */
} TIMECAPS, *PTIMECAPS, NEAR *NPTIMECAPS, FAR *LPTIMECAPS;

/* timer function prototypes */
WINMMAPI MMRESULT WINAPI timeGetSystemTime(LPMMTIME pmmt, UINT cbmmt);
WINMMAPI DWORD WINAPI timeGetTime(void);
WINMMAPI MMRESULT WINAPI timeSetEvent(UINT uDelay, UINT uResolution,
    LPTIMECALLBACK fptc, DWORD dwUser, UINT fuEvent);
WINMMAPI MMRESULT WINAPI timeKillEvent(UINT uTimerID);
WINMMAPI MMRESULT WINAPI timeGetDevCaps(LPTIMECAPS ptc, UINT cbtc);
WINMMAPI MMRESULT WINAPI timeBeginPeriod(UINT uPeriod);
WINMMAPI MMRESULT WINAPI timeEndPeriod(UINT uPeriod);

#endif  /* ifndef _WIN32_VXD */
#endif  /* ifndef MMNOTIMER */

#ifndef MMNOJOY
/****************************************************************************

			    Joystick support

****************************************************************************/

/* joystick error return values */
#define JOYERR_NOERROR        (0)                  /* no error */
#define JOYERR_PARMS          (JOYERR_BASE+5)      /* bad parameters */
#define JOYERR_NOCANDO        (JOYERR_BASE+6)      /* request not completed */
#define JOYERR_UNPLUGGED      (JOYERR_BASE+7)      /* joystick is unplugged */

/* constants used with JOYINFO and JOYINFOEX structures and MM_JOY* messages */
#define JOY_BUTTON1         0x0001
#define JOY_BUTTON2         0x0002
#define JOY_BUTTON3         0x0004
#define JOY_BUTTON4         0x0008
#define JOY_BUTTON1CHG      0x0100
#define JOY_BUTTON2CHG      0x0200
#define JOY_BUTTON3CHG      0x0400
#define JOY_BUTTON4CHG      0x0800

/* constants used with JOYINFOEX */
#define JOY_BUTTON5         0x00000010l
#define JOY_BUTTON6         0x00000020l
#define JOY_BUTTON7         0x00000040l
#define JOY_BUTTON8         0x00000080l
#define JOY_BUTTON9         0x00000100l
#define JOY_BUTTON10        0x00000200l
#define JOY_BUTTON11        0x00000400l
#define JOY_BUTTON12        0x00000800l
#define JOY_BUTTON13        0x00001000l
#define JOY_BUTTON14        0x00002000l
#define JOY_BUTTON15        0x00004000l
#define JOY_BUTTON16        0x00008000l
#define JOY_BUTTON17        0x00010000l
#define JOY_BUTTON18        0x00020000l
#define JOY_BUTTON19        0x00040000l
#define JOY_BUTTON20        0x00080000l
#define JOY_BUTTON21        0x00100000l
#define JOY_BUTTON22        0x00200000l
#define JOY_BUTTON23        0x00400000l
#define JOY_BUTTON24        0x00800000l
#define JOY_BUTTON25        0x01000000l
#define JOY_BUTTON26        0x02000000l
#define JOY_BUTTON27        0x04000000l
#define JOY_BUTTON28        0x08000000l
#define JOY_BUTTON29        0x10000000l
#define JOY_BUTTON30        0x20000000l
#define JOY_BUTTON31        0x40000000l
#define JOY_BUTTON32        0x80000000l

/* constants used with JOYINFOEX structure */
#define JOY_POVCENTERED		(WORD) -1
#define JOY_POVFORWARD		0
#define JOY_POVRIGHT		9000
#define JOY_POVBACKWARD		18000
#define JOY_POVLEFT		27000

#define JOY_RETURNX		0x00000001l
#define JOY_RETURNY		0x00000002l
#define JOY_RETURNZ		0x00000004l
#define JOY_RETURNR		0x00000008l
#define JOY_RETURNU		0x00000010l	/* axis 5 */
#define JOY_RETURNV		0x00000020l	/* axis 6 */
#define JOY_RETURNPOV		0x00000040l
#define JOY_RETURNBUTTONS	0x00000080l
#define JOY_RETURNRAWDATA	0x00000100l
#define JOY_RETURNPOVCTS	0x00000200l
#define JOY_RETURNCENTERED	0x00000400l
#define JOY_USEDEADZONE		0x00000800l
#define JOY_RETURNALL		(JOY_RETURNX | JOY_RETURNY | JOY_RETURNZ | \
				 JOY_RETURNR | JOY_RETURNU | JOY_RETURNV | \
				 JOY_RETURNPOV | JOY_RETURNBUTTONS)
#define JOY_CAL_READALWAYS	0x00010000l
#define JOY_CAL_READXYONLY	0x00020000l
#define JOY_CAL_READ3		0x00040000l
#define JOY_CAL_READ4		0x00080000l
#define JOY_CAL_READXONLY	0x00100000l
#define JOY_CAL_READYONLY	0x00200000l
#define JOY_CAL_READ5		0x00400000l
#define JOY_CAL_READ6		0x00800000l
#define JOY_CAL_READZONLY	0x01000000l
#define JOY_CAL_READRONLY	0x02000000l
#define JOY_CAL_READUONLY	0x04000000l
#define JOY_CAL_READVONLY	0x08000000l

/* joystick ID constants */
#define JOYSTICKID1         0
#define JOYSTICKID2         1

/* joystick driver capabilites */
#define JOYCAPS_HASZ		0x0001
#define JOYCAPS_HASR		0x0002
#define JOYCAPS_HASU		0x0004
#define JOYCAPS_HASV		0x0008
#define JOYCAPS_HASPOV		0x0010
#define JOYCAPS_POV4DIR		0x0020
#define JOYCAPS_POVCTS		0x0040

/* joystick device capabilities data structure */
#ifdef _WIN32

typedef struct tagJOYCAPSA {
    WORD    wMid;                /* manufacturer ID */
    WORD    wPid;                /* product ID */
    CHAR    szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    UINT    wXmin;               /* minimum x position value */
    UINT    wXmax;               /* maximum x position value */
    UINT    wYmin;               /* minimum y position value */
    UINT    wYmax;               /* maximum y position value */
    UINT    wZmin;               /* minimum z position value */
    UINT    wZmax;               /* maximum z position value */
    UINT    wNumButtons;         /* number of buttons */
    UINT    wPeriodMin;          /* minimum message period when captured */
    UINT    wPeriodMax;          /* maximum message period when captured */
#if (WINVER >= 0x0400)
    UINT    wRmin;               /* minimum r position value */
    UINT    wRmax;               /* maximum r position value */
    UINT    wUmin;               /* minimum u (5th axis) position value */
    UINT    wUmax;               /* maximum u (5th axis) position value */
    UINT    wVmin;               /* minimum v (6th axis) position value */
    UINT    wVmax;               /* maximum v (6th axis) position value */
    UINT    wCaps;	 	 /* joystick capabilites */
    UINT    wMaxAxes;	 	 /* maximum number of axes supported */
    UINT    wNumAxes;	 	 /* number of axes in use */
    UINT    wMaxButtons;	 /* maximum number of buttons supported */
    CHAR    szRegKey[MAXPNAMELEN];/* registry key */
    CHAR    szOEMVxD[MAX_JOYSTICKOEMVXDNAME]; /* OEM VxD in use */
#endif
} JOYCAPSA, *PJOYCAPSA, *NPJOYCAPSA, *LPJOYCAPSA;
typedef struct tagJOYCAPSW {
    WORD    wMid;                /* manufacturer ID */
    WORD    wPid;                /* product ID */
    WCHAR   szPname[MAXPNAMELEN];/* product name (NULL terminated string) */
    UINT    wXmin;               /* minimum x position value */
    UINT    wXmax;               /* maximum x position value */
    UINT    wYmin;               /* minimum y position value */
    UINT    wYmax;               /* maximum y position value */
    UINT    wZmin;               /* minimum z position value */
    UINT    wZmax;               /* maximum z position value */
    UINT    wNumButtons;         /* number of buttons */
    UINT    wPeriodMin;          /* minimum message period when captured */
    UINT    wPeriodMax;          /* maximum message period when captured */
#if (WINVER >= 0x0400)
    UINT    wRmin;               /* minimum r position value */
    UINT    wRmax;               /* maximum r position value */
    UINT    wUmin;               /* minimum u (5th axis) position value */
    UINT    wUmax;               /* maximum u (5th axis) position value */
    UINT    wVmin;               /* minimum v (6th axis) position value */
    UINT    wVmax;               /* maximum v (6th axis) position value */
    UINT    wCaps;	 	 /* joystick capabilites */
    UINT    wMaxAxes;	 	 /* maximum number of axes supported */
    UINT    wNumAxes;	 	 /* number of axes in use */
    UINT    wMaxButtons;	 /* maximum number of buttons supported */
    WCHAR   szRegKey[MAXPNAMELEN];/* registry key */
    WCHAR   szOEMVxD[MAX_JOYSTICKOEMVXDNAME]; /* OEM VxD in use */
#endif
} JOYCAPSW, *PJOYCAPSW, *NPJOYCAPSW, *LPJOYCAPSW;
#ifdef UNICODE
typedef JOYCAPSW JOYCAPS;
typedef PJOYCAPSW PJOYCAPS;
typedef NPJOYCAPSW NPJOYCAPS;
typedef LPJOYCAPSW LPJOYCAPS;
#else
typedef JOYCAPSA JOYCAPS;
typedef PJOYCAPSA PJOYCAPS;
typedef NPJOYCAPSA NPJOYCAPS;
typedef LPJOYCAPSA LPJOYCAPS;
#endif // UNICODE

#else
typedef struct joycaps_tag {
    WORD wMid;                  /* manufacturer ID */
    WORD wPid;                  /* product ID */
    char szPname[MAXPNAMELEN];  /* product name (NULL terminated string) */
    UINT wXmin;                 /* minimum x position value */
    UINT wXmax;                 /* maximum x position value */
    UINT wYmin;                 /* minimum y position value */
    UINT wYmax;                 /* maximum y position value */
    UINT wZmin;                 /* minimum z position value */
    UINT wZmax;                 /* maximum z position value */
    UINT wNumButtons;           /* number of buttons */
    UINT wPeriodMin;            /* minimum message period when captured */
    UINT wPeriodMax;            /* maximum message period when captured */
#if (WINVER >= 0x0400)
    UINT wRmin;                 /* minimum r position value */
    UINT wRmax;                 /* maximum r position value */
    UINT wUmin;                 /* minimum u (5th axis) position value */
    UINT wUmax;                 /* maximum u (5th axis) position value */
    UINT wVmin;                 /* minimum v (6th axis) position value */
    UINT wVmax;                 /* maximum v (6th axis) position value */
    UINT wCaps;                 /* joystick capabilites */
    UINT wMaxAxes;	 	/* maximum number of axes supported */
    UINT wNumAxes;	 	/* number of axes in use */
    UINT wMaxButtons;	 	/* maximum number of buttons supported */
    char szRegKey[MAXPNAMELEN]; /* registry key */
    char szOEMVxD[MAX_JOYSTICKOEMVXDNAME]; /* OEM VxD in use */
#endif
} JOYCAPS, *PJOYCAPS, NEAR *NPJOYCAPS, FAR *LPJOYCAPS;
#endif

/* joystick information data structure */
typedef struct joyinfo_tag {
    UINT wXpos;                 /* x position */
    UINT wYpos;                 /* y position */
    UINT wZpos;                 /* z position */
    UINT wButtons;              /* button states */
} JOYINFO, *PJOYINFO, NEAR *NPJOYINFO, FAR *LPJOYINFO;

#if(WINVER >= 0x0400)
typedef struct joyinfoex_tag {
    DWORD dwSize;		 /* size of structure */
    DWORD dwFlags;		 /* flags to indicate what to return */
    DWORD dwXpos;                /* x position */
    DWORD dwYpos;                /* y position */
    DWORD dwZpos;                /* z position */
    DWORD dwRpos;		 /* rudder/4th axis position */
    DWORD dwUpos;		 /* 5th axis position */
    DWORD dwVpos;		 /* 6th axis position */
    DWORD dwButtons;             /* button states */
    DWORD dwButtonNumber;        /* current button number pressed */
    DWORD dwPOV;                 /* point of view state */
    DWORD dwReserved1;		 /* reserved for communication between winmm & driver */
    DWORD dwReserved2;		 /* reserved for future expansion */
} JOYINFOEX, *PJOYINFOEX, NEAR *NPJOYINFOEX, FAR *LPJOYINFOEX;
#endif /* WINVER >= 0x0400 */

#ifndef _WIN32_VXD

/* joystick function prototypes */
WINMMAPI UINT WINAPI joyGetNumDevs(void);
#ifdef _WIN32

WINMMAPI MMRESULT WINAPI joyGetDevCapsA(UINT uJoyID, LPJOYCAPSA pjc, UINT cbjc);
WINMMAPI MMRESULT WINAPI joyGetDevCapsW(UINT uJoyID, LPJOYCAPSW pjc, UINT cbjc);
#ifdef UNICODE
#define joyGetDevCaps  joyGetDevCapsW
#else
#define joyGetDevCaps  joyGetDevCapsA
#endif // !UNICODE

#else
MMRESULT WINAPI joyGetDevCaps(UINT uJoyID, LPJOYCAPS pjc, UINT cbjc);
#endif
WINMMAPI MMRESULT WINAPI joyGetPos(UINT uJoyID, LPJOYINFO pji);

#if(WINVER >= 0x0400)
WINMMAPI MMRESULT WINAPI joyGetPosEx(UINT uJoyID, LPJOYINFOEX pji);
#endif /* WINVER >= 0x0400 */

WINMMAPI MMRESULT WINAPI joyGetThreshold(UINT uJoyID, LPUINT puThreshold);
WINMMAPI MMRESULT WINAPI joyReleaseCapture(UINT uJoyID);
WINMMAPI MMRESULT WINAPI joySetCapture(HWND hwnd, UINT uJoyID, UINT uPeriod,
    BOOL fChanged);
WINMMAPI MMRESULT WINAPI joySetThreshold(UINT uJoyID, UINT uThreshold);

#endif /* ifndef _WIN32_VXD */

#endif  /* ifndef MMNOJOY */

#ifndef MMNOMMIO
#ifndef _WIN32_VXD
/****************************************************************************

			Multimedia File I/O support

****************************************************************************/

/* MMIO error return values */
#define MMIOERR_BASE                256
#define MMIOERR_FILENOTFOUND        (MMIOERR_BASE + 1)  /* file not found */
#define MMIOERR_OUTOFMEMORY         (MMIOERR_BASE + 2)  /* out of memory */
#define MMIOERR_CANNOTOPEN          (MMIOERR_BASE + 3)  /* cannot open */
#define MMIOERR_CANNOTCLOSE         (MMIOERR_BASE + 4)  /* cannot close */
#define MMIOERR_CANNOTREAD          (MMIOERR_BASE + 5)  /* cannot read */
#define MMIOERR_CANNOTWRITE         (MMIOERR_BASE + 6)  /* cannot write */
#define MMIOERR_CANNOTSEEK          (MMIOERR_BASE + 7)  /* cannot seek */
#define MMIOERR_CANNOTEXPAND        (MMIOERR_BASE + 8)  /* cannot expand file */
#define MMIOERR_CHUNKNOTFOUND       (MMIOERR_BASE + 9)  /* chunk not found */
#define MMIOERR_UNBUFFERED          (MMIOERR_BASE + 10) /*  */
#define MMIOERR_PATHNOTFOUND        (MMIOERR_BASE + 11) /* path incorrect */
#define MMIOERR_ACCESSDENIED        (MMIOERR_BASE + 12) /* file was protected */
#define MMIOERR_SHARINGVIOLATION    (MMIOERR_BASE + 13) /* file in use */
#define MMIOERR_NETWORKERROR        (MMIOERR_BASE + 14) /* network not responding */
#define MMIOERR_TOOMANYOPENFILES    (MMIOERR_BASE + 15) /* no more file handles  */
#define MMIOERR_INVALIDFILE         (MMIOERR_BASE + 16) /* default error file error */

/* MMIO constants */
#define CFSEPCHAR       '+'             /* compound file name separator char. */

/* MMIO data types */
typedef DWORD           FOURCC;         /* a four character code */
typedef char _huge *    HPSTR;          /* a huge version of LPSTR */
DECLARE_HANDLE(HMMIO);                  /* a handle to an open file */
typedef LRESULT (CALLBACK MMIOPROC)(LPSTR lpmmioinfo, UINT uMsg,
	    LPARAM lParam1, LPARAM lParam2);
typedef MMIOPROC FAR *LPMMIOPROC;

/* general MMIO information data structure */
typedef struct _MMIOINFO
{
	/* general fields */
	DWORD           dwFlags;        /* general status flags */
	FOURCC          fccIOProc;      /* pointer to I/O procedure */
	LPMMIOPROC      pIOProc;        /* pointer to I/O procedure */
	UINT            wErrorRet;      /* place for error to be returned */
	HTASK           htask;          /* alternate local task */

	/* fields maintained by MMIO functions during buffered I/O */
	LONG            cchBuffer;      /* size of I/O buffer (or 0L) */
	HPSTR           pchBuffer;      /* start of I/O buffer (or NULL) */
	HPSTR           pchNext;        /* pointer to next byte to read/write */
	HPSTR           pchEndRead;     /* pointer to last valid byte to read */
	HPSTR           pchEndWrite;    /* pointer to last byte to write */
	LONG            lBufOffset;     /* disk offset of start of buffer */

	/* fields maintained by I/O procedure */
	LONG            lDiskOffset;    /* disk offset of next read or write */
	DWORD           adwInfo[3];     /* data specific to type of MMIOPROC */

	/* other fields maintained by MMIO */
	DWORD           dwReserved1;    /* reserved for MMIO use */
	DWORD           dwReserved2;    /* reserved for MMIO use */
	HMMIO           hmmio;          /* handle to open file */
} MMIOINFO, *PMMIOINFO, NEAR *NPMMIOINFO, FAR *LPMMIOINFO;
typedef const MMIOINFO FAR *LPCMMIOINFO;

/* RIFF chunk information data structure */
typedef struct _MMCKINFO
{
	FOURCC          ckid;           /* chunk ID */
	DWORD           cksize;         /* chunk size */
	FOURCC          fccType;        /* form type or list type */
	DWORD           dwDataOffset;   /* offset of data portion of chunk */
	DWORD           dwFlags;        /* flags used by MMIO functions */
} MMCKINFO, *PMMCKINFO, NEAR *NPMMCKINFO, FAR *LPMMCKINFO;
typedef const MMCKINFO *LPCMMCKINFO;

/* bit field masks */
#define MMIO_RWMODE     0x00000003      /* open file for reading/writing/both */
#define MMIO_SHAREMODE  0x00000070      /* file sharing mode number */

/* constants for dwFlags field of MMIOINFO */
#define MMIO_CREATE     0x00001000      /* create new file (or truncate file) */
#define MMIO_PARSE      0x00000100      /* parse new file returning path */
#define MMIO_DELETE     0x00000200      /* create new file (or truncate file) */
#define MMIO_EXIST      0x00004000      /* checks for existence of file */
#define MMIO_ALLOCBUF   0x00010000      /* mmioOpen() should allocate a buffer */
#define MMIO_GETTEMP    0x00020000      /* mmioOpen() should retrieve temp name */

#define MMIO_DIRTY      0x10000000      /* I/O buffer is dirty */

/* read/write mode numbers (bit field MMIO_RWMODE) */
#define MMIO_READ       0x00000000      /* open file for reading only */
#define MMIO_WRITE      0x00000001      /* open file for writing only */
#define MMIO_READWRITE  0x00000002      /* open file for reading and writing */

/* share mode numbers (bit field MMIO_SHAREMODE) */
#define MMIO_COMPAT     0x00000000      /* compatibility mode */
#define MMIO_EXCLUSIVE  0x00000010      /* exclusive-access mode */
#define MMIO_DENYWRITE  0x00000020      /* deny writing to other processes */
#define MMIO_DENYREAD   0x00000030      /* deny reading to other processes */
#define MMIO_DENYNONE   0x00000040      /* deny nothing to other processes */

/* various MMIO flags */
#define MMIO_FHOPEN             0x0010  /* mmioClose: keep file handle open */
#define MMIO_EMPTYBUF           0x0010  /* mmioFlush: empty the I/O buffer */
#define MMIO_TOUPPER            0x0010  /* mmioStringToFOURCC: to u-case */
#define MMIO_INSTALLPROC    0x00010000  /* mmioInstallIOProc: install MMIOProc */
#define MMIO_GLOBALPROC     0x10000000  /* mmioInstallIOProc: install globally */
#define MMIO_REMOVEPROC     0x00020000  /* mmioInstallIOProc: remove MMIOProc */
#define MMIO_UNICODEPROC    0x01000000  /* mmioInstallIOProc: Unicode MMIOProc */
#define MMIO_FINDPROC       0x00040000  /* mmioInstallIOProc: find an MMIOProc */
#define MMIO_FINDCHUNK          0x0010  /* mmioDescend: find a chunk by ID */
#define MMIO_FINDRIFF           0x0020  /* mmioDescend: find a LIST chunk */
#define MMIO_FINDLIST           0x0040  /* mmioDescend: find a RIFF chunk */
#define MMIO_CREATERIFF         0x0020  /* mmioCreateChunk: make a LIST chunk */
#define MMIO_CREATELIST         0x0040  /* mmioCreateChunk: make a RIFF chunk */

/* message numbers for MMIOPROC I/O procedure functions */
#define MMIOM_READ      MMIO_READ       /* read */
#define MMIOM_WRITE    MMIO_WRITE       /* write */
#define MMIOM_SEEK              2       /* seek to a new position in file */
#define MMIOM_OPEN              3       /* open file */
#define MMIOM_CLOSE             4       /* close file */
#define MMIOM_WRITEFLUSH        5       /* write and flush */

#if (WINVER >= 0x030a)
#define MMIOM_RENAME            6       /* rename specified file */
#endif /* ifdef WINVER >= 0x030a */

#define MMIOM_USER         0x8000       /* beginning of user-defined messages */

/* standard four character codes */
#define FOURCC_RIFF     mmioFOURCC('R', 'I', 'F', 'F')
#define FOURCC_LIST     mmioFOURCC('L', 'I', 'S', 'T')

/* four character codes used to identify standard built-in I/O procedures */
#define FOURCC_DOS      mmioFOURCC('D', 'O', 'S', ' ')
#define FOURCC_MEM      mmioFOURCC('M', 'E', 'M', ' ')

/* flags for mmioSeek() */
#ifndef SEEK_SET
#define SEEK_SET        0               /* seek to an absolute position */
#define SEEK_CUR        1               /* seek relative to current position */
#define SEEK_END        2               /* seek relative to end of file */
#endif  /* ifndef SEEK_SET */

/* other constants */
#define MMIO_DEFAULTBUFFER      8192    /* default buffer size */

/* MMIO macros */
#define mmioFOURCC(ch0, ch1, ch2, ch3)  MAKEFOURCC(ch0, ch1, ch2, ch3)

/* MMIO function prototypes */
#ifdef _WIN32

WINMMAPI FOURCC WINAPI mmioStringToFOURCCA(LPCSTR sz, UINT uFlags);
WINMMAPI FOURCC WINAPI mmioStringToFOURCCW(LPCWSTR sz, UINT uFlags);
#ifdef UNICODE
#define mmioStringToFOURCC  mmioStringToFOURCCW
#else
#define mmioStringToFOURCC  mmioStringToFOURCCA
#endif // !UNICODE
WINMMAPI LPMMIOPROC WINAPI mmioInstallIOProcA(FOURCC fccIOProc, LPMMIOPROC pIOProc, DWORD dwFlags);
WINMMAPI LPMMIOPROC WINAPI mmioInstallIOProcW(FOURCC fccIOProc, LPMMIOPROC pIOProc, DWORD dwFlags);
#ifdef UNICODE
#define mmioInstallIOProc  mmioInstallIOProcW
#else
#define mmioInstallIOProc  mmioInstallIOProcA
#endif // !UNICODE
WINMMAPI HMMIO WINAPI mmioOpenA(LPSTR pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen);
WINMMAPI HMMIO WINAPI mmioOpenW(LPWSTR pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen);
#ifdef UNICODE
#define mmioOpen  mmioOpenW
#else
#define mmioOpen  mmioOpenA
#endif // !UNICODE
WINMMAPI MMRESULT WINAPI mmioRenameA(LPCSTR pszFileName, LPCSTR pszNewFileName, LPCMMIOINFO pmmioinfo, DWORD fdwRename);
WINMMAPI MMRESULT WINAPI mmioRenameW(LPCWSTR pszFileName, LPCWSTR pszNewFileName, LPCMMIOINFO pmmioinfo, DWORD fdwRename);
#ifdef UNICODE
#define mmioRename  mmioRenameW
#else
#define mmioRename  mmioRenameA
#endif // !UNICODE
#else
FOURCC WINAPI mmioStringToFOURCC(LPCSTR sz, UINT uFlags);
LPMMIOPROC WINAPI mmioInstallIOProc(FOURCC fccIOProc, LPMMIOPROC pIOProc, DWORD dwFlags);
HMMIO WINAPI mmioOpen(LPSTR pszFileName, LPMMIOINFO pmmioinfo, DWORD fdwOpen);
#if (WINVER >= 0x030a)
MMRESULT WINAPI mmioRename(LPCSTR pszFileName, LPCSTR pszNewFileName, const MMIOINFO FAR* pmmioinfo, DWORD fdwRename);
#endif /* ifdef WINVER >= 0x030a */
#endif

WINMMAPI MMRESULT WINAPI mmioClose(HMMIO hmmio, UINT fuClose);
WINMMAPI LONG WINAPI mmioRead(HMMIO hmmio, HPSTR pch, LONG cch);
WINMMAPI LONG WINAPI mmioWrite(HMMIO hmmio, const char _huge* pch, LONG cch);
WINMMAPI LONG WINAPI mmioSeek(HMMIO hmmio, LONG lOffset, int iOrigin);
WINMMAPI MMRESULT WINAPI mmioGetInfo(HMMIO hmmio, LPMMIOINFO pmmioinfo, UINT fuInfo);
WINMMAPI MMRESULT WINAPI mmioSetInfo(HMMIO hmmio, LPCMMIOINFO pmmioinfo, UINT fuInfo);
WINMMAPI MMRESULT WINAPI mmioSetBuffer(HMMIO hmmio, LPSTR pchBuffer, LONG cchBuffer,
    UINT fuBuffer);
WINMMAPI MMRESULT WINAPI mmioFlush(HMMIO hmmio, UINT fuFlush);
WINMMAPI MMRESULT WINAPI mmioAdvance(HMMIO hmmio, LPMMIOINFO pmmioinfo, UINT fuAdvance);
WINMMAPI LRESULT WINAPI mmioSendMessage(HMMIO hmmio, UINT uMsg,
    LPARAM lParam1, LPARAM lParam2);
WINMMAPI MMRESULT WINAPI mmioDescend(HMMIO hmmio, LPMMCKINFO pmmcki,
    const MMCKINFO FAR* pmmckiParent, UINT fuDescend);
WINMMAPI MMRESULT WINAPI mmioAscend(HMMIO hmmio, LPMMCKINFO pmmcki, UINT fuAscend);
WINMMAPI MMRESULT WINAPI mmioCreateChunk(HMMIO hmmio, LPMMCKINFO pmmcki, UINT fuCreate);

#endif  /* ifndef _WIN32_VXD */
#endif  /* ifndef MMNOMMIO */

#ifndef MMNOMCI
#ifndef _WIN32_VXD
/****************************************************************************

			    MCI support

****************************************************************************/

#ifndef _MCIERROR_              /* MCIERROR is defined in some post 3.1 apps */
#define _MCIERROR_
typedef DWORD   MCIERROR;       /* error return code, 0 means no error */
#endif

#ifndef _MCIDEVICEID_           /* Same with MCIDEVICEID */
#define _MCIDEVICEID_
typedef UINT    MCIDEVICEID;    /* MCI device ID type */
#endif

typedef UINT (CALLBACK *YIELDPROC)(MCIDEVICEID mciId, DWORD dwYieldData);

/* MCI function prototypes */
#ifdef _WIN32

WINMMAPI MCIERROR WINAPI mciSendCommandA(MCIDEVICEID mciId, UINT uMsg, DWORD dwParam1, DWORD dwParam2);
WINMMAPI MCIERROR WINAPI mciSendCommandW(MCIDEVICEID mciId, UINT uMsg, DWORD dwParam1, DWORD dwParam2);
#ifdef UNICODE
#define mciSendCommand  mciSendCommandW
#else
#define mciSendCommand  mciSendCommandA
#endif // !UNICODE
WINMMAPI MCIERROR  WINAPI mciSendStringA(LPCSTR lpstrCommand, LPSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback);
WINMMAPI MCIERROR  WINAPI mciSendStringW(LPCWSTR lpstrCommand, LPWSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback);
#ifdef UNICODE
#define mciSendString  mciSendStringW
#else
#define mciSendString  mciSendStringA
#endif // !UNICODE
WINMMAPI MCIDEVICEID WINAPI mciGetDeviceIDA(LPCSTR pszDevice);
WINMMAPI MCIDEVICEID WINAPI mciGetDeviceIDW(LPCWSTR pszDevice);
#ifdef UNICODE
#define mciGetDeviceID  mciGetDeviceIDW
#else
#define mciGetDeviceID  mciGetDeviceIDA
#endif // !UNICODE
WINMMAPI MCIDEVICEID WINAPI mciGetDeviceIDFromElementIDA(DWORD dwElementID, LPCSTR lpstrType );
WINMMAPI MCIDEVICEID WINAPI mciGetDeviceIDFromElementIDW(DWORD dwElementID, LPCWSTR lpstrType );
#ifdef UNICODE
#define mciGetDeviceIDFromElementID  mciGetDeviceIDFromElementIDW
#else
#define mciGetDeviceIDFromElementID  mciGetDeviceIDFromElementIDA
#endif // !UNICODE
WINMMAPI BOOL WINAPI mciGetErrorStringA(MCIERROR mcierr, LPSTR pszText, UINT cchText);
WINMMAPI BOOL WINAPI mciGetErrorStringW(MCIERROR mcierr, LPWSTR pszText, UINT cchText);
#ifdef UNICODE
#define mciGetErrorString  mciGetErrorStringW
#else
#define mciGetErrorString  mciGetErrorStringA
#endif // !UNICODE

#else
MCIERROR WINAPI mciSendCommand(MCIDEVICEID mciId, UINT uMsg, DWORD dwParam1, DWORD dwParam2);
MCIERROR  WINAPI mciSendString(LPCSTR lpstrCommand, LPSTR lpstrReturnString, UINT uReturnLength, HWND hwndCallback);
MCIDEVICEID WINAPI mciGetDeviceID(LPCSTR pszDevice);
BOOL WINAPI mciGetErrorString(MCIERROR mcierr, LPSTR pszText, UINT cchText);
#endif

WINMMAPI BOOL WINAPI mciSetYieldProc(MCIDEVICEID mciId, YIELDPROC fpYieldProc,
    DWORD dwYieldData);

#if (WINVER >= 0x030a)
WINMMAPI HTASK WINAPI mciGetCreatorTask(MCIDEVICEID mciId);
WINMMAPI YIELDPROC WINAPI mciGetYieldProc(MCIDEVICEID mciId, LPDWORD pdwYieldData);
#endif /* ifdef WINVER >= 0x030a */

#if (WINVER < 0x030a)
WINMMAPI BOOL WINAPI mciExecute(LPCSTR pszCommand);
#endif /* ifdef WINVER < 0x030a */

/* MCI error return values */
#define MCIERR_INVALID_DEVICE_ID        (MCIERR_BASE + 1)
#define MCIERR_UNRECOGNIZED_KEYWORD     (MCIERR_BASE + 3)
#define MCIERR_UNRECOGNIZED_COMMAND     (MCIERR_BASE + 5)
#define MCIERR_HARDWARE                 (MCIERR_BASE + 6)
#define MCIERR_INVALID_DEVICE_NAME      (MCIERR_BASE + 7)
#define MCIERR_OUT_OF_MEMORY            (MCIERR_BASE + 8)
#define MCIERR_DEVICE_OPEN              (MCIERR_BASE + 9)
#define MCIERR_CANNOT_LOAD_DRIVER       (MCIERR_BASE + 10)
#define MCIERR_MISSING_COMMAND_STRING   (MCIERR_BASE + 11)
#define MCIERR_PARAM_OVERFLOW           (MCIERR_BASE + 12)
#define MCIERR_MISSING_STRING_ARGUMENT  (MCIERR_BASE + 13)
#define MCIERR_BAD_INTEGER              (MCIERR_BASE + 14)
#define MCIERR_PARSER_INTERNAL          (MCIERR_BASE + 15)
#define MCIERR_DRIVER_INTERNAL          (MCIERR_BASE + 16)
#define MCIERR_MISSING_PARAMETER        (MCIERR_BASE + 17)
#define MCIERR_UNSUPPORTED_FUNCTION     (MCIERR_BASE + 18)
#define MCIERR_FILE_NOT_FOUND           (MCIERR_BASE + 19)
#define MCIERR_DEVICE_NOT_READY         (MCIERR_BASE + 20)
#define MCIERR_INTERNAL                 (MCIERR_BASE + 21)
#define MCIERR_DRIVER                   (MCIERR_BASE + 22)
#define MCIERR_CANNOT_USE_ALL           (MCIERR_BASE + 23)
#define MCIERR_MULTIPLE                 (MCIERR_BASE + 24)
#define MCIERR_EXTENSION_NOT_FOUND      (MCIERR_BASE + 25)
#define MCIERR_OUTOFRANGE               (MCIERR_BASE + 26)
#define MCIERR_FLAGS_NOT_COMPATIBLE     (MCIERR_BASE + 28)
#define MCIERR_FILE_NOT_SAVED           (MCIERR_BASE + 30)
#define MCIERR_DEVICE_TYPE_REQUIRED     (MCIERR_BASE + 31)
#define MCIERR_DEVICE_LOCKED            (MCIERR_BASE + 32)
#define MCIERR_DUPLICATE_ALIAS          (MCIERR_BASE + 33)
#define MCIERR_BAD_CONSTANT             (MCIERR_BASE + 34)
#define MCIERR_MUST_USE_SHAREABLE       (MCIERR_BASE + 35)
#define MCIERR_MISSING_DEVICE_NAME      (MCIERR_BASE + 36)
#define MCIERR_BAD_TIME_FORMAT          (MCIERR_BASE + 37)
#define MCIERR_NO_CLOSING_QUOTE         (MCIERR_BASE + 38)
#define MCIERR_DUPLICATE_FLAGS          (MCIERR_BASE + 39)
#define MCIERR_INVALID_FILE             (MCIERR_BASE + 40)
#define MCIERR_NULL_PARAMETER_BLOCK     (MCIERR_BASE + 41)
#define MCIERR_UNNAMED_RESOURCE         (MCIERR_BASE + 42)
#define MCIERR_NEW_REQUIRES_ALIAS       (MCIERR_BASE + 43)
#define MCIERR_NOTIFY_ON_AUTO_OPEN      (MCIERR_BASE + 44)
#define MCIERR_NO_ELEMENT_ALLOWED       (MCIERR_BASE + 45)
#define MCIERR_NONAPPLICABLE_FUNCTION   (MCIERR_BASE + 46)
#define MCIERR_ILLEGAL_FOR_AUTO_OPEN    (MCIERR_BASE + 47)
#define MCIERR_FILENAME_REQUIRED        (MCIERR_BASE + 48)
#define MCIERR_EXTRA_CHARACTERS         (MCIERR_BASE + 49)
#define MCIERR_DEVICE_NOT_INSTALLED     (MCIERR_BASE + 50)
#define MCIERR_GET_CD                   (MCIERR_BASE + 51)
#define MCIERR_SET_CD                   (MCIERR_BASE + 52)
#define MCIERR_SET_DRIVE                (MCIERR_BASE + 53)
#define MCIERR_DEVICE_LENGTH            (MCIERR_BASE + 54)
#define MCIERR_DEVICE_ORD_LENGTH        (MCIERR_BASE + 55)
#define MCIERR_NO_INTEGER               (MCIERR_BASE + 56)

#define MCIERR_WAVE_OUTPUTSINUSE        (MCIERR_BASE + 64)
#define MCIERR_WAVE_SETOUTPUTINUSE      (MCIERR_BASE + 65)
#define MCIERR_WAVE_INPUTSINUSE         (MCIERR_BASE + 66)
#define MCIERR_WAVE_SETINPUTINUSE       (MCIERR_BASE + 67)
#define MCIERR_WAVE_OUTPUTUNSPECIFIED   (MCIERR_BASE + 68)
#define MCIERR_WAVE_INPUTUNSPECIFIED    (MCIERR_BASE + 69)
#define MCIERR_WAVE_OUTPUTSUNSUITABLE   (MCIERR_BASE + 70)
#define MCIERR_WAVE_SETOUTPUTUNSUITABLE (MCIERR_BASE + 71)
#define MCIERR_WAVE_INPUTSUNSUITABLE    (MCIERR_BASE + 72)
#define MCIERR_WAVE_SETINPUTUNSUITABLE  (MCIERR_BASE + 73)

#define MCIERR_SEQ_DIV_INCOMPATIBLE     (MCIERR_BASE + 80)
#define MCIERR_SEQ_PORT_INUSE           (MCIERR_BASE + 81)
#define MCIERR_SEQ_PORT_NONEXISTENT     (MCIERR_BASE + 82)
#define MCIERR_SEQ_PORT_MAPNODEVICE     (MCIERR_BASE + 83)
#define MCIERR_SEQ_PORT_MISCERROR       (MCIERR_BASE + 84)
#define MCIERR_SEQ_TIMER                (MCIERR_BASE + 85)
#define MCIERR_SEQ_PORTUNSPECIFIED      (MCIERR_BASE + 86)
#define MCIERR_SEQ_NOMIDIPRESENT        (MCIERR_BASE + 87)

#define MCIERR_NO_WINDOW                (MCIERR_BASE + 90)
#define MCIERR_CREATEWINDOW             (MCIERR_BASE + 91)
#define MCIERR_FILE_READ                (MCIERR_BASE + 92)
#define MCIERR_FILE_WRITE               (MCIERR_BASE + 93)

#define MCIERR_NO_IDENTITY              (MCIERR_BASE + 94)

/* all custom device driver errors must be >= than this value */
#define MCIERR_CUSTOM_DRIVER_BASE       (MCIERR_BASE + 256)

#define MCI_FIRST                       DRV_MCI_FIRST   /* 0x0800 */
/* MCI command message identifiers */
#define MCI_OPEN                        0x0803
#define MCI_CLOSE                       0x0804
#define MCI_ESCAPE                      0x0805
#define MCI_PLAY                        0x0806
#define MCI_SEEK                        0x0807
#define MCI_STOP                        0x0808
#define MCI_PAUSE                       0x0809
#define MCI_INFO                        0x080A
#define MCI_GETDEVCAPS                  0x080B
#define MCI_SPIN                        0x080C
#define MCI_SET                         0x080D
#define MCI_STEP                        0x080E
#define MCI_RECORD                      0x080F
#define MCI_SYSINFO                     0x0810
#define MCI_BREAK                       0x0811
#define MCI_SAVE                        0x0813
#define MCI_STATUS                      0x0814
#define MCI_CUE                         0x0830
#define MCI_REALIZE                     0x0840
#define MCI_WINDOW                      0x0841
#define MCI_PUT                         0x0842
#define MCI_WHERE                       0x0843
#define MCI_FREEZE                      0x0844
#define MCI_UNFREEZE                    0x0845
#define MCI_LOAD                        0x0850
#define MCI_CUT                         0x0851
#define MCI_COPY                        0x0852
#define MCI_PASTE                       0x0853
#define MCI_UPDATE                      0x0854
#define MCI_RESUME                      0x0855
#define MCI_DELETE                      0x0856

/* all custom MCI command messages must be >= than this value */
#define MCI_USER_MESSAGES               (DRV_MCI_FIRST + 0x400)
#define MCI_LAST                        0x0FFF

/* device ID for "all devices" */
#define MCI_ALL_DEVICE_ID               ((MCIDEVICEID)-1)

/* constants for predefined MCI device types */
#define MCI_DEVTYPE_VCR                 513 /* (MCI_STRING_OFFSET + 1) */
#define MCI_DEVTYPE_VIDEODISC           514 /* (MCI_STRING_OFFSET + 2) */
#define MCI_DEVTYPE_OVERLAY             515 /* (MCI_STRING_OFFSET + 3) */
#define MCI_DEVTYPE_CD_AUDIO            516 /* (MCI_STRING_OFFSET + 4) */
#define MCI_DEVTYPE_DAT                 517 /* (MCI_STRING_OFFSET + 5) */
#define MCI_DEVTYPE_SCANNER             518 /* (MCI_STRING_OFFSET + 6) */
#define MCI_DEVTYPE_ANIMATION           519 /* (MCI_STRING_OFFSET + 7) */
#define MCI_DEVTYPE_DIGITAL_VIDEO       520 /* (MCI_STRING_OFFSET + 8) */
#define MCI_DEVTYPE_OTHER               521 /* (MCI_STRING_OFFSET + 9) */
#define MCI_DEVTYPE_WAVEFORM_AUDIO      522 /* (MCI_STRING_OFFSET + 10) */
#define MCI_DEVTYPE_SEQUENCER           523 /* (MCI_STRING_OFFSET + 11) */

#define MCI_DEVTYPE_FIRST               MCI_DEVTYPE_VCR
#define MCI_DEVTYPE_LAST                MCI_DEVTYPE_SEQUENCER

#define MCI_DEVTYPE_FIRST_USER          0x1000
/* return values for 'status mode' command */
#define MCI_MODE_NOT_READY              (MCI_STRING_OFFSET + 12)
#define MCI_MODE_STOP                   (MCI_STRING_OFFSET + 13)
#define MCI_MODE_PLAY                   (MCI_STRING_OFFSET + 14)
#define MCI_MODE_RECORD                 (MCI_STRING_OFFSET + 15)
#define MCI_MODE_SEEK                   (MCI_STRING_OFFSET + 16)
#define MCI_MODE_PAUSE                  (MCI_STRING_OFFSET + 17)
#define MCI_MODE_OPEN                   (MCI_STRING_OFFSET + 18)

/* constants used in 'set time format' and 'status time format' commands */
#define MCI_FORMAT_MILLISECONDS         0
#define MCI_FORMAT_HMS                  1
#define MCI_FORMAT_MSF                  2
#define MCI_FORMAT_FRAMES               3
#define MCI_FORMAT_SMPTE_24             4
#define MCI_FORMAT_SMPTE_25             5
#define MCI_FORMAT_SMPTE_30             6
#define MCI_FORMAT_SMPTE_30DROP         7
#define MCI_FORMAT_BYTES                8
#define MCI_FORMAT_SAMPLES              9
#define MCI_FORMAT_TMSF                 10

/* MCI time format conversion macros */
#define MCI_MSF_MINUTE(msf)             ((BYTE)(msf))
#define MCI_MSF_SECOND(msf)             ((BYTE)(((WORD)(msf)) >> 8))
#define MCI_MSF_FRAME(msf)              ((BYTE)((msf)>>16))

#define MCI_MAKE_MSF(m, s, f)           ((DWORD)(((BYTE)(m) | \
						  ((WORD)(s)<<8)) | \
						 (((DWORD)(BYTE)(f))<<16)))

#define MCI_TMSF_TRACK(tmsf)            ((BYTE)(tmsf))
#define MCI_TMSF_MINUTE(tmsf)           ((BYTE)(((WORD)(tmsf)) >> 8))
#define MCI_TMSF_SECOND(tmsf)           ((BYTE)((tmsf)>>16))
#define MCI_TMSF_FRAME(tmsf)            ((BYTE)((tmsf)>>24))

#define MCI_MAKE_TMSF(t, m, s, f)       ((DWORD)(((BYTE)(t) | \
						  ((WORD)(m)<<8)) | \
						 (((DWORD)(BYTE)(s) | \
						   ((WORD)(f)<<8))<<16)))

#define MCI_HMS_HOUR(hms)               ((BYTE)(hms))
#define MCI_HMS_MINUTE(hms)             ((BYTE)(((WORD)(hms)) >> 8))
#define MCI_HMS_SECOND(hms)             ((BYTE)((hms)>>16))

#define MCI_MAKE_HMS(h, m, s)           ((DWORD)(((BYTE)(h) | \
						  ((WORD)(m)<<8)) | \
						 (((DWORD)(BYTE)(s))<<16)))

/* flags for wParam of MM_MCINOTIFY message */
#define MCI_NOTIFY_SUCCESSFUL           0x0001
#define MCI_NOTIFY_SUPERSEDED           0x0002
#define MCI_NOTIFY_ABORTED              0x0004
#define MCI_NOTIFY_FAILURE              0x0008

/* common flags for dwFlags parameter of MCI command messages */
#define MCI_NOTIFY                      0x00000001L
#define MCI_WAIT                        0x00000002L
#define MCI_FROM                        0x00000004L
#define MCI_TO                          0x00000008L
#define MCI_TRACK                       0x00000010L

/* flags for dwFlags parameter of MCI_OPEN command message */
#define MCI_OPEN_SHAREABLE              0x00000100L
#define MCI_OPEN_ELEMENT                0x00000200L
#define MCI_OPEN_ALIAS                  0x00000400L
#define MCI_OPEN_ELEMENT_ID             0x00000800L
#define MCI_OPEN_TYPE_ID                0x00001000L
#define MCI_OPEN_TYPE                   0x00002000L

/* flags for dwFlags parameter of MCI_SEEK command message */
#define MCI_SEEK_TO_START               0x00000100L
#define MCI_SEEK_TO_END                 0x00000200L

/* flags for dwFlags parameter of MCI_STATUS command message */
#define MCI_STATUS_ITEM                 0x00000100L
#define MCI_STATUS_START                0x00000200L

/* flags for dwItem field of the MCI_STATUS_PARMS parameter block */
#define MCI_STATUS_LENGTH               0x00000001L
#define MCI_STATUS_POSITION             0x00000002L
#define MCI_STATUS_NUMBER_OF_TRACKS     0x00000003L
#define MCI_STATUS_MODE                 0x00000004L
#define MCI_STATUS_MEDIA_PRESENT        0x00000005L
#define MCI_STATUS_TIME_FORMAT          0x00000006L
#define MCI_STATUS_READY                0x00000007L
#define MCI_STATUS_CURRENT_TRACK        0x00000008L

/* flags for dwFlags parameter of MCI_INFO command message */
#define MCI_INFO_PRODUCT                0x00000100L
#define MCI_INFO_FILE                   0x00000200L
#define MCI_INFO_MEDIA_UPC              0x00000400L
#define MCI_INFO_MEDIA_IDENTITY         0x00000800L
#define MCI_INFO_NAME                   0x00001000L
#define MCI_INFO_COPYRIGHT              0x00002000L

/* flags for dwFlags parameter of MCI_GETDEVCAPS command message */
#define MCI_GETDEVCAPS_ITEM             0x00000100L

/* flags for dwItem field of the MCI_GETDEVCAPS_PARMS parameter block */
#define MCI_GETDEVCAPS_CAN_RECORD       0x00000001L
#define MCI_GETDEVCAPS_HAS_AUDIO        0x00000002L
#define MCI_GETDEVCAPS_HAS_VIDEO        0x00000003L
#define MCI_GETDEVCAPS_DEVICE_TYPE      0x00000004L
#define MCI_GETDEVCAPS_USES_FILES       0x00000005L
#define MCI_GETDEVCAPS_COMPOUND_DEVICE  0x00000006L
#define MCI_GETDEVCAPS_CAN_EJECT        0x00000007L
#define MCI_GETDEVCAPS_CAN_PLAY         0x00000008L
#define MCI_GETDEVCAPS_CAN_SAVE         0x00000009L

/* flags for dwFlags parameter of MCI_SYSINFO command message */
#define MCI_SYSINFO_QUANTITY            0x00000100L
#define MCI_SYSINFO_OPEN                0x00000200L
#define MCI_SYSINFO_NAME                0x00000400L
#define MCI_SYSINFO_INSTALLNAME         0x00000800L

/* flags for dwFlags parameter of MCI_SET command message */
#define MCI_SET_DOOR_OPEN               0x00000100L
#define MCI_SET_DOOR_CLOSED             0x00000200L
#define MCI_SET_TIME_FORMAT             0x00000400L
#define MCI_SET_AUDIO                   0x00000800L
#define MCI_SET_VIDEO                   0x00001000L
#define MCI_SET_ON                      0x00002000L
#define MCI_SET_OFF                     0x00004000L

/* flags for dwAudio field of MCI_SET_PARMS or MCI_SEQ_SET_PARMS */
#define MCI_SET_AUDIO_ALL               0x00000000L
#define MCI_SET_AUDIO_LEFT              0x00000001L
#define MCI_SET_AUDIO_RIGHT             0x00000002L

/* flags for dwFlags parameter of MCI_BREAK command message */
#define MCI_BREAK_KEY                   0x00000100L
#define MCI_BREAK_HWND                  0x00000200L
#define MCI_BREAK_OFF                   0x00000400L

/* flags for dwFlags parameter of MCI_RECORD command message */
#define MCI_RECORD_INSERT               0x00000100L
#define MCI_RECORD_OVERWRITE            0x00000200L

/* flags for dwFlags parameter of MCI_SAVE command message */
#define MCI_SAVE_FILE                   0x00000100L

/* flags for dwFlags parameter of MCI_LOAD command message */
#define MCI_LOAD_FILE                   0x00000100L

/* generic parameter block for MCI command messages with no special parameters */
typedef struct tagMCI_GENERIC_PARMS {
    DWORD   dwCallback;
} MCI_GENERIC_PARMS, *PMCI_GENERIC_PARMS, FAR *LPMCI_GENERIC_PARMS;

/* parameter block for MCI_OPEN command message */
#ifdef _WIN32

typedef struct tagMCI_OPEN_PARMSA {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    LPCSTR     lpstrDeviceType;
    LPCSTR     lpstrElementName;
    LPCSTR     lpstrAlias;
} MCI_OPEN_PARMSA, *PMCI_OPEN_PARMSA, *LPMCI_OPEN_PARMSA;
typedef struct tagMCI_OPEN_PARMSW {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    LPCWSTR    lpstrDeviceType;
    LPCWSTR    lpstrElementName;
    LPCWSTR    lpstrAlias;
} MCI_OPEN_PARMSW, *PMCI_OPEN_PARMSW, *LPMCI_OPEN_PARMSW;
#ifdef UNICODE
typedef MCI_OPEN_PARMSW MCI_OPEN_PARMS;
typedef PMCI_OPEN_PARMSW PMCI_OPEN_PARMS;
typedef LPMCI_OPEN_PARMSW LPMCI_OPEN_PARMS;
#else
typedef MCI_OPEN_PARMSA MCI_OPEN_PARMS;
typedef PMCI_OPEN_PARMSA PMCI_OPEN_PARMS;
typedef LPMCI_OPEN_PARMSA LPMCI_OPEN_PARMS;
#endif // UNICODE

#else
typedef struct tagMCI_OPEN_PARMS {
    DWORD       dwCallback;
    MCIDEVICEID wDeviceID;
    WORD        wReserved0;
    LPCSTR      lpstrDeviceType;
    LPCSTR      lpstrElementName;
    LPCSTR      lpstrAlias;
} MCI_OPEN_PARMS, FAR *LPMCI_OPEN_PARMS;
#endif

/* parameter block for MCI_PLAY command message */
typedef struct tagMCI_PLAY_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
} MCI_PLAY_PARMS, *PMCI_PLAY_PARMS, FAR *LPMCI_PLAY_PARMS;

/* parameter block for MCI_SEEK command message */
typedef struct tagMCI_SEEK_PARMS {
    DWORD   dwCallback;
    DWORD   dwTo;
} MCI_SEEK_PARMS, *PMCI_SEEK_PARMS, FAR *LPMCI_SEEK_PARMS;

/* parameter block for MCI_STATUS command message */
typedef struct tagMCI_STATUS_PARMS {
    DWORD   dwCallback;
    DWORD   dwReturn;
    DWORD   dwItem;
    DWORD   dwTrack;
} MCI_STATUS_PARMS, *PMCI_STATUS_PARMS, FAR * LPMCI_STATUS_PARMS;

/* parameter block for MCI_INFO command message */
#ifdef _WIN32

typedef struct tagMCI_INFO_PARMSA {
    DWORD   dwCallback;
    LPSTR   lpstrReturn;
    DWORD   dwRetSize;
} MCI_INFO_PARMSA, * LPMCI_INFO_PARMSA;
typedef struct tagMCI_INFO_PARMSW {
    DWORD   dwCallback;
    LPWSTR  lpstrReturn;
    DWORD   dwRetSize;
} MCI_INFO_PARMSW, * LPMCI_INFO_PARMSW;
#ifdef UNICODE
typedef MCI_INFO_PARMSW MCI_INFO_PARMS;
typedef LPMCI_INFO_PARMSW LPMCI_INFO_PARMS;
#else
typedef MCI_INFO_PARMSA MCI_INFO_PARMS;
typedef LPMCI_INFO_PARMSA LPMCI_INFO_PARMS;
#endif // UNICODE

#else
typedef struct tagMCI_INFO_PARMS {
    DWORD   dwCallback;
    LPSTR   lpstrReturn;
    DWORD   dwRetSize;
} MCI_INFO_PARMS, FAR * LPMCI_INFO_PARMS;
#endif

/* parameter block for MCI_GETDEVCAPS command message */
typedef struct tagMCI_GETDEVCAPS_PARMS {
    DWORD   dwCallback;
    DWORD   dwReturn;
    DWORD   dwItem;
} MCI_GETDEVCAPS_PARMS, *PMCI_GETDEVCAPS_PARMS, FAR * LPMCI_GETDEVCAPS_PARMS;

/* parameter block for MCI_SYSINFO command message */
#ifdef _WIN32

typedef struct tagMCI_SYSINFO_PARMSA {
    DWORD   dwCallback;
    LPSTR   lpstrReturn;
    DWORD   dwRetSize;
    DWORD   dwNumber;
    UINT    wDeviceType;
} MCI_SYSINFO_PARMSA, *PMCI_SYSINFO_PARMSA, * LPMCI_SYSINFO_PARMSA;
typedef struct tagMCI_SYSINFO_PARMSW {
    DWORD   dwCallback;
    LPWSTR  lpstrReturn;
    DWORD   dwRetSize;
    DWORD   dwNumber;
    UINT    wDeviceType;
} MCI_SYSINFO_PARMSW, *PMCI_SYSINFO_PARMSW, * LPMCI_SYSINFO_PARMSW;
#ifdef UNICODE
typedef MCI_SYSINFO_PARMSW MCI_SYSINFO_PARMS;
typedef PMCI_SYSINFO_PARMSW PMCI_SYSINFO_PARMS;
typedef LPMCI_SYSINFO_PARMSW LPMCI_SYSINFO_PARMS;
#else
typedef MCI_SYSINFO_PARMSA MCI_SYSINFO_PARMS;
typedef PMCI_SYSINFO_PARMSA PMCI_SYSINFO_PARMS;
typedef LPMCI_SYSINFO_PARMSA LPMCI_SYSINFO_PARMS;
#endif // UNICODE
#else
typedef struct tagMCI_SYSINFO_PARMS {
    DWORD   dwCallback;
    LPSTR   lpstrReturn;
    DWORD   dwRetSize;
    DWORD   dwNumber;
    WORD    wDeviceType;
    WORD    wReserved0;
} MCI_SYSINFO_PARMS, FAR * LPMCI_SYSINFO_PARMS;
#endif

/* parameter block for MCI_SET command message */
typedef struct tagMCI_SET_PARMS {
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
} MCI_SET_PARMS, *PMCI_SET_PARMS, FAR *LPMCI_SET_PARMS;

/* parameter block for MCI_BREAK command message */
typedef struct tagMCI_BREAK_PARMS {
    DWORD   dwCallback;
#ifdef _WIN32
    int     nVirtKey;
    HWND    hwndBreak;
#else
    short   nVirtKey;
    WORD    wReserved0;             /* padding for Win 16 */
    HWND    hwndBreak;
    WORD    wReserved1;             /* padding for Win 16 */
#endif
} MCI_BREAK_PARMS, *PMCI_BREAK_PARMS, FAR * LPMCI_BREAK_PARMS;

/* parameter block for MCI_SAVE command message */
#ifdef _WIN32

typedef struct tagMCI_SAVE_PARMSA {
    DWORD    dwCallback;
    LPCSTR    lpfilename;
} MCI_SAVE_PARMSA, *PMCI_SAVE_PARMSA, * LPMCI_SAVE_PARMSA;
typedef struct tagMCI_SAVE_PARMSW {
    DWORD    dwCallback;
    LPCWSTR   lpfilename;
} MCI_SAVE_PARMSW, *PMCI_SAVE_PARMSW, * LPMCI_SAVE_PARMSW;
#ifdef UNICODE
typedef MCI_SAVE_PARMSW MCI_SAVE_PARMS;
typedef PMCI_SAVE_PARMSW PMCI_SAVE_PARMS;
typedef LPMCI_SAVE_PARMSW LPMCI_SAVE_PARMS;
#else
typedef MCI_SAVE_PARMSA MCI_SAVE_PARMS;
typedef PMCI_SAVE_PARMSA PMCI_SAVE_PARMS;
typedef LPMCI_SAVE_PARMSA LPMCI_SAVE_PARMS;
#endif // UNICODE

#else
typedef struct tagMCI_SAVE_PARMS {
    DWORD   dwCallback;
    LPCSTR  lpfilename;
} MCI_SAVE_PARMS, FAR * LPMCI_SAVE_PARMS;
#endif

/* parameter block for MCI_LOAD command message */
#ifdef _WIN32

typedef struct tagMCI_LOAD_PARMSA {
    DWORD    dwCallback;
    LPCSTR      lpfilename;
} MCI_LOAD_PARMSA, *PMCI_LOAD_PARMSA, * LPMCI_LOAD_PARMSA;
typedef struct tagMCI_LOAD_PARMSW {
    DWORD    dwCallback;
    LPCWSTR     lpfilename;
} MCI_LOAD_PARMSW, *PMCI_LOAD_PARMSW, * LPMCI_LOAD_PARMSW;
#ifdef UNICODE
typedef MCI_LOAD_PARMSW MCI_LOAD_PARMS;
typedef PMCI_LOAD_PARMSW PMCI_LOAD_PARMS;
typedef LPMCI_LOAD_PARMSW LPMCI_LOAD_PARMS;
#else
typedef MCI_LOAD_PARMSA MCI_LOAD_PARMS;
typedef PMCI_LOAD_PARMSA PMCI_LOAD_PARMS;
typedef LPMCI_LOAD_PARMSA LPMCI_LOAD_PARMS;
#endif // UNICODE

#else
typedef struct tagMCI_LOAD_PARMS {
    DWORD   dwCallback;
    LPCSTR  lpfilename;
} MCI_LOAD_PARMS, FAR * LPMCI_LOAD_PARMS;
#endif

/* parameter block for MCI_RECORD command message */
typedef struct tagMCI_RECORD_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
} MCI_RECORD_PARMS, FAR *LPMCI_RECORD_PARMS;

/* MCI extensions for videodisc devices */

/* flag for dwReturn field of MCI_STATUS_PARMS */
/* MCI_STATUS command, (dwItem == MCI_STATUS_MODE) */
#define MCI_VD_MODE_PARK                (MCI_VD_OFFSET + 1)

/* flag for dwReturn field of MCI_STATUS_PARMS */
/* MCI_STATUS command, (dwItem == MCI_VD_STATUS_MEDIA_TYPE) */
#define MCI_VD_MEDIA_CLV                (MCI_VD_OFFSET + 2)
#define MCI_VD_MEDIA_CAV                (MCI_VD_OFFSET + 3)
#define MCI_VD_MEDIA_OTHER              (MCI_VD_OFFSET + 4)

#define MCI_VD_FORMAT_TRACK             0x4001

/* flags for dwFlags parameter of MCI_PLAY command message */
#define MCI_VD_PLAY_REVERSE             0x00010000L
#define MCI_VD_PLAY_FAST                0x00020000L
#define MCI_VD_PLAY_SPEED               0x00040000L
#define MCI_VD_PLAY_SCAN                0x00080000L
#define MCI_VD_PLAY_SLOW                0x00100000L

/* flag for dwFlags parameter of MCI_SEEK command message */
#define MCI_VD_SEEK_REVERSE             0x00010000L

/* flags for dwItem field of MCI_STATUS_PARMS parameter block */
#define MCI_VD_STATUS_SPEED             0x00004002L
#define MCI_VD_STATUS_FORWARD           0x00004003L
#define MCI_VD_STATUS_MEDIA_TYPE        0x00004004L
#define MCI_VD_STATUS_SIDE              0x00004005L
#define MCI_VD_STATUS_DISC_SIZE         0x00004006L

/* flags for dwFlags parameter of MCI_GETDEVCAPS command message */
#define MCI_VD_GETDEVCAPS_CLV           0x00010000L
#define MCI_VD_GETDEVCAPS_CAV           0x00020000L

#define MCI_VD_SPIN_UP                  0x00010000L
#define MCI_VD_SPIN_DOWN                0x00020000L

/* flags for dwItem field of MCI_GETDEVCAPS_PARMS parameter block */
#define MCI_VD_GETDEVCAPS_CAN_REVERSE   0x00004002L
#define MCI_VD_GETDEVCAPS_FAST_RATE     0x00004003L
#define MCI_VD_GETDEVCAPS_SLOW_RATE     0x00004004L
#define MCI_VD_GETDEVCAPS_NORMAL_RATE   0x00004005L

/* flags for the dwFlags parameter of MCI_STEP command message */
#define MCI_VD_STEP_FRAMES              0x00010000L
#define MCI_VD_STEP_REVERSE             0x00020000L

/* flag for the MCI_ESCAPE command message */
#define MCI_VD_ESCAPE_STRING            0x00000100L

/* parameter block for MCI_PLAY command message */
typedef struct tagMCI_VD_PLAY_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
    DWORD   dwSpeed;
} MCI_VD_PLAY_PARMS, *PMCI_VD_PLAY_PARMS, FAR *LPMCI_VD_PLAY_PARMS;

/* parameter block for MCI_STEP command message */
typedef struct tagMCI_VD_STEP_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrames;
} MCI_VD_STEP_PARMS, *PMCI_VD_STEP_PARMS, FAR *LPMCI_VD_STEP_PARMS;

/* parameter block for MCI_ESCAPE command message */
#ifdef _WIN32

typedef struct tagMCI_VD_ESCAPE_PARMSA {
    DWORD   dwCallback;
    LPCSTR      lpstrCommand;
} MCI_VD_ESCAPE_PARMSA, *PMCI_VD_ESCAPE_PARMSA, *LPMCI_VD_ESCAPE_PARMSA;
typedef struct tagMCI_VD_ESCAPE_PARMSW {
    DWORD   dwCallback;
    LPCWSTR     lpstrCommand;
} MCI_VD_ESCAPE_PARMSW, *PMCI_VD_ESCAPE_PARMSW, *LPMCI_VD_ESCAPE_PARMSW;
#ifdef UNICODE
typedef MCI_VD_ESCAPE_PARMSW MCI_VD_ESCAPE_PARMS;
typedef PMCI_VD_ESCAPE_PARMSW PMCI_VD_ESCAPE_PARMS;
typedef LPMCI_VD_ESCAPE_PARMSW LPMCI_VD_ESCAPE_PARMS;
#else
typedef MCI_VD_ESCAPE_PARMSA MCI_VD_ESCAPE_PARMS;
typedef PMCI_VD_ESCAPE_PARMSA PMCI_VD_ESCAPE_PARMS;
typedef LPMCI_VD_ESCAPE_PARMSA LPMCI_VD_ESCAPE_PARMS;
#endif // UNICODE

#else
typedef struct tagMCI_VD_ESCAPE_PARMS {
    DWORD   dwCallback;
    LPCSTR  lpstrCommand;
} MCI_VD_ESCAPE_PARMS, FAR *LPMCI_VD_ESCAPE_PARMS;
#endif

/* MCI extensions for CD audio devices */

/* flags for the dwItem field of the MCI_STATUS_PARMS parameter block */
#define MCI_CDA_STATUS_TYPE_TRACK       0x00004001L

/* flags for the dwReturn field of MCI_STATUS_PARMS parameter block */
/* MCI_STATUS command, (dwItem == MCI_CDA_STATUS_TYPE_TRACK) */
#define MCI_CDA_TRACK_AUDIO             (MCI_CD_OFFSET + 0)
#define MCI_CDA_TRACK_OTHER             (MCI_CD_OFFSET + 1)

/* MCI extensions for waveform audio devices */

#define MCI_WAVE_PCM                    (MCI_WAVE_OFFSET + 0)
#define MCI_WAVE_MAPPER                 (MCI_WAVE_OFFSET + 1)

/* flags for the dwFlags parameter of MCI_OPEN command message */
#define MCI_WAVE_OPEN_BUFFER            0x00010000L

/* flags for the dwFlags parameter of MCI_SET command message */
#define MCI_WAVE_SET_FORMATTAG          0x00010000L
#define MCI_WAVE_SET_CHANNELS           0x00020000L
#define MCI_WAVE_SET_SAMPLESPERSEC      0x00040000L
#define MCI_WAVE_SET_AVGBYTESPERSEC     0x00080000L
#define MCI_WAVE_SET_BLOCKALIGN         0x00100000L
#define MCI_WAVE_SET_BITSPERSAMPLE      0x00200000L

/* flags for the dwFlags parameter of MCI_STATUS, MCI_SET command messages */
#define MCI_WAVE_INPUT                  0x00400000L
#define MCI_WAVE_OUTPUT                 0x00800000L

/* flags for the dwItem field of MCI_STATUS_PARMS parameter block */
#define MCI_WAVE_STATUS_FORMATTAG       0x00004001L
#define MCI_WAVE_STATUS_CHANNELS        0x00004002L
#define MCI_WAVE_STATUS_SAMPLESPERSEC   0x00004003L
#define MCI_WAVE_STATUS_AVGBYTESPERSEC  0x00004004L
#define MCI_WAVE_STATUS_BLOCKALIGN      0x00004005L
#define MCI_WAVE_STATUS_BITSPERSAMPLE   0x00004006L
#define MCI_WAVE_STATUS_LEVEL           0x00004007L

/* flags for the dwFlags parameter of MCI_SET command message */
#define MCI_WAVE_SET_ANYINPUT           0x04000000L
#define MCI_WAVE_SET_ANYOUTPUT          0x08000000L

/* flags for the dwFlags parameter of MCI_GETDEVCAPS command message */
#define MCI_WAVE_GETDEVCAPS_INPUTS      0x00004001L
#define MCI_WAVE_GETDEVCAPS_OUTPUTS     0x00004002L

/* parameter block for MCI_OPEN command message */
#ifdef _WIN32

typedef struct tagMCI_WAVE_OPEN_PARMSA {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    LPCSTR      lpstrDeviceType;
    LPCSTR      lpstrElementName;
    LPCSTR      lpstrAlias;
    DWORD   dwBufferSeconds;
} MCI_WAVE_OPEN_PARMSA, *PMCI_WAVE_OPEN_PARMSA, *LPMCI_WAVE_OPEN_PARMSA;
typedef struct tagMCI_WAVE_OPEN_PARMSW {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    LPCWSTR     lpstrDeviceType;
    LPCWSTR     lpstrElementName;
    LPCWSTR     lpstrAlias;
    DWORD   dwBufferSeconds;
} MCI_WAVE_OPEN_PARMSW, *PMCI_WAVE_OPEN_PARMSW, *LPMCI_WAVE_OPEN_PARMSW;
#ifdef UNICODE
typedef MCI_WAVE_OPEN_PARMSW MCI_WAVE_OPEN_PARMS;
typedef PMCI_WAVE_OPEN_PARMSW PMCI_WAVE_OPEN_PARMS;
typedef LPMCI_WAVE_OPEN_PARMSW LPMCI_WAVE_OPEN_PARMS;
#else
typedef MCI_WAVE_OPEN_PARMSA MCI_WAVE_OPEN_PARMS;
typedef PMCI_WAVE_OPEN_PARMSA PMCI_WAVE_OPEN_PARMS;
typedef LPMCI_WAVE_OPEN_PARMSA LPMCI_WAVE_OPEN_PARMS;
#endif // UNICODE

#else
typedef struct tagMCI_WAVE_OPEN_PARMS {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    WORD        wReserved0;
    LPCSTR      lpstrDeviceType;
    LPCSTR      lpstrElementName;
    LPCSTR      lpstrAlias;
    DWORD       dwBufferSeconds;
} MCI_WAVE_OPEN_PARMS, FAR *LPMCI_WAVE_OPEN_PARMS;
#endif

/* parameter block for MCI_DELETE command message */
typedef struct tagMCI_WAVE_DELETE_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
} MCI_WAVE_DELETE_PARMS, *PMCI_WAVE_DELETE_PARMS, FAR *LPMCI_WAVE_DELETE_PARMS;

/* parameter block for MCI_SET command message */
typedef struct tagMCI_WAVE_SET_PARMS {
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
#ifdef _WIN32
    UINT    wInput;
    UINT    wOutput;
#else
    WORD    wInput;
    WORD    wReserved0;
    WORD    wOutput;
    WORD    wReserved1;
#endif
    WORD    wFormatTag;
    WORD    wReserved2;
    WORD    nChannels;
    WORD    wReserved3;
    DWORD   nSamplesPerSec;
    DWORD   nAvgBytesPerSec;
    WORD    nBlockAlign;
    WORD    wReserved4;
    WORD    wBitsPerSample;
    WORD    wReserved5;
} MCI_WAVE_SET_PARMS, *PMCI_WAVE_SET_PARMS, FAR * LPMCI_WAVE_SET_PARMS;

/* MCI extensions for MIDI sequencer devices */

/* flags for the dwReturn field of MCI_STATUS_PARMS parameter block */
/* MCI_STATUS command, (dwItem == MCI_SEQ_STATUS_DIVTYPE) */
#define     MCI_SEQ_DIV_PPQN            (0 + MCI_SEQ_OFFSET)
#define     MCI_SEQ_DIV_SMPTE_24        (1 + MCI_SEQ_OFFSET)
#define     MCI_SEQ_DIV_SMPTE_25        (2 + MCI_SEQ_OFFSET)
#define     MCI_SEQ_DIV_SMPTE_30DROP    (3 + MCI_SEQ_OFFSET)
#define     MCI_SEQ_DIV_SMPTE_30        (4 + MCI_SEQ_OFFSET)

/* flags for the dwMaster field of MCI_SEQ_SET_PARMS parameter block */
/* MCI_SET command, (dwFlags == MCI_SEQ_SET_MASTER) */
#define     MCI_SEQ_FORMAT_SONGPTR      0x4001
#define     MCI_SEQ_FILE                0x4002
#define     MCI_SEQ_MIDI                0x4003
#define     MCI_SEQ_SMPTE               0x4004
#define     MCI_SEQ_NONE                65533
#define     MCI_SEQ_MAPPER              65535

/* flags for the dwItem field of MCI_STATUS_PARMS parameter block */
#define MCI_SEQ_STATUS_TEMPO            0x00004002L
#define MCI_SEQ_STATUS_PORT             0x00004003L
#define MCI_SEQ_STATUS_SLAVE            0x00004007L
#define MCI_SEQ_STATUS_MASTER           0x00004008L
#define MCI_SEQ_STATUS_OFFSET           0x00004009L
#define MCI_SEQ_STATUS_DIVTYPE          0x0000400AL
#define MCI_SEQ_STATUS_NAME             0x0000400BL
#define MCI_SEQ_STATUS_COPYRIGHT        0x0000400CL

/* flags for the dwFlags parameter of MCI_SET command message */
#define MCI_SEQ_SET_TEMPO               0x00010000L
#define MCI_SEQ_SET_PORT                0x00020000L
#define MCI_SEQ_SET_SLAVE               0x00040000L
#define MCI_SEQ_SET_MASTER              0x00080000L
#define MCI_SEQ_SET_OFFSET              0x01000000L

/* parameter block for MCI_SET command message */
typedef struct tagMCI_SEQ_SET_PARMS {
    DWORD   dwCallback;
    DWORD   dwTimeFormat;
    DWORD   dwAudio;
    DWORD   dwTempo;
    DWORD   dwPort;
    DWORD   dwSlave;
    DWORD   dwMaster;
    DWORD   dwOffset;
} MCI_SEQ_SET_PARMS, *PMCI_SEQ_SET_PARMS, FAR * LPMCI_SEQ_SET_PARMS;

/* MCI extensions for animation devices */

/* flags for dwFlags parameter of MCI_OPEN command message */
#define MCI_ANIM_OPEN_WS                0x00010000L
#define MCI_ANIM_OPEN_PARENT            0x00020000L
#define MCI_ANIM_OPEN_NOSTATIC          0x00040000L

/* flags for dwFlags parameter of MCI_PLAY command message */
#define MCI_ANIM_PLAY_SPEED             0x00010000L
#define MCI_ANIM_PLAY_REVERSE           0x00020000L
#define MCI_ANIM_PLAY_FAST              0x00040000L
#define MCI_ANIM_PLAY_SLOW              0x00080000L
#define MCI_ANIM_PLAY_SCAN              0x00100000L

/* flags for dwFlags parameter of MCI_STEP command message */
#define MCI_ANIM_STEP_REVERSE           0x00010000L
#define MCI_ANIM_STEP_FRAMES            0x00020000L

/* flags for dwItem field of MCI_STATUS_PARMS parameter block */
#define MCI_ANIM_STATUS_SPEED           0x00004001L
#define MCI_ANIM_STATUS_FORWARD         0x00004002L
#define MCI_ANIM_STATUS_HWND            0x00004003L
#define MCI_ANIM_STATUS_HPAL            0x00004004L
#define MCI_ANIM_STATUS_STRETCH         0x00004005L

/* flags for the dwFlags parameter of MCI_INFO command message */
#define MCI_ANIM_INFO_TEXT              0x00010000L

/* flags for dwItem field of MCI_GETDEVCAPS_PARMS parameter block */
#define MCI_ANIM_GETDEVCAPS_CAN_REVERSE 0x00004001L
#define MCI_ANIM_GETDEVCAPS_FAST_RATE   0x00004002L
#define MCI_ANIM_GETDEVCAPS_SLOW_RATE   0x00004003L
#define MCI_ANIM_GETDEVCAPS_NORMAL_RATE 0x00004004L
#define MCI_ANIM_GETDEVCAPS_PALETTES    0x00004006L
#define MCI_ANIM_GETDEVCAPS_CAN_STRETCH 0x00004007L
#define MCI_ANIM_GETDEVCAPS_MAX_WINDOWS 0x00004008L

/* flags for the MCI_REALIZE command message */
#define MCI_ANIM_REALIZE_NORM           0x00010000L
#define MCI_ANIM_REALIZE_BKGD           0x00020000L

/* flags for dwFlags parameter of MCI_WINDOW command message */
#define MCI_ANIM_WINDOW_HWND            0x00010000L
#define MCI_ANIM_WINDOW_STATE           0x00040000L
#define MCI_ANIM_WINDOW_TEXT            0x00080000L
#define MCI_ANIM_WINDOW_ENABLE_STRETCH  0x00100000L
#define MCI_ANIM_WINDOW_DISABLE_STRETCH 0x00200000L

/* flags for hWnd field of MCI_ANIM_WINDOW_PARMS parameter block */
/* MCI_WINDOW command message, (dwFlags == MCI_ANIM_WINDOW_HWND) */
#define MCI_ANIM_WINDOW_DEFAULT         0x00000000L

/* flags for dwFlags parameter of MCI_PUT command message */
#define MCI_ANIM_RECT                   0x00010000L
#define MCI_ANIM_PUT_SOURCE             0x00020000L
#define MCI_ANIM_PUT_DESTINATION        0x00040000L

/* flags for dwFlags parameter of MCI_WHERE command message */
#define MCI_ANIM_WHERE_SOURCE           0x00020000L
#define MCI_ANIM_WHERE_DESTINATION      0x00040000L

/* flags for dwFlags parameter of MCI_UPDATE command message */
#define MCI_ANIM_UPDATE_HDC             0x00020000L

/* parameter block for MCI_OPEN command message */
#ifdef _WIN32

typedef struct tagMCI_ANIM_OPEN_PARMSA {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    LPCSTR      lpstrDeviceType;
    LPCSTR      lpstrElementName;
    LPCSTR      lpstrAlias;
    DWORD   dwStyle;
    HWND    hWndParent;
} MCI_ANIM_OPEN_PARMSA, *PMCI_ANIM_OPEN_PARMSA, *LPMCI_ANIM_OPEN_PARMSA;
typedef struct tagMCI_ANIM_OPEN_PARMSW {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    LPCWSTR     lpstrDeviceType;
    LPCWSTR     lpstrElementName;
    LPCWSTR     lpstrAlias;
    DWORD   dwStyle;
    HWND    hWndParent;
} MCI_ANIM_OPEN_PARMSW, *PMCI_ANIM_OPEN_PARMSW, *LPMCI_ANIM_OPEN_PARMSW;
#ifdef UNICODE
typedef MCI_ANIM_OPEN_PARMSW MCI_ANIM_OPEN_PARMS;
typedef PMCI_ANIM_OPEN_PARMSW PMCI_ANIM_OPEN_PARMS;
typedef LPMCI_ANIM_OPEN_PARMSW LPMCI_ANIM_OPEN_PARMS;
#else
typedef MCI_ANIM_OPEN_PARMSA MCI_ANIM_OPEN_PARMS;
typedef PMCI_ANIM_OPEN_PARMSA PMCI_ANIM_OPEN_PARMS;
typedef LPMCI_ANIM_OPEN_PARMSA LPMCI_ANIM_OPEN_PARMS;
#endif // UNICODE

#else
typedef struct tagMCI_ANIM_OPEN_PARMS {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    WORD        wReserved0;
    LPCSTR      lpstrDeviceType;
    LPCSTR      lpstrElementName;
    LPCSTR      lpstrAlias;
    DWORD       dwStyle;
    HWND        hWndParent;
    WORD        wReserved1;
} MCI_ANIM_OPEN_PARMS, FAR *LPMCI_ANIM_OPEN_PARMS;
#endif

/* parameter block for MCI_PLAY command message */
typedef struct tagMCI_ANIM_PLAY_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrom;
    DWORD   dwTo;
    DWORD   dwSpeed;
} MCI_ANIM_PLAY_PARMS, *PMCI_ANIM_PLAY_PARMS, FAR *LPMCI_ANIM_PLAY_PARMS;

/* parameter block for MCI_STEP command message */
typedef struct tagMCI_ANIM_STEP_PARMS {
    DWORD   dwCallback;
    DWORD   dwFrames;
} MCI_ANIM_STEP_PARMS, *PMCI_ANIM_STEP_PARMS, FAR *LPMCI_ANIM_STEP_PARMS;

/* parameter block for MCI_WINDOW command message */
#ifdef _WIN32

typedef struct tagMCI_ANIM_WINDOW_PARMSA {
    DWORD   dwCallback;
    HWND    hWnd;
    UINT    nCmdShow;
    LPCSTR     lpstrText;
} MCI_ANIM_WINDOW_PARMSA, *PMCI_ANIM_WINDOW_PARMSA, * LPMCI_ANIM_WINDOW_PARMSA;
typedef struct tagMCI_ANIM_WINDOW_PARMSW {
    DWORD   dwCallback;
    HWND    hWnd;
    UINT    nCmdShow;
    LPCWSTR    lpstrText;
} MCI_ANIM_WINDOW_PARMSW, *PMCI_ANIM_WINDOW_PARMSW, * LPMCI_ANIM_WINDOW_PARMSW;
#ifdef UNICODE
typedef MCI_ANIM_WINDOW_PARMSW MCI_ANIM_WINDOW_PARMS;
typedef PMCI_ANIM_WINDOW_PARMSW PMCI_ANIM_WINDOW_PARMS;
typedef LPMCI_ANIM_WINDOW_PARMSW LPMCI_ANIM_WINDOW_PARMS;
#else
typedef MCI_ANIM_WINDOW_PARMSA MCI_ANIM_WINDOW_PARMS;
typedef PMCI_ANIM_WINDOW_PARMSA PMCI_ANIM_WINDOW_PARMS;
typedef LPMCI_ANIM_WINDOW_PARMSA LPMCI_ANIM_WINDOW_PARMS;
#endif // UNICODE

#else
typedef struct tagMCI_ANIM_WINDOW_PARMS {
    DWORD   dwCallback;
    HWND    hWnd;
    WORD    wReserved1;
    WORD    nCmdShow;
    WORD    wReserved2;
    LPCSTR  lpstrText;
} MCI_ANIM_WINDOW_PARMS, FAR * LPMCI_ANIM_WINDOW_PARMS;
#endif

/* parameter block for MCI_PUT, MCI_UPDATE, MCI_WHERE command messages */
typedef struct tagMCI_ANIM_RECT_PARMS {
    DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else   /* ifdef MCI_USE_OFFEXT */
    RECT    rc;
#endif  /* ifdef MCI_USE_OFFEXT */
} MCI_ANIM_RECT_PARMS;
typedef MCI_ANIM_RECT_PARMS * PMCI_ANIM_RECT_PARMS;
typedef MCI_ANIM_RECT_PARMS FAR * LPMCI_ANIM_RECT_PARMS;

/* parameter block for MCI_UPDATE PARMS */
typedef struct tagMCI_ANIM_UPDATE_PARMS {
    DWORD   dwCallback;
    RECT    rc;
    HDC     hDC;
} MCI_ANIM_UPDATE_PARMS, *PMCI_ANIM_UPDATE_PARMS, FAR * LPMCI_ANIM_UPDATE_PARMS;

/* MCI extensions for video overlay devices */

/* flags for dwFlags parameter of MCI_OPEN command message */
#define MCI_OVLY_OPEN_WS                0x00010000L
#define MCI_OVLY_OPEN_PARENT            0x00020000L

/* flags for dwFlags parameter of MCI_STATUS command message */
#define MCI_OVLY_STATUS_HWND            0x00004001L
#define MCI_OVLY_STATUS_STRETCH         0x00004002L

/* flags for dwFlags parameter of MCI_INFO command message */
#define MCI_OVLY_INFO_TEXT              0x00010000L

/* flags for dwItem field of MCI_GETDEVCAPS_PARMS parameter block */
#define MCI_OVLY_GETDEVCAPS_CAN_STRETCH 0x00004001L
#define MCI_OVLY_GETDEVCAPS_CAN_FREEZE  0x00004002L
#define MCI_OVLY_GETDEVCAPS_MAX_WINDOWS 0x00004003L

/* flags for dwFlags parameter of MCI_WINDOW command message */
#define MCI_OVLY_WINDOW_HWND            0x00010000L
#define MCI_OVLY_WINDOW_STATE           0x00040000L
#define MCI_OVLY_WINDOW_TEXT            0x00080000L
#define MCI_OVLY_WINDOW_ENABLE_STRETCH  0x00100000L
#define MCI_OVLY_WINDOW_DISABLE_STRETCH 0x00200000L

/* flags for hWnd parameter of MCI_OVLY_WINDOW_PARMS parameter block */
#define MCI_OVLY_WINDOW_DEFAULT         0x00000000L

/* flags for dwFlags parameter of MCI_PUT command message */
#define MCI_OVLY_RECT                   0x00010000L
#define MCI_OVLY_PUT_SOURCE             0x00020000L
#define MCI_OVLY_PUT_DESTINATION        0x00040000L
#define MCI_OVLY_PUT_FRAME              0x00080000L
#define MCI_OVLY_PUT_VIDEO              0x00100000L

/* flags for dwFlags parameter of MCI_WHERE command message */
#define MCI_OVLY_WHERE_SOURCE           0x00020000L
#define MCI_OVLY_WHERE_DESTINATION      0x00040000L
#define MCI_OVLY_WHERE_FRAME            0x00080000L
#define MCI_OVLY_WHERE_VIDEO            0x00100000L

/* parameter block for MCI_OPEN command message */
#ifdef _WIN32

typedef struct tagMCI_OVLY_OPEN_PARMSA {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    LPCSTR      lpstrDeviceType;
    LPCSTR      lpstrElementName;
    LPCSTR      lpstrAlias;
    DWORD   dwStyle;
    HWND    hWndParent;
} MCI_OVLY_OPEN_PARMSA, *PMCI_OVLY_OPEN_PARMSA, *LPMCI_OVLY_OPEN_PARMSA;
typedef struct tagMCI_OVLY_OPEN_PARMSW {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    LPCWSTR     lpstrDeviceType;
    LPCWSTR     lpstrElementName;
    LPCWSTR     lpstrAlias;
    DWORD   dwStyle;
    HWND    hWndParent;
} MCI_OVLY_OPEN_PARMSW, *PMCI_OVLY_OPEN_PARMSW, *LPMCI_OVLY_OPEN_PARMSW;
#ifdef UNICODE
typedef MCI_OVLY_OPEN_PARMSW MCI_OVLY_OPEN_PARMS;
typedef PMCI_OVLY_OPEN_PARMSW PMCI_OVLY_OPEN_PARMS;
typedef LPMCI_OVLY_OPEN_PARMSW LPMCI_OVLY_OPEN_PARMS;
#else
typedef MCI_OVLY_OPEN_PARMSA MCI_OVLY_OPEN_PARMS;
typedef PMCI_OVLY_OPEN_PARMSA PMCI_OVLY_OPEN_PARMS;
typedef LPMCI_OVLY_OPEN_PARMSA LPMCI_OVLY_OPEN_PARMS;
#endif // UNICODE

#else
typedef struct tagMCI_OVLY_OPEN_PARMS {
    DWORD   dwCallback;
    MCIDEVICEID wDeviceID;
    WORD        wReserved0;
    LPCSTR      lpstrDeviceType;
    LPCSTR      lpstrElementName;
    LPCSTR      lpstrAlias;
    DWORD       dwStyle;
    HWND        hWndParent;
    WORD        wReserved1;
} MCI_OVLY_OPEN_PARMS, FAR *LPMCI_OVLY_OPEN_PARMS;
#endif

/* parameter block for MCI_WINDOW command message */
#ifdef _WIN32

typedef struct tagMCI_OVLY_WINDOW_PARMSA {
    DWORD   dwCallback;
    HWND    hWnd;
    UINT    nCmdShow;
    LPCSTR      lpstrText;
} MCI_OVLY_WINDOW_PARMSA, *PMCI_OVLY_WINDOW_PARMSA, * LPMCI_OVLY_WINDOW_PARMSA;
typedef struct tagMCI_OVLY_WINDOW_PARMSW {
    DWORD   dwCallback;
    HWND    hWnd;
    UINT    nCmdShow;
    LPCWSTR     lpstrText;
} MCI_OVLY_WINDOW_PARMSW, *PMCI_OVLY_WINDOW_PARMSW, * LPMCI_OVLY_WINDOW_PARMSW;
#ifdef UNICODE
typedef MCI_OVLY_WINDOW_PARMSW MCI_OVLY_WINDOW_PARMS;
typedef PMCI_OVLY_WINDOW_PARMSW PMCI_OVLY_WINDOW_PARMS;
typedef LPMCI_OVLY_WINDOW_PARMSW LPMCI_OVLY_WINDOW_PARMS;
#else
typedef MCI_OVLY_WINDOW_PARMSA MCI_OVLY_WINDOW_PARMS;
typedef PMCI_OVLY_WINDOW_PARMSA PMCI_OVLY_WINDOW_PARMS;
typedef LPMCI_OVLY_WINDOW_PARMSA LPMCI_OVLY_WINDOW_PARMS;
#endif // UNICODE
#else
typedef struct tagMCI_OVLY_WINDOW_PARMS {
    DWORD   dwCallback;
    HWND    hWnd;
    WORD    wReserved1;
    UINT    nCmdShow;
    WORD    wReserved2;
    LPCSTR  lpstrText;
} MCI_OVLY_WINDOW_PARMS, FAR * LPMCI_OVLY_WINDOW_PARMS;
#endif

/* parameter block for MCI_PUT, MCI_UPDATE, and MCI_WHERE command messages */
typedef struct tagMCI_OVLY_RECT_PARMS {
    DWORD   dwCallback;
#ifdef MCI_USE_OFFEXT
    POINT   ptOffset;
    POINT   ptExtent;
#else   /* ifdef MCI_USE_OFFEXT */
    RECT    rc;
#endif  /* ifdef MCI_USE_OFFEXT */
} MCI_OVLY_RECT_PARMS, *PMCI_OVLY_RECT_PARMS, FAR * LPMCI_OVLY_RECT_PARMS;

/* parameter block for MCI_SAVE command message */
#ifdef _WIN32

typedef struct tagMCI_OVLY_SAVE_PARMSA {
    DWORD   dwCallback;
    LPCSTR      lpfilename;
    RECT    rc;
} MCI_OVLY_SAVE_PARMSA, *PMCI_OVLY_SAVE_PARMSA, * LPMCI_OVLY_SAVE_PARMSA;
typedef struct tagMCI_OVLY_SAVE_PARMSW {
    DWORD   dwCallback;
    LPCWSTR     lpfilename;
    RECT    rc;
} MCI_OVLY_SAVE_PARMSW, *PMCI_OVLY_SAVE_PARMSW, * LPMCI_OVLY_SAVE_PARMSW;
#ifdef UNICODE
typedef MCI_OVLY_SAVE_PARMSW MCI_OVLY_SAVE_PARMS;
typedef PMCI_OVLY_SAVE_PARMSW PMCI_OVLY_SAVE_PARMS;
typedef LPMCI_OVLY_SAVE_PARMSW LPMCI_OVLY_SAVE_PARMS;
#else
typedef MCI_OVLY_SAVE_PARMSA MCI_OVLY_SAVE_PARMS;
typedef PMCI_OVLY_SAVE_PARMSA PMCI_OVLY_SAVE_PARMS;
typedef LPMCI_OVLY_SAVE_PARMSA LPMCI_OVLY_SAVE_PARMS;
#endif // UNICODE
#else
typedef struct tagMCI_OVLY_SAVE_PARMS {
    DWORD   dwCallback;
    LPCSTR  lpfilename;
    RECT    rc;
} MCI_OVLY_SAVE_PARMS, FAR * LPMCI_OVLY_SAVE_PARMS;
#endif

/* parameter block for MCI_LOAD command message */
#ifdef _WIN32

typedef struct tagMCI_OVLY_LOAD_PARMSA {
    DWORD   dwCallback;
    LPCSTR      lpfilename;
    RECT    rc;
} MCI_OVLY_LOAD_PARMSA, *PMCI_OVLY_LOAD_PARMSA, * LPMCI_OVLY_LOAD_PARMSA;
typedef struct tagMCI_OVLY_LOAD_PARMSW {
    DWORD   dwCallback;
    LPCWSTR     lpfilename;
    RECT    rc;
} MCI_OVLY_LOAD_PARMSW, *PMCI_OVLY_LOAD_PARMSW, * LPMCI_OVLY_LOAD_PARMSW;
#ifdef UNICODE
typedef MCI_OVLY_LOAD_PARMSW MCI_OVLY_LOAD_PARMS;
typedef PMCI_OVLY_LOAD_PARMSW PMCI_OVLY_LOAD_PARMS;
typedef LPMCI_OVLY_LOAD_PARMSW LPMCI_OVLY_LOAD_PARMS;
#else
typedef MCI_OVLY_LOAD_PARMSA MCI_OVLY_LOAD_PARMS;
typedef PMCI_OVLY_LOAD_PARMSA PMCI_OVLY_LOAD_PARMS;
typedef LPMCI_OVLY_LOAD_PARMSA LPMCI_OVLY_LOAD_PARMS;
#endif // UNICODE
#else
typedef struct tagMCI_OVLY_LOAD_PARMS {
    DWORD   dwCallback;
    LPCSTR  lpfilename;
    RECT    rc;
} MCI_OVLY_LOAD_PARMS, FAR * LPMCI_OVLY_LOAD_PARMS;
#endif

#endif  /* ifndef _WIN32_VXD */
#endif  /* ifndef MMNOMCI */

/****************************************************************************

			DISPLAY Driver extensions

****************************************************************************/

#ifndef NEWTRANSPARENT
    #define NEWTRANSPARENT  3           /* use with SetBkMode() */

    #define QUERYROPSUPPORT 40          /* use to determine ROP support */
#endif  /* ifndef NEWTRANSPARENT */

/****************************************************************************

			DIB Driver extensions

****************************************************************************/

#define SELECTDIB       41                      /* DIB.DRV select dib escape */
#define DIBINDEX(n)     MAKELONG((n),0x10FF)

/****************************************************************************

			ScreenSaver support

    The current application will receive a syscommand of SC_SCREENSAVE just
    before the screen saver is invoked.  If the app wishes to prevent a
    screen save, return non-zero value, otherwise call DefWindowProc().

****************************************************************************/

#ifndef SC_SCREENSAVE

    #define SC_SCREENSAVE   0xF140

#endif  /* ifndef SC_SCREENSAVE */

#ifdef __cplusplus
}                       /* End of extern "C" { */
#endif  /* __cplusplus */

#ifdef _WIN32
#include <poppack.h>
#else
#ifndef RC_INVOKED
#pragma pack()
#endif
#endif

#endif  /* _INC_MMSYSTEM */
