/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    ketypes.h (X86)

Abstract:

    i386 Type definitions for the Kernel services.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006

--*/

#ifndef _I386_KETYPES_H
#define _I386_KETYPES_H

//
// Dependencies
//

//
// KPCR Access for non-IA64 builds
//
#define K0IPCR                  ((ULONG_PTR)(KIP0PCRADDRESS))
#define PCR                     ((KPCR *)K0IPCR)
#if defined(CONFIG_SMP) || defined(NT_BUILD)
#undef  KeGetPcr
#define KeGetPcr()              ((KPCR *)__readfsdword(FIELD_OFFSET(KPCR, SelfPcr)))
#endif

//
// CPU Vendors
//
typedef enum
{
    CPU_NONE      = 0,
    CPU_INTEL     = 1,
    CPU_AMD       = 2,
    CPU_CYRIX     = 3,
    CPU_TRANSMETA = 4,
    CPU_VIA       = 5,
    CPU_CENTAUR   = CPU_VIA,
    CPU_RISE      = 6,
    CPU_UNKNOWN   = 7
} CPU_VENDORS;

//
// Machine Types
//
#define MACHINE_TYPE_ISA        0x0000
#define MACHINE_TYPE_EISA       0x0001
#define MACHINE_TYPE_MCA        0x0002

//
// X86 80386 Segment Types
//
#define I386_TASK_GATE          0x5
#define I386_TSS                0x9
#define I386_ACTIVE_TSS         0xB
#define I386_CALL_GATE          0xC
#define I386_INTERRUPT_GATE     0xE
#define I386_TRAP_GATE          0xF

//
// Selector Names
//
#define RPL_MASK                0x0003
#define MODE_MASK               0x0001
#define KGDT_NULL               0x00
#define KGDT_R0_CODE            0x08
#define KGDT_R0_DATA            0x10
#define KGDT_R3_CODE            0x18
#define KGDT_R3_DATA            0x20
#define KGDT_TSS                0x28
#define KGDT_R0_PCR             0x30
#define KGDT_R3_TEB             0x38
#define KGDT_VDM_TILE           0x40
#define KGDT_LDT                0x48
#define KGDT_DF_TSS             0x50
#define KGDT_NMI_TSS            0x58

//
// Define the number of GDTs that can be queried by user mode
//
#define KGDT_NUMBER             10

//
// CR4
//
#define CR4_VME                 0x1
#define CR4_PVI                 0x2
#define CR4_TSD                 0x4
#define CR4_DE                  0x8
#define CR4_PSE                 0x10
#define CR4_PAE                 0x20
#define CR4_MCE                 0x40
#define CR4_PGE                 0x80
#define CR4_FXSR                0x200
#define CR4_XMMEXCPT            0x400

//
// EFlags
//
#define EFLAGS_CF               0x01L
#define EFLAGS_ZF               0x40L
#define EFLAGS_TF               0x100L
#define EFLAGS_INTERRUPT_MASK   0x200L
#define EFLAGS_DF               0x400L
#define EFLAGS_IOPL             0x3000L
#define EFLAGS_NESTED_TASK      0x4000L
#define EFLAGS_RF               0x10000
#define EFLAGS_V86_MASK         0x20000
#define EFLAGS_ALIGN_CHECK      0x40000
#define EFLAGS_VIF              0x80000
#define EFLAGS_VIP              0x100000
#define EFLAGS_ID               0x200000
#define EFLAGS_USER_SANITIZE    0x3F4DD7
#define EFLAG_SIGN              0x8000
#define EFLAG_ZERO              0x4000

//
// Legacy floating status word bit masks.
//
#define FSW_INVALID_OPERATION   0x1
#define FSW_DENORMAL            0x2
#define FSW_ZERO_DIVIDE         0x4
#define FSW_OVERFLOW            0x8
#define FSW_UNDERFLOW           0x10
#define FSW_PRECISION           0x20
#define FSW_STACK_FAULT         0x40

//
// Machine Specific Registers
//
#define MSR_AMD_ACCESS          0x9C5A203A
#define MSR_IA32_MISC_ENABLE    0x01A0
#define MSR_EFER                0xC0000080

//
// MSR internal Values
//
#define MSR_NXE                 0x0800
#define XHF_NOEXECUTE           0x100000
#define MSR_XD_ENABLE_MASK      0xFFFFFFFB

//
// IPI Types
//
#define IPI_APC                 1
#define IPI_DPC                 2
#define IPI_FREEZE              4
#define IPI_PACKET_READY        8
#define IPI_SYNCH_REQUEST       16

