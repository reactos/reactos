/* 
 * $Id: wcsrchr.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * wchar_t *wcsrchr (const wchar_t* str, wchar_t ch)
 */

.globl	_wcsrchr

_wcsrchr:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edx
	mov	0x8(%ebp),%esi
	mov	0xc(%ebp),%edx
	mov	$2,%ecx
	cld
.L1:	
	lodsw
	cmp	%ax,%dx
	jne	.L2
	mov	%esi,%ecx
.L2:	
	test	%ax,%ax
	jne	.L1
	mov	%ecx,%eax
	sub	$2,%eax
	pop	%edx
	pop	%esi
	leave
	ret
	

