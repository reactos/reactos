/*
 * WinWlx.h
 *
 * WinLogon eXtension
 *
 * Contributors:
 *   Created by Thomas Weidenmueller <w3seek@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __WINWLX_H
#define __WINWLX_H

#ifdef __cplusplus
extern "C" {
#endif

#define WLX_VERSION_1_0     0x00010000
#define WLX_VERSION_1_1     0x00010001
#define WLX_VERSION_1_2     0x00010002
#define WLX_VERSION_1_3     0x00010003
#define WLX_VERSION_1_4     0x00010004
#define WLX_CURRENT_VERSION (WLX_VERSION_1_4)

#define WLX_SAS_TYPE_TIMEOUT                 0x0000
#define WLX_SAS_TYPE_CTRL_ALT_DEL            0x0001
#define WLX_SAS_TYPE_SCRNSVR_TIMEOUT         0x0002
#define WLX_SAS_TYPE_SCRNSVR_ACTIVITY        0x0003
#define WLX_SAS_TYPE_USER_LOGOFF             0x0004
#define WLX_SAS_TYPE_SC_INSERT               0x0005
#define WLX_SAS_TYPE_SC_REMOVE               0x0006
#define WLX_SAS_TYPE_AUTHENTICATED           0x0007
#define WLX_SAS_TYPE_SC_FIRST_READER_ARRIVED 0x0008
#define WLX_SAS_TYPE_SC_LAST_READER_REMOVED  0x0009
#define WLX_SAS_TYPE_SWITCHUSER              0x000A
#define WLX_SAS_TYPE_MAX_MSFT_VALUE          0x007F

#define WLX_LOGON_OPT_NO_PROFILE 0x0001

#define WLX_PROFILE_TYPE_V1_0 0x0001
#define WLX_PROFILE_TYPE_V2_0 0x0002 /* Default */

#define WLX_SAS_ACTION_LOGON                0x0001
#define WLX_SAS_ACTION_NONE                 0x0002
#define WLX_SAS_ACTION_LOCK_WKSTA           0x0003
#define WLX_SAS_ACTION_LOGOFF               0x0004
#define WLX_SAS_ACTION_SHUTDOWN             0x0005
#define WLX_SAS_ACTION_PWD_CHANGED          0x0006
#define WLX_SAS_ACTION_TASKLIST             0x0007
#define WLX_SAS_ACTION_UNLOCK_WKSTA         0x0008
#define WLX_SAS_ACTION_FORCE_LOGOFF         0x0009
#define WLX_SAS_ACTION_SHUTDOWN_POWER_OFF   0x000A
#define WLX_SAS_ACTION_SHUTDOWN_REBOOT      0x000B
#define WLX_SAS_ACTION_SHUTDOWN_SLEEP       0x000C
#define WLX_SAS_ACTION_SHUTDOWN_SLEEP2      0x000D
#define WLX_SAS_ACTION_SHUTDOWN_HIBERNATE   0x000E
#define WLX_SAS_ACTION_RECONNECTED          0x000F
#define WLX_SAS_ACTION_DELAYED_FORCE_LOGOFF 0x0010
#define WLX_SAS_ACTION_SWITCH_CONSOLE       0x0011

#define WLX_WM_SAS (WM_USER + 0x0259)

#define WLX_DLG_SAS                  0x0065
#define WLX_DLG_INPUT_TIMEOUT        0x0066 /* Input (keys, ...) timed out */
#define WLX_DLG_SCREEN_SAVER_TIMEOUT 0x0067 /* Screen saver activated */
#define WLX_DLG_USER_LOGOFF          0x0068 /* User logged off */

#define WLX_DIRECTORY_LENGTH 0x0100

#define WLX_CREDENTIAL_TYPE_V1_0 0x0001
#define WLX_CREDENTIAL_TYPE_V2_0 0x0002

#define WLX_CONSOLESWITCHCREDENTIAL_TYPE_V1_0 0x0001

#define STATUSMSG_OPTION_NOANIMATION   0x0001
#define STATUSMSG_OPTION_SETFOREGROUND 0x0002

