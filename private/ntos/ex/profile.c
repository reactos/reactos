/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    profile.c

Abstract:

   This module implements the executive profile object. Functions are provided
   to create, start, stop, and query profile objects.

Author:

    Lou Perazzoli (loup) 21-Sep-1990

Environment:

    Kernel mode only.

Revision History:

--*/

#include "exp.h"

//
// Executive profile object.
//

typedef struct _EPROFILE {
    PKPROCESS Process;
    PVOID RangeBase;
    SIZE_T RangeSize;
    PVOID Buffer;
    ULONG BufferSize;
    ULONG BucketSize;
    PKPROFILE ProfileObject;
    PVOID LockedBufferAddress;
    PMDL Mdl;
    ULONG Segment;
    KPROFILE_SOURCE ProfileSource;
    KAFFINITY Affinity;
} EPROFILE, *PEPROFILE;

//
// Address of event object type descriptor.
//

POBJECT_TYPE ExProfileObjectType;

KMUTEX ExpProfileStateMutex;

ULONG ExpCurrentProfileUsage = 0;

GENERIC_MAPPING ExpProfileMapping = {
    STANDARD_RIGHTS_READ | PROFILE_CONTROL,
    STANDARD_RIGHTS_WRITE | PROFILE_CONTROL,
    STANDARD_RIGHTS_EXECUTE | PROFILE_CONTROL,
    PROFILE_ALL_ACCESS
};

#define ACTIVE_PROFILE_LIMIT 8

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT, ExpProfileInitialization)
#pragma alloc_text(PAGE, ExpProfileDelete)
#pragma alloc_text(PAGE, NtCreateProfile)
#pragma alloc_text(PAGE, NtStartProfile)
#pragma alloc_text(PAGE, NtStopProfile)
#pragma alloc_text(PAGE, NtSetIntervalProfile)
#pragma alloc_text(PAGE, NtQueryIntervalProfile)
#pragma alloc_text(PAGE, NtQueryPerformanceCounter)
#endif


BOOLEAN
ExpProfileInitialization (
    )

/*++

Routine Description:

    This function creates the profile object type descriptor at system
    initialization and stores the address of the object type descriptor
    in global storage.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the profile object type descriptor is
    successfully initialized. Otherwise a value of FALSE is returned.

--*/

{

    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    NTSTATUS Status;
    UNICODE_STRING TypeName;

    //
    // Initialize mutex for synchronizing start and stop operations.
    //

    KeInitializeMutex (&ExpProfileStateMutex, MUTEX_LEVEL_EX_PROFILE);

    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString(&TypeName, L"Profile");

    //
    // Create event object type descriptor.
    //

    RtlZeroMemory(&ObjectTypeInitializer,sizeof(ObjectTypeInitializer));
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.InvalidAttributes = OBJ_OPENLINK;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(EPROFILE);
    ObjectTypeInitializer.ValidAccessMask = PROFILE_ALL_ACCESS;
    ObjectTypeInitializer.DeleteProcedure = ExpProfileDelete;
    ObjectTypeInitializer.GenericMapping = ExpProfileMapping;

    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                (PSECURITY_DESCRIPTOR)NULL,
                                &ExProfileObjectType);

    //
    // If the event object type descriptor was successfully created, then
    // return a value of TRUE. Otherwise return a value of FALSE.
    //

    return (BOOLEAN)(NT_SUCCESS(Status));
}
VOID
ExpProfileDelete (
    IN PVOID Object
    )

/*++

Routine Description:


    This routine is called by the object management procedures whenever
    the last reference to a profile object has been removed.  This routine
    stops profiling, returns locked buffers and pages, dereferences the
    specified process and returns.

Arguments:

    Object - a pointer to the body of the profile object.

Return Value:

    None.

--*/

{
    PEPROFILE Profile;
    BOOLEAN   State;
    PEPROCESS ProcessAddress;

    Profile = (PEPROFILE)Object;

    if (Profile->LockedBufferAddress != NULL) {

        //
        // Stop profiling and unlock the buffers and deallocate pool.
        //

        State = KeStopProfile (Profile->ProfileObject);
        ASSERT (State != FALSE);

        MmUnmapLockedPages (Profile->LockedBufferAddress, Profile->Mdl);
        MmUnlockPages (Profile->Mdl);
        ExFreePool (Profile->ProfileObject);
    }

    if (Profile->Process != NULL) {
        ProcessAddress = CONTAINING_RECORD(Profile->Process, EPROCESS, Pcb);
        ObDereferenceObject ((PVOID)ProcessAddress);
    }

    return;
}

