/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/sem.c
 * PURPOSE:         Semaphore Implementation
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Thomas Weidenmueller
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

POBJECT_TYPE ExSemaphoreObjectType;

GENERIC_MAPPING ExSemaphoreMapping =
{
    STANDARD_RIGHTS_READ | SEMAPHORE_QUERY_STATE,
    STANDARD_RIGHTS_WRITE | SEMAPHORE_MODIFY_STATE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE,
    SEMAPHORE_ALL_ACCESS
};

static const INFORMATION_CLASS_INFO ExSemaphoreInfoClass[] =
{
     /* SemaphoreBasicInformation */
    IQS_SAME(SEMAPHORE_BASIC_INFORMATION, ULONG, ICIF_QUERY),
};

/* FUNCTIONS *****************************************************************/

CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpInitializeSemaphoreImplementation(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    NTSTATUS Status;
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
    Status = ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &ExSemaphoreObjectType);
    if (!NT_SUCCESS(Status)) return FALSE;
    return TRUE;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCreateSemaphore(OUT PHANDLE SemaphoreHandle,
                  IN ACCESS_MASK DesiredAccess,
                  IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,
                  IN LONG InitialCount,
                  IN LONG MaximumCount)
{
    PKSEMAPHORE Semaphore;
    HANDLE hSemaphore;
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
            ProbeForWriteHandle(SemaphoreHandle);
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Make sure the counts make sense */
    if ((MaximumCount <= 0) ||
        (InitialCount < 0) ||
        (InitialCount > MaximumCount))
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

        /* Check for success */
        if (NT_SUCCESS(Status))
        {
            /* Enter SEH Block for return */
            _SEH2_TRY
            {
                /* Return the handle */
                *SemaphoreHandle = hSemaphore;
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
NtOpenSemaphore(OUT PHANDLE SemaphoreHandle,
                IN ACCESS_MASK DesiredAccess,
                IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hSemaphore;
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
            ProbeForWriteHandle(SemaphoreHandle);
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
                                ExSemaphoreObjectType,
                                PreviousMode,
                                NULL,
                                DesiredAccess,
                                NULL,
                                &hSemaphore);

    /* Check for success */
    if (NT_SUCCESS(Status))
    {
        /* Enter SEH Block for return */
        _SEH2_TRY
        {
            /* Return the handle */
            *SemaphoreHandle = hSemaphore;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtQuerySemaphore(IN HANDLE SemaphoreHandle,
                 IN SEMAPHORE_INFORMATION_CLASS SemaphoreInformationClass,
                 OUT PVOID SemaphoreInformation,
                 IN ULONG SemaphoreInformationLength,
                 OUT PULONG ReturnLength OPTIONAL)
{
    PKSEMAPHORE Semaphore;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;
    PAGED_CODE();

    /* Check buffers and class validity */
    Status = DefaultQueryInfoBufferCheck(SemaphoreInformationClass,
                                         ExSemaphoreInfoClass,
                                         sizeof(ExSemaphoreInfoClass) /
                                         sizeof(ExSemaphoreInfoClass[0]),
                                         SemaphoreInformation,
                                         SemaphoreInformationLength,
                                         ReturnLength,
                                         NULL,
                                         PreviousMode);
    if (!NT_SUCCESS(Status))
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
    if (NT_SUCCESS(Status))
    {
        /* Entry SEH Block */
        _SEH2_TRY
        {
            PSEMAPHORE_BASIC_INFORMATION BasicInfo =
                (PSEMAPHORE_BASIC_INFORMATION)SemaphoreInformation;

            /* Return the basic information */
            BasicInfo->CurrentCount = KeReadStateSemaphore(Semaphore);
            BasicInfo->MaximumCount = Semaphore->Limit;

            /* Return the length */
            if (ReturnLength) *ReturnLength = sizeof(*BasicInfo);
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

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
NTAPI
NtReleaseSemaphore(IN HANDLE SemaphoreHandle,
                   IN LONG ReleaseCount,
                   OUT PLONG PreviousCount OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PKSEMAPHORE Semaphore;
    NTSTATUS Status;
    PAGED_CODE();

    /* Check if we were called from user-mode */
    if ((PreviousCount) && (PreviousMode != KernelMode))
    {
        /* Entry SEH Block */
        _SEH2_TRY
        {
            /* Make sure the state pointer is valid */
            ProbeForWriteLong(PreviousCount);
         }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Return the exception code */
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
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
        /* Enter SEH Block */
        _SEH2_TRY
        {
            /* Release the semaphore */
            LONG PrevCount = KeReleaseSemaphore(Semaphore,
                                                IO_NO_INCREMENT,
                                                ReleaseCount,
                                                FALSE);

            /* Return the old count if requested */
            if (PreviousCount) *PreviousCount = PrevCount;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            /* Get the exception code */
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        /* Dereference the Semaphore */
        ObDereferenceObject(Semaphore);
    }

    /* Return Status */
    return Status;
}

/* EOF */
