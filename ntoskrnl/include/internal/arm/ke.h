#ifndef __NTOSKRNL_INCLUDE_INTERNAL_ARM_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_ARM_KE_H

#if __GNUC__ >=3
#pragma GCC system_header
#endif


//
//Lockdown TLB entries
//
#define PCR_ENTRY            0
#define PDR_ENTRY            2

#define KeArchHaltProcessor() KeArmHaltProcessor()

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
        ULONG Sbo1:1;
        ULONG Sbz1:1;
        ULONG Sbo2:1;
        ULONG Reserved:14;
    };
    ULONG AsUlong;
} ARM_CONTROL_REGISTER, *PARM_CONTROL_REGISTER;

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

VOID
NTAPI
KeArmInitThreadWithContext(
    IN PKTHREAD Thread,
    IN PKSYSTEM_ROUTINE SystemRoutine,
    IN PKSTART_ROUTINE StartRoutine,
    IN PVOID StartContext,
    IN PCONTEXT Context
);

VOID
KiPassiveRelease(
    VOID

);

VOID
KiApcInterrupt(
    VOID                 
);

#include "mm.h"

VOID
KeFillFixedEntryTb(
    IN ARM_PTE Pte,
    IN PVOID Virtual,
    IN ULONG Index
);

VOID
KeFlushTb(
    VOID
);

#define KeArchInitThreadWithContext KeArmInitThreadWithContext
#define KiSystemStartupReal KiSystemStartup

#endif
