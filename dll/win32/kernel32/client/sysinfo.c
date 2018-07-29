/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/sysinfo.c
 * PURPOSE:         System Information Functions
 * PROGRAMMERS:     Emanuele Aliberti
 *                  Christoph von Wittich
 *                  Thomas Weidenmueller
 *                  Gunnar Andre Dalsnes
 *                  Stanislav Motylkov
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

UINT
WINAPI
GetRawSMBiosTableFromRegistry(OUT PVOID pData)
{
    static const LPCWSTR RegistryKey = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\mssmbios\\Data";
    static const LPCWSTR ValueNameStr = L"SMBiosData";

    PKEY_VALUE_PARTIAL_INFORMATION KeyInfo = NULL;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName;
    UNICODE_STRING ValueName;
    HANDLE KeyHandle;
    ULONG KeyInfoSize;
    ULONG ReturnSize = 0;
    NTSTATUS Status;

    RtlInitUnicodeString(&KeyName, RegistryKey);
    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE,
                               NULL,
                               NULL);

    Status = NtOpenKey(&KeyHandle,
                       KEY_READ,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        return 0;
    }

    // 256 KiB is more than enough for raw SMBIOS dump
    KeyInfoSize = sizeof(KEY_VALUE_PARTIAL_INFORMATION) + 256 * 1024;
    KeyInfo = RtlAllocateHeap(RtlGetProcessHeap(), 0, KeyInfoSize);
    if (KeyInfo == NULL)
    {
        NtClose(KeyHandle);
        goto cleanup;
    }

    RtlInitUnicodeString(&ValueName, ValueNameStr);

    Status = NtQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             KeyInfo,
                             KeyInfoSize,
                             &ReturnSize);

    NtClose(KeyHandle);
    ReturnSize = 0;

    if (!NT_SUCCESS(Status))
    {
        goto cleanup;
    }

    if (KeyInfo->Type != REG_BINARY)
    {
        goto cleanup;
    }

    ReturnSize = KeyInfo->DataLength;
    if (pData)
    {
        RtlCopyMemory(pData, KeyInfo->Data, KeyInfo->DataLength);
    }

cleanup:
    if (KeyInfo)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, KeyInfo);
    }
    return ReturnSize;
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

    /* FIXME: Should be SystemNativeBasicInformation */
    Status = NtQuerySystemInformation(SystemBasicInformation,
                                      &BasicInfo,
                                      sizeof(BasicInfo),
                                      0);
    if (!NT_SUCCESS(Status)) return;
                                  
    /* FIXME: Should be SystemNativeProcessorInformation */
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

/*
 * @unimplemented
 */
DWORD
WINAPI
GetFirmwareEnvironmentVariableW(IN LPCWSTR lpName,
                                IN LPCWSTR lpGuid,
                                IN PVOID pValue,
                                IN DWORD nSize)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetFirmwareEnvironmentVariableW(IN LPCWSTR lpName,
                                IN LPCWSTR lpGuid,
                                IN PVOID pValue,
                                IN DWORD nSize)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
GetFirmwareEnvironmentVariableA(IN LPCSTR lpName,
                                IN LPCSTR lpGuid,
                                IN PVOID pValue,
                                IN DWORD nSize)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetFirmwareEnvironmentVariableA(IN LPCSTR lpName,
                                IN LPCSTR lpGuid,
                                IN PVOID pValue,
                                IN DWORD nSize)
{
    STUB;
    return 0;
}

