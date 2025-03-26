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
PsReferenceProcessFilePointer(IN PEPROCESS Process,
                              OUT PFILE_OBJECT *FileObject)
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

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryInformationProcess(
    _In_ HANDLE ProcessHandle,
    _In_ PROCESSINFOCLASS ProcessInformationClass,
    _Out_ PVOID ProcessInformation,
    _In_ ULONG ProcessInformationLength,
    _Out_opt_ PULONG ReturnLength)
{
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    ULONG Length = 0;

    PAGED_CODE();

    /* Verify Information Class validity */
    Status = DefaultQueryInfoBufferCheck(ProcessInformationClass,
                                         PsProcessInfoClass,
                                         RTL_NUMBER_OF(PsProcessInfoClass),
                                         ICIF_PROBE_READ,
                                         ProcessInformation,
                                         ProcessInformationLength,
                                         ReturnLength,
                                         NULL,
                                         PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationProcess(): Information verification class failed! (Status -> 0x%lx, ProcessInformationClass -> %lx)\n", Status, ProcessInformationClass);
        return Status;
    }

    if (((ProcessInformationClass == ProcessCookie) ||
         (ProcessInformationClass == ProcessImageInformation)) &&
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

            /* Set return length */
            Length = sizeof(PROCESS_BASIC_INFORMATION);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

            /* Protect writes with SEH */
            _SEH2_TRY
            {
                /* Write all the information from the EPROCESS/KPROCESS */
                ProcessBasicInfo->ExitStatus = Process->ExitStatus;
                ProcessBasicInfo->PebBaseAddress = Process->Peb;
                ProcessBasicInfo->AffinityMask = Process->Pcb.Affinity;
                ProcessBasicInfo->UniqueProcessId = (ULONG_PTR)Process->
                                                    UniqueProcessId;
                ProcessBasicInfo->InheritedFromUniqueProcessId =
                    (ULONG_PTR)Process->InheritedFromUniqueProcessId;
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

            if (ProcessInformationLength != sizeof(QUOTA_LIMITS) &&
                ProcessInformationLength != sizeof(QUOTA_LIMITS_EX))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set return length */
            Length = ProcessInformationLength;
            Extended = (Length == sizeof(QUOTA_LIMITS_EX));

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

            /* Indicate success */
            Status = STATUS_SUCCESS;

            RtlZeroMemory(&QuotaLimits, sizeof(QuotaLimits));

            /* Get max/min working set sizes */
            QuotaLimits.MaximumWorkingSetSize =
                Process->Vm.MaximumWorkingSetSize << PAGE_SHIFT;
            QuotaLimits.MinimumWorkingSetSize =
                Process->Vm.MinimumWorkingSetSize << PAGE_SHIFT;

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
                QuotaLimits.PagedPoolLimit =
                    Process->QuotaBlock->QuotaEntry[PsPagedPool].Limit;
                QuotaLimits.NonPagedPoolLimit =
                    Process->QuotaBlock->QuotaEntry[PsNonPagedPool].Limit;
                QuotaLimits.PagefileLimit =
                    Process->QuotaBlock->QuotaEntry[PsPageFile].Limit;
            }

            /* Get additional information, if needed */
            if (Extended)
            {
                QuotaLimits.Flags |= (Process->Vm.Flags.MaximumWorkingSetHard ?
                    QUOTA_LIMITS_HARDWS_MAX_ENABLE : QUOTA_LIMITS_HARDWS_MAX_DISABLE);
                QuotaLimits.Flags |= (Process->Vm.Flags.MinimumWorkingSetHard ?
                    QUOTA_LIMITS_HARDWS_MIN_ENABLE : QUOTA_LIMITS_HARDWS_MIN_DISABLE);

                /* FIXME: Get the correct information */
                //QuotaLimits.WorkingSetLimit = (SIZE_T)-1; // Not used on Win2k3, it is set to 0
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
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

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
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

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

            /* Set return length */
            Length = sizeof(HANDLE);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

            /* Protect write with SEH */
            _SEH2_TRY
            {
                /* Return whether or not we have a debug port */
                *(PHANDLE)ProcessInformation = (Process->DebugPort ?
                                                (HANDLE)-1 : NULL);
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
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

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
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

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
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

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
                //VmCounters->PrivateUsage = Process->CommitCharge << PAGE_SHIFT;
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
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

            /* Enter SEH for writing back data */
            _SEH2_TRY
            {
                /* Write the current processing mode */
                *(PULONG)ProcessInformation = Process->
                                              DefaultHardErrorProcessing;
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
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

            /* Enter SEH for writing back data */
            _SEH2_TRY
            {
                /* Return boost status */
                *(PULONG)ProcessInformation = Process->Pcb.DisableBoost ?
                                              TRUE : FALSE;
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
                if ((Flags & ~PROCESS_LUID_DOSDEVICES_ONLY) != 0 ||
                    !ObIsLUIDDeviceMapsEnabled())
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
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

            /* Query the device map information */
            Status = ObQueryDeviceMapInformation(Process,
                                                 ProcessInformation,
                                                 Flags);

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
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

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
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

            /* Get the image path */
            Status = SeLocateProcessImageName(Process, &ImageName);
            if (NT_SUCCESS(Status))
            {
                /* Set return length */
                Length = ImageName->MaximumLength +
                         sizeof(OBJECT_NAME_INFORMATION);

                /* Make sure it's large enough */
                if (Length <= ProcessInformationLength)
                {
                    /* Enter SEH to protect write */
                    _SEH2_TRY
                    {
                        /* Copy it */
                        RtlCopyMemory(ProcessInformation,
                                      ImageName,
                                      Length);

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

        case ProcessDebugFlags:

            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length*/
            Length = sizeof(ULONG);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

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
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

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
                NewCookie = Prcb->KeSystemCalls ^ Prcb->InterruptTime ^
                            SystemTime.u.LowPart ^ SystemTime.u.HighPart;

                /* Set the new cookie or return the current one */
                Cookie = InterlockedCompareExchange((LONG*)&Process->Cookie,
                                                    NewCookie,
                                                    Cookie);
                if (!Cookie) Cookie = NewCookie;

                /* Set return length */
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

            /* Indicate success */
            Status = STATUS_SUCCESS;
            break;

        case ProcessDebugObjectHandle:
        {
            HANDLE DebugPort = 0;

            if (ProcessInformationLength != sizeof(HANDLE))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set the return length */
            Length = sizeof(HANDLE);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

            /* Get the debug port */
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
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
            break;
        }

        case ProcessHandleTracing:
            DPRINT1("Handle tracing Not implemented: %lx\n", ProcessInformationClass);
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
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

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

            /* Set return length */
            Length = sizeof(ULONG_PTR);

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ProcessHandle,
                                               PROCESS_QUERY_INFORMATION,
                                               PsProcessType,
                                               PreviousMode,
                                               (PVOID*)&Process,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

            /* Make sure the process isn't dying */
            if (ExAcquireRundownProtection(&Process->RundownProtect))
            {
                /* Get the WOW64 process structure */
#ifdef _WIN64
                Wow64 = (ULONG_PTR)Process->Wow64Process;
#else
                Wow64 = 0;
#endif
                /* Release the lock */
                ExReleaseRundownProtection(&Process->RundownProtect);
            }

            /* Protect write with SEH */
            _SEH2_TRY
            {
                /* Return whether or not we have a debug port */
                *(PULONG_PTR)ProcessInformation = Wow64;
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

        case ProcessExecuteFlags:
        {
            ULONG ExecuteOptions = 0;

            if (ProcessInformationLength != sizeof(ULONG))
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Set return length */
            Length = sizeof(ULONG);

            if (ProcessHandle != NtCurrentProcess())
            {
                return STATUS_INVALID_PARAMETER;
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
            DPRINT1("VDM/16-bit not implemented: %lx\n", ProcessInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case ProcessWorkingSetWatch:
            DPRINT1("WS Watch Not implemented: %lx\n", ProcessInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case ProcessPooledUsageAndLimits:
            DPRINT1("Pool limits Not implemented: %lx\n", ProcessInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        /* Not supported by Server 2003 */
        default:
            DPRINT1("Unsupported info class: %lx\n", ProcessInformationClass);
            Status = STATUS_INVALID_INFO_CLASS;
    }

    /* Protect write with SEH */
    _SEH2_TRY
    {
        /* Check if caller wanted return length */
        if ((ReturnLength) && (Length)) *ReturnLength = Length;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        /* Get exception code */
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetInformationProcess(IN HANDLE ProcessHandle,
                        IN PROCESSINFOCLASS ProcessInformationClass,
                        IN PVOID ProcessInformation,
                        IN ULONG ProcessInformationLength)
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

    /* Verify Information Class validity */
    Status = DefaultSetInfoBufferCheck(ProcessInformationClass,
                                       PsProcessInfoClass,
                                       RTL_NUMBER_OF(PsProcessInfoClass),
                                       ProcessInformation,
                                       ProcessInformationLength,
                                       PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationProcess(): Information verification class failed! (Status -> 0x%lx, ProcessInformationClass -> %lx)\n", Status, ProcessInformationClass);
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
    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       Access,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

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
            Status = ObReferenceObjectByHandle(PortHandle,
                                               0,
                                               LpcPortObjectType,
                                               PreviousMode,
                                               (PVOID)&ExceptionPort,
                                               NULL);
            if (!NT_SUCCESS(Status)) break;

            /* Change the pointer */
            if (InterlockedCompareExchangePointer(&Process->ExceptionPort,
                                                  ExceptionPort,
                                                  NULL))
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
                TokenHandle = ((PPROCESS_ACCESS_TOKEN)ProcessInformation)->
                               Token;
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

#if 0 // OLD AND DEPRECATED CODE!!!!

            /* FIXME - update the session id for the process token */
            //Status = PsLockProcess(Process, FALSE);
            if (!NT_SUCCESS(Status)) break;

            /* Write the session ID in the EPROCESS */
            Process->Session = UlongToPtr(SessionInfo.SessionId); // HACK!!!

            /* Check if the process also has a PEB */
            if (Process->Peb)
            {
                /*
                 * Attach to the process to make sure we're in the right
                 * context to access the PEB structure
                 */
                KeAttachProcess(&Process->Pcb);

                /* Enter SEH for write to user-mode PEB */
                _SEH2_TRY
                {
                    /* Write the session ID */
                    Process->Peb->SessionId = SessionInfo.SessionId;
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    /* Get exception code */
                    Status = _SEH2_GetExceptionCode();
                }
                _SEH2_END;

                /* Detach from the process */
                KeDetachProcess();
            }

            /* Unlock the process */
            //PsUnlockProcess(Process);

#endif

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
                HasPrivilege = SeCheckPrivilegedObject(SeIncreaseBasePriorityPrivilege,
                                                       ProcessHandle,
                                                       PROCESS_SET_INFORMATION,
                                                       PreviousMode);
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
            PsSetProcessPriorityByClass(Process,
                                        PriorityClass.Foreground ?
                                        PsProcessPriorityForeground :
                                        PsProcessPriorityBackground);
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
            PsSetProcessPriorityByClass(Process,
                                        Foreground.Foreground ?
                                        PsProcessPriorityForeground :
                                        PsProcessPriorityBackground);
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
                BasePriority = *(KPRIORITY*)ProcessInformation;
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
                HasPrivilege = SeCheckPrivilegedObject(SeIncreaseBasePriorityPrivilege,
                                                       ProcessHandle,
                                                       PROCESS_SET_INFORMATION,
                                                       PreviousMode);
                if (!HasPrivilege)
                {
                    ObDereferenceObject(Process);
                    DPRINT1("Privilege to change priority from %lx to %lx lacking\n", BasePriority, Process->Pcb.BasePriority);
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
                for (Next = Process->ThreadListHead.Flink;
                     Next != &Process->ThreadListHead;
                     Next = Next->Flink)
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
                for (Next = Process->ThreadListHead.Flink;
                     Next != &Process->ThreadListHead;
                     Next = Next->Flink)
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
#if defined (_X86_)
            Ke386SetIOPL();
#elif defined(_M_AMD64)
            /* On x64 this function isn't implemented.
               On Windows 2003 it returns success.
               On Vista+ it returns STATUS_NOT_IMPLEMENTED. */
            if ((ExGetPreviousMode() != KernelMode) &&
                (RtlRosGetAppcompatVersion() > _WIN32_WINNT_WS03))
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
             DPRINT1("VDM/16-bit Request not implemented: %lx\n", ProcessInformationClass);
             Status = STATUS_NOT_IMPLEMENTED;
             break;

        case ProcessQuotaLimits:

            Status = PspSetQuotaLimits(Process,
                                     1,
                                     ProcessInformation,
                                     ProcessInformationLength,
                                     PreviousMode);
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
            DPRINT1("Invalid Server 2003 Info Class: %lx\n", ProcessInformationClass);
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
NtSetInformationThread(IN HANDLE ThreadHandle,
                       IN THREADINFOCLASS ThreadInformationClass,
                       IN PVOID ThreadInformation,
                       IN ULONG ThreadInformationLength)
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

    /* Verify Information Class validity */
    Status = DefaultSetInfoBufferCheck(ThreadInformationClass,
                                       PsThreadInfoClass,
                                       RTL_NUMBER_OF(PsThreadInfoClass),
                                       ThreadInformation,
                                       ThreadInformationLength,
                                       PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationThread(): Information verification class failed! (Status -> 0x%lx, ThreadInformationClass -> %lx)\n", Status, ThreadInformationClass);
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
            if ((Priority > HIGH_PRIORITY) ||
                (Priority <= LOW_PRIORITY))
            {
                /* Fail */
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            /* Check for the required privilege */
            if (Priority >= LOW_REALTIME_PRIORITY)
            {
                HasPrivilege = SeCheckPrivilegedObject(SeIncreaseBasePriorityPrivilege,
                                                       ThreadHandle,
                                                       THREAD_SET_INFORMATION,
                                                       PreviousMode);
                if (!HasPrivilege)
                {
                    DPRINT1("Privilege to change priority to %lx lacking\n", Priority);
                    return STATUS_PRIVILEGE_NOT_HELD;
                }
            }

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               THREAD_SET_INFORMATION,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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
            if ((Priority > THREAD_BASE_PRIORITY_MAX) ||
                (Priority < THREAD_BASE_PRIORITY_MIN))
            {
                /* These ones are OK */
                if ((Priority != THREAD_BASE_PRIORITY_LOWRT + 1) &&
                    (Priority != THREAD_BASE_PRIORITY_IDLE - 1))
                {
                    /* Check if the process is real time */
                    if (PsGetCurrentProcess()->PriorityClass !=
                        PROCESS_PRIORITY_CLASS_REALTIME)
                    {
                        /* It isn't, fail */
                        Status = STATUS_INVALID_PARAMETER;
                        break;
                    }
                }
            }

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               THREAD_SET_INFORMATION,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               THREAD_SET_INFORMATION,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               THREAD_SET_THREAD_TOKEN,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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
                Address = *(PVOID*)ThreadInformation;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
                _SEH2_YIELD(break);
            }
            _SEH2_END;

            /* Reference the thread */
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               THREAD_SET_INFORMATION,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               THREAD_SET_INFORMATION,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Set the ideal */
            Status = KeSetIdealProcessorThread(&Thread->Tcb,
                                               (CCHAR)IdealProcessor);

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
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               THREAD_SET_INFORMATION,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               THREAD_SET_INFORMATION,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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
                            if (TlsIndex < (TLS_MINIMUM_AVAILABLE +
                                            TLS_EXPANSION_SLOTS) - 1)
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
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               THREAD_SET_INFORMATION,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               THREAD_SET_INFORMATION,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Set the flag */
            PspSetCrossThreadFlag(Thread, CT_HIDE_FROM_DEBUGGER_BIT);

            /* Dereference the thread */
            ObDereferenceObject(Thread);
            break;

        default:
            /* We don't implement it yet */
            DPRINT1("Not implemented: %d\n", ThreadInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryInformationThread(IN HANDLE ThreadHandle,
                         IN THREADINFOCLASS ThreadInformationClass,
                         OUT PVOID ThreadInformation,
                         IN ULONG ThreadInformationLength,
                         OUT PULONG ReturnLength OPTIONAL)
{
    PETHREAD Thread;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    ULONG Access;
    ULONG Length = 0;
    PTHREAD_BASIC_INFORMATION ThreadBasicInfo =
        (PTHREAD_BASIC_INFORMATION)ThreadInformation;
    PKERNEL_USER_TIMES ThreadTime = (PKERNEL_USER_TIMES)ThreadInformation;
    KIRQL OldIrql;
    ULONG ThreadTerminated;
    PAGED_CODE();

    /* Verify Information Class validity */
    Status = DefaultQueryInfoBufferCheck(ThreadInformationClass,
                                         PsThreadInfoClass,
                                         RTL_NUMBER_OF(PsThreadInfoClass),
                                         ICIF_PROBE_READ,
                                         ThreadInformation,
                                         ThreadInformationLength,
                                         ReturnLength,
                                         NULL,
                                         PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationThread(): Information verification class failed! (Status -> 0x%lx , ThreadInformationClass -> %lx)\n", Status, ThreadInformationClass);
        return Status;
    }

    /* Check what class this is */
    Access = THREAD_QUERY_INFORMATION;

    /* Check what kind of information class this is */
    switch (ThreadInformationClass)
    {
        /* Basic thread information */
        case ThreadBasicInformation:

            /* Set return length */
            Length = sizeof(THREAD_BASIC_INFORMATION);

            if (ThreadInformationLength != Length)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               Access,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               Access,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               Access,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Protect write with SEH */
            _SEH2_TRY
            {
                /* Return the Win32 Start Address */
                *(PVOID*)ThreadInformation = Thread->Win32StartAddress;
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

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               Access,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               Access,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Protect write with SEH */
            _SEH2_TRY
            {
                /* Return whether or not we are the last thread */
                *(PULONG)ThreadInformation = ((Thread->ThreadsProcess->
                                               ThreadListHead.Flink->Flink ==
                                               &Thread->ThreadsProcess->
                                               ThreadListHead) ?
                                              TRUE : FALSE);
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

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               Access,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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
            /* Reference the process */
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               Access,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
            if (!NT_SUCCESS(Status))
                break;

            /* Call the worker routine */
            Status = PspQueryDescriptorThread(Thread,
                                              ThreadInformation,
                                              ThreadInformationLength,
                                              ReturnLength);

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

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               Access,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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

        case ThreadIsTerminated:

            /* Set the return length*/
            Length = sizeof(ThreadTerminated);

            if (ThreadInformationLength != Length)
            {
                Status = STATUS_INFO_LENGTH_MISMATCH;
                break;
            }

            /* Reference the process */
            Status = ObReferenceObjectByHandle(ThreadHandle,
                                               Access,
                                               PsThreadType,
                                               PreviousMode,
                                               (PVOID*)&Thread,
                                               NULL);
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
            DPRINT1("Not implemented: %lx\n", ThreadInformationClass);
            Status = STATUS_NOT_IMPLEMENTED;
    }

    /* Protect write with SEH */
    _SEH2_TRY
    {
        /* Check if caller wanted return length */
        if (ReturnLength) *ReturnLength = Length;
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
