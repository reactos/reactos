/* $Id: stubs.c,v 1.74 2004/05/02 14:47:05 weiden Exp $
 *
 * KERNEL32.DLL stubs (unimplemented functions)
 * Remove from this file, if you implement them.
 */

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"


//#define _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
BOOL
STDCALL
BaseAttachCompleteThunk (VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
CmdBatNotification (
    DWORD   Unknown
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
CompareStringA (
    LCID    Locale,
    DWORD   dwCmpFlags,
    LPCSTR  lpString1,
    int cchCount1,
    LPCSTR  lpString2,
    int cchCount2
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
CompareStringW (
    LCID    Locale,
    DWORD   dwCmpFlags,
    LPCWSTR lpString1,
    int cchCount1,
    LPCWSTR lpString2,
    int cchCount2
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
CreateVirtualBuffer (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
ExitVDM (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
ExtendVirtualBuffer (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
int
STDCALL
FoldStringW (
    DWORD   dwMapFlags,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWSTR  lpDestStr,
    int cchDest
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
FoldStringA (
    DWORD   dwMapFlags,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPSTR   lpDestStr,
    int cchDest
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
FreeVirtualBuffer (
    HANDLE  hVirtualBuffer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#ifndef _OLE2NLS_IN_BUILD_





/*
 * @unimplemented
 */
DWORD
STDCALL
GetNextVDMCommand (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetStringTypeExW (
    LCID    Locale,
    DWORD   dwInfoType,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetStringTypeExA (
    LCID    Locale,
    DWORD   dwInfoType,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetStringTypeW (
    DWORD   dwInfoType,
    LPCWSTR lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
GetStringTypeA (
    LCID    Locale,
    DWORD   dwInfoType,
    LPCSTR  lpSrcStr,
    int cchSrc,
    LPWORD  lpCharType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


#endif

/*
 * @unimplemented
 */
BOOL
STDCALL
GetSystemPowerStatus (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetVDMCurrentDirectories (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
BOOL
STDCALL
IsDBCSLeadByte (
    BYTE    TestChar
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
IsDBCSLeadByteEx (
    UINT    CodePage,
    BYTE    TestChar
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

#endif

/*
 * @unimplemented
 */
DWORD
STDCALL
LoadModule (
    LPCSTR  lpModuleName,
    LPVOID  lpParameterBlock
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
RegisterConsoleVDM (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2,
    DWORD   Unknown3,
    DWORD   Unknown4,
    DWORD   Unknown5,
    DWORD   Unknown6,
    DWORD   Unknown7,
    DWORD   Unknown8,
    DWORD   Unknown9,
    DWORD   Unknown10
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
RegisterWowBaseHandlers (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
RegisterWowExec (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetSystemPowerState (
    BOOL fSuspend,
    BOOL fForce
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
SetVDMCurrentDirectories (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
TrimVirtualBuffer (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
VDMConsoleOperation (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
VDMOperationStarted (
    DWORD   Unknown0
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


#ifndef _OLE2NLS_IN_BUILD_

/*
 * @unimplemented
 */
DWORD
STDCALL
VerLanguageNameA (
    DWORD   wLang,
    LPSTR   szLang,
    DWORD   nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
VerLanguageNameW (
    DWORD   wLang,
    LPWSTR  szLang,
    DWORD   nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

#endif

/*
 * @unimplemented
 */
DWORD
STDCALL
VirtualBufferExceptionHandler (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ActivateActCtx(
    HANDLE hActCtx,
    ULONG_PTR *lpCookie
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
AddRefActCtx(
    HANDLE hActCtx
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AllocateUserPhysicalPages(
    HANDLE hProcess,
    PULONG_PTR NumberOfPages,
    PULONG_PTR PageArray
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
AssignProcessToJobObject(
    HANDLE hJob,
    HANDLE hProcess
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
BindIoCompletionCallback (
    HANDLE FileHandle,
    LPOVERLAPPED_COMPLETION_ROUTINE Function,
    ULONG Flags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CancelDeviceWakeupRequest(
    HANDLE hDevice
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CancelTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
/*
 * @unimplemented
 */

BOOL
STDCALL
ChangeTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    ULONG DueTime,
    ULONG Period
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateActCtxA(
    PCACTCTXA pActCtx
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateActCtxW(
    PCACTCTXW pActCtx
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CreateJobSet (
    ULONG NumJob,
    PJOB_SET_ARRAY UserJobSet,
    ULONG Flags)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateMemoryResourceNotification(
    MEMORY_RESOURCE_NOTIFICATION_TYPE NotificationType
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateTimerQueue(
    VOID
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CreateTimerQueueTimer(
    PHANDLE phNewTimer,
    HANDLE TimerQueue,
    WAITORTIMERCALLBACK Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    ULONG Flags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeactivateActCtx(
    DWORD dwFlags,
    ULONG_PTR ulCookie
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteTimerQueue(
    HANDLE TimerQueue
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteTimerQueueEx(
    HANDLE TimerQueue,
    HANDLE CompletionEvent
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteTimerQueueTimer(
    HANDLE TimerQueue,
    HANDLE Timer,
    HANDLE CompletionEvent
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindActCtxSectionGuid(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    const GUID *lpGuidToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindVolumeClose(
    HANDLE hFindVolume
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindVolumeMountPointClose(
    HANDLE hFindVolumeMountPoint
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FreeUserPhysicalPages(
    HANDLE hProcess,
    PULONG_PTR NumberOfPages,
    PULONG_PTR PageArray
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetCurrentActCtx(
    HANDLE *lphActCtx)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetDevicePowerState(
    HANDLE hDevice,
    BOOL *pfOn
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
GetNativeSystemInfo(
    LPSYSTEM_INFO lpSystemInfo
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetNumaHighestNodeNumber(
    PULONG HighestNodeNumber
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetNumaNodeProcessorMask(
    UCHAR Node,
    PULONGLONG ProcessorMask
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetNumaProcessorNode(
    UCHAR Processor,
    PUCHAR NodeNumber
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetProcessHandleCount(
    HANDLE hProcess,
    PDWORD pdwHandleCount
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetProcessPriorityBoost(
    HANDLE hProcess,
    PBOOL pDisablePriorityBoost
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetSystemRegistryQuota(
    PDWORD pdwQuotaAllowed,
    PDWORD pdwQuotaUsed
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetSystemTimes(
    LPFILETIME lpIdleTime,
    LPFILETIME lpKernelTime,
    LPFILETIME lpUserTime
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetThreadIOPendingFlag(
    HANDLE hThread,
    PBOOL lpIOIsPending
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetWriteWatch(
    DWORD  dwFlags,
    PVOID  lpBaseAddress,
    SIZE_T dwRegionSize,
    PVOID *lpAddresses,
    PULONG_PTR lpdwCount,
    PULONG lpdwGranularity
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
HeapQueryInformation (
    HANDLE HeapHandle, 
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL,
    PSIZE_T ReturnLength OPTIONAL
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
HeapSetInformation (
    HANDLE HeapHandle, 
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IsProcessInJob (
    HANDLE ProcessHandle,
    HANDLE JobHandle,
    PBOOL Result
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IsSystemResumeAutomatic(
    VOID
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IsWow64Process(
    HANDLE hProcess,
    PBOOL Wow64Process
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
MapUserPhysicalPages(
    PVOID VirtualAddress,
    ULONG_PTR NumberOfPages,
    PULONG_PTR PageArray OPTIONAL
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
MapUserPhysicalPagesScatter(
    PVOID *VirtualAddresses,
    ULONG_PTR NumberOfPages,
    PULONG_PTR PageArray OPTIONAL
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
OpenThread(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwThreadId
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
QueryActCtxW(
    DWORD dwFlags,
    HANDLE hActCtx,
    PVOID pvSubInstance,
    ULONG ulInfoClass,
    PVOID pvBuffer,
    SIZE_T cbBuffer OPTIONAL,
    SIZE_T *pcbWrittenOrRequired OPTIONAL
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
QueryInformationJobObject(
    HANDLE hJob,
    JOBOBJECTINFOCLASS JobObjectInformationClass,
    LPVOID lpJobObjectInformation,
    DWORD cbJobObjectInformationLength,
    LPDWORD lpReturnLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
QueryMemoryResourceNotification(
     HANDLE ResourceNotificationHandle,
    PBOOL  ResourceState
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
QueueUserAPC(
    PAPCFUNC pfnAPC,
    HANDLE hThread,
    ULONG_PTR dwData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
QueueUserWorkItem(
    LPTHREAD_START_ROUTINE Function,
    PVOID Context,
    ULONG Flags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ReadDirectoryChangesW(
    HANDLE hDirectory,
    LPVOID lpBuffer,
    DWORD nBufferLength,
    BOOL bWatchSubtree,
    DWORD dwNotifyFilter,
    LPDWORD lpBytesReturned,
    LPOVERLAPPED lpOverlapped,
    LPOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ReadFileScatter(
    HANDLE hFile,
    FILE_SEGMENT_ELEMENT aSegmentArray[],
    DWORD nNumberOfBytesToRead,
    LPDWORD lpReserved,
    LPOVERLAPPED lpOverlapped
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
RegisterWaitForSingleObject(
    PHANDLE phNewWaitObject,
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
RegisterWaitForSingleObjectEx(
    HANDLE hObject,
    WAITORTIMERCALLBACK Callback,
    PVOID Context,
    ULONG dwMilliseconds,
    ULONG dwFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
ReleaseActCtx(
    HANDLE hActCtx
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
ULONG
STDCALL
RemoveVectoredExceptionHandler(
    PVOID VectoredHandlerHandle
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
RequestDeviceWakeup(
    HANDLE hDevice
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
RequestWakeupLatency(
    LATENCY_TIME latency
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
ResetWriteWatch(
    LPVOID lpBaseAddress,
    SIZE_T dwRegionSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID
STDCALL
RestoreLastError(
    DWORD dwErrCode
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
DWORD
STDCALL
SetCriticalSectionSpinCount(
    LPCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetInformationJobObject(
    HANDLE hJob,
    JOBOBJECTINFOCLASS JobObjectInformationClass,
    LPVOID lpJobObjectInformation,
    DWORD cbJobObjectInformationLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetMessageWaitingIndicator(
    HANDLE hMsgIndicator,
    ULONG ulMsgCount
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetProcessPriorityBoost(
    HANDLE hProcess,
    BOOL bDisablePriorityBoost
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
EXECUTION_STATE
STDCALL
SetThreadExecutionState(
    EXECUTION_STATE esFlags
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
SetTimerQueueTimer(
    HANDLE TimerQueue,
    WAITORTIMERCALLBACK Callback,
    PVOID Parameter,
    DWORD DueTime,
    DWORD Period,
    BOOL PreferIo
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
TerminateJobObject(
    HANDLE hJob,
    UINT uExitCode
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
TzSpecificLocalTimeToSystemTime(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation,
    LPSYSTEMTIME lpLocalTime,
    LPSYSTEMTIME lpUniversalTime
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
UnregisterWait(
    HANDLE WaitHandle
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
UnregisterWaitEx(
    HANDLE WaitHandle,
    HANDLE CompletionEvent
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
WriteFileGather(
    HANDLE hFile,
    FILE_SEGMENT_ELEMENT aSegmentArray[],
    DWORD nNumberOfBytesToWrite,
    LPDWORD lpReserved,
    LPOVERLAPPED lpOverlapped
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
WTSGetActiveConsoleSessionId(VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ZombifyActCtx(
    HANDLE hActCtx
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CheckNameLegalDOS8Dot3W(
    LPCWSTR lpName,
    LPSTR lpOemName OPTIONAL,
    DWORD OemNameSize OPTIONAL,
    PBOOL pbNameContainsSpaces OPTIONAL,
    PBOOL pbNameLegal
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateJobObjectW(
    LPSECURITY_ATTRIBUTES lpJobAttributes,
    LPCWSTR lpName
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DnsHostnameToComputerNameW (
    LPCWSTR Hostname,
    LPWSTR ComputerName,
    LPDWORD nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindActCtxSectionStringW(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCWSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstVolumeW(
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstVolumeMountPointW(
    LPCWSTR lpszRootPathName,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindNextVolumeW(
    HANDLE hFindVolume,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindNextVolumeMountPointW(
    HANDLE hFindVolumeMountPoint,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetDllDirectoryW(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetFirmwareEnvironmentVariableW(
    LPCWSTR lpName,
    LPCWSTR lpGuid,
    PVOID   pBuffer,
    DWORD    nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetLongPathNameW(
    LPCWSTR lpszShortPath,
    LPWSTR  lpszLongPath,
    DWORD    cchBuffer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetModuleHandleExW(
    DWORD        dwFlags,
    LPCWSTR     lpModuleName,
    HMODULE*    phModule
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetSystemWow64DirectoryW(
    LPWSTR lpBuffer,
    UINT uSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetVolumeNameForVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint,
    LPWSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetVolumePathNameW(
    LPCWSTR lpszFileName,
    LPWSTR lpszVolumePathName,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetVolumePathNamesForVolumeNameW(
    LPCWSTR lpszVolumeName,
    LPWSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
OpenJobObjectW(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ReplaceFileW(
    LPCWSTR lpReplacedFileName,
    LPCWSTR lpReplacementFileName,
    LPCWSTR lpBackupFileName,
    DWORD   dwReplaceFlags,
    LPVOID  lpExclude,
    LPVOID  lpReserved
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetComputerNameExW (
    COMPUTER_NAME_FORMAT NameType,
    LPCWSTR lpBuffer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetDllDirectoryW(
    LPCWSTR lpPathName
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetFirmwareEnvironmentVariableW(
    LPCWSTR lpName,
    LPCWSTR lpGuid,
    PVOID    pValue,
    DWORD    nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint,
    LPCWSTR lpszVolumeName
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
VerifyVersionInfoW(
    LPOSVERSIONINFOEXW lpVersionInformation,
    DWORD dwTypeMask,
    DWORDLONG dwlConditionMask
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
CheckNameLegalDOS8Dot3A(
    LPCSTR lpName,
    LPSTR lpOemName OPTIONAL,
    DWORD OemNameSize OPTIONAL,
    PBOOL pbNameContainsSpaces OPTIONAL,
    PBOOL pbNameLegal
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
CreateJobObjectA(
    LPSECURITY_ATTRIBUTES lpJobAttributes,
    LPCSTR lpName
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeleteVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DnsHostnameToComputerNameA (
    LPCSTR Hostname,
    LPSTR ComputerName,
    LPDWORD nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindActCtxSectionStringA(
    DWORD dwFlags,
    const GUID *lpExtensionGuid,
    ULONG ulSectionId,
    LPCSTR lpStringToFind,
    PACTCTX_SECTION_KEYED_DATA ReturnedData
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstVolumeA(
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
FindFirstVolumeMountPointA(
    LPCSTR lpszRootPathName,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindNextVolumeA(
    HANDLE hFindVolume,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
FindNextVolumeMountPointA(
    HANDLE hFindVolumeMountPoint,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetDllDirectoryA(
    DWORD nBufferLength,
    LPSTR lpBuffer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetFirmwareEnvironmentVariableA(
    LPCSTR lpName,
    LPCSTR lpGuid,
    PVOID   pBuffer,
    DWORD    nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD
STDCALL
GetLongPathNameA(
    LPCSTR lpszShortPath,
    LPSTR  lpszLongPath,
    DWORD    cchBuffer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetModuleHandleExA(
    DWORD        dwFlags,
    LPCSTR     lpModuleName,
    HMODULE*    phModule
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
UINT
STDCALL
GetSystemWow64DirectoryA(
    LPSTR lpBuffer,
    UINT uSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetVolumeNameForVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint,
    LPSTR lpszVolumeName,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetVolumePathNameA(
    LPCSTR lpszFileName,
    LPSTR lpszVolumePathName,
    DWORD cchBufferLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
GetVolumePathNamesForVolumeNameA(
    LPCSTR lpszVolumeName,
    LPSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
STDCALL
OpenJobObjectA(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCSTR lpName
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
ReplaceFileA(
    LPCSTR  lpReplacedFileName,
    LPCSTR  lpReplacementFileName,
    LPCSTR  lpBackupFileName,
    DWORD   dwReplaceFlags,
    LPVOID  lpExclude,
    LPVOID  lpReserved
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetComputerNameExA (
    COMPUTER_NAME_FORMAT NameType,
    LPCSTR lpBuffer
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetDllDirectoryA(
    LPCSTR lpPathName
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetFirmwareEnvironmentVariableA(
    LPCSTR lpName,
    LPCSTR lpGuid,
    PVOID    pValue,
    DWORD    nSize
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint,
    LPCSTR lpszVolumeName
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
VerifyVersionInfoA(
    LPOSVERSIONINFOEXA lpVersionInformation,
    DWORD dwTypeMask,
    DWORDLONG dwlConditionMask
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
EnumSystemLanguageGroupsW(
    LANGUAGEGROUP_ENUMPROCW lpLanguageGroupEnumProc,
    DWORD                   dwFlags,
    LONG_PTR                lParam)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
ULONGLONG
STDCALL
VerSetConditionMask(
        ULONGLONG   ConditionMask,
        DWORD   TypeMask,
        BYTE    Condition
        )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL GetConsoleKeyboardLayoutNameA(LPSTR name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL GetConsoleKeyboardLayoutNameW(LPWSTR name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
DWORD STDCALL GetHandleContext(HANDLE hnd)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
HANDLE STDCALL CreateSocketHandle(VOID)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL SetHandleContext(HANDLE hnd,DWORD context)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL SetConsoleInputExeNameA(LPCSTR name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL SetConsoleInputExeNameW(LPCWSTR name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL UTRegister( HMODULE hModule, LPSTR lpsz16BITDLL,
                        LPSTR lpszInitName, LPSTR lpszProcName,
                        FARPROC *ppfn32Thunk, FARPROC pfnUT32CallBack,
                        LPVOID lpBuff )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL UTUnRegister( HMODULE hModule )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
FARPROC STDCALL DelayLoadFailureHook(unsigned int dliNotify, PDelayLoadInfo pdli)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
NTSTATUS STDCALL CreateNlsSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,ULONG Size,ULONG AccessMask)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL GetConsoleInputExeNameA(ULONG length,LPCSTR name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL GetConsoleInputExeNameW(ULONG length,LPCWSTR name)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL STDCALL IsValidUILanguage(LANGID langid)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
VOID STDCALL NlsConvertIntegerToString(ULONG Value,ULONG Base,ULONG strsize, LPWSTR str, ULONG strsize2)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
}

/*
 * @unimplemented
 */
UINT STDCALL SetCPGlobal(UINT CodePage)
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
SetClientTimeZoneInformation(
		       CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation
		       )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}
