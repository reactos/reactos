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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

	.text
	.code16

#define ASM
#include <arch.h>



.macro SAVE_CPU_REGS
	movl	%eax,i386_EAX
	movl	%ebx,i386_EBX
	movl	%ecx,i386_ECX
	movl	%edx,i386_EDX
	movl	%esp,i386_ESP
	movl	%ebp,i386_EBP
	movl	%esi,i386_ESI
	movl	%edi,i386_EDI
	movw	%ds,%ax
	movw	%ax,i386_DS
	movw	%es,%ax
	movw	%ax,i386_ES
	movw	%fs,%ax
	movw	%ax,i386_FS
	movw	%gs,%ax
	movw	%ax,i386_GS
	movw	%ss,%ax
	movw	%ax,i386_SS
	popl	%eax
	movl	%eax,i386_EIP
	popl	%eax
	movw	%ax,i386_CS
	popl	%eax
	movl	%eax,i386_EFLAGS
	movl	%cr0,%eax
	movl	%eax,i386_CR0
	//movl	%cr1,%eax
	//movl	%eax,i386_CR1
	movl	%cr2,%eax
	movl	%eax,i386_CR2
	movl	%cr3,%eax
	movl	%eax,i386_CR3
	movl	%dr0,%eax
	movl	%eax,i386_DR0
	movl	%dr1,%eax
	movl	%eax,i386_DR1
	movl	%dr2,%eax
	movl	%eax,i386_DR2
	movl	%dr3,%eax
	movl	%eax,i386_DR3
	movl	%dr6,%eax
	movl	%eax,i386_DR6
	movl	%dr7,%eax
	movl	%eax,i386_DR7
	sgdt	i386_GDTR
	sidt	i386_IDTR
	sldt	i386_LDTR
	str		i386_TR
.endm

.macro SAVE_CPU_REGS_ERROR_CODE
	movl	%eax,i386_EAX
	movl	%ebx,i386_EBX
	movl	%ecx,i386_ECX
	movl	%edx,i386_EDX
	movl	%esp,i386_ESP
	movl	%ebp,i386_EBP
	movl	%esi,i386_ESI
	movl	%edi,i386_EDI
	movw	%ds,%ax
	movw	%ax,i386_DS
	movw	%es,%ax
	movw	%ax,i386_ES
	movw	%fs,%ax
	movw	%ax,i386_FS
	movw	%gs,%ax
	movw	%ax,i386_GS
	movw	%ss,%ax
	movw	%ax,i386_SS
	popl	%eax
	movl	%eax,i386_ERROR_CODE
	popl	%eax
	movl	%eax,i386_EIP
	popl	%eax
	movw	%ax,i386_CS
	popl	%eax
	movl	%eax,i386_EFLAGS
	movl	%cr0,%eax
	movl	%eax,i386_CR0
	//movl	%cr1,%eax
	//movl	%eax,i386_CR1
	movl	%cr2,%eax
	movl	%eax,i386_CR2
	movl	%cr3,%eax
	movl	%eax,i386_CR3
	movl	%dr0,%eax
	movl	%eax,i386_DR0
	movl	%dr1,%eax
	movl	%eax,i386_DR1
	movl	%dr2,%eax
	movl	%eax,i386_DR2
	movl	%dr3,%eax
	movl	%eax,i386_DR3
	movl	%dr6,%eax
	movl	%eax,i386_DR6
	movl	%dr7,%eax
	movl	%eax,i386_DR7
	sgdt	i386_GDTR
	sidt	i386_IDTR
	sldt	i386_LDTR
	str		i386_TR
.endm



i386ExceptionHandlerText:
	.ascii "FreeLoader i386 Exception Handler\n"
	.asciz "Report bugs to Brian Palmer <brianp@reactos.org>\n\n"

i386DivideByZeroText:
	.asciz "Exception 00: DIVIDE BY ZERO\n\n"
i386DebugExceptionText:
	.asciz "Exception 01: DEBUG EXCEPTION\n\n"
i386NMIExceptionText:
	.asciz "Exception 02: NON-MASKABLE INTERRUPT EXCEPTION\n\n"
i386BreakpointText:
	.asciz "Exception 03: BREAKPOINT (INT 3)\n\n"
i386OverflowText:
	.asciz "Exception 04: OVERFLOW\n\n"
