/*
 * Copyright (C) 2004 Juan Lang
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifndef __WINE_NPAPI_H__
#define __WINE_NPAPI_H__

/* capabilities */
#define WNNC_SPEC_VERSION          0x00000001
#define WNNC_SPEC_VERSION51        0x00050001
#define WNNC_NET_TYPE              0x00000002
#define WNNC_NET_NONE              0x00000000

#define WNNC_DRIVER_VERSION        0x00000003

#define WNNC_USER                  0x00000004
#define WNNC_USR_GETUSER           0x00000001

#define WNNC_CONNECTION            0x00000006
#define WNNC_CON_ADDCONNECTION     0x00000001
#define WNNC_CON_CANCELCONNECTION  0x00000002
#define WNNC_CON_GETCONNECTIONS    0x00000004
#define WNNC_CON_ADDCONNECTION3    0x00000008

#define WNNC_DIALOG                0x00000008
#define WNNC_DLG_DEVICEMODE        0x00000001
#define WNNC_DLG_PROPERTYDIALOG    0x00000020
#define WNNC_DLG_SEARCHDIALOG      0x00000040
#define WNNC_DLG_FORMATNETWORKNAME 0x00000080
#define WNNC_DLG_PERMISSIONEDITOR  0x00000100
#define WNNC_DLG_GETRESOURCEPARENT 0x00000200
#define WNNC_DLG_GETRESOURCEINFORMATION 0x00000800

#define WNNC_ADMIN                 0x00000009
#define WNNC_ADM_GETDIRECTORYTYPE  0x00000001
#define WNNC_ADM_DIRECTORYNOTIFY   0x00000002

#define WNNC_ENUMERATION           0x0000000b
#define WNNC_ENUM_GLOBAL           0x00000001
#define WNNC_ENUM_LOCAL            0x00000002
#define WNNC_ENUM_CONTEXT          0x00000004

#define WNNC_START                 0x0000000c
#define WNNC_WAIT_FOR_START        0x00000001

typedef DWORD APIENTRY (*PF_NPGetCaps)(DWORD ndex);

/* get user */
typedef DWORD APIENTRY (*PF_NPGetUser)(LPWSTR lpName, LPWSTR lpUserName,
 LPDWORD lpnBufferLen);

/* enumeration-related */
typedef DWORD APIENTRY (*PF_NPOpenEnum)(DWORD dwScope, DWORD dwType, DWORD dwUsage,
 LPNETRESOURCEW lpNetResource, LPHANDLE lphEnum);
typedef DWORD APIENTRY (*PF_NPEnumResource)(HANDLE hEnum, LPDWORD lpcCount,
 LPVOID lpBuffer, LPDWORD lpBufferSize);
typedef DWORD APIENTRY (*PF_NPCloseEnum)(HANDLE hEnum);
typedef DWORD (APIENTRY *PF_NPGetResourceInformation)(LPNETRESOURCEW lpNetResource,
 LPVOID lpBuffer, LPDWORD lpcbBuffer, LPWSTR* lplpSystem);

/* connection-related */
typedef DWORD APIENTRY (*PF_NPAddConnection)(LPNETRESOURCEW lpNetResource,
 LPWSTR lpPassword, LPWSTR lpUserName);
typedef DWORD APIENTRY (*PF_NPAddConnection3)(HWND hwndOwner,
 LPNETRESOURCEW lpNetResource, LPWSTR lpPassword, LPWSTR lpUserName,
 DWORD dwFlags);
typedef DWORD APIENTRY (*PF_NPCancelConnection)(LPWSTR lpName, BOOL fForce);
typedef DWORD APIENTRY (*PF_NPGetConnection)(LPWSTR lpLocalName,
 LPWSTR lpRemoteName, LPDWORD lpnBufferLen);

/* network name manipulation */
typedef DWORD APIENTRY (*PF_NPGetUniversalName)(LPWSTR lpLocalPath,
 DWORD dwInfoLevel, LPVOID lpBuffer, LPDWORD lpnBufferSize);
typedef DWORD APIENTRY (*PF_NPFormatNetworkName)(LPWSTR lpRemoteName,
 LPWSTR lpFormattedName, LPDWORD lpnLength, DWORD dwFlags,
 DWORD dwAveCharPerLine);

/* dialogs */
typedef DWORD APIENTRY (*PF_NPDeviceMode)(HWND hParent);

/* search dialog */
#define WNSRCH_REFRESH_FIRST_LEVEL 0x00000001

typedef DWORD APIENTRY (*PF_NPSearchDialog)(HWND hwndParent,
 LPNETRESOURCEW lpNetResource, LPVOID lpBuffer, DWORD cbBuffer,
 LPDWORD lpnFlags);

/* property dialog */

#define WNTYPE_DRIVE   1
#define WNTYPE_FILE    2
#define WNTYPE_PRINTER 3
#define WNTYPE_COMM    4

#define WNPS_FILE 0
#define WNPS_DIR  1
#define WNPS_MULT 2

