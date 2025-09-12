/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/evtpair.c
 * PURPOSE:         Support for event pairs
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Thomas Weidenmueller
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE ExEventPairObjectType = NULL;

GENERIC_MAPPING ExEventPairMapping =
{
    STANDARD_RIGHTS_READ | SYNCHRONIZE,
    STANDARD_RIGHTS_WRITE | SYNCHRONIZE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    EVENT_PAIR_ALL_ACCESS
};

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeEventPairImplementation(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    NTSTATUS Status;
    DPRINT("Creating Event Pair Object Type\n");

    /* Create the Event Pair Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"EventPair");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KEVENT_PAIR);
    ObjectTypeInitializer.GenericMapping = ExEventPairMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = EVENT_PAIR_ALL_ACCESS;
    ObjectTypeInitializer.UseDefaultObject = TRUE;
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    Status = ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &ExEventPairObjectType);
    if (!NT_SUCCESS(Status)) return FALSE;
    return TRUE;
}

NTSTATUS
NTAPI
NtCreateEventPair(OUT PHANDLE EventPairHandle,
                  IN ACCESS_MASK DesiredAccess,
                  IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    PKEVENT_PAIR EventPair;
    HANDLE hEventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT("NtCreateEventPair: 0x%p\n", EventPairHandle);

    /* Check if we were called from user-mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH Block */
        _SEH2_TRY
        {
            /* Check handle pointer */
            ProbeForWriteHandle(EventPairHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Create the Object */
    DPRINT("Creating EventPair\n");
    Status = ObCreateObject(PreviousMode,
                            ExEventPairObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(KEVENT_PAIR),
                            0,
                            0,
                            (PVOID*)&EventPair);

    /* Check for Success */
    if (NT_SUCCESS(Status))
    {
        /* Initialize the Event */
        DPRINT("Initializing EventPair\n");
        KeInitializeEventPair(EventPair);

        /* Insert it */
        Status = ObInsertObject((PVOID)EventPair,
                                 NULL,
                                 DesiredAccess,
                                 0,
                                 NULL,
                                 &hEventPair);

        /* Check for success */
        if (NT_SUCCESS(Status))
        {
            /* Enter SEH */
            _SEH2_TRY
            {
                /* Return the handle */
                *EventPairHandle = hEventPair;
            }
            _SEH2_EXCEPT(ExSystemExceptionFilter())
            {
                /* Get the exception code */
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
    }

    /* Return Status */
    return Status;
}

NTSTATUS
NTAPI
NtOpenEventPair(OUT PHANDLE EventPairHandle,
                IN ACCESS_MASK DesiredAccess,
                IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hEventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if we were called from user-mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH Block */
        _SEH2_TRY
        {
            /* Check handle pointer */
            ProbeForWriteHandle(EventPairHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ExEventPairObjectType,
                                PreviousMode,
                                NULL,
                                DesiredAccess,
                                NULL,
                                &hEventPair);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Enter SEH */
        _SEH2_TRY
        {
            /* Return the handle */
            *EventPairHandle = hEventPair;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtSetHighEventPair(IN HANDLE EventPairHandle)
{
    PKEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT("NtSetHighEventPair(EventPairHandle 0x%p)\n", EventPairHandle);

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID*)&EventPair,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status))
    {
        /* Set the Event */
        KeSetEvent(&EventPair->HighEvent, EVENT_INCREMENT, FALSE);

        /* Dereference Object */
        ObDereferenceObject(EventPair);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtSetHighWaitLowEventPair(IN HANDLE EventPairHandle)
{
    PKEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT("NtSetHighWaitLowEventPair(Handle 0x%p)\n", EventPairHandle);

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID*)&EventPair,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status))
    {
        /* Set the Event */
        KeSetEvent(&EventPair->HighEvent, EVENT_INCREMENT, FALSE);

        /* Wait for the Other one */
        KeWaitForSingleObject(&EventPair->LowEvent,
                              WrEventPair,
                              PreviousMode,
                              FALSE,
                              NULL);

        /* Dereference Object */
        ObDereferenceObject(EventPair);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtSetLowEventPair(IN HANDLE EventPairHandle)
{
    PKEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT1("NtSetHighEventPair(EventPairHandle 0x%p)\n", EventPairHandle);

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID*)&EventPair,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status))
    {
        /* Set the Event */
        KeSetEvent(&EventPair->LowEvent, EVENT_INCREMENT, FALSE);

        /* Dereference Object */
        ObDereferenceObject(EventPair);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtSetLowWaitHighEventPair(IN HANDLE EventPairHandle)
{
    PKEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT("NtSetHighWaitLowEventPair(Handle 0x%p)\n", EventPairHandle);

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID*)&EventPair,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status))
    {
        /* Set the Event */
        KeSetEvent(&EventPair->LowEvent, EVENT_INCREMENT, FALSE);

        /* Wait for the Other one */
        KeWaitForSingleObject(&EventPair->HighEvent,
                              WrEventPair,
                              PreviousMode,
                              FALSE,
                              NULL);

        /* Dereference Object */
        ObDereferenceObject(EventPair);
    }

    /* Return status */
    return Status;
}


NTSTATUS
NTAPI
NtWaitLowEventPair(IN HANDLE EventPairHandle)
{
    PKEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT("NtSetHighWaitLowEventPair(Handle 0x%p)\n", EventPairHandle);

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID*)&EventPair,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status))
    {
        /* Wait for the Event */
        KeWaitForSingleObject(&EventPair->LowEvent,
                              WrEventPair,
                              PreviousMode,
                              FALSE,
                              NULL);

        /* Dereference Object */
        ObDereferenceObject(EventPair);
    }

    /* Return status */
    return Status;
}

NTSTATUS
NTAPI
NtWaitHighEventPair(IN HANDLE EventPairHandle)
{
    PKEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("NtSetHighWaitLowEventPair(Handle 0x%p)\n", EventPairHandle);

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID*)&EventPair,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status))
    {
        /* Wait for the Event */
        KeWaitForSingleObject(&EventPair->HighEvent,
                              WrEventPair,
                              PreviousMode,
                              FALSE,
                              NULL);

        /* Dereference Object */
        ObDereferenceObject(EventPair);
    }

    /* Return status */
    return Status;
}

/* EOF */
