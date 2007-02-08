/* -*- tab-width: 8; c-basic-offset: 4 -*- */

/*****************************************************************************
 * Copyright 1998, Luiz Otavio L. Zorzella
 *           1999, Eric Pouech
 *
 * Purpose:   multimedia declarations (internal to WINMM & MMSYSTEM DLLs)
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
 *****************************************************************************
 */

#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "mmddk.h"

#define WINE_DEFAULT_WINMM_DRIVER     "oss"
#define WINE_DEFAULT_WINMM_MAPPER     "msacm.drv"
#define WINE_DEFAULT_WINMM_MIDI       "midimap.dll"

typedef DWORD (WINAPI *MessageProc16)(UINT16 wDevID, UINT16 wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);
typedef DWORD (WINAPI *MessageProc32)(UINT wDevID, UINT wMsg, DWORD dwUser, DWORD dwParam1, DWORD dwParam2);

typedef enum {
    WINMM_MAP_NOMEM, 	/* ko, memory problem */
    WINMM_MAP_MSGERROR, /* ko, unknown message */
    WINMM_MAP_OK, 	/* ok, no memory allocated. to be sent to the proc. */
    WINMM_MAP_OKMEM, 	/* ok, some memory allocated, need to call UnMapMsg. to be sent to the proc. */
} WINMM_MapType;

/* Who said goofy boy ? */
#define	WINE_DI_MAGIC	0x900F1B01

typedef struct tagWINE_DRIVER
{
    DWORD			dwMagic;
    /* as usual LPWINE_DRIVER == hDriver32 */
    DWORD			dwFlags;
    union {
	struct {
	    HMODULE			hModule;
	    DRIVERPROC			lpDrvProc;
	    DWORD		  	dwDriverID;
	} d32;
	struct {
	    UINT16			hDriver16;
	} d16;
    } d;
    struct tagWINE_DRIVER*	lpPrevItem;
    struct tagWINE_DRIVER*	lpNextItem;
} WINE_DRIVER, *LPWINE_DRIVER;

typedef	DWORD	(CALLBACK *WINEMM_msgFunc16)(UINT16, WORD, DWORD, DWORD, DWORD);
typedef	DWORD	(CALLBACK *WINEMM_msgFunc32)(UINT  , UINT, DWORD, DWORD, DWORD);

/* for each loaded driver and each known type of driver, this structure contains
 * the information needed to access it
 */
typedef struct tagWINE_MM_DRIVER_PART {
    int				nIDMin;		/* lower bound of global indexes for this type */
    int				nIDMax;		/* hhigher bound of global indexes for this type */
    union {
	WINEMM_msgFunc32	fnMessage32;	/* pointer to function */
	WINEMM_msgFunc16	fnMessage16;
    } u;
} WINE_MM_DRIVER_PART;

#define	MMDRV_AUX		0
#define	MMDRV_MIXER		1
#define	MMDRV_MIDIIN		2
#define	MMDRV_MIDIOUT		3
#define	MMDRV_WAVEIN		4
#define	MMDRV_WAVEOUT		5
#define MMDRV_MAX		6

/* each low-level .drv will be associated with an instance of this structure */
typedef struct tagWINE_MM_DRIVER {
    HDRVR			hDriver;
    LPSTR			drvname;	/* name of the driver */
    unsigned			bIs32 : 1,	/* TRUE if 32 bit driver, FALSE for 16 */
	                        bIsMapper : 1;	/* TRUE if mapper */
    WINE_MM_DRIVER_PART		parts[MMDRV_MAX];/* Information for all known types */
} WINE_MM_DRIVER, *LPWINE_MM_DRIVER;

typedef struct tagWINE_MLD {
/* EPP struct tagWINE_MLD*	lpNext; */		/* not used so far */
       UINT			uDeviceID;
       UINT			type;
       UINT			mmdIndex;		/* index to low-level driver in MMDrvs table */
       DWORD			dwDriverInstance;	/* this value is driver related, as opposed to
							 * opendesc.dwInstance which is client (callback) related */
       WORD			bFrom32;
       WORD			dwFlags;
       DWORD			dwCallback;
       DWORD			dwClientInstance;
} WINE_MLD, *LPWINE_MLD;

