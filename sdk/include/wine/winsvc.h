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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __WINE_WINSVC_H
#define __WINE_WINSVC_H

#ifdef __cplusplus
extern "C" {
#endif /* defined(__cplusplus) */

#ifndef WINADVAPI
#ifdef _ADVAPI32_
#define WINADVAPI
#else
#define WINADVAPI DECLSPEC_IMPORT
#endif
#endif

/* Service database names */
#define SERVICES_ACTIVE_DATABASEA     "ServicesActive"
#define SERVICES_FAILED_DATABASEA     "ServicesFailed"

#if defined(_MSC_VER) || defined(__MINGW32__)
# define SERVICES_ACTIVE_DATABASEW    L"ServicesActive"
# define SERVICES_FAILED_DATABASEW    L"ServicesFailed"
#else
static const WCHAR SERVICES_ACTIVE_DATABASEW[] = { 'S','e','r','v','i','c','e','s','A','c','t','i','v','e',0 };
static const WCHAR SERVICES_FAILED_DATABASEW[] = { 'S','e','r','v','i','c','e','s','F','a','i','l','e','d',0 };
#endif

#define SERVICES_ACTIVE_DATABASE      WINELIB_NAME_AW( SERVICES_ACTIVE_DATABASE )
#define SERVICES_FAILED_DATABASE      WINELIB_NAME_AW( SERVICES_FAILED_DATABASE )

/* Service State requests */
#define SERVICE_ACTIVE                        0x00000001
#define SERVICE_INACTIVE                      0x00000002
#define SERVICE_STATE_ALL                     (SERVICE_ACTIVE | SERVICE_INACTIVE)

/* Controls */
#define SERVICE_CONTROL_STOP                  0x00000001
#define SERVICE_CONTROL_PAUSE                 0x00000002
#define SERVICE_CONTROL_CONTINUE              0x00000003
#define SERVICE_CONTROL_INTERROGATE           0x00000004
#define SERVICE_CONTROL_SHUTDOWN              0x00000005
#define SERVICE_CONTROL_PARAMCHANGE           0x00000006
#define SERVICE_CONTROL_NETBINDADD            0x00000007
#define SERVICE_CONTROL_NETBINDREMOVE         0x00000008
#define SERVICE_CONTROL_NETBINDENABLE         0x00000009
#define SERVICE_CONTROL_NETBINDDISABLE        0x0000000A
#define SERVICE_CONTROL_DEVICEEVENT           0x0000000B
#define SERVICE_CONTROL_HARDWAREPROFILECHANGE 0x0000000C
#define SERVICE_CONTROL_POWEREVENT            0x0000000D
#define SERVICE_CONTROL_SESSIONCHANGE         0x0000000E
#define SERVICE_CONTROL_PRESHUTDOWN           0x0000000F

/* Service State */
#define SERVICE_STOPPED          0x00000001
#define SERVICE_START_PENDING    0x00000002
#define SERVICE_STOP_PENDING     0x00000003
#define SERVICE_RUNNING          0x00000004
#define SERVICE_CONTINUE_PENDING 0x00000005
#define SERVICE_PAUSE_PENDING    0x00000006
#define SERVICE_PAUSED           0x00000007

/* Controls Accepted */
#define SERVICE_ACCEPT_STOP                  0x00000001
#define SERVICE_ACCEPT_PAUSE_CONTINUE        0x00000002
#define SERVICE_ACCEPT_SHUTDOWN              0x00000004
#define SERVICE_ACCEPT_PARAMCHANGE           0x00000008
#define SERVICE_ACCEPT_NETBINDCHANGE         0x00000010
#define SERVICE_ACCEPT_HARDWAREPROFILECHANGE 0x00000020
#define SERVICE_ACCEPT_POWEREVENT            0x00000040
#define SERVICE_ACCEPT_SESSIONCHANGE         0x00000080
#define SERVICE_ACCEPT_PRESHUTDOWN           0x00000100
#define SERVICE_ACCEPT_TIMECHANGE            0x00000200
#define SERVICE_ACCEPT_TRIGGEREVENT          0x00000400

/* Service Control Manager Object access types */
#define SC_MANAGER_CONNECT            0x0001
#define SC_MANAGER_CREATE_SERVICE     0x0002
#define SC_MANAGER_ENUMERATE_SERVICE  0x0004
#define SC_MANAGER_LOCK               0x0008
#define SC_MANAGER_QUERY_LOCK_STATUS  0x0010
#define SC_MANAGER_MODIFY_BOOT_CONFIG 0x0020
#define SC_MANAGER_ALL_ACCESS         ( STANDARD_RIGHTS_REQUIRED      | \
                                        SC_MANAGER_CONNECT            | \
                                        SC_MANAGER_CREATE_SERVICE     | \
                                        SC_MANAGER_ENUMERATE_SERVICE  | \
                                        SC_MANAGER_LOCK               | \
                                        SC_MANAGER_QUERY_LOCK_STATUS  | \
                                        SC_MANAGER_MODIFY_BOOT_CONFIG )

