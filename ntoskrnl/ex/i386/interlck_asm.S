/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/i386/interlck_asm.S
 * PURPOSE:         STDCALL Interlocked Functions
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

/*PLIST_ENTRY
 *NTAPI
 *ExInterlockedAddLargeInteger(IN PLIST_ENTRY ListHead,
 *                             IN PLIST_ENTRY ListEntry,
 *                             IN PKSPIN_LOCK Lock)
 */
.global _ExInterlockedAddLargeInteger@16
_ExInterlockedAddLargeInteger@16:

    /* Prepare stack frame */
    push ebp
    mov ebp, esp
    sub esp, 8

    /* Save lock pointer */
#ifdef CONFIG_SMP
    mov eax, [ebp+20]
#endif

    /* Save flags and disable interrupts */
.start1:
    pushfd
    cli

    /* Acquire lock */
    ACQUIRE_SPINLOCK(eax, .spin1)

    /* Do the calculation */
    mov eax, [ebp+8]
    mov ecx, [eax]
    mov edx, [eax+4]

    /* Save result */
    mov [ebp-8], ecx
    mov [ebp-4], edx

    /* Add increment */
    add ecx, [ebp+12]
    adc edx, [ebp+16]

    /* Save result */
    mov [eax], ecx
    mov [eax+4], edx

    /* Release lock */
#ifdef CONFIG_SMP
    mov eax, [ebp+20]
    RELEASE_SPINLOCK(eax)
#endif

    /* Restore flags */
    popfd

    /* Restore frame and return values */
    mov eax, [ebp-8]
    mov edx, [ebp-4]
    mov esp, ebp
    pop ebp
    ret 16

#ifdef CONFIG_SMP
.spin1:
    popfd
    SPIN_ON_LOCK(eax, .start1)
#endif

/*PLIST_ENTRY
 *NTAPI
 *ExInterlockedInsertHeadList(IN PLIST_ENTRY ListHead,
 *                            IN PLIST_ENTRY ListEntry,
 *                            IN PKSPIN_LOCK Lock)
 */
.global _ExInterlockedAddUlong@12
_ExInterlockedAddUlong@12:

    /* Save flags and disable interrupts */
    pushfd

    /* Get lock address */
#ifdef CONFIG_SMP
    mov edx, [ebp+16]
#endif
.start2:
    cli

    /* Acquire lock */
    ACQUIRE_SPINLOCK(edx, .spin2)

    /* Do the calculation */
    mov ecx, [esp+8]
#ifdef CONFIG_SMP
    mov eax, [ecx]
    add eax, [esp+12]

    /* Save result */
    mov [ecx], eax
#else

    /* Do the calculation */
    mov edx, [ecx]
    mov eax, edx
    add edx, [esp+12]

    /* Save result */
    mov [ecx], edx
#endif

    /* Release lock, restore flags and return */
#ifdef CONFIG_SMP
    sub eax, [esp+12]
    RELEASE_SPINLOCK(edx)
#endif
    popfd
    ret 12

#ifdef CONFIG_SMP
.spin2:
    popfd
    pushfd
    SPIN_ON_LOCK(eax, .start2)
#endif

/*PLIST_ENTRY
 *NTAPI
 *ExInterlockedInsertHeadList(IN PLIST_ENTRY ListHead,
 *                            IN PLIST_ENTRY ListEntry,
 *                            IN PKSPIN_LOCK Lock)
 */
.global _ExInterlockedInsertHeadList@12
_ExInterlockedInsertHeadList@12:

    /* Save lock pointer */
#ifdef CONFIG_SMP
    mov edx, [esp+12]
#endif

    /* Save flags and disable interrupts */
.start3:
    pushfd
    cli
    ACQUIRE_SPINLOCK(edx, .spin3)

    /* Get list pointers */
    mov eax, [esp+8]
    mov ecx, [esp+12]
    mov edx, [eax]

    /* Do the insert */
    mov [ecx], edx
    mov [ecx+4], eax
    mov [eax], ecx
    mov [edx+4], ecx

    /* Release lock */
#ifdef CONFIG_SMP
    mov ecx, [esp+16]
    RELEASE_SPINLOCK(ecx)
#endif

    /* Restore flags */
    popfd

    /* check if the list was empty and return NULL */
    xor eax, edx
    jz 2f

    /* Return pointer */
    mov eax, edx

2:
    ret 12

#ifdef CONFIG_SMP
.spin3:
    popfd
    SPIN_ON_LOCK(edx, .start3)
#endif