typedef struct _WLX_CLIENT_CREDENTIALS_INFO {
  DWORD dwType;
  PWSTR pszUserName;
  PWSTR pszDomain;
  PWSTR pszPassword;
  BOOL fPromptForPassword;
} WLX_CLIENT_CREDENTIALS_INFO_V1_0, *PWLX_CLIENT_CREDENTIALS_INFO_V1_0;

typedef struct _WLX_CLIENT_CREDENTIALS_INFO_2_0 {
  DWORD dwType;
  PWSTR pszUserName;
  PWSTR pszDomain;
  PWSTR pszPassword;
  BOOL fPromptForPassword;
  BOOL fDisconnectOnLogonFailure;
} WLX_CLIENT_CREDENTIALS_INFO_V2_0, *PWLX_CLIENT_CREDENTIALS_INFO_V2_0;

typedef struct _WLX_CONSOLESWITCH_CREDENTIALS_INFO {
  DWORD dwType;
  HANDLE UserToken;
  LUID LogonId;
  QUOTA_LIMITS  Quotas;
  PWSTR UserName;
  PWSTR Domain;
  LARGE_INTEGER LogonTime;
  BOOL SmartCardLogon;
  ULONG ProfileLength;
  DWORD MessageType;
  USHORT LogonCount;
  USHORT BadPasswordCount;
  LARGE_INTEGER ProfileLogonTime;
  LARGE_INTEGER LogoffTime;
  LARGE_INTEGER KickOffTime;
  LARGE_INTEGER PasswordLastSet;
  LARGE_INTEGER PasswordCanChange;
  LARGE_INTEGER PasswordMustChange;
  PWSTR LogonScript;
  PWSTR HomeDirectory;
  PWSTR FullName;
  PWSTR ProfilePath;
  PWSTR HomeDirectoryDrive;
  PWSTR LogonServer;
  ULONG UserFlags;
  ULONG PrivateDataLen;
  PBYTE PrivateData;
} WLX_CONSOLESWITCH_CREDENTIALS_INFO_V1_0, *PWLX_CONSOLESWITCH_CREDENTIALS_INFO_V1_0;

typedef struct _WLX_MPR_NOTIFY_INFO {
  PWSTR pszUserName;
  PWSTR pszDomain;
  PWSTR pszPassword;
  PWSTR pszOldPassword;
} WLX_MPR_NOTIFY_INFO, *PWLX_MPR_NOTIFY_INFO;

typedef struct _WLX_PROFILE_V1_0 {
  DWORD dwType;
  PWSTR pszProfile;
} WLX_PROFILE_V1_0, *PWLX_PROFILE_V1_0;

typedef struct _WLX_PROFILE_V2_0 {
  DWORD dwType;
  PWSTR pszProfile;
  PWSTR pszPolicy;
  PWSTR pszNetworkDefaultUserProfile;
  PWSTR pszServerName;
  PWSTR pszEnvironment;
} WLX_PROFILE_V2_0, *PWLX_PROFILE_V2_0;

typedef struct _WLX_SC_NOTIFICATION_INFO {
  PWSTR pszCard;
  PWSTR pszReader;
  PWSTR pszContainer;
  PWSTR pszCryptoProvider;
} WLX_SC_NOTIFICATION_INFO, *PWLX_SC_NOTIFICATION_INFO;

typedef struct _WLX_TERMINAL_SERVICES_DATA {
  WCHAR ProfilePath[WLX_DIRECTORY_LENGTH + 1];
  WCHAR HomeDir[WLX_DIRECTORY_LENGTH + 1];
  WCHAR HomeDirDrive[4];
} WLX_TERMINAL_SERVICES_DATA, *PWLX_TERMINAL_SERVICES_DATA;

/* GINA Version 1.0 */

BOOL
WINAPI
WlxActivateUserShell(
  _In_ PVOID pWlxContext,
  _In_ PWSTR pszDesktopName,
  _In_ PWSTR pszMprLogonScript,
  _In_ PVOID pEnvironment);

VOID
WINAPI
WlxDisplayLockedNotice(
  _In_ PVOID pWlxContext);

VOID
WINAPI
WlxDisplaySASNotice(
  _In_ PVOID pWlxContext);

