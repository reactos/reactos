/*++

Copyright (c) 1994-1997  Microsoft Corporation

Module Name:

    uuid.c

Abstract:

    This module implements the core time and sequence number allocation
    for UUIDs (exposed to user mode), as well as complete UUID
    creation (exposed to kernel mode only).

              (e.g. RPC Runtime)                (e.g. NTFS)
                      |                              |
                      V                              V
                NtAllocateUuids                 ExUuidCreate
                      |                              |
                      V                              V
                      |                         ExpUuidGetValues
                      |                              |
                      |                              |
                      +------> ExpAllocateUuids <----+



Author:

    Mario Goertzel (MarioGo)  22-Nov-1994

Revision History:

    MikeHill    17-Jan-96   Ported ExUuidCreate & ExpUuidGetValues from RPCRT4.
    MazharM     17-Feb-98   Add PNP support

--*/

#include "exp.h"



//
// Well known values
//

// Registry info for the sequen number
#define RPC_SEQUENCE_NUMBER_PATH L"\\Registry\\Machine\\Software\\Microsoft\\Rpc"
#define RPC_SEQUENCE_NUMBER_NAME L"UuidSequenceNumber"

// Masks and constants to interpret the UUID
#define UUID_TIME_HIGH_MASK    0x0FFF
#define UUID_VERSION           0x1000
#define UUID_RESERVED          0x80
#define UUID_CLOCK_SEQ_HI_MASK 0x3F

// Values for ExpUuidCacheValid
#define CACHE_LOCAL_ONLY 0
#define CACHE_VALID      1

//
// Custom types
//

// An alternative data-template for a UUID, useful during generation.
typedef struct _UUID_GENERATE {
    ULONG   TimeLow;
    USHORT  TimeMid;
    USHORT  TimeHiAndVersion;
    UCHAR   ClockSeqHiAndReserved;
    UCHAR   ClockSeqLow;
    UCHAR   NodeId[6];
} UUID_GENERATE;

// A cache of allocated UUIDs
typedef struct _UUID_CACHED_VALUES_STRUCT {
    ULONGLONG           Time;           // End time of allocation
    LONG                AllocatedCount; // Number of UUIDs allocated
    UCHAR               ClockSeqHiAndReserved;
    UCHAR               ClockSeqLow;
    UCHAR               NodeId[6];
} UUID_CACHED_VALUES_STRUCT;


//
//  Global variables
//

// UUID cache information
LARGE_INTEGER               ExpUuidLastTimeAllocated;
BOOLEAN                     ExpUuidCacheValid = CACHE_LOCAL_ONLY;

// Make cache allocate UUIDs on first call.
// Time = 0. Allocated = -1, ..., multicast bit in node id
UUID_CACHED_VALUES_STRUCT   ExpUuidCachedValues = { 0, -1, 0, 0, { 0x80, 'm', 'a', 'r', 'i', 'o' }};

// UUID Sequence number information
ULONG                       ExpUuidSequenceNumber;
BOOLEAN                     ExpUuidSequenceNumberValid;
BOOLEAN                     ExpUuidSequenceNumberNotSaved;

// A lock to protect all of the above global data.
FAST_MUTEX                  ExpUuidLock;

//
// Code section allocations
//

extern NTSTATUS ExpUuidLoadSequenceNumber(
    OUT PULONG
    );

extern NTSTATUS ExpUuidSaveSequenceNumber(
    IN ULONG
    );

extern NTSTATUS ExpUuidSaveSequenceNumberIf ();

extern NTSTATUS ExpUuidGetValues(
    OUT UUID_CACHED_VALUES_STRUCT *Values
    );


#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, ExpUuidLoadSequenceNumber)
#pragma alloc_text(PAGE, ExpUuidSaveSequenceNumber)
#pragma alloc_text(PAGE, ExpUuidSaveSequenceNumberIf)
#pragma alloc_text(INIT, ExpUuidInitialization)
#pragma alloc_text(PAGE, NtAllocateUuids)
#pragma alloc_text(PAGE, NtSetUuidSeed)
#pragma alloc_text(PAGE, ExpUuidGetValues)
#pragma alloc_text(PAGE, ExUuidCreate)
#endif


NTSTATUS
ExpUuidLoadSequenceNumber(
    OUT PULONG Sequence
    )
