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

#ifndef KJK_INTRIN_PPC_H_
#define KJK_INTRIN_PPC_H_

//#define PPC_QUAL static __inline__ __attribute__((always_inline))
#define PPC_QUAL extern __inline__

#ifndef __GNUC__
#error Unsupported compiler
#endif

/*** Stack frame juggling ***/
#define _ReturnAddress() (__builtin_return_address(0))
#define _AddressOfReturnAddress() (&(((void **)(__builtin_frame_address(0)))[1]))
/* TODO: __getcallerseflags but how??? */


/*** Atomic operations ***/
/* TODO: _ReadBarrier */
/* TODO: _WriteBarrier */

#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100
#define _ReadWriteBarrier() __sync_synchronize()
#else
/* TODO: _ReadWriteBarrier() */
#endif

#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100

PPC_QUAL char _InterlockedCompareExchange8(volatile char * const Destination, const char Exchange, const char Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

PPC_QUAL short _InterlockedCompareExchange16(volatile short * const Destination, const short Exchange, const short Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

PPC_QUAL long _InterlockedCompareExchange(volatile long * const Destination, const long Exchange, const long Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

PPC_QUAL long long _InterlockedCompareExchange64(volatile long long * const Destination, const long long Exchange, const long long Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

PPC_QUAL void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

PPC_QUAL long _InterlockedExchange(volatile long * const Target, const long Value)
{
	/* NOTE: __sync_lock_test_and_set would be an acquire barrier, so we force a full barrier */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}

PPC_QUAL void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value)
{
	/* NOTE: ditto */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}

PPC_QUAL long _InterlockedExchangeAdd(volatile long * const Addend, const long Value)
{
	return __sync_fetch_and_add(Addend, Value);
}

PPC_QUAL char _InterlockedAnd8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_and(value, mask);
}

PPC_QUAL short _InterlockedAnd16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_and(value, mask);
}

PPC_QUAL long _InterlockedAnd(volatile long * const value, const long mask)
{
	return __sync_fetch_and_and(value, mask);
}

PPC_QUAL char _InterlockedOr8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_or(value, mask);
}

PPC_QUAL short _InterlockedOr16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_or(value, mask);
}

PPC_QUAL long _InterlockedOr(volatile long * const value, const long mask)
{
	return __sync_fetch_and_or(value, mask);
}

PPC_QUAL char _InterlockedXor8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_xor(value, mask);
}

PPC_QUAL short _InterlockedXor16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_xor(value, mask);
}

PPC_QUAL long _InterlockedXor(volatile long * const value, const long mask)
{
	return __sync_fetch_and_xor(value, mask);
}

#else

PPC_QUAL char _InterlockedCompareExchange8(volatile char * const Destination, const char Exchange, const char Comperand)
{
        volatile long retval __asm__("r8") = 0;
	__asm__ __volatile__ (
	    "sync\n"
	    "1: lbarx   %0,0,%1\n"
            : "=r" (retval) : "r" (Destination));
        __asm__ __volatile__ (
	    "   cmpw    %3,%1\n"
	    "   bne-    2f\n"
	    "   stbcx.  %2,0,%0\n"
	    "   bne-    1b\n"
	    "2: isync"
	    :
	    : "r" (Destination), "r" (Comperand), "r" (Exchange), "r" (retval));
	return retval;
}

PPC_QUAL short _InterlockedCompareExchange16(volatile short * const Destination, const short Exchange, const short Comperand)
{
        volatile long retval __asm__("r8") = 0;
	__asm__ __volatile__ (
	    "sync\n"
	    "1: lharx   %0,0,%1\n"
            : "=&r" (retval) : "r" (Destination));
        __asm__ __volatile__ (
	    "   cmpw    %3,%1\n"
	    "   bne-    2f\n"
	    "   sthcx.  %2,0,%0\n"
	    "   bne-    1b\n"
	    "2: isync"
	    :
	    : "r" (Destination), "r" (Comperand), "r" (Exchange), "r" (retval));
	return retval;
}

PPC_QUAL long _InterlockedCompareExchange(volatile long * const Destination, const long Exchange, const long Comperand)
{
        volatile long retval __asm__("r8") = 0;
	__asm__ __volatile__ (
	    "sync\n"
	    "1: lwarx   %0,0,%1\n"
            : "=&r" (retval) : "r" (Destination));
        __asm__ __volatile__ (
	    "   cmpw    %3,%1\n"
	    "   bne-    2f\n"
	    "   stwcx.  %2,0,%0\n"
	    "   bne-    1b\n"
	    "2: isync"
	    :
	    : "r" (Destination), "r" (Comperand), "r" (Exchange), "r" (retval));
	return retval;
}

