/**
 * This file has no copyright assigned and is placed in the Public Domain.
 * This file is part of the w64 mingw-runtime package.
 * No warranty is given; refer to the file DISCLAIMER.PD within this package.
 */
#include <_mingw_mac.h>

	.file	"ceilf.S"
	.text
	.align 4
.globl __MINGW_USYMBOL(ceilf)
	.def	__MINGW_USYMBOL(ceilf);	.scl	2;	.type	32;	.endef
__MINGW_USYMBOL(ceilf):
#ifdef _WIN64
	subq	$24,%rsp
	movss	%xmm0,8(%rsp)
	flds	8(%rsp)

	fstcw	4(%rsp)			/* store fpu control word */

	movl	$0x0800,%edx		/* round towards +oo */
	orl	4(%rsp),%edx
	andl	$0xfbff,%edx
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
	movl	$0x0800,%edx		/* round towards +oo */
	orl	4(%esp),%edx
	andl	$0xfbff,%edx
	movl	%edx,(%esp)
	fldcw	(%esp)			/* load modified control word */

	frndint				/* round */

	fldcw	4(%esp)			/* restore original control word */

	addl	$8,%esp
	ret
#endif
