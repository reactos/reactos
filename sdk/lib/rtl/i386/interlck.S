/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/i386/interlck.S
 * PURPOSE:         Rtl Interlocked Functions for x86
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <asm.inc>

/* FUNCTIONS ****************************************************************/
.code

/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedPopEntrySList(
 *     IN PSLIST_HEADER ListHead);
 */
PUBLIC _ExpInterlockedPopEntrySListResume@0
PUBLIC _ExpInterlockedPopEntrySListEnd@0
PUBLIC _ExpInterlockedPopEntrySListFault@0
PUBLIC _RtlInterlockedPopEntrySList@4
_RtlInterlockedPopEntrySList@4:

    /* Save registers */
    push ebx
    push ebp

    /* Load ListHead into ebp */
    mov ebp, [esp + 12]
_ExpInterlockedPopEntrySListResume@0:
    /* Load ListHead->Next into eax */
    mov eax, [ebp]

    /* Load ListHead->Depth and ListHead->Sequence into edx */
    mov edx, [ebp + 4]

    /* Check if ListHead->Next is NULL */
    or eax, eax
    jz _ExpInterlockedPopEntrySList2

    /* Copy Depth and Sequence number and adjust Depth */
    lea ecx, [edx - 1]

_ExpInterlockedPopEntrySListFault@0:
    /* Get next pointer */
    mov ebx, [eax]
_ExpInterlockedPopEntrySListEnd@0:
    /* If [ebp] equals edx:eax, exchange it with ecx:ebx */
    LOCK cmpxchg8b qword ptr [ebp]

    /* If not equal, retry with edx:eax, being the content of [ebp] now */
    jnz _ExpInterlockedPopEntrySListResume@0

    /* Restore registers and return */
_ExpInterlockedPopEntrySList2:
    pop ebp
    pop ebx
    ret 4


/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedPushEntrySList(
 *     IN PSLIST_HEADER ListHead,
 *     IN PSLIST_ENTRY ListEntry);
 */
PUBLIC _RtlInterlockedPushEntrySList@8
_RtlInterlockedPushEntrySList@8:

    /* Save registers */
    push ebx
    push ebp

    /* Load ListHead into ebp */
    mov ebp, [esp + 12]

    /* Load ListEntry into ebx */
    mov ebx, [esp + 16]

    /* Load ListHead->Next into eax */
    mov eax, [ebp]

    /* Load ListHead->Depth and ListHead->Sequence into edx */
    mov edx, [ebp + 4]

_RtlpInterlockedPushEntrySListResume:
    /* Set ListEntry->Next to ListHead->Next */
    mov [ebx], eax

    /* Copy ListHead->Depth and ListHead->Sequence and adjust them */
    lea ecx, [edx + HEX(10001)]

    /* If [ebp] equals edx:eax, exchange it with ecx:ebx */
    LOCK cmpxchg8b qword ptr [ebp]

    /* If not equal, retry with edx:eax, being the content of [ebp] now */
    jnz _RtlpInterlockedPushEntrySListResume

    /* Restore registers and return */
    pop ebp
    pop ebx
    ret 8


/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedFlushSList(
 *     IN PSINGLE_LIST_ENTRY ListHead);
 */
PUBLIC _RtlInterlockedFlushSList@4
_RtlInterlockedFlushSList@4:

    /* Save registers */
    push ebx
    push ebp

    /* Clear ebx */
    xor ebx, ebx

    /* Load ListHead into ebp */
    mov ebp, [esp + 12]

    /* Load ListHead->Next into eax */
    mov eax, [ebp]

    /* Load ListHead->Depth and ListHead->Sequence into edx */
    mov edx, [ebp + 4]

_RtlpInterlockedFlushSListResume:
    /* Check if ListHead->Next is NULL */
    or eax, eax
    jz _RtlpInterlockedFlushSListEnd

    /* Copy Depth and Sequence number to ecx */
    mov ecx, edx

    /* Clear Depth in cx */
    xor cx, cx

    /* If [ebp] equals edx:eax, exchange it with ecx:ebx */
    LOCK cmpxchg8b qword ptr [ebp]

    /* If not equal, retry with edx:eax, being the content of [ebp] now */
    jnz _RtlpInterlockedFlushSListResume

    /* Restore registers and return */
_RtlpInterlockedFlushSListEnd:
    pop ebp
    pop ebx
    ret 4

END
