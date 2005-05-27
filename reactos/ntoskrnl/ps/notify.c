/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ps/notify.c
 * PURPOSE:         Notifications
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBAL *******************************************************************/

#define MAX_THREAD_NOTIFY_ROUTINE_COUNT    8

static ULONG PspThreadNotifyRoutineCount = 0;
static PCREATE_THREAD_NOTIFY_ROUTINE
PspThreadNotifyRoutine[MAX_THREAD_NOTIFY_ROUTINE_COUNT];

static PCREATE_PROCESS_NOTIFY_ROUTINE
PspProcessNotifyRoutine[MAX_PROCESS_NOTIFY_ROUTINE_COUNT];

static PLOAD_IMAGE_NOTIFY_ROUTINE
PspLoadImageNotifyRoutine[MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT];

static PVOID PspLegoNotifyRoutine;

/* FUNCTIONS ***************************************************************/

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsSetCreateProcessNotifyRoutine(IN PCREATE_PROCESS_NOTIFY_ROUTINE NotifyRoutine,
                                IN BOOLEAN Remove)
{
    ULONG i;

    /* Check if it's a removal or addition */
    if (Remove)
    {
        /* Loop the routines */
        for(i=0;i<MAX_PROCESS_NOTIFY_ROUTINE_COUNT;i++)
        {
            /* Check for a match */
            if ((PVOID)PspProcessNotifyRoutine[i] == (PVOID)NotifyRoutine)
            {
                /* Remove and return */
                PspProcessNotifyRoutine[i] = NULL;
                return(STATUS_SUCCESS);
            }
        }
    }
    else
    {
        /* Loop the routines */
        for(i=0;i<MAX_PROCESS_NOTIFY_ROUTINE_COUNT;i++)
        {
            /* Find an empty one */
            if (PspProcessNotifyRoutine[i] == NULL)
            {
                /* Add it */
                PspProcessNotifyRoutine[i] = NotifyRoutine;
                return STATUS_SUCCESS;
            }
        }
    }

    /* Nothing found */
    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 */
ULONG
STDCALL
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
STDCALL
PsRemoveLoadImageNotifyRoutine(IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine)
{
    ULONG i;

    /* Loop the routines */
    for(i=0;i<MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT;i++)
    {
        /* Check for a match */
        if ((PVOID)PspLoadImageNotifyRoutine[i] == (PVOID)NotifyRoutine)
        {
            /* Remove and return */
            PspLoadImageNotifyRoutine[i] = NULL;
            return(STATUS_SUCCESS);
        }
    }

    /* Nothing found */
    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsSetLoadImageNotifyRoutine(IN PLOAD_IMAGE_NOTIFY_ROUTINE NotifyRoutine)
{
    ULONG i;

    /* Loop the routines */
    for (i = 0; i < MAX_LOAD_IMAGE_NOTIFY_ROUTINE_COUNT; i++)
    {
        /* Find an empty one */
        if (PspLoadImageNotifyRoutine[i] == NULL)
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
STDCALL
PsRemoveCreateThreadNotifyRoutine(IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine)
{
    ULONG i;

    /* Loop the routines */
    for(i=0;i<MAX_THREAD_NOTIFY_ROUTINE_COUNT;i++)
    {
        /* Check for a match */
        if ((PVOID)PspThreadNotifyRoutine[i] == (PVOID)NotifyRoutine)
        {
            /* Remove and return */
            PspThreadNotifyRoutine[i] = NULL;
            return(STATUS_SUCCESS);
        }
    }

    /* Nothing found */
    return STATUS_INVALID_PARAMETER;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
PsSetCreateThreadNotifyRoutine(IN PCREATE_THREAD_NOTIFY_ROUTINE NotifyRoutine)
{
    if (PspThreadNotifyRoutineCount >= MAX_THREAD_NOTIFY_ROUTINE_COUNT)
    {
        return(STATUS_INSUFFICIENT_RESOURCES);
    }

    PspThreadNotifyRoutine[PspThreadNotifyRoutineCount] = NotifyRoutine;
    PspThreadNotifyRoutineCount++;

    return(STATUS_SUCCESS);
}

VOID
STDCALL
PspRunCreateThreadNotifyRoutines(PETHREAD CurrentThread,
                                 BOOLEAN Create)
{
    ULONG i;
    CLIENT_ID Cid = CurrentThread->Cid;

    for (i = 0; i < PspThreadNotifyRoutineCount; i++)
    {
        PspThreadNotifyRoutine[i](Cid.UniqueProcess, Cid.UniqueThread, Create);
    }
}

VOID
STDCALL
PspRunCreateProcessNotifyRoutines(PEPROCESS CurrentProcess,
                                  BOOLEAN Create)
{
    ULONG i;
    HANDLE ProcessId = (HANDLE)CurrentProcess->UniqueProcessId;
    HANDLE ParentId = CurrentProcess->InheritedFromUniqueProcessId;

    for(i = 0; i < MAX_PROCESS_NOTIFY_ROUTINE_COUNT; ++i)
    {
        if(PspProcessNotifyRoutine[i])
        {
            PspProcessNotifyRoutine[i](ParentId, ProcessId, Create);
        }
    }
}

VOID
STDCALL
PspRunLoadImageNotifyRoutines(PUNICODE_STRING FullImageName,
                              HANDLE ProcessId,
                              PIMAGE_INFO ImageInfo)
{
    ULONG i;

    for (i = 0; i < MAX_PROCESS_NOTIFY_ROUTINE_COUNT; ++ i)
    {
        if (PspLoadImageNotifyRoutine[i])
        {
            PspLoadImageNotifyRoutine[i](FullImageName, ProcessId, ImageInfo);
        }
    }
}

/* EOF */
