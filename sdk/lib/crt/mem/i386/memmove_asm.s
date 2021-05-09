/*
 * void *memcpy (void *to, const void *from, size_t count)
 *
 */

#include <asm.inc>
#include <ks386.inc>

PUBLIC _memcpy
PUBLIC _memmove
.code

_memcpy:
FUNC _memmove
	FPO 0, 3, 5, 2, 1, FRAME_NONFPO
	push ebp
	CFI_ADJUST_CFA_OFFSET 4
	CFI_REL_OFFSET ebp, 0
	mov ebp, esp
	CFI_DEF_CFA_REGISTER ebp

	push esi
	CFI_REL_OFFSET esi, -4
	push edi
	CFI_REL_OFFSET edi, -8

	mov	edi, [ebp + 8]
	mov	esi, [ebp + 12]
	mov	ecx, [ebp + 16]

	cmp	edi, esi
	jbe	.CopyUp
	mov	eax, ecx
	add	eax, esi
	cmp	edi, eax
	jb .CopyDown

.CopyUp:
	cld

	cmp	ecx, 16
	jb .L1
	mov edx, ecx
	test edi, 3
	je .L2
/*
 * Make the destination dword aligned
 */
	mov ecx, edi
	and ecx, 3
	sub ecx, 5
	not ecx
	sub edx, ecx
	rep movsb
	mov ecx, edx
.L2:
	shr ecx, 2
	rep movsd
	mov ecx, edx
	and ecx, 3
.L1:
	test ecx, ecx
	je .L3
	rep movsb
.L3:
	mov eax, [ebp + 8]
	pop edi
	CFI_SAME_VALUE edi
	pop esi
	CFI_SAME_VALUE esi
	leave
	CFI_ADJUST_CFA_OFFSET -4
	CFI_SAME_VALUE ebp
	CFI_DEF_CFA_REGISTER esp
	ret

.CopyDown:
	std

	add edi, ecx
	add esi, ecx

	cmp ecx, 16
	jb .L4
	mov edx, ecx
	test edi, 3
	je .L5

/*
 * Make the destination dword aligned
 */
	mov ecx, edi
	and ecx, 3
	sub edx, ecx
	dec esi
	dec edi
	rep movsb
	mov ecx, edx

	sub esi, 3
	sub edi, 3
.L6:
	shr ecx, 2
	rep movsd
	mov ecx, edx
	and ecx, 3
	je .L7
	add esi, 3
	add edi, 3
.L8:
	rep movsb
.L7:
	cld
	mov eax, [ebp + 8]
	pop edi
	CFI_SAME_VALUE edi
	pop esi
	CFI_SAME_VALUE esi
	leave
	CFI_ADJUST_CFA_OFFSET -4
	CFI_SAME_VALUE ebp
	CFI_DEF_CFA_REGISTER esp
	ret
.L5:
	sub edi, 4
	sub esi, 4
	jmp .L6

.L4:
	test ecx, ecx
	je .L7
	dec esi
	dec edi
	jmp .L8
ENDFUNC

END
