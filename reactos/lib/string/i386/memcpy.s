/* 
 * $Id: memcpy.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 */

/*
 * void *memcpy (void *to, const void *from, size_t count)
 */

.globl	_memcpy

_memcpy:
	push	%ebp
	mov	%esp,%ebp
	push	%esi
	push	%edi
	mov	0x8(%ebp),%edi
	mov	0xc(%ebp),%esi
	mov	0x10(%ebp),%ecx
	cld
	cmp	$4,%ecx
	jb	.L1
	test	$3,%edi
	je	.L2
/*
 * Should we make the source or the destination dword aligned?
 */
	mov	%ecx,%edx
	mov	%edi,%ecx
	and	$3,%ecx
	sub	%ecx,%edx
	rep	movsb
	mov	%edx,%ecx
.L2:
	cmp	$4,%ecx
	jb	.L1
	mov	%ecx,%edx
	shr	$2,%ecx
	rep	movsl
	mov	%edx,%ecx
	and	$3,%ecx
.L1:	
	test	%ecx,%ecx
	je	.L3
	rep	movsb
.L3:
	pop	%edi
	pop	%esi
	mov	0x8(%ebp),%eax
	leave
	ret

