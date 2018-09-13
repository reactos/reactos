/****************************** Module Header ******************************\
* Module Name: globals.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains all the server's global variables.  One must be
* executing on the server's context to manipulate any of these variables.
* Serializing access to them is also a good idea.
*
* History:
* 10-15-90 DarrinM      Created.
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

HANDLE ghModuleWin;
CRITICAL_SECTION gcsUserSrv;

DWORD gCmsHungAppTimeout = CMSHUNGAPPTIMEOUT;
DWORD gCmsWaitToKillTimeout = CMSWAITTOKILLTIMEOUT;
DWORD gdwHungToKillCount = CMSHUNGTOKILLCOUNT;
BOOL gfAutoEndTask;

DWORD gdwRIPFlags;

/*
 *  These globals are used when shutting down the services
 *  process.
 */
DWORD gdwServicesProcessId;
DWORD gdwServicesWaitToKillTimeout;
DWORD gdwProcessTerminateTimeout = 0;

/*
 * logon process id
 */
DWORD gIdLogon;

/*
 * Hard error stuff
 */
PHARDERRORINFO gphiList;
DWORD gdwHardErrorThreadId;
HANDLE gNtDllHandle;
HANDLE gEventSource;
PWSTR gpwszaSUCCESS;
PWSTR gpwszaSYSTEM_INFORMATION;
PWSTR gpwszaSYSTEM_WARNING;
PWSTR gpwszaSYSTEM_ERROR;

/*
 * EndTask / Shutdown stuff
 */
DWORD gdwThreadEndSession;
HANDLE gheventCancel;
HANDLE gheventCancelled;

ULONG gSessionId = 0;


