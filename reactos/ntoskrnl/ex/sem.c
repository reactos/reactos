/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/sem.c
 * PURPOSE:         Semaphore Implementation
 *
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  David Welch (welch@mcmail.com)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* GLOBALS ******************************************************************/

POBJECT_TYPE ExSemaphoreObjectType;

static GENERIC_MAPPING ExSemaphoreMapping =
{
    STANDARD_RIGHTS_READ    | SEMAPHORE_QUERY_STATE,
    STANDARD_RIGHTS_WRITE   | SEMAPHORE_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | SEMAPHORE_QUERY_STATE,
    SEMAPHORE_ALL_ACCESS
};

static const INFORMATION_CLASS_INFO ExSemaphoreInfoClass[] =
{
     /* SemaphoreBasicInformation */
    ICI_SQ_SAME( sizeof(SEMAPHORE_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ),
};

VOID
INIT_FUNCTION
STDCALL
ExpInitializeSemaphoreImplementation(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;

    DPRINT("Creating Semaphore Object Type\n");
  
    /* Create the Event Pair Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Semaphore");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KSEMAPHORE);
    ObjectTypeInitializer.GenericMapping = ExSemaphoreMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.ValidAccessMask = SEMAPHORE_ALL_ACCESS;
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ExSemaphoreObjectType);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtCreateSemaphore(OUT PHANDLE SemaphoreHandle,
                  IN ACCESS_MASK DesiredAccess,
                  IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
                  IN LONG InitialCount,
                  IN LONG MaximumCount)
{
    PKSEMAPHORE Semaphore;
    HANDLE hSemaphore;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check Output Safety */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY 
        {
            ProbeForWrite(SemaphoreHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Make sure the counts make sense */
    if (MaximumCount <= 0 || InitialCount < 0 || InitialCount > MaximumCount)
    {
        DPRINT("Invalid Count Data!\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Create the Semaphore Object */
    Status = ObCreateObject(PreviousMode,
                            ExSemaphoreObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(KSEMAPHORE),
                            0,
                            0,
                            (PVOID*)&Semaphore);

    /* Check for Success */
    if (NT_SUCCESS(Status))
    {
        /* Initialize it */
        KeInitializeSemaphore(Semaphore,
                              InitialCount,
                              MaximumCount);

        /* Insert it into the Object Tree */
        Status = ObInsertObject((PVOID)Semaphore,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hSemaphore);
        ObDereferenceObject(Semaphore);

        /* Check for success and return handle */
        if(NT_SUCCESS(Status))
        {
            _SEH_TRY
            {
                *SemaphoreHandle = hSemaphore;
            }
            _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
            {
                Status = _SEH_GetExceptionCode();
            }
            _SEH_END;
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
NtOpenSemaphore(OUT PHANDLE SemaphoreHandle,
                IN ACCESS_MASK DesiredAccess,
                IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hSemaphore;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();

    /* Check Output Safety */
    if(PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(SemaphoreHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ExSemaphoreObjectType,
                                NULL,
                                PreviousMode,
                                DesiredAccess,
                                NULL,
                                &hSemaphore);

    /* Check for success and return handle */
    if(NT_SUCCESS(Status))
    {
        _SEH_TRY
        {
            *SemaphoreHandle = hSemaphore;
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        } _SEH_END;
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtQuerySemaphore(IN HANDLE SemaphoreHandle,
                 IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
                 OUT PVOID SemaphoreInformation,
                 IN ULONG SemaphoreInformationLength,
                 OUT PULONG ReturnLength  OPTIONAL)
{
    PKSEMAPHORE Semaphore;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    /* Check buffers and class validity */
    DefaultQueryInfoBufferCheck(SemaphoreInformationClass,
                                ExSemaphoreInfoClass,
                                SemaphoreInformation,
                                SemaphoreInformationLength,
                                ReturnLength,
                                PreviousMode,
                                &Status);
    if(!NT_SUCCESS(Status))
    {
        /* Invalid buffers */
        DPRINT("NtQuerySemaphore() failed, Status: 0x%x\n", Status);
        return Status;
    }

    /* Get the Object */
    Status = ObReferenceObjectByHandle(SemaphoreHandle,
                                       SEMAPHORE_QUERY_STATE,
                                       ExSemaphoreObjectType,
                                       PreviousMode,
                                       (PVOID*)&Semaphore,
                                       NULL);

    /* Check for success */
    if(NT_SUCCESS(Status))
    {
        _SEH_TRY
        {
            PSEMAPHORE_BASIC_INFORMATION BasicInfo = (PSEMAPHORE_BASIC_INFORMATION)SemaphoreInformation;

            /* Return the basic information */
            BasicInfo->CurrentCount = KeReadStateSemaphore(Semaphore);
            BasicInfo->MaximumCount = Semaphore->Limit;

            /* Return length */
            if(ReturnLength) *ReturnLength = sizeof(SEMAPHORE_BASIC_INFORMATION);

        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        } _SEH_END;

        /* Dereference the Object */
        ObDereferenceObject(Semaphore);
   }

   /* Return status */
   return Status;
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
NtReleaseSemaphore(IN HANDLE SemaphoreHandle,
                   IN LONG ReleaseCount,
                   OUT PLONG PreviousCount  OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PKSEMAPHORE Semaphore;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    /* Check buffer validity */
    if(PreviousCount && PreviousMode != KernelMode)
    {
        _SEH_TRY
        {
            ProbeForWrite(PreviousCount,
                          sizeof(LONG),
                          sizeof(ULONG));
         }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Make sure count makes sense */
    if (ReleaseCount <= 0)
    {
        DPRINT("Invalid Release Count\n");
        return STATUS_INVALID_PARAMETER;
    }

    /* Get the Object */
    Status = ObReferenceObjectByHandle(SemaphoreHandle,
                                       SEMAPHORE_MODIFY_STATE,
                                       ExSemaphoreObjectType,
                                       PreviousMode,
                                       (PVOID*)&Semaphore,
                                       NULL);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Release the semaphore */
        _SEH_TRY
        {
            LONG PrevCount = KeReleaseSemaphore(Semaphore,
                                                IO_NO_INCREMENT,
                                                ReleaseCount,
                                                FALSE);
            ObDereferenceObject(Semaphore);

            /* Return the old count if requested */
            if(PreviousCount)
            {
                *PreviousCount = PrevCount;
            }
        }
        _SEH_EXCEPT(_SEH_ExSystemExceptionFilter)
        {
            Status = _SEH_GetExceptionCode();
        }
        _SEH_END;
    }

    /* Return Status */
    return Status;
}

/* EOF */
