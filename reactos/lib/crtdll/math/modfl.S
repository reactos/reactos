/* Copyright (C) 1994 DJ Delorie, see COPYING.DJ for details */
	.text
	.globl ___modfl
___modfl:
	pushl %ebp
	movl %esp,%ebp
	subl $4,%esp
	fldt 8(%ebp)
	movl 20(%ebp),%eax
	fnstcw -2(%ebp)
	movw -2(%ebp),%dx
	orb $0x0c,%dh
	movw %dx,-4(%ebp)
	fldcw -4(%ebp)
	fld %st(0)
	frndint
	fldcw -2(%ebp)
	fld %st(0)
	fstpt (%eax)
	fsubrp %st,%st(1)
	leave
	ret
