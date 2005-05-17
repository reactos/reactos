/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/evtpair.c
 * PURPOSE:         Support for event pairs
 *
 * PROGRAMMERS:     Alex Ionescu (Commented, reorganized, removed Thread Pair, used
 *                                KeInitializeEventPair, added SEH)
 *                  David Welch (welch@mcmail.com)
 *                  Skywing (skywing@valhallalegends.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED ExEventPairObjectType = NULL;

static GENERIC_MAPPING ExEventPairMapping = {
    STANDARD_RIGHTS_READ,
    STANDARD_RIGHTS_WRITE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    EVENT_PAIR_ALL_ACCESS};


/* FUNCTIONS *****************************************************************/

VOID
INIT_FUNCTION
ExpInitializeEventPairImplementation(VOID)
{
  OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
  UNICODE_STRING Name;

  DPRINT1("Creating Event Pair Object Type\n");
  
  /* Create the Event Pair Object Type */
  RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
  RtlInitUnicodeString(&Name, L"EventPair");
  ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
  ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KEVENT_PAIR);
  ObjectTypeInitializer.GenericMapping = ExEventPairMapping;
  ObjectTypeInitializer.PoolType = NonPagedPool;
  ObjectTypeInitializer.ValidAccessMask = EVENT_PAIR_ALL_ACCESS;
  ObjectTypeInitializer.UseDefaultObject = TRUE;
  ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ExEventPairObjectType);
}

NTSTATUS
STDCALL
NtCreateEventPair(OUT PHANDLE EventPairHandle,
                  IN ACCESS_MASK DesiredAccess,
                  IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    PKEVENT_PAIR EventPair;
    HANDLE hEventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("NtCreateEventPair: %x\n", EventPairHandle);

    /* Check Output Safety */
    if(PreviousMode == UserMode) {

        _SEH_TRY {

            ProbeForWrite(EventPairHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
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
    if(NT_SUCCESS(Status)) {

        /* Initalize the Event */
        DPRINT("Initializing EventPair\n");
        KeInitializeEventPair(EventPair);

        /* Insert it */
        Status = ObInsertObject((PVOID)EventPair,
                                 NULL,
                                 DesiredAccess,
                                 0,
                                 NULL,
                                 &hEventPair);
        ObDereferenceObject(EventPair);

        /* Check for success and return handle */
        if(NT_SUCCESS(Status)) {

            _SEH_TRY {

                *EventPairHandle = hEventPair;

            } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

                Status = _SEH_GetExceptionCode();

            } _SEH_END;
        }
    }

    /* Return Status */
    return Status;
}

NTSTATUS
STDCALL
NtOpenEventPair(OUT PHANDLE EventPairHandle,
                IN ACCESS_MASK DesiredAccess,
                IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hEventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    /* Check Output Safety */
    if(PreviousMode == UserMode) {

        _SEH_TRY {

            ProbeForWrite(EventPairHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ExEventPairObjectType,
                                NULL,
                                PreviousMode,
                                DesiredAccess,
                                NULL,
                                &hEventPair);

    /* Check for success and return handle */
    if(NT_SUCCESS(Status)) {

        _SEH_TRY {

            *EventPairHandle = hEventPair;

        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;
    }

    /* Return status */
    return Status;
}


NTSTATUS
STDCALL
NtSetHighEventPair(IN HANDLE EventPairHandle)
{
    PKEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("NtSetHighEventPair(EventPairHandle %x)\n", EventPairHandle);

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID*)&EventPair,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status)) {

        /* Set the Event */
        KeSetEvent(&EventPair->HighEvent, EVENT_INCREMENT, FALSE);

        /* Dereference Object */
        ObDereferenceObject(EventPair);
    }

    /* Return status */
    return Status;
}

NTSTATUS
STDCALL
NtSetHighWaitLowEventPair(IN HANDLE EventPairHandle)
{
    PKEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("NtSetHighWaitLowEventPair(EventPairHandle %x)\n", EventPairHandle);

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID*)&EventPair,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status)) {

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
STDCALL
NtSetLowEventPair(IN HANDLE EventPairHandle)
{
    PKEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    DPRINT1("NtSetHighEventPair(EventPairHandle %x)\n", EventPairHandle);

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID*)&EventPair,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status)) {

        /* Set the Event */
        KeSetEvent(&EventPair->LowEvent, EVENT_INCREMENT, FALSE);

        /* Dereference Object */
        ObDereferenceObject(EventPair);
    }

    /* Return status */
    return Status;
}


NTSTATUS STDCALL
NtSetLowWaitHighEventPair(IN HANDLE EventPairHandle)
{
    PKEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("NtSetHighWaitLowEventPair(EventPairHandle %x)\n", EventPairHandle);

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID*)&EventPair,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status)) {

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
STDCALL
NtWaitLowEventPair(IN HANDLE EventPairHandle)
{
    PKEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("NtSetHighWaitLowEventPair(EventPairHandle %x)\n", EventPairHandle);

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID*)&EventPair,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status)) {

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
STDCALL
NtWaitHighEventPair(IN HANDLE EventPairHandle)
{
    PKEVENT_PAIR EventPair;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;

    PAGED_CODE();
    DPRINT("NtSetHighWaitLowEventPair(EventPairHandle %x)\n", EventPairHandle);

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventPairHandle,
                                       SYNCHRONIZE,
                                       ExEventPairObjectType,
                                       PreviousMode,
                                       (PVOID*)&EventPair,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status)) {

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
