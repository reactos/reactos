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

void WINAPI WTSCloseServer(HANDLE);
BOOL WINAPI WTSDisconnectSession(HANDLE, DWORD, BOOL);
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
BOOL WINAPI WTSRegisterSessionNotification(HWND, DWORD);
BOOL WINAPI WTSTerminateProcess(HANDLE, DWORD, DWORD);
BOOL WINAPI WTSUnRegisterSessionNotification(HWND);
BOOL WINAPI WTSWaitSystemEvent(HANDLE, DWORD, DWORD*);

#ifdef __cplusplus
}
#endif

#endif