PPC_QUAL long long _InterlockedCompareExchange64(volatile long long * const Target, const long long Exchange, const long long Comperand)
{
    long long capture = *Target;
    if (*Target == Comperand) *Target = Exchange;
    return capture;
}

PPC_QUAL void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand)
{
    return (void *)_InterlockedCompareExchange
	((long *)Destination, (long) Exchange, (long) Comperand);
}

PPC_QUAL long _InterlockedExchange(volatile long * const Target, const long Value)
{
    long retval __asm__("r8");
    __asm__ __volatile__ (
        "sync\n"
        "1: lwarx   8,0,3\n"
        "   stwcx.  4,0,3\n"
        "   bne-    1b\n"
        "   mr      3,8\n"
        : "=b" (retval)
        : "b" (Target), "b" (Value)
        : "cr0", "memory");
    return retval;
}

PPC_QUAL void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value)
{
    return (void *)_InterlockedExchange((long *)Target, (long)Value);
}

#define PPC_MakeInterlockedFunction(type,name,op,proto) \
PPC_QUAL type name proto \
{ \
        long addend, y; \
        do \
        { \
                addend = *value; \
                y = _InterlockedCompareExchange(value, addend op modify, addend); \
	} \
	while(y != addend); \
 \
	return y; \
}

PPC_QUAL unsigned char _interlockedbittestandreset(volatile long * const a, const long b)
{
	long x;
	long y;
	long mask = ~(1<<b);

	do
	{
		x = *a;
		y = _InterlockedCompareExchange(a, x & mask, x);
	}
	while(y != x);

	return (y & ~mask) != 0;
}

PPC_MakeInterlockedFunction(long,_InterlockedExchangeAdd,+,(volatile long * const value, const long modify))
PPC_MakeInterlockedFunction(char,_InterlockedAnd8,&,(volatile char * const value, const char modify))
PPC_MakeInterlockedFunction(short,_InterlockedAnd16,&,(volatile short * const value, const short modify))
PPC_MakeInterlockedFunction(long,_InterlockedAnd,&,(volatile long * const value, const long modify))
PPC_MakeInterlockedFunction(char,_InterlockedOr8,|,(volatile char * const value, const char modify))
PPC_MakeInterlockedFunction(short,_InterlockedOr16,|,(volatile short * const value, const short modify))
PPC_MakeInterlockedFunction(long,_InterlockedOr,|,(volatile long * const value, const long modify))
PPC_MakeInterlockedFunction(char,_InterlockedXor8,^,(volatile char * const value, const char modify))
PPC_MakeInterlockedFunction(short,_InterlockedXor16,^,(volatile short * const value, const short modify))
PPC_MakeInterlockedFunction(long,_InterlockedXor,^,(volatile long * const value, const long modify))

PPC_QUAL unsigned char _interlockedbittestandset(volatile long * const a, const long b)
{
	long x;
	long y;
	long mask = 1<<b;

	do
	{
                x = *a;
		y = _InterlockedCompareExchange(a, x | mask, x);
	}
	while(y != x);

	return (y & ~mask) != 0;
}
#endif

PPC_QUAL long _InterlockedDecrement(volatile long * const lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, -1) - 1;
}

PPC_QUAL long _InterlockedIncrement(volatile long * const lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, 1) + 1;
}

/*** String operations ***/
/* NOTE: we don't set a memory clobber in the __stosX functions because Visual C++ doesn't */
/* Note that the PPC store multiple operations may raise an exception in LE
 * mode */
PPC_QUAL void __stosb(unsigned char * Dest, const unsigned char Data, unsigned long Count)
{
    memset(Dest, Data, Count);
}

PPC_QUAL void __stosw(unsigned short * Dest, const unsigned short Data, unsigned long Count)
{
    while(Count--)
	*Dest++ = Data;
}

PPC_QUAL void __stosd(unsigned long * Dest, const unsigned long Data, unsigned long Count)
{
    while(Count--)
	*Dest++ = Data;
}

PPC_QUAL void __movsb(unsigned char * Destination, const unsigned char * Source, unsigned long Count)
{
    memcpy(Destination, Source, Count);
}