/*++

Routine Description:

    This function loads the saved sequence number from the registry.
    This function is called only during system startup.

Arguments:

    Sequence - Pointer to storage for the sequence number.

Return Value:

    STATUS_SUCCESS when the sequence number is successfully read from the
        registry.

    STATUS_UNSUCCESSFUL when the sequence number is not correctly stored
        in the registry.

    Failure codes from ZwOpenKey() and ZwQueryValueKey() maybe returned.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyPath, KeyName;
    HANDLE Key;
    CHAR KeyValueBuffer[sizeof(KEY_VALUE_PARTIAL_INFORMATION) + sizeof(ULONG)];
    PKEY_VALUE_PARTIAL_INFORMATION KeyValueInformation;
    ULONG ResultLength;

    PAGED_CODE();

    KeyValueInformation = (PKEY_VALUE_PARTIAL_INFORMATION)KeyValueBuffer;

    RtlInitUnicodeString(&KeyPath, RPC_SEQUENCE_NUMBER_PATH);
    RtlInitUnicodeString(&KeyName, RPC_SEQUENCE_NUMBER_NAME);

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL
                              );

    Status =
    ZwOpenKey( &Key,
               GENERIC_READ,
               &ObjectAttributes
             );

    if (NT_SUCCESS(Status)) {
        Status =
        ZwQueryValueKey( Key,
                         &KeyName,
                         KeyValuePartialInformation,
                         KeyValueInformation,
                         sizeof(KeyValueBuffer),
                         &ResultLength
                       );

        ZwClose( Key );
        }

    if (NT_SUCCESS(Status)) {
        if ( KeyValueInformation->Type == REG_DWORD &&
             KeyValueInformation->DataLength == sizeof(ULONG)
           ) {
            *Sequence = *(PULONG)KeyValueInformation->Data;
            }
        else {
            Status = STATUS_UNSUCCESSFUL;
            }
        }

    return(Status);
}


NTSTATUS
ExpUuidSaveSequenceNumber(
    IN ULONG Sequence
    )
/*++

Routine Description:

    This function saves the uuid sequence number in the registry.  This
    value will be read by ExpUuidLoadSequenceNumber during the next boot.

    This routine assumes that the current thread has exclusive access
    to the the ExpUuid* values.

Arguments:

    Sequence - The sequence number to save.

Return Value:

    STATUS_SUCCESS

    Failure codes from ZwOpenKey() and ZwSetValueKey() maybe returned.

--*/
{
    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyPath, KeyName;
    HANDLE Key;

    PAGED_CODE();

    RtlInitUnicodeString(&KeyPath, RPC_SEQUENCE_NUMBER_PATH);
    RtlInitUnicodeString(&KeyName, RPC_SEQUENCE_NUMBER_NAME);

    InitializeObjectAttributes( &ObjectAttributes,
                                &KeyPath,
                                OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                                NULL,
                                NULL
                              );

    Status =
    ZwOpenKey( &Key,
               GENERIC_READ | GENERIC_WRITE,
               &ObjectAttributes
             );

    if (NT_SUCCESS(Status)) {
        Status =
        ZwSetValueKey( Key,
                       &KeyName,
                       0,
                       REG_DWORD,
                       &Sequence,
                       sizeof(ULONG)
                     );

        ZwClose( Key );
        }

    return(Status);
}



NTSTATUS
ExpUuidSaveSequenceNumberIf ()

/*++

Routine Description:

    This function saves the ExpUuidSequenceNumber, but only
    if necessary (as determined by the ExpUuidSequenceNumberNotSaved
    flag).

    This routine assumes that the current thread has exclusive access
    to the ExpUuid* values.

Arguments:

    None.

Return Value:

    STATUS_SUCCESS if the operation was successful.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    PAGED_CODE();

    // Does the sequence number need to be saved?
    if (ExpUuidSequenceNumberNotSaved == TRUE) {

        // Print this message just to make sure we aren't hitting the
        // registry too much under normal usage.

        KdPrint(("Uuid: Saving new sequence number.\n"));

        // Save the sequence number

        Status = ExpUuidSaveSequenceNumber(ExpUuidSequenceNumber);

        // Indicate that it's now been saved.
        if (NT_SUCCESS(Status)) {
            ExpUuidSequenceNumberNotSaved = FALSE;
            }
        }

    return( Status );
}




BOOLEAN
ExpUuidInitialization (
    VOID
    )
/*++

Routine Description:

    This function initializes the UUID allocation.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the initialization is successfully
    completed.  Otherwise, a value of FALSE is returned.

--*/

