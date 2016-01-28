#ifndef _WINNETWK_
#define _WINNETWK_

#ifdef __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4820)
#endif

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
#define WNNC_NET_FARALLON	0x00120000
#define WNNC_NET_APPLETALK	0x00130000
#define WNNC_NET_INTERGRAPH	0x00140000
#define WNNC_NET_SYMFONET   0x00150000
#define WNNC_NET_CLEARCASE  0x00160000
#define WNNC_NET_FRONTIER   0x00170000
#define WNNC_NET_BMC        0x00180000
#define WNNC_NET_DCE        0x00190000
#define WNNC_NET_AVID       0x001A0000
#define WNNC_NET_DOCUSPACE  0x001B0000
#define WNNC_NET_MANGOSOFT  0x001C0000
#define WNNC_NET_SERNET     0x001D0000
#define WNNC_NET_DECORB     0x00200000
#define WNNC_NET_PROTSTOR   0x00210000
#define WNNC_NET_FJ_REDIR   0x00220000
#define WNNC_NET_DISTINCT   0x00230000
#define WNNC_NET_TWINS      0x00240000
#define WNNC_NET_RDR2SAMPLE 0x00250000
#define WNNC_NET_CSC        0x00260000
#define WNNC_NET_3IN1       0x00270000
#define WNNC_NET_EXTENDNET  0x00290000
#define WNNC_NET_OBJECT_DIRE 0x00300000
#define WNNC_NET_MASFAX     0x00310000
#define WNNC_NET_HOB_NFS    0x00320000
#define WNNC_NET_SHIVA      0x00330000
#define WNNC_NET_IBMAL      0x00340000
#define WNNC_CRED_MANAGER   0xFFFF0000

