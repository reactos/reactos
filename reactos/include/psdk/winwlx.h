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


#define WLX_VERSION_1_0				(0x00010000)
#define WLX_VERSION_1_1				(0x00010001)
#define WLX_VERSION_1_2				(0x00010002)
#define WLX_VERSION_1_3				(0x00010003)
#define WLX_VERSION_1_4				(0x00010004)
#define WLX_CURRENT_VERSION			(WLX_VERSION_1_4)

#define WLX_SAS_TYPE_TIMEOUT			(0x0000)
#define WLX_SAS_TYPE_CTRL_ALT_DEL		(0x0001)
#define WLX_SAS_TYPE_SCRNSVR_TIMEOUT		(0x0002)
#define WLX_SAS_TYPE_SCRNSVR_ACTIVITY		(0x0003)
#define WLX_SAS_TYPE_USER_LOGOFF		(0x0004)
#define WLX_SAS_TYPE_SC_INSERT			(0x0005)
#define WLX_SAS_TYPE_SC_REMOVE			(0x0006)
#define WLX_SAS_TYPE_AUTHENTICATED		(0x0007)
#define WLX_SAS_TYPE_SC_FIRST_READER_ARRIVED	(0x0008)
#define WLX_SAS_TYPE_SC_LAST_READER_REMOVED	(0x0009)
#define WLX_SAS_TYPE_SWITCHUSER			(0x000A)
#define WLX_SAS_TYPE_MAX_MSFT_VALUE		(0x007F)

#define WLX_LOGON_OPT_NO_PROFILE		(0x0001)

#define WLX_PROFILE_TYPE_V1_0			(0x0001)
#define WLX_PROFILE_TYPE_V2_0			(0x0002) /* default */

#define WLX_SAS_ACTION_LOGON			(0x0001)
#define WLX_SAS_ACTION_NONE			(0x0002)
#define WLX_SAS_ACTION_LOCK_WKSTA		(0x0003)
#define WLX_SAS_ACTION_LOGOFF			(0x0004)
#define WLX_SAS_ACTION_SHUTDOWN			(0x0005)
#define WLX_SAS_ACTION_PWD_CHANGED		(0x0006)
#define WLX_SAS_ACTION_TASKLIST			(0x0007)
#define WLX_SAS_ACTION_UNLOCK_WKSTA		(0x0008)
#define WLX_SAS_ACTION_FORCE_LOGOFF		(0x0009)
#define WLX_SAS_ACTION_SHUTDOWN_POWER_OFF	(0x000A)
#define WLX_SAS_ACTION_SHUTDOWN_REBOOT		(0x000B)
#define WLX_SAS_ACTION_SHUTDOWN_SLEEP		(0x000C)
#define WLX_SAS_ACTION_SHUTDOWN_SLEEP2		(0x000D)
#define WLX_SAS_ACTION_SHUTDOWN_HIBERNATE	(0x000E)
#define WLX_SAS_ACTION_RECONNECTED		(0x000F)
#define WLX_SAS_ACTION_DELAYED_FORCE_LOGOFF	(0x0010)
#define WLX_SAS_ACTION_SWITCH_CONSOLE		(0x0011)

#define WLX_WM_SAS				(WM_USER + 0x0259)

#define WLX_DLG_SAS				(0x0065)
#define WLX_DLG_INPUT_TIMEOUT			(0x0066) /* Input (keys, ...) timed out */
#define WLX_DLG_SCREEN_SAVER_TIMEOUT		(0x0067) /* Screen saver activated */
#define WLX_DLG_USER_LOGOFF			(0x0068) /* User logged off */

#define WLX_DIRECTORY_LENGTH			(0x0100)

#define WLX_CREDENTIAL_TYPE_V1_0		(0x0001)
#define WLX_CREDENTIAL_TYPE_V2_0		(0x0002)

#define WLX_CONSOLESWITCHCREDENTIAL_TYPE_V1_0	(0x0001)

#define STATUSMSG_OPTION_NOANIMATION		(0x0001)
#define STATUSMSG_OPTION_SETFOREGROUND		(0x0002)


