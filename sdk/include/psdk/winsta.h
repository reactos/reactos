//
// Created by pierc on 25/08/2025.
//

#ifndef REACTOS_WINSTA_H
#define REACTOS_WINSTA_H

/* WinSta calling convention */
#define WINSTAAPI WINAPI
typedef unsigned char BOOLEAN, *PBOOLEAN;

#ifdef __cplusplus
extern "C"
{
#endif

// from wine
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
        UNICODE_STRING ImageName;
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

    BOOLEAN WINSTAAPI
    WinStationEnumerateW(
        _In_opt_ HANDLE hServer,
        _Out_ PSESSIONIDW *SessionIds,
        _Out_ PULONG Count);

    BOOLEAN WINSTAAPI
    WinStationGetAllProcesses(
        _In_opt_ HANDLE hServer,
        _In_ ULONG Level,
        _Out_ PULONG NumberOfProcesses,
        _Out_ PTS_ALL_PROCESSES_INFO *Processes);

    BOOLEAN WINSTAAPI
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
    BOOLEAN WINSTAAPI
    WinStationQueryInformationW(
        _In_opt_ HANDLE hServer,
        _In_ ULONG SessionId,
        _In_ WINSTATIONINFOCLASS WinStationInformationClass,
        _Out_writes_bytes_(WinStationInformationLength) PVOID WinStationInformation,
        _In_ ULONG WinStationInformationLength,
        _Out_ PULONG ReturnLength);

    BOOLEAN
    NTAPI
    WinStationEnumerateProcesses(
        _In_opt_ HANDLE ServerHandle,
        _Out_ PVOID *Processes);

    // end from wine

    /* Added prototypes for functions used outside winsta.dll */
    BOOLEAN WINSTAAPI
    WinStationUnRegisterConsoleNotification(
        _In_opt_ HANDLE hServer,
        _In_ HWND WindowHandle);

    BOOLEAN WINSTAAPI
    WinStationRegisterConsoleNotification(
        _In_opt_ HANDLE hServer,
        _In_ HWND WindowHandle,
        _In_ ULONG Flags);

    BOOLEAN
    NTAPI
    WinStationGetConnectionProperty(
        _In_ ULONG SessionId,
        _In_ LPCGUID PropertyType,
        _Out_ PTS_PROPERTY_INFORMATION PropertyBuffer);

#ifdef __cplusplus
}
#endif

#endif // REACTOS_WINSTA_H