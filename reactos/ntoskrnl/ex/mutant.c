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

    DPRINT("ExpDeleteMutant(ObjectBody %x)\n", ObjectBody);

    /* Make sure to release the Mutant */
    KeReleaseMutant((PKMUTANT)ObjectBody,
                    MUTANT_INCREMENT,
                    TRUE,
                    FALSE);
}

VOID
INIT_FUNCTION
ExpInitializeMutantImplementation(VOID)
{

    /* Allocate the Object Type */
    ExMutantObjectType = ExAllocatePoolWithTag(NonPagedPool, sizeof(OBJECT_TYPE), TAG('M', 't', 'n', 't'));

    /* Create the Object Type */
    RtlInitUnicodeString(&ExMutantObjectType->TypeName, L"Mutant");
    ExMutantObjectType->Tag = TAG('M', 't', 'n', 't');
    ExMutantObjectType->PeakObjects = 0;
    ExMutantObjectType->PeakHandles = 0;
    ExMutantObjectType->TotalObjects = 0;
    ExMutantObjectType->TotalHandles = 0;
    ExMutantObjectType->PagedPoolCharge = 0;
    ExMutantObjectType->NonpagedPoolCharge = sizeof(KMUTANT);
    ExMutantObjectType->Mapping = &ExpMutantMapping;
    ExMutantObjectType->Dump = NULL;
    ExMutantObjectType->Open = NULL;
    ExMutantObjectType->Close = NULL;
    ExMutantObjectType->Delete = ExpDeleteMutant;
    ExMutantObjectType->Parse = NULL;
    ExMutantObjectType->Open = NULL;
    ExMutantObjectType->Security = NULL;
    ExMutantObjectType->QueryName = NULL;
    ExMutantObjectType->OkayToClose = NULL;
    ObpCreateTypeObject(ExMutantObjectType);
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
    DPRINT("NtCreateMutant(0x%x, 0x%x, 0x%x)\n", MutantHandle, DesiredAccess, ObjectAttributes);

    /* Check Output Safety */
    if(PreviousMode == UserMode) {

        _SEH_TRY {

            ProbeForWrite(MutantHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
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
    DPRINT("NtOpenMutant(0x%x, 0x%x, 0x%x)\n", MutantHandle, DesiredAccess, ObjectAttributes);

    /* Check Output Safety */
    if(PreviousMode == UserMode) {

        _SEH_TRY {

            ProbeForWrite(MutantHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));
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

    DPRINT("NtReleaseMutant(MutantHandle 0%x PreviousCount 0%x)\n",
            MutantHandle,
            PreviousCount);

    /* Check Output Safety */
    if(PreviousMode != KernelMode && PreviousCount) {

        _SEH_TRY {

            ProbeForWrite(PreviousCount,
                          sizeof(LONG),
                          sizeof(ULONG));
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
