/* 
 * $Id: wcscat.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * wchar_t *wcscat (wchar_t *dest, const wchar_t *append)
 */

.globl	_wcscat

_wcscat:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edi
	mov	0x8(%ebp),%edi
	mov	0xc(%ebp),%esi
	cld
	xor	%eax,%eax
	mov	$-1,%ecx
	repne	scasw
	sub	$2,%edi
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

