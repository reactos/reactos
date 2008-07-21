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

#define KiGetPreviousMode(tf) \
    ((tf->Spsr & CPSR_MODES) == CPSR_USER_MODE) ? UserMode: KernelMode

#endif
