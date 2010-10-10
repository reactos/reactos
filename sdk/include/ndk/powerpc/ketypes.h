/*++ NDK Version: 0095

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    ketypes.h (PPC)

Abstract:

    PowerPC Type definitions for the Kernel services.

Author:

    Alex Ionescu (alex.ionescu@reactos.com)   06-Oct-2004

--*/

#ifndef _POWERPC_KETYPES_H
#define _POWERPC_KETYPES_H

//
// Dependencies
//

//
// IPI Types
//
#define IPI_APC                 1
#define IPI_DPC                 2
#define IPI_FREEZE              3
#define IPI_PACKET_READY        4
#define IPI_SYNCH_REQUEST       10
#define MAXIMUM_VECTOR          0x100

#define KSEG0_BASE 0x80000000

#define PRCB_MAJOR_VERSION 1
#define PRCB_BUILD_DEBUG 1

#ifndef ROUND_UP
#define ROUND_UP(x,y) (((x) + ((y)-1)) & ~((y)-1))
#endif

typedef double DOUBLE;

typedef struct _FX_SAVE_AREA {
    ULONG Fr[32];
} FX_SAVE_AREA, *PFX_SAVE_AREA;

typedef struct _FXSAVE_FORMAT
{
    ULONG Xer,Fpscr;
} FXSAVE_FORMAT, *PFXSAVE_FORMAT;

typedef struct _LDT_ENTRY {
    USHORT LimitLow;
    USHORT BaseLow;
    union
    {
        struct
        {
            UCHAR BaseMid;
            UCHAR Flags1;
            UCHAR Flags2;
            UCHAR BaseHi;
        } Bytes;
        struct
        {
            ULONG BaseMid : 8;
            ULONG Type : 5;
            ULONG Dpl : 2;
            ULONG Pres : 1;
            ULONG LimitHi : 4;
            ULONG Sys : 1;
            ULONG Reserved_0 : 1;
            ULONG Default_Big : 1;
            ULONG Granularity : 1;
            ULONG BaseHi : 8;
        } Bits;
    } HighWord;
} LDT_ENTRY;

#ifndef CONFIG_SMP
#define SYNCH_LEVEL DISPATCH_LEVEL
#else
#define SYNCH_LEVEL (IPI_LEVEL - 1)
#endif

//
// Trap Frame Definition
//
typedef struct _KTRAP_FRAME
{
    PVOID TrapFrame;
    UCHAR OldIrql;
    UCHAR PreviousMode;
    UCHAR SavedApcStateIndex;
    UCHAR SavedKernelApcDisable;
    UCHAR ExceptionRecord[ROUND_UP(sizeof(EXCEPTION_RECORD), sizeof(ULONGLONG))];
    ULONG FILL2;
    ULONG Gpr0;
    ULONG Gpr1;
    ULONG Gpr2;
    ULONG Gpr3;
    ULONG Gpr4;
    ULONG Gpr5;
    ULONG Gpr6;
    ULONG Gpr7;
    ULONG Gpr8;
    ULONG Gpr9;
    ULONG Gpr10;
    ULONG Gpr11;
    ULONG Gpr12;
    DOUBLE Fpr0;
    DOUBLE Fpr1;
    DOUBLE Fpr2;
    DOUBLE Fpr3;
    DOUBLE Fpr4;
    DOUBLE Fpr5;
    DOUBLE Fpr6;
    DOUBLE Fpr7;
    DOUBLE Fpr8;
    DOUBLE Fpr9;
    DOUBLE Fpr10;
    DOUBLE Fpr11;
    DOUBLE Fpr12;
    DOUBLE Fpr13;
    DOUBLE Fpscr;
    ULONG Cr;
    ULONG Xer;
    ULONG Msr;
    ULONG Iar;
    ULONG Lr;
    ULONG Ctr;
    ULONG Dr0;
    ULONG Dr1;
    ULONG Dr2;
    ULONG Dr3;
    ULONG Dr4;
    ULONG Dr5;
    ULONG Dr6;
    ULONG Dr7;
} KTRAP_FRAME, *PKTRAP_FRAME;

//
// GDT Entry Definition
//
typedef struct _KGDTENTRY
{
    USHORT LimitLow;
    USHORT BaseLow;
    union
    {
        struct
        {
            UCHAR BaseMid;
            UCHAR Flags1;
            UCHAR Flags2;
            UCHAR BaseHi;
        } Bytes;
        struct
        {
            ULONG BaseMid:8;
            ULONG Type:5;
            ULONG Dpl:2;
            ULONG Pres:1;
            ULONG LimitHi:4;
            ULONG Sys:1;
            ULONG Reserved_0:1;
            ULONG Default_Big:1;
            ULONG Granularity:1;
            ULONG BaseHi:8;
        } Bits;
    } HighWord;
} KGDTENTRY, *PKGDTENTRY;

