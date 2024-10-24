/*
	Compatibility <intrin.h> header for GCC -- GCC equivalents of intrinsic
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

#ifndef KJK_INTRIN_ARM_H_
#define KJK_INTRIN_ARM_H_

#ifndef __GNUC__
#error Unsupported compiler
#endif

#define _ReturnAddress() (__builtin_return_address(0))
#define _ReadWriteBarrier() __sync_synchronize()

__INTRIN_INLINE void __yield(void) { __asm__ __volatile__("yield"); }

__INTRIN_INLINE void __break(unsigned int value) { __asm__ __volatile__("bkpt %0": : "M" (value)); }

__INTRIN_INLINE unsigned short _byteswap_ushort(unsigned short value)
{
	return (value >> 8) | (value << 8);
}

__INTRIN_INLINE unsigned _CountLeadingZeros(long Mask)
{
    return Mask ? __builtin_clz(Mask) : 32;
}

__INTRIN_INLINE unsigned _CountTrailingZeros(long Mask)
{
    return Mask ? __builtin_ctz(Mask) : 32;
}

__INTRIN_INLINE unsigned char _BitScanForward(unsigned long * const Index, const unsigned long Mask)
{
	*Index = __builtin_ctz(Mask);
	return Mask ? 1 : 0;
}

__INTRIN_INLINE char _InterlockedCompareExchange8(volatile char * const Destination, const char Exchange, const char Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

__INTRIN_INLINE short _InterlockedCompareExchange16(volatile short * const Destination, const short Exchange, const short Comperand)
{
	short a, b;

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
                          : "r" (Destination), "r" (Exchange), "r" (Comperand)
                          : "cc", "memory");

	return a;
}

__INTRIN_INLINE short _InterlockedExchangeAdd16(volatile short * const Addend, const short Value)
{
	short a, b, c;

	__asm__ __volatile__ (  "0:\n\t"
                          "ldr %0, [%3]\n\t"
                          "add %1, %0, %4\n\t"
                          "swp %2, %1, [%3]\n\t"
                          "cmp %0, %2\n\t"
                          "swpne %1, %2, [%3]\n\t"
                          "bne 0b"
                          : "=&r" (a), "=&r" (b), "=&r" (c)
                          : "r" (Value), "r" (Addend)
                          : "cc", "memory");

	return a;
}

__INTRIN_INLINE long _InterlockedCompareExchange(volatile long * const dest, const long exch, const long comp)
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

__INTRIN_INLINE long long _InterlockedCompareExchange64(volatile long long * const dest, const long long exch, const long long comp)
{
    //
    // FIXME
    //
    long long result;
    result = *dest;
    if (*dest == comp) *dest = exch;
    return result;
}

__INTRIN_INLINE void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand)
{
    return (void*)_InterlockedCompareExchange((volatile long* const)Destination, (const long)Exchange, (const long)Comperand);
}


__INTRIN_INLINE long _InterlockedExchangeAdd(volatile long * const dest, const long add)
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

__INTRIN_INLINE long _InterlockedExchange(volatile long * const dest, const long exch)
{
	long a;

	__asm__ __volatile__ (  "swp %0, %2, [%1]"
                          : "=&r" (a)
                          : "r" (dest), "r" (exch));

	return a;
}


__INTRIN_INLINE void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value)
{
    return (void *)_InterlockedExchange((volatile long * const)Target, (const long)Value);
}



__INTRIN_INLINE unsigned char _BitScanReverse(unsigned long * const Index, const unsigned long Mask)
{
    *Index = 31 - __builtin_clz(Mask);
	return Mask ? 1 : 0;
}

__INTRIN_INLINE char _InterlockedAnd8(volatile char * const value, const char mask)
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

__INTRIN_INLINE short _InterlockedAnd16(volatile short * const value, const short mask)
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

__INTRIN_INLINE long _InterlockedAnd(volatile long * const value, const long mask)
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

__INTRIN_INLINE char _InterlockedOr8(volatile char * const value, const char mask)
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

__INTRIN_INLINE short _InterlockedOr16(volatile short * const value, const short mask)
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

__INTRIN_INLINE long _InterlockedOr(volatile long * const value, const long mask)
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

__INTRIN_INLINE char _InterlockedXor8(volatile char * const value, const char mask)
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

__INTRIN_INLINE short _InterlockedXor16(volatile short * const value, const short mask)
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

__INTRIN_INLINE long _InterlockedXor(volatile long * const value, const long mask)
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

__INTRIN_INLINE long _InterlockedDecrement(volatile long * const lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, -1) - 1;
}

__INTRIN_INLINE long _InterlockedIncrement(volatile long * const lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, 1) + 1;
}

__INTRIN_INLINE long _InterlockedDecrement16(volatile short * const lpAddend)
{
	return _InterlockedExchangeAdd16(lpAddend, -1) - 1;
}

__INTRIN_INLINE long _InterlockedIncrement16(volatile short * const lpAddend)
{
	return _InterlockedExchangeAdd16(lpAddend, 1) + 1;
}

__INTRIN_INLINE long _InterlockedAddLargeStatistic(volatile long long * const Addend, const long Value)
{
    *Addend += Value;
    return Value;
}

__INTRIN_INLINE void _disable(void)
{
    __asm__ __volatile__
    (
     "cpsid i    @ __cli" : : : "memory", "cc"
    );
}

__INTRIN_INLINE void _enable(void)
{
    __asm__ __volatile__
    (
     "cpsie i    @ __sti" : : : "memory", "cc"
    );
}

__INTRIN_INLINE unsigned char _interlockedbittestandset(volatile long * a, const long b)
{
	return (_InterlockedOr(a, 1 << b) >> b) & 1;
}

__INTRIN_INLINE unsigned char _interlockedbittestandreset(volatile long * a, const long b)
{
	return (_InterlockedAnd(a, ~(1 << b)) >> b) & 1;
}

#ifndef __MSVCRT__
__INTRIN_INLINE unsigned int _rotl(const unsigned int value, int shift)
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
