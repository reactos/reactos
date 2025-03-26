/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    ketypes.h (AMD64)

Abstract:

    amd64 Type definitions for the Kernel services.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006
    Timo Kreuzer (timo.kreuzer@reactos.org) - Updated - 14-Aug-2008

--*/

#ifndef _AMD64_KETYPES_H
#define _AMD64_KETYPES_H

//
// Dependencies
//

//
// Kernel Feature Bits
// See https://www.geoffchappell.com/studies/windows/km/ntoskrnl/structs/kprcb/featurebits.htm?tx=61&ts=0,1400
//
#define KF_SMEP                         0x00000001 // Win 6.2
#define KF_RDTSC                        0x00000002 // From ks386.inc, ksamd64.inc
#define KF_CR4                          0x00000004 // From ks386.inc, ksamd64.inc
#define KF_CMOV                         0x00000008
#define KF_GLOBAL_PAGE                  0x00000010 // From ks386.inc, ksamd64.inc
#define KF_LARGE_PAGE                   0x00000020 // From ks386.inc, ksamd64.inc
#define KF_MTRR                         0x00000040
#define KF_CMPXCHG8B                    0x00000080 // From ks386.inc, ksamd64.inc
#define KF_MMX                          0x00000100
#define KF_DTS                          0x00000200 // Win 5.2-6.2
#define KF_PAT                          0x00000400
#define KF_FXSR                         0x00000800
#define KF_FAST_SYSCALL                 0x00001000 // From ks386.inc, ksamd64.inc
#define KF_XMMI                         0x00002000 // SSE
#define KF_3DNOW                        0x00004000
#define KF_AMDK6MTRR                    0x00008000 // Win 5.0-6.1
#define KF_XSAVEOPT                     0x00008000 // From KF_XSAVEOPT_BIT
#define KF_XMMI64                       0x00010000 // SSE2
#define KF_BRANCH                       0x00020000 // From ksamd64.inc, Win 6.1-6.2
#define KF_00040000                     0x00040000 // Unclear
#define KF_SSE3                         0x00080000 // Win 6.0+
#define KF_CMPXCHG16B                   0x00100000 // Win 6.0-6.2
#define KF_AUTHENTICAMD                 0x00200000 // Win 6.1+
#define KF_ACNT2                        0x00400000 // Win 6.1+
#define KF_XSTATE                       0x00800000 // From ksamd64.inc, Win 6.1+
#define KF_GENUINE_INTEL                0x01000000 // Win 6.1+
#define KF_02000000                     0x02000000 // Unclear
#define KF_SLAT                         0x04000000 // Win 6.2+, Intel: EPT supported
#define KF_VIRT_FIRMWARE_ENABLED        0x08000000 // Win 6.2+
#define KF_RDWRFSGSBASE                 0x10000000 // From ksamd64.inc KF_RDWRFSGSBASE_BIT (0x1C)
#define KF_NX_BIT                       0x20000000
#define KF_NX_DISABLED                  0x40000000
#define KF_NX_ENABLED                   0x80000000
#define KF_RDRAND               0x0000000100000000ULL // Win 10.0+
#define KF_SMAP                 0x0000000200000000ULL // From ksamd64.inc
#define KF_RDTSCP               0x0000000400000000ULL // Win 10.0+
#define KF_HUGEPAGE             0x0000002000000000ULL // Win 10.0 1607+
#define KF_XSAVES               0x0000004000000000ULL // From ksamd64.inc KF_XSAVES_BIT (0x26)
#define KF_FPU_LEAKAGE          0x0000020000000000ULL // From ksamd64.inc KF_FPU_LEAKAGE_BIT (0x29)
#define KF_CAT                  0x0000100000000000ULL // From ksamd64.inc KF_CAT_BIT (0x02C)
#define KF_CET_SS               0x0000400000000000ULL // From ksamd64.inc
#define KF_SSSE3                0x0000800000000000ULL
#define KF_SSE4_1               0x0001000000000000ULL
#define KF_SSE4_2               0x0002000000000000ULL

#define KF_XSAVEOPT_BIT                 15 // From ksamd64.inc (0x0F -> 0x8000)
#define KF_XSTATE_BIT                   23 // From ksamd64.inc (0x17 -> 0x800000)
#define KF_RDWRFSGSBASE_BIT             28 // From ksamd64.inc (0x1C -> 0x10000000)
#define KF_XSAVES_BIT                   38 // From ksamd64.inc (0x26 -> 0x4000000000)
#define KF_FPU_LEAKAGE_BIT              41 // From ksamd64.inc (0x29 -> 0x20000000000)
#define KF_CAT_BIT                      44 // From ksamd64.inc (0x2C -> 0x100000000000)

