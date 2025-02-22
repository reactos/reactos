/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/sdk/crt/mem/i386/memchr_asm.s
 */

#include <asm.inc>
#include <ks386.inc>

/*
 * void* memchr(const void* s, int c, size_t n)
 */

PUBLIC	_memchr
.code

FUNC _memchr
	FPO 0, 3, 4, 1, 1, FRAME_NONFPO
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
ENDFUNC

END
