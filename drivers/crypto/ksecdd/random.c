/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Drivers
 * PURPOSE:         Kernel Security Support Provider Interface Driver
 *
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include "ksecdd.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

static struct
{
    ULONGLONG Key[2];      /* 128-bit key */
    ULONGLONG Counter[2];  /* 128-bit counter (CTR nonce + counter) */
    BOOLEAN Initialized;
} KsecSpeckCtx;

/* FUNCTIONS ******************************************************************/

#define SPECK_ROUNDS 32

static __forceinline VOID
SpeckEncryptBlock(_Inout_ ULONGLONG *X, _Inout_ ULONGLONG *Y,
                  _In_reads_(2) const ULONGLONG *Key)
{
    ULONGLONG k0 = Key[0];
    ULONGLONG k1 = Key[1];
    ULONG i;

    for (i = 0; i < SPECK_ROUNDS; ++i)
    {
        *X = (_rotr64(*X, 8) + *Y) ^ k0;
        *Y = _rotl64(*Y, 3) ^ *X;

        /* simple key schedule (inline) */
        k1 = (_rotr64(k1, 8) + k0) ^ i;
        k0 = _rotl64(k0, 3) ^ k1;
    }
}

static __forceinline VOID
KsecIncrementCounter(_Inout_ ULONGLONG Ctr[2])
{
    if (++Ctr[0] == 0)
    {
        ++Ctr[1];
    }
}

NTSTATUS
NTAPI
KsecGenRandom(
    PVOID Buffer,
    SIZE_T Length)
{
    UCHAR Block[16];
    PUCHAR Out = (PUCHAR)Buffer;
    SIZE_T Offset = 0;
    ULONGLONG X, Y;

    /* Ensure cipher is initialized */
    if (!KsecSpeckCtx.Initialized)
        return STATUS_NOT_READY;

    while (Offset < Length)
    {
        /* Encrypt current counter to produce keystream block */
        X = KsecSpeckCtx.Counter[0];
        Y = KsecSpeckCtx.Counter[1];

        SpeckEncryptBlock(&X, &Y, KsecSpeckCtx.Key);

        RtlCopyMemory(Block, &X, sizeof(ULONGLONG));
        RtlCopyMemory(Block + sizeof(ULONGLONG), &Y, sizeof(ULONGLONG));

        /* Increment CTR for next block */
        KsecIncrementCounter(KsecSpeckCtx.Counter);

        /* Copy as much as needed */
        SIZE_T ToCopy = min(16, Length - Offset);
        RtlCopyMemory(Out + Offset, Block, ToCopy);

        Offset += ToCopy;
    }

    return STATUS_SUCCESS;
}
VOID
NTSTATUS
NTAPI
KsecGatherEntropyData(
    PKSEC_ENTROPY_DATA EntropyData)
{
    MD4_CTX Md4Context;
    PTEB Teb;
    PPEB Peb;
    PWSTR String;
    ULONG ReturnLength;
    NTSTATUS Status;
    UCHAR Digest[16];

    /* Collect entropy sources (unchanged) */
    EntropyData->CurrentProcessId = PsGetCurrentProcessId();
    EntropyData->CurrentThreadId = PsGetCurrentThreadId();
    KeQueryTickCount(&EntropyData->TickCount);
    KeQuerySystemTime(&EntropyData->SystemTime);
    EntropyData->PerformanceCounter =
        KeQueryPerformanceCounter(&EntropyData->PerformanceFrequency);

    Teb = PsGetCurrentThread()->Tcb.Teb;
    if (Teb != NULL)
    {
        Peb = Teb->ProcessEnvironmentBlock;

        MD4Init(&Md4Context);
        _SEH2_TRY
        {
            String = Peb->ProcessParameters->Environment;
            while (*String)
            {
                String += wcslen(String) + 1;
            }

            MD4Update(&Md4Context,
                      (PUCHAR)Peb->ProcessParameters->Environment,
                      (ULONG)((PUCHAR)String -
                              (PUCHAR)Peb->ProcessParameters->Environment));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
        }
        _SEH2_END;

        MD4Final(&Md4Context);
        RtlCopyMemory(&EntropyData->EnvironmentHash,
                      Md4Context.digest, 16);
    }

    /* Hardware counters */
    KsecReadMachineSpecificCounters(
        &EntropyData->MachineSpecificCounters);

    /* System info queries (unchanged) */
    Status = ZwQuerySystemInformation(
        SystemProcessorPerformanceInformation,
        &EntropyData->SystemProcessorPerformanceInformation,
        sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION),
        &ReturnLength);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ZwQuerySystemInformation(
        SystemPerformanceInformation,
        &EntropyData->SystemPerformanceInformation,
        sizeof(SYSTEM_PERFORMANCE_INFORMATION),
        &ReturnLength);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ZwQuerySystemInformation(
        SystemExceptionInformation,
        &EntropyData->SystemExceptionInformation,
        sizeof(SYSTEM_EXCEPTION_INFORMATION),
        &ReturnLength);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ZwQuerySystemInformation(
        SystemLookasideInformation,
        &EntropyData->SystemLookasideInformation,
        sizeof(SYSTEM_LOOKASIDE_INFORMATION),
        &ReturnLength);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ZwQuerySystemInformation(
        SystemInterruptInformation,
        &EntropyData->SystemInterruptInformation,
        sizeof(SYSTEM_INTERRUPT_INFORMATION),
        &ReturnLength);
    if (!NT_SUCCESS(Status)) return Status;

    Status = ZwQuerySystemInformation(
        SystemProcessInformation,
        &EntropyData->SystemProcessInformation,
        sizeof(SYSTEM_PROCESS_INFORMATION),
        &ReturnLength);
    if (!NT_SUCCESS(Status)) return Status;


    MD4Init(&Md4Context);
    MD4Update(&Md4Context,
              (PUCHAR)EntropyData,
              sizeof(KSEC_ENTROPY_DATA));
    MD4Final(&Md4Context);

    RtlCopyMemory(Digest, Md4Context.digest, 16);

    /* Seed key (128-bit) */
    RtlCopyMemory(KsecSpeckCtx.Key, Digest, 16);

    /* Seed counter using timing + hash mix */
    KsecSpeckCtx.Counter[0] =
        EntropyData->PerformanceCounter.QuadPart ^
        *(ULONGLONG*)&Digest[0];

    KsecSpeckCtx.Counter[1] =
        EntropyData->TickCount.QuadPart ^
        *(ULONGLONG*)&Digest[8];

    KsecSpeckCtx.Initialized = TRUE;

    return STATUS_SUCCESS;
}
