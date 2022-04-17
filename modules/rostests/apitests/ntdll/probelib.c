/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Small library with probing utilities for thread/process classes information
 * COPYRIGHT:   Copyright 2020 George Bișoc <george.bisoc@reactos.org>
 */

#include "precomp.h"
#include <internal/ps_i.h>

VOID
QuerySetProcessValidator(
    _In_ ALIGNMENT_PROBE_MODE ValidationMode,
    _In_ ULONG InfoClassIndex,
    _In_ PVOID InfoPointer,
    _In_ ULONG InfoLength,
    _In_ NTSTATUS ExpectedStatus)
{
    NTSTATUS Status, SpecialStatus = STATUS_SUCCESS;

    /* Before doing anything, check if we want query or set validation */
    switch (ValidationMode)
    {
        case QUERY:
        {
            switch (InfoClassIndex)
            {
                case ProcessWorkingSetWatch:
                {
                    SpecialStatus = STATUS_UNSUCCESSFUL;
                    break;
                }

                case ProcessHandleTracing:
                {
                    SpecialStatus = STATUS_INVALID_PARAMETER;
                    break;
                }

                /*
                 * This class returns an arbitrary size pointed by InformationLength
                 * which equates to the image filename of the process. Such status
                 * is returned in an invalid address query (STATUS_ACCESS_VIOLATION)
                 * where the function expects STATUS_INFO_LENGTH_MISMATCH instead.
                */
                case ProcessImageFileName:
                {
                    SpecialStatus = STATUS_INFO_LENGTH_MISMATCH;
                    break;
                }

                /* These classes don't belong in the query group */
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
                {
                    SpecialStatus = STATUS_INVALID_INFO_CLASS;
                    break;
                }

                /* These classes don't exist in Server 2003 */
                case ProcessIoPriority:
                case ProcessTlsInformation:
                case ProcessCycleTime:
                case ProcessPagePriority:
                case ProcessInstrumentationCallback:
                case ProcessThreadStackAllocation:
                case ProcessWorkingSetWatchEx:
                case ProcessImageFileNameWin32:
                case ProcessImageFileMapping:
                case ProcessAffinityUpdateMode:
                case ProcessMemoryAllocationMode:
                {
                    SpecialStatus = STATUS_INVALID_INFO_CLASS;
                    break;
                }
            }

            /* Query the information */
            Status = NtQueryInformationProcess(NtCurrentProcess(),
                                               InfoClassIndex,
                                               InfoPointer,
                                               InfoLength,
                                               NULL);

            /* And probe the results we've got */
            ok(Status == ExpectedStatus || Status == SpecialStatus || Status == STATUS_DATATYPE_MISALIGNMENT,
                "0x%lx or special status (0x%lx) expected but got 0x%lx for class information %lu in query information process operation!\n", ExpectedStatus, SpecialStatus, Status, InfoClassIndex);
            break;
        }

        case SET:
        {
            switch (InfoClassIndex)
            {
                case ProcessIoPortHandlers:
                {
                    SpecialStatus = STATUS_INVALID_PARAMETER;
                    break;
                }

                /*
                 * This class returns STATUS_SUCCESS when testing
                 * for STATUS_ACCESS_VIOLATION (setting an invalid address).
                 */
                case ProcessWorkingSetWatch:
                {
                    SpecialStatus = STATUS_PORT_ALREADY_SET;
                    break;
                }

                case ProcessUserModeIOPL:
                {
                    SpecialStatus = STATUS_PRIVILEGE_NOT_HELD;
                    break;
                }

                /* These classes don't belong in the set group */
                case ProcessBasicInformation:
                case ProcessIoCounters:
                case ProcessVmCounters:
                case ProcessTimes:
                case ProcessDebugPort:
                case ProcessPooledUsageAndLimits:
                case ProcessHandleCount:
                case ProcessWow64Information:
                case ProcessImageFileName:
                case ProcessLUIDDeviceMapsEnabled:
                case ProcessDebugObjectHandle:
                case ProcessCookie:
                case ProcessImageInformation:
                {
                    SpecialStatus = STATUS_INVALID_INFO_CLASS;
                    break;
                }

                /* These classes don't exist in Server 2003 */
                case ProcessIoPriority:
                case ProcessTlsInformation:
                case ProcessCycleTime:
                case ProcessPagePriority:
                case ProcessInstrumentationCallback:
                case ProcessThreadStackAllocation:
                case ProcessWorkingSetWatchEx:
                case ProcessImageFileNameWin32:
                case ProcessImageFileMapping:
                case ProcessAffinityUpdateMode:
                case ProcessMemoryAllocationMode:
                {
                    SpecialStatus = STATUS_INVALID_INFO_CLASS;
                    break;
                }

                /* Alignment probing is not performed for these classes */
                case ProcessEnableAlignmentFaultFixup:
                case ProcessPriorityClass:
                case ProcessForegroundInformation:
                {
                    SpecialStatus = STATUS_ACCESS_VIOLATION;
                    break;
                }
            }

            /* Set the information */
            Status = NtSetInformationProcess(NtCurrentProcess(),
                                             InfoClassIndex,
                                             InfoPointer,
                                             InfoLength);

            /* And probe the results we've got */
            ok(Status == ExpectedStatus || Status == SpecialStatus || Status == STATUS_DATATYPE_MISALIGNMENT || Status == STATUS_SUCCESS,
                "0x%lx or special status (0x%lx) expected but got 0x%lx for class information %lu in set information process operation!\n", ExpectedStatus, SpecialStatus, Status, InfoClassIndex);
            break;
        }

        default:
            break;
    }
}