//
// Double fault stack size
//
#define DOUBLE_FAULT_STACK_SIZE 0x2000

//
// CPU Vendors
//
typedef enum
{
    CPU_UNKNOWN,
    CPU_AMD,
    CPU_INTEL,
    CPU_VIA
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
#define KGDT64_NULL             0x0000
#define KGDT64_R0_CODE          0x0010
#define KGDT64_R0_DATA          0x0018
#define KGDT64_R3_CMCODE        0x0020
#define KGDT64_R3_DATA          0x0028
#define KGDT64_R3_CODE          0x0030
#define KGDT64_SYS_TSS          0x0040
#define KGDT64_R3_CMTEB         0x0050
#define KGDT64_R0_LDT           0x0060

//
// CR0
//
#define CR0_PE                  0x00000001
#define CR0_MP                  0x00000002
#define CR0_EM                  0x00000004
#define CR0_TS                  0x00000008
#define CR0_ET                  0x00000010
#define CR0_NE                  0x00000020
#define CR0_WP                  0x00010000
#define CR0_AM                  0x00040000
#define CR0_NW                  0x20000000
#define CR0_CD                  0x40000000
#define CR0_PG                  0x80000000

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
#define CR4_CHANNELS            0x800
#define CR4_XSAVE               0x40000

//
// DR7
//
#define DR7_LEGAL               0xFFFF0355
#define DR7_ACTIVE              0x00000355
#define DR7_TRACE_BRANCH        0x00000200
#define DR7_LAST_BRANCH         0x00000100

//
// Debug flags
//
#define DEBUG_ACTIVE_DR7                        0x0001
#define DEBUG_ACTIVE_INSTRUMENTED               0x0002
#define DEBUG_ACTIVE_DBG_INSTRUMENTED           0x0003
#define DEBUG_ACTIVE_MINIMAL_THREAD             0x0004
#define DEBUG_ACTIVE_PRIMARY_THREAD             0x0080
#define DEBUG_ACTIVE_PRIMARY_THREAD_BIT         0x0007
#define DEBUG_ACTIVE_PRIMARY_THREAD_LOCK_BIT    0x001F
#define DEBUG_ACTIVE_SCHEDULED_THREAD           0x0040
#define DEBUG_ACTIVE_SCHEDULED_THREAD_BIT       0x0006
#define DEBUG_ACTIVE_SCHEDULED_THREAD_LOCK_BIT  0x001E
#define DEBUG_ACTIVE_SCHEDULED_THREAD_LOCK      0x40000000

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
//#define EFLAGS_NF               0x4000
#define EFLAGS_RF               0x10000
#define EFLAGS_V86_MASK         0x20000
#define EFLAGS_ALIGN_CHECK      0x40000
#define EFLAGS_VIF              0x80000
#define EFLAGS_VIP              0x100000
#define EFLAGS_ID               0x200000
#define EFLAGS_USER_SANITIZE    0x3F4DD7
#define EFLAG_SIGN              0x8000
#define EFLAG_ZERO              0x4000
#define EFLAGS_TF_MASK          0x0100
#define EFLAGS_TF_SHIFT         0x0008
#define EFLAGS_ID_MASK          0x200000
#define EFLAGS_IF_MASK          0x0200
#define EFLAGS_IF_SHIFT         0x0009

//
// MXCSR Floating Control/Status Bit Masks
//
#define XSW_INVALID_OPERATION   0x0001
#define XSW_DENORMAL            0x0002
#define XSW_ZERO_DIVIDE         0x0004
#define XSW_OVERFLOW            0x0008
#define XSW_UNDERFLOW           0x0010
#define XSW_PRECISION           0x0020
#define XCW_INVALID_OPERATION   0x0080
#define XCW_DENORMAL            0x0100
#define XCW_ZERO_DIVIDE         0x0200
#define XCW_OVERFLOW            0x0400
#define XCW_UNDERFLOW           0x0800
#define XCW_PRECISION           0x1000
#define XCW_ROUND_CONTROL       0x6000
#define XCW_FLUSH_ZERO          0x8000
#define XSW_ERROR_MASK          0x003F
#define XSW_ERROR_SHIFT         7

//
// Legacy floating status word bit masks.
//
#define FSW_INVALID_OPERATION   0x0001
#define FSW_DENORMAL            0x0002
#define FSW_ZERO_DIVIDE         0x0004
#define FSW_OVERFLOW            0x0008
#define FSW_UNDERFLOW           0x0010
#define FSW_PRECISION           0x0020
#define FSW_STACK_FAULT         0x0040
#define FSW_ERROR_SUMMARY       0x0080
#define FSW_CONDITION_CODE_0    0x0100
#define FSW_CONDITION_CODE_1    0x0200
#define FSW_CONDITION_CODE_2    0x0400
#define FSW_CONDITION_CODE_3    0x4000
#define FSW_ERROR_MASK          0x003F

//
// Machine Specific Registers
//
#define MSR_EFER                0xC0000080
#define MSR_STAR                0xC0000081
#define MSR_LSTAR               0xC0000082
#define MSR_CSTAR               0xC0000083
#define MSR_SYSCALL_MASK        0xC0000084
#define MSR_FS_BASE             0xC0000100
#define MSR_GS_BASE             0xC0000101
#define MSR_GS_SWAP             0xC0000102
#define MSR_MCG_STATUS          0x017A
#define MSR_AMD_ACCESS          0x9C5A203A
#define MSR_IA32_MISC_ENABLE    0x000001A0
#define MSR_LAST_BRANCH_FROM    0x01DB
#define MSR_LAST_BRANCH_TO      0x01DC
#define MSR_LAST_EXCEPTION_FROM 0x01DD
#define MSR_LAST_EXCEPTION_TO   0x01DE

//
// Caching values for the PAT MSR
//
#define PAT_UC                  0ULL
#define PAT_WC                  1ULL
#define PAT_WT                  4ULL
#define PAT_WP                  5ULL
#define PAT_WB                  6ULL
#define PAT_UCM                 7ULL

//
// Flags in MSR_EFER
//
#define MSR_SCE                 0x0001
#define MSR_LME                 0x0100
#define MSR_LMA                 0x0400
#define MSR_NXE                 0x0800
#define MSR_PAT                 0x0277
#define MSR_DEBUG_CTL           0x01D9

//
//  Flags in MSR_IA32_MISC_ENABLE
//
#define MSR_XD_ENABLE_MASK      0xFFFFFFFB

//
//  Flags in MSR_DEBUG_CTL
//
#define MSR_DEBUG_CTL_LBR       0x0001
#define MSR_DEBUG_CTL_BTF       0x0002

//
// IPI Types
//
#define IPI_APC                 1
#define IPI_DPC                 2
#define IPI_FREEZE              4
#define IPI_PACKET_READY        8
#define IPI_SYNCH_REQUEST       16

//
// Flags for KPRCB::IpiFrozen
//
// Values shown with !ipi extension in WinDbg:
// 0 = [Running], 1 = [Unknown], 2 = [Frozen], 3 = [Thaw], 4 = [Freeze Owner]
// 5 = [Target Freeze], 6-15 = [Unknown]
// 0x20 = [Active] (flag)
//
#define IPI_FROZEN_STATE_RUNNING 0
#define IPI_FROZEN_STATE_FROZEN 2
#define IPI_FROZEN_STATE_THAW 3
#define IPI_FROZEN_STATE_OWNER 4
#define IPI_FROZEN_STATE_TARGET_FREEZE 5
#define IPI_FROZEN_FLAG_ACTIVE 0x20

//
// PRCB Flags
//
#define PRCB_MINOR_VERSION      1
#define PRCB_MAJOR_VERSION      1
#define PRCB_BUILD_DEBUG        1
#define PRCB_BUILD_UNIPROCESSOR 2

//
// Exception active flags
//
#define KEXCEPTION_ACTIVE_INTERRUPT_FRAME 0x0000
#define KEXCEPTION_ACTIVE_EXCEPTION_FRAME 0x0001
#define KEXCEPTION_ACTIVE_SERVICE_FRAME   0x0002

//
// HAL Variables
//
#define PRIMARY_VECTOR_BASE     0x30
#define MAXIMUM_IDTVECTOR       0xFF
#define INITIAL_STALL_COUNT     100
#define MM_HAL_VA_START         0xFFFFFFFFFFC00000ULL /* This is Vista+ */
#define MM_HAL_VA_END           0xFFFFFFFFFFFFFFFFULL
#define APIC_BASE               0xFFFFFFFFFFFE0000ULL

//
// IOPM Definitions
//
#define IO_ACCESS_MAP_NONE      0
#define IOPM_OFFSET             FIELD_OFFSET(KTSS, IoMaps[0].IoMap)
#define KiComputeIopmOffset(MapNumber)              \
    (MapNumber == IO_ACCESS_MAP_NONE) ?             \
        (USHORT)(sizeof(KTSS)) :                    \
        (USHORT)(FIELD_OFFSET(KTSS, IoMaps[MapNumber-1].IoMap))

//
// Static Kernel-Mode Address start (use MM_KSEG0_BASE for actual)
//
#define KSEG0_BASE 0xfffff80000000000ULL

#define NMI_STACK_SIZE 0x2000
#define ISR_STACK_SIZE 0x6000

/* Number of bytes reserved for syscall parameters */
#define MAX_SYSCALL_PARAM_SIZE (16 * 8)

//
// Synchronization-level IRQL
//
#ifndef CONFIG_SMP
#define SYNCH_LEVEL             DISPATCH_LEVEL
#else
#define SYNCH_LEVEL             (IPI_LEVEL - 2)
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
    UINT64 P1Home;
    UINT64 P2Home;
    UINT64 P3Home;
    UINT64 P4Home;
    UINT64 P5;
    CHAR PreviousMode;
    UCHAR PreviousIrql;
    UCHAR FaultIndicator;
    UCHAR ExceptionActive;
    ULONG MxCsr;
    UINT64 Rax;
    UINT64 Rcx;
    UINT64 Rdx;
    UINT64 R8;
    UINT64 R9;
    UINT64 R10;
    UINT64 R11;
    union
    {
        UINT64 GsBase;
        UINT64 GsSwap;
    };
    M128A Xmm0;
    M128A Xmm1;
    M128A Xmm2;
    M128A Xmm3;
    M128A Xmm4;
    M128A Xmm5;
    union
    {
        UINT64 FaultAddress;
        UINT64 ContextRecord;
        UINT64 TimeStampCKCL;
    };
    UINT64 Dr0;
    UINT64 Dr1;
    UINT64 Dr2;
    UINT64 Dr3;
    UINT64 Dr6;
    UINT64 Dr7;
    union
    {
        struct
        {
            UINT64 DebugControl;
            UINT64 LastBranchToRip;
            UINT64 LastBranchFromRip;
            UINT64 LastExceptionToRip;
            UINT64 LastExceptionFromRip;
        };
        struct
        {
            UINT64 LastBranchControl;
            ULONG LastBranchMSR;
        };
    };
    USHORT SegDs;
    USHORT SegEs;
    USHORT SegFs;
    USHORT SegGs;
    UINT64 TrapFrame;
    UINT64 Rbx;
    UINT64 Rdi;
    UINT64 Rsi;
    UINT64 Rbp;
    union
    {
        UINT64 ErrorCode;
        UINT64 ExceptionFrame;
        UINT64 TimeStampKlog;
    };
    UINT64 Rip;
    USHORT SegCs;
    UCHAR Fill0;
    UCHAR Logging;
    USHORT Fill1[2];
    ULONG EFlags;
    ULONG Fill2;
    UINT64 Rsp;
    USHORT SegSs;
    USHORT Fill3;
    LONG CodePatchCycle;
} KTRAP_FRAME, *PKTRAP_FRAME;