PPC_QUAL void __movsw(unsigned short * Destination, const unsigned short * Source, unsigned long Count)
{
    memcpy(Destination, Source, Count * sizeof(*Source));
}

PPC_QUAL void __movsd(unsigned long * Destination, const unsigned long * Source, unsigned long Count)
{
    memcpy(Destination, Source, Count * sizeof(*Source));
}


/*** FS segment addressing ***/
/* On PowerPC, r13 points to TLS data, including the TEB at 0(r13) from what I
 * can tell */
PPC_QUAL void __writefsbyte(const unsigned long Offset, const unsigned char Data)
{
    char *addr;
    __asm__("\tadd %0,13,%1\n\tstb %2,0(%0)" : "=r" (addr) : "r" (Offset), "r" (Data));
}

PPC_QUAL void __writefsword(const unsigned long Offset, const unsigned short Data)
{
    char *addr;
    __asm__("\tadd %0,13,%1\n\tsth %2,0(%0)" : "=r" (addr) : "r" (Offset), "r" (Data));
}

PPC_QUAL void __writefsdword(const unsigned long Offset, const unsigned long Data)
{
    char *addr;
    __asm__("\tadd %0,13,%1\n\tstw %2,0(%0)" : "=r" (addr) : "r" (Offset), "r" (Data));
}

PPC_QUAL unsigned char __readfsbyte(const unsigned long Offset)
{
    unsigned short result;
    __asm__("\tadd 7,13,%1\n"
	    "\tlbz %0,0(7)\n"
	    : "=r" (result)
	    : "r" (Offset)
	    : "r7");
    return result;
}

PPC_QUAL unsigned short __readfsword(const unsigned long Offset)
{
    unsigned short result;
    __asm__("\tadd 7,13,%1\n"
	    "\tlhz %0,0(7)\n"
	    : "=r" (result)
	    : "r" (Offset)
	    : "r7");
    return result;
}

PPC_QUAL unsigned long __readfsdword(const unsigned long Offset)
{
    unsigned long result;
    __asm__("\tadd 7,13,%1\n"
	    "\tlwz %0,0(7)\n"
	    : "=r" (result)
	    : "r" (Offset)
	    : "r7");
    return result;
}

PPC_QUAL void __incfsbyte(const unsigned long Offset)
{
    __writefsbyte(Offset, __readfsbyte(Offset)+1);
}

PPC_QUAL void __incfsword(const unsigned long Offset)
{
    __writefsword(Offset, __readfsword(Offset)+1);
}

PPC_QUAL void __incfsdword(const unsigned long Offset)
{
    __writefsdword(Offset, __readfsdword(Offset)+1);
}

/* NOTE: the bizarre implementation of __addfsxxx mimics the broken Visual C++ behavior */
/* PPC Note: Not sure about the bizarre behavior.  We'll try to emulate it later */
PPC_QUAL void __addfsbyte(const unsigned long Offset, const unsigned char Data)
{
    __writefsbyte(Offset, __readfsbyte(Offset) + Data);
}

PPC_QUAL void __addfsword(const unsigned long Offset, const unsigned short Data)
{
    __writefsword(Offset, __readfsword(Offset) + Data);
}

PPC_QUAL void __addfsdword(const unsigned long Offset, const unsigned int Data)
{
    __writefsdword(Offset, __readfsdword(Offset) + Data);
}


/*** Bit manipulation ***/
PPC_QUAL unsigned char _BitScanForward(unsigned long * const Index, const unsigned long Mask)
{
    if(Mask == 0) return 0;
    else {
	unsigned long mask = Mask;
	mask &= -mask;
	*Index =
	    ((mask & 0xffff0000) ? 16 : 0) +
	    ((mask & 0xff00ff00) ? 8  : 0) +
	    ((mask & 0xf0f0f0f0) ? 4  : 0) +
	    ((mask & 0xcccccccc) ? 2  : 0) +
	    ((mask & 0xaaaaaaaa) ? 1  : 0);
	return 1;
    }
}

/* Thanks http://www.jjj.de/bitwizardry/files/bithigh.h */
PPC_QUAL unsigned char _BitScanReverse(unsigned long * const Index, const unsigned long Mask)
{
    unsigned long check = 16, checkmask;
    if(Mask == 0) return 0;
    else {
	unsigned long mask = Mask;
	*Index = 0;
	while(check) {
	    checkmask = ((1<<check)-1) << check;
	    if( mask & checkmask ) {
		mask >>= check;
		*Index += check;
	    }
	    check >>= 1;
	}
	return 1;
    }
}