i386BoundExceptionText:
	.asciz "Exception 05: BOUND EXCEPTION\n\n"
i386InvalidOpcodeText:
	.asciz "Exception 06: INVALID OPCODE\n\n"
i386FPUNotAvailableText:
	.asciz "Exception 07: FPU NOT AVAILABLE\n\n"
i386DoubleFaultText:
	.asciz "Exception 08: DOUBLE FAULT\n\n"
i386CoprocessorSegmentText:
	.asciz "Exception 09: COPROCESSOR SEGMENT OVERRUN\n\n"
i386InvalidTSSText:
	.asciz "Exception 0A: INVALID TSS\n\n"
i386SegmentNotPresentText:
	.asciz "Exception 0B: SEGMENT NOT PRESENT\n\n"
i386StackExceptionText:
	.asciz "Exception 0C: STACK EXCEPTION\n\n"
i386GeneralProtectionFaultText:
	.asciz "Exception 0D: GENERAL PROTECTION FAULT\n\n"
i386PageFaultText:
	.asciz "Exception 0E: PAGE FAULT\n\n"
i386CoprocessorErrorText:
	.asciz "Exception 10: COPROCESSOR ERROR\n\n"
i386AlignmentCheckText:
	.asciz "Exception 11: ALIGNMENT CHECK\n\n"
i386MachineCheckText:
	.asciz "Exception 12: MACHINE CHECK\n\n"

i386_EAX_Text:
	.asciz "EAX: "
i386_EBX_Text:
	.asciz "EBX: "
i386_ECX_Text:
	.asciz "ECX: "
i386_EDX_Text:
	.asciz "EDX: "
i386_ESP_Text:
	.asciz "        ESP: "
i386_EBP_Text:
	.asciz "        EBP: "
i386_ESI_Text:
	.asciz "        ESI: "
i386_EDI_Text:
	.asciz "        EDI: "
i386_CS_Text:
	.asciz "CS: "
i386_DS_Text:
	.asciz "DS: "
i386_ES_Text:
	.asciz "ES: "
i386_FS_Text:
	.asciz "FS: "
i386_GS_Text:
	.asciz "GS: "
i386_SS_Text:
	.asciz "SS: "
i386_EFLAGS_Text:
	.asciz "        EFLAGS: "
i386_EIP_Text:
	.asciz "        EIP: "
i386_ERROR_CODE_Text:
	.asciz "        ERROR CODE: "
i386_CR0_Text:
	.asciz "        CR0: "
i386_CR1_Text:
	.asciz "        CR1: "
i386_CR2_Text:
	.asciz "        CR2: "
i386_CR3_Text:
	.asciz "        CR3: "
i386_DR0_Text:
	.asciz "        DR0: "
i386_DR1_Text:
	.asciz "        DR1: "
i386_DR2_Text:
	.asciz "        DR2: "
i386_DR3_Text:
	.asciz "        DR3: "
i386_DR6_Text:
	.asciz "        DR6: "
i386_DR7_Text:
	.asciz "        DR7: "
i386_GDTR_Text:
	.asciz "        GDTR Base: "
i386_IDTR_Text:
	.asciz "        IDTR Base: "
i386_Limit_Text:
	.asciz " Limit: "
i386_LDTR_Text:
	.asciz "        LDTR: "
i386_TR_Text:
	.asciz " TR: "

/* Set by each exception handler to the address of the description text */
i386ExceptionDescriptionText:
	.long	0

/* Used to store the contents of all the registers when an exception occurs */
i386_EAX:
	.long	0
i386_EBX:
	.long	0
i386_ECX:
	.long	0
i386_EDX:
	.long	0
i386_ESP:
	.long	0
i386_EBP:
	.long	0
i386_ESI:
	.long	0
i386_EDI:
	.long	0
i386_CS:
	.word	0
i386_DS:
	.word	0
i386_ES:
	.word	0
i386_FS:
	.word	0
i386_GS:
	.word	0
i386_SS:
	.word	0
i386_EFLAGS:
	.long	0
i386_EIP:
	.long	0
i386_ERROR_CODE:
	.long	0
i386_CR0:
	.long	0
i386_CR1:
	.long	0
i386_CR2:
	.long	0
i386_CR3:
	.long	0
i386_DR0:
	.long	0
i386_DR1:
	.long	0
