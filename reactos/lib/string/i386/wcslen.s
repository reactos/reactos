/* 
 * $Id: wcslen.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * size_t wcslen (const wchar_t* str)
 */

.globl	_wcslen

_wcslen:
	push	%ebp
	mov	%esp,%ebp
	push	%edi
	mov	0x8(%ebp),%edi
	xor	%eax,%eax
	mov	$-1,%ecx
	cld
	repne	scasw
	not	%ecx
	dec	%ecx
	mov	%ecx,%eax
	pop	%edi
	leave
	ret

	

