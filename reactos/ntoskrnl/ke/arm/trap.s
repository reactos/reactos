/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/ke/arm/trap.s
 * PURPOSE:         Support for exceptions and interrupts on ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

    .title "ARM Trap Dispatching and Handling"
    .include "ntoskrnl/include/internal/arm/kxarm.h"
    .include "ntoskrnl/include/internal/arm/ksarm.h"

    .global KiArmVectorTable
    KiArmVectorTable:
        b .                                     // Reset
        ldr pc, _KiUndefinedInstructionJump     // Undefined Instruction
        ldr pc, _KiSoftwareInterruptJump        // Software Interrupt
        ldr pc, _KiPrefetchAbortJump            // Prefetch Abort
        ldr pc, _KiDataAbortJump                // Data Abort
        ldr pc, _KiReservedJump                 // Reserved
        ldr pc, _KiInterruptJump                // Interrupt
        ldr pc, _KiFastInterruptJump            // Fast Interrupt
        
    _KiUndefinedInstructionJump:    .word KiUndefinedInstructionException
    _KiSoftwareInterruptJump:       .word KiSoftwareInterruptException
    _KiPrefetchAbortJump:           .word KiPrefetchAbortException
    _KiDataAbortJump:               .word KiDataAbortException
    _KiReservedJump:                .word KiReservedException
    _KiInterruptJump:               .word KiInterruptException
    _KiFastInterruptJump:           .word KiFastInterruptException

    TEXTAREA
    NESTED_ENTRY KiUndefinedInstructionException
    PROLOG_END KiUndefinedInstructionException
    
    //
    // FIXME: TODO
    //
    b .
    
    ENTRY_END KiUndefinedInstructionException
    
    NESTED_ENTRY KiSoftwareInterruptException
    PROLOG_END KiSoftwareInterruptException
    
    //
    // FIXME: TODO
    //
    b .
    
    ENTRY_END KiSoftwareInterruptException

    NESTED_ENTRY KiPrefetchAbortException
    PROLOG_END KiPrefetchAbortException
    
    //
    // FIXME: TODO
    //
    b .
    
    ENTRY_END KiPrefetchAbortException

    NESTED_ENTRY KiDataAbortException
    PROLOG_END KiDataAbortException

    //
    // Save space for trap frame
    //
    sub sp, #TrapFrameLength
    
    //
    // Build the register part of the trap frame
    //
    stm sp, {r0-r15}

    //
    // TOOD: We'll worry about the rest later...
    //
    
    //
    // Call the C handler
    //
    mov a1, sp
    b KiDataAbortHandler

    //
    // Restore state
    //
    b .
    
    ENTRY_END KiDataAbortException

    NESTED_ENTRY KiInterruptException
    PROLOG_END KiInterruptException
    
    //
    // FIXME: TODO
    //
    b .
    
    ENTRY_END KiInterruptException

    NESTED_ENTRY KiFastInterruptException
    PROLOG_END KiFastInterruptException
    
    //
    // FIXME: TODO
    //
    b .
    
    ENTRY_END KiFastInterruptException
    

    NESTED_ENTRY KiReservedException
    PROLOG_END KiReservedException
    
    //
    // FIXME: TODO
    //
    b .
    
    ENTRY_END KiReservedException