typedef
struct _WLX_CLIENT_CREDENTIALS_INFO
{
	DWORD dwType;
	PWSTR pszUserName;
	PWSTR pszDomain;
	PWSTR pszPassword;
	BOOL  fPromptForPassword;
} WLX_CLIENT_CREDENTIALS_INFO_V1_0, *PWLX_CLIENT_CREDENTIALS_INFO_V1_0;

typedef
struct _WLX_CLIENT_CREDENTIALS_INFO_2_0
{
	DWORD dwType;
	PWSTR pszUserName;
	PWSTR pszDomain;
	PWSTR pszPassword;
	BOOL  fPromptForPassword;
	BOOL  fDisconnectOnLogonFailure;
} WLX_CLIENT_CREDENTIALS_INFO_V2_0, *PWLX_CLIENT_CREDENTIALS_INFO_V2_0;

typedef
struct _WLX_CONSOLESWITCH_CREDENTIALS_INFO
{
	DWORD         dwType;
	HANDLE        UserToken;
	LUID          LogonId;
	QUOTA_LIMITS  Quotas;
	PWSTR         UserName;
	PWSTR         Domain;
	LARGE_INTEGER LogonTime;
	BOOL          SmartCardLogon;
	ULONG         ProfileLength;
	DWORD         MessageType;
	USHORT        LogonCount;
	USHORT        BadPasswordCount;
	LARGE_INTEGER ProfileLogonTime;
	LARGE_INTEGER LogoffTime;
	LARGE_INTEGER KickOffTime;
	LARGE_INTEGER PasswordLastSet;
	LARGE_INTEGER PasswordCanChange;
	LARGE_INTEGER PasswordMustChange;
	PWSTR         LogonScript;
	PWSTR         HomeDirectory;
	PWSTR         FullName;
	PWSTR         ProfilePath;
	PWSTR         HomeDirectoryDrive;
	PWSTR         LogonServer;
	ULONG         UserFlags;
	ULONG         PrivateDataLen;
	PBYTE         PrivateData;
} WLX_CONSOLESWITCH_CREDENTIALS_INFO_V1_0, *PWLX_CONSOLESWITCH_CREDENTIALS_INFO_V1_0;

typedef
struct _WLX_MPR_NOTIFY_INFO
{
	PWSTR pszUserName;
	PWSTR pszDomain;
	PWSTR pszPassword;
	PWSTR pszOldPassword;
} WLX_MPR_NOTIFY_INFO, *PWLX_MPR_NOTIFY_INFO;

typedef
struct _WLX_PROFILE_V1_0
{
	DWORD dwType;
	PWSTR pszProfile;
} WLX_PROFILE_V1_0, *PWLX_PROFILE_V1_0;

typedef
struct _WLX_PROFILE_V2_0
{
	DWORD dwType;
	PWSTR pszProfile;
	PWSTR pszPolicy;
	PWSTR pszNetworkDefaultUserProfile;
	PWSTR pszServerName;
	PWSTR pszEnvironment;
} WLX_PROFILE_V2_0, *PWLX_PROFILE_V2_0;

typedef
struct _WLX_SC_NOTIFICATION_INFO
{
	PWSTR pszCard;
	PWSTR pszReader;
	PWSTR pszContainer;
	PWSTR pszCryptoProvider;
} WLX_SC_NOTIFICATION_INFO, *PWLX_SC_NOTIFICATION_INFO;

typedef
struct _WLX_TERMINAL_SERVICES_DATA
{
	WCHAR ProfilePath[WLX_DIRECTORY_LENGTH + 1];
	WCHAR HomeDir[WLX_DIRECTORY_LENGTH + 1];
	WCHAR HomeDirDrive[4];
} WLX_TERMINAL_SERVICES_DATA, *PWLX_TERMINAL_SERVICES_DATA;


/* GINA Version 1.0 */

BOOL WINAPI
WlxActivateUserShell(
	PVOID pWlxContext,
	PWSTR pszDesktopName,
	PWSTR pszMprLogonScript,
	PVOID pEnvironment);

