/*
    Compatibility <intrin.h> header for GCC/Clang on ARM64 Windows.
*/

#ifndef KJK_INTRIN_ARM64_H_
#define KJK_INTRIN_ARM64_H_

#ifndef __GNUC__
#error Unsupported compiler
#endif

#define _ReturnAddress() (__builtin_return_address(0))

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
    return __sync_lock_test_and_set(Target, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange16)
__INTRIN_INLINE short _InterlockedExchange16(volatile short *Target, short Value)
{
    return __sync_lock_test_and_set(Target, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange8)
__INTRIN_INLINE char _InterlockedExchange8(volatile char *Target, char Value)
{
    return __sync_lock_test_and_set(Target, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchange64)
__INTRIN_INLINE long long _InterlockedExchange64(volatile long long *Target, long long Value)
{
    return __sync_lock_test_and_set(Target, Value);
}
#endif

#if !HAS_BUILTIN(_InterlockedExchangePointer)
__INTRIN_INLINE void *_InterlockedExchangePointer(void *volatile *Target, void *Value)
{
    return __sync_lock_test_and_set(Target, Value);
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

#endif
