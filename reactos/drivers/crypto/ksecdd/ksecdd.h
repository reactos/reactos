/*
 * PROJECT:         ReactOS Drivers
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         Kernel Security Support Provider Interface Driver
 *
 * PROGRAMMERS:     Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#define _NO_KSECDD_IMPORT_
#include <ntifs.h>
#include <ndk/exfuncs.h>
#include <ndk/ketypes.h>
#include <pseh/pseh2.h>
#include <ntstrsafe.h>

#include <md4.h>
#include <md5.h>
#include <tomcrypt.h>
typedef aes_key AES_KEY, *PAES_KEY;
typedef des3_key DES3_KEY, *PDES3_KEY;

#define STATUS_KSEC_INTERNAL_ERROR ((NTSTATUS)0x80090304)

/* FIXME: this should be in some shared header */
#define RTL_ENCRYPT_OPTION_SAME_PROCESS   0
#define RTL_ENCRYPT_OPTION_CROSS_PROCESS  1
#define RTL_ENCRYPT_OPTION_SAME_LOGON     2

typedef struct _KSEC_CONNECTION_INFO
{
    ULONG Unknown0;
    NTSTATUS Status;
    ULONG_PTR Information;
    CHAR ConnectionString[128];
    ULONG Flags;
} KSEC_CONNECTION_INFO;

#if defined(_M_IX86) || defined(_M_AMD64)
typedef struct _KSEC_MACHINE_SPECIFIC_COUNTERS
{
    ULONG64 Tsc;
    ULONG64 Pmc0;
    ULONG64 Pmc1;
    ULONG64 Ctr0;
    ULONG64 Ctr1;
} KSEC_MACHINE_SPECIFIC_COUNTERS, *PKSEC_MACHINE_SPECIFIC_COUNTERS;
#elif defined(_M_ARM)
typedef struct _KSEC_MACHINE_SPECIFIC_COUNTERS
{
    ULONG Ccr;
} KSEC_MACHINE_SPECIFIC_COUNTERS, *PKSEC_MACHINE_SPECIFIC_COUNTERS;
#else
typedef ULONG KSEC_MACHINE_SPECIFIC_COUNTERS, *PKSEC_MACHINE_SPECIFIC_COUNTERS;
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

extern PEPROCESS KsecLsaProcess;;
extern HANDLE KsecLsaProcessHandle;

NTSTATUS
NTAPI
KsecDdDispatch(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp);

NTSTATUS
NTAPI
KsecGatherEntropyData(
    PKSEC_ENTROPY_DATA EntropyData);

NTSTATUS
NTAPI
KsecGenRandom(
    PVOID Buffer,
    SIZE_T Length);

VOID
NTAPI
KsecInitializeEncryptionSupport (
    VOID);

NTSTATUS
NTAPI
KsecEncryptMemory (
    _Inout_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG OptionFlags);

NTSTATUS
NTAPI
KsecDecryptMemory (
    _Inout_ PVOID Buffer,
    _In_ ULONG Length,
    _In_ ULONG OptionFlags);

