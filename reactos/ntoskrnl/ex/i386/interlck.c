/* $Id$
 *
 * reactos/ntoskrnl/ex/i386/interlck.c
 *
 */
#include <ntoskrnl.h>

#ifdef LOCK
#undef LOCK
#endif

#if defined(__GNUC__)

#ifdef MP
#define LOCK "lock ; "
#else
#define LOCK ""
#endif

#elif defined(_MSC_VER)

#ifdef MP
#define LOCK lock
#else
#define LOCK 
#endif

#endif

#if defined(__GNUC__)

/*
 * @implemented
 */
INTERLOCKED_RESULT FASTCALL
Exfi386InterlockedIncrementLong(IN PLONG Addend);

__asm__("\n\t.global @Exfi386InterlockedIncrementLong@4\n\t"
	"@Exfi386InterlockedIncrementLong@4:\n\t"
	LOCK
	"addl $1,(%ecx)\n\t"
	"lahf\n\t"
	"andl $0xC000, %eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)

/*
 * @implemented
 */
__declspec(naked)
INTERLOCKED_RESULT FASTCALL
Exfi386InterlockedIncrementLong(IN PLONG Addend)
{
        __asm LOCK add dword ptr [ecx], 1
	__asm lahf
	__asm and eax, 0xC000
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif


#if defined(__GNUC__)

/*
 * @implemented
 */
INTERLOCKED_RESULT FASTCALL
Exfi386InterlockedDecrementLong(IN PLONG Addend);

__asm__("\n\t.global @Exfi386InterlockedDecrementLong@4\n\t"
	"@Exfi386InterlockedDecrementLong@4:\n\t"
	LOCK
	"subl $1,(%ecx)\n\t"
	"lahf\n\t"
	"andl $0xC000, %eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)

/*
 * @implemented
 */
__declspec(naked)
INTERLOCKED_RESULT FASTCALL
Exfi386InterlockedDecrementLong(IN PLONG Addend)
{
	__asm LOCK sub dword ptr [ecx], 1
	__asm lahf
	__asm and eax, 0xC000
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif


#if defined(__GNUC__)

/*
 * @implemented
 */
ULONG FASTCALL
Exfi386InterlockedExchangeUlong(IN PULONG Target,
				IN ULONG Value);

__asm__("\n\t.global @Exfi386InterlockedExchangeUlong@8\n\t"
	"@Exfi386InterlockedExchangeUlong@8:\n\t"
	LOCK
	"xchgl %edx,(%ecx)\n\t"
	"movl  %edx,%eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)

/*
 * @implemented
 */
__declspec(naked)
ULONG FASTCALL
Exfi386InterlockedExchangeUlong(IN PULONG Target,
				IN ULONG Value)
{
	__asm LOCK xchg [ecx], edx
	__asm mov  eax, edx
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif


#if defined(__GNUC__)

INTERLOCKED_RESULT STDCALL
Exi386InterlockedIncrementLong(IN PLONG Addend);

__asm__("\n\t.global _Exi386InterlockedIncrementLong@4\n\t"
	"_Exi386InterlockedIncrementLong@4:\n\t"
	"movl 4(%esp),%eax\n\t"
	LOCK
	"addl $1,(%eax)\n\t"
	"lahf\n\t"
	"andl $0xC000, %eax\n\t"
	"ret $4\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
INTERLOCKED_RESULT STDCALL
Exi386InterlockedIncrementLong(IN PLONG Addend)
{
	__asm mov eax, Addend
	__asm LOCK add dword ptr [eax], 1
	__asm lahf
	__asm and eax, 0xC000
	__asm ret 4
}

#else
#error Unknown compiler for inline assembler
#endif


#if defined(__GNUC__)

INTERLOCKED_RESULT STDCALL
Exi386InterlockedDecrementLong(IN PLONG Addend);

__asm__("\n\t.global _Exi386InterlockedDecrementLong@4\n\t"
	"_Exi386InterlockedDecrementLong@4:\n\t"
	"movl 4(%esp),%eax\n\t"
	LOCK
	"subl $1,(%eax)\n\t"
	"lahf\n\t"
	"andl $0xC000, %eax\n\t"
	"ret $4\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
INTERLOCKED_RESULT STDCALL
Exi386InterlockedDecrementLong(IN PLONG Addend)
{
	__asm mov eax, Addend
	__asm LOCK sub dword ptr [eax], 1
	__asm lahf
	__asm and eax, 0xC000
	__asm ret 4
}

#else
#error Unknown compiler for inline assembler
#endif


#if defined(__GNUC__)

ULONG STDCALL
Exi386InterlockedExchangeUlong(IN PULONG Target,
			       IN ULONG Value);

__asm__("\n\t.global _Exi386InterlockedExchangeUlong@8\n\t"
	"_Exi386InterlockedExchangeUlong@8:\n\t"
	"movl 4(%esp),%edx\n\t"
	"movl 8(%esp),%eax\n\t"
	LOCK
	"xchgl %eax,(%edx)\n\t"
	"ret $8\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
ULONG STDCALL
Exi386InterlockedExchangeUlong(IN PULONG Target,
			       IN ULONG Value)
{
	__asm mov edx, Value
	__asm mov eax, Target
	__asm LOCK xchg [edx], eax
	__asm ret 8
}

#else
#error Unknown compiler for inline assembler
#endif



/**********************************************************************
 * FASTCALL: @InterlockedIncrement@4
 * STDCALL : _InterlockedIncrement@4
 */
#if defined(__GNUC__)
/*
 * @implemented
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
	LOCK
	"xaddl %eax,(%ecx)\n\t"
	"incl %eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)
/*
 * @implemented
 */
__declspec(naked)
LONG FASTCALL
InterlockedIncrement(PLONG Addend)
{
	__asm mov eax, 1
	__asm LOCK xadd [ecx], eax
	__asm inc eax
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif


/**********************************************************************
 * FASTCALL: @InterlockedDecrement@4
 * STDCALL : _InterlockedDecrement@4
 */
#if defined(__GNUC__)
/*
 * @implemented
 */
LONG FASTCALL
InterlockedDecrement(PLONG Addend);

__asm__("\n\t.global @InterlockedDecrement@4\n\t"
	"@InterlockedDecrement@4:\n\t"
	"movl $-1,%eax\n\t"
	LOCK
	"xaddl %eax,(%ecx)\n\t"
	"decl %eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)

/*
 * @implemented
 */
__declspec(naked)
LONG FASTCALL
InterlockedDecrement(PLONG Addend)
{
	__asm mov eax, -1
	__asm LOCK xadd [ecx], eax
	__asm dec eax
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif


/**********************************************************************
 * FASTCALL: @InterlockedExchange@8
 * STDCALL : _InterlockedExchange@8
 */

#if defined(__GNUC__)
/*
 * @implemented
 */
LONG FASTCALL
InterlockedExchange(PLONG Target,
		    LONG Value);

__asm__("\n\t.global @InterlockedExchange@8\n\t"
	"@InterlockedExchange@8:\n\t"
	LOCK
	"xchgl %edx,(%ecx)\n\t"
	"movl  %edx,%eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)
/*
 * @implemented
 */
__declspec(naked)
LONG FASTCALL
InterlockedExchange(PLONG Target,
		    LONG Value)
{
	__asm LOCK xchg [ecx], edx
	__asm mov eax, edx
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif

/**********************************************************************
 * FASTCALL: @InterlockedExchangeAdd@8
 * STDCALL: _InterlockedExchangeAdd@8
 */
#if defined(__GNUC__)
/*
 * @implemented
 */
LONG FASTCALL
InterlockedExchangeAdd(PLONG Addend,
		       LONG Value);

__asm__("\n\t.global @InterlockedExchangeAdd@8\n\t"
	"@InterlockedExchangeAdd@8:\n\t"
	LOCK
	"xaddl %edx,(%ecx)\n\t"
	"movl %edx,%eax\n\t"
	"ret\n\t");

#elif defined(_MSC_VER)
/*
 * @implemented
 */
__declspec(naked)
LONG FASTCALL
InterlockedExchangeAdd(PLONG Addend,
		       LONG Value)
{
	__asm LOCK xadd [ecx], edx
	__asm mov eax, edx
	__asm ret
}

#else
#error Unknown compiler for inline assembler
#endif


/**********************************************************************
 * FASTCALL: @InterlockedCompareExchange@12
 * STDCALL: _InterlockedCompareExchange@12
 */
#if defined(__GNUC__)

LONG FASTCALL
InterlockedCompareExchange(PLONG Destination,
			   LONG Exchange,
			   LONG Comperand);

__asm__("\n\t.global @InterlockedCompareExchange@12\n\t"
	"@InterlockedCompareExchange@12:\n\t"
	"movl 4(%esp),%eax\n\t"
	LOCK
	"cmpxchg %edx,(%ecx)\n\t"
	"ret $4\n\t");

#elif defined(_MSC_VER)

__declspec(naked)
LONG FASTCALL
InterlockedCompareExchange(PLONG Destination,
			   LONG Exchange,
			   LONG Comperand)
{
	__asm mov eax, Comperand
	__asm LOCK cmpxchg [ecx], edx
	__asm ret 4
}

#else
#error Unknown compiler for inline assembler
#endif

/**********************************************************************
 * FASTCALL: @InterlockedCompareExchange64@8
 */
#if defined(__GNUC__)
LONGLONG FASTCALL
ExfpInterlockedExchange64(LONGLONG volatile * Destination,
                         PLONGLONG Exchange);

__asm__("\n\t.global @ExfpInterlockedExchange64@8\n\t"
	"@ExfpInterlockedExchange64@8:\n\t"
	"pushl %ebx\n\t"
	"pushl %esi\n\t"
	"movl %ecx,%esi\n\t"
	"movl (%edx),%ebx\n\t"
	"movl 4(%edx),%ecx\n\t"
	"\n1:\t"
	"movl (%esi),%eax\n\t"
	"movl 4(%esi),%edx\n\t"
	LOCK
	"cmpxchg8b (%esi)\n\t"
	"jnz 1b\n\t"
	"popl %esi\n\t"
	"popl %ebx\n\t"
	"ret\n\t");

#else
#error Unknown compiler for inline assembler
#endif

/**********************************************************************
 * FASTCALL: @ExfInterlockedCompareExchange@12
 */
#if defined(__GNUC__)
LONGLONG FASTCALL
ExfInterlockedCompareExchange64(LONGLONG volatile * Destination,
                                PLONGLONG Exchange,
				PLONGLONG Comperand);

__asm__("\n\t.global @ExfInterlockedCompareExchange64@12\n\t"
	"@ExfInterlockedCompareExchange64@12:\n\t"
	"pushl %ebx\n\t"
	"pushl %esi\n\t"
	"movl %ecx,%esi\n\t"
	"movl (%edx),%ebx\n\t"
	"movl 4(%edx),%ecx\n\t"
	"movl 12(%esp),%edx\n\t"
	"movl (%edx),%eax\n\t"
	"movl 4(%edx),%edx\n\t"
	LOCK
	"cmpxchg8b (%esi)\n\t"
	"popl %esi\n\t"
	"popl %ebx\n\t"
	"ret  $4\n\t");

#else
#error Unknown compiler for inline assembler
#endif

/* EOF */
