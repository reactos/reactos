#ifndef KJK_INTRIN_H_
#define KJK_INTRIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/*** Stack frame juggling ***/
void * _ReturnAddress(void);
#pragma intrinsic(_ReturnAddress)
void * _AddressOfReturnAddress(void);
#pragma intrinsic(_AddressOfReturnAddress)
#if defined(_M_IX86) || defined(_M_AMD64)
unsigned int __getcallerseflags(void);
#pragma intrinsic(__getcallerseflags)
#endif

/*** Memory barriers ***/
void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadWriteBarrier)
void _ReadBarrier(void);
#pragma intrinsic(_ReadBarrier)
void _WriteBarrier(void);
#pragma intrinsic(_WriteBarrier)
#if defined(_M_IX86) || defined(_M_AMD64)
void _mm_mfence(void);
#pragma intrinsic(_mm_mfence)
void _mm_lfence(void);
#pragma intrinsic(_mm_lfence)
void _mm_sfence(void);
#pragma intrinsic(_mm_sfence)
void __nvreg_restore_fence(void);
void __nvreg_save_fence(void);
#endif
#if defined(_M_AMD64)
void __faststorefence(void);
#pragma intrinsic(__faststorefence)
#elif defined(_M_ARM)
__int16 __iso_volatile_load16(const volatile __int16 *);
#pragma intrinsic(__iso_volatile_load16)
__int32 __iso_volatile_load32(const volatile __int32 *);
#pragma intrinsic(__iso_volatile_load32)
__int64 __iso_volatile_load64(const volatile __int64 *);
#pragma intrinsic(__iso_volatile_load64)
__int8 __iso_volatile_load8(const volatile __int8 *);
#pragma intrinsic(__iso_volatile_load8)
void __iso_volatile_store16(volatile __int16 *, __int16);
#pragma intrinsic(__iso_volatile_store16)
void __iso_volatile_store32(volatile __int32 *, __int32);
#pragma intrinsic(__iso_volatile_store32)
void __iso_volatile_store64(volatile __int64 *, __int64);
#pragma intrinsic(__iso_volatile_store64)
void __iso_volatile_store8(volatile __int8 *, __int8);
#pragma intrinsic(__iso_volatile_store8)
#endif

