#ifndef _ARMDDK_
#define _ARMDDK_

//
// Page size
//
#ifndef PAGE_SIZE
#define PAGE_SIZE 0x1000
#endif

#ifndef _WINNT_
//
// IRQLs
//
#define PASSIVE_LEVEL                     0
#define LOW_LEVEL                         0
#define APC_LEVEL                         1
#define DISPATCH_LEVEL                    2
#define SYNCH_LEVEL                       DISPATCH_LEVEL
#define PROFILE_LEVEL                     27
#define CLOCK1_LEVEL                      28
#define CLOCK2_LEVEL                      28
#define IPI_LEVEL                         29
#define POWER_LEVEL                       30
#define HIGH_LEVEL                        31
#endif

//
// FIXME: mmtypes.h?
//
#define KIP0PCRADDRESS          0xFFDFF000
#define KI_USER_SHARED_DATA     0xFFFF9000
#define USPCR                   0x7FFF0000
#define PCR                     ((KPCR * const)KIP0PCRADDRESS)
#define USERPCR                 ((volatile KPCR * const)USPCR)
#define KeGetPcr()              PCR
#ifndef _WINNT_
#define SharedUserData          ((KUSER_SHARED_DATA * const)KI_USER_SHARED_DATA)

//
// Address space layout
//
extern PVOID MmHighestUserAddress;
extern PVOID MmSystemRangeStart;
extern ULONG_PTR MmUserProbeAddress;
#define MM_HIGHEST_USER_ADDRESS           MmHighestUserAddress
#define MM_SYSTEM_RANGE_START             MmSystemRangeStart
#define MM_USER_PROBE_ADDRESS             MmUserProbeAddress
#define MM_LOWEST_USER_ADDRESS            (PVOID)0x10000
#define MM_LOWEST_SYSTEM_ADDRESS          (PVOID)0xC0800000

//
// Maximum IRQs
//
#define MAXIMUM_VECTOR          16

#define KERNEL_STACK_SIZE                   12288
#define KERNEL_LARGE_STACK_SIZE             61440
#define KERNEL_LARGE_STACK_COMMIT           12288

//
// Used to contain PFNs and PFN counts
//
//typedef ULONG PFN_COUNT;
//typedef ULONG PFN_NUMBER, *PPFN_NUMBER;
//typedef LONG SPFN_NUMBER, *PSPFN_NUMBER;