NTSTATUS
NtCreateProfile (
    OUT PHANDLE ProfileHandle,
    IN HANDLE Process OPTIONAL,
    IN PVOID RangeBase,
    IN SIZE_T RangeSize,
    IN ULONG BucketSize,
    IN PULONG Buffer,
    IN ULONG BufferSize,
    IN KPROFILE_SOURCE ProfileSource,
    IN KAFFINITY Affinity
    )

/*++

Routine Description:

    This function creates a profile object.

Arguments:

    ProfileHandle - Supplies a pointer to a variable that will receive
                    the profile object handle.

    Process - Optionally, supplies the handle to the process whose
              address space to profile.  If the value is NULL (0), then
              all address spaces are included in the profile.

    RangeBase - Supplies the address of the first byte of the address
                  space for which profiling information is to be collected.


    RangeSize - Supplies the size of the range to profile in the
                address space.  RangeBase and RangeSize are interpreted
                such that RangeBase <= address < RangeBase+RangeSize
                will generate a profile hit.

    BucketSize - Supplies the LOG base 2 of the size of the profiling
                 bucket.  Thus, BucketSize = 2 yields four-byte
                 buckets, BucketSize = 7 yields 128-byte buckets.
                 All profile hits in a given bucket will increment
                 the corresponding counter in Buffer.  Buckets
                 cannot be smaller than a ULONG.  The acceptable range
                 of this value is 2 to 30 inclusive.

    Buffer - Supplies an array of ULONGs.  Each ULONG is a hit counter,
             which records the number of hits of the corresponding
             bucket.

    BufferSize - Size in bytes of Buffer.

    ProfileSource - Supplies the source for the profile interrupt

    Affinity - Supplies the processor set for the profile interrupt

Return Value:

    TBS

--*/

