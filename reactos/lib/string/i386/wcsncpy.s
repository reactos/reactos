/* 
 * $Id: wcsncpy.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * wchar_t* wcsncpy (wchar_t* to, const wchar_t* from)
 */

.globl	_wcsncpy

_wcsncpy:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edi
	mov	0x8(%ebp),%edi
	mov	0xc(%ebp),%esi
	mov	0x10(%ebp),%ecx
	cld
.L1:	
	dec	%ecx
	js	.L2
	lodsw
	stosw
	test	%ax,%ax
	jne	.L1
	rep	stosw
.L2:	
	mov	0x8(%ebp),%eax
	pop	%edi
	pop	%esi
	leave
	ret
	