/*** Atomic operations ***/
long _InterlockedCompareExchange(_Interlocked_operand_ volatile long * Destination, long Exchange, long Comperand);
#pragma intrinsic(_InterlockedCompareExchange)
char _InterlockedCompareExchange8(_Interlocked_operand_ char volatile * Destination, char Exchange, char Comparand);
#pragma intrinsic(_InterlockedCompareExchange8)
short _InterlockedCompareExchange16(_Interlocked_operand_ short volatile * Destination, short Exchange, short Comparand);
#pragma intrinsic(_InterlockedCompareExchange16)
__int64 _InterlockedCompareExchange64(_Interlocked_operand_ volatile __int64 * Destination, __int64 Exchange, __int64 Comperand);
#pragma intrinsic(_InterlockedCompareExchange64)
long _InterlockedExchange(_Interlocked_operand_ volatile long * Target, long Value);
#pragma intrinsic(_InterlockedExchange)
char _InterlockedExchange8(_Interlocked_operand_ char volatile * Target, char Value);
#pragma intrinsic(_InterlockedExchange8)
short _InterlockedExchange16(_Interlocked_operand_ short volatile * Target, short Value);
#pragma intrinsic(_InterlockedExchange16)
long _InterlockedExchangeAdd(_Interlocked_operand_ volatile long * Addend, long Value);
#pragma intrinsic(_InterlockedExchangeAdd)
char _InterlockedExchangeAdd8(_Interlocked_operand_ char volatile * Addend, char Value);
#pragma intrinsic(_InterlockedExchangeAdd8)
short _InterlockedExchangeAdd16(_Interlocked_operand_ short volatile * Addend, short Value);
#pragma intrinsic(_InterlockedExchangeAdd16)
char _InterlockedAnd8(_Interlocked_operand_ volatile char * value, char mask);
#pragma intrinsic(_InterlockedAnd8)
short _InterlockedAnd16(_Interlocked_operand_ volatile short * value, short mask);
#pragma intrinsic(_InterlockedAnd16)
long _InterlockedAnd(_Interlocked_operand_ volatile long * value, long mask);
#pragma intrinsic(_InterlockedAnd)
char _InterlockedOr8(_Interlocked_operand_ volatile char * value, char mask);
#pragma intrinsic(_InterlockedOr8)
short _InterlockedOr16(_Interlocked_operand_ volatile short * value, short mask);
#pragma intrinsic(_InterlockedOr16)
long _InterlockedOr(_Interlocked_operand_ volatile long * value, long mask);
#pragma intrinsic(_InterlockedOr)
char _InterlockedXor8(_Interlocked_operand_ volatile char * value, char mask);
#pragma intrinsic(_InterlockedXor8)
short _InterlockedXor16(_Interlocked_operand_ volatile short * value, short mask);
#pragma intrinsic(_InterlockedXor16)
long _InterlockedXor(_Interlocked_operand_ volatile long * value, long mask);
#pragma intrinsic(_InterlockedXor)
long _InterlockedDecrement(_Interlocked_operand_ volatile long * lpAddend);
#pragma intrinsic(_InterlockedDecrement)
long _InterlockedIncrement(_Interlocked_operand_ volatile long * lpAddend);
#pragma intrinsic(_InterlockedIncrement)
short _InterlockedDecrement16(_Interlocked_operand_ volatile short * lpAddend);
#pragma intrinsic(_InterlockedDecrement16)
short _InterlockedIncrement16(_Interlocked_operand_ volatile short * lpAddend);
#pragma intrinsic(_InterlockedIncrement16)
unsigned char _interlockedbittestandreset(volatile long * a, long b);
#pragma intrinsic(_interlockedbittestandreset)
unsigned char _interlockedbittestandset(volatile long * a, long b);
#pragma intrinsic(_interlockedbittestandset)
#if defined(_M_IX86)
long _InterlockedAddLargeStatistic(_Interlocked_operand_ volatile __int64 * Addend, long Value);
#pragma intrinsic(_InterlockedAddLargeStatistic)
#elif defined(_M_AMD64)
__int64 _InterlockedExchange64(volatile __int64 * Target, __int64 Value);
#pragma intrinsic(_InterlockedExchange64)
__int64 _InterlockedExchangeAdd64(volatile __int64 * Addend, __int64 Value);
#pragma intrinsic(_InterlockedExchangeAdd64)
void * _InterlockedCompareExchangePointer(void * volatile * Destination, void * Exchange, void * Comperand);
#pragma intrinsic(_InterlockedCompareExchangePointer)
void * _InterlockedExchangePointer(void * volatile * Target, void * Value);
#pragma intrinsic(_InterlockedExchangePointer)
unsigned char _InterlockedCompareExchange128(_Interlocked_operand_ volatile __int64 * Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64 * ComparandResult);
#pragma intrinsic(_InterlockedCompareExchange128)
__int64 _InterlockedAnd64(volatile __int64 * value, __int64 mask);
#pragma intrinsic(_InterlockedAnd64)
__int64 _InterlockedOr64(volatile __int64 * value, __int64 mask);
#pragma intrinsic(_InterlockedOr64)
__int64 _InterlockedDecrement64(volatile __int64 * lpAddend);
#pragma intrinsic(_InterlockedDecrement64)
__int64 _InterlockedIncrement64(volatile __int64 * lpAddend);
#pragma intrinsic(_InterlockedIncrement64)
unsigned char _interlockedbittestandreset64(volatile __int64 * a, __int64 b);
#pragma intrinsic(_interlockedbittestandreset64)
unsigned char _interlockedbittestandset64(volatile __int64 * a, __int64 b);
#pragma intrinsic(_interlockedbittestandset64)
long _InterlockedAnd_np(volatile long * Value, long Mask);
#pragma intrinsic(_InterlockedAnd_np)
char _InterlockedAnd8_np(volatile char * Value, char Mask);
#pragma intrinsic(_InterlockedAnd8_np)
short _InterlockedAnd16_np(volatile short * Value, short Mask);
#pragma intrinsic(_InterlockedAnd16_np)
__int64 _InterlockedAnd64_np(volatile __int64 * Value, __int64 Mask);
#pragma intrinsic(_InterlockedAnd64_np)
short _InterlockedCompareExchange16_np(volatile short * Destination, short Exchange, short Comparand);
#pragma intrinsic(_InterlockedCompareExchange16_np)
__int64 _InterlockedCompareExchange64_np(volatile __int64 * Destination, __int64 Exchange, __int64 Comparand);
#pragma intrinsic(_InterlockedCompareExchange64_np)
unsigned char _InterlockedCompareExchange128_np(volatile __int64 * Destination, __int64 ExchangeHigh, __int64 ExchangeLow, __int64 * ComparandResult);
#pragma intrinsic(_InterlockedCompareExchange128_np)
void * _InterlockedCompareExchangePointer_np(void * volatile * Destination, void * Exchange, void * Comparand);
#pragma intrinsic(_InterlockedCompareExchangePointer_np)
long _InterlockedCompareExchange_np(volatile long * Destination, long Exchange, long Comparand);
#pragma intrinsic(_InterlockedCompareExchange_np)
short _InterlockedOr16_np(volatile short * Value, short Mask);
#pragma intrinsic(_InterlockedOr16_np)
char _InterlockedOr8_np(volatile char * Value, char Mask);
#pragma intrinsic(_InterlockedOr8_np)
long _InterlockedOr_np(volatile long * Value, long Mask);
#pragma intrinsic(_InterlockedOr_np)
short _InterlockedXor16_np(volatile short * Value, short Mask);
#pragma intrinsic(_InterlockedXor16_np)
__int64 _InterlockedXor64_np(volatile __int64 * Value, __int64 Mask);
#pragma intrinsic(_InterlockedXor64_np)
char _InterlockedXor8_np(volatile char * Value, char Mask);
#pragma intrinsic(_InterlockedXor8_np)
long _InterlockedXor_np(volatile long * Value, long Mask);
#pragma intrinsic(_InterlockedXor_np)
__int64 _InterlockedOr64_np(volatile __int64 * Value, __int64 Mask);
#pragma intrinsic(_InterlockedOr64_np)
#elif defined(_M_ARM)
long _InterlockedAdd(_Interlocked_operand_ long volatile * Addend, long Value);
__int64 _InterlockedAdd64(_Interlocked_operand_ __int64 volatile * Addend, __int64 Value);
__int64 _InterlockedAdd64_acq(__int64 volatile * Addend, __int64 Value);
__int64 _InterlockedAdd64_nf(__int64 volatile * Addend, __int64 Value);
__int64 _InterlockedAdd64_rel(__int64 volatile * Addend, __int64 Value);
long _InterlockedAdd_acq(long volatile * Addend, long Value);
long _InterlockedAdd_nf(long volatile * Addend, long Value);
long _InterlockedAdd_rel(long volatile * Addend, long Value);
short _InterlockedAnd16_acq(short volatile * Value, short Mask);
short _InterlockedAnd16_nf(short volatile * Value, short Mask);
short _InterlockedAnd16_rel(short volatile * Value, short Mask);
__int64 _InterlockedAnd64_acq(__int64 volatile * Value, __int64 Mask);
__int64 _InterlockedAnd64_nf(__int64 volatile * Value, __int64 Mask);
__int64 _InterlockedAnd64_rel(__int64 volatile * Value, __int64 Mask);
char _InterlockedAnd8_acq(char volatile * Value, char Mask);
char _InterlockedAnd8_nf(char volatile * Value, char Mask);
char _InterlockedAnd8_rel(char volatile * Value, char Mask);
long _InterlockedAnd_acq(long volatile * Value, long Mask);
long _InterlockedAnd_nf(long volatile * Value, long Mask);
long _InterlockedAnd_rel(long volatile * Value, long Mask);
short _InterlockedCompareExchange16_acq(short volatile * Destination, short Exchange, short Comparand);
short _InterlockedCompareExchange16_nf(short volatile * Destination, short Exchange, short Comparand);
short _InterlockedCompareExchange16_rel(short volatile * Destination, short Exchange, short Comparand);
__int64 _InterlockedCompareExchange64_acq(__int64 volatile * Destination, __int64 Exchange, __int64 Comparand);
__int64 _InterlockedCompareExchange64_nf(__int64 volatile * Destination, __int64 Exchange, __int64 Comparand);
__int64 _InterlockedCompareExchange64_rel(__int64 volatile * Destination, __int64 Exchange, __int64 Comparand);
char _InterlockedCompareExchange8_acq(char volatile * Destination, char Exchange, char Comparand);
char _InterlockedCompareExchange8_nf(char volatile * Destination, char Exchange, char Comparand);
char _InterlockedCompareExchange8_rel(char volatile * Destination, char Exchange, char Comparand);
void * _InterlockedCompareExchangePointer_acq(void * volatile * Destination, void * Exchange, void * Comparand);
void * _InterlockedCompareExchangePointer_nf(void * volatile * Destination, void * Exchange, void * Comparand);
void * _InterlockedCompareExchangePointer_rel(void * volatile * Destination, void * Exchange, void * Comparand);
long _InterlockedCompareExchange_acq(long volatile * Destination, long Exchange, long Comparand);
long _InterlockedCompareExchange_nf(long volatile * Destination, long Exchange, long Comparand);
long _InterlockedCompareExchange_rel(long volatile * Destination, long Exchange, long Comparand);
short _InterlockedDecrement16_acq(short volatile * Addend);
short _InterlockedDecrement16_nf(short volatile * Addend);
short _InterlockedDecrement16_rel(short volatile * Addend);
__int64 _InterlockedDecrement64_acq(__int64 volatile * Addend);
__int64 _InterlockedDecrement64_nf(__int64 volatile * Addend);
__int64 _InterlockedDecrement64_rel(__int64 volatile * Addend);
long _InterlockedDecrement_acq(long volatile * Addend);
long _InterlockedDecrement_nf(long volatile * Addend);
long _InterlockedDecrement_rel(long volatile * Addend);
short _InterlockedExchange16_acq(short volatile * Target, short Value);
short _InterlockedExchange16_nf(short volatile * Target, short Value);
__int64 _InterlockedExchange64_acq(__int64 volatile * Target, __int64 Value);
__int64 _InterlockedExchange64_nf(__int64 volatile * Target, __int64 Value);
char _InterlockedExchange8_acq(char volatile * Target, char Value);
char _InterlockedExchange8_nf(char volatile * Target, char Value);
short _InterlockedExchangeAdd16_acq(short volatile * Addend, short Value);
short _InterlockedExchangeAdd16_nf(short volatile * Addend, short Value);
short _InterlockedExchangeAdd16_rel(short volatile * Addend, short Value);
__int64 _InterlockedExchangeAdd64_acq(__int64 volatile * Addend, __int64 Value);
__int64 _InterlockedExchangeAdd64_nf(__int64 volatile * Addend, __int64 Value);
__int64 _InterlockedExchangeAdd64_rel(__int64 volatile * Addend, __int64 Value);
char _InterlockedExchangeAdd8_acq(char volatile * Addend, char Value);
char _InterlockedExchangeAdd8_nf(char volatile * Addend, char Value);
char _InterlockedExchangeAdd8_rel(char volatile * Addend, char Value);
long _InterlockedExchangeAdd_acq(long volatile * Addend, long Value);
long _InterlockedExchangeAdd_nf(long volatile * Addend, long Value);
long _InterlockedExchangeAdd_rel(long volatile * Addend, long Value);
void * _InterlockedExchangePointer_acq(void * volatile * Target, void * Value);
void * _InterlockedExchangePointer_nf(void * volatile * Target, void * Value);
long _InterlockedExchange_acq(long volatile * Target, long Value);
long _InterlockedExchange_nf(long volatile * Target, long Value);
short _InterlockedIncrement16_acq(short volatile * Addend);
short _InterlockedIncrement16_nf(short volatile * Addend);
short _InterlockedIncrement16_rel(short volatile * Addend);
__int64 _InterlockedIncrement64_acq(__int64 volatile * Addend);
__int64 _InterlockedIncrement64_nf(__int64 volatile * Addend);
__int64 _InterlockedIncrement64_rel(__int64 volatile * Addend);
long _InterlockedIncrement_acq(long volatile * Addend);
long _InterlockedIncrement_nf(long volatile * Addend);
long _InterlockedIncrement_rel(long volatile * Addend);
short _InterlockedOr16_acq(short volatile * Value, short Mask);
short _InterlockedOr16_nf(short volatile * Value, short Mask);
short _InterlockedOr16_rel(short volatile * Value, short Mask);
__int64 _InterlockedOr64_acq(__int64 volatile * Value, __int64 Mask);
__int64 _InterlockedOr64_nf(__int64 volatile * Value, __int64 Mask);
__int64 _InterlockedOr64_rel(__int64 volatile * Value, __int64 Mask);
char _InterlockedOr8_acq(char volatile * Value, char Mask);
char _InterlockedOr8_nf(char volatile * Value, char Mask);
char _InterlockedOr8_rel(char volatile * Value, char Mask);
long _InterlockedOr_acq(long volatile * Value, long Mask);
long _InterlockedOr_nf(long volatile * Value, long Mask);
long _InterlockedOr_rel(long volatile * Value, long Mask);
short _InterlockedXor16_acq(short volatile * Value, short Mask);
short _InterlockedXor16_nf(short volatile * Value, short Mask);
short _InterlockedXor16_rel(short volatile * Value, short Mask);
__int64 _InterlockedXor64_acq(__int64 volatile * Value, __int64 Mask);
__int64 _InterlockedXor64_nf(__int64 volatile * Value, __int64 Mask);
__int64 _InterlockedXor64_rel(_Interlocked_operand_ __int64 volatile * Value, __int64 Mask);
char _InterlockedXor8_acq(char volatile * Value, char Mask);
char _InterlockedXor8_nf(char volatile * Value, char Mask);
char _InterlockedXor8_rel(char volatile * Value, char Mask);
long _InterlockedXor_acq(long volatile * Value, long Mask);
long _InterlockedXor_nf(long volatile * Value, long Mask);
long _InterlockedXor_rel(long volatile * Value, long Mask);
unsigned char _interlockedbittestandreset_acq(long volatile *, long);
unsigned char _interlockedbittestandreset_nf(long volatile *, long);
unsigned char _interlockedbittestandreset_rel(long volatile *, long);
unsigned char _interlockedbittestandset_acq(long volatile *, long);
unsigned char _interlockedbittestandset_nf(long volatile *, long);
unsigned char _interlockedbittestandset_rel(long volatile *, long);
#endif
#if defined(_M_AMD64) || defined(_M_ARM)
__int64 _InterlockedAnd64(_Interlocked_operand_ __int64 volatile * Value, __int64 Mask);
void * _InterlockedCompareExchangePointer(_Interlocked_operand_ void * volatile * Destination, void * Exchange, void * Comparand);
__int64 _InterlockedDecrement64(_Interlocked_operand_ __int64 volatile * Addend);
__int64 _InterlockedExchange64(_Interlocked_operand_ __int64 volatile * Target, __int64 Value);
__int64 _InterlockedExchangeAdd64(_Interlocked_operand_ __int64 volatile * Addend, __int64 Value);
void * _InterlockedExchangePointer(_Interlocked_operand_ void * volatile * Target, void * Value);
__int64 _InterlockedIncrement64(_Interlocked_operand_ __int64 volatile * Addend);
__int64 _InterlockedOr64(_Interlocked_operand_ __int64 volatile * Value, __int64 Mask);
__int64 _InterlockedXor64(_Interlocked_operand_ __int64 volatile * Value, __int64 Mask);
#endif

