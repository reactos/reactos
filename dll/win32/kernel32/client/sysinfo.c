/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:         System Information Functions
 * COPYRIGHT:       Emanuele Aliberti
 *                  Christoph von Wittich
 *                  Thomas Weidenmueller
 *                  Gunnar Andre Dalsnes
 *                  Stanislav Motylkov (x86corez@gmail.com)
 *                  Mark Jansen (mark.jansen@reactos.org)
 *                  Copyright 2023 Ratin Gao <ratin@knsoft.org>
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

#define PV_NT351 0x00030033

/* PRIVATE FUNCTIONS **********************************************************/

VOID
WINAPI
GetSystemInfoInternal(IN PSYSTEM_BASIC_INFORMATION BasicInfo,
                      IN PSYSTEM_PROCESSOR_INFORMATION ProcInfo,
                      OUT LPSYSTEM_INFO SystemInfo)
{
    RtlZeroMemory(SystemInfo, sizeof (SYSTEM_INFO));
    SystemInfo->wProcessorArchitecture = ProcInfo->ProcessorArchitecture;
    SystemInfo->wReserved = 0;
    SystemInfo->dwPageSize = BasicInfo->PageSize;
    SystemInfo->lpMinimumApplicationAddress = (PVOID)BasicInfo->MinimumUserModeAddress;
    SystemInfo->lpMaximumApplicationAddress = (PVOID)BasicInfo->MaximumUserModeAddress;
    SystemInfo->dwActiveProcessorMask = BasicInfo->ActiveProcessorsAffinityMask;
    SystemInfo->dwNumberOfProcessors = BasicInfo->NumberOfProcessors;
    SystemInfo->wProcessorLevel = ProcInfo->ProcessorLevel;
    SystemInfo->wProcessorRevision = ProcInfo->ProcessorRevision;
    SystemInfo->dwAllocationGranularity = BasicInfo->AllocationGranularity;

    switch (ProcInfo->ProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_INTEL:
            switch (ProcInfo->ProcessorLevel)
            {
                case 3:
                    SystemInfo->dwProcessorType = PROCESSOR_INTEL_386;
                    break;
                case 4:
                    SystemInfo->dwProcessorType = PROCESSOR_INTEL_486;
                    break;
                default:
                    SystemInfo->dwProcessorType = PROCESSOR_INTEL_PENTIUM;
            }
            break;

        case PROCESSOR_ARCHITECTURE_AMD64:
            SystemInfo->dwProcessorType = PROCESSOR_AMD_X8664;
            break;

        case PROCESSOR_ARCHITECTURE_IA64:
            SystemInfo->dwProcessorType = PROCESSOR_INTEL_IA64;
            break;

        default:
            SystemInfo->dwProcessorType = 0;
            break;
    }

    if (PV_NT351 > GetProcessVersion(0))
    {
        SystemInfo->wProcessorLevel = 0;
        SystemInfo->wProcessorRevision = 0;
    }
}

static
UINT
BaseQuerySystemFirmware(
    _In_ DWORD FirmwareTableProviderSignature,
    _In_ DWORD FirmwareTableID,
    _Out_writes_bytes_to_opt_(BufferSize, return) PVOID pFirmwareTableBuffer,
    _In_ DWORD BufferSize,
    _In_ SYSTEM_FIRMWARE_TABLE_ACTION Action)
{
    SYSTEM_FIRMWARE_TABLE_INFORMATION* SysFirmwareInfo;
    ULONG Result = 0, ReturnedSize;
    ULONG TotalSize = BufferSize + sizeof(SYSTEM_FIRMWARE_TABLE_INFORMATION);
    NTSTATUS Status;

    SysFirmwareInfo = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, TotalSize);
    if (!SysFirmwareInfo)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    _SEH2_TRY
    {
        SysFirmwareInfo->ProviderSignature = FirmwareTableProviderSignature;
        SysFirmwareInfo->TableID = FirmwareTableID;
        SysFirmwareInfo->Action = Action;
        SysFirmwareInfo->TableBufferLength = BufferSize;

        Status = NtQuerySystemInformation(SystemFirmwareTableInformation, SysFirmwareInfo, TotalSize, &ReturnedSize);

        if (NT_SUCCESS(Status) || Status == STATUS_BUFFER_TOO_SMALL)
            Result = SysFirmwareInfo->TableBufferLength;

        if (NT_SUCCESS(Status) && pFirmwareTableBuffer)
        {
            RtlCopyMemory(pFirmwareTableBuffer, SysFirmwareInfo->TableBuffer, SysFirmwareInfo->TableBufferLength);
        }
    }
    _SEH2_FINALLY
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, SysFirmwareInfo);
    }
    _SEH2_END;

    BaseSetLastNTError(Status);
    return Result;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/*
 * @implemented
 */