BOOL
WINAPI
WlxInitialize(
  _In_ LPWSTR lpWinsta,
  _In_ HANDLE hWlx,
  _In_ PVOID pvReserved,
  _In_ PVOID pWinlogonFunctions,
  _Out_ PVOID *pWlxContext);

BOOL
WINAPI
WlxIsLockOk(
  _In_ PVOID pWlxContext);

BOOL
WINAPI
WlxIsLogoffOk(
  _In_ PVOID pWlxContext);

INT
WINAPI
WlxLoggedOnSAS(
  _In_ PVOID pWlxContext,
  _In_ DWORD dwSasType,
  _In_ PVOID pReserved);

INT
WINAPI
WlxLoggedOutSAS(
  _In_ PVOID pWlxContext,
  _In_ DWORD dwSasType,
  _Out_ PLUID pAuthenticationId,
  _Inout_ PSID pLogonSid,
  _Out_ PDWORD pdwOptions,
  _Out_ PHANDLE phToken,
  _Out_ PWLX_MPR_NOTIFY_INFO pNprNotifyInfo,
  _Out_ PVOID *pProfile);

VOID
WINAPI
WlxLogoff(
  _In_ PVOID pWlxContext);

BOOL
WINAPI
WlxNegotiate(
  _In_ DWORD dwWinlogonVersion,
  _Out_ PDWORD pdwDllVersion);

VOID
WINAPI
WlxShutdown(
  _In_ PVOID pWlxContext,
  _In_ DWORD ShutdownType);

INT
WINAPI
WlxWkstaLockedSAS(
  _In_ PVOID pWlxContext,
  _In_ DWORD dwSasType);

/* GINA Version 1.1 */

BOOL
WINAPI
WlxScreenSaverNotify(
  _In_ PVOID pWlxContext,
  _Inout_ BOOL *pSecure);

BOOL
WINAPI
WlxStartApplication(
  _In_ PVOID pWlxContext,
  _In_ PWSTR pszDesktopName,
  _In_ PVOID pEnvironment,
  _In_ PWSTR pszCmdLine);

/* GINA Version 1.3 */

BOOL
WINAPI
WlxDisplayStatusMessage(
  _In_ PVOID pWlxContext,
  _In_ HDESK hDesktop,
  _In_ DWORD dwOptions,
  _In_ PWSTR pTitle,
  _In_ PWSTR pMessage);

BOOL
WINAPI
WlxGetStatusMessage(
  _In_ PVOID pWlxContext,
  _Out_ DWORD *pdwOptions,
  _Out_ PWSTR pMessage,
  _In_ DWORD dwBufferSize);

BOOL
WINAPI
WlxNetworkProviderLoad(
  _In_ PVOID pWlxContext,
  _Out_ PWLX_MPR_NOTIFY_INFO pNprNotifyInfo);

BOOL
WINAPI
WlxRemoveStatusMessage(
  _In_ PVOID pWlxContext);

/* GINA Version 1.4 */

VOID
WINAPI
WlxDisconnectNotify(
  _In_ PVOID pWlxContext);

BOOL
WINAPI
WlxGetConsoleSwitchCredentials(
  _In_ PVOID pWlxContext,
  _Out_ PVOID pCredInfo);

VOID
WINAPI
WlxReconnectNotify(
  _In_ PVOID pWlxContext);

/* Services that Winlogon provides */

#define WLX_DESKTOP_NAME   0x0001 /* Name present */
#define WLX_DESKTOP_HANDLE 0x0002 /* Handle present */

#define WLX_CREATE_INSTANCE_ONLY 0x0001
#define WLX_CREATE_USER          0x0002

#define WLX_OPTION_USE_CTRL_ALT_DEL  0x0001
#define WLX_OPTION_CONTEXT_POINTER   0x0002
#define WLX_OPTION_USE_SMART_CARD    0x0003
#define WLX_OPTION_FORCE_LOGOFF_TIME 0x0004
#define WLX_OPTION_IGNORE_AUTO_LOGON 0x0008
#define WLX_OPTION_NO_SWITCH_ON_SAS  0x0009

#define WLX_OPTION_SMART_CARD_PRESENT  0x00010001
#define WLX_OPTION_SMART_CARD_INFO     0x00010002
#define WLX_OPTION_DISPATCH_TABLE_SIZE 0x00010003

