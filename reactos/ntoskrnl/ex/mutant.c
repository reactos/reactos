/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/mutant.c
 * PURPOSE:         Executive Management of Mutants
 *
 * PROGRAMMERS:     Alex Ionescu - Fix tab/space mismatching, tiny fixes to query function and
 *                                 add more debug output.
 *                  David Welch (welch@cwcom.net)
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#ifndef MUTANT_INCREMENT
#define MUTANT_INCREMENT                1
#endif

POBJECT_TYPE ExMutantObjectType = NULL;

static GENERIC_MAPPING ExpMutantMapping = {
    STANDARD_RIGHTS_READ    | SYNCHRONIZE | MUTANT_QUERY_STATE,
    STANDARD_RIGHTS_WRITE   | SYNCHRONIZE,
    STANDARD_RIGHTS_EXECUTE | SYNCHRONIZE | MUTANT_QUERY_STATE,
    MUTANT_ALL_ACCESS};

static const INFORMATION_CLASS_INFO ExMutantInfoClass[] = {

     /* MutantBasicInformation */
    ICI_SQ_SAME( sizeof(MUTANT_BASIC_INFORMATION), sizeof(ULONG), ICIF_QUERY ),
};

/* FUNCTIONS *****************************************************************/

VOID
STDCALL
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
STDCALL
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
    ObpCreateTypeObject(&ObjectTypeInitializer, &Name, &ExMutantObjectType);
}

/*
 * @implemented
 */
NTSTATUS
STDCALL
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
    DPRINT("NtCreateMutant(0x%p, 0x%x, 0x%p)\n", MutantHandle, DesiredAccess, ObjectAttributes);

    /* Check Output Safety */
    if(PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWriteHandle(MutantHandle);
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

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
    if(NT_SUCCESS(Status)) {

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
        ObDereferenceObject(Mutant);

        /* Check for success and return handle */
        if(NT_SUCCESS(Status)) {

            _SEH_TRY {

                *MutantHandle = hMutant;

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
NtOpenMutant(OUT PHANDLE MutantHandle,
             IN ACCESS_MASK DesiredAccess,
             IN POBJECT_ATTRIBUTES ObjectAttributes)
{
    HANDLE hMutant;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();
    DPRINT("NtOpenMutant(0x%p, 0x%x, 0x%p)\n", MutantHandle, DesiredAccess, ObjectAttributes);

    /* Check Output Safety */
    if(PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWriteHandle(MutantHandle);
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Open the Object */
    Status = ObOpenObjectByName(ObjectAttributes,
                                ExMutantObjectType,
                                NULL,
                                PreviousMode,
                                DesiredAccess,
                                NULL,
                                &hMutant);

    /* Check for success and return handle */
    if(NT_SUCCESS(Status)) {

        _SEH_TRY {

            *MutantHandle = hMutant;

        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

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
NtQueryMutant(IN HANDLE MutantHandle,
              IN MUTANT_INFORMATION_CLASS MutantInformationClass,
              OUT PVOID MutantInformation,
              IN ULONG MutantInformationLength,
              OUT PULONG ResultLength  OPTIONAL)
{
    PKMUTANT Mutant;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status = STATUS_SUCCESS;
    PMUTANT_BASIC_INFORMATION BasicInfo = (PMUTANT_BASIC_INFORMATION)MutantInformation;

    PAGED_CODE();

    /* Check buffers and parameters */
    DefaultQueryInfoBufferCheck(MutantInformationClass,
                                ExMutantInfoClass,
                                MutantInformation,
                                MutantInformationLength,
                                ResultLength,
                                PreviousMode,
                                &Status);
    if(!NT_SUCCESS(Status)) {

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
    if(NT_SUCCESS(Status)) {

         _SEH_TRY {

            /* Fill out the Basic Information Requested */
            DPRINT("Returning Mutant Information\n");
            BasicInfo->CurrentCount = KeReadStateMutant(Mutant);
            BasicInfo->OwnedByCaller = (Mutant->OwnerThread == KeGetCurrentThread());
            BasicInfo->AbandonedState = Mutant->Abandoned;

            /* Return the Result Length if requested */
           if(ResultLength) *ResultLength = sizeof(MUTANT_BASIC_INFORMATION);

        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

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
STDCALL
NtReleaseMutant(IN HANDLE MutantHandle,
                IN PLONG PreviousCount  OPTIONAL)
{
    PKMUTANT Mutant;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    PreviousMode = ExGetPreviousMode();

    DPRINT("NtReleaseMutant(MutantHandle 0x%p PreviousCount 0x%p)\n",
            MutantHandle,
            PreviousCount);

    /* Check Output Safety */
    if(PreviousMode != KernelMode && PreviousCount) {

        _SEH_TRY {

            ProbeForWriteLong(PreviousCount);
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

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
    if(NT_SUCCESS(Status)) {

        LONG Prev = 0;

        /* release the mutant. doing so might raise an exception which we're
           required to catch! */
        _SEH_TRY {

            Prev = KeReleaseMutant(Mutant, MUTANT_INCREMENT, FALSE, FALSE);

        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();

        } _SEH_END;

        ObDereferenceObject(Mutant);

        if(NT_SUCCESS(Status)) {

            /* Return it */
            if(PreviousCount) {

                _SEH_TRY {

                    *PreviousCount = Prev;

                } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

                    Status = _SEH_GetExceptionCode();

                } _SEH_END;
            }
        }
    }

    /* Return Status */
    return Status;
}

/* EOF */