/* NOTE: again, the bizarre implementation follows Visual C++ */
PPC_QUAL unsigned char _bittest(const long * const a, const long b)
{
    return ((*a) & (1<<b)) != 0;
}

PPC_QUAL unsigned char _bittestandcomplement(long * const a, const long b)
{
    unsigned char ret = ((*a) & (1<<b)) != 0;
    (*a) ^= (1<<b);
    return ret;
}

PPC_QUAL unsigned char _bittestandreset(long * const a, const long b)
{
    unsigned char ret = ((*a) & (1<<b)) != 0;
    (*a) &= ~(1<<b);
    return ret;
}

PPC_QUAL unsigned char _bittestandset(long * const a, const long b)
{
    unsigned char ret = ((*a) & (1<<b)) != 0;
    (*a) |= (1<<b);
    return ret;
}

PPC_QUAL unsigned char _rotl8(const unsigned char value, const unsigned char shift)
{
    return (value << shift) | (value >> (8-shift));
}

PPC_QUAL unsigned short _rotl16(const unsigned short value, const unsigned char shift)
{
    return (value << shift) | (value >> (16-shift));
}

PPC_QUAL unsigned char _rotr8(const unsigned char value, const unsigned char shift)
{
    return (value >> shift) | (value << (8-shift));
}

PPC_QUAL unsigned short _rotr16(const unsigned short value, const unsigned char shift)
{
    return (value >> shift) | (value << (16-shift));
}

PPC_QUAL unsigned long long __ll_lshift(const unsigned long long Mask, int Bit)
{
    return Mask << Bit;
}

PPC_QUAL long long __ll_rshift(const long long Mask, const int Bit)
{
    return Mask >> Bit;
}

PPC_QUAL unsigned long long __ull_rshift(const unsigned long long Mask, int Bit)
{
    return Mask >> Bit;
}


/*** 64-bit math ***/
PPC_QUAL long long __emul(const int a, const int b)
{
    return a * b;
}

PPC_QUAL unsigned long long __emulu(const unsigned int a, const unsigned int b)
{
    return a * b;
}


/*** Port I/O ***/
PPC_QUAL unsigned char __inbyte(const unsigned long Port)
{
    int ret;
    __asm__(
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"isync\n\t"
	"sync\n\t"
	"lbz   %0,0(%1)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t" : "=r" (ret) : "b" (Port)
    );
    return ret;
}

PPC_QUAL unsigned short __inword(const unsigned long Port)
{
    int ret;
    __asm__(
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"isync\n\t"
	"sync\n\t"
	"lhz   %0,0(%1)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t" : "=r" (ret) : "b" (Port)
    );
    return ret;
}

PPC_QUAL unsigned long __indword(const unsigned long Port)
{
    int ret;
    __asm__(
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"isync\n\t"
	"sync\n\t"
	"lwz   %0,0(%1)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t" : "=r" (ret) : "b" (Port)
    );
    return ret;
}

PPC_QUAL void __inbytestring(unsigned long Port, unsigned char * Buffer, unsigned long Count)
{
    while(Count--) {
	*Buffer++ = __inbyte(Port);
    }
}

PPC_QUAL void __inwordstring(unsigned long Port, unsigned short * Buffer, unsigned long Count)
{
    while(Count--) {
	*Buffer++ = __inword(Port);
    }
}

PPC_QUAL void __indwordstring(unsigned long Port, unsigned long * Buffer, unsigned long Count)
{
    while(Count--) {
	*Buffer++ = __indword(Port);
    }
}

PPC_QUAL void __outbyte(unsigned long const Port, const unsigned char Data)
{
    __asm__(
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"sync\n\t"
	"eieio\n\t"
	"stb   %1,0(%0)\n\t"    /* Set actual value at phys addr r3 */
	"dcbst 0,%1\n\t"
	"mtmsr 5\n\t"
	"sync\n\t"
	"eieio\n\t" : : "b" (Port), "r" (Data)
	);
}

PPC_QUAL void __outword(unsigned long const Port, const unsigned short Data)
{
    __asm__(
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"sync\n\t"
	"eieio\n\t"
	"sth   %1,0(%0)\n\t"    /* Set actual value at phys addr r3 */
	"dcbst 0,%1\n\t"
	"mtmsr 5\n\t"
	"sync\n\t"
	"eieio\n\t" : : "b" (Port), "b" (Data)
	);
}

