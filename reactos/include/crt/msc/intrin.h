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
unsigned int __getcallerseflags(void);
#pragma intrinsic(__getcallerseflags)

/*** Atomic operations ***/
void _ReadWriteBarrier(void);
#pragma intrinsic(_ReadWriteBarrier)
void _ReadBarrier(void);
#pragma intrinsic(_ReadBarrier)
void _WriteBarrier(void);
#pragma intrinsic(_WriteBarrier)

long _InterlockedCompareExchange(volatile long * const Destination, const long Exchange, const long Comperand);
#pragma intrinsic(_InterlockedCompareExchange)
long _InterlockedExchange(volatile long * const Target, const long Value);
#pragma intrinsic(_InterlockedExchange)
long _InterlockedExchangeAdd(volatile long * const Addend, const long Value);
#pragma intrinsic(_InterlockedExchangeAdd)
char _InterlockedAnd8(volatile char * const value, const char mask);
#pragma intrinsic(_InterlockedAnd8)
short _InterlockedAnd16(volatile short * const value, const short mask);
#pragma intrinsic(_InterlockedAnd16)
long _InterlockedAnd(volatile long * const value, const long mask);
#pragma intrinsic(_InterlockedAnd)
char _InterlockedOr8(volatile char * const value, const char mask);
#pragma intrinsic(_InterlockedOr8)
short _InterlockedOr16(volatile short * const value, const short mask);
#pragma intrinsic(_InterlockedOr16)
long _InterlockedOr(volatile long * const value, const long mask);
#pragma intrinsic(_InterlockedOr)
char _InterlockedXor8(volatile char * const value, const char mask);
#pragma intrinsic(_InterlockedXor8)
short _InterlockedXor16(volatile short * const value, const short mask);
#pragma intrinsic(_InterlockedXor16)
long _InterlockedXor(volatile long * const value, const long mask);
#pragma intrinsic(_InterlockedXor)
long _InterlockedDecrement(volatile long * const lpAddend);
#pragma intrinsic(_InterlockedDecrement)
long _InterlockedIncrement(volatile long * const lpAddend);
#pragma intrinsic(_InterlockedIncrement)
short _InterlockedDecrement16(volatile short * const lpAddend);
#pragma intrinsic(_InterlockedDecrement16)
short _InterlockedIncrement16(volatile short * const lpAddend);
#pragma intrinsic(_InterlockedIncrement16)
unsigned char _interlockedbittestandreset(volatile long * a, const long b);
#pragma intrinsic(_interlockedbittestandreset)
unsigned char _interlockedbittestandset(volatile long * a, const long b);
#pragma intrinsic(_interlockedbittestandset)

#if defined(_M_IX86)
long _InterlockedAddLargeStatistic(volatile __int64 * const Addend, const long Value);
#pragma intrinsic(_InterlockedAddLargeStatistic)
#elif defined(_M_AMD64)
__int64 _InterlockedExchange64(volatile __int64 * const Target, const __int64 Value);
#pragma intrinsic(_InterlockedExchange64)
__int64 _InterlockedExchangeAdd64(volatile __int64 * const Addend, const __int64 Value);
#pragma intrinsic(_InterlockedExchangeAdd64)
void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand);
#pragma intrinsic(_InterlockedCompareExchangePointer)
void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value);
#pragma intrinsic(_InterlockedExchangePointer)
__int64 _InterlockedAnd64(volatile __int64 * const value, const __int64 mask);
#pragma intrinsic(_InterlockedAnd64)
__int64 _InterlockedOr64(volatile __int64 * const value, const __int64 mask);
#pragma intrinsic(_InterlockedOr64)
__int64 _InterlockedCompareExchange64(volatile __int64 * const Destination, const __int64 Exchange, const __int64 Comperand);
#pragma intrinsic(_InterlockedCompareExchange64)
__int64 _InterlockedDecrement64(volatile __int64 * const lpAddend);
#pragma intrinsic(_InterlockedDecrement64)
__int64 _InterlockedIncrement64(volatile __int64 * const lpAddend);
#pragma intrinsic(_InterlockedIncrement64)
unsigned char _interlockedbittestandreset64(volatile __int64 * a, const __int64 b);
#pragma intrinsic(_interlockedbittestandreset64)
unsigned char _interlockedbittestandset64(volatile __int64 * a, const __int64 b);
#pragma intrinsic(_interlockedbittestandset64)
#endif

/*** String operations ***/
void __stosb(unsigned char * Dest, const unsigned char Data, size_t Count);
#pragma intrinsic(__stosb)
void __stosw(unsigned short * Dest, const unsigned short Data, size_t Count);
#pragma intrinsic(__stosw)
void __stosd(unsigned long * Dest, const unsigned long Data, size_t Count);
#pragma intrinsic(__stosd)
void __movsb(unsigned char * Destination, const unsigned char * Source, size_t Count);
#pragma intrinsic(__movsb)
void __movsw(unsigned short * Destination, const unsigned short * Source, size_t Count);
#pragma intrinsic(__movsw)
void __movsd(unsigned long * Destination, const unsigned long * Source, size_t Count);
#pragma intrinsic(__movsd)

