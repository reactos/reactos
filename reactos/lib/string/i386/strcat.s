/* 
 * $Id: strcat.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * char *strcat (char *s, const char *append)
 */

.globl	_strcat

_strcat:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edi
	mov	0x8(%ebp),%edi
	mov	0xc(%ebp),%esi
	xor	%eax,%eax
	mov	$-1,%ecx	
	cld
	repne	scasb
	dec	%edi
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