#define RESOURCE_CONNECTED 1
#define RESOURCE_GLOBALNET 2
#define RESOURCE_REMEMBERED 3
#define RESOURCE_RECENT 4
#define RESOURCE_CONTEXT 5
#define RESOURCETYPE_ANY 0
#define RESOURCETYPE_DISK 1
#define RESOURCETYPE_PRINT 2
#define RESOURCETYPE_RESERVED 8
#define RESOURCETYPE_UNKNOWN        0xFFFFFFFF
#define RESOURCEUSAGE_CONNECTABLE   0x00000001
#define RESOURCEUSAGE_CONTAINER     0x00000002
#define RESOURCEUSAGE_NOLOCALDEVICE 0x00000004
#define RESOURCEUSAGE_SIBLING       0x00000008
#define RESOURCEUSAGE_ATTACHED      0x00000010
#define RESOURCEUSAGE_ALL           (RESOURCEUSAGE_CONNECTABLE | RESOURCEUSAGE_CONTAINER | RESOURCEUSAGE_ATTACHED)
#define RESOURCEUSAGE_RESERVED      0x80000000
#define RESOURCEDISPLAYTYPE_GENERIC 0
#define RESOURCEDISPLAYTYPE_DOMAIN 1
#define RESOURCEDISPLAYTYPE_SERVER 2
#define RESOURCEDISPLAYTYPE_SHARE 3
#define RESOURCEDISPLAYTYPE_FILE 4
#define RESOURCEDISPLAYTYPE_GROUP 5
#define RESOURCEDISPLAYTYPE_NETWORK 6
#define RESOURCEDISPLAYTYPE_ROOT 7
#define RESOURCEDISPLAYTYPE_SHAREADMIN 8
#define RESOURCEDISPLAYTYPE_DIRECTORY 9
#define RESOURCEDISPLAYTYPE_TREE 10
#define NETPROPERTY_PERSISTENT 1
#define CONNECT_UPDATE_PROFILE 1
#define CONNECT_UPDATE_RECENT 2
#define CONNECT_TEMPORARY 4
#define CONNECT_INTERACTIVE 8
#define CONNECT_PROMPT 16
#define CONNECT_NEED_DRIVE 32
#define CONNECT_REFCOUNT 64
#define CONNECT_REDIRECT 128
#define CONNECT_LOCALDRIVE 256
#define CONNECT_CURRENT_MEDIA 512
#define CONNECT_COMMANDLINE 2048
#define CONNECT_CMD_SAVECRED 4096
#define CONNDLG_RO_PATH 1
#define CONNDLG_CONN_POINT 2
#define CONNDLG_USE_MRU 4
#define CONNDLG_HIDE_BOX 8
#define CONNDLG_PERSIST 16
#define CONNDLG_NOT_PERSIST 32
#define DISC_UPDATE_PROFILE 1
#define DISC_NO_FORCE 64
#define WNFMT_MULTILINE 1
#define WNFMT_ABBREVIATED 2
#define WNFMT_INENUM 16
#define WNFMT_CONNECTION 32
#define WN_SUCCESS NO_ERROR
#define WN_NO_ERROR NO_ERROR
#define WN_NOT_SUPPORTED ERROR_NOT_SUPPORTED
#define WN_CANCEL ERROR_CANCELLED
#define WN_RETRY ERROR_RETRY
#define WN_NET_ERROR ERROR_UNEXP_NET_ERR
#define WN_MORE_DATA ERROR_MORE_DATA
#define WN_BAD_POINTER ERROR_INVALID_ADDRESS
#define WN_BAD_VALUE ERROR_INVALID_PARAMETER
#define WN_BAD_USER ERROR_BAD_USERNAME
#define WN_BAD_PASSWORD ERROR_INVALID_PASSWORD
#define WN_ACCESS_DENIED ERROR_ACCESS_DENIED
#define WN_FUNCTION_BUSY ERROR_BUSY
#define WN_WINDOWS_ERROR ERROR_UNEXP_NET_ERR
#define WN_OUT_OF_MEMORY ERROR_NOT_ENOUGH_MEMORY
#define WN_NO_NETWORK ERROR_NO_NETWORK
#define WN_EXTENDED_ERROR ERROR_EXTENDED_ERROR
#define WN_BAD_LEVEL ERROR_INVALID_LEVEL
#define WN_BAD_HANDLE ERROR_INVALID_HANDLE
#define WN_NOT_INITIALIZING ERROR_ALREADY_INITIALIZED
#define WN_NO_MORE_DEVICES ERROR_NO_MORE_DEVICES
#define WN_NOT_CONNECTED ERROR_NOT_CONNECTED
#define WN_OPEN_FILES ERROR_OPEN_FILES
#define WN_DEVICE_IN_USE ERROR_DEVICE_IN_USE
#define WN_BAD_NETNAME ERROR_BAD_NET_NAME
#define WN_BAD_LOCALNAME ERROR_BAD_DEVICE
#define WN_ALREADY_CONNECTED ERROR_ALREADY_ASSIGNED
#define WN_DEVICE_ERROR ERROR_GEN_FAILURE
#define WN_CONNECTION_CLOSED ERROR_CONNECTION_UNAVAIL
#define WN_NO_NET_OR_BAD_PATH ERROR_NO_NET_OR_BAD_PATH
#define WN_BAD_PROVIDER ERROR_BAD_PROVIDER
#define WN_CANNOT_OPEN_PROFILE ERROR_CANNOT_OPEN_PROFILE
#define WN_BAD_PROFILE ERROR_BAD_PROFILE
#define WN_BAD_DEV_TYPE ERROR_BAD_DEV_TYPE
#define WN_DEVICE_ALREADY_REMEMBERED ERROR_DEVICE_ALREADY_REMEMBERED
#define WN_NO_MORE_ENTRIES ERROR_NO_MORE_ITEMS
#define WN_NOT_CONTAINER ERROR_NOT_CONTAINER
#define WN_NOT_AUTHENTICATED ERROR_NOT_AUTHENTICATED
#define WN_NOT_LOGGED_ON ERROR_NOT_LOGGED_ON
#define WN_NOT_VALIDATED ERROR_NO_LOGON_SERVERS
#define UNIVERSAL_NAME_INFO_LEVEL 1
#define REMOTE_NAME_INFO_LEVEL 2
#define NETINFO_DLL16 1
#define NETINFO_DISKRED 4
#define NETINFO_PRINTERRED 8
#define RP_LOGON 1
#define RP_INIFILE 2
#define PP_DISPLAYERRORS 1
#define WNCON_FORNETCARD 1
#define WNCON_NOTROUTED 2
#define WNCON_SLOWLINK 4
#define WNCON_DYNAMIC 8

#ifndef RC_INVOKED

