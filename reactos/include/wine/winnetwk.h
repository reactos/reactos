/*
 * Copyright (C) the Wine project
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


#ifndef _WINNETWK_H_
#define _WINNETWK_H_

/*
 * Network types
 */

#define WNNC_NET_MSNET      0x00010000
#define WNNC_NET_LANMAN     0x00020000
#define WNNC_NET_NETWARE    0x00030000
#define WNNC_NET_VINES      0x00040000
#define WNNC_NET_10NET      0x00050000
#define WNNC_NET_LOCUS      0x00060000
#define WNNC_NET_SUN_PC_NFS 0x00070000
#define WNNC_NET_LANSTEP    0x00080000
#define WNNC_NET_9TILES     0x00090000
#define WNNC_NET_LANTASTIC  0x000A0000
#define WNNC_NET_AS400      0x000B0000
#define WNNC_NET_FTP_NFS    0x000C0000
#define WNNC_NET_PATHWORKS  0x000D0000
#define WNNC_NET_LIFENET    0x000E0000
#define WNNC_NET_POWERLAN   0x000F0000
#define WNNC_NET_BWNFS      0x00100000
#define WNNC_NET_COGENT     0x00110000
#define WNNC_NET_FARALLON   0x00120000
#define WNNC_NET_APPLETALK  0x00130000
#define WNNC_NET_INTERGRAPH 0x00140000

/*
 *  Network resources
 */

#define RESOURCE_CONNECTED      0x00000001
#define RESOURCE_GLOBALNET      0x00000002
#define RESOURCE_REMEMBERED     0x00000003
#define RESOURCE_RECENT         0x00000004
#define RESOURCE_CONTEXT        0x00000005

#define RESOURCETYPE_ANY        0x00000000
#define RESOURCETYPE_DISK       0x00000001
#define RESOURCETYPE_PRINT      0x00000002
#define RESOURCETYPE_RESERVED   0x00000008
#define RESOURCETYPE_UNKNOWN    0xFFFFFFFF

#define RESOURCEUSAGE_CONNECTABLE   0x00000001
#define RESOURCEUSAGE_CONTAINER     0x00000002
#define RESOURCEUSAGE_NOLOCALDEVICE 0x00000004
#define RESOURCEUSAGE_SIBLING       0x00000008
#define RESOURCEUSAGE_ATTACHED      0x00000010
#define RESOURCEUSAGE_ALL           (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER | RESOURCEUSAGE_ATTACHED)
#define RESOURCEUSAGE_RESERVED      0x80000000

#define RESOURCEDISPLAYTYPE_GENERIC        0x00000000
#define RESOURCEDISPLAYTYPE_DOMAIN         0x00000001
#define RESOURCEDISPLAYTYPE_SERVER         0x00000002
#define RESOURCEDISPLAYTYPE_SHARE          0x00000003
#define RESOURCEDISPLAYTYPE_FILE           0x00000004
#define RESOURCEDISPLAYTYPE_GROUP          0x00000005
#define RESOURCEDISPLAYTYPE_NETWORK        0x00000006
#define RESOURCEDISPLAYTYPE_ROOT           0x00000007
#define RESOURCEDISPLAYTYPE_SHAREADMIN     0x00000008
#define RESOURCEDISPLAYTYPE_DIRECTORY      0x00000009
#define RESOURCEDISPLAYTYPE_TREE           0x0000000A

typedef struct {
	DWORD	dwScope;
	DWORD	dwType;
	DWORD	dwDisplayType;
	DWORD	dwUsage;
	LPSTR	lpLocalName;
	LPSTR	lpRemoteName;
	LPSTR	lpComment ;
	LPSTR	lpProvider;
} NETRESOURCEA,*LPNETRESOURCEA;

typedef struct {
	DWORD	dwScope;
	DWORD	dwType;
	DWORD	dwDisplayType;
	DWORD	dwUsage;
	LPWSTR	lpLocalName;
	LPWSTR	lpRemoteName;
	LPWSTR	lpComment ;
	LPWSTR	lpProvider;
} NETRESOURCEW,*LPNETRESOURCEW;

DECL_WINELIB_TYPE_AW(NETRESOURCE)
DECL_WINELIB_TYPE_AW(LPNETRESOURCE)


/*
 *  Network connections
 */

#define NETPROPERTY_PERSISTENT       1

