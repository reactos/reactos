/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/i386/interlck.S
 * PURPOSE:         Rtl Interlocked Functions for x86
 * PROGRAMMERS:     Timo Kreuzer
 */

.intel_syntax noprefix

/* FUNCTIONS ****************************************************************/


/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedPopEntrySList(
 *     IN PSLIST_HEADER ListHead);
 */
.global _RtlInterlockedPopEntrySList@4
_RtlInterlockedPopEntrySList@4:

    /* Save registers */
    push ebx
    push ebp

    /* Load ListHead into ebp */
    mov ebp, [esp + 8]

    /* Load ListHead->Next into eax */
    mov eax, [ebp]

    /* Load ListHead->Depth and ListHead->Sequence into edx */
    mov edx, [ebp+4]

1:
    /* Check if ListHead->Next is NULL */
    or eax, eax
    jz 2f

    /* Copy Depth and Sequence number and adjust Depth */
    lea ecx, [edx - 1]

    /* Get next pointer */
    mov ebx, [eax]

    /* If [ebp] equals edx:eax, exchange it with ecx:ebx */
    lock cmpxchg8b [ebp]

    /* If not equal, retry with edx:eax, being the content of [ebp] now */
    jnz 1b

    /* Restore registers and return */
2:
    pop ebp
    pop ebx
    ret 4


/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedPushEntrySList(
 *     IN PSLIST_HEADER ListHead,
 *     IN PSLIST_ENTRY ListEntry);
 */
.global _RtlInterlockedPushEntrySList@8
_RtlInterlockedPushEntrySList@8:

    /* Save registers */
    push ebx
    push ebp

    /* Load ListHead into ebp */
    mov ebp, [esp + 4]

    /* Load ListEntry into ebx */
    mov ebx, [esp + 12]

    /* Load ListHead->Next into eax */
    mov eax, [ebp]

    /* Load ListHead->Depth and ListHead->Sequence into edx */
    mov edx, [ebp + 4]

1:
    /* Set ListEntry->Next to ListHead->Next */
    mov [ebx], eax

    /* Copy ListHead->Depth and ListHead->Sequence and adjust them */
    lea ecx, [edx + 0x10001]

    /* If [ebp] equals edx:eax, exchange it with ecx:ebx */
    lock cmpxchg8b [ebp]
 
    /* If not equal, retry with edx:eax, being the content of [ebp] now */
    jnz 1b

    /* Restore registers and return */
    pop ebp
    pop ebx
    ret 8


/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedFlushSList(
 *     IN PSINGLE_LIST_ENTRY ListHead);
 */
.global _RtlInterlockedFlushSList@4
_RtlInterlockedFlushSList@4:

    /* Save registers */
    push ebx
    push ebp

    /* Clear ebx */
    xor ebx, ebx

    /* Load ListHead into ebp */
    mov ebp, [esp + 4]

    /* Load ListHead->Next into eax */
    mov eax, [ebp]

    /* Load ListHead->Depth and ListHead->Sequence into edx */
    mov edx, [ebp + 4]

1:
    /* Check if ListHead->Next is NULL */
    or eax, eax
    jz 2f

    /* Copy Depth and Sequence number to ecx */
    mov ecx, edx

    /* Clear Depth in cx */
    xor cx, cx

    /* If [ebp] equals edx:eax, exchange it with ecx:ebx */
    lock cmpxchg8b [ebp]
 
    /* If not equal, retry with edx:eax, being the content of [ebp] now */
    jnz 1b

    /* Restore registers and return */
2:
    pop ebp
    pop ebx
    ret 4
