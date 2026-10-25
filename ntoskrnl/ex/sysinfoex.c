/*
 * PROJECT:     ReactOS kernel
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Implementation of NtQuerySystemInformationEx
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer (thomas.faber@reactos.org)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

/*
 * References:
 * - https://www.geoffchappell.com/studies/windows/km/ntoskrnl/api/ex/sysinfo/queryex.htm
 * - https://ntdoc.m417z.com/system_information_class
 * - https://github.com/yardenshafir/InformationClasses
 */

// TODO:
// - Validate all ProbeForWrite(ReturnLength...) for alignment (1?)
// - Remove duplicate probing
// - if (SystemInformationLength == 0) SystemInformation = NULL; at the main function?

static
NTSTATUS
ProbeOutputBuffer(
    _Out_writes_bytes_to_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _In_ ULONG Alignment)
{
    if (SystemInformationLength == 0)
    {
        return STATUS_SUCCESS;
    }
    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(SystemInformation, SystemInformationLength, Alignment);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;
    return STATUS_SUCCESS;
}

static
NTSTATUS
ProbeAndCaptureInputBuffer(
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_(SystemInformationLength, *ReturnLength) PVOID CapturedInputData,
    _In_ ULONG InputDataSize,
    _In_ ULONG InputDataAlignment)
{
    if (InputBufferLength < InputDataSize)
    {
        return STATUS_INVALID_PARAMETER;
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            ProbeForRead(InputBuffer, InputBufferLength, InputDataAlignment);
        }

        RtlCopyMemory(CapturedInputData, InputBuffer, InputDataSize);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static
NTSTATUS
CaptureGroupNumber(
    _In_reads_bytes_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_ PUSHORT GroupNumber)
{
    NTSTATUS Status;

    /* Special case for calls from NtQuerySystemInformation */
    if ((InputBuffer == NULL) && (InputBufferLength == 0))
    {
        *GroupNumber = ALL_PROCESSOR_GROUPS;
        return STATUS_SUCCESS;
    }

    /* Get the group number from the input buffer */
    Status = ProbeAndCaptureInputBuffer(InputBuffer,
                                        InputBufferLength,
                                        GroupNumber,
                                        sizeof(*GroupNumber),
                                        sizeof(*GroupNumber));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
ReturnRequiredLength(
    _Out_opt_ PULONG ReturnLength,
    _In_ ULONG RequiredLength)
{
    if (ReturnLength != NULL)
    {
        _SEH2_TRY
        {
            if (ExGetPreviousMode() != KernelMode)
            {
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
            }

            *ReturnLength = RequiredLength;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            return _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    return STATUS_INFO_LENGTH_MISMATCH;
}

static
NTSTATUS
ExpQueryExtendedSystemProcessorPerformanceInformation( // 8
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    USHORT GroupNumber;
    ULONG ProcessorCount;
    ULONG PerfInfoLength;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION PerfInfo;

    /* Get the group number from the input buffer */
    Status = CaptureGroupNumber(InputBuffer, InputBufferLength, &GroupNumber);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (GroupNumber != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // FIXME: We don't support processor groups yet
    ProcessorCount = KeNumberProcessors;

    /* If the buffer is too small, only return the required size */
    PerfInfoLength = ProcessorCount * sizeof(*PerfInfo);
    if (SystemInformationLength < PerfInfoLength)
    {
        return ReturnRequiredLength(ReturnLength, PerfInfoLength);
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
            ProbeForWrite(SystemInformation, SystemInformationLength, sizeof(ULONG));
        }

        /* Loop all processors and fill in the data */
        PerfInfo = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION)SystemInformation;
        for (ULONG i = 0; i < ProcessorCount; i++)
        {
            PKPRCB Prcb = KiProcessorBlock[i];

            PerfInfo[i].IdleTime.QuadPart =
                KiClockTicksTo100ns(Prcb->IdleThread->KernelTime);
            PerfInfo[i].KernelTime.QuadPart =
                KiClockTicksTo100ns(Prcb->KernelTime);
            PerfInfo[i].UserTime.QuadPart =
                KiClockTicksTo100ns(Prcb->UserTime);
            PerfInfo[i].DpcTime.QuadPart =
                KiClockTicksTo100ns(Prcb->DpcTime);
            PerfInfo[i].InterruptTime.QuadPart =
                KiClockTicksTo100ns(Prcb->InterruptTime);
            PerfInfo[i].InterruptCount = Prcb->InterruptCount;
        }

        *ReturnLength = PerfInfoLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static
NTSTATUS
ExpQueryExtendedSystemInterruptInformation( // 23
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    USHORT GroupNumber;
    ULONG ProcessorCount;
    ULONG IntInfoLength;
    PSYSTEM_INTERRUPT_INFORMATION IntInfo;

    /* Get the group number from the input buffer */
    Status = CaptureGroupNumber(InputBuffer, InputBufferLength, &GroupNumber);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (GroupNumber != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // FIXME: We don't support processor groups yet
    ProcessorCount = KeNumberProcessors;

    /* If the buffer is too small, only return the required size */
    IntInfoLength = ProcessorCount * sizeof(*IntInfo);
    if (SystemInformationLength < IntInfoLength)
    {
        return ReturnRequiredLength(ReturnLength, IntInfoLength);
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
            ProbeForWrite(SystemInformation, IntInfoLength, sizeof(ULONG));
        }

        /* Loop all processors and fill in the data */
        IntInfo = (PSYSTEM_INTERRUPT_INFORMATION)SystemInformation;
        for (ULONG i = 0; i < ProcessorCount; i++)
        {
            PKPRCB Prcb = KiProcessorBlock[i];

#ifdef _M_AMD64
            IntInfo[i].ContextSwitches = Prcb->KeContextSwitches;
#else
            IntInfo[i].ContextSwitches = 0; // Prcb->ContextSwitches;
#endif
            IntInfo[i].DpcCount = Prcb->DpcData->DpcCount;
            IntInfo[i].DpcRate = Prcb->DpcRequestRate;
            IntInfo[i].TimeIncrement = KeTimeIncrement;
            IntInfo[i].DpcBypassCount = 0; // FIXME: Where is this stored?
            IntInfo[i].ApcBypassCount = 0; // FIXME: Where is this stored?
        }

        *ReturnLength = IntInfoLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static
NTSTATUS
ExpQueryExtendedSystemProcessorIdleInformation( // 42
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    USHORT GroupNumber;
    ULONG ProcessorCount;
    ULONG IdleInfoLength;
    PSYSTEM_PROCESSOR_IDLE_INFORMATION IdleInfo;

    /* Get the group number from the input buffer */
    Status = CaptureGroupNumber(InputBuffer, InputBufferLength, &GroupNumber);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (GroupNumber != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // FIXME: We don't support processor groups yet
    ProcessorCount = KeNumberProcessors;

    /* If the buffer is too small, only return the required size */
    IdleInfoLength = ProcessorCount * sizeof(*IdleInfo);
    if (SystemInformationLength < IdleInfoLength)
    {
        return ReturnRequiredLength(ReturnLength, IdleInfoLength);
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
            ProbeForWrite(SystemInformation, IdleInfoLength, sizeof(ULONG));
        }

        /* Loop all processors and fill in the data */
        IdleInfo = (PSYSTEM_PROCESSOR_IDLE_INFORMATION)SystemInformation;
        for (ULONG i = 0; i < ProcessorCount; i++)
        {
            PKPRCB Prcb = KiProcessorBlock[i];

            /* So far we only support C1, which is the state of the processor in the idle thread */
            IdleInfo[i].IdleTime =
                KiClockTicksTo100ns(Prcb->IdleThread->KernelTime);
            IdleInfo[i].C1Time = IdleInfo[i].IdleTime;
            IdleInfo[i].C2Time = 0;
            IdleInfo[i].C3Time = 0;
            IdleInfo[i].C1Transitions = Prcb->IdleThread->ContextSwitches;
            IdleInfo[i].C2Transitions = 0;
            IdleInfo[i].C3Transitions = 0;
            IdleInfo[i].Padding = 0;
        }

        *ReturnLength = IdleInfoLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static
NTSTATUS
ExpQueryExtendedSystemProcessorPowerInformation( // 61
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    USHORT GroupNumber;
    ULONG ProcessorCount;
    ULONG PowerInfoLength;
    PSYSTEM_PROCESSOR_POWER_INFORMATION PowerInfo;

    /* Get the group number from the input buffer */
    Status = CaptureGroupNumber(InputBuffer, InputBufferLength, &GroupNumber);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (GroupNumber != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // FIXME: We don't support processor groups yet
    ProcessorCount = KeNumberProcessors;

    /* If the buffer is too small, only return the required size */
    PowerInfoLength = ProcessorCount * sizeof(*PowerInfo);
    if (SystemInformationLength < PowerInfoLength)
    {
        return ReturnRequiredLength(ReturnLength, PowerInfoLength);
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
            ProbeForWrite(SystemInformation, PowerInfoLength, sizeof(ULONG));
        }

        /* Loop all processors and fill in the data */
        PowerInfo = (PSYSTEM_PROCESSOR_POWER_INFORMATION)SystemInformation;
        for (ULONG i = 0; i < ProcessorCount; i++)
        {
            PKPRCB Prcb = KiProcessorBlock[i];

            /* Fill in frequency percentages */
            PowerInfo[i].CurrentFrequency = 100;
            PowerInfo[i].ThermalLimitFrequency = 0;
            PowerInfo[i].ConstantThrottleFrequency = 0;
            PowerInfo[i].DegradedThrottleFrequency = 0;
            PowerInfo[i].LastBusyFrequency = 0;
            PowerInfo[i].LastC3Frequency = 0;
            PowerInfo[i].LastAdjustedBusyFrequency = 0;
            PowerInfo[i].ProcessorMinThrottle = 5;
            PowerInfo[i].ProcessorMaxThrottle = 100;

            /* Fill in counts */
            PowerInfo[i].NumberOfFrequencies = 1;
            PowerInfo[i].PromotionCount = 0;
            PowerInfo[i].DemotionCount = 0;
            PowerInfo[i].ErrorCount = 0;
            PowerInfo[i].RetryCount = 0;

            /* Fill in times */
            PowerInfo[i].CurrentFrequencyTime = 0;
            PowerInfo[i].CurrentProcessorTime =
                KiClockTicksTo100ns(Prcb->KernelTime + Prcb->UserTime);
            PowerInfo[i].CurrentProcessorIdleTime =
                KiClockTicksTo100ns(Prcb->IdleThread->KernelTime);
            PowerInfo[i].LastProcessorTime = 0;
            PowerInfo[i].LastProcessorIdleTime = 0;
            PowerInfo[i].Energy = 0;
        }

        *ReturnLength = PowerInfoLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static
NTSTATUS
ExpQueryExtendedSystemWatchdogTimerInformation( // 72
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    WATCHDOG_INFORMATION_CLASS WdInfoClass;
    ULONG WatchdogInfoLength;
    PSYSTEM_WATCHDOG_TIMER_INFORMATION WatchdogInfo;

    /* Get the watchdog info class from the input buffer */
    Status = ProbeAndCaptureInputBuffer(InputBuffer,
                                        InputBufferLength,
                                        &WdInfoClass,
                                        sizeof(WdInfoClass),
                                        sizeof(WdInfoClass));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* This info class always requires a valid output buffer */
    if ((SystemInformation == NULL) ||
        (SystemInformationLength != sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* If the buffer is too small, only return the required size */
    WatchdogInfoLength = sizeof(SYSTEM_WATCHDOG_TIMER_INFORMATION);
    if (SystemInformationLength < WatchdogInfoLength)
    {
        return ReturnRequiredLength(ReturnLength, WatchdogInfoLength);
    }

    _SEH2_TRY
    {
        WatchdogInfo = (PSYSTEM_WATCHDOG_TIMER_INFORMATION)SystemInformation;
        if (ExGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
            ProbeForWrite(SystemInformation, sizeof(*WatchdogInfo), sizeof(ULONG));
        }

        switch (WdInfoClass)
        {
            case WdInfoNop:
            case WdInfoGeneratedLastReset:
                WatchdogInfo->WdInfoClass = WdInfoClass;
                WatchdogInfo->DataValue = 0;
                break;

            default:
                return STATUS_NOT_SUPPORTED;
        }

        *ReturnLength = WatchdogInfoLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static __inline BOOLEAN IsPowerOf2(IN ULONG Number)
{
    if (Number == 0)
        return FALSE;
    return (Number & (Number - 1)) == 0;
}

_Maybe_raises_SEH_exception_
static
NTSTATUS
ExpCopyLogicalProcessorInformationProcessorCore(
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION LogProcInfo = SystemInformation;
    ULONG MaxEntries = SystemInformationLength / sizeof(*LogProcInfo);

    /* Loop all processors */
    ULONG EntryIndex = 0;
    KAFFINITY ProcessedThreadsMask = 0;
    for (ULONG i = 0; i < KeNumberProcessors; i++)
    {
        PKPRCB Prcb = KiProcessorBlock[i];

        /* Check if we processed this core already */
        if (Prcb->MultiThreadProcessorSet & ProcessedThreadsMask)
        {
            continue;
        }

        ProcessedThreadsMask |= Prcb->MultiThreadProcessorSet;

        /* Get the core's PRCB */
        PKPRCB CorePrcb = Prcb->MultiThreadSetMaster ? Prcb->MultiThreadSetMaster : Prcb;

        /* Check if we should write entries */
        if ((LogProcInfo != NULL) && (EntryIndex < MaxEntries))
        {
            LogProcInfo[EntryIndex].Relationship = RelationProcessorCore;
            LogProcInfo[EntryIndex].ProcessorMask = CorePrcb->MultiThreadProcessorSet;
            LogProcInfo[EntryIndex].ProcessorCore.Flags = IsPowerOf2(CorePrcb->MultiThreadProcessorSet);
        }

        EntryIndex++;

        /* Enumerate valid cache levels (entries with Level != 0) */
        ULONG CacheCount = min(CorePrcb->CacheCount, ARRAYSIZE(CorePrcb->Cache));
        for (ULONG j = 0; j < CacheCount; j++)
        {
            if (CorePrcb->Cache[j].Level != 0)
            {
                if ((LogProcInfo != NULL) && (EntryIndex < MaxEntries))
                {
                    LogProcInfo[EntryIndex].Relationship = RelationCache;
                    LogProcInfo[EntryIndex].ProcessorMask = CorePrcb->MultiThreadProcessorSet;
                    LogProcInfo[EntryIndex].Cache = CorePrcb->Cache[j];
                }

                EntryIndex++;
            }
        }
    }

    /* Add the processor package */
    if ((LogProcInfo != NULL) && (EntryIndex < MaxEntries))
    {
        LogProcInfo[EntryIndex].Relationship = RelationProcessorPackage;
        LogProcInfo[EntryIndex].ProcessorMask = KeActiveProcessors;
        LogProcInfo[EntryIndex].ProcessorCore.Flags = 0;
    }

    EntryIndex++;

    /* Add the NUMA node */
    if ((LogProcInfo != NULL) && (EntryIndex < MaxEntries))
    {
        LogProcInfo[EntryIndex].Relationship = RelationNumaNode;
        LogProcInfo[EntryIndex].ProcessorMask = KeActiveProcessors;
        LogProcInfo[EntryIndex].NumaNode.NodeNumber = 0;
    }

    EntryIndex++;

    ULONG InfoLength = EntryIndex * sizeof(*LogProcInfo);
    if (ReturnLength != NULL)
        *ReturnLength = InfoLength;

    if (InfoLength > SystemInformationLength)
        return STATUS_INFO_LENGTH_MISMATCH;

    return STATUS_SUCCESS;
}

static
NTSTATUS
ExpQueryExtendedSystemLogicalProcessorInformation( // 73
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    USHORT RelationshipType;

    /* Get the logical processor relationship type from the input buffer */
    Status = ProbeAndCaptureInputBuffer(InputBuffer,
                                        InputBufferLength,
                                        &RelationshipType,
                                        sizeof(RelationshipType),
                                        sizeof(RelationshipType));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (RelationshipType != RelationProcessorCore)
    {
        return STATUS_INVALID_PARAMETER;
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            if (ReturnLength != NULL)
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);

            if (SystemInformation != NULL)
            {
                ProbeForWrite(SystemInformation,
                              SystemInformationLength,
                              TYPE_ALIGNMENT(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
            }
        }

        Status = ExpCopyLogicalProcessorInformationProcessorCore(SystemInformation,
                                                                 SystemInformationLength,
                                                                 ReturnLength);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

static
NTSTATUS
ExpQueryExtendedSystemProcessorIdleCycleTimeInformation( // 83
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    USHORT GroupNumber;
    ULONG ProcessorCount;
    ULONG CycleTimeInfoLength;
    ULONG MaxCount;
    PSYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION CycleTimeInfo;

    /* Get the group number from the input buffer */
    Status = CaptureGroupNumber(InputBuffer, InputBufferLength, &GroupNumber);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_INFO_LENGTH_MISMATCH)
            Status = STATUS_BUFFER_TOO_SMALL;

        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (GroupNumber != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // FIXME: We don't support processor groups yet
    ProcessorCount = KeNumberProcessors;

    /* If the buffer is too small, only return the required size */
    CycleTimeInfoLength = ProcessorCount * sizeof(*CycleTimeInfo);

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            if (ReturnLength != NULL)
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
            //ProbeForWrite(SystemInformation, CycleTimeInfoLength, sizeof(ULONG));
        }

        /* Loop all processors (or maximum writable) and fill in the data */
        CycleTimeInfo = (PSYSTEM_PROCESSOR_IDLE_CYCLE_TIME_INFORMATION)SystemInformation;
        MaxCount = min(SystemInformationLength / sizeof(*CycleTimeInfo), ProcessorCount);
        for (ULONG i = 0; i < MaxCount; i++)
        {
            PKPRCB Prcb = KiProcessorBlock[i];
            PETHREAD IdleThread = (PETHREAD)Prcb->IdleThread;
            CycleTimeInfo[i].CycleTime = IdleThread->CycleTime;
        }

        if (ReturnLength != NULL)
            *ReturnLength = CycleTimeInfoLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (SystemInformationLength == 0)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (SystemInformationLength < CycleTimeInfoLength)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
ExpQueryExtendedSystemProcessorPerformanceDistribution( // 100
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    USHORT GroupNumber;
    ULONG ProcessorCount;
    ULONG PerfDistInfoLength;
    PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION PerfDistInfo;

    /* Get the group number from the input buffer */
    Status = CaptureGroupNumber(InputBuffer, InputBufferLength, &GroupNumber);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (GroupNumber != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // FIXME: We don't support processor groups yet
    ProcessorCount = KeNumberProcessors;

    /* Calculate the header size (1 offset (ULONG) per processor) */
    ULONG HeaderSize = FIELD_OFFSET(SYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION, Offsets[ProcessorCount]);

    /* The header is followed by the state distributions for each processor. These are dynamically
       sized structures, depending on the number of supported P-states. They must be 8-byte aligned. */
    ULONG StartOffset = ALIGN_UP_BY(HeaderSize, sizeof(ULONG64));
    ULONG StateCount = 2; // FIXME: We only support C0 and C1 for now, so 2 states
    ULONG DistributionSize = FIELD_OFFSET(SYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION, States[StateCount]);
    PerfDistInfoLength = StartOffset + ProcessorCount * DistributionSize;

    /* If the buffer is too small, only return the required size */
    if (SystemInformationLength < PerfDistInfoLength)
    {
        return ReturnRequiredLength(ReturnLength, PerfDistInfoLength);
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
            ProbeForWrite(SystemInformation, PerfDistInfoLength, sizeof(ULONG64));
        }

        /* Loop all processors and fill in the data */
        ULONG CurrentOffset = StartOffset;
        PerfDistInfo = (PSYSTEM_PROCESSOR_PERFORMANCE_DISTRIBUTION)SystemInformation;
        for (ULONG i = 0; i < ProcessorCount; i++)
        {
            PKPRCB Prcb = KiProcessorBlock[i];
            PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION Distribution =
                (PSYSTEM_PROCESSOR_PERFORMANCE_STATE_DISTRIBUTION)((PUCHAR)SystemInformation + CurrentOffset);
            PerfDistInfo->Offsets[i] = CurrentOffset;
            CurrentOffset += DistributionSize;

            ULONG C1Time = Prcb->IdleThread->KernelTime;
            ULONG C0Time = Prcb->KernelTime + Prcb->UserTime - C1Time;
            ULONG TotalTime = C0Time + C1Time;

            /* Fill in the distribution */
            Distribution->ProcessorNumber = (USHORT)i;
            Distribution->StateCount = StateCount;
            Distribution->States[0].Hits = C0Time; // C0
            Distribution->States[0].PercentFrequency = 100 * C0Time / TotalTime;
            Distribution->States[1].Hits = C1Time; // C1
            Distribution->States[1].PercentFrequency = 100 * C1Time / TotalTime;
        }

        /* Make sure our calculation was right */
        ASSERT(CurrentOffset == PerfDistInfoLength);
        *ReturnLength = PerfDistInfoLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static
NTSTATUS
ExpQueryExtendedSystemLogicalProcessorAndGroupInformation( // 107
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    LOGICAL_PROCESSOR_RELATIONSHIP RelationshipType;
    PKPRCB Prcb0;
    ULONG ProcessorCount;
    ULONG LogsPerCore;
    ULONG PhysicalCoreCount;
    ULONG ValidCacheCount;
    ULONG_PTR AllProcessorsMask;
    ULONG RequiredLength;
    ULONG WrittenLength;
    ULONG i;
    NTSTATUS Status;

    /* Get the logical processor relationship type from the input buffer */
    Status = ProbeAndCaptureInputBuffer(InputBuffer,
                                        InputBufferLength,
                                        &RelationshipType,
                                        sizeof(RelationshipType),
                                        sizeof(RelationshipType));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /*
     * Entry sizes for each relationship type (header + struct body with 1 group).
     * SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX header: Relationship(4) + Size(4) = 8 bytes.
     * The union members all start at offset 8, so entry size = 8 + body_size.
     *
     * PROCESSOR_RELATIONSHIP body with 1 GROUP_AFFINITY:
     *   Flags(1)+EfficiencyClass(1)+Reserved(20)+GroupCount(2) = 24 bytes + sizeof(GROUP_AFFINITY)
     * CACHE_RELATIONSHIP body with 1 GROUP_AFFINITY:
     *   Level(1)+Assoc(1)+LineSize(2)+CacheSize(4)+Type(4)+Reserved(18)+GroupCount(2) = 32 bytes
     *   + sizeof(GROUP_AFFINITY)
     * NUMA_NODE_RELATIONSHIP body with 1 GROUP_AFFINITY:
     *   NodeNumber(4)+Reserved(18)+GroupCount(2) = 24 bytes + sizeof(GROUP_AFFINITY)
     * GROUP_RELATIONSHIP body with 1 PROCESSOR_GROUP_INFO:
     *   MaxGroupCount(2)+ActiveGroupCount(2)+Reserved(20) = 24 bytes + sizeof(PROCESSOR_GROUP_INFO)
     */
    const ULONG GroupCount = 1;
    const ULONG HeaderSize        = sizeof(LOGICAL_PROCESSOR_RELATIONSHIP) + sizeof(ULONG);
    const ULONG ProcessorBodySize = FIELD_OFFSET(PROCESSOR_RELATIONSHIP, GroupMask)
                                  + GroupCount * sizeof(GROUP_AFFINITY);
    const ULONG CacheBodySize     = FIELD_OFFSET(CACHE_RELATIONSHIP, GroupMask)
                                  + GroupCount * sizeof(GROUP_AFFINITY);
    const ULONG NumaBodySize      = FIELD_OFFSET(NUMA_NODE_RELATIONSHIP, GroupMask)
                                  + GroupCount * sizeof(GROUP_AFFINITY);
    const ULONG GroupBodySize     = FIELD_OFFSET(GROUP_RELATIONSHIP, GroupInfo)
                                  + GroupCount * sizeof(PROCESSOR_GROUP_INFO);

    const ULONG ProcessorEntrySize = HeaderSize + ProcessorBodySize;
    const ULONG CacheEntrySize     = HeaderSize + CacheBodySize;
    const ULONG NumaEntrySize      = HeaderSize + NumaBodySize;
    const ULONG GroupEntrySize     = HeaderSize + GroupBodySize;


    /*
     * Only RelationProcessorCore..RelationGroup (0-4) are valid.
     * All other values return STATUS_UNSUCCESSFUL with ReturnLength = 0.
     */
    if ((ULONG)RelationshipType > (ULONG)RelationGroup)
    {
        _SEH2_TRY
        {
            if (ReturnLength != NULL)
            {
                if (ExGetPreviousMode() != KernelMode)
                    ProbeForWrite(ReturnLength, sizeof(*ReturnLength), 1);
                *ReturnLength = 0;
            }
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            return _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        return STATUS_UNSUCCESSFUL;
    }

    /* Get topology information from the first PRCB */
    Prcb0 = KiProcessorBlock[0];
    ProcessorCount = KeNumberProcessors;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    LogsPerCore = max(1, (ULONG)Prcb0->LogicalProcessorsPerCore);
#else
    LogsPerCore = max(1, (ULONG)Prcb0->LogicalProcessorsPerPhysicalProcessor);
#endif
    PhysicalCoreCount = (ProcessorCount + LogsPerCore - 1) / LogsPerCore;

    /* Count valid cache levels */
    ValidCacheCount = 0;
    for (i = 0; i < min(Prcb0->CacheCount, ARRAYSIZE(Prcb0->Cache)); i++)
    {
        if (Prcb0->Cache[i].Level != 0)
            ValidCacheCount++;
    }

    /* Build the mask of all processors in group 0 */
    AllProcessorsMask = (ProcessorCount < (sizeof(ULONG_PTR) * 8))
        ? (((ULONG_PTR)1 << ProcessorCount) - 1)
        : ~(ULONG_PTR)0;

    /* Compute the total required length for the requested relationship type */
    switch (RelationshipType)
    {
        case RelationProcessorCore:    RequiredLength = ProcessorEntrySize * PhysicalCoreCount; break;
        case RelationNumaNode:         RequiredLength = NumaEntrySize * 1;                      break;
        case RelationCache:            RequiredLength = CacheEntrySize * ValidCacheCount;        break;
        case RelationProcessorPackage: RequiredLength = ProcessorEntrySize * 1;                 break;
        case RelationGroup:            RequiredLength = GroupEntrySize * 1;                     break;
        default: ASSERT(FALSE);        RequiredLength = 0;                                      break;
    }

    WrittenLength = 0;

    _SEH2_TRY
    {
        PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX Info;

        /* Probe the output buffer */
        if (SystemInformationLength > 0 && ExGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(SystemInformation, SystemInformationLength, sizeof(ULONG));
        }

        Info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)SystemInformation;

        switch (RelationshipType)
        {
            case RelationProcessorCore:
            {
                /* One entry per physical core with its logical processor affinity mask */
                for (i = 0; i < PhysicalCoreCount; i++)
                {
                    ULONG FirstLogProc, j;
                    ULONG_PTR CoreMask;

                    if (WrittenLength + ProcessorEntrySize > SystemInformationLength)
                        break;

                    FirstLogProc = i * LogsPerCore;
                    CoreMask = 0;
                    for (j = 0; j < LogsPerCore && (FirstLogProc + j) < ProcessorCount; j++)
                        CoreMask |= (ULONG_PTR)1 << (FirstLogProc + j);

                    RtlZeroMemory(Info, ProcessorEntrySize);
                    Info->Relationship = RelationProcessorCore;
                    Info->Size = ProcessorEntrySize;
                    /* Bit 0 (LTP_PC_SMT) set when multiple logical processors share the core */
                    Info->Processor.Flags = (LogsPerCore > 1) ? 1 : 0;
                    Info->Processor.EfficiencyClass = 0;
                    Info->Processor.GroupCount = 1;
                    Info->Processor.GroupMask[0].Group = 0;
                    Info->Processor.GroupMask[0].Mask = CoreMask;

                    Info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)
                           ((PUCHAR)Info + ProcessorEntrySize);
                    WrittenLength += ProcessorEntrySize;
                }
                break;
            }

            case RelationNumaNode:
            {
                /* Single NUMA node 0 containing all processors */
                if (WrittenLength + NumaEntrySize <= SystemInformationLength)
                {
                    RtlZeroMemory(Info, NumaEntrySize);
                    Info->Relationship = RelationNumaNode;
                    Info->Size = NumaEntrySize;
                    Info->NumaNode.NodeNumber = 0;
                    Info->NumaNode.GroupMask.Group = 0;
                    Info->NumaNode.GroupMask.Mask = AllProcessorsMask;

                    Info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)
                           ((PUCHAR)Info + NumaEntrySize);
                    WrittenLength += NumaEntrySize;
                }
                break;
            }

            case RelationCache:
            {
                /* One entry per valid cache level from the PRCB cache descriptors */
                for (i = 0; i < min(Prcb0->CacheCount, ARRAYSIZE(Prcb0->Cache)); i++)
                {
                    PCACHE_DESCRIPTOR CacheDesc = &Prcb0->Cache[i];

                    if (CacheDesc->Level == 0)
                        continue;

                    if (WrittenLength + CacheEntrySize > SystemInformationLength)
                        break;

                    RtlZeroMemory(Info, CacheEntrySize);
                    Info->Relationship = RelationCache;
                    Info->Size = CacheEntrySize;
                    Info->Cache.Level        = CacheDesc->Level;
                    Info->Cache.Associativity = CacheDesc->Associativity;
                    Info->Cache.LineSize     = CacheDesc->LineSize;
                    Info->Cache.CacheSize    = CacheDesc->Size;
                    Info->Cache.Type         = CacheDesc->Type;
                    Info->Cache.GroupMask.Group = 0;
                    Info->Cache.GroupMask.Mask  = AllProcessorsMask;

                    Info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)
                           ((PUCHAR)Info + CacheEntrySize);
                    WrittenLength += CacheEntrySize;
                }
                break;
            }

            case RelationProcessorPackage:
            {
                /* Single entry for the physical package containing all processors */
                if (WrittenLength + ProcessorEntrySize <= SystemInformationLength)
                {
                    RtlZeroMemory(Info, ProcessorEntrySize);
                    Info->Relationship = RelationProcessorPackage;
                    Info->Size = ProcessorEntrySize;
                    Info->Processor.Flags = 0;
                    Info->Processor.EfficiencyClass = 0;
                    Info->Processor.GroupCount = 1;
                    Info->Processor.GroupMask[0].Group = 0;
                    Info->Processor.GroupMask[0].Mask = AllProcessorsMask;

                    Info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)
                           ((PUCHAR)Info + ProcessorEntrySize);
                    WrittenLength += ProcessorEntrySize;
                }
                break;
            }

            case RelationGroup:
            {
                /* Single entry describing the one processor group (group 0) */
                if (WrittenLength + GroupEntrySize <= SystemInformationLength)
                {
                    RtlZeroMemory(Info, GroupEntrySize);
                    Info->Relationship = RelationGroup;
                    Info->Size = GroupEntrySize;
                    /* Report 1 active and 1 maximum group */
                    Info->Group.MaximumGroupCount = 1;
                    Info->Group.ActiveGroupCount  = 1;
                    /* Fill the single PROCESSOR_GROUP_INFO for group 0 */
                    Info->Group.GroupInfo[0].MaximumProcessorCount = (UCHAR)ProcessorCount;
                    Info->Group.GroupInfo[0].ActiveProcessorCount  = (UCHAR)ProcessorCount;
                    Info->Group.GroupInfo[0].ActiveProcessorMask   = AllProcessorsMask;

                    Info = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX)
                           ((PUCHAR)Info + GroupEntrySize);
                    WrittenLength += GroupEntrySize;
                }
                break;
            }

            case RelationProcessorDie:
            case RelationNumaNodeEx:
            case RelationProcessorModule:
            case RelationAll:
                return STATUS_INVALID_PARAMETER;
        }

        /* Write the return length (always, even on STATUS_INFO_LENGTH_MISMATCH) */
        if (ReturnLength != NULL)
        {
            if (ExGetPreviousMode() != KernelMode)
                ProbeForWrite(ReturnLength, sizeof(*ReturnLength), 1);
            *ReturnLength = RequiredLength;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return (SystemInformationLength >= RequiredLength) ? STATUS_SUCCESS : STATUS_INFO_LENGTH_MISMATCH;
}

static
NTSTATUS
ExpQueryExtendedSystemProcessorCycleTimeInformation( // 108 
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    USHORT GroupNumber;
    ULONG ProcessorCount;
    ULONG CycleTimeInfoLength;
    ULONG MaxCount;
    PSYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION CycleTimeInfo;

    /* Get the group number from the input buffer */
    Status = CaptureGroupNumber(InputBuffer, InputBufferLength, &GroupNumber);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_INFO_LENGTH_MISMATCH)
            return STATUS_BUFFER_TOO_SMALL;
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (GroupNumber != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // FIXME: We don't support processor groups yet
    ProcessorCount = KeNumberProcessors;

    /* If the buffer is too small, only return the required size */
    CycleTimeInfoLength = ProcessorCount * sizeof(*CycleTimeInfo);
    if (SystemInformationLength == 0)
    {
        Status = ReturnRequiredLength(ReturnLength, CycleTimeInfoLength);
        if (Status == STATUS_INFO_LENGTH_MISMATCH)
            return STATUS_BUFFER_TOO_SMALL;
        return Status;
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
            //ProbeForWrite(SystemInformation, CycleTimeInfoLength, sizeof(ULONG));
        }

        /* Loop all processors and fill in the data */
        CycleTimeInfo = (PSYSTEM_PROCESSOR_CYCLE_TIME_INFORMATION)SystemInformation;
        MaxCount = min(SystemInformationLength / sizeof(*CycleTimeInfo), ProcessorCount);
        for (ULONG i = 0; i < MaxCount; i++)
        {
            PKPRCB Prcb = KiProcessorBlock[i];
            CycleTimeInfo[i].CycleTime = Prcb->CycleTime;
        }

        *ReturnLength = CycleTimeInfoLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (SystemInformationLength == 0)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (SystemInformationLength < CycleTimeInfoLength)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    return STATUS_SUCCESS;
}

ULONG KeNodeDistance[16]; // Max 4 NUMA nodes

static
NTSTATUS
ExpQueryExtendedSystemNodeDistanceInformation( // 121
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    USHORT NodeNumber;
    ULONG NodeDistLength;
    PULONG NodeDistInfo = (PULONG)SystemInformation;

    /* Get the NUMA node from the input buffer */
    Status = ProbeAndCaptureInputBuffer(InputBuffer,
                                        InputBufferLength,
                                        &NodeNumber,
                                        sizeof(NodeNumber),
                                        sizeof(NodeNumber));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Validate the node number */
    if (NodeNumber >= KeNumberNodes)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* If the buffer is too small, only return the required size */
    NodeDistLength = KeNumberNodes * sizeof(*NodeDistInfo);
    if (SystemInformationLength < NodeDistLength)
    {
        return ReturnRequiredLength(ReturnLength, NodeDistLength);
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
            ProbeForWrite(SystemInformation, SystemInformationLength, sizeof(ULONG));
        }

        /* Check if we have more than 1 NUMA node */
        if (KeNumberNodes == 1)
        {
            /* There is only 1 node, so nothing to calculate */
            NodeDistInfo[0] = 0;
        }
        else
        {
            /* We have multiple nodes, return the distance from the distance matrix */
            for (ULONG i = 0; i < KeNumberNodes; i++)
            {
                ULONG NodeDistanceIndex = NodeNumber * KeNumberNodes + i;
                NodeDistInfo[i] = KeNodeDistance[NodeDistanceIndex];
            }
        }

        *ReturnLength = NodeDistLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static
NTSTATUS
ExpQueryExtendedSystemProcessorPerformanceInformationEx( // 141
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    USHORT GroupNumber;
    ULONG ProcessorCount;
    ULONG PerfInfoLength;
    PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX PerfInfo;

    /* Get the group number from the input buffer */
    Status = CaptureGroupNumber(InputBuffer, InputBufferLength, &GroupNumber);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (GroupNumber != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // FIXME: We don't support processor groups yet
    ProcessorCount = KeNumberProcessors;

    /* If the buffer is too small, only return the required size */
    PerfInfoLength = ProcessorCount * sizeof(*PerfInfo);
    if (SystemInformationLength < PerfInfoLength)
    {
        return ReturnRequiredLength(ReturnLength, PerfInfoLength);
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
            ProbeForWrite(SystemInformation, PerfInfoLength, sizeof(ULONG));
        }

        /* Loop all processors and fill in the data */
        PerfInfo = (PSYSTEM_PROCESSOR_PERFORMANCE_INFORMATION_EX)SystemInformation;
        for (ULONG i = 0; i < ProcessorCount; i++)
        {
            PKPRCB Prcb = KiProcessorBlock[i];

            PerfInfo[i].IdleTime.QuadPart = Prcb->IdleThread->KernelTime;
            PerfInfo[i].KernelTime.QuadPart = Prcb->KernelTime;
            PerfInfo[i].UserTime.QuadPart = Prcb->UserTime;
            PerfInfo[i].DpcTime.QuadPart = Prcb->DpcTime;
            PerfInfo[i].InterruptTime.QuadPart = Prcb->InterruptTime;
            PerfInfo[i].InterruptCount = Prcb->InterruptCount;
            PerfInfo[i].Spare0 = 0;
            PerfInfo[i].AvailableTime.QuadPart = 0;
            PerfInfo[i].Spare1.QuadPart = 0;
            PerfInfo[i].Spare2.QuadPart = 0;
        }

        *ReturnLength = PerfInfoLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static
NTSTATUS
ExpQueryExtendedSystemProcessorCycleStatsInformation( // 160
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    USHORT GroupNumber;
    ULONG ProcessorCount;
    ULONG CycleStatsInfoLength;
    ULONG MaxCount;
    PSYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION CycleStatsInfo;

    /* Get the group number from the input buffer */
    Status = CaptureGroupNumber(InputBuffer, InputBufferLength, &GroupNumber);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_INFO_LENGTH_MISMATCH)
            return STATUS_BUFFER_TOO_SMALL;
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if (GroupNumber != 0)
    {
        return STATUS_INVALID_PARAMETER;
    }

    // FIXME: We don't support processor groups yet
    ProcessorCount = KeNumberProcessors;

    /* If the buffer is too small, only return the required size */
    CycleStatsInfoLength = ProcessorCount * sizeof(*CycleStatsInfo);

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            if (ReturnLength != NULL)
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
            //ProbeForWrite(SystemInformation, CycleStatsInfoLength, sizeof(ULONG));
        }

        /* Loop all processors and fill in the data */
        CycleStatsInfo = (PSYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION)SystemInformation;
        MaxCount = min(SystemInformationLength / sizeof(*CycleStatsInfo), ProcessorCount);
        for (ULONG i = 0; i < MaxCount; i++)
        {
            PKPRCB Prcb = KiProcessorBlock[i];
            ULONG64 CycleTime = Prcb->CycleTime;

            // There is no frequency adjustment, so put everything on the highest frequency bucket and primary efficiency class
            PSYSTEM_PROCESSOR_CYCLE_STATS_INFORMATION Current = &CycleStatsInfo[i];
            Current->Cycles[SystemProcessorCycleStatsBucketLowestFrequency][SystemProcessorCycleStatsEfficiencyClassPrimary] = 0;
            Current->Cycles[SystemProcessorCycleStatsBucketLowestFrequency][SystemProcessorCycleStatsEfficiencyClassSecondary] = 0;
            Current->Cycles[SystemProcessorCycleStatsBucketLowFrequency][SystemProcessorCycleStatsEfficiencyClassPrimary] = 0;
            Current->Cycles[SystemProcessorCycleStatsBucketLowFrequency][SystemProcessorCycleStatsEfficiencyClassSecondary] = 0;
            Current->Cycles[SystemProcessorCycleStatsBucketHighFrequency][SystemProcessorCycleStatsEfficiencyClassPrimary] = 0;
            Current->Cycles[SystemProcessorCycleStatsBucketHighFrequency][SystemProcessorCycleStatsEfficiencyClassSecondary] = 0;
            Current->Cycles[SystemProcessorCycleStatsBucketHighestFrequency][SystemProcessorCycleStatsEfficiencyClassPrimary] = CycleTime;
            Current->Cycles[SystemProcessorCycleStatsBucketHighestFrequency][SystemProcessorCycleStatsEfficiencyClassSecondary] = 0;
        }

        if (ReturnLength != NULL)
            *ReturnLength = CycleStatsInfoLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    if (SystemInformationLength == 0)
    {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (SystemInformationLength < CycleStatsInfoLength)
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    return STATUS_SUCCESS;
}

static
NTSTATUS
ExpQueryExtendedSystemIsolatedUserModeInformation( // 165
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    HANDLE InputHandle;
    PSYSTEM_ISOLATED_USER_MODE_INFORMATION IsolatedUmInfo;

    if (InputBufferLength != sizeof(InputHandle))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* The input is a pointer sized value, but its content seems to be ignored */
    Status = ProbeAndCaptureInputBuffer(InputBuffer,
                                        InputBufferLength,
                                        &InputHandle,
                                        sizeof(InputHandle),
                                        sizeof(InputHandle));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* If the buffer is too small, only return the required size */
    if (SystemInformationLength < sizeof(*IsolatedUmInfo))
    {
        return ReturnRequiredLength(ReturnLength, sizeof(*IsolatedUmInfo));
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            if (ReturnLength != NULL)
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
        }

        /* Return the info (mostly unsupported features) */
        IsolatedUmInfo = (PSYSTEM_ISOLATED_USER_MODE_INFORMATION)SystemInformation;
        IsolatedUmInfo->SecureKernelRunning = 0;
        IsolatedUmInfo->HvciEnabled = 0;
        IsolatedUmInfo->HvciStrictMode = 0;
        IsolatedUmInfo->DebugEnabled = KdDebuggerEnabled;
        IsolatedUmInfo->FirmwarePageProtection = 0;
        IsolatedUmInfo->EncryptionKeyAvailable = 0;
        IsolatedUmInfo->SpareFlags = 0;
        IsolatedUmInfo->TrustletRunning = 0;
        IsolatedUmInfo->HvciDisableAllowed = 0;
        IsolatedUmInfo->HardwareEnforcedVbs = 0;
        IsolatedUmInfo->NoSecrets = 0;
        IsolatedUmInfo->EncryptionKeyPersistent = 0;
        IsolatedUmInfo->HardwareEnforcedHvpt = 0;
        IsolatedUmInfo->HardwareHvptAvailable = 0;
        IsolatedUmInfo->SpareFlags2 = 0;
        IsolatedUmInfo->EncryptionKeyTpmBound = 0;
        RtlZeroMemory(IsolatedUmInfo->Spare0, sizeof(IsolatedUmInfo->Spare0));
        IsolatedUmInfo->Spare1 = 0;

        if (ReturnLength != NULL)
            *ReturnLength = sizeof(*IsolatedUmInfo);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static
NTSTATUS
ExpQueryExtendedSystemCpuSetInformation( // 175
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    PEPROCESS Process;
    KAFFINITY ProcessorSet;
    PSYSTEM_CPU_SET_INFORMATION CpuSetInfo;
    ULONG CpuSetInfoLength;

    if (InputBufferLength != sizeof(ProcessHandle))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* The input is a pointer sized value, but its content seems to be ignored */
    Status = ProbeAndCaptureInputBuffer(InputBuffer,
                                        InputBufferLength,
                                        &ProcessHandle,
                                        sizeof(ProcessHandle),
                                        sizeof(ProcessHandle));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Use the process handle to determine the processor set */
    if ((ProcessHandle == NULL) || (ProcessHandle == NtCurrentProcess()))
    {
        ProcessorSet = KeQueryActiveProcessors();
    }
    else
    {
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_QUERY_LIMITED_INFORMATION,
                                           PsProcessType,
                                           ExGetPreviousMode(),
                                           (PVOID*)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        ProcessorSet = Process->Pcb.ActiveProcessors;

        ObDereferenceObject(Process);
    }

    /* Calculate infomation length based on how many bits are set in ProcessorSet */
    RTL_BITMAP Bitmap;
    RtlInitializeBitMap(&Bitmap, (PULONG)&ProcessorSet, sizeof(ProcessorSet) * 8);
    ULONG NumberOfProcessors = RtlNumberOfSetBits(&Bitmap);
    CpuSetInfoLength = NumberOfProcessors * sizeof(SYSTEM_CPU_SET_INFORMATION);

    /* Validate info size */
    if (SystemInformationLength < CpuSetInfoLength)
    {
        Status = ReturnRequiredLength(ReturnLength, CpuSetInfoLength);
        if (Status == STATUS_INFO_LENGTH_MISMATCH)
            Status = STATUS_BUFFER_TOO_SMALL;
        return Status;
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            if (ReturnLength != NULL)
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
        }

        CpuSetInfo = (PSYSTEM_CPU_SET_INFORMATION)SystemInformation;
        ULONG ProcessorIndex;
        while (ProcessorSet != 0)
        {
            /* Get the processor index */
            NT_VERIFY(BitScanForwardAffinity(&ProcessorIndex, ProcessorSet) != 0);
            ASSERT(ProcessorIndex < KeNumberProcessors);
            ProcessorSet &= ~AFFINITY_MASK(ProcessorIndex);

            PKPRCB Prcb = KiProcessorBlock[ProcessorIndex];

            CpuSetInfo[ProcessorIndex].Size = sizeof(SYSTEM_CPU_SET_INFORMATION);
            CpuSetInfo[ProcessorIndex].CpuSet.Id = (0x100 | ProcessorIndex);
            CpuSetInfo[ProcessorIndex].CpuSet.Group = 0;
            CpuSetInfo[ProcessorIndex].CpuSet.LogicalProcessorIndex = ProcessorIndex;
            CpuSetInfo[ProcessorIndex].CpuSet.CoreIndex = Prcb->MultiThreadSetMaster->Number;
            CpuSetInfo[ProcessorIndex].CpuSet.LastLevelCacheIndex = 0;
            CpuSetInfo[ProcessorIndex].CpuSet.NumaNodeIndex = Prcb->ParentNode->NodeNumber;
            CpuSetInfo[ProcessorIndex].CpuSet.EfficiencyClass = 0;
            CpuSetInfo[ProcessorIndex].CpuSet.AllFlags = 0;
            CpuSetInfo[ProcessorIndex].CpuSet.SchedulingClass = 0;
            CpuSetInfo[ProcessorIndex].CpuSet.AllocationTag = 0;

            if (ReturnLength != NULL)
                *ReturnLength = CpuSetInfoLength;
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_SUCCESS;
}

static
NTSTATUS
ExpQueryExtendedSystemSecureKernelProfileInformation( // 178
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    HANDLE InputHandle;
    PSYSTEM_SECURE_KERNEL_HYPERGUARD_PROFILE_INFORMATION SecureKernelInfo;
    ULONG SecureKernelInfoLength;

    if (InputBufferLength != sizeof(InputHandle))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* The input is a pointer sized value, but its content seems to be ignored */
    Status = ProbeAndCaptureInputBuffer(InputBuffer,
                                        InputBufferLength,
                                        &InputHandle,
                                        sizeof(InputHandle),
                                        sizeof(InputHandle));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    // TODO: Check input value

    /* Validate info size */
    SecureKernelInfoLength = sizeof(*SecureKernelInfo);
    if (SystemInformationLength < SecureKernelInfoLength)
    {
        Status = ReturnRequiredLength(ReturnLength, 0);
        if (Status == STATUS_INFO_LENGTH_MISMATCH)
            Status = STATUS_BUFFER_TOO_SMALL;
        return Status;
    }

    /* Validate the caller has the system profiling privilege */
    if (!SeSinglePrivilegeCheck(SeSystemProfilePrivilege, ExGetPreviousMode()))
    {
        return STATUS_PRIVILEGE_NOT_HELD;
    }

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            if (ReturnLength != NULL)
                ProbeForWrite(ReturnLength, sizeof(ULONG), 1);
        }

        /* We don't have a secure kernel */
        if (TRUE)
        {
            if (ReturnLength != NULL)
                *ReturnLength = 0;
            return STATUS_HV_FEATURE_UNAVAILABLE;
        }

        SecureKernelInfo = (PSYSTEM_SECURE_KERNEL_HYPERGUARD_PROFILE_INFORMATION)SystemInformation;

        /* TODO: Return actual data */
        RtlSecureZeroMemory(SecureKernelInfo, sizeof(*SecureKernelInfo));

        if (ReturnLength != NULL)
            *ReturnLength = sizeof(*SecureKernelInfo);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        return _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return STATUS_HV_FEATURE_UNAVAILABLE;
}

static
NTSTATUS
ExpQueryExtendedSystemInterruptSteeringInformation( // 180
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    SYSTEM_INTERRUPT_STEERING_INFORMATION_INPUT Input;

    /* The input is a pointer sized value, but its content seems to be ignored */
    Status = ProbeAndCaptureInputBuffer(InputBuffer,
                                        InputBufferLength,
                                        &Input,
                                        1,
                                        TYPE_ALIGNMENT(SYSTEM_INTERRUPT_STEERING_INFORMATION_INPUT));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ReturnRequiredLength(ReturnLength, 0);
    if ((Status == STATUS_ACCESS_VIOLATION) || (Status == STATUS_DATATYPE_MISALIGNMENT))
    {
        return Status;
    }

    return STATUS_ACCESS_DENIED;
}

static
NTSTATUS
ExpQueryExtendedSystemSupportedProcessorArchitectures( // 181
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    HANDLE ProcessHandle;
    PEPROCESS Process;
    ULONG ArchitectureCount;
    ULONG ArchInfoLength;
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION* ArchInfo;

    if (InputBufferLength != sizeof(ProcessHandle))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the logical processor relationship type from the input buffer */
    Status = ProbeAndCaptureInputBuffer(InputBuffer,
                                        InputBufferLength,
                                        &ProcessHandle,
                                        sizeof(ProcessHandle),
                                        sizeof(ProcessHandle));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    if ((ProcessHandle == NULL) || (ProcessHandle == NtCurrentProcess()))
    {
        Process = PsGetCurrentProcess();
    }
    else
    {
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_QUERY_LIMITED_INFORMATION,
                                           PsProcessType,
                                           ExGetPreviousMode(),
                                           (PVOID*)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status))
        {
            return Status;
        }

        //USHORT Machine = Process->Wow64Process->Wow64.Machine;
    }

#ifdef _M_AMD64
    ArchitectureCount = 2;
#else
    ArchitectureCount = 1;
#endif

    ArchInfoLength = (ArchitectureCount + 1) * sizeof(SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION);
    if (SystemInformationLength < ArchInfoLength)
    {
        Status = ReturnRequiredLength(ReturnLength, ArchInfoLength);
        if (Status == STATUS_INFO_LENGTH_MISMATCH)
            Status = STATUS_BUFFER_TOO_SMALL;
        goto Exit;
    }

    // FIXME: What to do with the process?

    _SEH2_TRY
    {
        if (ExGetPreviousMode() != KernelMode)
        {
            ProbeForWrite(ReturnLength, sizeof(ULONG), sizeof(ULONG));
            ProbeForWrite(SystemInformation, ArchInfoLength, sizeof(ULONG));
        }

        ArchInfo = (SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION*)SystemInformation;
        RtlZeroMemory(ArchInfo, ArchInfoLength);

        /* Fill in native info */
        ArchInfo[0].Machine = IMAGE_FILE_MACHINE_NATIVE;
        ArchInfo[0].KernelMode = 1;
        ArchInfo[0].UserMode = 1;
        ArchInfo[0].Native = 1;
        ArchInfo[0].Process = 0;
        ArchInfo[0].WoW64Container = 0;
        ArchInfo[0].ReservedZero0 = 0;

#ifdef _M_AMD64
        /* Fill in WOW64 info */
        ArchInfo[1].Machine = IMAGE_FILE_MACHINE_I386;
        ArchInfo[1].KernelMode = 0;
        ArchInfo[1].UserMode = 1;
        ArchInfo[1].Native = 0;
        ArchInfo[1].Process = 0;
        ArchInfo[1].WoW64Container = 0;
        ArchInfo[1].ReservedZero0 = 0;
#endif

        *ReturnLength = ArchInfoLength;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    Status = STATUS_SUCCESS;

Exit:

    if ((ProcessHandle != NULL) && (ProcessHandle != NtCurrentProcess()))
    {
        ObDereferenceObject(Process);
    }

    return Status;
}

static
NTSTATUS
ExpQueryExtendedSystemCodeIntegritySyntheticCacheInformation( // 209
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;
    ULONG64 Input = 0;

    /* The input is a pointer sized value, but its content seems to be ignored */
    Status = ProbeAndCaptureInputBuffer(InputBuffer,
                                        InputBufferLength,
                                        &Input,
                                        1,
                                        TYPE_ALIGNMENT(ULONG64));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ProbeOutputBuffer(SystemInformation, SystemInformationLength, sizeof(ULONG));
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    Status = ReturnRequiredLength(ReturnLength, 0);
    if ((Status == STATUS_ACCESS_VIOLATION) || (Status == STATUS_DATATYPE_MISALIGNMENT))
    {
        return Status;
    }

    return STATUS_ACCESS_DENIED;
}

static
NTSTATUS
ExpQueryExtendedSystemFeatureConfigurationInformation( // 210
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    //SYSTEM_FEATURE_CONFIGURATION_QUERY FeatureQuery = { 0 };

    return STATUS_NOT_IMPLEMENTED;
}

static
NTSTATUS
ExpQueryExtendedSystemFeatureConfigurationSectionInformation( // 211
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    return STATUS_NOT_IMPLEMENTED;
}

__kernel_entry
NTSYSCALLAPI
NTSTATUS
NTAPI
NtQuerySystemInformationEx(
    _In_ SYSTEM_INFORMATION_CLASS SystemInformationClass,
    _In_reads_bytes_opt_(InputBufferLength) PVOID InputBuffer,
    _In_ ULONG InputBufferLength,
    _Out_writes_bytes_to_opt_(SystemInformationLength, *ReturnLength) PVOID SystemInformation,
    _In_ ULONG SystemInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    NTSTATUS Status;

    /* Make sure we have an input buffer */
    if ((InputBuffer == NULL) || (InputBufferLength == 0))
    {
        return STATUS_INVALID_PARAMETER;
    }

    switch (SystemInformationClass)
    {
        case SystemProcessorPerformanceInformation: // 8
            Status = ExpQueryExtendedSystemProcessorPerformanceInformation(InputBuffer,
                                                                           InputBufferLength,
                                                                           SystemInformation,
                                                                           SystemInformationLength,
                                                                           ReturnLength);
            break;

        case SystemInterruptInformation: // 23
            Status = ExpQueryExtendedSystemInterruptInformation(InputBuffer,
                                                                InputBufferLength,
                                                                SystemInformation,
                                                                SystemInformationLength,
                                                                ReturnLength);
            break;

        case SystemProcessorIdleInformation: // 42
            Status = ExpQueryExtendedSystemProcessorIdleInformation(InputBuffer,
                                                                    InputBufferLength,
                                                                    SystemInformation,
                                                                    SystemInformationLength,
                                                                    ReturnLength);
            break;

        case SystemProcessorPowerInformation: // 61
            Status = ExpQueryExtendedSystemProcessorPowerInformation(InputBuffer,
                                                                    InputBufferLength,
                                                                    SystemInformation,
                                                                    SystemInformationLength,
                                                                    ReturnLength);
            break;

        case SystemWatchdogTimerInformation: // 72
            Status = ExpQueryExtendedSystemWatchdogTimerInformation(InputBuffer,
                                                                    InputBufferLength,
                                                                    SystemInformation,
                                                                    SystemInformationLength,
                                                                    ReturnLength);
            break;

        case SystemLogicalProcessorInformation: // 73
            Status = ExpQueryExtendedSystemLogicalProcessorInformation(InputBuffer,
                                                                       InputBufferLength,
                                                                       SystemInformation,
                                                                       SystemInformationLength,
                                                                       ReturnLength);
            break;

        case SystemProcessorIdleCycleTimeInformation: // 83
            Status = ExpQueryExtendedSystemProcessorIdleCycleTimeInformation(InputBuffer,
                                                                    InputBufferLength,
                                                                    SystemInformation,
                                                                    SystemInformationLength,
                                                                    ReturnLength);
            break;

        case SystemProcessorPerformanceDistribution: // 100
            Status = ExpQueryExtendedSystemProcessorPerformanceDistribution(InputBuffer,
                                                                            InputBufferLength,
                                                                            SystemInformation,
                                                                            SystemInformationLength,
                                                                            ReturnLength);
            break;

        case SystemLogicalProcessorAndGroupInformation: // 107
            Status = ExpQueryExtendedSystemLogicalProcessorAndGroupInformation(InputBuffer,
                                                                               InputBufferLength,
                                                                               SystemInformation,
                                                                               SystemInformationLength,
                                                                               ReturnLength);
            break;

        case SystemProcessorCycleTimeInformation: // 108
            Status = ExpQueryExtendedSystemProcessorCycleTimeInformation(InputBuffer,
                                                                         InputBufferLength,
                                                                         SystemInformation,
                                                                         SystemInformationLength,
                                                                         ReturnLength);
            break;

        case SystemNodeDistanceInformation: // 121
            Status = ExpQueryExtendedSystemNodeDistanceInformation(InputBuffer,
                                                                   InputBufferLength,
                                                                   SystemInformation,
                                                                   SystemInformationLength,
                                                                   ReturnLength);
            break;

        case SystemProcessorPerformanceInformationEx: // 141
            Status = ExpQueryExtendedSystemProcessorPerformanceInformationEx(InputBuffer,
                                                                             InputBufferLength,
                                                                             SystemInformation,
                                                                             SystemInformationLength,
                                                                             ReturnLength);
            break;

        case SystemProcessorCycleStatsInformation: // 160
            Status = ExpQueryExtendedSystemProcessorCycleStatsInformation(InputBuffer,
                                                                          InputBufferLength,
                                                                          SystemInformation,
                                                                          SystemInformationLength,
                                                                          ReturnLength);
            break;

        case SystemIsolatedUserModeInformation: // 165
            Status = ExpQueryExtendedSystemIsolatedUserModeInformation(InputBuffer,
                                                                       InputBufferLength,
                                                                       SystemInformation,
                                                                       SystemInformationLength,
                                                                       ReturnLength);
            break;

        case SystemCpuSetInformation: // 175
            Status = ExpQueryExtendedSystemCpuSetInformation(InputBuffer,
                                                             InputBufferLength,
                                                             SystemInformation,
                                                             SystemInformationLength,
                                                             ReturnLength);
            break;

        case SystemSecureKernelProfileInformation: // 178
            Status = ExpQueryExtendedSystemSecureKernelProfileInformation(InputBuffer,
                                                                          InputBufferLength,
                                                                          SystemInformation,
                                                                          SystemInformationLength,
                                                                          ReturnLength);
            break;

        case SystemInterruptSteeringInformation: // 180
            Status = ExpQueryExtendedSystemInterruptSteeringInformation(InputBuffer,
                                                                        InputBufferLength,
                                                                        SystemInformation,
                                                                        SystemInformationLength,
                                                                        ReturnLength);
            break;

        case SystemSupportedProcessorArchitectures: // 181
            Status = ExpQueryExtendedSystemSupportedProcessorArchitectures(InputBuffer,
                                                                           InputBufferLength,
                                                                           SystemInformation,
                                                                           SystemInformationLength,
                                                                           ReturnLength);
            break;

        case SystemCodeIntegritySyntheticCacheInformation: // 209
            Status = ExpQueryExtendedSystemCodeIntegritySyntheticCacheInformation(InputBuffer,
                                                                                  InputBufferLength,
                                                                                  SystemInformation,
                                                                                  SystemInformationLength,
                                                                                  ReturnLength);
            break;

        case SystemFeatureConfigurationInformation: // 210
            Status = ExpQueryExtendedSystemFeatureConfigurationInformation(InputBuffer,
                                                                           InputBufferLength,
                                                                           SystemInformation,
                                                                           SystemInformationLength,
                                                                           ReturnLength);
            break;

        case SystemFeatureConfigurationSectionInformation: // 211
            Status = ExpQueryExtendedSystemFeatureConfigurationSectionInformation(InputBuffer,
                                                                                  InputBufferLength,
                                                                                  SystemInformation,
                                                                                  SystemInformationLength,
                                                                                  ReturnLength);
            break;


        default:
            return STATUS_INVALID_INFO_CLASS;
    }

    return Status;
}