{
    NTSTATUS Status;

    PAGED_CODE();

    ExInitializeFastMutex(&ExpUuidLock);

    ExpUuidSequenceNumberValid = FALSE;

    // We can use the current time since we'll be changing the sequence number.

    KeQuerySystemTime(&ExpUuidLastTimeAllocated);

    return TRUE;
}


NTSTATUS
ExpAllocateUuids (
    OUT PLARGE_INTEGER Time,
    OUT PULONG Range,
    OUT PULONG Sequence
    )

/*++

Routine Description:

    Allocates a sequence number and a range of times for a set of UUIDs.
    The caller can use this together with the network address to
    generate complete UUIDs.

    This routine assumes that the current thread has exclusive access
    to the ExpUuid* values.

Arguments:

    Time - Supplies the address of a variable that will receive the
        start time (SYSTEMTIME format) of the range of time reserved.

    Range - Supplies the address of a variable that will receive the
        number of ticks (100ns) reserved after the value in Time.
        The range reserved is *Time to (*Time+*Range-1).

    Sequence - Supplies the address of a variable that will receive
        the time sequence number.  This value is used with the associated
        range of time to prevent problems with clocks going backwards.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_RETRY is returned if we're unable to reserve a range of
        UUIDs.  This will occur if system clock hasn't advanced
        and the allocator is out of cached values.

    STATUS_UNSUCCESSFUL is returned if some other service reports
        an error, most likly the registery.

--*/

{
    NTSTATUS Status;
    LARGE_INTEGER CurrentTime;
    LARGE_INTEGER AvailableTime;

    PAGED_CODE();

    //
    // Make sure we have a valid sequence number.  If not, make one up.
    //

    if (ExpUuidSequenceNumberValid == FALSE) {

        Status = ExpUuidLoadSequenceNumber(&ExpUuidSequenceNumber);

        if (!NT_SUCCESS(Status)) {
            // Unable read the sequence number, this means we should make one up.

            LARGE_INTEGER PerfCounter;
            LARGE_INTEGER PerfFrequency;

            // This should only happen when we're called
            // for the first time on a given machine. (machine, not boot)

            KdPrint(("Uuid: Generating first sequence number.\n"));

            PerfCounter = KeQueryPerformanceCounter(&PerfFrequency);

            ExpUuidSequenceNumber ^= (ULONG)((ULONG_PTR)&Status) ^ PerfCounter.LowPart ^
                PerfCounter.HighPart ^ (ULONG)((ULONG_PTR)Sequence);
            }
        else {
            // We increment the sequence number on every boot.
            ExpUuidSequenceNumber++;
            }

        ExpUuidSequenceNumberValid = TRUE;
        ExpUuidSequenceNumberNotSaved = TRUE;

        }

    //
    // Get the current time, usually we will have plenty of avaliable
    // to give the caller.  But we may need to deal with time going
    // backwards and really fast machines.
    //

    KeQuerySystemTime(&CurrentTime);

    AvailableTime.QuadPart = CurrentTime.QuadPart - ExpUuidLastTimeAllocated.QuadPart;

    if (AvailableTime.QuadPart < 0) {

        // Time has been set time backwards. This means that we must make sure
        // that somebody increments the sequence number and saves the new
        // sequence number in the registry.

        ExpUuidSequenceNumberNotSaved = TRUE;
        ExpUuidSequenceNumber++;

        // The sequence number has been changed, so it's now okay to set time
        // backwards.  Since time is going backwards anyway, it's okay to set
        // it back an extra millisecond or two.

        ExpUuidLastTimeAllocated.QuadPart = CurrentTime.QuadPart - 20000;
        AvailableTime.QuadPart = 20000;
        }

    if (AvailableTime.QuadPart == 0) {
        // System time hasn't moved.  The caller should yield the CPU and retry.
        return(STATUS_RETRY);
        }

    //
    // Common case, time has moved forward.
    //

    if (AvailableTime.QuadPart > 10*1000*1000) {
        // We never want to give out really old (> 1 second) Uuids.
        AvailableTime.QuadPart = 10*1000*1000;
        }

    if (AvailableTime.QuadPart > 10*1000) {
        // We've got over a millisecond to give out.  We'll save some time for
        // another caller so that we can avoid returning STATUS_RETRY very often.
        *Range = 10*1000;
        AvailableTime.QuadPart -= 10*1000;
        }
    else {
        // Not much time avaiable, give it all away.
        *Range = (ULONG)AvailableTime.QuadPart;
        AvailableTime.QuadPart = 0;
        }

    Time->QuadPart = CurrentTime.QuadPart - (*Range + AvailableTime.QuadPart);

    ExpUuidLastTimeAllocated.QuadPart = Time->QuadPart + *Range;

    // Last time allocated is just after the range we hand back to the caller
    // this may be almost a second behind the true system time.

    *Sequence = ExpUuidSequenceNumber;


    return(STATUS_SUCCESS);
}