typedef struct  {
       WINE_MLD			mld;
} WINE_WAVE, *LPWINE_WAVE;

typedef struct {
       WINE_MLD			mld;
       MIDIOPENDESC		mod;			/* FIXME: should be removed */
} WINE_MIDI, *LPWINE_MIDI;

typedef struct {
       WINE_MLD			mld;
} WINE_MIXER, *LPWINE_MIXER;

#define WINE_MMTHREAD_CREATED	0x4153494C	/* "BSIL" */
#define WINE_MMTHREAD_DELETED	0xDEADDEAD

typedef struct {
       DWORD			dwSignature;		/* 00 "BSIL" when ok, 0xDEADDEAD when being deleted */
       DWORD			dwCounter;		/* 04 > 1 when in mmThread functions */
       HANDLE			hThread;		/* 08 hThread */
       DWORD                    dwThreadID;     	/* 0C */
       DWORD    		fpThread;		/* 10 address of thread proc (segptr or lin depending on dwFlags) */
       DWORD			dwThreadPmt;    	/* 14 parameter to be passed upon thread creation to fpThread */
       DWORD                    dwSignalCount;	     	/* 18 counter used for signaling */
       HANDLE                   hEvent;     		/* 1C event */
       HANDLE                   hVxD;		     	/* 20 return from OpenVxDHandle */
       DWORD                    dwStatus;       	/* 24 0x00, 0x10, 0x20, 0x30 */
       DWORD			dwFlags;		/* 28 dwFlags upon creation */
       UINT16			hTask;          	/* 2C handle to created task */
} WINE_MMTHREAD;

typedef struct tagWINE_MCIDRIVER {
        UINT			wDeviceID;
        UINT			wType;
	LPWSTR			lpstrElementName;
        LPWSTR			lpstrDeviceType;
        LPWSTR			lpstrAlias;
        HDRVR			hDriver;
	DWORD			dwPrivate;
        YIELDPROC		lpfnYieldProc;
        DWORD	                dwYieldData;
        BOOL			bIs32;
        DWORD                   CreatorThread;
        UINT			uTypeCmdTable;
        UINT			uSpecificCmdTable;
        struct tagWINE_MCIDRIVER*lpNext;
} WINE_MCIDRIVER, *LPWINE_MCIDRIVER;

#define WINE_TIMER_IS32	0x80

typedef struct tagWINE_TIMERENTRY {
    UINT			wDelay;
    UINT			wResol;
    LPTIMECALLBACK              lpFunc; /* can be lots of things */
    DWORD			dwUser;
    UINT16			wFlags;
    UINT16			wTimerID;
    DWORD			dwTriggerTime;
    struct tagWINE_TIMERENTRY*	lpNext;
} WINE_TIMERENTRY, *LPWINE_TIMERENTRY;

enum mmioProcType {MMIO_PROC_16,MMIO_PROC_32A,MMIO_PROC_32W};

struct IOProcList
{
    struct IOProcList*pNext;       /* Next item in linked list */
    FOURCC            fourCC;      /* four-character code identifying IOProc */
    LPMMIOPROC	      pIOProc;     /* pointer to IProc */
    enum mmioProcType type;        /* 16, 32A or 32W */
    int		      count;	   /* number of objects linked to it */
};

typedef struct tagWINE_MMIO {
    MMIOINFO			info;
    struct tagWINE_MMIO*	lpNext;
    struct IOProcList*		ioProc;
    unsigned			bTmpIOProc : 1,
                                bBufferLoaded : 1;
    DWORD                       segBuffer16;
    DWORD                       dwFileSize;
} WINE_MMIO, *LPWINE_MMIO;

typedef struct tagWINE_PLAYSOUND {
    unsigned                    bLoop : 1,
                                bAlloc : 1;
    LPCWSTR		        pszSound;
    HMODULE		        hMod;
    DWORD		        fdwSound;
    HANDLE                      hThread;
    struct tagWINE_PLAYSOUND*   lpNext;
} WINE_PLAYSOUND, *LPWINE_PLAYSOUND;