/**
 * @name EnumSystemFirmwareTables
 * @implemented
 *
 * Obtains firmware table identifiers.
 * https://msdn.microsoft.com/en-us/library/windows/desktop/ms724259(v=vs.85).aspx
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
EnumSystemFirmwareTables(IN DWORD FirmwareTableProviderSignature,
                         OUT PVOID pFirmwareTableBuffer,
                         IN DWORD BufferSize)
{
    UINT uSize = 0;
    UINT uCount = 0;
    DWORD dwError = ERROR_SUCCESS;
    PDWORD pBuffer = NULL;

    switch (FirmwareTableProviderSignature)
    {
        case 'ACPI':
        {
            /* FIXME: Not implemented yet */
            dwError = ERROR_CALL_NOT_IMPLEMENTED;
            break;
        }
        case 'FIRM':
        {
            /* FIXME: Not implemented yet */
            dwError = ERROR_CALL_NOT_IMPLEMENTED;
            break;
        }
        case 'RSMB':
        {
            if (GetRawSMBiosTableFromRegistry(NULL) > 0)
            {
                uCount = 1;
                uSize = uCount * sizeof(DWORD);
                pBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, uSize);
                if (!pBuffer)
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                    return 0;
                }
                *pBuffer = 0;
            }
            break;
        }
        default:
        {
            dwError = ERROR_INVALID_FUNCTION;
        }
    }

    if (dwError == ERROR_SUCCESS)
    {
        if (uSize > 0 && BufferSize >= uSize)
        {
            /* Write to buffer */
            if (pFirmwareTableBuffer)
            {
                RtlMoveMemory(pFirmwareTableBuffer, pBuffer, uSize);
            }
        }
        else if (BufferSize < uSize)
        {
            dwError = ERROR_INSUFFICIENT_BUFFER;
        }
    }

    if (pBuffer)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, pBuffer);
    }
    SetLastError(dwError);
    return uSize;
}

/**
 * @name GetSystemFirmwareTable
 * @implemented
 *
 * Obtains the firmware table data.
 * https://msdn.microsoft.com/en-us/library/windows/desktop/ms724379(v=vs.85).aspx
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
GetSystemFirmwareTable(IN DWORD FirmwareTableProviderSignature,
                       IN DWORD FirmwareTableID,
                       OUT PVOID pFirmwareTableBuffer,
                       IN DWORD BufferSize)
{
    /* This function currently obtains data using a hack (registry workaround)
       which is located here: drivers/input/i8042prt/hwhacks.c
       i8042StoreSMBiosTables is responsible for writing SMBIOS table into registry

       Should be implemented correctly using NtQuerySystemInformation
       along with SystemFirmwareTableInformation class

       Reference: https://github.com/hfiref0x/VMDE/blob/master/src/vmde/sup.c
     */

    UINT uSize = 0;
    DWORD dwError = ERROR_SUCCESS;
    PVOID pBuffer = NULL;

    switch (FirmwareTableProviderSignature)
    {
        case 'ACPI':
        {
            /* FIXME: Not implemented yet */
            dwError = ERROR_CALL_NOT_IMPLEMENTED;
            break;
        }
        case 'FIRM':
        {
            /* FIXME: Not implemented yet */
            dwError = ERROR_CALL_NOT_IMPLEMENTED;
            break;
        }
        case 'RSMB':
        {
            uSize = GetRawSMBiosTableFromRegistry(NULL);
            if (uSize > 0)
            {
                pBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, uSize);
                if (!pBuffer)
                {
                    SetLastError(ERROR_OUTOFMEMORY);
                    return 0;
                }
                GetRawSMBiosTableFromRegistry(pBuffer);
            }
            else
            {
                dwError = ERROR_NOT_FOUND;
            }
            break;
        }
        default:
        {
            dwError = ERROR_INVALID_FUNCTION;
        }
    }

    if (dwError == ERROR_SUCCESS)
    {
        if (uSize > 0 && BufferSize >= uSize)
        {
            /* Write to buffer */
            if (pFirmwareTableBuffer)
            {
                RtlMoveMemory(pFirmwareTableBuffer, pBuffer, uSize);
            }
        }
        else if (BufferSize < uSize)
        {
            dwError = ERROR_INSUFFICIENT_BUFFER;
        }
    }

    if (pBuffer)
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, pBuffer);
    }
    SetLastError(dwError);
    return uSize;
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
