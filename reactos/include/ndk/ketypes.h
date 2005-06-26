/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/ketypes.h
 * PURPOSE:         Definitions for Kernel Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _KETYPES_H
#define _KETYPES_H

/* DEPENDENCIES **************************************************************/
#include "haltypes.h"
#include "potypes.h"
#include "mmtypes.h"
#include <arc/arc.h>

/* CONSTANTS *****************************************************************/
#define SSDT_MAX_ENTRIES 4
#define PROCESSOR_FEATURE_MAX 64

#define CONTEXT_DEBUGGER (CONTEXT_FULL | CONTEXT_FLOATING_POINT)

#define THREAD_WAIT_OBJECTS 4

/* EXPORTED DATA *************************************************************/
extern CHAR NTOSAPI KeNumberProcessors;
extern LOADER_PARAMETER_BLOCK NTOSAPI KeLoaderBlock;
extern ULONG NTOSAPI KeDcacheFlushCount;
extern ULONG NTOSAPI KeIcacheFlushCount;
extern KAFFINITY NTOSAPI KeActiveProcessors;
extern ULONG NTOSAPI KiDmaIoCoherency; /* RISC Architectures only */
extern ULONG NTOSAPI KeMaximumIncrement;
extern ULONG NTOSAPI KeMinimumIncrement;
extern ULONG NTOSAPI NtBuildNumber;
extern SSDT_ENTRY NTOSAPI KeServiceDescriptorTable[SSDT_MAX_ENTRIES];
extern SSDT_ENTRY NTOSAPI KeServiceDescriptorTableShadow[SSDT_MAX_ENTRIES];

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

typedef struct _CONFIGURATION_COMPONENT_DATA
{
    struct _CONFIGURATION_COMPONENT_DATA *Parent;
    struct _CONFIGURATION_COMPONENT_DATA *Child;
    struct _CONFIGURATION_COMPONENT_DATA *Sibling;
    CONFIGURATION_COMPONENT Component;
} CONFIGURATION_COMPONENT_DATA, *PCONFIGURATION_COMPONENT_DATA;

typedef enum _KAPC_ENVIRONMENT
{
    OriginalApcEnvironment,
    AttachedApcEnvironment,
    CurrentApcEnvironment
} KAPC_ENVIRONMENT;

typedef struct _KDPC_DATA
{
    LIST_ENTRY  DpcListHead;
    ULONG  DpcLock;
    ULONG  DpcQueueDepth;
    ULONG  DpcCount;
} KDPC_DATA, *PKDPC_DATA;

/* We don't want to force NTIFS usage only for a single structure */
#ifndef _NTIFS_
typedef struct _KAPC_STATE
{
    LIST_ENTRY  ApcListHead[2];
    PKPROCESS   Process;
    BOOLEAN     KernelApcInProgress;
    BOOLEAN     KernelApcPending;
    BOOLEAN     UserApcPending;
} KAPC_STATE, *PKAPC_STATE, *RESTRICTED_POINTER PRKAPC_STATE;
#endif

/* FIXME: Most of these should go to i386 directory */
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
    UCHAR RegisterArea[128];
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

typedef struct _KTRAP_FRAME
{
    PVOID DebugEbp;
    PVOID DebugEip;
    PVOID DebugArgMark;
    PVOID DebugPointer;
    PVOID TempCs;
    PVOID TempEip;
    ULONG Dr0;
    ULONG Dr1;
    ULONG Dr2;
    ULONG Dr3;
    ULONG Dr6;
    ULONG Dr7;
    USHORT Gs;
    USHORT Reserved1;
    USHORT Es;
    USHORT Reserved2;
    USHORT Ds;
    USHORT Reserved3;
    ULONG Edx;
    ULONG Ecx;
    ULONG Eax;
    ULONG PreviousMode;
    PVOID ExceptionList;
    USHORT Fs;
    USHORT Reserved4;
    ULONG Edi;
    ULONG Esi;
    ULONG Ebx;
    ULONG Ebp;
    ULONG ErrorCode;
    ULONG Eip;
    ULONG Cs;
    ULONG Eflags;
    ULONG Esp;
    USHORT Ss;
    USHORT Reserved5;
    USHORT V86_Es;
    USHORT Reserved6;
    USHORT V86_Ds;
    USHORT Reserved7;
    USHORT V86_Fs;
    USHORT Reserved8;
    USHORT V86_Gs;
    USHORT Reserved9;
} KTRAP_FRAME, *PKTRAP_FRAME;

