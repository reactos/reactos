/* $Id: interlck.c,v 1.8 2001/06/20 20:00:36 ekohl Exp $
 *
 * reactos/ntoskrnl/rtl/interlck.c
 *
 * FIXME: change decorated names when __fastcall will be available
 * (for both egcs 1.1.2 and gcc 2.95 FASTCALL == STDCALL).
 */
#include <reactos/config.h>
#include <ntos.h>
#include <internal/debug.h>

//#define USE_FASTCALL

#if 0
LONG FASTCALL InterlockedIncrement(PLONG Addend)
{
   LONG r;
   (*Addend)++;
   r = (*Addend);
   return(r);
}

LONG FASTCALL InterlockedDecrement(PLONG Addend)
{
   LONG r;
   (*Addend)--;
   r = (*Addend);
   return(r);
}
#endif

#ifdef I386_FIX

LONG FASTCALL InterlockedIncrement (PLONG Addend)
{
    *Addend = *Addend + 1;
    return *Addend;
}

LONG FASTCALL InterlockedDecrement (PLONG Addend)
{
    *Addend = *Addend - 1;
    return *Addend;
}

LONG
FASTCALL
InterlockedExchange (
	PLONG	Target,
	LONG	Value
	)
{
    LONG Val = *Target;
    *Target = Value;
    return Val;
}

LONG
FASTCALL
InterlockedExchangeAdd (
	PLONG	Addend,
	LONG	Value
	)
{
    LONG Val = *Addend;
    *Addend = Value;
    return Val;
}

PVOID
FASTCALL
InterlockedCompareExchange (
	PVOID	* Destination,
	PVOID	Exchange,
	PVOID	Comperand
	)
{
    LONG Val = *((LONG*)Destination);
    
    if (*((LONG*)Destination) == (LONG)Comperand) {
        *((LONG*)Destination) = (LONG)Exchange;
    }
    return (PVOID)Val;
}

#else /* I386_FIX */

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

#ifndef USE_FASTCALL
__asm__("\n\t.global _InterlockedIncrement@4\n\t"
	"_InterlockedIncrement@4:\n\t"
	"pushl %ebp\n\t"
	"movl  %esp,%ebp\n\t"
	"pushl %ebx\n\t"
	"movl $1,%eax\n\t"
	"movl 8(%ebp),%ebx\n\t"
	"xaddl %eax,(%ebx)\n\t"
	"incl %eax\n\t"
	"popl %ebx\n\t"
	"movl %ebp,%esp\n\t"
	"popl %ebp\n\t"
	"ret $4\n\t");
#else
__asm__("\n\t.global @InterlockedIncrement@4\n\t"
	"@InterlockedIncrement@4:\n\t"
	"movl $1,%eax\n\t"
	"xaddl %eax,(%ecx)\n\t"
	"incl %eax\n\t"
	"ret\n\t");
#endif

/**********************************************************************
 * FASTCALL: @InterlockedDecrement@4
 * STDCALL : _InterlockedDecrement@4
 */
LONG FASTCALL
InterlockedDecrement(PLONG Addend);

#ifndef USE_FASTCALL
__asm__("\n\t.global _InterlockedDecrement@4\n\t"
	"_InterlockedDecrement@4:\n\t"
	"pushl %ebp\n\t"
	"movl  %esp,%ebp\n\t"
	"pushl %ebx\n\t"
	"movl $-1,%eax\n\t"
	"movl 8(%ebp),%ebx\n\t"
	"xaddl %eax,(%ebx)\n\t"
	"decl %eax\n\t"
	"popl %ebx\n\t"
	"movl %ebp,%esp\n\t"
	"popl %ebp\n\t"
	"ret $4\n\t");
#else
__asm__("\n\t.global @InterlockedDecrement@4\n\t"
	"@InterlockedDecrement@4:\n\t"
	"movl $-1,%eax\n\t"
	"xaddl %eax,(%ecx)\n\t"
	"decl %eax\n\t"
	"ret\n\t");
#endif

/**********************************************************************
 * FASTCALL: @InterlockedExchange@8
 * STDCALL : _InterlockedExchange@8
 */

LONG FASTCALL
InterlockedExchange(PLONG Target,
		    LONG Value);

#ifndef USE_FASTCALL
__asm__("\n\t.global _InterlockedExchange@8\n\t"
	"_InterlockedExchange@8:\n\t"
	"pushl %ebp\n\t"
	"movl  %esp,%ebp\n\t"
	"pushl %ebx\n\t"
	"movl  12(%ebp),%eax\n\t"
	"movl  8(%ebp),%ebx\n\t"
	"xchgl %eax,(%ebx)\n\t"
	"popl  %ebx\n\t"
	"movl  %ebp,%esp\n\t"
	"popl  %ebp\n\t"
	"ret $8\n\t");
#else
__asm__("\n\t.global @InterlockedExchange@8\n\t"
	"@InterlockedExchange@8:\n\t"
	"movl (%ecx),%eax\n"
	"xchgl %edx,(%ecx)\n\t"
	"ret\n\t");
#endif


/**********************************************************************
 * FASTCALL: @InterlockedExchangeAdd@8
 * STDCALL : _InterlockedExchangeAdd@8
 */
LONG FASTCALL
InterlockedExchangeAdd(PLONG Addend,
		       LONG Value);

#ifndef USE_FASTCALL
__asm__("\n\t.global _InterlockedExchangeAdd@8\n\t"
	"_InterlockedExchangeAdd@8:\n\t"
	"movl 8(%esp),%eax\n\t"
	"movl 4(%esp),%ebx\n\t"
	"xaddl %eax,(%ebx)\n\t"
	"ret $8\n\t");
#else
__asm__("\n\t.global @InterlockedExchangeAdd@8\n\t"
	"@InterlockedExchangeAdd@8:\n\t"
	"xaddl %edx,(%ecx)\n\t"
	"movl %edx,%eax\n\t"
	"ret\n\t");
#endif


/**********************************************************************
 * FASTCALL: @InterlockedCompareExchange@12
 * STDCALL : _InterlockedCompareExchange@12
 */
PVOID FASTCALL
InterlockedCompareExchange(PVOID *Destination,
			   PVOID Exchange,
			   PVOID Comperand);

#ifndef USE_FASTCALL
__asm__("\n\t.global _InterlockedCompareExchange@12\n\t"
	"_InterlockedCompareExchange@12:\n\t"
	"movl 12(%esp),%eax\n\t"
	"movl 8(%esp),%edx\n\t"
	"movl 4(%esp),%ebx\n\t"
	"cmpxchg %edx,(%ebx)\n\t"
	"movl %edx,%eax\n\t"
	"ret $12\n\t");
#else
__asm__("\n\t.global @InterlockedCompareExchange@12\n\t"
	"@InterlockedCompareExchange@12:\n\t"
	"movl 4(%esp),%eax\n\t"
	"cmpxchg %edx,(%ecx)\n\t"
	"ret $4\n\t");
#endif

#endif /* I386_FIX */

/* EOF */
