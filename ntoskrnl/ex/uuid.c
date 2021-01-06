/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/uuid.c
 * PURPOSE:         UUID generator
 * PROGRAMMERS:     Eric Kohl
 *                  Thomas Weidenmueller
 *                  Pierre Schweitzer
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <debug.h>

#define SEED_BUFFER_SIZE 6

/* Number of 100ns ticks per clock tick. To be safe, assume that the clock
   resolution is at least 1000 * 100 * (1/1000000) = 1/10 of a second */
#define TICKS_PER_CLOCK_TICK 1000
#define SECSPERDAY  86400
#define TICKSPERSEC 10000000

/* UUID system time starts at October 15, 1582 */
#define SECS_15_OCT_1582_TO_1601  ((17 + 30 + 31 + 365 * 18 + 5) * SECSPERDAY)
#define TICKS_15_OCT_1582_TO_1601 ((ULONGLONG)SECS_15_OCT_1582_TO_1601 * TICKSPERSEC)

/* 10000 in 100-ns model = 0.1 microsecond */
#define TIME_FRAME 10000

/* GLOBALS ****************************************************************/

FAST_MUTEX ExpUuidLock;
LARGE_INTEGER ExpUuidLastTimeAllocated;
ULONG ExpUuidSequenceNumber = 0;
BOOLEAN ExpUuidSequenceNumberValid;
BOOLEAN ExpUuidSequenceNumberNotSaved = FALSE;
UUID_CACHED_VALUES_STRUCT ExpUuidCachedValues = {0ULL, 0xFFFFFFFF, {{0, 0, {0x80, 0x6E, 0x6F, 0x6E, 0x69, 0x63}}}};
BOOLEAN ExpUuidCacheValid = FALSE;
ULONG ExpLuidIncrement = 1;
LARGE_INTEGER ExpLuid = {{0x3e9, 0x0}};

/* FUNCTIONS ****************************************************************/

/*
 * @implemented
 */
CODE_SEG("INIT")
BOOLEAN
NTAPI
ExpUuidInitialization(VOID)
{
    ExInitializeFastMutex(&ExpUuidLock);

    ExpUuidSequenceNumberValid = FALSE;
    KeQuerySystemTime(&ExpUuidLastTimeAllocated);

    return TRUE;
}


/*
 * @implemented
 */
#define VALUE_BUFFER_SIZE 20
static NTSTATUS
ExpUuidLoadSequenceNumber(PULONG Sequence)
{
    UCHAR ValueBuffer[VALUE_BUFFER_SIZE];
    PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName, ValueName;
    HANDLE KeyHandle;
    ULONG ValueLength;
    NTSTATUS Status;

    PAGED_CODE();

    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\Microsoft\\Rpc");
    RtlInitUnicodeString(&ValueName, L"UuidSequenceNumber");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&KeyHandle, GENERIC_READ, &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwOpenKey() failed (Status %lx)\n", Status);
        return Status;
    }

    ValueInfo = (PKEY_VALUE_PARTIAL_INFORMATION)ValueBuffer;
    Status = ZwQueryValueKey(KeyHandle,
                             &ValueName,
                             KeyValuePartialInformation,
                             ValueBuffer,
                             VALUE_BUFFER_SIZE,
                             &ValueLength);
    ZwClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwQueryValueKey() failed (Status %lx)\n", Status);
        return Status;
    }

    if (ValueInfo->Type != REG_DWORD || ValueInfo->DataLength != sizeof(DWORD))
    {
        return STATUS_UNSUCCESSFUL;
    }

    *Sequence = *((PULONG)ValueInfo->Data);

    DPRINT("Loaded sequence %lx\n", *Sequence);

    return STATUS_SUCCESS;
}
#undef VALUE_BUFFER_SIZE

/*
 * @implemented
 */
