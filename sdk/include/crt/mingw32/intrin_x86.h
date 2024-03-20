/*
	Compatibility <intrin_x86.h> header for GCC -- GCC equivalents of intrinsic
	Microsoft Visual C++ functions. Originally developed for the ReactOS
	(<https://reactos.org/>) and TinyKrnl (<http://www.tinykrnl.org/>)
	projects.

	Copyright (c) 2006 KJK::Hyperion <hackbunny@reactos.com>

	Permission is hereby granted, free of charge, to any person obtaining a
	copy of this software and associated documentation files (the "Software"),
	to deal in the Software without restriction, including without limitation
	the rights to use, copy, modify, merge, publish, distribute, sublicense,
	and/or sell copies of the Software, and to permit persons to whom the
	Software is furnished to do so, subject to the following conditions:

	The above copyright notice and this permission notice shall be included in
	all copies or substantial portions of the Software.

	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
	DEALINGS IN THE SOFTWARE.
*/

#ifndef KJK_INTRIN_X86_H_
#define KJK_INTRIN_X86_H_

/*
	FIXME: review all "memory" clobbers, add/remove to match Visual C++
	behavior: some "obvious" memory barriers are not present in the Visual C++
	implementation - e.g. __stosX; on the other hand, some memory barriers that
	*are* present could have been missed
*/

/*
	NOTE: this is a *compatibility* header. Some functions may look wrong at
	first, but they're only "as wrong" as they would be on Visual C++. Our
	priority is compatibility

	NOTE: unlike most people who write inline asm for GCC, I didn't pull the
	constraints and the uses of __volatile__ out of my... hat. Do not touch
	them. I hate cargo cult programming

	NOTE: be very careful with declaring "memory" clobbers. Some "obvious"
	barriers aren't there in Visual C++ (e.g. __stosX)

	NOTE: review all intrinsics with a return value, add/remove __volatile__
	where necessary. If an intrinsic whose value is ignored generates a no-op
	under Visual C++, __volatile__ must be omitted; if it always generates code
	(for example, if it has side effects), __volatile__ must be specified. GCC
	will only optimize out non-volatile asm blocks with outputs, so input-only
	blocks are safe. Oddities such as the non-volatile 'rdmsr' are intentional
	and follow Visual C++ behavior

	NOTE: on GCC 4.1.0, please use the __sync_* built-ins for barriers and
	atomic operations. Test the version like this:

	#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100
		...

	Pay attention to the type of barrier. Make it match with what Visual C++
	would use in the same case
*/