typedef struct tagWINE_MM_IDATA {
    /* winmm part */
    HANDLE			hWinMM32Instance;
    HANDLE			hWinMM16Instance;
    CRITICAL_SECTION		cs;
    /* mci part */
    LPWINE_MCIDRIVER 		lpMciDrvs;
    /* low level drivers (unused yet) */
    /* LPWINE_WAVE		lpWave; */
    /* LPWINE_MIDI		lpMidi; */
    /* LPWINE_MIXER		lpMixer; */
    /* mmio part */
    LPWINE_MMIO			lpMMIO;
    /* playsound and sndPlaySound */
    WINE_PLAYSOUND*             lpPlaySound;
    HANDLE                      psLastEvent;
    HANDLE                      psStopEvent;
} WINE_MM_IDATA, *LPWINE_MM_IDATA;

/* function prototypes */

typedef LONG			(*MCIPROC)(DWORD, HDRVR, DWORD, DWORD, DWORD);
typedef	WINMM_MapType	        (*MMDRV_MAPFUNC)(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2);
typedef	WINMM_MapType	        (*MMDRV_UNMAPFUNC)(UINT wMsg, LPDWORD lpdwUser, LPDWORD lpParam1, LPDWORD lpParam2, MMRESULT ret);

HDRVR WINAPI	OpenDriverA(LPCSTR lpDriverName, LPCSTR lpSectionName, LPARAM lParam2);
LPWINE_DRIVER	DRIVER_FindFromHDrvr(HDRVR hDrvr);
BOOL		DRIVER_GetLibName(LPCWSTR keyName, LPCWSTR sectName, LPWSTR buf, int sz);
LPWINE_DRIVER	DRIVER_TryOpenDriver32(LPCWSTR fn, LPARAM lParam2);
void            DRIVER_UnloadAll(void);

BOOL		MMDRV_Init(void);
void            MMDRV_Exit(void);
UINT		MMDRV_GetNum(UINT);
LPWINE_MLD	MMDRV_Alloc(UINT size, UINT type, LPHANDLE hndl, DWORD* dwFlags,
                            DWORD* dwCallback, DWORD* dwInstance, BOOL bFrom32);
void		MMDRV_Free(HANDLE hndl, LPWINE_MLD mld);
DWORD		MMDRV_Open(LPWINE_MLD mld, UINT wMsg, DWORD dwParam1, DWORD dwParam2);
DWORD		MMDRV_Close(LPWINE_MLD mld, UINT wMsg);
LPWINE_MLD	MMDRV_Get(HANDLE hndl, UINT type, BOOL bCanBeID);
LPWINE_MLD	MMDRV_GetRelated(HANDLE hndl, UINT srcType, BOOL bSrcCanBeID, UINT dstTyped);
DWORD           MMDRV_Message(LPWINE_MLD mld, UINT wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2, BOOL bFrom32);
UINT		MMDRV_PhysicalFeatures(LPWINE_MLD mld, UINT uMsg, DWORD dwParam1, DWORD dwParam2);
BOOL            MMDRV_Is32(unsigned int);
void            MMDRV_InstallMap(unsigned int, MMDRV_MAPFUNC, MMDRV_UNMAPFUNC,
                                 MMDRV_MAPFUNC, MMDRV_UNMAPFUNC, LPDRVCALLBACK);

