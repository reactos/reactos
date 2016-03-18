/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/amd64/slist.S
 * PURPOSE:         Rtl Interlocked Functions for amd64
 * PROGRAMMERS:     Timo Kreuzer
 */

#include <asm.inc>
#include <ksamd64.inc>

EXTERN RtlpUse16ByteSLists:BYTE

/*
typedef union DECLSPEC_ALIGN(16) _SLIST_HEADER
{
    struct
    {
        ULONGLONG Alignment;
        ULONGLONG Region;
    };
    struct
    {
        ULONGLONG Depth:16;
        ULONGLONG Sequence:9;
        ULONGLONG NextEntry:39;
        ULONGLONG HeaderType:1;
        ULONGLONG Init:1;
        ULONGLONG Reserved:59;
        ULONGLONG Region:3;
    } Header8;
    struct
    {
        ULONGLONG Depth:16;
        ULONGLONG Sequence:48;
        ULONGLONG HeaderType:1;
        ULONGLONG Init:1;
        ULONGLONG Reserved:2;
        ULONGLONG NextEntry:60;
    } Header16;
    struct
    {
        ULONGLONG Depth:16;
        ULONGLONG Sequence:48;
        ULONGLONG HeaderType:1;
        ULONGLONG Reserved:3;
        ULONGLONG NextEntry:60;
    } HeaderX64;
} SLIST_HEADER, *PSLIST_HEADER;
*/

#define SLIST8A_DEPTH_MASK       HEX(000000000000FFFF)
#define SLIST8A_DEPTH_INC        HEX(0000000000000001)
#define SLIST8A_SEQUENCE_MASK    HEX(0000000001FF0000)
#define SLIST8A_SEQUENCE_INC     HEX(0000000000010000)
#define SLIST8A_NEXTENTRY_MASK   HEX(FFFFFFFFFE000000)
#define SLIST8A_NEXTENTRY_SHIFT  21
#define SLIST8B_HEADERTYPE_MASK  HEX(0000000000000001)
#define SLIST8B_INIT_MASK        HEX(0000000000000002)
#define SLIST8B_REGION_MASK      HEX(E000000000000000)
#define SLIST8_POINTER_MASK      HEX(000007FFFFFFFFFF)

#define SLIST16A_DEPTH_MASK      HEX(000000000000FFFF)
#define SLIST16A_DEPTH_INC       HEX(0000000000000001)
#define SLIST16A_SEQUENCE_MASK   HEX(FFFFFFFFFFFF0000)
#define SLIST16A_SEQUENCE_INC    HEX(0000000000010000)
#define SLIST16B_HEADERTYPE_MASK HEX(0000000000000001)
#define SLIST16B_INIT_MASK       HEX(0000000000000002)
#define SLIST16B_NEXTENTRY_MASK  HEX(FFFFFFFFFFFFFFF0)


/* FUNCTIONS ****************************************************************/

.code64

PUBLIC ExpInterlockedPopEntrySList
PUBLIC ExpInterlockedPopEntrySListResume
PUBLIC ExpInterlockedPopEntrySListFault
PUBLIC ExpInterlockedPopEntrySListEnd
PUBLIC ExpInterlockedPopEntrySListResume16
PUBLIC ExpInterlockedPopEntrySListFault16
PUBLIC ExpInterlockedPopEntrySListEnd16
PUBLIC ExpInterlockedPushEntrySList
PUBLIC ExpInterlockedFlushSList

PUBLIC RtlInterlockedFlushSList
PUBLIC RtlInterlockedPopEntrySList
PUBLIC RtlInterlockedPushEntrySList

/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedPopEntrySList(
 *     IN PSLIST_HEADER ListHead);
 */
RtlInterlockedPopEntrySList:
ExpInterlockedPopEntrySList:

    /* Load ListHead->Region into rdx */
    mov rdx, [rcx + 8]

    /* Load ListHead->Alignment into rax */
    mov rax, [rcx]

    /* Check for 16 byte SList support */
    cmp byte ptr [RtlpUse16ByteSLists], 0
    jne RtlInterlockedPopEntrySList16

    /* Use the 8 byte header */

ExpInterlockedPopEntrySListResume:

    /* Check if ListHead->NextEntry is NULL */
    mov r9, rax
    and r9, SLIST8A_NEXTENTRY_MASK
    jz RtlInterlockedPopEntrySListEmpty

    /* Copy Depth and Sequence number and adjust Depth */
    lea r8, [rax - SLIST8A_DEPTH_INC]
    and r8, (SLIST8A_SEQUENCE_MASK OR SLIST8A_DEPTH_MASK)

    /* Create a pointer template from rcx in rdx */
    mov rdx, (NOT SLIST8_POINTER_MASK)
    and rdx, rcx

    /* Shift the NextEntry pointer */
    shr r9, SLIST8A_NEXTENTRY_SHIFT

    /* Combine to new pointer in rdx */
    or rdx, r9

