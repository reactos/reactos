/* 
 * $Id: memset.s,v 1.2 2003/06/01 17:10:42 hbirr Exp $
 */

/*
 * void *memset (void *src, int val, size_t count)
 */

.globl	_memset

_memset:
	push	%ebp
	mov	%esp,%ebp
	push	%edi
	mov	0x8(%ebp),%edi
	movzb	0xc(%ebp),%eax
	mov	0x10(%ebp),%ecx
	cld
	cmp	$16,%ecx
	jb	.L1
	mov	$0x01010101,%edx
	mul	%edx
	mov	%ecx,%edx
	test	$3,%edi
	je	.L2
	mov	%edi,%ecx
	and	$3,%ecx
	sub	$5,%ecx
	not	%ecx
	sub	%ecx,%edx
	rep	stosb
	mov	%edx,%ecx
.L2:
	shr	$2,%ecx
	rep	stosl
	mov	%edx,%ecx
	and	$3,%ecx
.L1:	
	test	%ecx,%ecx
	je	.L3
	rep	stosb
.L3:
	pop	%edi
	mov	0x8(%ebp),%eax
	leave
	ret

