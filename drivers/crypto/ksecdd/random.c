/*
 * PROJECT:     ReactOS Kernel Security Support Provider Interface Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Random number generation and entropy gathering
 * COPYRIGHT:   Copyright 2014-2017 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "ksecdd.h"

#if defined(_M_IX86) || defined(_M_AMD64)
#include <immintrin.h>
#endif

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

static AES_KEY KsecRngAesKey;
static ULONG64 KsecRngCounter;


/* FUNCTIONS ******************************************************************/

/* New key = SHA1(1 || E || OldKey) || SHA1(2 || E || OldKey), truncated to 32 bytes */
static
VOID
KsecRngDeriveKey(
    _Out_writes_bytes_(32) PUCHAR KeyData)
{
    KSEC_ENTROPY_DATA EntropyData;
    SHA_CTX ShaContext;
    ULONG Digest[2][5];
    UCHAR Label;

    /* Partial data on failure is still usable entropy */
    KsecGatherEntropyData(&EntropyData);

    for (Label = 1; Label <= 2; Label++)
    {
        A_SHAInit(&ShaContext);
        A_SHAUpdate(&ShaContext, &Label, sizeof(Label));
        A_SHAUpdate(&ShaContext, (PUCHAR)&EntropyData, sizeof(EntropyData));
        A_SHAUpdate(&ShaContext, (PUCHAR)&KsecRngAesKey, sizeof(KsecRngAesKey));
        A_SHAFinal(&ShaContext, Digest[Label - 1]);
    }

    RtlCopyMemory(KeyData, Digest, 32);

    RtlSecureZeroMemory(&EntropyData, sizeof(EntropyData));
    RtlSecureZeroMemory(&ShaContext, sizeof(ShaContext));
    RtlSecureZeroMemory(Digest, sizeof(Digest));
}

/* AES-256-CTR. Counter block = { ThreadId, KsecRngCounter }, key re-derived per call */
static
VOID
KsecRngAesCtrFill(
    _Out_writes_bytes_(Length) PUCHAR Output,
    _In_ SIZE_T Length)
{
    UCHAR KeyData[32];
    ULONG64 CounterBlock[2];
    UCHAR KeyStream[16];
    SIZE_T Chunk;

    KsecRngDeriveKey(KeyData);
    aes_setup(KeyData, sizeof(KeyData), 0, &KsecRngAesKey);
    CounterBlock[0] = HandleToUlong(PsGetCurrentThreadId());

    while (Length > 0)
    {
        CounterBlock[1] = KsecRngCounter++;
        aes_ecb_encrypt((PUCHAR)CounterBlock, KeyStream, &KsecRngAesKey);

        Chunk = min(Length, sizeof(KeyStream));
        RtlCopyMemory(Output, KeyStream, Chunk);
        Output += Chunk;
        Length -= Chunk;
    }

    RtlSecureZeroMemory(KeyData, sizeof(KeyData));
    RtlSecureZeroMemory(KeyStream, sizeof(KeyStream));
}

#if defined(_M_IX86) || defined(_M_AMD64)
/* Returns the number of bytes filled. Gives up after three failures. */
static
SIZE_T
KsecRngRdRandFill(
    _Out_writes_bytes_(Length) PUCHAR Output,
    _In_ SIZE_T Length)
{
    unsigned int Value;
    ULONG Failures = 0;
    SIZE_T Done = 0;
    SIZE_T Chunk;

    if (!ExIsProcessorFeaturePresent(PF_RDRAND_INSTRUCTION_AVAILABLE))
        return 0;

    while (Done < Length)
    {
        /* Some AMD CPUs return all ones with CF set; count it as a failure */
        if (!_rdrand32_step(&Value) || (Value == 0xFFFFFFFF))
        {
            if (++Failures == 3)
                break;
            continue;
        }

        Chunk = min(Length - Done, sizeof(Value));
        RtlCopyMemory(Output + Done, &Value, Chunk);
        Done += Chunk;
    }

    RtlSecureZeroMemory(&Value, sizeof(Value));
    return Done;
}
#endif

/* RDRAND if available, otherwise (or after three RDRAND failures) AES-256-CTR */
NTSTATUS
NTAPI
KsecGenRandom(
    PVOID Buffer,
    SIZE_T Length)
{
    PUCHAR Output = Buffer;
    SIZE_T Done = 0;

    if (Length == 0)
        return STATUS_SUCCESS;

#if defined(_M_IX86) || defined(_M_AMD64)
    Done = KsecRngRdRandFill(Output, Length);
#endif

    if (Done < Length)
        KsecRngAesCtrFill(Output + Done, Length - Done);

    return STATUS_SUCCESS;
}

