/* 
 * $Id: wcscmp.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * int wcscmp (const wchar_t* s1, const wchar_t* s2)
 */

.globl	_wcscmp

_wcscmp:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edi
	mov	0x8(%ebp),%esi
	mov	0xc(%ebp),%edi
	cld
.L1:	
	lodsw
	scasw
	jne	.L2
	test	%ax,%ax
	jne	.L1
	xor	%eax,%eax
	jmp	.L3
.L2:
	sbb	%eax,%eax
	or	$1,%al
.L3:		
	pop	%edi
	pop	%esi
	leave
	ret
	