typedef struct _WLX_DESKTOP {
  DWORD Size;
  DWORD Flags;
  HDESK hDesktop;
  PWSTR pszDesktopName;
} WLX_DESKTOP, *PWLX_DESKTOP;

typedef INT
(WINAPI *PWLX_ASSIGN_SHELL_PROTECTION)(
  _In_ HANDLE hWlx,
  _In_ HANDLE hToken,
  _In_ HANDLE hProcess,
  _In_ HANDLE hThread);

typedef INT
(WINAPI *PWLX_CHANGE_PASSWORD_NOTIFY)(
  _In_ HANDLE hWlx,
  _In_ PWLX_MPR_NOTIFY_INFO pMprInfo,
  _In_ DWORD dwChangeInfo);

typedef INT
(WINAPI *PWLX_CHANGE_PASSWORD_NOTIFY_EX)(
  _In_ HANDLE hWlx,
  _In_ PWLX_MPR_NOTIFY_INFO pMprInfo,
  _In_ DWORD dwChangeInfo,
  _In_ PWSTR ProviderName,
  _In_ PVOID Reserved);

typedef BOOL
(WINAPI *PWLX_CLOSE_USER_DESKTOP)(
  _In_ HANDLE hWlx,
  _In_ PWLX_DESKTOP pDesktop,
  _In_ HANDLE hToken);

typedef BOOL
(WINAPI *PWLX_CREATE_USER_DESKTOP)(
  _In_ HANDLE hWlx,
  _In_ HANDLE hToken,
  _In_ DWORD Flags,
  _In_ PWSTR pszDesktopName,
  _Out_ PWLX_DESKTOP *ppDesktop);

typedef INT
(WINAPI *PWLX_DIALOG_BOX)(
  _In_ HANDLE hWlx,
  _In_ HANDLE hInst,
  _In_ LPWSTR lpszTemplate,
  _In_ HWND hwndOwner,
  _In_ DLGPROC dlgprc);

typedef INT
(WINAPI *PWLX_DIALOG_BOX_INDIRECT)(
  _In_ HANDLE hWlx,
  _In_ HANDLE hInst,
  _In_ LPCDLGTEMPLATE hDialogTemplate,
  _In_ HWND hwndOwner,
  _In_ DLGPROC dlgprc);

typedef INT
(WINAPI *PWLX_DIALOG_BOX_INDIRECT_PARAM)(
  _In_ HANDLE hWlx,
  _In_ HANDLE hInst,
  _In_ LPCDLGTEMPLATE hDialogTemplate,
  _In_ HWND hwndOwner,
  _In_ DLGPROC dlgprc,
  _In_ LPARAM dwInitParam);

typedef INT
(WINAPI *PWLX_DIALOG_BOX_PARAM)(
  _In_ HANDLE hWlx,
  _In_ HANDLE hInst,
  _In_ LPWSTR lpszTemplate,
  _In_ HWND hwndOwner,
  _In_ DLGPROC dlgprc,
  _In_ LPARAM dwInitParam);

typedef BOOL
(WINAPI *PWLX_DISCONNECT)(VOID);

typedef BOOL
(WINAPI *PWLX_GET_OPTION)(
  _In_ HANDLE hWlx,
  _In_ DWORD Option,
  _Out_ ULONG_PTR *Value);

typedef BOOL
(WINAPI *PWLX_GET_SOURCE_DESKTOP)(
  _In_ HANDLE hWlx,
  _Out_ PWLX_DESKTOP *ppDesktop);

typedef INT
(WINAPI *PWLX_MESSAGE_BOX)(
  _In_ HANDLE hWlx,
  _In_ HWND hwndOwner,
  _In_ LPWSTR lpszText,
  _In_ LPWSTR lpszTitle,
  _In_ UINT fuStyle);

typedef BOOL
(WINAPI *PWLX_QUERY_CLIENT_CREDENTIALS)(
  _Out_ PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pCred);

typedef DWORD
(WINAPI *PWLX_QUERY_CONSOLESWITCH_CREDENTIALS)(
  _Out_ PWLX_CONSOLESWITCH_CREDENTIALS_INFO_V1_0 pCred);

