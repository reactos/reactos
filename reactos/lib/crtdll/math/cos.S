/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
L0:
	.quad	0xffffffffffffffff

	.globl	_cos
_cos:
	fldl	4(%esp)
	fcos
	fstsw
	sahf
	jnp	L1
	fstp	%st(0)
	fldl	L0
L1:
	ret

