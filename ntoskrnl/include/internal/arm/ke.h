#ifndef __NTOSKRNL_INCLUDE_INTERNAL_ARM_KE_H
#define __NTOSKRNL_INCLUDE_INTERNAL_ARM_KE_H

//
//Lockdown TLB entries
//
#define PCR_ENTRY            0
#define PDR_ENTRY            2

#define IMAGE_FILE_MACHINE_ARCHITECTURE IMAGE_FILE_MACHINE_ARM

//
// BKPT is 4 bytes long
//
#define KD_BREAKPOINT_TYPE        ULONG
#define KD_BREAKPOINT_SIZE        sizeof(ULONG)
//#define KD_BREAKPOINT_VALUE

//
// Macros for getting and setting special purpose registers in portable code
//
#define KeGetContextPc(Context) \
    ((Context)->Pc)

#define KeSetContextPc(Context, ProgramCounter) \
    ((Context)->Pc = (ProgramCounter))

#define KeGetTrapFramePc(TrapFrame) \
    ((TrapFrame)->Pc)

#define KeGetContextReturnRegister(Context) \
    ((Context)->R0)

#define KeSetContextReturnRegister(Context, ReturnValue) \
    ((Context)->R0 = (ReturnValue))

//
// Returns the Interrupt State from a Trap Frame.
// ON = TRUE, OFF = FALSE
//
//#define KeGetTrapFrameInterruptState(TrapFrame) \

//
// Invalidates the TLB entry for a specified address
//
FORCEINLINE
VOID
KeInvalidateTlbEntry(IN PVOID Address)
{
    /* Invalidate the TLB entry for this address */
    KeArmInvalidateTlbEntry(Address);
}

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
