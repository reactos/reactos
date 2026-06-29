/*
	Compatibility <intrin_arm64.h> header providing GCC/Clang equivalents
	of intrinsic Microsoft Visual C++ functions.

	Copyright (c) 2026 Ahmed Arif <arif.ing@outlook.com>

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

#ifndef KJK_INTRIN_ARM64_H_
#define KJK_INTRIN_ARM64_H_

#if !defined(__GNUC__) && !defined(__clang__)
#error Unsupported compiler
#endif

#ifndef HAS_BUILTIN
#  ifdef __has_builtin
#    define HAS_BUILTIN(x) __has_builtin(x)
#  else
#    define HAS_BUILTIN(x) 0
#  endif
#endif

/* Fallback for tools that analyze this header in isolation; the real
   definition lives in sdk/include/vcruntime/_mingw.h and wins via the
   umbrella intrin.h. */
#ifndef __INTRIN_INLINE
#  define __INTRIN_INLINE extern __inline__ __attribute__((__always_inline__, __gnu_inline__))
#endif

#define _ReturnAddress() (__builtin_return_address(0))
/* AAPCS64 prologue stores LR at [fp, #8]. Requires -fno-omit-frame-pointer. */
#define _AddressOfReturnAddress() ((void *)((unsigned char *)__builtin_frame_address(0) + 8))

#if !HAS_BUILTIN(__break)
__INTRIN_INLINE void __break(int value)
{
    __asm__ __volatile__("brk %0" : : "i"(value));
}
#endif

#if !HAS_BUILTIN(_ReadWriteBarrier)
__INTRIN_INLINE void _ReadWriteBarrier(void)
{
    __asm__ __volatile__("" : : : "memory");
}
#endif

#define _ReadBarrier _ReadWriteBarrier
#define _WriteBarrier _ReadWriteBarrier

#if !HAS_BUILTIN(_byteswap_ushort)
__INTRIN_INLINE unsigned short _byteswap_ushort(unsigned short value)
{
    return __builtin_bswap16(value);
}
#endif

#if !HAS_BUILTIN(_byteswap_ulong)
__INTRIN_INLINE unsigned long __cdecl _byteswap_ulong(unsigned long value)
{
    return __builtin_bswap32(value);
}
#endif

#if !HAS_BUILTIN(_byteswap_uint64)
__INTRIN_INLINE unsigned __int64 __cdecl _byteswap_uint64(unsigned __int64 value)
{
    return __builtin_bswap64(value);
}
#endif

#if !HAS_BUILTIN(_BitScanForward)
__INTRIN_INLINE unsigned char _BitScanForward(unsigned long *Index, unsigned long Mask)
{
    if (!Mask)
        return 0;

    *Index = __builtin_ctzl(Mask);
    return 1;
}
#endif

#if !HAS_BUILTIN(_BitScanReverse)
__INTRIN_INLINE unsigned char _BitScanReverse(unsigned long *Index, unsigned long Mask)
{
    if (!Mask)
        return 0;

    *Index = (sizeof(Mask) * 8 - 1) - __builtin_clzl(Mask);
    return 1;
}
#endif

#if !HAS_BUILTIN(_BitScanForward64)
__INTRIN_INLINE unsigned char _BitScanForward64(unsigned long *Index, unsigned long long Mask)
{
    if (!Mask)
        return 0;

    *Index = __builtin_ctzll(Mask);
    return 1;
}
#endif

#if !HAS_BUILTIN(_BitScanReverse64)
__INTRIN_INLINE unsigned char _BitScanReverse64(unsigned long *Index, unsigned long long Mask)
{
    if (!Mask)
        return 0;

    *Index = 63 - __builtin_clzll(Mask);
    return 1;
}
#endif

#if !HAS_BUILTIN(_InterlockedCompareExchange)
__INTRIN_INLINE long _InterlockedCompareExchange(volatile long *Destination, long Exchange, long Comparand)
{
    return __sync_val_compare_and_swap(Destination, Comparand, Exchange);
}
#endif

