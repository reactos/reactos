/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/sdk/crt/mem/i386/memchr.s
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
	jecxz	.Lnotfound
	repne	scasb
	je	.Lfound
.Lnotfound:
	mov	$1,%edi
.Lfound:
	mov	%edi,%eax
	dec	%eax
	pop	%edi
	leave
	ret
