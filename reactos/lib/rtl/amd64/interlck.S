/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/amd64/interlck.S
 * PURPOSE:         Rtl Interlocked Functions for amd64
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <ndk/asm.h>
#include <ndk/amd64/asmmacro.S>
.intel_syntax noprefix


#define SLIST8A_DEPTH_MASK      0x000000000000FFFF
#define SLIST8A_DEPTH_INC       0x0000000000000001
#define SLIST8A_SEQUENCE_MASK   0x0000000001FF0000
#define SLIST8A_SEQUENCE_INC    0x0000000000010000
#define SLIST8A_NEXTENTRY_MASK  0xFFFFFFFFFE000000
#define SLIST8A_NEXTENTRY_SHIFT 21
#define SLIST8B_HEADERTYPE_MASK 0x0000000000000001
#define SLIST8B_INIT_MASK       0x0000000000000002
#define SLIST8B_REGION_MASK     0xE000000000000000

#define SLIST16A_DEPTH_MASK      0x000000000000FFFF
#define SLIST16A_DEPTH_INC       0x0000000000000001
#define SLIST16A_SEQUENCE_MASK   0xFFFFFFFFFFFF0000
#define SLIST16B_HEADERTYPE_MASK 0x0000000000000001
#define SLIST16B_INIT_MASK       0x0000000000000002
#define SLIST16B_NEXTENTY_MASK   0xFFFFFFFFFFFFFFF0


POINTER_MASK: .quad 0x000007FFFFFFFFF0

SLIST16_POINTER_MASK: .quad 0xFFFFFFFFFFFFFFF0


/* FUNCTIONS ****************************************************************/


/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedPopEntrySList(
 *     IN PSLIST_HEADER ListHead);
 */
.proc RtlInterlockedPopEntrySList

    /* Save registers */
    push rbx
    push rbp

    /* Load ListHead->Region into rax */
    mov rax, [rcx + 8]

    /* Check what kind of header this is */
    test rax, SLIST8B_HEADERTYPE_MASK
    jnz 3f

    /* We have an 8 byte header */

    /* Load ListHead->Alignment into rax */
    mov rax, [rcx]

    /* Check if ListHead->NextEntry is NULL */
    and rax, SLIST8A_NEXTENTRY_MASK
    jz 2f

    /* Copy rcx to rbp, as we need ecx for the exchange */
    mov rbp, rcx

    /* Copy the low 32 bits to eax */
    mov eax, [rbp]

    /* Copy the high 32 bits to edx */
    mov edx, [rbp + 4]

1:
    /* Use rbp as pointer template in rbx*/
    mov rbx, rbp
    and rbx, POINTER_MASK

    /* Extract the NextEntry pointer */
    mov rcx, [rbp]
    and rcx, SLIST8A_NEXTENTRY_MASK
    shr rcx, SLIST8A_NEXTENTRY_SHIFT

    /* Combine to new pointer in rcx */
    or rcx, rbx

    /* Load the next NextEntry pointer to rcx */
    mov rcx, [rcx]

    /* Copy Depth and Sequence number and adjust Depth */
    lea rbx, [rax - SLIST8A_DEPTH_INC]

    /* Shift bits in place */
    shl rcx, SLIST8A_NEXTENTRY_SHIFT

    /* Combine into rbx */
    and rbx, SLIST8A_SEQUENCE_INC | SLIST8A_DEPTH_MASK
    or rbx, rcx

    /* Copy higher 32 bits into ecx */
    mov rcx, rbx
    shr rcx, 32

    /* If [rbp] equals edx:eax, exchange it with ecx:ebx */
    lock cmpxchg8b [rbp]

    /* If not equal, retry with edx:eax, being the content of [rbp] now */
    jnz 1b

    /* Move result from edx:eax to rax */
    shl rdx, 32
    or rax, rdx

    /* Use rbp as pointer template */
    and rbp, POINTER_MASK

    and rax, SLIST8A_NEXTENTRY_MASK
    shr rax, SLIST8A_NEXTENTRY_SHIFT
    or rax, rbp

2:  
    /* Restore registers and return */
    pop rbp
    pop rbx
    ret

3:  /* This is a 16 byte header */

    /* Copy rcx to rbp, as we need rcx for the exchange */
    mov rbp, rcx


4:
    /* Check if ListHead->NextEntry is NULL */
    mov rcx, rdx
    and rcx, SLIST16_POINTER_MASK
    jz 5f

    /* Copy Depth and Sequence number and adjust Depth */
    lea rbx, [rax - SLIST16A_DEPTH_INC]

    /* Get next pointer */
    mov rcx, [rcx]

    /* Set ListHead->HeaderType = 1 */
    or rcx, 1

    /* If [rbp] equals rdx:rax, exchange it with rcx:rbx */
    lock cmpxchg16b [rbp]

    /* If not equal, retry with rdx:rax, being the content of [rbp] now */
    jnz 4b

5:


    /* Restore registers and return */
    pop rbp
    pop rbx
    ret
.endproc


/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedPushEntrySList(
 *     IN PSLIST_HEADER ListHead,
 *     IN PSLIST_ENTRY ListEntry);
 */
.proc RtlInterlockedPushEntrySList


    ret
.endproc


/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedFlushSList(
 *     IN PSINGLE_LIST_ENTRY ListHead);
 */
.proc RtlInterlockedFlushSList


    ret
.endproc