//
// PRCB Flags
//
#define PRCB_MINOR_VERSION      1
#define PRCB_MAJOR_VERSION      1
#define PRCB_BUILD_DEBUG        1
#define PRCB_BUILD_UNIPROCESSOR 2

//
// HAL Variables
//
#define INITIAL_STALL_COUNT     100
#define MM_HAL_VA_START         0xFFC00000
#define MM_HAL_VA_END           0xFFFFFFFF
#define APIC_BASE               0xFFFE0000

//
// IOPM Definitions
//
#define IOPM_COUNT              1
#define IOPM_SIZE               8192
#define IOPM_FULL_SIZE          8196
#define IO_ACCESS_MAP_NONE      0
#define IOPM_DIRECTION_MAP_SIZE 32
#define IOPM_OFFSET             FIELD_OFFSET(KTSS, IoMaps[0].IoMap)
#define KiComputeIopmOffset(MapNumber)              \
    (MapNumber == IO_ACCESS_MAP_NONE) ?             \
        (USHORT)(sizeof(KTSS)) :                    \
        (USHORT)(FIELD_OFFSET(KTSS, IoMaps[MapNumber-1].IoMap))

typedef UCHAR KIO_ACCESS_MAP[IOPM_SIZE];

typedef KIO_ACCESS_MAP *PKIO_ACCESS_MAP;

//
// Size of the XMM register save area in the FXSAVE format
//
#define SIZE_OF_FX_REGISTERS    128

//
// Static Kernel-Mode Address start (use MM_KSEG0_BASE for actual)
//
#define KSEG0_BASE              0x80000000

//
// Synchronization-level IRQL
//
#ifndef CONFIG_SMP
#define SYNCH_LEVEL             DISPATCH_LEVEL
#else
#if (NTDDI_VERSION < NTDDI_WS03)
#define SYNCH_LEVEL             (IPI_LEVEL - 1)
#else
#define SYNCH_LEVEL             (IPI_LEVEL - 2)
#endif
#endif

//
// Number of pool lookaside lists per pool in the PRCB
//
#define NUMBER_POOL_LOOKASIDE_LISTS 32

//
// Structure for CPUID
//
typedef union _CPU_INFO
{
    UINT32 AsUINT32[4];
    struct
    {
        ULONG Eax;
        ULONG Ebx;
        ULONG Ecx;
        ULONG Edx;
    };
} CPU_INFO, *PCPU_INFO;

//
// Trap Frame Definition
//
typedef struct _KTRAP_FRAME
{
    ULONG DbgEbp;
    ULONG DbgEip;
    ULONG DbgArgMark;
    ULONG DbgArgPointer;
    ULONG TempSegCs;
    ULONG TempEsp;
    ULONG Dr0;
    ULONG Dr1;
    ULONG Dr2;
    ULONG Dr3;
    ULONG Dr6;
    ULONG Dr7;
    ULONG SegGs;
    ULONG SegEs;
    ULONG SegDs;
    ULONG Edx;
    ULONG Ecx;
    ULONG Eax;
    ULONG PreviousPreviousMode;
    struct _EXCEPTION_REGISTRATION_RECORD FAR *ExceptionList;
    ULONG SegFs;
    ULONG Edi;
    ULONG Esi;
    ULONG Ebx;
    ULONG Ebp;
    ULONG ErrCode;
    ULONG Eip;
    ULONG SegCs;
    ULONG EFlags;
    ULONG HardwareEsp;
    ULONG HardwareSegSs;
    ULONG V86Es;
    ULONG V86Ds;
    ULONG V86Fs;
    ULONG V86Gs;
} KTRAP_FRAME, *PKTRAP_FRAME;

//
// Defines the Callback Stack Layout for User Mode Callbacks
//
typedef struct _KCALLOUT_FRAME
{
    ULONG InitialStack;
    ULONG TrapFrame;
    ULONG CallbackStack;
    ULONG Edi;
    ULONG Esi;
    ULONG Ebx;
    ULONG Ebp;
    ULONG ReturnAddress;
    ULONG Result;
    ULONG ResultLength;
} KCALLOUT_FRAME, *PKCALLOUT_FRAME;

