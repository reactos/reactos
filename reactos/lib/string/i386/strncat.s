/* 
 * $Id: strncat.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * char *strncat (char *s, const char *append, size_t n)
 */

.globl	_strncat

_strncat:
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
	mov	0x10(%ebp),%ecx
.L1:	
	dec	%ecx
	js	.L2
	lodsb
	stosb
	test	%al,%al
	jne	.L1
	jmp	.L3
.L2:
	xor	%eax,%eax
	stosb	
.L3:		
	mov	0x8(%ebp),%eax
	pop	%edi
	pop	%esi
	leave
	ret