typedef BOOL
(WINAPI *PWLX_QUERY_IC_CREDENTIALS)(
  _Out_ PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pCred);

typedef DWORD
(WINAPI *PWLX_QUERY_TERMINAL_SERVICES_DATA)(
  _In_ HANDLE hWlx,
  _Out_ PWLX_TERMINAL_SERVICES_DATA pTSData,
  _In_ WCHAR *UserName,
  _In_ WCHAR *Domain);

typedef BOOL
(WINAPI *PWLX_QUERY_TS_LOGON_CREDENTIALS)(
  _Out_ PWLX_CLIENT_CREDENTIALS_INFO_V2_0 pCred);

typedef VOID
(WINAPI *PWLX_SAS_NOTIFY)(
  _In_ HANDLE hWlx,
  _In_ DWORD dwSasType);

typedef VOID
(WINAPI *PWLX_SET_CONTEXT_POINTER)(
  _In_ HANDLE hWlx,
  _In_ PVOID pWlxContext);

typedef BOOL
(WINAPI *PWLX_SET_OPTION)(
  _In_ HANDLE hWlx,
  _In_ DWORD Option,
  _In_ ULONG_PTR Value,
  _Out_ ULONG_PTR *OldValue);

typedef BOOL
(WINAPI *PWLX_SET_RETURN_DESKTOP)(
  _In_ HANDLE hWlx,
  _In_ PWLX_DESKTOP pDesktop);

typedef BOOL
(WINAPI *PWLX_SET_TIMEOUT)(
  _In_ HANDLE hWlx,
  _In_ DWORD Timeout);

typedef INT
(WINAPI *PWLX_SWITCH_DESKTOP_TO_USER)(
  _In_ HANDLE hWlx);

typedef INT
(WINAPI *PWLX_SWITCH_DESKTOP_TO_WINLOGON)(
  _In_ HANDLE hWlx);

typedef VOID
(WINAPI *PWLX_USE_CTRL_ALT_DEL)(
  _In_ HANDLE hWlx);

typedef VOID
(WINAPI *PWLX_WIN31_MIGRATE)(
  _In_ HANDLE hWlx);

/* Function dispatch tables */

typedef struct _WLX_DISPATCH_VERSION_1_0 {
  PWLX_USE_CTRL_ALT_DEL WlxUseCtrlAltDel;
  PWLX_SET_CONTEXT_POINTER WlxSetContextPointer;
  PWLX_SAS_NOTIFY WlxSasNotify;
  PWLX_SET_TIMEOUT WlxSetTimeout;
  PWLX_ASSIGN_SHELL_PROTECTION WlxAssignShellProtection;
  PWLX_MESSAGE_BOX WlxMessageBox;
  PWLX_DIALOG_BOX WlxDialogBox;
  PWLX_DIALOG_BOX_PARAM WlxDialogBoxParam;
  PWLX_DIALOG_BOX_INDIRECT WlxDialogBoxIndirect;
  PWLX_DIALOG_BOX_INDIRECT_PARAM WlxDialogBoxIndirectParam;
  PWLX_SWITCH_DESKTOP_TO_USER WlxSwitchDesktopToUser;
  PWLX_SWITCH_DESKTOP_TO_WINLOGON WlxSwitchDesktopToWinlogon;
  PWLX_CHANGE_PASSWORD_NOTIFY WlxChangePasswordNotify;
} WLX_DISPATCH_VERSION_1_0, *PWLX_DISPATCH_VERSION_1_0;

