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

#ifndef KJK_INTRIN_H_
#define KJK_INTRIN_H_

#ifndef __GNUC__
#error Unsupported compiler
#endif

/*
	FIXME: review all "memory" clobbers, add/remove to match Visual C++
	behavior: some "obvious" memory barriers are not present in the Visual C++
	implementation - e.g. __stosX; on the other hand, some memory barriers that
	*are* present could have been missed
*/
/*
	FIXME: atomic intrinsics haven't been tested yet
*/

/*
	NOTE: this is a *compatibility* header. Some functions may look wrong at
	first, but they're only "as wrong" as they would be on Visual C++. Our
	priority is compatibility

	NOTE: unlike most people who write inline asm for GCC, I didn't pull the
	constraints and the uses of __volatile__ out of my... hat. Do not touch
	them. I hate cargo cult programming

	NOTE: be very careful with declaring "memory" clobbers. Some "obvious"
	barriers aren't there in Visual C++ (e.g. __stosX)

	NOTE: review all intrinsics with a return value, add/remove __volatile__
	where necessary. If an intrinsic whose value is ignored generates a no-op
	under Visual C++, __volatile__ must be omitted; if it always generates code
	(for example, if it has side effects), __volatile__ must be specified. GCC
	will only optimize out non-volatile asm blocks with outputs, so input-only
	blocks are safe. Oddities such as the non-volatile 'rdmsr' are intentional
	and follow Visual C++ behavior

	NOTE: on GCC 4.1.0, please use the __sync_* built-ins for barriers and
	atomic operations. Test the version like this:

	#if (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__) > 40100
		...

	Pay attention to the type of barrier. Make it match with what Visual C++
	would use in the same case
*/

/*
	BUGBUG: 'long long' arguments and returns mess up GCC royally. There has to
	be something we can do about them
*/
#ifdef __i386__

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

static __inline__ __attribute__((always_inline)) char _InterlockedCompareExchange8(volatile char * const Destination, const char Exchange, const char Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) short _InterlockedCompareExchange16(volatile short * const Destination, const short Exchange, const short Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) long _InterlockedCompareExchange(volatile long * const Destination, const long Exchange, const long Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) long long _InterlockedCompareExchange64(volatile long long * const Destination, const long long Exchange, const long long Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchange(volatile long * const Target, const long Value)
{
	/* NOTE: __sync_lock_test_and_set would be an acquire barrier, so we force a full barrier */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}

static __inline__ __attribute__((always_inline)) void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value)
{
	/* NOTE: ditto */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchangeAdd(volatile long * const Addend, const long Value)
{
	return __sync_fetch_and_add(Addend, Value);
}

static __inline__ __attribute__((always_inline)) char _InterlockedAnd8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_and(value, mask);
}

static __inline__ __attribute__((always_inline)) short _InterlockedAnd16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_and(value, mask);
}

static __inline__ __attribute__((always_inline)) long _InterlockedAnd(volatile long * const value, const long mask)
{
	return __sync_fetch_and_and(value, mask);
}

static __inline__ __attribute__((always_inline)) char _InterlockedOr8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_or(value, mask);
}

static __inline__ __attribute__((always_inline)) short _InterlockedOr16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_or(value, mask);
}

static __inline__ __attribute__((always_inline)) long _InterlockedOr(volatile long * const value, const long mask)
{
	return __sync_fetch_and_or(value, mask);
}

static __inline__ __attribute__((always_inline)) char _InterlockedXor8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_xor(value, mask);
}

static __inline__ __attribute__((always_inline)) short _InterlockedXor16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_xor(value, mask);
}

static __inline__ __attribute__((always_inline)) long _InterlockedXor(volatile long * const value, const long mask)
{
	return __sync_fetch_and_xor(value, mask);
}

#else