i386_DR2:
	.long	0
i386_DR3:
	.long	0
i386_DR6:
	.long	0
i386_DR7:
	.long	0
i386_GDTR:
	.word	0
	.long	0
i386_IDTR:
	.word	0
	.long	0
i386_LDTR:
	.word	0
i386_TR:
	.word	0

/* Used to store the current X and Y position on the screen */
i386_ScreenPosX:
	.long	0
i386_ScreenPosY:
	.long	0

/************************************************************************/
i386CommonExceptionHandler:
	.code32

	call	i386ClearScreenToBlue

	movl	$i386ExceptionHandlerText,%esi
	call	i386PrintText

	movl	i386ExceptionDescriptionText,%esi
	call	i386PrintText

	movl	$i386_EAX_Text,%esi
	call	i386PrintText
	movl	i386_EAX,%eax
	call	i386PrintHexDword					// Display EAX
	movl	$i386_ESP_Text,%esi
	call	i386PrintText
	movl	i386_ESP,%eax
	call	i386PrintHexDword					// Display ESP
	movl	$i386_CR0_Text,%esi
	call	i386PrintText
	movl	i386_CR0,%eax
	call	i386PrintHexDword					// Display CR0
	movl	$i386_DR0_Text,%esi
	call	i386PrintText
	movl	i386_DR0,%eax
	call	i386PrintHexDword					// Display DR0
	movl	$0,i386_ScreenPosX
	incl	i386_ScreenPosY
	movl	$i386_EBX_Text,%esi
	call	i386PrintText
	movl	i386_EBX,%eax
	call	i386PrintHexDword					// Display EBX
	movl	$i386_EBP_Text,%esi
	call	i386PrintText
	movl	i386_EBP,%eax
	call	i386PrintHexDword					// Display EBP
	movl	$i386_CR1_Text,%esi
	call	i386PrintText
	movl	i386_CR1,%eax
	call	i386PrintHexDword					// Display CR1
	movl	$i386_DR1_Text,%esi
	call	i386PrintText
	movl	i386_DR1,%eax
	call	i386PrintHexDword					// Display DR1
	movl	$0,i386_ScreenPosX
	incl	i386_ScreenPosY
	movl	$i386_ECX_Text,%esi
	call	i386PrintText
	movl	i386_ECX,%eax
	call	i386PrintHexDword					// Display ECX
	movl	$i386_ESI_Text,%esi
	call	i386PrintText
	movl	i386_ESI,%eax
	call	i386PrintHexDword					// Display ESI
	movl	$i386_CR2_Text,%esi
	call	i386PrintText
	movl	i386_CR2,%eax
	call	i386PrintHexDword					// Display CR2
	movl	$i386_DR2_Text,%esi
	call	i386PrintText
	movl	i386_DR2,%eax
	call	i386PrintHexDword					// Display DR2
	movl	$0,i386_ScreenPosX
	incl	i386_ScreenPosY
	movl	$i386_EDX_Text,%esi
	call	i386PrintText
	movl	i386_EDX,%eax
	call	i386PrintHexDword					// Display EDX
	movl	$i386_EDI_Text,%esi
	call	i386PrintText
	movl	i386_EDI,%eax
	call	i386PrintHexDword					// Display EDI
	movl	$i386_CR3_Text,%esi
	call	i386PrintText
	movl	i386_CR3,%eax
	call	i386PrintHexDword					// Display CR3
	movl	$i386_DR3_Text,%esi
	call	i386PrintText
	movl	i386_DR3,%eax
	call	i386PrintHexDword					// Display DR3
	incl	i386_ScreenPosY
	movl	$55,i386_ScreenPosX
	movl	$i386_DR6_Text,%esi
	call	i386PrintText
	movl	i386_DR6,%eax
	call	i386PrintHexDword					// Display DR6
	incl	i386_ScreenPosY
	movl	$55,i386_ScreenPosX
	movl	$i386_DR7_Text,%esi
	call	i386PrintText
	movl	i386_DR7,%eax
	call	i386PrintHexDword					// Display DR7
	movl	$0,i386_ScreenPosX
	incl	i386_ScreenPosY
	incl	i386_ScreenPosY
	movl	$i386_CS_Text,%esi
	call	i386PrintText
	movw	i386_CS,%ax
	call	i386PrintHexWord					// Display CS
	movl	$i386_EIP_Text,%esi
	call	i386PrintText
	movl	i386_EIP,%eax
	call	i386PrintHexDword					// Display EIP
	movl	$0,i386_ScreenPosX
	incl	i386_ScreenPosY
	movl	$i386_DS_Text,%esi
	call	i386PrintText
	movw	i386_DS,%ax
	call	i386PrintHexWord					// Display DS
	movl	$i386_ERROR_CODE_Text,%esi
	call	i386PrintText
	movl	i386_ERROR_CODE,%eax
	call	i386PrintHexDword					// Display ERROR CODE
	movl	$0,i386_ScreenPosX
	incl	i386_ScreenPosY
	movl	$i386_ES_Text,%esi
	call	i386PrintText
	movw	i386_ES,%ax
	call	i386PrintHexWord					// Display ES
	movl	$i386_EFLAGS_Text,%esi
	call	i386PrintText
	movl	i386_EFLAGS,%eax
	call	i386PrintHexDword					// Display EFLAGS
	movl	$0,i386_ScreenPosX
	incl	i386_ScreenPosY
	movl	$i386_FS_Text,%esi
	call	i386PrintText
	movw	i386_FS,%ax
	call	i386PrintHexWord					// Display FS
	movl	$i386_GDTR_Text,%esi
	call	i386PrintText
	movl	i386_GDTR+2,%eax
	call	i386PrintHexDword					// Display GDTR Base
	movl	$i386_Limit_Text,%esi
	call	i386PrintText
	movw	i386_GDTR,%ax
	call	i386PrintHexWord					// Display GDTR Limit
	movl	$0,i386_ScreenPosX
	incl	i386_ScreenPosY
	movl	$i386_GS_Text,%esi
	call	i386PrintText
	movw	i386_GS,%ax
	call	i386PrintHexWord					// Display GS
	movl	$i386_IDTR_Text,%esi
	call	i386PrintText
	movl	i386_IDTR+2,%eax
	call	i386PrintHexDword					// Display IDTR Base
	movl	$i386_Limit_Text,%esi
	call	i386PrintText
	movw	i386_IDTR,%ax
	call	i386PrintHexWord					// Display IDTR Limit
	movl	$0,i386_ScreenPosX
	incl	i386_ScreenPosY
	movl	$i386_SS_Text,%esi
	call	i386PrintText
	movw	i386_SS,%ax
	call	i386PrintHexWord					// Display SS
	movl	$i386_LDTR_Text,%esi
	call	i386PrintText
	movw	i386_LDTR,%ax
	call	i386PrintHexWord					// Display LDTR
	movl	$i386_TR_Text,%esi
	call	i386PrintText
	movw	i386_TR,%ax
	call	i386PrintHexWord					// Display TR
	movl	$0,i386_ScreenPosX
	incl	i386_ScreenPosY
	incl	i386_ScreenPosY

