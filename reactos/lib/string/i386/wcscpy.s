/* 
 * $Id: wcscpy.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * wchar_t* wcscpy (wchar_t* to, const wchar_t* from)
 */

.globl	_wcscpy

_wcscpy:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edi
	mov	0x8(%ebp),%edi
	mov	0xc(%ebp),%esi
	cld
.L1:	
	lodsw
	stosw
	test	%ax,%ax
	jne	.L1
	mov	0x8(%ebp),%eax
	pop	%edi
	pop	%esi
	leave
	ret
	

