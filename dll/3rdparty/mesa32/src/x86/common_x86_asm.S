/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Check extended CPU capabilities.  Now justs returns the raw CPUID
 * feature information, allowing the higher level code to interpret the
 * results.
 *
 * Written by Holger Waechtler <holger@akaflieg.extern.tu-berlin.de>
 *
 * Cleaned up and simplified by Gareth Hughes <gareth@valinux.com>
 *
 */

/*
 * NOTE: Avoid using spaces in between '(' ')' and arguments, especially
 * with macros like CONST, LLBL that expand to CONCAT(...).  Putting spaces
 * in there will break the build on some platforms.
 */

#include "matypes.h"
#include "common_x86_features.h"

	SEG_TEXT

ALIGNTEXT4
GLOBL GLNAME(_mesa_x86_has_cpuid)
HIDDEN(_mesa_x86_has_cpuid)
GLNAME(_mesa_x86_has_cpuid):

	/* Test for the CPUID command.  If the ID Flag bit in EFLAGS
	 * (bit 21) is writable, the CPUID command is present */
	PUSHF_L
	POP_L	(EAX)
	MOV_L	(EAX, ECX)
	XOR_L	(CONST(0x00200000), EAX)
	PUSH_L	(EAX)
	POPF_L
	PUSHF_L
	POP_L	(EAX)

	/* Verify the ID Flag bit has been written. */
	CMP_L	(ECX, EAX)
	SETNE	(AL)
	XOR_L	(CONST(0xff), EAX)

	RET


ALIGNTEXT4
GLOBL GLNAME(_mesa_x86_cpuid)
HIDDEN(_mesa_x86_cpuid)
GLNAME(_mesa_x86_cpuid):

	MOV_L	(REGOFF(4, ESP), EAX)		/* cpuid op */
	PUSH_L	(EDI)
	PUSH_L	(EBX)

	CPUID

	MOV_L	(REGOFF(16, ESP), EDI)	/* *eax */
	MOV_L	(EAX, REGIND(EDI))
	MOV_L	(REGOFF(20, ESP), EDI)	/* *ebx */
	MOV_L	(EBX, REGIND(EDI))
	MOV_L	(REGOFF(24, ESP), EDI)	/* *ecx */
	MOV_L	(ECX, REGIND(EDI))
	MOV_L	(REGOFF(28, ESP), EDI)	/* *edx */
	MOV_L	(EDX, REGIND(EDI))

	POP_L	(EBX)
	POP_L	(EDI)
	RET

ALIGNTEXT4
GLOBL GLNAME(_mesa_x86_cpuid_eax)
HIDDEN(_mesa_x86_cpuid_eax)
GLNAME(_mesa_x86_cpuid_eax):

	MOV_L	(REGOFF(4, ESP), EAX)		/* cpuid op */
	PUSH_L	(EBX)

	CPUID

	POP_L	(EBX)
	RET

ALIGNTEXT4
GLOBL GLNAME(_mesa_x86_cpuid_ebx)
HIDDEN(_mesa_x86_cpuid_ebx)
GLNAME(_mesa_x86_cpuid_ebx):

	MOV_L	(REGOFF(4, ESP), EAX)		/* cpuid op */
	PUSH_L	(EBX)

	CPUID
	MOV_L	(EBX, EAX)			/* return EBX */

	POP_L	(EBX)
	RET

ALIGNTEXT4
GLOBL GLNAME(_mesa_x86_cpuid_ecx)
HIDDEN(_mesa_x86_cpuid_ecx)
GLNAME(_mesa_x86_cpuid_ecx):

	MOV_L	(REGOFF(4, ESP), EAX)		/* cpuid op */
	PUSH_L	(EBX)

	CPUID
	MOV_L	(ECX, EAX)			/* return ECX */

	POP_L	(EBX)
	RET

ALIGNTEXT4
GLOBL GLNAME(_mesa_x86_cpuid_edx)
HIDDEN(_mesa_x86_cpuid_edx)
GLNAME(_mesa_x86_cpuid_edx):

	MOV_L	(REGOFF(4, ESP), EAX)		/* cpuid op */
	PUSH_L	(EBX)

	CPUID
	MOV_L	(EDX, EAX)			/* return EDX */

	POP_L	(EBX)
	RET

#ifdef USE_SSE_ASM
/* Execute an SSE instruction to see if the operating system correctly
 * supports SSE.  A signal handler for SIGILL should have been set
 * before calling this function, otherwise this could kill the client
 * application.
 *
 *        -----> !!!! ATTENTION DEVELOPERS !!!! <-----
 *
 * If you're debugging with gdb and you get stopped in this function,
 * just type 'continue'!  Execution will proceed normally.
 * See freedesktop.org bug #1709 for more info.
 */
ALIGNTEXT4
GLOBL GLNAME( _mesa_test_os_sse_support )
HIDDEN(_mesa_test_os_sse_support)
GLNAME( _mesa_test_os_sse_support ):

	XORPS	( XMM0, XMM0 )

	RET


/* Perform an SSE divide-by-zero to see if the operating system
 * correctly supports unmasked SIMD FPU exceptions.  Signal handlers for
 * SIGILL and SIGFPE should have been set before calling this function,
 * otherwise this could kill the client application.
 */
ALIGNTEXT4
GLOBL GLNAME( _mesa_test_os_sse_exception_support )
HIDDEN(_mesa_test_os_sse_exception_support)
GLNAME( _mesa_test_os_sse_exception_support ):

	PUSH_L	( EBP )
	MOV_L	( ESP, EBP )
	SUB_L	( CONST( 8 ), ESP )

	/* Save the original MXCSR register value.
	 */
	STMXCSR	( REGOFF( -4, EBP ) )

	/* Unmask the divide-by-zero exception and perform one.
	 */
	STMXCSR	( REGOFF( -8, EBP ) )
	AND_L	( CONST( 0xfffffdff ), REGOFF( -8, EBP ) )
	LDMXCSR	( REGOFF( -8, EBP ) )

	XORPS	( XMM0, XMM0 )

	PUSH_L	( CONST( 0x3f800000 ) )
	PUSH_L	( CONST( 0x3f800000 ) )
	PUSH_L	( CONST( 0x3f800000 ) )
	PUSH_L	( CONST( 0x3f800000 ) )

	MOVUPS	( REGIND( ESP ), XMM1 )

	DIVPS	( XMM0, XMM1 )

	/* Restore the original MXCSR register value.
	 */
	LDMXCSR	( REGOFF( -4, EBP ) )

	LEAVE
	RET

#endif

	
#if defined (__ELF__) && defined (__linux__)
	.section .note.GNU-stack,"",%progbits
#endif
