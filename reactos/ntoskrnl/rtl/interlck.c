/* $Id: interlck.c,v 1.9 2001/07/06 21:30:33 ekohl Exp $
 *
 * reactos/ntoskrnl/rtl/interlck.c
 *
 */
#include <reactos/config.h>
#include <ntos.h>
#include <internal/debug.h>

#define USE_FASTCALL

/**********************************************************************
 * FASTCALL: @InterlockedIncrement@4
 * STDCALL : _InterlockedIncrement@4
 */
LONG FASTCALL
InterlockedIncrement(PLONG Addend);
/*
 * FUNCTION: Increments a caller supplied variable of type LONG as an 
 * atomic operation
 * ARGUMENTS;
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
PVOID FASTCALL
InterlockedCompareExchange(PVOID *Destination,
			   PVOID Exchange,
			   PVOID Comperand);

__asm__("\n\t.global @InterlockedCompareExchange@12\n\t"
	"@InterlockedCompareExchange@12:\n\t"
	"movl 4(%esp),%eax\n\t"
	"cmpxchg %edx,(%ecx)\n\t"
	"ret $4\n\t");

/* EOF */
