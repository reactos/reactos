/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Power states (Active & Idle) management for System and Devices infrastructure
 * COPYRIGHT:   Copyright 2023 George Bișoc <george.bisoc@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

KDPC PopIdleScanDevicesDpc;
KTIMER PopIdleScanDevicesTimer;
LIST_ENTRY PopIdleDetectList;
ULONG PopIdleScanIntervalInSeconds = 1;
POWER_STATE_HANDLER PopDefaultPowerStateHandlers[PowerStateMaximum] = {0};

/* PRIVATE FUNCTIONS **********************************************************/

/* PUBLIC FUNCTIONS ***********************************************************/

_Function_class_(KDEFERRED_ROUTINE)
VOID
NTAPI
PopScanForIdleDevicesDpcRoutine(
    _In_ PKDPC Dpc,
    _In_ PVOID DeferredContext,
    _In_ PVOID SystemArgument1,
    _In_ PVOID SystemArgument2)
{
    NTSTATUS Status;
    KIRQL OldIrql;
    PLIST_ENTRY Entry;
    POWER_STATE State;
    PDEVICE_OBJECT DeviceObject;
    POP_DEVICE_IDLE_TYPE IdleType;
    ULONG IdleTreshold, NewIdleCount;
    PDEVICE_OBJECT_POWER_EXTENSION Dope;

    /* We do not care for these parameters */
    UNREFERENCED_PARAMETER(Dpc);
    UNREFERENCED_PARAMETER(SystemArgument1);
    UNREFERENCED_PARAMETER(SystemArgument2);

    /* Begin iterating over the global idle detect list for any registered devices */
    PopAcquireDopeLock(&OldIrql);
    for (Entry = PopIdleDetectList.Flink;
         Entry != &PopIdleDetectList;
         Entry = Entry->Flink)
    {
        /* Capture the DOPE of this device */
        Dope = CONTAINING_RECORD(Entry, DEVICE_OBJECT_POWER_EXTENSION, IdleList);
        ASSERT(Dope != NULL);

        /* Cache the device object */
        DeviceObject = Dope->DeviceObject;

        /*
         * FIRST RULE of the thumb: look for active busy references the caller
         * is still holding onto this device. A reference above 0 means the caller
         * has an active outstanding instance call of PoStartDeviceBusy, and this
         * could go for as much as the caller wishes, until it explicitly tells the
         * power manager that the device stopped being busy. We do not touch the busy
         * count here.
         */
        if (Dope->BusyReference > 0)
        {
            /*
             * The act of disabling the idle counter as per MSDN documentation actually means
             * resetting the said counter back to 0 if the device used to be idling before.
             */
            if (Dope->IdleCount > 0)
            {
                Dope->IdleCount = 0;
            }

            DPRINT("The device object (0x%p) with DOPE (0x%p) is busy", DeviceObject, Dope);
            continue;
        }

        /*
         * SECOND RULE of the thumb: this device does not have active busy references
         * but it is being held busy for a brief period of time. The function which is
         * responsible for that is PoSetDeviceBusyEx.
         */
        if (Dope->BusyCount > 0)
        {
            /*
             * As this device was beind held for a short period of time, now it is
             * time to decrease the busy count by one. The caller is responsible to
             * keep it busy with multiple PoSetDeviceBusyEx requests.
             */
            Dope->BusyCount--;

            /*
             * If this device used to be idling before at the time of declaring itself
             * as busy, reset its idle counter.
             */
            if (Dope->IdleCount > 0)
            {
                Dope->IdleCount = 0;
            }

            DPRINT("The device object (0x%p) with DOPE (0x%p) is busy for a brief period of time", DeviceObject, Dope);
            continue;
        }

        /* This device is not busy, increment the idle counter */
        NewIdleCount = InterlockedIncrement((volatile LONG *)&Dope->IdleCount);

        /* Obtain the idle time treshold based on the device type */
        IdleType = Dope->IdleType;
        if (IdleType == DeviceIdleNormal)
        {
            /*
             * Grab the treshold from the registered conservation or performance
             * idle time, depending on the power policy the power manager has
             * currently enforced.
             */
            if (PopDefaultPowerPolicy == &PopDcPowerPolicy)
            {
                /* The system runs on batteries, so favor the conservation idle time */
                IdleTreshold = Dope->ConservationIdleTime;
            }
            else
            {
                /*
                 * The system runs on AC power (typically from PSU or its batteries
                 * are charging), favor the performance idle time.
                 */
                IdleTreshold = Dope->PerformanceIdleTime;
            }
        }
        else if (IdleType == DeviceIdleDisk)
        {
            /*
             * This device is a disk or mass storage device, grab the treshold from
             * the default spin-down idle time of the currently enforced power policy.
             */
            IdleTreshold = PopDefaultPowerPolicy->SpindownTimeout;
        }
        else
        {
            /* The power manager does not know of this device, bail out */
            DPRINT1("The device (0x%p) with DOPE (0x%p) is of an unknown type (%lu), crash is imminent\n",
                    DeviceObject, Dope, IdleType);
            KeBugCheckEx(INTERNAL_POWER_ERROR,
                         0x200,
                         POP_IDLE_DETECT_UNKNOWN_DEVICE,
                         (ULONG_PTR)DeviceObject,
                         (ULONG_PTR)Dope);
        }

        /* Send a power IRP to this device if it is idling for long enough */
        if (IdleTreshold && (NewIdleCount == IdleTreshold))
        {
            State.DeviceState = Dope->IdleState;
            Status = PopRequestPowerIrp(DeviceObject,
                                        IRP_MN_SET_POWER,
                                        State,
                                        FALSE,
                                        NULL,
                                        NULL,
                                        NULL);
            NT_ASSERT(Status == STATUS_PENDING);
            Dope->CurrentState = Dope->IdleState;
        }
    }

    PopReleaseDopeLock(OldIrql);
}

VOID
NTAPI
PopCleanupPowerState(
    _In_ PPOWER_STATE PowerState)
{
    /* FIXME */
    UNIMPLEMENTED;
    NOTHING;
}

/* EOF */
