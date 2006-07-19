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

/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
NTSTATUS
NTAPI
NtQueryInformationProcess(IN HANDLE ProcessHandle,
                          IN PROCESSINFOCLASS ProcessInformationClass,
                          OUT PVOID ProcessInformation,
                          IN ULONG ProcessInformationLength,
                          OUT PULONG ReturnLength  OPTIONAL)
{
    PEPROCESS Process;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check validity of Information Class */
    Status = DefaultQueryInfoBufferCheck(ProcessInformationClass,
                                         PsProcessInfoClass,
                                         RTL_NUMBER_OF(PsProcessInfoClass),
                                         ProcessInformation,
                                         ProcessInformationLength,
                                         ReturnLength,
                                         PreviousMode);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("NtQueryInformationProcess() failed, Status: 0x%x\n", Status);
        return Status;
    }

    if(ProcessInformationClass != ProcessCookie)
    {
        Status = ObReferenceObjectByHandle(ProcessHandle,
                                           PROCESS_QUERY_INFORMATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&Process,
                                           NULL);
        if (!NT_SUCCESS(Status)) return(Status);
    }
    else if(ProcessHandle != NtCurrentProcess())
    {
        /* retreiving the process cookie is only allowed for the calling process
        itself! XP only allowes NtCurrentProcess() as process handles even if a
        real handle actually represents the current process. */
        return STATUS_INVALID_PARAMETER;
    }

    switch (ProcessInformationClass)
    {
        case ProcessBasicInformation:
            {
                PPROCESS_BASIC_INFORMATION ProcessBasicInformationP =
                    (PPROCESS_BASIC_INFORMATION)ProcessInformation;

                _SEH_TRY
                {
                    ProcessBasicInformationP->ExitStatus = Process->ExitStatus;
                    ProcessBasicInformationP->PebBaseAddress = Process->Peb;
                    ProcessBasicInformationP->AffinityMask = Process->Pcb.Affinity;
                    ProcessBasicInformationP->UniqueProcessId =
                        (ULONG)Process->UniqueProcessId;
                    ProcessBasicInformationP->InheritedFromUniqueProcessId =
                        (ULONG)Process->InheritedFromUniqueProcessId;
                    ProcessBasicInformationP->BasePriority =
                        Process->Pcb.BasePriority;

                    if (ReturnLength)
                    {
                        *ReturnLength = sizeof(PROCESS_BASIC_INFORMATION);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                break;
            }

        case ProcessQuotaLimits:
        case ProcessIoCounters:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case ProcessTimes:
            {
                PKERNEL_USER_TIMES ProcessTimeP = (PKERNEL_USER_TIMES)ProcessInformation;
                _SEH_TRY
                {
                    ProcessTimeP->CreateTime = Process->CreateTime;
                    ProcessTimeP->UserTime.QuadPart = Process->Pcb.UserTime * 100000LL;
                    ProcessTimeP->KernelTime.QuadPart = Process->Pcb.KernelTime * 100000LL;
                    ProcessTimeP->ExitTime = Process->ExitTime;

                    if (ReturnLength)
                    {
                        *ReturnLength = sizeof(KERNEL_USER_TIMES);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                break;
            }

        case ProcessDebugPort:
            {
                _SEH_TRY
                {
                    *(PHANDLE)ProcessInformation = (Process->DebugPort != NULL ? (HANDLE)-1 : NULL);
                    if (ReturnLength)
                    {
                        *ReturnLength = sizeof(HANDLE);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                break;
            }

        case ProcessLdtInformation:
        case ProcessWorkingSetWatch:
        case ProcessWx86Information:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case ProcessHandleCount:
            {
                ULONG HandleCount = ObpGetHandleCountByHandleTable(Process->ObjectTable);

                _SEH_TRY
                {
                    *(PULONG)ProcessInformation = HandleCount;
                    if (ReturnLength)
                    {
                        *ReturnLength = sizeof(ULONG);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                break;
            }

        case ProcessSessionInformation:
            {
                PPROCESS_SESSION_INFORMATION SessionInfo = (PPROCESS_SESSION_INFORMATION)ProcessInformation;

                _SEH_TRY
                {
                    SessionInfo->SessionId = Process->Session;
                    if (ReturnLength)
                    {
                        *ReturnLength = sizeof(PROCESS_SESSION_INFORMATION);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                break;
            }

        case ProcessWow64Information:
            DPRINT1("We currently don't support the ProcessWow64Information information class!\n");
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case ProcessVmCounters:
            {
                PVM_COUNTERS pOut = (PVM_COUNTERS)ProcessInformation;

                _SEH_TRY
                {
                    pOut->PeakVirtualSize            = Process->PeakVirtualSize;
                    /*
                    * Here we should probably use VirtualSize.LowPart, but due to
                    * incompatibilities in current headers (no unnamed union),
                    * I opted for cast.
                    */
                    pOut->VirtualSize                = (ULONG)Process->VirtualSize;
                    pOut->PageFaultCount             = Process->Vm.PageFaultCount;
                    pOut->PeakWorkingSetSize         = Process->Vm.PeakWorkingSetSize;
                    pOut->WorkingSetSize             = Process->Vm.WorkingSetSize;
                    pOut->QuotaPeakPagedPoolUsage    = Process->QuotaPeak[0]; // TODO: Verify!
                    pOut->QuotaPagedPoolUsage        = Process->QuotaUsage[0];     // TODO: Verify!
                    pOut->QuotaPeakNonPagedPoolUsage = Process->QuotaPeak[1]; // TODO: Verify!
                    pOut->QuotaNonPagedPoolUsage     = Process->QuotaUsage[1];     // TODO: Verify!
                    pOut->PagefileUsage              = Process->QuotaUsage[2];
                    pOut->PeakPagefileUsage          = Process->QuotaPeak[2];

                    if (ReturnLength)
                    {
                        *ReturnLength = sizeof(VM_COUNTERS);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                break;
            }

        case ProcessDefaultHardErrorMode:
            {
                PULONG HardErrMode = (PULONG)ProcessInformation;
                _SEH_TRY
                {
                    *HardErrMode = Process->DefaultHardErrorProcessing;
                    if (ReturnLength)
                    {
                        *ReturnLength = sizeof(ULONG);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                break;
            }

        case ProcessPriorityBoost:
            {
                PULONG BoostEnabled = (PULONG)ProcessInformation;

                _SEH_TRY
                {
                    *BoostEnabled = Process->Pcb.DisableBoost ? FALSE : TRUE;

                    if (ReturnLength)
                    {
                        *ReturnLength = sizeof(ULONG);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                break;
            }

        case ProcessDeviceMap:
            {
                PROCESS_DEVICEMAP_INFORMATION DeviceMap;

                ObQueryDeviceMapInformation(Process, &DeviceMap);

                _SEH_TRY
                {
                    *(PPROCESS_DEVICEMAP_INFORMATION)ProcessInformation = DeviceMap;
                    if (ReturnLength)
                    {
                        *ReturnLength = sizeof(PROCESS_DEVICEMAP_INFORMATION);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                break;
            }

        case ProcessPriorityClass:
            {
                PUSHORT Priority = (PUSHORT)ProcessInformation;

                _SEH_TRY
                {
                    *Priority = Process->PriorityClass;

                    if (ReturnLength)
                    {
                        *ReturnLength = sizeof(USHORT);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                break;
            }

        case ProcessImageFileName:
            {
                ULONG ImagePathLen = 0;
                PROS_SECTION_OBJECT Section;
                PUNICODE_STRING DstPath = (PUNICODE_STRING)ProcessInformation;
                PWSTR SrcBuffer = NULL, DstBuffer = (PWSTR)(DstPath + 1);

                Section = (PROS_SECTION_OBJECT)Process->SectionObject;

                if (Section != NULL && Section->FileObject != NULL)
                {
                    /* FIXME - check for SEC_IMAGE and/or SEC_FILE instead
                    of relying on FileObject being != NULL? */
                    SrcBuffer = Section->FileObject->FileName.Buffer;
                    if (SrcBuffer != NULL)
                    {
                        ImagePathLen = Section->FileObject->FileName.Length;
                    }
                }

                if(ProcessInformationLength < sizeof(UNICODE_STRING) + ImagePathLen + sizeof(WCHAR))
                {
                    Status = STATUS_INFO_LENGTH_MISMATCH;
                }
                else
                {
                    _SEH_TRY
                    {
                        /* copy the string manually, don't use RtlCopyUnicodeString with DstPath! */
                        DstPath->Length = ImagePathLen;
                        DstPath->MaximumLength = ImagePathLen + sizeof(WCHAR);
                        DstPath->Buffer = DstBuffer;
                        if (ImagePathLen != 0)
                        {
                            RtlCopyMemory(DstBuffer,
                                SrcBuffer,
                                ImagePathLen);
                        }
                        DstBuffer[ImagePathLen / sizeof(WCHAR)] = L'\0';

                        Status = STATUS_SUCCESS;
                    }
                    _SEH_HANDLE
                    {
                        Status = _SEH_GetExceptionCode();
                    }
                    _SEH_END;
                }
                break;
            }

        case ProcessCookie:
            {
                ULONG Cookie;

                /* receive the process cookie, this is only allowed for the current
                process! */

                Process = PsGetCurrentProcess();

                Cookie = Process->Cookie;
                if(Cookie == 0)
                {
                    LARGE_INTEGER SystemTime;
                    ULONG NewCookie;
                    PKPRCB Prcb;

                    /* generate a new cookie */

                    KeQuerySystemTime(&SystemTime);

                    Prcb = KeGetCurrentPrcb();

                    NewCookie = Prcb->KeSystemCalls ^ Prcb->InterruptTime ^
                        SystemTime.u.LowPart ^ SystemTime.u.HighPart;

                    /* try to set the new cookie, return the current one if another thread
                    set it in the meanwhile */
                    Cookie = InterlockedCompareExchange((LONG*)&Process->Cookie,
                        NewCookie,
                        Cookie);
                    if(Cookie == 0)
                    {
                        /* successfully set the cookie */
                        Cookie = NewCookie;
                    }
                }

                _SEH_TRY
                {
                    *(PULONG)ProcessInformation = Cookie;
                    if (ReturnLength)
                    {
                        *ReturnLength = sizeof(ULONG);
                    }
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;

                break;
            }

            /*
            * Note: The following 10 information classes are verified to not be
            * implemented on NT, and do indeed return STATUS_INVALID_INFO_CLASS;
            */
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

    if(ProcessInformationClass != ProcessCookie) ObDereferenceObject(Process);
    return Status;
}

/*
 * @unimplemented
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
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Verify Information Class validity */
    Status = DefaultSetInfoBufferCheck(ProcessInformationClass,
                                       PsProcessInfoClass,
                                       RTL_NUMBER_OF(PsProcessInfoClass),
                                       ProcessInformation,
                                       ProcessInformationLength,
                                       PreviousMode);
    if(!NT_SUCCESS(Status))
    {
        DPRINT1("NtSetInformationProcess() %x failed, Status: 0x%x\n", Status);
        return Status;
    }

    switch(ProcessInformationClass)
    {
        case ProcessSessionInformation:
            Access = PROCESS_SET_INFORMATION | PROCESS_SET_SESSIONID;
            break;
        case ProcessExceptionPort:
            Access = PROCESS_SET_INFORMATION | PROCESS_SUSPEND_RESUME;
            break;

        default:
            Access = PROCESS_SET_INFORMATION;
            break;
    }

    Status = ObReferenceObjectByHandle(ProcessHandle,
                                       Access,
                                       PsProcessType,
                                       PreviousMode,
                                       (PVOID*)&Process,
                                       NULL);
    if (!NT_SUCCESS(Status)) return(Status);

    switch (ProcessInformationClass)
    {
        case ProcessQuotaLimits:
        case ProcessBasePriority:
        case ProcessRaisePriority:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

        case ProcessExceptionPort:
            {
                HANDLE PortHandle = NULL;

                /* make a safe copy of the buffer on the stack */
                _SEH_TRY
                {
                    PortHandle = *(PHANDLE)ProcessInformation;
                    Status = STATUS_SUCCESS;
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;

                if(NT_SUCCESS(Status))
                {
                    PEPORT ExceptionPort;

                    /* in case we had success reading from the buffer, verify the provided
                    * LPC port handle
                    */
                    Status = ObReferenceObjectByHandle(PortHandle,
                        0,
                        LpcPortObjectType,
                        PreviousMode,
                        (PVOID)&ExceptionPort,
                        NULL);
                    if(NT_SUCCESS(Status))
                    {
                        /* lock the process to be thread-safe! */

                        Status = PsLockProcess(Process, FALSE);
                        if(NT_SUCCESS(Status))
                        {
                            /*
                            * according to "NT Native API" documentation, setting the exception
                            * port is only permitted once!
                            */
                            if(Process->ExceptionPort == NULL)
                            {
                                /* keep the reference to the handle! */
                                Process->ExceptionPort = ExceptionPort;
                                Status = STATUS_SUCCESS;
                            }
                            else
                            {
                                ObDereferenceObject(ExceptionPort);
                                Status = STATUS_PORT_ALREADY_SET;
                            }
                            PsUnlockProcess(Process);
                        }
                        else
                        {
                            ObDereferenceObject(ExceptionPort);
                        }
                    }
                }
                break;
            }

        case ProcessAccessToken:
            {
                HANDLE TokenHandle = NULL;

                /* make a safe copy of the buffer on the stack */
                _SEH_TRY
                {
                    TokenHandle = ((PPROCESS_ACCESS_TOKEN)ProcessInformation)->Token;
                    Status = STATUS_SUCCESS;
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;

                if(NT_SUCCESS(Status))
                {
                    /* in case we had success reading from the buffer, perform the actual task */
                    Status = PspAssignPrimaryToken(Process, TokenHandle);
                }
                break;
            }

        case ProcessDefaultHardErrorMode:
            {
                _SEH_TRY
                {
                    InterlockedExchange((LONG*)&Process->DefaultHardErrorProcessing,
                        *(PLONG)ProcessInformation);
                    Status = STATUS_SUCCESS;
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;
                break;
            }

        case ProcessSessionInformation:
            {
                PROCESS_SESSION_INFORMATION SessionInfo;
                Status = STATUS_SUCCESS;

                RtlZeroMemory(&SessionInfo, sizeof(SessionInfo));

                _SEH_TRY
                {
                    /* copy the structure to the stack */
                    SessionInfo = *(PPROCESS_SESSION_INFORMATION)ProcessInformation;
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;

                if(NT_SUCCESS(Status))
                {
                    /* we successfully copied the structure to the stack, continue processing */

                    /*
                    * setting the session id requires the SeTcbPrivilege!
                    */
                    if(!SeSinglePrivilegeCheck(SeTcbPrivilege,
                        PreviousMode))
                    {
                        DPRINT1("NtSetInformationProcess: Caller requires the SeTcbPrivilege privilege for setting ProcessSessionInformation!\n");
                        /* can't set the session id, bail! */
                        Status = STATUS_PRIVILEGE_NOT_HELD;
                        break;
                    }

                    /* FIXME - update the session id for the process token */

                    Status = PsLockProcess(Process, FALSE);
                    if(NT_SUCCESS(Status))
                    {
                        Process->Session = SessionInfo.SessionId;

                        /* Update the session id in the PEB structure */
                        if(Process->Peb != NULL)
                        {
                            /* we need to attach to the process to make sure we're in the right
                            context to access the PEB structure */
                            KeAttachProcess(&Process->Pcb);

                            _SEH_TRY
                            {
                                /* FIXME: Process->Peb->SessionId = SessionInfo.SessionId; */

                                Status = STATUS_SUCCESS;
                            }
                            _SEH_HANDLE
                            {
                                Status = _SEH_GetExceptionCode();
                            }
                            _SEH_END;

                            KeDetachProcess();
                        }

                        PsUnlockProcess(Process);
                    }
                }
                break;
            }

        case ProcessPriorityClass:
            {
                PROCESS_PRIORITY_CLASS ppc;

                _SEH_TRY
                {
                    ppc = *(PPROCESS_PRIORITY_CLASS)ProcessInformation;
                }
                _SEH_HANDLE
                {
                    Status = _SEH_GetExceptionCode();
                }
                _SEH_END;

                if(NT_SUCCESS(Status))
                {
                }

                break;
            }

        case ProcessLdtInformation:
        case ProcessLdtSize:
        case ProcessIoPortHandlers:
        case ProcessWorkingSetWatch:
        case ProcessUserModeIOPL:
        case ProcessEnableAlignmentFaultFixup:
        case ProcessAffinityMask:
            Status = STATUS_NOT_IMPLEMENTED;
            break;

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
    ObDereferenceObject(Process);
    return(Status);
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
