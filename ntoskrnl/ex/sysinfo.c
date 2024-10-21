/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/sysinfo.c
 * PURPOSE:         System information functions
 *
 * PROGRAMMERS:     David Welch (welch@mcmail.com)
 *                  Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <wmidata.h>
#include <wmistr.h>
#define NDEBUG
#include <debug.h>

/* The maximum size of an environment value (in bytes) */
#define MAX_ENVVAL_SIZE 1024

#define SIG_ACPI 0x41435049
#define SIG_FIRM 0x4649524D
#define SIG_RSMB 0x52534D42

extern LIST_ENTRY HandleTableListHead;
extern EX_PUSH_LOCK HandleTableListLock;

FAST_MUTEX ExpEnvironmentLock;
ERESOURCE ExpFirmwareTableResource;
LIST_ENTRY ExpFirmwareTableProviderListHead;

FORCEINLINE
NTSTATUS
ExpConvertLdrModuleToRtlModule(IN ULONG ModuleCount,
                               IN PLDR_DATA_TABLE_ENTRY LdrEntry,
                               OUT PRTL_PROCESS_MODULE_INFORMATION ModuleInfo)
{
    PCHAR p;
    NTSTATUS Status;
    ANSI_STRING ModuleName;

    /* Fill it out */
    ModuleInfo->MappedBase = NULL;
    ModuleInfo->ImageBase = LdrEntry->DllBase;
    ModuleInfo->ImageSize = LdrEntry->SizeOfImage;
    ModuleInfo->Flags = LdrEntry->Flags;
    ModuleInfo->LoadCount = LdrEntry->LoadCount;
    ModuleInfo->LoadOrderIndex = (USHORT)ModuleCount;
    ModuleInfo->InitOrderIndex = 0;

    /* Setup name */
    RtlInitEmptyAnsiString(&ModuleName,
                           ModuleInfo->FullPathName,
                           sizeof(ModuleInfo->FullPathName));

    /* Convert it */
    Status = RtlUnicodeStringToAnsiString(&ModuleName,
                                          &LdrEntry->FullDllName,
                                          FALSE);
    if ((NT_SUCCESS(Status)) || (Status == STATUS_BUFFER_OVERFLOW))
    {
        /* Calculate offset to name */
        p = ModuleName.Buffer + ModuleName.Length;
        while ((p > ModuleName.Buffer) && (*--p))
        {
            /* Check if we found the separator */
            if (*p == OBJ_NAME_PATH_SEPARATOR)
            {
                /* We did, break out */
                p++;
                break;
            }
        }

        /* Set the offset */
        ModuleInfo->OffsetToFileName = (USHORT)(p - ModuleName.Buffer);
    }
    else
    {
        /* Return empty name */
        ModuleInfo->FullPathName[0] = ANSI_NULL;
        ModuleInfo->OffsetToFileName = 0;
    }

    return Status;
}