VOID WINAPI
WlxDisplayLockedNotice(
	PVOID pWlxContext);

VOID WINAPI
WlxDisplaySASNotice(
	PVOID pWlxContext);

BOOL WINAPI
WlxInitialize(
	LPWSTR lpWinsta,
	HANDLE hWlx,
	PVOID  pvReserved,
	PVOID  pWinlogonFunctions,
	PVOID  *pWlxContext);

BOOL WINAPI
WlxIsLockOk(
	PVOID pWlxContext);

BOOL WINAPI
WlxIsLogoffOk(
	PVOID pWlxContext);

int WINAPI
WlxLoggedOnSAS(
	PVOID pWlxContext,
	DWORD dwSasType,
	PVOID pReserved);

int WINAPI
WlxLoggedOutSAS(
	PVOID                pWlxContext,
	DWORD                dwSasType,
	PLUID                pAuthenticationId,
	PSID                 pLogonSid,
	PDWORD               pdwOptions,
	PHANDLE              phToken,
	PWLX_MPR_NOTIFY_INFO pNprNotifyInfo,
	PVOID                *pProfile);

VOID WINAPI
WlxLogoff(
	PVOID pWlxContext);

BOOL WINAPI
WlxNegotiate(
	DWORD  dwWinlogonVersion,
	PDWORD pdwDllVersion);

VOID WINAPI
WlxShutdown(
	PVOID pWlxContext,
	DWORD ShutdownType);

int WINAPI
WlxWkstaLockedSAS(
	PVOID pWlxContext,
	DWORD dwSasType);


/* GINA Version 1.1 */

BOOL WINAPI
WlxScreenSaverNotify(
	PVOID pWlxContext,
	BOOL  *pSecure);

BOOL WINAPI
WlxStartApplication(
	PVOID pWlxContext,
	PWSTR pszDesktopName,
	PVOID pEnvironment,
	PWSTR pszCmdLine);


/* GINA Version 1.3 */

BOOL WINAPI
WlxDisplayStatusMessage(
	PVOID pWlxContext,
	HDESK hDesktop,
	DWORD dwOptions,
	PWSTR pTitle,
	PWSTR pMessage);

BOOL WINAPI
WlxGetStatusMessage(
	PVOID pWlxContext,
	DWORD *pdwOptions,
	PWSTR pMessage,
	DWORD dwBufferSize);

BOOL
WINAPI
WlxNetworkProviderLoad(
	PVOID                pWlxContext,
	PWLX_MPR_NOTIFY_INFO pNprNotifyInfo);

BOOL WINAPI
WlxRemoveStatusMessage(
	PVOID pWlxContext);


/* GINA Version 1.4 */

VOID WINAPI
WlxDisconnectNotify(
	PVOID pWlxContext);

BOOL WINAPI
WlxGetConsoleSwitchCredentials(
	PVOID pWlxContext,
	PVOID pCredInfo);

VOID WINAPI
WlxReconnectNotify(
	PVOID pWlxContext);


/* Services that Winlogon provides */

#define WLX_DESKTOP_NAME			(0x0001) /* Name present */
#define WLX_DESKTOP_HANDLE			(0x0002) /* Handle present */

#define WLX_CREATE_INSTANCE_ONLY		(0x0001)
#define WLX_CREATE_USER				(0x0002)

#define WLX_OPTION_USE_CTRL_ALT_DEL		(0x0001)
#define WLX_OPTION_CONTEXT_POINTER		(0x0002)
#define WLX_OPTION_USE_SMART_CARD		(0x0003)
#define WLX_OPTION_FORCE_LOGOFF_TIME		(0x0004)
#define WLX_OPTION_IGNORE_AUTO_LOGON		(0x0008)
#define WLX_OPTION_NO_SWITCH_ON_SAS		(0x0009)

#define WLX_OPTION_SMART_CARD_PRESENT		(0x00010001)
#define WLX_OPTION_SMART_CARD_INFO		(0x00010002)
#define WLX_OPTION_DISPATCH_TABLE_SIZE		(0x00010003)