/*** String operations ***/
#if defined(_M_IX86) || defined(_M_AMD64)
void __stosb(unsigned char * Dest, unsigned char Data, size_t Count);
#pragma intrinsic(__stosb)
void __stosw(unsigned short * Dest, unsigned short Data, size_t Count);
#pragma intrinsic(__stosw)
void __stosd(unsigned long * Dest, unsigned long Data, size_t Count);
#pragma intrinsic(__stosd)
void __movsb(unsigned char * Destination, unsigned char const * Source, size_t Count);
#pragma intrinsic(__movsb)
void __movsw(unsigned short * Destination, unsigned short const * Source, size_t Count);
#pragma intrinsic(__movsw)
void __movsd(unsigned long * Destination, unsigned long const * Source, size_t Count);
#pragma intrinsic(__movsd)
#endif
#ifdef _M_AMD64
void __stosq(unsigned __int64 * Dest, unsigned __int64 Data, size_t Count);
#pragma intrinsic(__stosq)
void __movsq(unsigned __int64 * Destination, unsigned __int64 const * Source, size_t Count);
#pragma intrinsic(__movsq)
#endif

/*** GS segment addressing ***/
#if defined(_M_AMD64)
void __writegsbyte(unsigned long Offset, unsigned char Data);
#pragma intrinsic(__writegsbyte)
void __writegsword(unsigned long Offset, unsigned short Data);
#pragma intrinsic(__writegsword)
void __writegsdword(unsigned long Offset, unsigned long Data);
#pragma intrinsic(__writegsdword)
void __writegsqword(unsigned long Offset, unsigned __int64 Data);
#pragma intrinsic(__writegsqword)
unsigned char __readgsbyte(unsigned long Offset);
#pragma intrinsic(__readgsbyte)
unsigned short __readgsword(unsigned long Offset);
#pragma intrinsic(__readgsword)
unsigned long __readgsdword(unsigned long Offset);
#pragma intrinsic(__readgsdword)
unsigned __int64 __readgsqword(unsigned long Offset);
#pragma intrinsic(__readgsqword)
void __incgsbyte(unsigned long Offset);
#pragma intrinsic(__incgsbyte)
void __incgsword(unsigned long Offset);
#pragma intrinsic(__incgsword)
void __incgsdword(unsigned long Offset);
#pragma intrinsic(__incgsdword)
void __incgsqword(unsigned long);
#pragma intrinsic(__incgsqword)
void __addgsbyte(unsigned long Offset, unsigned char Data);
#pragma intrinsic(__addgsbyte)
void __addgsword(unsigned long Offset, unsigned short Data);
#pragma intrinsic(__addgsword)
void __addgsdword(unsigned long Offset, unsigned long Data);
#pragma intrinsic(__addgsdword)
void __addgsqword(unsigned long Offset, unsigned __int64 Data);
#pragma intrinsic(__addgsqword)
#endif

