/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager debuggging routines
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
NTAPI
PopReportWatchdogTime(
    _In_ PPOP_IRP_DATA IrpData)
{
    PIRP Irp = IrpData->Irp;
    PDEVICE_OBJECT DeviceObject = IrpData->CurrentDevice;
    ULONGLONG TimeRemaining = IrpData->WatchdogStart;

    if (TimeRemaining == 540)
    {
        DbgPrint("IRP watchdog has reached 1 minute until time-out (DO 0x%p, IRP 0x%p, IrpData 0x%p)\n",
                 DeviceObject, Irp, IrpData);
    }

    if (TimeRemaining == 480)
    {
        DbgPrint("IRP watchdog has reached 2 minutes until time-out (DO 0x%p, IRP 0x%p, IrpData 0x%p)\n",
                 DeviceObject, Irp, IrpData);
    }

    if (TimeRemaining == 360)
    {
        DbgPrint("IRP watchdog has reached 4 minutes until time-out (DO 0x%p, IRP 0x%p, IrpData 0x%p)\n",
                 DeviceObject, Irp, IrpData);
    }

    if (TimeRemaining == 300)
    {
        DbgPrint("WARNING, SYSTEM IS DELAYED! IRP watchdog has reached 5 minutes until time-out (DO 0x%p, IRP 0x%p, IrpData 0x%p)\n",
                 DeviceObject, Irp, IrpData);
    }

    if (TimeRemaining == 240)
    {
        DbgPrint("WARNING, SYSTEM IS DELAYED! IRP watchdog has reached 6 minutes until time-out (DO 0x%p, IRP 0x%p, IrpData 0x%p)\n",
                 DeviceObject, Irp, IrpData);
    }

    if (TimeRemaining == 180)
    {
        DbgPrint("WARNING, SYSTEM IS DELAYED FOR TOO LONG, DEATH IS IMMINENT! IRP watchdog has reached 7 minutes until time-out (DO 0x%p, IRP 0x%p, IrpData 0x%p)\n",
                 DeviceObject, Irp, IrpData);
    }

    if (TimeRemaining == 120)
    {
        DbgPrint("CRITICAL, THE SYSTEM IS APPROACHING DEATH! IRP watchdog has reached 8 minutes until time-out (DO 0x%p, IRP 0x%p, IrpData 0x%p)\n",
                 DeviceObject, Irp, IrpData);
    }
}

PCSTR
NTAPI
PopTranslateSystemPowerStateToString(
    _In_ SYSTEM_POWER_STATE SystemState)
{
    switch (SystemState)
    {
        case PowerSystemWorking:
        {
            return "S0";
        }

        case PowerSystemSleeping1:
        {
            return "S1";
        }

        case PowerSystemSleeping2:
        {
            return "S2";
        }

        case PowerSystemSleeping3:
        {
            return "S3";
        }

        case PowerSystemHibernate:
        {
            return "S4";
        }

        case PowerSystemShutdown:
        {
            return "S5";
        }

        default:
        {
            return "Unknown";
        }
    }

    return NULL;
}

PCSTR
NTAPI
PopTranslateDevicePowerStateToString(
    _In_ DEVICE_POWER_STATE DeviceState)
{
    switch (DeviceState)
    {
        case PowerDeviceD0:
        {
            return "D0";
        }

        case PowerDeviceD1:
        {
            return "D1";
        }

        case PowerDeviceD2:
        {
            return "D2";
        }

        case PowerDeviceD3:
        {
            return "D3";
        }

        default:
        {
            return "Unknown";
        }
    }

    return NULL;
}

