/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
 *
 *  Moved to MSVC-compatible inline assembler by Mike Nordell, 2003-12-26
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
/*
 * FILE:            ntoskrnl/ke/i386/vm86_sup.S
 * PURPOSE:         V86 mode support
 * PROGRAMMER:      David Welch (welch@cwcom.net)
 * UPDATE HISTORY:
 *                  Created 09/10/00
 */

/* INCLUDES ******************************************************************/

#pragma hdrstop

#include <ddk/ntddk.h>
#include <ddk/status.h>
#include <internal/i386/segment.h>
#include <internal/i386/fpu.h>
#include <internal/ps.h>
#include <ddk/defines.h>
#include <internal/v86m.h>
#include <ntos/tss.h>
//#include <ntos/service.h>
#include <internal/trap.h>
#include <internal/ps.h>

#include <roscfg.h>
#include <internal/ntoskrnl.h>
#include <internal/i386/segment.h>


#define EXCEPTION_UNWINDING		0x02

#define EREC_FLAGS				0x04

#define ExceptionContinueExecution 0
#define ExceptionContinueSearch    1
#define ExceptionNestedException   2
#define ExceptionCollidedUnwind    3

//.globl _RtlpExecuteHandlerForException
//.globl _RtlpExecuteHandlerForUnwind

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


VOID STDCALL
AsmDebug(ULONG Value);


// EAX = value to print
__declspec(naked)
void do_debug()
{
	__asm
	{
		pusha
		push	eax
		call	AsmDebug
		popa
		ret
	}
}


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
__declspec(naked)
void RtlpExecuteHandler()
{
	__asm
	{
		push	ebp
		mov		ebp, esp
		push	REH_RFRAME[ebp]

		push	edx
		push	fs:0x0
		mov		fs:0x0, esp

		// Prepare to call the exception handler
		push	REH_DCONTEXT[ebp]
		push	REH_CONTEXT[ebp]
		push	REH_RFRAME[ebp]
		push	REH_ERECORD[ebp]

		// Now call the exception handler
		mov		eax, REH_EROUTINE[ebp]
		call	eax

		cmp		fs:0x0, -1
		jne		reh_stack_looks_ok

		// This should not happen
		push	0
		push	0
		push	0
		push	0
		call	RtlAssert

reh_loop:
		jmp	reh_loop
	
reh_stack_looks_ok:
		mov		esp, fs:0x0

		// Return to the 'front-end' for this function
		pop		dword ptr fs:0x0
		mov		esp, ebp
		pop		ebp
		ret
	}
}

#if 0

#endif	// 0

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
__declspec(naked)
void RtlpExceptionProtector()
{
	__asm
	{
		mov		eax, ExceptionContinueSearch
		mov		ecx, REP_ERECORD[esp]
		test	EREC_FLAGS[ecx], EXCEPTION_UNWINDING
		jnz		rep_end

		// Unwinding is not taking place, so return ExceptionNestedException

		// Set DispatcherContext field to the exception registration for the
		// exception handler that executed when a nested exception occurred
		mov		ecx, REP_DCONTEXT[esp]
		mov		eax, REP_RFRAME[esp]
		mov		[ecx], eax
		mov		eax, ExceptionNestedException

rep_end:
		ret
	}
}


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
__declspec(naked)
void RtlpExecuteHandlerForException()
{
	__asm
	{
		mov		edx, RtlpExceptionProtector
		jmp		RtlpExecuteHandler
	}
}


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
__declspec(naked)
void RtlpUnwindProtector()
{
	__asm
	{
		mov		eax, ExceptionContinueSearch
		mov		RUP_ERECORD[esp], ecx
		test	EREC_FLAGS[ecx], EXCEPTION_UNWINDING
		jz		rup_end

		// Unwinding is taking place, so return ExceptionCollidedUnwind

		mov		ecx, RUP_RFRAME[esp]
		mov		edx, RUP_DCONTEXT[esp]

		// Set DispatcherContext field to the exception registration for the
		// exception handler that executed when a collision occurred
		mov		eax, RUP_RFRAME[ecx]
		mov		[edx], eax
		mov		eax, ExceptionCollidedUnwind

rup_end:
		ret
	}
}


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
__declspec(naked)
void RtlpExecuteHandlerForUnwind()
{
	__asm mov	edx, RtlpUnwindProtector
	__asm jmp	RtlpExecuteHandler
}

