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

#define SLIST8A_DEPTH_MASK       0x000000000000FFFF
#define SLIST8A_DEPTH_INC        0x0000000000000001
#define SLIST8A_SEQUENCE_MASK    0x0000000001FF0000
#define SLIST8A_SEQUENCE_INC     0x0000000000010000
#define SLIST8A_NEXTENTRY_MASK   0xFFFFFFFFFE000000
#define SLIST8A_NEXTENTRY_SHIFT  21
#define SLIST8B_HEADERTYPE_MASK  0x0000000000000001
#define SLIST8B_INIT_MASK        0x0000000000000002
#define SLIST8B_REGION_MASK      0xE000000000000000
#define SLIST8_POINTER_MASK      0x000007FFFFFFFFF0

#define SLIST16A_DEPTH_MASK      0x000000000000FFFF
#define SLIST16A_DEPTH_INC       0x0000000000000001
#define SLIST16A_SEQUENCE_MASK   0xFFFFFFFFFFFF0000
#define SLIST16A_SEQUENCE_INC    0x0000000000010000
#define SLIST16B_HEADERTYPE_MASK 0x0000000000000001
#define SLIST16B_INIT_MASK       0x0000000000000002
#define SLIST16B_NEXTENTY_MASK   0xFFFFFFFFFFFFFFF0


/* FUNCTIONS ****************************************************************/

.global _ExpInterlockedPopEntrySList
.global _ExpInterlockedPopEntrySListResume
.global _ExpInterlockedPopEntrySListFault
.global _ExpInterlockedPopEntrySListEnd
.global _ExpInterlockedPopEntrySListResume16
.global _ExpInterlockedPopEntrySListFault16
.global _ExpInterlockedPopEntrySListEnd16
.global _ExpInterlockedPushEntrySList
.global _ExpInterlockedFlushSList

/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedPopEntrySList(
 *     IN PSLIST_HEADER ListHead);
 */
.proc RtlInterlockedPopEntrySList
_ExpInterlockedPopEntrySList:

    /* Load ListHead->Region into rdx */
    mov rdx, [rcx + 8]

    /* Load ListHead->Alignment into rax */
    mov rax, [rcx]

    /* Check what kind of header this is */
    test rdx, SLIST8B_HEADERTYPE_MASK
    jnz _RtlInterlockedPopEntrySList16

    /* We have an 8 byte header */

_ExpInterlockedPopEntrySListResume:

    /* Check if ListHead->NextEntry is NULL */
    mov r9, rax
    and r9, SLIST8A_NEXTENTRY_MASK
    jz _RtlInterlockedPopEntrySListEmpty

    /* Copy Depth and Sequence number and adjust Depth */
    lea r8, [rax - SLIST8A_DEPTH_INC]
    and r8, SLIST8A_SEQUENCE_MASK | SLIST8A_DEPTH_MASK

    /* Create a pointer template from rcx in rdx */
    mov rdx, ~SLIST8_POINTER_MASK
    and rdx, rcx

    /* Shift the NextEntry pointer */
    shr r9, SLIST8A_NEXTENTRY_SHIFT

    /* Combine to new pointer in rdx */
    or rdx, r9

_ExpInterlockedPopEntrySListFault:

    /* Load the next NextEntry pointer to r9 */
    mov r9, [rdx]

    /* Shift bits in place */
    shl r9, SLIST8A_NEXTENTRY_SHIFT

    /* Combine into r8 */
    or r8, r9

_ExpInterlockedPopEntrySListEnd:

    /* If [rcx] equals rax, exchange it with r8 */
    lock cmpxchg [rcx], r8

    /* If not equal, retry with rax, being the content of [rcx] now */
    jnz _ExpInterlockedPopEntrySListResume

    /* Shift the pointer bits in place */
    and rax, SLIST8A_NEXTENTRY_MASK
    shr rax, SLIST8A_NEXTENTRY_SHIFT

    /* Use rcx as pointer template */
    mov rdx, ~SLIST8_POINTER_MASK
    and rdx, rcx

    /* Combine result and return */
    or rax, rdx
    ret

_RtlInterlockedPopEntrySListEmpty:
    xor rax, rax
    ret

_RtlInterlockedPopEntrySList16:
    /* This is a 16 byte header */

    /* Save rbx */
    push rbx

    /* Copy rcx to r8, as we need rcx for the exchange */
    mov r8, rcx

_ExpInterlockedPopEntrySListResume16:

    /* Check if ListHead->NextEntry is NULL */
    mov r9, rdx
    and r9, SLIST16B_NEXTENTY_MASK
    jz _RtlInterlockedPopEntrySListEmpty16

_ExpInterlockedPopEntrySListFault16:

    /* Get next pointer */
    mov rcx, [r9]

    /* Set ListHead->HeaderType = 1 and  ListHead->Init = 1 */
    or rcx, 0x3

    /* Copy Depth and Sequence number and adjust Depth */
    lea rbx, [rax - SLIST16A_DEPTH_INC]

_ExpInterlockedPopEntrySListEnd16:

    /* If [r8] equals rdx:rax, exchange it with rcx:rbx */
    lock cmpxchg16b [r8]

    /* If not equal, retry with rdx:rax, being the content of [r8] now */
    jnz _ExpInterlockedPopEntrySListResume16

    /* Copy the old NextEntry pointer to rax */
    mov rax, rdx
    and rax, SLIST16B_NEXTENTY_MASK

    /* Return */
    pop rbx
    ret

