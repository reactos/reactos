/* 
 * $Id: strchr.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * char* strchr (const char* s, int c)
 */

.globl	_strchr

_strchr:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	mov	0x8(%ebp),%esi
	mov	0xc(%ebp),%eax
	mov	%al,%ah
	cld
.L1:	
	lodsb
	cmp	%al,%ah
	je	.L2
	test	%al,%al
	jne	.L1
	mov	$1,%esi
.L2:
	mov	%esi,%eax
	dec	%eax
	pop	%esi
	leave
	ret