ExpInterlockedPopEntrySListFault:

    /* Load the next NextEntry pointer to r9 */
    mov r9, [rdx]

    /* Shift bits in place */
    shl r9, SLIST8A_NEXTENTRY_SHIFT

    /* Combine into r8 */
    or r8, r9

ExpInterlockedPopEntrySListEnd:

    /* If [rcx] equals rax, exchange it with r8 */
    lock cmpxchg [rcx], r8

    /* If not equal, retry with rax, being the content of [rcx] now */
    jne ExpInterlockedPopEntrySListResume

    /* Shift the pointer bits in place */
    and rax, SLIST8A_NEXTENTRY_MASK
    shr rax, SLIST8A_NEXTENTRY_SHIFT

    /* Use rcx as pointer template */
    mov rdx, (NOT SLIST8_POINTER_MASK)
    and rdx, rcx

    /* Combine result and return */
    or rax, rdx
    ret

RtlInterlockedPopEntrySListEmpty:
    xor rax, rax
    ret

RtlInterlockedPopEntrySList16:
    /* This is a 16 byte header
       rcx == ListHead
       rdx == ListHead->Region
       rax == ListHead->Alignment */

    /* Save rbx */
    push rbx

    /* Copy rcx to r8, as we need rcx for the exchange */
    mov r8, rcx

ExpInterlockedPopEntrySListResume16:

    /* Set r9 = ListHead->NextEntry and check if it is NULL */
    mov r9, rdx
    and r9, SLIST16B_NEXTENTRY_MASK
    jz RtlInterlockedPopEntrySListEmpty16

ExpInterlockedPopEntrySListFault16:

    /* Set NewListHead.Next = ListHead->NextEntry->Next */
    mov rcx, [r9]

    /* Set NewListHead.HeaderType = 1 and  NewListHead.Init = 1 */
    or rcx, (SLIST16B_HEADERTYPE_MASK or SLIST16B_INIT_MASK)

    /* Copy Depth and Sequence number and adjust Depth */
    lea rbx, [rax - SLIST16A_DEPTH_INC]

ExpInterlockedPopEntrySListEnd16:

    /* If [r8] equals rdx:rax, exchange it with rcx:rbx */
    lock cmpxchg16b [r8]

    /* If not equal, retry with rdx:rax, being the content of [r8] now */
    jne ExpInterlockedPopEntrySListResume16

    /* Copy the old NextEntry pointer to rax */
    mov rax, rdx
    and rax, SLIST16B_NEXTENTRY_MASK

    /* Return */
    pop rbx
    ret

RtlInterlockedPopEntrySListEmpty16:
    xor rax, rax
    pop rbx
    ret


/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedPushEntrySList(
 *     IN PSLIST_HEADER ListHead,
 *     IN PSLIST_ENTRY ListEntry);
 */
RtlInterlockedPushEntrySList:
ExpInterlockedPushEntrySList:

#if DBG
    /* Make sure the ListEntry is 16 bytes aligned */
    test rdx, HEX(0F)
    jz ExpInterlockedPushEntrySListChecked
    /* Not aligned, raise an assertion */
    int HEX(2C)
ExpInterlockedPushEntrySListChecked:

    /* Make sure RtlpUse16ByteSLists is initialized */
    cmp byte ptr [RtlpUse16ByteSLists], HEX(FF)
    jne ExpInterlockedPushEntrySListChecked2
    /* Not initialized, raise an assertion */
    int HEX(2C)
ExpInterlockedPushEntrySListChecked2:
#endif

    /* Load ListHead->Alignment into rax */
    mov rax, [rcx]

    /* Load ListHead->Region into r9 */
    mov r9, [rcx + 8]

    /* Check for 16 byte SList support */
    cmp byte ptr [RtlpUse16ByteSLists], 0
    jne RtlInterlockedPushEntrySList16

    /* Use the 8 byte header */

RtlInterlockedPushEntrySListLoop:

    /* Get ListHead->NextEntry */
    mov r8, rax
    and r8, SLIST8A_NEXTENTRY_MASK
    jz RtlInterlockedPushEntrySListEmpty

    /* Shift the NextEntry pointer */
    shr r8, SLIST8A_NEXTENTRY_SHIFT

    /* Create a pointer template from rcx in rdx */
    mov r9, (NOT SLIST8_POINTER_MASK)
    and r9, rcx

    /* Combine to new pointer and save as ListEntry->NextEntry */
    or r8, r9