typedef
struct _WLX_DESKTOP
{
	DWORD Size;
	DWORD Flags;
	HDESK hDesktop;
	PWSTR pszDesktopName;
} WLX_DESKTOP, *PWLX_DESKTOP;

typedef
int (WINAPI *PWLX_ASSIGN_SHELL_PROTECTION)(
	HANDLE hWlx,
	HANDLE hToken,
	HANDLE hProcess,
	HANDLE hThread);

typedef
BOOL (WINAPI *PWLX_CLOSE_USER_DESKTOP)(
	HANDLE       hWlx,
	PWLX_DESKTOP pDesktop,
	HANDLE       hToken);

typedef
int (WINAPI *PWLX_CHANGE_PASSWORD_NOTIFY)(
	HANDLE               hWlx,
	PWLX_MPR_NOTIFY_INFO pMprInfo,
	DWORD                dwChangeInfo);

typedef
int (WINAPI *PWLX_CHANGE_PASSWORD_NOTIFY_EX)(
	HANDLE               hWlx,
	PWLX_MPR_NOTIFY_INFO pMprInfo,
	DWORD                dwChangeInfo,
	PWSTR                ProviderName,
	PVOID                Reserved);

typedef
BOOL (WINAPI *PWLX_CREATE_USER_DESKTOP)(
	HANDLE       hWlx,
	HANDLE       hToken,
	DWORD        Flags,
	PWSTR        pszDesktopName,
	PWLX_DESKTOP *ppDesktop);

typedef
int (WINAPI *PWLX_DIALOG_BOX)(
	HANDLE  hWlx,
	HANDLE  hInst,
	LPWSTR  lpszTemplate,
	HWND    hwndOwner,
	DLGPROC dlgprc);

typedef
int (WINAPI *PWLX_DIALOG_BOX_INDIRECT)(
	HANDLE         hWlx,
	HANDLE         hInst,
	LPCDLGTEMPLATE hDialogTemplate,
	HWND           hwndOwner,
	DLGPROC        dlgprc);

typedef
int (WINAPI *PWLX_DIALOG_BOX_INDIRECT_PARAM)(
	HANDLE         hWlx,
	HANDLE         hInst,
	LPCDLGTEMPLATE hDialogTemplate,
	HWND           hwndOwner,
	DLGPROC        dlgprc,
	LPARAM         dwInitParam);

typedef
int (WINAPI *PWLX_DIALOG_BOX_PARAM)(
	HANDLE  hWlx,
	HANDLE  hInst,
	LPWSTR  lpszTemplate,
	HWND    hwndOwner,
	DLGPROC dlgprc,
	LPARAM  dwInitParam);

typedef
BOOL (WINAPI *PWLX_DISCONNECT)();

typedef
BOOL (WINAPI *PWLX_GET_OPTION)(
	HANDLE    hWlx,
	DWORD     Option,
	ULONG_PTR *Value);

typedef
BOOL (WINAPI *PWLX_GET_SOURCE_DESKTOP)(
	HANDLE       hWlx,
	PWLX_DESKTOP *ppDesktop);

typedef
int (WINAPI *PWLX_MESSAGE_BOX)(
	HANDLE hWlx,
	HWND   hwndOwner,
	LPWSTR lpszText,
	LPWSTR lpszTitle,
	UINT   fuStyle);

typedef
BOOL (WINAPI *PWLX_QUERY_CLIENT_CREDENTIALS)(
	PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pCred);

typedef
DWORD (WINAPI *PWLX_QUERY_CONSOLESWITCH_CREDENTIALS)(
	PWLX_CONSOLESWITCH_CREDENTIALS_INFO_V1_0 pCred);

typedef
BOOL (WINAPI *PWLX_QUERY_IC_CREDENTIALS)(
	PWLX_CLIENT_CREDENTIALS_INFO_V1_0 pCred);

typedef
DWORD (WINAPI *PWLX_QUERY_TERMINAL_SERVICES_DATA)(
	HANDLE                      hWlx,
	PWLX_TERMINAL_SERVICES_DATA pTSData,
	WCHAR                       *UserName,
	WCHAR                       *Domain);