#ifdef __cplusplus
extern "C" {
#endif

/*** memcopy must be memmove ***/
void* __cdecl memmove(void* dest, const void* source, size_t num);
__INTRIN_INLINE void* __cdecl memcpy(void* dest, const void* source, size_t num)
{
    return memmove(dest, source, num);
}


/*** Stack frame juggling ***/
#define _ReturnAddress() (__builtin_return_address(0))
#define _AddressOfReturnAddress() (&(((void **)(__builtin_frame_address(0)))[1]))
/* TODO: __getcallerseflags but how??? */

/*** Memory barriers ***/

#if !HAS_BUILTIN(_ReadWriteBarrier)
__INTRIN_INLINE void _ReadWriteBarrier(void)
{
	__asm__ __volatile__("" : : : "memory");
}
#endif

/* GCC only supports full barriers */
#define _ReadBarrier _ReadWriteBarrier
#define _WriteBarrier _ReadWriteBarrier

#if !HAS_BUILTIN(_mm_mfence)
__INTRIN_INLINE void _mm_mfence(void)
{
	__asm__ __volatile__("mfence" : : : "memory");
}
#endif

#if !HAS_BUILTIN(_mm_lfence)
__INTRIN_INLINE void _mm_lfence(void)
{
	_ReadBarrier();
	__asm__ __volatile__("lfence");
	_ReadBarrier();
}
#endif

#if defined(__x86_64__) && !HAS_BUILTIN(__faststorefence)
__INTRIN_INLINE void __faststorefence(void)
{
	long local;
	__asm__ __volatile__("lock; orl $0, %0;" : : "m"(local));
}
#endif


/*** Atomic operations ***/

#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100

#if !HAS_BUILTIN(_InterlockedCompareExchange8)
__INTRIN_INLINE char _InterlockedCompareExchange8(volatile char * Destination, char Exchange, char Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}
#endif

#if !HAS_BUILTIN(_InterlockedCompareExchange16)
__INTRIN_INLINE short _InterlockedCompareExchange16(volatile short * Destination, short Exchange, short Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}
#endif

#if !HAS_BUILTIN(_InterlockedCompareExchange)
__INTRIN_INLINE long __cdecl _InterlockedCompareExchange(volatile long * Destination, long Exchange, long Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}
#endif

#if !HAS_BUILTIN(_InterlockedCompareExchangePointer)
__INTRIN_INLINE void * _InterlockedCompareExchangePointer(void * volatile * Destination, void * Exchange, void * Comperand)
{
	return (void *)__sync_val_compare_and_swap(Destination, Comperand, Exchange);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange8)
__INTRIN_INLINE char _InterlockedExchange8(volatile char * Target, char Value)
{
	/* NOTE: __sync_lock_test_and_set would be an acquire barrier, so we force a full barrier */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange16)
__INTRIN_INLINE short _InterlockedExchange16(volatile short * Target, short Value)
{
	/* NOTE: __sync_lock_test_and_set would be an acquire barrier, so we force a full barrier */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange)
__INTRIN_INLINE long __cdecl _InterlockedExchange(volatile long * Target, long Value)
{
	/* NOTE: __sync_lock_test_and_set would be an acquire barrier, so we force a full barrier */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangePointer)
__INTRIN_INLINE void * _InterlockedExchangePointer(void * volatile * Target, void * Value)
{
	/* NOTE: __sync_lock_test_and_set would be an acquire barrier, so we force a full barrier */
	__sync_synchronize();
	return (void *)__sync_lock_test_and_set(Target, Value);
}
#endif

#if defined(__x86_64__) && !HAS_BUILTIN(_InterlockedExchange64)
__INTRIN_INLINE long long _InterlockedExchange64(volatile long long * Target, long long Value)
{
	/* NOTE: __sync_lock_test_and_set would be an acquire barrier, so we force a full barrier */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangeAdd8)
__INTRIN_INLINE char _InterlockedExchangeAdd8(char volatile * Addend, char Value)
{
	return __sync_fetch_and_add(Addend, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangeAdd16)
__INTRIN_INLINE short _InterlockedExchangeAdd16(volatile short * Addend, short Value)
{
	return __sync_fetch_and_add(Addend, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangeAdd)
__INTRIN_INLINE long __cdecl _InterlockedExchangeAdd(volatile long * Addend, long Value)
{
	return __sync_fetch_and_add(Addend, Value);
}
#endif

#if defined(__x86_64__) && !HAS_BUILTIN(_InterlockedExchangeAdd64)
__INTRIN_INLINE long long _InterlockedExchangeAdd64(volatile long long * Addend, long long Value)
{
	return __sync_fetch_and_add(Addend, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedAnd8)
__INTRIN_INLINE char _InterlockedAnd8(volatile char * value, char mask)
{
	return __sync_fetch_and_and(value, mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedAnd16)
__INTRIN_INLINE short _InterlockedAnd16(volatile short * value, short mask)
{
	return __sync_fetch_and_and(value, mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedAnd)
__INTRIN_INLINE long _InterlockedAnd(volatile long * value, long mask)
{
	return __sync_fetch_and_and(value, mask);
}
#endif

#if defined(__x86_64__) && !HAS_BUILTIN(_InterlockedAnd64)
__INTRIN_INLINE long long _InterlockedAnd64(volatile long long * value, long long mask)
{
	return __sync_fetch_and_and(value, mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedOr8)
__INTRIN_INLINE char _InterlockedOr8(volatile char * value, char mask)
{
	return __sync_fetch_and_or(value, mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedOr16)
__INTRIN_INLINE short _InterlockedOr16(volatile short * value, short mask)
{
	return __sync_fetch_and_or(value, mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedOr)
__INTRIN_INLINE long _InterlockedOr(volatile long * value, long mask)
{
	return __sync_fetch_and_or(value, mask);
}
#endif

#if defined(__x86_64__) && !HAS_BUILTIN(_InterlockedOr64)
__INTRIN_INLINE long long _InterlockedOr64(volatile long long * value, long long mask)
{
	return __sync_fetch_and_or(value, mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedXor8)
__INTRIN_INLINE char _InterlockedXor8(volatile char * value, char mask)
{
	return __sync_fetch_and_xor(value, mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedXor16)
__INTRIN_INLINE short _InterlockedXor16(volatile short * value, short mask)
{
	return __sync_fetch_and_xor(value, mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedXor)
__INTRIN_INLINE long _InterlockedXor(volatile long * value, long mask)
{
	return __sync_fetch_and_xor(value, mask);
}
#endif

#if defined(__x86_64__) && !HAS_BUILTIN(_InterlockedXor64)
__INTRIN_INLINE long long _InterlockedXor64(volatile long long * value, long long mask)
{
	return __sync_fetch_and_xor(value, mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedDecrement)
__INTRIN_INLINE long __cdecl _InterlockedDecrement(volatile long * lpAddend)
{
	return __sync_sub_and_fetch(lpAddend, 1);
}
#endif

#if !HAS_BUILTIN(_InterlockedIncrement)
__INTRIN_INLINE long __cdecl _InterlockedIncrement(volatile long * lpAddend)
{
	return __sync_add_and_fetch(lpAddend, 1);
}
#endif

#if !HAS_BUILTIN(_InterlockedDecrement16)
__INTRIN_INLINE short _InterlockedDecrement16(volatile short * lpAddend)
{
	return __sync_sub_and_fetch(lpAddend, 1);
}
#endif

#if !HAS_BUILTIN(_InterlockedIncrement16)
__INTRIN_INLINE short _InterlockedIncrement16(volatile short * lpAddend)
{
	return __sync_add_and_fetch(lpAddend, 1);
}
#endif

#if defined(__x86_64__)
#if !HAS_BUILTIN(_InterlockedDecrement64)
__INTRIN_INLINE long long _InterlockedDecrement64(volatile long long * lpAddend)
{
	return __sync_sub_and_fetch(lpAddend, 1);
}
#endif

#if !HAS_BUILTIN(_InterlockedIncrement64)
__INTRIN_INLINE long long _InterlockedIncrement64(volatile long long * lpAddend)
{
	return __sync_add_and_fetch(lpAddend, 1);
}
#endif
#endif

#else /* (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100 */

#if !HAS_BUILTIN(_InterlockedCompareExchange8)
__INTRIN_INLINE char _InterlockedCompareExchange8(volatile char * Destination, char Exchange, char Comperand)
{
	char retval = Comperand;
	__asm__("lock; cmpxchgb %b[Exchange], %[Destination]" : [retval] "+a" (retval) : [Destination] "m" (*Destination), [Exchange] "q" (Exchange) : "memory");
	return retval;
}
#endif

#if !HAS_BUILTIN(_InterlockedCompareExchange16)
__INTRIN_INLINE short _InterlockedCompareExchange16(volatile short * Destination, short Exchange, short Comperand)
{
	short retval = Comperand;
	__asm__("lock; cmpxchgw %w[Exchange], %[Destination]" : [retval] "+a" (retval) : [Destination] "m" (*Destination), [Exchange] "q" (Exchange): "memory");
	return retval;
}
#endif

#if !HAS_BUILTIN(_InterlockedCompareExchange)
__INTRIN_INLINE long _InterlockedCompareExchange(volatile long * Destination, long Exchange, long Comperand)
{
	long retval = Comperand;
	__asm__("lock; cmpxchgl %k[Exchange], %[Destination]" : [retval] "+a" (retval) : [Destination] "m" (*Destination), [Exchange] "q" (Exchange): "memory");
	return retval;
}
#endif

#if !HAS_BUILTIN(_InterlockedCompareExchangePointer)
__INTRIN_INLINE void * _InterlockedCompareExchangePointer(void * volatile * Destination, void * Exchange, void * Comperand)
{
	void * retval = (void *)Comperand;
	__asm__("lock; cmpxchgl %k[Exchange], %[Destination]" : [retval] "=a" (retval) : "[retval]" (retval), [Destination] "m" (*Destination), [Exchange] "q" (Exchange) : "memory");
	return retval;
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange8)
__INTRIN_INLINE char _InterlockedExchange8(volatile char * Target, char Value)
{
	char retval = Value;
	__asm__("xchgb %[retval], %[Target]" : [retval] "+r" (retval) : [Target] "m" (*Target) : "memory");
	return retval;
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange16)
__INTRIN_INLINE short _InterlockedExchange16(volatile short * Target, short Value)
{
	short retval = Value;
	__asm__("xchgw %[retval], %[Target]" : [retval] "+r" (retval) : [Target] "m" (*Target) : "memory");
	return retval;
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange)
__INTRIN_INLINE long _InterlockedExchange(volatile long * Target, long Value)
{
	long retval = Value;
	__asm__("xchgl %[retval], %[Target]" : [retval] "+r" (retval) : [Target] "m" (*Target) : "memory");
	return retval;
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangePointer)
__INTRIN_INLINE void * _InterlockedExchangePointer(void * volatile * Target, void * Value)
{
	void * retval = Value;
	__asm__("xchgl %[retval], %[Target]" : [retval] "+r" (retval) : [Target] "m" (*Target) : "memory");
	return retval;
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangeAdd8)
__INTRIN_INLINE char _InterlockedExchangeAdd8(char volatile * Addend, char Value)
{
	char retval = Value;
	__asm__("lock; xaddb %[retval], %[Addend]" : [retval] "+r" (retval) : [Addend] "m" (*Addend) : "memory");
	return retval;
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangeAdd16)
__INTRIN_INLINE short _InterlockedExchangeAdd16(volatile short * Addend, short Value)
{
	short retval = Value;
	__asm__("lock; xaddw %[retval], %[Addend]" : [retval] "+r" (retval) : [Addend] "m" (*Addend) : "memory");
	return retval;
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangeAdd)
__INTRIN_INLINE long _InterlockedExchangeAdd(volatile long * Addend, long Value)
{
	long retval = Value;
	__asm__("lock; xaddl %[retval], %[Addend]" : [retval] "+r" (retval) : [Addend] "m" (*Addend) : "memory");
	return retval;
}
#endif

#if !HAS_BUILTIN(_InterlockedAnd8)
__INTRIN_INLINE char _InterlockedAnd8(volatile char * value, char mask)
{
	char x;
	char y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange8(value, x & mask, x);
	}
	while(y != x);

	return y;
}
#endif

#if !HAS_BUILTIN(_InterlockedAnd16)
__INTRIN_INLINE short _InterlockedAnd16(volatile short * value, short mask)
{
	short x;
	short y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange16(value, x & mask, x);
	}
	while(y != x);

	return y;
}
#endif

#if !HAS_BUILTIN(_InterlockedAnd)
__INTRIN_INLINE long _InterlockedAnd(volatile long * value, long mask)
{
	long x;
	long y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange(value, x & mask, x);
	}
	while(y != x);

	return y;
}
#endif

#if !HAS_BUILTIN(_InterlockedOr8)
__INTRIN_INLINE char _InterlockedOr8(volatile char * value, char mask)
{
	char x;
	char y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange8(value, x | mask, x);
	}
	while(y != x);

	return y;
}
#endif

#if !HAS_BUILTIN(_InterlockedOr16)
__INTRIN_INLINE short _InterlockedOr16(volatile short * value, short mask)
{
	short x;
	short y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange16(value, x | mask, x);
	}
	while(y != x);

	return y;
}
#endif

#if !HAS_BUILTIN(_InterlockedOr)
__INTRIN_INLINE long _InterlockedOr(volatile long * value, long mask)
{
	long x;
	long y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange(value, x | mask, x);
	}
	while(y != x);

	return y;
}
#endif

#if !HAS_BUILTIN(_InterlockedXor8)
__INTRIN_INLINE char _InterlockedXor8(volatile char * value, char mask)
{
	char x;
	char y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange8(value, x ^ mask, x);
	}
	while(y != x);

	return y;
}
#endif

#if !HAS_BUILTIN(_InterlockedXor16)
__INTRIN_INLINE short _InterlockedXor16(volatile short * value, short mask)
{
	short x;
	short y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange16(value, x ^ mask, x);
	}
	while(y != x);

	return y;
}
#endif

#if !HAS_BUILTIN(_InterlockedXor)
__INTRIN_INLINE long _InterlockedXor(volatile long * value, long mask)
{
	long x;
	long y;

	y = *value;

	do
	{
		x = y;
		y = _InterlockedCompareExchange(value, x ^ mask, x);
	}
	while(y != x);

	return y;
}
#endif

#if !HAS_BUILTIN(_InterlockedDecrement)
__INTRIN_INLINE long _InterlockedDecrement(volatile long * lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, -1) - 1;
}
#endif

#if !HAS_BUILTIN(_InterlockedIncrement)
__INTRIN_INLINE long _InterlockedIncrement(volatile long * lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, 1) + 1;
}
#endif

#if !HAS_BUILTIN(_InterlockedDecrement16)
__INTRIN_INLINE short _InterlockedDecrement16(volatile short * lpAddend)
{
	return _InterlockedExchangeAdd16(lpAddend, -1) - 1;
}
#endif

#if !HAS_BUILTIN(_InterlockedIncrement16)
__INTRIN_INLINE short _InterlockedIncrement16(volatile short * lpAddend)
{
	return _InterlockedExchangeAdd16(lpAddend, 1) + 1;
}
#endif

#if defined(__x86_64__)
#if !HAS_BUILTIN(_InterlockedDecrement64)
__INTRIN_INLINE long long _InterlockedDecrement64(volatile long long * lpAddend)
{
	return _InterlockedExchangeAdd64(lpAddend, -1) - 1;
}
#endif

#if !HAS_BUILTIN(_InterlockedIncrement64)
__INTRIN_INLINE long long _InterlockedIncrement64(volatile long long * lpAddend)
{
	return _InterlockedExchangeAdd64(lpAddend, 1) + 1;
}
#endif
#endif

#endif /* (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100 */

#if !HAS_BUILTIN(_InterlockedCompareExchange64)
#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100 && defined(__x86_64__)

__INTRIN_INLINE long long _InterlockedCompareExchange64(volatile long long * Destination, long long Exchange, long long Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

#else /* (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100 && defined(__x86_64__) */
__INTRIN_INLINE long long _InterlockedCompareExchange64(volatile long long * Destination, long long Exchange, long long Comperand)
{
	long long retval = Comperand;

	__asm__
	(
		"lock; cmpxchg8b %[Destination]" :
		[retval] "+A" (retval) :
			[Destination] "m" (*Destination),
			"b" ((unsigned long)((Exchange >>  0) & 0xFFFFFFFF)),
			"c" ((unsigned long)((Exchange >> 32) & 0xFFFFFFFF)) :
		"memory"
	);

	return retval;
}
#endif /* (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100 && defined(__x86_64__) */
#endif /* !HAS_BUILTIN(_InterlockedCompareExchange64) */

#if defined(__x86_64__) && !HAS_BUILTIN(_InterlockedCompareExchange128)
__INTRIN_INLINE unsigned char _InterlockedCompareExchange128(_Interlocked_operand_ __int64 volatile* Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64* ComparandResult)
{
    __int64 xchg[2] = { ExchangeLow, ExchangeHigh };
    return __sync_bool_compare_and_swap((__uint128_t*)Destination, *((__uint128_t*)ComparandResult), *((__uint128_t*)xchg));
}
#endif

#ifdef __i386__
__INTRIN_INLINE long _InterlockedAddLargeStatistic(volatile long long * Addend, long Value)
{
	__asm__
	(
		"lock; addl %[Value], %[Lo32];"
		"jae LABEL%=;"
		"lock; adcl $0, %[Hi32];"
		"LABEL%=:;" :
		[Lo32] "+m" (*((volatile long *)(Addend) + 0)), [Hi32] "+m" (*((volatile long *)(Addend) + 1)) :
		[Value] "ir" (Value) :
		"memory"
	);

	return Value;
}
#endif /* __i386__ */

#if !HAS_BUILTIN(_interlockedbittestandreset)
__INTRIN_INLINE unsigned char _interlockedbittestandreset(volatile long * a, long b)
{
	unsigned char retval;
	__asm__("lock; btrl %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval), [a] "+m" (*a) : [b] "Ir" (b) : "memory");
	return retval;
}
#endif

#if defined(__x86_64__) && !HAS_BUILTIN(_interlockedbittestandreset64)
__INTRIN_INLINE unsigned char _interlockedbittestandreset64(volatile long long * a, long long b)
{
	unsigned char retval;
	__asm__("lock; btrq %[b], %[a]; setb %b[retval]" : [retval] "=r" (retval), [a] "+m" (*a) : [b] "Ir" (b) : "memory");
	return retval;
}

#endif

#if !HAS_BUILTIN(_interlockedbittestandset)
__INTRIN_INLINE unsigned char _interlockedbittestandset(volatile long * a, long b)
{
	unsigned char retval;
	__asm__("lock; btsl %[b], %[a]; setc %b[retval]" : [retval] "=q" (retval), [a] "+m" (*a) : [b] "Ir" (b) : "memory");
	return retval;
}
#endif

#if defined(__x86_64__) && !HAS_BUILTIN(_interlockedbittestandset64)
__INTRIN_INLINE unsigned char _interlockedbittestandset64(volatile long long * a, long long b)
{
	unsigned char retval;
	__asm__("lock; btsq %[b], %[a]; setc %b[retval]" : [retval] "=r" (retval), [a] "+m" (*a) : [b] "Ir" (b) : "memory");
	return retval;
}
#endif

/*** String operations ***/

#if !HAS_BUILTIN(__stosb)
/* NOTE: we don't set a memory clobber in the __stosX functions because Visual C++ doesn't */
__INTRIN_INLINE void __stosb(unsigned char * Dest, unsigned char Data, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; stosb" :
		[Dest] "=D" (Dest), [Count] "=c" (Count) :
		"[Dest]" (Dest), "a" (Data), "[Count]" (Count)
	);
}
#endif

__INTRIN_INLINE void __stosw(unsigned short * Dest, unsigned short Data, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; stosw" :
		[Dest] "=D" (Dest), [Count] "=c" (Count) :
		"[Dest]" (Dest), "a" (Data), "[Count]" (Count)
	);
}

__INTRIN_INLINE void __stosd(unsigned long * Dest, unsigned long Data, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; stosl" :
		[Dest] "=D" (Dest), [Count] "=c" (Count) :
		"[Dest]" (Dest), "a" (Data), "[Count]" (Count)
	);
}

#ifdef __x86_64__
__INTRIN_INLINE void __stosq(unsigned long long * Dest, unsigned long long Data, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; stosq" :
		[Dest] "=D" (Dest), [Count] "=c" (Count) :
		"[Dest]" (Dest), "a" (Data), "[Count]" (Count)
	);
}
#endif

__INTRIN_INLINE void __movsb(unsigned char * Destination, const unsigned char * Source, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; movsb" :
		[Destination] "=D" (Destination), [Source] "=S" (Source), [Count] "=c" (Count) :
		"[Destination]" (Destination), "[Source]" (Source), "[Count]" (Count)
	);
}

