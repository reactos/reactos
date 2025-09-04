/*
 * PROJECT:     Terminal Services (winsta) Private API
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     WinStation private API definitions
 * COPYRIGHT:   Copyright 2025 Pierce Andjelkovic <pierceandjelkovic@gmail.com>
 */

#ifndef REACTOS_WINSTA_H
#define REACTOS_WINSTA_H

// refs: https://github.com/mirror/processhacker/blob/master/2.x/trunk/phlib/include/winsta.h
//       https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-tsts/ce70794f-2138-43e8-bf6c-2c147887d6a2

/* WinSta calling convention */
#define WINSTAAPI WINAPI
//#include <ntdef.h>

#include <stdarg.h>

#include <windef.h>
#include <winbase.h>

//#include <winnt.h>
//#include <ntdef.h>
typedef unsigned char BOOLEAN, *PBOOLEAN;

#ifdef __cplusplus
extern "C"
{
#endif

#define WINSTATIONNAME_LENGTH 32
typedef WCHAR WINSTATIONNAME[WINSTATIONNAME_LENGTH + 1];

typedef enum _WINSTATIONSTATECLASS
{
    State_Active,
    State_Connected,
    State_ConnectQuery,
    State_Shadow,
    State_Disconnected,
    State_Idle,
    State_Listen,
    State_Reset,
    State_Down,
    State_Init
} WINSTATIONSTATECLASS;

typedef struct _SESSIONIDW
{
    union
    {
        ULONG SessionId;
        ULONG LogonId;
    } DUMMYUNIONNAME;
    WINSTATIONNAME WinStationName;
    WINSTATIONSTATECLASS State;
} SESSIONIDW, *PSESSIONIDW;

typedef struct _TS_SYS_PROCESS_INFORMATION
{
    ULONG NextEntryOffset;
    ULONG NumberOfThreads;
    LARGE_INTEGER SpareLi1;
    LARGE_INTEGER SpareLi2;
    LARGE_INTEGER SpareLi3;
    LARGE_INTEGER CreateTime;
    LARGE_INTEGER UserTime;
    LARGE_INTEGER KernelTime;
    VOID* ImageName;
    //UNICODE_STRING ImageName;
    LONG BasePriority;
    ULONG UniqueProcessId;
    ULONG InheritedFromUniqueProcessId;
    ULONG HandleCount;
    ULONG SessionId;
    ULONG SpareUl3;
    SIZE_T PeakVirtualSize;
    SIZE_T VirtualSize;
    ULONG PageFaultCount;
    ULONG PeakWorkingSetSize;
    ULONG WorkingSetSize;
    SIZE_T QuotaPeakPagedPoolUsage;
    SIZE_T QuotaPagedPoolUsage;
    SIZE_T QuotaPeakNonPagedPoolUsage;
    SIZE_T QuotaNonPagedPoolUsage;
    SIZE_T PagefileUsage;
    SIZE_T PeakPagefileUsage;
    SIZE_T PrivatePageCount;
} TS_SYS_PROCESS_INFORMATION, *PTS_SYS_PROCESS_INFORMATION;

typedef struct _TS_ALL_PROCESSES_INFO
{
    PTS_SYS_PROCESS_INFORMATION pTsProcessInfo;
    DWORD SizeOfSid;
    PBYTE pSid;
} TS_ALL_PROCESSES_INFO, *PTS_ALL_PROCESSES_INFO;

typedef struct _TS_PROPERTY_INFORMATION
{
    ULONG Length;
    PVOID Buffer;
} TS_PROPERTY_INFORMATION, *PTS_PROPERTY_INFORMATION;

typedef struct _TS_COUNTER_HEADER
{
    ULONG CounterID;
    BOOLEAN Result;
} TS_COUNTER_HEADER, *PTS_COUNTER_HEADER;

typedef struct _TS_COUNTER
{
    TS_COUNTER_HEADER CounterHead;
    ULONG Value;
    LARGE_INTEGER StartTime;
} TS_COUNTER, *PTS_COUNTER;


    typedef enum _WINSTATIONINFOCLASS
    {
        WinStationCreateData,
        WinStationConfiguration,
        WinStationPdParams,
        WinStationWd,
        WinStationPd,
        WinStationPrinter,
        WinStationClient,
        WinStationModules,
        WinStationInformation,
        WinStationTrace,
        WinStationBeep,
        WinStationEncryptionOff,
        WinStationEncryptionPerm,
        WinStationNtSecurity,
        WinStationUserToken,
        WinStationUnused1,
        WinStationVideoData,
        WinStationInitialProgram,
        WinStationCd,
        WinStationSystemTrace,
        WinStationVirtualData,
        WinStationClientData,
        WinStationSecureDesktopEnter,
        WinStationSecureDesktopExit,
        WinStationLoadBalanceSessionTarget,
        WinStationLoadIndicator,
        WinStationShadowInfo,
        WinStationDigProductId,
        WinStationLockedState,
        WinStationRemoteAddress,
        WinStationIdleTime,
        WinStationLastReconnectType,
        WinStationDisallowAutoReconnect,
        WinStationUnused2,
        WinStationUnused3,
        WinStationUnused4,
        WinStationUnused5,
        WinStationReconnectedFromId,
        WinStationEffectsPolicy,
        WinStationType,
        WinStationInformationEx,
        WinStationValidationInfo
    } WINSTATIONINFOCLASS;

BOOLEAN
WINSTAAPI
WinStationReset(_In_opt_ HANDLE ServerHandle, _In_ ULONG SessionId, _In_ BOOLEAN bWait);

BOOLEAN
WINSTAAPI
WinStationDisconnect(_In_opt_ HANDLE ServerHandle, _In_ ULONG SessionId, _In_ BOOLEAN bWait);

VOID
WINSTAAPI
WinStationSendMessageA(PVOID A, PVOID B, PVOID C, PVOID D, PVOID E, PVOID F, PVOID G, PVOID H, PVOID I, PVOID J);

BOOLEAN
WINSTAAPI
WinStationSendMessageW(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG SessionId,
    _In_ PCWSTR Title,
    _In_ ULONG TitleLength,
    _In_ PCWSTR Message,
    _In_ ULONG MessageLength,
    _In_ ULONG Style,
    _In_ ULONG Timeout,
    _Out_ PULONG Response,
    _In_ BOOLEAN DoNotWait);

VOID
WINSTAAPI
WinStationEnumerateA(PVOID A, PVOID B, PVOID C);

BOOLEAN
WINSTAAPI
WinStationFreeMemory(_In_ PVOID Buffer);

BOOLEAN
WINSTAAPI
WinStationCloseServer(_In_ HANDLE ServerHandle);

HANDLE
WINSTAAPI
WinStationOpenServerW(_In_opt_ PCWSTR ServerName);

BOOLEAN
WINSTAAPI
WinStationShutdownSystem(
    _In_opt_ HANDLE ServerHandle,
    _In_ ULONG ShutdownFlags); // WSD_*

HANDLE
WINSTAAPI
WinStationVirtualOpen(_In_opt_ HANDLE ServerHandle, _In_ ULONG SessionId, _In_ PCSTR Name);

BOOLEAN
WINSTAAPI
WinStationTerminateProcess(_In_opt_ HANDLE ServerHandle, _In_ ULONG ProcessId, _In_ ULONG ExitCode);

VOID
WINSTAAPI
WinStationOpenServerA(PVOID A);

BOOLEAN
WINSTAAPI
WinStationFreeGAPMemory(_In_ ULONG Level, _In_ PTS_ALL_PROCESSES_INFO Processes, _In_ ULONG NumberOfProcesses);

BOOLEAN
WINSTAAPI
WinStationEnumerateW(_In_opt_ HANDLE hServer, _Out_ PSESSIONIDW *SessionIds, _Out_ PULONG Count);

BOOLEAN
WINSTAAPI
WinStationGetAllProcesses(
    _In_opt_ HANDLE hServer,
    _In_ ULONG Level,
    _Out_ PULONG NumberOfProcesses,
    _Out_ PTS_ALL_PROCESSES_INFO *Processes);

BOOLEAN
WINSTAAPI
WinStationGetProcessSid(
    _In_opt_ HANDLE hServer,
    _In_ ULONG ProcessId,
    _In_ FILETIME ProcessStartTime,
    _Out_ PVOID pProcessUserSid,
    _Inout_ PULONG dwSidSize);

/**
 * The WinStationQueryInformationW routine retrieves information about a window station.
 *
 * @param hServer A handle to an RD Session Host server. Specify a handle opened by the WinStationOpenServerW
 * function, or specify WINSTATION_CURRENT_SERVER to indicate the server on which your application is running.
 * @param SessionId A Remote Desktop Services session identifier.
 * To indicate the session in which the calling application is running (or the current session) specify
 * WINSTATION_CURRENT_SESSION. Only specify WINSTATION_CURRENT_SESSION when obtaining session information on the
 * local server. If WINSTATION_CURRENT_SESSION is specified when querying session information on a remote server,
 * the returned session information will be inconsistent. Do not use the returned data.
 * @param WinStationInformationClass A value from the TOKEN_INFORMATION_CLASS enumerated type identifying the type
 * of information to be retrieved.
 * @param WinStationInformation Pointer to a caller-allocated buffer that receives the requested information about
 * the token.
 * @param WinStationInformationLength Length, in bytes, of the caller-allocated TokenInformation buffer.
 * @param ReturnLength Pointer to a caller-allocated variable that receives the actual length, in bytes, of the
 * information returned in the TokenInformation buffer.
 * @return NTSTATUS Successful or errant status.
 * @sa https://learn.microsoft.com/en-us/windows-hardware/drivers/ddi/ntifs/nf-ntifs-ntqueryinformationtoken
 */
BOOLEAN
WINSTAAPI
WinStationQueryInformationW(
    _In_opt_ HANDLE hServer,
    _In_ ULONG SessionId,
    _In_ WINSTATIONINFOCLASS WinStationInformationClass,
    _Out_writes_bytes_(WinStationInformationLength) PVOID WinStationInformation,
    _In_ ULONG WinStationInformationLength,
    _Out_ PULONG ReturnLength);

BOOLEAN
WINSTAAPI
WinStationEnumerateProcesses(_In_opt_ HANDLE ServerHandle, _Out_ PVOID *Processes);

BOOLEAN
WINSTAAPI
WinStationUnRegisterConsoleNotification(_In_opt_ HANDLE hServer, _In_ HWND WindowHandle);

BOOLEAN
WINSTAAPI
WinStationRegisterConsoleNotification(_In_opt_ HANDLE hServer, _In_ HWND WindowHandle, _In_ ULONG Flags);

BOOLEAN
WINSTAAPI
WinStationGetConnectionProperty(
    _In_ ULONG SessionId,
    _In_ LPCGUID PropertyType,
    _Out_ PTS_PROPERTY_INFORMATION PropertyBuffer);

BOOLEAN
WINSTAAPI
WinStationWaitSystemEvent(
    _In_opt_ HANDLE hServer,
    _In_ ULONG Mask, // WEVENT_*
    _Out_ PULONG Flags);

#ifdef __cplusplus
}
#endif

#endif // REACTOS_WINSTA_H