SIZE_T
WINAPI
GetLargePageMinimum(VOID)
{
    return SharedUserData->LargePageMinimum;
}

/*
 * @implemented
 */
VOID
WINAPI
GetSystemInfo(IN LPSYSTEM_INFO lpSystemInfo)
{
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PROCESSOR_INFORMATION ProcInfo;
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &BasicInfo,
                                      sizeof(BasicInfo),
                                      0);
    if (!NT_SUCCESS(Status)) return;

    Status = NtQuerySystemInformation(SystemProcessorInformation,
                                      &ProcInfo,
                                      sizeof(ProcInfo),
                                      0);
    if (!NT_SUCCESS(Status)) return;

    GetSystemInfoInternal(&BasicInfo, &ProcInfo, lpSystemInfo);
}

/*
 * @implemented
 */
BOOL
WINAPI
IsProcessorFeaturePresent(IN DWORD ProcessorFeature)
{
    if (ProcessorFeature >= PROCESSOR_FEATURE_MAX) return FALSE;
    return ((BOOL)SharedUserData->ProcessorFeatures[ProcessorFeature]);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetSystemRegistryQuota(OUT PDWORD pdwQuotaAllowed,
                       OUT PDWORD pdwQuotaUsed)
{
    SYSTEM_REGISTRY_QUOTA_INFORMATION QuotaInfo;
    ULONG BytesWritten;
    NTSTATUS Status;

    Status = NtQuerySystemInformation(SystemRegistryQuotaInformation,
                                      &QuotaInfo,
                                      sizeof(QuotaInfo),
                                      &BytesWritten);
    if (NT_SUCCESS(Status))
    {
      if (pdwQuotaAllowed) *pdwQuotaAllowed = QuotaInfo.RegistryQuotaAllowed;
      if (pdwQuotaUsed) *pdwQuotaUsed = QuotaInfo.RegistryQuotaUsed;
      return TRUE;
    }

    BaseSetLastNTError(Status);
    return FALSE;
}

/*
 * @implemented
 */
VOID
WINAPI
GetNativeSystemInfo(IN LPSYSTEM_INFO lpSystemInfo)
{
    SYSTEM_BASIC_INFORMATION BasicInfo;
    SYSTEM_PROCESSOR_INFORMATION ProcInfo;
    NTSTATUS Status;

    Status = RtlGetNativeSystemInformation(SystemBasicInformation,
                                           &BasicInfo,
                                           sizeof(BasicInfo),
                                           0);
    if (!NT_SUCCESS(Status)) return;

    Status = RtlGetNativeSystemInformation(SystemProcessorInformation,
                                           &ProcInfo,
                                           sizeof(ProcInfo),
                                           0);
    if (!NT_SUCCESS(Status)) return;

    GetSystemInfoInternal(&BasicInfo, &ProcInfo, lpSystemInfo);
}

/*
 * @implemented
 */
BOOL
WINAPI
GetLogicalProcessorInformation(OUT PSYSTEM_LOGICAL_PROCESSOR_INFORMATION Buffer,
                               IN OUT PDWORD ReturnLength)
{
    NTSTATUS Status;

    if (!ReturnLength)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Status = NtQuerySystemInformation(SystemLogicalProcessorInformation,
                                      Buffer,
                                      *ReturnLength,
                                      ReturnLength);

    /* Normalize the error to what Win32 expects */
    if (Status == STATUS_INFO_LENGTH_MISMATCH) Status = STATUS_BUFFER_TOO_SMALL;
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetNumaHighestNodeNumber(OUT PULONG HighestNodeNumber)
{
    NTSTATUS Status;
    ULONG Length;
    ULONG PartialInfo[2]; // First two members of SYSTEM_NUMA_INFORMATION

    /* Query partial NUMA info */
    Status = NtQuerySystemInformation(SystemNumaProcessorMap,
                                      PartialInfo,
                                      sizeof(PartialInfo),
                                      &Length);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (Length < sizeof(ULONG))
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* First member of the struct is the highest node number */
    *HighestNodeNumber = PartialInfo[0];
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetNumaNodeProcessorMask(IN UCHAR Node,
                         OUT PULONGLONG ProcessorMask)
{
    NTSTATUS Status;
    SYSTEM_NUMA_INFORMATION NumaInformation;
    ULONG Length;

    /* Query NUMA information */
    Status = NtQuerySystemInformation(SystemNumaProcessorMap,
                                      &NumaInformation,
                                      sizeof(NumaInformation),
                                      &Length);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Validate input node number */
    if (Node > NumaInformation.HighestNodeNumber)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Return mask for that node */
    *ProcessorMask = NumaInformation.ActiveProcessorsAffinityMask[Node];
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetNumaProcessorNode(IN UCHAR Processor,
                     OUT PUCHAR NodeNumber)
{
    NTSTATUS Status;
    SYSTEM_NUMA_INFORMATION NumaInformation;
    ULONG Length;
    ULONG Node;
    ULONGLONG Proc;

    /* Can't handle processor number >= 32 */
    if (Processor >= MAXIMUM_PROCESSORS)
    {
        *NodeNumber = -1;
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Query NUMA information */
    Status = NtQuerySystemInformation(SystemNumaProcessorMap,
                                      &NumaInformation,
                                      sizeof(NumaInformation),
                                      &Length);
    if (!NT_SUCCESS(Status))
    {
        *NodeNumber = -1;
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Find ourselves */
    Node = 0;
    Proc = 1ULL << Processor;
    while ((Proc & NumaInformation.ActiveProcessorsAffinityMask[Node]) == 0ULL)
    {
        ++Node;
        /* Out of options */
        if (Node > NumaInformation.HighestNodeNumber)
        {
            *NodeNumber = -1;
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
    }

    /* Return found node */
    *NodeNumber = Node;
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetNumaAvailableMemoryNode(IN UCHAR Node,
                           OUT PULONGLONG AvailableBytes)
{
    NTSTATUS Status;
    SYSTEM_NUMA_INFORMATION NumaInformation;
    ULONG Length;

    /* Query NUMA information */
    Status = NtQuerySystemInformation(SystemNumaAvailableMemory,
                                      &NumaInformation,
                                      sizeof(NumaInformation),
                                      &Length);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    /* Validate input node number */
    if (Node > NumaInformation.HighestNodeNumber)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Return available memory for that node */
    *AvailableBytes = NumaInformation.AvailableMemory[Node];
    return TRUE;
}

_Success_(return > 0)
DWORD
WINAPI
GetFirmwareEnvironmentVariableExW(
    _In_ LPCWSTR lpName,
    _In_ LPCWSTR lpGuid,
    _Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
    _In_ DWORD nSize,
    _Out_opt_ PDWORD pdwAttribubutes);

_Success_(return > 0)
DWORD
WINAPI
GetFirmwareEnvironmentVariableExA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
    _In_ DWORD nSize,
    _Out_opt_ PDWORD pdwAttribubutes);

BOOL
WINAPI
SetFirmwareEnvironmentVariableExW(
    _In_ LPCWSTR lpName,
    _In_ LPCWSTR lpGuid,
    _In_reads_bytes_opt_(nSize) PVOID pValue,
    _In_ DWORD nSize,
    _In_ DWORD dwAttributes);

BOOL
WINAPI
SetFirmwareEnvironmentVariableExA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _In_reads_bytes_opt_(nSize) PVOID pValue,
    _In_ DWORD nSize,
    _In_ DWORD dwAttributes);

_Success_(return > 0)
DWORD
WINAPI
GetFirmwareEnvironmentVariableW(
    _In_ LPCWSTR lpName,
    _In_ LPCWSTR lpGuid,
    _Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
    _In_ DWORD nSize)
{
    return GetFirmwareEnvironmentVariableExW(lpName, lpGuid, pBuffer, nSize, NULL);
}

_Success_(return > 0)
DWORD
WINAPI
GetFirmwareEnvironmentVariableA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _Out_writes_bytes_to_opt_(nSize, return) PVOID pBuffer,
    _In_ DWORD nSize)
{
    return GetFirmwareEnvironmentVariableExA(lpName, lpGuid, pBuffer, nSize, NULL);
}

BOOL
WINAPI
SetFirmwareEnvironmentVariableW(
    _In_ LPCWSTR lpName,
    _In_ LPCWSTR lpGuid,
    _In_reads_bytes_opt_(nSize) PVOID pValue,
    _In_ DWORD nSize)
{
    return SetFirmwareEnvironmentVariableExW(lpName,
                                             lpGuid,
                                             pValue,
                                             nSize,
                                             VARIABLE_ATTRIBUTE_NON_VOLATILE);
}

BOOL
WINAPI
SetFirmwareEnvironmentVariableA(
    _In_ LPCSTR lpName,
    _In_ LPCSTR lpGuid,
    _In_reads_bytes_opt_(nSize) PVOID pValue,
    _In_ DWORD nSize)
{
    return SetFirmwareEnvironmentVariableExA(lpName,
                                             lpGuid,
                                             pValue,
                                             nSize,
                                             VARIABLE_ATTRIBUTE_NON_VOLATILE);
}

/**
 * @name EnumSystemFirmwareTables
 * @implemented
 *
 * Obtains firmware table identifiers.
 * https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-enumsystemfirmwaretables
 *
 * @param FirmwareTableProviderSignature
 * Can be either ACPI, FIRM, or RSMB.
 *
 * @param pFirmwareTableBuffer
 * Pointer to the output buffer, can be NULL.
 *
 * @param BufferSize
 * Size of the output buffer.
 *
 * @return
 * Actual size of the data in case of success, 0 otherwise.
 *
 * @remarks
 * Data would be written to buffer only if the specified size is
 * larger or equal to the actual size, in the other case Last Error
 * value would be set to ERROR_INSUFFICIENT_BUFFER.
 * In case of incorrect provider signature, Last Error value would be
 * set to ERROR_INVALID_FUNCTION.
 *
 */
UINT
WINAPI
EnumSystemFirmwareTables(
    _In_ DWORD FirmwareTableProviderSignature,
    _Out_writes_bytes_to_opt_(BufferSize, return) PVOID pFirmwareTableEnumBuffer,
    _In_ DWORD BufferSize)
{
    return BaseQuerySystemFirmware(FirmwareTableProviderSignature,
                                   0,
                                   pFirmwareTableEnumBuffer,
                                   BufferSize,
                                   SystemFirmwareTable_Enumerate);
}

/**
 * @name GetSystemFirmwareTable
 * @implemented
 *
 * Obtains the firmware table data.
 * https://learn.microsoft.com/en-us/windows/win32/api/sysinfoapi/nf-sysinfoapi-getsystemfirmwaretable
 *
 * @param FirmwareTableProviderSignature
 * Can be either ACPI, FIRM, or RSMB.
 *
 * @param FirmwareTableID
 * Correct table identifier.
 *
 * @param pFirmwareTableBuffer
 * Pointer to the output buffer, can be NULL.
 *
 * @param BufferSize
 * Size of the output buffer.
 *
 * @return
 * Actual size of the data in case of success, 0 otherwise.
 *
 * @remarks
 * Data would be written to buffer only if the specified size is
 * larger or equal to the actual size, in the other case Last Error
 * value would be set to ERROR_INSUFFICIENT_BUFFER.
 * In case of incorrect provider signature, Last Error value would be
 * set to ERROR_INVALID_FUNCTION.
 * Also Last Error value becomes ERROR_NOT_FOUND if incorrect
 * table identifier was specified along with ACPI provider, and
 * ERROR_INVALID_PARAMETER along with FIRM provider. The RSMB provider
 * accepts any table identifier.
 *
 */
UINT
WINAPI
GetSystemFirmwareTable(
    _In_ DWORD FirmwareTableProviderSignature,
    _In_ DWORD FirmwareTableID,
    _Out_writes_bytes_to_opt_(BufferSize, return) PVOID pFirmwareTableBuffer,
    _In_ DWORD BufferSize)
{
    return BaseQuerySystemFirmware(FirmwareTableProviderSignature,
                                   FirmwareTableID,
                                   pFirmwareTableBuffer,
                                   BufferSize,
                                   SystemFirmwareTable_Get);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
GetSystemFileCacheSize(OUT PSIZE_T lpMinimumFileCacheSize,
                       OUT PSIZE_T lpMaximumFileCacheSize,
                       OUT PDWORD lpFlags)
{
    STUB;
    return FALSE;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetSystemFileCacheSize(IN SIZE_T MinimumFileCacheSize,
                       IN SIZE_T MaximumFileCacheSize,
                       IN DWORD Flags)
{
    STUB;
    return FALSE;
}

/*
 * @unimplemented
 */
LONG
WINAPI
GetCurrentPackageId(UINT32 *BufferLength,
                    BYTE *Buffer)
{
    STUB;
    return APPMODEL_ERROR_NO_PACKAGE;
}
