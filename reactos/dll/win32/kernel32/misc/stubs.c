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
VOID
WINAPI
RestoreLastError(
    DWORD dwErrCode
    )
{
    STUB;
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
    return INVALID_HANDLE_VALUE;
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
FARPROC WINAPI DelayLoadFailureHook(LPCSTR pszDllName, LPCSTR pszProcName)
{
    STUB;
    return NULL;
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

BOOL
WINAPI
GetNumaAvailableMemory(PVOID lpInfo,
                       ULONG Length,
                       PULONG ReturnLength)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
GetNumaProcessorMap(PVOID lpInfo,
                    ULONG Length,
                    PULONG ReturnLength)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
NlsResetProcessLocale(VOID)
{
    STUB;
    return TRUE;
}

DWORD
WINAPI
AddLocalAlternateComputerNameA(LPSTR lpName, PNTSTATUS Status)
{
    STUB;
    return 0;
}

DWORD
WINAPI
AddLocalAlternateComputerNameW(LPWSTR lpName, PNTSTATUS Status)
{
    STUB;
    return 0;
}

NTSTATUS
WINAPI
BaseCleanupAppcompatCache()
{
    STUB;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
WINAPI
BaseCleanupAppcompatCacheSupport(PVOID pUnknown)
{
    STUB;
    return STATUS_NOT_IMPLEMENTED;
}

BOOL
WINAPI
BaseInitAppcompatCache(VOID)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
BaseInitAppcompatCacheSupport(VOID)
{
    STUB;
    return FALSE;
}

VOID
WINAPI
CreateProcessInternalWSecure(VOID)
{
    STUB;
}

DWORD
WINAPI
EnumerateLocalComputerNamesA(PVOID pUnknown, DWORD Size, LPSTR lpBuffer, LPDWORD lpnSize)
{
    STUB;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
EnumerateLocalComputerNamesW(PVOID pUnknown, DWORD Size, LPWSTR lpBuffer, LPDWORD lpnSize)
{
    STUB;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

PVOID
WINAPI
GetComPlusPackageInstallStatus(VOID)
{
    STUB;
    return NULL;
}

BOOL
WINAPI
GetConsoleCharType(HANDLE hConsole, COORD Coord, PDWORD Type)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
GetConsoleCursorMode(HANDLE hConsole, PBOOL pUnknown1, PBOOL pUnknown2)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
GetConsoleNlsMode(HANDLE hConsole, LPDWORD lpMode)
{
    STUB;
    return FALSE;
}

VOID
WINAPI
GetDefaultSortkeySize(LPVOID lpUnknown)
{
    STUB;
    lpUnknown = NULL;
}

VOID
WINAPI
GetLinguistLangSize(LPVOID lpUnknown)
{
    STUB;
    lpUnknown = NULL;
}

BOOL
WINAPI
OpenDataFile(HANDLE hFile, DWORD dwUnused)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
OpenProfileUserMapping(VOID)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
PrivMoveFileIdentityW(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
ReadConsoleInputExA(HANDLE hConsole, LPVOID lpBuffer, DWORD dwLen, LPDWORD Unknown1, DWORD Unknown2)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
ReadConsoleInputExW(HANDLE hConsole, LPVOID lpBuffer, DWORD dwLen, LPDWORD Unknown1, DWORD Unknown2)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
RegisterConsoleIME(HWND hWnd, LPDWORD ThreadId)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
RegisterConsoleOS2(BOOL bUnknown)
{
    STUB;
    return FALSE;
}

DWORD
WINAPI
RemoveLocalAlternateComputerNameA(LPSTR lpName, DWORD Unknown)
{
    STUB;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

DWORD
WINAPI
RemoveLocalAlternateComputerNameW(LPWSTR lpName, DWORD Unknown)
{
    STUB;
    return ERROR_CALL_NOT_IMPLEMENTED;
}

BOOL
WINAPI
SetComPlusPackageInstallStatus(LPVOID lpInfo)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
SetConsoleCursorMode(HANDLE hConsole, BOOL Unknown1, BOOL Unknown2)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
SetConsoleLocalEUDC(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
SetConsoleNlsMode(HANDLE hConsole, DWORD dwMode)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
SetConsoleOS2OemFormat(BOOL bUnknown)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
UnregisterConsoleIME(VOID)
{
    STUB;
    return FALSE;
}

BOOL WINAPI TermsrvAppInstallMode(void)
{
     STUB;
     return FALSE;
}

DWORD WINAPI SetTermsrvAppInstallMode(BOOL bInstallMode)
{
    STUB;
    return 0;
}

