/* 
 * $Id: memset.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * void *memset (void *src, int val, size_t count)
 */

.globl	_memset

_memset:
;	push	%ebp
;	mov	%esp,%ebp
	push	%edi
	push	%edx
;	mov	0x8(%ebp),%edi
;	movzb	0xc(%ebp),%eax
;	mov	0x10(%ebp),%ecx
	mov	0x10(%esp),%edi
	movzb	0x14(%esp),%eax
	mov	0x18(%esp),%ecx
	cld
	cmp	$4,%ecx
	jb	.L1
	mov	$0x01010101,%edx
	mul	%edx
	test	$3,%edi
	je	.L2
	mov	%ecx,%edx
	mov	%edi,%ecx
	and	$3,%ecx
	sub	%ecx,%edx
	rep	stosb
	mov	%edx,%ecx
.L2:
	cmp	$4,%ecx
	jb	.L1
	mov	%ecx,%edx
	shr	$2,%ecx
	rep	stosl
	mov	%edx,%ecx
	and	$3,%ecx
.L1:	
	test	%ecx,%ecx
	je	.L3
	rep	stosb
.L3:
	pop	%edx
	pop	%edi
;	mov	0x8(%ebp),%eax
	mov	0x8(%esp),%eax	
;	leave
	ret

