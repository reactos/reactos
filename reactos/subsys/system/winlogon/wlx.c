/* $Id: wlx.c,v 1.4 2004/03/28 12:21:41 weiden Exp $
 * 
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/winlogon/winlogon.c
 * PURPOSE:         Logon 
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ntos.h>
#include <windows.h>
#include <stdio.h>
#include <WinWlx.h>
#include <wchar.h>

#include "setup.h"
#include "winlogon.h"
#include "resource.h"

#define NDEBUG
#include <debug.h>

#define Unimplemented DbgPrint("WL: %S() at %S:%i unimplemented!\n", __FUNCTION__, __FILE__, __LINE__)

/*
 * @implemented
 */
VOID WINAPI
WlxUseCtrlAltDel(
  HANDLE hWlx
)
{
  WlxSetOption(hWlx, WLX_OPTION_USE_CTRL_ALT_DEL, TRUE, NULL);
}

/*
 * @implemented
 */
VOID WINAPI
WlxSetContextPointer(
  HANDLE hWlx,
  PVOID pWlxContext
)
{
  WlxSetOption(hWlx, WLX_OPTION_CONTEXT_POINTER, (ULONG_PTR)pWlxContext, NULL);
}

/*
 * @implemented
 */
VOID WINAPI
WlxSasNotify(
  HANDLE hWlx,
  DWORD dwSasType
)
{
  DispatchSAS((PWLSESSION)hWlx, dwSasType);
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxSetTimeout(
  HANDLE hWlx,
  DWORD Timeout
)
{
  /* Unimplemented; */
  return FALSE;
}

/*
 * @unimplemented
 */
int WINAPI
WlxAssignShellProtection(
  HANDLE hWlx,
  HANDLE hToken,
  HANDLE hProcess,
  HANDLE hThread
)
{
  Unimplemented;
  return 0;
}

/*
 * @unimplemented
 */
int WINAPI
WlxMessageBox(
  HANDLE hWlx,
  HWND hwndOwner,
  LPWSTR lpszText,
  LPWSTR lpszTitle,
  UINT fuStyle
)
{
  Unimplemented;
  return 0;
}

/*
 * @unimplemented
 */
int WINAPI
WlxDialogBox(
  HANDLE hWlx,
  HANDLE hInst,
  LPWSTR lpszTemplate,
  HWND hwndOwner,
  DLGPROC dlgprc
)
{
  Unimplemented;
  return 0;
}

/*
 * @unimplemented
 */
int WINAPI
WlxDialogBoxParam(
  HANDLE hWlx,
  HANDLE hInst,
  LPWSTR lpszTemplate,
  HWND hwndOwner,
  DLGPROC dlgprc,
  LPARAM dwInitParam
)
{
  Unimplemented;
  return 0;
}

/*
 * @unimplemented
 */
int WINAPI
WlxDialogBoxIndirect(
  HANDLE hWlx,
  HANDLE hInst,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hwndOwner,
  DLGPROC dlgprc
)
{
  Unimplemented;
  return 0;
}

/*
 * @unimplemented
 */
int WINAPI
WlxDialogBoxIndirectParam(
  HANDLE hWlx,
  HANDLE hInst,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hwndOwner,
  DLGPROC dlgprc,
  LPARAM dwInitParam
)
{
  Unimplemented;
  return 0;
}

/*
 * @unimplemented
 */
int WINAPI
WlxSwitchDesktopToUser(
  HANDLE hWlx
)
{
  Unimplemented;
  return 0;
}

/*
 * @unimplemented
 */
int WINAPI
WlxSwitchDesktopToWinlogon(
  HANDLE hWlx
)
{
  Unimplemented;
  return 0;
}

/*
 * @unimplemented
 */
int WINAPI
WlxChangePasswordNotify(
  HANDLE hWlx,
  PWLX_MPR_NOTIFY_INFO pMprInfo,
  DWORD dwChangeInfo
)
{
  Unimplemented;
  return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxGetSourceDesktop(
  HANDLE hWlx,
  PWLX_DESKTOP* ppDesktop
)
{
  Unimplemented;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxSetReturnDesktop(
  HANDLE hWlx,
  PWLX_DESKTOP pDesktop
)
{
  Unimplemented;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxCreateUserDesktop(
  HANDLE hWlx,
  HANDLE hToken,
  DWORD Flags,
  PWSTR pszDesktopName,
  PWLX_DESKTOP* ppDesktop
)
{
  Unimplemented;
  return FALSE;
}

/*
 * @unimplemented
 */
int WINAPI
WlxChangePasswordNotifyEx(
  HANDLE hWlx,
  PWLX_MPR_NOTIFY_INFO pMprInfo,
  DWORD dwChangeInfo,
  PWSTR ProviderName,
  PVOID Reserved
)
{
  Unimplemented;
  return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxCloseUserDesktop(
  HANDLE hWlx,
  PWLX_DESKTOP pDesktop,
  HANDLE hToken
)
{
  Unimplemented;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxSetOption(
  HANDLE hWlx,
  DWORD Option,
  ULONG_PTR Value,
  ULONG_PTR* OldValue
)
{
  PWLSESSION Session = (PWLSESSION)hWlx;
  
  if(Session || !Value)
  {
    switch(Option)
    {
      case WLX_OPTION_USE_CTRL_ALT_DEL:
        return TRUE;
      case WLX_OPTION_CONTEXT_POINTER:
      {
        *OldValue = (ULONG_PTR)Session->MsGina.Context;
        Session->MsGina.Context = (PVOID)Value;
        return TRUE;
      }    
      case WLX_OPTION_USE_SMART_CARD:
        return FALSE;
    }
  }
  
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxGetOption(
  HANDLE hWlx,
  DWORD Option,
  ULONG_PTR* Value
)
{
  PMSGINAINSTANCE Instance = (PMSGINAINSTANCE)hWlx;
  Unimplemented;
  if(Instance || !Value)
  {
    switch(Option)
    {
      case WLX_OPTION_USE_CTRL_ALT_DEL:
        return TRUE;
      case WLX_OPTION_CONTEXT_POINTER:
      {
        *Value = (ULONG_PTR)Instance->Context;
        return TRUE;
      }
      case WLX_OPTION_USE_SMART_CARD:
      case WLX_OPTION_SMART_CARD_PRESENT:
      case WLX_OPTION_SMART_CARD_INFO:
        *Value = 0;
        return FALSE;
      case WLX_OPTION_DISPATCH_TABLE_SIZE:
      {
        switch(Instance->Version)
        {
          case WLX_VERSION_1_0:
            *Value = sizeof(WLX_DISPATCH_VERSION_1_0);
            break;
          case WLX_VERSION_1_1:
            *Value = sizeof(WLX_DISPATCH_VERSION_1_1);
            break;
          case WLX_VERSION_1_2:
            *Value = sizeof(WLX_DISPATCH_VERSION_1_2);
            break;
          case WLX_VERSION_1_3:
            *Value = sizeof(WLX_DISPATCH_VERSION_1_3);
            break;
          case WLX_VERSION_1_4:
            *Value = sizeof(WLX_DISPATCH_VERSION_1_4);
            break;
          default:
            return 0;
        }
        return TRUE;
      }
    }
  }
  
  return FALSE;
}

/*
 * @unimplemented
 */
VOID WINAPI
WlxWin31Migrate(
  HANDLE hWlx
)
{
  Unimplemented;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxQueryClientCredentials(
  PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pCred
)
{
  Unimplemented;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxQueryInetConnectorCredentials(
  PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pCred
)
{
  Unimplemented;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD WINAPI
WlxQueryConsoleSwitchCredentials(
  PWLX_CONSOLESWITCH_CREDENTIALS_INFO_V1_0 pCred
)
{
  Unimplemented;
  return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxQueryTsLogonCredentials(
  PWLX_CLIENT_CREDENTIALS_INFO_V2_0 pCred
)
{
  Unimplemented;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL WINAPI
WlxDisconnect(void)
{
  Unimplemented;
  return FALSE;
}

/*
 * @unimplemented
 */
DWORD WINAPI
WlxQueryTerminalServicesData(
  HANDLE hWlx,
  PWLX_TERMINAL_SERVICES_DATA pTSData,
  WCHAR* UserName,
  WCHAR* Domain
)
{
  Unimplemented;
  return 0;
}

const
WLX_DISPATCH_VERSION_1_4 FunctionTable = {
  WlxUseCtrlAltDel,
  WlxSetContextPointer,
  WlxSasNotify,
  WlxSetTimeout,
  WlxAssignShellProtection,
  WlxMessageBox,
  WlxDialogBox,
  WlxDialogBoxParam,
  WlxDialogBoxIndirect,
  WlxDialogBoxIndirectParam,
  WlxSwitchDesktopToUser,
  WlxSwitchDesktopToWinlogon,
  WlxChangePasswordNotify,
  WlxGetSourceDesktop,
  WlxSetReturnDesktop,
  WlxCreateUserDesktop,
  WlxChangePasswordNotifyEx,
  WlxCloseUserDesktop,
  WlxSetOption,
  WlxGetOption,
  WlxWin31Migrate,
  WlxQueryClientCredentials,
  WlxQueryInetConnectorCredentials,
  WlxDisconnect,
  WlxQueryTerminalServicesData,
  WlxQueryConsoleSwitchCredentials,
  WlxQueryTsLogonCredentials
};

/******************************************************************************/

static void
GetMsGinaPath(WCHAR *path)
{
  DWORD Status, Type, Size;
  HANDLE hKey;
  
  Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Winlogon",
                        0,
                        KEY_QUERY_VALUE,
                        &hKey);
  if(Status != ERROR_SUCCESS)
  {
    wcscpy(path, L"msgina.dll");
    return;
  }
  
  Size = MAX_PATH * sizeof(WCHAR);
  Status = RegQueryValueEx(hKey,
                           L"GinaDLL",
                           NULL,
                           &Type,
                           (LPBYTE)path,
                           &Size);
  if((Status != ERROR_SUCCESS) || (Size != REG_SZ) || (Size == 0))
  {
    wcscpy(path, L"msgina.dll");
  }
  RegCloseKey(hKey);
}

BOOL 
CALLBACK 
GinaLoadFailedProc(
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
        case IDOK:
          EndDialog(hwndDlg, IDOK);
          break;
      }
      break;
    }
    case WM_INITDIALOG:
    {
      int len;
      WCHAR str[MAX_PATH], str2[MAX_PATH];
      
      if(lParam)
      {
        len = GetDlgItemText(hwndDlg, IDC_GINALOADFAILED, str, MAX_PATH);
        
        if(len)
        {
          wsprintf(str2, str, (LPWSTR)lParam);
          SetDlgItemText(hwndDlg, IDC_GINALOADFAILED, str2);
        }
      }
      SetFocus(GetDlgItem(hwndDlg, IDOK));
      break;
    }
    case WM_CLOSE:
    {
      EndDialog(hwndDlg, IDCANCEL);
      return TRUE;
    }
  }
  return FALSE;
}

BOOL
LoadGina(PMSGINAFUNCTIONS Functions, DWORD *DllVersion, HMODULE *GinaInstance)
{
  HMODULE hGina;
  WCHAR GinaDll[MAX_PATH + 1];
  
  GetMsGinaPath(GinaDll);
  
  if(!(hGina = LoadLibrary(GinaDll)))
  {
    DialogBoxParam(hAppInstance, MAKEINTRESOURCE(IDD_GINALOADFAILED), 0, GinaLoadFailedProc, (LPARAM)&GinaDll);
    return FALSE;
  }
  *GinaInstance = hGina;
  
  Functions->WlxNegotiate = (PFWLXNEGOTIATE)GetProcAddress(hGina, "WlxNegotiate");
  Functions->WlxInitialize = (PFWLXINITIALIZE)GetProcAddress(hGina, "WlxInitialize");
  
  if(Functions->WlxNegotiate)
  {
    if(!Functions->WlxNegotiate(WLX_VERSION_1_3, DllVersion))
    {
      return FALSE;
    }
    
    if(*DllVersion >= WLX_VERSION_1_0)
    {
      Functions->WlxActivateUserShell = (PFWLXACTIVATEUSERSHELL)GetProcAddress(hGina, "WlxActivateUserShell");
      Functions->WlxDisplayLockedNotice = (PFWLXDISPLAYLOCKEDNOTICE)GetProcAddress(hGina, "WlxDisplayLockedNotice");
      Functions->WlxDisplaySASNotice = (PFWLXDISPLAYSASNOTICE)GetProcAddress(hGina, "WlxDisplaySASNotice");
      Functions->WlxIsLockOk = (PFWLXISLOCKOK)GetProcAddress(hGina, "WlxIsLockOk");
      Functions->WlxIsLogoffOk = (PFWLXISLOGOFFOK)GetProcAddress(hGina, "WlxIsLogoffOk");
      Functions->WlxLoggedOnSAS = (PFWLXLOGGEDONSAS)GetProcAddress(hGina, "WlxLoggedOnSAS");
      Functions->WlxLoggedOutSAS = (PFWLXLOGGEDOUTSAS)GetProcAddress(hGina, "WlxLoggedOutSAS");
      Functions->WlxLogoff = (PFWLXLOGOFF)GetProcAddress(hGina, "WlxLogoff");
      Functions->WlxShutdown = (PFWLXSHUTDOWN)GetProcAddress(hGina, "WlxShutdown");
      Functions->WlxWkstaLockedSAS = (PFWLXWKSTALOCKEDSAS)GetProcAddress(hGina, "WlxWkstaLockedSAS");
    }
    
    if(*DllVersion >= WLX_VERSION_1_1)
    {
      Functions->WlxScreenSaverNotify = (PFWLXSCREENSAVERNOTIFY)GetProcAddress(hGina, "WlxScreenSaverNotify");
      Functions->WlxStartApplication = (PFWLXSTARTAPPLICATION)GetProcAddress(hGina, "WlxStartApplication");
    }
    
    if(*DllVersion >= WLX_VERSION_1_3)
    {
      Functions->WlxDisplayStatusMessage = (PFWLXDISPLAYSTATUSMESSAGE)GetProcAddress(hGina, "WlxDisplayStatusMessage");
      Functions->WlxGetStatusMessage = (PFWLXGETSTATUSMESSAGE)GetProcAddress(hGina, "WlxGetStatusMessage");
      Functions->WlxNetworkProviderLoad = (PFWLXNETWORKPROVIDERLOAD)GetProcAddress(hGina, "WlxNetworkProviderLoad");
      Functions->WlxRemoveStatusMessage = (PFWLXREMOVESTATUSMESSAGE)GetProcAddress(hGina, "WlxRemoveStatusMessage");
    }
    
    if(*DllVersion >= WLX_VERSION_1_4)
    {
      
    }
  }
  
  return (Functions->WlxNegotiate != NULL) && (Functions->WlxInitialize != NULL);
}

PWLSESSION
MsGinaInit(void)
{
  PWLSESSION WLSession;
  DWORD GinaDllVersion;
  
  WLSession = (PWLSESSION)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(WLSESSION));
  if(!WLSession)
  {
    return NULL;
  }
  
  if(!LoadGina(&WLSession->MsGina.Functions, &GinaDllVersion, &WLSession->MsGina.hDllInstance))
  {
    HeapFree(GetProcessHeap(), 0, WLSession);
    return NULL;
  }
  
  WLSession->MsGina.Context = NULL;
  WLSession->MsGina.Version = GinaDllVersion;
  WLSession->SuppressStatus = FALSE;
  
  if(!WLSession->MsGina.Functions.WlxInitialize(WLSession->InteractiveWindowStation,
                                               (HANDLE)WLSession,
                                               NULL,
                                               (PVOID)&FunctionTable,
                                               &WLSession->MsGina.Context))
  {
    HeapFree(GetProcessHeap(), 0, WLSession);
    return NULL;
  }
  return WLSession;
}

BOOL
WlxCreateWindowStationAndDesktops(PWLSESSION Session)
{
  /*
   * Create the interactive window station
   */
  Session->InteractiveWindowStation = CreateWindowStation(L"WinSta0", 0, GENERIC_ALL, NULL);
  if(!Session->InteractiveWindowStation)
  {
    DbgPrint("WL: Failed to create window station (0x%X)\n", GetLastError());
    return FALSE;
  }
  SetProcessWindowStation(Session->InteractiveWindowStation);
  
  /*
   * Create the application desktop
   */
  Session->ApplicationDesktop = CreateDesktop(L"Default",
                                              NULL,
                                              NULL,
                                              0,      /* FIXME: Set some flags */
                                              GENERIC_ALL,
                                              NULL);
  if(!Session->ApplicationDesktop)
  {
    DbgPrint("WL: Failed to create Default desktop (0x%X)\n", GetLastError());
    return FALSE;
  }
  
  /*
   * Create the winlogon desktop
   */
  Session->WinlogonDesktop = CreateDesktop(L"Winlogon",
                                           NULL,
                                           NULL,
                                           0,      /* FIXME: Set some flags */
                                           GENERIC_ALL,
                                           NULL);
  if(!Session->WinlogonDesktop)
  {
    DbgPrint("WL: Failed to create Winlogon desktop (0x%X)\n", GetLastError());
    return FALSE;
  }
  
  /*
   * Create the screen saver desktop
   */
  Session->ScreenSaverDesktop = CreateDesktop(L"Screen-Saver",
                                              NULL,
                                              NULL,
                                              0,      /* FIXME: Set some flags */
                                              GENERIC_ALL,
                                              NULL);
  if(!Session->ScreenSaverDesktop)
  {
    DbgPrint("WL: Failed to create Screen-Saver desktop (0x%X)\n", GetLastError());
    return FALSE;
  }
  
  return TRUE;
}

