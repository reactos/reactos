

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
// Synchronization-level IRQL
//
#ifndef CONFIG_SMP
#define SYNCH_LEVEL             DISPATCH_LEVEL
#else
#define SYNCH_LEVEL             (IPI_LEVEL - 2)
#endif

//
// IPI Types
//
#define IPI_APC                 1
#define IPI_DPC                 2
#define IPI_FREEZE              4
#define IPI_PACKET_READY        6
#define IPI_SYNCH_REQUEST       16

//
// PRCB Flags
//
#define PRCB_MAJOR_VERSION      1
#define PRCB_BUILD_DEBUG        1
#define PRCB_BUILD_UNIPROCESSOR 2

//
// Machine Types
//
#define MACHINE_TYPE_ISA        0x0000
#define MACHINE_TYPE_EISA       0x0001
#define MACHINE_TYPE_MCA        0x0002

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
// Double fault stack size
//
#define DOUBLE_FAULT_STACK_SIZE 0x8000

//
// Static Kernel-Mode Address start (use MM_KSEG0_BASE for actual)
//
#define KSEG0_BASE 0xFFFF800000000000ULL

//
// Structure for CPUID info
//
typedef union _CPU_INFO
{
    ULONG dummy;
} CPU_INFO, *PCPU_INFO;

typedef struct _KARM64_VFP_STATE
{
    struct _KARM64_VFP_STATE* Link; // 0x00
    ULONG Fpcr;                     // 0x08
    ULONG Fpsr;                     // 0x0c
    ARM64_NT_NEON128 V[32];         // 0x10
} KARM64_VFP_STATE, *PKARM64_VFP_STATE; // size = 0x210