/* FIXME: Win32k uses windows.h! */
#ifndef __WIN32K__
typedef struct _LDT_ENTRY
{
    WORD LimitLow;
    WORD BaseLow;
    union
    {
        struct
        {
            BYTE BaseMid;
            BYTE Flags1;
            BYTE Flags2;
            BYTE BaseHi;
        } Bytes;
        struct
        {
            DWORD BaseMid : 8;
            DWORD Type : 5;
            DWORD Dpl : 2;
            DWORD Pres : 1;
            DWORD LimitHi : 4;
            DWORD Sys : 1;
            DWORD Reserved_0 : 1;
            DWORD Default_Big : 1;
            DWORD Granularity : 1;
            DWORD BaseHi : 8;
        } Bits;
    } HighWord;
} LDT_ENTRY, *PLDT_ENTRY, *LPLDT_ENTRY;
#endif

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
            ULONG BaseMid       : 8;
            ULONG Type          : 5;
            ULONG Dpl           : 2;
            ULONG Pres          : 1;
            ULONG LimitHi       : 4;
            ULONG Sys           : 1;
            ULONG Reserved_0    : 1;
            ULONG Default_Big   : 1;
            ULONG Granularity   : 1;
            ULONG BaseHi        : 8;
        } Bits;
    } HighWord;
} KGDTENTRY, *PKGDTENTRY;

typedef struct _KIDTENTRY
{
    USHORT Offset;
    USHORT Selector;
    USHORT Access;
    USHORT ExtendedOffset;
} KIDTENTRY, *PKIDTENTRY;

typedef struct _HARDWARE_PTE_X86
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
} HARDWARE_PTE_X86, *PHARDWARE_PTE_X86;

#pragma pack(push,4)

/* Fixme: Use correct types? */
typedef struct _KPROCESSOR_STATE
{
    PCONTEXT ContextFrame;
    PVOID SpecialRegisters;
} KPROCESSOR_STATE;

/* Processor Control Block */
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
    ULONG SetMember;
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

/*
 * This is the complete, internal KPCR structure
 */
typedef struct _KIPCR
{
    KPCR_TIB  Tib;                /* 00 */
    struct _KPCR  *Self;          /* 1C */
    struct _KPRCB  *Prcb;         /* 20 */
    KIRQL  Irql;                  /* 24 */
    ULONG  IRR;                   /* 28 */
    ULONG  IrrActive;             /* 2C */
    ULONG  IDR;                   /* 30 */
    PVOID  KdVersionBlock;        /* 34 */
    PUSHORT  IDT;                 /* 38 */
    PUSHORT  GDT;                 /* 3C */
    struct _KTSS  *TSS;           /* 40 */
    USHORT  MajorVersion;         /* 44 */
    USHORT  MinorVersion;         /* 46 */
    KAFFINITY  SetMember;         /* 48 */
    ULONG  StallScaleFactor;      /* 4C */
    UCHAR  SparedUnused;          /* 50 */
    UCHAR  Number;                /* 51 */
    UCHAR  Reserved;              /* 52 */
    UCHAR  L2CacheAssociativity;  /* 53 */
    ULONG  VdmAlert;              /* 54 */
    ULONG  KernelReserved[14];    /* 58 */
    ULONG  L2CacheSize;           /* 90 */
    ULONG  HalReserved[16];       /* 94 */
    ULONG  InterruptMode;         /* D4 */
    UCHAR  KernelReserved2[0x48]; /* D8 */
    KPRCB  PrcbData;              /* 120 */
} KIPCR, *PKIPCR;

#pragma pack(pop)

#include <pshpack1.h>

