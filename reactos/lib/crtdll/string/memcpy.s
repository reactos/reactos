/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
	.file "memcpy.s"
	.text
	.align	4
	.globl	_memcpy
_memcpy:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	pushl	%edi
	movl	8(%ebp),%edi
	movl	12(%ebp),%esi
	movl	16(%ebp),%ecx
	call	___dj_movedata
	popl	%edi
	popl	%esi
	movl	8(%ebp),%eax
	leave
	ret