#define SERVICE_QUERY_CONFIG         0x0001
#define SERVICE_CHANGE_CONFIG        0x0002
#define SERVICE_QUERY_STATUS         0x0004
#define SERVICE_ENUMERATE_DEPENDENTS 0x0008
#define SERVICE_START                0x0010
#define SERVICE_STOP                 0x0020
#define SERVICE_PAUSE_CONTINUE       0x0040
#define SERVICE_INTERROGATE          0x0080
#define SERVICE_USER_DEFINED_CONTROL 0x0100

#define SERVICE_ALL_ACCESS           ( STANDARD_RIGHTS_REQUIRED     | \
                                       SERVICE_QUERY_CONFIG         | \
                                       SERVICE_CHANGE_CONFIG        | \
                                       SERVICE_QUERY_STATUS         | \
                                       SERVICE_ENUMERATE_DEPENDENTS | \
                                       SERVICE_START                | \
                                       SERVICE_STOP                 | \
                                       SERVICE_PAUSE_CONTINUE       | \
                                       SERVICE_INTERROGATE          | \
                                       SERVICE_USER_DEFINED_CONTROL )

#define SERVICE_NO_CHANGE 0xffffffff

#define SERVICE_RUNS_IN_SYSTEM_PROCESS 0x00000001

/* Handle types */

DECLARE_HANDLE(SC_HANDLE);
typedef SC_HANDLE *LPSC_HANDLE;
DECLARE_HANDLE(SERVICE_STATUS_HANDLE);
typedef LPVOID SC_LOCK;

/* Service status structure */

typedef struct _SERVICE_STATUS {
  DWORD dwServiceType;
  DWORD dwCurrentState;
  DWORD dwControlsAccepted;
  DWORD dwWin32ExitCode;
  DWORD dwServiceSpecificExitCode;
  DWORD dwCheckPoint;
  DWORD dwWaitHint;
} SERVICE_STATUS, *LPSERVICE_STATUS;

/* Service status process structure */

typedef struct _SERVICE_STATUS_PROCESS
{
  DWORD dwServiceType;
  DWORD dwCurrentState;
  DWORD dwControlsAccepted;
  DWORD dwWin32ExitCode;
  DWORD dwServiceSpecificExitCode;
  DWORD dwCheckPoint;
  DWORD dwWaitHint;
  DWORD dwProcessId;
  DWORD dwServiceFlags;
} SERVICE_STATUS_PROCESS, *LPSERVICE_STATUS_PROCESS;

#define SERVICE_NOTIFY_STATUS_CHANGE 2

#define SERVICE_NOTIFY_STOPPED 0x1
#define SERVICE_NOTIFY_START_PENDING 0x2
#define SERVICE_NOTIFY_STOP_PENDING 0x4
#define SERVICE_NOTIFY_RUNNING 0x8
#define SERVICE_NOTIFY_CONTINUE_PENDING 0x10
#define SERVICE_NOTIFY_PAUSE_PENDING 0x20
#define SERVICE_NOTIFY_PAUSED 0x40
#define SERVICE_NOTIFY_CREATED 0x80
#define SERVICE_NOTIFY_DELETED 0x100
#define SERVICE_NOTIFY_DELETE_PENDING 0x200