//
// Stub
//
typedef struct _KFLOATING_SAVE
{
    ULONG Reserved;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

/* The following flags control the contents of the CONTEXT structure. */
#define CONTEXT_ARM    0x0000040
#define CONTEXT_CONTROL         (CONTEXT_ARM | 0x00000001L)
#define CONTEXT_INTEGER         (CONTEXT_ARM | 0x00000002L)
#define CONTEXT_FULL (CONTEXT_CONTROL | CONTEXT_INTEGER)


typedef struct _NEON128 {
    ULONGLONG Low;
    LONGLONG High;
} NEON128, *PNEON128;

#define ARM_MAX_BREAKPOINTS 8
#define ARM_MAX_WATCHPOINTS 1

typedef struct _CONTEXT {
    /* The flags values within this flag control the contents of
       a CONTEXT record.

       If the context record is used as an input parameter, then
       for each portion of the context record controlled by a flag
       whose value is set, it is assumed that that portion of the
       context record contains valid context. If the context record
       is being used to modify a thread's context, then only that
       portion of the threads context will be modified.

       If the context record is used as an IN OUT parameter to capture
       the context of a thread, then only those portions of the thread's
       context corresponding to set flags will be returned.

       The context record is never used as an OUT only parameter. */

    ULONG ContextFlags;

    /* This section is specified/returned if the ContextFlags word contains
       the flag CONTEXT_INTEGER. */
    ULONG R0;
    ULONG R1;
    ULONG R2;
    ULONG R3;
    ULONG R4;
    ULONG R5;
    ULONG R6;
    ULONG R7;
    ULONG R8;
    ULONG R9;
    ULONG R10;
    ULONG R11;
    ULONG R12;

    ULONG Sp;
    ULONG Lr;
    ULONG Pc;
    ULONG Cpsr;

    /* Floating Point/NEON Registers */
    ULONG Fpscr;
    ULONG Padding;
    union {
        NEON128 Q[16];
        ULONGLONG D[32];
        ULONG S[32];
    } DUMMYUNIONNAME;

    /* Debug registers */
    ULONG Bvr[ARM_MAX_BREAKPOINTS];
    ULONG Bcr[ARM_MAX_BREAKPOINTS];
    ULONG Wvr[ARM_MAX_WATCHPOINTS];
    ULONG Wcr[ARM_MAX_WATCHPOINTS];

    ULONG Padding2[2];
} CONTEXT;

#endif

//
// Processor Control Region
//
#ifdef _WINNT_
#define KIRQL ULONG
#endif

typedef struct _NT_TIB_KPCR {
	struct _EXCEPTION_REGISTRATION_RECORD *ExceptionList;
	PVOID StackBase;
	PVOID StackLimit;
	PVOID SubSystemTib;
	_ANONYMOUS_UNION union {
		PVOID FiberData;
		ULONG Version;
	} DUMMYUNIONNAME;
	PVOID ArbitraryUserPointer;
	struct _NT_TIB_KPCR *Self;
} NT_TIB_KPCR,*PNT_TIB_KPCR;

typedef struct _KPCR
{
    union
    {
        NT_TIB_KPCR NtTib;
        struct
        {
            struct _EXCEPTION_REGISTRATION_RECORD *Used_ExceptionList; // Unused
            PVOID Used_StackBase; // Unused
            PVOID PerfGlobalGroupMask;
            PVOID TssCopy; // Unused
            ULONG ContextSwitches;
            KAFFINITY SetMemberCopy; // Unused
            PVOID Used_Self;
        };
    };
    struct _KPCR *Self;
    struct _KPRCB *Prcb;
    KIRQL Irql;
    ULONG IRR; // Unused
    ULONG IrrActive; // Unused
    ULONG IDR; // Unused
    PVOID KdVersionBlock;
    PVOID IDT; // Unused
    PVOID GDT; // Unused
    PVOID TSS; // Unused
    USHORT MajorVersion;
    USHORT MinorVersion;
    KAFFINITY SetMember;
    ULONG StallScaleFactor;
    UCHAR SpareUnused;
    UCHAR Number;
    UCHAR Spare0;
    UCHAR SecondLevelCacheAssociativity;
    ULONG VdmAlert;
    ULONG KernelReserved[14];
    ULONG SecondLevelCacheSize;
    ULONG HalReserved[16];
} KPCR, *PKPCR;

//
// Get the current TEB
//
FORCEINLINE
struct _TEB* NtCurrentTeb(VOID)
{
    return (struct _TEB*)USERPCR->Used_Self;
}

NTSYSAPI
struct _KTHREAD*
NTAPI
KeGetCurrentThread(VOID);

FORCEINLINE
NTSTATUS
KeSaveFloatingPointState(PVOID FloatingState)
{
  UNREFERENCED_PARAMETER(FloatingState);
  return STATUS_SUCCESS;
}

FORCEINLINE
NTSTATUS
KeRestoreFloatingPointState(PVOID FloatingState)
{
  UNREFERENCED_PARAMETER(FloatingState);
  return STATUS_SUCCESS;
}

extern volatile struct _KSYSTEM_TIME KeTickCount;

#ifndef YieldProcessor
#define YieldProcessor __yield
#endif

#define ASSERT_BREAKPOINT BREAKPOINT_COMMAND_STRING + 1

#define DbgRaiseAssertionFailure() __emit(0xdefc)

#define PCR_MINOR_VERSION 1
#define PCR_MAJOR_VERSION 1

#define RESULT_ZERO     0
#define RESULT_NEGATIVE 1
#define RESULT_POSITIVE 2

#if 0
DECLSPEC_IMPORT
VOID
__fastcall
KfReleaseSpinLock(
  IN OUT ULONG_PTR* SpinLock,
  IN KIRQL NewIrql);

DECLSPEC_IMPORT
KIRQL
__fastcall
KfAcquireSpinLock(
  IN OUT ULONG_PTR* SpinLock);
#endif

#ifndef _WINNT_
//
// IRQL Support on ARM is similar to MIPS/ALPHA
//
KIRQL
KfRaiseIrql(
    IN KIRQL NewIrql
);

VOID
KfLowerIrql(
    IN KIRQL NewIrql
);

KIRQL
KeRaiseIrqlToSynchLevel(
    VOID
);

KIRQL
KeRaiseIrqlToDpcLevel(
    VOID
);

#define KeLowerIrql(NewIrql) KfLowerIrql(NewIrql)
#define KeRaiseIrql(NewIrql, OldIrql) *(OldIrql) = KfRaiseIrql(NewIrql)

NTHALAPI
KIRQL
FASTCALL
KfAcquireSpinLock(
  IN OUT PKSPIN_LOCK SpinLock);
#define KeAcquireSpinLock(a,b) *(b) = KfAcquireSpinLock(a)

NTHALAPI
VOID
FASTCALL
KfReleaseSpinLock(
  IN OUT PKSPIN_LOCK SpinLock,
  IN KIRQL NewIrql);
#define KeReleaseSpinLock(a,b) KfReleaseSpinLock(a,b)

NTKERNELAPI
VOID
FASTCALL
KefAcquireSpinLockAtDpcLevel(
  IN OUT PKSPIN_LOCK SpinLock);
#define KeAcquireSpinLockAtDpcLevel(SpinLock) KefAcquireSpinLockAtDpcLevel(SpinLock)

NTKERNELAPI
VOID
FASTCALL
KefReleaseSpinLockFromDpcLevel(
  IN OUT PKSPIN_LOCK SpinLock);
#define KeReleaseSpinLockFromDpcLevel(SpinLock) KefReleaseSpinLockFromDpcLevel(SpinLock)

//
// Cache clean and flush
//
VOID
HalSweepDcache(
    VOID
);

VOID
HalSweepIcache(
    VOID
);

FORCEINLINE
VOID
_KeQueryTickCount(
  OUT PLARGE_INTEGER CurrentCount)
{
  for (;;) {
#ifdef NONAMELESSUNION
    CurrentCount->s.HighPart = KeTickCount.High1Time;
    CurrentCount->s.LowPart = KeTickCount.LowPart;
    if (CurrentCount->s.HighPart == KeTickCount.High2Time) break;
#else
    CurrentCount->HighPart = KeTickCount.High1Time;
    CurrentCount->LowPart = KeTickCount.LowPart;
    if (CurrentCount->HighPart == KeTickCount.High2Time) break;
#endif
    YieldProcessor();
  }
}
#define KeQueryTickCount(CurrentCount) _KeQueryTickCount(CurrentCount)
#endif

//
// Intrinsics
//
#define InterlockedDecrement _InterlockedDecrement
#define InterlockedIncrement _InterlockedIncrement
#define InterlockedExchange  _InterlockedExchange
#endif
