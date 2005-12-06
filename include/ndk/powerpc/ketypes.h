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
    UCHAR ExceptionRecord[ROUND_UP(sizeof(EXCEPTION_RECORD), ULONGLONG];
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
// Page Table Entry Definition
//
typedef struct _HARDWARE_PTE_PPC
{
    ULONG Dirty:2;
    ULONG Valid:1;
    ULONG GuardedStorage:1;
    ULONG MemoryCoherence:1;
    ULONG CacheDisable:1;
    ULONG WriteThrough:1;
    ULONG Change:1;
    ULONG Reference:1;
    ULONG Write:1;
    ULONG CopyOnWrite:1;
    ULONG rsvd1:1;
    ULONG PageFrameNumber:20;
} HARDWARE_PTE_PPC, *PHARDWARE_PTE_PPC;

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
    PCONTEXT ContextFrame;
    KSPECIAL_REGISTERS SpecialRegisters;
} KPROCESSOR_STATE;

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
    UCHAR PrcbPad0[92];
    PVOID LockQueue[33]; // Used for Queued Spinlocks
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
    UCHAR Spare1[6];
    struct _KNODE *ParentNode;
    ULONG MultiThreadProcessorSet;
    struct _KPRCB *MultiThreadSetMaster;
    ULONG ThreadStartCount[2];
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadNotPossible;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    ULONG KeAlignmentFixupCount;
    ULONG KeContextSwitches;
    ULONG KeDcacheFlushCount;
    ULONG KeExceptionDispatchCount;
    ULONG KeFirstLevelTbFills;
    ULONG KeFloatingEmulationCount;
    ULONG KeIcacheFlushCount;
    ULONG KeSecondLevelTbFills;
    ULONG KeSystemCalls;
    ULONG IoReadOperationCount;
    ULONG IoWriteOperationCount;
    ULONG IoOtherOperationCount;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
    ULONG SpareCounter1[8];
    PP_LOOKASIDE_LIST PPLookasideList[16];
    PP_LOOKASIDE_LIST PPNPagedLookasideList[32];
    PP_LOOKASIDE_LIST PPPagedLookasideList[32];
    ULONG PacketBarrier;
    ULONG ReverseStall;
    PVOID IpiFrame;
    UCHAR PrcbPad2[52];
    PVOID CurrentPacket[3];
    ULONG TargetSet;
    ULONG_PTR WorkerRoutine;
    ULONG IpiFrozen;
    UCHAR PrcbPad3[40];
    ULONG RequestSummary;
    struct _KPRCB *SignalDone;
    UCHAR PrcbPad4[56];
    struct _KDPC_DATA DpcData[2];
    PVOID DpcStack;
    ULONG MaximumDpcQueueDepth;
    ULONG DpcRequestRate;
    ULONG MinimumDpcRate;
    UCHAR DpcInterruptRequested;
    UCHAR DpcThreadRequested;
    UCHAR DpcRoutineActive;
    UCHAR DpcThreadActive;
    ULONG PrcbLock;
    ULONG DpcLastCount;
    ULONG TimerHand;
    ULONG TimerRequest;
    PVOID DpcThread;
    struct _KEVENT *DpcEvent;
    UCHAR ThreadDpcEnable;
    BOOLEAN QuantumEnd;
    UCHAR PrcbPad50;
    UCHAR IdleSchedule;
    ULONG DpcSetEventRequest;
    UCHAR PrcbPad5[18];
    LONG TickOffset;
    struct _KDPC* CallDpc;
    ULONG PrcbPad7[8];
    LIST_ENTRY WaitListHead;
    ULONG ReadySummary;
    ULONG SelectNextLast;
    LIST_ENTRY DispatcherReadyListHead[32];
    SINGLE_LIST_ENTRY DeferredReadyListHead;
    ULONG PrcbPad72[11];
    PVOID ChainedInterruptList;
    LONG LookasideIrpFloat;
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
    ULONG SpareFields0[1];
    CHAR VendorString[13];
    UCHAR InitialApicId;
    UCHAR LogicalProcessorsPerPhysicalProcessor;
    ULONG MHz;
    ULONG FeatureBits;
    LARGE_INTEGER UpdateSignature;
    LARGE_INTEGER IsrTime;
    LARGE_INTEGER SpareField1;
    FX_SAVE_AREA NpxSaveArea;
    PROCESSOR_POWER_STATE PowerState;
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
    struct _KPRCB *Prcb;
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
typedef struct _KTSS, KTSS, *PKTSS;

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

#endif
