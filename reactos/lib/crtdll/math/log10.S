/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
	.globl	_log10
_log10:
	fldlg2
	fldl	4(%esp)
	fyl2x
	ret
