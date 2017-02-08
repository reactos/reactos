/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/shutdown.c
 * PURPOSE:         Power managment
 *
 * PROGRAMMERS:     David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtShutdownSystem(IN SHUTDOWN_ACTION Action)
{
    POWER_ACTION PowerAction;

    /* Convert to power action */
    if (Action == ShutdownNoReboot)
    {
        PowerAction = PowerActionShutdown;
    }
    else if (Action == ShutdownReboot)
    {
        PowerAction = PowerActionShutdownReset;
    }
    else if (Action == ShutdownPowerOff)
    {
        PowerAction = PowerActionShutdownOff;
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Now call the power manager */
    DPRINT("Setting state to: %lx\n", PowerAction);
    return NtSetSystemPowerState(PowerAction,
                                 PowerSystemSleeping3,
                                 POWER_ACTION_OVERRIDE_APPS |
                                 POWER_ACTION_DISABLE_WAKES |
                                 POWER_ACTION_CRITICAL);
}

/* EOF */