#if defined(_M_AMD64)
/*** GS segment addressing ***/
void __writegsbyte(const unsigned long Offset, const unsigned char Data);
#pragma intrinsic(__writegsbyte)
void __writegsword(const unsigned long Offset, const unsigned short Data);
#pragma intrinsic(__writegsword)
void __writegsdword(const unsigned long Offset, const unsigned long Data);
#pragma intrinsic(__writegsdword)
void __writegsqword(const unsigned long Offset, const unsigned __int64 Data);
#pragma intrinsic(__writegsqword)
unsigned char __readgsbyte(const unsigned long Offset);
#pragma intrinsic(__readgsbyte)
unsigned short __readgsword(const unsigned long Offset);
#pragma intrinsic(__readgsword)
unsigned long __readgsdword(const unsigned long Offset);
#pragma intrinsic(__readgsdword)
unsigned __int64 __readgsqword(const unsigned long Offset);
#pragma intrinsic(__readgsqword)
void __incgsbyte(const unsigned long Offset);
#pragma intrinsic(__incgsbyte)
void __incgsword(const unsigned long Offset);
#pragma intrinsic(__incgsword)
void __incgsdword(const unsigned long Offset);
#pragma intrinsic(__incgsdword)
void __addgsbyte(const unsigned long Offset, const unsigned char Data);
#pragma intrinsic(__addgsbyte)
void __addgsword(const unsigned long Offset, const unsigned short Data);
#pragma intrinsic(__addgsword)
void __addgsdword(const unsigned long Offset, const unsigned int Data);
#pragma intrinsic(__addgsdword)
void __addgsqword(const unsigned long Offset, const unsigned __int64 Data);
#pragma intrinsic(__addgsqword)
#endif

#if defined(_M_IX86)
/*** FS segment addressing ***/
void __writefsbyte(const unsigned long Offset, const unsigned char Data);
#pragma intrinsic(__writefsbyte)
void __writefsword(const unsigned long Offset, const unsigned short Data);
#pragma intrinsic(__writefsword)
void __writefsdword(const unsigned long Offset, const unsigned long Data);
#pragma intrinsic(__writefsdword)
unsigned char __readfsbyte(const unsigned long Offset);
#pragma intrinsic(__readfsbyte)
unsigned short __readfsword(const unsigned long Offset);
#pragma intrinsic(__readfsword)
unsigned long __readfsdword(const unsigned long Offset);
#pragma intrinsic(__readfsdword)
void __incfsbyte(const unsigned long Offset);
#pragma intrinsic(__incfsbyte)
void __incfsword(const unsigned long Offset);
#pragma intrinsic(__incfsword)
void __incfsdword(const unsigned long Offset);
#pragma intrinsic(__incfsdword)
void __addfsbyte(const unsigned long Offset, const unsigned char Data);
#pragma intrinsic(__addfsbyte)
void __addfsword(const unsigned long Offset, const unsigned short Data);
#pragma intrinsic(__addfsword)
void __addfsdword(const unsigned long Offset, const unsigned int Data);
#pragma intrinsic(__addfsdword)
#endif


/*** Bit manipulation ***/
unsigned char _BitScanForward(unsigned long * const Index, const unsigned long Mask);
#pragma intrinsic(_BitScanForward)
unsigned char _BitScanReverse(unsigned long * const Index, const unsigned long Mask);
#pragma intrinsic(_BitScanReverse)
unsigned char _bittest(const long * const a, const long b);
#pragma intrinsic(_bittest)
unsigned char _bittestandcomplement(long * const a, const long b);
#pragma intrinsic(_bittestandcomplement)
unsigned char _bittestandreset(long * const a, const long b);
#pragma intrinsic(_bittestandreset)
unsigned char _bittestandset(long * const a, const long b);
#pragma intrinsic(_bittestandset)
unsigned char _rotl8(unsigned char value, unsigned char shift);
#pragma intrinsic(_rotl8)
unsigned short _rotl16(unsigned short value, unsigned char shift);
#pragma intrinsic(_rotl16)
unsigned int _rotl(unsigned int value, int shift);
#pragma intrinsic(_rotl)
unsigned int _rotr(unsigned int value, int shift);
#pragma intrinsic(_rotr)
unsigned char _rotr8(unsigned char value, unsigned char shift);
#pragma intrinsic(_rotr8)
unsigned short _rotr16(unsigned short value, unsigned char shift);
#pragma intrinsic(_rotr16)
unsigned __int64 __ll_lshift(const unsigned __int64 Mask, const int Bit);
#pragma intrinsic(__ll_lshift)
__int64 __ll_rshift(const __int64 Mask, const int Bit);
#pragma intrinsic(__ll_rshift)
unsigned __int64 __ull_rshift(const unsigned __int64 Mask, int Bit);
#pragma intrinsic(__ull_rshift)
unsigned short _byteswap_ushort(unsigned short value);
#pragma intrinsic(_byteswap_ushort)
unsigned long _byteswap_ulong(unsigned long value);
#pragma intrinsic(_byteswap_ulong)
unsigned __int64 _byteswap_uint64(unsigned __int64 value);
#pragma intrinsic(_byteswap_uint64)

