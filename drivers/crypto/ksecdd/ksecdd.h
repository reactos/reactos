/*
 * PROJECT:         ReactOS Drivers
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Security Support Provider Interface Driver
 *
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#define _NO_KSECDD_IMPORT_
#include <ntifs.h>
#include <ndk/extypes.h>

#if defined(_M_IX86) || defined(_M_AMD64)
typedef struct _KSEC_MACHINE_SPECIFIC_COUNTERS
{
    ULONG64 Tsc;
    ULONG64 Pmc0;
    ULONG64 Pmc1;
    ULONG64 Ctr0;
    ULONG64 Ctr1;
} KSEC_MACHINE_SPECIFIC_COUNTERS, *PKSEC_MACHINE_SPECIFIC_COUNTERS;
#else
typedef ULONG KSEC_MACHINE_SPECIFIC_COUNTERS;
#endif

typedef struct _KSEC_ENTROPY_DATA
{
    HANDLE CurrentProcessId;
    HANDLE CurrentThreadId;
    LARGE_INTEGER TickCount;
    LARGE_INTEGER SystemTime;
    LARGE_INTEGER PerformanceCounter;
    LARGE_INTEGER PerformanceFrequency;
    UCHAR EnvironmentHash[16];
    KSEC_MACHINE_SPECIFIC_COUNTERS MachineSpecificCounters;
    SYSTEM_PROCESSOR_PERFORMANCE_INFORMATION SystemProcessorPerformanceInformation;
    SYSTEM_PERFORMANCE_INFORMATION SystemPerformanceInformation;
    SYSTEM_EXCEPTION_INFORMATION SystemExceptionInformation;
    SYSTEM_LOOKASIDE_INFORMATION SystemLookasideInformation;
    SYSTEM_INTERRUPT_INFORMATION SystemInterruptInformation;
    SYSTEM_PROCESS_INFORMATION SystemProcessInformation;
} KSEC_ENTROPY_DATA, *PKSEC_ENTROPY_DATA;

NTSTATUS
NTAPI
KsecDdDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);


NTSTATUS
NTAPI
KsecGenRandom(
    PVOID Buffer,
    SIZE_T Length);