//
// IDT Entry Definition
//
typedef struct _KIDTENTRY
{
    USHORT Offset;
    USHORT Selector;
    USHORT Access;
    USHORT ExtendedOffset;
} KIDTENTRY, *PKIDTENTRY;

typedef struct _DESCRIPTOR
{
    USHORT Pad;
    USHORT Limit;
    ULONG Base;
} KDESCRIPTOR, *PKDESCRIPTOR;

//
// Special Registers Structure (outside of CONTEXT)
//
typedef struct _KSPECIAL_REGISTERS
{
    ULONG KernelDr0;
    ULONG KernelDr1;
    ULONG KernelDr2;
    ULONG KernelDr3;
    ULONG KernelDr4;
    ULONG KernelDr5;
    ULONG KernelDr6;
    ULONG KernelDr7;
    ULONG Sprg0;
    ULONG Sprg1;
    ULONG Sr0;
    ULONG Sr1;
    ULONG Sr2;
    ULONG Sr3;
    ULONG Sr4;
    ULONG Sr5;
    ULONG Sr6;
    ULONG Sr7;
    ULONG Sr8;
    ULONG Sr9;
    ULONG Sr10;
    ULONG Sr11;
    ULONG Sr12;
    ULONG Sr13;
    ULONG Sr14;
    ULONG Sr15;
    ULONG DBAT0L;
    ULONG DBAT0U;
    ULONG DBAT1L;
    ULONG DBAT1U;
    ULONG DBAT2L;
    ULONG DBAT2U;
    ULONG DBAT3L;
    ULONG DBAT3U;
    ULONG IBAT0L;
    ULONG IBAT0U;
    ULONG IBAT1L;
    ULONG IBAT1U;
    ULONG IBAT2L;
    ULONG IBAT2U;
    ULONG IBAT3L;
    ULONG IBAT3U;
    ULONG Sdr1;
    ULONG Reserved[9];
} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;