i386ExceptionHandlerHang:
	jmp		i386ExceptionHandlerHang

	iret

/************************************************************************/
i386ClearScreenToBlue:
	.code32

	cld
	movw	$0x1F20,%ax
	movl	$0xB0000,%edi
	movl	$0x8000,%ecx
	rep stosw

	ret

/************************************************************************/
/* ESI = Address of text to display                                     */
/************************************************************************/
i386PrintText:
	.code32

i386PrintTextLoop:
	lodsb

	// Check for end of string char
	cmp		$0,%al
	je		i386PrintTextDone

	// Check for newline char
	cmp		$0x0a,%al
	jne		i386PrintTextLoop2
	incl	i386_ScreenPosY
	movl	$0,i386_ScreenPosX
	jmp		i386PrintTextLoop

i386PrintTextLoop2:
	call	i386PrintTextCalcAddressOfNextChar

	stosb
	incl	i386_ScreenPosX

	jmp		i386PrintTextLoop

i386PrintTextDone:
	
	ret

/************************************************************************/
/* On return EDI = Address of next char in screen memory                */
/************************************************************************/
i386PrintTextCalcAddressOfNextChar:
	.code32

	push	%eax

	movl	$0xB8000,%edi
	addl	i386_ScreenPosX,%edi
	addl	i386_ScreenPosX,%edi
	movl	i386_ScreenPosY,%eax
	movl	$160,%ecx				// 80 columns, 2 bytes per column
	mull	%ecx
	addl	%eax,%edi

	pop		%eax

	ret

