/* 
 * $Id: wcsncat.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * wchar_t *wcsncat (wchar_t *dest, const wchar_t *append, size_t n)
 */

.globl	_wcsncat

_wcsncat:
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
	mov	0x10(%ebp),%ecx
.L1:
	dec	%ecx
	js	.L2	
	lodsw
	stosw
	test	%ax,%ax
	jne	.L1
	jmp	.L3
.L2:
	xor	%eax,%eax
	stosw
.L3:			
	mov	0x8(%ebp),%eax
	pop	%edi
	pop	%esi
	leave
	ret