typedef void (CALLBACK *PFN_SC_NOTIFY_CALLBACK)(void *);

typedef struct _SERVICE_NOTIFY_2A {
    DWORD dwVersion;
    PFN_SC_NOTIFY_CALLBACK pfnNotifyCallback;
    void *pContext;
    DWORD dwNotificationStatus;
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD dwNotificationTriggered;
    char *pszServiceNames;
} SERVICE_NOTIFY_2A, SERVICE_NOTIFYA;

typedef struct _SERVICE_NOTIFY_2W {
    DWORD dwVersion;
    PFN_SC_NOTIFY_CALLBACK pfnNotifyCallback;
    void *pContext;
    DWORD dwNotificationStatus;
    SERVICE_STATUS_PROCESS ServiceStatus;
    DWORD dwNotificationTriggered;
    WCHAR *pszServiceNames;
} SERVICE_NOTIFY_2W, SERVICE_NOTIFYW;

typedef enum _SC_STATUS_TYPE {
  SC_STATUS_PROCESS_INFO      = 0
} SC_STATUS_TYPE;

/* Service main function prototype */

typedef VOID (CALLBACK *LPSERVICE_MAIN_FUNCTIONA)(DWORD,LPSTR*);
typedef VOID (CALLBACK *LPSERVICE_MAIN_FUNCTIONW)(DWORD,LPWSTR*);
DECL_WINELIB_TYPE_AW(LPSERVICE_MAIN_FUNCTION)

/* Service start table */

typedef struct _SERVICE_TABLE_ENTRYA {
    LPSTR                    lpServiceName;
    LPSERVICE_MAIN_FUNCTIONA lpServiceProc;
} SERVICE_TABLE_ENTRYA, *LPSERVICE_TABLE_ENTRYA;

typedef struct _SERVICE_TABLE_ENTRYW {
  LPWSTR                   lpServiceName;
  LPSERVICE_MAIN_FUNCTIONW lpServiceProc;
} SERVICE_TABLE_ENTRYW, *LPSERVICE_TABLE_ENTRYW;

DECL_WINELIB_TYPE_AW(SERVICE_TABLE_ENTRY)
DECL_WINELIB_TYPE_AW(LPSERVICE_TABLE_ENTRY)

/* Service status enumeration structure */

typedef struct _ENUM_SERVICE_STATUSA {
  LPSTR          lpServiceName;
  LPSTR          lpDisplayName;
  SERVICE_STATUS ServiceStatus;
} ENUM_SERVICE_STATUSA, *LPENUM_SERVICE_STATUSA;

typedef struct _ENUM_SERVICE_STATUSW {
    LPWSTR         lpServiceName;
    LPWSTR         lpDisplayName;
    SERVICE_STATUS ServiceStatus;
} ENUM_SERVICE_STATUSW, *LPENUM_SERVICE_STATUSW;

DECL_WINELIB_TYPE_AW(ENUM_SERVICE_STATUS)
DECL_WINELIB_TYPE_AW(LPENUM_SERVICE_STATUS)

typedef struct _ENUM_SERVICE_STATUS_PROCESSA {
  LPSTR          lpServiceName;
  LPSTR          lpDisplayName;
  SERVICE_STATUS_PROCESS ServiceStatusProcess;
} ENUM_SERVICE_STATUS_PROCESSA, *LPENUM_SERVICE_STATUS_PROCESSA;

typedef struct _ENUM_SERVICE_STATUS_PROCESSW {
    LPWSTR         lpServiceName;
    LPWSTR         lpDisplayName;
    SERVICE_STATUS_PROCESS ServiceStatusProcess;
} ENUM_SERVICE_STATUS_PROCESSW, *LPENUM_SERVICE_STATUS_PROCESSW;

