/*
 * PROJECT:     ReactOS Kernel Security Support Provider Interface Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Random number generation and entropy gathering
 * COPYRIGHT:   Copyright 2025 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include "ksecdd.h"
#include <sha2.h>
#include <aes-ctr.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

static AES_CTR_CTX KsecRngContext;

/* FUNCTIONS ******************************************************************/

VOID
NTAPI
KsecInitializeGenRandomSupport(
    VOID)
{
    KSEC_ENTROPY_DATA EntropyData;

    /* Gather entropy data */
    KsecGatherEntropyData(&EntropyData);

    /* Initialize the AES-CTR DRBG with the gathered entropy */
    AES_CTR_Init(&KsecRngContext, EntropyData.Hash, sizeof(EntropyData.Hash));

    /* Erase the temp data */
    RtlSecureZeroMemory(&EntropyData, sizeof(EntropyData));
}

NTSTATUS
NTAPI
KsecGenRandom(
    _Out_writes_bytes_(Length) PVOID Buffer,
    _In_ SIZE_T Length)
{
    AES_CTR_GenRandomBytes(&KsecRngContext, Buffer, Length);
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
    KSEC_MACHINE_SPECIFIC_COUNTERS MachineSpecificCounters;
    ULONG BufferSize = 4096;
    PVOID Buffer;
    HANDLE HandleValue;
    LARGE_INTEGER LargeIntegerValue;
    ULONG64 Ulong64Value;
    SHA512_CTX Sha512Context;
    PTEB Teb;
    PPEB Peb;
    PWSTR String;
    ULONG ReturnLength;
    NTSTATUS Status;

    /* Initialize the SHA512 hash */
    SHA512_Init(&Sha512Context);

    /* Hash some generic values */
    HandleValue = PsGetCurrentProcessId();
    SHA512_Update(&Sha512Context, (PUCHAR)&HandleValue, sizeof(HandleValue));
    HandleValue = PsGetCurrentThreadId();
    SHA512_Update(&Sha512Context, (PUCHAR)&HandleValue, sizeof(HandleValue));
    KeQueryTickCount(&LargeIntegerValue);
    SHA512_Update(&Sha512Context, (PUCHAR)&Ulong64Value, sizeof(Ulong64Value));
    KeQuerySystemTime(&LargeIntegerValue);
    SHA512_Update(&Sha512Context, (PUCHAR)&LargeIntegerValue, sizeof(LargeIntegerValue));
    LargeIntegerValue = KeQueryPerformanceCounter(NULL);
    SHA512_Update(&Sha512Context, (PUCHAR)&LargeIntegerValue, sizeof(LargeIntegerValue));

    /* Check if we have a TEB/PEB for the process environment */
    Teb = PsGetCurrentThread()->Tcb.Teb;
    if (Teb != NULL)
    {
        _SEH2_TRY
        {
            /* Get the end of the environment */
            Peb = Teb->ProcessEnvironmentBlock;
            String = Peb->ProcessParameters->Environment;
            while (*String)
            {
                String += wcslen(String) + 1;
            }

            /* Update the SHA512 context from the environment data */
            SHA512_Update(&Sha512Context,
                          (PUCHAR)Peb->ProcessParameters->Environment,
                          (ULONG)((PUCHAR)String - (PUCHAR)Peb->ProcessParameters->Environment));
        }
        _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
        {
            /* Simply ignore the exception */
        }
        _SEH2_END;
    }

    /* Read some machine specific hardware counters */
    KsecReadMachineSpecificCounters(&MachineSpecificCounters);
    SHA512_Update(&Sha512Context, (PUCHAR)&MachineSpecificCounters, sizeof(MachineSpecificCounters));

    /* Allocate a buffer for system information */
    Buffer = ExAllocatePoolWithTag(PagedPool, BufferSize, 'cesK');
    if (Buffer == NULL)
    {
        DPRINT1("Failed to allocate buffer for system information\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Query processor performance information (one struct per processor) */
    Status = ZwQuerySystemInformation(SystemProcessorPerformanceInformation,
                                      Buffer,
                                      BufferSize,
                                      &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        if (Status != STATUS_INFO_LENGTH_MISMATCH)
        {
            DPRINT1("Failed to query system processor performance information: 0x%lx\n", Status);
            goto Exit;
        }

        ReturnLength = BufferSize;
    }

    SHA512_Update(&Sha512Context, Buffer, ReturnLength);

    /* Query system performance information */
    Status = ZwQuerySystemInformation(SystemPerformanceInformation,
                                      Buffer,
                                      sizeof(SYSTEM_PERFORMANCE_INFORMATION),
                                      &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to query system performance information: 0x%lx\n", Status);
        goto Exit;
    }

    SHA512_Update(&Sha512Context, Buffer, sizeof(SYSTEM_PERFORMANCE_INFORMATION));

    /* Query exception information */
    Status = ZwQuerySystemInformation(SystemExceptionInformation,
                                      Buffer,
                                      sizeof(SYSTEM_EXCEPTION_INFORMATION),
                                      &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to query system exception information: 0x%lx\n", Status);
        goto Exit;
    }

    SHA512_Update(&Sha512Context, Buffer, sizeof(SYSTEM_EXCEPTION_INFORMATION));

    /* Query lookaside information */
    Status = ZwQuerySystemInformation(SystemLookasideInformation,
                                      Buffer,
                                      sizeof(SYSTEM_LOOKASIDE_INFORMATION),
                                      &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to query system lookaside information: 0x%lx\n", Status);
        goto Exit;
    }

    SHA512_Update(&Sha512Context, Buffer, sizeof(SYSTEM_LOOKASIDE_INFORMATION));

    /* Query interrupt information */
    Status = ZwQuerySystemInformation(SystemInterruptInformation,
                                      Buffer,
                                      sizeof(SYSTEM_INTERRUPT_INFORMATION),
                                      &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to query system interrupt information: 0x%lx\n", Status);
        goto Exit;
    }

    SHA512_Update(&Sha512Context, Buffer, sizeof(SYSTEM_INTERRUPT_INFORMATION));

    /* Query process information (this one has a dynamic buffer size, retry up to 5 times) */
    for (ULONG i = 0; i < 5; i++)
    {
        Status = ZwQuerySystemInformation(SystemProcessInformation,
                                          Buffer,
                                          BufferSize,
                                          &ReturnLength);
        if (Status == STATUS_INFO_LENGTH_MISMATCH)
        {
            /* The buffer was too small, reallocate it with the required size */
            ExFreePoolWithTag(Buffer, 'cesK');
            BufferSize = ReturnLength;
            Buffer = ExAllocatePoolWithTag(PagedPool, BufferSize, 'cesK');
            if (Buffer == NULL)
            {
                DPRINT1("Failed to allocate buffer for system information\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }
        }
        else if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to query system process information: 0x%lx\n", Status);
            goto Exit;
        }
        else
        {
            break;
        }
    }

    SHA512_Update(&Sha512Context, Buffer, min(ReturnLength, BufferSize));

    /* Finalize the hash and store it in the output buffer */
    SHA512_Final(EntropyData->Hash, &Sha512Context);

    /* Erase the temp data */
    RtlSecureZeroMemory(Buffer, sizeof(Buffer));
    RtlSecureZeroMemory(&Sha512Context, sizeof(Sha512Context));

    Status = STATUS_SUCCESS;

Exit:

    ExFreePoolWithTag(Buffer, 'cesK');

    return Status;
}