#define SEED_SIZE 6 * sizeof(CHAR)


NTSTATUS
NtSetUuidSeed (
    IN PCHAR Seed
    )
/*++

Routine Description:

    This routine is used to set the seed used for UUID generation. The seed
    will be set by RPCSS at startup and each time a card is replaced.
    
Arguments:

    Seed - Pointer to a six byte buffer
    
Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_ACCESS_DENIED If caller doesn't have the permissions to make this call. 
    You need to be logged on as Local System in order to call this API.

    STATUS_ACCESS_VIOLATION is returned if the Seed could not be read.
    
--*/
{
    NTSTATUS Status;
    LUID AuthenticationId;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    LUID SystemLuid = SYSTEM_LUID;
    BOOLEAN CapturedSubjectContext = FALSE;
    
    PAGED_CODE();

    ASSERT(KeGetPreviousMode() != KernelMode);

    try {
        //
        // Check if the caller has the appropriate permission
        //
        SeCaptureSubjectContext(&SubjectContext); 
        CapturedSubjectContext = TRUE;

        Status = SeQueryAuthenticationIdToken(
                             SeQuerySubjectContextToken(&SubjectContext),
                             &AuthenticationId);
        if (!NT_SUCCESS(Status)) {
            ExRaiseStatus(Status);
            }
        
        if (RtlCompareMemory(&AuthenticationId, &SystemLuid, sizeof(LUID)) != sizeof(LUID)) {
            ExRaiseStatus(STATUS_ACCESS_DENIED);
            }

        //
        // Store the UUID seed
        //
        ProbeForRead(Seed, SEED_SIZE, sizeof(CHAR));
        RtlCopyMemory(&ExpUuidCachedValues.NodeId[0], Seed, SEED_SIZE);

        if ((Seed[0] & 0x80) == 0)
            {
            // If the high bit is not set the NodeId is a valid IEEE 802
            // address and should be globally unique.
            ExpUuidCacheValid = CACHE_VALID;
            }
        else
            {
            ExpUuidCacheValid = CACHE_LOCAL_ONLY;
            }
        
        Status = STATUS_SUCCESS;
    }
    except (EXCEPTION_EXECUTE_HANDLER) {
        Status = GetExceptionCode();
    }

    if (CapturedSubjectContext) {
        SeReleaseSubjectContext( &SubjectContext );
        }

    return Status;
}


NTSTATUS
NtAllocateUuids (
    OUT PULARGE_INTEGER Time,
    OUT PULONG Range,
    OUT PULONG Sequence,
    OUT PCHAR Seed
    )

/*++

Routine Description:

    This function reserves a range of time for the caller(s) to use for
    handing out Uuids.  As far a possible the same range of time and
    sequence number will never be given out.

    (It's possible to reboot 2^14-1 times and set the clock backwards and then
    call this allocator and get a duplicate.  Since only the low 14bits of the
    sequence number are used in a real uuid.)

Arguments:

    Time - Supplies the address of a variable that will receive the
        start time (SYSTEMTIME format) of the range of time reserved.

    Range - Supplies the address of a variable that will receive the
        number of ticks (100ns) reserved after the value in Time.
        The range reserved is *Time to (*Time + *Range - 1).

    Sequence - Supplies the address of a variable that will receive
        the time sequence number.  This value is used with the associated
        range of time to prevent problems with clocks going backwards.

    Seed - Pointer to a 6 byte buffer. The current seed is written into this buffer.
    
Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_RETRY is returned if we're unable to reserve a range of
        UUIDs.  This may (?) occur if system clock hasn't advanced
        and the allocator is out of cached values.

    STATUS_ACCESS_VIOLATION is returned if the output parameter for the
        UUID cannot be written.

    STATUS_UNSUCCESSFUL is returned if some other service reports
        an error, most likly the registery.

--*/

