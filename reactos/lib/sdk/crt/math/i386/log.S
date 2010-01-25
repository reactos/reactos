/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

	.file	"log.S"
	.text
#ifdef _WIN64
	.align 8
#else
	.align 4
#endif
one:	.double 1.0
	/* It is not important that this constant is precise.  It is only
	   a value which is known to be on the safe side for using the
	   fyl2xp1 instruction.  */
limit:	.double 0.29

.globl _log
	.def	_log;	.scl	2;	.type	32;	.endef
_log:
#ifdef _WIN64
	fldln2			// log(2)
	fldt	(%rdx)		// x : log(2)
	fld	%st		// x : x : log(2)
	fsubl	one		// x-1 : x : log(2)
	fld	%st		// x-1 : x-1 : x : log(2)
	fabs			// |x-1| : x-1 : x : log(2)
	fcompl	limit		// x-1 : x : log(2)
	fnstsw			// x-1 : x : log(2)
	andb	$0x45, %ah
	jz	2f
	fstp	%st(1)		// x-1 : log(2)
	fyl2xp1			// log(x)
	movq	%rcx,%rax
	movq	$0,8(%rcx)
	fstpt	(%rcx)
	ret

2:	fstp	%st(0)		// x : log(2)
	fyl2x			// log(x)
	movq	%rcx,%rax
	movq	$0,8(%rcx)
	fstpt	(%rcx)
	ret
#else
	fldln2			// log(2)
	fldt	4(%esp)		// x : log(2)
	fld	%st		// x : x : log(2)
	fsubl	one		// x-1 : x : log(2)
	fld	%st		// x-1 : x-1 : x : log(2)
	fabs			// |x-1| : x-1 : x : log(2)
	fcompl	limit		// x-1 : x : log(2)
	fnstsw			// x-1 : x : log(2)
	andb	$0x45, %ah
	jz	2f
	fstp	%st(1)		// x-1 : log(2)
	fyl2xp1			// log(x)
	ret

2:	fstp	%st(0)		// x : log(2)
	fyl2x			// log(x)
	ret
#endif