typedef struct _WLX_DISPATCH_VERSION_1_1 {
  PWLX_USE_CTRL_ALT_DEL WlxUseCtrlAltDel;
  PWLX_SET_CONTEXT_POINTER WlxSetContextPointer;
  PWLX_SAS_NOTIFY WlxSasNotify;
  PWLX_SET_TIMEOUT WlxSetTimeout;
  PWLX_ASSIGN_SHELL_PROTECTION WlxAssignShellProtection;
  PWLX_MESSAGE_BOX WlxMessageBox;
  PWLX_DIALOG_BOX WlxDialogBox;
  PWLX_DIALOG_BOX_PARAM WlxDialogBoxParam;
  PWLX_DIALOG_BOX_INDIRECT WlxDialogBoxIndirect;
  PWLX_DIALOG_BOX_INDIRECT_PARAM WlxDialogBoxIndirectParam;
  PWLX_SWITCH_DESKTOP_TO_USER WlxSwitchDesktopToUser;
  PWLX_SWITCH_DESKTOP_TO_WINLOGON WlxSwitchDesktopToWinlogon;
  PWLX_CHANGE_PASSWORD_NOTIFY WlxChangePasswordNotify;
  PWLX_GET_SOURCE_DESKTOP WlxGetSourceDesktop;
  PWLX_SET_RETURN_DESKTOP WlxSetReturnDesktop;
  PWLX_CREATE_USER_DESKTOP WlxCreateUserDesktop;
  PWLX_CHANGE_PASSWORD_NOTIFY_EX WlxChangePasswordNotifyEx;
} WLX_DISPATCH_VERSION_1_1, *PWLX_DISPATCH_VERSION_1_1;

typedef struct _WLX_DISPATCH_VERSION_1_2 {
  PWLX_USE_CTRL_ALT_DEL WlxUseCtrlAltDel;
  PWLX_SET_CONTEXT_POINTER WlxSetContextPointer;
  PWLX_SAS_NOTIFY WlxSasNotify;
  PWLX_SET_TIMEOUT WlxSetTimeout;
  PWLX_ASSIGN_SHELL_PROTECTION WlxAssignShellProtection;
  PWLX_MESSAGE_BOX WlxMessageBox;
  PWLX_DIALOG_BOX WlxDialogBox;
  PWLX_DIALOG_BOX_PARAM WlxDialogBoxParam;
  PWLX_DIALOG_BOX_INDIRECT WlxDialogBoxIndirect;
  PWLX_DIALOG_BOX_INDIRECT_PARAM WlxDialogBoxIndirectParam;
  PWLX_SWITCH_DESKTOP_TO_USER WlxSwitchDesktopToUser;
  PWLX_SWITCH_DESKTOP_TO_WINLOGON WlxSwitchDesktopToWinlogon;
  PWLX_CHANGE_PASSWORD_NOTIFY WlxChangePasswordNotify;
  PWLX_GET_SOURCE_DESKTOP WlxGetSourceDesktop;
  PWLX_SET_RETURN_DESKTOP WlxSetReturnDesktop;
  PWLX_CREATE_USER_DESKTOP WlxCreateUserDesktop;
  PWLX_CHANGE_PASSWORD_NOTIFY_EX WlxChangePasswordNotifyEx;
  PWLX_CLOSE_USER_DESKTOP WlxCloseUserDesktop;
} WLX_DISPATCH_VERSION_1_2, *PWLX_DISPATCH_VERSION_1_2;

typedef struct _WLX_DISPATCH_VERSION_1_3 {
  PWLX_USE_CTRL_ALT_DEL WlxUseCtrlAltDel;
  PWLX_SET_CONTEXT_POINTER WlxSetContextPointer;
  PWLX_SAS_NOTIFY WlxSasNotify;
  PWLX_SET_TIMEOUT WlxSetTimeout;
  PWLX_ASSIGN_SHELL_PROTECTION WlxAssignShellProtection;
  PWLX_MESSAGE_BOX WlxMessageBox;
  PWLX_DIALOG_BOX WlxDialogBox;
  PWLX_DIALOG_BOX_PARAM WlxDialogBoxParam;
  PWLX_DIALOG_BOX_INDIRECT WlxDialogBoxIndirect;
  PWLX_DIALOG_BOX_INDIRECT_PARAM WlxDialogBoxIndirectParam;
  PWLX_SWITCH_DESKTOP_TO_USER WlxSwitchDesktopToUser;
  PWLX_SWITCH_DESKTOP_TO_WINLOGON WlxSwitchDesktopToWinlogon;
  PWLX_CHANGE_PASSWORD_NOTIFY WlxChangePasswordNotify;
  PWLX_GET_SOURCE_DESKTOP WlxGetSourceDesktop;
  PWLX_SET_RETURN_DESKTOP WlxSetReturnDesktop;
  PWLX_CREATE_USER_DESKTOP WlxCreateUserDesktop;
  PWLX_CHANGE_PASSWORD_NOTIFY_EX WlxChangePasswordNotifyEx;
  PWLX_CLOSE_USER_DESKTOP WlxCloseUserDesktop;
  PWLX_SET_OPTION WlxSetOption;
  PWLX_GET_OPTION WlxGetOption;
  PWLX_WIN31_MIGRATE WlxWin31Migrate;
  PWLX_QUERY_CLIENT_CREDENTIALS WlxQueryClientCredentials;
  PWLX_QUERY_IC_CREDENTIALS WlxQueryInetConnectorCredentials;
  PWLX_DISCONNECT WlxDisconnect;
  PWLX_QUERY_TERMINAL_SERVICES_DATA WlxQueryTerminalServicesData;
} WLX_DISPATCH_VERSION_1_3, *PWLX_DISPATCH_VERSION_1_3;

