/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Kernel
 * FILE:            ntoskrnl/ex/mutant.c
 * PURPOSE:         Executive Management of Mutants
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 *                  Thomas Weidenmueller
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, ExpInitializeMutantImplementation)
#endif

/* DATA **********************************************************************/

POBJECT_TYPE ExMutantObjectType = NULL;

GENERIC_MAPPING ExpMutantMapping =
{
    STANDARD_RIGHTS_READ    | SYNCHRONIZE | MUTANT_QUERY_STATE,
    STANDARD_RIGHTS_WRITE   | SYNCHRONIZE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | MUTANT_QUERY_STATE,
    MUTANT_ALL_ACCESS
};

static const INFORMATION_CLASS_INFO ExMutantInfoClass[] =
{
     /* MutantBasicInformation */
    ICI_SQ_SAME( sizeof(MUTANT_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY),
};

/* FUNCTIONS *****************************************************************/

VOID
NTAPI
ExpDeleteMutant(PVOID ObjectBody)
{
    DPRINT("ExpDeleteMutant(ObjectBody 0x%p)\n", ObjectBody);

    /* Make sure to release the Mutant */
    KeReleaseMutant((PKMUTANT)ObjectBody,
                    MUTANT_INCREMENT,
                    TRUE,
                    FALSE);
}

VOID
INIT_FUNCTION
NTAPI
ExpInitializeMutantImplementation(VOID)
{
    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    UNICODE_STRING Name;
    DPRINT("Creating Mutant Object Type\n");

    /* Create the Event Pair Object Type */
    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    RtlInitUnicodeString(&Name, L"Mutant");
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(KMUTANT);
    ObjectTypeInitializer.GenericMapping = ExpMutantMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DeleteProcedure = ExpDeleteMutant;
    ObjectTypeInitializer.ValidAccessMask = MUTANT_ALL_ACCESS;
    ObCreateObjectType(&Name, &ObjectTypeInitializer, NULL, &ExMutantObjectType);
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtCreateMutant(OUT PHANDLE MutantHandle,
               IN ACCESS_MASK DesiredAccess,
               IN POBJECT_ATTRIBUTES ObjectAttributes  OPTIONAL,
               IN BOOLEAN InitialOwner)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    HANDLE hMutant;
    PKMUTANT Mutant;
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();
    DPRINT("NtCreateMutant(0x%p, 0x%x, 0x%p)\n",
            MutantHandle, DesiredAccess, ObjectAttributes);

    /* Check if we were called from user-mode */
    if(PreviousMode != KernelMode)
    {
        /* Enter SEH Block */
        _SEH2_TRY
        {
            /* Check handle pointer */
            ProbeForWriteHandle(MutantHandle);
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        /* Bail out if pointer was invalid */
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Create the Mutant Object*/
    Status = ObCreateObject(PreviousMode,
                            ExMutantObjectType,
                            ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(KMUTANT),
                            0,
                            0,
                            (PVOID*)&Mutant);

    /* Check for success */
    if(NT_SUCCESS(Status))
    {
        /* Initalize the Kernel Mutant */
        DPRINT("Initializing the Mutant\n");
        KeInitializeMutant(Mutant, InitialOwner);

        /* Insert the Object */
        Status = ObInsertObject((PVOID)Mutant,
                                NULL,
                                DesiredAccess,
                                0,
                                NULL,
                                &hMutant);

        /* Check for success */
        if(NT_SUCCESS(Status))
        {
            /* Enter SEH for return */
            _SEH2_TRY
            {
                /* Return the handle to the caller */
                *MutantHandle = hMutant;
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
NtOpenMutant(OUT PHANDLE MutantHandle,
             IN ACCESS_MASK DesiredAccess,
             IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hMutant;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();
    DPRINT("NtOpenMutant(0x%p, 0x%x, 0x%p)\n",
            MutantHandle, DesiredAccess, ObjectAttributes);

    /* Check if we were called from user-mode */
    if(PreviousMode != KernelMode)
    {
        /* Enter SEH Block */
        _SEH2_TRY
        {
            /* Check handle pointer */
            ProbeForWriteHandle(MutantHandle);
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        /* Bail out if pointer was invalid */
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ExMutantObjectType,
                                PreviousMode,
                                NULL,
                                DesiredAccess,
                                NULL,
                                &hMutant);

    /* Check for success */
    if(NT_SUCCESS(Status))
    {
        /* Enter SEH for return */
        _SEH2_TRY
        {
            /* Return the handle to the caller */
            *MutantHandle = hMutant;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
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
NtQueryMutant(IN HANDLE MutantHandle,
              IN MUTANT_INFORMATION_CLASS MutantInformationClass,
              OUT PVOID MutantInformation,
              IN ULONG MutantInformationLength,
              OUT PULONG ResultLength  OPTIONAL)
{
    PKMUTANT Mutant;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PMUTANT_BASIC_INFORMATION BasicInfo =
        (PMUTANT_BASIC_INFORMATION)MutantInformation;
    PAGED_CODE();

    /* Check buffers and parameters */
    Status = DefaultQueryInfoBufferCheck(MutantInformationClass,
                                         ExMutantInfoClass,
                                         sizeof(ExMutantInfoClass) /
                                         sizeof(ExMutantInfoClass[0]),
                                         MutantInformation,
                                         MutantInformationLength,
                                         ResultLength,
                                         NULL,
                                         PreviousMode);
    if(!NT_SUCCESS(Status))
    {
        DPRINT("NtQueryMutant() failed, Status: 0x%x\n", Status);
        return Status;
    }

    /* Open the Object */
    Status = ObReferenceObjectByHandle(MutantHandle,
                                       MUTANT_QUERY_STATE,
                                       ExMutantObjectType,
                                       PreviousMode,
                                       (PVOID*)&Mutant,
                                       NULL);
    /* Check for Status */
    if(NT_SUCCESS(Status))
    {
        /* Enter SEH Block for return */
         _SEH2_TRY
         {
            /* Fill out the Basic Information Requested */
            DPRINT("Returning Mutant Information\n");
            BasicInfo->CurrentCount = KeReadStateMutant(Mutant);
            BasicInfo->OwnedByCaller = (Mutant->OwnerThread ==
                                        KeGetCurrentThread());
            BasicInfo->AbandonedState = Mutant->Abandoned;

            /* Return the Result Length if requested */
           if(ResultLength) *ResultLength = sizeof(MUTANT_BASIC_INFORMATION);
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        /* Release the Object */
        ObDereferenceObject(Mutant);
    }

    /* Return Status */
    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtReleaseMutant(IN HANDLE MutantHandle,
                IN PLONG PreviousCount OPTIONAL)
{
    PKMUTANT Mutant;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PAGED_CODE();
    DPRINT("NtReleaseMutant(MutantHandle 0x%p PreviousCount 0x%p)\n",
            MutantHandle,
            PreviousCount);

     /* Check if we were called from user-mode */
    if((PreviousCount) && (PreviousMode != KernelMode))
    {
        /* Entry SEH Block */
        _SEH2_TRY
        {
            /* Make sure the state pointer is valid */
            ProbeForWriteLong(PreviousCount);
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        /* Bail out if pointer was invalid */
        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Open the Object */
    Status = ObReferenceObjectByHandle(MutantHandle,
                                       MUTANT_QUERY_STATE,
                                       ExMutantObjectType,
                                       PreviousMode,
                                       (PVOID*)&Mutant,
                                       NULL);

    /* Check for Success and release if such */
    if(NT_SUCCESS(Status))
    {
        /*
         * Release the mutant. doing so might raise an exception which we're
         * required to catch!
         */
        _SEH2_TRY
        {
            /* Release the mutant */
            LONG Prev = KeReleaseMutant(Mutant,
                                        MUTANT_INCREMENT,
                                        FALSE,
                                        FALSE);

            /* Return the previous count if requested */
            if(PreviousCount) *PreviousCount = Prev;
        }
        _SEH2_EXCEPT(ExSystemExceptionFilter())
        {
            Status = _SEH2_GetExceptionCode();
        }
        _SEH2_END;

        /* Dereference it */
        ObDereferenceObject(Mutant);
    }

    /* Return Status */
    return Status;
}

/* EOF */