typedef DWORD APIENTRY (*PF_NPGetPropertyText)(DWORD iButton, DWORD nPropSel,
 LPWSTR lpName, LPWSTR lpButtonName, DWORD nButtonNameLen, DWORD nType);

typedef DWORD APIENTRY (*PF_NPPropertyDialog)(HWND hwndParent, DWORD iButtonDlg,
 DWORD nPropSel, LPWSTR lpFileName, DWORD nType);

/* admin */
#define WNDT_NORMAL  0
#define WNDT_NETWORK 1

#define WNDN_MKDIR 1
#define WNDN_RMDIR 2
#define WNDN_MVDIR 3

typedef DWORD APIENTRY (*PF_NPGetDirectoryType)(LPWSTR lpName, LPINT lpType,
 BOOL bFlushCache);
typedef DWORD APIENTRY (*PF_NPDirectoryNotify)(HWND hwnd, LPWSTR lpDir,
 DWORD dwOper);

/* permission editor dialogs */
#define WNPERMC_PERM  0x00000001
#define WNPERMC_AUDIT 0x00000002
#define WNPERMC_OWNER 0x00000004

typedef DWORD APIENTRY (*PF_NPFMXGetPermCaps)(LPWSTR lpDriveName);

#define WNPERM_DLG_PERM  0
#define WNPERM_DLG_AUDIT 1
#define WNPERM_DLG_OWNER 2

typedef DWORD APIENTRY (*PF_NPFMXEditPerm)(LPWSTR lpDriveName, HWND hwndFMX,
 DWORD nDialogType);

typedef DWORD APIENTRY (*PF_NPFMXGetPermHelp)(LPWSTR lpDriveName,
 DWORD nDialogType, BOOL fDirectory, LPVOID lpFileNameBuffer,
 LPDWORD lpBufferSize, LPDWORD lpnHelpContext);

VOID WINAPI WNetSetLastErrorA(DWORD err, LPSTR lpError, LPSTR lpProviders);
VOID WINAPI WNetSetLastErrorW(DWORD err, LPWSTR lpError, LPWSTR lpProviders);
#define WNetSetLastError WINELIB_NAME_AW(WNetSetLastError)

/* provider classes */
#define WN_NETWORK_CLASS         0x00000001
#define WN_CREDENTIAL_CLASS      0x00000002
#define WN_PRIMARY_AUTHENT_CLASS 0x00000004
#define WN_SERVICE_CLASS         0x00000008

#define WN_VALID_LOGON_ACCOUNT   0x00000001
#define WN_NT_PASSWORD_CHANGED   0x00000002

/* notifications */
typedef DWORD APIENTRY (*PF_NPLogonNotify)(PLUID lpLogonId,
 LPCWSTR lpAuthentInfoType, LPVOID lpAuthentInfo,
 LPCWSTR lpPreviousAuthentInfoType, LPVOID lpPreviousAuthentInfo,
 LPWSTR lpStationName, LPVOID StationHandle, LPWSTR *lpLogonScript);
typedef DWORD APIENTRY (*PF_NPPasswordChangeNotify)(LPCWSTR lpAuthentInfoType,
 LPVOID lpAuthentInfo, LPCWSTR lpPreviousAuthentInfoType,
 LPVOID lpPreviousAuthentInfo, LPWSTR lpStationName, LPVOID StationHandle,
 DWORD dwChangeInfo);

#define NOTIFY_PRE  0x00000001
#define NOTIFY_POST 0x00000002

typedef struct _NOTIFYINFO
{
    DWORD  dwNotifyStatus;
    DWORD  dwOperationStatus;
    LPVOID lpContext;
} NOTIFYINFO, *LPNOTIFYINFO;

/* FIXME: NetResource is declared as a NETRESOURCE in psdk, not a NETRESOURCEW,
 * but how can the type change in a service provider?  Defaulting to wide-char
 * for consistency with the rest of the api.
 */
typedef struct _NOTIFYADD
{
    HWND         hwndOwner;
    NETRESOURCEW NetResource;
    DWORD        dwAddFlags;
} NOTIFYADD, *LPNOTIFYADD;

/* FIXME: lpName and lpProvider are declared as LPTSTRs in psdk, but again
 * for consistency with rest of api defaulting to LPWSTRs.
 */
typedef struct _NOTIFYCANCEL
{
    LPWSTR lpName;
    LPWSTR lpProvider;
    DWORD  dwFlags;
    BOOL   fForce;
} NOTIFYCANCEL, *LPNOTIFYCANCEL;

typedef DWORD APIENTRY (*PF_AddConnectNotify)(LPNOTIFYINFO lpNotifyInfo,
 LPNOTIFYADD lpAddInfo);
typedef DWORD APIENTRY (*PF_CancelConnectNotify)(LPNOTIFYINFO lpNotifyInfo,
 LPNOTIFYADD lpAddInfo);

#endif /* ndef __WINE_NPAPI_H__ */