typedef struct _NETRESOURCEA {
	DWORD dwScope;
	DWORD dwType;
	DWORD dwDisplayType;
	DWORD dwUsage;
	LPSTR lpLocalName;
	LPSTR lpRemoteName;
	LPSTR lpComment ;
	LPSTR lpProvider;
}NETRESOURCEA,*LPNETRESOURCEA;
typedef struct _NETRESOURCEW {
	DWORD dwScope;
	DWORD dwType;
	DWORD dwDisplayType;
	DWORD dwUsage;
	LPWSTR lpLocalName;
	LPWSTR lpRemoteName;
	LPWSTR lpComment ;
	LPWSTR lpProvider;
}NETRESOURCEW,*LPNETRESOURCEW;
typedef struct _CONNECTDLGSTRUCTA{
	DWORD cbStructure;
	HWND hwndOwner;
	LPNETRESOURCEA lpConnRes;
	DWORD dwFlags;
	DWORD dwDevNum;
} CONNECTDLGSTRUCTA,*LPCONNECTDLGSTRUCTA;
typedef struct _CONNECTDLGSTRUCTW{
	DWORD cbStructure;
	HWND hwndOwner;
	LPNETRESOURCEW lpConnRes;
	DWORD dwFlags;
	DWORD dwDevNum;
} CONNECTDLGSTRUCTW,*LPCONNECTDLGSTRUCTW;
typedef struct _DISCDLGSTRUCTA{
	DWORD cbStructure;
	HWND hwndOwner;
	LPSTR lpLocalName;
	LPSTR lpRemoteName;
	DWORD dwFlags;
} DISCDLGSTRUCTA,*LPDISCDLGSTRUCTA;
typedef struct _DISCDLGSTRUCTW{
	DWORD cbStructure;
	HWND hwndOwner;
	LPWSTR lpLocalName;
	LPWSTR lpRemoteName;
	DWORD dwFlags;
} DISCDLGSTRUCTW,*LPDISCDLGSTRUCTW;
typedef struct _UNIVERSAL_NAME_INFOA { LPSTR lpUniversalName; }UNIVERSAL_NAME_INFOA,*LPUNIVERSAL_NAME_INFOA;
typedef struct _UNIVERSAL_NAME_INFOW { LPWSTR lpUniversalName; }UNIVERSAL_NAME_INFOW,*LPUNIVERSAL_NAME_INFOW;
typedef struct _REMOTE_NAME_INFOA {
	LPSTR lpUniversalName;
	LPSTR lpConnectionName;
	LPSTR lpRemainingPath;
}REMOTE_NAME_INFOA,*LPREMOTE_NAME_INFOA;
typedef struct _REMOTE_NAME_INFOW {
	LPWSTR lpUniversalName;
	LPWSTR lpConnectionName;
	LPWSTR lpRemainingPath;
}REMOTE_NAME_INFOW,*LPREMOTE_NAME_INFOW;
typedef struct _NETINFOSTRUCT{
	DWORD cbStructure;
	DWORD dwProviderVersion;
	DWORD dwStatus;
	DWORD dwCharacteristics;
	ULONG_PTR dwHandle;
	WORD wNetType;
	DWORD dwPrinters;
	DWORD dwDrives;
} NETINFOSTRUCT,*LPNETINFOSTRUCT;
typedef UINT(PASCAL *PFNGETPROFILEPATHA)(LPCSTR,LPSTR,UINT);
typedef UINT(PASCAL *PFNGETPROFILEPATHW)(LPCWSTR,LPWSTR,UINT);
typedef UINT(PASCAL *PFNRECONCILEPROFILEA)(LPCSTR,LPCSTR,DWORD);
typedef UINT(PASCAL *PFNRECONCILEPROFILEW)(LPCWSTR,LPCWSTR,DWORD);
typedef BOOL(PASCAL *PFNPROCESSPOLICIESA)(HWND,LPCSTR,LPCSTR,LPCSTR,DWORD);
typedef BOOL(PASCAL *PFNPROCESSPOLICIESW)(HWND,LPCWSTR,LPCWSTR,LPCWSTR,DWORD);
typedef struct _NETCONNECTINFOSTRUCT{
	DWORD cbStructure;
	DWORD dwFlags;
	DWORD dwSpeed;
	DWORD dwDelay;
	DWORD dwOptDataSize;
} NETCONNECTINFOSTRUCT,*LPNETCONNECTINFOSTRUCT;

_Check_return_
DWORD
APIENTRY
WNetAddConnectionA(
  _In_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR);

_Check_return_
DWORD
APIENTRY
WNetAddConnectionW(
  _In_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR);

_Check_return_
DWORD
APIENTRY
WNetAddConnection2A(
  _In_ LPNETRESOURCEA,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ DWORD);

_Check_return_
DWORD
APIENTRY
WNetAddConnection2W(
  _In_ LPNETRESOURCEW,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ DWORD);

