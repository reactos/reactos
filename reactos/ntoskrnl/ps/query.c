/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/query.c
 * PURPOSE:         Process Manager: Thread/Process Query/Set Information
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* Include Information Class Tables */
#include "internal/ps_i.h"

/* PRIVATE FUNCTIONS *********************************************************/

/* FIXME:
 * This entire API is messed up because:
 * 1) Directly pokes SECTION_OBJECT/FILE_OBJECT without special reffing.
 * 2) Ignores SeAuditProcessImageFileName stuff added in XP (and ROS).
 * 3) Doesn't use ObQueryNameString.
 */
NTSTATUS
NTAPI
PspGetImagePath(IN PEPROCESS Process,
                OUT PUNICODE_STRING DstPath,
                IN ULONG ProcessInformationLength)
{
    NTSTATUS Status;
    ULONG ImagePathLen = 0;
    PROS_SECTION_OBJECT Section;
    PWSTR SrcBuffer = NULL, DstBuffer = (PWSTR)(DstPath + 1);

    Section = (PROS_SECTION_OBJECT)Process->SectionObject;
    if ((Section)&& (Section->FileObject))
    {
        /* FIXME - check for SEC_IMAGE and/or SEC_FILE instead
        of relying on FileObject being != NULL? */
        SrcBuffer = Section->FileObject->FileName.Buffer;
        if (SrcBuffer) ImagePathLen = Section->FileObject->FileName.Length;
    }

    if (ProcessInformationLength < (sizeof(UNICODE_STRING) +
                                    ImagePathLen +
                                    sizeof(WCHAR)))
    {
        return STATUS_INFO_LENGTH_MISMATCH;
    }

    Status = STATUS_SUCCESS;
    _SEH_TRY
    {
        /* copy the string manually, don't use RtlCopyUnicodeString with DstPath! */
        DstPath->Length = ImagePathLen;
        DstPath->MaximumLength = ImagePathLen + sizeof(WCHAR);
        DstPath->Buffer = DstBuffer;
        if (ImagePathLen) RtlCopyMemory(DstBuffer, SrcBuffer, ImagePathLen);
        DstBuffer[ImagePathLen / sizeof(WCHAR)] = L'\0';
    }
    _SEH_HANDLE
    {
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* Return status */
    return Status;
}

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryInformationProcess(IN HANDLE ProcessHandle,
                          IN PROCESSINFOCLASS ProcessInformationClass,
                          OUT PVOID ProcessInformation,
                          IN ULONG ProcessInformationLength,
                          OUT PULONG ReturnLength OPTIONAL)
{
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    ULONG Length = 0;
    PPROCESS_BASIC_INFORMATION ProcessBasicInfo =
        (PPROCESS_BASIC_INFORMATION)ProcessInformation;
    PKERNEL_USER_TIMES ProcessTime = (PKERNEL_USER_TIMES)ProcessInformation;
    ULONG HandleCount;
    PPROCESS_SESSION_INFORMATION SessionInfo =
        (PPROCESS_SESSION_INFORMATION)ProcessInformation;
    PVM_COUNTERS VmCounters = (PVM_COUNTERS)ProcessInformation;
    PROCESS_DEVICEMAP_INFORMATION DeviceMap;
    ULONG Cookie;
    PAGED_CODE();

    /* Check validity of Information Class */
    Status = DefaultQueryInfoBufferCheck(ProcessInformationClass,
                                         PsProcessInfoClass,
                                         RTL_NUMBER_OF(PsProcessInfoClass),
                                         ProcessInformation,
                                         ProcessInformationLength,
                                         ReturnLength,
                                         PreviousMode);
    if (!NT_SUCCESS(Status)) return Status;

    /* Check if this isn't the cookie class */
    if(ProcessInformationClass != ProcessCookie)
    {
        /* Reference the process */
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_QUERY_INFORMATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status)) return Status;
    }
    else if(ProcessHandle != NtCurrentProcess())
    {
        /*
         * Retreiving the process cookie is only allowed for the calling process
         * itself! XP only allowes NtCurrentProcess() as process handles even if
         * a real handle actually represents the current process.
         */
        return STATUS_INVALID_PARAMETER;
    }

    /* Check the information class */
    switch (ProcessInformationClass)
    {
        /* Basic process information */
        case ProcessBasicInformation:

            /* Protect writes with SEH */
            _SEH_TRY
            {
                /* Write all the information from the EPROCESS/KPROCESS */
                ProcessBasicInfo->ExitStatus = Process->ExitStatus;
                ProcessBasicInfo->PebBaseAddress = Process->Peb;
                ProcessBasicInfo->AffinityMask = Process->Pcb.Affinity;
                ProcessBasicInfo->UniqueProcessId = (ULONG)Process->
                                                    UniqueProcessId;
                ProcessBasicInfo->InheritedFromUniqueProcessId =
                    (ULONG)Process->InheritedFromUniqueProcessId;
                ProcessBasicInfo->BasePriority = Process->Pcb.BasePriority;

                /* Set return length */
                Length = sizeof(PROCESS_BASIC_INFORMATION);
            }
            _SEH_HANDLE
            {
                /* Get exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            break;

        /* Quote limits and I/O Counters: not implemented */
        case ProcessQuotaLimits:
        case ProcessIoCounters:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        /* Timing */
        case ProcessTimes:

            /* Protect writes with SEH */
            _SEH_TRY
            {
                /* Copy time information from EPROCESS/KPROCESS */
                ProcessTime->CreateTime = Process->CreateTime;
                ProcessTime->UserTime.QuadPart = Process->Pcb.UserTime *
                                                 100000LL;
                ProcessTime->KernelTime.QuadPart = Process->Pcb.KernelTime *
                                                   100000LL;
                ProcessTime->ExitTime = Process->ExitTime;

                /* Set the return length */
                Length = sizeof(KERNEL_USER_TIMES);
            }
            _SEH_HANDLE
            {
                /* Get exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            break;

        /* Process Debug Port */
        case ProcessDebugPort:

            /* Protect write with SEH */
            _SEH_TRY
            {
                /* Return whether or not we have a debug port */
                *(PHANDLE)ProcessInformation = (Process->DebugPort ?
                                                (HANDLE)-1 : NULL);

                /* Set the return length*/
                Length = sizeof(HANDLE);
            }
            _SEH_HANDLE
            {
                /* Get exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            break;

        /* LDT, WS and VDM Information: not implemented */
        case ProcessLdtInformation:
        case ProcessWorkingSetWatch:
        case ProcessWx86Information:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case ProcessHandleCount:

            /* Count the number of handles this process has */
            HandleCount = ObpGetHandleCountByHandleTable(Process->ObjectTable);

            /* Protect write in SEH */
            _SEH_TRY
            {
                /* Return the count of handles */
                *(PULONG)ProcessInformation = HandleCount;

                /* Set the return length*/
                Length = sizeof(ULONG);
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            break;

        /* Session ID for the process */
        case ProcessSessionInformation:

            /* Enter SEH for write safety */
            _SEH_TRY
            {
                /* Write back the Session ID */
                SessionInfo->SessionId = Process->Session;

                /* Set the return length */
                Length = sizeof(PROCESS_SESSION_INFORMATION);
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            break;

        /* WOW64: Not implemented */
        case ProcessWow64Information:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        /* Virtual Memory Statistics */
        case ProcessVmCounters:

            /* Enter SEH for write safety */
            _SEH_TRY
            {
                /* Return data from EPROCESS */
                VmCounters->PeakVirtualSize = Process->PeakVirtualSize;
                VmCounters->VirtualSize = Process->VirtualSize;
                VmCounters->PageFaultCount = Process->Vm.PageFaultCount;
                VmCounters->PeakWorkingSetSize = Process->Vm.PeakWorkingSetSize;
                VmCounters->WorkingSetSize = Process->Vm.WorkingSetSize;
                VmCounters->QuotaPeakPagedPoolUsage = Process->QuotaPeak[0];
                VmCounters->QuotaPagedPoolUsage = Process->QuotaUsage[0];
                VmCounters->QuotaPeakNonPagedPoolUsage = Process->QuotaPeak[1];
                VmCounters->QuotaNonPagedPoolUsage = Process->QuotaUsage[1];
                VmCounters->PagefileUsage = Process->QuotaUsage[2];
                VmCounters->PeakPagefileUsage = Process->QuotaPeak[2];

                /* Set the return length */
                *ReturnLength = sizeof(VM_COUNTERS);
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            break;

        /* Hard Error Processing Mode */
        case ProcessDefaultHardErrorMode:

            /* Enter SEH for writing back data */
            _SEH_TRY
            {
                /* Write the current processing mode */
                *(PULONG)ProcessInformation = Process->
                                              DefaultHardErrorProcessing;

                /* Set the return length */
                Length = sizeof(ULONG);
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            break;

        /* Priority Boosting status */
        case ProcessPriorityBoost:

            /* Enter SEH for writing back data */
            _SEH_TRY
            {
                /* Return boost status */
                *(PULONG)ProcessInformation = Process->Pcb.DisableBoost ?
                                              FALSE : TRUE;

                /* Set the return length */
                Length = sizeof(ULONG);
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            break;

        /* DOS Device Map */
        case ProcessDeviceMap:

            /* Query the device map information */
            ObQueryDeviceMapInformation(Process, &DeviceMap);

            /* Enter SEH for writing back data */
            _SEH_TRY
            {
                *(PPROCESS_DEVICEMAP_INFORMATION)ProcessInformation = DeviceMap;

                /* Set the return length */
                Length = sizeof(PROCESS_DEVICEMAP_INFORMATION);
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            break;

        /* Priority class */
        case ProcessPriorityClass:

            /* Enter SEH for writing back data */
            _SEH_TRY
            {
                /* Return current priority class */
                *(PUSHORT)ProcessInformation = Process->PriorityClass;

                /* Set the return length */
                Length = sizeof(USHORT);
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            break;

        case ProcessImageFileName:

            /* Get the image path */
            Status = PspGetImagePath(Process,
                                     (PUNICODE_STRING)ProcessInformation,
                                     ProcessInformationLength);
            break;

        /* Per-process security cookie */
        case ProcessCookie:

            /* Get the current process and cookie */
            Process = PsGetCurrentProcess();
            Cookie = Process->Cookie;
            if(!Cookie)
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
                if(!Cookie) Cookie = NewCookie;

                /* Set return length */
                Length = sizeof(ULONG);
            }

            /* Enter SEH to protect write */
            _SEH_TRY
            {
                /* Write back the cookie */
                *(PULONG)ProcessInformation = Cookie;
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            break;

        /* Not yet implemented, or unknown */
        case ProcessBasePriority:
        case ProcessRaisePriority:
        case ProcessExceptionPort:
        case ProcessAccessToken:
        case ProcessLdtSize:
        case ProcessIoPortHandlers:
        case ProcessUserModeIOPL:
        case ProcessEnableAlignmentFaultFixup:
        case ProcessAffinityMask:
        case ProcessForegroundInformation:
        default:
            Status = STATUS_INVALID_INFO_CLASS;
    }

    /* Protect write with SEH */
    _SEH_TRY
    {
        /* Check if caller wanted return length */
        if (ReturnLength) *ReturnLength = Length;
    }
    _SEH_HANDLE
    {
        /* Get exception code */
        Status = _SEH_GetExceptionCode();
    }
    _SEH_END;

    /* If we referenced the process, dereference it */
    if(ProcessInformationClass != ProcessCookie) ObDereferenceObject(Process);
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
    PROCESS_SESSION_INFORMATION SessionInfo;
    PEPORT ExceptionPort;
    PAGED_CODE();

    /* Verify Information Class validity */
    Status = DefaultSetInfoBufferCheck(ProcessInformationClass,
                                       PsProcessInfoClass,
                                       RTL_NUMBER_OF(PsProcessInfoClass),
                                       ProcessInformation,
                                       ProcessInformationLength,
                                       PreviousMode);
    if (!NT_SUCCESS(Status)) return Status;

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
        /* Quotas and priorities: not implemented */
        case ProcessQuotaLimits:
        case ProcessBasePriority:
        case ProcessRaisePriority:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        /* Error/Exception Port */
        case ProcessExceptionPort:

            /* Use SEH for capture */
            _SEH_TRY
            {
                /* Capture the handle */
                PortHandle = *(PHANDLE)ProcessInformation;
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            if (!NT_SUCCESS(Status)) break;

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

            /* Use SEH for capture */
            _SEH_TRY
            {
                /* Save the token handle */
                TokenHandle = ((PPROCESS_ACCESS_TOKEN)ProcessInformation)->
                               Token;
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            if (!NT_SUCCESS(Status)) break;

            /* Assign the actual token */
            Status = PspAssignPrimaryToken(Process, TokenHandle);
            break;

        /* Hard error processing */
        case ProcessDefaultHardErrorMode:

            /* Enter SEH for direct buffer read */
            _SEH_TRY
            {
                /* Update the current mode abd return the previous one */
                InterlockedExchange((LONG*)&Process->DefaultHardErrorProcessing,
                                    *(PLONG)ProcessInformation);
            }
            _SEH_HANDLE
            {
                /* Get exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            break;

        /* Session ID */
        case ProcessSessionInformation:

            /* Enter SEH for capture */
            _SEH_TRY
            {
                /* Capture the caller's buffer */
                SessionInfo = *(PPROCESS_SESSION_INFORMATION)ProcessInformation;
            }
            _SEH_HANDLE
            {
                /* Get the exception code */
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
            if (!NT_SUCCESS(Status)) break;

            /* Setting the session id requires the SeTcbPrivilege */
            if (!SeSinglePrivilegeCheck(SeTcbPrivilege, PreviousMode))
            {
                /* Can't set the session ID, bail out. */
                Status = STATUS_PRIVILEGE_NOT_HELD;
                break;
            }

            /* FIXME - update the session id for the process token */
            //Status = PsLockProcess(Process, FALSE);
            if (!NT_SUCCESS(Status)) break;

            /* Write the session ID in the EPROCESS */
            Process->Session = SessionInfo.SessionId;

            /* Check if the process also has a PEB */
            if (Process->Peb)
            {
                /*
                 * Attach to the process to make sure we're in the right
                 * context to access the PEB structure
                 */
                KeAttachProcess(&Process->Pcb);

                /* Enter SEH for write to user-mode PEB */
                _SEH_TRY
                {
                    /* Write the session ID */
                    Process->Peb->SessionId = SessionInfo.SessionId;
                }
                _SEH_HANDLE
                {
                    /* Get exception code */
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;

                /* Detach from the process */
                KeDetachProcess();
            }

            /* Unlock the process */
            //PsUnlockProcess(Process);
            break;

        /* Priority class: HACK! */
        case ProcessPriorityClass:
            break;

        /* We currently don't implement any of these */
        case ProcessLdtInformation:
        case ProcessLdtSize:
        case ProcessIoPortHandlers:
        case ProcessWorkingSetWatch:
        case ProcessUserModeIOPL:
        case ProcessEnableAlignmentFaultFixup:
        case ProcessAffinityMask:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        /* Supposedly these are invalid...!? verify! */
        case ProcessBasicInformation:
        case ProcessIoCounters:
        case ProcessTimes:
        case ProcessPooledUsageAndLimits:
        case ProcessWx86Information:
        case ProcessHandleCount:
        case ProcessWow64Information:
        case ProcessDebugPort:
        default:
            Status = STATUS_INVALID_INFO_CLASS;
    }

    /* Dereference and return status */
    ObDereferenceObject(Process);
    return Status;
}

/*
 * @unimplemented
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
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    DPRINT1("%s called for: %d\n", __FUNCTION__, ThreadInformationClass);
    /* FIXME: This is REALLY wrong. Some types don't need THREAD_SET_INFORMATION */
    /* FIXME: We should also check for certain things before doing the reference */
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_SET_INFORMATION,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
#if 0
        switch (ThreadInformationClass)
        {
            case ThreadPriority:

                if (u.Priority < LOW_PRIORITY || u.Priority >= MAXIMUM_PRIORITY)
                {
                    Status = STATUS_INVALID_PARAMETER;
                    break;
                }
                KeSetPriorityThread(&Thread->Tcb, u.Priority);
                break;

            case ThreadBasePriority:
                KeSetBasePriorityThread (&Thread->Tcb, u.Increment);
                break;

            case ThreadAffinityMask:

                /* Check if this is valid */
                DPRINT1("%lx, %lx\n", Thread->ThreadsProcess->Pcb.Affinity, u.Affinity);
                if ((Thread->ThreadsProcess->Pcb.Affinity & u.Affinity) !=
                    u.Affinity)
                {
                    DPRINT1("Wrong affinity given\n");
                    Status = STATUS_INVALID_PARAMETER;
                }
                else
                {
                    Status = KeSetAffinityThread(&Thread->Tcb, u.Affinity);
                }
                break;

            case ThreadImpersonationToken:
                Status = PsAssignImpersonationToken (Thread, u.Handle);
                break;

            case ThreadQuerySetWin32StartAddress:
                Thread->Win32StartAddress = u.Address;
                break;

            default:
                /* Shoult never occure if the data table is correct */
                KEBUGCHECK(0);
        }
#endif
        ObDereferenceObject (Thread);
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
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    DPRINT1("%s called for: %d\n", __FUNCTION__, ThreadInformationClass);
    Status = ObReferenceObjectByHandle(ThreadHandle,
                                       THREAD_QUERY_INFORMATION,
                                       PsThreadType,
                                       PreviousMode,
                                       (PVOID*)&Thread,
                                       NULL);
    if (!NT_SUCCESS(Status)) return Status;

#if 0
    switch (ThreadInformationClass)
    {
        case ThreadBasicInformation:
            /* A test on W2K agains ntdll shows NtQueryInformationThread return STATUS_PENDING
            * as ExitStatus for current/running thread, while KETHREAD's ExitStatus is
            * 0. So do the conversion here:
            * -Gunnar     */
            u.TBI.ExitStatus = (Thread->ExitStatus == 0) ? STATUS_PENDING : Thread->ExitStatus;
            u.TBI.TebBaseAddress = (PVOID)Thread->Tcb.Teb;
            u.TBI.ClientId = Thread->Cid;
            u.TBI.AffinityMask = Thread->Tcb.Affinity;
            u.TBI.Priority = Thread->Tcb.Priority;
            u.TBI.BasePriority = KeQueryBasePriorityThread(&Thread->Tcb);
            break;

        case ThreadTimes:
            u.TTI.KernelTime.QuadPart = Thread->Tcb.KernelTime * 100000LL;
            u.TTI.UserTime.QuadPart = Thread->Tcb.UserTime * 100000LL;
            u.TTI.CreateTime = Thread->CreateTime;
            /*This works*/
            u.TTI.ExitTime = Thread->ExitTime;
            break;

        case ThreadQuerySetWin32StartAddress:
            u.Address = Thread->Win32StartAddress;
            break;

        case ThreadPerformanceCount:
            /* Nebbett says this class is always zero */
            u.Count.QuadPart = 0;
            break;

        case ThreadAmILastThread:
            if (Thread->ThreadsProcess->ThreadListHead.Flink->Flink ==
                &Thread->ThreadsProcess->ThreadListHead)
            {
                u.Last = TRUE;
            }
            else
            {
                u.Last = FALSE;
            }
            break;

        case ThreadIsIoPending:
            {
                KIRQL OldIrql;

                /* Raise the IRQL to protect the IRP list */
                KeRaiseIrql(APC_LEVEL, &OldIrql);
                u.IsIoPending = !IsListEmpty(&Thread->IrpList);
                KeLowerIrql(OldIrql);
                break;
            }

        default:
            /* Shoult never occure if the data table is correct */
            KEBUGCHECK(0);
    }
#endif
    ObDereferenceObject(Thread);
    return(Status);
}

/* EOF */