PCWSTR
NTAPI
PopGetPowerInformationLevelName(
    _In_ POWER_INFORMATION_LEVEL InformationLevel)
{
    switch (InformationLevel)
    {
        case SystemPowerPolicyAc:
        {
            return L"SystemPowerPolicyAc";
        }

        case SystemPowerPolicyDc:
        {
            return L"SystemPowerPolicyDc";
        }

        case VerifySystemPolicyAc:
        {
            return L"VerifySystemPolicyAc";
        }

        case VerifySystemPolicyDc:
        {
            return L"VerifySystemPolicyDc";
        }

        case SystemPowerCapabilities:
        {
            return L"SystemPowerCapabilities";
        }

        case SystemBatteryState:
        {
            return L"SystemBatteryState";
        }

        case SystemPowerStateHandler:
        {
            return L"SystemPowerStateHandler";
        }

        case ProcessorStateHandler:
        {
            return L"ProcessorStateHandler";
        }

        case SystemPowerPolicyCurrent:
        {
            return L"SystemPowerPolicyCurrent";
        }

        case AdministratorPowerPolicy:
        {
            return L"AdministratorPowerPolicy";
        }

        case SystemReserveHiberFile:
        {
            return L"SystemReserveHiberFile";
        }

        case ProcessorInformation:
        {
            return L"ProcessorInformation";
        }

        case SystemPowerInformation:
        {
            return L"SystemPowerInformation";
        }

        case ProcessorStateHandler2:
        {
            return L"ProcessorStateHandler2";
        }

        case LastWakeTime:
        {
            return L"LastWakeTime";
        }

        case LastSleepTime:
        {
            return L"LastSleepTime";
        }

        case SystemExecutionState:
        {
            return L"SystemExecutionState";
        }

        case SystemPowerStateNotifyHandler:
        {
            return L"SystemPowerStateNotifyHandler";
        }

        case ProcessorPowerPolicyAc:
        {
            return L"ProcessorPowerPolicyAc";
        }

        case ProcessorPowerPolicyDc:
        {
            return L"ProcessorPowerPolicyDc";
        }

        case VerifyProcessorPowerPolicyAc:
        {
            return L"VerifyProcessorPowerPolicyAc";
        }

        case VerifyProcessorPowerPolicyDc:
        {
            return L"VerifyProcessorPowerPolicyDc";
        }

        case ProcessorPowerPolicyCurrent:
        {
            return L"ProcessorPowerPolicyCurrent";
        }

        case SystemPowerStateLogging:
        {
            return L"SystemPowerStateLogging";
        }

        case SystemPowerLoggingEntry:
        {
            return L"SystemPowerLoggingEntry";
        }

        case SetPowerSettingValue:
        {
            return L"SetPowerSettingValue";
        }

        case NotifyUserPowerSetting:
        {
            return L"NotifyUserPowerSetting";
        }

        case PowerInformationLevelUnused0:
        {
            return L"PowerInformationLevelUnused0";
        }

        case SystemMonitorHiberBootPowerOff:
        {
            return L"SystemMonitorHiberBootPowerOff";
        }

        case SystemVideoState:
        {
            return L"SystemVideoState";
        }

        case TraceApplicationPowerMessage:
        {
            return L"TraceApplicationPowerMessage";
        }

        case TraceApplicationPowerMessageEnd:
        {
            return L"TraceApplicationPowerMessageEnd";
        }

        case ProcessorPerfStates:
        {
            return L"ProcessorPerfStates";
        }

        case ProcessorIdleStates:
        {
            return L"ProcessorIdleStates";
        }

        case ProcessorCap:
        {
            return L"ProcessorCap";
        }

        case SystemWakeSource:
        {
            return L"SystemWakeSource";
        }

        case SystemHiberFileInformation:
        {
            return L"SystemHiberFileInformation";
        }

        case TraceServicePowerMessage:
        {
            return L"TraceServicePowerMessage";
        }

        case ProcessorLoad:
        {
            return L"ProcessorLoad";
        }

        case PowerShutdownNotification:
        {
            return L"PowerShutdownNotification";
        }

        case MonitorCapabilities:
        {
            return L"MonitorCapabilities";
        }

        case SessionPowerInit:
        {
            return L"SessionPowerInit";
        }

        case SessionDisplayState:
        {
            return L"SessionDisplayState";
        }

        case PowerRequestCreate:
        {
            return L"PowerRequestCreate";
        }

        case PowerRequestAction:
        {
            return L"PowerRequestAction";
        }

        case GetPowerRequestList:
        {
            return L"GetPowerRequestList";
        }

        case ProcessorInformationEx:
        {
            return L"ProcessorInformationEx";
        }

        case NotifyUserModeLegacyPowerEvent:
        {
            return L"NotifyUserModeLegacyPowerEvent";
        }

        case GroupPark:
        {
            return L"GroupPark";
        }

        case ProcessorIdleDomains:
        {
            return L"ProcessorIdleDomains";
        }

        case WakeTimerList:
        {
            return L"WakeTimerList";
        }

        case SystemHiberFileSize:
        {
            return L"SystemHiberFileSize";
        }

        case ProcessorIdleStatesHv:
        {
            return L"ProcessorIdleStatesHv";
        }

        case ProcessorPerfStatesHv:
        {
            return L"ProcessorPerfStatesHv";
        }

        case ProcessorPerfCapHv:
        {
            return L"ProcessorPerfCapHv";
        }

        case ProcessorSetIdle:
        {
            return L"ProcessorSetIdle";
        }

        case LogicalProcessorIdling:
        {
            return L"LogicalProcessorIdling";
        }

        case UserPresence:
        {
            return L"UserPresence";
        }

        case PowerSettingNotificationName:
        {
            return L"PowerSettingNotificationName";
        }

        case GetPowerSettingValue:
        {
            return L"GetPowerSettingValue";
        }

        case IdleResiliency:
        {
            return L"IdleResiliency";
        }

        case SessionRITState:
        {
            return L"SessionRITState";
        }

        case SessionConnectNotification:
        {
            return L"SessionConnectNotification";
        }

        case SessionPowerCleanup:
        {
            return L"SessionPowerCleanup";
        }

        case SessionLockState:
        {
            return L"SessionLockState";
        }

        case SystemHiberbootState:
        {
            return L"SystemHiberbootState";
        }

        case PlatformInformation:
        {
            return L"PlatformInformation";
        }

        case PdcInvocation:
        {
            return L"PdcInvocation";
        }

        case MonitorInvocation:
        {
            return L"MonitorInvocation";
        }

        case FirmwareTableInformationRegistered:
        {
            return L"FirmwareTableInformationRegistered";
        }

        case SetShutdownSelectedTime:
        {
            return L"SetShutdownSelectedTime";
        }

        case SuspendResumeInvocation:
        {
            return L"SuspendResumeInvocation";
        }

        case PlmPowerRequestCreate:
        {
            return L"PlmPowerRequestCreate";
        }

        case ScreenOff:
        {
            return L"ScreenOff";
        }

        case CsDeviceNotification:
        {
            return L"CsDeviceNotification";
        }

        case PlatformRole:
        {
            return L"PlatformRole";
        }

        case LastResumePerformance:
        {
            return L"LastResumePerformance";
        }

        case DisplayBurst:
        {
            return L"DisplayBurst";
        }

        case ExitLatencySamplingPercentage:
        {
            return L"ExitLatencySamplingPercentage";
        }

        case RegisterSpmPowerSettings:
        {
            return L"RegisterSpmPowerSettings";
        }

        case PlatformIdleStates:
        {
            return L"PlatformIdleStates";
        }

        case ProcessorIdleVeto:
        {
            return L"ProcessorIdleVeto";
        }

        case PlatformIdleVeto:
        {
            return L"PlatformIdleVeto";
        }

        case SystemBatteryStatePrecise:
        {
            return L"SystemBatteryStatePrecise";
        }

        case ThermalEvent:
        {
            return L"ThermalEvent";
        }

        case PowerRequestActionInternal:
        {
            return L"PowerRequestActionInternal";
        }

        case BatteryDeviceState:
        {
            return L"BatteryDeviceState";
        }

        case PowerInformationInternal:
        {
            return L"PowerInformationInternal";
        }

        case ThermalStandby:
        {
            return L"ThermalStandby";
        }

        case SystemHiberFileType:
        {
            return L"SystemHiberFileType";
        }

        case PhysicalPowerButtonPress:
        {
            return L"PhysicalPowerButtonPress";
        }

        case QueryPotentialDripsConstraint:
        {
            return L"QueryPotentialDripsConstraint";
        }

        case EnergyTrackerCreate:
        {
            return L"EnergyTrackerCreate";
        }

        case EnergyTrackerQuery:
        {
            return L"EnergyTrackerQuery";
        }

        case UpdateBlackBoxRecorder:
        {
            return L"UpdateBlackBoxRecorder";
        }

        case SessionAllowExternalDmaDevices:
        {
            return L"SessionAllowExternalDmaDevices";
        }

        default:
        {
            return L"Unknown Class";
        }
    }

    return NULL;
}

