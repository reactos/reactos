/* 
 * $Id: wcsncmp.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * int wcsncmp (const wchar_t* s1, const wchar_t* s2, size_t n)
 */

.globl	_wcsncmp

_wcsncmp:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edi
	mov	0x8(%ebp),%esi
	mov	0xc(%ebp),%edi
	mov	0xc(%ebp),%ecx
	cld
.L1:
	dec	%ecx
	js	.L2
	lodsw
	scasw
	jne	.L3
	test	%ax,%ax
	jne	.L1
.L2:	
	xor	%eax,%eax
	jmp	.L4
.L3:
	sbb	%eax,%eax
	or	$1,%al
.L4:		
	pop	%edi
	pop	%esi
	leave
	ret
	

