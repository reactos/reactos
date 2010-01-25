/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */

	.file	"floor.S"
	.text
#ifdef _WIN64
	.align 8
#else
	.align 4
#endif
.globl _floor
	.def	_floor;	.scl	2;	.type	32;	.endef
_floor:
#ifdef _WIN64
	fldt	(%rdx)
	subq	$24,%rsp

	fstcw	8(%rsp)			/* store fpu control word */

	/* We use here %edx although only the low 1 bits are defined.
	   But none of the operations should care and they are faster
	   than the 16 bit operations.  */
	movl	$0x400,%edx		/* round towards -oo */
	orl	8(%rsp),%edx
	andl	$0xf7ff,%edx
	movl	%edx,(%rsp)
	fldcw	(%rsp)			/* load modified control word */

	frndint				/* round */

	fldcw	8(%rsp)			/* restore original control word */

	addq	$24,%rsp
	movq	%rcx,%rax
	movq	$0,8(%rcx)
	fstpt	(%rcx)
	ret
#else
	fldt	4(%esp)
	subl	$8,%esp

	fstcw	4(%esp)			/* store fpu control word */

	/* We use here %edx although only the low 1 bits are defined.
	   But none of the operations should care and they are faster
	   than the 16 bit operations.  */
	movl	$0x400,%edx		/* round towards -oo */
	orl	4(%esp),%edx
	andl	$0xf7ff,%edx
	movl	%edx,(%esp)
	fldcw	(%esp)			/* load modified control word */

	frndint				/* round */

	fldcw	4(%esp)			/* restore original control word */

	addl	$8,%esp
	ret
#endif
