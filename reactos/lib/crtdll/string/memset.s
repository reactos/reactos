/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
	.file "memset.s"
	.text
	.align	4
	.globl	_memset
_memset:
	pushl	%ebp
	movl	%esp,%ebp
	pushl	%edi
	movl	8(%ebp),%edi
	movl	12(%ebp),%eax
	movl	16(%ebp),%ecx
	cld

	# We will handle memsets of <= 15 bytes one byte at a time.
	# This avoids some extra overhead for small memsets, and
	# knowing we are setting > 15 bytes eliminates some annoying
	# checks in the "long move" case.
	cmpl	$15,%ecx
	jle	L3

	# Otherwise, tile the byte value out into %eax.
	# 0x41 -> 0x41414141, etc.
	movb	%al,%ah
	movl	%eax,%edx
	sall	$16,%eax
	movw	%dx,%ax
	jmp	L2

	# Handle any cruft necessary to get %edi long-aligned.
L1:	stosb
	decl	%ecx
L2:	testl	$3,%edi
	jnz	L1

	# Now slam out all of the longs.
	movl	%ecx,%edx
	shrl	$2,%ecx
	rep
	stosl

	# Finally, handle any trailing cruft.  We know the high three bytes
	# of %ecx must be zero, so just put the "slop count" in the low byte.
	movb	%dl,%cl
	andb	$3,%cl
L3:	rep
	stosb
	popl	%edi
	movl	8(%ebp),%eax
	leave
	ret