DECL_WINELIB_TYPE_AW(ENUM_SERVICE_STATUS_PROCESS)
DECL_WINELIB_TYPE_AW(LPENUM_SERVICE_STATUS_PROCESS)

typedef enum _SC_ENUM_TYPE {
    SC_ENUM_PROCESS_INFO      = 0
} SC_ENUM_TYPE;

typedef struct _QUERY_SERVICE_CONFIGA {
    DWORD   dwServiceType;
    DWORD   dwStartType;
    DWORD   dwErrorControl;
    LPSTR   lpBinaryPathName;
    LPSTR   lpLoadOrderGroup;
    DWORD   dwTagId;
    LPSTR   lpDependencies;
    LPSTR   lpServiceStartName;
    LPSTR   lpDisplayName;
} QUERY_SERVICE_CONFIGA, *LPQUERY_SERVICE_CONFIGA;

typedef struct _QUERY_SERVICE_CONFIGW {
    DWORD   dwServiceType;
    DWORD   dwStartType;
    DWORD   dwErrorControl;
    LPWSTR  lpBinaryPathName;
    LPWSTR  lpLoadOrderGroup;
    DWORD   dwTagId;
    LPWSTR  lpDependencies;
    LPWSTR  lpServiceStartName;
    LPWSTR  lpDisplayName;
} QUERY_SERVICE_CONFIGW, *LPQUERY_SERVICE_CONFIGW;

/* defines and structures for ChangeServiceConfig2 */
#define SERVICE_CONFIG_DESCRIPTION              1
#define SERVICE_CONFIG_FAILURE_ACTIONS          2
#define SERVICE_CONFIG_DELAYED_AUTO_START_INFO  3
#define SERVICE_CONFIG_FAILURE_ACTIONS_FLAG     4
#define SERVICE_CONFIG_SERVICE_SID_INFO         5
#define SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO 6
#define SERVICE_CONFIG_PRESHUTDOWN_INFO         7


typedef struct _SERVICE_DESCRIPTIONA {
   LPSTR lpDescription;
} SERVICE_DESCRIPTIONA,*LPSERVICE_DESCRIPTIONA;

typedef struct _SERVICE_DESCRIPTIONW {
   LPWSTR lpDescription;
} SERVICE_DESCRIPTIONW,*LPSERVICE_DESCRIPTIONW;

DECL_WINELIB_TYPE_AW(SERVICE_DESCRIPTION)
DECL_WINELIB_TYPE_AW(LPSERVICE_DESCRIPTION)

typedef enum _SC_ACTION_TYPE {
    SC_ACTION_NONE        = 0,
    SC_ACTION_RESTART     = 1,
    SC_ACTION_REBOOT      = 2,
    SC_ACTION_RUN_COMMAND = 3
} SC_ACTION_TYPE;

typedef struct _SC_ACTION {
   SC_ACTION_TYPE  Type;
   DWORD       Delay;
} SC_ACTION,*LPSC_ACTION;

typedef struct _SERVICE_FAILURE_ACTIONSA {
   DWORD   dwResetPeriod;
   LPSTR   lpRebootMsg;
   LPSTR   lpCommand;
   DWORD   cActions;
   SC_ACTION * lpsaActions;
} SERVICE_FAILURE_ACTIONSA,*LPSERVICE_FAILURE_ACTIONSA;

typedef struct _SERVICE_FAILURE_ACTIONSW {
   DWORD   dwResetPeriod;
   LPWSTR  lpRebootMsg;
   LPWSTR  lpCommand;
   DWORD   cActions;
   SC_ACTION * lpsaActions;
} SERVICE_FAILURE_ACTIONSW,*LPSERVICE_FAILURE_ACTIONSW;

DECL_WINELIB_TYPE_AW(SERVICE_FAILURE_ACTIONS)
DECL_WINELIB_TYPE_AW(LPSERVICE_FAILURE_ACTIONS)

