/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/i386/fastinterlck_asm.S
 * PURPOSE:         FASTCALL Interlocked Functions
 * PROGRAMMERS:     Alex Ionescu (alex@relsoft.net)
 */
 
/* INCLUDES ******************************************************************/
#include <ndk/asm.h>
#include <internal/i386/asmmacro.S>
.intel_syntax noprefix

/* FUNCTIONS ****************************************************************/

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
.global @ExInterlockedAddLargeStatistic@8
@ExInterlockedAddLargeStatistic@8:

#ifdef CONFIG_SMP
    /* Do the addition */
    lock add [ecx], edx

    /* Check for carry bit and return */
    jb 1f
    ret

1:
    /* Add carry */
    lock adc dword ptr [ecx+4], 0
#else
    /* Do the addition and add the carry */
    add dword ptr [ecx], edx
    adc dword ptr [ecx+4], 0
#endif
    /* Return */
    ret

/*ULONG
 *FASTCALL
 *ExfInterlockedAddUlong(IN PULONG Addend,
 *                       IN ULONG Increment,
 *                       IN PKSPIN_LOCK Lock)
 */
.global @ExfInterlockedAddUlong@12
@ExfInterlockedAddUlong@12:

    /* Save flags */
    pushfd

#ifdef CONFIG_SMP
    /* Get lock address */
    mov eax, [esp+8]
.start1:
#endif
    /* Disable interrupts */
    cli

    /* Acquire lock */
    ACQUIRE_SPINLOCK(eax, .spin1)

    /* Do the add */
    mov eax, [ecx]
    add [ecx], edx

#ifdef CONFIG_SMP
    /* Get spinlock address and release it */
    mov edx, [esp+8]
    RELEASE_SPINLOCK(edx)
#endif

    /* Restore flags and return */
    popfd
    ret 4

#ifdef CONFIG_SMP
.spin1:
    /* Restore flags and spin */
    popfd
    pushfd
    SPIN_ON_LOCK(eax, .start1)
#endif

/*PLIST_ENTRY
 *FASTCALL
 *ExfInterlockedInsertHeadList(IN PLIST_ENTRY ListHead,
 *                             IN PLIST_ENTRY ListEntry,
 *                             IN PKSPIN_LOCK Lock)
 */
.global @ExfInterlockedInsertHeadList@12
@ExfInterlockedInsertHeadList@12:

#ifdef CONFIG_SMP
    /* Save lock address */
    push esi
    mov esi, [esp+8]
#endif

    /* Save flags and disable interrupts */
    pushfd
.start2:
    cli

    /* Acquire lock */
    ACQUIRE_SPINLOCK(esi, .spin2)

    /* Get list pointer */
    mov eax, [ecx]

    /* Do the insert */
    mov [edx], eax
    mov [edx+4], ecx
    mov [ecx], edx
    mov [eax+4], edx

    /* Release lock and restore flags */
    RELEASE_SPINLOCK(esi)
    popfd

#ifdef CONFIG_SMP
    pop esi
#endif

    /* Check if list was empty */
    xor eax, ecx
    jz 2f

    /* Return list pointer */
    xor eax, ecx
2:
    ret 4

#ifdef CONFIG_SMP
.spin2:
    popfd
    pushfd
    SPIN_ON_LOCK(esi, .start2)
#endif

/*PLIST_ENTRY
 *FASTCALL
 *ExfInterlockedInsertTailList(IN PLIST_ENTRY ListHead,
 *                            IN PLIST_ENTRY ListEntry,
 *                            IN PKSPIN_LOCK Lock)
 */
.global @ExfInterlockedInsertTailList@12
@ExfInterlockedInsertTailList@12:

#ifdef CONFIG_SMP
    /* Save lock address */
    push esi
    mov esi, [esp+8]
#endif

    /* Save flags and disable interrupts */
    pushfd
.start3:
    cli

    /* Acquire lock */
    ACQUIRE_SPINLOCK(esi, .spin3)

    /* Get list pointer */
    mov eax, [ecx+4]

    /* Do the insert */
    mov [edx], ecx
    mov [edx+4], eax
    mov [ecx+4], edx
    mov [eax], edx

    /* Release lock and restore flags */
    RELEASE_SPINLOCK(esi)
    popfd

#ifdef CONFIG_SMP
    pop esi
#endif

    /* Check if list was empty */
    xor eax, ecx
    jz 2f

    /* Return list pointer */
    xor eax, ecx
2:
    ret 4

#ifdef CONFIG_SMP
.spin3:
    popfd
    pushfd
    SPIN_ON_LOCK(esi, .start3)
