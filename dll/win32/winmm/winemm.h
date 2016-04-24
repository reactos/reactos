/*
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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef _WINEMM_H_
#define _WINEMM_H_

#include <wine/config.h>

#include <assert.h>
#include <stdio.h>

#define _INC_WINDOWS
#define COM_NO_WINDOWS_H

#define NONAMELESSUNION
#define NONAMELESSSTRUCT

#include <windef.h>
#include <winbase.h>
#include <winuser.h>
#include <winreg.h>
#include <mmddk.h>

#include <wine/debug.h>
#include <wine/exception.h>
#include <wine/unicode.h>

#define WINE_DEFAULT_WINMM_DRIVER     "alsa,oss,coreaudio,esd"
#define WINE_DEFAULT_WINMM_MAPPER     "msacm32.drv"
#define WINE_DEFAULT_WINMM_MIDI       "midimap.dll"

/* Who said goofy boy ? */
#define	WINE_DI_MAGIC	0x900F1B01

typedef struct tagWINE_DRIVER
{
    DWORD			dwMagic;
    /* as usual LPWINE_DRIVER == hDriver32 */
    DWORD			dwFlags;
    HMODULE			hModule;
    DRIVERPROC			lpDrvProc;
    DWORD_PTR		  	dwDriverID;
    struct tagWINE_DRIVER*	lpPrevItem;
    struct tagWINE_DRIVER*	lpNextItem;
} WINE_DRIVER, *LPWINE_DRIVER;

typedef	DWORD	(CALLBACK *WINEMM_msgFunc32)(UINT, UINT, DWORD_PTR, DWORD_PTR, DWORD_PTR);

/* for each loaded driver and each known type of driver, this structure contains
 * the information needed to access it
 */
typedef struct tagWINE_MM_DRIVER_PART {
    int				nIDMin;		/* lower bound of global indexes for this type */
    int				nIDMax;		/* hhigher bound of global indexes for this type */
    WINEMM_msgFunc32	        fnMessage32;	/* pointer to function */
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
    unsigned			bIsMapper : 1;	/* TRUE if mapper */
    WINE_MM_DRIVER_PART		parts[MMDRV_MAX];/* Information for all known types */
} WINE_MM_DRIVER, *LPWINE_MM_DRIVER;

typedef struct tagWINE_MLD {
/* EPP struct tagWINE_MLD*	lpNext; */		/* not used so far */
       UINT			uDeviceID;
       UINT			type;
       UINT			mmdIndex;		/* index to low-level driver in MMDrvs table */
       DWORD_PTR		dwDriverInstance;	/* this value is driver related, as opposed to
							 * opendesc.dwInstance which is client (callback) related */
       WORD			dwFlags;
       DWORD_PTR		dwCallback;
       DWORD_PTR		dwClientInstance;
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

typedef struct tagWINE_MCIDRIVER {
        UINT			wDeviceID;
        UINT			wType;
	LPWSTR			lpstrElementName;
        LPWSTR			lpstrDeviceType;
        LPWSTR			lpstrAlias;
        HDRVR			hDriver;
        DWORD_PTR               dwPrivate;
        YIELDPROC		lpfnYieldProc;
        DWORD	                dwYieldData;
        DWORD                   CreatorThread;
        UINT			uTypeCmdTable;
        UINT			uSpecificCmdTable;
        struct tagWINE_MCIDRIVER*lpNext;
} WINE_MCIDRIVER, *LPWINE_MCIDRIVER;

struct IOProcList
{
    struct IOProcList*pNext;       /* Next item in linked list */
    FOURCC            fourCC;      /* four-character code identifying IOProc */
    LPMMIOPROC	      pIOProc;     /* pointer to IProc */
    BOOL              is_unicode;  /* 32A or 32W */
    int		      count;	   /* number of objects linked to it */
};

typedef struct tagWINE_MMIO {
    MMIOINFO			info;
    struct tagWINE_MMIO*	lpNext;
    struct IOProcList*		ioProc;
    unsigned			bTmpIOProc : 1,
                                bBufferLoaded : 1;
    DWORD                       dwFileSize;
} WINE_MMIO, *LPWINE_MMIO;

/* function prototypes */
BOOL WINMM_CheckForMMSystem(void);
LPWINE_DRIVER	DRIVER_FindFromHDrvr(HDRVR hDrvr);
BOOL		DRIVER_GetLibName(LPCWSTR keyName, LPCWSTR sectName, LPWSTR buf, int sz);
LPWINE_DRIVER	DRIVER_TryOpenDriver32(LPCWSTR fn, LPARAM lParam2);
void            DRIVER_UnloadAll(void);
HDRVR WINAPI OpenDriverA(LPCSTR lpDriverName, LPCSTR lpSectionName, LPARAM lParam);
BOOL	MMDRV_Install(LPCSTR drvRegName, LPCSTR drvFileName, BOOL bIsMapper);
BOOL LoadRegistryMMEDrivers(char* key);
BOOL		MMDRV_Init(void);
void            MMDRV_Exit(void);
UINT		MMDRV_GetNum(UINT);
LPWINE_MLD	MMDRV_Alloc(UINT size, UINT type, LPHANDLE hndl, DWORD* dwFlags,
                            DWORD_PTR* dwCallback, DWORD_PTR* dwInstance);
void		MMDRV_Free(HANDLE hndl, LPWINE_MLD mld);
DWORD		MMDRV_Open(LPWINE_MLD mld, UINT wMsg, DWORD_PTR dwParam1, DWORD dwParam2);
DWORD		MMDRV_Close(LPWINE_MLD mld, UINT wMsg);
LPWINE_MLD	MMDRV_Get(HANDLE hndl, UINT type, BOOL bCanBeID);
LPWINE_MLD	MMDRV_GetRelated(HANDLE hndl, UINT srcType, BOOL bSrcCanBeID, UINT dstTyped);
DWORD           MMDRV_Message(LPWINE_MLD mld, UINT wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
UINT		MMDRV_PhysicalFeatures(LPWINE_MLD mld, UINT uMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

const char* 	MCI_MessageToString(UINT wMsg);
DWORD           MCI_SendCommand(UINT wDevID, UINT16 wMsg, DWORD_PTR dwParam1, DWORD_PTR dwParam2);
LPWSTR          MCI_strdupAtoW(LPCSTR str);
LPSTR           MCI_strdupWtoA(LPCWSTR str);

const char*     WINMM_ErrorToString(MMRESULT error);

void		TIME_MMTimeStop(void);

MMRESULT WINMM_CheckCallback(DWORD_PTR dwCallback, DWORD fdwOpen, BOOL mixer) DECLSPEC_HIDDEN;

/* Global variables */
extern CRITICAL_SECTION WINMM_cs;
extern HINSTANCE hWinMM32Instance;
extern HANDLE psLastEvent;
extern HANDLE psStopEvent;

/* GetDriverFlags() returned bits is not documented (nor the call itself)
 * Here are Wine only definitions of the bits
 */
#define WINE_GDF_EXIST	        0x80000000
#define WINE_GDF_EXTERNAL_MASK  0xF0000000
#define WINE_GDF_SESSION        0x00000001


/* Modification to take into account Windows NT's registry format */

#define NT_MME_DRIVERS32_KEY \
    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers32"

#define NT_MME_DRIVERS_KEY \
    "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Drivers"

INT LoadRegistryMMEDrivers(char* key);

#endif /* _WINEMM_H_ */