/************************************************************************/
/* Prints the value in EAX on the screen in hex                         */
/************************************************************************/
i386PrintHexDword:
	.code32

	call	i386PrintHex1

i386PrintHex1:
	call	i386PrintHex2
i386PrintHex2:
	call	i386PrintHex3
i386PrintHex3:
	movb	$4,%cl
	rol		%cl,%eax
	push	%eax
	andb	$0x0f,%al
	movl	$i386PrintHexTable,%ebx
	xlat	/*$i386PrintHexTable*/
	call	i386PrintTextCalcAddressOfNextChar
	stosb
	incl	i386_ScreenPosX
	pop		%eax

	ret

i386PrintHexTable:
	.ascii	"0123456789ABCDEF"

/************************************************************************/
/* Prints the value in AX on the screen in hex                         */
/************************************************************************/
i386PrintHexWord:
	.code32

	call	i386PrintHexWord1
i386PrintHexWord1:
	call	i386PrintHexWord2
i386PrintHexWord2:
	movb	$4,%cl
	rol		%cl,%ax
	push	%eax
	andb	$0x0f,%al
	movl	$i386PrintHexTable,%ebx
	xlat	/*$i386PrintHexTable*/
	call	i386PrintTextCalcAddressOfNextChar
	stosb
	incl	i386_ScreenPosX
	pop		%eax

	ret

/************************************************************************/
/* Prints the value in AL on the screen in hex                         */
/************************************************************************/
i386PrintHexByte:
	.code32

	call	i386PrintHexByte1
i386PrintHexByte1:
	movb	$4,%cl
	rol		%cl,%al
	push	%eax
	andb	$0x0f,%al
	movl	$i386PrintHexTable,%ebx
	xlat	/*$i386PrintHexTable*/
	call	i386PrintTextCalcAddressOfNextChar
	stosb
	incl	i386_ScreenPosX
	pop		%eax

	ret

/************************************************************************/
EXTERN(i386DivideByZero)
	.code32

	SAVE_CPU_REGS

	movl	$i386DivideByZeroText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386DebugException)
	.code32

	SAVE_CPU_REGS

	movl	$i386DebugExceptionText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386NMIException)
	.code32

	SAVE_CPU_REGS

	movl	$i386NMIExceptionText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386Breakpoint)
	.code32

	SAVE_CPU_REGS

	movl	$i386BreakpointText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386Overflow)
	.code32

	SAVE_CPU_REGS

	movl	$i386OverflowText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386BoundException)
	.code32

	SAVE_CPU_REGS

	movl	$i386BoundExceptionText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386InvalidOpcode)
	.code32

	SAVE_CPU_REGS

	movl	$i386InvalidOpcodeText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386FPUNotAvailable)
	.code32

	SAVE_CPU_REGS

	movl	$i386FPUNotAvailableText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386DoubleFault)
	.code32

	SAVE_CPU_REGS_ERROR_CODE

	movl	$i386DoubleFaultText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386CoprocessorSegment)
	.code32

	SAVE_CPU_REGS

	movl	$i386CoprocessorSegmentText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386InvalidTSS)
	.code32

	SAVE_CPU_REGS_ERROR_CODE

	movl	$i386InvalidTSSText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386SegmentNotPresent)
	.code32

	SAVE_CPU_REGS_ERROR_CODE

	movl	$i386SegmentNotPresentText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386StackException)
	.code32

	SAVE_CPU_REGS_ERROR_CODE

	movl	$i386StackExceptionText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386GeneralProtectionFault)
	.code32

	SAVE_CPU_REGS_ERROR_CODE

	movl	$i386GeneralProtectionFaultText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386PageFault)
	.code32

	SAVE_CPU_REGS_ERROR_CODE

	movl	$i386PageFaultText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386CoprocessorError)
	.code32

	SAVE_CPU_REGS

	movl	$i386CoprocessorErrorText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386AlignmentCheck)
	.code32

	SAVE_CPU_REGS

	movl	$i386AlignmentCheckText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************/
EXTERN(i386MachineCheck)
	.code32

	SAVE_CPU_REGS

	movl	$i386MachineCheckText,i386ExceptionDescriptionText
	jmp		i386CommonExceptionHandler