#endif

/*PLIST_ENTRY
 *FASTCALL
 *ExfInterlockedRemoveHeadList(IN PLIST_ENTRY ListHead,
 *                             IN PKSPIN_LOCK Lock)
 */
.global @ExfInterlockedRemoveHeadList@8
@ExfInterlockedRemoveHeadList@8:

    /* Save flags and disable interrupts */
.start4:
    pushfd
    cli
    ACQUIRE_SPINLOCK(edx, .spin4)

    /* Get list pointer */
    mov eax, [ecx]

    /* Check if it's empty */
    cmp eax, ecx
    je 2f

    /* Get the next entry and do the deletion */
#ifdef CONFIG_SMP
    push ebx
    mov ebx, [eax]
    mov [ecx], ebx
    mov [ebx+4], ecx
#else
    mov edx, [eax]
    mov [ecx], edx
    mov [edx+4], ecx
#endif

    /* Release lock */
#ifdef CONFIG_SMP
    RELEASE_SPINLOCK(edx)
    pop ebx
#endif

    /* Restore flags */
    popfd

    /* Return */
    ret

2:
    /* Release lock */
    RELEASE_SPINLOCK(edx)

    /* Restore flags */
    popfd

    /* Return empty list */
    xor eax, eax
    ret

#ifdef CONFIG_SMP
.spin4:
    popfd
    SPIN_ON_LOCK(edx, .start4)
#endif

/*PSINGLE_LIST_ENTRY
 *FASTCALL
 *ExfInterlockedPopEntryList(IN PSINGLE_LIST_ENTRY ListHead,
 *                           IN PKSPIN_LOCK Lock)
 */
.global @ExfInterlockedPopEntryList@8
@ExfInterlockedPopEntryList@8:

    /* Save flags and disable interrupts */
.start5:
    pushfd
    cli
    ACQUIRE_SPINLOCK(edx, .spin5)

    /* Get list pointer */
    mov eax, [ecx]

    /* Check if it's empty */
    or eax, eax
    je 3f

    /* Get next entry and do deletion */
#ifdef CONFIG_SMP
    push edx
#endif
    mov edx, [eax]
    mov [ecx], edx
#ifdef CONFIG_SMP
    pop edx
#endif

2:
    /* Release lock */
    RELEASE_SPINLOCK(edx)

    /* Restore flags */
    popfd

    /* Return */
    ret

3:
    /* Return empty list */
    xor eax, eax
    jmp 2b

#ifdef CONFIG_SMP
.spin5:
    popfd
    SPIN_ON_LOCK(edx, .start5)
#endif

/*PSINGLE_LIST_ENTRY
 *FASTCALL
 *ExfInterlockedPushEntryList(IN PSINGLE_LIST_ENTRY ListHead,
 *                            IN PSINGLE_LIST_ENTRY ListEntry,
 *                            IN PKSPIN_LOCK Lock)
 */
.global @ExfInterlockedPushEntryList@12
@ExfInterlockedPushEntryList@12:

    /* Save flags */
    pushfd

    /* Save lock pointer */
#ifdef CONFIG_SMP
    push edx
    mov edx, [esp+12]
#endif

    /* Disable interrupts */
.start6:
    cli
#ifdef CONFIG_SMP
    ACQUIRE_SPINLOCK(edx, .spin6)
    pop edx
#endif

    /* Get list pointer */
    mov eax, [ecx]

    /* Do push */
    mov [edx], eax
    mov [ecx], edx

    /* Release lock */
#ifdef CONFIG_SMP
    mov edx, [esp+8]
    RELEASE_SPINLOCK(edx)
#endif

    /* Restore flags */
    popfd

    /* Return */
    ret 4

#ifdef CONFIG_SMP
.spin6:
    pop edx
    popfd
    pushfd
    push edx
    mov edx, [esp+12]
    SPIN_ON_LOCK(edx, .start6)
#endif

/*PSINGLE_LIST_ENTRY
 *FASTCALL
 *ExInterlockedPopEntrySList(IN PSINGLE_LIST_ENTRY ListHead,
 *                           IN PKSPIN_LOCK Lock)
 */
.global @ExInterlockedPopEntrySList@8
.global @InterlockedPopEntrySList@4
.global _ExpInterlockedPopEntrySListResume@0
.global _ExpInterlockedPopEntrySListFault@0
.global _ExpInterlockedPopEntrySListEnd@0
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
    jz 2f

    /* Copy sequence number and adjust it */
    lea ecx, [edx-1]

    /* Get next pointer and do the exchange */
