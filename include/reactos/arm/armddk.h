#ifndef _ARMDDK_
#define _ARMDDK_

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

//
// FIXME: mmtypes.h?
//
#define KIPCR                   0xFFFFF000
#define KI_USER_SHARED_DATA     0xFFFFE000
#define USPCR                   0x7FFF0000
#define PCR                     ((volatile KPCR * const)KIPCR)
#define USERPCR                 ((volatile KPCR * const)USPCR)

//
// Maximum IRQs
//
#define MAXIMUM_VECTOR          16

//
// Just read it from the PCR
//
#define KeGetCurrentProcessorNumber()  (int)PCR->Number
#define KeGetCurrentIrql()             PCR->CurrentIrql
#define _KeGetCurrentThread()          PCR->CurrentThread
#define _KeGetPreviousMode()           PCR->CurrentThread->PreviousMode
#define _KeIsExecutingDpc()            (PCR->DpcRoutineActive != 0)
#define KeGetCurrentThread()           _KeGetCurrentThread()
#define KeGetPreviousMode()            _KeGetPreviousMode()
#define KeGetDcacheFillSize()          PCR->DcacheFillSize


//
// Used to contain PFNs and PFN counts
//
typedef ULONG PFN_COUNT;
typedef ULONG PFN_NUMBER, *PPFN_NUMBER;
typedef LONG SPFN_NUMBER, *PSPFN_NUMBER;

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
	ULONG Psr;
} CONTEXT;

//
// Processor Control Region
// On ARM, it's actually readable from user-mode, much like KUSER_SHARED_DATA
//
#ifdef _WINNT_H
typedef
VOID
(*PKINTERRUPT_ROUTINE)(VOID);
#endif
typedef struct _KPCR
{
    ULONG MinorVersion;
    ULONG MajorVersion;
    PKINTERRUPT_ROUTINE InterruptRoutine[MAXIMUM_VECTOR];
    PVOID XcodeDispatch;
    ULONG FirstLevelDcacheSize;
    ULONG FirstLevelDcacheFillSize;
    ULONG FirstLevelIcacheSize;
    ULONG FirstLevelIcacheFillSize;
    ULONG SecondLevelDcacheSize;
    ULONG SecondLevelDcacheFillSize;
    ULONG SecondLevelIcacheSize;
    ULONG SecondLevelIcacheFillSize;
    struct _KPRCB *Prcb;
    struct _TEB *Teb;
    PVOID TlsArray;
    ULONG DcacheFillSize;
    ULONG IcacheAlignment;
    ULONG IcacheFillSize;
    ULONG ProcessorId;
    ULONG ProfileInterval;
    ULONG ProfileCount;
    ULONG StallExecutionCount;
    ULONG StallScaleFactor;
    CCHAR Number;
    PVOID DataBusError;
    PVOID InstructionBusError;
    ULONG CachePolicy;
    ULONG AlignedCachePolicy;
    UCHAR IrqlMask[HIGH_LEVEL + 1];
    ULONG IrqlTable[HIGH_LEVEL + 1];
    UCHAR CurrentIrql;
    KAFFINITY SetMember;
    struct _KTHREAD *CurrentThread;
    ULONG ReservedVectors;
    KAFFINITY NotMember;
    ULONG SystemReserved[6];
    ULONG DcacheAlignment;
    ULONG HalReserved[64];
    BOOLEAN FirstLevelActive;
    BOOLEAN DpcRoutineActive;
    ULONG CurrentPid;
    BOOLEAN OnInterruptStack;
    PVOID SavedInitialStack;
    PVOID SavedStackLimit;
    PVOID SystemServiceDispatchStart;
    PVOID SystemServiceDispatchEnd;
    PVOID InterruptStack;
    PVOID PanicStack;
    PVOID BadVaddr;
    PVOID InitialStack;
    PVOID StackLimit;
    ULONG QuantumEnd;
    PVOID PerfGlobalGroupMask;
    ULONG ContextSwitches;
} KPCR, *PKPCR;

//
// Get the current TEB
//
FORCEINLINE
struct _TEB* NtCurrentTeb(VOID)
{
    return (struct _TEB*)USERPCR->Teb;
}

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

#endif