static __inline__ __attribute__((always_inline)) char _InterlockedCompareExchange8(volatile char * const Destination, const char Exchange, const char Comperand)
{
	char retval = Comperand;
	__asm__("lock; cmpxchgb %b[Exchange], %[Destination]" : "a" (retval) : [Destination] "rm" (Destination), [Exchange] "r" (Exchange) : "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) short _InterlockedCompareExchange16(volatile short * const Destination, const short Exchange, const short Comperand)
{
	short retval = Comperand;
	__asm__("lock; cmpxchgw %w[Exchange], %[Destination]" : "a" (retval) : [Destination] "rm" (Destination), [Exchange] "r" (Exchange): "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) long _InterlockedCompareExchange(volatile long * const Destination, const long Exchange, const long Comperand)
{
	long retval = Comperand;
	__asm__("lock; cmpxchgl %k[Exchange], %[Destination]" : "a" (retval) : [Destination] "rm" (Destination), [Exchange] "r" (Exchange): "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) long long _InterlockedCompareExchange64(volatile long long * const Destination, const long long Exchange, const long long Comperand)
{
	unsigned long lo32Retval = (unsigned long)((Comperand >>  0) & 0xFFFFFFFF);
	long hi32Retval = (unsigned long)((Comperand >> 32) & 0xFFFFFFFF);

	unsigned long lo32Exchange = (unsigned long)((Exchange >>  0) & 0xFFFFFFFF);
	long hi32Exchange = (unsigned long)((Exchange >> 32) & 0xFFFFFFFF);

	__asm__
	(
		"cmpxchg8b %[Destination]" :
		"a" (lo32Retval), "d" (hi32Retval) :
		[Destination] "rm" (Destination), "b" (lo32Exchange), "c" (hi32Exchange) :
		"memory"
	);

	{
		union u_
		{
			long long ll;
			struct s_
			{
				unsigned long lo32;
				long hi32;
			}
			s;
		}
		u = { s : { lo32 : lo32Retval, hi32 : hi32Retval } };

		return u.ll;
	}
}

static __inline__ __attribute__((always_inline)) void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand)
{
	void * retval;
	__asm__("lock; cmpxchgl %k[Exchange], %[Destination]" : "a" (retval) : [Destination] "rm" (Destination), [Exchange] "r" (Exchange), "a" (Comperand) : "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchange(volatile long * const Target, const long Value)
{
	long retval = Value;
	__asm__("lock; xchgl %[retval], %[Target]" : [retval] "r" (retval) : [Target] "rm" (Target) : "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value)
{
	void * retval = Value;
	__asm__("lock; xchgl %[retval], %[Target]" : [retval] "r" (retval) : [Target] "rm" (Target) : "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchangeAdd(volatile long * const Addend, const long Value)
{
	long retval = Value;
	__asm__("lock; xaddl %[retval], %[Addend]" : "r" (retval) : "rm" (Addend));
	return retval;
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

#endif

static __inline__ __attribute__((always_inline)) long _InterlockedAddLargeStatistic(volatile long long * const Addend, const long Value)
{
	__asm__
	(
		"lock; add %[Value], %[Lo32];"
		"jae LABEL%=;"
		"lock; adc $0, %[Hi32];"
		"LABEL%=:;" :
		[Lo32] "=m" (*((volatile long *)(Addend) + 0)), [Hi32] "=m" (*((volatile long *)(Addend) + 1)) :
		[Value] "ir" (Value)
	);

	return Value;
}

static __inline__ __attribute__((always_inline)) long _InterlockedDecrement(volatile long * const lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, -1) - 1;
}

static __inline__ __attribute__((always_inline)) long _InterlockedIncrement(volatile long * const lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, 1) + 1;
}

static __inline__ __attribute__((always_inline)) unsigned char _interlockedbittestandreset(volatile long * const a, const long b)
{
	unsigned char retval;
	__asm__("lock; btr %[b], %[a]; setc %b[retval]" : [retval] "=q" (retval) : [a] "rm" (a), [b] "Nr" (b));
	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned char _interlockedbittestandset(volatile long * const a, const long b)
{
	unsigned char retval;
	__asm__("lock; bts %[b], %[a]; setc %b[retval]" : [retval] "=q" (retval) : [a] "rm" (a), [b] "Nr" (b));
	return retval;
}


/*** String operations ***/
/* NOTE: we don't set a memory clobber in the __stosX functions because Visual C++ doesn't */
static __inline__ __attribute__((always_inline)) void __stosb(unsigned char * Dest, const unsigned char Data, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; stosb" :
		[Dest] "=D" (Dest), [Count] "=c" (Count) :
		"[Dest]" (Dest), "a" (Data), "[Count]" (Count)
	);
}

static __inline__ __attribute__((always_inline)) void __stosw(unsigned short * Dest, const unsigned short Data, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; stosw" :
		[Dest] "=D" (Dest), [Count] "=c" (Count) :
		"[Dest]" (Dest), "a" (Data), "[Count]" (Count)
	);
}

static __inline__ __attribute__((always_inline)) void __stosd(unsigned long * Dest, const unsigned long Data, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; stosl" :
		[Dest] "=D" (Dest), [Count] "=c" (Count) :
		"[Dest]" (Dest), "a" (Data), "[Count]" (Count)
	);
}

static __inline__ __attribute__((always_inline)) void __movsb(unsigned char * Destination, const unsigned char * Source, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; movsb" :
		[Destination] "=D" (Destination), [Source] "=S" (Source), [Count] "=c" (Count) :
		"[Destination]" (Destination), "[Source]" (Source), "[Count]" (Count)
	);
}

static __inline__ __attribute__((always_inline)) void __movsw(unsigned short * Destination, const unsigned short * Source, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; movsw" :
		[Destination] "=D" (Destination), [Source] "=S" (Source), [Count] "=c" (Count) :
		"[Destination]" (Destination), "[Source]" (Source), "[Count]" (Count)
	);
}

static __inline__ __attribute__((always_inline)) void __movsd(unsigned long * Destination, const unsigned long * Source, size_t Count)
{
	__asm__ __volatile__
	(
		"rep; movsd" :
		[Destination] "=D" (Destination), [Source] "=S" (Source), [Count] "=c" (Count) :
		"[Destination]" (Destination), "[Source]" (Source), "[Count]" (Count)
	);
}


/*** FS segment addressing ***/
static __inline__ __attribute__((always_inline)) void __writefsbyte(const unsigned long Offset, const unsigned char Data)
{
	__asm__("movb %b[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data));
}

static __inline__ __attribute__((always_inline)) void __writefsword(const unsigned long Offset, const unsigned short Data)
{
	__asm__("movw %w[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data));
}

static __inline__ __attribute__((always_inline)) void __writefsdword(const unsigned long Offset, const unsigned long Data)
{
	__asm__("movl %k[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data));
}

static __inline__ __attribute__((always_inline)) unsigned char __readfsbyte(const unsigned long Offset)
{
	unsigned char value;
	__asm__("movb %%fs:%a[Offset], %b[value]" : [value] "=q" (value) : [Offset] "irm" (Offset));
	return value;
}

static __inline__ __attribute__((always_inline)) unsigned short __readfsword(const unsigned long Offset)
{
	unsigned short value;
	__asm__("movw %%fs:%a[Offset], %w[value]" : [value] "=q" (value) : [Offset] "irm" (Offset));
	return value;
}

static __inline__ __attribute__((always_inline)) unsigned long __readfsdword(const unsigned long Offset)
{
	unsigned long value;
	__asm__("movl %%fs:%a[Offset], %k[value]" : [value] "=q" (value) : [Offset] "irm" (Offset));
	return value;
}

static __inline__ __attribute__((always_inline)) void __incfsbyte(const unsigned long Offset)
{
	__asm__("incb %%fs:%a[Offset]" : : [Offset] "ir" (Offset));
}

static __inline__ __attribute__((always_inline)) void __incfsword(const unsigned long Offset)
{
	__asm__("incw %%fs:%a[Offset]" : : [Offset] "ir" (Offset));
}

static __inline__ __attribute__((always_inline)) void __incfsdword(const unsigned long Offset)
{
	__asm__("incl %%fs:%a[Offset]" : : [Offset] "ir" (Offset));
}

/* NOTE: the bizarre implementation of __addfsxxx mimics the broken Visual C++ behavior */
static __inline__ __attribute__((always_inline)) void __addfsbyte(const unsigned long Offset, const unsigned char Data)
{
	if(!__builtin_constant_p(Offset))
		__asm__("addb %k[Offset], %%fs:%a[Offset]" : : [Offset] "r" (Offset));
	else
		__asm__("addb %b[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data));
}

static __inline__ __attribute__((always_inline)) void __addfsword(const unsigned long Offset, const unsigned short Data)
{
	if(!__builtin_constant_p(Offset))
		__asm__("addw %k[Offset], %%fs:%a[Offset]" : : [Offset] "r" (Offset));
	else
		__asm__("addw %w[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data));
}

static __inline__ __attribute__((always_inline)) void __addfsdword(const unsigned long Offset, const unsigned int Data)
{
	if(!__builtin_constant_p(Offset))
		__asm__("addl %k[Offset], %%fs:%a[Offset]" : : [Offset] "r" (Offset));
	else
		__asm__("addl %k[Data], %%fs:%a[Offset]" : : [Offset] "ir" (Offset), [Data] "iq" (Data));
}


/*** Bit manipulation ***/
static  __inline__ __attribute__((always_inline)) unsigned char _BitScanForward(unsigned long * const Index, const unsigned long Mask)
{
	__asm__("bsfl %[Mask], %[Index]" : [Index] "=r" (*Index) : [Mask] "mr" (Mask));
	return Mask ? 1 : 0;
}

static  __inline__ __attribute__((always_inline)) unsigned char _BitScanReverse(unsigned long * const Index, const unsigned long Mask)
{
	__asm__("bsrl %[Mask], %[Index]" : [Index] "=r" (*Index) : [Mask] "mr" (Mask));
	return Mask ? 1 : 0;
}

/* NOTE: again, the bizarre implementation follows Visual C++ */
static  __inline__ __attribute__((always_inline)) unsigned char _bittest(const long * const a, const long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("bt %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*(a + (b / 32))), [b] "Ir" (b % 32));
	else
		__asm__("bt %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*a), [b] "r" (b));

	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned char _bittestandcomplement(long * const a, const long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("btc %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*(a + (b / 32))), [b] "Ir" (b % 32));
	else
		__asm__("btc %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*a), [b] "r" (b));

	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned char _bittestandreset(long * const a, const long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("btr %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*(a + (b / 32))), [b] "Ir" (b % 32));
	else
		__asm__("btr %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*a), [b] "r" (b));

	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned char _bittestandset(long * const a, const long b)
{
	unsigned char retval;

	if(__builtin_constant_p(b))
		__asm__("bts %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*(a + (b / 32))), [b] "Ir" (b % 32));
	else
		__asm__("bts %[b], %[a]; setb %b[retval]" : [retval] "=q" (retval) : [a] "mr" (*a), [b] "r" (b));

	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned char _rotl8(const unsigned char value, const unsigned char shift)
{
	unsigned char retval;
	__asm__("rolb %b[shift], %b[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned short _rotl16(const unsigned short value, const unsigned char shift)
{
	unsigned short retval;
	__asm__("rolw %b[shift], %w[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned char _rotr8(const unsigned char value, const unsigned char shift)
{
	unsigned char retval;
	__asm__("rorb %b[shift], %b[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned short _rotr16(const unsigned short value, const unsigned char shift)
{
	unsigned short retval;
	__asm__("rorw %b[shift], %w[retval]" : [retval] "=rm" (retval) : "[retval]" (value), [shift] "Nc" (shift));
	return retval;
}

static __inline__ __attribute__((always_inline)) unsigned long long __ll_lshift(const unsigned long long Mask, int Bit)
{
	unsigned long lo32 = (unsigned long)((Mask >>  0) & 0xFFFFFFFF);
	unsigned long hi32 = (unsigned long)((Mask >> 32) & 0xFFFFFFFF);

	__asm__
	(
		"shldl %b[Bit], %k[Lo32], %k[Hi32]; sall %b[Bit], %k[Lo32]" :
		[Lo32] "=q" (lo32), [Hi32] "=qm" (hi32):
		"[Lo32]" (lo32), "[Hi32]" (hi32), [Bit] "Ic" (Bit)
	);

	{
		union u_
		{
			unsigned long long ull;
			struct s_
			{
				unsigned long lo32;
				unsigned long hi32;
			}
			s;
		}
		u = { s : { lo32 : lo32, hi32 : hi32 } };

		return u.ull;
	}
}

static __inline__ __attribute__((always_inline)) long long __ll_rshift(const long long Mask, const int Bit)
{
	unsigned long lo32 = (unsigned long)((Mask >>  0) & 0xFFFFFFFF);
	long hi32 = (unsigned long)((Mask >> 32) & 0xFFFFFFFF);

	__asm__
	(
		"shrdl %b[Bit], %k[Lo32], %k[Hi32]; sarl %b[Bit], %k[Lo32]" :
		[Lo32] "=q" (lo32), [Hi32] "=qm" (hi32):
		"[Lo32]" (lo32), "[Hi32]" (hi32), [Bit] "Ic" (Bit)
	);

	{
		union u_
		{
			long long ll;
			struct s_
			{
				unsigned long lo32;
				long hi32;
			}
			s;
		}
		u = { s : { lo32 : lo32, hi32 : hi32 } };

		return u.ll;
	}
}

static __inline__ __attribute__((always_inline)) unsigned long long __ull_rshift(const unsigned long long Mask, int Bit)
{
	unsigned long lo32 = (unsigned long)((Mask >>  0) & 0xFFFFFFFF);
	unsigned long hi32 = (unsigned long)((Mask >> 32) & 0xFFFFFFFF);

	__asm__
	(
		"shrdl %b[Bit], %k[Hi32], %k[Lo32]; shrl %b[Bit], %k[Hi32]" :
		[Lo32] "=qm" (lo32), [Hi32] "=q" (hi32):
		"[Lo32]" (lo32), "[Hi32]" (hi32), [Bit] "Ic" (Bit)
	);

	{
		union u_
		{
			unsigned long long ull;
			struct s_
			{
				unsigned long lo32;
				unsigned long hi32;
			}
			s;
		}
		u = { s : { lo32 : lo32, hi32 : hi32 } };

		return u.ull;
	}
}


/*** 64-bit math ***/
static __inline__ __attribute__((always_inline)) long long __emul(const int a, const int b)
{
	unsigned long lo32;
	long hi32;

	__asm__("imul %[b]" : "=a" (lo32), "=d" (hi32) : [a] "a" (a), [b] "rm" (b));

	{
		union u_
		{
			long long ll;
			struct s_
			{
				unsigned long lo32;
				long hi32;
			}
			s;
		}
		u = { s : { lo32 : lo32, hi32 : hi32 } };

		return u.ll;
	}
}

static __inline__ __attribute__((always_inline)) unsigned long long __emulu(const unsigned int a, const unsigned int b)
{
	unsigned long lo32;
	unsigned long hi32;

	__asm__("mul %[b]" : "=a" (lo32), "=d" (hi32) : [a] "a" (a), [b] "rm" (b));

	{
		union u_
		{
			unsigned long long ull;
			struct s_
			{
				unsigned long lo32;
				unsigned long hi32;
			}
			s;
		}
		u = { s : { lo32 : lo32, hi32 : hi32 } };

		return u.ull;
	}
}


/*** Port I/O ***/
static __inline__ __attribute__((always_inline)) unsigned char __inbyte(const unsigned short Port)
{
	unsigned char byte;
	__asm__ __volatile__("inb %w[Port], %b[byte]" : [byte] "=a" (byte) : [Port] "Nd" (Port));
	return byte;
}

static __inline__ __attribute__((always_inline)) unsigned short __inword(const unsigned short Port)
{
	unsigned short word;
	__asm__ __volatile__("inw %w[Port], %w[word]" : [word] "=a" (word) : [Port] "Nd" (Port));
	return word;
}

static __inline__ __attribute__((always_inline)) unsigned long __indword(const unsigned short Port)
{
	unsigned long dword;
	__asm__ __volatile__("inl %w[Port], %k[dword]" : [dword] "=a" (dword) : [Port] "Nd" (Port));
	return dword;
}

static __inline__ __attribute__((always_inline)) void __inbytestring(unsigned short Port, unsigned char * Buffer, unsigned long Count)
{
	__asm__ __volatile__
	(
		"rep; insb" :
		[Buffer] "=D" (Buffer), [Count] "=c" (Count) :
		"d" (Port), "[Buffer]" (Buffer), "[Count]" (Count) :
		"memory"
	);
}

static __inline__ __attribute__((always_inline)) void __inwordstring(unsigned short Port, unsigned short * Buffer, unsigned long Count)
{
	__asm__ __volatile__
	(
		"rep; insw" :
		[Buffer] "=D" (Buffer), [Count] "=c" (Count) :
		"d" (Port), "[Buffer]" (Buffer), "[Count]" (Count) :
		"memory"
	);
}

static __inline__ __attribute__((always_inline)) void __indwordstring(unsigned short Port, unsigned long * Buffer, unsigned long Count)
{
	__asm__ __volatile__
	(
		"rep; insl" :
		[Buffer] "=D" (Buffer), [Count] "=c" (Count) :
		"d" (Port), "[Buffer]" (Buffer), "[Count]" (Count) :
		"memory"
	);
}

static __inline__ __attribute__((always_inline)) void __outbyte(unsigned short const Port, const unsigned char Data)
{
	__asm__ __volatile__("outb %b[Data], %w[Port]" : : [Port] "Nd" (Port), [Data] "a" (Data));
}

static __inline__ __attribute__((always_inline)) void __outword(unsigned short const Port, const unsigned short Data)
{
	__asm__ __volatile__("outw %w[Data], %w[Port]" : : [Port] "Nd" (Port), [Data] "a" (Data));
}

static __inline__ __attribute__((always_inline)) void __outdword(unsigned short const Port, const unsigned long Data)
{
	__asm__ __volatile__("outl %k[Data], %w[Port]" : : [Port] "Nd" (Port), [Data] "a" (Data));
}

static __inline__ __attribute__((always_inline)) void __outbytestring(unsigned short const Port, const unsigned char * const Buffer, const unsigned long Count)
{
	__asm__ __volatile__("rep; outsb" : : [Port] "d" (Port), [Buffer] "S" (Buffer), "c" (Count));
}

static __inline__ __attribute__((always_inline)) void __outwordstring(unsigned short const Port, const unsigned short * const Buffer, const unsigned long Count)
{
	__asm__ __volatile__("rep; outsw" : : [Port] "d" (Port), [Buffer] "S" (Buffer), "c" (Count));
}

static __inline__ __attribute__((always_inline)) void __outdwordstring(unsigned short const Port, const unsigned long * const Buffer, const unsigned long Count)
{
	__asm__ __volatile__("rep; outsl" : : [Port] "d" (Port), [Buffer] "S" (Buffer), "c" (Count));
}


/*** System information ***/
static __inline__ __attribute__((always_inline)) void __cpuid(int CPUInfo[], const int InfoType)
{
	__asm__("cpuid" : "=a" (CPUInfo[0]), "=b" (CPUInfo[1]), "=c" (CPUInfo[2]), "=d" (CPUInfo[3]) : "a" (InfoType));
}

static __inline__ __attribute__((always_inline)) unsigned long long __rdtsc(void)
{
	unsigned long lo32;
	unsigned long hi32;

	__asm__("rdtsc" : "=a" (lo32), "=d" (hi32));

	{
		union u_
		{
			unsigned long long ull;
			struct s_
			{
				unsigned long lo32;
				unsigned long hi32;
			}
			s;
		}
		u = { s : { lo32 : lo32, hi32 : hi32 } };

		return u.ull;
	}
}


/*** Interrupts ***/
static __inline__ __attribute__((always_inline)) void __debugbreak(void)
{
	__asm__("int $3");
}

static __inline__ __attribute__((always_inline)) void __int2c(void)
{
	__asm__("int $0x2c");
}

static __inline__ __attribute__((always_inline)) void _disable(void)
{
	__asm__("cli");
}

static __inline__ __attribute__((always_inline)) void _enable(void)
{
	__asm__("sti");
}


/*** Protected memory management ***/
static __inline__ __attribute__((always_inline)) unsigned long __readcr0(void)
{
	unsigned long value;
	__asm__("mov %%cr0, %[value]" : [value] "=q" (value));
	return value;
}

static __inline__ __attribute__((always_inline)) unsigned long __readcr2(void)
{
	unsigned long value;
	__asm__("mov %%cr2, %[value]" : [value] "=q" (value));
	return value;
}

static __inline__ __attribute__((always_inline)) unsigned long __readcr3(void)
{
	unsigned long value;
	__asm__("mov %%cr3, %[value]" : [value] "=q" (value));
	return value;
}

static __inline__ __attribute__((always_inline)) unsigned long __readcr4(void)
{
	unsigned long value;
	__asm__("mov %%cr4, %[value]" : [value] "=q" (value));
	return value;
}

static __inline__ __attribute__((always_inline)) void __writecr0(const unsigned long long Data)
{
	__asm__("mov %[Data], %%cr0" : : [Data] "q" ((const unsigned long)(Data & 0xFFFFFFFF)));
}

static __inline__ __attribute__((always_inline)) void __writecr3(const unsigned long long Data)
{
	__asm__("mov %[Data], %%cr3" : : [Data] "q" ((const unsigned long)(Data & 0xFFFFFFFF)));
}

static __inline__ __attribute__((always_inline)) void __writecr4(const unsigned long long Data)
{
	__asm__("mov %[Data], %%cr4" : : [Data] "q" ((const unsigned long)(Data & 0xFFFFFFFF)));
}

static __inline__ __attribute__((always_inline)) void __invlpg(void * const Address)
{
	__asm__("invlpg %[Address]" : : [Address] "m" (*((unsigned char *)(Address))));
}


/*** System operations ***/
static __inline__ __attribute__((always_inline)) unsigned long long __readmsr(const int reg)
{
	unsigned long lo32;
	unsigned long hi32;

	__asm__("rdmsr" : "=a" (lo32), "=d" (hi32) : "c" (reg));

	{
		union u_
		{
			unsigned long long ull;
			struct s_
			{
				unsigned long lo32;
				unsigned long hi32;
			}
			s;
		}
		u = { s : { lo32 : lo32, hi32 : hi32 } };

		return u.ull;
	}
}

static __inline__ __attribute__((always_inline)) void __writemsr(const unsigned long Register, const unsigned long long Value)
{
	__asm__
	(
		"wrmsr" :
		:
		"a" ((unsigned long)((Value >>  0) & 0xFFFFFFFF)),
		"d" ((unsigned long)((Value >> 32) & 0xFFFFFFFF)),
		"c" (Register)
	);
}

static __inline__ __attribute__((always_inline)) unsigned long long __readpmc(const int counter)
{
	unsigned long lo32;
	unsigned long hi32;

	__asm__("rdpmc" : "=a" (lo32), "=d" (hi32) : "c" (counter));

	{
		union u_
		{
			unsigned long long ull;
			struct s_
			{
				unsigned long lo32;
				unsigned long hi32;
			}
			s;
		}
		u = { s : { lo32 : lo32, hi32 : hi32 } };

		return u.ull;
	}
}

/* NOTE: an immediate value for 'a' will raise an ICE in Visual C++ */
static __inline__ __attribute__((always_inline)) unsigned long __segmentlimit(const unsigned long a)
{
	unsigned long retval;
	__asm__ __volatile__("lsl %[a], %[retval]" : [retval] "=r" (retval) : [a] "rm" (a));
	return retval;
}

static __inline__ __attribute__((always_inline)) void __wbinvd(void)
{
	__asm__("wbinvd");
}

#elif defined(_M_PPC)

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

static __inline__ __attribute__((always_inline)) char _InterlockedCompareExchange8(volatile char * const Destination, const char Exchange, const char Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) short _InterlockedCompareExchange16(volatile short * const Destination, const short Exchange, const short Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) long _InterlockedCompareExchange(volatile long * const Destination, const long Exchange, const long Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) long long _InterlockedCompareExchange64(volatile long long * const Destination, const long long Exchange, const long long Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand)
{
	return __sync_val_compare_and_swap(Destination, Comperand, Exchange);
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchange(volatile long * const Target, const long Value)
{
	/* NOTE: __sync_lock_test_and_set would be an acquire barrier, so we force a full barrier */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}

static __inline__ __attribute__((always_inline)) void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value)
{
	/* NOTE: ditto */
	__sync_synchronize();
	return __sync_lock_test_and_set(Target, Value);
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchangeAdd(volatile long * const Addend, const long Value)
{
	return __sync_fetch_and_add(Addend, Value);
}

static __inline__ __attribute__((always_inline)) char _InterlockedAnd8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_and(value, mask);
}

static __inline__ __attribute__((always_inline)) short _InterlockedAnd16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_and(value, mask);
}

static __inline__ __attribute__((always_inline)) long _InterlockedAnd(volatile long * const value, const long mask)
{
	return __sync_fetch_and_and(value, mask);
}

static __inline__ __attribute__((always_inline)) char _InterlockedOr8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_or(value, mask);
}

static __inline__ __attribute__((always_inline)) short _InterlockedOr16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_or(value, mask);
}

static __inline__ __attribute__((always_inline)) long _InterlockedOr(volatile long * const value, const long mask)
{
	return __sync_fetch_and_or(value, mask);
}

static __inline__ __attribute__((always_inline)) char _InterlockedXor8(volatile char * const value, const char mask)
{
	return __sync_fetch_and_xor(value, mask);
}

static __inline__ __attribute__((always_inline)) short _InterlockedXor16(volatile short * const value, const short mask)
{
	return __sync_fetch_and_xor(value, mask);
}

static __inline__ __attribute__((always_inline)) long _InterlockedXor(volatile long * const value, const long mask)
{
	return __sync_fetch_and_xor(value, mask);
}

#else

static __inline__ __attribute__((always_inline)) char _InterlockedCompareExchange8(volatile char * const Destination, const char Exchange, const char Comperand)
{
	char retval = Comperand;
	__asm__ __volatile__ (
	    "sync\n"
	    "1: lbarx   %0,0,%1\n"
	    "   subf.   %0,%2,%0\n"
	    "   bne     2f\n"
	    "   stbcx.  %3,0,%1\n"
	    "   bne-    1b\n"
	    "2: isync"
	    : "=b" (retval)
	    : "b" (Destination), "r" (Comperand), "r" (Exchange)
	    : "cr0", "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) short _InterlockedCompareExchange16(volatile short * const Destination, const short Exchange, const short Comperand)
{
	short retval = Comperand;
	__asm__ __volatile__ (
	    "sync\n"
	    "1: lharx   %0,0,%1\n"
	    "   subf.   %0,%2,%0\n"
	    "   bne     2f\n"
	    "   sthcx.  %3,0,%1\n"
	    "   bne-    1b\n"
	    "2: isync"
	    : "=b" (retval)
	    : "b" (Destination), "r" (Comperand), "r" (Exchange)
	    : "cr0", "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) long _InterlockedCompareExchange(volatile long * const Destination, const long Exchange, const long Comperand)
{
	long retval = Comperand;
	__asm__ __volatile__ (
	    "sync\n"
	    "1: lwarx   %0,0,%1\n"
	    "   subf.   %0,%2,%0\n"
	    "   bne     2f\n"
	    "   stwcx.  %3,0,%1\n"
	    "   bne-    1b\n"
	    "2: isync"
	    : "=b" (retval)
	    : "b" (Destination), "r" (Comperand), "r" (Exchange)
	    : "cr0", "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) long long _InterlockedCompareExchange64(volatile long long * const Destination, const long long Exchange, const long long Comperand)
{
	unsigned long lo32Retval = (unsigned long)((Comperand >>  0) & 0xFFFFFFFF);
	long hi32Retval = (unsigned long)((Comperand >> 32) & 0xFFFFFFFF);

	unsigned long lo32Exchange = (unsigned long)((Exchange >>  0) & 0xFFFFFFFF);
	long hi32Exchange = (unsigned long)((Exchange >> 32) & 0xFFFFFFFF);

#if 0
	__asm__
	(
		"cmpxchg8b %[Destination]" :
		"a" (lo32Retval), "d" (hi32Retval) :
		[Destination] "rm" (Destination), "b" (lo32Exchange), "c" (hi32Exchange) :
		"memory"
	);
#endif
	{
		union u_
		{
			long long ll;
			struct s_
			{
				unsigned long lo32;
				long hi32;
			}
			s;
		}
		u = { s : { lo32 : lo32Retval, hi32 : hi32Retval } };

		return u.ll;
	}
}

static __inline__ __attribute__((always_inline)) void * _InterlockedCompareExchangePointer(void * volatile * const Destination, void * const Exchange, void * const Comperand)
{
    return (void *)_InterlockedCompareExchange
	((long *)Destination, (long) Exchange, (long) Comperand);
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchange(volatile long * const Target, const long Value)
{
        long retval;
	__asm__ __volatile__ (
	    "sync\n"
	    "1: lwarx   %0,0,%1\n"
	    "   stwcx.  %2,0,%1\n"
	    "   bne-    1b\n"
	    : "=b" (retval)
	    : "b" (Target), "b" (Value)
	    : "cr0", "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) void * _InterlockedExchangePointer(void * volatile * const Target, void * const Value)
{
        void * retval;
	__asm__ __volatile__ (
	    "sync\n"
	    "1: lwarx   %0,0,%1\n"
	    "   stwcx.  %2,0,%1\n"
	    "   bne-    1b\n"
	    : "=b" (retval)
	    : "b" (Target), "b" (Value)
	    : "cr0", "memory");
	return retval;
}

static __inline__ __attribute__((always_inline)) long _InterlockedExchangeAdd(volatile long * const Addend, const long Value)
{
        long x;
	long y = *Addend;
	long addend = y;
	
	do
	{
	    x = y;
	    y = _InterlockedCompareExchange(Addend, addend + Value, x);
	} 
	while(y != x);

	return y;
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

#endif

static __inline__ __attribute__((always_inline)) long _InterlockedAddLargeStatistic(volatile long long * const Addend, const long Value)
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

static __inline__ __attribute__((always_inline)) long _InterlockedDecrement(volatile long * const lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, -1) - 1;
}

static __inline__ __attribute__((always_inline)) long _InterlockedIncrement(volatile long * const lpAddend)
{
	return _InterlockedExchangeAdd(lpAddend, 1) + 1;
}

static __inline__ __attribute__((always_inline)) unsigned char _interlockedbittestandreset(volatile long * const a, const long b)
{
	long x;
	long y;
	long mask = ~(1<<b);

	y = *a;

	do
	{
		x = y;
		y = _InterlockedCompareExchange(a, x & mask, x);
	}
	while(y != x);

	return (y & ~mask) != 0;
}

static __inline__ __attribute__((always_inline)) unsigned char _interlockedbittestandset(volatile long * const a, const long b)
{
	long x;
	long y;
	long mask = 1<<b;

	y = *a;

	do
	{
		x = y;
		y = _InterlockedCompareExchange(a, x | mask, x);
	}
	while(y != x);

	return (y & ~mask) != 0;
}


/*** String operations ***/
/* NOTE: we don't set a memory clobber in the __stosX functions because Visual C++ doesn't */
/* Note that the PPC store multiple operations may raise an exception in LE 
 * mode */
static __inline__ __attribute__((always_inline)) void __stosb(unsigned char * Dest, const unsigned char Data, size_t Count)
{
    memset(Dest, Data, Count);
}

static __inline__ __attribute__((always_inline)) void __stosw(unsigned short * Dest, const unsigned short Data, size_t Count)
{
    while(Count--) 
	*Dest++ = Data;
}

static __inline__ __attribute__((always_inline)) void __stosd(unsigned long * Dest, const unsigned long Data, size_t Count)
{
    while(Count--)
	*Dest++ = Data;
}

static __inline__ __attribute__((always_inline)) void __movsb(unsigned char * Destination, const unsigned char * Source, size_t Count)
{
    memcpy(Destination, Source, Count);
}

static __inline__ __attribute__((always_inline)) void __movsw(unsigned short * Destination, const unsigned short * Source, size_t Count)
{
    memcpy(Destination, Source, Count * sizeof(*Source));
}

static __inline__ __attribute__((always_inline)) void __movsd(unsigned long * Destination, const unsigned long * Source, size_t Count)
{
    memcpy(Destination, Source, Count * sizeof(*Source));
}


/*** FS segment addressing ***/
/* On PowerPC, r13 points to TLS data, including the TEB at 0(r13) from what I
 * can tell */
static __inline__ __attribute__((always_inline)) void __writefsbyte(const unsigned long Offset, const unsigned char Data)
{
    char *addr;
    __asm__("\tadd %2,13,%0\n"
	    "\tstb %1,0(%2)\n" 
	    : 
	    : "b" (Offset), "b" (Data), "b" (addr));
}

static __inline__ __attribute__((always_inline)) void __writefsword(const unsigned long Offset, const unsigned short Data)
{
    char *addr;
    __asm__("\tadd %2,13,%0\n"
	    "\tsth %1,0(%2)\n"
	    : 
	    : "b" (Offset), "b" (Data), "b" (addr));
}

static __inline__ __attribute__((always_inline)) void __writefsdword(const unsigned long Offset, const unsigned long Data)
{
    char *addr;
    __asm__("\tadd %2,13,%0\n"
	    "\tstw %1,0(%2)\n" 
	    : 
	    : "b" (Offset), "b" (Data), "b" (addr));
}

static __inline__ __attribute__((always_inline)) unsigned char __readfsbyte(const unsigned long Offset)
{
    char result;
    char *addr;
    __asm__("\tadd %2,13,%1\n"
	    "\tlbz %0,0(%2)\n"
	    : "=b" (result)
	    : "b" (Offset), "b" (addr));
}

static __inline__ __attribute__((always_inline)) unsigned short __readfsword(const unsigned long Offset)
{
    unsigned short result;
    char *addr;
    __asm__("\tadd %2,13,%1\n"
	    "\tlhz %0,0(%2)\n"
	    : "=b" (result)
	    : "b" (Offset), "b" (addr));
    return result;
}

static __inline__ __attribute__((always_inline)) unsigned long __readfsdword(const unsigned long Offset)
{
    unsigned long result = 0;
    char *addr = 0;
    __asm__("\tadd %2,13,%1\n"
	    "\tlwz %0,0(%2)\n"
	    : "=b" (result)
	    : "b" (Offset), "b" (addr));
    return result;
}

static __inline__ __attribute__((always_inline)) void __incfsbyte(const unsigned long Offset)
{
    __writefsbyte(Offset, __readfsbyte(Offset)+1);
}

static __inline__ __attribute__((always_inline)) void __incfsword(const unsigned long Offset)
{
    __writefsword(Offset, __readfsword(Offset)+1);
}

static __inline__ __attribute__((always_inline)) void __incfsdword(const unsigned long Offset)
{
    __writefsdword(Offset, __readfsdword(Offset)+1);
}

/* NOTE: the bizarre implementation of __addfsxxx mimics the broken Visual C++ behavior */
/* PPC Note: Not sure about the bizarre behavior.  We'll try to emulate it later */
static __inline__ __attribute__((always_inline)) void __addfsbyte(const unsigned long Offset, const unsigned char Data)
{
    __writefsbyte(Offset, __readfsbyte(Offset) + Data);
}

static __inline__ __attribute__((always_inline)) void __addfsword(const unsigned long Offset, const unsigned short Data)
{
    __writefsword(Offset, __readfsword(Offset) + Data);
}

static __inline__ __attribute__((always_inline)) void __addfsdword(const unsigned long Offset, const unsigned int Data)
{
    __writefsdword(Offset, __readfsdword(Offset) + Data);
}


/*** Bit manipulation ***/
static  __inline__ __attribute__((always_inline)) unsigned char _BitScanForward(unsigned long * const Index, const unsigned long Mask)
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
static  __inline__ __attribute__((always_inline)) unsigned char _BitScanReverse(unsigned long * const Index, const unsigned long Mask)
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
static  __inline__ __attribute__((always_inline)) unsigned char _bittest(const long * const a, const long b)
{
    return ((*a) & (1<<b)) != 0;
}

static __inline__ __attribute__((always_inline)) unsigned char _bittestandcomplement(long * const a, const long b)
{
    unsigned char ret = ((*a) & (1<<b)) != 0;
    (*a) ^= (1<<b);
    return ret;
}

static __inline__ __attribute__((always_inline)) unsigned char _bittestandreset(long * const a, const long b)
{
    unsigned char ret = ((*a) & (1<<b)) != 0;
    (*a) &= ~(1<<b);
    return ret;
}

static __inline__ __attribute__((always_inline)) unsigned char _bittestandset(long * const a, const long b)
{
    unsigned char ret = ((*a) & (1<<b)) != 0;
    (*a) |= (1<<b);
    return ret;
}

static __inline__ __attribute__((always_inline)) unsigned char _rotl8(const unsigned char value, const unsigned char shift)
{
    return (value << shift) | (value >> (8-shift));
}

static __inline__ __attribute__((always_inline)) unsigned short _rotl16(const unsigned short value, const unsigned char shift)
{
    return (value << shift) | (value >> (16-shift));    
}

static __inline__ __attribute__((always_inline)) unsigned char _rotr8(const unsigned char value, const unsigned char shift)
{
    return (value >> shift) | (value << (8-shift));    
}

static __inline__ __attribute__((always_inline)) unsigned short _rotr16(const unsigned short value, const unsigned char shift)
{
    return (value >> shift) | (value << (16-shift));    
}

static __inline__ __attribute__((always_inline)) unsigned long long __ll_lshift(const unsigned long long Mask, int Bit)
{
    return Mask << Bit;
}

static __inline__ __attribute__((always_inline)) long long __ll_rshift(const long long Mask, const int Bit)
{
    return Mask >> Bit;
}

static __inline__ __attribute__((always_inline)) unsigned long long __ull_rshift(const unsigned long long Mask, int Bit)
{
    return Mask >> Bit;
}


/*** 64-bit math ***/
static __inline__ __attribute__((always_inline)) long long __emul(const int a, const int b)
{
    return a * b;
}

static __inline__ __attribute__((always_inline)) unsigned long long __emulu(const unsigned int a, const unsigned int b)
{
    return a * b;
}


/*** Port I/O ***/
static __inline__ __attribute__((always_inline)) unsigned char __inbyte(const unsigned short Port)
{
    int ret;
    __asm__(
	"mfmsr 5\n\t"
	"xori  %1,%1,7\n\t"     /* Undo effects of LE without swapping */
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"isync\n\t"
	"sync\n\t"
	"lbz   %0,0(%1)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t" : "=r" (ret) : "b" (Port)
    );
    return ret;
}

static __inline__ __attribute__((always_inline)) unsigned short __inword(const unsigned short Port)
{
    int ret;
    __asm__(
	"mfmsr 5\n\t"
	"xori  %1,%1,6\n\t"     /* Undo effects of LE without swapping */
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"isync\n\t"
	"sync\n\t"
	"lhz   %0,0(%1)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t" : "=r" (ret) : "b" (Port)
    );
    return ret;
}

static __inline__ __attribute__((always_inline)) unsigned long __indword(const unsigned short Port)
{
    int ret;
    __asm__(
	"mfmsr 5\n\t"
	"xori  %1,%1,4\n\t"     /* Undo effects of LE without swapping */
	"andi. 6,5,0xffef\n\t"/* turn off MSR[DR] */
	"mtmsr 6\n\t"
	"isync\n\t"
	"sync\n\t"
	"lwz   %0,0(%1)\n\t"    /* Get actual value at phys addr r3 */
	"mtmsr 5\n\t" : "=r" (ret) : "b" (Port)
    );
    return ret;
}

static __inline__ __attribute__((always_inline)) void __inbytestring(unsigned short Port, unsigned char * Buffer, unsigned long Count)
{
    while(Count--) {
	*Buffer++ = __inbyte(Port);
    }
}

static __inline__ __attribute__((always_inline)) void __inwordstring(unsigned short Port, unsigned short * Buffer, unsigned long Count)
{
    while(Count--) {
	*Buffer++ = __inword(Port);
    }
}

static __inline__ __attribute__((always_inline)) void __indwordstring(unsigned short Port, unsigned long * Buffer, unsigned long Count)
{
    while(Count--) {
	*Buffer++ = __indword(Port);
    }
}

static __inline__ __attribute__((always_inline)) void __outbyte(unsigned short const Port, const unsigned char Data)
{
    __asm__(
	"mfmsr 5\n\t"
	"xori  %0,%0,7\n\t"     /* Undo effects of LE without swapping */
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

static __inline__ __attribute__((always_inline)) void __outword(unsigned short const Port, const unsigned short Data)
{
    __asm__(
	"mfmsr 5\n\t"
	"xori  %0,%0,7\n\t"     /* Undo effects of LE without swapping */
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

static __inline__ __attribute__((always_inline)) void __outdword(unsigned short const Port, const unsigned long Data)
{
    __asm__(
	"mfmsr 5\n\t"
	"xori  %0,%0,7\n\t"     /* Undo effects of LE without swapping */
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

static __inline__ __attribute__((always_inline)) void __outbytestring(unsigned short const Port, const unsigned char * const Buffer, const unsigned long Count)
{
    unsigned long count = Count;
    unsigned char *buffer = Buffer;
    while(count--) {
	__outbyte(Port, *buffer++);
    }
}

static __inline__ __attribute__((always_inline)) void __outwordstring(unsigned short const Port, const unsigned short * const Buffer, const unsigned long Count)
{
    unsigned long count = Count;
    unsigned short *buffer = Buffer;
    while(count--) {
	__outword(Port, *buffer++);
    }
}

static __inline__ __attribute__((always_inline)) void __outdwordstring(unsigned short const Port, const unsigned long * const Buffer, const unsigned long Count)
{
    unsigned long count = Count;
    unsigned long *buffer = Buffer;
    while(count--) {
	__outdword(Port, *buffer++);
    }
}


/*** System information ***/
static __inline__ __attribute__((always_inline)) void __cpuid(int CPUInfo[], const int InfoType)
{
    unsigned long lo32;
    __asm__("mfpvr" : "=b" (lo32));
    return lo32;
}

static __inline__ __attribute__((always_inline)) unsigned long long __rdtsc(void)
{
    unsigned long lo32;
    __asm__("mfdec" : "=b" (lo32));
    return lo32;
}


/*** Interrupts ***/
/* Finally decided to do this by enabling single step trap */
static __inline__ __attribute__((always_inline)) void __debugbreak(void)
{
    
}

static __inline__ __attribute__((always_inline)) void __int2c(void)
{
    /* Not sure yet */
}

#ifndef _ENABLE_DISABLE_DEFINED
#define _ENABLE_DISABLE_DEFINED
static __inline__ __attribute__((always_inline)) void _disable(void)
{
    __asm__ __volatile__("mfmsr 0\n\t" \
			 "li    8,0x7fff\n\t" \
			 "and   0,8,0\n\t" \
			 "mtmsr 0\n\t");
}

static __inline__ __attribute__((always_inline)) void _enable(void)
{
 __asm__ __volatile__("mfmsr 0\n\t" \
                      "lis    8,0x8000@ha\n\t" \
                      "or    0,8,0\n\t" \
                      "mtmsr 0\n\t");
}
#endif

/*** Protected memory management ***/
static __inline__ __attribute__((always_inline)) unsigned long __readsdr1(void)
{
    unsigned long value;
    __asm__("mfsdr1 %0" : "=b" (value));
    return value;
}

static __inline__ __attribute__((always_inline)) void __writesdr1(const unsigned long long Data)
{
    __asm__("mtsdr1 %0" : : "b" (Data));
}

/*** System operations ***/
/* This likely has a different meaning from the X86 equivalent.  We'll keep
 * the name cause it fits */
static __inline__ __attribute__((always_inline)) unsigned long long __readmsr()
{
    unsigned long temp;
    __asm__("mfmsr %0" : "=b" (temp));
    return temp;
}

static __inline__ __attribute__((always_inline)) void __writemsr(const unsigned long Value)
{
    __asm__("mtmsr %0" : : "b" (Value));
}

/* We'll make sure of the following:
 * IO operations have completed
 * Write operations through cache have completed
 * We've reloaded anything in the data or instruction cache that might have
 * changed in real ram.
 */
static __inline__ __attribute__((always_inline)) void __wbinvd(void)
{
    __asm__("eieio\n\t"
	    "dcs\n\t"
	    "sync\n\t"
	    "isync\n\t");
}
#else
/* TODO: the x64 architecture shares most of the intrinsics. It should be easy to support */
#error Unsupported architecture
#endif


/*** Miscellaneous ***/
/* BUGBUG: only good for use in macros. Cannot be taken the address of */
#define __noop(...) ((void)0)

/* TODO: __assume. GCC only supports the weaker __builtin_expect */

#endif

/* EOF */