typedef struct _KTSSNOIOPM
{
    USHORT PreviousTask;
    USHORT Reserved1;
    ULONG  Esp0;
    USHORT Ss0;
    USHORT Reserved2;
    ULONG  Esp1;
    USHORT Ss1;
    USHORT Reserved3;
    ULONG  Esp2;
    USHORT Ss2;
    USHORT Reserved4;
    ULONG  Cr3;
    ULONG  Eip;
    ULONG  Eflags;
    ULONG  Eax;
    ULONG  Ecx;
    ULONG  Edx;
    ULONG  Ebx;
    ULONG  Esp;
    ULONG  Ebp;
    ULONG  Esi;
    ULONG  Edi;
    USHORT Es;
    USHORT Reserved5;
    USHORT Cs;
    USHORT Reserved6;
    USHORT Ss;
    USHORT Reserved7;
    USHORT Ds;
    USHORT Reserved8;
    USHORT Fs;
    USHORT Reserved9;
    USHORT Gs;
    USHORT Reserved10;
    USHORT Ldt;
    USHORT Reserved11;
    USHORT Trap;
    USHORT IoMapBase;
    /* no interrupt redirection map */
    UCHAR IoBitmap[1];
} KTSSNOIOPM;

typedef struct _KTSS
{
    USHORT PreviousTask;
    USHORT Reserved1;
    ULONG  Esp0;
    USHORT Ss0;
    USHORT Reserved2;
    ULONG  Esp1;
    USHORT Ss1;
    USHORT Reserved3;
    ULONG  Esp2;
    USHORT Ss2;
    USHORT Reserved4;
    ULONG  Cr3;
    ULONG  Eip;
    ULONG  Eflags;
    ULONG  Eax;
    ULONG  Ecx;
    ULONG  Edx;
    ULONG  Ebx;
    ULONG  Esp;
    ULONG  Ebp;
    ULONG  Esi;
    ULONG  Edi;
    USHORT Es;
    USHORT Reserved5;
    USHORT Cs;
    USHORT Reserved6;
    USHORT Ss;
    USHORT Reserved7;
    USHORT Ds;
    USHORT Reserved8;
    USHORT Fs;
    USHORT Reserved9;
    USHORT Gs;
    USHORT Reserved10;
    USHORT Ldt;
    USHORT Reserved11;
    USHORT Trap;
    USHORT IoMapBase;
    /* no interrupt redirection map */
    UCHAR  IoBitmap[8193];
} KTSS;

#include <poppack.h>

/* i386 Doesn't have Exception Frames */
typedef struct _KEXCEPTION_FRAME
{

} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

typedef struct _KNODE
{
   SLIST_HEADER DeadStackList;
   SLIST_HEADER PfnDereferenceSListHead;
   ULONG ProcessorMask;
   ULONG Color;
   UCHAR Seed;
   UCHAR NodeNumber;
   ULONG Flags;
   ULONG MmShiftedColor;
   ULONG FreeCount[2];
   struct _SINGLE_LIST_ENTRY *PfnDeferredList;
} KNODE, *PKNODE;

typedef struct _KPROFILE
{
    CSHORT Type;
    CSHORT Size;
    LIST_ENTRY ListEntry;
    PVOID RegionStart;
    PVOID RegionEnd;
    ULONG BucketShift;
    PVOID Buffer;
    CSHORT Source;
    ULONG Affinity;
    BOOLEAN Active;
    struct _KPROCESS *Process;
} KPROFILE, *PKPROFILE;

typedef struct _KINTERRUPT
{
    CSHORT              Type;
    CSHORT              Size;
    LIST_ENTRY          InterruptListEntry;
    PKSERVICE_ROUTINE   ServiceRoutine;
    PVOID               ServiceContext;
    KSPIN_LOCK          SpinLock;
    ULONG               TickCount;
    PKSPIN_LOCK         ActualLock;
    PVOID               DispatchAddress;
    ULONG               Vector;
    KIRQL               Irql;
    KIRQL               SynchronizeIrql;
    BOOLEAN             FloatingSave;
    BOOLEAN             Connected;
    CHAR                Number;
    UCHAR               ShareVector;
    KINTERRUPT_MODE     Mode;
    ULONG               ServiceCount;
    ULONG               DispatchCount;
    ULONG               DispatchCode[106];
} KINTERRUPT, *PKINTERRUPT;

typedef struct _KEVENT_PAIR
{
    CSHORT Type;
    CSHORT Size;
    KEVENT LowEvent;
    KEVENT HighEvent;
} KEVENT_PAIR, *PKEVENT_PAIR;