/*** 64-bit math ***/
__int64 __emul(const int a, const int b);
#pragma intrinsic(__emul)
unsigned __int64 __emulu(const unsigned int a, const unsigned int b);
#pragma intrinsic(__emulu)

/*** Port I/O ***/
unsigned char __inbyte(const unsigned short Port);
#pragma intrinsic(__inbyte)
unsigned short __inword(const unsigned short Port);
#pragma intrinsic(__inword)
unsigned long __indword(const unsigned short Port);
#pragma intrinsic(__indword)
void __inbytestring(unsigned short Port, unsigned char * Buffer, unsigned long Count);
#pragma intrinsic(__inbytestring)
void __inwordstring(unsigned short Port, unsigned short * Buffer, unsigned long Count);
#pragma intrinsic(__inwordstring)
void __indwordstring(unsigned short Port, unsigned long * Buffer, unsigned long Count);
#pragma intrinsic(__indwordstring)
void __outbyte(unsigned short const Port, const unsigned char Data);
#pragma intrinsic(__outbyte)
void __outword(unsigned short const Port, const unsigned short Data);
#pragma intrinsic(__outword)
void __outdword(unsigned short const Port, const unsigned long Data);
#pragma intrinsic(__outdword)
void __outbytestring(unsigned short const Port, const unsigned char * const Buffer, const unsigned long Count);
#pragma intrinsic(__outbytestring)
void __outwordstring(unsigned short const Port, const unsigned short * const Buffer, const unsigned long Count);
#pragma intrinsic(__outwordstring)
void __outdwordstring(unsigned short const Port, const unsigned long * const Buffer, const unsigned long Count);
#pragma intrinsic(__outdwordstring)

/*** System information ***/
void __cpuid(int CPUInfo[], const int InfoType);
#pragma intrinsic(__cpuid)
unsigned __int64 __rdtsc(void);
#pragma intrinsic(__rdtsc)
void __writeeflags(uintptr_t Value);
#pragma intrinsic(__writeeflags)
uintptr_t __readeflags(void);
#pragma intrinsic(__readeflags)

/*** Interrupts ***/
void __debugbreak(void);
#pragma intrinsic(__debugbreak)
void __int2c(void);
#pragma intrinsic(__int2c)
void _disable(void);
#pragma intrinsic(_disable)
void _enable(void);
#pragma intrinsic(_enable)
void __halt(void);
#pragma intrinsic(__halt)

/*** Protected memory management ***/
void __writecr0(const unsigned __int64 Data);
#pragma intrinsic(__writecr0)
void __writecr3(const unsigned __int64 Data);
#pragma intrinsic(__writecr3)
void __writecr4(const unsigned __int64 Data);
#pragma intrinsic(__writecr4)

#ifdef _M_AMD64
void __writecr8(const unsigned __int64 Data);
#pragma intrinsic(__writecr8)
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
#else
unsigned long __readcr0(void);
unsigned long __readcr2(void);
unsigned long __readcr3(void);
//unsigned long __readcr4(void);
//#pragma intrinsic(__readcr4)
// HACK: MSVC is broken
unsigned long  ___readcr4(void);
#define __readcr4 ___readcr4

unsigned int __readdr(unsigned int reg);
void __writedr(unsigned reg, unsigned int value);
#endif

void __invlpg(void * const Address);
#pragma intrinsic(__invlpg)

/*** System operations ***/
unsigned __int64 __readmsr(const int reg);
#pragma intrinsic(__readmsr)
void __writemsr(const unsigned long Register, const unsigned __int64 Value);
#pragma intrinsic(__writemsr)
unsigned __int64 __readpmc(const int counter);
#pragma intrinsic(__readpmc)
unsigned long __segmentlimit(const unsigned long a);
#pragma intrinsic(__segmentlimit)
void __wbinvd(void);
#pragma intrinsic(__wbinvd)
void __lidt(void *Source);
#pragma intrinsic(__lidt)
void __sidt(void *Destination);
#pragma intrinsic(__sidt)
void _mm_pause(void);
#pragma intrinsic(_mm_pause)

#ifdef __cplusplus
}
#endif

#endif /* KJK_INTRIN_H_ */

/* EOF */
