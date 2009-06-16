#ifndef KJK_INTRIN_H_
#define KJK_INTRIN_H_

#ifdef __cplusplus
extern "C" {
#endif

/*** Stack frame juggling ***/
void * _ReturnAddress(void);
void * _AddressOfReturnAddress(void);
unsigned int __getcallerseflags(void);

/*** Atomic operations ***/
void _ReadWriteBarrier(void);
void _ReadBarrier(void);
void _WriteBarrier(void);

char _InterlockedCompareExchange8(volatile char * const Destination, const char Exchange, const char Comperand);
short _InterlockedCompareExchange16(volatile short * const Destination, const short Exchange, const short Comperand);
long _InterlockedCompareExchange(volatile long * const Destination, const long Exchange, const long Comperand);
void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand);
long _InterlockedExchange(volatile long * const Target, const long Value);
void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value);
long _InterlockedExchangeAdd16(volatile short * const Addend, const short Value);
long _InterlockedExchangeAdd(volatile long * const Addend, const long Value);
char _InterlockedAnd8(volatile char * const value, const char mask);
short _InterlockedAnd16(volatile short * const value, const short mask);
long _InterlockedAnd(volatile long * const value, const long mask);
char _InterlockedOr8(volatile char * const value, const char mask);
short _InterlockedOr16(volatile short * const value, const short mask);
long _InterlockedOr(volatile long * const value, const long mask);
char _InterlockedXor8(volatile char * const value, const char mask);
short _InterlockedXor16(volatile short * const value, const short mask);
long _InterlockedXor(volatile long * const value, const long mask);
long _InterlockedAddLargeStatistic(volatile __int64 * const Addend, const long Value);
long _InterlockedDecrement(volatile long * const lpAddend);
long _InterlockedIncrement(volatile long * const lpAddend);
long _InterlockedDecrement16(volatile short * const lpAddend);
long _InterlockedIncrement16(volatile short * const lpAddend);
unsigned char _interlockedbittestandreset(volatile long * a, const long b);
unsigned char _interlockedbittestandset(volatile long * a, const long b);

#if defined(_M_AMD64)
__int64 _InterlockedExchange64(volatile __int64 * const Target, const __int64 Value);
__int64 _InterlockedExchangeAdd64(volatile __int64 * const Addend, const __int64 Value);
long _InterlockedAnd64(volatile __int64 * const value, const __int64 mask);
long _InterlockedOr64(volatile __int64 * const value, const __int64 mask);
__int64 _InterlockedCompareExchange64(volatile __int64 * const Destination, const __int64 Exchange, const __int64 Comperand);
__int64 _InterlockedDecrement64(volatile __int64 * const lpAddend);
__int64 _InterlockedIncrement64(volatile __int64 * const lpAddend);
unsigned char _interlockedbittestandreset64(volatile __int64 * a, const __int64 b);
unsigned char _interlockedbittestandset64(volatile __int64 * a, const __int64 b);
#endif

/*** String operations ***/
void __stosb(unsigned char * Dest, const unsigned char Data, size_t Count);
void __stosw(unsigned short * Dest, const unsigned short Data, size_t Count);
void __stosd(unsigned long * Dest, const unsigned long Data, size_t Count);
void __movsb(unsigned char * Destination, const unsigned char * Source, size_t Count);
void __movsw(unsigned short * Destination, const unsigned short * Source, size_t Count);
void __movsd(unsigned long * Destination, const unsigned long * Source, size_t Count);

#if defined(_M_AMD64)
/*** GS segment addressing ***/
void __writegsbyte(const unsigned long Offset, const unsigned char Data);
void __writegsword(const unsigned long Offset, const unsigned short Data);
void __writegsdword(const unsigned long Offset, const unsigned long Data);
void __writegsqword(const unsigned long Offset, const unsigned __int64 Data);
unsigned char __readgsbyte(const unsigned long Offset);
unsigned short __readgsword(const unsigned long Offset);
unsigned long __readgsdword(const unsigned long Offset);
unsigned __int64 __readgsqword(const unsigned long Offset);
void __incgsbyte(const unsigned long Offset);
void __incgsword(const unsigned long Offset);
void __incgsdword(const unsigned long Offset);
void __addgsbyte(const unsigned long Offset, const unsigned char Data);
void __addgsword(const unsigned long Offset, const unsigned short Data);
void __addgsdword(const unsigned long Offset, const unsigned int Data);
void __addgsqword(const unsigned long Offset, const unsigned __int64 Data);
#endif