#if !HAS_BUILTIN(_InterlockedCompareExchange16)
__INTRIN_INLINE short _InterlockedCompareExchange16(volatile short *Destination, short Exchange, short Comparand)
{
    return __sync_val_compare_and_swap(Destination, Comparand, Exchange);
}
#endif

#if !HAS_BUILTIN(_InterlockedCompareExchange8)
__INTRIN_INLINE char _InterlockedCompareExchange8(volatile char *Destination, char Exchange, char Comparand)
{
    return __sync_val_compare_and_swap(Destination, Comparand, Exchange);
}
#endif

#if !HAS_BUILTIN(_InterlockedCompareExchange64)
__INTRIN_INLINE long long _InterlockedCompareExchange64(volatile long long *Destination, long long Exchange, long long Comparand)
{
    return __sync_val_compare_and_swap(Destination, Comparand, Exchange);
}
#endif

#if !HAS_BUILTIN(_InterlockedCompareExchangePointer)
__INTRIN_INLINE void *_InterlockedCompareExchangePointer(void *volatile *Destination, void *Exchange, void *Comparand)
{
    return __sync_val_compare_and_swap(Destination, Comparand, Exchange);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange)
__INTRIN_INLINE long _InterlockedExchange(volatile long *Target, long Value)
{
    /* MS _InterlockedExchange is a full barrier; __sync_lock_test_and_set is acquire-only. */
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange16)
__INTRIN_INLINE short _InterlockedExchange16(volatile short *Target, short Value)
{
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange8)
__INTRIN_INLINE char _InterlockedExchange8(volatile char *Target, char Value)
{
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange64)
__INTRIN_INLINE long long _InterlockedExchange64(volatile long long *Target, long long Value)
{
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangePointer)
__INTRIN_INLINE void *_InterlockedExchangePointer(void *volatile *Target, void *Value)
{
    return __atomic_exchange_n(Target, Value, __ATOMIC_SEQ_CST);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangeAdd)
