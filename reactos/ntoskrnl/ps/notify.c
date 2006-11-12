/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ps/notify.c
 * PURPOSE:         Process Manager: Callbacks to Registered Clients (Drivers)
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Thomas Weidenmueller (w3seek@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

BOOLEAN PsImageNotifyEnabled = TRUE;
ULONG PspThreadNotifyRoutineCount;
PCREATE_THREAD_NOTIFY_ROUTINE
PspThreadNotifyRoutine[PSP_MAX_CREATE_THREAD_NOTIFY];
PCREATE_PROCESS_NOTIFY_ROUTINE
PspProcessNotifyRoutine[PSP_MAX_CREATE_PROCESS_NOTIFY];
PLOAD_IMAGE_NOTIFY_ROUTINE
PspLoadImageNotifyRoutine[PSP_MAX_LOAD_IMAGE_NOTIFY];
PLEGO_NOTIFY_ROUTINE PspLegoNotifyRoutine;

/* PUBLIC FUNCTIONS **********************************************************/

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsSetCreateProcessNotifyRoutine(IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
                                IN BOOLEAN Remove)
{
    ULONG i;

    /* Check if it's a removal or addition */
    if (Remove)
    {
        /* Loop the routines */
        for(i = 0; i < PSP_MAX_CREATE_PROCESS_NOTIFY; i++)
        {
            /* Check for a match */
            if (PspProcessNotifyRoutine[i] == NotifyRoutine)
            {
                /* Remove and return */
                PspProcessNotifyRoutine[i] = NULL;
                return STATUS_SUCCESS;
            }
        }
    }
    else
    {
        /* Loop the routines */
        for(i = 0; i < PSP_MAX_CREATE_PROCESS_NOTIFY; i++)
        {
            /* Find an empty one */
            if (!PspProcessNotifyRoutine[i])
            {
                /* Add it */
                PspProcessNotifyRoutine[i] = NotifyRoutine;
                return STATUS_SUCCESS;
            }
        }
    }

    /* Nothing found */
    return Remove ? STATUS_PROCEDURE_NOT_FOUND : STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 */
ULONG
NTAPI
PsSetLegoNotifyRoutine(PVOID LegoNotifyRoutine)
{
    /* Set the System-Wide Lego Routine */
    PspLegoNotifyRoutine = LegoNotifyRoutine;

    /* Return the location to the Lego Data */
    return FIELD_OFFSET(KTHREAD, LegoData);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsRemoveLoadImageNotifyRoutine(IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine)
{
    ULONG i;

    /* Loop the routines */
    for(i = 0; i < PSP_MAX_LOAD_IMAGE_NOTIFY; i++)
    {
        /* Check for a match */
        if (PspLoadImageNotifyRoutine[i] == NotifyRoutine)
        {
            /* Remove and return */
            PspLoadImageNotifyRoutine[i] = NULL;
            return STATUS_SUCCESS;
        }
    }

    /* Nothing found */
    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsSetLoadImageNotifyRoutine(IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine)
{
    ULONG i;

    /* Loop the routines */
    for (i = 0; i < PSP_MAX_LOAD_IMAGE_NOTIFY; i++)
    {
        /* Find an empty one */
        if (!PspLoadImageNotifyRoutine[i])
        {
            /* Add it */
            PspLoadImageNotifyRoutine[i] = NotifyRoutine;
            return STATUS_SUCCESS;
        }
    }

    /* Nothing found */
    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsRemoveCreateThreadNotifyRoutine(IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine)
{
    ULONG i;

    /* Loop the routines */
    for(i = 0; i < PSP_MAX_CREATE_THREAD_NOTIFY; i++)
    {
        /* Check for a match */
        if (PspThreadNotifyRoutine[i] == NotifyRoutine)
        {
            /* Remove and return */
            PspThreadNotifyRoutine[i] = NULL;
            return STATUS_SUCCESS;
        }
    }

    /* Nothing found */
    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
PsSetCreateThreadNotifyRoutine(IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine)
{
    /* Make sure we didn't register too many */
    if (PspThreadNotifyRoutineCount >= PSP_MAX_CREATE_THREAD_NOTIFY)
    {
        /* Fail */
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Register this one */
    PspThreadNotifyRoutine[PspThreadNotifyRoutineCount++] = NotifyRoutine;
    return STATUS_SUCCESS;
}

/* EOF */