typedef
BOOL (WINAPI *PWLX_QUERY_TS_LOGON_CREDENTIALS)(
	PWLX_CLIENT_CREDENTIALS_INFO_V2_0 pCred);

typedef
VOID (WINAPI *PWLX_SAS_NOTIFY)(
	HANDLE hWlx,
	DWORD  dwSasType);

typedef
VOID (WINAPI *PWLX_SET_CONTEXT_POINTER)(
	HANDLE hWlx,
	PVOID  pWlxContext);

typedef
BOOL (WINAPI *PWLX_SET_RETURN_DESKTOP)(
	HANDLE       hWlx,
	PWLX_DESKTOP pDesktop);

typedef
BOOL (WINAPI *PWLX_SET_OPTION)(
	HANDLE    hWlx,
	DWORD     Option,
	ULONG_PTR Value,
	ULONG_PTR *OldValue);

typedef
BOOL (WINAPI *PWLX_SET_TIMEOUT)(
	HANDLE hWlx,
	DWORD  Timeout);

typedef
int (WINAPI *PWLX_SWITCH_DESKTOP_TO_USER)(
	HANDLE hWlx);

typedef
int (WINAPI *PWLX_SWITCH_DESKTOP_TO_WINLOGON)(
	HANDLE hWlx);

typedef
VOID (WINAPI *PWLX_USE_CTRL_ALT_DEL)(
	HANDLE hWlx);

typedef
VOID (WINAPI *PWLX_WIN31_MIGRATE)(
	HANDLE hWlx);


/* function dispatch tables */

typedef
struct _WLX_DISPATCH_VERSION_1_0
{
	PWLX_USE_CTRL_ALT_DEL           WlxUseCtrlAltDel;
	PWLX_SET_CONTEXT_POINTER        WlxSetContextPointer;
	PWLX_SAS_NOTIFY                 WlxSasNotify;
	PWLX_SET_TIMEOUT                WlxSetTimeout;
	PWLX_ASSIGN_SHELL_PROTECTION    WlxAssignShellProtection;
	PWLX_MESSAGE_BOX                WlxMessageBox;
	PWLX_DIALOG_BOX                 WlxDialogBox;
	PWLX_DIALOG_BOX_PARAM           WlxDialogBoxParam;
	PWLX_DIALOG_BOX_INDIRECT        WlxDialogBoxIndirect;
	PWLX_DIALOG_BOX_INDIRECT_PARAM  WlxDialogBoxIndirectParam;
	PWLX_SWITCH_DESKTOP_TO_USER     WlxSwitchDesktopToUser;
	PWLX_SWITCH_DESKTOP_TO_WINLOGON WlxSwitchDesktopToWinlogon;
	PWLX_CHANGE_PASSWORD_NOTIFY     WlxChangePasswordNotify;
} WLX_DISPATCH_VERSION_1_0, *PWLX_DISPATCH_VERSION_1_0;

typedef
struct _WLX_DISPATCH_VERSION_1_1
{
	PWLX_USE_CTRL_ALT_DEL           WlxUseCtrlAltDel;
	PWLX_SET_CONTEXT_POINTER        WlxSetContextPointer;
	PWLX_SAS_NOTIFY                 WlxSasNotify;
	PWLX_SET_TIMEOUT                WlxSetTimeout;
	PWLX_ASSIGN_SHELL_PROTECTION    WlxAssignShellProtection;
	PWLX_MESSAGE_BOX                WlxMessageBox;
	PWLX_DIALOG_BOX                 WlxDialogBox;
	PWLX_DIALOG_BOX_PARAM           WlxDialogBoxParam;
	PWLX_DIALOG_BOX_INDIRECT        WlxDialogBoxIndirect;
	PWLX_DIALOG_BOX_INDIRECT_PARAM  WlxDialogBoxIndirectParam;
	PWLX_SWITCH_DESKTOP_TO_USER     WlxSwitchDesktopToUser;
	PWLX_SWITCH_DESKTOP_TO_WINLOGON WlxSwitchDesktopToWinlogon;
	PWLX_CHANGE_PASSWORD_NOTIFY     WlxChangePasswordNotify;
	PWLX_GET_SOURCE_DESKTOP         WlxGetSourceDesktop;
	PWLX_SET_RETURN_DESKTOP         WlxSetReturnDesktop;
	PWLX_CREATE_USER_DESKTOP        WlxCreateUserDesktop;
	PWLX_CHANGE_PASSWORD_NOTIFY_EX  WlxChangePasswordNotifyEx;
} WLX_DISPATCH_VERSION_1_1, *PWLX_DISPATCH_VERSION_1_1;