WINE_MCIDRIVER* MCI_GetDriver(UINT16 uDevID);
UINT		MCI_GetDriverFromString(LPCWSTR str);
DWORD		MCI_WriteString(LPWSTR lpDstStr, DWORD dstSize, LPCWSTR lpSrcStr);
const char* 	MCI_MessageToString(UINT wMsg);
UINT	WINAPI	MCI_DefYieldProc(MCIDEVICEID wDevID, DWORD data);
LRESULT		MCI_CleanUp(LRESULT dwRet, UINT wMsg, DWORD dwParam2);
DWORD		MCI_SendCommand(UINT wDevID, UINT16 wMsg, DWORD dwParam1, DWORD dwParam2, BOOL bFrom32);
DWORD		MCI_SendCommandFrom32(UINT wDevID, UINT16 wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
DWORD		MCI_SendCommandFrom16(UINT wDevID, UINT16 wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
UINT		MCI_SetCommandTable(void *table, UINT uDevType);
BOOL	        MCI_DeleteCommandTable(UINT uTbl, BOOL delete);
LPWSTR          MCI_strdupAtoW(LPCSTR str);
LPSTR           MCI_strdupWtoA(LPCWSTR str);

BOOL            WINMM_CheckForMMSystem(void);
const char*     WINMM_ErrorToString(MMRESULT error);

UINT            MIXER_Open(LPHMIXER lphMix, UINT uDeviceID, DWORD_PTR dwCallback,
                           DWORD_PTR dwInstance, DWORD fdwOpen, BOOL bFrom32);
UINT            MIDI_OutOpen(HMIDIOUT* lphMidiOut, UINT uDeviceID, DWORD_PTR dwCallback,
                             DWORD_PTR dwInstance, DWORD dwFlags, BOOL bFrom32);
UINT            MIDI_InOpen(HMIDIIN* lphMidiIn, UINT uDeviceID, DWORD_PTR dwCallback,
                            DWORD_PTR dwInstance, DWORD dwFlags, BOOL bFrom32);
MMRESULT        MIDI_StreamOpen(HMIDISTRM* lphMidiStrm, LPUINT lpuDeviceID,
                                DWORD cMidi, DWORD_PTR dwCallback,
                                DWORD_PTR dwInstance, DWORD fdwOpen, BOOL bFrom32);
UINT            WAVE_Open(HANDLE* lphndl, UINT uDeviceID, UINT uType,
                          LPCWAVEFORMATEX lpFormat, DWORD_PTR dwCallback, 
                          DWORD_PTR dwInstance, DWORD dwFlags, BOOL bFrom32);

HMMIO           MMIO_Open(LPSTR szFileName, MMIOINFO* refmminfo,
                          DWORD dwOpenFlags, enum mmioProcType type);
LPMMIOPROC      MMIO_InstallIOProc(FOURCC fccIOProc, LPMMIOPROC pIOProc,
                                   DWORD dwFlags, enum mmioProcType type);
LRESULT         MMIO_SendMessage(HMMIO hmmio, UINT uMessage, LPARAM lParam1, 
                                 LPARAM lParam2, enum mmioProcType type);
LPWINE_MMIO     MMIO_Get(HMMIO h);

WORD            TIME_SetEventInternal(UINT wDelay, UINT wResol, LPTIMECALLBACK lpFunc,
                                      DWORD dwUser, UINT wFlags);
void    	TIME_MMTimeStart(void);
void		TIME_MMTimeStop(void);

/* Global variables */
extern WINE_MM_IDATA  WINMM_IData;

/* pointers to 16 bit functions (if sibling MMSYSTEM.DLL is loaded
 * NULL otherwise
 */
extern  WINE_MMTHREAD*  (*pFnGetMMThread16)(UINT16);
extern  LPWINE_DRIVER   (*pFnOpenDriver16)(LPCWSTR,LPCWSTR,LPARAM);
extern  LRESULT         (*pFnCloseDriver16)(UINT16,LPARAM,LPARAM);
extern  LRESULT         (*pFnSendMessage16)(UINT16,UINT,LPARAM,LPARAM);
extern  WINMM_MapType   (*pFnMciMapMsg16To32W)(WORD,WORD,DWORD,DWORD*);
extern  WINMM_MapType   (*pFnMciUnMapMsg16To32W)(WORD,WORD,DWORD,DWORD);
extern  WINMM_MapType   (*pFnMciMapMsg32WTo16)(WORD,WORD,DWORD,DWORD*);
extern  WINMM_MapType   (*pFnMciUnMapMsg32WTo16)(WORD,WORD,DWORD,DWORD);
extern  LRESULT         (*pFnCallMMDrvFunc16)(DWORD /* in fact FARPROC16 */,WORD,WORD,LONG,LONG,LONG);
extern  unsigned        (*pFnLoadMMDrvFunc16)(LPCSTR,LPWINE_DRIVER, LPWINE_MM_DRIVER);
extern  LRESULT         (*pFnMmioCallback16)(DWORD,LPMMIOINFO,UINT,LPARAM,LPARAM);
extern  void            (WINAPI *pFnReleaseThunkLock)(DWORD*);
extern  void            (WINAPI *pFnRestoreThunkLock)(DWORD);
/* GetDriverFlags() returned bits is not documented (nor the call itself)
 * Here are Wine only definitions of the bits
 */
#define WINE_GDF_EXIST	0x80000000
#define WINE_GDF_16BIT	0x10000000
