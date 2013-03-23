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
#endif
#ifdef _M_AMD64
void __faststorefence(void);
#pragma intrinsic(__faststorefence)
#endif

/*** Atomic operations ***/
long _InterlockedCompareExchange(volatile long * Destination, long Exchange, long Comperand);
#pragma intrinsic(_InterlockedCompareExchange)
long _InterlockedExchange(volatile long * Target, long Value);
#pragma intrinsic(_InterlockedExchange)
long _InterlockedExchangeAdd(volatile long * Addend, long Value);
#pragma intrinsic(_InterlockedExchangeAdd)
char _InterlockedAnd8(volatile char * value, char mask);
#pragma intrinsic(_InterlockedAnd8)
short _InterlockedAnd16(volatile short * value, short mask);
#pragma intrinsic(_InterlockedAnd16)
long _InterlockedAnd(volatile long * value, long mask);
#pragma intrinsic(_InterlockedAnd)
char _InterlockedOr8(volatile char * value, char mask);
#pragma intrinsic(_InterlockedOr8)
short _InterlockedOr16(volatile short * value, short mask);
#pragma intrinsic(_InterlockedOr16)
long _InterlockedOr(volatile long * value, long mask);
#pragma intrinsic(_InterlockedOr)
char _InterlockedXor8(volatile char * value, char mask);
#pragma intrinsic(_InterlockedXor8)
short _InterlockedXor16(volatile short * value, short mask);
#pragma intrinsic(_InterlockedXor16)
long _InterlockedXor(volatile long * value, long mask);
#pragma intrinsic(_InterlockedXor)
long _InterlockedDecrement(volatile long * lpAddend);
#pragma intrinsic(_InterlockedDecrement)
long _InterlockedIncrement(volatile long * lpAddend);
#pragma intrinsic(_InterlockedIncrement)
short _InterlockedDecrement16(volatile short * lpAddend);
#pragma intrinsic(_InterlockedDecrement16)
short _InterlockedIncrement16(volatile short * lpAddend);
#pragma intrinsic(_InterlockedIncrement16)
unsigned char _interlockedbittestandreset(volatile long * a, long b);
#pragma intrinsic(_interlockedbittestandreset)
unsigned char _interlockedbittestandset(volatile long * a, long b);
#pragma intrinsic(_interlockedbittestandset)

#if defined(_M_IX86)
long _InterlockedAddLargeStatistic(volatile __int64 * Addend, long Value);
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
__int64 _InterlockedAnd64(volatile __int64 * value, __int64 mask);
#pragma intrinsic(_InterlockedAnd64)
__int64 _InterlockedOr64(volatile __int64 * value, __int64 mask);
#pragma intrinsic(_InterlockedOr64)
__int64 _InterlockedCompareExchange64(volatile __int64 * Destination, __int64 Exchange, __int64 Comperand);
#pragma intrinsic(_InterlockedCompareExchange64)
__int64 _InterlockedDecrement64(volatile __int64 * lpAddend);
#pragma intrinsic(_InterlockedDecrement64)
__int64 _InterlockedIncrement64(volatile __int64 * lpAddend);
#pragma intrinsic(_InterlockedIncrement64)
unsigned char _interlockedbittestandreset64(volatile __int64 * a, __int64 b);
#pragma intrinsic(_interlockedbittestandreset64)
unsigned char _interlockedbittestandset64(volatile __int64 * a, __int64 b);
#pragma intrinsic(_interlockedbittestandset64)
#endif

#if defined(_M_IX86) || defined(_M_AMD64)
/*** String operations ***/
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

#if defined(_M_AMD64)
/*** GS segment addressing ***/
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
void __addgsbyte(unsigned long Offset, unsigned char Data);
#pragma intrinsic(__addgsbyte)
void __addgsword(unsigned long Offset, unsigned short Data);
#pragma intrinsic(__addgsword)
void __addgsdword(unsigned long Offset, unsigned int Data);
#pragma intrinsic(__addgsdword)
void __addgsqword(unsigned long Offset, unsigned __int64 Data);
#pragma intrinsic(__addgsqword)
#endif

#if defined(_M_IX86)
/*** FS segment addressing ***/
void __writefsbyte(unsigned long Offset, unsigned char Data);
#pragma intrinsic(__writefsbyte)
void __writefsword(unsigned long Offset, unsigned short Data);
#pragma intrinsic(__writefsword)
void __writefsdword(unsigned long Offset, unsigned long Data);
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
void __addfsdword(unsigned long Offset, unsigned int Data);
#pragma intrinsic(__addfsdword)
#endif


/*** Bit manipulation ***/
unsigned char _BitScanForward(unsigned long * Index, unsigned long Mask);
#pragma intrinsic(_BitScanForward)
unsigned char _BitScanReverse(unsigned long * Index, unsigned long Mask);
#pragma intrinsic(_BitScanReverse)
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
unsigned int _rotl(unsigned int value, int shift);
#pragma intrinsic(_rotl)
unsigned int _rotr(unsigned int value, int shift);
#pragma intrinsic(_rotr)
unsigned char _rotr8(unsigned char value, unsigned char shift);
#pragma intrinsic(_rotr8)
unsigned short _rotr16(unsigned short value, unsigned char shift);
#pragma intrinsic(_rotr16)
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
#endif
#ifdef _M_AMD64
unsigned char _bittest64(__int64 const *a, __int64 b);
#pragma intrinsic(_bittest64)
#endif

#if defined(_M_IX86) || defined(_M_AMD64)
/*** 64-bit math ***/
__int64 __emul(int a, int b);
#pragma intrinsic(__emul)
unsigned __int64 __emulu(unsigned int a, unsigned int b);
#pragma intrinsic(__emulu)
#endif
#ifdef _M_AMD64
unsigned __int64 __umulh(unsigned __int64 a, unsigned __int64 b);
#pragma intrinsic(__umulh)
#endif

#if defined(_M_IX86) || defined(_M_AMD64)
/*** Port I/O ***/
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
#endif

#if defined(_M_IX86) || defined(_M_AMD64)
/*** System information ***/
void __cpuid(int CPUInfo[], int InfoType);
#pragma intrinsic(__cpuid)
unsigned __int64 __rdtsc(void);
#pragma intrinsic(__rdtsc)
void __writeeflags(uintptr_t Value);
#pragma intrinsic(__writeeflags)
uintptr_t __readeflags(void);
#pragma intrinsic(__readeflags)
#endif

/*** Interrupts ***/
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
#endif

/*** Protected memory management ***/
#if defined(_M_IX86) || defined(_M_AMD64)
void __writecr0(unsigned __int64 Data);
#pragma intrinsic(__writecr0)
void __writecr3(unsigned __int64 Data);
#pragma intrinsic(__writecr3)
void __writecr4(unsigned __int64 Data);
#pragma intrinsic(__writecr4)
#endif
#ifdef _M_AMD64
void __writecr8(unsigned __int64 Data);
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
#elif defined(_M_IX86)
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

#ifdef _M_IX86
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
#endif

/*** System operations ***/
#if defined(_M_IX86) || defined(_M_AMD64)
unsigned __int64 __readmsr(int reg);
#pragma intrinsic(__readmsr)
void __writemsr(unsigned long Register, unsigned __int64 Value);
#pragma intrinsic(__writemsr)
unsigned __int64 __readpmc(int counter);
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

#ifdef __cplusplus
}
#endif

#endif /* KJK_INTRIN_H_ */

/* EOF */