#if defined(_M_IX86)
/*** FS segment addressing ***/
void __writefsbyte(const unsigned long Offset, const unsigned char Data);
void __writefsword(const unsigned long Offset, const unsigned short Data);
void __writefsdword(const unsigned long Offset, const unsigned long Data);
unsigned char __readfsbyte(const unsigned long Offset);
unsigned short __readfsword(const unsigned long Offset);
unsigned long __readfsdword(const unsigned long Offset);
void __incfsbyte(const unsigned long Offset);
void __incfsword(const unsigned long Offset);
void __incfsdword(const unsigned long Offset);
void __addfsbyte(const unsigned long Offset, const unsigned char Data);
void __addfsword(const unsigned long Offset, const unsigned short Data);
void __addfsdword(const unsigned long Offset, const unsigned int Data)
#endif


/*** Bit manipulation ***/
unsigned char _BitScanForward(unsigned long * const Index, const unsigned long Mask);
unsigned char _BitScanReverse(unsigned long * const Index, const unsigned long Mask);
unsigned char _bittest(const long * const a, const long b);
unsigned char _bittestandcomplement(long * const a, const long b);
unsigned char _bittestandreset(long * const a, const long b);
unsigned char _bittestandset(long * const a, const long b);
unsigned char _rotl8(unsigned char value, unsigned char shift);
unsigned short _rotl16(unsigned short value, unsigned char shift);
unsigned int _rotl(unsigned int value, int shift);
unsigned int _rotr(unsigned int value, int shift);
unsigned char _rotr8(unsigned char value, unsigned char shift);
unsigned short _rotr16(unsigned short value, unsigned char shift);
unsigned __int64 __ll_lshift(const unsigned __int64 Mask, const int Bit);
__int64 __ll_rshift(const __int64 Mask, const int Bit);
unsigned __int64 __ull_rshift(const unsigned __int64 Mask, int Bit);
unsigned short _byteswap_ushort(unsigned short value);
unsigned long _byteswap_ulong(unsigned long value);
unsigned __int64 _byteswap_uint64(unsigned __int64 value);

/*** 64-bit math ***/
__int64 __emul(const int a, const int b);
unsigned __int64 __emulu(const unsigned int a, const unsigned int b);

/*** Port I/O ***/
unsigned char __inbyte(const unsigned short Port);
unsigned short __inword(const unsigned short Port);
unsigned long __indword(const unsigned short Port);
void __inbytestring(unsigned short Port, unsigned char * Buffer, unsigned long Count);
void __inwordstring(unsigned short Port, unsigned short * Buffer, unsigned long Count);
void __indwordstring(unsigned short Port, unsigned long * Buffer, unsigned long Count);
void __outbyte(unsigned short const Port, const unsigned char Data);
void __outword(unsigned short const Port, const unsigned short Data);
void __outdword(unsigned short const Port, const unsigned long Data);
void __outbytestring(unsigned short const Port, const unsigned char * const Buffer, const unsigned long Count);
void __outwordstring(unsigned short const Port, const unsigned short * const Buffer, const unsigned long Count);
void __outdwordstring(unsigned short const Port, const unsigned long * const Buffer, const unsigned long Count);

/*** System information ***/
void __cpuid(int CPUInfo[], const int InfoType);
unsigned __int64 __rdtsc(void);
void __writeeflags(uintptr_t Value);
uintptr_t __readeflags(void);

/*** Interrupts ***/
void __debugbreak(void);
void __int2c(void);
void _disable(void);
void _enable(void);

/*** Protected memory management ***/
void __writecr0(const unsigned __int64 Data);
void __writecr3(const unsigned __int64 Data);
void __writecr4(const unsigned __int64 Data);

#ifdef _M_AMD64
void __writecr8(const unsigned __int64 Data);
#endif

unsigned __int64 __readcr0(void);
unsigned __int64 __readcr2(void);
unsigned __int64 __readcr3(void);
unsigned __int64 __readcr4(void);

#ifdef _M_AMD64
unsigned __int64 __readcr8(void);
#endif

unsigned __int64 __readdr(unsigned int reg);
void __writedr(unsigned reg, unsigned __int64 value);

void __invlpg(void * const Address);

/*** System operations ***/
unsigned __int64 __readmsr(const int reg);
void __writemsr(const unsigned long Register, const unsigned __int64 Value);
unsigned __int64 __readpmc(const int counter);
unsigned long __segmentlimit(const unsigned long a);
void __wbinvd(void);
void __lidt(void *Source);
void __sidt(void *Destination);
void _mm_pause(void);

#ifdef __cplusplus
}
#endif

#endif /* KJK_INTRIN_H_ */

/* EOF */