/*** FS segment addressing ***/
#if defined(_M_IX86)
void __writefsbyte(unsigned long Offset, unsigned char Data);
#pragma intrinsic(__writefsbyte)
void __writefsword(unsigned long Offset, unsigned short Data);
#pragma intrinsic(__writefsword)
void __writefsdword(unsigned long Offset, unsigned long Data);
#pragma intrinsic(__writefsdword)
void __writefsqword(unsigned long Offset, unsigned __int64 Data);
#pragma intrinsic(__writefsdword)
unsigned char __readfsbyte(unsigned long Offset);
#pragma intrinsic(__readfsbyte)
unsigned short __readfsword(unsigned long Offset);
#pragma intrinsic(__readfsword)
unsigned long __readfsdword(unsigned long Offset);
#pragma intrinsic(__readfsdword)
void __incfsbyte(unsigned long Offset);
#pragma intrinsic(__incfsbyte)
void __incfsword(unsigned long Offset);
#pragma intrinsic(__incfsword)
void __incfsdword(unsigned long Offset);
#pragma intrinsic(__incfsdword)
void __addfsbyte(unsigned long Offset, unsigned char Data);
#pragma intrinsic(__addfsbyte)
void __addfsword(unsigned long Offset, unsigned short Data);
#pragma intrinsic(__addfsword)
void __addfsdword(unsigned long Offset, unsigned long Data);
#pragma intrinsic(__addfsdword)
#endif

