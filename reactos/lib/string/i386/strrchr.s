/* 
 * $Id: strrchr.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * char* strrchr (const char* s, int c)
 */

.globl	_strrchr

_strrchr:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	mov	0x8(%ebp),%esi
	mov	0xc(%ebp),%eax
	mov	$1,%ecx
	mov	%al,%ah
	cld
.L1:	
	lodsb
	cmp	%al,%ah
	jne	.L2
	mov	%esi,%ecx
.L2:
	test	%al,%al	
	jne	.L1
	mov	%ecx,%eax
	dec	%eax
	pop	%esi
	leave
	ret

