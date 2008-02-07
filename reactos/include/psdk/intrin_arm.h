/*
	Compatibility <intrin.h> header for GCC -- GCC equivalents of intrinsic
	Microsoft Visual C++ functions. Originally developed for the ReactOS
	(<http://www.reactos.org/>) and TinyKrnl (<http://www.tinykrnl.org/>)
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

#ifndef KJK_INTRIN_ARM_H_
#define KJK_INTRIN_ARM_H_

#ifndef __GNUC__
#error Unsupported compiler
#endif

#define _ReadWriteBarrier() __sync_synchronize()

FORCEINLINE char _InterlockedCompareExchange8(volatile char * const Destination, const char Exchange, const char Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

FORCEINLINE short _InterlockedCompareExchange16(volatile short * const Destination, const short Exchange, const short Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

FORCEINLINE long _InterlockedCompareExchange(volatile long * const Destination, const long Exchange, const long Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

FORCEINLINE long long _InterlockedCompareExchange64(volatile long long * const Destination, const long long Exchange, const long long Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

FORCEINLINE void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

FORCEINLINE long _InterlockedExchange(volatile long * const Target, const long Value)
{
	/* NOTE: __sync_lock_test_and_set would be an acquire barrier, so we force a full barrier */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}

FORCEINLINE void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value)
{
	/* NOTE: ditto */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchangeAdd16(volatile short * const Addend, const short Value)
{
	return __sync_fetch_and_add(Addend, Value);
}

FORCEINLINE long _InterlockedExchangeAdd(volatile long * const Addend, const long Value)
{
	return __sync_fetch_and_add(Addend, Value);
}

FORCEINLINE char _InterlockedAnd8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_and(value, mask);
}

FORCEINLINE short _InterlockedAnd16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_and(value, mask);
}

FORCEINLINE long _InterlockedAnd(volatile long * const value, const long mask)
{
	return __sync_fetch_and_and(value, mask);
}

FORCEINLINE char _InterlockedOr8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_or(value, mask);
}

FORCEINLINE short _InterlockedOr16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_or(value, mask);
}

FORCEINLINE long _InterlockedOr(volatile long * const value, const long mask)
{
	return __sync_fetch_and_or(value, mask);
}

FORCEINLINE char _InterlockedXor8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_xor(value, mask);
}

FORCEINLINE short _InterlockedXor16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_xor(value, mask);
}

FORCEINLINE long _InterlockedXor(volatile long * const value, const long mask)
{
	return __sync_fetch_and_xor(value, mask);
}

static __inline__ __attribute__((always_inline)) long _InterlockedDecrement(volatile long * const lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, -1) - 1;
}

static __inline__ __attribute__((always_inline)) long _InterlockedIncrement(volatile long * const lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, 1) + 1;
}

static __inline__ __attribute__((always_inline)) long _InterlockedDecrement16(volatile short * const lpAddend)
{
	return _InterlockedExchangeAdd16(lpAddend, -1) - 1;
}

static __inline__ __attribute__((always_inline)) long _InterlockedIncrement16(volatile short * const lpAddend)
{
	return _InterlockedExchangeAdd16(lpAddend, 1) + 1;
}

static __inline__ __attribute__((always_inline)) void _disable(void)
{
    __asm__ __volatile__
    (
     "mrs r1, cpsr;"
     "orr r1, r1, #0x80;"
     "msr cpsr, r1;"
    );
}

static __inline__ __attribute__((always_inline)) void _enable(void)
{
    __asm__ __volatile__
    (
     "mrs r1, cpsr;"
     "bic r1, r1, #0x80;"
     "msr cpsr, r1;"
    );
}

#endif
/* EOF */
