/* 
 * $Id: strcmp.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * int *strcmp (const char* s1, const char* s2)
 */

.globl	_strcmp

_strcmp:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edi
	mov	0x8(%ebp),%esi
	mov	0xc(%ebp),%edi
	xor	%eax,%eax
	cld
.L1:
	lodsb
	scasb
	jne	.L2
	test	%eax,%eax		// bit 8..31 are set to 0
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

