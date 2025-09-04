/*
 * Copyright 2005 Ulrich Czekalla (For CodeWeavers)
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

#ifndef __WINE_WTSAPI32_H
#define __WINE_WTSAPI32_H

#ifdef __cplusplus
extern "C" {
#endif

/*
 * pResponse values from WTSSendMessage(), in addition
 * to those from the standard MessageBox() API.
 */
#ifndef IDTIMEOUT
#define IDTIMEOUT 32000
#endif
#ifndef IDASYNC
#define IDASYNC   32001
#endif

typedef enum _WTS_VIRTUAL_CLASS
{
    WTSVirtualClientData,
    WTSVirtualFileHandle
} WTS_VIRTUAL_CLASS;

typedef enum tagWTS_INFO_CLASS
{
    WTSInitialProgram,
    WTSApplicationName,
    WTSWorkingDirectory,
    WTSOEMId,
    WTSSessionId,
    WTSUserName,
    WTSWinStationName,
    WTSDomainName,
    WTSConnectState,
    WTSClientBuildNumber,
    WTSClientName,
    WTSClientDirectory,
    WTSClientProductId,
    WTSClientHardwareId,
    WTSClientAddress,
    WTSClientDisplay,
    WTSClientProtocolType,
#if (NTDDI_VERSION >= NTDDI_WS08)
    WTSIdleTime,
    WTSLogonTime,
    WTSIncomingBytes,
    WTSOutgoingBytes,
    WTSIncomingFrames,
    WTSOutgoingFrames,
    WTSClientInfo,
    WTSSessionInfo,
    WTSSessionInfoEx,
    WTSConfigInfo,
    WTSValidationInfo,
    WTSSessionAddressV4,
    WTSIsRemoteSession
#endif /* (NTDDI_VERSION >= NTDDI_WS08) */
} WTS_INFO_CLASS;

typedef enum _WTS_CONNECTSTATE_CLASS
{
    WTSActive,
    WTSConnected,
    WTSConnectQuery,
    WTSShadow,
    WTSDisconnected,
    WTSIdle,
    WTSListen,
    WTSReset,
    WTSDown,
    WTSInit
} WTS_CONNECTSTATE_CLASS;

typedef enum _WTS_CONFIG_CLASS
{
    WTSUserConfigInitialProgram,
    WTSUserConfigWorkingDirectory,
    WTSUserConfigInheritInitialProgram,
    WTSUserConfigAllowLogonTerminalServer,
    WTSUserConfigTimeoutSettingsConnections,
    WTSUserConfigTimeoutSettingsDisconnections,
    WTSUserConfigTimeoutSettingsIdle,
    WTSUserConfigDeviceClientDrives,
    WTSUserConfigDeviceClientPrinters,
    WTSUserConfigDeviceClientDefaultPrinter,
    WTSUserConfigBrokenTimeoutSettings,
    WTSUserConfigModemCallbackSettings,
    WTSUserConfigModemCallbackPhoneNumber,
    WTSUserConfigShadowSettings,
    WTSUserConfigTerminalServerProfilePath,
    WTSUserConfigTerminalServerHomeDirectory,
    WTSUserConfigfTerminalServerRemoteHomeDir
} WTS_CONFIG_CLASS;

typedef enum _WTS_TYPE_CLASS
{
    WTSTypeProcessInfoLevel0,
    WTSTypeProcessInfoLevel1,
    WTSTypeSessionInfoLevel1
} WTS_TYPE_CLASS;

typedef struct _WTS_PROCESS_INFOA
{
    DWORD SessionId;
    DWORD ProcessId;
    LPSTR pProcessName;
    PSID pUserSid;
} WTS_PROCESS_INFOA, *PWTS_PROCESS_INFOA;

typedef struct _WTS_PROCESS_INFOW
{
    DWORD SessionId;
    DWORD ProcessId;
    LPWSTR pProcessName;
    PSID pUserSid;
} WTS_PROCESS_INFOW, *PWTS_PROCESS_INFOW;

DECL_WINELIB_TYPE_AW(WTS_PROCESS_INFO)
DECL_WINELIB_TYPE_AW(PWTS_PROCESS_INFO)

typedef struct _WTS_SESSION_INFOA
{
    DWORD SessionId;
    LPSTR pWinStationName;
    WTS_CONNECTSTATE_CLASS State;
} WTS_SESSION_INFOA, *PWTS_SESSION_INFOA;

typedef struct _WTS_SESSION_INFOW
{
    DWORD SessionId;
    LPWSTR pWinStationName;
    WTS_CONNECTSTATE_CLASS State;
} WTS_SESSION_INFOW, *PWTS_SESSION_INFOW;

DECL_WINELIB_TYPE_AW(WTS_SESSION_INFO)
DECL_WINELIB_TYPE_AW(PWTS_SESSION_INFO)

typedef struct _WTS_SESSION_INFO_1A
{
    DWORD ExecEnvId;
    WTS_CONNECTSTATE_CLASS State;
    DWORD SessionId;
    char *pSessionName;
    char *pHostName;
    char *pUserName;
    char *pDomainName;
    char *pFarmName;
} WTS_SESSION_INFO_1A, *PWTS_SESSION_INFO_1A;

typedef struct _WTS_SESSION_INFO_1W
{
    DWORD ExecEnvId;
    WTS_CONNECTSTATE_CLASS State;
    DWORD SessionId;
    WCHAR *pSessionName;
    WCHAR *pHostName;
    WCHAR *pUserName;
    WCHAR *pDomainName;
    WCHAR *pFarmName;
} WTS_SESSION_INFO_1W, *PWTS_SESSION_INFO_1W;