VOID
QuerySetThreadValidator(
    _In_ ALIGNMENT_PROBE_MODE ValidationMode,
    _In_ ULONG InfoClassIndex,
    _In_ PVOID InfoPointer,
    _In_ ULONG InfoLength,
    _In_ NTSTATUS ExpectedStatus)
{
    NTSTATUS Status, SpecialStatus = STATUS_SUCCESS;

    /* Before doing anything, check if we want query or set validation */
    switch (ValidationMode)
    {
        case QUERY:
        {
            switch (InfoClassIndex)
            {
                /* These classes don't belong in the query group */
                case ThreadPriority:
                case ThreadBasePriority:
                case ThreadAffinityMask:
                case ThreadImpersonationToken:
                case ThreadEnableAlignmentFaultFixup:
                case ThreadZeroTlsCell:
                case ThreadIdealProcessor:
                case ThreadSetTlsArrayAddress:
                case ThreadHideFromDebugger:
                case ThreadSwitchLegacyState:
                {
                    SpecialStatus = STATUS_INVALID_INFO_CLASS;
                    break;
                }

                /* These classes don't exist in Server 2003 SP2 */
                case ThreadEventPair_Reusable:
                case ThreadLastSystemCall:
                case ThreadIoPriority:
                case ThreadCycleTime:
                case ThreadPagePriority:
                case ThreadActualBasePriority:
                case ThreadTebInformation:
                case ThreadCSwitchMon:
                {
                    SpecialStatus = STATUS_INVALID_INFO_CLASS;
                    break;
                }
            }

            /* Query the information */
            Status = NtQueryInformationThread(NtCurrentThread(),
                                              InfoClassIndex,
                                              InfoPointer,
                                              InfoLength,
                                              NULL);

            /* And probe the results we've got */
            ok(Status == ExpectedStatus || Status == SpecialStatus || Status == STATUS_DATATYPE_MISALIGNMENT,
                "0x%lx or special status (0x%lx) expected but got 0x%lx for class information %lu in query information thread operation!\n", ExpectedStatus, SpecialStatus, Status, InfoClassIndex);
            break;
        }

        case SET:
        {
            switch (InfoClassIndex)
            {
                /* This class is not implemented in Windows Server 2003 SP2 */
                case ThreadSwitchLegacyState:
                {
                    SpecialStatus = STATUS_NOT_IMPLEMENTED;
                    break;
                }

                /*
                 * This class doesn't take a strict type for size length.
                 * The function happily succeds on an information length
                 * mismatch scenario with STATUS_SUCCESS.
                 */
                case ThreadHideFromDebugger:
                {
                    SpecialStatus = STATUS_INFO_LENGTH_MISMATCH;
                    break;
                }

                /* These classes don't belong in the set group */
                case ThreadBasicInformation:
                case ThreadTimes:
                case ThreadDescriptorTableEntry:
                case ThreadPerformanceCount:
                case ThreadAmILastThread:
                case ThreadIsIoPending:
                case ThreadIsTerminated:
                {
                    SpecialStatus = STATUS_INVALID_INFO_CLASS;
                    break;
                }

                /* These classes don't exist in Server 2003 SP2 */
                case ThreadEventPair_Reusable:
                case ThreadLastSystemCall:
                case ThreadIoPriority:
                case ThreadCycleTime:
                case ThreadPagePriority:
                case ThreadActualBasePriority:
                case ThreadTebInformation:
                case ThreadCSwitchMon:
                {
                    SpecialStatus = STATUS_INVALID_INFO_CLASS;
                    break;
                }

                /* Alignment probing is not performed for this class */
                case ThreadEnableAlignmentFaultFixup:
                {
                    SpecialStatus = STATUS_ACCESS_VIOLATION;
                    break;
                }
            }

            /* Set the information */
            Status = NtSetInformationThread(NtCurrentThread(),
                                            InfoClassIndex,
                                            InfoPointer,
                                            InfoLength);

            /* And probe the results we've got */
            ok(Status == ExpectedStatus || Status == SpecialStatus || Status == STATUS_DATATYPE_MISALIGNMENT || Status == STATUS_SUCCESS,
                "0x%lx or special status (0x%lx) expected but got 0x%lx for class information %lu in set information thread operation!\n", ExpectedStatus, SpecialStatus, Status, InfoClassIndex);
        }

        default:
            break;
    }
}
