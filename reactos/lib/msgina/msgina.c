/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/* $Id: msgina.c,v 1.9 2004/03/28 12:19:07 weiden Exp $
 *
 * PROJECT:         ReactOS msgina.dll
 * FILE:            lib/msgina/msgina.c
 * PURPOSE:         ReactOS Logon GINA DLL
 * PROGRAMMER:      Thomas Weidenmueller (w3seek@users.sourceforge.net)
 * UPDATE HISTORY:
 *      24-11-2003  Created
 */
#include <windows.h>
#include <WinWlx.h>
#include "msgina.h"
#include "resource.h"

extern HINSTANCE hDllInstance;

typedef struct _DISPLAYSTATUSMSG
{
  PGINA_CONTEXT Context;
  HDESK hDesktop;
  DWORD dwOptions;
  PWSTR pTitle;
  PWSTR pMessage;
  HANDLE StartupEvent;
} DISPLAYSTATUSMSG, *PDISPLAYSTATUSMSG;

BOOL CALLBACK 
LoggedOnDlgProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_COMMAND:
    {
      switch(LOWORD(wParam))
      {
        case IDYES:
        case IDNO:
        {
          EndDialog(hwndDlg, LOWORD(wParam));
          break;
        }
      }
      return FALSE;
    }
    case WM_INITDIALOG:
    {
      SetFocus(GetDlgItem(hwndDlg, IDNO));
      break;
    }
    case WM_CLOSE:
    {
      EndDialog(hwndDlg, IDNO);
      return TRUE;
    }
  }
  return FALSE;
}


/*
 * @implemented
 */
