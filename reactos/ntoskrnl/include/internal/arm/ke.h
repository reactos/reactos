#ifndef __NTOSKRNL_INCLUDE_INTERNAL_ARM_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_ARM_KE_H

//
//Lockdown TLB entries
//
#define PCR_ENTRY            0
#define PDR_ENTRY            2

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

#define KiSystemStartupReal KiSystemStartup

#define KiGetPreviousMode(tf) \
    ((tf->Spsr & CPSR_MODES) == CPSR_USER_MODE) ? UserMode: KernelMode

#endif
