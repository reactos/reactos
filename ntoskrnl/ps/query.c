/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/query.c
 * PURPOSE:         Process Manager: Thread/Process Query/Set Information
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 *                  Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* Debugging Level */
ULONG PspTraceLevel = 0;

/* PRIVATE FUNCTIONS *********************************************************/

NTSTATUS
NTAPI
PsReferenceProcessFilePointer(_In_ PEPROCESS Process, _Outptr_ PFILE_OBJECT *FileObject)
{
    PSECTION Section;
    PAGED_CODE();

    /* Lock the process */
    if (!ExAcquireRundownProtection(&Process->RundownProtect))
    {
        return STATUS_PROCESS_IS_TERMINATING;
    }

    /* Get the section */
    Section = Process->SectionObject;
    if (Section)
    {
        /* Get the file object and reference it */
        *FileObject = MmGetFileObjectForSection((PVOID)Section);
        ObReferenceObject(*FileObject);
    }

    /* Release the protection */
    ExReleaseRundownProtection(&Process->RundownProtect);

    /* Return status */
    return Section ? STATUS_SUCCESS : STATUS_UNSUCCESSFUL;
}

#if DBG
static PCSTR
PspDumpProcessInfoClassName(_In_ PROCESSINFOCLASS ProcessInformationClass)
{
    static CHAR UnknownClassName[11];

#define DBG_PROCESS_INFO_CLASS(InfoClass) [InfoClass] = #InfoClass
    static const PCSTR ProcessInfoClasses[] = {
        DBG_PROCESS_INFO_CLASS(ProcessBasicInformation),
        DBG_PROCESS_INFO_CLASS(ProcessQuotaLimits),
        DBG_PROCESS_INFO_CLASS(ProcessVmCounters),
        DBG_PROCESS_INFO_CLASS(ProcessTimes),
        DBG_PROCESS_INFO_CLASS(ProcessBasePriority),
        DBG_PROCESS_INFO_CLASS(ProcessRaisePriority),
        DBG_PROCESS_INFO_CLASS(ProcessDebugPort),
        DBG_PROCESS_INFO_CLASS(ProcessExceptionPort),
        DBG_PROCESS_INFO_CLASS(ProcessAccessToken),
        DBG_PROCESS_INFO_CLASS(ProcessLdtInformation),
        DBG_PROCESS_INFO_CLASS(ProcessLdtSize),
        DBG_PROCESS_INFO_CLASS(ProcessDefaultHardErrorMode),
        DBG_PROCESS_INFO_CLASS(ProcessIoPortHandlers),
        DBG_PROCESS_INFO_CLASS(ProcessPooledUsageAndLimits),
        DBG_PROCESS_INFO_CLASS(ProcessWorkingSetWatch),
        DBG_PROCESS_INFO_CLASS(ProcessUserModeIOPL),
        DBG_PROCESS_INFO_CLASS(ProcessEnableAlignmentFaultFixup),
        DBG_PROCESS_INFO_CLASS(ProcessPriorityClass),
        DBG_PROCESS_INFO_CLASS(ProcessWx86Information),
        DBG_PROCESS_INFO_CLASS(ProcessHandleCount),
        DBG_PROCESS_INFO_CLASS(ProcessAffinityMask),
        DBG_PROCESS_INFO_CLASS(ProcessPriorityBoost),
        DBG_PROCESS_INFO_CLASS(ProcessDeviceMap),
        DBG_PROCESS_INFO_CLASS(ProcessSessionInformation),
        DBG_PROCESS_INFO_CLASS(ProcessForegroundInformation),
        DBG_PROCESS_INFO_CLASS(ProcessWow64Information),
        DBG_PROCESS_INFO_CLASS(ProcessImageFileName),
        DBG_PROCESS_INFO_CLASS(ProcessLUIDDeviceMapsEnabled),
        DBG_PROCESS_INFO_CLASS(ProcessBreakOnTermination),
        DBG_PROCESS_INFO_CLASS(ProcessDebugObjectHandle),
        DBG_PROCESS_INFO_CLASS(ProcessDebugFlags),
        DBG_PROCESS_INFO_CLASS(ProcessHandleTracing),
        DBG_PROCESS_INFO_CLASS(ProcessIoPriority),
        DBG_PROCESS_INFO_CLASS(ProcessExecuteFlags),
        DBG_PROCESS_INFO_CLASS(ProcessTlsInformation),
        DBG_PROCESS_INFO_CLASS(ProcessCookie),
        DBG_PROCESS_INFO_CLASS(ProcessImageInformation),
        DBG_PROCESS_INFO_CLASS(ProcessCycleTime),
        DBG_PROCESS_INFO_CLASS(ProcessPagePriority),
        DBG_PROCESS_INFO_CLASS(ProcessInstrumentationCallback),
        DBG_PROCESS_INFO_CLASS(ProcessThreadStackAllocation),
        DBG_PROCESS_INFO_CLASS(ProcessWorkingSetWatchEx),
        DBG_PROCESS_INFO_CLASS(ProcessImageFileNameWin32),
        DBG_PROCESS_INFO_CLASS(ProcessImageFileMapping),
        DBG_PROCESS_INFO_CLASS(ProcessAffinityUpdateMode),
        DBG_PROCESS_INFO_CLASS(ProcessMemoryAllocationMode),
        DBG_PROCESS_INFO_CLASS(ProcessGroupInformation),
        DBG_PROCESS_INFO_CLASS(ProcessConsoleHostProcess),
        DBG_PROCESS_INFO_CLASS(ProcessWindowInformation),
    };
#undef DBG_PROCESS_INFO_CLASS

    if (ProcessInformationClass < RTL_NUMBER_OF(ProcessInfoClasses))
    {
        return ProcessInfoClasses[ProcessInformationClass];
    }

    RtlStringCchPrintfA(UnknownClassName, _countof(UnknownClassName), "%lu", ProcessInformationClass);
    return UnknownClassName;
}

