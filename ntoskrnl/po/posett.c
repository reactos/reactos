/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power Manager power settings callback mechanism
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

LIST_ENTRY PopPowerSettingCallbacksList;
ULONG PopPowerSettingCallbacksCount;
PKTHREAD PopPowerSettingOwnerLockThread;
FAST_MUTEX PopPowerSettingLock;

/* POWER SETTINGS DATABASE ****************************************************/

POP_POWER_SETTING_DATABASE PopPowerSettingsDatabase[POP_MAX_POWER_SETTINGS] =
{
    {&GUID_MAX_POWER_SAVINGS, NULL},
    {&GUID_MIN_POWER_SAVINGS, NULL},
    {&GUID_TYPICAL_POWER_SAVINGS, NULL},
    {&NO_SUBGROUP_GUID, NULL},
    {&ALL_POWERSCHEMES_GUID, NULL},
    {&GUID_POWERSCHEME_PERSONALITY, NULL},
    {&GUID_ACTIVE_POWERSCHEME, NULL},
    {&GUID_VIDEO_POWERDOWN_TIMEOUT, NULL},
    {&GUID_VIDEO_ANNOYANCE_TIMEOUT, NULL},
    {&GUID_VIDEO_ADAPTIVE_PERCENT_INCREASE, NULL},
    {&GUID_VIDEO_DIM_TIMEOUT, NULL},
    {&GUID_VIDEO_ADAPTIVE_POWERDOWN, NULL},
    {&GUID_MONITOR_POWER_ON, NULL},
    {&GUID_DEVICE_POWER_POLICY_VIDEO_BRIGHTNESS, NULL},
    {&GUID_DEVICE_POWER_POLICY_VIDEO_DIM_BRIGHTNESS, NULL},
    {&GUID_VIDEO_CURRENT_MONITOR_BRIGHTNESS, NULL},
    {&GUID_VIDEO_ADAPTIVE_DISPLAY_BRIGHTNESS, NULL},
    {&GUID_SESSION_DISPLAY_STATE, NULL},
    {&GUID_CONSOLE_DISPLAY_STATE, NULL},
    {&GUID_ALLOW_DISPLAY_REQUIRED, NULL},
    {&GUID_DISK_SUBGROUP, NULL},
    {&GUID_DISK_POWERDOWN_TIMEOUT, NULL},
    {&GUID_DISK_IDLE_TIMEOUT, NULL},
    {&GUID_DISK_BURST_IGNORE_THRESHOLD, NULL},
    {&GUID_DISK_ADAPTIVE_POWERDOWN, NULL},
    {&GUID_SLEEP_SUBGROUP, NULL},
    {&GUID_SLEEP_IDLE_THRESHOLD, NULL},
    {&GUID_STANDBY_TIMEOUT, NULL},
    {&GUID_UNATTEND_SLEEP_TIMEOUT, NULL},
    {&GUID_HIBERNATE_TIMEOUT, NULL},
    {&GUID_HIBERNATE_FASTS4_POLICY, NULL},
    {&GUID_CRITICAL_POWER_TRANSITION, NULL},
    {&GUID_SYSTEM_AWAYMODE, NULL},
    {&GUID_ALLOW_AWAYMODE, NULL},
    {&GUID_ALLOW_STANDBY_STATES, NULL},
    {&GUID_ALLOW_RTC_WAKE, NULL},
    {&GUID_ALLOW_SYSTEM_REQUIRED, NULL},
    {&GUID_SYSTEM_BUTTON_SUBGROUP, NULL},
    {&GUID_POWERBUTTON_ACTION, NULL},
    {&GUID_POWERBUTTON_ACTION_FLAGS, NULL},
    {&GUID_SLEEPBUTTON_ACTION, NULL},
    {&GUID_SLEEPBUTTON_ACTION_FLAGS, NULL},
    {&GUID_USERINTERFACEBUTTON_ACTION, NULL},
    {&GUID_LIDCLOSE_ACTION, NULL},
    {&GUID_LIDCLOSE_ACTION_FLAGS, NULL},
    {&GUID_LIDOPEN_POWERSTATE, NULL},
    {&GUID_BATTERY_SUBGROUP, NULL},
    {&GUID_BATTERY_DISCHARGE_ACTION_0, NULL},
    {&GUID_BATTERY_DISCHARGE_LEVEL_0, NULL},
    {&GUID_BATTERY_DISCHARGE_FLAGS_0, NULL},
    {&GUID_BATTERY_DISCHARGE_ACTION_1, NULL},
    {&GUID_BATTERY_DISCHARGE_LEVEL_1, NULL},
    {&GUID_BATTERY_DISCHARGE_FLAGS_1, NULL},
    {&GUID_BATTERY_DISCHARGE_ACTION_2, NULL},
    {&GUID_BATTERY_DISCHARGE_LEVEL_2, NULL},
    {&GUID_BATTERY_DISCHARGE_FLAGS_2, NULL},
    {&GUID_BATTERY_DISCHARGE_ACTION_3, NULL},
    {&GUID_BATTERY_DISCHARGE_LEVEL_3, NULL},
    {&GUID_BATTERY_DISCHARGE_FLAGS_3, NULL},
    {&GUID_PROCESSOR_SETTINGS_SUBGROUP, NULL},
    {&GUID_PROCESSOR_THROTTLE_POLICY, NULL},
    {&GUID_PROCESSOR_THROTTLE_MAXIMUM, NULL},
    {&GUID_PROCESSOR_THROTTLE_MINIMUM, NULL},
    {&GUID_PROCESSOR_ALLOW_THROTTLING, NULL},
    {&GUID_PROCESSOR_IDLESTATE_POLICY, NULL},
    {&GUID_PROCESSOR_PERFSTATE_POLICY, NULL},
    {&GUID_PROCESSOR_PERF_INCREASE_THRESHOLD, NULL},
    {&GUID_PROCESSOR_PERF_DECREASE_THRESHOLD, NULL},
    {&GUID_PROCESSOR_PERF_INCREASE_POLICY, NULL},
    {&GUID_PROCESSOR_PERF_DECREASE_POLICY, NULL},
    {&GUID_PROCESSOR_PERF_INCREASE_TIME, NULL},
    {&GUID_PROCESSOR_PERF_DECREASE_TIME, NULL},
    {&GUID_PROCESSOR_PERF_TIME_CHECK, NULL},
    {&GUID_PROCESSOR_PERF_BOOST_POLICY, NULL},
    {&GUID_PROCESSOR_IDLE_ALLOW_SCALING, NULL},
    {&GUID_PROCESSOR_IDLE_DISABLE, NULL},
    {&GUID_PROCESSOR_IDLE_TIME_CHECK, NULL},
    {&GUID_PROCESSOR_IDLE_DEMOTE_THRESHOLD, NULL},
    {&GUID_PROCESSOR_IDLE_PROMOTE_THRESHOLD, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_INCREASE_THRESHOLD, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_DECREASE_THRESHOLD, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_INCREASE_POLICY, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_DECREASE_POLICY, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_MAX_CORES, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_MIN_CORES, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_INCREASE_TIME, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_DECREASE_TIME, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_DECREASE_FACTOR, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_AFFINITY_HISTORY_THRESHOLD, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_AFFINITY_WEIGHTING, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_DECREASE_FACTOR, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_HISTORY_THRESHOLD, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_WEIGHTING, NULL},
    {&GUID_PROCESSOR_CORE_PARKING_OVER_UTILIZATION_THRESHOLD, NULL},
    {&GUID_PROCESSOR_PARKING_CORE_OVERRIDE, NULL},
    {&GUID_PROCESSOR_PARKING_PERF_STATE, NULL},
    {&GUID_PROCESSOR_PERF_HISTORY, NULL},
    {&GUID_SYSTEM_COOLING_POLICY, NULL},
    {&GUID_LOCK_CONSOLE_ON_WAKE, NULL},
    {&GUID_DEVICE_IDLE_POLICY, NULL},
    {&GUID_ACDC_POWER_SOURCE, NULL},
    {&GUID_LIDSWITCH_STATE_CHANGE, NULL},
    {&GUID_BATTERY_PERCENTAGE_REMAINING, NULL},
    {&GUID_IDLE_BACKGROUND_TASK, NULL},
    {&GUID_BACKGROUND_TASK_NOTIFICATION, NULL},
    {&GUID_APPLAUNCH_BUTTON, NULL},
    {&GUID_PCIEXPRESS_SETTINGS_SUBGROUP, NULL},
    {&GUID_PCIEXPRESS_ASPM_POLICY, NULL},
    {&GUID_ENABLE_SWITCH_FORCED_SHUTDOWN, NULL},
};

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

