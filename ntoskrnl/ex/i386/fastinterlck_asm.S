/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/i386/fastinterlck_asm.S
 * PURPOSE:         FASTCALL Interlocked Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <asm.inc>
#include <ks386.inc>
#include <internal/i386/asmmacro.S>

/* FUNCTIONS ****************************************************************/

.code32

/*
 * NOTE: These functions must obey the following rules:
 *  - Acquire locks only on MP systems.
 *  - Be safe at HIGH_LEVEL (no paged access).
 *  - Preserve flags.
 *  - Disable interrups.
 */

/*VOID
 *FASTCALL
 *ExInterlockedAddLargeStatistic(IN PLARGE_INTEGER Addend,
 *                               IN ULONG Increment)
 */
PUBLIC @ExInterlockedAddLargeStatistic@8
@ExInterlockedAddLargeStatistic@8:

#ifdef CONFIG_SMP
    /* Do the addition */
    lock add [ecx], edx

    /* Check for carry bit and return */
    jc .l1
    ret

.l1:
    /* Add carry */
    lock adc dword ptr [ecx+4], 0
#else
    /* Do the addition and add the carry */
    add dword ptr [ecx], edx
    adc dword ptr [ecx+4], 0
#endif
    /* Return */
    ret



/*PSINGLE_LIST_ENTRY
 *FASTCALL
 *ExInterlockedPopEntrySList(IN PSINGLE_LIST_ENTRY ListHead,
 *                           IN PKSPIN_LOCK Lock)
 */
PUBLIC @ExInterlockedPopEntrySList@8
PUBLIC @InterlockedPopEntrySList@4
PUBLIC _ExpInterlockedPopEntrySListResume@0
PUBLIC _ExpInterlockedPopEntrySListFault@0
PUBLIC _ExpInterlockedPopEntrySListEnd@0
@ExInterlockedPopEntrySList@8:
@InterlockedPopEntrySList@4:

    /* Save registers */
    push ebx
    push ebp

    /* Pointer to list */
    mov ebp, ecx

    /* Get sequence number and link pointer */
_ExpInterlockedPopEntrySListResume@0:
    mov edx, [ebp+4]
    mov eax, [ebp]

    /* Check if the list is empty */
    or eax, eax
    jz .l7

    /* Copy depth and adjust it */
    lea ecx, [edx-1]

    /* Get next pointer and do the exchange */
_ExpInterlockedPopEntrySListFault@0:
    mov ebx, [eax]
_ExpInterlockedPopEntrySListEnd@0:
    LOCK cmpxchg8b qword ptr [ebp]
    jnz _ExpInterlockedPopEntrySListResume@0

    /* Restore registers and return */
.l7:
    pop ebp
    pop ebx
    ret

/*PSINGLE_LIST_ENTRY
 *FASTCALL
 *ExInterlockedPushEntrySList(IN PSINGLE_LIST_ENTRY ListHead,
 *                            IN PSINGLE_LIST_ENTRY ListEntry,
 *                            IN PKSPIN_LOCK Lock)
 */
PUBLIC @ExInterlockedPushEntrySList@12
@ExInterlockedPushEntrySList@12:

    /* So we can fall through below */
    pop [esp]

PUBLIC @InterlockedPushEntrySList@8
@InterlockedPushEntrySList@8:

    /* Save registers */
    push ebx
    push ebp

    /* Pointer to list */
    mov ebp, ecx
    mov ebx, edx

    /* Get sequence number and link pointer */
    mov edx, [ebp+4]
    mov eax, [ebp]

.l8:
    /* Set link pointer */
    mov [ebx], eax

    /* Copy sequence number and adjust it */
    lea ecx, [edx + HEX(10001)]

    /* Do the exchange */
    LOCK cmpxchg8b qword ptr [ebp]
    jnz .l8

    /* Restore registers and return */
    pop ebp
    pop ebx
    ret

/*PSINGLE_LIST_ENTRY
 *FASTCALL
 *ExInterlockedFlushSList(IN PSINGLE_LIST_ENTRY ListHead)
 */
PUBLIC @ExInterlockedFlushSList@4
@ExInterlockedFlushSList@4:

    /* Save registers */
    push ebx
    push ebp

    /* Clear ebx */
    xor ebx, ebx

    /* Pointer to list */
    mov ebp, ecx

    /* Get sequence number and link pointer */
    mov edx, [ebp+4]
    mov eax, [ebp]

.l9:
    /* Check if the list is empty */
    or eax, eax
    jz .l10

    /* Clear depth and pointer */
    mov ecx, edx
    mov cx, bx

    /* Do the exchange */
    LOCK cmpxchg8b qword ptr [ebp]
    jnz .l9

    /* Restore registers and return */
.l10:
    pop ebp
    pop ebx
    ret

END
/* EOF */
