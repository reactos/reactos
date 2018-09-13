/*++ BUILD Version: 0001

Copyright (c) 1990  Microsoft Corporation

Module Name:

    VINT.H

Abstract:

    This module contains macro support for manipulating virtual
    interrupt bit from v86 mode and 16bit protect mode. FCLI/FST/FIRET
    result in exact behavior of these instructions on the chip without
    trapping.

Author:

    Sudeepb 08-Dec-1992 Created

Revision History:
    sudeepb 16-Mar-1993 added FIRET

--*/

/*
See \nt\private\inc\vdm.h for a complete list
of the NTVDM state flag bit definitions

INTERRUPT_PENDING_BIT - set if interrupts pending
VIRTUAL_INTERRUPT_BIT - This bit always correctly reflects the interrupt
                        disbale/enable state of the vDM while in 16bit land.

MIPS_BIT_MASK         - tells whether VDM is running on x86/mips
EXEC_BIT_MASK         - tells if DOS is in int21/exec operation.
*/

#define  INTERRUPT_PENDING_BIT      0x0003
#define  VDM_INTS_HOOKED_IN_PM      0x0004
#define  VIRTUAL_INTERRUPT_BIT      0x0200

#define  MIPS_BIT_MASK              0x400
#define  EXEC_BIT_MASK              0x800
#define  RM_BIT_MASK                0x1000
#define  RI_BIT_MASK                0x2000

#if defined(NEC_98)
#define  FIXED_NTVDMSTATE_SEGMENT   0x60                          
#else  // !NEC_98
#define  FIXED_NTVDMSTATE_SEGMENT   0x70
#endif // !NEC_98
#define  FIXED_NTVDMSTATE_OFFSET    0x14
#define  FIXED_NTVDMSTATE_LINEAR    ((FIXED_NTVDMSTATE_SEGMENT << 4) + FIXED_NTVDMSTATE_OFFSET)
#if defined(NEC_98)
#define  FIXED_NTVDMSTATE_REL40     0x214                         
#else  // !NEC_98
#define  FIXED_NTVDMSTATE_REL40     0x314
#endif // !NEC_98

#define  FIXED_NTVDMSTATE_SIZE	    4
#if defined(NEC_98)
#define  NTIO_LOAD_SEGMENT          0x60                          
#else  // !NEC_98
#define  NTIO_LOAD_SEGMENT	    0x70
#endif // !NEC_98
#define  NTIO_LOAD_OFFSET           0
#define  pNtVDMState                ((PULONG)FIXED_NTVDMSTATE_LINEAR)

#define  VDM_TIMECHANGE             0x00400000

/* ASM
; FCLI macro should be used in v86mode/16bit  preotect mode code to replace
; costly cli's. Please note that this macro could destroy the Overflow
; bit in the flag.

FCLI	macro
    local a,b,c
    push    ds
    push    ax
    mov     ax,40h
    mov     ds,ax
    lahf
    test    word ptr ds:FIXED_NTVDMSTATE_REL40, MIPS_BIT_MASK OR RI_BIT_MASK
    jnz     short b
    lock    and	word ptr ds:FIXED_NTVDMSTATE_REL40,NOT VIRTUAL_INTERRUPT_BIT
a:
    sahf
    pop     ax
    pop     ds
    jmp     short c
b:
    cli
    jmp     short a
c:
endm

;
; FSTI macro should be used in v86mode or 16bit protectmode code to replace
; costly sti's. Please note that this macro could destroy the Overflow bit
; in the flag.

FSTI   macro
    local a,b,c
    push    ds
    push    ax
    mov     ax,40h
    mov     ds,ax
    lahf
    test    word ptr ds:FIXED_NTVDMSTATE_REL40, INTERRUPT_PENDING_BIT
    jnz     short b
    test    word ptr ds:FIXED_NTVDMSTATE_REL40, MIPS_BIT_MASK OR RI_BIT_MASK
    jnz     short b
    lock    or word ptr ds:FIXED_NTVDMSTATE_REL40, VIRTUAL_INTERRUPT_BIT
a:
    sahf
    pop     ax
    pop     ds
    jmp     short c
b:
    sti
    jmp     short a
c:
endm

FIRET MACRO
    local a,b,d,e,f,g,i,j,k
    push    ds
    push    ax

;; Do real IRET on MIPS or if interrupts are pending

    mov     ax,40h
    mov     ds,ax
    test    word ptr ds:FIXED_NTVDMSTATE_REL40, MIPS_BIT_MASK OR RI_BIT_MASK
    jnz     short b

;; running on x86 can assume 386 or above instructions
    push    bp
    mov     bp,sp
    mov     ax,[bp+10]      ; get flags
    pop     bp
    test    ax,100h         ; test if trap flag is set
    jnz     short b         ; if so, do iret

    test    ax,200h         ; test if interrupt flag is set
    jz      short i         ; ZR -> flag image has IF not set
    lock    or word ptr ds:FIXED_NTVDMSTATE_REL40, VIRTUAL_INTERRUPT_BIT
    test    word ptr ds:FIXED_NTVDMSTATE_REL40, INTERRUPT_PENDING_BIT
    jnz     short b
j:
    xchg    ah,al           ; AH=low byte AL=high byte
    cld
    test    al,4            ; check direction flag
    jnz     short d         ;
e:
    test    al,8            ; check overflow flag
    jnz     short f         ; go to f if flag image has OF set
    jo      short k         ; go to k to reset OF
g:
    sahf                    ; set low byte of flags from ah
    pop     ax
    pop     ds
    retf    2               ; IRET and discard flags
i:
    lock    and word ptr ds:FIXED_NTVDMSTATE_REL40,NOT VIRTUAL_INTERRUPT_BIT
    jmp     short j
f:
    jo      short g         ; all OK if OF bit set in real flag
    ; set the overflow bit in real flag
    push    ax
    mov     al,127
    add     al,2            ; will set OF
    pop     ax
    jmp     short g

k:
    ; reset the OF
    push    ax
    xor     al,al           ; will reset OF
    pop     ax
    jmp     short g
d:
    std
    jmp     short e
b:
    pop     ax
    pop     ds
    iret
endm

 */