_RtlInterlockedPopEntrySListEmpty16:
    xor rax, rax
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
_ExpInterlockedPushEntrySList:

    /* Load ListHead->Alignment into rax */
    mov rax, [rcx]

    /* Load ListHead->Region into rdx */
    mov r9, [rcx + 8]

    /* Check what kind of header this is */
    test r9, SLIST8B_HEADERTYPE_MASK
    jnz _RtlInterlockedPushEntrySList16

    /* We have an 8 byte header */

_RtlInterlockedPushEntrySListLoop:

    /* Get ListHead->NextEntry */
    mov r8, rax
    and r8, SLIST8A_NEXTENTRY_MASK
    jz _RtlInterlockedPushEntrySListEmpty

    /* Shift the NextEntry pointer */
    shr r8, SLIST8A_NEXTENTRY_SHIFT

    /* Create a pointer template from rcx in rdx */
    mov r9, ~SLIST8_POINTER_MASK
    and r9, rcx

    /* Combine to new pointer and save as ListEntry->NextEntry */
    or r8, r9

_RtlInterlockedPushEntrySListEmpty:
    /* Store the NextEntry pointer in the new ListEntry */
    mov [rdx], r8

    /* Shift and mask the new ListEntry pointer */
    mov r8, rdx
    shl r8, SLIST8A_NEXTENTRY_SHIFT
    and r8, SLIST8A_NEXTENTRY_MASK

    /* Copy and adjust depth and sequence number */
    lea r9, [rax + SLIST8A_DEPTH_INC + SLIST8A_SEQUENCE_INC]
    and r9, SLIST8A_SEQUENCE_MASK | SLIST8A_DEPTH_MASK

    /* Combine to exchange value in r8 */
    or r8, r9

    /* Save the NextEntry in r9 */
    mov r9, [rdx]

    /* If [rcx] equals rax, exchange it with r8 */
    lock cmpxchg [rcx], r8

    /* If not equal, retry with rax, being the content of [rcx] now */
    jnz _RtlInterlockedPushEntrySListLoop

    /* Return the old NextEntry pointer */
    mov rax, r9
    ret

_RtlInterlockedPushEntrySList16:
    /* This is a 16 byte header */

    /* Save rbx */
    push rbx

    /* Copy rcx/rdx to r8/r9, as we need rcx/rdx for the exchange */
    mov r8, rcx
    mov r9, rdx

    /* Set ListHead->HeaderType = 1 and  ListHead->Init = 1 */
    mov rcx, rdx
    or rcx, 0x3

    mov rdx, [r8 + 8]

_RtlInterlockedPushEntrySListLoop16:

    /* Move ListHead->NextEntry to rbx */
    mov rbx, rdx
    and rbx, SLIST16B_NEXTENTY_MASK

    /* Store next pointer in ListEntry->NextEntry */
    mov [r9], rbx

    /* Copy Depth and Sequence number and adjust Depth */
    lea rbx, [rax + SLIST16A_DEPTH_INC + SLIST16A_SEQUENCE_INC]

    /* If [r8] equals rdx:rax, exchange it with rcx:rbx */
    lock cmpxchg16b [r8]

    /* If not equal, retry with rdx:rax, being the content of [r8] now */
    jnz _RtlInterlockedPushEntrySListLoop16

    /* Copy the old NextEntry pointer to rax */
    mov rax, rdx
    and rax, SLIST16B_NEXTENTY_MASK

    /* Return */
    pop rbx
    ret

.endproc

/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedFlushSList(
 *     IN PSINGLE_LIST_ENTRY ListHead);
 */
.proc RtlInterlockedFlushSList
_ExpInterlockedFlushSList:

    /* Load ListHead->Region into rdx */
    mov rax, [rcx + 8]

    /* Check what kind of header this is */
    test rax, SLIST8B_HEADERTYPE_MASK
    jnz _RtlInterlockedFlushSList16

    /* We have an 8 byte header */

_RtlInterlockedFlushSListLoop:

    /* Zero ListHead->Alignment */ 
    xor r8, r8

    /* If [rcx] equals rax, exchange it with r8 */
    lock cmpxchg [rcx], r8

    /* If not equal, retry with rax, being the content of [rcx] now */
    jnz _RtlInterlockedFlushSListLoop

    /* Use rcx as pointer template */
    mov rdx, ~SLIST8_POINTER_MASK
    or rdx, rcx

    /* Combine result and return */
    or rax, rdx
    ret

_RtlInterlockedFlushSList16:
    /* We have a 16 byte header */
    push rbx

    mov rdx, [rcx + 8]
    xor rbx, rbx
    mov rcx, 0x3

_RtlInterlockedFlushSListLoop16:

    /* If [r8] equals rdx:rax, exchange it with rcx:rbx */
    lock cmpxchg16b [r8]

    /* If not equal, retry with rdx:rax, being the content of [r8] now */
    jnz _RtlInterlockedFlushSListLoop16

    /* Copy the old NextEntry pointer to rax */
    mov rax, rdx
    and rax, SLIST16B_NEXTENTY_MASK

    /* Return */
    pop rbx
    ret

.endproc