//
// LDT Entry Definition
//
#ifndef _LDT_ENTRY_DEFINED
#define _LDT_ENTRY_DEFINED
typedef struct _LDT_ENTRY
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
} LDT_ENTRY, *PLDT_ENTRY, *LPLDT_ENTRY;
#endif

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
// IDT Entry Access Definition
//
typedef struct _KIDT_ACCESS
{
    union
    {
        struct
        {
            UCHAR Reserved;
            UCHAR SegmentType:4;
            UCHAR SystemSegmentFlag:1;
            UCHAR Dpl:2;
            UCHAR Present:1;
        };
        USHORT Value;
    };
} KIDT_ACCESS, *PKIDT_ACCESS;

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

#ifndef NTOS_MODE_USER
//
// Macro to get current KPRCB
//
FORCEINLINE
struct _KPRCB *
KeGetCurrentPrcb(VOID)
{
    return (struct _KPRCB *)(ULONG_PTR)__readfsdword(FIELD_OFFSET(KPCR, Prcb));
}

//
// FN/FX (FPU) Save Area Structures
//
typedef struct _FNSAVE_FORMAT
{
    ULONG ControlWord;
    ULONG StatusWord;
    ULONG TagWord;
    ULONG ErrorOffset;
    ULONG ErrorSelector;
    ULONG DataOffset;
    ULONG DataSelector;
    UCHAR RegisterArea[80];
} FNSAVE_FORMAT, *PFNSAVE_FORMAT;

typedef struct _FXSAVE_FORMAT
{
    USHORT ControlWord;
    USHORT StatusWord;
    USHORT TagWord;
    USHORT ErrorOpcode;
    ULONG ErrorOffset;
    ULONG ErrorSelector;
    ULONG DataOffset;
    ULONG DataSelector;
    ULONG MXCsr;
    ULONG MXCsrMask;
    UCHAR RegisterArea[SIZE_OF_FX_REGISTERS];
    UCHAR Reserved3[128];
    UCHAR Reserved4[224];
    UCHAR Align16Byte[8];
} FXSAVE_FORMAT, *PFXSAVE_FORMAT;

typedef struct _FX_SAVE_AREA
{
    union
    {
        FNSAVE_FORMAT FnArea;
        FXSAVE_FORMAT FxArea;
    } U;
    ULONG NpxSavedCpu;
    ULONG Cr0NpxState;
} FX_SAVE_AREA, *PFX_SAVE_AREA;

//
// Special Registers Structure (outside of CONTEXT)
//
typedef struct _KSPECIAL_REGISTERS
{
    ULONG Cr0;
    ULONG Cr2;
    ULONG Cr3;
    ULONG Cr4;
    ULONG KernelDr0;
    ULONG KernelDr1;
    ULONG KernelDr2;
    ULONG KernelDr3;
    ULONG KernelDr6;
    ULONG KernelDr7;
    KDESCRIPTOR Gdtr;
    KDESCRIPTOR Idtr;
    USHORT Tr;
    USHORT Ldtr;
    ULONG Reserved[6];
} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;

//
// Processor State Data
//
typedef struct _KPROCESSOR_STATE
{
    CONTEXT ContextFrame;
    KSPECIAL_REGISTERS SpecialRegisters;
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;

//
// Processor Region Control Block
//
#pragma pack(push,4)
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
    PP_LOOKASIDE_LIST PPNPagedLookasideList[NUMBER_POOL_LOOKASIDE_LISTS];
    PP_LOOKASIDE_LIST PPPagedLookasideList[NUMBER_POOL_LOOKASIDE_LISTS];
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
    struct _KPCR *SelfPcr;
    struct _KPRCB *Prcb;
    KIRQL Irql;
    ULONG IRR;
    ULONG IrrActive;
    ULONG IDR;
    PVOID KdVersionBlock;
    PKIDTENTRY IDT;
    PKGDTENTRY GDT;
    struct _KTSS *TSS;
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
    ULONG InterruptMode;
    UCHAR Spare1;
    ULONG KernelReserved2[17];
    KPRCB PrcbData;
} KIPCR, *PKIPCR;
#pragma pack(pop)

//
// TSS Definition
//
typedef struct _KiIoAccessMap
{
    UCHAR DirectionMap[IOPM_DIRECTION_MAP_SIZE];
    UCHAR IoMap[IOPM_FULL_SIZE];
} KIIO_ACCESS_MAP;