//
// Processor State Data
//
#pragma pack(push,4)
typedef struct _KPROCESSOR_STATE
{
    CONTEXT ContextFrame;
    KSPECIAL_REGISTERS SpecialRegisters;
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;

//
// Processor Region Control Block
//
typedef struct _KPRCB
{
    USHORT MinorVersion;
    USHORT MajorVersion;
    struct _KTHREAD *CurrentThread;
    struct _KTHREAD *NextThread;
    struct _KTHREAD *IdleThread;
    UCHAR Number;
    UCHAR Reserved;
    USHORT BuildType;
    KAFFINITY SetMember;
    UCHAR CpuType;
    UCHAR CpuID;
    USHORT CpuStep;
    KPROCESSOR_STATE ProcessorState;
    ULONG KernelReserved[16];
    ULONG HalReserved[16];
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG CFlushSize;
    UCHAR PrcbPad0[88];
#else
    UCHAR PrcbPad0[92];
#endif
    KSPIN_LOCK_QUEUE LockQueue[LockQueueMaximumLock];
    struct _KTHREAD *NpxThread;
    ULONG InterruptCount;
    ULONG KernelTime;
    ULONG UserTime;
    ULONG DpcTime;
    ULONG DebugDpcTime;
    ULONG InterruptTime;
    ULONG AdjustDpcThreshold;
    ULONG PageColor;
    UCHAR SkipTick;
    UCHAR DebuggerSavedIRQL;
#if (NTDDI_VERSION >= NTDDI_WS03)
    UCHAR NodeColor;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    UCHAR PollSlot;
#else
    UCHAR Spare1;
#endif
    ULONG NodeShiftedColor;
#else
    UCHAR Spare1[6];
#endif
    struct _KNODE *ParentNode;
    ULONG MultiThreadProcessorSet;
    struct _KPRCB *MultiThreadSetMaster;
#if (NTDDI_VERSION >= NTDDI_WS03)
    ULONG SecondaryColorMask;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG DpcTimeLimit;
#else
    LONG Sleeping;
#endif
#else
    ULONG ThreadStartCount[2];
#endif
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadNotPossible;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    ULONG KeAlignmentFixupCount;
#endif
    ULONG SpareCounter0;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    ULONG KeDcacheFlushCount;
    ULONG KeExceptionDispatchCount;
    ULONG KeFirstLevelTbFills;
    ULONG KeFloatingEmulationCount;
    ULONG KeIcacheFlushCount;
    ULONG KeSecondLevelTbFills;
    ULONG KeSystemCalls;
#endif
    volatile ULONG IoReadOperationCount;
    volatile ULONG IoWriteOperationCount;
    volatile ULONG IoOtherOperationCount;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
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
    ULONG KeAlignmentFixupCount;
    ULONG KeExceptionDispatchCount;
    ULONG KeSystemCalls;
    ULONG PrcbPad1[3];
#else
    ULONG SpareCounter1[8];
#endif
    PP_LOOKASIDE_LIST PPLookasideList[16];
    PP_LOOKASIDE_LIST PPNPagedLookasideList[32];
    PP_LOOKASIDE_LIST PPPagedLookasideList[32];
    volatile ULONG PacketBarrier;
    volatile ULONG ReverseStall;
    PVOID IpiFrame;
    UCHAR PrcbPad2[52];
    volatile PVOID CurrentPacket[3];
    volatile ULONG TargetSet;
    volatile PKIPI_WORKER WorkerRoutine;
    volatile ULONG IpiFrozen;
    UCHAR PrcbPad3[40];
    volatile ULONG RequestSummary;
    volatile struct _KPRCB *SignalDone;
    UCHAR PrcbPad4[56];
    struct _KDPC_DATA DpcData[2];
    PVOID DpcStack;
    ULONG MaximumDpcQueueDepth;
    ULONG DpcRequestRate;
    ULONG MinimumDpcRate;
    volatile UCHAR DpcInterruptRequested;
    volatile UCHAR DpcThreadRequested;
    volatile UCHAR DpcRoutineActive;
    volatile UCHAR DpcThreadActive;
    ULONG PrcbLock;
    ULONG DpcLastCount;
    volatile ULONG TimerHand;
    volatile ULONG TimerRequest;
    PVOID DpcThread;
    KEVENT DpcEvent;
    UCHAR ThreadDpcEnable;
    volatile BOOLEAN QuantumEnd;
    UCHAR PrcbPad50;
    volatile UCHAR IdleSchedule;
    LONG DpcSetEventRequest;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    LONG Sleeping;
    ULONG PeriodicCount;
    ULONG PeriodicBias;
    UCHAR PrcbPad5[6];
#else
    UCHAR PrcbPad5[18];
#endif
    LONG TickOffset;
    KDPC CallDpc;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    LONG ClockKeepAlive;
    UCHAR ClockCheckSlot;
    UCHAR ClockPollCycle;
    UCHAR PrcbPad6[2];
    LONG DpcWatchdogPeriod;
    LONG DpcWatchDogCount;
    LONG ThreadWatchdogPeriod;
    LONG ThreadWatchDogCount;
    ULONG PrcbPad70[2];
#else
    ULONG PrcbPad7[8];
#endif
    LIST_ENTRY WaitListHead;
    ULONG ReadySummary;
    ULONG QueueIndex;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    SINGLE_LIST_ENTRY DeferredReadyListHead;
    ULONGLONG StartCycles;
    ULONGLONG CycleTime;
    ULONGLONG PrcbPad71[3];
    LIST_ENTRY DispatcherReadyListHead[32];
#else
    LIST_ENTRY DispatcherReadyListHead[32];
    SINGLE_LIST_ENTRY DeferredReadyListHead;
    ULONG PrcbPad72[11];
#endif
    PVOID ChainedInterruptList;
    LONG LookasideIrpFloat;
    volatile LONG MmPageFaultCount;
    volatile LONG MmCopyOnWriteCount;
    volatile LONG MmTransitionCount;
    volatile LONG MmCacheTransitionCount;
    volatile LONG MmDemandZeroCount;
    volatile LONG MmPageReadCount;
    volatile LONG MmPageReadIoCount;
    volatile LONG MmCacheReadCount;
    volatile LONG MmCacheIoCount;
    volatile LONG MmDirtyPagesWriteCount;
    volatile LONG MmDirtyWriteIoCount;
    volatile LONG MmMappedPagesWriteCount;
    volatile LONG MmMappedWriteIoCount;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG CachedCommit;
    ULONG CachedResidentAvailable;
    PVOID HyperPte;
    UCHAR CpuVendor;
    UCHAR PrcbPad9[3];
#else
    ULONG SpareFields0[1];
#endif
    CHAR VendorString[13];
    UCHAR InitialApicId;
    UCHAR LogicalProcessorsPerPhysicalProcessor;
    ULONG MHz;
    ULONG FeatureBits;
    LARGE_INTEGER UpdateSignature;
    volatile LARGE_INTEGER IsrTime;
    LARGE_INTEGER SpareField1;
    FX_SAVE_AREA NpxSaveArea;
    PROCESSOR_POWER_STATE PowerState;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    KDPC DpcWatchdogDoc;
    KTIMER DpcWatchdogTimer;
    PVOID WheaInfo;
    PVOID EtwSupport;
    SLIST_HEADER InterruptObjectPool;
    LARGE_INTEGER HyperCallPagePhysical;
    LARGE_INTEGER HyperCallPageVirtual;
    PVOID RateControl;
    CACHE_DESCRIPTOR Cache[5];
    ULONG CacheCount;
    ULONG CacheProcessorMask[5];
    UCHAR LogicalProcessorsPerCore;
    UCHAR PrcbPad8[3];
    ULONG PackageProcessorSet;
    ULONG CoreProcessorSet;
#endif
} KPRCB, *PKPRCB;

//
// Processor Control Region
//
typedef struct _KIPCR
{
    USHORT MinorVersion;
    USHORT MajorVersion;
    PKINTERRUPT_ROUTINE InterruptRoutine[MAXIMUM_VECTOR];
    ULONG PcrPage2;
    ULONG Kseg0Top;
    ULONG Spare7[30];
    ULONG FirstLevelDcacheSize;
    ULONG FirstLevelDcacheFillSize;
    ULONG FirstLevelIcacheSize;
    ULONG FirstLevelIcacheFillSize;
    ULONG SecondLevelDcacheSize;
    ULONG SecondLevelDcacheFillSize;
    ULONG SecondLevelIcacheSize;
    ULONG SecondLevelIcacheFillSize;
    struct _KPRCB *PrcbData;
    PVOID Teb;
    ULONG DcacheAlignment;
    ULONG DcacheFillSize;
    ULONG IcacheAlignment;
    ULONG IcacheFillSize;
    ULONG ProcessorVersion;
    ULONG ProcessorRevision;
    ULONG ProfileInterval;
    ULONG ProfileCount;
    ULONG StallExecutionCount;
    ULONG StallScaleFactor;
    ULONG Spare;
    union
    {
        ULONG CachePolicy;
        struct
        {
            UCHAR IcacheMode;
            UCHAR DcacheMode;
            USHORT ModeSpare;
        };
    };
    UCHAR IrqlMask[32];
    UCHAR IrqlTable[9];
    UCHAR CurrentIrql;
    CCHAR Number;
    KAFFINITY SetMember;
    ULONG ReservedVectors;
    struct _KTHREAD *CurrentThread;
    ULONG AlignedCachePolicy;
    union
    {
        ULONG SoftwareInterrupt;
        struct
        {
            UCHAR ApcInterrupt;
            UCHAR DispatchInterrupt;
            UCHAR Spare4;
            UCHAR Spare5;
        };
    };
    KAFFINITY NotMember;
    ULONG SystemReserved[16];
    ULONG HalReserved[16];
    ULONG FirstLevelActive;
    ULONG SystemServiceDispatchStart;
    ULONG SystemServiceDispatchEnd;
    ULONG InterruptStack;
    ULONG QuantumEnd;
    PVOID InitialStack;
    PVOID PanicStack;
    ULONG BadVaddr;
    PVOID StackLimit;
    PVOID SavedStackLimit;
    ULONG SavedV0;
    ULONG SavedV1;
    UCHAR DebugActive;
    UCHAR Spare6[3];
    ULONG GprSave[6];
    ULONG SiR0;
    ULONG SiR2;
    ULONG SiR3;
    ULONG SiR4;
    ULONG SiR5;
    ULONG Spare0;
    ULONG Spare8;
    ULONG PgDirRa;
    ULONG OnInterruptStack;
    ULONG SavedInitialStack;
} KIPCR, *PKIPCR;
#pragma pack(pop)

//
// TSS Definition
//
typedef struct _KTSS {
} KTSS, *PKTSS;

//
// PowerPC Exception Frame
//
typedef struct _KEXCEPTION_FRAME
{
    ULONG Fill1;
    ULONG Gpr13;
    ULONG Gpr14;
    ULONG Gpr15;
    ULONG Gpr16;
    ULONG Gpr17;
    ULONG Gpr18;
    ULONG Gpr19;
    ULONG Gpr20;
    ULONG Gpr21;
    ULONG Gpr22;
    ULONG Gpr23;
    ULONG Gpr24;
    ULONG Gpr25;
    ULONG Gpr26;
    ULONG Gpr27;
    ULONG Gpr28;
    ULONG Gpr29;
    ULONG Gpr30;
    ULONG Gpr31;
    DOUBLE Fpr14;
    DOUBLE Fpr15;
    DOUBLE Fpr16;
    DOUBLE Fpr17;
    DOUBLE Fpr18;
    DOUBLE Fpr19;
    DOUBLE Fpr20;
    DOUBLE Fpr21;
    DOUBLE Fpr22;
    DOUBLE Fpr23;
    DOUBLE Fpr24;
    DOUBLE Fpr25;
    DOUBLE Fpr26;
    DOUBLE Fpr27;
    DOUBLE Fpr28;
    DOUBLE Fpr29;
    DOUBLE Fpr30;
    DOUBLE Fpr31;
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

FORCEINLINE
struct _KPRCB *
KeGetCurrentPrcb(VOID)
{
    return (struct _KPRCB *)(ULONG_PTR)__readfsdword(FIELD_OFFSET(KIPCR, PrcbData));
}

#endif
