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

static __inline__ __attribute__((always_inline)) long _InterlockedCompareExchange(volatile long * const dest, const long exch, const long comp)
{
	long a, b;
    
	__asm__ __volatile__ (    "0:\n\t"
                          "ldr %1, [%2]\n\t"
                          "cmp %1, %4\n\t"
                          "bne 1f\n\t"
                          "swp %0, %3, [%2]\n\t"
                          "cmp %0, %1\n\t"
                          "swpne %3, %0, [%2]\n\t"
                          "bne 0b\n\t"
                          "1:"
                          : "=&r" (a), "=&r" (b)
                          : "r" (dest), "r" (exch), "r" (comp)
                          : "cc", "memory");
    
	return a;
}

static __inline__ __attribute__((always_inline)) long long _InterlockedCompareExchange64(volatile long long * const dest, const long long exch, const long long comp)
{
    //
    // FIXME
    //
    long long result;
    result = *dest;
    if (*dest == comp) *dest = exch;
    return result;
}

static __inline__ __attribute__((always_inline)) void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand)
{
    return (void*)_InterlockedCompareExchange((volatile long* const)Destination, (const long)Exchange, (const long)Comperand);
}


static __inline__ __attribute__((always_inline)) long _InterlockedExchangeAdd(volatile long * const dest, const long add)
{
	long a, b, c;
    
	__asm__ __volatile__ (  "0:\n\t"
                          "ldr %0, [%3]\n\t"
                          "add %1, %0, %4\n\t"
                          "swp %2, %1, [%3]\n\t"
                          "cmp %0, %2\n\t"
                          "swpne %1, %2, [%3]\n\t"
                          "bne 0b"
                          : "=&r" (a), "=&r" (b), "=&r" (c)
                          : "r" (dest), "r" (add)
                          : "cc", "memory");
    
	return a;
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchange(volatile long * const dest, const long exch)
{
	long a;
    
	__asm__ __volatile__ (  "swp %0, %2, [%1]"
                          : "=&r" (a)
                          : "r" (dest), "r" (exch));
    
	return a;
}


static __inline__ __attribute__((always_inline)) void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value)
{
    return _InterlockedExchange(Target, Value);
}

static __inline__ __attribute__((always_inline)) char _InterlockedAnd8(volatile char * const value, const char mask)
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

static __inline__ __attribute__((always_inline)) short _InterlockedAnd16(volatile short * const value, const short mask)
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

static __inline__ __attribute__((always_inline)) long _InterlockedAnd(volatile long * const value, const long mask)
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

static __inline__ __attribute__((always_inline)) char _InterlockedOr8(volatile char * const value, const char mask)
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

static __inline__ __attribute__((always_inline)) short _InterlockedOr16(volatile short * const value, const short mask)
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

static __inline__ __attribute__((always_inline)) long _InterlockedOr(volatile long * const value, const long mask)
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

static __inline__ __attribute__((always_inline)) char _InterlockedXor8(volatile char * const value, const char mask)
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

static __inline__ __attribute__((always_inline)) short _InterlockedXor16(volatile short * const value, const short mask)
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

static __inline__ __attribute__((always_inline)) long _InterlockedXor(volatile long * const value, const long mask)
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

#ifndef __MSVCRT__
static __inline__ __attribute__((always_inline)) unsigned long _rotl(const unsigned long value, const unsigned char shift)
{
	return (((value) << ((int)(shift))) | ((value) >> (32 - (int)(shift))));
}
#endif

#define _clz(a) \
({ ULONG __value, __arg = (a); \
asm ("clz\t%0, %1": "=r" (__value): "r" (__arg)); \
__value; })

#endif
/* EOF */