//
// Dummy LDT_ENTRY
//
#ifndef _LDT_ENTRY_DEFINED
#define _LDT_ENTRY_DEFINED
typedef ULONG LDT_ENTRY;
#endif

//
// GDT Entry Definition
//
typedef union _KGDTENTRY64
{
    struct
    {
        USHORT LimitLow;
        USHORT BaseLow;
        union
        {
            struct
            {
                UCHAR BaseMiddle;
                UCHAR Flags1;
                UCHAR Flags2;
                UCHAR BaseHigh;
            } Bytes;
            struct
            {
                ULONG BaseMiddle:8;
                ULONG Type:5;
                ULONG Dpl:2;
                ULONG Present:1;
                ULONG LimitHigh:4;
                ULONG System:1;
                ULONG LongMode:1;
                ULONG DefaultBig:1;
                ULONG Granularity:1;
                ULONG BaseHigh:8;
            } Bits;
        };
        ULONG BaseUpper;
        ULONG MustBeZero;
    };
    UINT64 Alignment;
} KGDTENTRY64, *PKGDTENTRY64;
#define KGDTENTRY KGDTENTRY64
#define PKGDTENTRY PKGDTENTRY64

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
typedef union _KIDTENTRY64
{
    struct
    {
        USHORT OffsetLow;
        USHORT Selector;
        USHORT IstIndex:3;
        USHORT Reserved0:5;
        USHORT Type:5;
        USHORT Dpl:2;
        USHORT Present:1;
        USHORT OffsetMiddle;
        ULONG OffsetHigh;
        ULONG Reserved1;
    };
    UINT64 Alignment;
} KIDTENTRY64, *PKIDTENTRY64;
#define KIDTENTRY KIDTENTRY64
#define PKIDTENTRY PKIDTENTRY64

