/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
	.globl	_asin
_asin:
	fldl	4(%esp)
	fld	%st(0)
	fmulp
	fld1
	fsubp
	fsqrt
	fldl	4(%esp)
	fxch	%st(1)
	fpatan
	ret