#define CONNECT_UPDATE_PROFILE      0x00000001
#define CONNECT_UPDATE_RECENT       0x00000002
#define CONNECT_TEMPORARY           0x00000004
#define CONNECT_INTERACTIVE         0x00000008
#define CONNECT_PROMPT              0x00000010
#define CONNECT_NEED_DRIVE          0x00000020
#define CONNECT_REFCOUNT            0x00000040
#define CONNECT_REDIRECT            0x00000080
#define CONNECT_LOCALDRIVE          0x00000100
#define CONNECT_CURRENT_MEDIA       0x00000200

DWORD WINAPI WNetAddConnectionA(LPCSTR,LPCSTR,LPCSTR);
DWORD WINAPI WNetAddConnectionW(LPCWSTR,LPCWSTR,LPCWSTR);
#define      WNetAddConnection WINELIB_NAME_AW(WNetAddConnection)
DWORD WINAPI WNetAddConnection2A(LPNETRESOURCEA,LPCSTR,LPCSTR,DWORD);
DWORD WINAPI WNetAddConnection2W(LPNETRESOURCEW,LPCWSTR,LPCWSTR,DWORD);
#define      WNetAddConnection2 WINELIB_NAME_AW(WNetAddConnection2)
DWORD WINAPI WNetAddConnection3A(HWND,LPNETRESOURCEA,LPCSTR,LPCSTR,DWORD);
DWORD WINAPI WNetAddConnection3W(HWND,LPNETRESOURCEW,LPCWSTR,LPCWSTR,DWORD);
#define      WNetAddConnection3 WINELIB_NAME_AW(WNetAddConnection3)
DWORD WINAPI WNetCancelConnectionA(LPCSTR,BOOL);
DWORD WINAPI WNetCancelConnectionW(LPCWSTR,BOOL);
#define      WNetCancelConnection WINELIB_NAME_AW(WNetCancelConnection)
DWORD WINAPI WNetCancelConnection2A(LPCSTR,DWORD,BOOL);
DWORD WINAPI WNetCancelConnection2W(LPCWSTR,DWORD,BOOL);
#define      WNetCancelConnection2 WINELIB_NAME_AW(WNetCancelConnection2)
DWORD WINAPI WNetGetConnectionA(LPCSTR,LPSTR,LPDWORD);
DWORD WINAPI WNetGetConnectionW(LPCWSTR,LPWSTR,LPDWORD);
#define      WNetGetConnection WINELIB_NAME_AW(WNetGetConnection)
DWORD WINAPI WNetUseConnectionA(HWND,LPNETRESOURCEA,LPCSTR,LPCSTR,DWORD,LPSTR,LPDWORD,LPDWORD);
DWORD WINAPI WNetUseConnectionW(HWND,LPNETRESOURCEW,LPCWSTR,LPCWSTR,DWORD,LPWSTR,LPDWORD,LPDWORD);
#define      WNetUseConnection WINELIB_NAME_AW(WNetUseConnection)
DWORD WINAPI WNetSetConnectionA(LPCSTR,DWORD,LPVOID);
DWORD WINAPI WNetSetConnectionW(LPCWSTR,DWORD,LPVOID);
#define      WNetSetConnection WINELIB_NAME_AW(WNetSetConnection)

/*
 *  Network connection dialogs
 */

typedef struct {
    DWORD cbStructure;       /* size of this structure in bytes */
    HWND hwndOwner;          /* owner window for the dialog */
    LPNETRESOURCEA lpConnRes;/* Requested Resource info    */
    DWORD dwFlags;           /* flags (see below) */
    DWORD dwDevNum;          /* number of devices connected to */
} CONNECTDLGSTRUCTA, *LPCONNECTDLGSTRUCTA;
typedef struct {
    DWORD cbStructure;       /* size of this structure in bytes */
    HWND hwndOwner;          /* owner window for the dialog */
    LPNETRESOURCEW lpConnRes;/* Requested Resource info    */
    DWORD dwFlags;           /* flags (see below) */
    DWORD dwDevNum;          /* number of devices connected to */
} CONNECTDLGSTRUCTW, *LPCONNECTDLGSTRUCTW;

DECL_WINELIB_TYPE_AW(CONNECTDLGSTRUCT)
DECL_WINELIB_TYPE_AW(LPCONNECTDLGSTRUCT)