{

    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;

    LARGE_INTEGER OutputTime;
    ULONG OutputRange;
    ULONG OutputSequence;

    PAGED_CODE();

    //
    // Establish an exception handler and attempt to write the output
    // arguments. If the write attempt fails, then return
    // the exception code as the service status. Otherwise return success
    // as the service status.
    //

    try {

        //
        // Get previous processor mode and probe arguments if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForWrite((PVOID)Time, sizeof(LARGE_INTEGER), sizeof(ULONG));
            ProbeForWrite((PVOID)Range, sizeof(ULONG), sizeof(ULONG));
            ProbeForWrite((PVOID)Sequence, sizeof(ULONG), sizeof(ULONG));
            ProbeForWrite((PVOID)Seed, SEED_SIZE, sizeof(CHAR));
            }
        }
    except (ExSystemExceptionFilter()) {
        return GetExceptionCode();
        }

    // Take the lock, because we're about to update the UUID cache.

    KeEnterCriticalRegion();
    ExAcquireFastMutexUnsafe(&ExpUuidLock);

    // Get the sequence number and a range of times that can
    // be used in UUID-generation.

    Status = ExpAllocateUuids( &OutputTime, &OutputRange, &OutputSequence );

    if( !NT_SUCCESS(Status) ) {
        ExReleaseFastMutexUnsafe(&ExpUuidLock);
        KeLeaveCriticalRegion();
        return( Status );
        }

    // If necessary, save the sequence number.  If there's an error,
    // we'll just leave it marked as dirty, and retry on some future call.

    ExpUuidSaveSequenceNumberIf();

    // Release the lock
    ExReleaseFastMutexUnsafe(&ExpUuidLock);
    KeLeaveCriticalRegion();

    //
    // Attempt to store the result of this call into the output parameters.
    // This is done within an exception handler in case output parameters
    // are now invalid.
    //

    try {
        Time->QuadPart = OutputTime.QuadPart;
        *Range = OutputRange;
        *Sequence = OutputSequence;
        RtlCopyMemory((PVOID) Seed, &ExpUuidCachedValues.NodeId[0], SEED_SIZE);
    }
    except (ExSystemExceptionFilter()) {
        return GetExceptionCode();
        }

    return(STATUS_SUCCESS);
}




NTSTATUS
ExpUuidGetValues(
    OUT UUID_CACHED_VALUES_STRUCT *Values
    )
/*++

Routine Description:

    This routine allocates a block of UUIDs and stores them in
    the caller-provided cached-values structure.

    This routine assumes that the current thread has exclusive
    access to the ExpUuid* values.

    Note that the Time value in this cache is different than the
    Time value returned by NtAllocateUuids (and ExpAllocateUuids).
    As a result, the cache must be interpreted differently in
    order to determine the valid range.  The valid range from
    these two routines is:

        NtAllocateUuids:  [ Time, Time+Range )
        ExpUuidGetValues: ( Values.Time-Values.Range, Values.Time ]

Arguments:

    Values - Set to contain everything needed to allocate a block of uuids.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_RETRY is returned if we're unable to reserve a range of
        UUIDs.  This will occur if system clock hasn't advanced
        and the allocator is out of cached values.

    STATUS_NO_MEMORY is returned if we're unable to reserve a range
        of UUIDs, for some reason other than the clock not advancing.

--*/
{
    NTSTATUS Status;
    LARGE_INTEGER Time;
    ULONG Range;
    ULONG Sequence;

    PAGED_CODE();

    // Allocate a range of times for use in UUIDs.

    Status = ExpAllocateUuids(&Time, &Range, &Sequence);

    if (STATUS_RETRY == Status) {
        return(Status);
        }

    else if (!NT_SUCCESS(Status)) {
        return(STATUS_NO_MEMORY);
        }

    // ExpAllocateUuids keeps time in SYSTEM_TIME format which is 100ns ticks since
    // Jan 1, 1601.  UUIDs use time in 100ns ticks since Oct 15, 1582.

    // 17 Days in Oct + 30 (Nov) + 31 (Dec) + 18 years and 5 leap days.

    Time.QuadPart +=   (ULONGLONG) (1000*1000*10)       // seconds
                     * (ULONGLONG) (60 * 60 * 24)       // days
                     * (ULONGLONG) (17+30+31+365*18+5); // # of days

    ASSERT(Range);

    Values->ClockSeqHiAndReserved =
        UUID_RESERVED | (((UCHAR) (Sequence >> 8))
        & (UCHAR) UUID_CLOCK_SEQ_HI_MASK);

    Values->ClockSeqLow = (UCHAR) (Sequence & 0x00FF);


    // We'll modify the Time value so that it indicates the
    // end of the range rather than the beginning of it.

    // The order of these assignments is important

    Values->Time = Time.QuadPart + (Range - 1);
    Values->AllocatedCount = Range;

    return(STATUS_SUCCESS);
}



