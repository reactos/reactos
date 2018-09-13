/*****************************************************************/
/**               Microsoft Windows for Workgroups              **/
/**           Copyright (C) Microsoft Corp., 1991-1995          **/
/*****************************************************************/

/* NETSPI.H -- Network service provider interface definitions.
 */

#ifndef _INC_NETSPI
#define _INC_NETSPI

#ifndef _WINNETWK_
#include <winnetwk.h>
#endif

#ifndef _INC_NETMPR_
#include <netmpr.h>
#endif

#ifndef RC_INVOKED
#pragma pack(1)         /* Assume byte packing throughout */
#endif /* !RC_INVOKED */

#ifdef __cplusplus
extern "C" {            /* Assume C declarations for C++ */
#endif  /* __cplusplus */

//
// Capability masks and values.
//

#define WNNC_SPEC_VERSION   0x01
#define     WNNC_SPEC_VERSION51 0x00050001

#define WNNC_NET_TYPE       0x02
// Net types defined in Winnetwk.h

#define WNNC_DRIVER_VERSION 0x03

#define WNNC_USER           0x04
#define     WNNC_USR_GETUSER    0x01

#define WNNC_CONNECTION     0x06
#define     WNNC_CON_ADDCONNECTION      0x01
#define     WNNC_CON_CANCELCONNECTION   0x02
#define     WNNC_CON_GETCONNECTIONS     0x04
#define     WNNC_CON_GETPERFORMANCE     0x08
#define     WNNC_CON_GETUNIVERSALNAME   0x10
#define     WNNC_CON_FORMATCONNECTION   0x20

#define WNNC_DIALOG         0x08
#define     WNNC_DLG_FORMATNETWORKNAME  0x080
#define     WNNC_DLG_GETRESOURCEPARENT  0x100
#define     WNNC_DLG_GETRESOURCEINFORMATION  0x800

#define WNNC_ERROR          0x0A
#define  WNNC_ERR_GETERROR              0x01
#define  WNNC_ERR_GETERRORTEXT          0x02

#define WNNC_ENUMERATION    0x0B
#define     WNNC_ENUM_GLOBAL    0x01
#define     WNNC_ENUM_LOCAL     0x02
#define     WNNC_ENUM_CONTEXT   0x04

#define WNNC_START          0x0C
#define     WNNC_START_WONT     0x00
#define     WNNC_START_UNKNOWN  0xFFFFFFFF
#define     WNNC_START_DONE     0x01
#define     WNNC_START_INACTIVE 0xFFFFFFFE

#define WNNC_RESOURCE       0x0D
#define     WNNC_RES_VALIDLOCALDEVICE   0x80

#define WNNC_AUTHENTICATION 0x0E
#define     WNNC_AUTH_LOGON             0x02
#define     WNNC_AUTH_LOGOFF            0x04
#define		WNNC_AUTH_GETHOMEDIRECTORY	0x10
#define		WNNC_AUTH_GETPOLICYPATH		0x20

#define WNNC_MAXCAPNO WNNC_AUTHENTICATION

//
// Profile strings.
//
#define NPProvider      "NPProvider"
#define NPName          "NPName"
#define NPDescription   "NPDescription"
#define NPID            "NPID"

//
// Various defines.
//
//Spec version
#define WNNC_DRIVER_MAJOR1  1  
#define WNNC_DRIVER_MINOR1  1
#define WNNC_DRIVER(major,minor) (major*0x00010000 + minor)

//
// NP SPI Definitions.
//

#define SPIENTRY DWORD WINAPI

typedef SPIENTRY F_NPGetCaps(
    DWORD nIndex
    );

F_NPGetCaps NPGetCaps;
typedef F_NPGetCaps FAR *PF_NPGetCaps;

typedef SPIENTRY F_NPGetUniversalName(
	LPTSTR  lpLocalPath,
	DWORD   dwInfoLevel,
	LPVOID  lpBuffer,
	LPDWORD lpBufferSize
    );

F_NPGetUniversalName NPGetUniversalName;
typedef F_NPGetUniversalName FAR *PF_NPGetUniversalName;

typedef SPIENTRY F_NPGetUser(
    LPTSTR  lpName,
    LPTSTR  lpAuthenticationID,
    LPDWORD lpBufferSize
    );

F_NPGetUser NPGetUser;
typedef F_NPGetUser FAR *PF_NPGetUser;

typedef SPIENTRY F_NPValidLocalDevice(
    DWORD dwType,
    DWORD dwNumber
    );

F_NPValidLocalDevice NPValidLocalDevice;
typedef F_NPValidLocalDevice FAR *PF_NPValidLocalDevice;

typedef SPIENTRY F_NPAddConnection(
    HWND hwndOwner,
    LPNETRESOURCE lpNetResource,
    LPTSTR lpPassword,
    LPTSTR lpUserID,
    DWORD dwFlags,
	LPTSTR lpAccessName,
	LPDWORD lpBufferSize,
	LPDWORD lpResult
    );

F_NPAddConnection NPAddConnection;
typedef F_NPAddConnection FAR *PF_NPAddConnection;

typedef SPIENTRY F_NPCancelConnection(
    LPTSTR lpName,
    BOOL fForce,
 	DWORD dwFlags
    );

F_NPCancelConnection NPCancelConnection;
typedef F_NPCancelConnection FAR *PF_NPCancelConnection;

typedef SPIENTRY F_NPGetConnection(
    LPTSTR lpLocalName,
    LPTSTR lpRemoteName,
    LPDWORD lpBufferSize
    );

F_NPGetConnection NPGetConnection;
typedef F_NPGetConnection FAR *PF_NPGetConnection;

typedef SPIENTRY F_NPGetConnectionPerformance(
    LPTSTR lpRemoteName, 
    LPNETCONNECTINFOSTRUCT lpNetConnectInfoStruct
    );

F_NPGetConnectionPerformance NPGetConnectionPerformance;
typedef F_NPGetConnectionPerformance FAR *PF_NPGetConnectionPerformance;

typedef SPIENTRY F_NPFormatNetworkName(
    LPTSTR lpRemoteName,
    LPTSTR lpFormattedName,
    LPDWORD lpnLength,
    DWORD dwFlags,
    DWORD dwAveCharPerLine
    );

F_NPFormatNetworkName NPFormatNetworkName;
typedef F_NPFormatNetworkName FAR *PF_NPFormatNetworkName;

typedef DWORD (FAR PASCAL *NPDISPLAYCALLBACK)(
    LPVOID  lpUserData,
    DWORD   dwDisplayType,
    LPTSTR  lpszHeaders
    );

typedef SPIENTRY F_NPOpenEnum(
    DWORD dwScope,
    DWORD dwType,
    DWORD dwUsage,
    LPNETRESOURCE lpNetResource,
    LPHANDLE lphEnum
    );

F_NPOpenEnum NPOpenEnum;
typedef F_NPOpenEnum FAR *PF_NPOpenEnum;

typedef SPIENTRY F_NPEnumResource(
    HANDLE hEnum,
    LPDWORD lpcCount,
    LPVOID lpBuffer,
    DWORD cbBuffer,
    LPDWORD lpcbFree
    );

F_NPEnumResource NPEnumResource;
typedef F_NPEnumResource FAR *PF_NPEnumResource;

typedef SPIENTRY F_NPCloseEnum(
    HANDLE hEnum
    );

F_NPCloseEnum NPCloseEnum;
typedef F_NPCloseEnum FAR *PF_NPCloseEnum;

typedef SPIENTRY F_NPGetResourceParent(
    LPNETRESOURCE lpNetResource,
    LPVOID lpBuffer,
    LPDWORD cbBuffer
    );

F_NPGetResourceParent NPGetResourceParent;
typedef F_NPGetResourceParent FAR *PF_NPGetResourceParent;

typedef SPIENTRY F_NPGetResourceInformation(
	LPNETRESOURCE lpNetResource,
	LPVOID lpBuffer,
	LPDWORD cbBuffer,
	LPSTR *lplpSystem
    );

F_NPGetResourceInformation NPGetResourceInformation;
typedef F_NPGetResourceInformation FAR *PF_NPGetResourceInformation;

typedef struct _LOGONINFO {
    LPTSTR lpUsername;
    LPTSTR lpPassword;
	DWORD cbUsername;
	DWORD cbPassword;
} LOGONINFO, FAR *LPLOGONINFO;

typedef SPIENTRY F_NPLogon(
    HWND hwndOwner,
    LPLOGONINFO lpAuthentInfo,
    LPLOGONINFO lpPreviousAuthentInfo,
    LPTSTR lpLogonScript,
    DWORD dwBufferSize,
    DWORD dwFlags
    );

F_NPLogon NPLogon;
typedef F_NPLogon FAR *PF_NPLogon;

typedef SPIENTRY F_NPLogoff(
    HWND hwndOwner,
    LPLOGONINFO lpAuthentInfo,
    DWORD dwReason
    );

F_NPLogoff NPLogoff;
typedef F_NPLogoff FAR *PF_NPLogoff;

typedef SPIENTRY F_NPChangePassword(
    LPLOGONINFO lpAuthentInfo,
    LPLOGONINFO lpPreviousAuthentInfo,
	DWORD		dwAction
	);

F_NPChangePassword NPChangePassword;
typedef F_NPChangePassword FAR *PF_NPChangePassword;

typedef SPIENTRY F_NPChangePasswordHwnd(
	HWND hwndOwner
	);

F_NPChangePasswordHwnd NPChangePasswordHwnd;
typedef F_NPChangePasswordHwnd FAR *PF_NPChangePasswordHwnd;


typedef SPIENTRY F_NPGetPasswordStatus(
	DWORD		nIndex
	);

F_NPGetPasswordStatus NPGetPasswordStatus;
typedef F_NPGetPasswordStatus FAR *PF_NPGetPasswordStatus;


typedef SPIENTRY F_NPGetHomeDirectory(
    LPTSTR lpDirectory,
    LPDWORD lpBufferSize
    );

F_NPGetHomeDirectory NPGetHomeDirectory;
typedef F_NPGetHomeDirectory FAR *PF_NPGetHomeDirectory;

typedef SPIENTRY F_NPGetPolicyPath(
    LPTSTR lpPath,
    LPDWORD lpBufferSize,
	DWORD dwFlags
    );

// flags for NPGetPolicyPath
#define GPP_LOADBALANCE	0x0001

F_NPGetPolicyPath NPGetPolicyPath;
typedef F_NPGetPolicyPath FAR *PF_NPGetPolicyPath;

//
// MPR Services.
//

#define NPSGetProviderHandle NPSGetProviderHandleA
#define NPSGetProviderName NPSGetProviderNameA
#define NPSGetSectionName NPSGetSectionNameA
#define NPSSetExtendedError NPSSetExtendedErrorA
#define NPSSetCustomText NPSSetCustomTextA
#define NPSCopyString NPSCopyStringA
#define NPSDeviceGetNumber NPSDeviceGetNumberA
#define NPSDeviceGetString NPSDeviceGetStringA
#define NPSNotifyRegister NPSNotifyRegisterA
#define NPSNotifyGetContext NPSNotifyGetContextA
#define NPSAuthenticationDialog NPSAuthenticationDialogA

#define NPSERVICE	WINAPI
#define HPROVIDER   LPVOID
typedef HPROVIDER FAR * PHPROVIDER;

typedef struct {
    DWORD  cbStructure;       /* size of this structure in bytes */
    HWND   hwndOwner;         /* owner window for the authentication dialog */
    LPCSTR lpResource;        /* remote name of resource being accessed */
    LPSTR  lpUsername;        /* default username to show, NULL to hide field */
    DWORD  cbUsername;        /* size of lpUsername buffer, set to length copied on exit */
    LPSTR  lpPassword;        /* default password to show */
    DWORD  cbPassword;        /* size of lpPassword buffer, set to length copied on exit */
    LPSTR  lpOrgUnit;         /* default org unit to show, NULL to hide field */
    DWORD  cbOrgUnit;         /* size of lpOrgUnit buffer, set to length copied on exit */
    LPCSTR lpOUTitle;         /* title of org unit field, NULL for default title */
    LPCSTR lpExplainText;     /* explanatory text at top, NULL for default text */
    LPCSTR lpDefaultUserName; /* explanatory text at top, NULL for default text */
    DWORD  dwFlags;           /* flags (see below) */
} AUTHDLGSTRUCTA, FAR *LPAUTHDLGSTRUCTA;
#define AUTHDLGSTRUCT AUTHDLGSTRUCTA
#define LPAUTHDLGSTRUCT LPAUTHDLGSTRUCTA

#define AUTHDLG_ENABLECACHE       0x00000001  /* enable and show PW cache checkbox */
#define AUTHDLG_CHECKCACHE        0x00000002  /* check PW cache checkbox by default */
#define AUTHDLG_CACHEINVALID      0x00000004  /* cached PW was invalid (special text) */
#define AUTHDLG_USE_DEFAULT_NAME  0x00000008  /* enable and show use "guest" box */
#define AUTHDLG_CHECKDEFAULT_NAME 0x00000010  /* check "guest" box               */
#define AUTHDLG_LOGON             0x00000020  /* include Windows logo bitmap */

#define AUTHDLG_ENABLECACHE       0x00000001  /* enable and show PW cache checkbox */
#define AUTHDLG_CHECKCACHE        0x00000002  /* check PW cache checkbox by default */
#define AUTHDLG_CACHEINVALID      0x00000004  /* cached PW was invalid (special text) */
#define AUTHDLG_USE_DEFAULT_NAME  0x00000008  /* enable and show use "guest" box */
#define AUTHDLG_CHECKDEFAULT_NAME 0x00000010  /* check "guest" box               */
#define AUTHDLG_LOGON             0x00000020  /* include Windows logo bitmap */

DWORD
NPSERVICE
NPSAuthenticationDialog(
    LPAUTHDLGSTRUCT lpAuthDlgStruct
    );

DWORD
NPSERVICE
NPSGetProviderHandle( 
	PHPROVIDER phProvider
	);

DWORD
NPSERVICE
NPSGetProviderName(
	HPROVIDER hProvider,
	LPCSTR FAR * lpszProviderName
	);

DWORD
NPSERVICE
NPSGetSectionName(
	HPROVIDER hProvider,
	LPCSTR FAR * lpszSectionName
	);

DWORD
NPSERVICE NPSSetExtendedError (
	DWORD NetSpecificError,
	LPSTR lpExtendedErrorText 
    );

VOID
NPSERVICE NPSSetCustomText (
	LPSTR lpCustomErrorText 
    );

DWORD
NPSERVICE
NPSCopyString (
    LPCTSTR lpString,
    LPVOID  lpBuffer,
    LPDWORD lpdwBufferSize
    );

DWORD
NPSERVICE
NPSDeviceGetNumber (
    LPTSTR  lpLocalName,
    LPDWORD lpdwNumber,
    LPDWORD lpdwType
    );

DWORD
NPSERVICE
NPSDeviceGetString (
    DWORD   dwNumber,
    DWORD   dwType,
    LPTSTR  lpLocalName,
    LPDWORD lpdwBufferSize
    );

// Notification Service.

enum NOTIFYTYPE { NotifyAddConnection, 
                  NotifyCancelConnection, 
                  NotifyGetConnectionPerformance };

#define NOTIFY_PRE              0x00
#define NOTIFY_POST             0x01

typedef struct _NOTIFYINFO {
    DWORD cbStructure;          /* size of NOTIFYINFO */
    DWORD  dwNotifyStatus;      /* Pre/post notification status */
    DWORD  dwOperationStatus;   /* Status of operation */
    LPVOID lpNPContext;         /* NP context */
} NOTIFYINFO, FAR *LPNOTIFYINFO;

typedef struct _NOTIFYADD {
    DWORD cbStructure;          /* size of NOTIFYADD */
    HWND hwndOwner;             /* hWnd for UI */
    NETRESOURCE NetResource;    /* Resource to add */
    DWORD dwAddFlags;           /* Add flags */
    LPTSTR lpAccessName;        /* System name for connection */
    LPDWORD lpBufferSize;       /* Size of AccessName buffer */
    DWORD dwResult;             /* Info about connection */
    DWORD dwAddContext;         /* Context of add connection */
} NOTIFYADD, FAR *LPNOTIFYADD;

#define CONNECT_CTXT_RESTORE        0x00000001
#define CONNECT_CTXT_GLOBAL         0x00000002
#define CONNECT_CTXT_PROVIDER       0x00000004
#define CONNECT_CTXT_SINGLE         0x00000008

typedef struct _NOTIFYCANCEL {
    DWORD cbStructure;          /* size of NOTIFYCANCEL */
    LPTSTR lpName;              /* Local device name or remote name of resource */
    LPTSTR lpProvider;          /* Provider name of resource cancelled */
    DWORD dwFlags;              /* Cancel flags */
    BOOL fForce;                /* Cancel force */
} NOTIFYCANCEL, FAR *LPNOTIFYCANCEL;

typedef struct _NOTIFYPERFORMANCE {
	DWORD cbStructure;          /* size of NOTIFYPERFORMANCE */
	LPTSTR lpRemoteName;        /* network resource name */
	LPTSTR lpProviderName;      /* provider to try/provider that responded */
	LPNETCONNECTINFOSTRUCT lpNetConnectInfo; /* performance information requested/returned */
} NOTIFYPERFORMANCE, FAR *LPNOTIFYPERFORMANCE;

typedef DWORD (FAR PASCAL *NOTIFYCALLBACK)( LPNOTIFYINFO lpNotifyInfo, LPVOID lpOperationInfo );

DWORD
NPSERVICE
NPSNotifyRegister(
    enum NOTIFYTYPE NotifyType,
    NOTIFYCALLBACK P_FNotifyCallBack
    );

LPVOID
NPSERVICE
NPSNotifyGetContext (
    NOTIFYCALLBACK P_FNotifyCallBack
    );

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#ifndef RC_INVOKED
#pragma pack()
#endif  /* !RC_INVOKED */

#endif  /* !_INC_NETSPI */