/*** Bit manipulation ***/
unsigned char _BitScanForward(unsigned long * Index, unsigned long Mask);
#pragma intrinsic(_BitScanForward)
unsigned char _BitScanReverse(unsigned long * Index, unsigned long Mask);
#pragma intrinsic(_BitScanReverse)
#ifdef _WIN64
unsigned char _BitScanForward64(unsigned long * Index, unsigned long long Mask);
#pragma intrinsic(_BitScanForward64)
unsigned char _BitScanReverse64(unsigned long * Index, unsigned long long Mask);
#pragma intrinsic(_BitScanReverse64)
#endif
unsigned char _bittest(const long * a, long b);
#pragma intrinsic(_bittest)
unsigned char _bittestandcomplement(long * a, long b);
#pragma intrinsic(_bittestandcomplement)
unsigned char _bittestandreset(long * a, long b);
#pragma intrinsic(_bittestandreset)
unsigned char _bittestandset(long * a, long b);
#pragma intrinsic(_bittestandset)
unsigned char _rotl8(unsigned char value, unsigned char shift);
#pragma intrinsic(_rotl8)
unsigned short _rotl16(unsigned short value, unsigned char shift);
#pragma intrinsic(_rotl16)
_Check_return_ unsigned int _rotl(unsigned int value, int shift);
#pragma intrinsic(_rotl)
_Check_return_ unsigned __int64 __cdecl _rotl64(_In_ unsigned __int64 Value, _In_ int Shift);
#pragma intrinsic(_rotl64)
_Check_return_ unsigned long __cdecl _lrotl(_In_ unsigned long, _In_ int);
#pragma intrinsic(_lrotl)
unsigned char _rotr8(unsigned char value, unsigned char shift);
#pragma intrinsic(_rotr8)
unsigned short _rotr16(unsigned short value, unsigned char shift);
#pragma intrinsic(_rotr16)
_Check_return_ unsigned int _rotr(unsigned int value, int shift);
#pragma intrinsic(_rotr)
_Check_return_ unsigned __int64 __cdecl _rotr64(_In_ unsigned __int64 Value, _In_ int Shift);
#pragma intrinsic(_rotr64)
_Check_return_ unsigned long __cdecl _lrotr(_In_ unsigned long, _In_ int);
#pragma intrinsic(_lrotr)
unsigned short _byteswap_ushort(unsigned short value);
#pragma intrinsic(_byteswap_ushort)
unsigned long _byteswap_ulong(unsigned long value);
#pragma intrinsic(_byteswap_ulong)
unsigned __int64 _byteswap_uint64(unsigned __int64 value);
#pragma intrinsic(_byteswap_uint64)
#if defined(_M_IX86) || defined(_M_AMD64)
unsigned __int64 __ll_lshift(unsigned __int64 Mask, int Bit);
#pragma intrinsic(__ll_lshift)
__int64 __ll_rshift(__int64 Mask, int Bit);
#pragma intrinsic(__ll_rshift)
unsigned __int64 __ull_rshift(unsigned __int64 Mask, int Bit);
#pragma intrinsic(__ull_rshift)
unsigned int __lzcnt(unsigned int Value);
#pragma intrinsic(__lzcnt)
unsigned short __lzcnt16(unsigned short Value);
#pragma intrinsic(__lzcnt16)
unsigned int __popcnt(unsigned int Value);
#pragma intrinsic(__popcnt)
unsigned short __popcnt16(unsigned short Value);
#pragma intrinsic(__popcnt16)
#endif
#ifdef _M_AMD64
unsigned __int64 __shiftleft128(unsigned __int64 LowPart, unsigned __int64 HighPart, unsigned char Shift);
#pragma intrinsic(__shiftleft128)
unsigned __int64 __shiftright128(unsigned __int64 LowPart, unsigned __int64 HighPart, unsigned char Shift);
#pragma intrinsic(__shiftright128)
unsigned char _bittest64(__int64 const *a, __int64 b);
#pragma intrinsic(_bittest64)
unsigned char _bittestandcomplement64(__int64 *a, __int64 b);
#pragma intrinsic(_bittestandcomplement64)
unsigned char _bittestandreset64(__int64 *a, __int64 b);
#pragma intrinsic(_bittestandreset64)
unsigned char _bittestandset64(__int64 *a, __int64 b);
#pragma intrinsic(_bittestandset64)
unsigned __int64 __lzcnt64(unsigned __int64 Value);
#pragma intrinsic(__lzcnt64)
unsigned __int64 __popcnt64(unsigned __int64 Value);
#pragma intrinsic(__popcnt64)
#elif defined(_M_ARM)
unsigned int _CountLeadingOnes(unsigned long Value);
unsigned int _CountLeadingOnes64(unsigned __int64 Value);
unsigned int _CountLeadingSigns(long Value);
unsigned int _CountLeadingSigns64(__int64 Value);
unsigned int _CountLeadingZeros(unsigned long Value);
unsigned int _CountLeadingZeros64(unsigned __int64 Value);
unsigned int _CountOneBits(unsigned long Value);
unsigned int _CountOneBits64(unsigned __int64 Value);
#endif

