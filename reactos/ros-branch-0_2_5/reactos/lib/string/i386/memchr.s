/* $Id: memchr.s,v 1.1 2003/05/27 18:58:15 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/string/i386/memchr.s
 */

/*
 * void* memchr(const void* s, int c, size_t n)
 */

.globl	_memchr

_memchr:
	push	%ebp
	mov	%esp,%ebp
	push	%edi
	mov	0x8(%ebp),%edi
	mov	0xc(%ebp),%eax
	mov	0x10(%ebp),%ecx
	cld
	repne	scasb
	je	.L1
	mov	$1,%edi
.L1:
	mov	%edi,%eax
	dec	%eax
	pop	%edi
	leave
	ret

