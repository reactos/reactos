
#include <asm.inc>
#include <ks386.inc>

/*
 * void *memset (void *src, int val, size_t count)
 */

PUBLIC _memset
.code

FUNC _memset
	FPO 0, 3, 4, 1, 1, FRAME_NONFPO
	push ebp
	mov ebp, esp
	push edi
	mov edi, [ebp + 8]
	movzx eax, byte ptr [ebp + 12]
	mov ecx, [ebp + 16]
	cld
	cmp ecx, 16
	jb .L1
	mov edx, HEX(01010101)
	mul edx
	mov edx, ecx
	test edi, 3
	je .L2
	mov ecx, edi
	and ecx, 3
	sub ecx, 5
	not ecx
	sub edx, ecx
	rep stosb
	mov ecx, edx
.L2:
	shr ecx, 2
	rep stosd
	mov ecx, edx
	and ecx, 3
.L1:
	test ecx, ecx
	je .L3
	rep stosb
.L3:
	pop edi
	mov eax, [ebp + 8]
	leave
	ret
ENDFUNC

END
