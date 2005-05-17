/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/profile.c
 * PURPOSE:         Support for Executive Profile Objects
 *
 * PROGRAMMERS:     Alex Ionescu
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include <internal/debug.h>

/* This structure is a *GUESS* -- Alex */
typedef struct _EPROFILE {
    PEPROCESS Process;
    PVOID ImageBase;
    ULONG ImageSize;
    ULONG BucketSize;
    PVOID Buffer;
    ULONG BufferSize;
    PKPROFILE KeProfile;
    KPROFILE_SOURCE ProfileSource;
    KAFFINITY Affinity;
    PMDL Mdl;
    PVOID LockedBuffer;
} EPROFILE, *PEPROFILE;

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED ExProfileObjectType = NULL;

static KMUTEX ExpProfileMutex;

#define PROFILE_CONTROL 1

static GENERIC_MAPPING ExpProfileMapping = {
    STANDARD_RIGHTS_READ    | PROFILE_CONTROL,
    STANDARD_RIGHTS_WRITE   | PROFILE_CONTROL,
    STANDARD_RIGHTS_EXECUTE | PROFILE_CONTROL,
    STANDARD_RIGHTS_ALL};

VOID
STDCALL
ExpDeleteProfile(PVOID ObjectBody)
{
    PEPROFILE Profile;

    /* Typecast the Object */
    Profile = (PEPROFILE)ObjectBody;

    /* Check if there if the Profile was started */
    if (Profile->LockedBuffer) {

        /* Stop the Profile */
        KeStopProfile(Profile->KeProfile);

        /* Unmap the Locked Buffer */
        MmUnmapLockedPages(Profile->LockedBuffer, Profile->Mdl);
        MmUnlockPages(Profile->Mdl);
        ExFreePool(Profile->Mdl);
    }

    /* Check if a Process is associated */
    if (Profile->Process != NULL) {

        /* Dereference it */
        ObDereferenceObject(Profile->Process);
        Profile->Process = NULL;
    }
}

VOID
INIT_FUNCTION
ExpInitializeProfileImplementation(VOID)
{
    /* Initialize the Mutex to lock the States */
    KeInitializeMutex(&ExpProfileMutex, 0x40);

    /* Create the Object Type */
    ExProfileObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
    RtlInitUnicodeString(&ExProfileObjectType->TypeName, L"Profile");
    ExProfileObjectType->Tag = TAG('P', 'R', 'O', 'F');
    ExProfileObjectType->PeakObjects = 0;
    ExProfileObjectType->PeakHandles = 0;
    ExProfileObjectType->TotalObjects = 0;
    ExProfileObjectType->TotalHandles = 0;
    ExProfileObjectType->PagedPoolCharge = 0;
    ExProfileObjectType->NonpagedPoolCharge = sizeof(EPROFILE);
    ExProfileObjectType->Mapping = &ExpProfileMapping;
    ExProfileObjectType->Dump = NULL;
    ExProfileObjectType->Open = NULL;
    ExProfileObjectType->Close = NULL;
    ExProfileObjectType->Delete = ExpDeleteProfile;
    ExProfileObjectType->Parse = NULL;
    ExProfileObjectType->Security = NULL;
    ExProfileObjectType->QueryName = NULL;
    ExProfileObjectType->OkayToClose = NULL;
    ExProfileObjectType->Create = NULL;
    ObpCreateTypeObject(ExProfileObjectType);
}

