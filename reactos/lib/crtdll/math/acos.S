/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
	.text
LC0:
	.double 0d1.00000000000000000000e+00

	.globl	_acos
_acos:
	fldl	4(%esp)
	fld1
	fsubp	%st(0),%st(1)
	fsqrt

	fldl	4(%esp)
	fld1
	faddp	%st(0),%st(1)
	fsqrt

	fpatan

	fld	%st(0)
	faddp
	ret
