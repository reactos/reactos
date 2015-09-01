/*
 * COPYRIGHT:         GNU GPL - See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Memory functions
 * FILE:              lib/rtl/i386/rtlswap.S
 * PROGRAMER:         Alex Ionescu (alex.ionescu@reactos.org)
 */

#include <asm.inc>

/* GLOBALS *******************************************************************/

PUBLIC _RtlCompareMemory@12
PUBLIC _RtlCompareMemoryUlong@12
PUBLIC _RtlFillMemory@12
PUBLIC _RtlFillMemoryUlong@12
PUBLIC _RtlMoveMemory@12
PUBLIC _RtlZeroMemory@8

/* FUNCTIONS *****************************************************************/
.code

_RtlCompareMemory@12:

    /* Save volatiles */
    push esi
    push edi

    /* Clear direction flag and load pointers and size in ULONGs */
    cld
    mov esi, [esp+12]
    mov edi, [esp+16]
    mov ecx, [esp+20]
    shr ecx, 2
    jz NoUlongs

    /* Compare the ULONGs */
    repe cmpsd
    jnz NotEqual

NoUlongs:

    /* Compare what's left */
    mov ecx, [esp+20]
    and ecx, 3
    jz NoneLeft
    repe cmpsb
    jnz NotEqual2

NoneLeft:

    /* We're done, return full count */
    mov eax, [esp+20]
    pop edi
    pop esi
    ret 12

NotEqual:
    /* Compare the last ULONG */
    sub esi, 4
    sub edi, 4
    mov ecx, 5
    repe cmpsb

NotEqual2:

    /* Remember how many matched */
    dec esi
    sub esi, [esp+12]

    /* Return count */
    mov eax, esi
    pop edi
    pop esi
    ret 12


_RtlCompareMemoryUlong@12:

    /* Get pointers and size in ULONGs */
    push edi
    mov edi, [esp+8]
    mov ecx, [esp+12]
    mov eax, [esp+16]
    shr ecx, 2

    /* Do the compare and check result */
    repe scasd
    jz Done
    sub edi, 4

    /* Return count */
Done:
    sub edi, [esp+8]
    mov eax, edi
    pop edi
    ret 12


_RtlFillMemory@12:

    /* Get pointers and size  */
    push edi
    mov edi, [esp+8]
    mov ecx, [esp+12]

    /* Get pattern */
    mov al, [esp+16]
    mov ah, al
    shl eax, 16
    mov al, [esp+16]
    mov ah, al

    /* Clear direction flag and set ULONG size and UCHAR remainder */
    cld
    mov edx, ecx
    and edx, 3
    shr ecx, 2

    /* Do the fill */
    rep stosd
    or ecx, edx
    jnz ByteFill

    /* Return */
    pop edi
    ret 12

ByteFill:
    /* Fill what's left */
    rep stosb
    pop edi
    ret 12


_RtlFillMemoryUlong@12:

    /* Get pointer, size and pattern */
    push edi
    mov edi, [esp+8]
    mov ecx, [esp+12]
    mov eax, [esp+16]
    shr ecx, 2

    /* Do the fill and return */
    rep stosd
    pop edi
    ret 12


_RtlFillMemoryUlonglong@16:

    /* Save volatiles */
    push edi
    push esi

    /* Get pointer, size and pattern */
    mov ecx, [esp+16]
    mov esi, [esp+12]
    mov eax, [esp+20]
    shr ecx, 2
    sub ecx, 2

    /* Save the first part */
    mov [esi], eax

    /* Read second part */
    mov eax, [esp+24]
    lea edi, [esi+8]
    mov [esi+4], eax

    /* Do the fill and return */
    rep movsd
    pop esi
    pop edi
    ret 16


_RtlZeroMemory@8:

    /* Get pointers and size  */
    push edi
    mov edi, [esp+8]
    mov ecx, [esp+12]

    /* Get pattern */
    xor eax, eax

    /* Clear direction flag and set ULONG size and UCHAR remainder */
    cld
    mov edx, ecx
    and edx, 3
    shr ecx, 2

    /* Do the fill */
    rep stosd
    or ecx, edx
    jnz ByteZero

    /* Return */
    pop edi
    ret 8

ByteZero:
    /* Fill what's left */
    rep stosb
    pop edi
    ret 8


_RtlMoveMemory@12:
	push ebp
	mov ebp, esp

    /* Save non-volatiles */
    push esi
    push edi

    /* Get pointers and size  */
	mov	edi, [ebp + 8]
	mov	esi, [ebp + 12]
	mov	ecx, [ebp + 16]

    /* Use downward copy if source < dest and overlapping */
	cmp	edi, esi
	jbe	.CopyUp
	mov	eax, ecx
	add	eax, esi
	cmp	edi, eax
	jb .CopyDown

.CopyUp:
	cld

    /* Check for small moves */
	cmp	ecx, 16
	jb .CopyUpBytes

    /* Check if its already aligned */
	mov edx, ecx
	test edi, 3
	je .CopyUpDwords

    /* Make the destination dword aligned */
	mov ecx, edi
	and ecx, 3
	sub ecx, 5
	not ecx
	sub edx, ecx
	rep movsb
	mov ecx, edx

.CopyUpDwords:
	shr ecx, 2
	rep movsd
	mov ecx, edx
	and ecx, 3

.CopyUpBytes:
	test ecx, ecx
	je .CopyUpEnd
	rep movsb

.CopyUpEnd:
	mov eax, [ebp + 8]
	pop edi
	pop esi
	pop ebp
	ret 12

.CopyDown:
	std

    /* Go to the end of the region */
	add edi, ecx
	add esi, ecx

    /* Check for small moves */
	cmp ecx, 16
	jb .CopyDownSmall

    /* Check if its already aligned */
	mov edx, ecx
	test edi, 3
	je .CopyDownAligned

    /* Make the destination dword aligned */
	mov ecx, edi
	and ecx, 3
	sub edx, ecx
	dec esi
	dec edi
	rep movsb
	mov ecx, edx
	sub esi, 3
	sub edi, 3

.CopyDownDwords:
	shr ecx, 2
	rep movsd
	mov ecx, edx
	and ecx, 3
	je .CopyDownEnd
	add esi, 3
	add edi, 3

.CopyDownBytes:
	rep movsb

.CopyDownEnd:
	cld
	mov eax, [ebp + 8]
	pop edi
	pop esi
	pop ebp
	ret 12

.CopyDownAligned:
	sub edi, 4
	sub esi, 4
	jmp .CopyDownDwords

.CopyDownSmall:
	test ecx, ecx
	je .CopyDownEnd
	dec esi
	dec edi
	jmp .CopyDownBytes

END