DECL_WINELIB_TYPE_AW(WTS_SESSION_INFO_1)
DECL_WINELIB_TYPE_AW(PWTS_SESSION_INFO_1)

typedef struct _WTS_SERVER_INFOA
{
    LPSTR pServerName;
} WTS_SERVER_INFOA, *PWTS_SERVER_INFOA;

typedef struct _WTS_SERVER_INFOW
{
    LPWSTR pServerName;
} WTS_SERVER_INFOW, *PWTS_SERVER_INFOW;

DECL_WINELIB_TYPE_AW(WTS_SERVER_INFO)
DECL_WINELIB_TYPE_AW(PWTS_SERVER_INFO)

#define WTS_CURRENT_SERVER_HANDLE ((HANDLE)NULL)
#define WTS_CURRENT_SESSION (~0u)

#define NOTIFY_FOR_THIS_SESSION 0
#define NOTIFY_FOR_ALL_SESSIONS 1

void WINAPI WTSCloseServer(HANDLE);
BOOL WINAPI WTSConnectSessionA(ULONG, ULONG, PSTR, BOOL);
BOOL WINAPI WTSConnectSessionW(ULONG, ULONG, PWSTR, BOOL);
#define     WTSConnectSession WINELIB_NAME_AW(WTSConnectSession)
BOOL WINAPI WTSDisconnectSession(HANDLE, DWORD, BOOL);
BOOL WINAPI WTSEnableChildSessions(BOOL);
BOOL WINAPI WTSEnumerateProcessesA(HANDLE, DWORD, DWORD, PWTS_PROCESS_INFOA *, DWORD *);
BOOL WINAPI WTSEnumerateProcessesW(HANDLE, DWORD, DWORD, PWTS_PROCESS_INFOW *, DWORD *);
#define     WTSEnumerateProcesses WINELIB_NAME_AW(WTSEnumerateProcesses)
BOOL WINAPI WTSEnumerateServersA( LPSTR, DWORD, DWORD, PWTS_SERVER_INFOA*, DWORD*);
BOOL WINAPI WTSEnumerateServersW( LPWSTR, DWORD, DWORD, PWTS_SERVER_INFOW*, DWORD*);
#define     WTSEnumerateServers WINELIB_NAME_AW(WTSEnumerateServers)
BOOL WINAPI WTSEnumerateSessionsA(HANDLE, DWORD, DWORD, PWTS_SESSION_INFOA *, DWORD *);
BOOL WINAPI WTSEnumerateSessionsW(HANDLE, DWORD, DWORD, PWTS_SESSION_INFOW *, DWORD *);
#define     WTSEnumerateSessions WINELIB_NAME_AW(WTSEnumerateSessions)
void WINAPI WTSFreeMemory(PVOID);
HANDLE WINAPI WTSOpenServerA(LPSTR);
HANDLE WINAPI WTSOpenServerW(LPWSTR);
#define     WTSOpenServer WINELIB_NAME_AW(WTSOpenServer)
BOOL WINAPI WTSQuerySessionInformationA(HANDLE, DWORD, WTS_INFO_CLASS, LPSTR *, DWORD *);
BOOL WINAPI WTSQuerySessionInformationW(HANDLE, DWORD, WTS_INFO_CLASS, LPWSTR *, DWORD *);
#define     WTSQuerySessionInformation WINELIB_NAME_AW(WTSQuerySessionInformation)
BOOL WINAPI WTSQueryUserConfigA(LPSTR,LPSTR,WTS_CONFIG_CLASS,LPSTR*,DWORD*);
BOOL WINAPI WTSQueryUserConfigW(LPWSTR,LPWSTR,WTS_CONFIG_CLASS,LPWSTR*,DWORD*);
#define     WTSQueryUserConfig WINELIB_NAME_AW(WTSQueryUserConfig)
BOOL WINAPI WTSQueryUserToken(ULONG, PHANDLE);
/**
 * Registers the specified window to receive session change notifications.
 * This function allows applications to be notified of session connect,
 * disconnect, logon, logoff, and other session-related events.
 *
 * @param hWnd A handle to the window that will receive session change notifications through the WM_WTSSESSION_CHANGE
 * message. The window must belong to the calling process.
 * @param dwFlags Specifies which sessions (current or all sessions) the notifications should apply to. Possible values:
 *                - NOTIFY_FOR_THIS_SESSION: Notifications are for the current session only.
 *                - NOTIFY_FOR_ALL_SESSIONS: Notifications apply to all sessions.
 *
 * @return If the function succeeds, the return value is TRUE. If the function fails, the return value is FALSE. Use
 * GetLastError to get extended error information.
 */
BOOL WINAPI WTSRegisterSessionNotification(HWND hWnd, DWORD dwFlags);
BOOL WINAPI WTSRegisterSessionNotificationEx(HANDLE, HWND, DWORD);
BOOL WINAPI WTSStartRemoteControlSessionA(LPSTR, ULONG, BYTE, USHORT);
BOOL WINAPI WTSStartRemoteControlSessionW(LPWSTR, ULONG, BYTE, USHORT);
#define     WTSStartRemoteControlSession WINELIB_NAME_AW(WTSStartRemoteControlSession)
BOOL WINAPI WTSStopRemoteControlSession(ULONG);
BOOL WINAPI WTSTerminateProcess(HANDLE, DWORD, DWORD);
BOOL WINAPI WTSUnRegisterSessionNotification(HWND);
BOOL WINAPI WTSUnRegisterSessionNotificationEx(HANDLE, HWND);
BOOL WINAPI WTSWaitSystemEvent(_In_ HANDLE, _In_ DWORD, _Out_ PDWORD);

#ifdef __cplusplus
}
#endif

#endif
