/* $Id: except.s,v 1.2 2002/10/26 07:32:08 chorns Exp $
 *
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS kernel
 * PURPOSE:           Kernel-mode exception support for IA-32
 * FILE:              ntoskrnl/rtl/i386/except.s
 * PROGRAMER:         Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:             This file is shared with lib/ntdll/rtl/i386/except.s.
 *                    Please keep them in sync.
 */

#define EXCEPTION_UNWINDING		0x02

#define EREC_FLAGS				0x04

#define ExceptionContinueExecution 0
#define ExceptionContinueSearch    1
#define ExceptionNestedException   2
#define ExceptionCollidedUnwind    3

.globl _RtlpExecuteHandlerForException
.globl _RtlpExecuteHandlerForUnwind

#define CONTEXT_FLAGS	0x00
#define CONTEXT_SEGGS	0x8C
#define CONTEXT_SEGFS	0x90
#define CONTEXT_SEGES	0x94
#define CONTEXT_SEGDS	0x98
#define CONTEXT_EDI		0x9C
#define CONTEXT_ESI		0xA0
#define CONTEXT_EBX		0xA4
#define CONTEXT_EDX		0xA8
#define CONTEXT_ECX		0xAC
#define CONTEXT_EAX		0xB0
#define CONTEXT_EBP		0xB4
#define CONTEXT_EIP		0xB8
#define CONTEXT_SEGCS	0xBC
#define CONTEXT_EFLAGS	0xC0
#define CONTEXT_ESP		0xC4
#define CONTEXT_SEGSS	0xC8


#define RCC_CONTEXT		0x08

// EAX = value to print
_do_debug:
	pushal
	pushl	%eax
	call	_AsmDebug@4
	popal
	ret

#ifndef __NTOSKRNL__

//
// VOID
// RtlpCaptureContext(PCONTEXT pContext);
//
// Parameters:
//   [ESP+08h] - PCONTEXT_X86 pContext
// Registers:
//   None
// Returns:
//   Nothing
// Notes:
//   Grabs the current CPU context.
.globl _RtlpCaptureContext
_RtlpCaptureContext:
	pushl   %ebp
    movl	%esp, %ebp
	movl	RCC_CONTEXT(%ebp), %edx		// EDX = Address of context structure

	cld
	pushf
	pop		%eax
	movl	%eax, CONTEXT_EFLAGS(%edx)
	xorl	%eax, %eax
	movl	%eax, CONTEXT_EAX(%edx)
	movl	%eax, CONTEXT_EBX(%edx)
	movl	%eax, CONTEXT_ECX(%edx)
	movl	%eax, CONTEXT_EDX(%edx)
	movl	%eax, CONTEXT_ESI(%edx)
	movl	%eax, CONTEXT_EDI(%edx)
	movl	%cs, %eax
	movl	%eax, CONTEXT_SEGCS(%edx)
	movl	%ds, %eax
	movl	%eax, CONTEXT_SEGDS(%edx)
	movl	%es, %eax
	movl	%eax, CONTEXT_SEGES(%edx)
	movl	%fs, %eax
	movl	%eax, CONTEXT_SEGFS(%edx)
	movl	%gs, %eax
	movl	%eax, CONTEXT_SEGGS(%edx)
	movl	%ss, %eax
	movl	%eax, CONTEXT_SEGSS(%edx)

	//
	// STACK LAYOUT: - (ESP to put in context structure)
	//               - RETURN ADDRESS OF CALLER OF CALLER
	//               - EBP OF CALLER OF CALLER
	//                 ...
	//               - RETURN ADDRESS OF CALLER
	//               - EBP OF CALLER
	//                 ...
	//

	// Get return address of the caller of the caller of this function
	movl	%ebp, %ebx
	//movl	4(%ebx), %eax			// EAX = return address of caller
	movl	(%ebx), %ebx			// EBX = EBP of caller

	movl	4(%ebx), %eax			// EAX = return address of caller of caller
	movl	(%ebx), %ebx			// EBX = EBP of caller of caller

	movl	%eax, CONTEXT_EIP(%edx)	// EIP = return address of caller of caller
	movl	%ebx, CONTEXT_EBP(%edx)	// EBP = EBP of caller of caller
	addl	$8, %ebx
	movl	%ebx, CONTEXT_ESP(%edx)	// ESP = EBP of caller of caller + 8

    movl	%ebp, %esp
    popl	%ebp
    ret

#endif /* !__NTOSKRNL__ */

#define REH_ERECORD		0x08
#define REH_RFRAME		0x0C
#define REH_CONTEXT		0x10
#define REH_DCONTEXT	0x14
#define REH_EROUTINE	0x18

