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
	ULONG R[32];
	ULONG SRR0, SRR1;
	ULONG LR, CTR;
} KTRAP_FRAME, *PKTRAP_FRAME;

//
// Page Table Entry Definition
//
// I'll use the same table format
//
typedef struct _SOFTWARE_PTE_PPC
{
    ULONG Valid             : 1;
    ULONG Write             : 1;
    ULONG Owner             : 1;
    ULONG WriteThrough      : 1;
    ULONG CacheDisable      : 1;
    ULONG Accessed          : 1;
    ULONG Dirty             : 1;
    ULONG LargePage         : 1;
    ULONG Global            : 1;
    ULONG CopyOnWrite       : 1;
    ULONG Prototype         : 1;
    ULONG reserved          : 1;
    ULONG PageFrameNumber   : 20;
} SOFTWARE_PTE_X86, *PSOFTWARE_PTE_PPC;

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
	ULONG MSR, SDR0, SDR1;
	ULONG BATU[4], BATL[4];
	ULONG SR[8];
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
    union
    {
        NT_TIB NtTib;
        struct 
        {
            struct _EXCEPTION_REGISTRATION_RECORD *Used_ExceptionList;
            PVOID Used_StackBase;
            PVOID PerfGlobalGroupMask;
            PVOID TssCopy;
            ULONG ContextSwitches;
            KAFFINITY SetMemberCopy;
            PVOID Used_Self;
        };
    };
    struct _KPCR *Self;          /* 1C */
    struct _KPRCB *Prcb;         /* 20 */
    KIRQL Irql;                  /* 24 */
    ULONG IRR;                   /* 28 */
    ULONG IrrActive;             /* 2C */
    ULONG IDR;                   /* 30 */
    PVOID KdVersionBlock;        /* 34 */
    struct _KTSS *TSS;           /* 40 */
    USHORT MajorVersion;         /* 44 */
    USHORT MinorVersion;         /* 46 */
    KAFFINITY SetMember;         /* 48 */
    ULONG StallScaleFactor;      /* 4C */
    UCHAR SparedUnused;          /* 50 */
    UCHAR Number;                /* 51 */
    UCHAR Reserved;              /* 52 */
    UCHAR L2CacheAssociativity;  /* 53 */
    ULONG VdmAlert;              /* 54 */
    ULONG KernelReserved[14];    /* 58 */
    ULONG L2CacheSize;           /* 90 */
    ULONG HalReserved[16];       /* 94 */
    ULONG InterruptMode;         /* D4 */
    UCHAR KernelReserved2[0x48]; /* D8 */
    KPRCB PrcbData;              /* 120 */
} KIPCR, *PKIPCR;
#pragma pack(pop)

//
// TSS Definition
//
typedef struct _KiIoAccessMap
{
    UCHAR DirectionMap[32];
    UCHAR IoMap[8196];
} KIIO_ACCESS_MAP;

#include <pshpack1.h>
typedef struct _KTSS
{
    USHORT Backlink;
    USHORT Reserved0;

    KTRAP_FRAME Registers;

    KIIO_ACCESS_MAP IoMaps[1];
    UCHAR IntDirectionMap[32];
} KTSS, *PKTSS;
#include <poppack.h>

//
// PowerPC Exception Frame
//
typedef struct _KEXCEPTION_FRAME {
	
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

#endif
