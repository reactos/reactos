/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/nt/event.c
 * PURPOSE:         Named event support
 *
 * PROGRAMMERS:     Alex Ionescu(alex@relsoft.net) - Fixed bugs/commented
 *                  Philip Susi and David Welch
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED ExEventObjectType = NULL;

static GENERIC_MAPPING ExpEventMapping = {
    STANDARD_RIGHTS_READ | SYNCHRONIZE | EVENT_QUERY_STATE,
    STANDARD_RIGHTS_WRITE | SYNCHRONIZE | EVENT_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | EVENT_QUERY_STATE,
    EVENT_ALL_ACCESS};

static const INFORMATION_CLASS_INFO ExEventInfoClass[] = {

    /* EventBasicInformation */
    ICI_SQ_SAME( sizeof(EVENT_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY),
};

/* FUNCTIONS *****************************************************************/

VOID
INIT_FUNCTION
ExpInitializeEventImplementation(VOID)
{
  OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
  UNICODE_STRING Name;

  DPRINT1("Creating Event Object Type\n");
  
  /* Create the Event Object Type */
  RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
  RtlInitUnicodeString(&Name, L"Event");
  ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
  ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KEVENT);
  ObjectTypeInitializer.GenericMapping = ExpEventMapping;
  ObjectTypeInitializer.PoolType = NonPagedPool;
  ObjectTypeInitializer.ValidAccessMask = EVENT_ALL_ACCESS;
  ObjectTypeInitializer.UseDefaultObject = TRUE;
  ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ExEventObjectType);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtClearEvent(IN HANDLE EventHandle)
{
    PKEVENT Event;
    NTSTATUS Status;

    PAGED_CODE();

    /* Reference the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
                                       EVENT_MODIFY_STATE,
                                       ExEventObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&Event,
                                       NULL);

    /* Check for Success */
    if(NT_SUCCESS(Status)) {

        /* Clear the Event and Dereference */
        KeClearEvent(Event);
        ObDereferenceObject(Event);
    }

    /* Return Status */
    return Status;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
NtCreateEvent(OUT PHANDLE EventHandle,
              IN ACCESS_MASK DesiredAccess,
              IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
              IN EVENT_TYPE EventType,
              IN BOOLEAN InitialState)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PKEVENT Event;
    HANDLE hEvent;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("NtCreateEvent(0x%x, 0x%x, 0x%x)\n", EventHandle, DesiredAccess, ObjectAttributes);

    /* Check Output Safety */
    if(PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWrite(EventHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Create the Object */
    Status = ObCreateObject(PreviousMode,
                            ExEventObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(KEVENT),
                            0,
                            0,
                            (PVOID*)&Event);

    /* Check for Success */
    if(NT_SUCCESS(Status)) {

        /* Initalize the Event */
        KeInitializeEvent(Event,
                          EventType,
                          InitialState);

        /* Insert it */
        Status = ObInsertObject((PVOID)Event,
                                 NULL,
                                 DesiredAccess,
                                 0,
                                 NULL,
                                 &hEvent);
        ObDereferenceObject(Event);

        /* Check for success and return handle */
        if(NT_SUCCESS(Status)) {

            _SEH_TRY {

                *EventHandle = hEvent;

            } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

                Status = _SEH_GetExceptionCode();

            } _SEH_END;
        }
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtOpenEvent(OUT PHANDLE EventHandle,
            IN ACCESS_MASK DesiredAccess,
            IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hEvent;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("NtOpenEvent(0x%x, 0x%x, 0x%x)\n", EventHandle, DesiredAccess, ObjectAttributes);

    /* Check Output Safety */
    if(PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWrite(EventHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ExEventObjectType,
                                NULL,
                                PreviousMode,
                                DesiredAccess,
                                NULL,
                                &hEvent);

    /* Check for success and return handle */
    if(NT_SUCCESS(Status)) {

        _SEH_TRY {

            *EventHandle = hEvent;

        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;
    }

    /* Return status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtPulseEvent(IN HANDLE EventHandle,
             OUT PLONG PreviousState OPTIONAL)
{
    PKEVENT Event;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("NtPulseEvent(EventHandle 0%x PreviousState 0%x)\n",
            EventHandle, PreviousState);

    /* Check buffer validity */
    if(PreviousState && PreviousMode == UserMode) {

        _SEH_TRY {

            ProbeForWrite(PreviousState,
                          sizeof(LONG),
                          sizeof(ULONG));
         } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
                                       EVENT_MODIFY_STATE,
                                       ExEventObjectType,
                                       PreviousMode,
                                       (PVOID*)&Event,
                                       NULL);

    /* Check for success */
    if(NT_SUCCESS(Status)) {

        /* Pulse the Event */
        LONG Prev = KePulseEvent(Event, EVENT_INCREMENT, FALSE);
        ObDereferenceObject(Event);

        /* Return it */
        if(PreviousState) {

            _SEH_TRY {

                *PreviousState = Prev;

            } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

                Status = _SEH_GetExceptionCode();

            } _SEH_END;
        }
   }

   /* Return Status */
   return Status;
}


/*
 * @implemented
 */
NTSTATUS
STDCALL
NtQueryEvent(IN HANDLE EventHandle,
             IN EVENT_INFORMATION_CLASS EventInformationClass,
             OUT PVOID EventInformation,
             IN ULONG EventInformationLength,
             OUT PULONG ReturnLength  OPTIONAL)
{
    PKEVENT Event;
    KPROCESSOR_MODE PreviousMode  = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PEVENT_BASIC_INFORMATION BasicInfo = (PEVENT_BASIC_INFORMATION)EventInformation;

    PAGED_CODE();
    DPRINT("NtQueryEvent(0x%x, 0x%x, 0x%x)\n", EventHandle, EventInformationClass);

    /* Check buffers and class validity */
    DefaultQueryInfoBufferCheck(EventInformationClass,
                                ExEventInfoClass,
                                EventInformation,
                                EventInformationLength,
                                ReturnLength,
                                PreviousMode,
                                &Status);
    if(!NT_SUCCESS(Status)) {

        /* Invalid buffers */
        DPRINT("NtQuerySemaphore() failed, Status: 0x%x\n", Status);
        return Status;
    }

    /* Get the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
                                       EVENT_QUERY_STATE,
                                       ExEventObjectType,
                                       PreviousMode,
                                       (PVOID*)&Event,
                                       NULL);

    /* Check for success */
    if(NT_SUCCESS(Status)) {

        _SEH_TRY {

            /* Return Event Type and State */
            BasicInfo->EventType = Event->Header.Type;
            BasicInfo->EventState = KeReadStateEvent(Event);

            /* Return length */
            if(ReturnLength) *ReturnLength = sizeof(EVENT_BASIC_INFORMATION);

        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        /* Dereference the Object */
        ObDereferenceObject(Event);
   }

   /* Return status */
   return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtResetEvent(IN HANDLE EventHandle,
             OUT PLONG PreviousState OPTIONAL)
{
    PKEVENT Event;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("NtResetEvent(EventHandle 0%x PreviousState 0%x)\n",
            EventHandle, PreviousState);

    /* Check buffer validity */
    if(PreviousState && PreviousMode == UserMode) {

        _SEH_TRY {

            ProbeForWrite(PreviousState,
                          sizeof(LONG),
                          sizeof(ULONG));
         } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
                                       EVENT_MODIFY_STATE,
                                       ExEventObjectType,
                                       PreviousMode,
                                       (PVOID*)&Event,
                                       NULL);

    /* Check for success */
    if(NT_SUCCESS(Status)) {

        /* Reset the Event */
        LONG Prev = KeResetEvent(Event);
        ObDereferenceObject(Event);

        /* Return it */
        if(PreviousState) {

            _SEH_TRY {

                *PreviousState = Prev;

            } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

                Status = _SEH_GetExceptionCode();

            } _SEH_END;
        }
   }

   /* Return Status */
   return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtSetEvent(IN HANDLE EventHandle,
           OUT PLONG PreviousState  OPTIONAL)
{
    PKEVENT Event;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("NtSetEvent(EventHandle 0%x PreviousState 0%x)\n",
           EventHandle, PreviousState);

    /* Check buffer validity */
    if(PreviousState != NULL && PreviousMode == UserMode) {

        _SEH_TRY {

            ProbeForWrite(PreviousState,
                          sizeof(LONG),
                          sizeof(ULONG));
         } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
                                       EVENT_MODIFY_STATE,
                                       ExEventObjectType,
                                       PreviousMode,
                                       (PVOID*)&Event,
                                       NULL);

    /* Check for success */
    if(NT_SUCCESS(Status)) {

        /* Set the Event */
        LONG Prev = KeSetEvent(Event, EVENT_INCREMENT, FALSE);
        ObDereferenceObject(Event);

        /* Return it */
        if(PreviousState) {

            _SEH_TRY {

                *PreviousState = Prev;

            } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

                Status = _SEH_GetExceptionCode();

            } _SEH_END;
        }
   }

   /* Return Status */
   return Status;
}

/* EOF */