VOID
NTAPI
PopReportBatteryInformation(
    _In_ PBATTERY_INFORMATION Info)
{
    DbgPrint("================== BATTERY INFORMATION ==================\n");
    DbgPrint("Capabilities -> 0x%lx\n", Info->Capabilities);
    DbgPrint("Technology -> %u\n", Info->Technology);
    DbgPrint("Chemistry -> %c\n", Info->Chemistry[4]);
    DbgPrint("DesignedCapacity -> %u\n", Info->DesignedCapacity);
    DbgPrint("FullChargedCapacity -> %u\n", Info->FullChargedCapacity);
    DbgPrint("DefaultAlert1 -> %u\n", Info->DefaultAlert1);
    DbgPrint("DefaultAlert2 -> %u\n", Info->DefaultAlert2);
    DbgPrint("CriticalBias -> %u\n", Info->CriticalBias);
    DbgPrint("CycleCount -> %u\n\n", Info->CycleCount);
}

VOID
NTAPI
PopReportBatteryStatus(
    _In_ PBATTERY_STATUS Status)
{
    LARGE_INTEGER Time;
    TIME_FIELDS CurrentTime;

    /* Query the current system time and convert it to a readable format */
    KeQuerySystemTime(&Time);
    ExSystemTimeToLocalTime(&Time, &Time);
    RtlTimeToTimeFields(&Time, &CurrentTime);

    DbgPrint("================== BATTERY STATUS (Last Update: %d:%d:%d) ==================\n",
             CurrentTime.Hour, CurrentTime.Minute, CurrentTime.Second);
    DbgPrint("PowerState -> 0x%lx\n", Status->PowerState);
    DbgPrint("Capacity -> %u\n", Status->Capacity);
    DbgPrint("Voltage -> %u\n", Status->Voltage);
    DbgPrint("Rate -> %d\n\n", Status->Rate);
}