/*PLIST_ENTRY
 *NTAPI
 *ExInterlockedInsertTailList(IN PLIST_ENTRY ListHead,
 *                            IN PLIST_ENTRY ListEntry,
 *                            IN PKSPIN_LOCK Lock)
 */
.global _ExInterlockedInsertTailList@12
_ExInterlockedInsertTailList@12:

    /* Save lock pointer */
#ifdef CONFIG_SMP
    mov edx, [esp+12]
#endif

    /* Save flags and disable interrupts */
.start4:
    pushfd
    cli
    ACQUIRE_SPINLOCK(edx, .spin4)

    /* Get list pointers */
    mov eax, [esp+8]
    mov ecx, [esp+12]
    mov edx, [eax+4]

    /* Do the insert */
    mov [ecx], eax
    mov [ecx+4], edx
    mov [eax+4], ecx
    mov [edx], ecx

    /* Release lock */
#ifdef CONFIG_SMP
    mov ecx, [esp+16]
    RELEASE_SPINLOCK(ecx)
#endif

    /* Restore flags */
    popfd

    /* check if the list was empty and return NULL */
    xor eax, edx
    jz 2f

    /* Return pointer */
    mov eax, edx

2:
    ret 12

#ifdef CONFIG_SMP
.spin4:
    popfd
    SPIN_ON_LOCK(edx, .start4)
#endif

/*PLIST_ENTRY
 *NTAPI
 *ExInterlockedRemoveHeadList(IN PLIST_ENTRY ListHead,
 *                            IN PKSPIN_LOCK Lock)
 */
.global _ExInterlockedRemoveHeadList@8
_ExInterlockedRemoveHeadList@8:

    /* Save lock pointer */
#ifdef CONFIG_SMP
    mov edx, [esp+8]
#endif

    /* Save flags and disable interrupts */
.start5:
    pushfd
    cli
    ACQUIRE_SPINLOCK(edx, .spin5)

    /* Get list pointers */
    mov edx, [esp+8]
    mov eax, [edx]

    /* Check if it's empty */
    cmp eax, edx
    je 2f

    /* Get next entry and do deletion */
    mov ecx, [eax]
    mov [edx], ecx
    mov [ecx+4], edx

    /* Release lock */
#ifdef CONFIG_SMP
    mov ecx, [esp+12]
    RELEASE_SPINLOCK(edx)
#endif

    /* Restore flags */
    popfd

    /* Return */
    ret 8

2:
    /* Release lock */
#ifdef CONFIG_SMP
    mov edx, [esp+12]
    RELEASE_SPINLOCK(edx)
#endif

    /* Restore flags */
    popfd

    /* Return empty list */
    xor eax, eax
    ret 8

#ifdef CONFIG_SMP
.spin5:
    popfd
    SPIN_ON_LOCK(edx, .start5)
#endif

/*PSINGLE_LIST_ENTRY
 *NTAPI
 *ExInterlockedPopEntryList(IN PSINGLE_LIST_ENTRY ListHead,
 *                          IN PKSPIN_LOCK Lock)
 */
.global _ExInterlockedPopEntryList@8
_ExInterlockedPopEntryList@8:

    /* Save lock pointer */
#ifdef CONFIG_SMP
    mov edx, [esp+8]
#endif

    /* Save flags and disable interrupts */
.start6:
    pushfd
    cli
    ACQUIRE_SPINLOCK(edx, .spin6)

    /* Get list pointers */
    mov edx, [esp+8]
    mov eax, [ecx]

    /* Check if it's empty */
    or eax, eax
    je 3f

    /* Get next entry and do deletion */
    mov edx, [eax]
    mov [ecx], edx

2:
    /* Release lock */
#ifdef CONFIG_SMP
    mov ecx, [esp+12]
    RELEASE_SPINLOCK(edx)
#endif

    /* Restore flags */
    popfd

    /* Return */
    ret 8

3:
    /* Return empty list */
    xor eax, eax
    jmp 2b

#ifdef CONFIG_SMP
.spin6:
    popfd
    SPIN_ON_LOCK(edx, .start6)
#endif

/*PSINGLE_LIST_ENTRY
 *NTAPI
 *ExInterlockedPushEntryList(IN PSINGLE_LIST_ENTRY ListHead,
 *                           IN PSINGLE_LIST_ENTRY ListEntry,
 *                           IN PKSPIN_LOCK Lock)
 */
.global _ExInterlockedPushEntryList@12
_ExInterlockedPushEntryList@12:

    /* Save lock pointer */
#ifdef CONFIG_SMP
    mov edx, [esp+12]
#endif

    /* Save flags and disable interrupts */