typedef struct _KDESCRIPTOR
{
    USHORT Pad[3];
    USHORT Limit;
    PVOID Base;
} KDESCRIPTOR, *PKDESCRIPTOR;

#ifndef NTOS_MODE_USER

//
// Special Registers Structure (outside of CONTEXT)
//
typedef struct _KSPECIAL_REGISTERS
{
    ULONG64 Cr0;
    ULONG64 Cr2;
    ULONG64 Cr3;
    ULONG64 Cr4;
    ULONG64 KernelDr0;
    ULONG64 KernelDr1;
    ULONG64 KernelDr2;
    ULONG64 KernelDr3;
    ULONG64 KernelDr6;
    ULONG64 KernelDr7;
    KDESCRIPTOR Gdtr;
    KDESCRIPTOR Idtr;
    USHORT Tr;
    USHORT Ldtr;
    ULONG MxCsr;
    ULONG64 DebugControl;
    ULONG64 LastBranchToRip;
    ULONG64 LastBranchFromRip;
    ULONG64 LastExceptionToRip;
    ULONG64 LastExceptionFromRip;
    ULONG64 Cr8;
    ULONG64 MsrGsBase;
    ULONG64 MsrGsSwap;
    ULONG64 MsrStar;
    ULONG64 MsrLStar;
    ULONG64 MsrCStar;
    ULONG64 MsrSyscallMask;
} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;