typedef struct _SERVICE_DELAYED_AUTO_START_INFO {
    BOOL fDelayedAutostart;
} SERVICE_DELAYED_AUTO_START_INFO,*LPSERVICE_DELAYED_AUTO_START_INFO;

typedef struct _SERVICE_FAILURE_ACTIONS_FLAG {
    BOOL fFailureActionsOnNonCrashFailures;
} SERVICE_FAILURE_ACTIONS_FLAG,*LPSERVICE_FAILURE_ACTIONS_FLAG;

typedef struct _SERVICE_SID_INFO {
    DWORD dwServiceSidType;
} SERVICE_SID_INFO,*LPSERVICE_SID_INFO;

typedef struct _SERVICE_REQUIRED_PRIVILEGES_INFOA {
    LPSTR pmszRequiredPrivileges;
} SERVICE_REQUIRED_PRIVILEGES_INFOA,*LPSERVICE_REQUIRED_PRIVILEGES_INFOA;

typedef struct _SERVICE_REQUIRED_PRIVILEGES_INFOW {
    LPWSTR pmszRequiredPrivileges;
} SERVICE_REQUIRED_PRIVILEGES_INFOW,*LPSERVICE_REQUIRED_PRIVILEGES_INFOW;

DECL_WINELIB_TYPE_AW(SERVICE_REQUIRED_PRIVILEGES_INFO)
DECL_WINELIB_TYPE_AW(LPSERVICE_REQUIRED_PRIVILEGES_INFO)

typedef struct _SERVICE_PRESHUTDOWN_INFO {
    DWORD dwPreshutdownTimeout;
} SERVICE_PRESHUTDOWN_INFO,*LPSERVICE_PRESHUTDOWN_INFO;

typedef struct _QUERY_SERVICE_LOCK_STATUSA
{
  DWORD fIsLocked;
  LPSTR lpLockOwner;
  DWORD dwLockDuration;
} QUERY_SERVICE_LOCK_STATUSA, *LPQUERY_SERVICE_LOCK_STATUSA;

typedef struct _QUERY_SERVICE_LOCK_STATUSW
{
  DWORD fIsLocked;
  LPWSTR lpLockOwner;
  DWORD dwLockDuration;
} QUERY_SERVICE_LOCK_STATUSW, *LPQUERY_SERVICE_LOCK_STATUSW;

DECL_WINELIB_TYPE_AW(QUERY_SERVICE_LOCK_STATUS)

/* Service control handler function prototype */

typedef VOID (WINAPI *LPHANDLER_FUNCTION)(DWORD);
typedef DWORD (WINAPI *LPHANDLER_FUNCTION_EX)(DWORD,DWORD,LPVOID,LPVOID);

/* API function prototypes */

