/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
	.file "memmove.s"
	.globl	_memmove
_memmove:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%esi
	pushl	%edi
	movl	8(%ebp),%edi
	movl	12(%ebp),%esi
	movl	16(%ebp),%ecx
	jecxz	L2
	cld
	cmpl	%esi,%edi
	jb	L3

	std
	addl	%ecx,%esi
	addl	%ecx,%edi
	decl	%esi
	decl	%edi
L3:
	rep
	movsb

L2:
	cld
	popl	%edi
	popl	%esi
	movl	8(%ebp),%eax
	leave
	ret

