/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
	.data
LCW1:
	.word	0
	.align	4

	.text

	.globl	_fmod
_fmod:
	fldl	4(%esp)
	fldl	12(%esp)
	ftst
	fnstsw	%ax
	fxch	%st(1)
	sahf
	jnz	next
	fstpl	%st(0)
	jmp	out
next:
	fpreml
	fnstsw	%ax
	sahf
	jpe	next
	fstpl	%st(1)
out:
	ret


