/*
 * Written by J.T. Conklin <jtc@netbsd.org>.
 * Public domain.
 *
 * Changes for long double by Ulrich Drepper <drepper@cygnus.com>
 * 
 * Removed header file dependency for use in libmingwex.a by
 *   Danny Smith <dannysmith@users.sourceforge.net>
 */
#include <_mingw_mac.h>

	.file	"floorf.S"
	.text
#ifdef _WIN64
	.align 8
#else
	.align 4
#endif
.globl __MINGW_USYMBOL(floorf)
	.def	__MINGW_USYMBOL(floorf);	.scl	2;	.type	32;	.endef
__MINGW_USYMBOL(floorf):
#ifdef _WIN64
	subq	$24,%rsp
	movss	%xmm0,8(%rsp)
	flds	8(%rsp)

	fstcw	4(%rsp)			/* store fpu control word */
	movl	$0x400,%edx		/* round towards -oo */
	orl	4(%rsp),%edx
	andl	$0xf7ff,%edx
	movl	%edx,(%rsp)
	fldcw	(%rsp)			/* load modified control word */

	frndint				/* round */

	fldcw	4(%rsp)			/* restore original control word */

	fstps	8(%rsp)
	movss	8(%rsp),%xmm0
	addq	$24,%rsp
	ret
#else
	flds	4(%esp)
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