WINADVAPI BOOL        WINAPI ChangeServiceConfigA(SC_HANDLE,DWORD,DWORD,DWORD,LPCSTR,LPCSTR,LPDWORD,LPCSTR,LPCSTR,LPCSTR,LPCSTR);
WINADVAPI BOOL        WINAPI ChangeServiceConfigW(SC_HANDLE,DWORD,DWORD,DWORD,LPCWSTR,LPCWSTR,LPDWORD,LPCWSTR,LPCWSTR,LPCWSTR,LPCWSTR);
#define                      ChangeServiceConfig WINELIB_NAME_AW(ChangeServiceConfig)
WINADVAPI BOOL        WINAPI ChangeServiceConfig2A(SC_HANDLE,DWORD,LPVOID);
WINADVAPI BOOL        WINAPI ChangeServiceConfig2W(SC_HANDLE,DWORD,LPVOID);
#define                      ChangeServiceConfig2 WINELIB_NAME_AW(ChangeServiceConfig2)
WINADVAPI BOOL        WINAPI CloseServiceHandle(SC_HANDLE);
WINADVAPI BOOL        WINAPI ControlService(SC_HANDLE,DWORD,LPSERVICE_STATUS);
WINADVAPI SC_HANDLE   WINAPI CreateServiceA(SC_HANDLE,LPCSTR,LPCSTR,DWORD,DWORD,DWORD,DWORD,LPCSTR,LPCSTR,LPDWORD,LPCSTR,LPCSTR,LPCSTR);
WINADVAPI SC_HANDLE   WINAPI CreateServiceW(SC_HANDLE,LPCWSTR,LPCWSTR,DWORD,DWORD,DWORD,DWORD,LPCWSTR,LPCWSTR,LPDWORD,LPCWSTR,LPCWSTR,LPCWSTR);
#define                      CreateService WINELIB_NAME_AW(CreateService)
WINADVAPI BOOL        WINAPI DeleteService(SC_HANDLE);
WINADVAPI BOOL        WINAPI EnumDependentServicesA(SC_HANDLE,DWORD,LPENUM_SERVICE_STATUSA,DWORD,LPDWORD,LPDWORD);
WINADVAPI BOOL        WINAPI EnumDependentServicesW(SC_HANDLE,DWORD,LPENUM_SERVICE_STATUSW,DWORD,LPDWORD,LPDWORD);
#define                      EnumDependentServices WINELIB_NAME_AW(EnumDependentServices)
WINADVAPI BOOL        WINAPI EnumServicesStatusA(SC_HANDLE,DWORD,DWORD,LPENUM_SERVICE_STATUSA,DWORD,LPDWORD,LPDWORD,LPDWORD);
WINADVAPI BOOL        WINAPI EnumServicesStatusW(SC_HANDLE,DWORD,DWORD,LPENUM_SERVICE_STATUSW,DWORD,LPDWORD,LPDWORD,LPDWORD);
#define                      EnumServicesStatus WINELIB_NAME_AW(EnumServicesStatus)
WINADVAPI BOOL        WINAPI EnumServicesStatusExA(SC_HANDLE,SC_ENUM_TYPE,DWORD,DWORD,LPBYTE,DWORD,LPDWORD,LPDWORD,LPDWORD,LPCSTR);
WINADVAPI BOOL        WINAPI EnumServicesStatusExW(SC_HANDLE,SC_ENUM_TYPE,DWORD,DWORD,LPBYTE,DWORD,LPDWORD,LPDWORD,LPDWORD,LPCWSTR);
#define                      EnumServicesStatus WINELIB_NAME_AW(EnumServicesStatus)
WINADVAPI BOOL        WINAPI GetServiceDisplayNameA(SC_HANDLE,LPCSTR,LPSTR,LPDWORD);
WINADVAPI BOOL        WINAPI GetServiceDisplayNameW(SC_HANDLE,LPCWSTR,LPWSTR,LPDWORD);
#define                      GetServiceDisplayName WINELIB_NAME_AW(GetServiceDisplayName)
WINADVAPI BOOL        WINAPI GetServiceKeyNameA(SC_HANDLE,LPCSTR,LPSTR,LPDWORD);
WINADVAPI BOOL        WINAPI GetServiceKeyNameW(SC_HANDLE,LPCWSTR,LPWSTR,LPDWORD);
#define                      GetServiceKeyName WINELIB_NAME_AW(GetServiceKeyName)
WINADVAPI SC_LOCK     WINAPI LockServiceDatabase(SC_HANDLE);
WINADVAPI BOOL        WINAPI NotifyBootConfigStatus(BOOL);
WINADVAPI DWORD       WINAPI NotifyServiceStatusChangeW(SC_HANDLE,DWORD,SERVICE_NOTIFYW*);
WINADVAPI SC_HANDLE   WINAPI OpenSCManagerA(LPCSTR,LPCSTR,DWORD);
WINADVAPI SC_HANDLE   WINAPI OpenSCManagerW(LPCWSTR,LPCWSTR,DWORD);
#define                      OpenSCManager WINELIB_NAME_AW(OpenSCManager)
WINADVAPI SC_HANDLE   WINAPI OpenServiceA(SC_HANDLE,LPCSTR,DWORD);
WINADVAPI SC_HANDLE   WINAPI OpenServiceW(SC_HANDLE,LPCWSTR,DWORD);
#define                      OpenService WINELIB_NAME_AW(OpenService)
WINADVAPI BOOL        WINAPI QueryServiceStatus(SC_HANDLE,LPSERVICE_STATUS);
WINADVAPI BOOL        WINAPI QueryServiceStatusEx(SC_HANDLE,SC_STATUS_TYPE,LPBYTE,DWORD,LPDWORD);
WINADVAPI BOOL        WINAPI QueryServiceConfigA(SC_HANDLE,LPQUERY_SERVICE_CONFIGA,DWORD,LPDWORD);
WINADVAPI BOOL        WINAPI QueryServiceConfigW(SC_HANDLE,LPQUERY_SERVICE_CONFIGW,DWORD,LPDWORD);
#define                      QueryServiceConfig WINELIB_NAME_AW(QueryServiceConfig)
WINADVAPI BOOL        WINAPI QueryServiceConfig2A(SC_HANDLE,DWORD,LPBYTE,DWORD,LPDWORD);
WINADVAPI BOOL        WINAPI QueryServiceConfig2W(SC_HANDLE,DWORD,LPBYTE,DWORD,LPDWORD);
#define                      QueryServiceConfig2 WINELIB_NAME_AW(QueryServiceConfig2)
WINADVAPI BOOL        WINAPI QueryServiceLockStatusA(SC_HANDLE,LPQUERY_SERVICE_LOCK_STATUSA,DWORD,LPDWORD);
WINADVAPI BOOL        WINAPI QueryServiceLockStatusW(SC_HANDLE,LPQUERY_SERVICE_LOCK_STATUSW,DWORD,LPDWORD);
#define                      QueryServiceLockStatus WINELIB_NAME_AW(QueryServiceLockStatus)
WINADVAPI BOOL        WINAPI QueryServiceObjectSecurity(SC_HANDLE,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR,DWORD,LPDWORD);
WINADVAPI SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerA(LPCSTR,LPHANDLER_FUNCTION);
WINADVAPI SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerW(LPCWSTR,LPHANDLER_FUNCTION);
#define                      RegisterServiceCtrlHandler WINELIB_NAME_AW(RegisterServiceCtrlHandler)
WINADVAPI SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerExA(LPCSTR,LPHANDLER_FUNCTION_EX,LPVOID);
WINADVAPI SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerExW(LPCWSTR,LPHANDLER_FUNCTION_EX,LPVOID);
#define                      RegisterServiceCtrlHandlerEx WINELIB_NAME_AW(RegisterServiceCtrlHandlerEx)
WINADVAPI BOOL        WINAPI SetServiceObjectSecurity(SC_HANDLE,SECURITY_INFORMATION,PSECURITY_DESCRIPTOR);
WINADVAPI BOOL        WINAPI SetServiceStatus(SERVICE_STATUS_HANDLE,LPSERVICE_STATUS);
WINADVAPI BOOL        WINAPI StartServiceA(SC_HANDLE,DWORD,LPCSTR*);
WINADVAPI BOOL        WINAPI StartServiceW(SC_HANDLE,DWORD,LPCWSTR*);
#define                      StartService WINELIB_NAME_AW(StartService)
WINADVAPI BOOL        WINAPI StartServiceCtrlDispatcherA(const SERVICE_TABLE_ENTRYA*);
WINADVAPI BOOL        WINAPI StartServiceCtrlDispatcherW(const SERVICE_TABLE_ENTRYW*);
#define                      StartServiceCtrlDispatcher WINELIB_NAME_AW(StartServiceCtrlDispatcher)
WINADVAPI BOOL        WINAPI UnlockServiceDatabase(SC_LOCK);

#ifdef __cplusplus
} /* extern "C" */
#endif /* defined(__cplusplus) */

#endif /* !defined(__WINE_WINSVC_H) */