PPC_QUAL void __outdword(unsigned long const Port, const unsigned long Data)
{
    __asm__(
	"mfmsr 5\n\t"
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"sync\n\t"
	"eieio\n\t"
	"stw   %1,0(%0)\n\t"    /* Set actual value at phys addr r3 */
	"dcbst 0,%1\n\t"
	"mtmsr 5\n\t"
	"sync\n\t"
	"eieio\n\t" : : "b" (Port), "b" (Data)
	);
}

PPC_QUAL void __outbytestring(unsigned long const Port, const unsigned char * const Buffer, const unsigned long Count)
{
    unsigned long count = Count;
    const unsigned char *buffer = Buffer;
    while(count--) {
	__outbyte(Port, *buffer++);
    }
}

PPC_QUAL void __outwordstring(unsigned long const Port, const unsigned short * const Buffer, const unsigned long Count)
{
    unsigned long count = Count;
    const unsigned short *buffer = Buffer;
    while(count--) {
	__outword(Port, *buffer++);
    }
}

PPC_QUAL void __outdwordstring(unsigned long const Port, const unsigned long * const Buffer, const unsigned long Count)
{
    unsigned long count = Count;
    const unsigned long *buffer = Buffer;
    while(count--) {
	__outdword(Port, *buffer++);
    }
}


/*** System information ***/
PPC_QUAL void __cpuid(int CPUInfo[], const int InfoType)
{
    unsigned long lo32;
    __asm__("mfpvr" : "=b" (lo32));
}

PPC_QUAL unsigned long long __rdtsc(void)
{
    unsigned long lo32;
    __asm__("mfdec %0" : "=b" (lo32));
    return -lo32;
}


/*** Interrupts ***/
/* Finally decided to do this by enabling single step trap */
PPC_QUAL void __debugbreak(void)
{

}

PPC_QUAL void __int2c(void)
{
    /* Not sure yet */
}

#ifndef _ENABLE_DISABLE_DEFINED
#define _ENABLE_DISABLE_DEFINED
PPC_QUAL void _disable(void)
{
    __asm__ __volatile__("mfmsr 0\n\t" \
			 "li    8,0x7fff\n\t" \
			 "and   0,8,0\n\t" \
			 "mtmsr 0\n\t");
}

PPC_QUAL void _enable(void)
{
    __asm__ __volatile__("mfmsr 8\n\t" \
			 "ori   8,8,0x8000\n\t" \
			 "mtmsr 8\n\t");
}

/*** Protected memory management ***/
PPC_QUAL unsigned long __readsdr1(void)
{
    unsigned long value;
    __asm__("mfsdr1 %0" : "=b" (value));
    return value;
}

PPC_QUAL void __writesdr1(const unsigned long long Data)
{
    __asm__("mtsdr1 %0" : : "b" (Data));
}

/*** System operations ***/
/* This likely has a different meaning from the X86 equivalent.  We'll keep
 * the name cause it fits */
PPC_QUAL unsigned long long __readmsr()
{
    unsigned long temp;
    __asm__("mfmsr %0" : "=b" (temp));
    return temp;
}

PPC_QUAL void __writemsr(const unsigned long Value)
{
    __asm__("mtmsr %0" : : "b" (Value));
}

/* We'll make sure of the following:
 * IO operations have completed
 * Write operations through cache have completed
 * We've reloaded anything in the data or instruction cache that might have
 * changed in real ram.
 */
PPC_QUAL void __wbinvd(void)
{
    __asm__("eieio\n\t"
	    "dcs\n\t"
	    "sync\n\t"
	    "isync\n\t");
}
#endif

PPC_QUAL long _InterlockedAddLargeStatistic(volatile long long * const Addend, const long Value)
{
#if 0
	__asm__
	(
		"lock; add %[Value], %[Lo32];"
		"jae LABEL%=;"
		"lock; adc $0, %[Hi32];"
		"LABEL%=:;" :
		[Lo32] "=m" (*((volatile long *)(Addend) + 0)), [Hi32] "=m" (*((volatile long *)(Addend) + 1)) :
		[Value] "ir" (Value)
	);
#endif
	return Value;
}

/*** Miscellaneous ***/
/* BUGBUG: only good for use in macros. Cannot be taken the address of */
#define __noop(...) ((void)0)

#define __assume(x) if (!(x)) __builtin_unreachable()

#endif
/* EOF */