/*** 64/128-bit math ***/
__int64 __cdecl _abs64(__int64);
#pragma intrinsic(_abs64)
#if defined(_M_IX86) || defined(_M_AMD64)
__int64 __emul(int a, int b);
#pragma intrinsic(__emul)
unsigned __int64 __emulu(unsigned int a, unsigned int b);
#pragma intrinsic(__emulu)
#endif
#ifdef _M_AMD64
__int64 __mulh(__int64 a, __int64 b);
#pragma intrinsic(__mulh)
unsigned __int64 __umulh(unsigned __int64 a, unsigned __int64 b);
#pragma intrinsic(__umulh)
__int64 _mul128(__int64 Multiplier, __int64 Multiplicand, __int64 * HighProduct);
#pragma intrinsic(_mul128)
unsigned __int64 _umul128(unsigned __int64 Multiplier, unsigned __int64 Multiplicand, unsigned __int64 * HighProduct);
#pragma intrinsic(_umul128)
#elif defined(_M_ARM)
long _MulHigh(long Multiplier, long Multiplicand);
#pragma intrinsic(_MulHigh)
unsigned long _MulUnsignedHigh(unsigned long Multiplier, unsigned long Multiplicand);
#pragma intrinsic(_MulUnsignedHigh)
#endif

/** Floating point stuff **/
#if defined(_M_ARM)
int _isunordered(double arg1, double arg2);
#pragma intrinsic(_isunordered)
int _isunorderedf(float arg1, float arg2);
#pragma intrinsic(_isunorderedf)
double _CopyDoubleFromInt64(__int64);
#pragma intrinsic(_CopyDoubleFromInt64)
float _CopyFloatFromInt32(__int32);
#pragma intrinsic(_CopyFloatFromInt32)
__int32 _CopyInt32FromFloat(float);
#pragma intrinsic(_CopyInt32FromFloat)
__int64 _CopyInt64FromDouble(double);
#pragma intrinsic(_CopyInt64FromDouble)
#endif

