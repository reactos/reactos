/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
	.text
	.globl	_modf
_modf:
	pushl	%ebp
	movl	%esp,%ebp
	subl	$16,%esp
	pushl	%ebx
	fnstcw	-4(%ebp)
	fwait
	movw	-4(%ebp),%ax
	orw	$0x0c3f,%ax
	movw	%ax,-8(%ebp)
	fldcw	-8(%ebp)
	fwait
	fldl	8(%ebp)
	frndint
	fstpl	-16(%ebp)
	fwait
	movl	-16(%ebp),%edx
	movl	-12(%ebp),%ecx
	movl	16(%ebp),%ebx
	movl	%edx,(%ebx)
	movl	%ecx,4(%ebx)
	fldl	8(%ebp)
	fsubl	-16(%ebp)
	leal	-20(%ebp),%esp
	fclex
	fldcw	-4(%ebp)
	fwait
	popl	%ebx
	leave
	ret