__INTRIN_INLINE void __movsw(unsigned short * Destination, const unsigned short * Source, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; movsw" :
		[Destination] "=D" (Destination), [Source] "=S" (Source), [Count] "=c" (Count) :
		"[Destination]" (Destination), "[Source]" (Source), "[Count]" (Count)
	);
}

__INTRIN_INLINE void __movsd(unsigned long * Destination, const unsigned long * Source, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; movsl" :
		[Destination] "=D" (Destination), [Source] "=S" (Source), [Count] "=c" (Count) :
		"[Destination]" (Destination), "[Source]" (Source), "[Count]" (Count)
	);
}

#ifdef __x86_64__
__INTRIN_INLINE void __movsq(unsigned long long * Destination, const unsigned long long * Source, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; movsq" :
		[Destination] "=D" (Destination), [Source] "=S" (Source), [Count] "=c" (Count) :
		"[Destination]" (Destination), "[Source]" (Source), "[Count]" (Count)
	);
}
#endif

#if defined(__x86_64__)

/*** GS segment addressing ***/

__INTRIN_INLINE void __writegsbyte(unsigned long Offset, unsigned char Data)
{
	__asm__ __volatile__("movb %b[Data], %%gs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "ir" (Data) : "memory");
}

__INTRIN_INLINE void __writegsword(unsigned long Offset, unsigned short Data)
{
	__asm__ __volatile__("movw %w[Data], %%gs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "ir" (Data) : "memory");
}

__INTRIN_INLINE void __writegsdword(unsigned long Offset, unsigned long Data)
{
	__asm__ __volatile__("movl %k[Data], %%gs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "ir" (Data) : "memory");
}

__INTRIN_INLINE void __writegsqword(unsigned long Offset, unsigned long long Data)
{
	__asm__ __volatile__("movq %q[Data], %%gs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "ir" (Data) : "memory");
}

#if !HAS_BUILTIN(__readgsbyte)
__INTRIN_INLINE unsigned char __readgsbyte(unsigned long Offset)
{
	unsigned char value;
	__asm__ __volatile__("movb %%gs:%a[Offset], %b[value]" : [value] "=r" (value) : [Offset] "ir" (Offset));
	return value;
}
#endif

#if !HAS_BUILTIN(__readgsword)
__INTRIN_INLINE unsigned short __readgsword(unsigned long Offset)
{
	unsigned short value;
	__asm__ __volatile__("movw %%gs:%a[Offset], %w[value]" : [value] "=r" (value) : [Offset] "ir" (Offset));
	return value;
}
#endif

#if !HAS_BUILTIN(__readgsdword)
__INTRIN_INLINE unsigned long __readgsdword(unsigned long Offset)
{
	unsigned long value;
	__asm__ __volatile__("movl %%gs:%a[Offset], %k[value]" : [value] "=r" (value) : [Offset] "ir" (Offset));
	return value;
}
#endif

#if !HAS_BUILTIN(__readgsqword)
__INTRIN_INLINE unsigned long long __readgsqword(unsigned long Offset)
{
	unsigned long long value;
	__asm__ __volatile__("movq %%gs:%a[Offset], %q[value]" : [value] "=r" (value) : [Offset] "ir" (Offset));
	return value;
}
#endif

__INTRIN_INLINE void __incgsbyte(unsigned long Offset)
{
	__asm__ __volatile__("incb %%gs:%a[Offset]" : : [Offset] "ir" (Offset) : "memory");
}

__INTRIN_INLINE void __incgsword(unsigned long Offset)
{
	__asm__ __volatile__("incw %%gs:%a[Offset]" : : [Offset] "ir" (Offset) : "memory");
}

__INTRIN_INLINE void __incgsdword(unsigned long Offset)
{
	__asm__ __volatile__("incl %%gs:%a[Offset]" : : [Offset] "ir" (Offset) : "memory");
}

__INTRIN_INLINE void __incgsqword(unsigned long Offset)
{
	__asm__ __volatile__("incq %%gs:%a[Offset]" : : [Offset] "ir" (Offset) : "memory");
}

__INTRIN_INLINE void __addgsbyte(unsigned long Offset, unsigned char Data)
{
	__asm__ __volatile__("addb %b[Data], %%gs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "ir" (Data) : "memory");
}

__INTRIN_INLINE void __addgsword(unsigned long Offset, unsigned short Data)
{
	__asm__ __volatile__("addw %w[Data], %%gs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "ir" (Data) : "memory");
}

__INTRIN_INLINE void __addgsdword(unsigned long Offset, unsigned long Data)
{
	__asm__ __volatile__("addl %k[Data], %%gs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "ir" (Data) : "memory");
}

__INTRIN_INLINE void __addgsqword(unsigned long Offset, unsigned long long Data)
{
	__asm__ __volatile__("addq %k[Data], %%gs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "ir" (Data) : "memory");
}

#else /* defined(__x86_64__) */

/*** FS segment addressing ***/

__INTRIN_INLINE void __writefsbyte(unsigned long Offset, unsigned char Data)
{
	__asm__ __volatile__("movb %b[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data) : "memory");
}

__INTRIN_INLINE void __writefsword(unsigned long Offset, unsigned short Data)
{
	__asm__ __volatile__("movw %w[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "ir" (Data) : "memory");
}

__INTRIN_INLINE void __writefsdword(unsigned long Offset, unsigned long Data)
{
	__asm__ __volatile__("movl %k[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "ir" (Data) : "memory");
}

#if !HAS_BUILTIN(__readfsbyte)
__INTRIN_INLINE unsigned char __readfsbyte(unsigned long Offset)
{
	unsigned char value;
	__asm__ __volatile__("movb %%fs:%a[Offset], %b[value]" : [value] "=q" (value) : [Offset] "ir" (Offset));
	return value;
}
#endif

#if !HAS_BUILTIN(__readfsword)
__INTRIN_INLINE unsigned short __readfsword(unsigned long Offset)
{
	unsigned short value;
	__asm__ __volatile__("movw %%fs:%a[Offset], %w[value]" : [value] "=r" (value) : [Offset] "ir" (Offset));
	return value;
}
#endif

#if !HAS_BUILTIN(__readfsdword)
__INTRIN_INLINE unsigned long __readfsdword(unsigned long Offset)
{
	unsigned long value;
	__asm__ __volatile__("movl %%fs:%a[Offset], %k[value]" : [value] "=r" (value) : [Offset] "ir" (Offset));
	return value;
}
#endif

__INTRIN_INLINE void __incfsbyte(unsigned long Offset)
{
	__asm__ __volatile__("incb %%fs:%a[Offset]" : : [Offset] "ir" (Offset) : "memory");
}

__INTRIN_INLINE void __incfsword(unsigned long Offset)
{
	__asm__ __volatile__("incw %%fs:%a[Offset]" : : [Offset] "ir" (Offset) : "memory");
}

__INTRIN_INLINE void __incfsdword(unsigned long Offset)
{
	__asm__ __volatile__("incl %%fs:%a[Offset]" : : [Offset] "ir" (Offset) : "memory");
}

/* NOTE: the bizarre implementation of __addfsxxx mimics the broken Visual C++ behavior */
__INTRIN_INLINE void __addfsbyte(unsigned long Offset, unsigned char Data)
{
	if(!__builtin_constant_p(Offset))
		__asm__ __volatile__("addb %b[Offset], %%fs:%a[Offset]" : : [Offset] "r" (Offset) : "memory");
	else
		__asm__ __volatile__("addb %b[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data) : "memory");
}

__INTRIN_INLINE void __addfsword(unsigned long Offset, unsigned short Data)
{
	if(!__builtin_constant_p(Offset))
		__asm__ __volatile__("addw %w[Offset], %%fs:%a[Offset]" : : [Offset] "r" (Offset) : "memory");
	else
		__asm__ __volatile__("addw %w[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data) : "memory");
}

__INTRIN_INLINE void __addfsdword(unsigned long Offset, unsigned long Data)
{
	if(!__builtin_constant_p(Offset))
		__asm__ __volatile__("addl %k[Offset], %%fs:%a[Offset]" : : [Offset] "r" (Offset) : "memory");
	else
		__asm__ __volatile__("addl %k[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data) : "memory");
}

#endif /* defined(__x86_64__) */


/*** Bit manipulation ***/

#if !HAS_BUILTIN(_BitScanForward)
__INTRIN_INLINE unsigned char _BitScanForward(unsigned long * Index, unsigned long Mask)
{
	__asm__("bsfl %[Mask], %[Index]" : [Index] "=r" (*Index) : [Mask] "mr" (Mask));
	return Mask ? 1 : 0;
}
#endif

#if !HAS_BUILTIN(_BitScanReverse)
__INTRIN_INLINE unsigned char _BitScanReverse(unsigned long * Index, unsigned long Mask)
{
	__asm__("bsrl %[Mask], %[Index]" : [Index] "=r" (*Index) : [Mask] "mr" (Mask));
	return Mask ? 1 : 0;
}
#endif

#if !HAS_BUILTIN(_bittest)
/* NOTE: again, the bizarre implementation follows Visual C++ */
__INTRIN_INLINE unsigned char _bittest(const long * a, long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("bt %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*(a + (b / 32))), [b] "Ir" (b % 32));
	else
		__asm__("bt %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "m" (*a), [b] "r" (b));

	return retval;
}
#endif

#ifdef __x86_64__
#if !HAS_BUILTIN(_BitScanForward64)
__INTRIN_INLINE unsigned char _BitScanForward64(unsigned long * Index, unsigned long long Mask)
{
	unsigned long long Index64;
	__asm__("bsfq %[Mask], %[Index]" : [Index] "=r" (Index64) : [Mask] "mr" (Mask));
	*Index = Index64;
	return Mask ? 1 : 0;
}
#endif

#if !HAS_BUILTIN(_BitScanReverse64)
__INTRIN_INLINE unsigned char _BitScanReverse64(unsigned long * Index, unsigned long long Mask)
{
	unsigned long long Index64;
	__asm__("bsrq %[Mask], %[Index]" : [Index] "=r" (Index64) : [Mask] "mr" (Mask));
	*Index = Index64;
	return Mask ? 1 : 0;
}
#endif

#if !HAS_BUILTIN(_bittest64)
__INTRIN_INLINE unsigned char _bittest64(const long long * a, long long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("bt %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*(a + (b / 64))), [b] "Ir" (b % 64));
	else
		__asm__("bt %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "m" (*a), [b] "r" (b));

	return retval;
}
#endif
#endif

#if !HAS_BUILTIN(_bittestandcomplement)
__INTRIN_INLINE unsigned char _bittestandcomplement(long * a, long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("btc %[b], %[a]; setb %b[retval]" : [a] "+mr" (*(a + (b / 32))), [retval] "=q" (retval) : [b] "Ir" (b % 32));
	else
		__asm__("btc %[b], %[a]; setb %b[retval]" : [a] "+m" (*a), [retval] "=q" (retval) : [b] "r" (b));

	return retval;
}
#endif

#if !HAS_BUILTIN(_bittestandreset)
__INTRIN_INLINE unsigned char _bittestandreset(long * a, long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("btr %[b], %[a]; setb %b[retval]" : [a] "+mr" (*(a + (b / 32))), [retval] "=q" (retval) : [b] "Ir" (b % 32));
	else
		__asm__("btr %[b], %[a]; setb %b[retval]" : [a] "+m" (*a), [retval] "=q" (retval) : [b] "r" (b));

	return retval;
}
#endif

#if !HAS_BUILTIN(_bittestandset)
__INTRIN_INLINE unsigned char _bittestandset(long * a, long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("bts %[b], %[a]; setb %b[retval]" : [a] "+mr" (*(a + (b / 32))), [retval] "=q" (retval) : [b] "Ir" (b % 32));
	else
		__asm__("bts %[b], %[a]; setb %b[retval]" : [a] "+m" (*a), [retval] "=q" (retval) : [b] "r" (b));

	return retval;
}
#endif

#ifdef __x86_64__
#if !HAS_BUILTIN(_bittestandset64)
__INTRIN_INLINE unsigned char _bittestandset64(long long * a, long long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("btsq %[b], %[a]; setb %b[retval]" : [a] "+mr" (*(a + (b / 64))), [retval] "=q" (retval) : [b] "Ir" (b % 64));
	else
		__asm__("btsq %[b], %[a]; setb %b[retval]" : [a] "+m" (*a), [retval] "=q" (retval) : [b] "r" (b));

	return retval;
}
#endif

#if !HAS_BUILTIN(_bittestandreset64)
__INTRIN_INLINE unsigned char _bittestandreset64(long long * a, long long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("btrq %[b], %[a]; setb %b[retval]" : [a] "+mr" (*(a + (b / 64))), [retval] "=q" (retval) : [b] "Ir" (b % 64));
	else
		__asm__("btrq %[b], %[a]; setb %b[retval]" : [a] "+m" (*a), [retval] "=q" (retval) : [b] "r" (b));

	return retval;
}
#endif

#if !HAS_BUILTIN(_bittestandcomplement64)
__INTRIN_INLINE unsigned char _bittestandcomplement64(long long * a, long long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("btcq %[b], %[a]; setb %b[retval]" : [a] "+mr" (*(a + (b / 64))), [retval] "=q" (retval) : [b] "Ir" (b % 64));
	else
		__asm__("btcq %[b], %[a]; setb %b[retval]" : [a] "+m" (*a), [retval] "=q" (retval) : [b] "r" (b));

	return retval;
}
#endif
#endif /* __x86_64__ */

#if !HAS_BUILTIN(_rotl8)
__INTRIN_INLINE unsigned char __cdecl _rotl8(unsigned char value, unsigned char shift)
{
	unsigned char retval;
	__asm__("rolb %b[shift], %b[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}
#endif

#if !HAS_BUILTIN(_rotl16)
__INTRIN_INLINE unsigned short __cdecl _rotl16(unsigned short value, unsigned char shift)
{
	unsigned short retval;
	__asm__("rolw %b[shift], %w[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}
#endif

#if !HAS_BUILTIN(_rotl)
__INTRIN_INLINE unsigned int __cdecl _rotl(unsigned int value, int shift)
{
	unsigned int retval;
	__asm__("roll %b[shift], %k[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}
#endif

#if !HAS_BUILTIN(_rotl64)
#ifdef __x86_64__
__INTRIN_INLINE unsigned long long _rotl64(unsigned long long value, int shift)
{
	unsigned long long retval;
	__asm__("rolq %b[shift], %k[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}
#else /* __x86_64__ */
__INTRIN_INLINE unsigned long long __cdecl _rotl64(unsigned long long value, int shift)
{
    /* FIXME: this is probably not optimal */
    return (value << shift) | (value >> (64 - shift));
}
#endif /* __x86_64__ */
#endif /* !HAS_BUILTIN(_rotl64) */

#if !HAS_BUILTIN(_rotr)
__INTRIN_INLINE unsigned int __cdecl _rotr(unsigned int value, int shift)
{
	unsigned int retval;
	__asm__("rorl %b[shift], %k[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}
#endif

#if !HAS_BUILTIN(_rotr8)
__INTRIN_INLINE unsigned char __cdecl _rotr8(unsigned char value, unsigned char shift)
{
	unsigned char retval;
	__asm__("rorb %b[shift], %b[retval]" : [retval] "=qm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}
#endif

#if !HAS_BUILTIN(_rotr16)
__INTRIN_INLINE unsigned short __cdecl _rotr16(unsigned short value, unsigned char shift)
{
	unsigned short retval;
	__asm__("rorw %b[shift], %w[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}
#endif

#if !HAS_BUILTIN(_rotr64)
#ifdef __x86_64__
__INTRIN_INLINE unsigned long long _rotr64(unsigned long long value, int shift)
{
	unsigned long long retval;
	__asm__("rorq %b[shift], %k[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}
#else /* __x86_64__ */
__INTRIN_INLINE unsigned long long __cdecl _rotr64(unsigned long long value, int shift)
{
    /* FIXME: this is probably not optimal */
    return (value >> shift) | (value << (64 - shift));
}
#endif /* __x86_64__ */
#endif /* !HAS_BUILTIN(_rotr64) */

#if !HAS_BUILTIN(_lrotl)
__INTRIN_INLINE unsigned long __cdecl _lrotl(unsigned long value, int shift)
{
	unsigned long retval;
	__asm__("roll %b[shift], %k[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}
#endif

#if !HAS_BUILTIN(_lrotr)
__INTRIN_INLINE unsigned long __cdecl _lrotr(unsigned long value, int shift)
{
	unsigned long retval;
	__asm__("rorl %b[shift], %k[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}
#endif

#ifdef __x86_64__
__INTRIN_INLINE unsigned long long __ll_lshift(unsigned long long Mask, int Bit)
{
    unsigned long long retval;
    unsigned char shift = Bit & 0x3F;

    __asm__
    (
        "shlq %[shift], %[Mask]" : "=r"(retval) : [Mask] "0"(Mask), [shift] "c"(shift)
    );

    return retval;
}

__INTRIN_INLINE long long __ll_rshift(long long Mask, int Bit)
{
    long long retval;
    unsigned char shift = Bit & 0x3F;

    __asm__
    (
        "sarq %[shift], %[Mask]" : "=r"(retval) : [Mask] "0"(Mask), [shift] "c"(shift)
    );

    return retval;
}

__INTRIN_INLINE unsigned long long __ull_rshift(unsigned long long Mask, int Bit)
{
    long long retval;
    unsigned char shift = Bit & 0x3F;

    __asm__
    (
        "shrq %[shift], %[Mask]" : "=r"(retval) : [Mask] "0"(Mask), [shift] "c"(shift)
    );

    return retval;
}
#else
/*
	NOTE: in __ll_lshift, __ll_rshift and __ull_rshift we use the "A"
	constraint (edx:eax) for the Mask argument, because it's the only way GCC
	can pass 64-bit operands around - passing the two 32 bit parts separately
	just confuses it. Also we declare Bit as an int and then truncate it to
	match Visual C++ behavior
*/
__INTRIN_INLINE unsigned long long __ll_lshift(unsigned long long Mask, int Bit)
{
	unsigned long long retval = Mask;

	__asm__
	(
		"shldl %b[Bit], %%eax, %%edx; sall %b[Bit], %%eax" :
		"+A" (retval) :
		[Bit] "Nc" ((unsigned char)((unsigned long)Bit) & 0xFF)
	);

	return retval;
}

__INTRIN_INLINE long long __ll_rshift(long long Mask, int Bit)
{
	long long retval = Mask;

	__asm__
	(
		"shrdl %b[Bit], %%edx, %%eax; sarl %b[Bit], %%edx" :
		"+A" (retval) :
		[Bit] "Nc" ((unsigned char)((unsigned long)Bit) & 0xFF)
	);

	return retval;
}

__INTRIN_INLINE unsigned long long __ull_rshift(unsigned long long Mask, int Bit)
{
	unsigned long long retval = Mask;

	__asm__
	(
		"shrdl %b[Bit], %%edx, %%eax; shrl %b[Bit], %%edx" :
		"+A" (retval) :
		[Bit] "Nc" ((unsigned char)((unsigned long)Bit) & 0xFF)
	);

	return retval;
}
#endif

__INTRIN_INLINE unsigned short __cdecl _byteswap_ushort(unsigned short value)
{
	unsigned short retval;
	__asm__("rorw $8, %w[retval]" : [retval] "=rm" (retval) : "[retval]" (value));
	return retval;
}

__INTRIN_INLINE unsigned long __cdecl _byteswap_ulong(unsigned long value)
{
	unsigned long retval;
	__asm__("bswapl %[retval]" : [retval] "=r" (retval) : "[retval]" (value));
	return retval;
}

#ifdef __x86_64__
__INTRIN_INLINE unsigned long long _byteswap_uint64(unsigned long long value)
{
	unsigned long long retval;
	__asm__("bswapq %[retval]" : [retval] "=r" (retval) : "[retval]" (value));
	return retval;
}
#else
__INTRIN_INLINE unsigned long long __cdecl _byteswap_uint64(unsigned long long value)
{
	union {
		unsigned long long int64part;
		struct {
			unsigned long lowpart;
			unsigned long hipart;
		};
	} retval;
	retval.int64part = value;
	__asm__("bswapl %[lowpart]\n"
	        "bswapl %[hipart]\n"
	        : [lowpart] "=r" (retval.hipart), [hipart] "=r" (retval.lowpart)  : "[lowpart]" (retval.lowpart), "[hipart]" (retval.hipart) );
	return retval.int64part;
}
#endif

#if !HAS_BUILTIN(__lzcnt)
__INTRIN_INLINE unsigned int __lzcnt(unsigned int value)
{
	return __builtin_clz(value);
}
#endif

#if !HAS_BUILTIN(__lzcnt16)
__INTRIN_INLINE unsigned short __lzcnt16(unsigned short value)
{
	return __builtin_clz(value);
}
#endif

#if !HAS_BUILTIN(__popcnt)
__INTRIN_INLINE unsigned int __popcnt(unsigned int value)
{
	return __builtin_popcount(value);
}
#endif

#if !HAS_BUILTIN(__popcnt16)
__INTRIN_INLINE unsigned short __popcnt16(unsigned short value)
{
	return __builtin_popcount(value);
}
#endif

#ifdef __x86_64__
#if !HAS_BUILTIN(__lzcnt64)
__INTRIN_INLINE unsigned long long __lzcnt64(unsigned long long value)
{
	return __builtin_clzll(value);
}
#endif

#if !HAS_BUILTIN(__popcnt64)
__INTRIN_INLINE unsigned long long __popcnt64(unsigned long long value)
{
	return __builtin_popcountll(value);
}
#endif
#endif

/*** 64-bit math ***/

#if !HAS_BUILTIN(__emul)
__INTRIN_INLINE long long __emul(int a, int b)
{
	long long retval;
	__asm__("imull %[b]" : "=A" (retval) : [a] "a" (a), [b] "rm" (b));
	return retval;
}
#endif

#if !HAS_BUILTIN(__emulu)
__INTRIN_INLINE unsigned long long __emulu(unsigned int a, unsigned int b)
{
	unsigned long long retval;
	__asm__("mull %[b]" : "=A" (retval) : [a] "a" (a), [b] "rm" (b));
	return retval;
}
#endif

__INTRIN_INLINE long long __cdecl _abs64(long long value)
{
    return (value >= 0) ? value : -value;
}

#ifdef __x86_64__
#if !HAS_BUILTIN(__mulh)
__INTRIN_INLINE long long __mulh(long long a, long long b)
{
	long long retval;
	__asm__("imulq %[b]" : "=d" (retval) : [a] "a" (a), [b] "rm" (b));
	return retval;
}
#endif

#if !HAS_BUILTIN(__umulh)
__INTRIN_INLINE unsigned long long __umulh(unsigned long long a, unsigned long long b)
{
	unsigned long long retval;
	__asm__("mulq %[b]" : "=d" (retval) : [a] "a" (a), [b] "rm" (b));
	return retval;
}
#endif
#endif

/*** Port I/O ***/

__INTRIN_INLINE unsigned char __inbyte(unsigned short Port)
{
	unsigned char byte;
	__asm__ __volatile__("inb %w[Port], %b[byte]" : [byte] "=a" (byte) : [Port] "Nd" (Port));
	return byte;
}

__INTRIN_INLINE unsigned short __inword(unsigned short Port)
{
	unsigned short word;
	__asm__ __volatile__("inw %w[Port], %w[word]" : [word] "=a" (word) : [Port] "Nd" (Port));
	return word;
}

__INTRIN_INLINE unsigned long __indword(unsigned short Port)
{
	unsigned long dword;
	__asm__ __volatile__("inl %w[Port], %k[dword]" : [dword] "=a" (dword) : [Port] "Nd" (Port));
	return dword;
}

__INTRIN_INLINE void __inbytestring(unsigned short Port, unsigned char * Buffer, unsigned long Count)
{
	__asm__ __volatile__
	(
		"rep; insb" :
		[Buffer] "=D" (Buffer), [Count] "=c" (Count) :
		"d" (Port), "[Buffer]" (Buffer), "[Count]" (Count) :
		"memory"
	);
}

__INTRIN_INLINE void __inwordstring(unsigned short Port, unsigned short * Buffer, unsigned long Count)
{
	__asm__ __volatile__
	(
		"rep; insw" :
		[Buffer] "=D" (Buffer), [Count] "=c" (Count) :
		"d" (Port), "[Buffer]" (Buffer), "[Count]" (Count) :
		"memory"
	);
}

__INTRIN_INLINE void __indwordstring(unsigned short Port, unsigned long * Buffer, unsigned long Count)
{
	__asm__ __volatile__
	(
		"rep; insl" :
		[Buffer] "=D" (Buffer), [Count] "=c" (Count) :
		"d" (Port), "[Buffer]" (Buffer), "[Count]" (Count) :
		"memory"
	);
}

__INTRIN_INLINE void __outbyte(unsigned short Port, unsigned char Data)
{
	__asm__ __volatile__("outb %b[Data], %w[Port]" : : [Port] "Nd" (Port), [Data] "a" (Data));
}

__INTRIN_INLINE void __outword(unsigned short Port, unsigned short Data)
{
	__asm__ __volatile__("outw %w[Data], %w[Port]" : : [Port] "Nd" (Port), [Data] "a" (Data));
}

__INTRIN_INLINE void __outdword(unsigned short Port, unsigned long Data)
{
	__asm__ __volatile__("outl %k[Data], %w[Port]" : : [Port] "Nd" (Port), [Data] "a" (Data));
}

__INTRIN_INLINE void __outbytestring(unsigned short Port, unsigned char * Buffer, unsigned long Count)
{
	__asm__ __volatile__("rep; outsb" : : [Port] "d" (Port), [Buffer] "S" (Buffer), "c" (Count));
}

__INTRIN_INLINE void __outwordstring(unsigned short Port, unsigned short * Buffer, unsigned long Count)
{
	__asm__ __volatile__("rep; outsw" : : [Port] "d" (Port), [Buffer] "S" (Buffer), "c" (Count));
}

__INTRIN_INLINE void __outdwordstring(unsigned short Port, unsigned long * Buffer, unsigned long Count)
{
	__asm__ __volatile__("rep; outsl" : : [Port] "d" (Port), [Buffer] "S" (Buffer), "c" (Count));
}

__INTRIN_INLINE int __cdecl _inp(unsigned short Port)
{
	return __inbyte(Port);
}

__INTRIN_INLINE unsigned short __cdecl _inpw(unsigned short Port)
{
	return __inword(Port);
}

__INTRIN_INLINE unsigned long __cdecl _inpd(unsigned short Port)
{
	return __indword(Port);
}

__INTRIN_INLINE int __cdecl _outp(unsigned short Port, int databyte)
{
	__outbyte(Port, (unsigned char)databyte);
	return databyte;
}

__INTRIN_INLINE unsigned short __cdecl _outpw(unsigned short Port, unsigned short dataword)
{
	__outword(Port, dataword);
	return dataword;
}

__INTRIN_INLINE unsigned long __cdecl _outpd(unsigned short Port, unsigned long dataword)
{
	__outdword(Port, dataword);
	return dataword;
}


/*** System information ***/

__INTRIN_INLINE void __cpuid(int CPUInfo[4], int InfoType)
{
	__asm__ __volatile__("cpuid" : "=a" (CPUInfo[0]), "=b" (CPUInfo[1]), "=c" (CPUInfo[2]), "=d" (CPUInfo[3]) : "a" (InfoType));
}

__INTRIN_INLINE void __cpuidex(int CPUInfo[4], int InfoType, int ECXValue)
{
	__asm__ __volatile__("cpuid" : "=a" (CPUInfo[0]), "=b" (CPUInfo[1]), "=c" (CPUInfo[2]), "=d" (CPUInfo[3]) : "a" (InfoType), "c" (ECXValue));
}

#if !HAS_BUILTIN(__rdtsc)
__INTRIN_INLINE unsigned long long __rdtsc(void)
{
#ifdef __x86_64__
	unsigned long long low, high;
	__asm__ __volatile__("rdtsc" : "=a"(low), "=d"(high));
	return low | (high << 32);
#else
	unsigned long long retval;
	__asm__ __volatile__("rdtsc" : "=A"(retval));
	return retval;
#endif
}
#endif /* !HAS_BUILTIN(__rdtsc) */

__INTRIN_INLINE void __writeeflags(uintptr_t Value)
{
	__asm__ __volatile__("push %0\n popf" : : "rim"(Value));
}

__INTRIN_INLINE uintptr_t __readeflags(void)
{
	uintptr_t retval;
	__asm__ __volatile__("pushf\n pop %0" : "=rm"(retval));
	return retval;
}

/*** Interrupts ***/

#if !HAS_BUILTIN(__debugbreak)
__INTRIN_INLINE void __cdecl __debugbreak(void)
{
	__asm__("int $3");
}
#endif

#if !HAS_BUILTIN(__ud2)
__INTRIN_INLINE void __ud2(void)
{
	__asm__("ud2");
}
#endif

#if !HAS_BUILTIN(__int2c)
__INTRIN_INLINE void __int2c(void)
{
	__asm__("int $0x2c");
}
#endif

__INTRIN_INLINE void __cdecl _disable(void)
{
	__asm__("cli" : : : "memory");
}

__INTRIN_INLINE void __cdecl _enable(void)
{
	__asm__("sti" : : : "memory");
}

__INTRIN_INLINE void __halt(void)
{
	__asm__("hlt" : : : "memory");
}

#if !HAS_BUILTIN(__fastfail)
__declspec(noreturn)
__INTRIN_INLINE void __fastfail(unsigned int Code)
{
	__asm__("int $0x29" : : "c"(Code) : "memory");
	__builtin_unreachable();
}
#endif

/*** Protected memory management ***/

#ifdef __x86_64__

__INTRIN_INLINE void __writecr0(unsigned long long Data)
{
	__asm__("mov %[Data], %%cr0" : : [Data] "r" (Data) : "memory");
}

__INTRIN_INLINE void __writecr3(unsigned long long Data)
{
	__asm__("mov %[Data], %%cr3" : : [Data] "r" (Data) : "memory");
}

__INTRIN_INLINE void __writecr4(unsigned long long Data)
{
	__asm__("mov %[Data], %%cr4" : : [Data] "r" (Data) : "memory");
}

__INTRIN_INLINE void __writecr8(unsigned long long Data)
{
	__asm__("mov %[Data], %%cr8" : : [Data] "r" (Data) : "memory");
}

__INTRIN_INLINE unsigned long long __readcr0(void)
{
	unsigned long long value;
	__asm__ __volatile__("mov %%cr0, %[value]" : [value] "=r" (value));
	return value;
}

__INTRIN_INLINE unsigned long long __readcr2(void)
{
	unsigned long long value;
	__asm__ __volatile__("mov %%cr2, %[value]" : [value] "=r" (value));
	return value;
}

__INTRIN_INLINE unsigned long long __readcr3(void)
{
	unsigned long long value;
	__asm__ __volatile__("mov %%cr3, %[value]" : [value] "=r" (value));
	return value;
}

__INTRIN_INLINE unsigned long long __readcr4(void)
{
	unsigned long long value;
	__asm__ __volatile__("mov %%cr4, %[value]" : [value] "=r" (value));
	return value;
}

__INTRIN_INLINE unsigned long long __readcr8(void)
{
	unsigned long long value;
	__asm__ __volatile__("movq %%cr8, %q[value]" : [value] "=r" (value));
	return value;
}

#else /* __x86_64__ */

__INTRIN_INLINE void __writecr0(unsigned int Data)
{
	__asm__("mov %[Data], %%cr0" : : [Data] "r" (Data) : "memory");
}

__INTRIN_INLINE void __writecr3(unsigned int Data)
{
	__asm__("mov %[Data], %%cr3" : : [Data] "r" (Data) : "memory");
}

__INTRIN_INLINE void __writecr4(unsigned int Data)
{
	__asm__("mov %[Data], %%cr4" : : [Data] "r" (Data) : "memory");
}

__INTRIN_INLINE unsigned long __readcr0(void)
{
	unsigned long value;
	__asm__ __volatile__("mov %%cr0, %[value]" : [value] "=r" (value));
	return value;
}

__INTRIN_INLINE unsigned long __readcr2(void)
{
	unsigned long value;
	__asm__ __volatile__("mov %%cr2, %[value]" : [value] "=r" (value));
	return value;
}

__INTRIN_INLINE unsigned long __readcr3(void)
{
	unsigned long value;
	__asm__ __volatile__("mov %%cr3, %[value]" : [value] "=r" (value));
	return value;
}

__INTRIN_INLINE unsigned long __readcr4(void)
{
	unsigned long value;
	__asm__ __volatile__("mov %%cr4, %[value]" : [value] "=r" (value));
	return value;
}

#endif /* __x86_64__ */

#ifdef __x86_64__

__INTRIN_INLINE unsigned long long __readdr(unsigned int reg)
{
	unsigned long long value;
	switch (reg)
	{
		case 0:
			__asm__ __volatile__("movq %%dr0, %q[value]" : [value] "=r" (value));
			break;
		case 1:
			__asm__ __volatile__("movq %%dr1, %q[value]" : [value] "=r" (value));
			break;
		case 2:
			__asm__ __volatile__("movq %%dr2, %q[value]" : [value] "=r" (value));
			break;
		case 3:
			__asm__ __volatile__("movq %%dr3, %q[value]" : [value] "=r" (value));
			break;
		case 4:
			__asm__ __volatile__("movq %%dr4, %q[value]" : [value] "=r" (value));
			break;
		case 5:
			__asm__ __volatile__("movq %%dr5, %q[value]" : [value] "=r" (value));
			break;
		case 6:
			__asm__ __volatile__("movq %%dr6, %q[value]" : [value] "=r" (value));
			break;
		case 7:
			__asm__ __volatile__("movq %%dr7, %q[value]" : [value] "=r" (value));
			break;
	}
	return value;
}

__INTRIN_INLINE void __writedr(unsigned reg, unsigned long long value)
{
	switch (reg)
	{
		case 0:
			__asm__("movq %q[value], %%dr0" : : [value] "r" (value) : "memory");
			break;
		case 1:
			__asm__("movq %q[value], %%dr1" : : [value] "r" (value) : "memory");
			break;
		case 2:
			__asm__("movq %q[value], %%dr2" : : [value] "r" (value) : "memory");
			break;
		case 3:
			__asm__("movq %q[value], %%dr3" : : [value] "r" (value) : "memory");
			break;
		case 4:
			__asm__("movq %q[value], %%dr4" : : [value] "r" (value) : "memory");
			break;
		case 5:
			__asm__("movq %q[value], %%dr5" : : [value] "r" (value) : "memory");
			break;
		case 6:
			__asm__("movq %q[value], %%dr6" : : [value] "r" (value) : "memory");
			break;
		case 7:
			__asm__("movq %q[value], %%dr7" : : [value] "r" (value) : "memory");
			break;
	}
}

#else /* __x86_64__ */

__INTRIN_INLINE unsigned int __readdr(unsigned int reg)
{
	unsigned int value;
	switch (reg)
	{
		case 0:
			__asm__ __volatile__("mov %%dr0, %[value]" : [value] "=r" (value));
			break;
		case 1:
			__asm__ __volatile__("mov %%dr1, %[value]" : [value] "=r" (value));
			break;
		case 2:
			__asm__ __volatile__("mov %%dr2, %[value]" : [value] "=r" (value));
			break;
		case 3:
			__asm__ __volatile__("mov %%dr3, %[value]" : [value] "=r" (value));
			break;
		case 4:
			__asm__ __volatile__("mov %%dr4, %[value]" : [value] "=r" (value));
			break;
		case 5:
			__asm__ __volatile__("mov %%dr5, %[value]" : [value] "=r" (value));
			break;
		case 6:
			__asm__ __volatile__("mov %%dr6, %[value]" : [value] "=r" (value));
			break;
		case 7:
			__asm__ __volatile__("mov %%dr7, %[value]" : [value] "=r" (value));
			break;
	}
	return value;
}

__INTRIN_INLINE void __writedr(unsigned reg, unsigned int value)
{
	switch (reg)
	{
		case 0:
			__asm__("mov %[value], %%dr0" : : [value] "r" (value) : "memory");
			break;
		case 1:
			__asm__("mov %[value], %%dr1" : : [value] "r" (value) : "memory");
			break;
		case 2:
			__asm__("mov %[value], %%dr2" : : [value] "r" (value) : "memory");
			break;
		case 3:
			__asm__("mov %[value], %%dr3" : : [value] "r" (value) : "memory");
			break;
		case 4:
			__asm__("mov %[value], %%dr4" : : [value] "r" (value) : "memory");
			break;
		case 5:
			__asm__("mov %[value], %%dr5" : : [value] "r" (value) : "memory");
			break;
		case 6:
			__asm__("mov %[value], %%dr6" : : [value] "r" (value) : "memory");
			break;
		case 7:
			__asm__("mov %[value], %%dr7" : : [value] "r" (value) : "memory");
			break;
	}
}

#endif /* __x86_64__ */

__INTRIN_INLINE void __invlpg(void *Address)
{
	__asm__ __volatile__ ("invlpg (%[Address])" : : [Address] "b" (Address) : "memory");
}


/*** System operations ***/

__INTRIN_INLINE unsigned long long __readmsr(unsigned long reg)
{
#ifdef __x86_64__
	unsigned long low, high;
	__asm__ __volatile__("rdmsr" : "=a" (low), "=d" (high) : "c" (reg));
	return ((unsigned long long)high << 32) | low;
#else
	unsigned long long retval;
	__asm__ __volatile__("rdmsr" : "=A" (retval) : "c" (reg));
	return retval;
#endif
}

__INTRIN_INLINE void __writemsr(unsigned long Register, unsigned long long Value)
{
#ifdef __x86_64__
	__asm__ __volatile__("wrmsr" : : "a" (Value), "d" (Value >> 32), "c" (Register));
#else
	__asm__ __volatile__("wrmsr" : : "A" (Value), "c" (Register));
#endif
}

__INTRIN_INLINE unsigned long long __readpmc(unsigned long counter)
{
	unsigned long long retval;
	__asm__ __volatile__("rdpmc" : "=A" (retval) : "c" (counter));
	return retval;
}

/* NOTE: an immediate value for 'a' will raise an ICE in Visual C++ */
__INTRIN_INLINE unsigned long __segmentlimit(unsigned long a)
{
	unsigned long retval;
	__asm__ __volatile__("lsl %[a], %[retval]" : [retval] "=r" (retval) : [a] "rm" (a));
	return retval;
}

__INTRIN_INLINE void __wbinvd(void)
{
	__asm__ __volatile__("wbinvd" : : : "memory");
}

__INTRIN_INLINE void __lidt(void *Source)
{
	__asm__ __volatile__("lidt %0" : : "m"(*(short*)Source));
}

__INTRIN_INLINE void __sidt(void *Destination)
{
	__asm__ __volatile__("sidt %0" : : "m"(*(short*)Destination) : "memory");
}

__INTRIN_INLINE void _sgdt(void *Destination)
{
	__asm__ __volatile__("sgdt %0" : : "m"(*(short*)Destination) : "memory");
}

/*** Misc operations ***/

#if !HAS_BUILTIN(_mm_pause)
__INTRIN_INLINE void _mm_pause(void)
{
	__asm__ __volatile__("pause" : : : "memory");
}
#endif

__INTRIN_INLINE void __nop(void)
{
	__asm__ __volatile__("nop");
}

#ifdef __cplusplus
}
#endif

#endif /* KJK_INTRIN_X86_H_ */

/* EOF */
