/*
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ke/i386/trap.s
 * PURPOSE:         Exception handlers
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.h>
#include <internal/i386/asmmacro.S>

/* NOTES:
 * Why not share the epilogue?
 * 1) An extra jmp is expensive (jmps are very costly)
 * 2) Eventually V86 exit should be handled through ABIOS, and we
 *    handle ABIOS exit in the shared trap exit code already.
 * Why not share the KiTrapHandler call?
 * 1) Would make using the trap-prolog macro much harder.
 * 2) Eventually some of these traps might be re-implemented in assembly
 *    to improve speed and depend less on the compiler and/or use features
 *    not present as C keywords. When that happens, less traps will use the
 *    shared C handler, so the shared-code would need to be un-shared.
 */

/* FUNCTIONS *****************************************************************/

.globl _KiTrap0
_KiTrap0:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(0)

    /* Call the C exception handler */
    push 0
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap1
_KiTrap1:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(1)

    /* Call the C exception handler */
    push 1
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap2
_KiTrap2:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(2)

    /* Call the C exception handler */
    push 2
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap3
_KiTrap3:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(3)

    /* Call the C exception handler */
    push 3
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap4
_KiTrap4:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(4)

    /* Call the C exception handler */
    push 4
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap5
_KiTrap5:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(5)

    /* Call the C exception handler */
    push 5
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap6
_KiTrap6:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(6)

    /* Call the C exception handler */
    push 6
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap7
_KiTrap7:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(7)

    /* Call the C exception handler */
    push 7
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap8
_KiTrap8:
    call _KiDoubleFaultHandler
    iret

.globl _KiTrap9
_KiTrap9:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(9)

    /* Call the C exception handler */
    push 9
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap10
_KiTrap10:
    /* Enter trap */
    TRAP_PROLOG(10)

    /* Call the C exception handler */
    push 10
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap11
_KiTrap11:
    /* Enter trap */
    TRAP_PROLOG(11)

    /* Call the C exception handler */
    push 11
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap12
_KiTrap12:
    /* Enter trap */
    TRAP_PROLOG(12)

    /* Call the C exception handler */
    push 12
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap13
_KiTrap13:
    /* Enter trap */
    TRAP_PROLOG(13)

    /* Call the C exception handler */
    push 13
    push ebp
    call _KiTrapHandler
    add esp, 8
    
    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap14
_KiTrap14:
    /* Enter trap */
    TRAP_PROLOG(14)

    /* Call the C exception handler */
    push 14
    push ebp
    call _KiPageFaultHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap15
_KiTrap15:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(15)

    /* Call the C exception handler */
    push 15
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap16
_KiTrap16:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(16)

    /* Call the C exception handler */
    push 16
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap17
_KiTrap17:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(17)

    /* Call the C exception handler */
    push 17
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap18
_KiTrap18:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(18)

    /* Call the C exception handler */
    push 18
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrap19
_KiTrap19:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(19)

    /* Call the C exception handler */
    push 19
    push ebp
    call _KiTrapHandler
    add esp, 8

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiTrapUnknown
_KiTrapUnknown:
    /* Push error code */
    push 0

    /* Enter trap */
    TRAP_PROLOG(255)

    /* Check for v86 recovery */
    cmp eax, 1

    /* Return to caller */
    jne _Kei386EoiHelper@0
    jmp _KiV86Complete

.globl _KiCoprocessorError@0
_KiCoprocessorError@0:

    /* Get the NPX Thread's Initial stack */
    mov eax, [fs:KPCR_NPX_THREAD]
    mov eax, [eax+KTHREAD_INITIAL_STACK]

    /* Make space for the FPU Save area */
    sub eax, SIZEOF_FX_SAVE_AREA

    /* Set the CR0 State */
    mov dword ptr [eax+FN_CR0_NPX_STATE], 8

    /* Update it */
    mov eax, cr0
    or eax, 8
    mov cr0, eax

    /* Return to caller */
    ret

.globl _Ki386AdjustEsp0@4
_Ki386AdjustEsp0@4:

    /* Get the current thread */
    mov eax, [fs:KPCR_CURRENT_THREAD]

    /* Get trap frame and stack */
    mov edx, [esp+4]
    mov eax, [eax+KTHREAD_INITIAL_STACK]

    /* Check if V86 */
    test dword ptr [edx+KTRAP_FRAME_EFLAGS], EFLAGS_V86_MASK
    jnz NoAdjust

    /* Bias the stack */
    sub eax, KTRAP_FRAME_V86_GS - KTRAP_FRAME_SS

NoAdjust:
    /* Skip FX Save Area */
    sub eax, SIZEOF_FX_SAVE_AREA

    /* Disable interrupts */
    pushf
    cli

    /* Adjust ESP0 */
    mov edx, [fs:KPCR_TSS]
    mov ss:[edx+KTSS_ESP0], eax

    /* Enable interrupts and return */
    popf
    ret 4

/* EOF */
