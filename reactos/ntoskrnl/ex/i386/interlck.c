/* $Id: interlck.c,v 1.2 2002/08/28 07:08:25 hbirr Exp $
 *
 * reactos/ntoskrnl/ex/i386/interlck.c
 *
 */
#include <ddk/ntddk.h>


INTERLOCKED_RESULT FASTCALL
Exfi386InterlockedIncrementLong(IN PLONG Addend);

__asm__("\n\t.global @Exfi386InterlockedIncrementLong@4\n\t"
	"@Exfi386InterlockedIncrementLong@4:\n\t"
	"addl $1,(%ecx)\n\t"
	"lahf\n\t"
	"andl $0xC000, %eax\n\t"
	"ret\n\t");


INTERLOCKED_RESULT FASTCALL
Exfi386InterlockedDecrementLong(IN PLONG Addend);

__asm__("\n\t.global @Exfi386InterlockedDecrementLong@4\n\t"
	"@Exfi386InterlockedDecrementLong@4:\n\t"
	"subl $1,(%ecx)\n\t"
	"lahf\n\t"
	"andl $0xC000, %eax\n\t"
	"ret\n\t");


ULONG FASTCALL
Exfi386InterlockedExchangeUlong(IN PULONG Target,
				IN ULONG Value);

__asm__("\n\t.global @Exfi386InterlockedExchangeUlong@8\n\t"
	"@Exfi386InterlockedExchangeUlong@8:\n\t"
	"movl (%ecx),%eax\n"
	"xchgl %edx,(%ecx)\n\t"
	"ret\n\t");



INTERLOCKED_RESULT STDCALL
Exi386InterlockedIncrementLong(IN PLONG Addend);

__asm__("\n\t.global _Exi386InterlockedIncrementLong@4\n\t"
	"_Exi386InterlockedIncrementLong@4:\n\t"
	"movl 4(%esp),%eax\n\t"
	"addl $1,(%eax)\n\t"
	"lahf\n\t"
	"andl $0xC000, %eax\n\t"
	"ret $4\n\t");


INTERLOCKED_RESULT STDCALL
Exi386InterlockedDecrementLong(IN PLONG Addend);

__asm__("\n\t.global _Exi386InterlockedDecrementLong@4\n\t"
	"_Exi386InterlockedDecrementLong@4:\n\t"
	"movl 4(%esp),%eax\n\t"
	"subl $1,(%eax)\n\t"
	"lahf\n\t"
	"andl $0xC000, %eax\n\t"
	"ret $4\n\t");


ULONG STDCALL
Exi386InterlockedExchangeUlong(IN PULONG Target,
			       IN ULONG Value);

__asm__("\n\t.global _Exi386InterlockedExchangeUlong@8\n\t"
	"_Exi386InterlockedExchangeUlong@8:\n\t"
	"movl 4(%esp),%edx\n\t"
	"movl 8(%esp),%eax\n\t"
	"xchgl %eax,(%edx)\n\t"
	"ret $8\n\t");


/**********************************************************************
 * FASTCALL: @InterlockedIncrement@4
 * STDCALL : _InterlockedIncrement@4
 */
LONG FASTCALL
InterlockedIncrement(PLONG Addend);
/*
 * FUNCTION: Increments a caller supplied variable of type LONG as an 
 * atomic operation
 * ARGUMENTS:
 *     Addend = Points to a variable whose value is to be increment
 * RETURNS: The incremented value
 */

__asm__("\n\t.global @InterlockedIncrement@4\n\t"
	"@InterlockedIncrement@4:\n\t"
	"movl $1,%eax\n\t"
	"xaddl %eax,(%ecx)\n\t"
	"incl %eax\n\t"
	"ret\n\t");


/**********************************************************************
 * FASTCALL: @InterlockedDecrement@4
 * STDCALL : _InterlockedDecrement@4
 */
LONG FASTCALL
InterlockedDecrement(PLONG Addend);

__asm__("\n\t.global @InterlockedDecrement@4\n\t"
	"@InterlockedDecrement@4:\n\t"
	"movl $-1,%eax\n\t"
	"xaddl %eax,(%ecx)\n\t"
	"decl %eax\n\t"
	"ret\n\t");


/**********************************************************************
 * FASTCALL: @InterlockedExchange@8
 * STDCALL : _InterlockedExchange@8
 */

LONG FASTCALL
InterlockedExchange(PLONG Target,
		    LONG Value);

__asm__("\n\t.global @InterlockedExchange@8\n\t"
	"@InterlockedExchange@8:\n\t"
	"movl (%ecx),%eax\n"
	"xchgl %edx,(%ecx)\n\t"
	"ret\n\t");


/**********************************************************************
 * FASTCALL: @InterlockedExchangeAdd@8
 * STDCALL: _InterlockedExchangeAdd@8
 */
LONG FASTCALL
InterlockedExchangeAdd(PLONG Addend,
		       LONG Value);

__asm__("\n\t.global @InterlockedExchangeAdd@8\n\t"
	"@InterlockedExchangeAdd@8:\n\t"
	"xaddl %edx,(%ecx)\n\t"
	"movl %edx,%eax\n\t"
	"ret\n\t");


/**********************************************************************
 * FASTCALL: @InterlockedCompareExchange@12
 * STDCALL: _InterlockedCompareExchange@12
 */
LONG FASTCALL
InterlockedCompareExchange(PLONG Destination,
			   LONG Exchange,
			   LONG Comperand);

__asm__("\n\t.global @InterlockedCompareExchange@12\n\t"
	"@InterlockedCompareExchange@12:\n\t"
	"movl 4(%esp),%eax\n\t"
	"cmpxchg %edx,(%ecx)\n\t"
	"ret $4\n\t");

/* EOF */