typedef
struct _WLX_DISPATCH_VERSION_1_2
{
	PWLX_USE_CTRL_ALT_DEL           WlxUseCtrlAltDel;
	PWLX_SET_CONTEXT_POINTER        WlxSetContextPointer;
	PWLX_SAS_NOTIFY                 WlxSasNotify;
	PWLX_SET_TIMEOUT                WlxSetTimeout;
	PWLX_ASSIGN_SHELL_PROTECTION    WlxAssignShellProtection;
	PWLX_MESSAGE_BOX                WlxMessageBox;
	PWLX_DIALOG_BOX                 WlxDialogBox;
	PWLX_DIALOG_BOX_PARAM           WlxDialogBoxParam;
	PWLX_DIALOG_BOX_INDIRECT        WlxDialogBoxIndirect;
	PWLX_DIALOG_BOX_INDIRECT_PARAM  WlxDialogBoxIndirectParam;
	PWLX_SWITCH_DESKTOP_TO_USER     WlxSwitchDesktopToUser;
	PWLX_SWITCH_DESKTOP_TO_WINLOGON WlxSwitchDesktopToWinlogon;
	PWLX_CHANGE_PASSWORD_NOTIFY     WlxChangePasswordNotify;
	PWLX_GET_SOURCE_DESKTOP         WlxGetSourceDesktop;
	PWLX_SET_RETURN_DESKTOP         WlxSetReturnDesktop;
	PWLX_CREATE_USER_DESKTOP        WlxCreateUserDesktop;
	PWLX_CHANGE_PASSWORD_NOTIFY_EX  WlxChangePasswordNotifyEx;
	PWLX_CLOSE_USER_DESKTOP         WlxCloseUserDesktop ;
} WLX_DISPATCH_VERSION_1_2, *PWLX_DISPATCH_VERSION_1_2;

typedef
struct _WLX_DISPATCH_VERSION_1_3
{
	PWLX_USE_CTRL_ALT_DEL             WlxUseCtrlAltDel;
	PWLX_SET_CONTEXT_POINTER          WlxSetContextPointer;
	PWLX_SAS_NOTIFY                   WlxSasNotify;
	PWLX_SET_TIMEOUT                  WlxSetTimeout;
	PWLX_ASSIGN_SHELL_PROTECTION      WlxAssignShellProtection;
	PWLX_MESSAGE_BOX                  WlxMessageBox;
	PWLX_DIALOG_BOX                   WlxDialogBox;
	PWLX_DIALOG_BOX_PARAM             WlxDialogBoxParam;
	PWLX_DIALOG_BOX_INDIRECT          WlxDialogBoxIndirect;
	PWLX_DIALOG_BOX_INDIRECT_PARAM    WlxDialogBoxIndirectParam;
	PWLX_SWITCH_DESKTOP_TO_USER       WlxSwitchDesktopToUser;
	PWLX_SWITCH_DESKTOP_TO_WINLOGON   WlxSwitchDesktopToWinlogon;
	PWLX_CHANGE_PASSWORD_NOTIFY       WlxChangePasswordNotify;
	PWLX_GET_SOURCE_DESKTOP           WlxGetSourceDesktop;
	PWLX_SET_RETURN_DESKTOP           WlxSetReturnDesktop;
	PWLX_CREATE_USER_DESKTOP          WlxCreateUserDesktop;
	PWLX_CHANGE_PASSWORD_NOTIFY_EX    WlxChangePasswordNotifyEx;
	PWLX_CLOSE_USER_DESKTOP           WlxCloseUserDesktop ;
	PWLX_SET_OPTION                   WlxSetOption;
	PWLX_GET_OPTION                   WlxGetOption;
	PWLX_WIN31_MIGRATE                WlxWin31Migrate;
	PWLX_QUERY_CLIENT_CREDENTIALS     WlxQueryClientCredentials;
	PWLX_QUERY_IC_CREDENTIALS         WlxQueryInetConnectorCredentials;
	PWLX_DISCONNECT                   WlxDisconnect;
	PWLX_QUERY_TERMINAL_SERVICES_DATA WlxQueryTerminalServicesData;
} WLX_DISPATCH_VERSION_1_3, *PWLX_DISPATCH_VERSION_1_3;

