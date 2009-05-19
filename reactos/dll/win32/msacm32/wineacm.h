/* -*- tab-width: 8; c-basic-offset: 4 -*- */
/*
 * Copyright 2000 Eric Pouech
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

#ifndef __WINE_WINEACM_H
#define __WINE_WINEACM_H

/***********************************************************************
 * Wine specific - Win32
 */
typedef struct _WINE_ACMDRIVERID *PWINE_ACMDRIVERID;
typedef struct _WINE_ACMDRIVER   *PWINE_ACMDRIVER;

#define WINE_ACMOBJ_DONTCARE	0x5EED0000
#define WINE_ACMOBJ_DRIVERID	0x5EED0001
#define WINE_ACMOBJ_DRIVER	0x5EED0002
#define WINE_ACMOBJ_STREAM	0x5EED0003
#define WINE_ACMOBJ_NOTIFYWND   0x5EED0004
#define WINE_ACMOBJ_LOCALDRIVER 0x5EED0005

typedef struct _WINE_ACMOBJ
{
    DWORD		dwType;
    PWINE_ACMDRIVERID	pACMDriverID;
} WINE_ACMOBJ, *PWINE_ACMOBJ;

typedef struct _WINE_ACMLOCALDRIVER * PWINE_ACMLOCALDRIVER;
typedef struct _WINE_ACMLOCALDRIVERINST * PWINE_ACMLOCALDRIVERINST;
typedef struct _WINE_ACMLOCALDRIVER
{
    WINE_ACMOBJ         obj;
    HMODULE             hModule;
    DRIVERPROC          lpDrvProc;
    PWINE_ACMLOCALDRIVERINST pACMInstList;
    PWINE_ACMLOCALDRIVER pNextACMLocalDrv;
    PWINE_ACMLOCALDRIVER pPrevACMLocalDrv;
} WINE_ACMLOCALDRIVER;

typedef struct _WINE_ACMLOCALDRIVERINST
{
    PWINE_ACMLOCALDRIVER pLocalDriver;
    DWORD dwDriverID;
    BOOL bSession;
    PWINE_ACMLOCALDRIVERINST pNextACMInst;
} WINE_ACMLOCALDRIVERINST;

typedef struct _WINE_ACMDRIVER
{
    WINE_ACMOBJ		obj;
    HDRVR      		hDrvr;
    PWINE_ACMLOCALDRIVERINST pLocalDrvrInst;

    PWINE_ACMDRIVER	pNextACMDriver;
} WINE_ACMDRIVER;

typedef struct _WINE_ACMSTREAM
{
    WINE_ACMOBJ		obj;
    PWINE_ACMDRIVER	pDrv;
    ACMDRVSTREAMINSTANCE drvInst;
    HACMDRIVER		hAcmDriver;
} WINE_ACMSTREAM, *PWINE_ACMSTREAM;

typedef struct _WINE_ACMDRIVERID
{
    WINE_ACMOBJ		obj;
    LPWSTR		pszDriverAlias;
    LPWSTR              pszFileName;
    PWINE_ACMLOCALDRIVER pLocalDriver;          /* NULL if global */
    PWINE_ACMDRIVER     pACMDriverList;
    PWINE_ACMDRIVERID   pNextACMDriverID;
    PWINE_ACMDRIVERID	pPrevACMDriverID;
    /* information about the driver itself, either gotten from registry or driver itself */
    DWORD		cFilterTags;
    DWORD		cFormatTags;
    DWORD		fdwSupport;
    struct {
	DWORD			dwFormatTag;
	DWORD			cbwfx;
    }* 			aFormatTag;
} WINE_ACMDRIVERID;

typedef struct _WINE_ACMNOTIFYWND * PWINE_ACMNOTIFYWND;
typedef struct _WINE_ACMNOTIFYWND
{
    WINE_ACMOBJ		obj;
    HWND                hNotifyWnd;          /* Window to notify on ACM events: driver add, driver removal, priority change */
    DWORD               dwNotifyMsg;         /* Notification message to send to window */
    DWORD		fdwSupport;
    PWINE_ACMNOTIFYWND  pNextACMNotifyWnd;
    PWINE_ACMNOTIFYWND  pPrevACMNotifyWnd;
} WINE_ACMNOTIFYWND;

/* From internal.c */
extern HANDLE MSACM_hHeap;
extern PWINE_ACMDRIVERID MSACM_pFirstACMDriverID;
extern PWINE_ACMDRIVERID MSACM_RegisterDriver(LPCWSTR pszDriverAlias, LPCWSTR pszFileName,
					      PWINE_ACMLOCALDRIVER pLocalDriver);
extern void MSACM_RegisterAllDrivers(void);
extern PWINE_ACMDRIVERID MSACM_UnregisterDriver(PWINE_ACMDRIVERID p);
extern void MSACM_UnregisterAllDrivers(void);
extern PWINE_ACMDRIVERID MSACM_GetDriverID(HACMDRIVERID hDriverID);
extern PWINE_ACMDRIVER MSACM_GetDriver(HACMDRIVER hDriver);
extern PWINE_ACMNOTIFYWND MSACM_GetNotifyWnd(HACMDRIVERID hDriver);
extern PWINE_ACMOBJ MSACM_GetObj(HACMOBJ hObj, DWORD type);

extern MMRESULT MSACM_Message(HACMDRIVER, UINT, LPARAM, LPARAM);
extern BOOL MSACM_FindFormatTagInCache(const WINE_ACMDRIVERID*, DWORD, LPDWORD);

extern void MSACM_RePositionDriver(PWINE_ACMDRIVERID, DWORD);
extern void MSACM_WriteCurrentPriorities(void);
extern void MSACM_BroadcastNotification(void);
extern void MSACM_DisableNotifications(void);
extern void MSACM_EnableNotifications(void);
extern PWINE_ACMNOTIFYWND MSACM_RegisterNotificationWindow(HWND hNotifyWnd, DWORD dwNotifyMsg);
extern PWINE_ACMNOTIFYWND MSACM_UnRegisterNotificationWindow(const WINE_ACMNOTIFYWND*);

extern PWINE_ACMDRIVERID MSACM_RegisterDriverFromRegistry(LPCWSTR pszRegEntry);

extern PWINE_ACMLOCALDRIVER MSACM_RegisterLocalDriver(HMODULE hModule, DRIVERPROC lpDriverProc);
extern PWINE_ACMLOCALDRIVERINST MSACM_OpenLocalDriver(PWINE_ACMLOCALDRIVER, LPARAM);
extern LRESULT MSACM_CloseLocalDriver(PWINE_ACMLOCALDRIVERINST);
/*
extern PWINE_ACMLOCALDRIVER MSACM_GetLocalDriver(HACMDRIVER hDriver);
*/
/* From msacm32.c */
extern HINSTANCE MSACM_hInstance32;

/* From pcmcnvtr.c */
LRESULT CALLBACK	PCM_DriverProc(DWORD_PTR dwDevID, HDRVR hDriv, UINT wMsg,
				       LPARAM dwParam1, LPARAM dwParam2);

/* Dialog box templates */
#include "msacmdlg.h"

#endif /* __WINE_WINEACM_H */