typedef struct _KEXECUTE_OPTIONS
{
    UCHAR ExecuteDisable:1;
    UCHAR ExecuteEnable:1;
    UCHAR DisableThunkEmulation:1;
    UCHAR Permanent:1;
    UCHAR ExecuteDispatchEnable:1;
    UCHAR ImageDispatchEnable:1;
    UCHAR Spare:2;
} KEXECUTE_OPTIONS, *PKEXECUTE_OPTIONS;

typedef enum _KOBJECTS
{
    EventNotificationObject = 0,
    EventSynchronizationObject = 1,
    MutantObject = 2,
    ProcessObject = 3,
    QueueObject = 4,
    SemaphoreObject = 5,
    ThreadObject = 6,
    GateObject = 7,
    TimerNotificationObject = 8,
    TimerSynchronizationObject = 9,
    Spare2Object = 10,
    Spare3Object = 11,
    Spare4Object = 12,
    Spare5Object = 13,
    Spare6Object = 14,
    Spare7Object = 15,
    Spare8Object = 16,
    Spare9Object = 17,
    ApcObject = 18,
    DpcObject = 19,
    DeviceQueueObject = 20,
    EventPairObject = 21,
    InterruptObject = 22,
    ProfileObject = 23,
    ThreadedDpcObject = 24,
    MaximumKernelObject = 25
} KOBJECTS;

#include <pshpack1.h>

typedef struct _KTHREAD
{
    /* For waiting on thread exit */
    DISPATCHER_HEADER DispatcherHeader;    /* 00 */

    /* List of mutants owned by the thread */
    LIST_ENTRY        MutantListHead;      /* 10 */
    PVOID             InitialStack;        /* 18 */
    ULONG_PTR         StackLimit;          /* 1C */

    /* Pointer to the thread's environment block in user memory */
    struct _TEB       *Teb;                /* 20 */

    /* Pointer to the thread's TLS array */
    PVOID             TlsArray;            /* 24 */
    PVOID             KernelStack;         /* 28 */
    UCHAR             DebugActive;         /* 2C */

    /* Thread state (one of THREAD_STATE_xxx constants below) */
    UCHAR             State;               /* 2D */
    BOOLEAN           Alerted[2];          /* 2E */
    UCHAR             Iopl;                /* 30 */
    UCHAR             NpxState;            /* 31 */
    CHAR              Saturation;          /* 32 */
    CHAR              Priority;            /* 33 */
    KAPC_STATE        ApcState;            /* 34 */
    ULONG             ContextSwitches;     /* 4C */
    LONG              WaitStatus;          /* 50 */
    KIRQL             WaitIrql;            /* 54 */
    CHAR              WaitMode;            /* 55 */
    UCHAR             WaitNext;            /* 56 */
    UCHAR             WaitReason;          /* 57 */
    union                                  /* 58 */
    {
        PKWAIT_BLOCK  WaitBlockList;      /* 58 */
        PKGATE        GateObject;         /* 58 */
    };                                     /* 58 */
    LIST_ENTRY        WaitListEntry;       /* 5C */
    ULONG             WaitTime;            /* 64 */
    CHAR              BasePriority;        /* 68 */
    UCHAR             DecrementCount;      /* 69 */
    UCHAR             PriorityDecrement;   /* 6A */
    CHAR              Quantum;             /* 6B */
    KWAIT_BLOCK       WaitBlock[4];        /* 6C */
    PVOID             LegoData;            /* CC */
    union
    {
        struct
        {
            USHORT KernelApcDisable;
            USHORT SpecialApcDisable;
        };
        ULONG      CombinedApcDisable;     /* D0 */
    };
    KAFFINITY         UserAffinity;        /* D4 */
    UCHAR             SystemAffinityActive;/* D8 */
    UCHAR             PowerState;          /* D9 */
    UCHAR             NpxIrql;             /* DA */
    UCHAR             Pad[1];              /* DB */
    PVOID             ServiceTable;        /* DC */
    struct _KQUEUE    *Queue;              /* E0 */
    KSPIN_LOCK        ApcQueueLock;        /* E4 */
    KTIMER            Timer;               /* E8 */
    LIST_ENTRY        QueueListEntry;      /* 110 */
    KAFFINITY         Affinity;            /* 118 */
    UCHAR             Preempted;           /* 11C */
    UCHAR             ProcessReadyQueue;   /* 11D */
    UCHAR             KernelStackResident; /* 11E */
    UCHAR             NextProcessor;       /* 11F */
    PVOID             CallbackStack;       /* 120 */
    struct _W32THREAD *Win32Thread;        /* 124 */
    struct _KTRAP_FRAME *TrapFrame;        /* 128 */
    PKAPC_STATE       ApcStatePointer[2];  /* 12C */
    UCHAR             EnableStackSwap;     /* 134 */
    UCHAR             LargeStack;          /* 135 */
    UCHAR             ResourceIndex;       /* 136 */
    UCHAR             PreviousMode;        /* 137 */
    ULONG             KernelTime;          /* 138 */
    ULONG             UserTime;            /* 13C */
    KAPC_STATE        SavedApcState;       /* 140 */
    UCHAR             Alertable;           /* 158 */
    UCHAR             ApcStateIndex;       /* 159 */
    UCHAR             ApcQueueable;        /* 15A */
    UCHAR             AutoAlignment;       /* 15B */
    PVOID             StackBase;           /* 15C */
    KAPC              SuspendApc;          /* 160 */
    KSEMAPHORE        SuspendSemaphore;    /* 190 */
    LIST_ENTRY        ThreadListEntry;     /* 1A4 */
    CHAR              FreezeCount;         /* 1AC */
    UCHAR             SuspendCount;        /* 1AD */
    UCHAR             IdealProcessor;      /* 1AE */
    UCHAR             DisableBoost;        /* 1AF */
    UCHAR             QuantumReset;        /* 1B0 */
} KTHREAD;

