/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
	.globl	_sqrt
_sqrt:
	fldl	4(%esp)
	fsqrt
	ret