//
// Processor State Data
//
typedef struct _KPROCESSOR_STATE
{
    KSPECIAL_REGISTERS SpecialRegisters;
    CONTEXT ContextFrame;
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;

#if (NTDDI_VERSION < NTDDI_LONGHORN)
#define GENERAL_LOOKASIDE_POOL PP_LOOKASIDE_LIST
#endif

typedef struct _KREQUEST_PACKET
{
    PVOID CurrentPacket[3];
    PVOID WorkerRoutine;
} KREQUEST_PACKET, *PKREQUEST_PACKET;

typedef struct _REQUEST_MAILBOX
{
    INT64 RequestSummary;
    KREQUEST_PACKET RequestPacket;
    PVOID Virtual[7];
} REQUEST_MAILBOX, *PREQUEST_MAILBOX;

//
// Processor Region Control Block
//
typedef struct _KPRCB
{
    ULONG MxCsr;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    USHORT Number;
#else
    UCHAR Number;
    UCHAR NestingLevel;
#endif
    UCHAR InterruptRequest;
    UCHAR IdleHalt;
    struct _KTHREAD *CurrentThread;
    struct _KTHREAD *NextThread;
    struct _KTHREAD *IdleThread;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    UCHAR NestingLevel;
    UCHAR Group;
    UCHAR PrcbPad00[6];
#else
    UINT64 UserRsp;
#endif
    UINT64 RspBase;
    UINT64 PrcbLock;
    UINT64 SetMember;
    KPROCESSOR_STATE ProcessorState;
    CHAR CpuType;
    CHAR CpuID;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    union
    {
        USHORT CpuStep;
        struct
        {
            UCHAR CpuStepping;
            UCHAR CpuModel;
        };
    };
#else
    USHORT CpuStep;
#endif
    ULONG MHz;
    UINT64 HalReserved[8];
    USHORT MinorVersion;
    USHORT MajorVersion;
    UCHAR BuildType;
    UCHAR CpuVendor;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    UCHAR CoresPerPhysicalProcessor;
    UCHAR LogicalProcessorsPerCore;
#else
    UCHAR InitialApicId;
    UCHAR LogicalProcessorsPerPhysicalProcessor;
#endif
    ULONG ApicMask;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG CFlushSize;
#else
    UCHAR CFlushSize;
    UCHAR PrcbPad0x[3];
#endif
    PVOID AcpiReserved;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG InitialApicId;
    ULONG Stride;
    UINT64 PrcbPad01[3];
#else
    UINT64 PrcbPad00[4];
#endif
    KSPIN_LOCK_QUEUE LockQueue[LockQueueMaximumLock]; // 2003: 33, vista:49
    PP_LOOKASIDE_LIST PPLookasideList[16];
    GENERAL_LOOKASIDE_POOL PPNPagedLookasideList[NUMBER_POOL_LOOKASIDE_LISTS];
    GENERAL_LOOKASIDE_POOL PPPagedLookasideList[NUMBER_POOL_LOOKASIDE_LISTS];
    UINT64 PacketBarrier;
    SINGLE_LIST_ENTRY DeferredReadyListHead;
    LONG MmPageFaultCount;
    LONG MmCopyOnWriteCount;
    LONG MmTransitionCount;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    LONG MmCacheTransitionCount;
#endif
    LONG MmDemandZeroCount;
    LONG MmPageReadCount;
    LONG MmPageReadIoCount;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    LONG MmCacheReadCount;
    LONG MmCacheIoCount;
#endif
    LONG MmDirtyPagesWriteCount;
    LONG MmDirtyWriteIoCount;
    LONG MmMappedPagesWriteCount;
    LONG MmMappedWriteIoCount;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG KeSystemCalls;
    ULONG KeContextSwitches;
    ULONG CcFastReadNoWait;
    ULONG CcFastReadWait;
    ULONG CcFastReadNotPossible;
    ULONG CcCopyReadNoWait;
    ULONG CcCopyReadWait;
    ULONG CcCopyReadNoWaitMiss;
    LONG LookasideIrpFloat;
#else
    LONG LookasideIrpFloat;
    ULONG KeSystemCalls;
#endif
    LONG IoReadOperationCount;
    LONG IoWriteOperationCount;
    LONG IoOtherOperationCount;
    LARGE_INTEGER IoReadTransferCount;
    LARGE_INTEGER IoWriteTransferCount;
    LARGE_INTEGER IoOtherTransferCount;
#if (NTDDI_VERSION < NTDDI_LONGHORN)
    ULONG KeContextSwitches;
    UCHAR PrcbPad2[12];
#endif
    UINT64 TargetSet;
    ULONG IpiFrozen;
    UCHAR PrcbPad3[116];
    REQUEST_MAILBOX RequestMailbox[64];
    UINT64 SenderSummary;
    UCHAR PrcbPad4[120];
    KDPC_DATA DpcData[2];
    PVOID DpcStack;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID SparePtr0;
#else
    PVOID SavedRsp;
#endif
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
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    ULONG KeExceptionDispatchCount;
#else
    LONG PrcbPad40;
    PVOID DpcThread;
#endif
    KEVENT DpcEvent;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    PVOID PrcbPad51;
#endif
    KDPC CallDpc;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    LONG ClockKeepAlive;
    UCHAR ClockCheckSlot;
    UCHAR ClockPollCycle;
    UCHAR PrcbPad6[2];
    LONG DpcWatchdogPeriod;
    LONG DpcWatchdogCount;
    UINT64 PrcbPad70[2];
#else
    UINT64 PrcbPad7[4];
#endif
    LIST_ENTRY WaitListHead;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    UINT64 WaitLock;
#endif
    ULONG ReadySummary;
    ULONG QueueIndex;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    UINT64 PrcbPad71[12];
#endif
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
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    UCHAR PrcbPad80[5];
    ULONG DpcTimeCount;
    ULONG DpcTimeLimit;
    ULONG PeriodicCount;
    ULONG PeriodicBias;
    UINT64 PrcbPad81[2];
#else
    UCHAR PrcbPad8[13];
#endif
    struct _KNODE *ParentNode;
    UINT64 MultiThreadProcessorSet;
    struct _KPRCB *MultiThreadSetMaster;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    UINT64 StartCycles;
    LONG MmSpinLockOrdering;
    ULONG PageColor;
    ULONG NodeColor;
    ULONG NodeShiftedColor;
    ULONG SecondaryColorMask;
#endif
    LONG Sleeping;
#if (NTDDI_VERSION >= NTDDI_LONGHORN)
    UINT64 CycleTime;
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
    ULONG PrcbPad91[3];
    PROCESSOR_POWER_STATE PowerState;
    ULONG KeAlignmentFixupCount;
    UCHAR VendorString[13];
    UCHAR PrcbPad10[3];
    ULONG FeatureBits;
    LARGE_INTEGER UpdateSignature;
    KDPC DpcWatchdogDpc;
    KTIMER DpcWatchdogTimer;
    CACHE_DESCRIPTOR Cache[5];
    ULONG CacheCount;
    ULONG CachedCommit;
    ULONG CachedResidentAvailable;
    PVOID HyperPte;
    PVOID WheaInfo;
    PVOID EtwSupport;
    SLIST_HEADER InterruptObjectPool;
    SLIST_HEADER HypercallPageList;
    PVOID HypercallPageVirtual;
    PVOID VirtualApicAssist;
    UINT64* StatisticsPage;
    PVOID RateControl;
    UINT64 CacheProcessorMask[5];
    UINT64 PackageProcessorSet;
    UINT64 CoreProcessorSet;
#else
    ULONG PrcbPad90[1];
    ULONG DebugDpcTime;
    ULONG PageColor;
    ULONG NodeColor;
    ULONG NodeShiftedColor;
    ULONG SecondaryColorMask;
    UCHAR PrcbPad9[12];
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
    UCHAR VendorString[13];
    UCHAR PrcbPad10[2];
    ULONG FeatureBits;
    LARGE_INTEGER UpdateSignature;
    PROCESSOR_POWER_STATE PowerState;
    CACHE_DESCRIPTOR Cache[5];
    ULONG CacheCount;
#endif
#ifdef __REACTOS__
#if  (NTDDI_VERSION < NTDDI_WINBLUE)
    // On Win 8.1+ the FeatureBits field is extended to 64 bits
    ULONG FeatureBitsHigh;
#endif
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
            union _KGDTENTRY64 *GdtBase;
            struct _KTSS64 *TssBase;
            ULONG64 UserRsp;
            struct _KPCR *Self;
            struct _KPRCB *CurrentPrcb;
            PKSPIN_LOCK_QUEUE LockArray;
            PVOID Used_Self;
        };
    };
    union _KIDTENTRY64 *IdtBase;
    ULONG64 Unused[2];
    KIRQL Irql;
    UCHAR SecondLevelCacheAssociativity;
    UCHAR ObsoleteNumber;
    UCHAR Fill0;
    ULONG Unused0[3];
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    PVOID Unused1[3];
    ULONG KernelReserved[15];
    ULONG SecondLevelCacheSize;
    ULONG HalReserved[16];
    ULONG Unused2;
    ULONG Fill1;
    PVOID KdVersionBlock; // 0x108
    PVOID Unused3;
    ULONG PcrAlign1[24];
    ULONG Fill2[2]; // 0x178
    KPRCB Prcb; // 0x180

    // hack:
    ULONG ContextSwitches;

} KIPCR, *PKIPCR;

