/*
 * FILE:            hal/halx86/generic/v86.S
 * COPYRIGHT:       See COPYING in the top level directory
 * PURPOSE:         System Traps, Entrypoints and Exitpoints
 * PROGRAMMER:      Timo Kreuzer (timo.kreuzer@reactos.org)
 * NOTE:            See asmmacro.S for the shared entry/exit code.
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>

#include <ks386.inc>

.code

PUBLIC @HalpExitToV86@4
@HalpExitToV86@4:
    /* Point esp to the iret frame and return */
    lea esp, [ecx + KTRAP_FRAME_EIP]
    iretd

/* Here starts the real mode code */
.code16
PUBLIC _HalpRealModeStart
_HalpRealModeStart:
    /* INT 0x10: AH = 0 (Set video Mode), AL = 0x12 (Mode 12) */
    mov eax, HEX(12)
    int HEX(10)

    /* BOP */
    .byte HEX(C4), HEX(C4)

/* The real mode stack */
.align 4
.space 2048
_HalpRealModeEnd:
PUBLIC _HalpRealModeEnd
.endcode16

END
