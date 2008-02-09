#ifndef _ARMDDK_
#define _ARMDDK_

//
// IRQLs
//
#define PASSIVE_LEVEL           0
#define LOW_LEVEL               0
#define APC_LEVEL               1
#define DISPATCH_LEVEL          2
#define IPI_LEVEL               7
#define POWER_LEVEL             7
#define PROFILE_LEVEL           8
#define HIGH_LEVEL              8
#define SYNCH_LEVEL             (IPI_LEVEL - 1)

//
// FIXME: mmtypes.h?
//
#define KIPCR                   0xFFFFF000
#define USPCR                   0x7FFF0000
#define PCR                     ((volatile KPCR * const)USPCR)
#define USERPCR                 ((volatile KPCR * const)KIPCR)

//
// Just read it from the PCR
//
#define KeGetCurrentProcessorNumber() ((ULONG)(PCR->Number))

//
// Stub
//
typedef struct _KFLOATING_SAVE
{
    ULONG Reserved;
} KFLOATING_SAVE, *PKFLOATING_SAVE;

//
// Processor Control Region
// On ARM, it's actually readable from user-mode, much like KUSER_SHARED_DATA
//
#ifdef _WINNT_H
#define PKINTERRUPT_ROUTINE PVOID // Hack!
#endif
typedef struct _KPCR
{
    ULONG MinorVersion;
    ULONG MajorVersion;
    PKINTERRUPT_ROUTINE InterruptRoutine[64];
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
    UCHAR IrqlMask[64];
    UCHAR IrqlTable[64];
    UCHAR CurrentIrql;
    KAFFINITY SetMember;
    struct _KTHREAD *CurrentThread;
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
KeSwapIrql(
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

#define KeLowerIrql(NewIrql) KeSwapIrql(NewIrql)
#define KeRaiseIrql(NewIrql, OldIrql) *(OldIrql) = KeSwapIrql(NewIrql)

#endif