_ExpInterlockedPopEntrySListFault@0:
    mov ebx, [eax]
_ExpInterlockedPopEntrySListEnd@0:
    LOCK cmpxchg8b [ebp]
    jnz _ExpInterlockedPopEntrySListResume@0

    /* Restore registers and return */
2:
    pop ebp
    pop ebx
    ret

/*PSINGLE_LIST_ENTRY
 *FASTCALL
 *ExInterlockedPushEntrySList(IN PSINGLE_LIST_ENTRY ListHead,
 *                            IN PSINGLE_LIST_ENTRY ListEntry,
 *                            IN PKSPIN_LOCK Lock)
 */
.global @ExInterlockedPushEntrySList@12
@ExInterlockedPushEntrySList@12:

    /* So we can fall through below */
    pop [esp]

.global @InterlockedPushEntrySList@8
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

1:
    /* Set link pointer */
    mov [ebx], eax

    /* Copy sequence number and adjust it */
    lea ecx, [edx+0x10001]

    /* Do the exchange */
    LOCK cmpxchg8b [ebp]
    jnz 1b

    /* Restore registers and return */
2:
    pop ebp
    pop ebx
    ret

/*PSINGLE_LIST_ENTRY
 *FASTCALL
 *ExInterlockedFlushSList(IN PSINGLE_LIST_ENTRY ListHead)
 */
.global @ExInterlockedFlushSList@4
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

1:
    /* Check if the list is empty */
    or eax, eax
    jz 2f

    /* Clear sequence and pointer */
    mov ecx, edx
    mov cx, bx

    /* Do the exchange */
    LOCK cmpxchg8b [ebp]
    jnz 1b

    /* Restore registers and return */
2:
    pop ebp
    pop ebx
    ret

/*INTERLOCKED_RESULT
 *FASTCALL
 *Exfi386InterlockedIncrementLong(IN PLONG Addend)
 */
.global @Exfi386InterlockedIncrementLong@4
@Exfi386InterlockedIncrementLong@4:

    /* Do the op */
    LOCK add dword ptr [ecx], 1

    /* Return */
    lahf
    and eax, EFLAG_SELECT
    ret

/*INTERLOCKED_RESULT
 *FASTCALL
 *Exfi386InterlockedDecrementLong(IN PLONG Addend)
 */
.global @Exfi386InterlockedDecrementLong@4
@Exfi386InterlockedDecrementLong@4:

    /* Do the op */
    LOCK sub dword ptr [ecx], 1

    /* Return */
    lahf
    and eax, EFLAG_SELECT
    ret

/*ULONG
 *FASTCALL
 *Exfi386InterlockedExchangeUlong(IN PULONG Taget,
 *                                IN ULONG Value)
 */
.global @Exfi386InterlockedExchangeUlong@8
@Exfi386InterlockedExchangeUlong@8:

#ifdef CONFIG_SMP
    /* On MP, do the exchange */
    xchg [ecx], edx
    mov eax, edx
#else
    /* On UP, use cmpxchg */
    mov eax, [ecx]
1:
    cmpxchg [ecx], edx
    jnz 1b
#endif

    /* Return */
    ret

/*PVOID
 *FASTCALL
 *ExfInterlockedCompareExchange64(IN PLONGLONG Destination,
 *                                IN PLONGLONG Exchange,
 *                                IN PLONGLONG Comperand)
 */
.global @ExfInterlockedCompareExchange64@12
@ExfInterlockedCompareExchange64@12:

    /* Save registers */
    push ebx
    push ebp

    /* Get destination pointer, exchange value and comperand value/address */
    mov ebp, ecx
    mov ebx, [edx]
    mov ecx, [edx+4]
    mov edx, [esp+12]
    mov eax, [edx]
    mov edx, [edx+4]

    /* Do the op */
    LOCK cmpxchg8b [ebp]

    /* Restore volatiles */
    pop ebp
    pop ebx

    /* Return */
    ret 4

/*PVOID
 *FASTCALL
 *ExfInterlockedCompareExchange64(IN PLONGLONG Destination,
 *                                IN PLONGLONG Exchange,
 *                                IN PLONGLONG Comperand,
 *                                IN PKSPIN_LOCK Lock)
 */
.global @ExInterlockedCompareExchange64@16
@ExInterlockedCompareExchange64@16:

    /* Save registers */
    push ebp
    push ebp

    /* Get destination pointer, exchange value and comperand value/address */
    mov ebp, ecx
    mov ebx, [edx]
    mov ecx, [edx+4]
    mov edx, [esp+12]
    mov eax, [edx]
    mov edx, [edx+4]

    /* Do the op */
    LOCK cmpxchg8b [ebp]

    /* Restore volatiles */
    pop ebp
    pop ebx

    /* Return */
    ret 8