#define CONNDLG_RO_PATH     0x00000001 /* Resource path should be read-only    */
#define CONNDLG_CONN_POINT  0x00000002 /* Netware -style movable connection point enabled */
#define CONNDLG_USE_MRU     0x00000004 /* Use MRU combobox  */
#define CONNDLG_HIDE_BOX    0x00000008 /* Hide persistent connect checkbox  */
#define CONNDLG_PERSIST     0x00000010 /* Force persistent connection */
#define CONNDLG_NOT_PERSIST 0x00000020 /* Force connection NOT persistent */

typedef struct {
    DWORD           cbStructure;      /* size of this structure in bytes */
    HWND            hwndOwner;        /* owner window for the dialog */
    LPSTR           lpLocalName;      /* local device name */
    LPSTR           lpRemoteName;     /* network resource name */
    DWORD           dwFlags;          /* flags */
} DISCDLGSTRUCTA, *LPDISCDLGSTRUCTA;
typedef struct {
    DWORD           cbStructure;      /* size of this structure in bytes */
    HWND            hwndOwner;        /* owner window for the dialog */
    LPWSTR          lpLocalName;      /* local device name */
    LPWSTR          lpRemoteName;     /* network resource name */
    DWORD           dwFlags;          /* flags */
} DISCDLGSTRUCTW, *LPDISCDLGSTRUCTW;

DECL_WINELIB_TYPE_AW(DISCDLGSTRUCT)
DECL_WINELIB_TYPE_AW(LPDISCDLGSTRUCT)

#define DISC_UPDATE_PROFILE         0x00000001
#define DISC_NO_FORCE               0x00000040

DWORD WINAPI WNetConnectionDialog(HWND,DWORD);
DWORD WINAPI WNetDisconnectDialog(HWND,DWORD);
DWORD WINAPI WNetConnectionDialog1A(LPCONNECTDLGSTRUCTA);
DWORD WINAPI WNetConnectionDialog1W(LPCONNECTDLGSTRUCTW);
#define      WNetConnectionDialog1 WINELIB_NAME_AW(WNetConnectionDialog1)
DWORD WINAPI WNetDisconnectDialog1A(LPDISCDLGSTRUCTA);
DWORD WINAPI WNetDisconnectDialog1W(LPDISCDLGSTRUCTW);
#define      WNetDisconnectDialog1 WINELIB_NAME_AW(WNetDisconnectDialog1)

/*
 *  Network browsing
 */

DWORD WINAPI WNetOpenEnumA(DWORD,DWORD,DWORD,LPNETRESOURCEA,LPHANDLE);
DWORD WINAPI WNetOpenEnumW(DWORD,DWORD,DWORD,LPNETRESOURCEW,LPHANDLE);
#define      WNetOpenEnum WINELIB_NAME_AW(WNetOpenEnum)
DWORD WINAPI WNetEnumResourceA(HANDLE,LPDWORD,LPVOID,LPDWORD);
DWORD WINAPI WNetEnumResourceW(HANDLE,LPDWORD,LPVOID,LPDWORD);
#define      WNetEnumResource WINELIB_NAME_AW(WNetEnumResource)
DWORD WINAPI WNetGetResourceInformationA(LPNETRESOURCEA,LPVOID,LPDWORD,LPSTR*);
DWORD WINAPI WNetGetResourceInformationW(LPNETRESOURCEW,LPVOID,LPDWORD,LPWSTR*);
#define      WNetGetResourceInformation WINELIB_NAME_AW(WNetGetResourceInformation)
DWORD WINAPI WNetGetResourceParentA(LPNETRESOURCEA,LPVOID,LPDWORD);
DWORD WINAPI WNetGetResourceParentW(LPNETRESOURCEW,LPVOID,LPDWORD);
#define      WNetGetResourceParent WINELIB_NAME_AW(WNetGetResourceParent)
DWORD WINAPI WNetCloseEnum(HANDLE);

/*
 *  Universal naming
 */

#define UNIVERSAL_NAME_INFO_LEVEL   0x00000001
#define REMOTE_NAME_INFO_LEVEL      0x00000002

typedef struct {
    LPSTR    lpUniversalName;
} UNIVERSAL_NAME_INFOA, *LPUNIVERSAL_NAME_INFOA;
typedef struct {
    LPWSTR   lpUniversalName;
} UNIVERSAL_NAME_INFOW, *LPUNIVERSAL_NAME_INFOW;

