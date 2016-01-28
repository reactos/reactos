#ifndef _WINSVC_
#define _WINSVC_

#ifdef __cplusplus
extern "C" {
#endif
#define SERVICES_ACTIVE_DATABASEA "ServicesActive"
#define SERVICES_ACTIVE_DATABASEW L"ServicesActive"
#define SERVICES_FAILED_DATABASEA "ServicesFailed"
#define SERVICES_FAILED_DATABASEW L"ServicesFailed"
#define SC_GROUP_IDENTIFIERA '+'
#define SC_GROUP_IDENTIFIERW L'+'
#define SC_MANAGER_ALL_ACCESS	0xf003f
#define SC_MANAGER_CONNECT	1
#define SC_MANAGER_CREATE_SERVICE	2
#define SC_MANAGER_ENUMERATE_SERVICE	4
#define SC_MANAGER_LOCK	8
#define SC_MANAGER_QUERY_LOCK_STATUS	16
#define SC_MANAGER_MODIFY_BOOT_CONFIG	32
#define SERVICE_NO_CHANGE 0xffffffff
#define SERVICE_STOPPED	1
#define SERVICE_START_PENDING	2
#define SERVICE_STOP_PENDING	3
#define SERVICE_RUNNING	4
#define SERVICE_CONTINUE_PENDING	5
#define SERVICE_PAUSE_PENDING	6
#define SERVICE_PAUSED	7
#define SERVICE_ACCEPT_STOP	1
#define SERVICE_ACCEPT_PAUSE_CONTINUE	2
#define SERVICE_ACCEPT_SHUTDOWN 4
#define SERVICE_ACCEPT_PARAMCHANGE    8
#define SERVICE_ACCEPT_NETBINDCHANGE  16
#define SERVICE_ACCEPT_HARDWAREPROFILECHANGE   32
#define SERVICE_ACCEPT_POWEREVENT              64
#define SERVICE_ACCEPT_SESSIONCHANGE           128
#define SERVICE_CONTROL_STOP	1
#define SERVICE_CONTROL_PAUSE	2
#define SERVICE_CONTROL_CONTINUE	3
#define SERVICE_CONTROL_INTERROGATE	4
#define SERVICE_CONTROL_SHUTDOWN	5
#define SERVICE_CONTROL_PARAMCHANGE     6
#define SERVICE_CONTROL_NETBINDADD      7
#define SERVICE_CONTROL_NETBINDREMOVE   8
#define SERVICE_CONTROL_NETBINDENABLE   9
#define SERVICE_CONTROL_NETBINDDISABLE  10
#define SERVICE_CONTROL_DEVICEEVENT     11
#define SERVICE_CONTROL_HARDWAREPROFILECHANGE 12
#define SERVICE_CONTROL_POWEREVENT            13
#define SERVICE_CONTROL_SESSIONCHANGE         14
#define SERVICE_ACTIVE 1
#define SERVICE_INACTIVE 2
#define SERVICE_STATE_ALL 3
#define SERVICE_QUERY_CONFIG 1
#define SERVICE_CHANGE_CONFIG 2
#define SERVICE_QUERY_STATUS 4
#define SERVICE_ENUMERATE_DEPENDENTS 8
#define SERVICE_START 16
#define SERVICE_STOP 32
#define SERVICE_PAUSE_CONTINUE 64
#define SERVICE_INTERROGATE 128
#define SERVICE_USER_DEFINED_CONTROL 256
#define SERVICE_ALL_ACCESS (STANDARD_RIGHTS_REQUIRED|SERVICE_QUERY_CONFIG|SERVICE_CHANGE_CONFIG|SERVICE_QUERY_STATUS|SERVICE_ENUMERATE_DEPENDENTS|SERVICE_START|SERVICE_STOP|SERVICE_PAUSE_CONTINUE|SERVICE_INTERROGATE|SERVICE_USER_DEFINED_CONTROL)
#define SERVICE_RUNS_IN_SYSTEM_PROCESS 1

#define SERVICE_CONFIG_DESCRIPTION              1
#define SERVICE_CONFIG_FAILURE_ACTIONS          2
#define SERVICE_CONFIG_DELAYED_AUTO_START_INFO  3
#define SERVICE_CONFIG_FAILURE_ACTIONS_FLAG     4
#define SERVICE_CONFIG_SERVICE_SID_INFO         5
#define SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO 6
#define SERVICE_CONFIG_PRESHUTDOWN_INFO         7
#define SERVICE_CONFIG_TRIGGER_INFO             8
#define SERVICE_CONFIG_PREFERRED_NODE           9
#define SERVICE_CONFIG_RUNLEVEL_INFO            10

#define SERVICE_NOTIFY_STATUS_CHANGE_1 1
#define SERVICE_NOTIFY_STATUS_CHANGE_2 2
#define SERVICE_NOTIFY_STATUS_CHANGE   SERVICE_NOTIFY_STATUS_CHANGE_2

#define SERVICE_NOTIFY_STOPPED          1
#define SERVICE_NOTIFY_START_PENDING    2
#define SERVICE_NOTIFY_STOP_PENDING     4
#define SERVICE_NOTIFY_RUNNING          8
#define SERVICE_NOTIFY_CONTINUE_PENDING 16
#define SERVICE_NOTIFY_PAUSE_PENDING    32
#define SERVICE_NOTIFY_PAUSED           64
#define SERVICE_NOTIFY_CREATED          128
#define SERVICE_NOTIFY_DELETED          256
#define SERVICE_NOTIFY_DELETE_PENDING   512

#ifndef _SERVICE_PRESHUTDOWN_INFO_DEFINED_
#define _SERVICE_PRESHUTDOWN_INFO_DEFINED_
typedef struct _SERVICE_PRESHUTDOWN_INFO {
    DWORD dwPreshutdownTimeout;
} SERVICE_PRESHUTDOWN_INFO, *LPSERVICE_PRESHUTDOWN_INFO;
#endif

typedef struct _SERVICE_STATUS {
	DWORD dwServiceType;
	DWORD dwCurrentState;
	DWORD dwControlsAccepted;
	DWORD dwWin32ExitCode;
	DWORD dwServiceSpecificExitCode;
	DWORD dwCheckPoint;
	DWORD dwWaitHint;
} SERVICE_STATUS,*LPSERVICE_STATUS;
typedef struct _SERVICE_STATUS_PROCESS {
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
typedef enum _SC_STATUS_TYPE {
	SC_STATUS_PROCESS_INFO = 0
} SC_STATUS_TYPE;
typedef enum _SC_ENUM_TYPE {
        SC_ENUM_PROCESS_INFO = 0
} SC_ENUM_TYPE;
typedef struct _ENUM_SERVICE_STATUSA {
	LPSTR lpServiceName;
	LPSTR lpDisplayName;
	SERVICE_STATUS ServiceStatus;
} ENUM_SERVICE_STATUSA,*LPENUM_SERVICE_STATUSA;
typedef struct _ENUM_SERVICE_STATUSW {
	LPWSTR lpServiceName;
	LPWSTR lpDisplayName;
	SERVICE_STATUS ServiceStatus;
} ENUM_SERVICE_STATUSW,*LPENUM_SERVICE_STATUSW;
typedef struct _ENUM_SERVICE_STATUS_PROCESSA {
	LPSTR lpServiceName;
	LPSTR lpDisplayName;
	SERVICE_STATUS_PROCESS ServiceStatusProcess;
} ENUM_SERVICE_STATUS_PROCESSA,*LPENUM_SERVICE_STATUS_PROCESSA;
typedef struct _ENUM_SERVICE_STATUS_PROCESSW {
	LPWSTR lpServiceName;
	LPWSTR lpDisplayName;
	SERVICE_STATUS_PROCESS ServiceStatusProcess;
} ENUM_SERVICE_STATUS_PROCESSW,*LPENUM_SERVICE_STATUS_PROCESSW;
typedef struct _QUERY_SERVICE_CONFIGA {
	DWORD dwServiceType;
	DWORD dwStartType;
	DWORD dwErrorControl;
	LPSTR lpBinaryPathName;
	LPSTR lpLoadOrderGroup;
	DWORD dwTagId;
	LPSTR lpDependencies;
	LPSTR lpServiceStartName;
	LPSTR lpDisplayName;
} QUERY_SERVICE_CONFIGA,*LPQUERY_SERVICE_CONFIGA;
typedef struct _QUERY_SERVICE_CONFIGW {
	DWORD dwServiceType;
	DWORD dwStartType;
	DWORD dwErrorControl;
	LPWSTR lpBinaryPathName;
	LPWSTR lpLoadOrderGroup;
	DWORD dwTagId;
	LPWSTR lpDependencies;
	LPWSTR lpServiceStartName;
	LPWSTR lpDisplayName;
} QUERY_SERVICE_CONFIGW,*LPQUERY_SERVICE_CONFIGW;
typedef struct _QUERY_SERVICE_LOCK_STATUSA {
	DWORD fIsLocked;
	LPSTR lpLockOwner;
	DWORD dwLockDuration;
} QUERY_SERVICE_LOCK_STATUSA,*LPQUERY_SERVICE_LOCK_STATUSA;
typedef struct _QUERY_SERVICE_LOCK_STATUSW {
	DWORD fIsLocked;
	LPWSTR lpLockOwner;
	DWORD dwLockDuration;
} QUERY_SERVICE_LOCK_STATUSW,*LPQUERY_SERVICE_LOCK_STATUSW;
typedef void (WINAPI *LPSERVICE_MAIN_FUNCTIONA)(DWORD,LPSTR*);
typedef void (WINAPI *LPSERVICE_MAIN_FUNCTIONW)(DWORD,LPWSTR*);
typedef struct _SERVICE_TABLE_ENTRYA {
	LPSTR lpServiceName;
	LPSERVICE_MAIN_FUNCTIONA lpServiceProc;
} SERVICE_TABLE_ENTRYA,*LPSERVICE_TABLE_ENTRYA;
typedef struct _SERVICE_TABLE_ENTRYW {
	LPWSTR lpServiceName;
	LPSERVICE_MAIN_FUNCTIONW lpServiceProc;
} SERVICE_TABLE_ENTRYW,*LPSERVICE_TABLE_ENTRYW;
DECLARE_HANDLE(SC_HANDLE);
typedef SC_HANDLE *LPSC_HANDLE;
typedef PVOID SC_LOCK;
DECLARE_HANDLE(SERVICE_STATUS_HANDLE);
typedef VOID(WINAPI *LPHANDLER_FUNCTION)(DWORD);
typedef DWORD (WINAPI *LPHANDLER_FUNCTION_EX)(DWORD,DWORD,LPVOID,LPVOID);
typedef struct _SERVICE_DESCRIPTIONA {
	LPSTR lpDescription;
} SERVICE_DESCRIPTIONA,*LPSERVICE_DESCRIPTIONA;
typedef struct _SERVICE_DESCRIPTIONW {
	LPWSTR lpDescription;
} SERVICE_DESCRIPTIONW,*LPSERVICE_DESCRIPTIONW;
typedef enum _SC_ACTION_TYPE {
        SC_ACTION_NONE          = 0,
        SC_ACTION_RESTART       = 1,
        SC_ACTION_REBOOT        = 2,
        SC_ACTION_RUN_COMMAND   = 3
} SC_ACTION_TYPE;
typedef struct _SC_ACTION {
	SC_ACTION_TYPE	Type;
	DWORD		Delay;
} SC_ACTION,*LPSC_ACTION;
typedef struct _SERVICE_FAILURE_ACTIONSA {
	DWORD	dwResetPeriod;
	LPSTR	lpRebootMsg;
	LPSTR	lpCommand;
	DWORD	cActions;
	SC_ACTION * lpsaActions;
} SERVICE_FAILURE_ACTIONSA,*LPSERVICE_FAILURE_ACTIONSA;
typedef struct _SERVICE_FAILURE_ACTIONSW {
	DWORD	dwResetPeriod;
	LPWSTR	lpRebootMsg;
	LPWSTR	lpCommand;
	DWORD	cActions;
	SC_ACTION * lpsaActions;
} SERVICE_FAILURE_ACTIONSW,*LPSERVICE_FAILURE_ACTIONSW;

typedef struct _SERVICE_REQUIRED_PRIVILEGES_INFOA {
  LPSTR pmszRequiredPrivileges;
} SERVICE_REQUIRED_PRIVILEGES_INFOA, *LPSERVICE_REQUIRED_PRIVILEGES_INFOA;

typedef struct _SERVICE_REQUIRED_PRIVILEGES_INFOW {
  LPWSTR pmszRequiredPrivileges;
} SERVICE_REQUIRED_PRIVILEGES_INFOW, *LPSERVICE_REQUIRED_PRIVILEGES_INFOW;

typedef VOID (CALLBACK *PFN_SC_NOTIFY_CALLBACK)(_In_ PVOID);
typedef struct _SERVICE_NOTIFY_1 {
  DWORD dwVersion;
  PFN_SC_NOTIFY_CALLBACK pfnNotifyCallback;
  PVOID pContext;
  DWORD dwNotificationStatus;
  SERVICE_STATUS_PROCESS ServiceStatus;
} SERVICE_NOTIFY_1, *PSERVICE_NOTIFY_1;
typedef struct _SERVICE_NOTIFY_2A {
  DWORD dwVersion;
  PFN_SC_NOTIFY_CALLBACK pfnNotifyCallback;
  PVOID pContext;
  DWORD dwNotificationStatus;
  SERVICE_STATUS_PROCESS ServiceStatus;
  DWORD dwNotificationTriggered;
  LPSTR pszServiceNames;
} SERVICE_NOTIFY_2A, *PSERVICE_NOTIFY_2A;
typedef struct _SERVICE_NOTIFY_2W {
  DWORD dwVersion;
  PFN_SC_NOTIFY_CALLBACK pfnNotifyCallback;
  PVOID pContext;
  DWORD dwNotificationStatus;
  SERVICE_STATUS_PROCESS ServiceStatus;
  DWORD dwNotificationTriggered;
  LPWSTR pszServiceNames;
} SERVICE_NOTIFY_2W, *PSERVICE_NOTIFY_2W;
typedef SERVICE_NOTIFY_2A SERVICE_NOTIFYA, *PSERVICE_NOTIFYA;
typedef SERVICE_NOTIFY_2W SERVICE_NOTIFYW, *PSERVICE_NOTIFYW;

BOOL WINAPI ChangeServiceConfigA(_In_ SC_HANDLE, _In_ DWORD, _In_ DWORD, _In_ DWORD, _In_opt_ LPCSTR, _In_opt_ LPCSTR, _Out_opt_ LPDWORD, _In_opt_ LPCSTR, _In_opt_ LPCSTR, _In_opt_ LPCSTR, _In_opt_ LPCSTR);
BOOL WINAPI ChangeServiceConfigW(_In_ SC_HANDLE, _In_ DWORD, _In_ DWORD, _In_ DWORD, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR, _Out_opt_ LPDWORD, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR);
BOOL WINAPI ChangeServiceConfig2A(_In_ SC_HANDLE, _In_ DWORD, _In_opt_ LPVOID);
BOOL WINAPI ChangeServiceConfig2W(_In_ SC_HANDLE, _In_ DWORD, _In_opt_ LPVOID);
BOOL WINAPI CloseServiceHandle(_In_ SC_HANDLE);
BOOL WINAPI ControlService(_In_ SC_HANDLE, _In_ DWORD, _Out_ LPSERVICE_STATUS);
_Must_inspect_result_ SC_HANDLE WINAPI CreateServiceA(_In_ SC_HANDLE, _In_ LPCSTR, _In_opt_ LPCSTR, _In_ DWORD, _In_ DWORD, _In_ DWORD, _In_ DWORD, _In_opt_ LPCSTR, _In_opt_ LPCSTR, _Out_opt_ PDWORD, _In_opt_ LPCSTR, _In_opt_ LPCSTR, _In_opt_ LPCSTR);
_Must_inspect_result_ SC_HANDLE WINAPI CreateServiceW(_In_ SC_HANDLE, _In_ LPCWSTR, _In_opt_ LPCWSTR, _In_ DWORD, _In_ DWORD, _In_ DWORD, _In_ DWORD, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR, _Out_opt_ PDWORD, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR, _In_opt_ LPCWSTR);
BOOL WINAPI DeleteService(_In_ SC_HANDLE);

_Must_inspect_result_
BOOL
WINAPI
EnumDependentServicesA(
  _In_ SC_HANDLE hService,
  _In_ DWORD dwServiceState,
  _Out_writes_bytes_opt_(cbBufSize) LPENUM_SERVICE_STATUSA lpServices,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded,
  _Out_ LPDWORD lpServicesReturned);

_Must_inspect_result_
BOOL
WINAPI
EnumDependentServicesW(
  _In_ SC_HANDLE hService,
  _In_ DWORD dwServiceState,
  _Out_writes_bytes_opt_(cbBufSize) LPENUM_SERVICE_STATUSW lpServices,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded,
  _Out_ LPDWORD lpServicesReturned);

_Must_inspect_result_
BOOL
WINAPI
EnumServicesStatusA(
  _In_ SC_HANDLE hSCManager,
  _In_ DWORD dwServiceType,
  _In_ DWORD dwServiceState,
  _Out_writes_bytes_opt_(cbBufSize) LPENUM_SERVICE_STATUSA lpServices,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded,
  _Out_ LPDWORD lpServicesReturned,
  _Inout_opt_ LPDWORD lpResumeHandle);

_Must_inspect_result_
BOOL
WINAPI
EnumServicesStatusW(
  _In_ SC_HANDLE hSCManager,
  _In_ DWORD dwServiceType,
  _In_ DWORD dwServiceState,
  _Out_writes_bytes_opt_(cbBufSize) LPENUM_SERVICE_STATUSW lpServices,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded,
  _Out_ LPDWORD lpServicesReturned,
  _Inout_opt_ LPDWORD lpResumeHandle);

_Must_inspect_result_
BOOL
WINAPI
EnumServicesStatusExA(
  _In_ SC_HANDLE hSCManager,
  _In_ SC_ENUM_TYPE InfoLevel,
  _In_ DWORD dwServiceType,
  _In_ DWORD dwServiceState,
  _Out_writes_bytes_opt_(cbBufSize) LPBYTE lpServices,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded,
  _Out_ LPDWORD lpServicesReturned,
  _Inout_opt_ LPDWORD lpResumeHandle,
  _In_opt_ LPCSTR pszGroupName);

_Must_inspect_result_
BOOL
WINAPI
EnumServicesStatusExW(
  _In_ SC_HANDLE hSCManager,
  _In_ SC_ENUM_TYPE InfoLevel,
  _In_ DWORD dwServiceType,
  _In_ DWORD dwServiceState,
  _Out_writes_bytes_opt_(cbBufSize) LPBYTE lpServices,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded,
  _Out_ LPDWORD lpServicesReturned,
  _Inout_opt_ LPDWORD lpResumeHandle,
  _In_opt_ LPCWSTR pszGroupName);

_Must_inspect_result_
BOOL
WINAPI
GetServiceDisplayNameA(
  _In_ SC_HANDLE hSCManager,
  _In_ LPCSTR lpServiceName,
  _Out_writes_opt_(*lpcchBuffer) LPSTR lpDisplayName,
  _Inout_ LPDWORD lpcchBuffer);

_Must_inspect_result_
BOOL
WINAPI
GetServiceDisplayNameW(
  _In_ SC_HANDLE hSCManager,
  _In_ LPCWSTR lpServiceName,
  _Out_writes_opt_(*lpcchBuffer) LPWSTR lpDisplayName,
  _Inout_ LPDWORD lpcchBuffer);

_Must_inspect_result_
BOOL
WINAPI
GetServiceKeyNameA(
  _In_ SC_HANDLE hSCManager,
  _In_ LPCSTR lpDisplayName,
  _Out_writes_opt_(*lpcchBuffer) LPSTR lpServiceName,
  _Inout_ LPDWORD lpcchBuffer);

_Must_inspect_result_
BOOL
WINAPI
GetServiceKeyNameW(
  _In_ SC_HANDLE hSCManager,
  _In_ LPCWSTR lpDisplayName,
  _Out_writes_opt_(*lpcchBuffer) LPWSTR lpServiceName,
  _Inout_ LPDWORD lpcchBuffer);

SC_LOCK WINAPI LockServiceDatabase(_In_ SC_HANDLE);
BOOL WINAPI NotifyBootConfigStatus(_In_ BOOL);
_Must_inspect_result_ SC_HANDLE WINAPI OpenSCManagerA(_In_opt_ LPCSTR, _In_opt_ LPCSTR, _In_ DWORD);
_Must_inspect_result_ SC_HANDLE WINAPI OpenSCManagerW(_In_opt_ LPCWSTR, _In_opt_ LPCWSTR, _In_ DWORD);
_Must_inspect_result_ SC_HANDLE WINAPI OpenServiceA(_In_ SC_HANDLE, _In_ LPCSTR, _In_ DWORD);
_Must_inspect_result_ SC_HANDLE WINAPI OpenServiceW(_In_ SC_HANDLE, _In_ LPCWSTR, _In_ DWORD);

#if (NTDDI_VERSION >= NTDDI_VISTA)
DWORD WINAPI NotifyServiceStatusChangeA(_In_ SC_HANDLE, _In_ DWORD, _In_ PSERVICE_NOTIFYA);
DWORD WINAPI NotifyServiceStatusChangeW(_In_ SC_HANDLE, _In_ DWORD, _In_ PSERVICE_NOTIFYW);
#endif

_Must_inspect_result_
BOOL
WINAPI
QueryServiceConfigA(
  _In_ SC_HANDLE hService,
  _Out_writes_bytes_opt_(cbBufSize) LPQUERY_SERVICE_CONFIGA lpServiceConfig,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded);

_Must_inspect_result_
BOOL
WINAPI
QueryServiceConfigW(
  _In_ SC_HANDLE hService,
  _Out_writes_bytes_opt_(cbBufSize) LPQUERY_SERVICE_CONFIGW lpServiceConfig,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded);

_When_(dwInfoLevel == SERVICE_CONFIG_DESCRIPTION, _At_(cbBufSize, _In_range_(>=, sizeof(LPSERVICE_DESCRIPTIONA))))
_When_(dwInfoLevel == SERVICE_CONFIG_FAILURE_ACTIONS, _At_(cbBufSize, _In_range_(>=, sizeof(LPSERVICE_FAILURE_ACTIONSA))))
_When_(dwInfoLevel == SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO, _At_(cbBufSize, _In_range_(>=, sizeof(LPSERVICE_REQUIRED_PRIVILEGES_INFOA))))
_Must_inspect_result_
BOOL
WINAPI
QueryServiceConfig2A(
  _In_ SC_HANDLE hService,
  _In_ DWORD dwInfoLevel,
  _Out_writes_bytes_opt_(cbBufSize) LPBYTE lpBuffer,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded);

_When_(dwInfoLevel == SERVICE_CONFIG_DESCRIPTION, _At_(cbBufSize, _In_range_(>=, sizeof(LPSERVICE_DESCRIPTIONW))))
_When_(dwInfoLevel == SERVICE_CONFIG_FAILURE_ACTIONS, _At_(cbBufSize, _In_range_(>=, sizeof(LPSERVICE_FAILURE_ACTIONSW))))
_When_(dwInfoLevel == SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO, _At_(cbBufSize, _In_range_(>=, sizeof(LPSERVICE_REQUIRED_PRIVILEGES_INFOW))))
_Must_inspect_result_
BOOL
WINAPI
QueryServiceConfig2W(
  _In_ SC_HANDLE hService,
  _In_ DWORD dwInfoLevel,
  _Out_writes_bytes_opt_(cbBufSize) LPBYTE lpBuffer,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded);

_Must_inspect_result_
BOOL
WINAPI
QueryServiceLockStatusA(
  _In_ SC_HANDLE hSCManager,
  _Out_writes_bytes_opt_(cbBufSize) LPQUERY_SERVICE_LOCK_STATUSA lpLockStatus,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded);

_Must_inspect_result_
BOOL
WINAPI
QueryServiceLockStatusW(
  _In_ SC_HANDLE hSCManager,
  _Out_writes_bytes_opt_(cbBufSize) LPQUERY_SERVICE_LOCK_STATUSW lpLockStatus,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded);

_Must_inspect_result_
BOOL
WINAPI
QueryServiceObjectSecurity(
  _In_ SC_HANDLE hService,
  _In_ SECURITY_INFORMATION dwSecurityInformation,
  _Out_writes_bytes_opt_(cbBufSize) PSECURITY_DESCRIPTOR lpSecurityDescriptor,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded);

_Must_inspect_result_ BOOL WINAPI QueryServiceStatus(_In_ SC_HANDLE, _Out_ LPSERVICE_STATUS);

_Must_inspect_result_
BOOL
WINAPI
QueryServiceStatusEx(
  _In_ SC_HANDLE hService,
  _In_ SC_STATUS_TYPE InfoLevel,
  _Out_writes_bytes_opt_(cbBufSize) LPBYTE lpBuffer,
  _In_ DWORD cbBufSize,
  _Out_ LPDWORD pcbBytesNeeded);

_Must_inspect_result_ SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerA(_In_ LPCSTR, _In_ LPHANDLER_FUNCTION);
_Must_inspect_result_ SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerW(_In_ LPCWSTR, _In_ LPHANDLER_FUNCTION);
_Must_inspect_result_ SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerExA(_In_ LPCSTR, _In_ LPHANDLER_FUNCTION_EX, _In_opt_ LPVOID);
_Must_inspect_result_ SERVICE_STATUS_HANDLE WINAPI RegisterServiceCtrlHandlerExW(_In_ LPCWSTR, _In_ LPHANDLER_FUNCTION_EX, _In_opt_ LPVOID);
BOOL WINAPI SetServiceObjectSecurity(_In_ SC_HANDLE, _In_ SECURITY_INFORMATION, _In_ PSECURITY_DESCRIPTOR);
BOOL WINAPI SetServiceStatus(_In_ SERVICE_STATUS_HANDLE, _In_ LPSERVICE_STATUS);

BOOL
WINAPI
StartServiceA(
  _In_ SC_HANDLE hService,
  _In_ DWORD dwNumServiceArgs,
  _In_reads_opt_(dwNumServiceArgs) LPCSTR *lpServiceArgVectors);

BOOL
WINAPI
StartServiceW(
  _In_ SC_HANDLE hService,
  _In_ DWORD dwNumServiceArgs,
  _In_reads_opt_(dwNumServiceArgs) LPCWSTR *lpServiceArgVectors);

BOOL WINAPI StartServiceCtrlDispatcherA(_In_ const SERVICE_TABLE_ENTRYA*);
BOOL WINAPI StartServiceCtrlDispatcherW(_In_ const SERVICE_TABLE_ENTRYW*);
BOOL WINAPI UnlockServiceDatabase(_In_ SC_LOCK);

#ifdef UNICODE
typedef ENUM_SERVICE_STATUSW ENUM_SERVICE_STATUS,*LPENUM_SERVICE_STATUS;
typedef ENUM_SERVICE_STATUS_PROCESSW ENUM_SERVICE_STATUS_PROCESS;
typedef LPENUM_SERVICE_STATUS_PROCESSW LPENUM_SERVICE_STATUS_PROCESS;
typedef QUERY_SERVICE_CONFIGW QUERY_SERVICE_CONFIG,*LPQUERY_SERVICE_CONFIG;
typedef QUERY_SERVICE_LOCK_STATUSW QUERY_SERVICE_LOCK_STATUS,*LPQUERY_SERVICE_LOCK_STATUS;
typedef SERVICE_TABLE_ENTRYW SERVICE_TABLE_ENTRY,*LPSERVICE_TABLE_ENTRY;
typedef LPSERVICE_MAIN_FUNCTIONW LPSERVICE_MAIN_FUNCTION;
typedef SERVICE_DESCRIPTIONW SERVICE_DESCRIPTION;
typedef LPSERVICE_DESCRIPTIONW LPSERVICE_DESCRIPTION;
typedef SERVICE_FAILURE_ACTIONSW SERVICE_FAILURE_ACTIONS;
typedef LPSERVICE_FAILURE_ACTIONSW LPSERVICE_FAILURE_ACTIONS;
typedef SERVICE_REQUIRED_PRIVILEGES_INFOW SERVICE_REQUIRED_PRIVILEGES_INFO;
typedef LPSERVICE_REQUIRED_PRIVILEGES_INFOW LPSERVICE_REQUIRED_PRIVILEGES_INFO;
typedef SERVICE_NOTIFY_2W SERVICE_NOTIFY_2;
typedef PSERVICE_NOTIFY_2W PSERVICE_NOTIFY_2;
typedef SERVICE_NOTIFYW SERVICE_NOTIFY;
typedef PSERVICE_NOTIFYW PSERVICE_NOTIFY;
#define SERVICES_ACTIVE_DATABASE SERVICES_ACTIVE_DATABASEW
#define SERVICES_FAILED_DATABASE SERVICES_FAILED_DATABASEW
#define SC_GROUP_IDENTIFIER SC_GROUP_IDENTIFIERW
#define ChangeServiceConfig ChangeServiceConfigW
#define ChangeServiceConfig2 ChangeServiceConfig2W
#define CreateService CreateServiceW
#define EnumDependentServices EnumDependentServicesW
#define EnumServicesStatus EnumServicesStatusW
#define EnumServicesStatusEx  EnumServicesStatusExW
#define GetServiceDisplayName GetServiceDisplayNameW
#define GetServiceKeyName GetServiceKeyNameW
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define NotifyServiceStatusChange NotifyServiceStatusChangeW
#endif
#define OpenSCManager OpenSCManagerW
#define OpenService OpenServiceW
#define QueryServiceConfig QueryServiceConfigW
#define QueryServiceConfig2 QueryServiceConfig2W
#define QueryServiceLockStatus QueryServiceLockStatusW
#define RegisterServiceCtrlHandler RegisterServiceCtrlHandlerW
#define RegisterServiceCtrlHandlerEx RegisterServiceCtrlHandlerExW
#define StartService StartServiceW
#define StartServiceCtrlDispatcher StartServiceCtrlDispatcherW
#else
typedef ENUM_SERVICE_STATUSA ENUM_SERVICE_STATUS,*LPENUM_SERVICE_STATUS;
typedef ENUM_SERVICE_STATUS_PROCESSA ENUM_SERVICE_STATUS_PROCESS;
typedef LPENUM_SERVICE_STATUS_PROCESSA LPENUM_SERVICE_STATUS_PROCESS;
typedef QUERY_SERVICE_CONFIGA QUERY_SERVICE_CONFIG,*LPQUERY_SERVICE_CONFIG;
typedef QUERY_SERVICE_LOCK_STATUSA QUERY_SERVICE_LOCK_STATUS,*LPQUERY_SERVICE_LOCK_STATUS;
typedef SERVICE_TABLE_ENTRYA SERVICE_TABLE_ENTRY,*LPSERVICE_TABLE_ENTRY;
typedef LPSERVICE_MAIN_FUNCTIONA LPSERVICE_MAIN_FUNCTION;
typedef SERVICE_DESCRIPTIONA SERVICE_DESCRIPTION;
typedef LPSERVICE_DESCRIPTIONA LPSERVICE_DESCRIPTION;
typedef SERVICE_FAILURE_ACTIONSA SERVICE_FAILURE_ACTIONS;
typedef LPSERVICE_FAILURE_ACTIONSA LPSERVICE_FAILURE_ACTIONS;
typedef SERVICE_REQUIRED_PRIVILEGES_INFOA SERVICE_REQUIRED_PRIVILEGES_INFO;
typedef LPSERVICE_REQUIRED_PRIVILEGES_INFOA LPSERVICE_REQUIRED_PRIVILEGES_INFO;
typedef SERVICE_NOTIFY_2A SERVICE_NOTIFY_2;
typedef PSERVICE_NOTIFY_2A PSERVICE_NOTIFY_2;
typedef SERVICE_NOTIFYA SERVICE_NOTIFY;
typedef PSERVICE_NOTIFYA PSERVICE_NOTIFY;
#define SERVICES_ACTIVE_DATABASE SERVICES_ACTIVE_DATABASEA
#define SERVICES_FAILED_DATABASE SERVICES_FAILED_DATABASEA
#define SC_GROUP_IDENTIFIER SC_GROUP_IDENTIFIERA
#define ChangeServiceConfig ChangeServiceConfigA
#define ChangeServiceConfig2 ChangeServiceConfig2A
#define CreateService CreateServiceA
#define EnumDependentServices EnumDependentServicesA
#define EnumServicesStatus EnumServicesStatusA
#define EnumServicesStatusEx  EnumServicesStatusExA
#define GetServiceDisplayName GetServiceDisplayNameA
#define GetServiceKeyName GetServiceKeyNameA
#define OpenSCManager OpenSCManagerA
#define OpenService OpenServiceA
#if (NTDDI_VERSION >= NTDDI_VISTA)
#define NotifyServiceStatusChange NotifyServiceStatusChangeA
#endif
#define QueryServiceConfig QueryServiceConfigA
#define QueryServiceConfig2 QueryServiceConfig2A
#define QueryServiceLockStatus QueryServiceLockStatusA
#define RegisterServiceCtrlHandler RegisterServiceCtrlHandlerA
#define RegisterServiceCtrlHandlerEx RegisterServiceCtrlHandlerExA
#define StartService StartServiceA
#define StartServiceCtrlDispatcher StartServiceCtrlDispatcherA
#endif
#ifdef __cplusplus
}
#endif
#endif /* _WINSVC_H */