_Check_return_
DWORD
APIENTRY
WNetAddConnection3A(
  _In_opt_ HWND,
  _In_ LPNETRESOURCEA,
  _In_opt_ LPCSTR,
  _In_opt_ LPCSTR,
  _In_ DWORD);

_Check_return_
DWORD
APIENTRY
WNetAddConnection3W(
  _In_opt_ HWND,
  _In_ LPNETRESOURCEW,
  _In_opt_ LPCWSTR,
  _In_opt_ LPCWSTR,
  _In_ DWORD);

_Check_return_ DWORD APIENTRY WNetCancelConnectionA(_In_ LPCSTR, _In_ BOOL);
_Check_return_ DWORD APIENTRY WNetCancelConnectionW(_In_ LPCWSTR, _In_ BOOL);

_Check_return_
DWORD
APIENTRY
WNetCancelConnection2A(
  _In_ LPCSTR,
  _In_ DWORD,
  _In_ BOOL);

_Check_return_
DWORD
APIENTRY
WNetCancelConnection2W(
  _In_ LPCWSTR,
  _In_ DWORD,
  _In_ BOOL);

_Check_return_
DWORD
APIENTRY
WNetGetConnectionA(
  _In_ LPCSTR lpLocalName,
  _Out_writes_opt_(*lpnLength) LPSTR lpRemoteName,
  _Inout_ LPDWORD lpnLength);

_Check_return_
DWORD
APIENTRY
WNetGetConnectionW(
  _In_ LPCWSTR lpLocalName,
  _Out_writes_opt_(*lpnLength) LPWSTR lpRemoteName,
  _Inout_ LPDWORD lpnLength);

_Check_return_
DWORD
APIENTRY
WNetUseConnectionA(
  _In_opt_ HWND hwndOwner,
  _In_ LPNETRESOURCEA lpNetResource,
  _In_opt_ LPCSTR lpPassword,
  _In_opt_ LPCSTR lpUserId,
  _In_ DWORD dwFlags,
  _Out_writes_opt_(*lpBufferSize) LPSTR lpAccessName,
  _Inout_opt_ LPDWORD lpBufferSize,
  _Out_opt_ LPDWORD lpResult);

_Check_return_
DWORD
APIENTRY
WNetUseConnectionW(
  _In_opt_ HWND hwndOwner,
  _In_ LPNETRESOURCEW lpNetResource,
  _In_opt_ LPCWSTR lpPassword,
  _In_opt_ LPCWSTR lpUserId,
  _In_ DWORD dwFlags,
  _Out_writes_opt_(*lpBufferSize) LPWSTR lpAccessName,
  _Inout_opt_ LPDWORD lpBufferSize,
  _Out_opt_ LPDWORD lpResult);

DWORD APIENTRY WNetSetConnectionA(LPCSTR,DWORD,PVOID);
DWORD APIENTRY WNetSetConnectionW(LPCWSTR,DWORD,PVOID);
_Check_return_ DWORD APIENTRY WNetConnectionDialog(_In_ HWND, _In_ DWORD);
_Check_return_ DWORD APIENTRY WNetDisconnectDialog(_In_opt_ HWND, _In_ DWORD);

_Check_return_
DWORD
APIENTRY
WNetConnectionDialog1A(
  _Inout_ LPCONNECTDLGSTRUCTA);

_Check_return_
DWORD
APIENTRY
WNetConnectionDialog1W(
  _Inout_ LPCONNECTDLGSTRUCTW);

_Check_return_ DWORD APIENTRY WNetDisconnectDialog1A(_In_ LPDISCDLGSTRUCTA);
_Check_return_ DWORD APIENTRY WNetDisconnectDialog1W(_In_ LPDISCDLGSTRUCTW);

_Check_return_
DWORD
APIENTRY
WNetOpenEnumA(
  _In_ DWORD,
  _In_ DWORD,
  _In_ DWORD,
  _In_opt_ LPNETRESOURCEA,
  _Out_ LPHANDLE);

_Check_return_
DWORD
APIENTRY
WNetOpenEnumW(
  _In_ DWORD,
  _In_ DWORD,
  _In_ DWORD,
  _In_opt_ LPNETRESOURCEW,
  _Out_ LPHANDLE);

_Check_return_
DWORD
APIENTRY
WNetEnumResourceA(
  _In_ HANDLE hEnum,
  _Inout_ LPDWORD lpcCount,
  _Out_writes_bytes_(*lpBufferSize) LPVOID lpBuffer,
  _Inout_ LPDWORD lpBufferSize);