DECL_WINELIB_TYPE_AW(UNIVERSAL_NAME_INFO)
DECL_WINELIB_TYPE_AW(LPUNIVERSAL_NAME_INFO)

typedef struct {
    LPSTR    lpUniversalName;
    LPSTR    lpConnectionName;
    LPSTR    lpRemainingPath;
}REMOTE_NAME_INFOA, *LPREMOTE_NAME_INFOA;
typedef struct {
    LPWSTR   lpUniversalName;
    LPWSTR   lpConnectionName;
    LPWSTR   lpRemainingPath;
}REMOTE_NAME_INFOW, *LPREMOTE_NAME_INFOW;

DECL_WINELIB_TYPE_AW(REMOTE_NAME_INFO)
DECL_WINELIB_TYPE_AW(LPREMOTE_NAME_INFO)

DWORD WINAPI WNetGetUniversalNameA(LPCSTR,DWORD,LPVOID,LPDWORD);
DWORD WINAPI WNetGetUniversalNameW(LPCWSTR,DWORD,LPVOID,LPDWORD);
#define      WNetGetUniversalName WINELIB_NAME_AW(WNetGetUniversalName)

/*
 *  Other
 */

DWORD WINAPI WNetGetUserA(LPCSTR,LPSTR,LPDWORD);
DWORD WINAPI WNetGetUserW(LPCWSTR,LPWSTR,LPDWORD);
#define      WNetGetUser WINELIB_NAME_AW(WNetGetUser)

#define WNFMT_MULTILINE         0x01
#define WNFMT_ABBREVIATED       0x02
#define WNFMT_INENUM            0x10
#define WNFMT_CONNECTION        0x20

DWORD WINAPI WNetGetProviderNameA(DWORD,LPSTR,LPDWORD);
DWORD WINAPI WNetGetProviderNameW(DWORD,LPWSTR,LPDWORD);
#define      WNetGetProviderName WINELIB_NAME_AW(WNetGetProviderName)

typedef struct {
    DWORD cbStructure;
    DWORD dwProviderVersion;
    DWORD dwStatus;
    DWORD dwCharacteristics;
    DWORD dwHandle;
    WORD  wNetType;
    DWORD dwPrinters;
    DWORD dwDrives;
} NETINFOSTRUCT, *LPNETINFOSTRUCT;

#define NETINFO_DLL16       0x00000001
#define NETINFO_DISKRED     0x00000004
#define NETINFO_PRINTERRED  0x00000008

DWORD WINAPI WNetGetNetworkInformationA(LPCSTR,LPNETINFOSTRUCT);
DWORD WINAPI WNetGetNetworkInformationW(LPCWSTR,LPNETINFOSTRUCT);
#define      WNetGetNetworkInformation WINELIB_NAME_AW(WNetGetNetworkInformation)


/*
 *  Status codes
 */

DWORD WINAPI WNetGetLastErrorA(LPDWORD,LPSTR,DWORD,LPSTR,DWORD);
DWORD WINAPI WNetGetLastErrorW(LPDWORD,LPWSTR,DWORD,LPWSTR,DWORD);
#define      WNetGetLastError WINELIB_NAME_AW(WNetGetLastError)

#define WN_SUCCESS                      NO_ERROR
#define WN_NO_ERROR                     NO_ERROR
#define WN_NOT_SUPPORTED                ERROR_NOT_SUPPORTED
#define WN_CANCEL                       ERROR_CANCELLED
#define WN_RETRY                        ERROR_RETRY
#define WN_NET_ERROR                    ERROR_UNEXP_NET_ERR
#define WN_MORE_DATA                    ERROR_MORE_DATA
#define WN_BAD_POINTER                  ERROR_INVALID_ADDRESS
#define WN_BAD_VALUE                    ERROR_INVALID_PARAMETER
#define WN_BAD_USER                     ERROR_BAD_USERNAME
#define WN_BAD_PASSWORD                 ERROR_INVALID_PASSWORD
#define WN_ACCESS_DENIED                ERROR_ACCESS_DENIED
#define WN_FUNCTION_BUSY                ERROR_BUSY
#define WN_WINDOWS_ERROR                ERROR_UNEXP_NET_ERR
#define WN_OUT_OF_MEMORY                ERROR_NOT_ENOUGH_MEMORY
#define WN_NO_NETWORK                   ERROR_NO_NETWORK
#define WN_EXTENDED_ERROR               ERROR_EXTENDED_ERROR
#define WN_BAD_LEVEL                    ERROR_INVALID_LEVEL
#define WN_BAD_HANDLE                   ERROR_INVALID_HANDLE
#define WN_NOT_INITIALIZING             ERROR_ALREADY_INITIALIZED
#define WN_NO_MORE_DEVICES              ERROR_NO_MORE_DEVICES

