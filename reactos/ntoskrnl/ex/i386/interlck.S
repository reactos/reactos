/* $Id: interlck.c 15557 2005-05-28 07:26:38Z hbirr $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ex/i386/interlck.c
 * PURPOSE:         No purpose listed.
 *
 * PROGRAMMERS:     No programmer listed.
 */

#ifdef CONFIG_SMP
#define LOCK lock
#else
#define LOCK
#endif

.global @Exfi386InterlockedIncrementLong@4
@Exfi386InterlockedIncrementLong@4:
	LOCK
	addl $1,(%ecx)
	lahf
	andl $0xC000, %eax
	ret

.global @Exfi386InterlockedDecrementLong@4
@Exfi386InterlockedDecrementLong@4:
	LOCK
	subl $1,(%ecx)
	lahf
	andl $0xC000, %eax
	ret

.global @Exfi386InterlockedExchangeUlong@8
@Exfi386InterlockedExchangeUlong@8:
	LOCK
	xchgl %edx,(%ecx)
	movl  %edx,%eax
	ret

.global _Exi386InterlockedIncrementLong@4
_Exi386InterlockedIncrementLong@4:
	movl 4(%esp),%eax
	LOCK
	addl $1,(%eax)
	lahf
	andl $0xC000, %eax
	ret $4

.global _Exi386InterlockedDecrementLong@4
_Exi386InterlockedDecrementLong@4:
	movl 4(%esp),%eax
	LOCK
	subl $1,(%eax)
	lahf
	andl $0xC000, %eax
	ret $4

.global _Exi386InterlockedExchangeUlong@8
_Exi386InterlockedExchangeUlong@8:
	movl 4(%esp),%edx
	movl 8(%esp),%eax
	LOCK
	xchgl %eax,(%edx)
	ret $8

.global @InterlockedIncrement@4
@InterlockedIncrement@4:
	movl $1,%eax
	LOCK
	xaddl %eax,(%ecx)
	incl %eax
	ret

.global @InterlockedDecrement@4
@InterlockedDecrement@4:
	movl $-1,%eax
	LOCK
	xaddl %eax,(%ecx)
	decl %eax
	ret

.global @InterlockedExchange@8
@InterlockedExchange@8:
	LOCK
	xchgl %edx,(%ecx)
	movl  %edx,%eax
	ret

.global @InterlockedExchangeAdd@8
@InterlockedExchangeAdd@8:
	LOCK
	xaddl %edx,(%ecx)
	movl %edx,%eax
	ret

.global @InterlockedClearBit@8
@InterlockedClearBit@8:
	LOCK
	btr %edx,(%ecx)
	setc %al
	ret

.global @InterlockedCompareExchange@12
@InterlockedCompareExchange@12:
	movl 4(%esp),%eax
	LOCK
	cmpxchg %edx,(%ecx)
	ret $4

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

.global @ExfInterlockedCompareExchange64@12
@ExfInterlockedCompareExchange64@12:
	pushl %ebx
	pushl %esi
	movl %ecx,%esi
	movl (%edx),%ebx
	movl 4(%edx),%ecx
	movl 12(%esp),%edx
	movl (%edx),%eax
	movl 4(%edx),%edx
	LOCK
	cmpxchg8b (%esi)
	popl %esi
	popl %ebx
	ret  $4

/* EOF */