typedef struct _KTRAP_FRAME
{
    UCHAR ExceptionActive;
    UCHAR ContextFromKFramesUnwound;
    UCHAR DebugRegistersValid;
    union
    {
        CHAR PreviousMode;
        UCHAR PreviousIrql;
    };
    ULONG Reserved;
    union
    {
        ULONG64 FaultAddress;
        ULONG64 TrapFrame;
    };
    PKARM64_VFP_STATE VfpState;
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
    ULONG64 TrapFrame;
    ULONG64 OutputBuffer;
    ULONG64 OutputLength;
    ULONG64 Pad;
    ULONG64 D8;
    ULONG64 D9;
    ULONG64 D10;
    ULONG64 D11;
    ULONG64 D12;
    ULONG64 D13;
    ULONG64 D14;
    ULONG64 D15;
    ULONG64 X19;
    ULONG64 X20;
    ULONG64 X21;
    ULONG64 X22;
    ULONG64 X23;
    ULONG64 X24;
    ULONG64 X25;
    ULONG64 X26;
    ULONG64 X27;
    ULONG64 X28;
    ULONG64 Fp;
    ULONG64 Return;
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

//
// Defines the Callback Stack Layout for User Mode Callbacks
//
typedef KEXCEPTION_FRAME KCALLOUT_FRAME, *PKCALLOUT_FRAME;

#ifndef NTOS_MODE_USER

//
// Frame left on the old stack by a context switch
//
typedef struct _KSWITCH_FRAME
{
    KIRQL ApcBypass;
    UCHAR Fill1[7];
    ULONG64 Pad;
    ULONG64 D8;
    ULONG64 D9;
    ULONG64 D10;
    ULONG64 D11;
    ULONG64 D12;
    ULONG64 D13;
    ULONG64 D14;
    ULONG64 D15;
    ULONG64 X19;
    ULONG64 X20;
    ULONG64 X21;
    ULONG64 X22;
    ULONG64 X23;
    ULONG64 X24;
    ULONG64 X25;
    ULONG64 X26;
    ULONG64 X27;
    ULONG64 X28;
    ULONG64 Fp;
    ULONG64 Return;
} KSWITCH_FRAME, *PKSWITCH_FRAME;

//
// Arguments picked up by the first switch to a new thread
//
typedef struct _KSTART_FRAME
{
    ULONG64 SystemRoutine;
    ULONG64 StartRoutine;
    ULONG64 StartContext;
    ULONG64 UserThread;
} KSTART_FRAME, *PKSTART_FRAME;

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

typedef struct _DISPATCHER_CONTEXT
{
    ULONG64 ControlPc;
    ULONG64 ImageBase;
    struct _RUNTIME_FUNCTION *FunctionEntry;
    ULONG64 EstablisherFrame;
    ULONG64 TargetPc;
    PCONTEXT ContextRecord;
    PEXCEPTION_ROUTINE LanguageHandler;
    PVOID HandlerData;
    struct _UNWIND_HISTORY_TABLE *HistoryTable;
    ULONG ScopeIndex;
    BOOLEAN ControlPcIsUnwound;
    PUCHAR NonVolatileRegisters;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

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

#define NUMBER_POOL_LOOKASIDE_LISTS 32

//
// Processor Region Control Block
// The fields up to Number follow WoA, the rest is private
//
typedef struct _KPRCB
{
    UCHAR LegacyNumber;
    UCHAR ReservedMustBeZero;
    UCHAR IdleHalt;
    struct _KTHREAD *CurrentThread;
    struct _KTHREAD *NextThread;
    struct _KTHREAD *IdleThread;
    UCHAR NestingLevel;
    UCHAR ClockOwner;
    union
    {
        UCHAR PendingTickFlags;
        struct
        {
            UCHAR PendingTick:1;
            UCHAR PendingBackupTick:1;
        };
    };
    UCHAR IdleState;
    ULONG Number;
    UINT64 PrcbLock;
    KAFFINITY SetMember;
    KPROCESSOR_STATE ProcessorState;
    CHAR CpuType;
    CHAR CpuID;
    USHORT CpuStep;
    ULONG MHz;
    ULONG64 CycleCounterFrequency;
    UINT64 HalReserved[8];
    USHORT MinorVersion;
    USHORT MajorVersion;
    UCHAR BuildType;
    UCHAR CpuVendor;
    UCHAR CoresPerPhysicalProcessor;
    UCHAR LogicalProcessorsPerCore;
    PVOID AcpiReserved;
    PVOID PanicStackBase;
    PVOID IsrStack;
    KSPIN_LOCK_QUEUE LockQueue[LockQueueMaximumLock];
    PP_LOOKASIDE_LIST PPLookasideList[16];
    PP_LOOKASIDE_LIST PPNPagedLookasideList[NUMBER_POOL_LOOKASIDE_LISTS];
    PP_LOOKASIDE_LIST PPPagedLookasideList[NUMBER_POOL_LOOKASIDE_LISTS];
    UINT64 PacketBarrier;
    SINGLE_LIST_ENTRY DeferredReadyListHead;
    LONG MmPageFaultCount;
    LONG MmCopyOnWriteCount;
    LONG MmTransitionCount;
    LONG MmCacheTransitionCount;
    LONG MmDemandZeroCount;
    LONG MmPageReadCount;
    LONG MmPageReadIoCount;
    LONG MmCacheReadCount;
    LONG MmCacheIoCount;
    LONG MmDirtyPagesWriteCount;
    LONG MmDirtyWriteIoCount;
    LONG MmMappedPagesWriteCount;
    LONG MmMappedWriteIoCount;
    LONG LookasideIrpFloat;
    ULONG KeSystemCalls;
    LONG IoReadOperationCount;
    LONG IoWriteOperationCount;
    LONG IoOtherOperationCount;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
    ULONG KeContextSwitches;
    ULONG IpiFrozen;
    UINT64 TargetSet;
    volatile ULONG64 RequestSummary;
    volatile struct _KPRCB *SignalDone;
    KDPC_DATA DpcData[2];
    PVOID DpcStack;
    PVOID SavedSp;
    LONG MaximumDpcQueueDepth;
    ULONG DpcRequestRate;
    ULONG MinimumDpcRate;
    UCHAR DpcInterruptRequested;
    UCHAR DpcThreadRequested;
    UCHAR DpcRoutineActive;
    UCHAR DpcThreadActive;
    UINT64 TimerHand;
    UINT64 TimerRequest;
    LONG TickOffset;
    LONG MasterOffset;
    ULONG DpcLastCount;
    UCHAR ThreadDpcEnable;
    UCHAR QuantumEnd;
    UCHAR PrcbPad50;
    UCHAR IdleSchedule;
    LONG DpcSetEventRequest;
    LONG PrcbPad40;
    PVOID DpcThread;
    KEVENT DpcEvent;
    KDPC CallDpc;
    LIST_ENTRY WaitListHead;
    ULONG ReadySummary;
    ULONG QueueIndex;
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
    UCHAR PrcbPad8[5];
    struct _KNODE *ParentNode;
    KAFFINITY MultiThreadProcessorSet;
    struct _KPRCB *MultiThreadSetMaster;
    ULONG DebugDpcTime;
    ULONG PageColor;
    ULONG NodeColor;
    ULONG NodeShiftedColor;
    ULONG SecondaryColorMask;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadNotPossible;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    ULONG KeAlignmentFixupCount;
    ULONG KeDcacheFlushCount;
    ULONG KeExceptionDispatchCount;
    ULONG KeFirstLevelTbFills;
    ULONG KeFloatingEmulationCount;
    ULONG KeIcacheFlushCount;
    ULONG KeSecondLevelTbFills;
    LONG Sleeping;
    UCHAR VendorString[13];
    UCHAR PrcbPad10[3];
    ULONG FeatureBits;
    ULONG MaxBreakpoints;
    ULONG MaxWatchpoints;
    LARGE_INTEGER UpdateSignature;
    PROCESSOR_POWER_STATE PowerState;
    CACHE_DESCRIPTOR Cache[5];
    ULONG CacheCount;
    UINT64 StartCycles;
    UINT64 CycleTime;
    ULONG FeatureBitsHigh;
} KPRCB, *PKPRCB;

//
// Processor Control Region
// Based on WoA
//
typedef struct _KIPCR
{
    union
    {
        NT_TIB NtTib;
        struct
        {
            PVOID TibPad0[2];
            PVOID Spare1;
            struct _KPCR *Self;
            PVOID PcrReserved0;
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
    union
    {
        USHORT SoftwareInterruptPending;
        struct
        {
            UCHAR ApcInterrupt;
            UCHAR DispatchInterrupt;
        };
    };
    USHORT InterruptPad;
    union
    {
        UCHAR BtiMitigation;
        struct
        {
            UCHAR KvaVbar:1;
            UCHAR BtiVbar:3;
            UCHAR BtiCswapHvc:1;
            UCHAR BtiCswapSmc:1;
        };
    };
    union
    {
        UCHAR SsbMitigationFlags;
        struct
        {
            UCHAR SsbMitigationFirmware:1;
            UCHAR SsbMitigationDynamic:1;
            UCHAR SsbMitigationKernel:1;
            UCHAR SsbMitigationUser:1;
            UCHAR SsbMitigationReserved:4;
        };
    };
    UCHAR BhbMitigation;
    union
    {
        UCHAR CachePrefetcherMitigationFlags;
        struct
        {
            UCHAR CachePrefetcherMitigation:1;
            UCHAR Pad2:7;
        };
    };
    ULONG64 PanicStorage[6];
    PVOID KdVersionBlock;
    PVOID HalReserved[14];
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
    return &((PKIPCR)KeGetPcr())->Prcb;
}

//
// Just read it from the PCR
//
#define KeGetCurrentIrql()             KeGetPcr()->CurrentIrql
#define _KeIsExecutingDpc()            (KeGetCurrentPrcb()->DpcRoutineActive != 0)

#endif // !NTOS_MODE_USER

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // !_ARM64_KETYPES_H