#define WN_NOT_CONNECTED                ERROR_NOT_CONNECTED
#define WN_OPEN_FILES                   ERROR_OPEN_FILES
#define WN_DEVICE_IN_USE                ERROR_DEVICE_IN_USE
#define WN_BAD_NETNAME                  ERROR_BAD_NET_NAME
#define WN_BAD_LOCALNAME                ERROR_BAD_DEVICE
#define WN_ALREADY_CONNECTED            ERROR_ALREADY_ASSIGNED
#define WN_DEVICE_ERROR                 ERROR_GEN_FAILURE
#define WN_CONNECTION_CLOSED            ERROR_CONNECTION_UNAVAIL
#define WN_NO_NET_OR_BAD_PATH           ERROR_NO_NET_OR_BAD_PATH
#define WN_BAD_PROVIDER                 ERROR_BAD_PROVIDER
#define WN_CANNOT_OPEN_PROFILE          ERROR_CANNOT_OPEN_PROFILE
#define WN_BAD_PROFILE                  ERROR_BAD_PROFILE
#define WN_BAD_DEV_TYPE                 ERROR_BAD_DEV_TYPE
#define WN_DEVICE_ALREADY_REMEMBERED    ERROR_DEVICE_ALREADY_REMEMBERED

#define WN_NO_MORE_ENTRIES              ERROR_NO_MORE_ITEMS
#define WN_NOT_CONTAINER                ERROR_NOT_CONTAINER

#define WN_NOT_AUTHENTICATED            ERROR_NOT_AUTHENTICATED
#define WN_NOT_LOGGED_ON                ERROR_NOT_LOGGED_ON
#define WN_NOT_VALIDATED                ERROR_NO_LOGON_SERVERS


/*
 *  Multinet (for Shell)
 */

typedef struct {
	DWORD	cbStructure;
	DWORD	dwFlags;
	DWORD	dwSpeed;
	DWORD	dwDelay;
	DWORD	dwOptDataSize;
} NETCONNECTINFOSTRUCT,*LPNETCONNECTINFOSTRUCT;

#define WNCON_FORNETCARD        0x00000001
#define WNCON_NOTROUTED         0x00000002
#define WNCON_SLOWLINK          0x00000004
#define WNCON_DYNAMIC           0x00000008

DWORD WINAPI MultinetGetConnectionPerformanceA(LPNETRESOURCEA,LPNETCONNECTINFOSTRUCT);
DWORD WINAPI MultinetGetConnectionPerformanceW(LPNETRESOURCEW,LPNETCONNECTINFOSTRUCT);
#define      MultinetGetConnectionPerformance WINELIB_NAME_AW(MultinetGetConnectionPerformance)
DWORD WINAPI MultinetGetErrorTextA(DWORD,DWORD,DWORD);
DWORD WINAPI MultinetGetErrorTextW(DWORD,DWORD,DWORD);
#define      MultinetGetErrorText WINELIB_NAME_AW(MultinetGetErrorText)

/*
 * Password cache
 */

/* WNetEnumCachedPasswords */
typedef struct tagPASSWORD_CACHE_ENTRY
{
	WORD cbEntry;
	WORD cbResource;
	WORD cbPassword;
	BYTE iEntry;
	BYTE nType;
	BYTE abResource[1];
} PASSWORD_CACHE_ENTRY;

typedef BOOL (CALLBACK *ENUMPASSWORDPROC)(PASSWORD_CACHE_ENTRY *, DWORD);
UINT WINAPI WNetEnumCachedPasswords( LPSTR, WORD, BYTE, ENUMPASSWORDPROC, DWORD);
DWORD WINAPI WNetGetCachedPassword( LPSTR, WORD, LPSTR, LPWORD, BYTE );
DWORD WINAPI WNetCachePassword( LPSTR, WORD, LPSTR, WORD, BYTE, WORD );


#endif /* _WINNETWK_H_ */