NTSTATUS
STDCALL
NtCreateProfile(OUT PHANDLE ProfileHandle,
                IN HANDLE Process OPTIONAL,
                IN PVOID ImageBase,
                IN ULONG ImageSize,
                IN ULONG BucketSize,
                IN PVOID Buffer,
                IN ULONG BufferSize,
                IN KPROFILE_SOURCE ProfileSource,
                IN KAFFINITY Affinity)
{
    HANDLE hProfile;
    PEPROFILE Profile;
    PEPROCESS pProcess;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    OBJECT_ATTRIBUTES ObjectAttributes;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    /* Easy way out */
    if(BufferSize == 0) return STATUS_INVALID_PARAMETER_7;

    /* Check the Parameters for validity */
    if(PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWrite(ProfileHandle,
                          sizeof(HANDLE),
                          sizeof(ULONG));

            ProbeForWrite(Buffer,
                          BufferSize,
                          sizeof(ULONG));
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();
        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Check if a process was specified */
    if (Process) {

        /* Reference it */
        Status = ObReferenceObjectByHandle(Process,
                                           PROCESS_QUERY_INFORMATION,
                                           PsProcessType,
                                           PreviousMode,
                                           (PVOID*)&pProcess,
                                           NULL);
        if (!NT_SUCCESS(Status)) return(Status);

    } else {

        /* No process was specified, which means a System-Wide Profile */
        pProcess = NULL;

        /* For this, we need to check the Privilege */
        if(!SeSinglePrivilegeCheck(SeSystemProfilePrivilege, PreviousMode)) {

            DPRINT1("NtCreateProfile: Caller requires the SeSystemProfilePrivilege privilege!\n");
            return STATUS_PRIVILEGE_NOT_HELD;
        }
    }

    /* Create the object */
    InitializeObjectAttributes(&ObjectAttributes,
                               NULL,
                               0,
                               NULL,
                               NULL);
    Status = ObCreateObject(KernelMode,
                            ExProfileObjectType,
                            &ObjectAttributes,
                            PreviousMode,
                            NULL,
                            sizeof(EPROFILE),
                            0,
                            0,
                            (PVOID*)&Profile);
    if (!NT_SUCCESS(Status)) return(Status);

    /* Initialize it */
    Profile->ImageBase = ImageBase;
    Profile->ImageSize = ImageSize;
    Profile->Buffer = Buffer;
    Profile->BufferSize = BufferSize;
    Profile->BucketSize = BucketSize;
    Profile->LockedBuffer = NULL;
    Profile->Affinity = Affinity;
    Profile->Process = pProcess;

    /* Insert into the Object Tree */
    Status = ObInsertObject ((PVOID)Profile,
                             NULL,
                             PROFILE_CONTROL,
                             0,
                             NULL,
                             &hProfile);
    ObDereferenceObject(Profile);

    /* Check for Success */
    if (!NT_SUCCESS(Status)) {

        /* Dereference Process on failure */
        if (pProcess) ObDereferenceObject(pProcess);
        return Status;
    }

    /* Copy the created handle back to the caller*/
    _SEH_TRY {

        *ProfileHandle = hProfile;

    } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

        Status = _SEH_GetExceptionCode();
    } _SEH_END;

    /* Return Status */
    return Status;
}

NTSTATUS
STDCALL
NtQueryPerformanceCounter(OUT PLARGE_INTEGER PerformanceCounter,
                          OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    LARGE_INTEGER PerfFrequency;
    NTSTATUS Status = STATUS_SUCCESS;

    /* Check the Parameters for validity */
    if(PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWrite(PerformanceCounter,
                          sizeof(LARGE_INTEGER),
                          sizeof(ULONG));

            ProbeForWrite(PerformanceFrequency,
                          sizeof(LARGE_INTEGER),
                          sizeof(ULONG));
        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();
        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    _SEH_TRY {

        /* Query the Kernel */
        *PerformanceCounter = KeQueryPerformanceCounter(&PerfFrequency);

        /* Return Frequency if requested */
        if(PerformanceFrequency) {

            *PerformanceFrequency = PerfFrequency;
        }
    } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

        Status = _SEH_GetExceptionCode();

    } _SEH_END;

    return Status;
}

