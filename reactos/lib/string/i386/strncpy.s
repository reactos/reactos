/* 
 * $Id: strncpy.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * char *strncpy (char *to, const char *from, size_t n)
 */

.globl	_strncpy

_strncpy:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edi
	mov	0x8(%ebp),%edi
	mov	0xc(%ebp),%esi
	mov	0x10(%ebp),%ecx
	xor	%eax,%eax
	cld
.L1:
	dec	%ecx
	js	.L2	
	lodsb
	stosb
	test	%al,%al
	jne	.L1
	rep	stosb
.L2:	
	mov	0x8(%ebp),%eax
	pop	%edi
	pop	%esi
	leave
	ret

