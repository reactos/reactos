/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
	.data
LCW1:
	.word	0
LCW2:
	.word	0
LC0:
	.double	0d1.0e+00

	.text

	.globl	_exp
_exp:
	fldl	4(%esp)
	fldl2e
	fmulp
	fstcww	LCW1
	fstcww	LCW2
	fwait
	andw	$0xf3ff,LCW2
	orw	$0x0400,LCW2
	fldcww	LCW2
	fldl	%st(0)
	frndint
	fldcww	LCW1
	fxch	%st(1)
	fsub	%st(1),%st
	f2xm1
	faddl	LC0
	fscale
	fstp	%st(1)
	ret
