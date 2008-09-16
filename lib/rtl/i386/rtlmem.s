/*
 * COPYRIGHT:         See COPYING in the top level directory
 * PROJECT:           ReactOS Run-Time Library
 * PURPOSE:           Memory functions
 * FILE:              lib/rtl/i386/rtlswap.S
 * PROGRAMER:         Alex Ionescu (alex.ionescu@reactos.org)
 */

.intel_syntax noprefix

/* GLOBALS *******************************************************************/

.globl _RtlCompareMemory@12
.globl _RtlCompareMemoryUlong@12
.globl _RtlFillMemory@12
.globl _RtlFillMemoryUlong@12
.globl _RtlMoveMemory@12
.globl _RtlZeroMemory@8
.globl @RtlPrefetchMemoryNonTemporal@8

/* FUNCTIONS *****************************************************************/

.func RtlCompareMemory@12
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

    /* Remember how many mathced */
    dec esi
    sub esi, [esp+12]

    /* Return count */
    mov eax, esi
    pop edi
    pop esi
    ret 12
.endfunc

.func RtlCompareMemoryUlong@12
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
.endfunc

.func RtlFillMemory@12
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
.endfunc

.func RtlFillMemoryUlong@12
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
.endfunc

.func RtlFillMemoryUlonglong@16
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
.endfunc

.func RtlZeroMemory@8
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
.endfunc

.func RtlMoveMemory@12
_RtlMoveMemory@12:

    /* Save volatiles */
    push esi
    push edi

    /* Get pointers and size  */
    mov esi, [esp+16]
    mov edi, [esp+12]
    mov ecx, [esp+20]
    cld

    /* Check if the destination is higher (or equal) */
    cmp esi, edi
    jbe Overlap

    /* Set ULONG size and UCHAR remainder */
DoMove:
    mov edx, ecx
    and edx, 3
    shr ecx, 2

    /* Do the move */
    rep movsd
    or ecx, edx
    jnz ByteMove

    /* Return */
    pop edi
    pop esi
    ret 12

ByteMove:
    /* Move what's left */
    rep movsb

DoneMove:
    /* Restore volatiles */
    pop edi
    pop esi
    ret 12

Overlap:
    /* Don't copy if they're equal */
    jz DoneMove

    /* Compare pointer distance with given length and check for overlap */
    mov eax, edi
    sub eax, esi
    cmp ecx, eax
    jbe DoMove

    /* Set direction flag for backward move */
    std

    /* Copy byte-by-byte the non-overlapping distance */
    add esi, ecx
    add edi, ecx
    dec esi
    dec edi

    /* Do the move, reset flag and return */
    rep movsb
    cld
    jmp DoneMove
.endfunc

.func @RtlPrefetchMemoryNonTemporal@8, @RtlPrefetchMemoryNonTemporal@8
@RtlPrefetchMemoryNonTemporal@8:

    /*
     * Overwritten by ntoskrnl/ke/i386/kernel.c if SSE is supported
     * (see Ki386SetProcessorFeatures())
     */
    ret

    /* Get granularity */
    mov eax, [_Ke386CacheAlignment]

FetchLine:

    /* Prefetch this line */
    prefetchnta byte ptr [ecx]

    /* Update address and count */
    add ecx, eax
    sub edx, eax

    /* Keep looping for the next line, or return if done */
    ja FetchLine
    ret
.endfunc

/* FIXME: HACK */
_Ke386CacheAlignment:
    .long   0x40
