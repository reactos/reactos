/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
.data
	.align	2
nan:
	.long	0xffffffff
	.byte	0xff
	.byte	0xff
	.byte	0xff
	.byte	0x7f

.text
	.globl	_atan2
_atan2:
	fldl	4(%esp)
	fldl	12(%esp)
	ftst
	fnstsw	%ax
	sahf
	jne	doit
	fxch	%st(1)
	ftst
	fnstsw	%ax
	sahf
	je	isanan
	fxch	%st(1)
doit:
	fpatan
	ret
isanan:
	movl	$1,_errno
	fstp	%st(0)
	fstp	%st(0)
	fldl	nan
	ret