_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
PopScanForActivePowerRequestsDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    ULONG RequestCount = 0;
    KLOCK_QUEUE_HANDLE LockHandle;
    PLIST_ENTRY ListHead, NextEntry;
    PPOP_POWER_REQUEST PowerRequest;

    /* Don't bother if the list is empty */
    if (IsListEmpty(&PopPowerRequestsList))
    {
        return;
    }

    /*
     * We don't want any other threads to touch the list as we are logging
     * out information so protect ourselves with a lock.
     */
    PopAcquirePowerRequestLock(&LockHandle);

    /* We have some power requests, print the banner and the number of requests created */
    DbgPrint("================== POWER REQUESTS ==================\n\n");
    DbgPrint("There are %lu kernel-mode power requests and %lu user-mode power requests created.\n\n",
             PopTotalKernelPowerRequestsCount, PopTotalUserPowerRequestsCount);

    /* Now loop each power request and print out the details */
    ListHead = &PopPowerRequestsList;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        PowerRequest = CONTAINING_RECORD(NextEntry, POP_POWER_REQUEST, Link);
        DbgPrint("======== #%lu POWER REQUEST DUMP INFO ========\n\n", RequestCount);

        if (PowerRequest->RequestorMode == KernelMode)
        {
            DbgPrint("Power request is coming from kernel mode\n");
        }
        else
        {
            DbgPrint("Power request is coming from user mode\n");
        }

        DbgPrint("Device requestor -> 0x%p\n", PowerRequest->DeviceRequestor);
        DbgPrint("Power request is %s\n", PowerRequest->Legacy ? "Legacy" : "Vista+");

        DbgPrint("Active system required requests -> %lu\n", PowerRequest->ActiveRequests.ActiveSystemRequiredRequests);
        DbgPrint("Active display required requests -> %lu\n", PowerRequest->ActiveRequests.ActiveDisplayRequiredRequests);
        DbgPrint("Active away mode requests -> %lu\n", PowerRequest->ActiveRequests.ActiveAwayModeRequiredRequests);
        DbgPrint("Active execution requests -> %lu\n", PowerRequest->ActiveRequests.ActiveExecutionRequests);
        DbgPrint("User present requests -> %lu\n", PowerRequest->ActiveRequests.ActiveUserPresentRequests);

        DbgPrint("Power request use count -> %lu\n", PowerRequest->UseCount);
        DbgPrint("Power request image file creator -> %s\n", PowerRequest->ImageFileName);

        if (PowerRequest->Context != NULL)
        {
            DbgPrint("Power request Reason Context pointer -> 0x%p\n", PowerRequest->Context);
        }

        RequestCount++;
    }

    PopReleasePowerRequestLock(&LockHandle);
}

VOID
NTAPI
PopReportPowerRequest(
    _In_ PPOP_POWER_REQUEST PowerRequest)
{
    ULONG StringIndex;

    DbgPrint("================== A NEW POWER REQUEST HAS BEEN CREATED ==================\n\n");
    DbgPrint("Device requestor -> 0x%p\n", PowerRequest->DeviceRequestor);
    DbgPrint("Image file creator -> %s\n", PowerRequest->ImageFileName);

    if (PowerRequest->Context != NULL)
    {
        if (PowerRequest->Context->Flags == DIAGNOSTIC_REASON_SIMPLE_STRING)
        {
            DbgPrint("Reason context of this power request:\n");
            DbgPrint("       %wZ\n", PowerRequest->Context->SimpleString);
        }
        else
        {
            DbgPrint("Reason context of this power request (detailed):\n\n");
            if (PowerRequest->Context->ResourceFileName.Buffer != NULL)
            {
                DbgPrint("Resource filename -> %wZ\n", PowerRequest->Context->ResourceFileName);
            }

            for (StringIndex = 0; StringIndex < PowerRequest->Context->StringCount; StringIndex++)
            {
                DbgPrint("      %wZ\n", PowerRequest->Context->ReasonStrings[StringIndex]);
            }
        }
    }
}

/* EOF */
