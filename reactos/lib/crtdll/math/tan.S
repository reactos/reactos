/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
L0:
	.quad	0xffffffffffffffff

	.globl	_tan
_tan:
	fldl	4(%esp)
	fptan
	fstsw
	fstp	%st(0)
	sahf
	jnp	L1
/*	fstp	%st(0) - if exception, there is nothing on the stack */
	fldl	L0
L1:
	ret