// Parameters:
//   None
// Registers:
//   [EBP+08h] - PEXCEPTION_RECORD ExceptionRecord
//   [EBP+0Ch] - PEXCEPTION_REGISTRATION RegistrationFrame
//   [EBP+10h] - PVOID Context
//   [EBP+14h] - PVOID DispatcherContext
//   [EBP+18h] - PEXCEPTION_HANDLER ExceptionRoutine
//   EDX       - Address of protecting exception handler
// Returns:
//   EXCEPTION_DISPOSITION
// Notes:
//   Setup the protecting exception handler and call the exception
//   handler in the right context.
_RtlpExecuteHandler:
	pushl    %ebp
    movl     %esp, %ebp
    pushl    REH_RFRAME(%ebp)

    pushl    %edx
    pushl    %fs:0x0
    movl     %esp, %fs:0x0

    // Prepare to call the exception handler
    pushl    REH_DCONTEXT(%ebp)
    pushl    REH_CONTEXT(%ebp)
    pushl    REH_RFRAME(%ebp)
    pushl    REH_ERECORD(%ebp)

    // Now call the exception handler
    movl     REH_EROUTINE(%ebp), %eax
    call    *%eax

	cmpl	$-1, %fs:0x0
	jne		.reh_stack_looks_ok

	// This should not happen
	pushl	0
	pushl	0
	pushl	0
	pushl	0
	call	_RtlAssert@16

.reh_loop:
	jmp	.reh_loop
	
.reh_stack_looks_ok:
    movl     %fs:0x0, %esp

    // Return to the 'front-end' for this function
    popl     %fs:0x0
    movl     %ebp, %esp
    popl     %ebp
    ret


#define REP_ERECORD     0x04
#define REP_RFRAME      0x08
#define REP_CONTEXT     0x0C
#define REP_DCONTEXT    0x10

// Parameters:
//   [ESP+04h] - PEXCEPTION_RECORD ExceptionRecord
//   [ESP+08h] - PEXCEPTION_REGISTRATION RegistrationFrame
//   [ESP+0Ch] - PCONTEXT Context
//   [ESP+10h] - PVOID DispatcherContext
// Registers:
//   None
// Returns:
//   EXCEPTION_DISPOSITION
// Notes:
//    This exception handler protects the exception handling
//    mechanism by detecting nested exceptions.
_RtlpExceptionProtector:
    movl     $ExceptionContinueSearch, %eax
    movl     REP_ERECORD(%esp), %ecx
    testl    $EXCEPTION_UNWINDING, EREC_FLAGS(%ecx)
    jnz      .rep_end

    // Unwinding is not taking place, so return ExceptionNestedException

    // Set DispatcherContext field to the exception registration for the
    // exception handler that executed when a nested exception occurred
    movl     REP_DCONTEXT(%esp), %ecx
    movl     REP_RFRAME(%esp), %eax
    movl     %eax, (%ecx)
    movl     $ExceptionNestedException, %eax

.rep_end:
    ret


// Parameters:
//   [ESP+04h] - PEXCEPTION_RECORD ExceptionRecord
//   [ESP+08h] - PEXCEPTION_REGISTRATION RegistrationFrame
//   [ESP+0Ch] - PCONTEXT Context
//   [ESP+10h] - PVOID DispatcherContext
//   [ESP+14h] - PEXCEPTION_HANDLER ExceptionHandler
// Registers:
//   None
// Returns:
//   EXCEPTION_DISPOSITION
// Notes:
//   Front-end
_RtlpExecuteHandlerForException:
    movl     $_RtlpExceptionProtector, %edx
    jmp      _RtlpExecuteHandler


#define RUP_ERECORD     0x04
#define RUP_RFRAME      0x08
#define RUP_CONTEXT     0x0C
#define RUP_DCONTEXT    0x10

// Parameters:
//   [ESP+04h] - PEXCEPTION_RECORD ExceptionRecord
//   [ESP+08h] - PEXCEPTION_REGISTRATION RegistrationFrame
//   [ESP+0Ch] - PCONTEXT Context
//   [ESP+10h] - PVOID DispatcherContext
// Registers:
//   None
// Returns:
//   EXCEPTION_DISPOSITION
// Notes:
//    This exception handler protects the exception handling
//    mechanism by detecting collided unwinds.
_RtlpUnwindProtector:
    movl     $ExceptionContinueSearch, %eax
    movl     %ecx, RUP_ERECORD(%esp)
    testl    $EXCEPTION_UNWINDING, EREC_FLAGS(%ecx)
    jz       .rup_end

    // Unwinding is taking place, so return ExceptionCollidedUnwind

    movl     RUP_RFRAME(%esp), %ecx
    movl     RUP_DCONTEXT(%esp), %edx

    // Set DispatcherContext field to the exception registration for the
    // exception handler that executed when a collision occurred
    movl     RUP_RFRAME(%ecx), %eax
    movl     %eax, (%edx)
    movl     $ExceptionCollidedUnwind, %eax

.rup_end:
    ret


// Parameters:
//   [ESP+04h] - PEXCEPTION_RECORD ExceptionRecord
//   [ESP+08h] - PEXCEPTION_REGISTRATION RegistrationFrame
//   [ESP+0Ch] - PCONTEXT Context
//   [ESP+10h] - PVOID DispatcherContext
//   [ESP+14h] - PEXCEPTION_HANDLER ExceptionHandler
// Registers:
//   None
// Returns:
//   EXCEPTION_DISPOSITION
_RtlpExecuteHandlerForUnwind:
    movl     $_RtlpUnwindProtector, %edx
    jmp      _RtlpExecuteHandler