/************************************************************************
 * DEBUGGING SUPPORT FUNCTIONS
 ************************************************************************/
EXTERN(_INSTRUCTION_BREAKPOINT1)
	.code32

	pushl	%eax

	movl	8(%esp),%eax

	movl	%eax,%dr0
	movl	%dr7,%eax
	andl	$0xfff0ffff,%eax
	orl		$0x00000303,%eax
	movl	%eax,%dr7

	popl	%eax

	ret

EXTERN(_MEMORY_READWRITE_BREAKPOINT1)
	.code32

	pushl	%eax

	movl	8(%esp),%eax

	movl	%eax,%dr0
	movl	%dr7,%eax
	andl	$0xfff0ffff,%eax
	orl		$0x00030303,%eax
	movl	%eax,%dr7

	popl	%eax

	ret

EXTERN(_MEMORY_WRITE_BREAKPOINT1)
	.code32

	pushl	%eax

	movl	8(%esp),%eax

	movl	%eax,%dr0
	movl	%dr7,%eax
	andl	$0xfff0ffff,%eax
	orl		$0x00010303,%eax
	movl	%eax,%dr7

	popl	%eax

	ret

EXTERN(_INSTRUCTION_BREAKPOINT2)
	.code32

	pushl	%eax

	movl	8(%esp),%eax

	movl	%eax,%dr1
	movl	%dr7,%eax
	andl	$0xff0fffff,%eax
	orl		$0x0000030c,%eax
	movl	%eax,%dr7

	popl	%eax

	ret

EXTERN(_MEMORY_READWRITE_BREAKPOINT2)
	.code32

	pushl	%eax

	movl	8(%esp),%eax

	movl	%eax,%dr1
	movl	%dr7,%eax
	andl	$0xff0fffff,%eax
	orl		$0x0030030c,%eax
	movl	%eax,%dr7

	popl	%eax

	ret

EXTERN(_MEMORY_WRITE_BREAKPOINT2)
	.code32

	pushl	%eax

	movl	8(%esp),%eax

	movl	%eax,%dr1
	movl	%dr7,%eax
	andl	$0xff0fffff,%eax
	orl		$0x0010030c,%eax
	movl	%eax,%dr7

	popl	%eax

	ret

EXTERN(_INSTRUCTION_BREAKPOINT3)
	.code32

	pushl	%eax

	movl	8(%esp),%eax

	movl	%eax,%dr2
	movl	%dr7,%eax
	andl	$0xf0ffffff,%eax
	orl		$0x00000330,%eax
	movl	%eax,%dr7

	popl	%eax

	ret

EXTERN(_MEMORY_READWRITE_BREAKPOINT3)
	.code32

	pushl	%eax

	movl	8(%esp),%eax

	movl	%eax,%dr2
	movl	%dr7,%eax
	andl	$0xf0ffffff,%eax
	orl		$0x03000330,%eax
	movl	%eax,%dr7

	popl	%eax

	ret

EXTERN(_MEMORY_WRITE_BREAKPOINT3)
	.code32

	pushl	%eax

	movl	8(%esp),%eax

	movl	%eax,%dr2
	movl	%dr7,%eax
	andl	$0xf0ffffff,%eax
	orl		$0x01000330,%eax
	movl	%eax,%dr7

	popl	%eax

	ret

EXTERN(_INSTRUCTION_BREAKPOINT4)
	.code32

	pushl	%eax

	movl	8(%esp),%eax

	movl	%eax,%dr3
	movl	%dr7,%eax
	andl	$0x0fffffff,%eax
	orl		$0x000003c0,%eax
	movl	%eax,%dr7

	popl	%eax

	ret

EXTERN(_MEMORY_READWRITE_BREAKPOINT4)
	.code32

	pushl	%eax

	movl	8(%esp),%eax

	movl	%eax,%dr3
	movl	%dr7,%eax
	andl	$0x0fffffff,%eax
	orl		$0x300003c0,%eax
	movl	%eax,%dr7

	popl	%eax

	ret

EXTERN(_MEMORY_WRITE_BREAKPOINT4)
	.code32

	pushl	%eax

	movl	8(%esp),%eax

	movl	%eax,%dr3
	movl	%dr7,%eax
	andl	$0x0fffffff,%eax
	orl		$0x100003c0,%eax
	movl	%eax,%dr7

	popl	%eax

	ret
