/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
	.globl	_fabs
_fabs:
	fldl	4(%esp)
	fabs
	ret