NTSTATUS
NTAPI
PopAllocatePowerSettingCallback(
    _In_opt_ PDEVICE_OBJECT DeviceObject,
    _In_ LPCGUID SettingGuid,
    _In_ PPOWER_SETTING_CALLBACK Callback,
    _In_opt_ PVOID Context,
    _Out_ PPOP_POWER_SETTING_CALLBACK *PowerSettingCallback)
{
    PPOP_POWER_SETTING_CALLBACK SettingCallback;

    PAGED_CODE();

    /* Allocate some memory pool for the power setting callback */
    SettingCallback = PopAllocatePool(sizeof(*SettingCallback),
                                      TRUE,
                                      TAG_PO_POWER_SETTING_CALLBACK);
    if (SettingCallback == NULL)
    {
        DPRINT1("Failed to allocate memory for the power setting callback\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Setup the callback fields */
    SettingCallback->DeviceObject = DeviceObject;
    SettingCallback->Callback = Callback;
    SettingCallback->Context = Context;
    SettingCallback->EnterCallback = FALSE;

    /* Initialize the link entry */
    InitializeListHead(&SettingCallback->Link);

    /* Copy the power setting GUID identifier */
    RtlCopyMemory(&SettingCallback->SettingGuid,
                  SettingGuid,
                  sizeof(GUID));

    /* Initialize the callback return event */
    KeInitializeEvent(&SettingCallback->CallbackReturned,
                      NotificationEvent,
                      FALSE);

    return STATUS_SUCCESS;
}

PPOP_POWER_SETTING_CALLBACK
NTAPI
PopFindPowerSettingCallbackByCallback(
    _In_ PPOWER_SETTING_CALLBACK Callback)
{
    PLIST_ENTRY ListHead, NextEntry;
    PPOP_POWER_SETTING_CALLBACK SettingCallback;

    PAGED_CODE();

    /* Lock down the power setting callbacks list and look for the callback */
    PopAcquirePowerSettingLock();
    ListHead = &PopPowerSettingCallbacksList;
    for (NextEntry = ListHead->Flink;
         NextEntry != ListHead;
         NextEntry = NextEntry->Flink)
    {
        /* Check if this is the callback we're looking for */
        SettingCallback = CONTAINING_RECORD(NextEntry, POP_POWER_SETTING_CALLBACK, Link);
        if (SettingCallback->Callback == Callback)
        {
            PopReleasePowerSettingLock();
            return SettingCallback;
        }
    }

    /* Otherwise we haven't found anything we looked for */
    PopReleasePowerSettingLock();
    return NULL;
}

PWORKER_THREAD_ROUTINE
NTAPI
PopGetPowerSettingHelper(
    _In_ LPCGUID SettingGuid)
{
    ULONG SettingIndex;

    PAGED_CODE();

    /* Iterate over the power settings database and look for the right setting */
    for (SettingIndex = 0;
         SettingIndex < POP_MAX_POWER_SETTINGS;
         SettingIndex++)
    {
        /* Is this what the caller asked for? */
        if (PopIsEqualGuid(PopPowerSettingsDatabase[SettingIndex].SettingGuid, SettingGuid))
        {
            return PopPowerSettingsDatabase[SettingIndex].SettingRoutine;
        }
    }

    return NULL;
}

BOOLEAN
NTAPI
PopIsPowerSettingValid(
    _In_ LPCGUID SettingGuid)
{
    //ULONG SettingIndex;

    PAGED_CODE();
    return TRUE;
}

/* EOF */