_Check_return_
DWORD
APIENTRY
WNetEnumResourceW(
  _In_ HANDLE hEnum,
  _Inout_ LPDWORD lpcCount,
  _Out_writes_bytes_(*lpBufferSize) LPVOID lpBuffer,
  _Inout_ LPDWORD lpBufferSize);

_Check_return_ DWORD APIENTRY WNetCloseEnum(_In_ HANDLE);

_Check_return_
DWORD
APIENTRY
WNetGetUniversalNameA(
  _In_ LPCSTR lpLocalPath,
  _In_ DWORD dwInfoLevel,
  _Out_writes_bytes_(*lpBufferSize) LPVOID lpBuffer,
  _Inout_ LPDWORD lpBufferSize);

_Check_return_
DWORD
APIENTRY
WNetGetUniversalNameW(
  _In_ LPCWSTR lpLocalPath,
  _In_ DWORD dwInfoLevel,
  _Out_writes_bytes_(*lpBufferSize) LPVOID lpBuffer,
  _Inout_ LPDWORD lpBufferSize);

_Check_return_
DWORD
APIENTRY
WNetGetUserA(
  _In_opt_ LPCSTR lpName,
  _Out_writes_(*lpnLength) LPSTR lpUserName,
  _Inout_ LPDWORD lpnLength);

_Check_return_
DWORD
APIENTRY
WNetGetUserW(
  _In_opt_ LPCWSTR lpName,
  _Out_writes_(*lpnLength) LPWSTR lpUserName,
  _Inout_ LPDWORD lpnLength);

_Check_return_
DWORD
APIENTRY
WNetGetProviderNameA(
  _In_ DWORD dwNetType,
  _Out_writes_(*lpBufferSize) LPSTR lpProviderName,
  _Inout_ LPDWORD lpBufferSize);

_Check_return_
DWORD
APIENTRY
WNetGetProviderNameW(
  _In_ DWORD dwNetType,
  _Out_writes_(*lpBufferSize) LPWSTR lpProviderName,
  _Inout_ LPDWORD lpBufferSize);

_Check_return_
DWORD
APIENTRY
WNetGetNetworkInformationA(
  _In_ LPCSTR,
  _Out_ LPNETINFOSTRUCT);

_Check_return_
DWORD
APIENTRY
WNetGetNetworkInformationW(
  _In_ LPCWSTR,
  _Out_ LPNETINFOSTRUCT);

_Check_return_
DWORD
APIENTRY
WNetGetResourceInformationA(
  _In_ LPNETRESOURCEA lpNetResource,
  _Out_writes_bytes_(*lpcbBuffer) LPVOID lpBuffer,
  _Inout_ LPDWORD lpcbBuffer,
  _Outptr_ LPSTR *lplpSystem);

_Check_return_
DWORD
APIENTRY
WNetGetResourceInformationW(
  _In_ LPNETRESOURCEW lpNetResource,
  _Out_writes_bytes_(*lpcbBuffer) LPVOID lpBuffer,
  _Inout_ LPDWORD lpcbBuffer,
  _Outptr_ LPWSTR *lplpSystem);

_Check_return_
DWORD
APIENTRY
WNetGetLastErrorA(
  _Out_ LPDWORD lpError,
  _Out_writes_(nErrorBufSize) LPSTR lpErrorBuf,
  _In_ DWORD nErrorBufSize,
  _Out_writes_(nNameBufSize) LPSTR lpNameBuf,
  _In_ DWORD nNameBufSize);

_Check_return_
DWORD
APIENTRY
WNetGetLastErrorW(
  _Out_ LPDWORD lpError,
  _Out_writes_(nErrorBufSize) LPWSTR lpErrorBuf,
  _In_ DWORD nErrorBufSize,
  _Out_writes_(nNameBufSize) LPWSTR lpNameBuf,
  _In_ DWORD nNameBufSize);

DWORD WINAPI WNetRestoreConnectionA(HWND,LPCSTR);
DWORD WINAPI WNetRestoreConnectionW(HWND,LPCWSTR);

_Check_return_
DWORD
APIENTRY
MultinetGetConnectionPerformanceA(
  _In_ LPNETRESOURCEA,
  _Out_ LPNETCONNECTINFOSTRUCT);

_Check_return_
DWORD
APIENTRY
MultinetGetConnectionPerformanceW(
  _In_ LPNETRESOURCEW,
  _Out_ LPNETCONNECTINFOSTRUCT);