NTSTATUS
NTAPI
ExpQueryModuleInformation(IN PLIST_ENTRY KernelModeList,
                          IN PLIST_ENTRY UserModeList,
                          OUT PRTL_PROCESS_MODULES Modules,
                          IN ULONG Length,
                          OUT PULONG ReturnLength)
{
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG RequiredLength;
    PRTL_PROCESS_MODULE_INFORMATION ModuleInfo;
    PLDR_DATA_TABLE_ENTRY LdrEntry;
    ULONG ModuleCount = 0;
    PLIST_ENTRY NextEntry;

    /* Setup defaults */
    RequiredLength = FIELD_OFFSET(RTL_PROCESS_MODULES, Modules);
    ModuleInfo = &Modules->Modules[0];

    /* Loop the kernel list */
    NextEntry = KernelModeList->Flink;
    while (NextEntry != KernelModeList)
    {
        /* Get the entry */
        LdrEntry = CONTAINING_RECORD(NextEntry,
                                     LDR_DATA_TABLE_ENTRY,
                                     InLoadOrderLinks);

        /* Update size and check if we can manage one more entry */
        RequiredLength += sizeof(RTL_PROCESS_MODULE_INFORMATION);
        if (Length >= RequiredLength)
        {
            Status = ExpConvertLdrModuleToRtlModule(ModuleCount,
                                                    LdrEntry,
                                                    ModuleInfo);

            /* Go to the next module */
            ModuleInfo++;
        }
        else
        {
            /* Set error code */
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }

        /* Update count and move to next entry */
        ModuleCount++;
        NextEntry = NextEntry->Flink;
    }

    /* Check if caller also wanted user modules */
    if (UserModeList)
    {
        NextEntry = UserModeList->Flink;
        while (NextEntry != UserModeList)
        {
            /* Get the entry */
            LdrEntry = CONTAINING_RECORD(NextEntry,
                                         LDR_DATA_TABLE_ENTRY,
                                         InLoadOrderLinks);

            /* Update size and check if we can manage one more entry */
            RequiredLength += sizeof(RTL_PROCESS_MODULE_INFORMATION);
            if (Length >= RequiredLength)
            {
                Status = ExpConvertLdrModuleToRtlModule(ModuleCount,
                                                        LdrEntry,
                                                        ModuleInfo);

                /* Go to the next module */
                ModuleInfo++;
            }
            else
            {
                /* Set error code */
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }

            /* Update count and move to next entry */
            ModuleCount++;
            NextEntry = NextEntry->Flink;
        }
    }

    /* Update return length */
    if (ReturnLength) *ReturnLength = RequiredLength;

    /* Validate the length again */
    if (Length >= FIELD_OFFSET(RTL_PROCESS_MODULES, Modules))
    {
        /* Set the final count */
        Modules->NumberOfModules = ModuleCount;
    }
    else
    {
        /* Otherwise, we failed */
        Status = STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Done */
    return Status;
}

VOID
NTAPI
ExUnlockUserBuffer(PMDL Mdl)
{
    MmUnlockPages(Mdl);
    ExFreePoolWithTag(Mdl, TAG_MDL);
}

NTSTATUS
NTAPI
ExLockUserBuffer(
    PVOID BaseAddress,
    ULONG Length,
    KPROCESSOR_MODE AccessMode,
    LOCK_OPERATION Operation,
    PVOID *MappedSystemVa,
    PMDL *OutMdl)
{
    PMDL Mdl;
    ASSERT(KeGetCurrentIrql() <= DISPATCH_LEVEL);

    *MappedSystemVa = NULL;
    *OutMdl = NULL;

    /* Allocate an MDL for the buffer */
    Mdl = IoAllocateMdl(BaseAddress, Length, FALSE, TRUE, NULL);
    if (Mdl == NULL)
    {
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Enter SEH for probing */
    _SEH2_TRY
    {
        MmProbeAndLockPages(Mdl, AccessMode, Operation);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ExFreePoolWithTag(Mdl, TAG_MDL);
        _SEH2_YIELD(return _SEH2_GetExceptionCode());
    }
    _SEH2_END;

    /* Return the safe kernel mode buffer */
    *MappedSystemVa = MmGetSystemAddressForMdlSafe(Mdl, NormalPagePriority);
    if (*MappedSystemVa == NULL)
    {
        ExUnlockUserBuffer(Mdl);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Return the MDL */
    *OutMdl = Mdl;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
ExpGetRawSMBiosTable(
    _Out_opt_ PVOID Buffer,
    _Out_ ULONG * OutSize,
    _In_ ULONG BufferSize)
{
    NTSTATUS Status;
    PVOID DataBlockObject;
    PWNODE_ALL_DATA AllData;
    ULONG WMIBufSize;

    ASSERT(OutSize != NULL);
    *OutSize = 0;

    /* Open the data block object for the SMBIOS table */
    Status = IoWMIOpenBlock(&MSSmBios_RawSMBiosTables_GUID,
                            WMIGUID_QUERY,
                            &DataBlockObject);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoWMIOpenBlock failed: 0x%08lx\n", Status);
        return Status;
    }

    /* Query the required buffer size */
    WMIBufSize = 0;
    Status = IoWMIQueryAllData(DataBlockObject, &WMIBufSize, NULL);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoWMIOpenBlock failed: 0x%08lx\n", Status);
        return Status;
    }

    AllData = ExAllocatePoolWithTag(PagedPool, WMIBufSize, 'itfS');
    if (AllData == NULL)
    {
        DPRINT1("Failed to allocate %lu bytes for SMBIOS tables\n", WMIBufSize);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Query the buffer data */
    Status = IoWMIQueryAllData(DataBlockObject, &WMIBufSize, AllData);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("IoWMIOpenBlock failed: 0x%08lx\n", Status);
        ExFreePoolWithTag(AllData, 'itfS');
        return Status;
    }

    Status = STATUS_SUCCESS;
    *OutSize = AllData->FixedInstanceSize;
    if (Buffer != NULL)
    {
        if (BufferSize >= *OutSize)
        {
            RtlMoveMemory(Buffer, AllData + 1, *OutSize);
        }
        else
        {
            Status = STATUS_BUFFER_TOO_SMALL;
        }
    }

    /* Free the buffer */
    ExFreePoolWithTag(AllData, 'itfS');
    return Status;
}

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
VOID
NTAPI
ExGetCurrentProcessorCpuUsage(PULONG CpuUsage)
{
    PKPRCB Prcb;
    ULONG TotalTime;
    ULONGLONG ScaledIdle;

    Prcb = KeGetCurrentPrcb();

    ScaledIdle = (ULONGLONG)Prcb->IdleThread->KernelTime * 100;
    TotalTime = Prcb->KernelTime + Prcb->UserTime;
    if (TotalTime != 0)
        *CpuUsage = (ULONG)(100 - (ScaledIdle / TotalTime));
    else
        *CpuUsage = 0;
}

/*
 * @implemented
 */
VOID
NTAPI
ExGetCurrentProcessorCounts(PULONG IdleTime,
                            PULONG KernelAndUserTime,
                            PULONG ProcessorNumber)
{
    PKPRCB Prcb;

    Prcb = KeGetCurrentPrcb();

    *IdleTime = Prcb->IdleThread->KernelTime;
    *KernelAndUserTime = Prcb->KernelTime + Prcb->UserTime;
    *ProcessorNumber = (ULONG)Prcb->Number;
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
ExIsProcessorFeaturePresent(IN ULONG ProcessorFeature)
{
    /* Quick check to see if it exists at all */
    if (ProcessorFeature >= PROCESSOR_FEATURE_MAX) return(FALSE);

    /* Return our support for it */
    return(SharedUserData->ProcessorFeatures[ProcessorFeature]);
}

/*
 * @implemented
 */
BOOLEAN
NTAPI
ExVerifySuite(SUITE_TYPE SuiteType)
{
    if (SuiteType == Personal) return TRUE;
    return FALSE;
}

NTSTATUS
NTAPI
NtQuerySystemEnvironmentValue(IN PUNICODE_STRING VariableName,
                              OUT PWSTR ValueBuffer,
                              IN ULONG ValueBufferLength,
                              IN OUT PULONG ReturnLength OPTIONAL)
{
    ANSI_STRING AName;
    UNICODE_STRING WName;
    ARC_STATUS Result;
    PCH AnsiValueBuffer;
    ANSI_STRING AValue;
    UNICODE_STRING WValue;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if the call came from user mode */
    PreviousMode = ExGetPreviousMode();
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            /* Probe the input and output buffers */
            ProbeForRead(VariableName, sizeof(UNICODE_STRING), sizeof(ULONG));
            ProbeForWrite(ValueBuffer, ValueBufferLength, sizeof(WCHAR));
            if (ReturnLength != NULL) ProbeForWriteUlong(ReturnLength);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* According to NTInternals the SeSystemEnvironmentName privilege is required! */
    if (!SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege, PreviousMode))
    {
        DPRINT1("NtQuerySystemEnvironmentValue: Caller requires the SeSystemEnvironmentPrivilege privilege!\n");
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Copy the name to kernel space if necessary */
    Status = ProbeAndCaptureUnicodeString(&WName, PreviousMode, VariableName);
    if (!NT_SUCCESS(Status)) return Status;

    /* Convert the name to ANSI and release the captured UNICODE string */
    Status = RtlUnicodeStringToAnsiString(&AName, &WName, TRUE);
    ReleaseCapturedUnicodeString(&WName, PreviousMode);
    if (!NT_SUCCESS(Status)) return Status;

    /* Allocate a buffer for the ANSI environment variable */
    AnsiValueBuffer = ExAllocatePoolWithTag(NonPagedPool, MAX_ENVVAL_SIZE, 'rvnE');
    if (AnsiValueBuffer == NULL)
    {
        RtlFreeAnsiString(&AName);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Get the environment variable and free the ANSI name */
    Result = HalGetEnvironmentVariable(AName.Buffer,
                                       MAX_ENVVAL_SIZE,
                                       AnsiValueBuffer);
    RtlFreeAnsiString(&AName);

    /* Check if we had success */
    if (Result == ESUCCESS)
    {
        /* Copy the result back to the caller. */
        _SEH2_TRY
        {
            /* Initialize ANSI string from the result */
            RtlInitAnsiString(&AValue, AnsiValueBuffer);

            /* Initialize a UNICODE string from the callers buffer */
            RtlInitEmptyUnicodeString(&WValue, ValueBuffer, (USHORT)ValueBufferLength);

            /* Convert the result to UNICODE */
            Status = RtlAnsiStringToUnicodeString(&WValue, &AValue, FALSE);

            if (ReturnLength != NULL)
                *ReturnLength = WValue.Length;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }
    else
    {
        Status = STATUS_UNSUCCESSFUL;
    }

    /* Free the allocated ANSI value buffer */
    ExFreePoolWithTag(AnsiValueBuffer, 'rvnE');

    return Status;
}


NTSTATUS
NTAPI
NtSetSystemEnvironmentValue(IN PUNICODE_STRING VariableName,
                            IN PUNICODE_STRING Value)
{
    UNICODE_STRING CapturedName, CapturedValue;
    ANSI_STRING AName, AValue;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    /*
     * Copy the strings to kernel space if necessary
     */
    Status = ProbeAndCaptureUnicodeString(&CapturedName,
                                          PreviousMode,
                                          VariableName);
    if (NT_SUCCESS(Status))
    {
        Status = ProbeAndCaptureUnicodeString(&CapturedValue,
                                              PreviousMode,
                                              Value);
        if (NT_SUCCESS(Status))
        {
            /*
             * according to ntinternals the SeSystemEnvironmentName privilege is required!
             */
            if (SeSinglePrivilegeCheck(SeSystemEnvironmentPrivilege,
                                       PreviousMode))
            {
                /*
                 * convert the strings to ANSI
                 */
                Status = RtlUnicodeStringToAnsiString(&AName,
                                                      &CapturedName,
                                                      TRUE);
                if (NT_SUCCESS(Status))
                {
                    Status = RtlUnicodeStringToAnsiString(&AValue,
                                                          &CapturedValue,
                                                          TRUE);
                    if (NT_SUCCESS(Status))
                    {
                        ARC_STATUS Result = HalSetEnvironmentVariable(AName.Buffer,
                                                                      AValue.Buffer);

                        Status = (Result ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL);
                    }
                }
            }
            else
            {
                DPRINT1("NtSetSystemEnvironmentValue: Caller requires the SeSystemEnvironmentPrivilege privilege!\n");
                Status = STATUS_PRIVILEGE_NOT_HELD;
            }

            ReleaseCapturedUnicodeString(&CapturedValue,
                                         PreviousMode);
        }

        ReleaseCapturedUnicodeString(&CapturedName,
                                     PreviousMode);
    }

    return Status;
}

NTSTATUS
NTAPI
NtEnumerateSystemEnvironmentValuesEx(IN ULONG InformationClass,
                                     IN PVOID Buffer,
                                     IN ULONG BufferLength)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtQuerySystemEnvironmentValueEx(
    _In_ PUNICODE_STRING VariableName,
    _In_ LPGUID VendorGuid,
    _Out_opt_ PVOID Value,
    _Inout_ PULONG ReturnLength,
    _Out_opt_ PULONG Attributes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
NtSetSystemEnvironmentValueEx(
    _In_ PUNICODE_STRING VariableName,
    _In_ LPGUID VendorGuid,
    _In_reads_bytes_opt_(ValueLength) PVOID Value,
    _In_ ULONG ValueLength,
    _In_ ULONG Attributes)
{
    UNIMPLEMENTED;
    return STATUS_NOT_IMPLEMENTED;
}

/* --- Query/Set System Information --- */

/*
 * NOTE: QSI_DEF(n) and SSI_DEF(n) define _cdecl function symbols
 * so the stack is popped only in one place on x86 platform.
 */
#define QSI_USE(n) QSI##n
#define QSI_DEF(n) \
static NTSTATUS QSI_USE(n) (PVOID Buffer, ULONG Size, PULONG ReqSize)

#define SSI_USE(n) SSI##n
#define SSI_DEF(n) \
static NTSTATUS SSI_USE(n) (PVOID Buffer, ULONG Size)

VOID
NTAPI
ExQueryPoolUsage(OUT PULONG PagedPoolPages,
                 OUT PULONG NonPagedPoolPages,
                 OUT PULONG PagedPoolAllocs,
                 OUT PULONG PagedPoolFrees,
                 OUT PULONG PagedPoolLookasideHits,
                 OUT PULONG NonPagedPoolAllocs,
                 OUT PULONG NonPagedPoolFrees,
                 OUT PULONG NonPagedPoolLookasideHits);

/* Class 0 - Basic Information */
QSI_DEF(SystemBasicInformation)
{
    PSYSTEM_BASIC_INFORMATION Sbi
        = (PSYSTEM_BASIC_INFORMATION) Buffer;

    *ReqSize = sizeof(SYSTEM_BASIC_INFORMATION);

    /* Check user buffer's size */
    if (Size != sizeof(SYSTEM_BASIC_INFORMATION))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    RtlZeroMemory(Sbi, Size);
    Sbi->Reserved = 0;
    Sbi->TimerResolution = KeMaximumIncrement;
    Sbi->PageSize = PAGE_SIZE;
    Sbi->NumberOfPhysicalPages = MmNumberOfPhysicalPages;
    Sbi->LowestPhysicalPageNumber = (ULONG)MmLowestPhysicalPage;
    Sbi->HighestPhysicalPageNumber = (ULONG)MmHighestPhysicalPage;
    Sbi->AllocationGranularity = MM_VIRTMEM_GRANULARITY; /* hard coded on Intel? */
    Sbi->MinimumUserModeAddress = 0x10000; /* Top of 64k */
    Sbi->MaximumUserModeAddress = (ULONG_PTR)MmHighestUserAddress;
    Sbi->ActiveProcessorsAffinityMask = KeActiveProcessors;
    Sbi->NumberOfProcessors = KeNumberProcessors;

    return STATUS_SUCCESS;
}

/* Class 1 - Processor Information */
QSI_DEF(SystemProcessorInformation)
{
    PSYSTEM_PROCESSOR_INFORMATION Spi
        = (PSYSTEM_PROCESSOR_INFORMATION) Buffer;

    *ReqSize = sizeof(SYSTEM_PROCESSOR_INFORMATION);

    /* Check user buffer's size */
    if (Size < sizeof(SYSTEM_PROCESSOR_INFORMATION))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }
    Spi->ProcessorArchitecture = KeProcessorArchitecture;
    Spi->ProcessorLevel = KeProcessorLevel;
    Spi->ProcessorRevision = KeProcessorRevision;
#if (NTDDI_VERSION < NTDDI_WIN8)
    Spi->Reserved = 0;
#else
    Spi->MaximumProcessors = 0;
#endif
    Spi->ProcessorFeatureBits = KeFeatureBits;

    DPRINT("Arch %u Level %u Rev 0x%x\n", Spi->ProcessorArchitecture,
        Spi->ProcessorLevel, Spi->ProcessorRevision);

    return STATUS_SUCCESS;
}

/* Class 2 - Performance Information */
QSI_DEF(SystemPerformanceInformation)
{
    LONG i;
    ULONG IdleUser, IdleKernel;
    PKPRCB Prcb;
    PSYSTEM_PERFORMANCE_INFORMATION Spi
        = (PSYSTEM_PERFORMANCE_INFORMATION) Buffer;

    PEPROCESS TheIdleProcess;

    *ReqSize = sizeof(SYSTEM_PERFORMANCE_INFORMATION);

    /* Check user buffer's size */
    if (Size < sizeof(SYSTEM_PERFORMANCE_INFORMATION))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    TheIdleProcess = PsIdleProcess;

    IdleKernel = KeQueryRuntimeProcess(&TheIdleProcess->Pcb, &IdleUser);
    Spi->IdleProcessTime.QuadPart = UInt32x32To64(IdleKernel, KeMaximumIncrement);
    Spi->IoReadTransferCount = IoReadTransferCount;
    Spi->IoWriteTransferCount = IoWriteTransferCount;
    Spi->IoOtherTransferCount = IoOtherTransferCount;
    Spi->IoReadOperationCount = IoReadOperationCount;
    Spi->IoWriteOperationCount = IoWriteOperationCount;
    Spi->IoOtherOperationCount = IoOtherOperationCount;
    for (i = 0; i < KeNumberProcessors; i ++)
    {
        Prcb = KiProcessorBlock[i];
        if (Prcb)
        {
            Spi->IoReadTransferCount.QuadPart += Prcb->IoReadTransferCount.QuadPart;
            Spi->IoWriteTransferCount.QuadPart += Prcb->IoWriteTransferCount.QuadPart;
            Spi->IoOtherTransferCount.QuadPart += Prcb->IoOtherTransferCount.QuadPart;
            Spi->IoReadOperationCount += Prcb->IoReadOperationCount;
            Spi->IoWriteOperationCount += Prcb->IoWriteOperationCount;
            Spi->IoOtherOperationCount += Prcb->IoOtherOperationCount;
        }
    }

    Spi->AvailablePages = (ULONG)MmAvailablePages;

    Spi->CommittedPages = MmTotalCommittedPages;
    /*
     *  Add up the full system total + pagefile.
     *  All this make Taskmgr happy but not sure it is the right numbers.
     *  This too, fixes some of GlobalMemoryStatusEx numbers.
     */
    Spi->CommitLimit = MmNumberOfPhysicalPages + MiFreeSwapPages + MiUsedSwapPages;

    Spi->PeakCommitment = MmPeakCommitment;
    Spi->PageFaultCount = 0; /* FIXME */
    Spi->CopyOnWriteCount = 0; /* FIXME */
    Spi->TransitionCount = 0; /* FIXME */
    Spi->CacheTransitionCount = 0; /* FIXME */
    Spi->DemandZeroCount = 0; /* FIXME */
    Spi->PageReadCount = 0; /* FIXME */
    Spi->PageReadIoCount = 0; /* FIXME */
    Spi->CacheReadCount = 0; /* FIXME */
    Spi->CacheIoCount = 0; /* FIXME */
    Spi->DirtyPagesWriteCount = 0; /* FIXME */
    Spi->DirtyWriteIoCount = 0; /* FIXME */
    Spi->MappedPagesWriteCount = 0; /* FIXME */
    Spi->MappedWriteIoCount = 0; /* FIXME */

    Spi->PagedPoolPages = 0;
    Spi->NonPagedPoolPages = 0;
    Spi->PagedPoolAllocs = 0;
    Spi->PagedPoolFrees = 0;
    Spi->PagedPoolLookasideHits = 0;
    Spi->NonPagedPoolAllocs = 0;
    Spi->NonPagedPoolFrees = 0;
    Spi->NonPagedPoolLookasideHits = 0;
    ExQueryPoolUsage(&Spi->PagedPoolPages,
                     &Spi->NonPagedPoolPages,
                     &Spi->PagedPoolAllocs,
                     &Spi->PagedPoolFrees,
                     &Spi->PagedPoolLookasideHits,
                     &Spi->NonPagedPoolAllocs,
                     &Spi->NonPagedPoolFrees,
                     &Spi->NonPagedPoolLookasideHits);
    Spi->FreeSystemPtes = 0; /* FIXME */

    Spi->ResidentSystemCodePage = 0; /* FIXME */

    Spi->TotalSystemDriverPages = 0; /* FIXME */
    Spi->Spare3Count = 0; /* FIXME */

    Spi->ResidentSystemCachePage = MiMemoryConsumers[MC_USER].PagesUsed; /* FIXME */
    Spi->ResidentPagedPoolPage = 0; /* FIXME */

    Spi->ResidentSystemDriverPage = 0; /* FIXME */
    Spi->CcFastReadNoWait = 0; /* FIXME */
    Spi->CcFastReadWait = 0; /* FIXME */
    Spi->CcFastReadResourceMiss = 0; /* FIXME */
    Spi->CcFastReadNotPossible = 0; /* FIXME */

    Spi->CcFastMdlReadNoWait = 0; /* FIXME */
    Spi->CcFastMdlReadWait = 0; /* FIXME */
    Spi->CcFastMdlReadResourceMiss = 0; /* FIXME */
    Spi->CcFastMdlReadNotPossible = 0; /* FIXME */

    Spi->CcMapDataNoWait = CcMapDataNoWait;
    Spi->CcMapDataWait = CcMapDataWait;
    Spi->CcMapDataNoWaitMiss = 0; /* FIXME */
    Spi->CcMapDataWaitMiss = 0; /* FIXME */

    Spi->CcPinMappedDataCount = CcPinMappedDataCount;
    Spi->CcPinReadNoWait = CcPinReadNoWait;
    Spi->CcPinReadWait = CcPinReadWait;
    Spi->CcPinReadNoWaitMiss = 0; /* FIXME */
    Spi->CcPinReadWaitMiss = 0; /* FIXME */
    Spi->CcCopyReadNoWait = 0; /* FIXME */
    Spi->CcCopyReadWait = 0; /* FIXME */
    Spi->CcCopyReadNoWaitMiss = 0; /* FIXME */
    Spi->CcCopyReadWaitMiss = 0; /* FIXME */

    Spi->CcMdlReadNoWait = 0; /* FIXME */
    Spi->CcMdlReadWait = 0; /* FIXME */
    Spi->CcMdlReadNoWaitMiss = 0; /* FIXME */
    Spi->CcMdlReadWaitMiss = 0; /* FIXME */
    Spi->CcReadAheadIos = 0; /* FIXME */
    Spi->CcLazyWriteIos = CcLazyWriteIos;
    Spi->CcLazyWritePages = CcLazyWritePages;
    Spi->CcDataFlushes = CcDataFlushes;
    Spi->CcDataPages = CcDataPages;

    Spi->ContextSwitches = 0;
    Spi->FirstLevelTbFills = 0;
    Spi->SecondLevelTbFills = 0;
    Spi->SystemCalls = 0;
    for (i = 0; i < KeNumberProcessors; i ++)
    {
        Prcb = KiProcessorBlock[i];
        if (Prcb)
        {
            Spi->ContextSwitches += KeGetContextSwitches(Prcb);
            Spi->FirstLevelTbFills += Prcb->KeFirstLevelTbFills;
            Spi->SecondLevelTbFills += Prcb->KeSecondLevelTbFills;
            Spi->SystemCalls += Prcb->KeSystemCalls;
        }
    }

    return STATUS_SUCCESS;
}

/* Class 3 - Time Of Day Information */
QSI_DEF(SystemTimeOfDayInformation)
{
    SYSTEM_TIMEOFDAY_INFORMATION Sti;
    LARGE_INTEGER CurrentTime;

    /* Set amount of written information to 0 */
    *ReqSize = 0;

    /* Check user buffer's size */
    if (Size > sizeof(SYSTEM_TIMEOFDAY_INFORMATION))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Get current time */
    KeQuerySystemTime(&CurrentTime);

    /* Zero local buffer */
    RtlZeroMemory(&Sti, sizeof(SYSTEM_TIMEOFDAY_INFORMATION));

    /* Fill local time structure */
    Sti.BootTime= KeBootTime;
    Sti.CurrentTime = CurrentTime;
    Sti.TimeZoneBias.QuadPart = ExpTimeZoneBias.QuadPart;
    Sti.TimeZoneId = ExpTimeZoneId;
    Sti.Reserved = 0;

    /* Copy as much as requested by caller */
    RtlCopyMemory(Buffer, &Sti, Size);

    /* Set amount of information we copied */
    *ReqSize = Size;

    return STATUS_SUCCESS;
}

/* Class 4 - Path Information (DEPRECATED) */
QSI_DEF(SystemPathInformation)
{
    /*
     * Since NT 3.51, this information class is trivially implemented.
     * The path to the NT directory is now stored in KUSER_SHARED_DATA
     * as the NtSystemRoot member.
     * Windows Checked builds show the following message and break to
     * the debugger before failing the function as not implemented.
     */
#if DBG
    DPRINT1("EX: SystemPathInformation now available via SharedUserData\n");
    // DbgBreakPoint(); // Not needed in ReactOS.
#endif
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 5 - Process Information */
QSI_DEF(SystemProcessInformation)
{
    PSYSTEM_PROCESS_INFORMATION SpiCurrent;
    PSYSTEM_THREAD_INFORMATION ThreadInfo;
    PEPROCESS Process = NULL, SystemProcess;
    PETHREAD CurrentThread;
    ANSI_STRING ImageName;
    ULONG CurrentSize;
    USHORT ImageNameMaximumLength; // image name length in bytes
    USHORT ImageNameLength;
    PLIST_ENTRY CurrentEntry;
    ULONG TotalSize = 0, ThreadsCount;
    ULONG TotalUser, TotalKernel;
    PUCHAR Current;
    NTSTATUS Status = STATUS_SUCCESS;
    PUNICODE_STRING TempProcessImageName;
    _SEH2_VOLATILE PUNICODE_STRING ProcessImageName = NULL;
    PWCHAR szSrc;
    BOOLEAN Overflow = FALSE;

    _SEH2_TRY
    {
        /* scan the process list */

        PSYSTEM_PROCESS_INFORMATION Spi
            = (PSYSTEM_PROCESS_INFORMATION) Buffer;

        *ReqSize = sizeof(SYSTEM_PROCESS_INFORMATION);

        /* Check for overflow */
        if (Size < sizeof(SYSTEM_PROCESS_INFORMATION))
        {
            Overflow = TRUE;
        }

        /* Zero user's buffer */
        if (!Overflow) RtlZeroMemory(Spi, Size);

        SystemProcess = PsIdleProcess;
        Process = SystemProcess;
        Current = (PUCHAR) Spi;

        do
        {
            SpiCurrent = (PSYSTEM_PROCESS_INFORMATION) Current;

            /* Lock the Process */
            KeEnterCriticalRegion();
            ExAcquirePushLockShared(&Process->ProcessLock);

            if ((Process->ProcessExiting) &&
                (Process->Pcb.Header.SignalState) &&
                !(Process->ActiveThreads) &&
                (IsListEmpty(&Process->Pcb.ThreadListHead)))
            {
                DPRINT1("Process %p (%s:%p) is a zombie\n",
                        Process, Process->ImageFileName, Process->UniqueProcessId);
                CurrentSize = 0;
                ImageNameMaximumLength = 0;

                /* Unlock the Process */
                ExReleasePushLockShared(&Process->ProcessLock);
                KeLeaveCriticalRegion();
                goto Skip;
            }

            ThreadsCount = 0;
            CurrentEntry = Process->Pcb.ThreadListHead.Flink;
            while (CurrentEntry != &Process->Pcb.ThreadListHead)
            {
                ThreadsCount++;
                CurrentEntry = CurrentEntry->Flink;
            }

            // size of the structure for every process
            CurrentSize = sizeof(SYSTEM_PROCESS_INFORMATION) + sizeof(SYSTEM_THREAD_INFORMATION) * ThreadsCount;
            ImageNameLength = 0;
            Status = SeLocateProcessImageName(Process, &TempProcessImageName);
            ProcessImageName = TempProcessImageName;
            szSrc = NULL;
            if (NT_SUCCESS(Status) && (ProcessImageName->Length > 0))
            {
              szSrc = (PWCHAR)((PCHAR)ProcessImageName->Buffer + ProcessImageName->Length);
              /* Loop the file name*/
              while (szSrc > ProcessImageName->Buffer)
              {
                /* Make sure this isn't a backslash */
                if (*--szSrc == OBJ_NAME_PATH_SEPARATOR)
                {
                    szSrc++;
                    break;
                }
                else
                {
                    ImageNameLength += sizeof(WCHAR);
                }
              }
            }
            if (!ImageNameLength && Process != PsIdleProcess)
            {
              ImageNameLength = (USHORT)strlen(Process->ImageFileName) * sizeof(WCHAR);
            }

            /* Round up the image name length as NT does */
            if (ImageNameLength > 0)
                ImageNameMaximumLength = ROUND_UP(ImageNameLength + sizeof(WCHAR), 8);
            else
                ImageNameMaximumLength = 0;

            TotalSize += CurrentSize + ImageNameMaximumLength;

            /* Check for overflow */
            if (TotalSize > Size)
            {
                Overflow = TRUE;
            }

            /* Fill system information */
            if (!Overflow)
            {
                SpiCurrent->NextEntryOffset = CurrentSize + ImageNameMaximumLength; // relative offset to the beginning of the next structure
                SpiCurrent->NumberOfThreads = ThreadsCount;
                SpiCurrent->CreateTime = Process->CreateTime;
                SpiCurrent->ImageName.Length = ImageNameLength;
                SpiCurrent->ImageName.MaximumLength = ImageNameMaximumLength;
                SpiCurrent->ImageName.Buffer = (void*)(Current + CurrentSize);

                /* Copy name to the end of the struct */
                if(Process != PsIdleProcess)
                {
                    if (szSrc)
                    {
                        RtlCopyMemory(SpiCurrent->ImageName.Buffer, szSrc, SpiCurrent->ImageName.Length);
                    }
                    else
                    {
                        RtlInitAnsiString(&ImageName, Process->ImageFileName);
                        Status = RtlAnsiStringToUnicodeString(&SpiCurrent->ImageName, &ImageName, FALSE);
                        if (!NT_SUCCESS(Status))
                        {
                            SpiCurrent->ImageName.Length = 0;
                        }
                    }
                }
                else
                {
                    RtlInitUnicodeString(&SpiCurrent->ImageName, NULL);
                }

                SpiCurrent->BasePriority = Process->Pcb.BasePriority;
                SpiCurrent->UniqueProcessId = Process->UniqueProcessId;
                SpiCurrent->InheritedFromUniqueProcessId = Process->InheritedFromUniqueProcessId;

                /* PsIdleProcess shares its handle table with PsInitialSystemProcess,
                 * so return the handle count for System only, not Idle one. */
                SpiCurrent->HandleCount = (Process == PsIdleProcess) ? 0 : ObGetProcessHandleCount(Process);

                SpiCurrent->PeakVirtualSize = Process->PeakVirtualSize;
                SpiCurrent->VirtualSize = Process->VirtualSize;
                SpiCurrent->PageFaultCount = Process->Vm.PageFaultCount;
                SpiCurrent->PeakWorkingSetSize = Process->Vm.PeakWorkingSetSize;
                SpiCurrent->WorkingSetSize = Process->Vm.WorkingSetSize;
                SpiCurrent->QuotaPeakPagedPoolUsage = Process->QuotaPeak[PsPagedPool];
                SpiCurrent->QuotaPagedPoolUsage = Process->QuotaUsage[PsPagedPool];
                SpiCurrent->QuotaPeakNonPagedPoolUsage = Process->QuotaPeak[PsNonPagedPool];
                SpiCurrent->QuotaNonPagedPoolUsage = Process->QuotaUsage[PsNonPagedPool];
                SpiCurrent->PagefileUsage = Process->QuotaUsage[PsPageFile];
                SpiCurrent->PeakPagefileUsage = Process->QuotaPeak[PsPageFile];
                SpiCurrent->PrivatePageCount = Process->CommitCharge;
                ThreadInfo = (PSYSTEM_THREAD_INFORMATION)(SpiCurrent + 1);

                CurrentEntry = Process->Pcb.ThreadListHead.Flink;
                while (CurrentEntry != &Process->Pcb.ThreadListHead)
                {
                    CurrentThread = CONTAINING_RECORD(CurrentEntry, ETHREAD, Tcb.ThreadListEntry);

                    ThreadInfo->KernelTime.QuadPart = UInt32x32To64(CurrentThread->Tcb.KernelTime, KeMaximumIncrement);
                    ThreadInfo->UserTime.QuadPart = UInt32x32To64(CurrentThread->Tcb.UserTime, KeMaximumIncrement);
                    ThreadInfo->CreateTime.QuadPart = CurrentThread->CreateTime.QuadPart;
                    ThreadInfo->WaitTime = CurrentThread->Tcb.WaitTime;
                    ThreadInfo->StartAddress = (PVOID) CurrentThread->StartAddress;
                    ThreadInfo->ClientId = CurrentThread->Cid;
                    ThreadInfo->Priority = CurrentThread->Tcb.Priority;
                    ThreadInfo->BasePriority = CurrentThread->Tcb.BasePriority;
                    ThreadInfo->ContextSwitches = CurrentThread->Tcb.ContextSwitches;
                    ThreadInfo->ThreadState = CurrentThread->Tcb.State;
                    ThreadInfo->WaitReason = CurrentThread->Tcb.WaitReason;

                    ThreadInfo++;
                    CurrentEntry = CurrentEntry->Flink;
                }

                /* Query total user/kernel times of a process */
                TotalKernel = KeQueryRuntimeProcess(&Process->Pcb, &TotalUser);
                SpiCurrent->UserTime.QuadPart = UInt32x32To64(TotalUser, KeMaximumIncrement);
                SpiCurrent->KernelTime.QuadPart = UInt32x32To64(TotalKernel, KeMaximumIncrement);
            }

            if (ProcessImageName)
            {
                /* Release the memory allocated by SeLocateProcessImageName */
                ExFreePoolWithTag(ProcessImageName, TAG_SEPA);
                ProcessImageName = NULL;
            }

            /* Unlock the Process */
            ExReleasePushLockShared(&Process->ProcessLock);
            KeLeaveCriticalRegion();

            /* Handle idle process entry */
Skip:
            if (Process == PsIdleProcess) Process = NULL;

            Process = PsGetNextProcess(Process);
            ThreadsCount = 0;
            if ((Process == SystemProcess) || (Process == NULL))
            {
                if (!Overflow)
                    SpiCurrent->NextEntryOffset = 0;
                break;
            }
            else
                Current += CurrentSize + ImageNameMaximumLength;
          }  while ((Process != SystemProcess) && (Process != NULL));

          if(Process != NULL)
            ObDereferenceObject(Process);
          Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        if(Process != NULL)
            ObDereferenceObject(Process);
        if (ProcessImageName)
        {
            /* Release the memory allocated by SeLocateProcessImageName */
            ExFreePoolWithTag(ProcessImageName, TAG_SEPA);
        }

        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END

    if (Overflow)
        Status = STATUS_INFO_LENGTH_MISMATCH;

    *ReqSize = TotalSize;
    return Status;
}

/* Class 6 - Call Count Information */
QSI_DEF(SystemCallCountInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemCallCountInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 7 - Device Information */
QSI_DEF(SystemDeviceInformation)
{
    PSYSTEM_DEVICE_INFORMATION Sdi
        = (PSYSTEM_DEVICE_INFORMATION) Buffer;
    PCONFIGURATION_INFORMATION ConfigInfo;

    *ReqSize = sizeof(SYSTEM_DEVICE_INFORMATION);

    /* Check user buffer's size */
    if (Size < sizeof(SYSTEM_DEVICE_INFORMATION))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    ConfigInfo = IoGetConfigurationInformation();

    Sdi->NumberOfDisks = ConfigInfo->DiskCount;
    Sdi->NumberOfFloppies = ConfigInfo->FloppyCount;
    Sdi->NumberOfCdRoms = ConfigInfo->CdRomCount;
    Sdi->NumberOfTapes = ConfigInfo->TapeCount;
    Sdi->NumberOfSerialPorts = ConfigInfo->SerialCount;
    Sdi->NumberOfParallelPorts = ConfigInfo->ParallelCount;

    return STATUS_SUCCESS;
}

/* Class 8 - Processor Performance Information */
QSI_DEF(SystemProcessorPerformanceInformation)
{
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION Spi
        = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION) Buffer;

    LONG i;
    ULONG TotalTime;
    PKPRCB Prcb;

    *ReqSize = KeNumberProcessors * sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION);

    /* Check user buffer's size */
    if (Size < *ReqSize)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    for (i = 0; i < KeNumberProcessors; i++)
    {
        /* Get the PRCB on this processor */
        Prcb = KiProcessorBlock[i];

        /* Calculate total user and kernel times */
        TotalTime = Prcb->IdleThread->KernelTime + Prcb->IdleThread->UserTime;
        Spi->IdleTime.QuadPart = UInt32x32To64(TotalTime, KeMaximumIncrement);
        Spi->KernelTime.QuadPart =  UInt32x32To64(Prcb->KernelTime, KeMaximumIncrement);
        Spi->UserTime.QuadPart = UInt32x32To64(Prcb->UserTime, KeMaximumIncrement);
        Spi->DpcTime.QuadPart = UInt32x32To64(Prcb->DpcTime, KeMaximumIncrement);
        Spi->InterruptTime.QuadPart = UInt32x32To64(Prcb->InterruptTime, KeMaximumIncrement);
        Spi->InterruptCount = Prcb->InterruptCount;
        Spi++;
    }

    return STATUS_SUCCESS;
}

/* Class 9 - Flags Information */
QSI_DEF(SystemFlagsInformation)
{
#if (NTDDI_VERSION >= NTDDI_VISTA)
    *ReqSize = sizeof(SYSTEM_FLAGS_INFORMATION);
#endif

    if (sizeof(SYSTEM_FLAGS_INFORMATION) != Size)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    ((PSYSTEM_FLAGS_INFORMATION) Buffer)->Flags = NtGlobalFlag;
#if (NTDDI_VERSION < NTDDI_VISTA)
    *ReqSize = sizeof(SYSTEM_FLAGS_INFORMATION);
#endif

    return STATUS_SUCCESS;
}

SSI_DEF(SystemFlagsInformation)
{
    if (sizeof(SYSTEM_FLAGS_INFORMATION) != Size)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    if (!SeSinglePrivilegeCheck(SeDebugPrivilege, ExGetPreviousMode()))
    {
#if (NTDDI_VERSION < NTDDI_WIN7)
        return STATUS_ACCESS_VIOLATION;
#else
        return STATUS_ACCESS_DENIED;
#endif
    }

    NtGlobalFlag = ((PSYSTEM_FLAGS_INFORMATION) Buffer)->Flags;
    return STATUS_SUCCESS;
}

/* Class 10 - Call Time Information */
QSI_DEF(SystemCallTimeInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemCallTimeInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 11 - Module Information */
QSI_DEF(SystemModuleInformation)
{
    NTSTATUS Status;

    /* Acquire system module list lock */
    KeEnterCriticalRegion();
    ExAcquireResourceExclusiveLite(&PsLoadedModuleResource, TRUE);

    /* Call the generic handler with the system module list */
    Status = ExpQueryModuleInformation(&PsLoadedModuleList,
                                       &MmLoadedUserImageList,
                                       (PRTL_PROCESS_MODULES)Buffer,
                                       Size,
                                       ReqSize);

    /* Release list lock and return status */
    ExReleaseResourceLite(&PsLoadedModuleResource);
    KeLeaveCriticalRegion();
    return Status;
}

/* Class 12 - Locks Information */
QSI_DEF(SystemLocksInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemLocksInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 13 - Stack Trace Information */
QSI_DEF(SystemStackTraceInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemStackTraceInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 14 - Paged Pool Information */
QSI_DEF(SystemPagedPoolInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemPagedPoolInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 15 - Non Paged Pool Information */
QSI_DEF(SystemNonPagedPoolInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemNonPagedPoolInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 16 - Handle Information */
QSI_DEF(SystemHandleInformation)
{
    PSYSTEM_HANDLE_INFORMATION HandleInformation;
    PLIST_ENTRY NextTableEntry;
    PHANDLE_TABLE HandleTable;
    PHANDLE_TABLE_ENTRY HandleTableEntry;
    EXHANDLE Handle;
    ULONG Index = 0;
    NTSTATUS Status;
    PMDL Mdl;
    PAGED_CODE();

    DPRINT("NtQuerySystemInformation - SystemHandleInformation\n");

    /* Set initial required buffer size */
    *ReqSize = FIELD_OFFSET(SYSTEM_HANDLE_INFORMATION, Handles);

    /* Check user's buffer size */
    if (Size < *ReqSize)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* We need to lock down the memory */
    Status = ExLockUserBuffer(Buffer,
                              Size,
                              ExGetPreviousMode(),
                              IoWriteAccess,
                              (PVOID*)&HandleInformation,
                              &Mdl);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to lock the user buffer: 0x%lx\n", Status);
        return Status;
    }

    /* Reset of count of handles */
    HandleInformation->NumberOfHandles = 0;

    /* Enter a critical region */
    KeEnterCriticalRegion();

    /* Acquire the handle table lock */
    ExAcquirePushLockShared(&HandleTableListLock);

    /* Enumerate all system handles */
    for (NextTableEntry = HandleTableListHead.Flink;
         NextTableEntry != &HandleTableListHead;
         NextTableEntry = NextTableEntry->Flink)
    {
        /* Get current handle table */
        HandleTable = CONTAINING_RECORD(NextTableEntry, HANDLE_TABLE, HandleTableList);

        /* Set the initial value and loop the entries */
        Handle.Value = 0;
        while ((HandleTableEntry = ExpLookupHandleTableEntry(HandleTable, Handle)))
        {
            /* Validate the entry */
            if ((HandleTableEntry->Object) &&
                (HandleTableEntry->NextFreeTableEntry != -2))
            {
                /* Increase of count of handles */
                ++HandleInformation->NumberOfHandles;

                /* Lock the entry */
                if (ExpLockHandleTableEntry(HandleTable, HandleTableEntry))
                {
                    /* Increase required buffer size */
                    *ReqSize += sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO);

                    /* Check user's buffer size */
                    if (*ReqSize > Size)
                    {
                        Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    else
                    {
                        POBJECT_HEADER ObjectHeader = ObpGetHandleObject(HandleTableEntry);

                        /* Filling handle information */
                        HandleInformation->Handles[Index].UniqueProcessId =
                            (USHORT)(ULONG_PTR) HandleTable->UniqueProcessId;

                        HandleInformation->Handles[Index].CreatorBackTraceIndex = 0;

#if 0 /* FIXME!!! Type field corrupted */
                        HandleInformation->Handles[Index].ObjectTypeIndex =
                            (UCHAR) ObjectHeader->Type->Index;
#else
                        HandleInformation->Handles[Index].ObjectTypeIndex = 0;
#endif

                        HandleInformation->Handles[Index].HandleAttributes =
                            HandleTableEntry->ObAttributes & OBJ_HANDLE_ATTRIBUTES;

                        HandleInformation->Handles[Index].HandleValue =
                            (USHORT)(ULONG_PTR) Handle.GenericHandleOverlay;

                        HandleInformation->Handles[Index].Object = &ObjectHeader->Body;

                        HandleInformation->Handles[Index].GrantedAccess =
                            HandleTableEntry->GrantedAccess;

                        ++Index;
                    }

                    /* Unlock it */
                    ExUnlockHandleTableEntry(HandleTable, HandleTableEntry);
                }
            }

            /* Go to the next entry */
            Handle.Value += sizeof(HANDLE);
        }
    }

    /* Release the lock */
    ExReleasePushLockShared(&HandleTableListLock);

    /* Leave the critical region */
    KeLeaveCriticalRegion();

    /* Release the locked user buffer */
    ExUnlockUserBuffer(Mdl);

    return Status;
}

/* Class 17 -  Information */
QSI_DEF(SystemObjectInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemObjectInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 18 -  Information */
QSI_DEF(SystemPageFileInformation)
{
    UNICODE_STRING FileName; /* FIXME */
    SYSTEM_PAGEFILE_INFORMATION *Spfi = (SYSTEM_PAGEFILE_INFORMATION *) Buffer;

    if (Size < sizeof(SYSTEM_PAGEFILE_INFORMATION))
    {
        * ReqSize = sizeof(SYSTEM_PAGEFILE_INFORMATION);
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    RtlInitUnicodeString(&FileName, NULL); /* FIXME */

    /* FIXME */
    Spfi->NextEntryOffset = 0;

    Spfi->TotalSize = MiFreeSwapPages + MiUsedSwapPages;
    Spfi->TotalInUse = MiUsedSwapPages;
    Spfi->PeakUsage = MiUsedSwapPages; /* FIXME */
    Spfi->PageFileName = FileName;
    return STATUS_SUCCESS;
}

/* Class 19 - Vdm Instemul Information */
QSI_DEF(SystemVdmInstemulInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemVdmInstemulInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 20 - Vdm Bop Information */
QSI_DEF(SystemVdmBopInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemVdmBopInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 21 - File Cache Information */
QSI_DEF(SystemFileCacheInformation)
{
    SYSTEM_FILECACHE_INFORMATION *Sci = (SYSTEM_FILECACHE_INFORMATION *) Buffer;

    *ReqSize = sizeof(SYSTEM_FILECACHE_INFORMATION);

    if (Size < *ReqSize)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    RtlZeroMemory(Sci, sizeof(SYSTEM_FILECACHE_INFORMATION));

    /* Return the Byte size not the page size. */
    Sci->CurrentSize = MiMemoryConsumers[MC_USER].PagesUsed; /* FIXME */
    Sci->PeakSize = MiMemoryConsumers[MC_USER].PagesUsed; /* FIXME */
    /* Taskmgr multiplies this one by page size right away */
    Sci->CurrentSizeIncludingTransitionInPages = MiMemoryConsumers[MC_USER].PagesUsed; /* FIXME: Should be */
    /* system working set and standby pages. */
    Sci->PageFaultCount = 0; /* FIXME */
    Sci->MinimumWorkingSet = 0; /* FIXME */
    Sci->MaximumWorkingSet = 0; /* FIXME */

    return STATUS_SUCCESS;
}

SSI_DEF(SystemFileCacheInformation)
{
    if (Size < sizeof(SYSTEM_FILECACHE_INFORMATION))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }
    /* FIXME */
    DPRINT1("NtSetSystemInformation - SystemFileCacheInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 22 - Pool Tag Information */
QSI_DEF(SystemPoolTagInformation)
{
    if (Size < sizeof(SYSTEM_POOLTAG_INFORMATION)) return STATUS_INFO_LENGTH_MISMATCH;
    return ExGetPoolTagInfo(Buffer, Size, ReqSize);
}

/* Class 23 - Interrupt Information for all processors */
QSI_DEF(SystemInterruptInformation)
{
    PKPRCB Prcb;
    LONG i;
    ULONG ti;
    PSYSTEM_INTERRUPT_INFORMATION sii = (PSYSTEM_INTERRUPT_INFORMATION)Buffer;

    if(Size < KeNumberProcessors * sizeof(SYSTEM_INTERRUPT_INFORMATION))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    ti = KeQueryTimeIncrement();

    for (i = 0; i < KeNumberProcessors; i++)
    {
        Prcb = KiProcessorBlock[i];
        sii->ContextSwitches = KeGetContextSwitches(Prcb);
        sii->DpcCount = Prcb->DpcData[0].DpcCount;
        sii->DpcRate = Prcb->DpcRequestRate;
        sii->TimeIncrement = ti;
        sii->DpcBypassCount = 0;
        sii->ApcBypassCount = 0;
        sii++;
    }

    return STATUS_SUCCESS;
}

/* Class 24 - DPC Behaviour Information */
QSI_DEF(SystemDpcBehaviourInformation)
{
    PSYSTEM_DPC_BEHAVIOR_INFORMATION sdbi = (PSYSTEM_DPC_BEHAVIOR_INFORMATION)Buffer;

    if (Size < sizeof(SYSTEM_DPC_BEHAVIOR_INFORMATION))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    sdbi->DpcQueueDepth = KiMaximumDpcQueueDepth;
    sdbi->MinimumDpcRate = KiMinimumDpcRate;
    sdbi->AdjustDpcThreshold = KiAdjustDpcThreshold;
    sdbi->IdealDpcRate = KiIdealDpcRate;

    return STATUS_SUCCESS;
}

SSI_DEF(SystemDpcBehaviourInformation)
{
    /* FIXME */
    DPRINT1("NtSetSystemInformation - SystemDpcBehaviourInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 25 - Full Memory Information */
QSI_DEF(SystemFullMemoryInformation)
{
    PULONG Spi = (PULONG) Buffer;

    PEPROCESS TheIdleProcess;

    *ReqSize = sizeof(ULONG);

    if (sizeof(ULONG) != Size)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    DPRINT("SystemFullMemoryInformation\n");

    TheIdleProcess = PsIdleProcess;

    DPRINT("PID: %p, KernelTime: %u PFFree: %lu PFUsed: %lu\n",
           TheIdleProcess->UniqueProcessId,
           TheIdleProcess->Pcb.KernelTime,
           MiFreeSwapPages,
           MiUsedSwapPages);

    *Spi = MiMemoryConsumers[MC_USER].PagesUsed;

    return STATUS_SUCCESS;
}

/* Class 26 - Load Image */
SSI_DEF(SystemLoadGdiDriverInformation)
{
    PSYSTEM_GDI_DRIVER_INFORMATION DriverInfo = (PVOID)Buffer;
    UNICODE_STRING ImageName;
    PVOID ImageBase;
    PVOID SectionPointer;
    ULONG_PTR EntryPoint;
    NTSTATUS Status;
    ULONG DirSize;
    PIMAGE_NT_HEADERS NtHeader;

    /* Validate size */
    if (Size != sizeof(SYSTEM_GDI_DRIVER_INFORMATION))
    {
        /* Incorrect buffer length, fail */
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Only kernel mode can call this function */
    if (ExGetPreviousMode() != KernelMode) return STATUS_PRIVILEGE_NOT_HELD;

    /* Load the driver */
    ImageName = DriverInfo->DriverName;
    Status = MmLoadSystemImage(&ImageName,
                               NULL,
                               NULL,
                               0,
                               &SectionPointer,
                               &ImageBase);
    if (!NT_SUCCESS(Status)) return Status;

    /* Return the export pointer */
    DriverInfo->ExportSectionPointer =
        RtlImageDirectoryEntryToData(ImageBase,
                                     TRUE,
                                     IMAGE_DIRECTORY_ENTRY_EXPORT,
                                     &DirSize);

    /* Get the entrypoint */
    NtHeader = RtlImageNtHeader(ImageBase);
    EntryPoint = NtHeader->OptionalHeader.AddressOfEntryPoint;
    EntryPoint += (ULONG_PTR)ImageBase;

    /* Save other data */
    DriverInfo->ImageAddress = ImageBase;
    DriverInfo->SectionPointer = SectionPointer;
    DriverInfo->EntryPoint = (PVOID)EntryPoint;
    DriverInfo->ImageLength = NtHeader->OptionalHeader.SizeOfImage;

    /* All is good */
    return STATUS_SUCCESS;
}

/* Class 27 - Unload Image */
SSI_DEF(SystemUnloadGdiDriverInformation)
{
    PVOID *SectionPointer = Buffer;

    /* Validate size */
    if (Size != sizeof(PVOID))
    {
        /* Incorrect length, fail */
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Only kernel mode can call this function */
    if (ExGetPreviousMode() != KernelMode) return STATUS_PRIVILEGE_NOT_HELD;

    /* Unload the image */
    MmUnloadSystemImage(*SectionPointer);
    return STATUS_SUCCESS;
}

/* Class 28 - Time Adjustment Information */
QSI_DEF(SystemTimeAdjustmentInformation)
{
    PSYSTEM_QUERY_TIME_ADJUST_INFORMATION TimeInfo =
        (PSYSTEM_QUERY_TIME_ADJUST_INFORMATION)Buffer;

    /* Check if enough storage was provided */
    if (sizeof(SYSTEM_QUERY_TIME_ADJUST_INFORMATION) > Size)
    {
        * ReqSize = sizeof(SYSTEM_QUERY_TIME_ADJUST_INFORMATION);
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Give time values to our caller */
    TimeInfo->TimeIncrement = KeMaximumIncrement;
    TimeInfo->TimeAdjustment = KeTimeAdjustment;
    TimeInfo->Enable = !KiTimeAdjustmentEnabled;

    return STATUS_SUCCESS;
}

SSI_DEF(SystemTimeAdjustmentInformation)
{
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PSYSTEM_SET_TIME_ADJUST_INFORMATION TimeInfo =
        (PSYSTEM_SET_TIME_ADJUST_INFORMATION)Buffer;

    /* Check size of a buffer, it must match our expectations */
    if (sizeof(SYSTEM_SET_TIME_ADJUST_INFORMATION) != Size)
        return STATUS_INFO_LENGTH_MISMATCH;

    /* Check who is calling */
    if (PreviousMode != KernelMode)
    {
        /* Check access rights */
        if (!SeSinglePrivilegeCheck(SeSystemtimePrivilege, PreviousMode))
        {
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    /* FIXME: behaviour suggests the member be named 'Disable' */
    if (TimeInfo->Enable)
    {
        /* Disable time adjustment and set default value */
        KiTimeAdjustmentEnabled = FALSE;
        KeTimeAdjustment = KeMaximumIncrement;
    }
    else
    {
        /* Check if a valid time adjustment value is given */
        if (TimeInfo->TimeAdjustment == 0) return STATUS_INVALID_PARAMETER_2;

        /* Enable time adjustment and set the adjustment value */
        KiTimeAdjustmentEnabled = TRUE;
        KeTimeAdjustment = TimeInfo->TimeAdjustment;
    }

    return STATUS_SUCCESS;
}

/* Class 29 - Summary Memory Information */
QSI_DEF(SystemSummaryMemoryInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemSummaryMemoryInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 30 - Next Event Id Information */
QSI_DEF(SystemNextEventIdInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemNextEventIdInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 31 */
QSI_DEF(SystemPerformanceTraceInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemPerformanceTraceInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 32 - Crash Dump Information */
QSI_DEF(SystemCrashDumpInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemCrashDumpInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 33 - Exception Information */
QSI_DEF(SystemExceptionInformation)
{
    PSYSTEM_EXCEPTION_INFORMATION ExceptionInformation =
        (PSYSTEM_EXCEPTION_INFORMATION)Buffer;
    PKPRCB Prcb;
    ULONG AlignmentFixupCount = 0, ExceptionDispatchCount = 0;
    ULONG FloatingEmulationCount = 0, ByteWordEmulationCount = 0;
    CHAR i;

    /* Check size of a buffer, it must match our expectations */
    if (sizeof(SYSTEM_EXCEPTION_INFORMATION) != Size)
        return STATUS_INFO_LENGTH_MISMATCH;

    /* Sum up exception count information from all processors */
    for (i = 0; i < KeNumberProcessors; i++)
    {
        Prcb = KiProcessorBlock[i];
        if (Prcb)
        {
            AlignmentFixupCount += Prcb->KeAlignmentFixupCount;
            ExceptionDispatchCount += Prcb->KeExceptionDispatchCount;
#ifndef _M_ARM
            FloatingEmulationCount += Prcb->KeFloatingEmulationCount;
#endif // _M_ARM
        }
    }

    /* Save information in user's buffer */
    ExceptionInformation->AlignmentFixupCount = AlignmentFixupCount;
    ExceptionInformation->ExceptionDispatchCount = ExceptionDispatchCount;
    ExceptionInformation->FloatingEmulationCount = FloatingEmulationCount;
    ExceptionInformation->ByteWordEmulationCount = ByteWordEmulationCount;

    return STATUS_SUCCESS;
}

/* Class 34 - Crash Dump State Information */
QSI_DEF(SystemCrashDumpStateInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemCrashDumpStateInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 35 - Kernel Debugger Information */
QSI_DEF(SystemKernelDebuggerInformation)
{
    PSYSTEM_KERNEL_DEBUGGER_INFORMATION skdi = (PSYSTEM_KERNEL_DEBUGGER_INFORMATION) Buffer;

#if (NTDDI_VERSION >= NTDDI_VISTA)
    *ReqSize = sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION);
#endif

    if (Size < sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    skdi->KernelDebuggerEnabled = KD_DEBUGGER_ENABLED;
    skdi->KernelDebuggerNotPresent = KD_DEBUGGER_NOT_PRESENT;

#if (NTDDI_VERSION < NTDDI_VISTA)
    *ReqSize = sizeof(SYSTEM_KERNEL_DEBUGGER_INFORMATION);
#endif

    return STATUS_SUCCESS;
}

/* Class 36 - Context Switch Information */
QSI_DEF(SystemContextSwitchInformation)
{
    PSYSTEM_CONTEXT_SWITCH_INFORMATION ContextSwitchInformation =
        (PSYSTEM_CONTEXT_SWITCH_INFORMATION)Buffer;
    ULONG ContextSwitches;
    PKPRCB Prcb;
    CHAR i;

    /* Check size of a buffer, it must match our expectations */
    if (sizeof(SYSTEM_CONTEXT_SWITCH_INFORMATION) != Size)
        return STATUS_INFO_LENGTH_MISMATCH;

    /* Calculate total value of context switches across all processors */
    ContextSwitches = 0;
    for (i = 0; i < KeNumberProcessors; i ++)
    {
        Prcb = KiProcessorBlock[i];
        if (Prcb)
        {
            ContextSwitches += KeGetContextSwitches(Prcb);
        }
    }

    ContextSwitchInformation->ContextSwitches = ContextSwitches;

    /* FIXME */
    ContextSwitchInformation->FindAny = 0;
    ContextSwitchInformation->FindLast = 0;
    ContextSwitchInformation->FindIdeal = 0;
    ContextSwitchInformation->IdleAny = 0;
    ContextSwitchInformation->IdleCurrent = 0;
    ContextSwitchInformation->IdleLast = 0;
    ContextSwitchInformation->IdleIdeal = 0;
    ContextSwitchInformation->PreemptAny = 0;
    ContextSwitchInformation->PreemptCurrent = 0;
    ContextSwitchInformation->PreemptLast = 0;
    ContextSwitchInformation->SwitchToIdle = 0;

    return STATUS_SUCCESS;
}

/* Class 37 - Registry Quota Information */
QSI_DEF(SystemRegistryQuotaInformation)
{
    PSYSTEM_REGISTRY_QUOTA_INFORMATION srqi = (PSYSTEM_REGISTRY_QUOTA_INFORMATION) Buffer;

    *ReqSize = sizeof(SYSTEM_REGISTRY_QUOTA_INFORMATION);
    if (Size < sizeof(SYSTEM_REGISTRY_QUOTA_INFORMATION))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    DPRINT1("Faking max registry size of 32 MB\n");
    srqi->RegistryQuotaAllowed = 0x2000000;
    srqi->RegistryQuotaUsed = 0x200000;
    srqi->PagedPoolSize = 0x200000;

    return STATUS_SUCCESS;
}

SSI_DEF(SystemRegistryQuotaInformation)
{
    /* FIXME */
    DPRINT1("NtSetSystemInformation - SystemRegistryQuotaInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 38 - Load And Call Image */
SSI_DEF(SystemExtendServiceTableInformation)
{
    UNICODE_STRING ImageName;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    PLDR_DATA_TABLE_ENTRY ModuleObject;
    NTSTATUS Status;
    PIMAGE_NT_HEADERS NtHeader;
    DRIVER_OBJECT Win32k;
    PDRIVER_INITIALIZE DriverInit;
    PVOID ImageBase;
    ULONG_PTR EntryPoint;

    /* Validate the size */
    if (Size != sizeof(UNICODE_STRING)) return STATUS_INFO_LENGTH_MISMATCH;

    /* Check who is calling */
    if (PreviousMode != KernelMode)
    {
        static const UNICODE_STRING Win32kName =
            RTL_CONSTANT_STRING(L"\\SystemRoot\\System32\\win32k.sys");

        /* Make sure we can load drivers */
        if (!SeSinglePrivilegeCheck(SeLoadDriverPrivilege, UserMode))
        {
            /* FIXME: We can't, fail */
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        _SEH2_TRY
        {
            /* Probe and copy the unicode string */
            ProbeForRead(Buffer, sizeof(ImageName), 1);
            ImageName = *(PUNICODE_STRING)Buffer;

            /* Probe the string buffer */
            ProbeForRead(ImageName.Buffer, ImageName.Length, sizeof(WCHAR));

            /* Check if we have the correct name (nothing else is allowed!) */
            if (!RtlEqualUnicodeString(&ImageName, &Win32kName, FALSE))
            {
                _SEH2_YIELD(return STATUS_PRIVILEGE_NOT_HELD);
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;

        /* Recursively call the function, so that we are from kernel mode */
        return ZwSetSystemInformation(SystemExtendServiceTableInformation,
                                      (PVOID)&Win32kName,
                                      sizeof(Win32kName));
    }

    /* Load the image */
    Status = MmLoadSystemImage((PUNICODE_STRING)Buffer,
                               NULL,
                               NULL,
                               0,
                               (PVOID)&ModuleObject,
                               &ImageBase);

    if (!NT_SUCCESS(Status)) return Status;

    /* Get the headers */
    NtHeader = RtlImageNtHeader(ImageBase);
    if (!NtHeader)
    {
        /* Fail */
        MmUnloadSystemImage(ModuleObject);
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    /* Get the entrypoint */
    EntryPoint = NtHeader->OptionalHeader.AddressOfEntryPoint;
    EntryPoint += (ULONG_PTR)ImageBase;
    DriverInit = (PDRIVER_INITIALIZE)EntryPoint;

    /* Create a dummy device */
    RtlZeroMemory(&Win32k, sizeof(Win32k));
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);
    Win32k.DriverStart = ImageBase;

    /* Call it */
    Status = (DriverInit)(&Win32k, NULL);
    ASSERT(KeGetCurrentIrql() == PASSIVE_LEVEL);

    /* Unload if we failed */
    if (!NT_SUCCESS(Status)) MmUnloadSystemImage(ModuleObject);
    return Status;
}

/* Class 39 - Priority Separation */
SSI_DEF(SystemPrioritySeperation)
{
    /* Check if the size is correct */
    if (Size != sizeof(ULONG))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* We need the TCB privilege */
    if (!SeSinglePrivilegeCheck(SeTcbPrivilege, ExGetPreviousMode()))
    {
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    /* Modify the quantum table */
    PsChangeQuantumTable(TRUE, *(PULONG)Buffer);

    return STATUS_SUCCESS;
}

/* Class 40 */
QSI_DEF(SystemVerifierAddDriverInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemVerifierAddDriverInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 41 */
QSI_DEF(SystemVerifierRemoveDriverInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemVerifierRemoveDriverInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 42 - Power Information */
QSI_DEF(SystemProcessorIdleInformation)
{
    *ReqSize = sizeof(PROCESSOR_POWER_INFORMATION) * KeNumberProcessors;

    if (sizeof(PROCESSOR_POWER_INFORMATION) * KeNumberProcessors > Size)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemPowerInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 43 */
QSI_DEF(SystemLegacyDriverInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemLegacyDriverInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 44 - Current Time Zone Information */
QSI_DEF(SystemCurrentTimeZoneInformation)
{
    *ReqSize = sizeof(RTL_TIME_ZONE_INFORMATION);

    if (sizeof(RTL_TIME_ZONE_INFORMATION) != Size)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Copy the time zone information struct */
    memcpy(Buffer,
           &ExpTimeZoneInfo,
           sizeof(RTL_TIME_ZONE_INFORMATION));

    return STATUS_SUCCESS;
}

SSI_DEF(SystemCurrentTimeZoneInformation)
{
    /* Check user buffer's size */
    if (Size < sizeof(RTL_TIME_ZONE_INFORMATION))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    return ExpSetTimeZoneInformation((PRTL_TIME_ZONE_INFORMATION)Buffer);
}

static
VOID
ExpCopyLookasideInformation(
    PSYSTEM_LOOKASIDE_INFORMATION *InfoPointer,
    PULONG RemainingPointer,
    PLIST_ENTRY ListHead,
    BOOLEAN ListUsesMisses)

{
    PSYSTEM_LOOKASIDE_INFORMATION Info;
    PGENERAL_LOOKASIDE LookasideList;
    PLIST_ENTRY ListEntry;
    ULONG Remaining;

    /* Get info pointer and remaining count of free array element */
    Info = *InfoPointer;
    Remaining = *RemainingPointer;

    /* Loop as long as we have lookaside lists and free array elements */
    for (ListEntry = ListHead->Flink;
         (ListEntry != ListHead) && (Remaining > 0);
         ListEntry = ListEntry->Flink, Remaining--)
    {
        LookasideList = CONTAINING_RECORD(ListEntry, GENERAL_LOOKASIDE, ListEntry);

        /* Fill the next array element */
        Info->CurrentDepth = LookasideList->Depth;
        Info->MaximumDepth = LookasideList->MaximumDepth;
        Info->TotalAllocates = LookasideList->TotalAllocates;
        Info->TotalFrees = LookasideList->TotalFrees;
        Info->Type = LookasideList->Type;
        Info->Tag = LookasideList->Tag;
        Info->Size = LookasideList->Size;

        /* Check how the lists track misses/hits */
        if (ListUsesMisses)
        {
            /* Copy misses */
            Info->AllocateMisses = LookasideList->AllocateMisses;
            Info->FreeMisses = LookasideList->FreeMisses;
        }
        else
        {
            /* Calculate misses */
            Info->AllocateMisses = LookasideList->TotalAllocates
                                   - LookasideList->AllocateHits;
            Info->FreeMisses = LookasideList->TotalFrees
                               - LookasideList->FreeHits;
        }
    }

    /* Return the updated pointer and remaining count */
    *InfoPointer = Info;
    *RemainingPointer = Remaining;
}

/* Class 45 - Lookaside Information */
QSI_DEF(SystemLookasideInformation)
{
    KPROCESSOR_MODE PreviousMode;
    PSYSTEM_LOOKASIDE_INFORMATION Info;
    PMDL Mdl;
    ULONG MaxCount, Remaining;
    KIRQL OldIrql;
    NTSTATUS Status;

    /* First we need to lock down the memory, since we are going to access it
       at high IRQL */
    PreviousMode = ExGetPreviousMode();
    Status = ExLockUserBuffer(Buffer,
                              Size,
                              PreviousMode,
                              IoWriteAccess,
                              (PVOID*)&Info,
                              &Mdl);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to lock the user buffer: 0x%lx\n", Status);
        return Status;
    }

    /* Calculate how many items we can store */
    Remaining = MaxCount = Size / sizeof(SYSTEM_LOOKASIDE_INFORMATION);
    if (Remaining == 0)
    {
        goto Leave;
    }

    /* Copy info from pool lookaside lists */
    ExpCopyLookasideInformation(&Info,
                                &Remaining,
                                &ExPoolLookasideListHead,
                                FALSE);
    if (Remaining == 0)
    {
        goto Leave;
    }

    /* Copy info from system lookaside lists */
    ExpCopyLookasideInformation(&Info,
                                &Remaining,
                                &ExSystemLookasideListHead,
                                TRUE);
    if (Remaining == 0)
    {
        goto Leave;
    }

    /* Acquire spinlock for ExpNonPagedLookasideListHead */
    KeAcquireSpinLock(&ExpNonPagedLookasideListLock, &OldIrql);

    /* Copy info from non-paged lookaside lists */
    ExpCopyLookasideInformation(&Info,
                                &Remaining,
                                &ExpNonPagedLookasideListHead,
                                TRUE);

    /* Release spinlock for ExpNonPagedLookasideListHead */
    KeReleaseSpinLock(&ExpNonPagedLookasideListLock, OldIrql);

    if (Remaining == 0)
    {
        goto Leave;
    }

    /* Acquire spinlock for ExpPagedLookasideListHead */
    KeAcquireSpinLock(&ExpPagedLookasideListLock, &OldIrql);

    /* Copy info from paged lookaside lists */
    ExpCopyLookasideInformation(&Info,
                                &Remaining,
                                &ExpPagedLookasideListHead,
                                TRUE);

    /* Release spinlock for ExpPagedLookasideListHead */
    KeReleaseSpinLock(&ExpPagedLookasideListLock, OldIrql);

Leave:

    /* Release the locked user buffer */
    ExUnlockUserBuffer(Mdl);

    /* Return the size of the actually written data */
    *ReqSize = (MaxCount - Remaining) * sizeof(SYSTEM_LOOKASIDE_INFORMATION);
    return STATUS_SUCCESS;
}

/* Class 46 - Set time slip event */
SSI_DEF(SystemTimeSlipNotification)
{
    /* FIXME */
    DPRINT1("NtSetSystemInformation - SystemTimeSlipNotification not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS
NTAPI
MmSessionCreate(OUT PULONG SessionId);

NTSTATUS
NTAPI
MmSessionDelete(IN ULONG SessionId);

/* Class 47 - Create a new session (TSE) */
SSI_DEF(SystemSessionCreate)
{
    ULONG SessionId;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();
    NTSTATUS Status;

    if (Size != sizeof(ULONG)) return STATUS_INFO_LENGTH_MISMATCH;

    if (PreviousMode != KernelMode)
    {
        if (!SeSinglePrivilegeCheck(SeLoadDriverPrivilege, PreviousMode))
        {
            return STATUS_PRIVILEGE_NOT_HELD;
        }

        ProbeForWriteUlong(Buffer);
    }

    Status = MmSessionCreate(&SessionId);
    if (NT_SUCCESS(Status)) *(PULONG)Buffer = SessionId;

    return Status;
}

/* Class 48 - Delete an existing session (TSE) */
SSI_DEF(SystemSessionDetach)
{
    ULONG SessionId;
    KPROCESSOR_MODE PreviousMode = KeGetPreviousMode();

    if (Size != sizeof(ULONG)) return STATUS_INFO_LENGTH_MISMATCH;

    if (PreviousMode != KernelMode)
    {
        if (!SeSinglePrivilegeCheck(SeLoadDriverPrivilege, PreviousMode))
        {
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    SessionId = *(PULONG)Buffer;

    return MmSessionDelete(SessionId);
}

/* Class 49 - UNKNOWN */
QSI_DEF(SystemSessionInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemSessionInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 50 - System range start address */
QSI_DEF(SystemRangeStartInformation)
{
    /* Check user buffer's size */
    if (Size != sizeof(ULONG_PTR)) return STATUS_INFO_LENGTH_MISMATCH;

    *(PULONG_PTR)Buffer = (ULONG_PTR)MmSystemRangeStart;

    if (ReqSize) *ReqSize = sizeof(ULONG_PTR);

    return STATUS_SUCCESS;
}

/* Class 51 - Driver verifier information */
QSI_DEF(SystemVerifierInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemVerifierInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

SSI_DEF(SystemVerifierInformation)
{
    /* FIXME */
    DPRINT1("NtSetSystemInformation - SystemVerifierInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 52 */
SSI_DEF(SystemVerifierThunkExtend)
{
    /* FIXME */
    DPRINT1("NtSetSystemInformation - SystemVerifierThunkExtend not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 53 - A session's processes */
QSI_DEF(SystemSessionProcessesInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemSessionProcessInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 54 - Load & map in system space */
SSI_DEF(SystemLoadGdiDriverInSystemSpaceInformation)
{
    /* FIXME */
    DPRINT1("NtSetSystemInformation - SystemLoadGdiDriverInSystemSpaceInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 55 - NUMA processor information */
QSI_DEF(SystemNumaProcessorMap)
{
    ULONG MaxEntries, Node;
    PSYSTEM_NUMA_INFORMATION NumaInformation = (PSYSTEM_NUMA_INFORMATION)Buffer;

    /* Validate input size */
    if (Size < sizeof(ULONG))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Return highest node */
    NumaInformation->HighestNodeNumber = KeNumberNodes - 1;

    /* Compute how much entries we will be able to put in output structure */
    MaxEntries = (Size - FIELD_OFFSET(SYSTEM_NUMA_INFORMATION, ActiveProcessorsAffinityMask)) / sizeof(ULONGLONG);
    /* Make sure we don't overflow KeNodeBlock */
    if (MaxEntries > KeNumberNodes)
    {
        MaxEntries = KeNumberNodes;
    }

    /* If we have entries to write, and room for it */
    if (Size >= FIELD_OFFSET(SYSTEM_NUMA_INFORMATION, ActiveProcessorsAffinityMask) &&
        MaxEntries != 0)
    {
        /* Already set size we return */
        *ReqSize = FIELD_OFFSET(SYSTEM_NUMA_INFORMATION, ActiveProcessorsAffinityMask) +
                   MaxEntries * sizeof(ULONGLONG);

        /* For each node, return processor mask */
        for (Node = 0; Node < MaxEntries; ++Node)
        {
            NumaInformation->ActiveProcessorsAffinityMask[Node] = KeNodeBlock[Node]->ProcessorMask;
        }
    }
    else
    {
        /* We only returned highest node number */
        *ReqSize = sizeof(ULONG);
    }

    return STATUS_SUCCESS;
}

/* Class 56 - Prefetcher information */
QSI_DEF(SystemPrefetcherInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemPrefetcherInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 57 - Extended process information */
QSI_DEF(SystemExtendedProcessInformation)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemExtendedProcessInformation not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 58 - Recommended shared data alignment */
QSI_DEF(SystemRecommendedSharedDataAlignment)
{
    /* FIXME */
    DPRINT1("NtQuerySystemInformation - SystemRecommendedSharedDataAlignment not implemented\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* Class 60 - NUMA memory information */
QSI_DEF(SystemNumaAvailableMemory)
{
    ULONG MaxEntries, Node;
    PSYSTEM_NUMA_INFORMATION NumaInformation = (PSYSTEM_NUMA_INFORMATION)Buffer;

    /* Validate input size */
    if (Size < sizeof(ULONG))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* Return highest node */
    NumaInformation->HighestNodeNumber = KeNumberNodes - 1;

    /* Compute how much entries we will be able to put in output structure */
    MaxEntries = (Size - FIELD_OFFSET(SYSTEM_NUMA_INFORMATION, AvailableMemory)) / sizeof(ULONGLONG);
    /* Make sure we don't overflow KeNodeBlock */
    if (MaxEntries > KeNumberNodes)
    {
        MaxEntries = KeNumberNodes;
    }

    /* If we have entries to write, and room for it */
    if (Size >= FIELD_OFFSET(SYSTEM_NUMA_INFORMATION, AvailableMemory) &&
        MaxEntries != 0)
    {
        /* Already set size we return */
        *ReqSize = FIELD_OFFSET(SYSTEM_NUMA_INFORMATION, AvailableMemory) +
                   MaxEntries * sizeof(ULONGLONG);

        /* If we have a single entry (us), directly return MM information */
        if (MaxEntries == 1)
        {
            NumaInformation->AvailableMemory[0] = MmAvailablePages << PAGE_SHIFT;
        }
        else
        {
            /* Otherwise, for each node, return available bytes */
            for (Node = 0; Node < MaxEntries; ++Node)
            {
                NumaInformation->AvailableMemory[Node] = (KeNodeBlock[Node]->FreeCount[0] + KeNodeBlock[Node]->FreeCount[1]) << PAGE_SHIFT;
            }
        }
    }
    else
    {
        /* We only returned highest node number */
        *ReqSize = sizeof(ULONG);
    }

    return STATUS_SUCCESS;
}

/* Class 64 - Extended handle information */
QSI_DEF(SystemExtendedHandleInformation)
{
    PSYSTEM_HANDLE_INFORMATION_EX HandleInformation;
    PLIST_ENTRY NextTableEntry;
    PHANDLE_TABLE HandleTable;
    PHANDLE_TABLE_ENTRY HandleTableEntry;
    EXHANDLE Handle;
    ULONG Index = 0;
    NTSTATUS Status;
    PMDL Mdl;
    PAGED_CODE();

    DPRINT("NtQuerySystemInformation - SystemExtendedHandleInformation\n");

    /* Set initial required buffer size */
    *ReqSize = FIELD_OFFSET(SYSTEM_HANDLE_INFORMATION_EX, Handle);

    /* Check user's buffer size */
    if (Size < *ReqSize)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    /* We need to lock down the memory */
    Status = ExLockUserBuffer(Buffer,
                              Size,
                              ExGetPreviousMode(),
                              IoWriteAccess,
                              (PVOID*)&HandleInformation,
                              &Mdl);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to lock the user buffer: 0x%lx\n", Status);
        return Status;
    }

    /* Reset of count of handles */
    HandleInformation->Count = 0;

    /* Enter a critical region */
    KeEnterCriticalRegion();

    /* Acquire the handle table lock */
    ExAcquirePushLockShared(&HandleTableListLock);

    /* Enumerate all system handles */
    for (NextTableEntry = HandleTableListHead.Flink;
         NextTableEntry != &HandleTableListHead;
         NextTableEntry = NextTableEntry->Flink)
    {
        /* Get current handle table */
        HandleTable = CONTAINING_RECORD(NextTableEntry, HANDLE_TABLE, HandleTableList);

        /* Set the initial value and loop the entries */
        Handle.Value = 0;
        while ((HandleTableEntry = ExpLookupHandleTableEntry(HandleTable, Handle)))
        {
            /* Validate the entry */
            if ((HandleTableEntry->Object) &&
                (HandleTableEntry->NextFreeTableEntry != -2))
            {
                /* Increase of count of handles */
                ++HandleInformation->Count;

                /* Lock the entry */
                if (ExpLockHandleTableEntry(HandleTable, HandleTableEntry))
                {
                    /* Increase required buffer size */
                    *ReqSize += sizeof(SYSTEM_HANDLE_TABLE_ENTRY_INFO_EX);

                    /* Check user's buffer size */
                    if (*ReqSize > Size)
                    {
                        Status = STATUS_INFO_LENGTH_MISMATCH;
                    }
                    else
                    {
                        POBJECT_HEADER ObjectHeader = ObpGetHandleObject(HandleTableEntry);

                        /* Filling handle information */
                        HandleInformation->Handle[Index].UniqueProcessId =
                            (USHORT)(ULONG_PTR) HandleTable->UniqueProcessId;

                        HandleInformation->Handle[Index].CreatorBackTraceIndex = 0;

#if 0 /* FIXME!!! Type field corrupted */
                        HandleInformation->Handles[Index].ObjectTypeIndex =
                            (UCHAR) ObjectHeader->Type->Index;
#else
                        HandleInformation->Handle[Index].ObjectTypeIndex = 0;
#endif

                        HandleInformation->Handle[Index].HandleAttributes =
                            HandleTableEntry->ObAttributes & OBJ_HANDLE_ATTRIBUTES;

                        HandleInformation->Handle[Index].HandleValue =
                            (USHORT)(ULONG_PTR) Handle.GenericHandleOverlay;

                        HandleInformation->Handle[Index].Object = &ObjectHeader->Body;

                        HandleInformation->Handle[Index].GrantedAccess =
                            HandleTableEntry->GrantedAccess;

                        HandleInformation->Handle[Index].Reserved = 0;

                        ++Index;
                    }

                    /* Unlock it */
                    ExUnlockHandleTableEntry(HandleTable, HandleTableEntry);
                }
            }

            /* Go to the next entry */
            Handle.Value += sizeof(HANDLE);
        }
    }

    /* Release the lock */
    ExReleasePushLockShared(&HandleTableListLock);

    /* Leave the critical region */
    KeLeaveCriticalRegion();

    /* Release the locked user buffer */
    ExUnlockUserBuffer(Mdl);

    return Status;
}

/* Class 70 - System object security mode information */
QSI_DEF(SystemObjectSecurityMode)
{
    PULONG ObjectSecurityInfo = (PULONG)Buffer;

    /* Validate input size */
    if (Size != sizeof(ULONG))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    *ObjectSecurityInfo = ObpObjectSecurityMode;

    return STATUS_SUCCESS;
}

/* Class 73 - Logical processor information */
QSI_DEF(SystemLogicalProcessorInformation)
{
    LONG i;
    PKPRCB Prcb;
    KAFFINITY CurrentProc;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG DataSize = 0, ProcessorFlags;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION CurrentInfo;

    /* First, browse active processors, thanks to the map */
    i = 0;
    CurrentInfo = Buffer;
    CurrentProc = KeActiveProcessors;
    do
    {
        /* If current processor is active and is main in case of HT/MC, return it */
        Prcb = KiProcessorBlock[i];
        if ((CurrentProc & 1) &&
            Prcb == Prcb->MultiThreadSetMaster)
        {
            /* Assume processor can do HT or multicore */
            ProcessorFlags = 1;

            /* If set is the same for PRCB and multithread, then
             * actually, the processor is single core
             */
            if (Prcb->SetMember == Prcb->MultiThreadProcessorSet)
            {
                ProcessorFlags = 0;
            }

            /* Check we have enough room to return */
            DataSize += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
            if (DataSize > Size)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            else
            {
                /* Zero output and return */
                RtlZeroMemory(CurrentInfo, sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
                CurrentInfo->ProcessorMask = Prcb->MultiThreadProcessorSet;

                /* Processor core needs 1 if HT/MC is supported */
                CurrentInfo->Relationship = RelationProcessorCore;
                CurrentInfo->ProcessorCore.Flags = ProcessorFlags;
                ++CurrentInfo;
            }
        }

        /* Move to the next proc */
        CurrentProc >>= 1;
        ++i;
    /* Loop while there's someone in the bitmask */
    } while (CurrentProc != 0);

    /* Now, return the NUMA nodes */
    for (i = 0; i < KeNumberNodes; ++i)
    {
        /* Check we have enough room to return */
        DataSize += sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
        if (DataSize > Size)
        {
            Status = STATUS_INFO_LENGTH_MISMATCH;
        }
        else
        {
            /* Zero output and return */
            RtlZeroMemory(CurrentInfo, sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
            CurrentInfo->ProcessorMask = KeActiveProcessors;

            /* NUMA node needs its ID */
            CurrentInfo->Relationship = RelationNumaNode;
            CurrentInfo->NumaNode.NodeNumber = i;
            ++CurrentInfo;
        }
    }

    *ReqSize = DataSize;

    return Status;
}

/* Class 76 - System firmware table information */
QSI_DEF(SystemFirmwareTableInformation)
{
    PSYSTEM_FIRMWARE_TABLE_INFORMATION SysFirmwareInfo = (PSYSTEM_FIRMWARE_TABLE_INFORMATION)Buffer;
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG InputBufSize;
    ULONG DataSize = 0;
    ULONG TableCount = 0;

    DPRINT("NtQuerySystemInformation - SystemFirmwareTableInformation\n");

    /* Set initial required buffer size */
    *ReqSize = FIELD_OFFSET(SYSTEM_FIRMWARE_TABLE_INFORMATION, TableBuffer);

    /* Check user's buffer size */
    if (Size < *ReqSize)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    InputBufSize = SysFirmwareInfo->TableBufferLength;
    switch (SysFirmwareInfo->ProviderSignature)
    {
        /*
         * ExpFirmwareTableResource and ExpFirmwareTableProviderListHead
         * variables should be used there somehow...
         */
        case SIG_ACPI:
        {
            /* FIXME: Not implemented yet */
            DPRINT1("ACPI provider not implemented\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        case SIG_FIRM:
        {
            /* FIXME: Not implemented yet */
            DPRINT1("FIRM provider not implemented\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;
        }
        case SIG_RSMB:
        {
            Status = ExpGetRawSMBiosTable(NULL, &DataSize, 0);
            if (DataSize > 0)
            {
                TableCount = 1;
                if (SysFirmwareInfo->Action == SystemFirmwareTable_Enumerate)
                {
                    DataSize = TableCount * sizeof(ULONG);
                    if (DataSize <= InputBufSize)
                    {
                        *(ULONG *)SysFirmwareInfo->TableBuffer = 0;
                    }
                }
                else if (SysFirmwareInfo->Action == SystemFirmwareTable_Get
                         && DataSize <= InputBufSize)
                {
                    Status = ExpGetRawSMBiosTable(SysFirmwareInfo->TableBuffer, &DataSize, InputBufSize);
                }
                SysFirmwareInfo->TableBufferLength = DataSize;
            }
            break;
        }
        default:
        {
            DPRINT1("SystemFirmwareTableInformation: Unsupported provider (0x%x)\n",
                    SysFirmwareInfo->ProviderSignature);
            Status = STATUS_ILLEGAL_FUNCTION;
        }
    }

    if (NT_SUCCESS(Status))
    {
        switch (SysFirmwareInfo->Action)
        {
            case SystemFirmwareTable_Enumerate:
            case SystemFirmwareTable_Get:
            {
                if (SysFirmwareInfo->TableBufferLength > InputBufSize)
                {
                    Status = STATUS_BUFFER_TOO_SMALL;
                }
                break;
            }
            default:
            {
                DPRINT1("SystemFirmwareTableInformation: Unsupported action (0x%x)\n",
                        SysFirmwareInfo->Action);
                Status = STATUS_ILLEGAL_FUNCTION;
            }
        }
    }
    else
    {
        SysFirmwareInfo->TableBufferLength = 0;
    }
    return Status;
}

/* Query/Set Calls Table */
typedef
struct _QSSI_CALLS
{
    NTSTATUS (* Query) (PVOID,ULONG,PULONG);
    NTSTATUS (* Set) (PVOID,ULONG);
} QSSI_CALLS;

// QS    Query & Set
// QX    Query
// XS    Set
// XX    unknown behaviour
//
#define SI_QS(n) {QSI_USE(n),SSI_USE(n)}
#define SI_QX(n) {QSI_USE(n),NULL}
#define SI_XS(n) {NULL,SSI_USE(n)}
#define SI_XX(n) {NULL,NULL}

static
QSSI_CALLS
CallQS[] =
{
    SI_QX(SystemBasicInformation),
    SI_QX(SystemProcessorInformation),
    SI_QX(SystemPerformanceInformation),
    SI_QX(SystemTimeOfDayInformation),
    SI_QX(SystemPathInformation),
    SI_QX(SystemProcessInformation),
    SI_QX(SystemCallCountInformation),
    SI_QX(SystemDeviceInformation),
    SI_QX(SystemProcessorPerformanceInformation),
    SI_QS(SystemFlagsInformation),
    SI_QX(SystemCallTimeInformation), /* should be SI_XX */
    SI_QX(SystemModuleInformation),
    SI_QX(SystemLocksInformation),
    SI_QX(SystemStackTraceInformation), /* should be SI_XX */
    SI_QX(SystemPagedPoolInformation), /* should be SI_XX */
    SI_QX(SystemNonPagedPoolInformation), /* should be SI_XX */
    SI_QX(SystemHandleInformation),
    SI_QX(SystemObjectInformation),
    SI_QX(SystemPageFileInformation),
    SI_QX(SystemVdmInstemulInformation),
    SI_QX(SystemVdmBopInformation), /* it should be SI_XX */
    SI_QS(SystemFileCacheInformation),
    SI_QX(SystemPoolTagInformation),
    SI_QX(SystemInterruptInformation),
    SI_QS(SystemDpcBehaviourInformation),
    SI_QX(SystemFullMemoryInformation), /* it should be SI_XX */
    SI_XS(SystemLoadGdiDriverInformation),
    SI_XS(SystemUnloadGdiDriverInformation),
    SI_QS(SystemTimeAdjustmentInformation),
    SI_QX(SystemSummaryMemoryInformation), /* it should be SI_XX */
    SI_QX(SystemNextEventIdInformation), /* it should be SI_XX */
    SI_QX(SystemPerformanceTraceInformation), /* it should be SI_XX */
    SI_QX(SystemCrashDumpInformation),
    SI_QX(SystemExceptionInformation),
    SI_QX(SystemCrashDumpStateInformation),
    SI_QX(SystemKernelDebuggerInformation),
    SI_QX(SystemContextSwitchInformation),
    SI_QS(SystemRegistryQuotaInformation),
    SI_XS(SystemExtendServiceTableInformation),
    SI_XS(SystemPrioritySeperation),
    SI_QX(SystemVerifierAddDriverInformation), /* it should be SI_XX */
    SI_QX(SystemVerifierRemoveDriverInformation), /* it should be SI_XX */
    SI_QX(SystemProcessorIdleInformation), /* it should be SI_XX */
    SI_QX(SystemLegacyDriverInformation), /* it should be SI_XX */
    SI_QS(SystemCurrentTimeZoneInformation), /* it should be SI_QX */
    SI_QX(SystemLookasideInformation),
    SI_XS(SystemTimeSlipNotification),
    SI_XS(SystemSessionCreate),
    SI_XS(SystemSessionDetach),
    SI_QX(SystemSessionInformation), /* it should be SI_XX */
    SI_QX(SystemRangeStartInformation),
    SI_QS(SystemVerifierInformation),
    SI_XS(SystemVerifierThunkExtend),
    SI_QX(SystemSessionProcessesInformation),
    SI_XS(SystemLoadGdiDriverInSystemSpaceInformation),
    SI_QX(SystemNumaProcessorMap),
    SI_QX(SystemPrefetcherInformation),
    SI_QX(SystemExtendedProcessInformation),
    SI_QX(SystemRecommendedSharedDataAlignment),
    SI_XX(SystemComPlusPackage),
    SI_QX(SystemNumaAvailableMemory),
    SI_XX(SystemProcessorPowerInformation), /* FIXME: not implemented */
    SI_XX(SystemEmulationBasicInformation), /* FIXME: not implemented */
    SI_XX(SystemEmulationProcessorInformation), /* FIXME: not implemented */
    SI_QX(SystemExtendedHandleInformation),
    SI_XX(SystemLostDelayedWriteInformation), /* FIXME: not implemented */
    SI_XX(SystemBigPoolInformation), /* FIXME: not implemented */
    SI_XX(SystemSessionPoolTagInformation), /* FIXME: not implemented */
    SI_XX(SystemSessionMappedViewInformation), /* FIXME: not implemented */
    SI_XX(SystemHotpatchInformation), /* FIXME: not implemented */
    SI_QX(SystemObjectSecurityMode),
    SI_XX(SystemWatchdogTimerHandler), /* FIXME: not implemented */
    SI_XX(SystemWatchdogTimerInformation), /* FIXME: not implemented */
    SI_QX(SystemLogicalProcessorInformation),
    SI_XX(SystemWow64SharedInformation), /* FIXME: not implemented */
    SI_XX(SystemRegisterFirmwareTableInformationHandler), /* FIXME: not implemented */
    SI_QX(SystemFirmwareTableInformation),
};

C_ASSERT(SystemBasicInformation == 0);
#define MIN_SYSTEM_INFO_CLASS (SystemBasicInformation)
#define MAX_SYSTEM_INFO_CLASS RTL_NUMBER_OF(CallQS)

/*
 * @implemented
 */
__kernel_entry
NTSTATUS
NTAPI
NtQuerySystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status = STATUS_NOT_IMPLEMENTED;
    ULONG CapturedResultLength = 0;
    ULONG Alignment = TYPE_ALIGNMENT(ULONG);
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    _SEH2_TRY
    {
#if (NTDDI_VERSION >= NTDDI_VISTA)
        /*
         * Check whether the request is valid.
         */
        if (SystemInformationClass < MIN_SYSTEM_INFO_CLASS ||
            SystemInformationClass >= MAX_SYSTEM_INFO_CLASS)
        {
            _SEH2_YIELD(return STATUS_INVALID_INFO_CLASS);
        }
#endif

        if (PreviousMode != KernelMode)
        {
            /* SystemKernelDebuggerInformation needs only BOOLEAN alignment */
            if (SystemInformationClass == SystemKernelDebuggerInformation)
                Alignment = TYPE_ALIGNMENT(BOOLEAN);

            ProbeForWrite(SystemInformation, SystemInformationLength, Alignment);
            if (ReturnLength != NULL)
                ProbeForWriteUlong(ReturnLength);
        }

        if (ReturnLength)
            *ReturnLength = 0;

#if (NTDDI_VERSION < NTDDI_VISTA)
        /*
         * Check whether the request is valid.
         */
        if (SystemInformationClass < MIN_SYSTEM_INFO_CLASS ||
            SystemInformationClass >= MAX_SYSTEM_INFO_CLASS)
        {
            _SEH2_YIELD(return STATUS_INVALID_INFO_CLASS);
        }
#endif

        if (CallQS[SystemInformationClass].Query != NULL)
        {
            /* Hand the request to a subhandler */
            Status = CallQS[SystemInformationClass].Query(SystemInformation,
                                                          SystemInformationLength,
                                                          &CapturedResultLength);

            /* Save the result length to the caller */
            if (ReturnLength)
                *ReturnLength = CapturedResultLength;
        }
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

__kernel_entry
NTSTATUS
NTAPI
NtSetSystemInformation(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _In_reads_bytes_(SystemInformationLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength)
{
    NTSTATUS Status = STATUS_INVALID_INFO_CLASS;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    _SEH2_TRY
    {
        /*
         * If called from user mode, check possible unsafe arguments.
         */
        if (PreviousMode != KernelMode)
        {
            ProbeForRead(SystemInformation, SystemInformationLength, sizeof(ULONG));
        }

        /*
         * Check whether the request is valid.
         */
        if ((SystemInformationClass >= MIN_SYSTEM_INFO_CLASS) &&
            (SystemInformationClass < MAX_SYSTEM_INFO_CLASS))
        {
            if (CallQS[SystemInformationClass].Set != NULL)
            {
                /* Hand the request to a subhandler */
                Status = CallQS[SystemInformationClass].Set(SystemInformation,
                                                            SystemInformationLength);
            }
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

ULONG
NTAPI
NtGetCurrentProcessorNumber(VOID)
{
    /* Just use Ke */
    return KeGetCurrentProcessorNumber();
}

#undef ExGetPreviousMode
KPROCESSOR_MODE
NTAPI
ExGetPreviousMode(VOID)
{
    /* Just use Ke */
    return KeGetPreviousMode();
}
