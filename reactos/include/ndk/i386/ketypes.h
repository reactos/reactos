/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/i386/ketypes.h
 * PURPOSE:         I386-specific definitions for Kernel Types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _I386_KETYPES_H
#define _I386_KETYPES_H

/* DEPENDENCIES **************************************************************/

/* CONSTANTS *****************************************************************/

/* X86 80386 Segment Types */
#define I386_TSS               0x9
#define I386_ACTIVE_TSS        0xB
#define I386_CALL_GATE         0xC
#define I386_INTERRUPT_GATE    0xE
#define I386_TRAP_GATE         0xF

/* EXPORTED DATA *************************************************************/

/* ENUMERATIONS **************************************************************/

/* TYPES *********************************************************************/

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

typedef struct _DESCRIPTOR
{
    WORD Pad;
    WORD Limit;
    DWORD Base;
} KDESCRIPTOR, *PKDESCRIPTOR;

typedef struct _KSPECIAL_REGISTERS
{
    DWORD Cr0;
    DWORD Cr2;
    DWORD Cr3;
    DWORD Cr4;
    DWORD KernelDr0;
    DWORD KernelDr1;
    DWORD KernelDr2;
    DWORD KernelDr3;
    DWORD KernelDr6;
    DWORD KernelDr7;
    KDESCRIPTOR Gdtr;
    KDESCRIPTOR Idtr;
    WORD Tr;
    WORD Ldtr;
    DWORD Reserved[6];
} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;

#pragma pack(push,4)

typedef struct _KPROCESSOR_STATE
{
    PCONTEXT ContextFrame;
    KSPECIAL_REGISTERS SpecialRegisters;
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

#endif