#ifdef _WINE
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
DWORD WINAPI WNetCachePassword( LPSTR, WORD, LPSTR, WORD, BYTE, WORD );
UINT WINAPI WNetEnumCachedPasswords( LPSTR, WORD, BYTE, ENUMPASSWORDPROC, DWORD);
DWORD WINAPI WNetGetCachedPassword( LPSTR, WORD, LPSTR, LPWORD, BYTE );
#endif

#ifdef UNICODE
#define PFNPROCESSPOLICIES PFNPROCESSPOLICIESW
#define PFNRECONCILEPROFILE PFNRECONCILEPROFILEW
#define PFNGETPROFILEPATH PFNGETPROFILEPATHW
typedef NETRESOURCEW NETRESOURCE,*LPNETRESOURCE;
typedef CONNECTDLGSTRUCTW CONNECTDLGSTRUCT,*LPCONNECTDLGSTRUCT;
typedef DISCDLGSTRUCTW DISCDLGSTRUCT,*LPDISCDLGSTRUCT;
typedef REMOTE_NAME_INFOW REMOTE_NAME_INFO,*LPREMOTE_NAME_INFO;
typedef UNIVERSAL_NAME_INFOW UNIVERSAL_NAME_INFO,*LPUNIVERSAL_NAME_INFO;
#define WNetEnumResource WNetEnumResourceW
#define WNetOpenEnum WNetOpenEnumW
#define WNetGetResourceInformation WNetGetResourceInformationW
#define WNetGetUniversalName WNetGetUniversalNameW
#define WNetSetConnection WNetSetConnectionW
#define WNetUseConnection WNetUseConnectionW
#define WNetGetConnection WNetGetConnectionW
#define WNetCancelConnection2 WNetCancelConnection2W
#define WNetCancelConnection WNetCancelConnectionW
#define WNetAddConnection3 WNetAddConnection3W
#define WNetAddConnection2 WNetAddConnection2W
#define WNetAddConnection WNetAddConnectionW
#define WNetConnectionDialog1 WNetConnectionDialog1W
#define WNetDisconnectDialog1 WNetDisconnectDialog1W
#define WNetGetNetworkInformation WNetGetNetworkInformationW
#define WNetGetProviderName WNetGetProviderNameW
#define WNetGetUser WNetGetUserW
#define MultinetGetConnectionPerformance MultinetGetConnectionPerformanceW
#define WNetGetLastError WNetGetLastErrorW
#else
#define PFNGETPROFILEPATH PFNGETPROFILEPATHA
#define PFNRECONCILEPROFILE PFNRECONCILEPROFILEA
#define PFNPROCESSPOLICIES PFNPROCESSPOLICIESA
typedef NETRESOURCEA NETRESOURCE,*LPNETRESOURCE;
typedef CONNECTDLGSTRUCTA CONNECTDLGSTRUCT,*LPCONNECTDLGSTRUCT;
typedef DISCDLGSTRUCTA DISCDLGSTRUCT,*LPDISCDLGSTRUCT;
typedef UNIVERSAL_NAME_INFOA UNIVERSAL_NAME_INFO,*LPUNIVERSAL_NAME_INFO;
typedef REMOTE_NAME_INFOA REMOTE_NAME_INFO,*LPREMOTE_NAME_INFO;
#define WNetOpenEnum WNetOpenEnumA
#define WNetEnumResource WNetEnumResourceA
#define WNetGetResourceInformation WNetGetResourceInformationA
#define WNetGetUniversalName WNetGetUniversalNameA
#define WNetConnectionDialog1 WNetConnectionDialog1A
#define WNetDisconnectDialog1 WNetDisconnectDialog1A
#define WNetAddConnection2 WNetAddConnection2A
#define WNetAddConnection3 WNetAddConnection3A
#define WNetCancelConnection WNetCancelConnectionA
#define WNetCancelConnection2 WNetCancelConnection2A
#define WNetGetConnection WNetGetConnectionA
#define WNetUseConnection WNetUseConnectionA
#define WNetSetConnection WNetSetConnectionA
#define WNetAddConnection WNetAddConnectionA
#define WNetGetUser WNetGetUserA
#define WNetGetProviderName WNetGetProviderNameA
#define WNetGetNetworkInformation WNetGetNetworkInformationA
#define WNetGetLastError WNetGetLastErrorA
#define MultinetGetConnectionPerformance MultinetGetConnectionPerformanceA
#endif

#endif /* !RC_INVOKED */

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* _WINNETWK_ */
