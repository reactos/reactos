/* $Id: interlck.c,v 1.6 1999/12/11 21:14:48 dwelch Exp $
 *
 * reactos/ntoskrnl/rtl/interlck.c
 *
 * FIXME: change decorated names when __fastcall will be available
 * (for both egcs 1.1.2 and gcc 2.95 FASTCALL == STDCALL).
 */
#include <reactos/config.h>
#include <ntos.h>
#include <internal/debug.h>

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

/**********************************************************************
 * FASTCALL: @InterlockedIncrement@0
 * STDCALL : _InterlockedIncrement@4
 */
#if 1
LONG FASTCALL InterlockedIncrement (PLONG Addend);
/*
 * FUNCTION: Increments a caller supplied variable of type LONG as an 
 * atomic operation
 * ARGUMENTS;
 *     Addend = Points to a variable whose value is to be increment
 * RETURNS: The incremented value
 */
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
#endif

#if 0
/*
__asm__(
#ifndef CONFIG_USE_FASTCALL 
	".global _InterlockedIncrement@4\n"
	"\t_InterlockedIncrement@4:\n"
	"\tmovl 4(%esp), %ecx\n"
#else
	".global @InterlockedIncrement@0\n"
	"\t@InterlockedIncrement@0:\n"
#endif
	"\tmov  $1, %eax\n"
	"\txadd %ecx, %eax\n"
	"\tinc  %eax\n\n"
#ifndef CONFIG_USE_FASTCALL 
	"\tret  $4\n"
#endif
	);
*/
#endif       
       
/**********************************************************************
 * FASTCALL: @InterlockedDecrement@0
 * STDCALL : _InterlockedDecrement@4
 */
#if 1
LONG FASTCALL InterlockedDecrement(PLONG Addend);
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
#endif
 
/**********************************************************************
 * FASTCALL: @InterlockedExchange@0
 * STDCALL : _InterlockedExchange@8
 */
LONG
FASTCALL
InterlockedExchange (
	PLONG	Target,
	LONG	Value
	);
__asm__(
	"\n\t.global _InterlockedExchange@8\n\t"
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
	"ret $8\n\t"
	);
/*
__asm__(
#ifndef CONFIG_USE_FASTCALL 
	".global _InterlockedExchange@8\n"
	"_InterlockedExchange@8:\n"
	"\tmovl 4(%esp), %ecx\n"
	"\tmovl 8(%esp), %edx\n"
#else
	".global @InterlockedExchange@0\n"
	"@InterlockedExchange@0:\n"
#endif
	"\tmovl %ecx, %eax\n"
	"__InterlockedExchange_Loop:\n"
	"\tlock\n"
	"\tcmpxchg %ecx, %edx\n"
	"\tjne __InterlockedExchange_Loop\n"
#ifndef CONFIG_USE_FASTCALL 
	"\tmovl %ecx, 4(%esp)\n"
	"\tret $8\n"
#else
	"\tret\n"
#endif
	);
*/


/**********************************************************************
 * FASTCALL: @InterlockedExchangeAdd@0
 * STDCALL : _InterlockedExchangeAdd@8
 */
LONG
FASTCALL
InterlockedExchangeAdd (
	PLONG	Addend,
	LONG	Value
	);
__asm__(
	"\n\t.global _InterlockedExchangeAdd@8\n\t"
	"_InterlockedExchangeAdd@8:\n\t"
	"movl 8(%esp),%eax\n\t"
	"movl 4(%esp),%ebx\n\t"
	"xaddl %eax,(%ebx)\n\t"
	"ret $8\n\t"
       );
/*
__asm__(
#ifndef CONFIG_USE_FASTCALL 
	".global _InterlockedExchangeAdd@8\n"
	"\t_InterlockedExchangeAdd@8:\n"
	"\tmovl 4(%esp), %ecx\n"
	"\tmovl 8(%esp), %edx\n"
#else
	".global @InterlockedExchangeAdd@0\n"
	"\t@InterlockedExchangeAdd@0:\n"
#endif
	"\txadd %edx, %ecx\n"
	"\tmovl %edx, %eax\n"
#ifndef CONFIG_USE_FASTCALL 
	"\tret  $8\n"
#else
	"\tret\n"
#endif
	);
*/


/**********************************************************************
 * FASTCALL: @InterlockedCompareExchange@4
 * STDCALL : _InterlockedCompareExchange@12
 */
PVOID
FASTCALL
InterlockedCompareExchange (
	PVOID	* Destination,
	PVOID	Exchange,
	PVOID	Comperand
	);
__asm__(
	"\n\t.global _InterlockedCompareExchange@12\n\t"
	"_InterlockedCompareExchange@12:\n\t"
	"movl 12(%esp),%eax\n\t"
	"movl 8(%esp),%edx\n\t"
	"movl 4(%esp),%ebx\n\t"
	"cmpxchg %edx,(%ebx)\n\t"
	"movl %edx,%eax\n\t"
	"ret $12\n\t"
	);
/*
__asm__(
#ifndef CONFIG_USE_FASTCALL 
	".global _InterlockedCompareExchange@12\n"
	"\t_InterlockedCompareExchange@12:\n"
	"\tmovl 4(%esp), %ecx\n"
	"\tmovl 8(%esp), %edx\n"
	"\tmovl 12(%esp), %eax\n"
#else
	".global @InterlockedCompareExchange@4\n"
	"\t@InterlockedCompareExchange@4:\n"
	"\tmovl 4(%esp), %eax\n"
#endif
	"\tcmpxchg %ecx, %edx\n"
#ifndef CONFIG_USE_FASTCALL 
	"\tret $12\n"
#else
	"\tret $4\n"
#endif
*/


/* EOF */