static PCSTR
PspDumpThreadInfoClassName(_In_ THREADINFOCLASS ThreadInformationClass)
{
    static CHAR UnknownClassName[11];

#define DBG_THREAD_INFO_CLASS(InfoClass) [InfoClass] = #InfoClass
    static const PCSTR ThreadInfoClasses[] = {
        DBG_THREAD_INFO_CLASS(ThreadBasicInformation),
        DBG_THREAD_INFO_CLASS(ThreadTimes),
        DBG_THREAD_INFO_CLASS(ThreadPriority),
        DBG_THREAD_INFO_CLASS(ThreadBasePriority),
        DBG_THREAD_INFO_CLASS(ThreadAffinityMask),
        DBG_THREAD_INFO_CLASS(ThreadImpersonationToken),
        DBG_THREAD_INFO_CLASS(ThreadDescriptorTableEntry),
        DBG_THREAD_INFO_CLASS(ThreadEnableAlignmentFaultFixup),
        DBG_THREAD_INFO_CLASS(ThreadEventPair_Reusable),
        DBG_THREAD_INFO_CLASS(ThreadQuerySetWin32StartAddress),
        DBG_THREAD_INFO_CLASS(ThreadZeroTlsCell),
        DBG_THREAD_INFO_CLASS(ThreadPerformanceCount),
        DBG_THREAD_INFO_CLASS(ThreadAmILastThread),
        DBG_THREAD_INFO_CLASS(ThreadIdealProcessor),
        DBG_THREAD_INFO_CLASS(ThreadPriorityBoost),
        DBG_THREAD_INFO_CLASS(ThreadSetTlsArrayAddress),
        DBG_THREAD_INFO_CLASS(ThreadIsIoPending),
        DBG_THREAD_INFO_CLASS(ThreadHideFromDebugger),
        DBG_THREAD_INFO_CLASS(ThreadBreakOnTermination),
        DBG_THREAD_INFO_CLASS(ThreadSwitchLegacyState),
        DBG_THREAD_INFO_CLASS(ThreadIsTerminated),
        DBG_THREAD_INFO_CLASS(ThreadLastSystemCall),
        DBG_THREAD_INFO_CLASS(ThreadIoPriority),
        DBG_THREAD_INFO_CLASS(ThreadCycleTime),
        DBG_THREAD_INFO_CLASS(ThreadPagePriority),
        DBG_THREAD_INFO_CLASS(ThreadActualBasePriority),
        DBG_THREAD_INFO_CLASS(ThreadTebInformation),
        DBG_THREAD_INFO_CLASS(ThreadCSwitchMon),
        DBG_THREAD_INFO_CLASS(ThreadCSwitchPmu),
        DBG_THREAD_INFO_CLASS(ThreadWow64Context),
        DBG_THREAD_INFO_CLASS(ThreadGroupInformation),
        DBG_THREAD_INFO_CLASS(ThreadUmsInformation),
        DBG_THREAD_INFO_CLASS(ThreadCounterProfiling),
        DBG_THREAD_INFO_CLASS(ThreadIdealProcessorEx),
        DBG_THREAD_INFO_CLASS(ThreadCpuAccountingInformation),
        DBG_THREAD_INFO_CLASS(ThreadSuspendCount),
        DBG_THREAD_INFO_CLASS(ThreadHeterogeneousCpuPolicy),
        DBG_THREAD_INFO_CLASS(ThreadContainerId),
        DBG_THREAD_INFO_CLASS(ThreadNameInformation),
        DBG_THREAD_INFO_CLASS(ThreadSelectedCpuSets),
        DBG_THREAD_INFO_CLASS(ThreadSystemThreadInformation),
        DBG_THREAD_INFO_CLASS(ThreadActualGroupAffinity),
        DBG_THREAD_INFO_CLASS(ThreadDynamicCodePolicyInfo),
        DBG_THREAD_INFO_CLASS(ThreadExplicitCaseSensitivity),
        DBG_THREAD_INFO_CLASS(ThreadWorkOnBehalfTicket),
        DBG_THREAD_INFO_CLASS(ThreadSubsystemInformation),
        DBG_THREAD_INFO_CLASS(ThreadDbgkWerReportActive),
        DBG_THREAD_INFO_CLASS(ThreadAttachContainer),
        DBG_THREAD_INFO_CLASS(ThreadManageWritesToExecutableMemory),
        DBG_THREAD_INFO_CLASS(ThreadPowerThrottlingState),
        DBG_THREAD_INFO_CLASS(ThreadWorkloadClass),
        DBG_THREAD_INFO_CLASS(ThreadCreateStateChange),
        DBG_THREAD_INFO_CLASS(ThreadApplyStateChange),
        DBG_THREAD_INFO_CLASS(ThreadStrongerBadHandleChecks),
        DBG_THREAD_INFO_CLASS(ThreadEffectiveIoPriority),
        DBG_THREAD_INFO_CLASS(ThreadEffectivePagePriority),
    };
#undef DBG_THREAD_INFO_CLASS

    if (ThreadInformationClass < RTL_NUMBER_OF(ThreadInfoClasses))
    {
        return ThreadInfoClasses[ThreadInformationClass];
    }

    RtlStringCchPrintfA(UnknownClassName, _countof(UnknownClassName), "%lu", ThreadInformationClass);
    return UnknownClassName;
}
#endif // #if DBG

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_writes_bytes_to_opt_(ProcessInformationLength, *ReturnLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    ULONG Length = 0;

    PAGED_CODE();

    /* Validate the information class */
    Status = DefaultQueryInfoBufferCheck(
        ProcessInformationClass, PsProcessInfoClass, RTL_NUMBER_OF(PsProcessInfoClass), ICIF_PROBE_READ,
        ProcessInformation, ProcessInformationLength, ReturnLength, NULL, PreviousMode);
    if (!NT_SUCCESS(Status))
    {
#if DBG
        DPRINT1(
            "NtQueryInformationProcess(ProcessInformationClass: %s): Class validation failed! (Status: 0x%lx)\n",
            PspDumpProcessInfoClassName(ProcessInformationClass), Status);
#endif
        return Status;
    }

    if (((ProcessInformationClass == ProcessCookie) || (ProcessInformationClass == ProcessImageInformation)) &&
        (ProcessHandle != NtCurrentProcess()))
    {
        /*
         * Retrieving the process cookie is only allowed for the calling process
         * itself! XP only allows NtCurrentProcess() as process handles even if
         * a real handle actually represents the current process.
         */
        return STATUS_INVALID_PARAMETER;
    }

    /* Check the information class */
    switch (ProcessInformationClass)
    {
        /* Basic process information */
        case ProcessBasicInformation:
        {
            PPROCESS_BASIC_INFORMATION ProcessBasicInfo = (PPROCESS_BASIC_INFORMATION)ProcessInformation;

            if (ProcessInformationLength != sizeof(PROCESS_BASIC_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length */
            Length = sizeof(PROCESS_BASIC_INFORMATION);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Protect writes with SEH */
            _SEH2_TRY
            {
                /* Write all the information from the EPROCESS/KPROCESS */
                ProcessBasicInfo->ExitStatus = Process->ExitStatus;
                ProcessBasicInfo->PebBaseAddress = Process->Peb;
                ProcessBasicInfo->AffinityMask = Process->Pcb.Affinity;
                ProcessBasicInfo->UniqueProcessId = (ULONG_PTR)Process->UniqueProcessId;
                ProcessBasicInfo->InheritedFromUniqueProcessId = (ULONG_PTR)Process->InheritedFromUniqueProcessId;
                ProcessBasicInfo->BasePriority = Process->Pcb.BasePriority;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;
        }

        /* Process quota limits */
        case ProcessQuotaLimits:
        {
            QUOTA_LIMITS_EX QuotaLimits;
            BOOLEAN Extended;

            if (ProcessInformationLength != sizeof(QUOTA_LIMITS) && ProcessInformationLength != sizeof(QUOTA_LIMITS_EX))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length */
            Length = ProcessInformationLength;
            Extended = (Length == sizeof(QUOTA_LIMITS_EX));

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Indicate success */
            Status = STATUS_SUCCESS;

            RtlZeroMemory(&QuotaLimits, sizeof(QuotaLimits));

            /* Get max/min working set sizes */
            QuotaLimits.MaximumWorkingSetSize = Process->Vm.MaximumWorkingSetSize << PAGE_SHIFT;
            QuotaLimits.MinimumWorkingSetSize = Process->Vm.MinimumWorkingSetSize << PAGE_SHIFT;

            /* Get default time limits */
            QuotaLimits.TimeLimit.QuadPart = -1LL;

            /* Is quota block a default one? */
            if (Process->QuotaBlock == &PspDefaultQuotaBlock)
            {
                /* Get default pools and pagefile limits */
                QuotaLimits.PagedPoolLimit = (SIZE_T)-1;
                QuotaLimits.NonPagedPoolLimit = (SIZE_T)-1;
                QuotaLimits.PagefileLimit = (SIZE_T)-1;
            }
            else
            {
                /* Get limits from non-default quota block */
                QuotaLimits.PagedPoolLimit = Process->QuotaBlock->QuotaEntry[PsPagedPool].Limit;
                QuotaLimits.NonPagedPoolLimit = Process->QuotaBlock->QuotaEntry[PsNonPagedPool].Limit;
                QuotaLimits.PagefileLimit = Process->QuotaBlock->QuotaEntry[PsPageFile].Limit;
            }

            /* Get additional information, if needed */
            if (Extended)
            {
                QuotaLimits.Flags |=
                    (Process->Vm.Flags.MaximumWorkingSetHard ? QUOTA_LIMITS_HARDWS_MAX_ENABLE
                                                             : QUOTA_LIMITS_HARDWS_MAX_DISABLE);
                QuotaLimits.Flags |=
                    (Process->Vm.Flags.MinimumWorkingSetHard ? QUOTA_LIMITS_HARDWS_MIN_ENABLE
                                                             : QUOTA_LIMITS_HARDWS_MIN_DISABLE);

                /* FIXME: Get the correct information */
                // QuotaLimits.WorkingSetLimit = (SIZE_T)-1; // Not used on Win2k3, it is set to 0
                QuotaLimits.CpuRateLimit.RateData = 0;
            }

            /* Protect writes with SEH */
            _SEH2_TRY
            {
                RtlCopyMemory(ProcessInformation, &QuotaLimits, Length);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;
        }

        case ProcessIoCounters:
        {
            PIO_COUNTERS IoCounters = (PIO_COUNTERS)ProcessInformation;
            PROCESS_VALUES ProcessValues;

            if (ProcessInformationLength != sizeof(IO_COUNTERS))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            Length = sizeof(IO_COUNTERS);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Query IO counters from the process */
            KeQueryValuesProcess(&Process->Pcb, &ProcessValues);

            _SEH2_TRY
            {
                RtlCopyMemory(IoCounters, &ProcessValues.IoInfo, sizeof(IO_COUNTERS));
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Ignore exception */
            }
            _SEH2_END;

            /* Set status to success in any case */
            Status = STATUS_SUCCESS;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;
        }

        /* Timing */
        case ProcessTimes:
        {
            PKERNEL_USER_TIMES ProcessTime = (PKERNEL_USER_TIMES)ProcessInformation;
            ULONG UserTime, KernelTime;

            /* Set the return length */
            if (ProcessInformationLength != sizeof(KERNEL_USER_TIMES))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            Length = sizeof(KERNEL_USER_TIMES);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Protect writes with SEH */
            _SEH2_TRY
            {
                /* Copy time information from EPROCESS/KPROCESS */
                KernelTime = KeQueryRuntimeProcess(&Process->Pcb, &UserTime);
                ProcessTime->CreateTime = Process->CreateTime;
                ProcessTime->UserTime.QuadPart = (LONGLONG)UserTime * KeMaximumIncrement;
                ProcessTime->KernelTime.QuadPart = (LONGLONG)KernelTime * KeMaximumIncrement;
                ProcessTime->ExitTime = Process->ExitTime;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;
        }

        /* Process Debug Port */
        case ProcessDebugPort:

            if (ProcessInformationLength != sizeof(HANDLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length */
            Length = sizeof(HANDLE);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Protect write with SEH */
            _SEH2_TRY
            {
                /* Return whether or not we have a debug port */
                *(PHANDLE)ProcessInformation = (Process->DebugPort ? (HANDLE)-1 : NULL);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;

        case ProcessHandleCount:
        {
            ULONG HandleCount;

            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length*/
            Length = sizeof(ULONG);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Count the number of handles this process has */
            HandleCount = ObGetProcessHandleCount(Process);

            /* Protect write in SEH */
            _SEH2_TRY
            {
                /* Return the count of handles */
                *(PULONG)ProcessInformation = HandleCount;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;
        }

        /* Session ID for the process */
        case ProcessSessionInformation:
        {
            PPROCESS_SESSION_INFORMATION SessionInfo = (PPROCESS_SESSION_INFORMATION)ProcessInformation;

            if (ProcessInformationLength != sizeof(PROCESS_SESSION_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length*/
            Length = sizeof(PROCESS_SESSION_INFORMATION);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Enter SEH for write safety */
            _SEH2_TRY
            {
                /* Write back the Session ID */
                SessionInfo->SessionId = PsGetProcessSessionId(Process);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;
        }

        /* Virtual Memory Statistics */
        case ProcessVmCounters:
        {
            PVM_COUNTERS VmCounters = (PVM_COUNTERS)ProcessInformation;

            /* Validate the input length */
            if ((ProcessInformationLength != sizeof(VM_COUNTERS)) &&
                (ProcessInformationLength != sizeof(VM_COUNTERS_EX)))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Enter SEH for write safety */
            _SEH2_TRY
            {
                /* Return data from EPROCESS */
                VmCounters->PeakVirtualSize = Process->PeakVirtualSize;
                VmCounters->VirtualSize = Process->VirtualSize;
                VmCounters->PageFaultCount = Process->Vm.PageFaultCount;
                VmCounters->PeakWorkingSetSize = Process->Vm.PeakWorkingSetSize;
                VmCounters->WorkingSetSize = Process->Vm.WorkingSetSize;
                VmCounters->QuotaPeakPagedPoolUsage = Process->QuotaPeak[PsPagedPool];
                VmCounters->QuotaPagedPoolUsage = Process->QuotaUsage[PsPagedPool];
                VmCounters->QuotaPeakNonPagedPoolUsage = Process->QuotaPeak[PsNonPagedPool];
                VmCounters->QuotaNonPagedPoolUsage = Process->QuotaUsage[PsNonPagedPool];
                VmCounters->PagefileUsage = Process->QuotaUsage[PsPageFile] << PAGE_SHIFT;
                VmCounters->PeakPagefileUsage = Process->QuotaPeak[PsPageFile] << PAGE_SHIFT;
                // VmCounters->PrivateUsage = Process->CommitCharge << PAGE_SHIFT;
                //

                /* Set the return length */
                Length = ProcessInformationLength;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;
        }

        /* Hard Error Processing Mode */
        case ProcessDefaultHardErrorMode:

            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length*/
            Length = sizeof(ULONG);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Enter SEH for writing back data */
            _SEH2_TRY
            {
                /* Write the current processing mode */
                *(PULONG)ProcessInformation = Process->DefaultHardErrorProcessing;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;

        /* Priority Boosting status */
        case ProcessPriorityBoost:

            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length */
            Length = sizeof(ULONG);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Enter SEH for writing back data */
            _SEH2_TRY
            {
                /* Return boost status */
                *(PULONG)ProcessInformation = Process->Pcb.DisableBoost ? TRUE : FALSE;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;

        /* DOS Device Map */
        case ProcessDeviceMap:
        {
            ULONG Flags;

            if (ProcessInformationLength == sizeof(PROCESS_DEVICEMAP_INFORMATION_EX))
            {
                /* Protect read in SEH */
                _SEH2_TRY
                {
                    PPROCESS_DEVICEMAP_INFORMATION_EX DeviceMapEx = ProcessInformation;

                    Flags = DeviceMapEx->Flags;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* Get the exception code */
                    Status = _SEH2_GetExceptionCode();
                    _SEH2_YIELD(break);
                }
                _SEH2_END;

                /* Only one flag is supported and it needs LUID mappings */
                if ((Flags & ~PROCESS_LUID_DOSDEVICES_ONLY) != 0 || !ObIsLUIDDeviceMapsEnabled())
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
            }
            else
            {
                /* This has to be the size of the Query union field for x64 compatibility! */
                if (ProcessInformationLength != RTL_FIELD_SIZE(PROCESS_DEVICEMAP_INFORMATION, Query))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                    break;
                }

                /* No flags for standard call */
                Flags = 0;
            }

            /* Set the return length */
            Length = ProcessInformationLength;

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Query the device map information */
            Status = ObQueryDeviceMapInformation(Process, ProcessInformation, Flags);

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;
        }

        /* Priority class */
        case ProcessPriorityClass:
        {
            PPROCESS_PRIORITY_CLASS PsPriorityClass = (PPROCESS_PRIORITY_CLASS)ProcessInformation;

            if (ProcessInformationLength != sizeof(PROCESS_PRIORITY_CLASS))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length*/
            Length = sizeof(PROCESS_PRIORITY_CLASS);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Enter SEH for writing back data */
            _SEH2_TRY
            {
                /* Return current priority class */
                PsPriorityClass->PriorityClass = Process->PriorityClass;
                PsPriorityClass->Foreground = FALSE;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;
        }

        case ProcessImageFileName:
        {
            PUNICODE_STRING ImageName;

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Get the image path */
            Status = SeLocateProcessImageName(Process, &ImageName);
            if (NT_SUCCESS(Status))
            {
                /* Set the return length */
                Length = ImageName->MaximumLength + sizeof(OBJECT_NAME_INFORMATION);

                /* Make sure it's large enough */
                if (Length <= ProcessInformationLength)
                {
                    /* Enter SEH to protect write */
                    _SEH2_TRY
                    {
                        /* Copy it */
                        RtlCopyMemory(ProcessInformation, ImageName, Length);

                        /* Update pointer */
                        ((PUNICODE_STRING)ProcessInformation)->Buffer =
                            (PWSTR)((PUNICODE_STRING)ProcessInformation + 1);
                    }
                    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                    {
                        /* Get the exception code */
                        Status = _SEH2_GetExceptionCode();
                    }
                    _SEH2_END;
                }
                else
                {
                    /* Buffer too small */
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }

                /* Free the image path */
                ExFreePoolWithTag(ImageName, TAG_SEPA);
            }
            /* Dereference the process */
            ObDereferenceObject(Process);
            break;
        }

#if (NTDDI_VERSION >= NTDDI_VISTA) || (DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA)
        case ProcessImageFileNameWin32:
        {
            PFILE_OBJECT FileObject;
            POBJECT_NAME_INFORMATION ObjectNameInformation;

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle,
                PROCESS_QUERY_INFORMATION, // FIXME: Use PROCESS_QUERY_LIMITED_INFORMATION if implemented
                PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
            {
                break;
            }

            /* Get the image path */
            Status = PsReferenceProcessFilePointer(Process, &FileObject);
            ObDereferenceObject(Process);
            if (!NT_SUCCESS(Status))
            {
                break;
            }
            Status = IoQueryFileDosDeviceName(FileObject, &ObjectNameInformation);
            ObDereferenceObject(FileObject);
            if (!NT_SUCCESS(Status))
            {
                break;
            }

            /* Determine return length and output */
            Length = sizeof(UNICODE_STRING) + ObjectNameInformation->Name.MaximumLength;
            if (Length <= ProcessInformationLength)
            {
                _SEH2_TRY
                {
                    PUNICODE_STRING ImageName = (PUNICODE_STRING)ProcessInformation;
                    ImageName->Length = ObjectNameInformation->Name.Length;
                    ImageName->MaximumLength = ObjectNameInformation->Name.MaximumLength;
                    if (ObjectNameInformation->Name.MaximumLength)
                    {
                        ImageName->Buffer = (PWSTR)(ImageName + 1);
                        RtlCopyMemory(
                            ImageName->Buffer, ObjectNameInformation->Name.Buffer,
                            ObjectNameInformation->Name.MaximumLength);
                    }
                    else
                    {
                        ASSERT(ImageName->Length == 0);
                        ImageName->Buffer = NULL;
                    }
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
            else
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
            }
            ExFreePool(ObjectNameInformation);

            break;
        }
#endif /* (NTDDI_VERSION >= NTDDI_VISTA) || (DLL_EXPORT_VERSION >= _WIN32_WINNT_VISTA) */

        case ProcessDebugFlags:

            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length*/
            Length = sizeof(ULONG);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Enter SEH for writing back data */
            _SEH2_TRY
            {
                /* Return the debug flag state */
                *(PULONG)ProcessInformation = Process->NoDebugInherit ? 0 : 1;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;

        case ProcessBreakOnTermination:

            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length */
            Length = sizeof(ULONG);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Enter SEH for writing back data */
            _SEH2_TRY
            {
                /* Return the BreakOnTermination state */
                *(PULONG)ProcessInformation = Process->BreakOnTermination;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;

        /* Per-process security cookie */
        case ProcessCookie:
        {
            ULONG Cookie;

            if (ProcessInformationLength != sizeof(ULONG))
            {
                /* Length size wrong, bail out */
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Get the current process and cookie */
            Process = PsGetCurrentProcess();
            Cookie = Process->Cookie;
            if (!Cookie)
            {
                LARGE_INTEGER SystemTime;
                ULONG NewCookie;
                PKPRCB Prcb;

                /* Generate a new cookie */
                KeQuerySystemTime(&SystemTime);
                Prcb = KeGetCurrentPrcb();
                NewCookie = Prcb->KeSystemCalls ^ Prcb->InterruptTime ^ SystemTime.u.LowPart ^ SystemTime.u.HighPart;

                /* Set the new cookie or return the current one */
                Cookie = InterlockedCompareExchange((LONG *)&Process->Cookie, NewCookie, Cookie);
                if (!Cookie)
                    Cookie = NewCookie;

                /* Set the return length */
                Length = sizeof(ULONG);
            }

            /* Indicate success */
            Status = STATUS_SUCCESS;

            /* Enter SEH to protect write */
            _SEH2_TRY
            {
                /* Write back the cookie */
                *(PULONG)ProcessInformation = Cookie;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case ProcessImageInformation:

            if (ProcessInformationLength != sizeof(SECTION_IMAGE_INFORMATION))
            {
                /* Break out */
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the length required and validate it */
            Length = sizeof(SECTION_IMAGE_INFORMATION);

            /* Indicate success */
            Status = STATUS_SUCCESS;

            /* Enter SEH to protect write */
            _SEH2_TRY
            {
                MmGetImageInformation((PSECTION_IMAGE_INFORMATION)ProcessInformation);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;

        case ProcessDebugObjectHandle:
        {
            HANDLE DebugPort = NULL;

            if (ProcessInformationLength != sizeof(HANDLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length */
            Length = sizeof(HANDLE);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Get the debug port. Continue even if this fails. */
            Status = DbgkOpenProcessDebugPort(Process, PreviousMode, &DebugPort);

            /* Let go of the process */
            ObDereferenceObject(Process);

            /* Protect write in SEH */
            _SEH2_TRY
            {
                /* Return debug port's handle */
                *(PHANDLE)ProcessInformation = DebugPort;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                if (DebugPort)
                    ObCloseHandle(DebugPort, PreviousMode);

                /* Get the exception code.
                 * Note: This overwrites any previous failure status. */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case ProcessHandleTracing:
            DPRINT1("Handle tracing not implemented: %lu\n", ProcessInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case ProcessLUIDDeviceMapsEnabled:

            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length */
            Length = sizeof(ULONG);

            /* Indicate success */
            Status = STATUS_SUCCESS;

            /* Protect write in SEH */
            _SEH2_TRY
            {
                /* Query Ob */
                *(PULONG)ProcessInformation = ObIsLUIDDeviceMapsEnabled();
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;

        case ProcessWx86Information:

            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length */
            Length = sizeof(ULONG);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Protect write in SEH */
            _SEH2_TRY
            {
                /* Return if the flag is set */
                *(PULONG)ProcessInformation = (ULONG)Process->VdmAllowed;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the process */
            ObDereferenceObject(Process);
            break;

        case ProcessWow64Information:
        {
            ULONG_PTR Wow64 = 0;

            if (ProcessInformationLength != sizeof(ULONG_PTR))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length */
            Length = sizeof(ULONG_PTR);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(
                ProcessHandle, PROCESS_QUERY_INFORMATION, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
            if (!NT_SUCCESS(Status))
                break;

#ifdef _WIN64
            /* Make sure the process isn't dying */
            if (ExAcquireRundownProtection(&Process->RundownProtect))
            {
                /* Get the WOW64 process structure */
                Wow64 = (ULONG_PTR)Process->Wow64Process;
                /* Release the lock */
                ExReleaseRundownProtection(&Process->RundownProtect);
            }
#endif

            /* Dereference the process */
            ObDereferenceObject(Process);

            /* Protect write with SEH */
            _SEH2_TRY
            {
                /* Return the Wow64 process information */
                *(PULONG_PTR)ProcessInformation = Wow64;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case ProcessExecuteFlags:
        {
            ULONG ExecuteOptions = 0;

            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length */
            Length = sizeof(ULONG);

            if (ProcessHandle != NtCurrentProcess())
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Get the options */
            Status = MmGetExecuteOptions(&ExecuteOptions);
            if (NT_SUCCESS(Status))
            {
                /* Protect write with SEH */
                _SEH2_TRY
                {
                    /* Return them */
                    *(PULONG)ProcessInformation = ExecuteOptions;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* Get exception code */
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;
            }
            break;
        }

        case ProcessLdtInformation:
            DPRINT1("VDM/16-bit not implemented: %lu\n", ProcessInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case ProcessWorkingSetWatch:
            DPRINT1("WS Watch not implemented: %lu\n", ProcessInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case ProcessPooledUsageAndLimits:
            DPRINT1("Pool limits not implemented: %lu\n", ProcessInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        /* Not supported by Server 2003 */
        default:
#if DBG
            DPRINT1("Unsupported info class: %s\n", PspDumpProcessInfoClassName(ProcessInformationClass));
#endif
            Status = STATUS_INVALID_INFO_CLASS;
    }

    /* Check if caller wants the return length and if there is one */
    if (ReturnLength != NULL && Length != 0)
    {
        /* Protect write with SEH */
        _SEH2_TRY
        {
            *ReturnLength = Length;
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Get exception code.
             * Note: This overwrites any previous failure status. */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _In_reads_bytes_(ProcessInformationLength) PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength)
{
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ACCESS_MASK Access;
    NTSTATUS Status;
    HANDLE PortHandle = NULL;
    HANDLE TokenHandle = NULL;
    HANDLE DirectoryHandle = NULL;
    PROCESS_SESSION_INFORMATION SessionInfo = {0};
    PROCESS_PRIORITY_CLASS PriorityClass = {0};
    PROCESS_FOREGROUND_BACKGROUND Foreground = {0};
    PVOID ExceptionPort;
    ULONG Break;
    KAFFINITY ValidAffinity, Affinity = 0;
    KPRIORITY BasePriority = 0;
    UCHAR MemoryPriority = 0;
    BOOLEAN DisableBoost = 0;
    ULONG DefaultHardErrorMode = 0;
    ULONG DebugFlags = 0, EnableFixup = 0, Boost = 0;
    ULONG NoExecute = 0, VdmPower = 0;
    BOOLEAN HasPrivilege;
    PLIST_ENTRY Next;
    PETHREAD Thread;
    PAGED_CODE();

    /* Validate the information class */
    Status = DefaultSetInfoBufferCheck(
        ProcessInformationClass, PsProcessInfoClass, RTL_NUMBER_OF(PsProcessInfoClass), ProcessInformation,
        ProcessInformationLength, PreviousMode);
    if (!NT_SUCCESS(Status))
    {
#if DBG
        DPRINT1(
            "NtSetInformationProcess(ProcessInformationClass: %s): Class validation failed! (Status: 0x%lx)\n",
            PspDumpProcessInfoClassName(ProcessInformationClass), Status);
#endif
        return Status;
    }

    /* Check what class this is */
    Access = PROCESS_SET_INFORMATION;
    if (ProcessInformationClass == ProcessSessionInformation)
    {
        /* Setting the Session ID needs a special mask */
        Access |= PROCESS_SET_SESSIONID;
    }
    else if (ProcessInformationClass == ProcessExceptionPort)
    {
        /* Setting the exception port needs a special mask */
        Access |= PROCESS_SUSPEND_RESUME;
    }

    /* Reference the process */
    Status = ObReferenceObjectByHandle(ProcessHandle, Access, PsProcessType, PreviousMode, (PVOID *)&Process, NULL);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Check what kind of information class this is */
    switch (ProcessInformationClass)
    {
        case ProcessWx86Information:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Use SEH for capture */
            _SEH2_TRY
            {
                /* Capture the boolean */
                VdmPower = *(PULONG)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Getting VDM powers requires the SeTcbPrivilege */
            if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
            {
                /* We don't hold the privilege, bail out */
                Status = STATUS_PRIVILEGE_NOT_HELD;
                DPRINT1("Need TCB privilege\n");
                break;
            }

            /* Set or clear the flag */
            if (VdmPower)
            {
                PspSetProcessFlag(Process, PSF_VDM_ALLOWED_BIT);
            }
            else
            {
                PspClearProcessFlag(Process, PSF_VDM_ALLOWED_BIT);
            }
            break;

        /* Error/Exception Port */
        case ProcessExceptionPort:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(HANDLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Use SEH for capture */
            _SEH2_TRY
            {
                /* Capture the handle */
                PortHandle = *(PHANDLE)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Setting the error port requires the SeTcbPrivilege */
            if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
            {
                /* We don't hold the privilege, bail out */
                Status = STATUS_PRIVILEGE_NOT_HELD;
                break;
            }

            /* Get the LPC Port */
            Status =
                ObReferenceObjectByHandle(PortHandle, 0, LpcPortObjectType, PreviousMode, (PVOID)&ExceptionPort, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Change the pointer */
            if (InterlockedCompareExchangePointer(&Process->ExceptionPort, ExceptionPort, NULL))
            {
                /* We already had one, fail */
                ObDereferenceObject(ExceptionPort);
                Status = STATUS_PORT_ALREADY_SET;
            }
            break;

        /* Security Token */
        case ProcessAccessToken:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(PROCESS_ACCESS_TOKEN))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Use SEH for capture */
            _SEH2_TRY
            {
                /* Save the token handle */
                TokenHandle = ((PPROCESS_ACCESS_TOKEN)ProcessInformation)->Token;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Assign the actual token */
            Status = PspSetPrimaryToken(Process, TokenHandle, NULL);
            break;

        /* Hard error processing */
        case ProcessDefaultHardErrorMode:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Enter SEH for direct buffer read */
            _SEH2_TRY
            {
                DefaultHardErrorMode = *(PULONG)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Set the mode */
            Process->DefaultHardErrorProcessing = DefaultHardErrorMode;

            /* Call Ke for the update */
            if (DefaultHardErrorMode & SEM_NOALIGNMENTFAULTEXCEPT)
            {
                KeSetAutoAlignmentProcess(&Process->Pcb, TRUE);
            }
            else
            {
                KeSetAutoAlignmentProcess(&Process->Pcb, FALSE);
            }
            Status = STATUS_SUCCESS;
            break;

        /* Session ID */
        case ProcessSessionInformation:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(PROCESS_SESSION_INFORMATION))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Enter SEH for capture */
            _SEH2_TRY
            {
                /* Capture the caller's buffer */
                SessionInfo = *(PPROCESS_SESSION_INFORMATION)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Setting the session id requires the SeTcbPrivilege */
            if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
            {
                /* We don't hold the privilege, bail out */
                Status = STATUS_PRIVILEGE_NOT_HELD;
                break;
            }

            /*
             * Since we cannot change the session ID of the given
             * process anymore because it is set once and for all
             * at process creation time and because it is stored
             * inside the Process->Session structure managed by MM,
             * we fake changing it: we just return success if the
             * user-defined value is the same as the session ID of
             * the process, and otherwise we fail.
             */
            if (SessionInfo.SessionId == PsGetProcessSessionId(Process))
            {
                Status = STATUS_SUCCESS;
            }
            else
            {
                Status = STATUS_ACCESS_DENIED;
            }

            break;

        case ProcessPriorityClass:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(PROCESS_PRIORITY_CLASS))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Enter SEH for capture */
            _SEH2_TRY
            {
                /* Capture the caller's buffer */
                PriorityClass = *(PPROCESS_PRIORITY_CLASS)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Return the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Check for invalid PriorityClass value */
            if (PriorityClass.PriorityClass > PROCESS_PRIORITY_CLASS_ABOVE_NORMAL)
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            if ((PriorityClass.PriorityClass != Process->PriorityClass) &&
                (PriorityClass.PriorityClass == PROCESS_PRIORITY_CLASS_REALTIME))
            {
                /* Check the privilege */
                HasPrivilege = SeCheckPrivilegedObject(
                    SeIncreaseBasePriorityPrivilege, ProcessHandle, PROCESS_SET_INFORMATION, PreviousMode);
                if (!HasPrivilege)
                {
                    ObDereferenceObject(Process);
                    DPRINT1("Privilege to change priority to realtime lacking\n");
                    return STATUS_PRIVILEGE_NOT_HELD;
                }
            }

            /* Check if we have a job */
            if (Process->Job)
            {
                DPRINT1("Jobs not yet supported\n");
            }

            /* Set process priority class */
            Process->PriorityClass = PriorityClass.PriorityClass;

            /* Set process priority mode (foreground or background) */
            PsSetProcessPriorityByClass(
                Process, PriorityClass.Foreground ? PsProcessPriorityForeground : PsProcessPriorityBackground);
            Status = STATUS_SUCCESS;
            break;

        case ProcessForegroundInformation:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(PROCESS_FOREGROUND_BACKGROUND))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Enter SEH for capture */
            _SEH2_TRY
            {
                /* Capture the caller's buffer */
                Foreground = *(PPROCESS_FOREGROUND_BACKGROUND)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Return the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Set process priority mode (foreground or background) */
            PsSetProcessPriorityByClass(
                Process, Foreground.Foreground ? PsProcessPriorityForeground : PsProcessPriorityBackground);
            Status = STATUS_SUCCESS;
            break;

        case ProcessBasePriority:

            /* Validate input length */
            if (ProcessInformationLength != sizeof(KPRIORITY))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Enter SEH for direct buffer read */
            _SEH2_TRY
            {
                BasePriority = *(KPRIORITY *)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Break = 0;
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Extract the memory priority out of there */
            if (BasePriority & 0x80000000)
            {
                MemoryPriority = MEMORY_PRIORITY_FOREGROUND;
                BasePriority &= ~0x80000000;
            }
            else
            {
                MemoryPriority = MEMORY_PRIORITY_BACKGROUND;
            }

            /* Validate the number */
            if ((BasePriority > HIGH_PRIORITY) || (BasePriority <= LOW_PRIORITY))
            {
                ObDereferenceObject(Process);
                return STATUS_INVALID_PARAMETER;
            }

            /* Check if the new base is higher */
            if (BasePriority > Process->Pcb.BasePriority)
            {
                HasPrivilege = SeCheckPrivilegedObject(
                    SeIncreaseBasePriorityPrivilege, ProcessHandle, PROCESS_SET_INFORMATION, PreviousMode);
                if (!HasPrivilege)
                {
                    ObDereferenceObject(Process);
                    DPRINT1(
                        "Privilege to change priority from %lx to %lx lacking\n", Process->Pcb.BasePriority,
                        BasePriority);
                    return STATUS_PRIVILEGE_NOT_HELD;
                }
            }

            /* Call Ke */
            KeSetPriorityAndQuantumProcess(&Process->Pcb, BasePriority, 0);

            /* Now set the memory priority */
            MmSetMemoryPriorityProcess(Process, MemoryPriority);
            Status = STATUS_SUCCESS;
            break;

        case ProcessRaisePriority:

            /* Validate input length */
            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Enter SEH for direct buffer read */
            _SEH2_TRY
            {
                Boost = *(PULONG)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Break = 0;
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Make sure the process isn't dying */
            if (ExAcquireRundownProtection(&Process->RundownProtect))
            {
                /* Lock it */
                KeEnterCriticalRegion();
                ExAcquirePushLockShared(&Process->ProcessLock);

                /* Loop the threads */
                for (Next = Process->ThreadListHead.Flink; Next != &Process->ThreadListHead; Next = Next->Flink)
                {
                    /* Call Ke for the thread */
                    Thread = CONTAINING_RECORD(Next, ETHREAD, ThreadListEntry);
                    KeBoostPriorityThread(&Thread->Tcb, Boost);
                }

                /* Release the lock and rundown */
                ExReleasePushLockShared(&Process->ProcessLock);
                KeLeaveCriticalRegion();
                ExReleaseRundownProtection(&Process->RundownProtect);

                /* Set success code */
                Status = STATUS_SUCCESS;
            }
            else
            {
                /* Avoid race conditions */
                Status = STATUS_PROCESS_IS_TERMINATING;
            }
            break;

        case ProcessBreakOnTermination:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Enter SEH for direct buffer read */
            _SEH2_TRY
            {
                Break = *(PULONG)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Break = 0;
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Setting 'break on termination' requires the SeDebugPrivilege */
            if (!SeSinglePrivilegeCheck(SeDebugPrivilege, PreviousMode))
            {
                /* We don't hold the privilege, bail out */
                Status = STATUS_PRIVILEGE_NOT_HELD;
                break;
            }

            /* Set or clear the flag */
            if (Break)
            {
                PspSetProcessFlag(Process, PSF_BREAK_ON_TERMINATION_BIT);
            }
            else
            {
                PspClearProcessFlag(Process, PSF_BREAK_ON_TERMINATION_BIT);
            }

            break;

        case ProcessAffinityMask:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(KAFFINITY))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Enter SEH for direct buffer read */
            _SEH2_TRY
            {
                Affinity = *(PKAFFINITY)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Break = 0;
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Make sure it's valid for the CPUs present */
            ValidAffinity = Affinity & KeActiveProcessors;
            if (!Affinity || (ValidAffinity != Affinity))
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Check if it's within job affinity limits */
            if (Process->Job)
            {
                /* Not yet implemented */
                UNIMPLEMENTED;
                Status = STATUS_NOT_IMPLEMENTED;
                break;
            }

            /* Make sure the process isn't dying */
            if (ExAcquireRundownProtection(&Process->RundownProtect))
            {
                /* Lock it */
                KeEnterCriticalRegion();
                ExAcquirePushLockShared(&Process->ProcessLock);

                /* Call Ke to do the work */
                KeSetAffinityProcess(&Process->Pcb, ValidAffinity);

                /* Release the lock and rundown */
                ExReleasePushLockShared(&Process->ProcessLock);
                KeLeaveCriticalRegion();
                ExReleaseRundownProtection(&Process->RundownProtect);

                /* Set success code */
                Status = STATUS_SUCCESS;
            }
            else
            {
                /* Avoid race conditions */
                Status = STATUS_PROCESS_IS_TERMINATING;
            }
            break;

        /* Priority Boosting status */
        case ProcessPriorityBoost:

            /* Validate input length */
            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Enter SEH for direct buffer read */
            _SEH2_TRY
            {
                DisableBoost = *(PBOOLEAN)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Break = 0;
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Make sure the process isn't dying */
            if (ExAcquireRundownProtection(&Process->RundownProtect))
            {
                /* Lock it */
                KeEnterCriticalRegion();
                ExAcquirePushLockShared(&Process->ProcessLock);

                /* Call Ke to do the work */
                KeSetDisableBoostProcess(&Process->Pcb, DisableBoost);

                /* Loop the threads too */
                for (Next = Process->ThreadListHead.Flink; Next != &Process->ThreadListHead; Next = Next->Flink)
                {
                    /* Call Ke for the thread */
                    Thread = CONTAINING_RECORD(Next, ETHREAD, ThreadListEntry);
                    KeSetDisableBoostThread(&Thread->Tcb, DisableBoost);
                }

                /* Release the lock and rundown */
                ExReleasePushLockShared(&Process->ProcessLock);
                KeLeaveCriticalRegion();
                ExReleaseRundownProtection(&Process->RundownProtect);

                /* Set success code */
                Status = STATUS_SUCCESS;
            }
            else
            {
                /* Avoid race conditions */
                Status = STATUS_PROCESS_IS_TERMINATING;
            }
            break;

        case ProcessDebugFlags:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Enter SEH for direct buffer read */
            _SEH2_TRY
            {
                DebugFlags = *(PULONG)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Set the mode */
            if (DebugFlags & ~1)
            {
                Status = STATUS_INVALID_PARAMETER;
            }
            else
            {
                if (DebugFlags & 1)
                {
                    PspClearProcessFlag(Process, PSF_NO_DEBUG_INHERIT_BIT);
                }
                else
                {
                    PspSetProcessFlag(Process, PSF_NO_DEBUG_INHERIT_BIT);
                }
            }

            /* Done */
            Status = STATUS_SUCCESS;
            break;

        case ProcessEnableAlignmentFaultFixup:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(BOOLEAN))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Enter SEH for direct buffer read */
            _SEH2_TRY
            {
                EnableFixup = *(PULONG)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Set the mode */
            if (EnableFixup)
            {
                Process->DefaultHardErrorProcessing |= SEM_NOALIGNMENTFAULTEXCEPT;
            }
            else
            {
                Process->DefaultHardErrorProcessing &= ~SEM_NOALIGNMENTFAULTEXCEPT;
            }

            /* Call Ke for the update */
            KeSetAutoAlignmentProcess(&Process->Pcb, FALSE);
            Status = STATUS_SUCCESS;
            break;

        case ProcessUserModeIOPL:

            /* Only TCB can do this */
            if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
            {
                /* We don't hold the privilege, bail out */
                DPRINT1("Need TCB to set IOPL\n");
                Status = STATUS_PRIVILEGE_NOT_HELD;
                break;
            }

            /* Only supported on x86 */
#if defined(_X86_)
            Ke386SetIOPL();
#elif defined(_M_AMD64)
            /* On x64 this function isn't implemented.
               On Windows 2003 it returns success.
               On Vista+ it returns STATUS_NOT_IMPLEMENTED. */
            if ((ExGetPreviousMode() != KernelMode) && (RtlRosGetAppcompatVersion() > _WIN32_WINNT_WS03))
            {
                Status = STATUS_NOT_IMPLEMENTED;
            }
#else
            Status = STATUS_NOT_IMPLEMENTED;
#endif
            /* Done */
            break;

        case ProcessExecuteFlags:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            if (ProcessHandle != NtCurrentProcess())
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Enter SEH for direct buffer read */
            _SEH2_TRY
            {
                NoExecute = *(PULONG)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Call Mm for the update */
            Status = MmSetExecuteOptions(NoExecute);
            break;

        case ProcessDeviceMap:

            /* Check buffer length */
            if (ProcessInformationLength != sizeof(HANDLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Use SEH for capture */
            _SEH2_TRY
            {
                /* Capture the handle */
                DirectoryHandle = *(PHANDLE)ProcessInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Call Ob to set the device map */
            Status = ObSetDeviceMap(Process, DirectoryHandle);
            break;

        /* We currently don't implement any of these */
        case ProcessLdtInformation:
        case ProcessLdtSize:
        case ProcessIoPortHandlers:
            DPRINT1("VDM/16-bit Request not implemented: %lu\n", ProcessInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case ProcessQuotaLimits:

            Status = PspSetQuotaLimits(Process, 1, ProcessInformation, ProcessInformationLength, PreviousMode);
            break;

        case ProcessWorkingSetWatch:
            DPRINT1("WS watch not implemented\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case ProcessHandleTracing:
            DPRINT1("Handle tracing not implemented\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        /* Anything else is invalid */
        default:
#if DBG
            DPRINT1("Invalid Server 2003 Info Class: %s\n", PspDumpProcessInfoClassName(ProcessInformationClass));
#endif
            Status = STATUS_INVALID_INFO_CLASS;
    }

    /* Dereference and return status */
    ObDereferenceObject(Process);
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _In_reads_bytes_(ThreadInformationLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength)
{
    PETHREAD Thread;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    HANDLE TokenHandle = NULL;
    KPRIORITY Priority = 0;
    KAFFINITY Affinity = 0, CombinedAffinity;
    PVOID Address = NULL;
    PEPROCESS Process;
    ULONG_PTR DisableBoost = 0;
    ULONG_PTR IdealProcessor = 0;
    ULONG_PTR Break = 0;
    PTEB Teb;
    ULONG_PTR TlsIndex = 0;
    PVOID *ExpansionSlots;
    PETHREAD ProcThread;
    BOOLEAN HasPrivilege;
    PAGED_CODE();

    /* Validate the information class */
    Status = DefaultSetInfoBufferCheck(
        ThreadInformationClass, PsThreadInfoClass, RTL_NUMBER_OF(PsThreadInfoClass), ThreadInformation,
        ThreadInformationLength, PreviousMode);
    if (!NT_SUCCESS(Status))
    {
#if DBG
        DPRINT1(
            "NtSetInformationThread(ThreadInformationClass: %s): Class validation failed! (Status: 0x%lx)\n",
            PspDumpThreadInfoClassName(ThreadInformationClass), Status);
#endif
        return Status;
    }

    /* Check what kind of information class this is */
    switch (ThreadInformationClass)
    {
        /* Thread priority */
        case ThreadPriority:

            /* Check buffer length */
            if (ThreadInformationLength != sizeof(KPRIORITY))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Use SEH for capture */
            _SEH2_TRY
            {
                /* Get the priority */
                Priority = *(PLONG)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Validate it */
            if ((Priority > HIGH_PRIORITY) || (Priority <= LOW_PRIORITY))
            {
                /* Fail */
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Check for the required privilege */
            if (Priority >= LOW_REALTIME_PRIORITY)
            {
                HasPrivilege = SeCheckPrivilegedObject(
                    SeIncreaseBasePriorityPrivilege, ThreadHandle, THREAD_SET_INFORMATION, PreviousMode);
                if (!HasPrivilege)
                {
                    DPRINT1("Privilege to change priority to %lx lacking\n", Priority);
                    return STATUS_PRIVILEGE_NOT_HELD;
                }
            }

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(
                ThreadHandle, THREAD_SET_INFORMATION, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Set the priority */
            KeSetPriorityThread(&Thread->Tcb, Priority);

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadBasePriority:

            /* Check buffer length */
            if (ThreadInformationLength != sizeof(LONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Use SEH for capture */
            _SEH2_TRY
            {
                /* Get the priority */
                Priority = *(PLONG)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Validate it */
            if ((Priority > THREAD_BASE_PRIORITY_MAX) || (Priority < THREAD_BASE_PRIORITY_MIN))
            {
                /* These ones are OK */
                if ((Priority != THREAD_BASE_PRIORITY_LOWRT + 1) && (Priority != THREAD_BASE_PRIORITY_IDLE - 1))
                {
                    /* Check if the process is real time */
                    if (PsGetCurrentProcess()->PriorityClass != PROCESS_PRIORITY_CLASS_REALTIME)
                    {
                        /* It isn't, fail */
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                }
            }

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(
                ThreadHandle, THREAD_SET_INFORMATION, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Set the base priority */
            KeSetBasePriorityThread(&Thread->Tcb, Priority);

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadAffinityMask:

            /* Check buffer length */
            if (ThreadInformationLength != sizeof(ULONG_PTR))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Use SEH for capture */
            _SEH2_TRY
            {
                /* Get the priority */
                Affinity = *(PULONG_PTR)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Validate it */
            if (!Affinity)
            {
                /* Fail */
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(
                ThreadHandle, THREAD_SET_INFORMATION, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Get the process */
            Process = Thread->ThreadsProcess;

            /* Try to acquire rundown */
            if (ExAcquireRundownProtection(&Process->RundownProtect))
            {
                /* Lock it */
                KeEnterCriticalRegion();
                ExAcquirePushLockShared(&Process->ProcessLock);

                /* Combine masks */
                CombinedAffinity = Affinity & Process->Pcb.Affinity;
                if (CombinedAffinity != Affinity)
                {
                    /* Fail */
                    Status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    /* Set the affinity */
                    KeSetAffinityThread(&Thread->Tcb, CombinedAffinity);
                }

                /* Release the lock and rundown */
                ExReleasePushLockShared(&Process->ProcessLock);
                KeLeaveCriticalRegion();
                ExReleaseRundownProtection(&Process->RundownProtect);
            }
            else
            {
                /* Too late */
                Status = STATUS_PROCESS_IS_TERMINATING;
            }

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadImpersonationToken:

            /* Check buffer length */
            if (ThreadInformationLength != sizeof(HANDLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Use SEH for capture */
            _SEH2_TRY
            {
                /* Save the token handle */
                TokenHandle = *(PHANDLE)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(
                ThreadHandle, THREAD_SET_THREAD_TOKEN, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Assign the actual token */
            Status = PsAssignImpersonationToken(Thread, TokenHandle);

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadQuerySetWin32StartAddress:

            /* Check buffer length */
            if (ThreadInformationLength != sizeof(ULONG_PTR))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Use SEH for capture */
            _SEH2_TRY
            {
                /* Get the priority */
                Address = *(PVOID *)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(
                ThreadHandle, THREAD_SET_INFORMATION, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Set the address */
            Thread->Win32StartAddress = Address;

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadIdealProcessor:

            /* Check buffer length */
            if (ThreadInformationLength != sizeof(ULONG_PTR))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Use SEH for capture */
            _SEH2_TRY
            {
                /* Get the priority */
                IdealProcessor = *(PULONG_PTR)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Validate it */
            if (IdealProcessor > MAXIMUM_PROCESSORS)
            {
                /* Fail */
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(
                ThreadHandle, THREAD_SET_INFORMATION, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Set the ideal */
            Status = KeSetIdealProcessorThread(&Thread->Tcb, (CCHAR)IdealProcessor);

            /* Get the TEB and protect the thread */
            Teb = Thread->Tcb.Teb;
            if ((Teb) && (ExAcquireRundownProtection(&Thread->RundownProtect)))
            {
                /* Save the ideal processor */
                Teb->IdealProcessor = Thread->Tcb.IdealProcessor;

                /* Release rundown protection */
                ExReleaseRundownProtection(&Thread->RundownProtect);
            }

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadPriorityBoost:

            /* Check buffer length */
            if (ThreadInformationLength != sizeof(ULONG_PTR))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Use SEH for capture */
            _SEH2_TRY
            {
                /* Get the priority */
                DisableBoost = *(PULONG_PTR)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(
                ThreadHandle, THREAD_SET_INFORMATION, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Call the kernel */
            KeSetDisableBoostThread(&Thread->Tcb, (BOOLEAN)DisableBoost);

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadZeroTlsCell:

            /* Check buffer length */
            if (ThreadInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Use SEH for capture */
            _SEH2_TRY
            {
                /* Get the priority */
                TlsIndex = *(PULONG)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(
                ThreadHandle, THREAD_SET_INFORMATION, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* This is only valid for the current thread */
            if (Thread != PsGetCurrentThread())
            {
                /* Fail */
                Status = STATUS_INVALID_PARAMETER;
                ObDereferenceObject(Thread);
                break;
            }

            /* Get the process */
            Process = Thread->ThreadsProcess;

            /* Loop the threads */
            ProcThread = PsGetNextProcessThread(Process, NULL);
            while (ProcThread)
            {
                /* Acquire rundown */
                if (ExAcquireRundownProtection(&ProcThread->RundownProtect))
                {
                    /* Get the TEB */
                    Teb = ProcThread->Tcb.Teb;
                    if (Teb)
                    {
                        /* Check if we're in the expansion range */
                        if (TlsIndex > TLS_MINIMUM_AVAILABLE - 1)
                        {
                            if (TlsIndex < (TLS_MINIMUM_AVAILABLE + TLS_EXPANSION_SLOTS) - 1)
                            {
                                /* Check if we have expansion slots */
                                ExpansionSlots = Teb->TlsExpansionSlots;
                                if (ExpansionSlots)
                                {
                                    /* Clear the index */
                                    ExpansionSlots[TlsIndex - TLS_MINIMUM_AVAILABLE] = 0;
                                }
                            }
                        }
                        else
                        {
                            /* Clear the index */
                            Teb->TlsSlots[TlsIndex] = NULL;
                        }
                    }

                    /* Release rundown */
                    ExReleaseRundownProtection(&ProcThread->RundownProtect);
                }

                /* Go to the next thread */
                ProcThread = PsGetNextProcessThread(Process, ProcThread);
            }

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadBreakOnTermination:

            /* Check buffer length */
            if (ThreadInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Enter SEH for direct buffer read */
            _SEH2_TRY
            {
                Break = *(PULONG)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Break = 0;
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Setting 'break on termination' requires the SeDebugPrivilege */
            if (!SeSinglePrivilegeCheck(SeDebugPrivilege, PreviousMode))
            {
                /* We don't hold the privilege, bail out */
                Status = STATUS_PRIVILEGE_NOT_HELD;
                break;
            }

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(
                ThreadHandle, THREAD_SET_INFORMATION, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Set or clear the flag */
            if (Break)
            {
                PspSetCrossThreadFlag(Thread, CT_BREAK_ON_TERMINATION_BIT);
            }
            else
            {
                PspClearCrossThreadFlag(Thread, CT_BREAK_ON_TERMINATION_BIT);
            }

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadHideFromDebugger:

            /* Check buffer length */
            if (ThreadInformationLength != 0)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(
                ThreadHandle, THREAD_SET_INFORMATION, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Set the flag */
            PspSetCrossThreadFlag(Thread, CT_HIDE_FROM_DEBUGGER_BIT);

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        /* Anything else */
        default:
            /* Not yet implemented */
#if DBG
            DPRINT1("Not implemented: %s\n", PspDumpThreadInfoClassName(ThreadInformationClass));
#endif
            Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryInformationThread(
    _In_ HANDLE ThreadHandle,
    _In_ THREADINFOCLASS ThreadInformationClass,
    _Out_writes_bytes_to_opt_(ThreadInformationLength, *ReturnLength) PVOID ThreadInformation,
    _In_ ULONG ThreadInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    PETHREAD Thread;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    ULONG Access;
    ULONG Length = 0;
    PTHREAD_BASIC_INFORMATION ThreadBasicInfo = (PTHREAD_BASIC_INFORMATION)ThreadInformation;
    PKERNEL_USER_TIMES ThreadTime = (PKERNEL_USER_TIMES)ThreadInformation;
    KIRQL OldIrql;
    ULONG ThreadTerminated;
    PAGED_CODE();

    /* Validate the information class */
    Status = DefaultQueryInfoBufferCheck(
        ThreadInformationClass, PsThreadInfoClass, RTL_NUMBER_OF(PsThreadInfoClass), ICIF_PROBE_READ, ThreadInformation,
        ThreadInformationLength, ReturnLength, NULL, PreviousMode);
    if (!NT_SUCCESS(Status))
    {
#if DBG
        DPRINT1(
            "NtQueryInformationThread(ThreadInformationClass: %s): Class validation failed! (Status: 0x%lx)\n",
            PspDumpThreadInfoClassName(ThreadInformationClass), Status);
#endif
        return Status;
    }

    /* Check what class this is */
    Access = THREAD_QUERY_INFORMATION;

    /* Check what kind of information class this is */
    switch (ThreadInformationClass)
    {
        /* Basic thread information */
        case ThreadBasicInformation:

            /* Set the return length */
            Length = sizeof(THREAD_BASIC_INFORMATION);

            if (ThreadInformationLength != Length)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the thread */
            Status =
                ObReferenceObjectByHandle(ThreadHandle, Access, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Protect writes with SEH */
            _SEH2_TRY
            {
                /* Write all the information from the ETHREAD/KTHREAD */
                ThreadBasicInfo->ExitStatus = Thread->ExitStatus;
                ThreadBasicInfo->TebBaseAddress = (PVOID)Thread->Tcb.Teb;
                ThreadBasicInfo->ClientId = Thread->Cid;
                ThreadBasicInfo->AffinityMask = Thread->Tcb.Affinity;
                ThreadBasicInfo->Priority = Thread->Tcb.Priority;
                ThreadBasicInfo->BasePriority = KeQueryBasePriorityThread(&Thread->Tcb);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        /* Thread time information */
        case ThreadTimes:

            /* Set the return length */
            Length = sizeof(KERNEL_USER_TIMES);

            if (ThreadInformationLength != Length)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the thread */
            Status =
                ObReferenceObjectByHandle(ThreadHandle, Access, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Protect writes with SEH */
            _SEH2_TRY
            {
                /* Copy time information from ETHREAD/KTHREAD */
                ThreadTime->KernelTime.QuadPart = Thread->Tcb.KernelTime * KeMaximumIncrement;
                ThreadTime->UserTime.QuadPart = Thread->Tcb.UserTime * KeMaximumIncrement;
                ThreadTime->CreateTime = Thread->CreateTime;

                /* Exit time is in a union and only valid on actual exit! */
                if (KeReadStateThread(&Thread->Tcb))
                {
                    ThreadTime->ExitTime = Thread->ExitTime;
                }
                else
                {
                    ThreadTime->ExitTime.QuadPart = 0;
                }
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadQuerySetWin32StartAddress:

            /* Set the return length*/
            Length = sizeof(PVOID);

            if (ThreadInformationLength != Length)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the thread */
            Status =
                ObReferenceObjectByHandle(ThreadHandle, Access, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Protect write with SEH */
            _SEH2_TRY
            {
                /* Return the Win32 Start Address */
                *(PVOID *)ThreadInformation = Thread->Win32StartAddress;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadPerformanceCount:

            /* Set the return length*/
            Length = sizeof(LARGE_INTEGER);

            if (ThreadInformationLength != Length)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the thread */
            Status =
                ObReferenceObjectByHandle(ThreadHandle, Access, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Protect write with SEH */
            _SEH2_TRY
            {
                /* FIXME */
                (*(PLARGE_INTEGER)ThreadInformation).QuadPart = 0;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadAmILastThread:

            /* Set the return length*/
            Length = sizeof(ULONG);

            if (ThreadInformationLength != Length)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the thread */
            Status =
                ObReferenceObjectByHandle(ThreadHandle, Access, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Protect write with SEH */
            _SEH2_TRY
            {
                /* Return whether or not we are the last thread */
                *(PULONG)ThreadInformation =
                    ((Thread->ThreadsProcess->ThreadListHead.Flink->Flink == &Thread->ThreadsProcess->ThreadListHead)
                         ? TRUE
                         : FALSE);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadIsIoPending:

            /* Set the return length*/
            Length = sizeof(ULONG);

            if (ThreadInformationLength != Length)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the thread */
            Status =
                ObReferenceObjectByHandle(ThreadHandle, Access, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Raise the IRQL to protect the IRP list */
            KeRaiseIrql(APC_LEVEL, &OldIrql);

            /* Protect write with SEH */
            _SEH2_TRY
            {
                /* Check if the IRP list is empty or not */
                *(PULONG)ThreadInformation = !IsListEmpty(&Thread->IrpList);
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Lower IRQL back */
            KeLowerIrql(OldIrql);

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        /* LDT and GDT information */
        case ThreadDescriptorTableEntry:

#if defined(_X86_)
            /* Reference the thread */
            Status =
                ObReferenceObjectByHandle(ThreadHandle, Access, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Call the worker routine */
            Status = PspQueryDescriptorThread(Thread, ThreadInformation, ThreadInformationLength, ReturnLength);

            /* Dereference the thread */
            ObDereferenceObject(Thread);
#else
            /* Only implemented on x86 */
            Status = STATUS_NOT_IMPLEMENTED;
#endif
            break;

        case ThreadPriorityBoost:

            /* Set the return length*/
            Length = sizeof(ULONG);

            if (ThreadInformationLength != Length)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the thread */
            Status =
                ObReferenceObjectByHandle(ThreadHandle, Access, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            _SEH2_TRY
            {
                *(PULONG)ThreadInformation = Thread->Tcb.DisableBoost ? 1 : 0;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadBreakOnTermination:

            /* Set the return length */
            Length = sizeof(ULONG);

            if (ThreadInformationLength != Length)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the thread */
            Status =
                ObReferenceObjectByHandle(ThreadHandle, Access, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            _SEH2_TRY
            {
                *(PULONG)ThreadInformation = Thread->BreakOnTermination;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        case ThreadIsTerminated:

            /* Set the return length*/
            Length = sizeof(ThreadTerminated);

            if (ThreadInformationLength != Length)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the thread */
            Status =
                ObReferenceObjectByHandle(ThreadHandle, Access, PsThreadType, PreviousMode, (PVOID *)&Thread, NULL);
            if (!NT_SUCCESS(Status))
                break;

            ThreadTerminated = PsIsThreadTerminating(Thread);

            _SEH2_TRY
            {
                *(PULONG)ThreadInformation = ThreadTerminated ? 1 : 0;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        /* Anything else */
        default:
            /* Not yet implemented */
#if DBG
            DPRINT1("Not implemented: %s\n", PspDumpThreadInfoClassName(ThreadInformationClass));
#endif
            Status = STATUS_NOT_IMPLEMENTED;
    }

    /* Protect write with SEH */
    _SEH2_TRY
    {
        /* Check if caller wanted return length */
        if (ReturnLength)
            *ReturnLength = Length;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

/* EOF */
