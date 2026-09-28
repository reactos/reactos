/*
 * PROJECT:     uACPI-NT
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     System sleep state handling on the ACPI FDO
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include <uacpint.h>
#include <uacpi/registers.h>
#include <uacpi/acpi.h>
#include <uacpi/kernel_api.h>
#include <debug.h>

volatile LONG UacpiNtSystemResuming;
volatile LONG UacpiNtWakeSourceClaimed;

/* State given to \_PTS, so the resume IRP can run the matching \_WAK */
static uacpi_sleep_state UacpiNtPendingSleepState = UACPI_SLEEP_STATE_S0;

static
uacpi_sleep_state
NTAPI
UacpiNtSleepStateFromSystem(
    _In_ SYSTEM_POWER_STATE SystemState)
{
    switch (SystemState)
    {
        case PowerSystemSleeping1:
            return UACPI_SLEEP_STATE_S1;

        case PowerSystemSleeping2:
            return UACPI_SLEEP_STATE_S2;

        case PowerSystemSleeping3:
            return UACPI_SLEEP_STATE_S3;

        case PowerSystemHibernate:
            return UACPI_SLEEP_STATE_S4;

        case PowerSystemShutdown:
            return UACPI_SLEEP_STATE_S5;

        default:
            return UACPI_SLEEP_STATE_S0;
    }
}

static
VOID
NTAPI
UacpiNtResumeFromSleep(VOID)
{
    uacpi_sleep_state WokeFrom = UacpiNtPendingSleepState;
    uacpi_status UacpiStatus;

    /* This also runs \_SI._SST(1) */
    UacpiStatus = uacpi_wake_from_sleep_state(WokeFrom);
    if (uacpi_unlikely_error(UacpiStatus))
        DPRINT1("uACPI-NT: \\_WAK for S%d failed: %s\n", WokeFrom, uacpi_status_to_string(UacpiStatus));
    else
        DPRINT("uACPI-NT: \\_WAK for S%d done\n", WokeFrom);

    UacpiNtPendingSleepState = UACPI_SLEEP_STATE_S0;

    /* Power resources and wake masks go back before any D0 IRP shows up */
    UacpiNtPowerResResume();
    UacpiNtWakeRestoreSuspended();

    /* Only hibernate removes power from the wake circuitry */
    if (WokeFrom == UACPI_SLEEP_STATE_S4)
        UacpiNtWakeReArmAfterHibernate();

    /* Firmware may have reset PCI link routing */
    UacpiNtIrqLinksResume();
}

static
VOID
NTAPI
UacpiNtPrepareForSleep(
    _In_ SYSTEM_POWER_STATE SystemState,
    _In_ POWER_ACTION ShutdownType)
{
    uacpi_sleep_state Target = UacpiNtSleepStateFromSystem(SystemState);
    uacpi_status UacpiStatus;

    /* Runs \_PTS and \_SI._SST for the target state */
    UacpiStatus = uacpi_prepare_for_sleep_state(Target);
    if (uacpi_unlikely_error(UacpiStatus))
        DPRINT1("uACPI-NT: \\_PTS for S%d failed: %s\n", Target, uacpi_status_to_string(UacpiStatus));
    else
        DPRINT("uACPI-NT: \\_PTS for S%d done\n", Target);

    /* The HAL writes SLP_TYP and decides between off and reset */
    DPRINT("uACPI-NT: entering S%d, shutdown type %d\n", Target, ShutdownType);

    if (Target == UACPI_SLEEP_STATE_S5)
    {
        /* Nothing may wake the machine from soft off */
        uacpi_disable_all_gpes();
    }
    else
    {
        /*
         * uacpi_enter_sleep_state is never called since the HAL owns the
         * sleep write, so do its housekeeping here. A stale WAK_STS or GPE
         * status would wake the machine right back up.
         */
        uacpi_write_register_field(UACPI_REGISTER_FIELD_WAK_STS, ACPI_PM1_STS_CLEAR);
        uacpi_disable_all_gpes();
        UacpiNtClearAllEvents();

        /* Devices that cannot wake from this depth stay masked until resume */
        UacpiNtWakeSuspendShallow(SystemState);
        uacpi_enable_all_wake_gpes();

        UacpiNtPendingSleepState = Target;

        /* A button press in the resume window is the wake event */
        InterlockedExchange(&UacpiNtWakeSourceClaimed, 0);
        InterlockedExchange(&UacpiNtSystemResuming, 1);
    }

    UacpiNtHalRefreshWakeCache();

    /* Nothing may be frozen mid AML holding an interpreter or EC lock */
    uacpi_kernel_wait_for_work_completion();
}

NTSTATUS
NTAPI
UacpiNtSystemSetPower(
    _In_ PUACPINT_FDO Fdo,
    _In_ PIRP Irp)
{
    PIO_STACK_LOCATION IoStack = IoGetCurrentIrpStackLocation(Irp);
    SYSTEM_POWER_STATE SystemState = IoStack->Parameters.Power.State.SystemState;

    if (Fdo->InterpreterReady)
    {
        PoSetPowerState(Fdo->Shared.Self, SystemPowerState, IoStack->Parameters.Power.State);

        if (SystemState == PowerSystemWorking)
        {
            if (UacpiNtPendingSleepState != UACPI_SLEEP_STATE_S0)
                UacpiNtResumeFromSleep();

            /* Any wake press was delivered by now */
            InterlockedExchange(&UacpiNtSystemResuming, 0);
        }
        else if (SystemState > PowerSystemWorking)
        {
            UacpiNtPrepareForSleep(SystemState, IoStack->Parameters.Power.ShutdownType);
        }
    }

    PoStartNextPowerIrp(Irp);
    IoSkipCurrentIrpStackLocation(Irp);
    return PoCallDriver(Fdo->LowerDevice, Irp);
}