NTSTATUS
STDCALL
NtStartProfile(IN HANDLE ProfileHandle)
{
    PEPROFILE Profile;
    PKPROFILE KeProfile;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    PVOID TempLockedBuffer;
    NTSTATUS Status;

    PAGED_CODE();

    /* Get the Object */
    Status = ObReferenceObjectByHandle(ProfileHandle,
                                       PROFILE_CONTROL,
                                       ExProfileObjectType,
                                       PreviousMode,
                                       (PVOID*)&Profile,
                                       NULL);
    if (!NT_SUCCESS(Status)) return(Status);

    /* To avoid a Race, wait on the Mutex */
    KeWaitForSingleObject(&ExpProfileMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    /* The Profile can still be enabled though, so handle that */
    if (Profile->LockedBuffer) {

        /* Release our lock, dereference and return */
        KeReleaseMutex(&ExpProfileMutex, FALSE);
        ObDereferenceObject(Profile);
        return STATUS_PROFILING_NOT_STOPPED;
    }

    /* Allocate a Kernel Profile Object. */
    KeProfile = ExAllocatePoolWithTag(NonPagedPool,
                                      sizeof(EPROFILE),
                                      TAG('P', 'r', 'o', 'f'));

    /* Allocate the Mdl Structure */
    Profile->Mdl = MmCreateMdl(NULL, Profile->Buffer, Profile->BufferSize);

    /* Probe and Lock for Write Access */
    MmProbeAndLockPages(Profile->Mdl, PreviousMode, IoWriteAccess);

    /* Map the pages */
    TempLockedBuffer = MmMapLockedPages(Profile->Mdl, KernelMode);

    /* Initialize the Kernel Profile Object */
    Profile->KeProfile = KeProfile;
    KeInitializeProfile(KeProfile,
                        (PKPROCESS)Profile->Process,
                        Profile->ImageBase,
                        Profile->ImageSize,
                        Profile->BucketSize,
                        Profile->ProfileSource,
                        Profile->Affinity);

    /* Start the Profiling */
    KeStartProfile(KeProfile, TempLockedBuffer);

    /* Now it's safe to save this */
    Profile->LockedBuffer = TempLockedBuffer;

    /* Release mutex, dereference and return */
    KeReleaseMutex(&ExpProfileMutex, FALSE);
    ObDereferenceObject(Profile);
    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
NtStopProfile(IN HANDLE ProfileHandle)
{
    PEPROFILE Profile;
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    NTSTATUS Status;

    PAGED_CODE();

    /* Get the Object */
    Status = ObReferenceObjectByHandle(ProfileHandle,
                                       PROFILE_CONTROL,
                                       ExProfileObjectType,
                                       PreviousMode,
                                       (PVOID*)&Profile,
                                       NULL);
    if (!NT_SUCCESS(Status)) return(Status);

    /* Get the Mutex */
    KeWaitForSingleObject(&ExpProfileMutex,
                          Executive,
                          KernelMode,
                          FALSE,
                          NULL);

    /* Make sure the Profile Object is really Started */
    if (!Profile->LockedBuffer) {

        Status = STATUS_PROFILING_NOT_STARTED;
        goto Exit;
    }

    /* Stop the Profile */
    KeStopProfile(Profile->KeProfile);

    /* Unlock the Buffer */
    MmUnmapLockedPages(Profile->LockedBuffer, Profile->Mdl);
    MmUnlockPages(Profile->Mdl);
    ExFreePool(Profile->KeProfile);

    /* Clear the Locked Buffer pointer, meaning the Object is Stopped */
    Profile->LockedBuffer = NULL;

Exit:
    /* Release Mutex, Dereference and Return */
    KeReleaseMutex(&ExpProfileMutex, FALSE);
    ObDereferenceObject(Profile);
    return Status;
}

NTSTATUS
STDCALL
NtQueryIntervalProfile(IN  KPROFILE_SOURCE ProfileSource,
                       OUT PULONG Interval)
{
    KPROCESSOR_MODE PreviousMode = ExGetPreviousMode();
    ULONG ReturnInterval;
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    /* Check the Parameters for validity */
    if(PreviousMode != KernelMode) {

        _SEH_TRY {

            ProbeForWrite(Interval,
                          sizeof(ULONG),
                          sizeof(ULONG));

        } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

            Status = _SEH_GetExceptionCode();
        } _SEH_END;

        if(!NT_SUCCESS(Status)) return Status;
    }

    /* Query the Interval */
    ReturnInterval = KeQueryIntervalProfile(ProfileSource);

    /* Return the data */
    _SEH_TRY  {

        *Interval = ReturnInterval;

    } _SEH_EXCEPT(_SEH_ExSystemExceptionFilter) {

        Status = _SEH_GetExceptionCode();

    } _SEH_END;

    /* Return Success */
    return STATUS_SUCCESS;
}

NTSTATUS
STDCALL
NtSetIntervalProfile(IN ULONG Interval,
                     IN KPROFILE_SOURCE Source)
{
    /* Let the Kernel do the job */
    KeSetIntervalProfile(Interval, Source);

    /* Nothing can go wrong */
    return STATUS_SUCCESS;
}
