

#ifndef _ARM64_KETYPES_H
#define _ARM64_KETYPES_H

#ifdef __cplusplus
extern "C" {
#endif

/* Interrupt request levels */
#define PASSIVE_LEVEL           0
#define LOW_LEVEL               0
#define APC_LEVEL               1
#define DISPATCH_LEVEL          2
#define CMCI_LEVEL              5
#define CLOCK_LEVEL             13
#define IPI_LEVEL               14
#define DRS_LEVEL               14
#define POWER_LEVEL             14
#define PROFILE_LEVEL           15
#define HIGH_LEVEL              15

//
// IPI Types
//
#define IPI_APC                 1
#define IPI_DPC                 2
#define IPI_FREEZE              4
#define IPI_PACKET_READY        6
#define IPI_SYNCH_REQUEST       16

#define KSEG0_BASE 0xfffff80000000000ULL

//
// PRCB Flags
//
#define PRCB_MAJOR_VERSION      1
#define PRCB_BUILD_DEBUG        1
#define PRCB_BUILD_UNIPROCESSOR 2

//
// No LDTs on ARM64
//
#define LDT_ENTRY              ULONG


//
// HAL Variables
//
#define INITIAL_STALL_COUNT     100
#define MM_HAL_VA_START         0xFFFFFFFFFFC00000ULL
#define MM_HAL_VA_END           0xFFFFFFFFFFFFFFFFULL

//
// Static Kernel-Mode Address start (use MM_KSEG0_BASE for actual)
//
#define KSEG0_BASE 0xfffff80000000000ULL

//
// Structure for CPUID info
//
typedef union _CPU_INFO
{
    ULONG dummy;
} CPU_INFO, *PCPU_INFO;

typedef struct _KTRAP_FRAME
{
    UCHAR ExceptionActive;
    UCHAR ContextFromKFramesUnwound;
    UCHAR DebugRegistersValid;
    union
    {
        struct
        {
            CHAR PreviousMode;
            UCHAR PreviousIrql;
        };
    };
    ULONG Reserved;
    union
    {
        struct
        {
            ULONG64 FaultAddress;
            ULONG64 TrapFrame;
        };
    };
    //struct PKARM64_VFP_STATE VfpState;
    ULONG VfpState;
    ULONG Bcr[8];
    ULONG64 Bvr[8];
    ULONG Wcr[2];
    ULONG64 Wvr[2];
    ULONG Spsr;
    ULONG Esr;
    ULONG64 Sp;
    union
    {
        ULONG64 X[19];
        struct
        {
            ULONG64 X0;
            ULONG64 X1;
            ULONG64 X2;
            ULONG64 X3;
            ULONG64 X4;
            ULONG64 X5;
            ULONG64 X6;
            ULONG64 X7;
            ULONG64 X8;
            ULONG64 X9;
            ULONG64 X10;
            ULONG64 X11;
            ULONG64 X12;
            ULONG64 X13;
            ULONG64 X14;
            ULONG64 X15;
            ULONG64 X16;
            ULONG64 X17;
            ULONG64 X18;
        };
    };
    ULONG64 Lr;
    ULONG64 Fp;
    ULONG64 Pc;
} KTRAP_FRAME, *PKTRAP_FRAME;

typedef struct _KEXCEPTION_FRAME
{
    ULONG dummy;
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

#ifndef NTOS_MODE_USER

typedef struct _TRAPFRAME_LOG_ENTRY
{
    ULONG64 Thread;
    UCHAR CpuNumber;
    UCHAR TrapType;
    USHORT Padding;
    ULONG Cpsrl;
    ULONG64 X0;
    ULONG64 X1;
    ULONG64 X2;
    ULONG64 X3;
    ULONG64 X4;
    ULONG64 X5;
    ULONG64 X6;
    ULONG64 X7;
    ULONG64 Fp;
    ULONG64 Lr;
    ULONG64 Sp;
    ULONG64 Pc;
    ULONG64 Far;
    ULONG Esr;
    ULONG Reserved1;
} TRAPFRAME_LOG_ENTRY, *PTRAPFRAME_LOG_ENTRY;


//
// Special Registers Structure (outside of CONTEXT)
// Based on WoA symbols
//
typedef struct _KSPECIAL_REGISTERS
{
    ULONG64 Elr_El1;
    UINT32  Spsr_El1;
    ULONG64 Tpidr_El0;
    ULONG64 Tpidrro_El0;
    ULONG64 Tpidr_El1;
    ULONG64 KernelBvr[8];
    ULONG   KernelBcr[8];
    ULONG64 KernelWvr[2];
    ULONG   KernelWcr[2];
} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;

//
// ARM64 Architecture State
// Based on WoA symbols
//
typedef struct _KARM64_ARCH_STATE
{
    ULONG64 Midr_El1;
    ULONG64 Sctlr_El1;
    ULONG64 Actlr_El1;
    ULONG64 Cpacr_El1;
    ULONG64 Tcr_El1;
    ULONG64 Ttbr0_El1;
    ULONG64 Ttbr1_El1;
    ULONG64 Esr_El1;
    ULONG64 Far_El1;
    ULONG64 Pmcr_El0;
    ULONG64 Pmcntenset_El0;
    ULONG64 Pmccntr_El0;
    ULONG64 Pmxevcntr_El0[31];
    ULONG64 Pmxevtyper_El0[31];
    ULONG64 Pmovsclr_El0;
    ULONG64 Pmselr_El0;
    ULONG64 Pmuserenr_El0;
    ULONG64 Mair_El1;
    ULONG64 Vbar_El1;
} KARM64_ARCH_STATE, *PKARM64_ARCH_STATE;

typedef struct _KPROCESSOR_STATE
{
    KSPECIAL_REGISTERS SpecialRegisters; // 0
    KARM64_ARCH_STATE ArchState;         // 160
    CONTEXT ContextFrame;                // 800
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;


//
// Processor Region Control Block
// Based on WoA
//
typedef struct _KPRCB
{
    UCHAR LegacyNumber;
    UCHAR ReservedMustBeZero;
    UCHAR IdleHalt;
    PKTHREAD CurrentThread;
    PKTHREAD NextThread;
    PKTHREAD IdleThread;
    UCHAR NestingLevel;
    UCHAR ClockOwner;
    union
    {
        UCHAR PendingTickFlags;
        struct
        {
            UCHAR PendingTick : 1;
            UCHAR PendingBackupTick : 1;
        };
    };
    UCHAR PrcbPad00[1];
    ULONG Number;
    ULONG PrcbLock;
    PCHAR PriorityState;
    KPROCESSOR_STATE ProcessorState;
    USHORT ProcessorModel;
    USHORT ProcessorRevision;
    ULONG MHz;
    UINT64 CycleCounterFrequency;
    ULONG HalReserved[15];
    USHORT MinorVersion;
    USHORT MajorVersion;
    UCHAR BuildType;
    UCHAR CpuVendor;
    UCHAR CoresPerPhysicalProcessor;
    UCHAR LogicalProcessorsPerCore;
    PVOID AcpiReserved;
    ULONG GroupSetMember;
    UCHAR Group;
    UCHAR GroupIndex;
    //UCHAR _PADDING1_[0x62];
    KSPIN_LOCK_QUEUE DECLSPEC_ALIGN(128) LockQueue[17];
    UCHAR ProcessorVendorString[2];
    UCHAR _PADDING2_[0x2];
    ULONG FeatureBits;
    ULONG MaxBreakpoints;
    ULONG MaxWatchpoints;
    PCONTEXT Context;
    ULONG ContextFlagsInit;
    //UCHAR _PADDING3_[0x60];
    PP_LOOKASIDE_LIST DECLSPEC_ALIGN(128) PPLookasideList[16];
    LONG PacketBarrier;
    SINGLE_LIST_ENTRY DeferredReadyListHead;
    LONG MmPageFaultCount;
    LONG MmCopyOnWriteCount;
    LONG MmTransitionCount;
    LONG MmDemandZeroCount;
    LONG MmPageReadCount;
    LONG MmPageReadIoCount;
    LONG MmDirtyPagesWriteCount;
    LONG MmDirtyWriteIoCount;
    LONG MmMappedPagesWriteCount;
    LONG MmMappedWriteIoCount;
    ULONG KeSystemCalls;
    ULONG KeContextSwitches;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadNotPossible;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    LONG LookasideIrpFloat;
    LONG IoReadOperationCount;
    LONG IoWriteOperationCount;
    LONG IoOtherOperationCount;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
    UCHAR _PADDING4_[0x8];
    struct _REQUEST_MAILBOX* Mailbox;
    LONG TargetCount;
    ULONG IpiFrozen;
    ULONG RequestSummary;
    KDPC_DATA DpcData[2];
    PVOID DpcStack;
    PVOID SpBase;
    LONG MaximumDpcQueueDepth;
    ULONG DpcRequestRate;
    ULONG MinimumDpcRate;
    ULONG DpcLastCount;
    UCHAR ThreadDpcEnable;
    UCHAR QuantumEnd;
    UCHAR DpcRoutineActive;
    UCHAR IdleSchedule;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    union
    {
        LONG DpcRequestSummary;
        SHORT DpcRequestSlot[2];
        struct
        {
            SHORT NormalDpcState;
            SHORT ThreadDpcState;
        };
        struct
        {
            ULONG DpcNormalProcessingActive : 1;
            ULONG DpcNormalProcessingRequested : 1;
            ULONG DpcNormalThreadSignal : 1;
            ULONG DpcNormalTimerExpiration : 1;
            ULONG DpcNormalDpcPresent : 1;
            ULONG DpcNormalLocalInterrupt : 1;
            ULONG DpcNormalSpare : 10;
            ULONG DpcThreadActive : 1;
            ULONG DpcThreadRequested : 1;
            ULONG DpcThreadSpare : 14;
        };
    };
#else
    LONG DpcSetEventRequest;
#endif
    ULONG LastTimerHand;
    ULONG LastTick;
    ULONG ClockInterrupts;
    ULONG ReadyScanTick;
    ULONG PrcbPad10[1];
    ULONG InterruptLastCount;
    ULONG InterruptRate;
    UCHAR _PADDING5_[0x4];
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    KGATE DpcGate;
#else
    KEVENT DpcEvent;
#endif
    ULONG MPAffinity;
    KDPC CallDpc;
    LONG ClockKeepAlive;
    UCHAR ClockCheckSlot;
    UCHAR ClockPollCycle;
    //UCHAR _PADDING6_[0x2];
    LONG DpcWatchdogPeriod;
    LONG DpcWatchdogCount;
    LONG KeSpinLockOrdering;
    UCHAR _PADDING7_[0x38];
    LIST_ENTRY WaitListHead;
    ULONG WaitLock;
    ULONG ReadySummary;
    LONG AffinitizedSelectionMask;
    ULONG QueueIndex;
    KDPC TimerExpirationDpc;
    //RTL_RB_TREE ScbQueue;
    LIST_ENTRY ScbList;
    UCHAR _PADDING8_[0x38];
    LIST_ENTRY DispatcherReadyListHead[32];
    ULONG InterruptCount;
    ULONG KernelTime;
    ULONG UserTime;
    ULONG DpcTime;
    ULONG InterruptTime;
    ULONG AdjustDpcThreshold;
    UCHAR SkipTick;
    UCHAR DebuggerSavedIRQL;
    UCHAR PollSlot;
    UCHAR GroupSchedulingOverQuota;
    ULONG DpcTimeCount;
    ULONG DpcTimeLimit;
    ULONG PeriodicCount;
    ULONG PeriodicBias;
    ULONG AvailableTime;
    ULONG ScbOffset;
    ULONG KeExceptionDispatchCount;
    struct _KNODE* ParentNode;
    UCHAR _PADDING9_[0x4];
    ULONG64 AffinitizedCycles;
    ULONG64 StartCycles;
    ULONG64 GenerationTarget;
    ULONG64 CycleCounterHigh;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    KENTROPY_TIMING_STATE EntropyTimingState;
#endif /* (NTDDI_VERSION >= NTDDI_WIN8) */
    LONG MmSpinLockOrdering;
    ULONG PageColor;
    ULONG NodeColor;
    ULONG NodeShiftedColor;
    ULONG SecondaryColorMask;
    ULONG64 CycleTime;
    UCHAR _PADDING10_[0x58];
    ULONG CcFastMdlReadNoWait;
    ULONG CcFastMdlReadWait;
    ULONG CcFastMdlReadNotPossible;
    ULONG CcMapDataNoWait;
    ULONG CcMapDataWait;
    ULONG CcPinMappedDataCount;
    ULONG CcPinReadNoWait;
    ULONG CcPinReadWait;
    ULONG CcMdlReadNoWait;
    ULONG CcMdlReadWait;
    ULONG CcLazyWriteHotSpots;
    ULONG CcLazyWriteIos;
    ULONG CcLazyWritePages;
    ULONG CcDataFlushes;
    ULONG CcDataPages;
    ULONG CcLostDelayedWrites;
    ULONG CcFastReadResourceMiss;
    ULONG CcCopyReadWaitMiss;
    ULONG CcFastMdlReadResourceMiss;
    ULONG CcMapDataNoWaitMiss;
    ULONG CcMapDataWaitMiss;
    ULONG CcPinReadNoWaitMiss;
    ULONG CcPinReadWaitMiss;
    ULONG CcMdlReadNoWaitMiss;
    ULONG CcMdlReadWaitMiss;
    ULONG CcReadAheadIos;
    LONG MmCacheTransitionCount;
    LONG MmCacheReadCount;
    LONG MmCacheIoCount;
    UCHAR _PADDING11_[0xC];
    PROCESSOR_POWER_STATE PowerState;
    ULONG SharedReadyQueueOffset;
    ULONG PrcbPad15[2];
    ULONG DeviceInterrupts;
    PVOID IsrDpcStats;
    ULONG KeAlignmentFixupCount;
    KDPC DpcWatchdogDpc;
    KTIMER DpcWatchdogTimer;
    SLIST_HEADER InterruptObjectPool;
    //KAFFINITY_EX PackageProcessorSet;
    UCHAR _PADDING12_[0x4];
    ULONG SharedReadyQueueMask;
    struct _KSHARED_READY_QUEUE* SharedReadyQueue;
    ULONG CoreProcessorSet;
    ULONG ScanSiblingMask;
    ULONG LLCMask;
    ULONG CacheProcessorMask[5];
    ULONG ScanSiblingIndex;
    CACHE_DESCRIPTOR Cache[6];
    UCHAR CacheCount;
    UCHAR PrcbPad20[3];
    ULONG CachedCommit;
    ULONG CachedResidentAvailable;
    PVOID HyperPte;
    PVOID WheaInfo;
    PVOID EtwSupport;
    UCHAR _PADDING13_[0x74];
    SYNCH_COUNTERS SynchCounters;
    //FILESYSTEM_DISK_COUNTERS FsCounters;
    UCHAR _PADDING14_[0x8];
    ULONG PanicStackBase;
    PVOID IsrStack;
    ULONG PteBitCache;
    ULONG PteBitOffset;
    KTIMER_TABLE TimerTable;
    GENERAL_LOOKASIDE_POOL PPNxPagedLookasideList[32];
    GENERAL_LOOKASIDE_POOL PPNPagedLookasideList[32];
    GENERAL_LOOKASIDE_POOL PPPagedLookasideList[32];
    SINGLE_LIST_ENTRY AbSelfIoBoostsList;
    SINGLE_LIST_ENTRY AbPropagateBoostsList;
    KDPC AbDpc;
    UCHAR _PADDING15_[0x58];
    //REQUEST_MAILBOX RequestMailbox[1];

    // FIXME: Oldstyle stuff
#if (NTDDI_VERSION < NTDDI_WIN8) // FIXME
    UCHAR CpuType;
    volatile UCHAR DpcInterruptRequested;
    volatile UCHAR DpcThreadRequested;
    volatile UCHAR DpcThreadActive;
    volatile ULONG TimerHand;
    volatile ULONG TimerRequest;
    ULONG DebugDpcTime;
    LONG Sleeping;
    KAFFINITY SetMember;
    CHAR VendorString[13];
#endif

} KPRCB, *PKPRCB;

//
// Processor Control Region
// Based on WoA
//
typedef struct _KIPCR
{
    union
    {
        struct
        {
            ULONG TibPad0[2];
            PVOID Spare1;
            struct _KPCR *Self;
            PVOID  PcrReserved0;
            struct _KSPIN_LOCK_QUEUE* LockArray;
            PVOID Used_Self;
        };
    };
    KIRQL CurrentIrql;
    UCHAR SecondLevelCacheAssociativity;
    UCHAR Pad1[2];
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    ULONG SecondLevelCacheSize;
    struct
    {
        UCHAR ApcInterrupt;
        UCHAR DispatchInterrupt;
    };
    USHORT InterruptPad;
    UCHAR BtiMitigation;
    struct
    {
        UCHAR SsbMitigationFirmware:1;
        UCHAR SsbMitigationDynamic:1;
        UCHAR SsbMitigationKernel:1;
        UCHAR SsbMitigationUser:1;
        UCHAR SsbMitigationReserved:4;
    };
    UCHAR Pad2[2];
    ULONG64 PanicStorage[6];
    PVOID KdVersionBlock;
    PVOID HalReserved[134];
    PVOID KvaUserModeTtbr1;

    /* Private members, not in ntddk.h */
    PVOID Idt[256];
    PVOID* IdtExt;
    PVOID PcrAlign[15];
    KPRCB Prcb;
} KIPCR, *PKIPCR;
//
// Macro to get current KPRCB
//
FORCEINLINE
struct _KPRCB *
KeGetCurrentPrcb(VOID)
{  
    //UNIMPLEMENTED;
    return 0;
}

//
// Just read it from the PCR
//
#define KeGetCurrentIrql()             KeGetPcr()->CurrentIrql
#define _KeGetCurrentThread()          KeGetCurrentPrcb()->CurrentThread
#define _KeGetPreviousMode()           KeGetCurrentPrcb()->CurrentThread->PreviousMode
#define _KeIsExecutingDpc()            (KeGetCurrentPrcb()->DpcRoutineActive != 0)
#define KeGetCurrentThread()           _KeGetCurrentThread()
#define KeGetPreviousMode()            _KeGetPreviousMode()

#endif // !NTOS_MODE_USER

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // !_ARM64_KETYPES_H
