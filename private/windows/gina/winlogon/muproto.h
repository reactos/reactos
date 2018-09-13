#ifndef _MUPROTOH_
#define _MUPROTOH_

BOOL InitializeMultiUserFunctionsPtrs (void);


//
// GetProcAddr Prototypes for winsta.dll
//

//_WinStationCallback
typedef BOOLEAN   (*PWINSTATION_CALLBACK) (
                    HANDLE hServer,
                    ULONG SessionId,
                    LPWSTR pPhoneNumber
                    );
PWINSTATION_CALLBACK gpfnWinStationCallback;

//WinStationDisconnect

typedef BOOLEAN (*PWINSTATION_DISCONNECT) (
                    HANDLE hServer,
                    ULONG SessionId,
                    BOOLEAN bWait
                    );
PWINSTATION_DISCONNECT gpfnWinStationDisconnect;

//WinStationConnectW

typedef BOOLEAN (*PWINSTATION_CONNECT) (
                    HANDLE hServer,
                    ULONG SessionId,
                    ULONG TargetSessionId,
                    PWCHAR pPassword,
                    BOOLEAN bWait
                    );
PWINSTATION_CONNECT gpfnWinStationConnect;

//WinStationQueryInformationW

typedef BOOLEAN (*PWINSTATION_QUERY_INFORMATION) (
                    HANDLE hServer,
                    ULONG SessionId,
                    WINSTATIONINFOCLASS WinStationInformationClass,
                    PVOID  pWinStationInformation,
                    ULONG WinStationInformationLength,
                    PULONG  pReturnLength
                    );
PWINSTATION_QUERY_INFORMATION gpfnWinStationQueryInformation;

//ServerQueryInetConnectorInformationW

typedef BOOLEAN (*PSERVER_QUERY_IC_INFORMATION) (
                    HANDLE hServer,
                    PVOID  pWinStationInformation,
                    ULONG WinStationInformationLength,
                    PULONG  pReturnLength
                    );
PSERVER_QUERY_IC_INFORMATION gpfnServerQueryInetConnectorInformation;

//WinStationEnumerate_IndexedW

typedef BOOLEAN (*PWINSTATION_ENUMERATE_INDEXED) (
                    HANDLE  hServer,
                    PULONG  pEntries,
                    PLOGONIDW pLogonId,
                    PULONG  pByteCount,
                    PULONG  pIndex
                    );
PWINSTATION_ENUMERATE_INDEXED gpfnWinStationEnumerate_Indexed;

//WinStationNameFromLogonIdW

typedef BOOLEAN (*PWINSTATION_NAME_FROM_SESSIONID) (
                    HANDLE hServer,
                    ULONG SessionId,
                    PWINSTATIONNAMEW pWinStationName
                    );
PWINSTATION_NAME_FROM_SESSIONID gpfnWinStationNameFromSessionId;

//WinStationShutdownSystem

typedef BOOLEAN (*PWINSTATION_SHUTDOWN_SYSTEM) (
                    HANDLE hServer,
                    ULONG ShutdownFlags
                    );
PWINSTATION_SHUTDOWN_SYSTEM gpfnWinStationShutdownSystem;

//_WinStationWaitForConnect

typedef BOOLEAN (*PWINSTATION_WAIT_FOR_CONNECT) (
                    VOID
                    );
PWINSTATION_WAIT_FOR_CONNECT gpfnWinStationWaitForConnect;

//WinStationSetInformationW

typedef BOOLEAN (*PWINSTATION_SET_INFORMATION) (
                    HANDLE hServer,
                    ULONG SessionId,
                    WINSTATIONINFOCLASS WinStationInformationClass,
                    PVOID pWinStationInformation,
                    ULONG WinStationInformationLength
                    );
PWINSTATION_SET_INFORMATION gpfnWinStationSetInformation;

//_WinStationNotifyLogon

typedef BOOLEAN (*PWINSTATION_NOTIFY_LOGON) (
                    BOOLEAN fUserIsAdmin,
                    HANDLE UserToken,
                    PWCHAR pDomain,
                    PWCHAR pUserName,
                    PWCHAR pPassword,
                    UCHAR Seed,
                    PUSERCONFIGW pUserConfig
                    );
PWINSTATION_NOTIFY_LOGON gpfnWinStationNotifyLogon;

//_WinStationInitNewSession

typedef BOOLEAN (*PWINSTATION_NOTIFY_NEW_SESSION) (
                    HANDLE hServer,
                    ULONG SessionId
                    );
PWINSTATION_NOTIFY_NEW_SESSION gpfnWinStationNotifyNewSession;

typedef LONG ( * PREGUSERCONFIGQUERY) ( WCHAR *,
                                        WCHAR *,
                                        PUSERCONFIGW,
                                        ULONG,
                                        PULONG );

PREGUSERCONFIGQUERY gpfnRegUserConfigQuery;

typedef LONG ( * PREGDEFAULTUSERCONFIGQUERY) ( WCHAR *,
                                               PUSERCONFIGW,
                                               ULONG,
                                               PULONG );

PREGDEFAULTUSERCONFIGQUERY gpfnRegDefaultUserConfigQuery;


#endif