/*** Port I/O ***/
#if defined(_M_IX86) || defined(_M_AMD64)
unsigned char __inbyte(unsigned short Port);
#pragma intrinsic(__inbyte)
unsigned short __inword(unsigned short Port);
#pragma intrinsic(__inword)
unsigned long __indword(unsigned short Port);
#pragma intrinsic(__indword)
void __inbytestring(unsigned short Port, unsigned char * Buffer, unsigned long Count);
#pragma intrinsic(__inbytestring)
void __inwordstring(unsigned short Port, unsigned short * Buffer, unsigned long Count);
#pragma intrinsic(__inwordstring)
void __indwordstring(unsigned short Port, unsigned long * Buffer, unsigned long Count);
#pragma intrinsic(__indwordstring)
void __outbyte(unsigned short Port, unsigned char Data);
#pragma intrinsic(__outbyte)
void __outword(unsigned short Port, unsigned short Data);
#pragma intrinsic(__outword)
void __outdword(unsigned short Port, unsigned long Data);
#pragma intrinsic(__outdword)
void __outbytestring(unsigned short Port, unsigned char * Buffer, unsigned long Count);
#pragma intrinsic(__outbytestring)
void __outwordstring(unsigned short Port, unsigned short * Buffer, unsigned long Count);
#pragma intrinsic(__outwordstring)
void __outdwordstring(unsigned short Port, unsigned long * Buffer, unsigned long Count);
#pragma intrinsic(__outdwordstring)
int __cdecl _inp(unsigned short Port);
#pragma intrinsic(_inp)
unsigned long __cdecl _inpd(unsigned short Port);
#pragma intrinsic(_inpd)
unsigned short __cdecl _inpw(unsigned short Port);
#pragma intrinsic(_inpw)
int __cdecl inp(unsigned short Port);
#pragma intrinsic(inp)
unsigned long __cdecl inpd(unsigned short Port);
#pragma intrinsic(inpd)
unsigned short __cdecl inpw(unsigned short Port);
#pragma intrinsic(inpw)
int __cdecl _outp(unsigned short Port, int Value);
#pragma intrinsic(_outp)
unsigned long __cdecl _outpd(unsigned short Port, unsigned long Value);
#pragma intrinsic(_outpd)
unsigned short __cdecl _outpw(unsigned short Port, unsigned short Value);
#pragma intrinsic(_outpw)
int __cdecl outp(unsigned short Port, int Value);
#pragma intrinsic(outp)
unsigned long __cdecl outpd(unsigned short Port, unsigned long Value);
#pragma intrinsic(outpd)
unsigned short __cdecl outpw(unsigned short Port, unsigned short Value);
#pragma intrinsic(outpw)
#endif

/*** System information ***/
#if defined(_M_IX86) || defined(_M_AMD64)
void __cpuid(int CPUInfo[4], int InfoType);
#pragma intrinsic(__cpuid)
void __cpuidex(int CPUInfo[4], int InfoType, int ECXValue);
#pragma intrinsic(__cpuidex)
unsigned __int64 __rdtsc(void);
#pragma intrinsic(__rdtsc)
unsigned __int64 __rdtscp(unsigned int *);
#pragma intrinsic(__rdtscp)
void __writeeflags(uintptr_t Value);
#pragma intrinsic(__writeeflags)
uintptr_t __readeflags(void);
#pragma intrinsic(__readeflags)
#endif

/*** Interrupts and traps ***/
void __debugbreak(void);
#pragma intrinsic(__debugbreak)
void _disable(void);
#pragma intrinsic(_disable)
void _enable(void);
#pragma intrinsic(_enable)
#if defined(_M_IX86) || defined(_M_AMD64)
void __int2c(void);
#pragma intrinsic(__int2c)
void __halt(void);
#pragma intrinsic(__halt)
void __ud2(void);
#pragma intrinsic(__ud2)
#if (_MSC_VER >= 1700)
__declspec(noreturn) void __fastfail(unsigned int Code);
#pragma intrinsic(__fastfail)
#else
__declspec(noreturn) __forceinline
void __fastfail(unsigned int Code)
{
    __asm
    {
        mov ecx, Code
        int 29h
    }
}
#endif
#endif
#if defined(_M_ARM)
int __trap(int Arg1, ...);
#endif

/*** Protected memory management ***/
#if defined(_M_IX86) || defined(_M_AMD64)
void __writecr0(uintptr_t Data);
#pragma intrinsic(__writecr0)
void __writecr3(uintptr_t Data);
#pragma intrinsic(__writecr3)
void __writecr4(uintptr_t Data);
#pragma intrinsic(__writecr4)
void __writecr8(uintptr_t Data);
#pragma intrinsic(__writecr8)
#endif
#if defined(_M_IX86)
unsigned long __readcr0(void);
#pragma intrinsic(__readcr0)
unsigned long __readcr2(void);
#pragma intrinsic(__readcr2)
unsigned long __readcr3(void);
#pragma intrinsic(__readcr3)
//unsigned long __readcr4(void);
//#pragma intrinsic(__readcr4)
// HACK: MSVC is broken
unsigned long  ___readcr4(void);
#define __readcr4 ___readcr4
unsigned long __readcr8(void);
#pragma intrinsic(__readcr8)
unsigned int __readdr(unsigned int reg);
#pragma intrinsic(__readdr)
void __writedr(unsigned reg, unsigned int value);
#pragma intrinsic(__writedr)
// This intrinsic is broken and generates wrong opcodes,
// when optimization is enabled!
#pragma warning(push)
#pragma warning(disable:4711)
void  __forceinline __invlpg_fixed(void * Address)
{
    _ReadWriteBarrier();
   __asm
   {
       mov eax, Address
       invlpg [eax]
   }
    _ReadWriteBarrier();
}
#pragma warning(pop)
#define __invlpg __invlpg_fixed
#elif defined(_M_AMD64)
void __invlpg(void * Address);
#pragma intrinsic(__invlpg)
#elif defined(_M_AMD64)
unsigned __int64 __readcr0(void);
#pragma intrinsic(__readcr0)
unsigned __int64 __readcr2(void);
#pragma intrinsic(__readcr2)
unsigned __int64 __readcr3(void);
#pragma intrinsic(__readcr3)
unsigned __int64 __readcr4(void);
#pragma intrinsic(__readcr4)
unsigned __int64 __readcr8(void);
#pragma intrinsic(__readcr8)
unsigned __int64 __readdr(unsigned int reg);
#pragma intrinsic(__readdr)
void __writedr(unsigned reg, unsigned __int64 value);
#pragma intrinsic(__writedr)
#elif defined(_M_ARM)
void __cdecl __prefetch(const void *);
#pragma intrinsic(__prefetch)
#endif

