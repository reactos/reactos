/*
 *  FreeLoader
 *  Copyright (C) 1998-2002  Brian Palmer  <brianp@sginet.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <asm.inc>
#include <arch/pc/x86common.h>

EXTERN _i386PrintExceptionText@12:PROC

.code32

MACRO(SAVE_CPU_REGS)
    /* push the rest of the KTRAP_FRAME */
    push ebp
    push ebx
    push esi
    push edi
    push fs
    push 0 // ExceptionList
    push 0 // PreviousPreviousMode
    push eax
    push ecx
    push edx
    push ds
    push es
    push gs
    mov eax, dr7
    push eax
    mov eax, dr6
    push eax
    mov eax, dr3
    push eax
    mov eax, dr2
    push eax
    mov eax, dr1
    push eax
    mov eax, dr0
    push eax
    sub esp, 6 * 4

    /* push KSPECIAL_REGISTERS */
    /* Gdtr, Idtr, Tr, Ldtr, Reserved */
    sub esp, 44
    sgdt [esp]
    sidt [esp + 8]
    str word ptr [esp + 16]
    sldt word ptr [esp + 18]
    mov eax, dr7;
    push eax
    mov eax, dr6;
    push eax
    mov eax, dr3;
    push eax
    mov eax, dr2;
    push eax
    mov eax, dr1;
    push eax
    mov eax, dr0;
    push eax
    mov eax, cr4;
    push eax
    mov eax, cr3;
    push eax
    mov eax, cr2;
    push eax
    mov eax, cr0;
    push eax
ENDM

/* Set by each exception handler to the index of the description text */
i386ExceptionIndex:
    .long   0

/************************************************************************/
i386CommonExceptionHandler:

    SAVE_CPU_REGS

    lea eax, [esp + (21 * 4)] // KTRAP_FRAME
    push esp // KSPECIAL_REGISTERS
    push eax
    push dword ptr ds:[i386ExceptionIndex]
    call _i386PrintExceptionText@12

    cli
i386ExceptionHandlerHang:
    hlt
    jmp i386ExceptionHandlerHang

    iret

MACRO(TRAP_STUB, function, index)
    PUBLIC VAL(function)
    &function:
    push 0 // Fake error code
    mov dword ptr ds:[i386ExceptionIndex], VAL(index)
    jmp i386CommonExceptionHandler
ENDM

MACRO(TRAP_STUB2, function, index)
    PUBLIC VAL(function)
    &function:
    mov dword ptr ds:[i386ExceptionIndex], VAL(index)
    jmp i386CommonExceptionHandler
ENDM

/************************************************************************/
TRAP_STUB _i386DivideByZero, 0

TRAP_STUB _i386DebugException, 1
TRAP_STUB _i386NMIException, 2
TRAP_STUB _i386Breakpoint, 3
TRAP_STUB _i386Overflow, 4
TRAP_STUB _i386BoundException, 5
TRAP_STUB _i386InvalidOpcode, 6
TRAP_STUB _i386FPUNotAvailable, 7
TRAP_STUB2 _i386DoubleFault, 8
TRAP_STUB _i386CoprocessorSegment, 9
TRAP_STUB2 _i386InvalidTSS, 10
TRAP_STUB2 _i386SegmentNotPresent, 11
TRAP_STUB2 _i386StackException, 12
TRAP_STUB2 _i386GeneralProtectionFault, 13
TRAP_STUB2 _i386PageFault, 14
// 15 is reserved
TRAP_STUB _i386CoprocessorError, 16
TRAP_STUB _i386AlignmentCheck, 17
TRAP_STUB _i386MachineCheck, 18
TRAP_STUB _i386SimdFloatError, 19

/************************************************************************
 * DEBUGGING SUPPORT FUNCTIONS
 ************************************************************************/

MACRO(BREAKPOINT_TEMPLATE, functionname, mask1, mask2)
    PUBLIC VAL(functionname)
#ifdef _USE_ML
    functionname:
#else
    \functionname:
#endif
    push eax
    mov eax, [esp + 8]
    mov dr3, eax
    mov eax, dr7
    and eax, VAL(mask1)
    or  eax, VAL(mask2)
    mov dr7, eax
    pop eax
    ret
ENDM

BREAKPOINT_TEMPLATE _INSTRUCTION_BREAKPOINT1, HEX(0fff0ffff), HEX(000000303)
BREAKPOINT_TEMPLATE _MEMORY_READWRITE_BREAKPOINT1, HEX(0fff0ffff), HEX(000030303)
BREAKPOINT_TEMPLATE _MEMORY_WRITE_BREAKPOINT1, HEX(0fff0ffff), HEX(000010303)
BREAKPOINT_TEMPLATE _INSTRUCTION_BREAKPOINT2, HEX(0ff0fffff), HEX(00000030c)
BREAKPOINT_TEMPLATE _MEMORY_READWRITE_BREAKPOINT2, HEX(0ff0fffff), HEX(00030030c)
BREAKPOINT_TEMPLATE _MEMORY_WRITE_BREAKPOINT2, HEX(0ff0fffff), HEX(00010030c)
BREAKPOINT_TEMPLATE _INSTRUCTION_BREAKPOINT3, HEX(0f0ffffff), HEX(000000330)
BREAKPOINT_TEMPLATE _MEMORY_READWRITE_BREAKPOINT3, HEX(0f0ffffff), HEX(003000330)
BREAKPOINT_TEMPLATE _MEMORY_WRITE_BREAKPOINT3, HEX(0f0ffffff), HEX(001000330)
BREAKPOINT_TEMPLATE _INSTRUCTION_BREAKPOINT4, HEX(00fffffff), HEX(0000003c0)
BREAKPOINT_TEMPLATE _MEMORY_READWRITE_BREAKPOINT4, HEX(00fffffff), HEX(0300003c0)
BREAKPOINT_TEMPLATE _MEMORY_WRITE_BREAKPOINT4, HEX(00fffffff), HEX(0100003c0)

END
