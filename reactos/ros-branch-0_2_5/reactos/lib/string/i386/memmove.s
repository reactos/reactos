/* 
 * $Id: memmove.s,v 1.1 2003/06/04 18:14:46 hbirr Exp $
 */

/*
 * void *memmove (void *to, const void *from, size_t count)
 */

.globl	_memmove

_memmove:
	push	%ebp
	mov	%esp,%ebp
	
	push	%esi
	push	%edi
	
	mov	8(%ebp),%edi
	mov	12(%ebp),%esi
	mov	16(%ebp),%ecx
	
	cmp	%esi,%edi
	jbe	.CopyUp
	mov	%ecx,%eax
	add	%esi,%eax
	cmp	%eax,%edi
	jb	.CopyDown
	
.CopyUp:	
	cld
	
	cmp	$16,%ecx
	jb	.L1
	mov	%ecx,%edx
	test	$3,%edi
	je	.L2
/*
 * Make the destination dword aligned
 */
        mov	%edi,%ecx
        and	$3,%ecx
        sub	$5,%ecx
        not	%ecx
        sub	%ecx,%edx
        rep	movsb
        mov	%edx,%ecx	
.L2:
	shr	$2,%ecx
	rep	movsl
	mov	%edx,%ecx
	and	$3,%ecx
.L1:	
	test	%ecx,%ecx
	je	.L3
	rep	movsb
.L3:
	mov	16(%ebp),%eax
	pop	%edi
	pop	%esi
	leave
	ret

.CopyDown:
        std
        
	add	%ecx,%edi
	add	%ecx,%esi
	
	cmp	$16,%ecx
	jb	.L4
        mov	%ecx,%edx
	test	$3,%edi
	je	.L5
	
/*
 * Make the destination dword aligned
 */
	mov	%edi,%ecx
	and	$3,%ecx
	sub	%ecx,%edx
	dec	%esi
	dec	%edi
	rep	movsb
	mov	%edx,%ecx
	
	sub	$3,%esi
	sub	$3,%edi
.L6:	
	shr	$2,%ecx
	rep	movsl
	mov	%edx,%ecx
	and	$3,%ecx
	je	.L7
	add	$3,%esi
	add	$3,%edi
.L8:	
	rep	movsb
.L7:
	cld
	mov	8(%ebp),%eax
	pop	%edi
	pop	%esi
	leave
	ret
.L5:
	sub	$4,%edi
	sub	$4,%esi
	jmp	.L6
		
.L4:
	test	%ecx,%ecx
	je	.L7	
	dec	%esi
	dec	%edi
	jmp	.L8

