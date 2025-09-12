/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/event.c
 * PURPOSE:         Event support
 * PROGRAMMERS:     Alex Ionescu(alex@relsoft.net)
 *                  Thomas Weidenmueller
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE ExEventObjectType = NULL;

GENERIC_MAPPING ExpEventMapping =
{
    STANDARD_RIGHTS_READ | EVENT_QUERY_STATE,
    STANDARD_RIGHTS_WRITE | EVENT_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    EVENT_ALL_ACCESS
};

static const INFORMATION_CLASS_INFO ExEventInfoClass[] =
{
    /* EventBasicInformation */
    IQS_SAME(EVENT_BASIC_INFORMATION, ULONG, ICIF_QUERY),
};

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeEventImplementation(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    NTSTATUS Status;
    DPRINT("Creating Event Object Type\n");

    /* Create the Event Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Event");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KEVENT);
    ObjectTypeInitializer.GenericMapping = ExpEventMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.ValidAccessMask = EVENT_ALL_ACCESS;
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    Status = ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &ExEventObjectType);
    if (!NT_SUCCESS(Status)) return FALSE;
    return TRUE;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
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
    if(NT_SUCCESS(Status))
    {
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
NTAPI
NtCreateEvent(OUT PHANDLE EventHandle,
              IN ACCESS_MASK DesiredAccess,
              IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
              IN EVENT_TYPE EventType,
              IN BOOLEAN InitialState)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PKEVENT Event;
    HANDLE hEvent;
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT("NtCreateEvent(0x%p, 0x%x, 0x%p)\n",
            EventHandle, DesiredAccess, ObjectAttributes);

    /* Check if we were called from user-mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH Block */
        _SEH2_TRY
        {
            /* Check handle pointer */
            ProbeForWriteHandle(EventHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
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
    if (NT_SUCCESS(Status))
    {
        /* Initialize the Event */
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

        /* Check for success */
        if (NT_SUCCESS(Status))
        {
            /* Enter SEH for return */
            _SEH2_TRY
            {
                /* Return the handle to the caller */
                *EventHandle = hEvent;
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

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtOpenEvent(OUT PHANDLE EventHandle,
            IN ACCESS_MASK DesiredAccess,
            IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hEvent;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT("NtOpenEvent(0x%p, 0x%x, 0x%p)\n",
            EventHandle, DesiredAccess, ObjectAttributes);

    /* Check if we were called from user-mode */
    if (PreviousMode != KernelMode)
    {
        /* Enter SEH Block */
        _SEH2_TRY
        {
            /* Check handle pointer */
            ProbeForWriteHandle(EventHandle);
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
                                ExEventObjectType,
                                PreviousMode,
                                NULL,
                                DesiredAccess,
                                NULL,
                                &hEvent);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Enter SEH for return */
        _SEH2_TRY
        {
            /* Return the handle to the caller */
            *EventHandle = hEvent;
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

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtPulseEvent(IN HANDLE EventHandle,
             OUT PLONG PreviousState OPTIONAL)
{
    PKEVENT Event;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT("NtPulseEvent(EventHandle 0%p PreviousState 0%p)\n",
            EventHandle, PreviousState);

    /* Check if we were called from user-mode */
    if((PreviousState) && (PreviousMode != KernelMode))
    {
        /* Entry SEH Block */
        _SEH2_TRY
        {
            /* Make sure the state pointer is valid */
            ProbeForWriteLong(PreviousState);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
                                       EVENT_MODIFY_STATE,
                                       ExEventObjectType,
                                       PreviousMode,
                                       (PVOID*)&Event,
                                       NULL);

    /* Check for success */
    if(NT_SUCCESS(Status))
    {
        /* Pulse the Event */
        LONG Prev = KePulseEvent(Event, EVENT_INCREMENT, FALSE);
        ObDereferenceObject(Event);

        /* Check if caller wants the old state back */
        if(PreviousState)
        {
            /* Entry SEH Block for return */
            _SEH2_TRY
            {
                /* Return previous state */
                *PreviousState = Prev;
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

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQueryEvent(IN HANDLE EventHandle,
             IN EVENT_INFORMATION_CLASS EventInformationClass,
             OUT PVOID EventInformation,
             IN ULONG EventInformationLength,
             OUT PULONG ReturnLength  OPTIONAL)
{
    PKEVENT Event;
    KPROCESSOR_MODE PreviousMode  = ExGetPreviousMode();
    NTSTATUS Status;
    PEVENT_BASIC_INFORMATION BasicInfo =
        (PEVENT_BASIC_INFORMATION)EventInformation;
    PAGED_CODE();
    DPRINT("NtQueryEvent(0x%p, 0x%x)\n", EventHandle, EventInformationClass);

    /* Check buffers and class validity */
    Status = DefaultQueryInfoBufferCheck(EventInformationClass,
                                         ExEventInfoClass,
                                         sizeof(ExEventInfoClass) /
                                         sizeof(ExEventInfoClass[0]),
                                         ICIF_PROBE_READ_WRITE,
                                         EventInformation,
                                         EventInformationLength,
                                         ReturnLength,
                                         NULL,
                                         PreviousMode);
    if(!NT_SUCCESS(Status))
    {
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
    if(NT_SUCCESS(Status))
    {
        /* Entry SEH Block */
        _SEH2_TRY
        {
            /* Return Event Type and State */
            BasicInfo->EventType = Event->Header.Type;
            BasicInfo->EventState = KeReadStateEvent(Event);

            /* Return length */
            if(ReturnLength) *ReturnLength = sizeof(EVENT_BASIC_INFORMATION);
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

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
NTAPI
NtResetEvent(IN HANDLE EventHandle,
             OUT PLONG PreviousState OPTIONAL)
{
    PKEVENT Event;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT("NtResetEvent(EventHandle 0%p PreviousState 0%p)\n",
            EventHandle, PreviousState);

    /* Check if we were called from user-mode */
    if ((PreviousState) && (PreviousMode != KernelMode))
    {
        /* Entry SEH Block */
        _SEH2_TRY
        {
            /* Make sure the state pointer is valid */
            ProbeForWriteLong(PreviousState);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
                                       EVENT_MODIFY_STATE,
                                       ExEventObjectType,
                                       PreviousMode,
                                       (PVOID*)&Event,
                                       NULL);

    /* Check for success */
    if(NT_SUCCESS(Status))
    {
        /* Reset the Event */
        LONG Prev = KeResetEvent(Event);
        ObDereferenceObject(Event);

        /* Check if caller wants the old state back */
        if(PreviousState)
        {
            /* Entry SEH Block for return */
            _SEH2_TRY
            {
                /* Return previous state */
                *PreviousState = Prev;
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

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetEvent(IN HANDLE EventHandle,
           OUT PLONG PreviousState  OPTIONAL)
{
    PKEVENT Event;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();
    DPRINT("NtSetEvent(EventHandle 0%p PreviousState 0%p)\n",
           EventHandle, PreviousState);

    /* Check if we were called from user-mode */
    if ((PreviousState) && (PreviousMode != KernelMode))
    {
        /* Entry SEH Block */
        _SEH2_TRY
        {
            /* Make sure the state pointer is valid */
            ProbeForWriteLong(PreviousState);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
                                       EVENT_MODIFY_STATE,
                                       ExEventObjectType,
                                       PreviousMode,
                                       (PVOID*)&Event,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Set the Event */
        LONG Prev = KeSetEvent(Event, EVENT_INCREMENT, FALSE);
        ObDereferenceObject(Event);

        /* Check if caller wants the old state back */
        if (PreviousState)
        {
            /* Entry SEH Block for return */
            _SEH2_TRY
            {
                /* Return previous state */
                *PreviousState = Prev;
            }
            _SEH2_EXCEPT(ExSystemExceptionFilter())
            {
                Status = _SEH2_GetExceptionCode();
            }
            _SEH2_END;
        }
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetEventBoostPriority(IN HANDLE EventHandle)
{
    PKEVENT Event;
    NTSTATUS Status;
    PAGED_CODE();

    /* Open the Object */
    Status = ObReferenceObjectByHandle(EventHandle,
                                       EVENT_MODIFY_STATE,
                                       ExEventObjectType,
                                       ExGetPreviousMode(),
                                       (PVOID*)&Event,
                                       NULL);
    if (NT_SUCCESS(Status))
    {
        /* Set the Event */
        KeSetEventBoostPriority(Event, NULL);
        ObDereferenceObject(Event);
    }

    /* Return Status */
    return Status;
}

/* EOF */