.start7:
    pushfd
    cli
    ACQUIRE_SPINLOCK(edx, .spin7)

    /* Get list pointers */
    mov edx, [esp+8]
    mov eax, [edx]
    mov ecx, [esp+12]

    /* Do push */
    mov [ecx], eax
    mov [edx], ecx

    /* Release lock */
#ifdef CONFIG_SMP
    mov ecx, [esp+16]
    RELEASE_SPINLOCK(edx)
#endif

    /* Restore flags */
    popfd

    /* Return */
    ret 12

#ifdef CONFIG_SMP
.spin7:
    popfd
    SPIN_ON_LOCK(edx, .start7)
#endif

/*INTERLOCKED_RESULT
 *NTAPI
 *ExInterlockedIncrementLong(IN PLONG Addend,
 *                           IN PKSPIN_LOCK Lock)
 */
.global _ExInterlockedIncrementLong@8
_ExInterlockedIncrementLong@8:

    /* Get addend */
    mov eax, [esp+4]

    /* Do the op */
    LOCK add dword ptr [eax], 1

    /* Return */
    lahf
    and eax, EFLAG_SELECT
    ret 8

/*INTERLOCKED_RESULT
 *NTAPI
 *ExInterlockedDecrementLong(IN PLONG Addend,
 *                           IN PKSPIN_LOCK Lock)
 */
.global _ExInterlockedDecrementLong@8
_ExInterlockedDecrementLong@8:

    /* Get addend */
    mov eax, [esp+4]

    /* Do the op */
    LOCK sub dword ptr [eax], 1

    /* Return */
    lahf
    and eax, EFLAG_SELECT
    ret 8

/*ULONG
 *NTAPI
 *ExInterlockedExchangeUlong(IN PULONG Taget,
 *                           IN ULONG Value,
 *                           IN PKSPIN_LOCK Lock)
 */
.global _ExInterlockedExchangeUlong@12
_ExInterlockedExchangeUlong@12:

    /* Get pointers */
    mov edx, [esp+4]
    mov eax, [esp+8]

#ifdef CONFIG_SMP
    /* On MP, do the exchange */
    xchg [edx], eax
#else
    /* On UP, disable interrupts and save flags */
    pushfd
    cli

    /* Switch values */
    mov eax, [edx]
    mov [edx], ecx
    popfd
#endif

    /* Return */
    ret 12

/*INTERLOCKED_RESULT
 *NTAPI
 *Exi386InterlockedIncrementLong(IN PLONG Addend,
 *                               IN PKSPIN_LOCK Lock)
 */
.global _Exi386InterlockedIncrementLong@4
_Exi386InterlockedIncrementLong@4:

    /* Get addend */
    mov eax, [esp+4]

    /* Do the op */
    LOCK add dword ptr [eax], 1

    /* Return */
    lahf
    and eax, EFLAG_SELECT
    ret 4

/*INTERLOCKED_RESULT
 *NTAPI
 *Exi386InterlockedDecrementLong(IN PLONG Addend,
 *                               IN PKSPIN_LOCK Lock)
 */
.global _Exi386InterlockedDecrementLong@4
_Exi386InterlockedDecrementLong@4:

    /* Get addend */
    mov eax, [esp+4]

    /* Do the op */
    LOCK sub dword ptr [eax], 1

    /* Return */
    lahf
    and eax, EFLAG_SELECT
    ret 8

/*ULONG
 *NTAPI
 *Exi386InterlockedExchangeUlong(IN PULONG Taget,
 *                               IN ULONG Value,
 *                               IN PKSPIN_LOCK Lock)
 */
.global _Exi386InterlockedExchangeUlong@12
_Exi386InterlockedExchangeUlong@12:

    /* Get pointers */
    mov edx, [esp+4]
    mov eax, [esp+8]

#ifdef CONFIG_SMP
    /* On MP, do the exchange */
    xchg [edx], eax
#else
    /* On UP, disable interrupts and save flags */
    pushfd
    cli

    /* Switch values */
    mov eax, [edx]
    mov [edx], ecx
    popfd
#endif

    /* Return */
    ret 8

/* Ugly hack: Use InterlockedExchange64 */
.att_syntax
.global @ExfpInterlockedExchange64@8
@ExfpInterlockedExchange64@8:
	pushl %ebx
	pushl %esi
	movl %ecx,%esi
	movl (%edx),%ebx
	movl 4(%edx),%ecx
1:
	movl (%esi),%eax
	movl 4(%esi),%edx
	LOCK
	cmpxchg8b (%esi)
	jnz 1b
	popl %esi
	popl %ebx
	ret
	
/* EOF */