/*** System operations ***/
#if defined(_M_IX86) || defined(_M_AMD64)
unsigned __int64 __readmsr(unsigned long reg);
#pragma intrinsic(__readmsr)
void __writemsr(unsigned long Register, unsigned __int64 Value);
#pragma intrinsic(__writemsr)
unsigned __int64 __readpmc(unsigned long counter);
#pragma intrinsic(__readpmc)
unsigned long __segmentlimit(unsigned long a);
#pragma intrinsic(__segmentlimit)
void __wbinvd(void);
#pragma intrinsic(__wbinvd)
void __lidt(void *Source);
#pragma intrinsic(__lidt)
void __sidt(void *Destination);
#pragma intrinsic(__sidt)
void _mm_pause(void);
#pragma intrinsic(_mm_pause)
#endif
#if defined(_M_ARM)
unsigned int _MoveFromCoprocessor(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
#pragma intrinsic(_MoveFromCoprocessor)
unsigned int _MoveFromCoprocessor2(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
#pragma intrinsic(_MoveFromCoprocessor2)
unsigned __int64 _MoveFromCoprocessor64(unsigned int, unsigned int, unsigned int);
#pragma intrinsic(_MoveFromCoprocessor64)
void _MoveToCoprocessor(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
#pragma intrinsic(_MoveToCoprocessor)
void _MoveToCoprocessor2(unsigned int, unsigned int, unsigned int, unsigned int, unsigned int, unsigned int);
#pragma intrinsic(_MoveToCoprocessor2)
void _MoveToCoprocessor64(unsigned __int64, unsigned int, unsigned int, unsigned int);
#pragma intrinsic(_MoveToCoprocessor64)
int _ReadStatusReg(int);
#pragma intrinsic(_ReadStatusReg)
void _WriteStatusReg(int, int, int);
#pragma intrinsic(_WriteStatusReg)
void __yield(void);
#pragma intrinsic(__yield)
void __wfe(void);
#pragma intrinsic(__wfe)
void __wfi(void);
#pragma intrinsic(__wfi)
unsigned int __swi(unsigned int, ...);
#pragma intrinsic(__swi)
unsigned int __hvc(unsigned int, ...);
#pragma intrinsic(__hvc)
__int64 __ldrexd(__int64 volatile *);
#pragma intrinsic(__ldrexd)
unsigned __int64 __rdpmccntr64(void);
#pragma intrinsic(__rdpmccntr64)
void __sev(void);
#pragma intrinsic(__sev)
#endif

/** Secure virtual machine **/
#if defined(_M_IX86) || defined(_M_AMD64)
void __svm_clgi(void);
#pragma intrinsic(__svm_clgi)
void __svm_invlpga(void * Va, int Asid);
#pragma intrinsic(__svm_invlpga)
void __svm_skinit(int Slb);
#pragma intrinsic(__svm_skinit)
void __svm_stgi(void);
#pragma intrinsic(__svm_stgi)
void __svm_vmload(uintptr_t VmcbPhysicalAddress);
#pragma intrinsic(__svm_vmload)
void __svm_vmrun(uintptr_t VmcbPhysicalAddress);
#pragma intrinsic(__svm_vmrun)
void __svm_vmsave(uintptr_t VmcbPhysicalAddress);
#pragma intrinsic(__svm_vmsave)
#endif

/** Virtual machine extension **/
#if defined(_M_IX86) || defined(_M_AMD64)
void __vmx_off(void);
void __vmx_vmptrst(unsigned __int64 * VmcsPhysicalAddress );
#endif
#if defined(_M_AMD64)
unsigned char __vmx_on(unsigned __int64 * VmsSupportPhysicalAddress);
unsigned char __vmx_vmclear(unsigned __int64 * VmcsPhysicalAddress);
unsigned char __vmx_vmlaunch(void);
unsigned char __vmx_vmptrld(unsigned __int64 *VmcsPhysicalAddress );
unsigned char __vmx_vmread(size_t Field, size_t *FieldValue);
unsigned char __vmx_vmresume(void);
unsigned char __vmx_vmwrite(size_t Field, size_t FieldValue);
#endif

/** Misc **/
void __nop(void);
#pragma intrinsic(__nop)
#if (_MSC_VER >= 1700)
void __code_seg(const char *);
#pragma intrinsic(__code_seg)
#endif
#ifdef _M_ARM
int _AddSatInt(int, int);
#pragma intrinsic(_AddSatInt)
int _DAddSatInt(int, int);
#pragma intrinsic(_DAddSatInt)
int _DSubSatInt(int, int);
#pragma intrinsic(_DSubSatInt)
int _SubSatInt(int, int);
#pragma intrinsic(_SubSatInt)
void __emit(unsigned __int32);
#pragma intrinsic(__emit)
void __static_assert(int, const char *);
#pragma intrinsic(__static_assert)
#endif

#ifdef __cplusplus
}
#endif

#endif /* KJK_INTRIN_H_ */

/* EOF */