RtlInterlockedPushEntrySListEmpty:
    /* Store the NextEntry pointer in the new ListEntry */
    mov [rdx], r8

    /* Shift and mask the new ListEntry pointer */
    mov r8, rdx
    shl r8, SLIST8A_NEXTENTRY_SHIFT
    and r8, SLIST8A_NEXTENTRY_MASK

    /* Copy and adjust depth and sequence number */
    lea r9, [rax + SLIST8A_DEPTH_INC + SLIST8A_SEQUENCE_INC]
    and r9, SLIST8A_SEQUENCE_MASK OR SLIST8A_DEPTH_MASK

    /* Combine to exchange value in r8 */
    or r8, r9

    /* Save the NextEntry in r9 */
    mov r9, [rdx]

    /* If [rcx] equals rax, exchange it with r8 */
    lock cmpxchg [rcx], r8

    /* If not equal, retry with rax, being the content of [rcx] now */
    jne RtlInterlockedPushEntrySListLoop

    /* Return the old NextEntry pointer */
    mov rax, r9
    ret

RtlInterlockedPushEntrySList16:
    /* This is a 16 byte header
       rcx = ListHead
       rdx = ListEntry
       rax = ListHead->Alignment
       r9 = ListHead->Region */

    /* Save rbx */
    push rbx

    /* Copy rcx/rdx to r8/r9, as we need rcx/rdx for the exchange */
    mov r8, rcx
    mov r9, rdx

    /* Set NewListHead.NextEntry = ListEntry */
    mov rcx, rdx

    /* Set NewListHead.HeaderType = 1 and NewListHead.Init = 1 */
    or rcx, (SLIST16B_HEADERTYPE_MASK or SLIST16B_INIT_MASK)

    /* Set rdx = ListHead->Region */
    mov rdx, [r8 + 8]

RtlInterlockedPushEntrySListLoop16:
    /* r8 = ListHead
       r9 = ListEntry
       rax = ListHead->Alignment
       rdx = ListHead->Region
    */

    /* Move ListHead->NextEntry to rbx */
    mov rbx, rdx
    and rbx, SLIST16B_NEXTENTRY_MASK

    /* Store next pointer in ListEntry->Next */
    mov [r9], rbx

    /* Copy and increment Depth and Sequence number to rbx */
    lea rbx, [rax + SLIST16A_DEPTH_INC + SLIST16A_SEQUENCE_INC]

    /* If [r8] equals rdx:rax, exchange it with rcx:rbx */
    lock cmpxchg16b [r8]

    /* If not equal, retry with rdx:rax, being the content of [r8] now */
    jne RtlInterlockedPushEntrySListLoop16

    /* Copy the old NextEntry pointer to rax */
    mov rax, rdx
    and rax, SLIST16B_NEXTENTRY_MASK

    /* Return */
    pop rbx
    ret


/* PSLIST_ENTRY
 * NTAPI
 * RtlInterlockedFlushSList(
 *     IN PSLIST_HEADER ListHead);
 */
RtlInterlockedFlushSList:
ExpInterlockedFlushSList:

    /* Load ListHead->Region into rdx */
    mov rdx, [rcx + 8]

    /* Load ListHead->Alignment into rax */
    mov rax, [rcx]

    /* Check for 16 byte SList support */
    cmp byte ptr [RtlpUse16ByteSLists], 0
    jne RtlInterlockedFlushSList16

    /* Use the 8 byte header */

RtlInterlockedFlushSListLoop:

    /* Zero NewListHead.Alignment */
    xor r8, r8

    /* If [rcx] equals rax, exchange it with r8 */
    lock cmpxchg [rcx], r8

    /* If not equal, retry with rax, being the content of [rcx] now */
    jne RtlInterlockedFlushSListLoop

    /* Create a pointer template from rcx in rdx */
    mov rdx, (NOT SLIST8_POINTER_MASK)
    and rdx, rcx

    /* Load the old NextEntry pointer into rax */
    and rax, SLIST8A_NEXTENTRY_MASK
    shr rax, SLIST8A_NEXTENTRY_SHIFT

    /* Combine result and return */
    or rax, rdx
    ret

RtlInterlockedFlushSList16:
    /* We have a 16 byte header
        rcx = ListHead
        rax = ListHead->Alignment
        rdx = ListHead->Region
    */

    /* Save rbx */
    push rbx

    /* Load ListHead into r8, as we need rcx for the exchange */
    mov r8, rcx

    /* Initialize an ampty NewListHead in rcx:rbx */
    xor rbx, rbx
    mov rcx, (SLIST16B_HEADERTYPE_MASK or SLIST16B_INIT_MASK)

RtlInterlockedFlushSListLoop16:

    /* If [r8] equals rdx:rax, exchange it with rcx:rbx */
    lock cmpxchg16b [r8]

    /* If not equal, retry with rdx:rax, being the content of [r8] now */
    jne RtlInterlockedFlushSListLoop16

    /* Copy the old NextEntry pointer to rax */
    mov rax, rdx
    and rax, SLIST16B_NEXTENTRY_MASK

    /* Return */
    pop rbx
    ret

END