static NTSTATUS
ExpUuidSaveSequenceNumber(PULONG Sequence)
{
    OBJECT_ATTRIBUTES ObjectAttributes;
    UNICODE_STRING KeyName, ValueName;
    HANDLE KeyHandle;
    NTSTATUS Status;

    PAGED_CODE();

    RtlInitUnicodeString(&KeyName, L"\\Registry\\Machine\\Software\\Microsoft\\Rpc");
    RtlInitUnicodeString(&ValueName, L"UuidSequenceNumber");

    InitializeObjectAttributes(&ObjectAttributes,
                               &KeyName,
                               OBJ_CASE_INSENSITIVE | OBJ_KERNEL_HANDLE,
                               NULL,
                               NULL);
    Status = ZwOpenKey(&KeyHandle,
                       GENERIC_READ | GENERIC_WRITE,
                       &ObjectAttributes);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwOpenKey() failed (Status %lx)\n", Status);
        return Status;
    }

    Status = ZwSetValueKey(KeyHandle,
                           &ValueName,
                           0,
                           REG_DWORD,
                           Sequence,
                           sizeof(ULONG));
    ZwClose(KeyHandle);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("ZwSetValueKey() failed (Status %lx)\n", Status);
    }

    return Status;
}

/*
 * @implemented
 * Warning! This function must be called
 * with ExpUuidLock held!
 */
static VOID
ExpUuidSaveSequenceNumberIf(VOID)
{
    NTSTATUS Status;

    PAGED_CODE();

    /* Only save sequence if it has to */
    if (ExpUuidSequenceNumberNotSaved == TRUE)
    {
        Status = ExpUuidSaveSequenceNumber(&ExpUuidSequenceNumber);
        if (NT_SUCCESS(Status))
        {
            ExpUuidSequenceNumberNotSaved = FALSE;
        }
    }
}

/*
 * @implemented
 * Warning! This function must be called
 * with ExpUuidLock held!
 */
static NTSTATUS
ExpAllocateUuids(PULARGE_INTEGER Time,
                 PULONG Range,
                 PULONG Sequence)
{
    NTSTATUS Status;
    LARGE_INTEGER Counter, Frequency, CurrentTime, TimeDiff, ClockDiff;

    PAGED_CODE();

    /* Initialize sequence number */
    if (!ExpUuidSequenceNumberValid)
    {
        /* Try to load sequence number */
        Status = ExpUuidLoadSequenceNumber(&ExpUuidSequenceNumber);
        if (NT_SUCCESS(Status))
        {
            ++ExpUuidSequenceNumber;
        }
        else
        {
            /* If we cannot, generate a "true" random */
            Counter = KeQueryPerformanceCounter(&Frequency);
            ExpUuidSequenceNumber ^= (ULONG_PTR)&Status ^ (ULONG_PTR)Sequence ^ Counter.LowPart ^ Counter.HighPart;
        }

        /* It's valid and to be saved */
        ExpUuidSequenceNumberValid = TRUE;
        ExpUuidSequenceNumberNotSaved = TRUE;
    }

    KeQuerySystemTime(&CurrentTime);
    TimeDiff.QuadPart = CurrentTime.QuadPart - ExpUuidLastTimeAllocated.QuadPart;
    /* If time went backwards, change sequence (see RFC example) */
    if (TimeDiff.QuadPart < 0)
    {
        ++ExpUuidSequenceNumber;
        TimeDiff.QuadPart = 2 * TIME_FRAME;

        /* It's to be saved */
        ExpUuidSequenceNumberNotSaved = TRUE;
        ExpUuidLastTimeAllocated.QuadPart = CurrentTime.QuadPart - 2 * TIME_FRAME;
    }

    if (TimeDiff.QuadPart == 0)
    {
        return STATUS_RETRY;
    }

    /* If time diff > 0.1ms, squash it to reduce it to keep our clock resolution */
    if (TimeDiff.HighPart > 0 || TimeDiff.QuadPart > TICKS_PER_CLOCK_TICK * TIME_FRAME)
    {
        TimeDiff.QuadPart = TICKS_PER_CLOCK_TICK * TIME_FRAME;
    }

    if (TimeDiff.HighPart < 0 || TimeDiff.QuadPart <= TIME_FRAME)
    {
        *Range = TimeDiff.QuadPart;
        ClockDiff.QuadPart = 0LL;
    }
    else
    {
        *Range = TIME_FRAME;
        ClockDiff.QuadPart = TimeDiff.QuadPart - TIME_FRAME;
        --ClockDiff.HighPart;
    }

    Time->QuadPart = CurrentTime.QuadPart - *Range - ClockDiff.QuadPart;
    ExpUuidLastTimeAllocated.QuadPart = CurrentTime.QuadPart - ClockDiff.QuadPart;
    *Sequence = ExpUuidSequenceNumber;

    return STATUS_SUCCESS;
}

/*
 * @implemented
 * Warning! This function must be called
 * with ExpUuidLock held!
 */