typedef struct _KTSS
{
    USHORT Backlink;
    USHORT Reserved0;
    ULONG Esp0;
    USHORT Ss0;
    USHORT Reserved1;
    ULONG NotUsed1[4];
    ULONG CR3;
    ULONG Eip;
    ULONG EFlags;
    ULONG Eax;
    ULONG Ecx;
    ULONG Edx;
    ULONG Ebx;
    ULONG Esp;
    ULONG Ebp;
    ULONG Esi;
    ULONG Edi;
    USHORT Es;
    USHORT Reserved2;
    USHORT Cs;
    USHORT Reserved3;
    USHORT Ss;
    USHORT Reserved4;
    USHORT Ds;
    USHORT Reserved5;
    USHORT Fs;
    USHORT Reserved6;
    USHORT Gs;
    USHORT Reserved7;
    USHORT LDT;
    USHORT Reserved8;
    USHORT Flags;
    USHORT IoMapBase;
    KIIO_ACCESS_MAP IoMaps[IOPM_COUNT];
    UCHAR IntDirectionMap[IOPM_DIRECTION_MAP_SIZE];
} KTSS, *PKTSS;

//
// i386 CPUs don't have exception frames
//
typedef struct _KEXCEPTION_FRAME KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

#endif /* NTOS_MODE_USER */

typedef enum _WOW64_SERVICE_TYPES
{
    ServiceNoTurbo = 0x0000,
    Service0Arg = 0x0001,
    Service0ArgReloadState = 0x0002,
    Service1ArgSp = 0x0003,
    Service1ArgNSp = 0x0004,
    Service2ArgNSpNSp = 0x0005,
    Service2ArgNSpNSpReloadState = 0x0006,
    Service2ArgSpNSp = 0x0007,
    Service2ArgSpSp = 0x0008,
    Service2ArgNSpSp = 0x0009,
    Service3ArgNSpNSpNSp = 0x000A,
    Service3ArgSpSpSp = 0x000B,
    Service3ArgSpNSpNSp = 0x000C,
    Service3ArgSpNSpNSpReloadState = 0x000D,
    Service3ArgSpSpNSp = 0x000E,
    Service3ArgNSpSpNSp = 0x000F,
    Service3ArgSpNSpSp = 0x0010,
    Service4ArgNSpNSpNSpNSp = 0x0011,
    Service4ArgSpSpNSpNSp = 0x0012,
    Service4ArgSpSpNSpNSpReloadState = 0x0013,
    Service4ArgSpNSpNSpNSp = 0x0014,
    Service4ArgSpNSpNSpNSpReloadState = 0x0015,
    Service4ArgNSpSpNSpNSp = 0x0016,
    Service4ArgSpSpSpNSp = 0x0017,
    ServiceCpupTdQuerySystemTime = 0x0018,
    ServiceCpupTdGetCurrentProcessorNumber = 0x0019,
    ServiceCpupTdReadWriteFile = 0x001A,
    ServiceCpupTdDeviceIoControlFile = 0x001B,
    ServiceCpupTdRemoveIoCompletion = 0x001C,
    ServiceCpupTdWaitForMultipleObjects = 0x001D,
    ServiceCpupTdWaitForMultipleObjects32 = 0x001E,
    Wow64ServiceTypesCount // = 0x001F
} WOW64_SERVICE_TYPES, *PWOW64_SERVICE_TYPES;

typedef enum _VDM_INDEX
{
    VDM_INDEX_Invalid = 0,
    VDM_INDEX_0F,
    VDM_INDEX_ESPrefix,
    VDM_INDEX_CSPrefix,
    VDM_INDEX_SSPrefix,
    VDM_INDEX_DSPrefix,
    VDM_INDEX_FSPrefix,
    VDM_INDEX_GSPrefix,
    VDM_INDEX_OPER32Prefix,
    VDM_INDEX_ADDR32Prefix,
    VDM_INDEX_INSB,
    VDM_INDEX_INSW,
    VDM_INDEX_OUTSB,
    VDM_INDEX_OUTSW,
    VDM_INDEX_PUSHF,
    VDM_INDEX_POPF,
    VDM_INDEX_INTnn,
    VDM_INDEX_INTO,
    VDM_INDEX_IRET,
    VDM_INDEX_NPX,
    VDM_INDEX_INBimm,
    VDM_INDEX_INWimm,
    VDM_INDEX_OUTBimm,
    VDM_INDEX_OUTWimm,
    VDM_INDEX_INB,
    VDM_INDEX_INW,
    VDM_INDEX_OUTB,
    VDM_INDEX_OUTW,
    VDM_INDEX_LOCKPrefix,
    VDM_INDEX_REPNEPrefix,
    VDM_INDEX_REPPrefix,
    VDM_INDEX_CLI,
    VDM_INDEX_STI,
    VDM_INDEX_HLT,
    MAX_VDM_INDEX
} VDM_INDEX, *PVDM_INDEX;

#endif /* _I386_KETYPES_H */
