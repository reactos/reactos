/* 
 * $Id: wcschr.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * wchar_t *wcschr (const wchar_t* str, wchar_t ch)
 */

.globl	_wcschr

_wcschr:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edx
	mov	0x8(%ebp),%esi
	mov	0xc(%ebp),%edx
	cld
.L1:	
	lodsw
	cmp	%ax,%dx
	je	.L2
	test	%ax,%ax
	jne	.L1
	mov	$2,%esi
.L2:
	mov	%esi,%eax
	sub	$2,%eax
	pop	%edx
	pop	%esi
	leave
	ret
	