VOID
NTAPI
KsecReadMachineSpecificCounters(
    _Out_ PKSEC_MACHINE_SPECIFIC_COUNTERS MachineSpecificCounters)
{
#if defined(_M_IX86) || defined(_M_AMD64)
    /* Check if RDTSC is available */
    if (ExIsProcessorFeaturePresent(PF_RDTSC_INSTRUCTION_AVAILABLE))
    {
        /* Read the TSC value */
        MachineSpecificCounters->Tsc = __rdtsc();
    }
#if 0 // FIXME: investigate what the requirements are for these
    /* Read the CPU event counter MSRs */
    //MachineSpecificCounters->Ctr0 = __readmsr(0x12);
    //MachineSpecificCounters->Ctr1 = __readmsr(0x13);

    /* Check if this is an MMX capable CPU */
    if (ExIsProcessorFeaturePresent(PF_MMX_INSTRUCTIONS_AVAILABLE))
    {
        /* Read the CPU performance counters 0 and 1 */
        MachineSpecificCounters->Pmc0 = __readpmc(0);
        MachineSpecificCounters->Pmc1 = __readpmc(1);
    }
#endif
#elif defined(_M_ARM)
    /* Read the Cycle Counter Register */
    MachineSpecificCounters->Ccr = _MoveFromCoprocessor(CP15_PMCCNTR);
#elif defined(_M_ARM64)
    /* Read the virtual count of the generic timer */
#ifdef _MSC_VER
    MachineSpecificCounters->Cntvct = _ReadStatusReg(ARM64_CNTVCT);
#else
    __asm__ __volatile__("mrs %0, cntvct_el0" : "=r"(MachineSpecificCounters->Cntvct));
#endif
#else
    #error Implement me!
#endif
}

/*!
 *  \see http://blogs.msdn.com/b/michael_howard/archive/2005/01/14/353379.aspx (DEAD_LINK)
 */
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

    /* Query some generic values */
    EntropyData->CurrentProcessId = PsGetCurrentProcessId();
    EntropyData->CurrentThreadId = PsGetCurrentThreadId();
    KeQueryTickCount(&EntropyData->TickCount);
    KeQuerySystemTime(&EntropyData->SystemTime);
    EntropyData->PerformanceCounter = KeQueryPerformanceCounter(
                                            &EntropyData->PerformanceFrequency);

    /* Check if we have a TEB/PEB for the process environment */
    Teb = PsGetCurrentThread()->Tcb.Teb;
    if (Teb != NULL)
    {
        Peb = Teb->ProcessEnvironmentBlock;

        /* Initialize the MD4 context */
        MD4Init(&Md4Context);
        _SEH2_TRY
        {
            /* Get the end of the environment */
            String = Peb->ProcessParameters->Environment;
            while (*String)
            {
                String += wcslen(String) + 1;
            }

            /* Update the MD4 context from the environment data */
            MD4Update(&Md4Context,
                      (PUCHAR)Peb->ProcessParameters->Environment,
                      (ULONG)((PUCHAR)String - (PUCHAR)Peb->ProcessParameters->Environment));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Simply ignore the exception */
        }
        _SEH2_END;

        /* Finalize and copy the MD4 hash */
        MD4Final(&Md4Context);
        RtlCopyMemory(&EntropyData->EnvironmentHash, Md4Context.digest, 16);
        RtlSecureZeroMemory(&Md4Context, sizeof(Md4Context));
    }

    /* Read some machine specific hardware counters */
    KsecReadMachineSpecificCounters(&EntropyData->MachineSpecificCounters);

    /* Query processor performance information */
    Status = ZwQuerySystemInformation(SystemProcessorPerformanceInformation,
                                      &EntropyData->SystemProcessorPerformanceInformation,
                                      sizeof(SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION),
                                      &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Query system performance information */
    Status = ZwQuerySystemInformation(SystemPerformanceInformation,
                                      &EntropyData->SystemPerformanceInformation,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Query exception information */
    Status = ZwQuerySystemInformation(SystemExceptionInformation,
                                      &EntropyData->SystemExceptionInformation,
                                      sizeof(SYSTEM_EXCEPTION_INFORMATION),
                                      &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Query lookaside information */
    Status = ZwQuerySystemInformation(SystemLookasideInformation,
                                      &EntropyData->SystemLookasideInformation,
                                      sizeof(SYSTEM_LOOKASIDE_INFORMATION),
                                      &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Query interrupt information */
    Status = ZwQuerySystemInformation(SystemInterruptInformation,
                                      &EntropyData->SystemInterruptInformation,
                                      sizeof(SYSTEM_INTERRUPT_INFORMATION),
                                      &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    /* Query process information */
    Status = ZwQuerySystemInformation(SystemProcessInformation,
                                      &EntropyData->SystemProcessInformation,
                                      sizeof(SYSTEM_PROCESS_INFORMATION),
                                      &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    return STATUS_SUCCESS;
}
