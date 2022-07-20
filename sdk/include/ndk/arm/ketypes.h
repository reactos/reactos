/*++ NDK Version: 0098

Copyright (c) Alex Ionescu.  All rights reserved.

Header Name:

    ketypes.h (ARM)

Abstract:

    ARM Type definitions for the Kernel services.

Author:

    Alex Ionescu (alexi@tinykrnl.org) - Updated - 27-Feb-2006
    Timo Kreuzer (timo.kreuzer@reactos.org) - Updated 19-Apr-2015

--*/

#ifndef _ARM_KETYPES_H
#define _ARM_KETYPES_H

#ifdef __cplusplus
extern "C" {
#endif

//
// Dependencies
//


#define SYNCH_LEVEL DISPATCH_LEVEL


//
// CPU Vendors
//
typedef enum
{
    CPU_UNKNOWN = 0,
} CPU_VENDORS;

//
// Co-Processor register definitions
//
#define CP15_MIDR       15, 0,  0,  0, 0
#define CP15_CTR        15, 0,  0,  0, 1
#define CP15_TCMTR      15, 0,  0,  0, 2
#define CP15_TLBTR      15, 0,  0,  0, 3
#define CP15_MPIDR      15, 0,  0,  0, 5
#define CP15_PFR0       15, 0,  0,  1, 0
#define CP15_PFR1       15, 0,  0,  1, 1
#define CP15_DFR0       15, 0,  0,  1, 2
#define CP15_AFR0       15, 0,  0,  1, 3
#define CP15_MMFR0      15, 0,  0,  1, 4
#define CP15_MMFR1      15, 0,  0,  1, 5
#define CP15_MMFR2      15, 0,  0,  1, 6
#define CP15_MMFR3      15, 0,  0,  1, 7
#define CP15_ISAR0      15, 0,  0,  2, 0
#define CP15_ISAR1      15, 0,  0,  2, 1
#define CP15_ISAR2      15, 0,  0,  2, 2
#define CP15_ISAR3      15, 0,  0,  2, 3
#define CP15_ISAR4      15, 0,  0,  2, 4
#define CP15_ISAR5      15, 0,  0,  2, 5
#define CP15_ISAR6      15, 0,  0,  2, 6
#define CP15_ISAR7      15, 0,  0,  2, 7
#define CP15_SCTLR      15, 0,  1,  0, 0
#define CP15_ACTLR      15, 0,  1,  0, 1
#define CP15_CPACR      15, 0,  1,  0, 2
#define CP15_SCR        15, 0,  1,  1, 0
#define CP15_SDER       15, 0,  1,  1, 1
#define CP15_NSACR      15, 0,  1,  1, 2
#define CP15_TTBR0      15, 0,  2,  0, 0
#define CP15_TTBR1      15, 0,  2,  0, 1
#define CP15_TTBCR      15, 0,  2,  0, 2
#define CP15_DACR       15, 0,  3,  0, 0
#define CP15_DFSR       15, 0,  5,  0, 0
#define CP15_IFSR       15, 0,  5,  0, 1
#define CP15_DFAR       15, 0,  6,  0, 0
#define CP15_IFAR       15, 0,  6,  0, 2
#define CP15_ICIALLUIS  15, 0,  7,  1, 0
#define CP15_BPIALLIS   15, 0,  7,  1, 6
#define CP15_ICIALLU    15, 0,  7,  5, 0
#define CP15_ICIMVAU    15, 0,  7,  5, 1
#define CP15_BPIALL     15, 0,  7,  5, 6
#define CP15_BPIMVA     15, 0,  7,  5, 7
#define CP15_DCIMVAC    15, 0,  7,  6, 1
#define CP15_DCISW      15, 0,  7,  6, 2
#define CP15_DCCMVAC    15, 0,  7, 10, 1
#define CP15_DCCSW      15, 0,  7, 10, 2
#define CP15_DCCMVAU    15, 0,  7, 11, 1
#define CP15_DCCIMVAC   15, 0,  7, 14, 1
#define CP15_DCCISW     15, 0,  7, 14, 2
#define CP15_PAR        15, 0,  7,  4, 0
#define CP15_ATS1CPR    15, 0,  7,  8, 0
#define CP15_ATS1CPW    15, 0,  7,  8, 1
#define CP15_ATS1CUR    15, 0,  7,  8, 2
#define CP15_ATS1CUW    15, 0,  7,  8, 3
#define CP15_ISB        15, 0,  7,  5, 4
#define CP15_DSB        15, 0,  7, 10, 4
#define CP15_DMB        15, 0,  7, 10, 5
#define CP15_TLBIALLIS  15, 0,  8,  3, 0
#define CP15_TLBIMVAIS  15, 0,  8,  3, 1
#define CP15_TLBIASIDIS 15, 0,  8,  3, 2
#define CP15_TLBIMVAAIS 15, 0,  8,  3, 3
#define CP15_ITLBIALL   15, 0,  8,  5, 0
#define CP15_ITLBIMVA   15, 0,  8,  5, 1
#define CP15_ITLBIASID  15, 0,  8,  5, 2
#define CP15_DTLBIALL   15, 0,  8,  6, 0
#define CP15_DTLBIMVA   15, 0,  8,  6, 1
#define CP15_DTLBIASID  15, 0,  8,  6, 2
#define CP15_TLBIALL    15, 0,  8,  7, 0
#define CP15_TLBIMVA    15, 0,  8,  7, 1
#define CP15_TLBIASID   15, 0,  8,  7, 2
#define CP15_TLBIMVAA   15, 0,  8,  7, 3
#define CP15_PMCR       15, 0,  9, 12, 0
#define CP15_PMCNTENSET 15, 0,  9, 12, 1
#define CP15_PMCNTENCLR 15, 0,  9, 12, 2
#define CP15_PMOVSR     15, 0,  9, 12, 3
#define CP15_PSWINC     15, 0,  9, 12, 4
#define CP15_PMSELR     15, 0,  9, 12, 5
#define CP15_PMCCNTR    15, 0,  9, 13, 0
#define CP15_PMXEVTYPER 15, 0,  9, 13, 1
#define CP15_PMXEVCNTR  15, 0,  9, 13, 2
#define CP15_PMUSERENR  15, 0,  9, 14, 0
#define CP15_PMINTENSET 15, 0,  9, 14, 1
#define CP15_PMINTENCLR 15, 0,  9, 14, 2
#define CP15_PRRR       15, 0, 10,  2, 0
#define CP15_NMRR       15, 0, 10,  2, 1
#define CP15_VBAR       15, 0, 12,  0, 0
#define CP15_MVBAR      15, 0, 12,  0, 1
#define CP15_ISR        15, 0, 12,  1, 0
#define CP15_CONTEXTIDR 15, 0, 13,  0, 1
#define CP15_TPIDRURW   15, 0, 13,  0, 2
#define CP15_TPIDRURO   15, 0, 13,  0, 3
#define CP15_TPIDRPRW   15, 0, 13,  0, 4
#define CP15_CCSIDR     15, 1,  0,  0, 0
#define CP15_CLIDR      15, 1,  0,  0, 1
#define CP15_AIDR       15, 1,  0,  0, 7
#define CP15_CSSELR     15, 2,  0,  0, 0
#define CP14_DBGDIDR    14, 0,  0,  0, 0
#define CP14_DBGWFAR    14, 0,  0,  6, 0
#define CP14_DBGVCR     14, 0,  0,  7, 0
#define CP14_DBGECR     14, 0,  0,  9, 0
#define CP14_DBGDSCCR   14, 0,  0, 10, 0
#define CP14_DBGDSMCR   14, 0,  0, 11, 0
#define CP14_DBGDTRRX   14, 0,  0,  0, 2
#define CP14_DBGPCSR    14, 0,  0,  1, 2
#define CP14_DBGITR     14, 0,  0,  1, 2
#define CP14_DBGDSCR    14, 0,  0,  2, 2
#define CP14_DBGDTRTX   14, 0,  0,  3, 2
#define CP14_DBGDRCR    14, 0,  0,  4, 2
#define CP14_DBGCIDSR   14, 0,  0,  9, 2
#define CP14_DBGBVR0    14, 0,  0,  0, 4
#define CP14_DBGBVR1    14, 0,  0,  1, 4
#define CP14_DBGBVR2    14, 0,  0,  2, 4
#define CP14_DBGBVR3    14, 0,  0,  3, 4
#define CP14_DBGBVR4    14, 0,  0,  4, 4
#define CP14_DBGBVR5    14, 0,  0,  5, 4
#define CP14_DBGBVR6    14, 0,  0,  6, 4
#define CP14_DBGBVR7    14, 0,  0,  7, 4
#define CP14_DBGBCR0    14, 0,  0,  0, 5
#define CP14_DBGBCR1    14, 0,  0,  1, 5
#define CP14_DBGBCR2    14, 0,  0,  2, 5
#define CP14_DBGBCR3    14, 0,  0,  3, 5
#define CP14_DBGBCR4    14, 0,  0,  4, 5
#define CP14_DBGBCR5    14, 0,  0,  5, 5
#define CP14_DBGBCR6    14, 0,  0,  6, 5
#define CP14_DBGBCR7    14, 0,  0,  7, 5
#define CP14_DBGWVR0    14, 0,  0,  0, 6
#define CP14_DBGWVR1    14, 0,  0,  1, 6
#define CP14_DBGWVR2    14, 0,  0,  2, 6
#define CP14_DBGWVR3    14, 0,  0,  3, 6
#define CP14_DBGWCR0    14, 0,  0,  0, 7
#define CP14_DBGWCR1    14, 0,  0,  1, 7
#define CP14_DBGWCR2    14, 0,  0,  2, 7
#define CP14_DBGWCR3    14, 0,  0,  3, 7
#define CPVFP_FPSID     10, 7,  0,  0, 0
#define CPVFP_FPSCR     10, 7,  1,  0, 0
#define CPVFP_MVFR1     10, 7,  6,  0, 0
#define CPVFP_MVFR0     10, 7,  7,  0, 0
#define CPVFP_FPEXC     10, 7,  8,  0, 0
#define CP15_TTBRx_PD_MASK 0xffffc000


//
// CPSR Values
//
#define CPSRM_USER           0x10
#define CPSRM_FIQ            0x11
#define CPSRM_INT            0x12
#define CPSRM_SVC            0x13
#define CPSRM_ABT            0x17
#define CPSRM_UDF            0x1b
#define CPSRM_SYS            0x1f
#define CPSRM_MASK           0x1f
#define SYSCALL_PSR          0x30

#define CPSRF_N 0x80000000
#define CPSRF_Z 0x40000000
#define CPSRF_C 0x20000000
#define CPSRF_V 0x10000000
#define CPSRF_Q 0x08000000
#define CPSR_IT_MASK 0x600fc00

#define FPSCRF_N  0x80000000
#define FPSCRF_Z  0x40000000
#define FPSCRF_C  0x20000000
#define FPSCRF_V  0x10000000
#define FPSCRF_QC 0x08000000

#define FPSCRM_AHP 0x4000000
#define FPSCRM_DN 0x2000000
#define FPSCRM_FZ 0x1000000
#define FPSCRM_RMODE_MASK 0xc00000
#define FPSCRM_RMODE_RN 0x0
#define FPSCRM_RMODE_RP 0x400000
#define FPSCRM_RMODE_RM 0x800000
#define FPSCRM_RMODE_RZ 0xc00000
#define FPSCRM_DEPRECATED 0x370000

#define FPSCR_IDE 0x8000
#define FPSCR_IXE 0x1000
#define FPSCR_UFE 0x800
#define FPSCR_OFE 0x400
#define FPSCR_DZE 0x200
#define FPSCR_IOE 0x100
#define FPSCR_IDC 0x80
#define FPSCR_IXC 0x10
#define FPSCR_UFC 0x8
#define FPSCR_OFC 0x4
#define FPSCR_DZC 0x2
#define FPSCR_IOC 0x1

#define CPSRC_INT 0x80
#define CPSRC_ABORT 0x100
#define CPSRC_THUMB 0x20

#define SWFS_PAGE_FAULT 0x10
#define SWFS_ALIGN_FAULT 0x20
#define SWFS_HWERR_FAULT 0x40
#define SWFS_DEBUG_FAULT 0x80
#define SWFS_EXECUTE 0x8
#define SWFS_WRITE 0x1

#define CP14_DBGDSCR_MOE_MASK 0x3c
#define CP14_DBGDSCR_MOE_SHIFT 0x2
#define CP14_DBGDSCR_MOE_HALT 0x0
#define CP14_DBGDSCR_MOE_BP 0x1
#define CP14_DBGDSCR_MOE_WPASYNC 0x2
#define CP14_DBGDSCR_MOE_BKPT 0x3
#define CP14_DBGDSCR_MOE_EXTERNAL 0x4
#define CP14_DBGDSCR_MOE_VECTOR 0x5
#define CP14_DBGDSCR_MOE_WPSYNC 0xa

#define CP15_PMCR_DP 0x20
#define CP15_PMCR_X 0x10
#define CP15_PMCR_CLKCNT_DIV 0x8
#define CP15_PMCR_CLKCNT_RST 0x4
#define CP15_PMCR_CNT_RST 0x2
#define CP15_PMCR_ENABLE 0x1

//
// C1 Register Values
//
#define C1_MMU_CONTROL       0x01
#define C1_ALIGNMENT_CONTROL 0x02
#define C1_DCACHE_CONTROL    0x04
#define C1_ICACHE_CONTROL    0x1000
#define C1_VECTOR_CONTROL    0x2000

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
#define PRCB_MINOR_VERSION      1
#define PRCB_MAJOR_VERSION      1
#define PRCB_BUILD_DEBUG        1
#define PRCB_BUILD_UNIPROCESSOR 2

//
// No LDTs on ARM
//
#define LDT_ENTRY              ULONG

//
// HAL Variables
//
#define INITIAL_STALL_COUNT     100
#define MM_HAL_VA_START         0xFFC00000
#define MM_HAL_VA_END           0xFFFFFFFF

//
// Static Kernel-Mode Address start (use MM_KSEG0_BASE for actual)
//
#define KSEG0_BASE              0x80000000

//
// Number of pool lookaside lists per pool in the PRCB
//
#define NUMBER_POOL_LOOKASIDE_LISTS 32

//
// Structure for CPUID info
//
typedef union _CPU_INFO
{
    ULONG Dummy;
} CPU_INFO, *PCPU_INFO;


//
// ARM VFP State
// Based on Windows RT 8.1 symbols and ksarm.h
//
typedef struct _KARM_VFP_STATE
{
    struct _KARM_VFP_STATE* Link; // 0x00
    ULONG Fpscr;                  // 0x04
    ULONG Reserved;               // 0x08
    ULONG Reserved2;              // 0x0c
    ULONGLONG VfpD[32];           // 0x10
} KARM_VFP_STATE, *PKARM_VFP_STATE; // size = 0x110

//
// Trap Frame Definition
// Based on Windows RT 8.1 symbols and ksarm.h
//
typedef struct _KTRAP_FRAME
{
    ULONG Arg3;
    ULONG FaultStatus;
    union
    {
        ULONG FaultAddress;
        ULONG TrapFrame;
    };
    ULONG Reserved;
    BOOLEAN ExceptionActive;
    BOOLEAN ContextFromKFramesUnwound;
    BOOLEAN DebugRegistersValid;
    union
    {
        CHAR PreviousMode;
        KIRQL PreviousIrql;
    };
    PKARM_VFP_STATE VfpState;
    ULONG Bvr[8];
    ULONG Bcr[8];
    ULONG Wvr[1];
    ULONG Wcr[1];
    ULONG R0;
    ULONG R1;
    ULONG R2;
    ULONG R3;
    ULONG R12;
    ULONG Sp;
    ULONG Lr;
    ULONG R11;
    ULONG Pc;
    ULONG Cpsr;
} KTRAP_FRAME, *PKTRAP_FRAME;

#ifndef NTOS_MODE_USER

//
// Exception Frame Definition
// FIXME: this should go into ntddk.h
//
typedef struct _KEXCEPTION_FRAME
{
    ULONG Param5;        // 0x00
    ULONG TrapFrame;     // 0x04
    ULONG OutputBuffer;  // 0x08
    ULONG OutputLength;  // 0x0c
    ULONG Pad;           // 0x04
    ULONG R4;            // 0x14
    ULONG R5;            // 0x18
    ULONG R6;            // 0x1c
    ULONG R7;            // 0x20
    ULONG R8;            // 0x24
    ULONG R9;            // 0x28
    ULONG R10;           // 0x2c
    ULONG R11;           // 0x30
    ULONG Return;        // 0x34
} KEXCEPTION_FRAME, *PKEXCEPTION_FRAME; // size = 0x38

//
// ARM Architecture State
// Based on Windows RT 8.1 symbols and ksarm.h
//
typedef struct _KARM_ARCH_STATE
{
    ULONG Cp15_Cr0_CpuId;
    ULONG Cp15_Cr1_Control;
    ULONG Cp15_Cr1_AuxControl;
    ULONG Cp15_Cr1_Cpacr;
    ULONG Cp15_Cr2_TtbControl;
    ULONG Cp15_Cr2_Ttb0;
    ULONG Cp15_Cr2_Ttb1;
    ULONG Cp15_Cr3_Dacr;
    ULONG Cp15_Cr5_Dfsr;
    ULONG Cp15_Cr5_Ifsr;
    ULONG Cp15_Cr6_Dfar;
    ULONG Cp15_Cr6_Ifar;
    ULONG Cp15_Cr9_PmControl;
    ULONG Cp15_Cr9_PmCountEnableSet;
    ULONG Cp15_Cr9_PmCycleCounter;
    ULONG Cp15_Cr9_PmEventCounter[31];
    ULONG Cp15_Cr9_PmEventType[31];
    ULONG Cp15_Cr9_PmInterruptSelect;
    ULONG Cp15_Cr9_PmOverflowStatus;
    ULONG Cp15_Cr9_PmSelect;
    ULONG Cp15_Cr9_PmUserEnable;
    ULONG Cp15_Cr10_PrimaryMemoryRemap;
    ULONG Cp15_Cr10_NormalMemoryRemap;
    ULONG Cp15_Cr12_VBARns;
    ULONG Cp15_Cr13_ContextId;
} KARM_ARCH_STATE, *PKARM_ARCH_STATE;

///
/// "Custom" definition start
///

//
// ARM Internal Registers
//
typedef union _ARM_TTB_REGISTER
{
    struct
    {
        ULONG Reserved:14;
        ULONG BaseAddress:18;
    };
    ULONG AsUlong;
} ARM_TTB_REGISTER;

typedef union _ARM_STATUS_REGISTER
{

    struct
    {
        ULONG Mode:5;
        ULONG State:1;
        ULONG FiqDisable:1;
        ULONG IrqDisable:1;
        ULONG ImpreciseAbort:1;
        ULONG Endianness:1;
        ULONG Sbz:6;
        ULONG GreaterEqual:4;
        ULONG Sbz1:4;
        ULONG Java:1;
        ULONG Sbz2:2;
        ULONG StickyOverflow:1;
        ULONG Overflow:1;
        ULONG CarryBorrowExtend:1;
        ULONG Zero:1;
        ULONG NegativeLessThan:1;
    };
    ULONG AsUlong;
} ARM_STATUS_REGISTER;

typedef union _ARM_DOMAIN_REGISTER
{
    struct
    {
        ULONG Domain0:2;
        ULONG Domain1:2;
        ULONG Domain2:2;
        ULONG Domain3:2;
        ULONG Domain4:2;
        ULONG Domain5:2;
        ULONG Domain6:2;
        ULONG Domain7:2;
        ULONG Domain8:2;
        ULONG Domain9:2;
        ULONG Domain10:2;
        ULONG Domain11:2;
        ULONG Domain12:2;
        ULONG Domain13:2;
        ULONG Domain14:2;
        ULONG Domain15:2;
    };
    ULONG AsUlong;
} ARM_DOMAIN_REGISTER;

typedef union _ARM_CONTROL_REGISTER
{
    struct
    {
        ULONG MmuEnabled:1;
        ULONG AlignmentFaultsEnabled:1;
        ULONG DCacheEnabled:1;
        ULONG Sbo:4;
        ULONG BigEndianEnabled:1;
        ULONG System:1;
        ULONG Rom:1;
        ULONG Sbz:2;
        ULONG ICacheEnabled:1;
        ULONG HighVectors:1;
        ULONG RoundRobinReplacementEnabled:1;
        ULONG Armv4Compat:1;
        ULONG Ignored:6;
        ULONG UnalignedAccess:1;
        ULONG ExtendedPageTables:1;
        ULONG Sbz1:1;
        ULONG ExceptionBit:1;
        ULONG Sbz2:1;
        ULONG Nmif:1;
        ULONG TexRemap:1;
        ULONG ForceAp:1;
        ULONG Reserved:2;
    };
    ULONG AsUlong;
} ARM_CONTROL_REGISTER, *PARM_CONTROL_REGISTER;

C_ASSERT(sizeof(ARM_CONTROL_REGISTER) == sizeof(ULONG));

typedef union _ARM_ID_CODE_REGISTER
{
    struct
    {
        ULONG Revision:4;
        ULONG PartNumber:12;
        ULONG Architecture:4;
        ULONG Variant:4;
        ULONG Identifier:8;
    };
    ULONG AsUlong;
} ARM_ID_CODE_REGISTER, *PARM_ID_CODE_REGISTER;

typedef union _ARM_CACHE_REGISTER
{
    struct
    {
        ULONG ILength:2;
        ULONG IMultipler:1;
        ULONG IAssociativty:3;
        ULONG ISize:4;
        ULONG IReserved:2;
        ULONG DLength:2;
        ULONG DMultipler:1;
        ULONG DAssociativty:3;
        ULONG DSize:4;
        ULONG DReserved:2;
        ULONG Separate:1;
        ULONG CType:4;
        ULONG Reserved:3;
    };
    ULONG AsUlong;
} ARM_CACHE_REGISTER, *PARM_CACHE_REGISTER;

typedef union _ARM_LOCKDOWN_REGISTER
{
    struct
    {
        ULONG Preserve:1;
        ULONG Ignored:25;
        ULONG Victim:3;
        ULONG Reserved:3;
    };
    ULONG AsUlong;
} ARM_LOCKDOWN_REGISTER, *PARM_LOCKDOWN_REGISTER;

//
// ARM Domains
//
typedef enum _ARM_DOMAINS
{
    Domain0,
    Domain1,
    Domain2,
    Domain3,
    Domain4,
    Domain5,
    Domain6,
    Domain7,
    Domain8,
    Domain9,
    Domain10,
    Domain11,
    Domain12,
    Domain13,
    Domain14,
    Domain15
} ARM_DOMAINS;

///
/// "Custom" definition end
///

typedef struct _DESCRIPTOR
{
    USHORT Pad;
    USHORT Dummy1;
    ULONG Dummy2;
} KDESCRIPTOR, *PKDESCRIPTOR;


//
// Special Registers Structure (outside of CONTEXT)
// Based on Windows RT 8.1 symbols and ksarm.h
//
typedef struct _KSPECIAL_REGISTERS
{
    ULONG Reserved[7];     // 0x00
    ULONG Cp15_Cr13_UsrRW; // 0x1c
    ULONG Cp15_Cr13_UsrRO; // 0x20
    ULONG Cp15_Cr13_SvcRW; // 0x24
    ULONG KernelBvr[8];    // 0x28
    ULONG KernelBcr[8];    // 0x48
    ULONG KernelWvr[1];    // 0x68
    ULONG KernelWcr[1];    // 0x6c
    ULONG Fpexc;           // 0x70
    ULONG Fpinst;          // 0x74
    ULONG Fpinst2;         // 0x78
    ULONG UserSp;          // 0x7c
    ULONG UserLr;          // 0x80
    ULONG AbortSp;         // 0x84
    ULONG AbortLr;         // 0x88
    ULONG AbortSpsr;       // 0x8c
    ULONG UdfSp;           // 0x90
    ULONG UdfLr;           // 0x94
    ULONG UdfSpsr;         // 0x98
    ULONG IrqSp;           // 0x9c
    ULONG IrqLr;           // 0xa0
    ULONG IrqSpsr;         // 0xa4
} KSPECIAL_REGISTERS, *PKSPECIAL_REGISTERS;

//
// Processor State
// Based on Windows RT 8.1 symbols and ksarm.h
//
typedef struct _KPROCESSOR_STATE
{
    KSPECIAL_REGISTERS SpecialRegisters; // 0x000
    KARM_ARCH_STATE ArchState;           // 0x0a8
    CONTEXT ContextFrame;                // 0x200
} KPROCESSOR_STATE, *PKPROCESSOR_STATE;
C_ASSERT(sizeof(KPROCESSOR_STATE) == 0x3a0);

//
// ARM Mini Stack
// Based on Windows RT 8.1 symbols and ksarm.h
//
typedef struct _KARM_MINI_STACK
{
    ULONG Pc;
    ULONG Cpsr;
    ULONG R4;
    ULONG R5;
    ULONG R6;
    ULONG R7;
    ULONG Reserved[2];
} KARM_MINI_STACK, *PKARM_MINI_STACK; // size = 0x20

typedef struct _DISPATCHER_CONTEXT
{
    ULONG ControlPc; // 0x0
    PVOID ImageBase; // 0x4
    PVOID FunctionEntry; // 0x8
    PVOID EstablisherFrame; // 0xc
    ULONG TargetPc; // 0x10
    PVOID ContextRecord; // 0x14
    PVOID LanguageHandler; // 0x18
    PVOID HandlerData; // 0x1c
    PVOID HistoryTable; // 0x20
    ULONG ScopeIndex; // 0x24
    ULONG ControlPcIsUnwound; // 0x28
    PVOID NonVolatileRegisters; // 0x2c
    ULONG Reserved; // 0x30
} DISPATCHER_CONTEXT, *PDISPATCHER_CONTEXT;

//
// Machine Frame
// Based on ksarm.h
//
typedef struct _MACHINE_FRAME
{
    ULONG Sp;
    ULONG Pc;
} MACHINE_FRAME, *PMACHINE_FRAME;

//
// Defines the Callback Stack Layout for User Mode Callbacks
//
typedef KEXCEPTION_FRAME KCALLOUT_FRAME, PKCALLOUT_FRAME;

//
// User mode callout frame
//
typedef struct _UCALLOUT_FRAME
{
    PVOID Buffer;
    ULONG Length;
    ULONG ApiNumber;
    ULONG OriginalLr;
    MACHINE_FRAME MachineFrame;
} UCALLOUT_FRAME, *PUCALLOUT_FRAME;

typedef struct _KSTART_FRAME
{
    ULONG R0;
    ULONG R1;
    ULONG R2;
    ULONG Return;
} KSTART_FRAME, *PKSTART_FRAME;

typedef struct _KSWITCH_FRAME
{
    KIRQL ApcBypass;
    UCHAR Fill[7];
    ULONG R11;
    ULONG Return;
} KSWITCH_FRAME, *PKSWITCH_FRAME;

//
// Cache types
// (These are made up constants!)
//
enum _ARM_CACHE_TYPES
{
    FirstLevelDcache = 0,
    SecondLevelDcache = 1,
    FirstLevelIcache = 2,
    SecondLevelIcache = 3,
    GlobalDcache = 4,
    GlobalIcache = 5
};

#if (NTDDI_VERSION < NTDDI_LONGHORN)
#define GENERAL_LOOKASIDE_POOL PP_LOOKASIDE_LIST
#endif

//
// Processor Region Control Block
// Based on Windows RT 8.1 symbols
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
    KARM_MINI_STACK FiqMiniStack;
    KARM_MINI_STACK IrqMiniStack;
    KARM_MINI_STACK UdfMiniStack;
    KARM_MINI_STACK AbtMiniStack;
    KARM_MINI_STACK PanicMiniStack;
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
C_ASSERT(FIELD_OFFSET(KPRCB, ProcessorState) == 0x20);
C_ASSERT(FIELD_OFFSET(KPRCB, ProcessorModel) == 0x3C0);
C_ASSERT(FIELD_OFFSET(KPRCB, LockQueue) == 0x480);
C_ASSERT(FIELD_OFFSET(KPRCB, PacketBarrier) == 0x600);
C_ASSERT(FIELD_OFFSET(KPRCB, Mailbox) == 0x680);
C_ASSERT(FIELD_OFFSET(KPRCB, DpcData) == 0x690);
C_ASSERT(FIELD_OFFSET(KPRCB, DpcStack) == 0x6c0);
//C_ASSERT(FIELD_OFFSET(KPRCB, CallDpc) == 0x714);


//
// Processor Control Region
// Based on Windows RT 8.1 symbols
//
typedef struct _KIPCR
{
    union
    {
        NT_TIB NtTib;
        struct
        {
            ULONG TibPad0[2];
            PVOID Spare1;
            struct _KPCR *Self;
            struct _KPRCB *CurrentPrcb;
            struct _KSPIN_LOCK_QUEUE* LockArray;
            PVOID Used_Self;
        };
    };
    KIRQL CurrentIrql;
    UCHAR SecondLevelCacheAssociativity;
    ULONG Unused0[3];
    USHORT MajorVersion;
    USHORT MinorVersion;
    ULONG StallScaleFactor;
    PVOID Unused1[3];
    ULONG KernelReserved[15];
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
    ULONG HalReserved[32];
    PVOID KdVersionBlock;
    PVOID Unused3;
    ULONG PcrAlign1[8];

    /* Private members, not in ntddk.h */
    PVOID Idt[256];
    PVOID* IdtExt;
    ULONG PcrAlign2[19];
    UCHAR _PADDING1_[0x4];
    KPRCB Prcb;
} KIPCR, *PKIPCR;

C_ASSERT(FIELD_OFFSET(KIPCR, Prcb.LegacyNumber) == 0x580);

//
// Macro to get current KPRCB
//
FORCEINLINE
struct _KPRCB *
KeGetCurrentPrcb(VOID)
{
    return KeGetPcr()->CurrentPrcb;
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
//#define KeGetDcacheFillSize()          PCR->DcacheFillSize

#endif // !NTOS_MODE_USER

#ifdef __cplusplus
}; // extern "C"
#endif

#endif // !_ARM_KETYPES_H
