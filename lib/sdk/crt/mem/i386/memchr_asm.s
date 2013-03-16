/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/sdk/crt/mem/i386/memchr.s
 */

#include <asm.inc>
#include <ks386.inc>

/*
 * void* memchr(const void* s, int c, size_t n)
 */

PUBLIC	_memchr
.code

_memchr:
	push ebp
	mov ebp, esp
	push edi
	mov	edi, [ebp + 8]
	mov	eax, [ebp + 12]
	mov	ecx, [ebp + 16]
	cld
	jecxz .Lnotfound
	repne scasb
	je .Lfound
.Lnotfound:
	mov edi, 1
.Lfound:
	mov eax, edi
	dec eax
	pop edi
	leave
	ret

END
