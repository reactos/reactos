/*
 *  ReactOS kernel
 *  Copyright (C) 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: winlogon.h,v 1.1 2003/12/01 18:21:04 weiden Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS winlogon
 * FILE:            subsys/system/winlogon/winlogon.h
 * PURPOSE:         Winlogon
 * PROGRAMMER:      
 */

#ifndef __WINLOGON_MAIN_H__
#define __WINLOGON_MAIN_H__

#include <WinWlx.h>

typedef BOOL (WINAPI * PFWLXNEGOTIATE)  (DWORD, DWORD *);
typedef BOOL (WINAPI * PFWLXINITIALIZE) (LPWSTR, HANDLE, PVOID, PVOID, PVOID *);
typedef VOID (WINAPI * PFWLXDISPLAYSASNOTICE) (PVOID);
typedef int  (WINAPI * PFWLXLOGGEDOUTSAS) (PVOID, DWORD, PLUID, PSID, PDWORD,
                                           PHANDLE, PWLX_MPR_NOTIFY_INFO, 
                                           PVOID *);
typedef BOOL (WINAPI * PFWLXACTIVATEUSERSHELL) (PVOID, PWSTR, PWSTR, PVOID);
typedef int  (WINAPI * PFWLXLOGGEDONSAS) (PVOID, DWORD, PVOID);
typedef VOID (WINAPI * PFWLXDISPLAYLOCKEDNOTICE) (PVOID);
typedef int  (WINAPI * PFWLXWKSTALOCKEDSAS) (PVOID, DWORD);
typedef BOOL (WINAPI * PFWLXISLOCKOK) (PVOID);
typedef BOOL (WINAPI * PFWLXISLOGOFFOK) (PVOID);
typedef VOID (WINAPI * PFWLXLOGOFF) (PVOID);
typedef VOID (WINAPI * PFWLXSHUTDOWN) (PVOID, DWORD);

/* version 1.1 */

typedef BOOL (WINAPI * PFWLXSCREENSAVERNOTIFY) (PVOID, BOOL *);
typedef BOOL (WINAPI * PFWLXSTARTAPPLICATION) (PVOID, PWSTR, PVOID, PWSTR);

/* version 1.3 */

typedef BOOL (WINAPI * PFWLXNETWORKPROVIDERLOAD) (PVOID, PWLX_MPR_NOTIFY_INFO);
typedef BOOL (WINAPI * PFWLXDISPLAYSTATUSMESSAGE) (PVOID, HDESK, DWORD, PWSTR, PWSTR);
typedef BOOL (WINAPI * PFWLXGETSTATUSMESSAGE) (PVOID, DWORD *, PWSTR, DWORD);
typedef BOOL (WINAPI * PFWLXREMOVESTATUSMESSAGE) (PVOID);

typedef struct _MSGINAFUNCTIONS
{
  PFWLXNEGOTIATE            WlxNegotiate;
  PFWLXINITIALIZE           WlxInitialize;
  PFWLXDISPLAYSASNOTICE     WlxDisplaySASNotice;
  PFWLXLOGGEDOUTSAS         WlxLoggedOutSAS;
  PFWLXACTIVATEUSERSHELL    WlxActivateUserShell;
  PFWLXLOGGEDONSAS          WlxLoggedOnSAS;
  PFWLXDISPLAYLOCKEDNOTICE  WlxDisplayLockedNotice;
  PFWLXWKSTALOCKEDSAS       WlxWkstaLockedSAS;
  PFWLXISLOCKOK             WlxIsLockOK;
  PFWLXISLOGOFFOK           WlxIsLogOffOK;
  PFWLXLOGOFF               WlxLogOff;
  PFWLXSHUTDOWN             WlxShutdown;
  
  PFWLXSCREENSAVERNOTIFY    WlxScreenSaverNotify;
  PFWLXSTARTAPPLICATION     WlxStartApplication;
  
  PFWLXNETWORKPROVIDERLOAD  WlxNetworkProviderLoad;
  PFWLXDISPLAYSTATUSMESSAGE WlxDisplayStatusMessage;
  PFWLXGETSTATUSMESSAGE     WlxGetStatusMessage;
  PFWLXREMOVESTATUSMESSAGE  WlxRemoveStatusMessage;
} MSGINAFUNCTIONS, *PMSGINAFUNCTIONS;

typedef struct _MSGINAINSTANCE
{
  HANDLE hDllInstance;
  PMSGINAFUNCTIONS Functions;
  PVOID Context;
} MSGINAINSTANCE, *PMSGINAINSTANCE;

extern HINSTANCE hAppInstance;
extern PMSGINAINSTANCE MsGinaInst;
extern HWINSTA InteractiveWindowStation;   /* WinSta0 */
extern HDESK ApplicationDesktop;           /* WinSta0\Default */
extern HDESK WinlogonDesktop;              /* WinSta0\Winlogon */
extern HDESK ScreenSaverDesktop;           /* WinSta0\Screen-Saver */

extern MSGINAFUNCTIONS MsGinaFunctions;


#endif /* __WINLOGON_MAIN_H__ */

/* EOF */
