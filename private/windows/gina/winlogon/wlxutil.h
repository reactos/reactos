//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       wlxutil.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    8-24-94   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef _WLXUTIL_H_
#define _WLXUTIL_H_


VOID WINAPI WlxUseCtrlAltDel(HANDLE);
VOID WINAPI WlxSasNotify(HANDLE, DWORD);
VOID WINAPI WlxSetContextPointer(HANDLE, PVOID);
BOOL WINAPI WlxSetTimeout(HANDLE, DWORD);
int WINAPI  WlxAssignShellProtection(HANDLE, HANDLE, HANDLE, HANDLE);
int WINAPI  WlxMessageBox(HANDLE, HWND, LPWSTR, LPWSTR, UINT);
int WINAPI  WlxDialogBox(HANDLE, HANDLE, LPWSTR, HWND, DLGPROC);
int WINAPI  WlxDialogBoxIndirect(HANDLE, HANDLE, LPCDLGTEMPLATE, HWND, DLGPROC);
int WINAPI  WlxDialogBoxParam(HANDLE, HANDLE, LPWSTR, HWND, DLGPROC, LPARAM);
int WINAPI  WlxDialogBoxIndirectParam(HANDLE, HANDLE, LPCDLGTEMPLATE, HWND, DLGPROC, LPARAM);
int WINAPI  WlxSwitchDesktopToUser(HANDLE);
int WINAPI  WlxSwitchDesktopToWinlogon(HANDLE);
int WINAPI  WlxChangePasswordNotify(HANDLE, PWLX_MPR_NOTIFY_INFO, DWORD);
BOOL WINAPI WlxGetSourceDesktop(HANDLE, PWLX_DESKTOP *);
BOOL WINAPI WlxSetReturnDesktop(HANDLE, PWLX_DESKTOP);
BOOL WINAPI WlxCreateUserDesktop(HANDLE, HANDLE, DWORD, PWSTR, PWLX_DESKTOP *);
int WINAPI WlxChangePasswordNotifyEx(HANDLE, PWLX_MPR_NOTIFY_INFO, DWORD, PWSTR, PVOID);
BOOL WINAPI WlxCloseUserDesktop(HANDLE, PWLX_DESKTOP, HANDLE);
BOOL WINAPI WlxSetOption(HANDLE hWlx, DWORD Option, ULONG_PTR Value, ULONG_PTR * OldValue);
BOOL WINAPI WlxGetOption(HANDLE hWlx, DWORD Option, ULONG_PTR * Value);
VOID WINAPI WlxWin31Migrate( HANDLE  hWlx);
BOOL WINAPI WlxQueryClientCredentials( PWLX_CLIENT_CREDENTIALS_INFO_V1_0 );
BOOL WINAPI WlxQueryInetConnectorCredentials( PWLX_CLIENT_CREDENTIALS_INFO_V1_0 );
BOOL WINAPI WlxDisconnect(void);
DWORD WINAPI WlxQueryTerminalServicesData(HANDLE, PWLX_TERMINAL_SERVICES_DATA, WCHAR *, WCHAR *);

DWORD WINAPI QueryTerminalServicesDataWorker(PTERMINAL, WCHAR *, WCHAR *);
BOOL StartApplication(PTERMINAL, PWSTR, PVOID, PWSTR);
VOID QueryVerboseStatus(VOID);
VOID StatusMessage(BOOL bVerbose, DWORD dwOptions, UINT idMsg, ...);
DWORD StatusMessage2(BOOL bVerbose, LPWSTR lpMessage);
VOID RemoveStatusMessage(BOOL bForce);
VOID WaitForServices(PTERMINAL pTerm);

BOOL
InitializeAutoLogonInfo(
    PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pInfo,
    LPTSTR pUserName,
    LPTSTR pDomain,
    LPTSTR pPassword,
    BOOL fPromptForPassword
    );

extern  WLX_DISPATCH_VERSION_1_3    WlxDispatchTable;


void
SASRouter(  PTERMINAL   pTerm,
            DWORD       SasType );

BOOL
SendSasToTopWindow(
    PTERMINAL    pTerm,
    DWORD       SasType);

VOID
ChangeStateForSAS(PTERMINAL  pTerm);

#define MAPPERFLAG_WINLOGON     8
BOOL
SetMapperFlag(
    HWND    hWnd,
    DWORD   Flag,
    PTERMINAL pTerm
    );

VOID
DestroyMprInfo(
    PWLX_MPR_NOTIFY_INFO    pMprInfo);

DWORD
LogoffFlagsToWlxCode(DWORD Flags);

#endif