NTSTATUS
ExUuidCreate(
    OUT UUID *Uuid
    )

/*++

Routine Description:

    This routine creates a DCE UUID and returns it in the caller's
    buffer.

Arguments:

    Uuid - will receive the UUID.

Return Value:

    STATUS_SUCCESS is returned if the service is successfully executed.

    STATUS_RETRY is returned if we're unable to reserve a range of
        UUIDs.  This will occur if system clock hasn't advanced
        and the allocator is out of cached values.

--*/

{
    NTSTATUS Status = STATUS_SUCCESS;

    UUID_GENERATE  *UuidGen = (UUID_GENERATE *) Uuid;
    ULONGLONG       Time;
    LONG            Delta;

    PAGED_CODE();

    //
    // Get a value from the cache.  If the cache is empty, we'll fill
    // it and retry.  The first time cache will be empty.
    //

    for(;;) {

        // Get the highest value in the cache (though it may not
        // be available).
        Time = ExpUuidCachedValues.Time;

        // Copy the static info into the UUID.  We can't do this later
        // because the clock sequence could be updated by another thread.

        *(PULONG)&UuidGen->ClockSeqHiAndReserved =
            *(PULONG)&ExpUuidCachedValues.ClockSeqHiAndReserved;
        *(PULONG)&UuidGen->NodeId[2] =
            *(PULONG)&ExpUuidCachedValues.NodeId[2];

        // See what we need to subtract from Time to get a valid GUID.
        Delta = InterlockedDecrement(&ExpUuidCachedValues.AllocatedCount);

        if (Time != ExpUuidCachedValues.Time) {

            // If our captured time doesn't match the cache then another
            // thread already took the lock and updated the cache. We'll
            // just loop and try again.
            continue;
            }

        // If the cache hadn't already run dry, we can break out of this retry
        // loop.
        if (Delta >= 0) {
            break;
            }

        //
        // Allocate a new block of Uuids.
        //

        // Take the cache lock
        KeEnterCriticalRegion();
        ExAcquireFastMutexUnsafe(&ExpUuidLock);

        // If the cache has already been updated, try again.
        if (Time != ExpUuidCachedValues.Time) {
            // Release the lock
            ExReleaseFastMutexUnsafe(&ExpUuidLock);
            KeLeaveCriticalRegion();
            continue;
            }

        // Update the cache.
        Status = ExpUuidGetValues( &ExpUuidCachedValues );

        if (Status != STATUS_SUCCESS) {
            // Release the lock
            ExReleaseFastMutexUnsafe(&ExpUuidLock);
            KeLeaveCriticalRegion();
            return(Status);
            }

        // The sequence number may have been dirtied, see if it needs
        // to be saved.  If there's an error, we'll ignore it and
        // retry on a future call.

        ExpUuidSaveSequenceNumberIf();

        // Release the lock
        ExReleaseFastMutexUnsafe(&ExpUuidLock);
        KeLeaveCriticalRegion();

        // Loop
        }

    // Adjust the time to that of the next available UUID.
    Time -= Delta;

    // Finish filling in the UUID.

    UuidGen->TimeLow = (ULONG) Time;
    UuidGen->TimeMid = (USHORT) (Time >> 32);
    UuidGen->TimeHiAndVersion = (USHORT)
        (( (USHORT)(Time >> (32+16))
        & UUID_TIME_HIGH_MASK) | UUID_VERSION);

    ASSERT(Status == STATUS_SUCCESS);

    if (ExpUuidCacheValid == CACHE_LOCAL_ONLY) {
        Status = RPC_NT_UUID_LOCAL_ONLY;
        }

    return(Status);
}