typedef struct _WLX_DISPATCH_VERSION_1_4 {
  PWLX_USE_CTRL_ALT_DEL WlxUseCtrlAltDel;
  PWLX_SET_CONTEXT_POINTER WlxSetContextPointer;
  PWLX_SAS_NOTIFY WlxSasNotify;
  PWLX_SET_TIMEOUT WlxSetTimeout;
  PWLX_ASSIGN_SHELL_PROTECTION WlxAssignShellProtection;
  PWLX_MESSAGE_BOX WlxMessageBox;
  PWLX_DIALOG_BOX WlxDialogBox;
  PWLX_DIALOG_BOX_PARAM WlxDialogBoxParam;
  PWLX_DIALOG_BOX_INDIRECT WlxDialogBoxIndirect;
  PWLX_DIALOG_BOX_INDIRECT_PARAM WlxDialogBoxIndirectParam;
  PWLX_SWITCH_DESKTOP_TO_USER WlxSwitchDesktopToUser;
  PWLX_SWITCH_DESKTOP_TO_WINLOGON WlxSwitchDesktopToWinlogon;
  PWLX_CHANGE_PASSWORD_NOTIFY WlxChangePasswordNotify;
  PWLX_GET_SOURCE_DESKTOP WlxGetSourceDesktop;
  PWLX_SET_RETURN_DESKTOP WlxSetReturnDesktop;
  PWLX_CREATE_USER_DESKTOP WlxCreateUserDesktop;
  PWLX_CHANGE_PASSWORD_NOTIFY_EX WlxChangePasswordNotifyEx;
  PWLX_CLOSE_USER_DESKTOP WlxCloseUserDesktop;
  PWLX_SET_OPTION WlxSetOption;
  PWLX_GET_OPTION WlxGetOption;
  PWLX_WIN31_MIGRATE WlxWin31Migrate;
  PWLX_QUERY_CLIENT_CREDENTIALS WlxQueryClientCredentials;
  PWLX_QUERY_IC_CREDENTIALS WlxQueryInetConnectorCredentials;
  PWLX_DISCONNECT WlxDisconnect;
  PWLX_QUERY_TERMINAL_SERVICES_DATA WlxQueryTerminalServicesData;
  PWLX_QUERY_CONSOLESWITCH_CREDENTIALS WlxQueryConsoleSwitchCredentials;
  PWLX_QUERY_TS_LOGON_CREDENTIALS WlxQueryTsLogonCredentials;
} WLX_DISPATCH_VERSION_1_4, *PWLX_DISPATCH_VERSION_1_4;

/* Non-GINA notification DLLs */

typedef DWORD
(CALLBACK *PFNMSGECALLBACK)(
  _In_ BOOL bVerbose,
  _In_ LPWSTR lpMessage);

typedef struct _WLX_NOTIFICATION_INFO {
  ULONG Size;
  ULONG Flags;
  PWSTR UserName;
  PWSTR Domain;
  PWSTR WindowStation;
  HANDLE hToken;
  HDESK hDesktop;
  PFNMSGECALLBACK pStatusCallback;
} WLX_NOTIFICATION_INFO, *PWLX_NOTIFICATION_INFO;

#ifdef __cplusplus
}
#endif

#endif /*__WINWLX_H */