typedef
struct _WLX_DISPATCH_VERSION_1_4
{
	PWLX_USE_CTRL_ALT_DEL                WlxUseCtrlAltDel;
	PWLX_SET_CONTEXT_POINTER             WlxSetContextPointer;
	PWLX_SAS_NOTIFY                      WlxSasNotify;
	PWLX_SET_TIMEOUT                     WlxSetTimeout;
	PWLX_ASSIGN_SHELL_PROTECTION         WlxAssignShellProtection;
	PWLX_MESSAGE_BOX                     WlxMessageBox;
	PWLX_DIALOG_BOX                      WlxDialogBox;
	PWLX_DIALOG_BOX_PARAM                WlxDialogBoxParam;
	PWLX_DIALOG_BOX_INDIRECT             WlxDialogBoxIndirect;
	PWLX_DIALOG_BOX_INDIRECT_PARAM       WlxDialogBoxIndirectParam;
	PWLX_SWITCH_DESKTOP_TO_USER          WlxSwitchDesktopToUser;
	PWLX_SWITCH_DESKTOP_TO_WINLOGON      WlxSwitchDesktopToWinlogon;
	PWLX_CHANGE_PASSWORD_NOTIFY          WlxChangePasswordNotify;
	PWLX_GET_SOURCE_DESKTOP              WlxGetSourceDesktop;
	PWLX_SET_RETURN_DESKTOP              WlxSetReturnDesktop;
	PWLX_CREATE_USER_DESKTOP             WlxCreateUserDesktop;
	PWLX_CHANGE_PASSWORD_NOTIFY_EX       WlxChangePasswordNotifyEx;
	PWLX_CLOSE_USER_DESKTOP              WlxCloseUserDesktop ;
	PWLX_SET_OPTION                      WlxSetOption;
	PWLX_GET_OPTION                      WlxGetOption;
	PWLX_WIN31_MIGRATE                   WlxWin31Migrate;
	PWLX_QUERY_CLIENT_CREDENTIALS        WlxQueryClientCredentials;
	PWLX_QUERY_IC_CREDENTIALS            WlxQueryInetConnectorCredentials;
	PWLX_DISCONNECT                      WlxDisconnect;
	PWLX_QUERY_TERMINAL_SERVICES_DATA    WlxQueryTerminalServicesData;
	PWLX_QUERY_CONSOLESWITCH_CREDENTIALS WlxQueryConsoleSwitchCredentials;
	PWLX_QUERY_TS_LOGON_CREDENTIALS      WlxQueryTsLogonCredentials;
} WLX_DISPATCH_VERSION_1_4, *PWLX_DISPATCH_VERSION_1_4;


/* non-GINA notification DLLs*/

typedef
DWORD (CALLBACK *PFNMSGECALLBACK)(
	BOOL bVerbose,
	LPWSTR lpMessage);

typedef
struct _WLX_NOTIFICATION_INFO
{
	ULONG           Size;
	ULONG           Flags;
	PWSTR           UserName;
	PWSTR           Domain;
	PWSTR           WindowStation;
	HANDLE          hToken;
	HDESK           hDesktop;
	PFNMSGECALLBACK pStatusCallback;
} WLX_NOTIFICATION_INFO, *PWLX_NOTIFICATION_INFO;


#ifdef __cplusplus
}
#endif

#endif /*__WINWLX_H */

/* EOF */
