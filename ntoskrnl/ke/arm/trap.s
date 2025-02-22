/*
 * PROJECT:         ReactOS Kernel
 * LICENSE:         BSD - See COPYING.ARM in the top level directory
 * FILE:            ntoskrnl/ke/arm/trap.s
 * PURPOSE:         Support for exceptions and interrupts on ARM machines
 * PROGRAMMERS:     ReactOS Portable Systems Group
 */

#include <ksarm.h>

    IMPORT KiUndefinedExceptionHandler
    IMPORT KiSoftwareInterruptHandler
    IMPORT KiPrefetchAbortHandler
    IMPORT KiDataAbortHandler
    IMPORT KiInterruptHandler

    TEXTAREA

    EXPORT KiArmVectorTable
KiArmVectorTable
        b .                                     // Reset
        ldr pc, _KiUndefinedInstructionJump     // Undefined Instruction
        ldr pc, _KiSoftwareInterruptJump        // Software Interrupt
        ldr pc, _KiPrefetchAbortJump            // Prefetch Abort
        ldr pc, _KiDataAbortJump                // Data Abort
        b .                                     // Reserved
        ldr pc, _KiInterruptJump                // Interrupt
        ldr pc, _KiFastInterruptJump            // Fast Interrupt

_KiUndefinedInstructionJump    DCD KiUndefinedInstructionException
_KiSoftwareInterruptJump       DCD KiSoftwareInterruptException
_KiPrefetchAbortJump           DCD KiPrefetchAbortException
_KiDataAbortJump               DCD KiDataAbortException
_KiInterruptJump               DCD KiInterruptException
_KiFastInterruptJump           DCD KiFastInterruptException

    // Might need to move these to a custom header, when used by HAL as well

    MACRO
    TRAP_PROLOG $Abort
        __debugbreak
    MEND

    MACRO
    SYSCALL_PROLOG $Abort
        __debugbreak
    MEND

    MACRO
    TRAP_EPILOG $SystemCall
        __debugbreak
    MEND

    NESTED_ENTRY KiUndefinedInstructionException
    PROLOG_END KiUndefinedInstructionException

    /* Handle trap entry */
    TRAP_PROLOG 0 // NotFromAbort

    /* Call the C handler */
    ldr lr, =KiExceptionExit
    mov r0, sp
    ldr pc, =KiUndefinedExceptionHandler

    NESTED_END KiUndefinedInstructionException


    NESTED_ENTRY KiSoftwareInterruptException
    PROLOG_END KiSoftwareInterruptException

    /* Handle trap entry */
    SYSCALL_PROLOG

    /* Call the C handler */
    ldr lr, =KiServiceExit
    mov r0, sp
    ldr pc, =KiSoftwareInterruptHandler

    NESTED_END KiSoftwareInterruptException


    NESTED_ENTRY KiPrefetchAbortException
    PROLOG_END KiPrefetchAbortException

    /* Handle trap entry */
    TRAP_PROLOG 0 // NotFromAbort

    /* Call the C handler */
    ldr lr, =KiExceptionExit
    mov r0, sp
    ldr pc, =KiPrefetchAbortHandler

    NESTED_END KiPrefetchAbortException


    NESTED_ENTRY KiDataAbortException
    PROLOG_END KiDataAbortException

    /* Handle trap entry */
    TRAP_PROLOG 1 // FromAbort

    /* Call the C handler */
    ldr lr, =KiExceptionExit
    mov r0, sp
    ldr pc, =KiDataAbortHandler

    NESTED_END KiDataAbortException


    NESTED_ENTRY KiInterruptException
    PROLOG_END KiInterruptException

    /* Handle trap entry */
    TRAP_PROLOG 0 // NotFromAbort

    /* Call the C handler */
    ldr lr, =KiExceptionExit
    mov r0, sp
    mov r1, #0
    ldr pc, =KiInterruptHandler

    NESTED_END KiInterruptException


    NESTED_ENTRY KiFastInterruptException
    PROLOG_END KiFastInterruptException

    // FIXME-PERF: Implement FIQ exception
    __debugbreak

    NESTED_END KiFastInterruptException


    NESTED_ENTRY KiExceptionExit
    PROLOG_END KiExceptionExit

    /* Handle trap exit */
    TRAP_EPILOG 0 // NotFromSystemCall

    NESTED_END KiExceptionExit

    NESTED_ENTRY KiServiceExit
    PROLOG_END KiServiceExit

    /* Handle trap exit */
    TRAP_EPILOG 1 // FromSystemCall

    NESTED_END KiServiceExit


    LEAF_ENTRY KiInterruptTemplate
    DCD 0
    LEAF_END KiInterruptTemplate

    END
/* EOF */
