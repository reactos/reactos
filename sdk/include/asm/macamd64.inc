/*
 * PROJECT:     ReactOS SDK
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     ReactOS AMD64 architecture asm macros
 * COPYRIGHT:   Timo Kreuzer (timo.kreuzer@reactos.org)
 */

#include <asm.inc>

MACRO(push_reg, Reg)
    push Reg
    .pushreg Reg
ENDM

MACRO(rex_push_reg, Reg)
    db HEX(048) ; REX prefix
    push Reg
    .pushreg Reg
ENDM

MACRO(push_eflags)
    pushfq
    .allocstack 8
ENDM

MACRO(rex_push_eflags)
    db HEX(048) ; REX prefix
    pushfq
    .allocstack 8
ENDM

MACRO(rex_jmp_reg, Reg)
    db HEX(048) ; REX prefix / hint tail call to unwinder
    jmp Reg
ENDM

MACRO(ret_zero)
    ret
ENDM

MACRO(alloc_stack, Size)
    sub rsp, Size
    .allocstack Size
ENDM

MACRO(save_reg, Reg, Offset)
    mov [rsp + Offset], Reg
    .savereg Reg, Offset
ENDM

MACRO(save_xmm128, Reg, Offset)
    movaps  [rsp + Offset], Reg
    .savexmm128 Reg, Offset
ENDM

MACRO(push_frame, Code)
    .pushframe Code
ENDM

MACRO(set_frame, Reg, Offset)
    if Offset
        lea Reg, [rsp + Offset]
    else
        mov Reg, rsp
    endif
    .setframe Reg, Offset
ENDM

MACRO(END_PROLOGUE)
    .endprolog
ENDM

MACRO(BEGIN_EPILOGUE)
    .beginepilog
ENDM

MACRO(LEAF_ENTRY, Name, Section, NoPad)
//Section segment para 'CODE'
//ifb <NoPad>
//    db 6 dup (0cch)
//endif
    align 16
    public Name
.PROC Name
    END_PROLOGUE
ENDM

MACRO(LEAF_ENTRY_ARG1, Name, Section, Arg1, NoPad)
//Section segment para 'CODE'
//ifb <NoPad>
//    db 6 dup (0cch)
//endif
    align 16
    public Name
.PROC Name
    END_PROLOGUE
ENDM

MACRO(LEAF_ENTRY_ARG2, Name, Section, Arg1, Arg2, NoPad)
//Section segment para 'CODE'
//ifb <NoPad>
//    db 6 dup (0cch)
//endif
    align 16
    public Name
.PROC Name
    END_PROLOGUE
ENDM

MACRO(LEAF_ENTRY_ARG3, Name, Section, Arg1, Arg2, Arg3, NoPad)
//Section segment para 'CODE'
//ifb <NoPad>
//    db 6 dup (0cch)
//endif
    align 16
    public Name
.PROC Name
    END_PROLOGUE
ENDM

MACRO(LEAF_END, Name, Section)
.ENDP
//Section ends
ENDM

MACRO(NESTED_ENTR, Name, Section, Handler, NoPad)
    ifdef _CurrentSection_
        ifdif <Section>, _CurrentSection_
            .err <NESTED_ENTRY invoked for different sections within same module>
        endif
    endif
    _CurrentSection_ EQU <Section>
    Section segment para 'CODE'
    ifb <NoPad>
        db      6 dup (0cch)
    endif
    align   16
    public  Name
    ifb <Handler>
Name    proc    frame
    else
Name    proc    frame:Handler
    endif
ENDM

MACRO(NESTED_END, Name, section)
Name    endp
Section ends
ENDM

MACRO(ALTERNATE_ENTRY, Name)
Name:
ENDM

MACRO(Yield)
    pause
ENDM

/* EOF */