{

    PEPROFILE Profile;
    HANDLE Handle;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PEPROCESS ProcessAddress;
    OBJECT_ATTRIBUTES ObjectAttributes;
    BOOLEAN HasPrivilege = FALSE;
    ULONG Segment = FALSE;
    USHORT PowerOf2;

    //
    // Verify that the base and size arguments are reasonable.
    //

    if (BufferSize == 0) {
        return STATUS_INVALID_PARAMETER_7;
    }

#ifdef i386
    //
    //        sleazy use of bucket size.  If bucket size is zero, and
    //        RangeBase < 64K, then create a profile object to attach
    //        to a non-flat code segment.  In this case, RangeBase is
    //        the non-flat CS for this profile object.
    //

    if ((BucketSize == 0) && (RangeBase < (PVOID)(64 * 1024))) {

        if (BufferSize < sizeof(ULONG)) {
            return STATUS_INVALID_PARAMETER_7;
        }

        Segment = (ULONG)RangeBase;
        RangeBase = 0;
        BucketSize = RangeSize / (BufferSize / sizeof(ULONG));

        //
        // Convert Bucket size of log2(BucketSize)
        //
        PowerOf2 = 0;
        BucketSize = BucketSize - 1;
        while (BucketSize >>= 1) {
            PowerOf2++;
        }

        BucketSize = PowerOf2 + 1;

        if (BucketSize < 2) {
            BucketSize = 2;
        }
    }
#endif

    if ((BucketSize > 31) || (BucketSize < 2)) {
        return STATUS_INVALID_PARAMETER;
    }

    if ((RangeSize >> (BucketSize - 2)) > BufferSize) {
        return STATUS_BUFFER_TOO_SMALL;
    }

    if (((ULONG_PTR)RangeBase + RangeSize) < RangeSize) {
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Establish an exception handler, probe the output handle address, and
    // attempt to create a profile object. If the probe fails, then return the
    // exception code as the service status. Otherwise return the status value
    // returned by the object insertion routine.
    //

    try {
        //
        // Get previous processor mode and probe output handle address if
        // necessary.
        //

        PreviousMode = KeGetPreviousMode ();

        if (PreviousMode != KernelMode) {
            ProbeForWriteHandle(ProfileHandle);

            ProbeForWrite(Buffer,
                          BufferSize,
                          sizeof(ULONG));
        }

    //
    // If an exception occurs during the probe of the output handle address,
    // then always handle the exception and return the exception code as the
    // status value.
    //

    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

//
// TODO post NT5:
//
// Currently, if a process isn't specified, there is no privilege check if
//   RangeBase > MM_HIGHEST_USER_ADDRESS.
// The check for user-space addresses is SeSystemProfilePrivilege.
// Querying a specific process requires only PROCESS_QUERY_INFORMATION.
//
// The spec says:
//
//     Process - If specified, a handle to a process which describes the address space to profile.
//     If not present, then all address spaces are included in the profile.
//     Profiling a process requires PROCESS_QUERY_INFORMATION access to that process and
//     SeProfileSingleProcessPrivilege privilege.
//     Profiling all processes requires SeSystemProfilePrivilege privilege.
//
// So two changes appear needed.
//   A check on SeProfileSingleProcessPrivilege needs to be added to the single process case,
//   and SeSystemProfilePrivilege privilege should be required for both user and system address profiling.
//


    if (!ARGUMENT_PRESENT(Process)) {

        //
        // Don't attach segmented profile objects to all processes
        //

        if (Segment) {
            return STATUS_INVALID_PARAMETER;
        }

        //
        // Profile all processes. Make sure that the specified
        // address range is in system space, unless SeSystemProfilePrivilege.
        //

        if (RangeBase <= MM_HIGHEST_USER_ADDRESS) {

            //
            // Check for privilege before allowing a user to profile
            // all processes and USER addresses.
            //

            if (PreviousMode != KernelMode) {
                HasPrivilege =  SeSinglePrivilegeCheck(
                                    SeSystemProfilePrivilege,
                                    PreviousMode
                                    );

                if (!HasPrivilege) {
#if DBG
                    DbgPrint("SeSystemProfilePrivilege needed to profile all USER addresses.\n");
#endif //DBG
                    return( STATUS_PRIVILEGE_NOT_HELD );
                }

            }
        }

        ProcessAddress = NULL;


    } else {

        //
        // Reference the specified process.
        //

        Status = ObReferenceObjectByHandle ( Process,
                                             PROCESS_QUERY_INFORMATION,
                                             PsProcessType,
                                             PreviousMode,
                                             (PVOID *)&ProcessAddress,
                                             NULL );

        if (!NT_SUCCESS(Status)) {
            return Status;
        }
    }

    InitializeObjectAttributes( &ObjectAttributes,
                                NULL,
                                OBJ_EXCLUSIVE,
                                NULL,
                                NULL );

    Status = ObCreateObject( KernelMode,
                             ExProfileObjectType,
                             &ObjectAttributes,
                             PreviousMode,
                             NULL,
                             sizeof(EPROFILE),
                             0,
                             sizeof(EPROFILE) + sizeof(KPROFILE),
                             (PVOID *)&Profile);

    //
    // If the profile object was successfully allocated, initialize
    // the profile object.
    //
    if (NT_SUCCESS(Status)) {


        if (ProcessAddress != NULL) {
            Profile->Process = &ProcessAddress->Pcb;
        } else {
            Profile->Process = NULL;
        }

        Profile->RangeBase = RangeBase;
        Profile->RangeSize = RangeSize;
        Profile->Buffer = Buffer;
        Profile->BufferSize = BufferSize;
        Profile->BucketSize = BucketSize;
        Profile->LockedBufferAddress = NULL;
        Profile->Segment = Segment;
        Profile->ProfileSource = ProfileSource;
        Profile->Affinity = Affinity;

        Status = ObInsertObject(Profile,
                                NULL,
                                PROFILE_CONTROL,
                                0,
                                (PVOID *)NULL,
                                &Handle);
        //
        // If the profile object was successfully inserted in the current
        // process' handle table, then attempt to write the profile object
        // handle value. If the write attempt fails, then do not report
        // an error. When the caller attempts to access the handle value,
        // an access violation will occur.
        //
        if (NT_SUCCESS(Status)) {
            try {
                *ProfileHandle = Handle;
            } except(EXCEPTION_EXECUTE_HANDLER) {
            }
        }
    }

    //
    // If we failed, remove our reference to the process object.
    //
    if (!NT_SUCCESS(Status)) {
        if (ProcessAddress != NULL) {
            ObDereferenceObject ((PVOID)ProcessAddress);
        }
    }

    //
    // Return service status.
    //

    return Status;
}

NTSTATUS
NtStartProfile (
    IN HANDLE ProfileHandle
    )

/*++

Routine Description:

    The NtStartProfile routine starts the collecting data for the
    specified profile object.  This involved allocating nonpaged
    pool to lock the specified buffer in memory, creating a kernel
    profile object and starting collecting on that profile object.

Arguments:

    ProfileHandle - Supplies the profile handle to start profiling on.

Return Value:

    TBS

--*/

{

    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PEPROFILE Profile;
    PKPROFILE ProfileObject;
    PVOID LockedVa;
    BOOLEAN State;

    PreviousMode = KeGetPreviousMode();

    Status = ObReferenceObjectByHandle( ProfileHandle,
                                        PROFILE_CONTROL,
                                        ExProfileObjectType,
                                        PreviousMode,
                                        (PVOID *)&Profile,
                                        NULL);
    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    //
    // Acquire the profile state mutex so two threads can't
    // operate on the same profile object simultaneously.
    //

    KeWaitForSingleObject( &ExpProfileStateMutex,
                           Executive,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    //
    // Make sure profiling is not already enabled.
    //

    if (Profile->LockedBufferAddress != NULL) {
        KeReleaseMutex (&ExpProfileStateMutex, FALSE);
        ObDereferenceObject ((PVOID)Profile);
        return STATUS_PROFILING_NOT_STOPPED;
    }

    if (ExpCurrentProfileUsage == ACTIVE_PROFILE_LIMIT) {
        KeReleaseMutex (&ExpProfileStateMutex, FALSE);
        ObDereferenceObject ((PVOID)Profile);
        return STATUS_PROFILING_AT_LIMIT;
    }

    ProfileObject = ExAllocatePoolWithTag (NonPagedPool,
                                    MmSizeOfMdl(Profile->Buffer,
                                                Profile->BufferSize) +
                                        sizeof(KPROFILE),
                                        'forP');

    if (ProfileObject == NULL) {
        KeReleaseMutex (&ExpProfileStateMutex, FALSE);
        ObDereferenceObject ((PVOID)Profile);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Profile->Mdl = (PMDL)(ProfileObject + 1);
    Profile->ProfileObject = ProfileObject;

    //
    // Probe and lock the specified buffer.
    //

    MmInitializeMdl(Profile->Mdl, Profile->Buffer, Profile->BufferSize);

    try {

        LockedVa = NULL;

        MmProbeAndLockPages (Profile->Mdl,
                             PreviousMode,
                             IoWriteAccess );

        LockedVa = (PVOID)43;  // flag to notice MmMapLockedPages failed.

        LockedVa = MmMapLockedPagesSpecifyCache (Profile->Mdl,
                                                 KernelMode,
                                                 MmCached,
                                                 NULL,
                                                 FALSE,
                                                 NormalPagePriority);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        if (LockedVa == (PVOID)43 ) {
            MmUnlockPages (Profile->Mdl);
        }
        ExFreePool (ProfileObject);
        KeReleaseMutex (&ExpProfileStateMutex, FALSE);
        ObDereferenceObject ((PVOID)Profile);
        return GetExceptionCode();
    }

    if (LockedVa == NULL) {
        MmUnlockPages (Profile->Mdl);
        ExFreePool (ProfileObject);
        KeReleaseMutex (&ExpProfileStateMutex, FALSE);
        ObDereferenceObject ((PVOID)Profile);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    // Initialize the profile object.
    //

    KeInitializeProfile (ProfileObject,
                         Profile->Process,
                         Profile->RangeBase,
                         Profile->RangeSize,
                         Profile->BucketSize,
                         Profile->Segment,
                         Profile->ProfileSource,
                         Profile->Affinity);
    try {
        State = KeStartProfile (ProfileObject, LockedVa);
        ASSERT (State != FALSE);

    } except (EXCEPTION_EXECUTE_HANDLER) {

        MmUnlockPages (Profile->Mdl);
        MmUnmapLockedPages (LockedVa, Profile->Mdl);
        ExFreePool (ProfileObject);
        KeReleaseMutex (&ExpProfileStateMutex, FALSE);
        ObDereferenceObject ((PVOID)Profile);
        return GetExceptionCode();
    }

    Profile->LockedBufferAddress = LockedVa;

    KeReleaseMutex (&ExpProfileStateMutex, FALSE);
    ObDereferenceObject ((PVOID)Profile);

    return STATUS_SUCCESS;
}

NTSTATUS
NtStopProfile (
    IN HANDLE ProfileHandle
    )

/*++

Routine Description:

    The NtStopProfile routine stops collecting data for the
    specified profile object.  This involves stopping the data
    collection on the profile object, unlocking the locked buffers,
    and deallocating the pool for the MDL and profile object.

Arguments:

    ProfileHandle - Supplies a the profile handle to stop profiling.

Return Value:

    TBS

--*/

{

    PEPROFILE Profile;
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    BOOLEAN State;

    PreviousMode = KeGetPreviousMode();

    Status = ObReferenceObjectByHandle( ProfileHandle,
                                        PROFILE_CONTROL,
                                        ExProfileObjectType,
                                        PreviousMode,
                                        (PVOID *)&Profile,
                                        NULL);

    if (!NT_SUCCESS(Status)) {
        return Status;
    }

    KeWaitForSingleObject( &ExpProfileStateMutex,
                           Executive,
                           KernelMode,
                           FALSE,
                           (PLARGE_INTEGER)NULL);

    //
    // Check to see if profiling is not active.
    //

    if (Profile->LockedBufferAddress == NULL) {
        KeReleaseMutex (&ExpProfileStateMutex, FALSE);
        ObDereferenceObject ((PVOID)Profile);
        return STATUS_PROFILING_NOT_STARTED;
    }

    //
    // Stop profiling and unlock the buffer.
    //

    State = KeStopProfile (Profile->ProfileObject);
    ASSERT (State != FALSE);

    MmUnmapLockedPages (Profile->LockedBufferAddress, Profile->Mdl);
    MmUnlockPages (Profile->Mdl);
    ExFreePool (Profile->ProfileObject);
    Profile->LockedBufferAddress = NULL;
    KeReleaseMutex (&ExpProfileStateMutex, FALSE);

    ObDereferenceObject ((PVOID)Profile);
    return STATUS_SUCCESS;
}

NTSTATUS
NtSetIntervalProfile (
    IN ULONG Interval,
    IN KPROFILE_SOURCE Source
    )

/*++

Routine Description:

    This routine allows the system-wide interval (and thus the profiling
    rate) for profiling to be set.

Arguments:

    Interval - Supplies the sampling interval in 100ns units.

    Source - Specifies the profile source to be set.

Return Value:

    TBS

--*/

{

    KeSetIntervalProfile (Interval, Source);
    return STATUS_SUCCESS;
}

NTSTATUS
NtQueryIntervalProfile (
    IN KPROFILE_SOURCE ProfileSource,
    OUT PULONG Interval
    )

/*++

Routine Description:

    This routine queries the system-wide interval (and thus the profiling
    rate) for profiling.

Arguments:

    Source - Specifies the profile source to be queried.

    Interval - Returns the sampling interval in 100ns units.

Return Value:

    TBS

--*/

{
    ULONG CapturedInterval;
    KPROCESSOR_MODE PreviousMode;

    PreviousMode = KeGetPreviousMode ();
    if (PreviousMode != KernelMode) {

        //
        // Probe accessibility of user's buffer.
        //

        try {
            ProbeForWriteUlong (Interval);

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // If an exception occurs during the probe or capture
            // of the initial values, then handle the exception and
            // return the exception code as the status value.
            //

            return GetExceptionCode();
        }
    }

    CapturedInterval = KeQueryIntervalProfile (ProfileSource);

    try {
        *Interval = CapturedInterval;

    } except (EXCEPTION_EXECUTE_HANDLER) {
        NOTHING;
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NtQueryPerformanceCounter (
    OUT PLARGE_INTEGER PerformanceCounter,
    OUT PLARGE_INTEGER PerformanceFrequency OPTIONAL
    )

/*++

Routine Description:

    This function returns current value of performance counter and,
    optionally, the frequency of the performance counter.

    Performance frequency is the frequency of the performance counter
    in Hertz, i.e., counts/second.  Note that this value is implementation
    dependent.  If the implementation does not have hardware to support
    performance timing, the value returned is 0.

Arguments:

    PerformanceCounter - supplies the address of a variable to receive
        the current Performance Counter value.

    PerformanceFrequency - Optionally, supplies the address of a
        variable to receive the performance counter frequency.

Return Value:

    STATUS_ACCESS_VIOLATION or STATUS_SUCCESS.

--*/

{
    KPROCESSOR_MODE PreviousMode;
    LARGE_INTEGER KernelPerformanceFrequency;

    PreviousMode = KeGetPreviousMode();
    if (PreviousMode != KernelMode) {

        //
        // Probe accessibility of user's buffer.
        //

        try {
            ProbeForWrite ( PerformanceCounter,
                            sizeof (LARGE_INTEGER),
                            sizeof (ULONG)
                          );

            if (ARGUMENT_PRESENT(PerformanceFrequency)) {
                ProbeForWrite ( PerformanceFrequency,
                                sizeof (LARGE_INTEGER),
                                sizeof (ULONG)
                              );
            }

        } except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // If an exception occurs during the probe or capture
            // of the initial values, then handle the exception and
            // return the exception code as the status value.
            //

            return GetExceptionCode();
        }
    }

    try {
        *PerformanceCounter = KeQueryPerformanceCounter (
                                  (PLARGE_INTEGER)&KernelPerformanceFrequency );
        if (ARGUMENT_PRESENT(PerformanceFrequency)) {
            *PerformanceFrequency = KernelPerformanceFrequency;
        }
    } except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }

    return STATUS_SUCCESS;
}