BOOL WINAPI
WlxNegotiate(
	DWORD  dwWinlogonVersion,
	PDWORD pdwDllVersion)
{
  if(!pdwDllVersion || (dwWinlogonVersion < GINA_VERSION))
    return FALSE;
  
  *pdwDllVersion = GINA_VERSION;
  
  return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
WlxInitialize(
	LPWSTR lpWinsta,
	HANDLE hWlx,
	PVOID  pvReserved,
	PVOID  pWinlogonFunctions,
	PVOID  *pWlxContext)
{
  PGINA_CONTEXT pgContext;
  
  pgContext = (PGINA_CONTEXT)LocalAlloc(LMEM_FIXED | LMEM_ZEROINIT, sizeof(GINA_CONTEXT));
  if(!pgContext)
    return FALSE;
  
  /* return the context to winlogon */
  *pWlxContext = (PVOID)pgContext;
  
  pgContext->hDllInstance = hDllInstance;
  
  /* save pointer to dispatch table */
  pgContext->pWlxFuncs = (PWLX_DISPATCH_VERSION)pWinlogonFunctions;
  
  /* save the winlogon handle used to call the dispatch functions */
  pgContext->hWlx = hWlx;
  
  /* save window station */
  pgContext->station = lpWinsta;
  
  /* clear status window handle */
  pgContext->hStatusWindow = 0;
  
  /* notify winlogon that we will use the default SAS */
  pgContext->pWlxFuncs->WlxUseCtrlAltDel(hWlx);
  
  return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
WlxStartApplication(
	PVOID pWlxContext,
	PWSTR pszDesktopName,
	PVOID pEnvironment,
	PWSTR pszCmdLine)
{
  PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  BOOL Ret;
  
  si.cb = sizeof(STARTUPINFO);
  si.lpReserved = NULL;
  si.lpTitle = pszCmdLine;
  si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
  si.dwFlags = 0;
  si.wShowWindow = SW_SHOW;  
  si.lpReserved2 = NULL;
  si.cbReserved2 = 0;
  si.lpDesktop = pszDesktopName;
  
  Ret = CreateProcessAsUser(pgContext->UserToken,
                            NULL,
                            pszCmdLine,
                            NULL,
                            NULL,
                            FALSE,
                            CREATE_UNICODE_ENVIRONMENT,
                            pEnvironment,
                            NULL,
                            &si,
                            &pi);
  
  VirtualFree(pEnvironment, 0, MEM_RELEASE);
  return Ret;
}


/*
 * @implemented
 */
BOOL WINAPI
WlxActivateUserShell(
	PVOID pWlxContext,
	PWSTR pszDesktopName,
	PWSTR pszMprLogonScript,
	PVOID pEnvironment)
{
  PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  HKEY hKey;
  DWORD BufSize, ValueType;
  WCHAR pszUserInitApp[MAX_PATH];
  WCHAR pszExpUserInitApp[MAX_PATH];
  BOOL Ret;
  
  /* get the path of userinit */
  if(RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
                  L"SOFTWARE\\ReactOS\\Windows NT\\CurrentVersion\\Winlogon", 
                  0, KEY_QUERY_VALUE, &hKey) != ERROR_SUCCESS)
  {DbgPrint("GINA: Failed: 1\n");
    VirtualFree(pEnvironment, 0, MEM_RELEASE);
    return FALSE;
  }
  BufSize = MAX_PATH * sizeof(WCHAR);
  if((RegQueryValueEx(hKey, L"Userinit", NULL, &ValueType, (LPBYTE)pszUserInitApp, 
                     &BufSize) != ERROR_SUCCESS) || 
                     !((ValueType == REG_SZ) || (ValueType == REG_EXPAND_SZ)))
  {DbgPrint("GINA: Failed: 2\n");
    RegCloseKey(hKey);
    VirtualFree(pEnvironment, 0, MEM_RELEASE);
    return FALSE;
  }
  RegCloseKey(hKey);
  
  /* start userinit */
  /* FIXME - allow to start more applications that are comma-separated */
  si.cb = sizeof(STARTUPINFO);
  si.lpReserved = NULL;
  si.lpTitle = L"userinit";
  si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0;
  si.dwFlags = 0;
  si.wShowWindow = SW_SHOW;  
  si.lpReserved2 = NULL;
  si.cbReserved2 = 0;
  si.lpDesktop = pszDesktopName;
  
  ExpandEnvironmentStrings(pszUserInitApp, pszExpUserInitApp, MAX_PATH);
  
  Ret = CreateProcessAsUser(pgContext->UserToken,
                            pszExpUserInitApp,
                            NULL,
                            NULL,
                            NULL,
                            FALSE,
                            CREATE_UNICODE_ENVIRONMENT,
                            pEnvironment,
                            NULL,
                            &si,
                            &pi);
  if(!Ret) DbgPrint("GINA: Failed: 3\n");
  VirtualFree(pEnvironment, 0, MEM_RELEASE);
  return Ret;
}


/*
 * @implemented
 */
int WINAPI
WlxLoggedOnSAS(
	PVOID pWlxContext,
	DWORD dwSasType,
	PVOID pReserved)
{
  PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
  int SasAction = WLX_SAS_ACTION_NONE;
  
  switch(dwSasType)
  {
    case WLX_SAS_TYPE_CTRL_ALT_DEL:
    {
      int Result;
      /* display "Are you sure you want to log off?" dialog */
      Result = pgContext->pWlxFuncs->WlxDialogBoxParam(pgContext->hWlx,
                                                       pgContext->hDllInstance,
                                                       (LPTSTR)IDD_LOGOFF_DLG,
                                                       NULL,
                                                       LoggedOnDlgProc,
                                                       (LPARAM)pgContext);
      if(Result == IDOK)
      {
        SasAction = WLX_SAS_ACTION_LOCK_WKSTA;
      }
      break;
    }
    case WLX_SAS_TYPE_SC_INSERT:
    {
      DbgPrint("WlxLoggedOnSAS: SasType WLX_SAS_TYPE_SC_INSERT not supported!\n");
      break;
    }
    case WLX_SAS_TYPE_SC_REMOVE:
    {
      DbgPrint("WlxLoggedOnSAS: SasType WLX_SAS_TYPE_SC_REMOVE not supported!\n");
      break;
    }
    case WLX_SAS_TYPE_TIMEOUT:
    {
      DbgPrint("WlxLoggedOnSAS: SasType WLX_SAS_TYPE_TIMEOUT not supported!\n");
      break;
    }
    default:
    {
      DbgPrint("WlxLoggedOnSAS: Unknown SasType: 0x%x\n", dwSasType);
      break;
    }
  }
  
  return SasAction;
}


BOOL 
CALLBACK 
StatusMessageWindowProc(
  HWND hwndDlg,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam
)
{
  switch(uMsg)
  {
    case WM_INITDIALOG:
    {
      PDISPLAYSTATUSMSG msg = (PDISPLAYSTATUSMSG)lParam;
      if(!msg)
        return FALSE;
      
      msg->Context->hStatusWindow = hwndDlg;
      
      if(msg->pTitle)
        SetWindowText(hwndDlg, msg->pTitle);
      SetDlgItemText(hwndDlg, IDC_STATUSLABEL, msg->pMessage);
      if(!msg->Context->SignaledStatusWindowCreated)
      {
        msg->Context->SignaledStatusWindowCreated = TRUE;
        SetEvent(msg->StartupEvent);
      }
      break;
    }
  }
  return FALSE;
}


DWORD WINAPI
StartupWindowThread(LPVOID lpParam)
{
  HDESK OldDesk;
  PDISPLAYSTATUSMSG msg = (PDISPLAYSTATUSMSG)lpParam;
  
  OldDesk = GetThreadDesktop(GetCurrentThreadId());
  
  if(!SetThreadDesktop(msg->hDesktop))
  {
    HeapFree(GetProcessHeap(), 0, lpParam);
    return FALSE;
  }
  DialogBoxParam(hDllInstance, 
                 MAKEINTRESOURCE(IDD_STATUSWINDOW),
                 0,
                 StatusMessageWindowProc,
                 (LPARAM)lpParam);
  SetThreadDesktop(OldDesk);
  
  msg->Context->hStatusWindow = 0;
  msg->Context->SignaledStatusWindowCreated = FALSE;
  
  HeapFree(GetProcessHeap(), 0, lpParam);
  return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
WlxDisplayStatusMessage(
	PVOID pWlxContext,
	HDESK hDesktop,
	DWORD dwOptions,
	PWSTR pTitle,
	PWSTR pMessage)
{
  PDISPLAYSTATUSMSG msg;
  HANDLE Thread;
  DWORD ThreadId;
  PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
  
  if(!pgContext->hStatusWindow)
  {
    msg = (PDISPLAYSTATUSMSG)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(DISPLAYSTATUSMSG));
    if(!msg)
      return FALSE;
    
    msg->Context = pgContext;
    msg->dwOptions = dwOptions;
    msg->pTitle = pTitle;
    msg->pMessage = pMessage;
    msg->hDesktop = hDesktop;
    
    msg->StartupEvent = CreateEvent(NULL,
                                    TRUE,
                                    FALSE,
                                    NULL);
    
    if(!msg->StartupEvent)
      return FALSE;
    
    Thread = CreateThread(NULL,
                          0,
                          StartupWindowThread,
                          (PVOID)msg,
                          0,
                          &ThreadId);
    if(Thread)
    {
      CloseHandle(Thread);
      WaitForSingleObject(msg->StartupEvent, INFINITE);
      CloseHandle(msg->StartupEvent);
      return TRUE;
    }
    
    return FALSE;
  }
  
  if(pTitle)
    SetWindowText(pgContext->hStatusWindow, pTitle);
  
  SetDlgItemText(pgContext->hStatusWindow, IDC_STATUSLABEL, pMessage);
  
  return TRUE;
}


/*
 * @implemented
 */
BOOL WINAPI
WlxRemoveStatusMessage(
	PVOID pWlxContext)
{
  PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
  if(pgContext->hStatusWindow)
  {
    EndDialog(pgContext->hStatusWindow, 0);
    pgContext->hStatusWindow = 0;
  }
  
  return TRUE;
}


/*
 * @implemented
 */
VOID WINAPI
WlxDisplaySASNotice(
	PVOID pWlxContext)
{
  PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
  pgContext->pWlxFuncs->WlxSasNotify(pgContext->hWlx, WLX_SAS_TYPE_CTRL_ALT_DEL);
}


static PWSTR
DuplicationString(PWSTR Str)
{
  DWORD cb;
  PWSTR NewStr;

  cb = (wcslen(Str) + 1) * sizeof(WCHAR);
  if((NewStr = LocalAlloc(LMEM_FIXED, cb)))
  {
    memcpy(NewStr, Str, cb);
  }
  return NewStr;
}


/*
 * @unimplemented
 */
int WINAPI
WlxLoggedOutSAS(
	PVOID                pWlxContext,
	DWORD                dwSasType,
	PLUID                pAuthenticationId,
	PSID                 pLogonSid,
	PDWORD               pdwOptions,
	PHANDLE              phToken,
	PWLX_MPR_NOTIFY_INFO pNprNotifyInfo,
	PVOID                *pProfile)
{
  PGINA_CONTEXT pgContext = (PGINA_CONTEXT)pWlxContext;
  TOKEN_STATISTICS Stats;
  DWORD cbStats;

  if(!phToken)
  {
    DbgPrint("msgina: phToken == NULL!\n");
    return WLX_SAS_ACTION_NONE;
  }

  if(!LogonUser(L"Administrator", NULL, L"Secrect",
                LOGON32_LOGON_INTERACTIVE, /* FIXME - use LOGON32_LOGON_UNLOCK instead! */
                LOGON32_PROVIDER_DEFAULT,
                phToken))
  {
    DbgPrint("msgina: Logonuser() failed\n");
    return WLX_SAS_ACTION_NONE;
  }
  
  if(!(*phToken))
  {
    DbgPrint("msgina: *phToken == NULL!\n");
    return WLX_SAS_ACTION_NONE;
  }

  pgContext->UserToken =*phToken;
  
  *pdwOptions = 0;
  *pProfile =NULL; 
  
  if(!GetTokenInformation(*phToken,
                          TokenStatistics,
                          (PVOID)&Stats,
                          sizeof(TOKEN_STATISTICS),
                          &cbStats))
  {
    DbgPrint("msgina: Couldn't get Autentication id from user token!\n");
    return WLX_SAS_ACTION_NONE;
  }
  *pAuthenticationId = Stats.AuthenticationId; 
  pNprNotifyInfo->pszUserName = DuplicationString(L"Administrator");
  pNprNotifyInfo->pszDomain = NULL;
  pNprNotifyInfo->pszPassword = DuplicationString(L"Secret");
  pNprNotifyInfo->pszOldPassword = NULL;

  return WLX_SAS_ACTION_LOGON;
}


BOOL STDCALL
DllMain(
	HINSTANCE hinstDLL,
	DWORD     dwReason,
	LPVOID    lpvReserved)
{
  switch (dwReason)
  {
    case DLL_PROCESS_ATTACH:
      /* fall through */
    case DLL_THREAD_ATTACH:
      hDllInstance = hinstDLL;
      break;
    case DLL_THREAD_DETACH:
      break;
    case DLL_PROCESS_DETACH:
      break;
  }
  return TRUE;
}

