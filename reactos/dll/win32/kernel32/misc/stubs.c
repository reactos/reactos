/*
 * KERNEL32.DLL stubs (STUB functions)
 * Remove from this file, if you implement them.
 */

#include <k32.h>

#define NDEBUG
#include <debug.h>

#define STUB \
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED); \
  DPRINT1("%s() is UNIMPLEMENTED!\n", __FUNCTION__)

/*
 * @unimplemented
 */
BOOL
WINAPI
BaseAttachCompleteThunk (VOID)
{
    STUB;
    return FALSE;
}

/*
 * @unimplemented
 */
VOID WINAPI
BaseDumpAppcompatCache(VOID)
{
    STUB;
}

/*
 * @unimplemented
 */
VOID WINAPI
BaseFlushAppcompatCache(VOID)
{
    STUB;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
BaseCheckAppcompatCache(ULONG Unknown1,
                        ULONG Unknown2,
                        ULONG Unknown3,
                        PULONG Unknown4)
{
    STUB;
    if (Unknown4) *Unknown4 = 0;
    return TRUE;
}

/*
 * @unimplemented
 */
VOID WINAPI
BaseUpdateAppcompatCache(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3)
{
    STUB;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
CmdBatNotification (
    DWORD   Unknown
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
CreateVirtualBuffer (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
ExitVDM (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
ExtendVirtualBuffer (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
FreeVirtualBuffer (
    HANDLE  hVirtualBuffer
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
GetNextVDMCommand (
    DWORD   Unknown0
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetSystemPowerStatus (
    LPSYSTEM_POWER_STATUS PowerStatus
    )
{
    STUB;
    PowerStatus->ACLineStatus = 1;
    PowerStatus->BatteryFlag = 128;
    PowerStatus->BatteryLifePercent = 255;
    PowerStatus->Reserved1 = 0;
    PowerStatus->BatteryLifeTime = -1;
    PowerStatus->BatteryFullLifeTime = -1;
    return TRUE;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
GetVDMCurrentDirectories (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
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
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
RegisterWowBaseHandlers (
    DWORD   Unknown0
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
RegisterWowExec (
    DWORD   Unknown0
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI
SetSystemPowerState (
    BOOL fSuspend,
    BOOL fForce
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
SetVDMCurrentDirectories (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
TrimVirtualBuffer (
    DWORD   Unknown0
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
VDMConsoleOperation (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
VDMOperationStarted (
    DWORD   Unknown0
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
VirtualBufferExceptionHandler (
    DWORD   Unknown0,
    DWORD   Unknown1,
    DWORD   Unknown2
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
AllocateUserPhysicalPages(
    HANDLE hProcess,
    PULONG_PTR NumberOfPages,
    PULONG_PTR UserPfnArray
    )
{
    STUB;
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
BindIoCompletionCallback(HANDLE FileHandle,
                         LPOVERLAPPED_COMPLETION_ROUTINE Function,
                         ULONG Flags)
{
    NTSTATUS Status = 0;

    DPRINT("(%p, %p, %d)\n", FileHandle, Function, Flags);

    Status = RtlSetIoCompletionCallback(FileHandle,
                                        (PIO_APC_ROUTINE)Function,
                                        Flags);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
CancelDeviceWakeupRequest(
    HANDLE hDevice
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
CreateJobSet (
    ULONG NumJob,
    PJOB_SET_ARRAY UserJobSet,
    ULONG Flags)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
FindVolumeMountPointClose(
    HANDLE hFindVolumeMountPoint
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
FreeUserPhysicalPages(
    HANDLE hProcess,
    PULONG_PTR NumberOfPages,
    PULONG_PTR PageArray
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetDevicePowerState(
    HANDLE hDevice,
    BOOL *pfOn
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
GetNumaHighestNodeNumber(
    PULONG HighestNodeNumber
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetNumaNodeProcessorMask(
    UCHAR Node,
    PULONGLONG ProcessorMask
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetNumaProcessorNode(
    UCHAR Processor,
    PUCHAR NodeNumber
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
HeapQueryInformation (
    HANDLE HeapHandle,
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL,
    PSIZE_T ReturnLength OPTIONAL
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
HeapSetInformation (
    HANDLE HeapHandle,
    HEAP_INFORMATION_CLASS HeapInformationClass,
    PVOID HeapInformation OPTIONAL,
    SIZE_T HeapInformationLength OPTIONAL
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
IsSystemResumeAutomatic(
    VOID
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
MapUserPhysicalPages(
    PVOID VirtualAddress,
    ULONG_PTR NumberOfPages,
    PULONG_PTR PageArray  OPTIONAL
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
MapUserPhysicalPagesScatter(
    PVOID *VirtualAddresses,
    ULONG_PTR NumberOfPages,
    PULONG_PTR PageArray  OPTIONAL
    )
{
    STUB;
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
ReadFileScatter(HANDLE hFile,
                FILE_SEGMENT_ELEMENT aSegmentArray[],
                DWORD nNumberOfBytesToRead,
                LPDWORD lpReserved,
                LPOVERLAPPED lpOverlapped)
{
    PIO_STATUS_BLOCK pIOStatus;
    LARGE_INTEGER Offset;
    NTSTATUS Status;

    DPRINT("(%p %p %u %p)\n", hFile, aSegmentArray, nNumberOfBytesToRead, lpOverlapped);

    Offset.LowPart  = lpOverlapped->Offset;
    Offset.HighPart = lpOverlapped->OffsetHigh;
    pIOStatus = (PIO_STATUS_BLOCK) lpOverlapped;
    pIOStatus->Status = STATUS_PENDING;
    pIOStatus->Information = 0;

    Status = NtReadFileScatter(hFile,
                               NULL,
                               NULL,
                               NULL,
                               pIOStatus,
                               aSegmentArray,
                               nNumberOfBytesToRead,
                               &Offset,
                               NULL);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @unimplemented
 */
ULONG
WINAPI
RemoveVectoredExceptionHandler(
    PVOID VectoredHandlerHandle
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
RequestDeviceWakeup(
    HANDLE hDevice
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
RequestWakeupLatency(
    LATENCY_TIME latency
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
UINT
WINAPI
ResetWriteWatch(
    LPVOID lpBaseAddress,
    SIZE_T dwRegionSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
VOID
WINAPI
RestoreLastError(
    DWORD dwErrCode
    )
{
    STUB;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetMessageWaitingIndicator(
    HANDLE hMsgIndicator,
    ULONG ulMsgCount
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
EXECUTION_STATE
WINAPI
SetThreadExecutionState(
    EXECUTION_STATE esFlags
    )
{
    static EXECUTION_STATE current =
        ES_SYSTEM_REQUIRED | ES_DISPLAY_REQUIRED | ES_USER_PRESENT;
    EXECUTION_STATE old = current;

    DPRINT1("(0x%x): stub, harmless.\n", esFlags);

    if (!(current & ES_CONTINUOUS) || (esFlags & ES_CONTINUOUS))
        current = esFlags;
    return old;
}

/*
 * @implemented
 */
BOOL
WINAPI
WriteFileGather(HANDLE hFile,
                FILE_SEGMENT_ELEMENT aSegmentArray[],
                DWORD nNumberOfBytesToWrite,
                LPDWORD lpReserved,
                LPOVERLAPPED lpOverlapped)
{
    PIO_STATUS_BLOCK IOStatus;
    LARGE_INTEGER Offset;
    NTSTATUS Status;

    DPRINT("%p %p %u %p\n", hFile, aSegmentArray, nNumberOfBytesToWrite, lpOverlapped);

    Offset.LowPart = lpOverlapped->Offset;
    Offset.HighPart = lpOverlapped->OffsetHigh;
    IOStatus = (PIO_STATUS_BLOCK) lpOverlapped;
    IOStatus->Status = STATUS_PENDING;
    IOStatus->Information = 0;

    Status = NtWriteFileGather(hFile,
                               NULL,
                               NULL,
                               NULL,
                               IOStatus,
                               aSegmentArray,
                               nNumberOfBytesToWrite,
                               &Offset,
                               NULL);

    if (!NT_SUCCESS(Status))
    {
        SetLastError(RtlNtStatusToDosError(Status));
        return FALSE;
    }

    return TRUE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DeleteVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
FindFirstVolumeMountPointW(
    LPCWSTR lpszRootPathName,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
FindNextVolumeW(
	HANDLE handle,
	LPWSTR volume,
	DWORD len
    )
{
    MOUNTMGR_MOUNT_POINTS *data = handle;

    while (data->Size < data->NumberOfMountPoints)
    {
        static const WCHAR volumeW[] = {'\\','?','?','\\','V','o','l','u','m','e','{',};
        WCHAR *link = (WCHAR *)((char *)data + data->MountPoints[data->Size].SymbolicLinkNameOffset);
        DWORD size = data->MountPoints[data->Size].SymbolicLinkNameLength;
        data->Size++;
        /* skip non-volumes */
        if (size < sizeof(volumeW) || memcmp( link, volumeW, sizeof(volumeW) )) continue;
        if (size + sizeof(WCHAR) >= len * sizeof(WCHAR))
        {
            SetLastError( ERROR_FILENAME_EXCED_RANGE );
            return FALSE;
        }
        memcpy( volume, link, size );
        volume[1] = '\\';  /* map \??\ to \\?\ */
        volume[size / sizeof(WCHAR)] = '\\';  /* Windows appends a backslash */
        volume[size / sizeof(WCHAR) + 1] = 0;
        DPRINT( "returning entry %u %s\n", data->Size - 1, volume );
        return TRUE;
    }
    SetLastError( ERROR_NO_MORE_FILES );
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
FindNextVolumeMountPointW(
    HANDLE hFindVolumeMountPoint,
    LPWSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
GetFirmwareEnvironmentVariableW(
    LPCWSTR lpName,
    LPCWSTR lpGuid,
    PVOID   pBuffer,
    DWORD    nSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetVolumePathNameW(
    LPCWSTR lpszFileName,
    LPWSTR lpszVolumePathName,
    DWORD cchBufferLength
    )
{
    STUB;
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetVolumePathNamesForVolumeNameW(
    LPCWSTR lpszVolumeName,
    LPWSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetFirmwareEnvironmentVariableW(
    LPCWSTR lpName,
    LPCWSTR lpGuid,
    PVOID    pValue,
    DWORD    nSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetVolumeMountPointW(
    LPCWSTR lpszVolumeMountPoint,
    LPCWSTR lpszVolumeName
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DeleteVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE
WINAPI
FindFirstVolumeMountPointA(
    LPCSTR lpszRootPathName,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
FindNextVolumeA(HANDLE handle,
                LPSTR volume,
                DWORD len)
{
    WCHAR *buffer = HeapAlloc(GetProcessHeap(), 0, len * sizeof(WCHAR));
    BOOL ret;

    if ((ret = FindNextVolumeW( handle, buffer, len )))
    {
        if (!WideCharToMultiByte( CP_ACP, 0, buffer, -1, volume, len, NULL, NULL )) ret = FALSE;
    }

    HeapFree( GetProcessHeap(), 0, buffer );
    return ret;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
FindNextVolumeMountPointA(
    HANDLE hFindVolumeMountPoint,
    LPSTR lpszVolumeMountPoint,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
GetFirmwareEnvironmentVariableA(
    LPCSTR lpName,
    LPCSTR lpGuid,
    PVOID   pBuffer,
    DWORD    nSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetVolumePathNameA(
    LPCSTR lpszFileName,
    LPSTR lpszVolumePathName,
    DWORD cchBufferLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetVolumePathNamesForVolumeNameA(
    LPCSTR lpszVolumeName,
    LPSTR lpszVolumePathNames,
    DWORD cchBufferLength,
    PDWORD lpcchReturnLength
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetFirmwareEnvironmentVariableA(
    LPCSTR lpName,
    LPCSTR lpGuid,
    PVOID    pValue,
    DWORD    nSize
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetVolumeMountPointA(
    LPCSTR lpszVolumeMountPoint,
    LPCSTR lpszVolumeName
    )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI GetConsoleKeyboardLayoutNameA(LPSTR name)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI GetConsoleKeyboardLayoutNameW(LPWSTR name)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD WINAPI GetHandleContext(HANDLE hnd)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
HANDLE WINAPI CreateSocketHandle(VOID)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI SetHandleContext(HANDLE hnd,DWORD context)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI UTRegister( HMODULE hModule, LPSTR lpsz16BITDLL,
                        LPSTR lpszInitName, LPSTR lpszProcName,
                        FARPROC *ppfn32Thunk, FARPROC pfnUT32CallBack,
                        LPVOID lpBuff )
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
VOID WINAPI UTUnRegister( HMODULE hModule )
{
    STUB;
}

/*
 * @unimplemented
 */
#if 0
FARPROC WINAPI DelayLoadFailureHook(unsigned int dliNotify, PDelayLoadInfo pdli)
#else
FARPROC WINAPI DelayLoadFailureHook(unsigned int dliNotify, PVOID pdli)
#endif
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
NTSTATUS WINAPI CreateNlsSecurityDescriptor(PSECURITY_DESCRIPTOR SecurityDescriptor,ULONG Size,ULONG AccessMask)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI IsValidUILanguage(LANGID langid)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
VOID WINAPI NlsConvertIntegerToString(ULONG Value,ULONG Base,ULONG strsize, LPWSTR str, ULONG strsize2)
{
    STUB;
}

/*
 * @unimplemented
 */
UINT WINAPI SetCPGlobal(UINT CodePage)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetClientTimeZoneInformation(
    CONST TIME_ZONE_INFORMATION *lpTimeZoneInformation)
{
    STUB;
    return 0;
}

ULONG
WINAPI
NlsGetCacheUpdateCount(VOID)
{
    STUB;
    return 0;
}

BOOL
WINAPI
Wow64EnableWow64FsRedirection (BOOL Wow64EnableWow64FsRedirection)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
Wow64DisableWow64FsRedirection (VOID ** pv)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
Wow64RevertWow64FsRedirection (VOID * pv)
{
    STUB;
    return FALSE;
}

UINT
WINAPI
EnumSystemFirmwareTables(IN DWORD FirmwareTableProviderSignature,
                         OUT PVOID pFirmwareTableBuffer,
                         IN DWORD BufferSize)
{
    STUB;
    return 0;
}

BOOL
WINAPI
GetSystemFileCacheSize(OUT PSIZE_T lpMinimumFileCacheSize,
                       OUT PSIZE_T lpMaximumFileCacheSize,
                       OUT PDWORD lpFlags)
{
    STUB;
    return FALSE;
}

UINT
WINAPI
GetSystemFirmwareTable(IN DWORD FirmwareTableProviderSignature,
                       IN DWORD FirmwareTableID,
                       OUT PVOID pFirmwareTableBuffer,
                       IN DWORD BufferSize)
{
    STUB;
    return 0;
}

BOOL
WINAPI
SetSystemFileCacheSize(IN SIZE_T MinimumFileCacheSize,
                       IN SIZE_T MaximumFileCacheSize,
                       IN DWORD Flags)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
SetThreadStackGuarantee(IN OUT PULONG StackSizeInBytes)
{
    STUB;
    return FALSE;
}

HANDLE
WINAPI
ReOpenFile(IN HANDLE hOriginalFile,
           IN DWORD dwDesiredAccess,
           IN DWORD dwShareMode,
           IN DWORD dwFlags)
{
    STUB;
    return INVALID_HANDLE_VALUE;
}

BOOL
WINAPI
SetProcessWorkingSetSizeEx(IN HANDLE hProcess,
                           IN SIZE_T dwMinimumWorkingSetSize,
                           IN SIZE_T dwMaximumWorkingSetSize,
                           IN DWORD Flags)
{
    STUB;
    return FALSE;
}


BOOL
WINAPI
GetProcessWorkingSetSizeEx(IN HANDLE hProcess,
                           OUT PSIZE_T lpMinimumWorkingSetSize,
                           OUT PSIZE_T lpMaximumWorkingSetSize,
                           OUT PDWORD Flags)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
GetLogicalProcessorInformation(OUT PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer,
                               IN OUT PDWORD ReturnLength)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
GetNumaAvailableMemoryNode(IN UCHAR Node,
                           OUT PULONGLONG AvailableBytes)
{
    STUB;
    return FALSE;
}