#include <poppack.h>

/*
 * NAME:           KPROCESS
 * DESCRIPTION:    Internal Kernel Process Structure.
 * PORTABILITY:    Architecture Dependent.
 * KERNEL VERSION: 5.2
 * DOCUMENTATION:  http://reactos.com/wiki/index.php/KPROCESS
 */
typedef struct _KPROCESS
{
    DISPATCHER_HEADER     Header;                    /* 000 */
    LIST_ENTRY            ProfileListHead;           /* 010 */
    PHYSICAL_ADDRESS      DirectoryTableBase;        /* 018 */
    KGDTENTRY             LdtDescriptor;             /* 020 */
    KIDTENTRY             Int21Descriptor;           /* 028 */
    USHORT                IopmOffset;                /* 030 */
    UCHAR                 Iopl;                      /* 032 */
    UCHAR                 Unused;                    /* 033 */
    ULONG                 ActiveProcessors;          /* 034 */
    ULONG                 KernelTime;                /* 038 */
    ULONG                 UserTime;                  /* 03C */
    LIST_ENTRY            ReadyListHead;             /* 040 */
    LIST_ENTRY            SwapListEntry;             /* 048 */
    PVOID                 VdmTrapcHandler;           /* 04C */
    LIST_ENTRY            ThreadListHead;            /* 050 */
    KSPIN_LOCK            ProcessLock;               /* 058 */
    KAFFINITY             Affinity;                  /* 05C */
    union
    {
        struct
        {
            ULONG         AutoAlignment:1;           /* 060.0 */
            ULONG         DisableBoost:1;            /* 060.1 */
            ULONG         DisableQuantum:1;          /* 060.2 */
            ULONG         ReservedFlags:29;          /* 060.3 */
        };
        ULONG             ProcessFlags;              /* 060 */
    };
    CHAR                  BasePriority;              /* 064 */
    CHAR                  QuantumReset;              /* 065 */
    UCHAR                 State;                     /* 066 */
    UCHAR                 ThreadSeed;                /* 067 */
    UCHAR                 PowerState;                /* 068 */
    UCHAR                 IdealNode;                 /* 069 */
    UCHAR                 Visited;                   /* 06A */
    KEXECUTE_OPTIONS      Flags;                     /* 06B */
    ULONG                 StackCount;                /* 06C */
    LIST_ENTRY            ProcessListEntry;          /* 070 */
} KPROCESS;

typedef enum _KTHREAD_STATE
{
    Initialized,
    Ready,
    Running,
    Standby,
    Terminated,
    Waiting,
    Transition,
    DeferredReady,
} KTHREAD_STATE, *PKTHREAD_STATE;

#endif