__INTRIN_INLINE long _InterlockedExchangeAdd(volatile long *Addend, long Value)
{
    return __sync_fetch_and_add(Addend, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangeAdd16)
__INTRIN_INLINE short _InterlockedExchangeAdd16(volatile short *Addend, short Value)
{
    return __sync_fetch_and_add(Addend, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangeAdd8)
__INTRIN_INLINE char _InterlockedExchangeAdd8(volatile char *Addend, char Value)
{
    return __sync_fetch_and_add(Addend, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangeAdd64)
__INTRIN_INLINE long long _InterlockedExchangeAdd64(volatile long long *Addend, long long Value)
{
    return __sync_fetch_and_add(Addend, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedAnd)
__INTRIN_INLINE long _InterlockedAnd(volatile long *Value, long Mask)
{
    return __sync_fetch_and_and(Value, Mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedAnd16)
__INTRIN_INLINE short _InterlockedAnd16(volatile short *Value, short Mask)
{
    return __sync_fetch_and_and(Value, Mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedAnd8)
__INTRIN_INLINE char _InterlockedAnd8(volatile char *Value, char Mask)
{
    return __sync_fetch_and_and(Value, Mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedAnd64)
__INTRIN_INLINE long long _InterlockedAnd64(volatile long long *Value, long long Mask)
{
    return __sync_fetch_and_and(Value, Mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedOr)
__INTRIN_INLINE long _InterlockedOr(volatile long *Value, long Mask)
{
    return __sync_fetch_and_or(Value, Mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedOr16)
__INTRIN_INLINE short _InterlockedOr16(volatile short *Value, short Mask)
{
    return __sync_fetch_and_or(Value, Mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedOr8)
__INTRIN_INLINE char _InterlockedOr8(volatile char *Value, char Mask)
{
    return __sync_fetch_and_or(Value, Mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedOr64)
__INTRIN_INLINE long long _InterlockedOr64(volatile long long *Value, long long Mask)
{
    return __sync_fetch_and_or(Value, Mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedXor)
__INTRIN_INLINE long _InterlockedXor(volatile long *Value, long Mask)
{
    return __sync_fetch_and_xor(Value, Mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedXor16)
__INTRIN_INLINE short _InterlockedXor16(volatile short *Value, short Mask)
{
    return __sync_fetch_and_xor(Value, Mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedXor8)
__INTRIN_INLINE char _InterlockedXor8(volatile char *Value, char Mask)
{
    return __sync_fetch_and_xor(Value, Mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedXor64)
__INTRIN_INLINE long long _InterlockedXor64(volatile long long *Value, long long Mask)
{
    return __sync_fetch_and_xor(Value, Mask);
}
#endif

#if !HAS_BUILTIN(_InterlockedIncrement)
__INTRIN_INLINE long _InterlockedIncrement(volatile long *Addend)
{
    return __sync_add_and_fetch(Addend, 1);
}
#endif

#if !HAS_BUILTIN(_InterlockedIncrement16)
__INTRIN_INLINE short _InterlockedIncrement16(volatile short *Addend)
{
    return __sync_add_and_fetch(Addend, 1);
}
#endif

#if !HAS_BUILTIN(_InterlockedIncrement64)
__INTRIN_INLINE long long _InterlockedIncrement64(volatile long long *Addend)
{
    return __sync_add_and_fetch(Addend, 1);
}
#endif

#if !HAS_BUILTIN(_InterlockedDecrement)
__INTRIN_INLINE long _InterlockedDecrement(volatile long *Addend)
{
    return __sync_sub_and_fetch(Addend, 1);
}
#endif

#if !HAS_BUILTIN(_InterlockedDecrement16)
__INTRIN_INLINE short _InterlockedDecrement16(volatile short *Addend)
{
    return __sync_sub_and_fetch(Addend, 1);
}
#endif

#if !HAS_BUILTIN(_InterlockedDecrement64)
__INTRIN_INLINE long long _InterlockedDecrement64(volatile long long *Addend)
{
    return __sync_sub_and_fetch(Addend, 1);
}
#endif

#if !HAS_BUILTIN(_InterlockedCompareExchange128)
/* may_alias avoids strict-aliasing UB on the (__int64*) / __int128 pair. */
typedef __int128 __attribute__((may_alias)) __ms_int128_t;
__INTRIN_INLINE unsigned char _InterlockedCompareExchange128(
    volatile __int64 *Destination, __int64 ExchangeHigh,
    __int64 ExchangeLow, __int64 *ComparandResult)
{
    /* Baseline armv8-a LL/SC; matches MS full-barrier contract. */
    unsigned __int64 cmp_lo = (unsigned __int64)ComparandResult[0];
    unsigned __int64 cmp_hi = (unsigned __int64)ComparandResult[1];
    unsigned __int64 old_lo, old_hi;
    unsigned int status;
    __asm__ __volatile__(
        "1: ldaxp   %[ol], %[oh], %[mem]      \n"
        "   cmp     %[ol], %[cl]              \n"
        "   ccmp    %[oh], %[ch], #0, eq      \n"
        "   b.ne    2f                        \n"
        "   stlxp   %w[st], %[xl], %[xh], %[mem] \n"
        "   cbnz    %w[st], 1b                \n"
        "   mov     %w[st], #0                \n"
        "   b       3f                        \n"
        "2: clrex                             \n"
        "   mov     %w[st], #1                \n"
        "3:                                   \n"
        : [ol] "=&r"(old_lo), [oh] "=&r"(old_hi),
          [st] "=&r"(status),
          [mem] "+Q"(*(volatile __ms_int128_t *)Destination)
        : [cl] "r"(cmp_lo), [ch] "r"(cmp_hi),
          [xl] "r"((unsigned __int64)ExchangeLow),
          [xh] "r"((unsigned __int64)ExchangeHigh)
        : "cc", "memory");
    __asm__ __volatile__("dmb ish" ::: "memory");
    if (status == 0)
        return 1;
    ComparandResult[0] = (__int64)old_lo;
    ComparandResult[1] = (__int64)old_hi;
    return 0;
}
#endif

#if !HAS_BUILTIN(__nop)
__INTRIN_INLINE void __nop(void)
{
    __asm__ __volatile__("nop");
}
#endif

/*** Bit manipulation ***/
#if !HAS_BUILTIN(_bittest)
__INTRIN_INLINE unsigned char _bittest(const long *a, long b)
{
    return (a[b / (sizeof(long) * 8)] >> (b % (sizeof(long) * 8))) & 1;
}
#endif

#if !HAS_BUILTIN(_bittestandset)
__INTRIN_INLINE unsigned char _bittestandset(long *a, long b)
{
    long bit = 1L << (b % (sizeof(long) * 8));
    long *ptr = &a[b / (sizeof(long) * 8)];
    unsigned char retval = (*ptr >> (b % (sizeof(long) * 8))) & 1;
    *ptr |= bit;
    return retval;
}
#endif

#if !HAS_BUILTIN(_bittestandreset)
__INTRIN_INLINE unsigned char _bittestandreset(long *a, long b)
{
    long bit = 1L << (b % (sizeof(long) * 8));
    long *ptr = &a[b / (sizeof(long) * 8)];
    unsigned char retval = (*ptr >> (b % (sizeof(long) * 8))) & 1;
    *ptr &= ~bit;
    return retval;
}
#endif

#if !HAS_BUILTIN(_bittestandcomplement)
__INTRIN_INLINE unsigned char _bittestandcomplement(long *a, long b)
{
    long bit = 1L << (b % (sizeof(long) * 8));
    long *ptr = &a[b / (sizeof(long) * 8)];
    unsigned char retval = (*ptr >> (b % (sizeof(long) * 8))) & 1;
    *ptr ^= bit;
    return retval;
}
#endif

#if !HAS_BUILTIN(_bittest64)
__INTRIN_INLINE unsigned char _bittest64(const long long *a, long long b)
{
    return (a[b / 64] >> (b % 64)) & 1;
}
#endif

#if !HAS_BUILTIN(_bittestandset64)
__INTRIN_INLINE unsigned char _bittestandset64(long long *a, long long b)
{
    long long bit = 1LL << (b % 64);
    long long *ptr = &a[b / 64];
    unsigned char retval = (*ptr >> (b % 64)) & 1;
    *ptr |= bit;
    return retval;
}
#endif

#if !HAS_BUILTIN(_bittestandreset64)
__INTRIN_INLINE unsigned char _bittestandreset64(long long *a, long long b)
{
    long long bit = 1LL << (b % 64);
    long long *ptr = &a[b / 64];
    unsigned char retval = (*ptr >> (b % 64)) & 1;
    *ptr &= ~bit;
    return retval;
}
#endif

#if !HAS_BUILTIN(_bittestandcomplement64)
__INTRIN_INLINE unsigned char _bittestandcomplement64(long long *a, long long b)
{
    long long bit = 1LL << (b % 64);
    long long *ptr = &a[b / 64];
    unsigned char retval = (*ptr >> (b % 64)) & 1;
    *ptr ^= bit;
    return retval;
}
#endif

/*** Interlocked bit test ***/
#if !HAS_BUILTIN(_interlockedbittestandreset)
__INTRIN_INLINE unsigned char _interlockedbittestandreset(volatile long *a, long b)
{
    unsigned int bit = b & 31;
    long mask = 1L << bit;
    long old = __sync_fetch_and_and(a, ~mask);
    return (unsigned char)((old >> bit) & 1);
}
#endif

#if !HAS_BUILTIN(_interlockedbittestandset)
__INTRIN_INLINE unsigned char _interlockedbittestandset(volatile long *a, long b)
{
    unsigned int bit = b & 31;
    long mask = 1L << bit;
    long old = __sync_fetch_and_or(a, mask);
    return (unsigned char)((old >> bit) & 1);
}
#endif

#if !HAS_BUILTIN(_interlockedbittestandreset64)
__INTRIN_INLINE unsigned char _interlockedbittestandreset64(volatile long long *a, long long b)
{
    unsigned int bit = b & 63;
    long long mask = 1LL << bit;
    long long old = __sync_fetch_and_and(a, ~mask);
    return (unsigned char)((old >> bit) & 1);
}
#endif

#if !HAS_BUILTIN(_interlockedbittestandset64)
__INTRIN_INLINE unsigned char _interlockedbittestandset64(volatile long long *a, long long b)
{
    unsigned int bit = b & 63;
    long long mask = 1LL << bit;
    long long old = __sync_fetch_and_or(a, mask);
    return (unsigned char)((old >> bit) & 1);
}
#endif

/*** Rotates ***/
#if !HAS_BUILTIN(_rotl8)
__INTRIN_INLINE unsigned char __cdecl _rotl8(unsigned char value, unsigned char shift)
{
    shift &= 7;
    if (!shift)
        return value;
    return (value << shift) | (value >> (8 - shift));
}
#endif

#if !HAS_BUILTIN(_rotl16)
__INTRIN_INLINE unsigned short __cdecl _rotl16(unsigned short value, unsigned char shift)
{
    shift &= 15;
    if (!shift)
        return value;
    return (value << shift) | (value >> (16 - shift));
}
#endif

#if !HAS_BUILTIN(_rotl)
__INTRIN_INLINE unsigned int __cdecl _rotl(unsigned int value, int shift)
{
    shift &= 31;
    if (!shift)
        return value;
    return (value << shift) | (value >> (32 - shift));
}
#endif

#if !HAS_BUILTIN(_rotl64)
__INTRIN_INLINE unsigned long long _rotl64(unsigned long long value, int shift)
{
    shift &= 63;
    if (!shift)
        return value;
    return (value << shift) | (value >> (64 - shift));
}
#endif

#if !HAS_BUILTIN(_rotr8)
__INTRIN_INLINE unsigned char __cdecl _rotr8(unsigned char value, unsigned char shift)
{
    shift &= 7;
    if (!shift)
        return value;
    return (value >> shift) | (value << (8 - shift));
}
#endif

#if !HAS_BUILTIN(_rotr16)
__INTRIN_INLINE unsigned short __cdecl _rotr16(unsigned short value, unsigned char shift)
{
    shift &= 15;
    if (!shift)
        return value;
    return (value >> shift) | (value << (16 - shift));
}
#endif

#if !HAS_BUILTIN(_rotr)
__INTRIN_INLINE unsigned int __cdecl _rotr(unsigned int value, int shift)
{
    shift &= 31;
    if (!shift)
        return value;
    return (value >> shift) | (value << (32 - shift));
}
#endif

#if !HAS_BUILTIN(_rotr64)
__INTRIN_INLINE unsigned long long _rotr64(unsigned long long value, int shift)
{
    shift &= 63;
    if (!shift)
        return value;
    return (value >> shift) | (value << (64 - shift));
}
#endif

#if !HAS_BUILTIN(_lrotl)
__INTRIN_INLINE unsigned long __cdecl _lrotl(unsigned long value, int shift)
{
    shift &= 31;
    if (!shift)
        return value;
    return (value << shift) | (value >> (32 - shift));
}
#endif

#if !HAS_BUILTIN(_lrotr)
__INTRIN_INLINE unsigned long __cdecl _lrotr(unsigned long value, int shift)
{
    shift &= 31;
    if (!shift)
        return value;
    return (value >> shift) | (value << (32 - shift));
}
#endif

/*** Bit count ***/
#if !HAS_BUILTIN(_CountLeadingZeros)
__INTRIN_INLINE unsigned int _CountLeadingZeros(unsigned long value)
{
    if (!value)
        return sizeof(value) * 8;
    return __builtin_clzl(value);
}
#endif

#if !HAS_BUILTIN(_CountLeadingZeros64)
__INTRIN_INLINE unsigned int _CountLeadingZeros64(unsigned __int64 value)
{
    if (!value)
        return 64;
    return __builtin_clzll(value);
}
#endif

#if !HAS_BUILTIN(_CountOneBits)
__INTRIN_INLINE unsigned int _CountOneBits(unsigned long value)
{
    return __builtin_popcountl(value);
}
#endif

#if !HAS_BUILTIN(_CountOneBits64)
__INTRIN_INLINE unsigned int _CountOneBits64(unsigned __int64 value)
{
    return __builtin_popcountll(value);
}
#endif

/*** 64-bit math ***/
#if !HAS_BUILTIN(__mulh)
__INTRIN_INLINE long long __mulh(long long a, long long b)
{
    return ((__int128)a * (__int128)b) >> 64;
}
#endif

#if !HAS_BUILTIN(__umulh)
__INTRIN_INLINE unsigned long long __umulh(unsigned long long a, unsigned long long b)
{
    return ((unsigned __int128)a * (unsigned __int128)b) >> 64;
}
#endif

#if !HAS_BUILTIN(_abs64)
__INTRIN_INLINE long long __cdecl _abs64(long long value)
{
    return (value >= 0) ? value : -value;
}
#endif

/*** ARM64 hints and barriers ***/
#if !HAS_BUILTIN(__yield)
/* YIELD is a pure scheduling hint; no memory ordering per MS docs. */
__INTRIN_INLINE void __yield(void)
{
    __asm__ __volatile__("yield");
}
#endif

#if !HAS_BUILTIN(__dmb)
__INTRIN_INLINE void __dmb(unsigned int type)
{
    /* MS _ARM64_BARRIER_* constants: OSHLD=1 OSHST=2 OSH=3 NSHLD=5 NSHST=6
       NSH=7 ISHLD=9 ISHST=10 ISH=11 LD=13 ST=14 SY=15. The dmb option must
       be an assembler literal, so we dispatch on the runtime value. */
    switch (type)
    {
    case 1:  __asm__ __volatile__("dmb oshld" ::: "memory"); break;
    case 2:  __asm__ __volatile__("dmb oshst" ::: "memory"); break;
    case 3:  __asm__ __volatile__("dmb osh"   ::: "memory"); break;
    case 5:  __asm__ __volatile__("dmb nshld" ::: "memory"); break;
    case 6:  __asm__ __volatile__("dmb nshst" ::: "memory"); break;
    case 7:  __asm__ __volatile__("dmb nsh"   ::: "memory"); break;
    case 9:  __asm__ __volatile__("dmb ishld" ::: "memory"); break;
    case 10: __asm__ __volatile__("dmb ishst" ::: "memory"); break;
    case 11: __asm__ __volatile__("dmb ish"   ::: "memory"); break;
    case 13: __asm__ __volatile__("dmb ld"    ::: "memory"); break;
    case 14: __asm__ __volatile__("dmb st"    ::: "memory"); break;
    default: __asm__ __volatile__("dmb sy"    ::: "memory"); break;
    }
}
#endif

#if !HAS_BUILTIN(__dsb)
__INTRIN_INLINE void __dsb(unsigned int type)
{
    switch (type)
    {
    case 1:  __asm__ __volatile__("dsb oshld" ::: "memory"); break;
    case 2:  __asm__ __volatile__("dsb oshst" ::: "memory"); break;
    case 3:  __asm__ __volatile__("dsb osh"   ::: "memory"); break;
    case 5:  __asm__ __volatile__("dsb nshld" ::: "memory"); break;
    case 6:  __asm__ __volatile__("dsb nshst" ::: "memory"); break;
    case 7:  __asm__ __volatile__("dsb nsh"   ::: "memory"); break;
    case 9:  __asm__ __volatile__("dsb ishld" ::: "memory"); break;
    case 10: __asm__ __volatile__("dsb ishst" ::: "memory"); break;
    case 11: __asm__ __volatile__("dsb ish"   ::: "memory"); break;
    case 13: __asm__ __volatile__("dsb ld"    ::: "memory"); break;
    case 14: __asm__ __volatile__("dsb st"    ::: "memory"); break;
    default: __asm__ __volatile__("dsb sy"    ::: "memory"); break;
    }
}
#endif

#if !HAS_BUILTIN(__isb)
__INTRIN_INLINE void __isb(unsigned int type)
{
    /* ISB on ARM64 only supports the 'sy' option; accept the type arg
       for API compatibility with MS docs and ignore it. */
    (void)type;
    __asm__ __volatile__("isb" ::: "memory");
}
#endif

/* _ReadStatusReg/_WriteStatusReg and __setReg intentionally omitted: they take
   a 15-bit encoded sysreg op0:op1:CRn:CRm:op2 immediate. Callers in HAL/KE
   should use named MSR/MRS asm wrappers instead. */

#if !HAS_BUILTIN(__getReg)
#define __getReg(reg) \
    __extension__({ \
        _Static_assert((unsigned long long)(reg) <= 31ULL, \
                       "__getReg: register index out of range"); \
        unsigned __int64 _value; \
        __asm__ __volatile__("mov %0, x%c1" : "=r"(_value) : "i"(reg)); \
        _value; \
    })
#endif

/*** _InterlockedAdd (returns the new, post-add value) ***/
#if !HAS_BUILTIN(_InterlockedAdd)
__INTRIN_INLINE long _InterlockedAdd(volatile long *Addend, long Value)
{
    return __atomic_add_fetch(Addend, Value, __ATOMIC_SEQ_CST);
}
#endif

#if !HAS_BUILTIN(_InterlockedAdd64)
__INTRIN_INLINE long long _InterlockedAdd64(volatile long long *Addend, long long Value)
{
    return __atomic_add_fetch(Addend, Value, __ATOMIC_SEQ_CST);
}
#endif

/*** TEB access via x18 (Windows ARM64 ABI dedicates x18 to the TEB).
 *** Requires the toolchain to reserve x18 (default for aarch64-w64-mingw32;
 *** add -ffixed-x18 on bare aarch64 targets if ever building elsewhere). ***/
#if !HAS_BUILTIN(__readx18byte)
__INTRIN_INLINE unsigned char __readx18byte(unsigned long offset)
{
    unsigned char value;
    __asm__ __volatile__("ldrb %w0, [x18, %1]" : "=r"(value) : "r"((unsigned long long)offset));
    return value;
}
#endif

#if !HAS_BUILTIN(__readx18word)
__INTRIN_INLINE unsigned short __readx18word(unsigned long offset)
{
    unsigned short value;
    __asm__ __volatile__("ldrh %w0, [x18, %1]" : "=r"(value) : "r"((unsigned long long)offset));
    return value;
}
#endif

#if !HAS_BUILTIN(__readx18dword)
__INTRIN_INLINE unsigned long __readx18dword(unsigned long offset)
{
    unsigned long value;
    __asm__ __volatile__("ldr %w0, [x18, %1]" : "=r"(value) : "r"((unsigned long long)offset));
    return value;
}
#endif

#if !HAS_BUILTIN(__readx18qword)
__INTRIN_INLINE unsigned __int64 __readx18qword(unsigned long offset)
{
    unsigned __int64 value;
    __asm__ __volatile__("ldr %0, [x18, %1]" : "=r"(value) : "r"((unsigned long long)offset));
    return value;
}
#endif

#if !HAS_BUILTIN(__writex18byte)
__INTRIN_INLINE void __writex18byte(unsigned long offset, unsigned char value)
{
    __asm__ __volatile__("strb %w1, [x18, %0]" : : "r"((unsigned long long)offset), "r"(value) : "memory");
}
#endif

#if !HAS_BUILTIN(__writex18word)
__INTRIN_INLINE void __writex18word(unsigned long offset, unsigned short value)
{
    __asm__ __volatile__("strh %w1, [x18, %0]" : : "r"((unsigned long long)offset), "r"(value) : "memory");
}
#endif

#if !HAS_BUILTIN(__writex18dword)
__INTRIN_INLINE void __writex18dword(unsigned long offset, unsigned long value)
{
    __asm__ __volatile__("str %w1, [x18, %0]" : : "r"((unsigned long long)offset), "r"(value) : "memory");
}
#endif

#if !HAS_BUILTIN(__writex18qword)
__INTRIN_INLINE void __writex18qword(unsigned long offset, unsigned __int64 value)
{
    __asm__ __volatile__("str %1, [x18, %0]" : : "r"((unsigned long long)offset), "r"(value) : "memory");
}
#endif

#endif