//
// TSS Definition
//
typedef struct _KiIoAccessMap
{
    UCHAR DirectionMap[32];
    UCHAR IoMap[8196];
} KIIO_ACCESS_MAP;


#pragma pack(push,4)
typedef struct _KTSS64
{
 /* 000 */  ULONG Reserved0;
 /* 004 */  UINT64 Rsp0;
 /* 00c */  UINT64 Rsp1;
 /* 014 */  UINT64 Rsp2;
 /* 01c */  UINT64 Ist[8];
 /* 05c */  UINT64 Reserved1;
 /* 064 */  USHORT Reserved2;
 /* 066 */  USHORT IoMapBase;
} KTSS64, *PKTSS64;
#pragma pack(pop)
#define KTSS KTSS64
#define PKTSS PKTSS64

//
// KEXCEPTION_FRAME
//
typedef struct _KEXCEPTION_FRAME
{
    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    ULONG64 Spare1;
#else
    ULONG64 InitialStack;
#endif
    M128A Xmm6;
    M128A Xmm7;
    M128A Xmm8;
    M128A Xmm9;
    M128A Xmm10;
    M128A Xmm11;
    M128A Xmm12;
    M128A Xmm13;
    M128A Xmm14;
    M128A Xmm15;
    ULONG64 TrapFrame;
#if (NTDDI_VERSION < NTDDI_WIN8)
    ULONG64 CallbackStack;
#endif
    ULONG64 OutputBuffer;
    ULONG64 OutputLength;
#if (NTDDI_VERSION >= NTDDI_WIN8)
    ULONG64 Spare2;
#endif
    ULONG64 MxCsr;
    ULONG64 Rbp;
    ULONG64 Rbx;
    ULONG64 Rdi;
    ULONG64 Rsi;
    ULONG64 R12;
    ULONG64 R13;
    ULONG64 R14;
    ULONG64 R15;
    ULONG64 Return;
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME;

typedef struct _MACHINE_FRAME
{
    ULONG64 Rip;
    USHORT SegCs;
    USHORT Fill1[3];
    ULONG EFlags;
    ULONG Fill2;
    ULONG64 Rsp;
    USHORT SegSs;
    USHORT Fill3[3];
} MACHINE_FRAME, *PMACHINE_FRAME;

//
// Defines the Callback Stack Layout for User Mode Callbacks
//
typedef KEXCEPTION_FRAME KCALLOUT_FRAME, *PKCALLOUT_FRAME;

//
// User side callout frame
//
typedef struct _UCALLOUT_FRAME
{
    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    PVOID Buffer;
    ULONG Length;
    ULONG ApiNumber;
    MACHINE_FRAME MachineFrame;
} UCALLOUT_FRAME, *PUCALLOUT_FRAME; // size = 0x0058

//
// User side APC dispatcher frame
//
typedef struct _UAPC_FRAME
{
    CONTEXT Context;
    MACHINE_FRAME MachineFrame;
} UAPC_FRAME, *PUAPC_FRAME;

//
// Stack frame layout for KiUserExceptionDispatcher
// The name is totally made up
//
typedef struct _KUSER_EXCEPTION_STACK
{
    CONTEXT Context;
    EXCEPTION_RECORD ExceptionRecord;
    ULONG64 Alignment;
    MACHINE_FRAME MachineFrame;
} KUSER_EXCEPTION_STACK, * PKUSER_EXCEPTION_STACK;

typedef struct _DISPATCHER_CONTEXT
{
    ULONG64 ControlPc;
    ULONG64 ImageBase;
    struct _RUNTIME_FUNCTION *FunctionEntry;
    ULONG64 EstablisherFrame;
    ULONG64 TargetIp;
    PCONTEXT ContextRecord;
    PEXCEPTION_ROUTINE LanguageHandler;
    PVOID HandlerData;
    struct _UNWIND_HISTORY_TABLE *HistoryTable;
    ULONG ScopeIndex;
    ULONG Fill0;
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

typedef struct _KSTART_FRAME
{
    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 Reserved;
    ULONG64 Return;
} KSTART_FRAME, *PKSTART_FRAME;

typedef struct _KSWITCH_FRAME
{
    ULONG64 P1Home;
    ULONG64 P2Home;
    ULONG64 P3Home;
    ULONG64 P4Home;
    ULONG64 P5Home;
    KIRQL ApcBypass;
    UCHAR Fill1[7];
    ULONG64 Rbp;
    ULONG64 Return;
} KSWITCH_FRAME, *PKSWITCH_FRAME;

#define PROCESSOR_START_FLAG_FORCE_ENABLE_NX 0x0001
typedef struct _KPROCESSOR_START_BLOCK
{
    ULONG CompletionFlag; // 0x0004
    ULONG Flags; // 0x0008
    ULONG Gdt32; // 0x000C
    ULONG Idt32; // 0x0012
    PVOID Gdt; // 0x0018
    // ???
    ULONG64 TiledMemoryMap; // 0x0058
    UCHAR PmTarget[6]; // 0x0060
    UCHAR LmIdentityTarget[6]; // 0x0066
    ULONG64 LmTarget; // 0x0070
    struct _KPROCESSOR_START_BLOCK *SelfMap; // 0x0078
    ULONG64 MsrPat; // 0x0080
    ULONG64 MsrEFER; // 0x0088
    KPROCESSOR_STATE ProcessorState; // 0x0090
} KPROCESSOR_START_BLOCK, *PKPROCESSOR_START_BLOCK; // size 00640

//
// Inline function to get current KPRCB
//
FORCEINLINE
struct _KPRCB *
KeGetCurrentPrcb(VOID)
{
    return (struct _KPRCB *)__readgsqword(FIELD_OFFSET(KIPCR, CurrentPrcb));
}

#endif
#endif
