/* 
 * $Id: strcpy.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * char *strcpy (char *to, const char *from)
 */

.globl	_strcpy

_strcpy:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edi
	mov	0x8(%ebp),%edi
	mov	0xc(%ebp),%esi
	cld
.L1:	
	lodsb
	stosb
	test	%al,%al
	jne	.L1
	mov	0x8(%ebp),%eax
	pop	%edi
	pop	%esi
	leave
	ret

