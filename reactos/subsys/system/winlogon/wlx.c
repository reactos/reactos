/* $Id: wlx.c,v 1.1 2003/12/01 18:21:04 weiden Exp $
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
#include <napi/lpc.h>
#include <wchar.h>

#include "setup.h"
#include "winlogon.h"
#include "resource.h"

#define NDEBUG
#include <debug.h>

#define Unimplemented DbgPrint("WL: %S() at %S:%i unimplemented!\n", __FUNCTION__, __FILE__, __LINE__)

PMSGINAINSTANCE MsGinaInst;

/*
 * @unimplemented
 */
VOID WINAPI
WlxUseCtrlAltDel(
  HANDLE hWlx
)
{
  Unimplemented;
}

/*
 * @unimplemented
 */
VOID WINAPI
WlxSetContextPointer(
  HANDLE hWlx,
  PVOID pWlxContext
)
{
  Unimplemented;
}

/*
 * @unimplemented
 */
VOID WINAPI
WlxSasNotify(
  HANDLE hWlx,
  DWORD dwSasType
)
{
  Unimplemented;
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
  Unimplemented;
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
  Unimplemented;
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
  Unimplemented;
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
LoadGina(PMSGINAFUNCTIONS Functions)
{
  HMODULE hGina;
  WCHAR GinaDll[MAX_PATH + 1];
  DWORD DllVersion = 0;
  
  MsGinaInst = NULL;
  
  GetMsGinaPath(GinaDll);
  
  hGina = LoadLibrary(GinaDll);
  if(!hGina)
  {
    DialogBoxParam(hAppInstance, MAKEINTRESOURCE(IDD_GINALOADFAILED), 0, GinaLoadFailedProc, (LPARAM)&GinaDll);
    return FALSE;
  }
  
  Functions->WlxNegotiate = (PFWLXNEGOTIATE)GetProcAddress(hGina, "WlxNegotiate");
  Functions->WlxInitialize = (PFWLXINITIALIZE)GetProcAddress(hGina, "WlxInitialize");
  
  if(Functions->WlxNegotiate)
  {
    if(!Functions->WlxNegotiate(WLX_VERSION_1_3, &DllVersion))
    {
      return FALSE;
    }
    
    /* FIXME - allow other versions */
    if(DllVersion != WLX_VERSION_1_3)
    {
      return FALSE;
    }
    
    if(DllVersion >= WLX_VERSION_1_3)
    {
      Functions->WlxNetworkProviderLoad = (PFWLXNETWORKPROVIDERLOAD)GetProcAddress(hGina, "WlxNetworkProviderLoad");
      Functions->WlxDisplayStatusMessage = (PFWLXDISPLAYSTATUSMESSAGE)GetProcAddress(hGina, "WlxDisplayStatusMessage");
      Functions->WlxGetStatusMessage = (PFWLXGETSTATUSMESSAGE)GetProcAddress(hGina, "WlxGetStatusMessage");
      Functions->WlxRemoveStatusMessage = (PFWLXREMOVESTATUSMESSAGE)GetProcAddress(hGina, "WlxRemoveStatusMessage");
    }
  }
  
  return (Functions->WlxNegotiate != NULL) && (Functions->WlxInitialize != NULL);
}

BOOL
MsGinaInit()
{
  PMSGINAINSTANCE Instance;
  
  Instance = (PMSGINAINSTANCE)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MSGINAINSTANCE));
  if(!Instance)
  {
    return 0;
  }
  
  Instance->Functions = &MsGinaFunctions;
  Instance->hDllInstance = NULL; /* FIXME */
  Instance->Context = NULL;
  
  MsGinaInst = Instance;
  
  if(!Instance->Functions->WlxInitialize(InteractiveWindowStation,
                                         (HANDLE)Instance,
                                         NULL,
                                         (PVOID)&FunctionTable,
                                         &Instance->Context))
  {
    return 0;
  }
  return TRUE;
}