static NTSTATUS
ExpUuidGetValues(PUUID_CACHED_VALUES_STRUCT CachedValues)
{
    NTSTATUS Status;
    ULARGE_INTEGER Time;
    ULONG Range;
    ULONG Sequence;

    PAGED_CODE();

    /* Allocate UUIDs */
    Status = ExpAllocateUuids(&Time, &Range, &Sequence);
    if (Status == STATUS_RETRY)
    {
        return Status;
    }

    if (!NT_SUCCESS(Status))
    {
        return STATUS_NO_MEMORY;
    }

    /* We need at least one UUID */
    ASSERT(Range != 0);

    /* Set up our internal cache
     * See format_uuid_v1 in RFC4122 for magic values
     */
    CachedValues->ClockSeqLow = Sequence;
    CachedValues->ClockSeqHiAndReserved = (Sequence & 0x3F00) >> 8;
    CachedValues->ClockSeqHiAndReserved |= 0x80;
    CachedValues->AllocatedCount = Range;

    /*
     * Time is relative to UUID time
     * And we set last time range for all the possibly
     * returnable UUID
     */
    Time.QuadPart += TICKS_15_OCT_1582_TO_1601;
    CachedValues->Time = Time.QuadPart + (Range - 1);

    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
CODE_SEG("INIT")
BOOLEAN
NTAPI
ExLuidInitialization(VOID)
{
    return TRUE;
}

/*
 * @implemented
 */
VOID
NTAPI
ExAllocateLocallyUniqueId(OUT LUID *LocallyUniqueId)
{
    /* Atomically increment the luid */
    *(LONG64*)LocallyUniqueId = InterlockedExchangeAdd64(&ExpLuid.QuadPart,
                                                         ExpLuidIncrement);
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
NtAllocateLocallyUniqueId(OUT LUID *LocallyUniqueId)
{
    KPROCESSOR_MODE PreviousMode;
    PAGED_CODE();

    /* Probe if user mode */
    PreviousMode = ExGetPreviousMode();
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWrite(LocallyUniqueId,
                          sizeof(LUID),
                          sizeof(ULONG));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* Do the allocation */
    ExAllocateLocallyUniqueId(LocallyUniqueId);
    return STATUS_SUCCESS;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
ExUuidCreate(OUT UUID *Uuid)
{
    NTSTATUS Status;
    LONG AllocatedCount;
    LARGE_INTEGER Time;
    BOOLEAN Valid;

    PAGED_CODE();

    Status = STATUS_SUCCESS;
    /* Loop until we have an UUID to return */
    while (TRUE)
    {
        /* Try to gather node values */
        do
        {
            Time.QuadPart = ExpUuidCachedValues.Time;

            RtlCopyMemory(Uuid->Data4,
                          ExpUuidCachedValues.GuidInit,
                          sizeof(Uuid->Data4));

            Valid = ExpUuidCacheValid;
            AllocatedCount = InterlockedDecrement(&ExpUuidCachedValues.AllocatedCount);
        }
        /* Loop till we can do it without being disturbed */
        while (Time.QuadPart != ExpUuidCachedValues.Time);

        /* We have more than an allocated UUID left, that's OK to return! */
        if (AllocatedCount >= 0)
        {
            break;
        }

        /*
         * Here, we're out of UUIDs, we need to allocate more
         * We need to be alone to do it, so lock the mutex
         */
        ExAcquireFastMutex(&ExpUuidLock);
        if (Time.QuadPart == ExpUuidCachedValues.Time)
        {
            /* If allocation fails, bail out! */
            Status = ExpUuidGetValues(&ExpUuidCachedValues);
            if (Status != STATUS_SUCCESS)
            {
                ExReleaseFastMutex(&ExpUuidLock);
                return Status;
            }

            /* Save our current sequence if changed */
            ExpUuidSaveSequenceNumberIf();
        }
        ExReleaseFastMutex(&ExpUuidLock);
    }

    /*
     * Once here, we've got an UUID to return
     * But, if our init wasn't sane, then, make
     * sure it's only used locally
     */
    if (!Valid)
    {
        Status = RPC_NT_UUID_LOCAL_ONLY;
    }

    /* Set our timestamp - see RFC4211 */
    Time.QuadPart -= AllocatedCount;
    Uuid->Data1 = Time.LowPart;
    Uuid->Data2 = Time.HighPart;
    /* We also set the bit for GUIDv1 */
    Uuid->Data3 = ((Time.HighPart >> 16) & 0x0FFF) | 0x1000;

    return Status;
}

/*
 * @implemented
 */
NTSTATUS
NTAPI
NtAllocateUuids(OUT PULARGE_INTEGER Time,
                OUT PULONG Range,
                OUT PULONG Sequence,
                OUT PUCHAR Seed)
{
    ULARGE_INTEGER IntTime;
    ULONG IntRange, IntSequence;
    NTSTATUS Status;
    KPROCESSOR_MODE PreviousMode;

    PAGED_CODE();

    /* Probe if user mode */
    PreviousMode = ExGetPreviousMode();
    if (PreviousMode != KernelMode)
    {
        _SEH2_TRY
        {
            ProbeForWrite(Time,
                          sizeof(ULARGE_INTEGER),
                          sizeof(ULONG));

            ProbeForWrite(Range,
                          sizeof(ULONG),
                          sizeof(ULONG));

            ProbeForWrite(Sequence,
                          sizeof(ULONG),
                          sizeof(ULONG));

            ProbeForWrite(Seed,
                          SEED_BUFFER_SIZE,
                          sizeof(UCHAR));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            _SEH2_YIELD(return _SEH2_GetExceptionCode());
        }
        _SEH2_END;
    }

    /* During allocation we must be alone */
    ExAcquireFastMutex(&ExpUuidLock);

    Status = ExpAllocateUuids(&IntTime,
                              &IntRange,
                              &IntSequence);
    if (!NT_SUCCESS(Status))
    {
        ExReleaseFastMutex(&ExpUuidLock);
        return Status;
    }

    /* If sequence number was changed, save it */
    ExpUuidSaveSequenceNumberIf();

    /* Allocation done, so we can release */
    ExReleaseFastMutex(&ExpUuidLock);

    /* Write back UUIDs to caller */
    _SEH2_TRY
    {
        Time->QuadPart = IntTime.QuadPart;
        *Range = IntRange;
        *Sequence = IntSequence;

        RtlCopyMemory(Seed,
                      &ExpUuidCachedValues.NodeId[0],
                      SEED_BUFFER_SIZE);

        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(ExSystemExceptionFilter())
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    return Status;
}


/*
 * @implemented
 */
NTSTATUS
NTAPI
NtSetUuidSeed(IN PUCHAR Seed)
{
    NTSTATUS Status;
    BOOLEAN GotContext;
    PACCESS_TOKEN Token;
    SECURITY_SUBJECT_CONTEXT SubjectContext;
    LUID CallerLuid, SystemLuid = SYSTEM_LUID;

    PAGED_CODE();

    /* Should only be done by umode */
    ASSERT(KeGetPreviousMode() != KernelMode);

    /* No context to release */
    GotContext = FALSE;
    _SEH2_TRY
    {
        /* Get our caller context and remember to release it */
        SeCaptureSubjectContext(&SubjectContext);
        GotContext = TRUE;

        /* Get caller access token and its associated ID */
        Token = SeQuerySubjectContextToken(&SubjectContext);
        Status = SeQueryAuthenticationIdToken(Token, &CallerLuid);
        if (!NT_SUCCESS(Status))
        {
            RtlRaiseStatus(Status);
        }

        /* This call is only allowed for SYSTEM */
        if (!RtlEqualLuid(&CallerLuid, &SystemLuid))
        {
            RtlRaiseStatus(STATUS_ACCESS_DENIED);
        }

        /* Check for buffer validity and then copy it to our seed */
        ProbeForRead(Seed, SEED_BUFFER_SIZE, sizeof(UCHAR));
        RtlCopyMemory(&ExpUuidCachedValues.NodeId[0], Seed, SEED_BUFFER_SIZE);

        /*
         * According to RFC 4122, UUID seed is based on MAC addresses
         * If it is randomly set, then, it must have its multicast be set
         * to be valid and avoid collisions
         * Reflect it here
         */
        ExpUuidCacheValid = ~(*Seed >> 7) & 1;

        Status = STATUS_SUCCESS;
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        Status = _SEH2_GetExceptionCode();
    }
    _SEH2_END;

    /* Release context if required */
    if (GotContext)
    {
        SeReleaseSubjectContext(&SubjectContext);
    }

    return Status;
}

/* EOF */