/*** Non-586 functions ***/

/*PSINGLE_LIST_ENTRY
 *FASTCALL
 *ExfInterlockedPopEntrySList(IN PSINGLE_LIST_ENTRY ListHead,
 *                            IN PKSPIN_LOCK Lock)
 */
.global @ExfInterlockedPopEntrySList@8
@ExfInterlockedPopEntrySList@8:

    /* Save flags */
.starta:
    pushfd

    /* Disable interrupts */
    cli

    /* Acquire lock */
    ACQUIRE_SPINLOCK(edx, .spina)

    /* Get the next link and check if it's empty */
    mov eax, [ecx]
    or eax, eax
    jz 1f

    /* Get address of the next link and store it */
    push [eax]
    pop [ecx]

    /* Decrement list depth */
    dec dword ptr [ecx+4]

1:
#ifdef CONFIG_SMP
    /* Release spinlock */
    RELEASE_SPINLOCK(edx)
#endif

    /* Restore flags and return */
    popfd
    ret 0

#ifdef CONFIG_SMP
.spina:
    /* Restore flags and spin */
    popfd
    SPIN_ON_LOCK(edx, .starta)
#endif

/*PSINGLE_LIST_ENTRY
 *FASTCALL
 *ExfInterlockedPushEntrySList(IN PSINGLE_LIST_ENTRY ListHead,
 *                             IN PSINGLE_LIST_ENTRY ListEntry,
 *                             IN PKSPIN_LOCK Lock)
 */
.global @ExfInterlockedPushEntrySList@12
@ExfInterlockedPushEntrySList@12:

    /* Save flags */
.startb:
    pushfd

    /* Disable interrupts */
    cli

    /* Acquire lock */
#ifndef CONFIG_SMP
    mov eax, [esp+8]
    ACQUIRE_SPINLOCK(eax, .spinb)
#endif

    /* Get the next link and check if it's empty */
    push [ecx]

    /* Get address of the next link and store it */
    pop [edx]
    mov [ecx], edx

    /* Increment list depth */
    inc dword ptr [ecx+4]

#ifdef CONFIG_SMP
    /* Release spinlock */
    RELEASE_SPINLOCK(eax)
#endif

    /* Restore flags and return */
    popfd
    ret 4

#ifdef CONFIG_SMP
.spinb:
    /* Restore flags and spin */
    popfd
    SPIN_ON_LOCK(eax, .startb)
#endif

/*PVOID
 *FASTCALL
 *ExpInterlockedCompareExchange64(IN PLONGLONG Destination,
 *                                IN PLONGLONG Exchange,
 *                                IN PLONGLONG Comperand,
 *                                IN PKSPIN_LOCK Lock)
 */
.global @ExpInterlockedCompareExchange64@16
@ExpInterlockedCompareExchange64@16:

    /* Save registers */
    push ebp
    push ebp

    /* Get destination pointer, exchange value and comperand value/address */
    mov ebp, ecx
    mov ebx, [edx]
    mov ecx, [edx+4]
    mov edx, [esp+12]
    mov eax, [edx]
    mov edx, [edx+4]

#ifdef CONFIG_SMP
    /* Save ESI so we can store KSPINLOCK in it */
    push esi

    /* Save flags and lock, and disable interrupts */
    pushfd
    mov esi, [esp+24]
.startc:
    cli

    /* Acquire the spinlock */
    ACQUIRE_SPINLOCK(esi, .spinc)
#else
    /* Save flags and disable interrupts */
    pushfd
    cli
#endif

    /* Compare bottom */
    cmp eax, [ebp]
    jne NoMatch

    /* Compare top */
    cmp edx, [ebp+4]
    jne NoMatch

    /* Save new value */
    mov [ebp], ebx
    mov [ebp+4], ecx

AfterSave:
#ifdef CONFIG_SMP
    /* Release lock, restore volatiles and flags */
    RELEASE_SPINLOCK(esi)
    popfd
    pop esi
#else
    popfd
#endif

    /* Restore the other volatiles and return */
    pop ebp
    pop ebx

    /* Return */
    ret 8

NoMatch:
    /* Return the current value */
    mov eax, [ebp]
    mov edx, [ebp+4]
    jmp AfterSave

#ifdef CONFIG_SMP
.spinc:
    /* Restore flags and spin */
    popfd
    pushfd
    SPIN_ON_LOCK(esi, .startc)
#endif
/* EOF */
