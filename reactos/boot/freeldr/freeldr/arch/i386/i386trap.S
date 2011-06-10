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

.intel_syntax noprefix
	.text
	.code16

#include <arch/pc/x86common.h>
#include <ver.h>

#define SCREEN_ATTR 0x1f			/* Bright white on blue background */

.macro SAVE_CPU_REGS
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
	str [esp + 16]
	sldt [esp + 18]
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
.endm

/* Set by each exception handler to the address of the description text */
i386ExceptionIndex:
	.long	0


/************************************************************************/
i386CommonExceptionHandler:
	.code32

	SAVE_CPU_REGS

    lea eax, [esp + (21 * 4)] // KTRAP_FRAME
    push esp // KSPECIAL_REGISTERS
    push eax
    push i386ExceptionIndex
    call _i386PrintExceptionText@12

	cli
i386ExceptionHandlerHang:
	hlt
	jmp	i386ExceptionHandlerHang

	iret

.macro TRAP_STUB function, index
    EXTERN(\function)
    push 0 // Fake error code
	mov dword ptr i386ExceptionIndex, \index
	jmp i386CommonExceptionHandler
.endm

.macro TRAP_STUB2 function, index
    EXTERN(\function)
	mov dword ptr i386ExceptionIndex, \index
	jmp i386CommonExceptionHandler
.endm

/************************************************************************/
TRAP_STUB i386DivideByZero, 0
TRAP_STUB i386DebugException, 1
TRAP_STUB i386NMIException, 2
TRAP_STUB i386Breakpoint, 3
TRAP_STUB i386Overflow, 4
TRAP_STUB i386BoundException, 5
TRAP_STUB i386InvalidOpcode, 6
TRAP_STUB i386FPUNotAvailable, 7
TRAP_STUB2 i386DoubleFault, 8
TRAP_STUB i386CoprocessorSegment, 9
TRAP_STUB2 i386InvalidTSS, 10
TRAP_STUB2 i386SegmentNotPresent, 11
TRAP_STUB2 i386StackException, 12
TRAP_STUB2 i386GeneralProtectionFault, 13
TRAP_STUB2 i386PageFault, 14
// 15 is reserved
TRAP_STUB i386CoprocessorError, 16
TRAP_STUB i386AlignmentCheck, 17
TRAP_STUB i386MachineCheck, 18
TRAP_STUB i386SimdFloatError, 19

/************************************************************************
 * DEBUGGING SUPPORT FUNCTIONS
 ************************************************************************/

.macro BREAKPOINT_TEPLATE functionname, mask1, mask2
    EXTERN(\functionname)
	push eax
	mov eax, [esp + 8]
	mov dr3, eax
	mov eax, dr7
	and eax, \mask1
	or  eax, \mask2
	mov dr7, eax
	pop eax
	ret
.endm

BREAKPOINT_TEPLATE _INSTRUCTION_BREAKPOINT1, 0xfff0ffff, 0x00000303
BREAKPOINT_TEPLATE _MEMORY_READWRITE_BREAKPOINT1, 0xfff0ffff, 0x00030303
BREAKPOINT_TEPLATE _MEMORY_WRITE_BREAKPOINT1, 0xfff0ffff, 0x00010303
BREAKPOINT_TEPLATE _INSTRUCTION_BREAKPOINT2, 0xff0fffff, 0x0000030c
BREAKPOINT_TEPLATE _MEMORY_READWRITE_BREAKPOINT2, 0xff0fffff, 0x0030030c
BREAKPOINT_TEPLATE _MEMORY_WRITE_BREAKPOINT2, 0xff0fffff, 0x0010030c
BREAKPOINT_TEPLATE _INSTRUCTION_BREAKPOINT3, 0xf0ffffff, 0x00000330
BREAKPOINT_TEPLATE _MEMORY_READWRITE_BREAKPOINT3, 0xf0ffffff, 0x03000330
BREAKPOINT_TEPLATE _MEMORY_WRITE_BREAKPOINT3, 0xf0ffffff, 0x01000330
BREAKPOINT_TEPLATE _INSTRUCTION_BREAKPOINT4, 0x0fffffff, 0x000003c0
BREAKPOINT_TEPLATE _MEMORY_READWRITE_BREAKPOINT4, 0x0fffffff, 0x300003c0
BREAKPOINT_TEPLATE _MEMORY_WRITE_BREAKPOINT4, 0x0fffffff, 0x100003c0


