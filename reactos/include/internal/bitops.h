#ifndef _I386_BITOPS_H
#define _I386_BITOPS_H

/*
 * Copyright 1992, Linus Torvalds.
 */
/*
 * Reused for the ReactOS kernel by David Welch (1998)
 */

/*
 * These have to be done with inline assembly: that way the bit-setting
 * is guaranteed to be atomic. All bit operations return 0 if the bit
 * was cleared before the operation and != 0 if it was not.
 *
 * bit 0 is the LSB of addr; bit 32 is the LSB of (addr+1).
 */

#ifdef __SMP__
#define LOCK_PREFIX "lock ; "
#define SMPVOL volatile
#else
#define LOCK_PREFIX ""
#define SMPVOL
#endif

/*
 * Some hacks to defeat gcc over-optimizations..
 */
struct __dummy { unsigned long a[100]; };
#define ADDR (*(struct __dummy *) addr)
#define CONST_ADDR (*(const struct __dummy *) addr)

extern __inline__ int set_bit(int nr, SMPVOL void * addr)
{
	int oldbit;

	__asm__ __volatile__(LOCK_PREFIX
		"btsl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (ADDR)
		:"ir" (nr));
	return oldbit;
}

extern __inline__ int clear_bit(int nr, SMPVOL void * addr)
{
	int oldbit;

	__asm__ __volatile__(LOCK_PREFIX
		"btrl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (ADDR)
		:"ir" (nr));
	return oldbit;
}

extern __inline__ int change_bit(int nr, SMPVOL void * addr)
{
	int oldbit;

	__asm__ __volatile__(LOCK_PREFIX
		"btcl %2,%1\n\tsbbl %0,%0"
		:"=r" (oldbit),"=m" (ADDR)
		:"ir" (nr));
	return oldbit;
}

/*
 * This routine doesn't need to be atomic.
 */
extern __inline__ int test_bit(int nr, const SMPVOL void * addr)
{
	return ((1UL << (nr & 31)) & (((const unsigned int *) addr)[nr >> 5])) != 0;
}

/*
 * Find-bit routines..
 */
extern __inline__ int find_first_zero_bit(void * addr, unsigned size)
{
	int res;

	if (!size)
		return 0;
	__asm__("cld\n\t"
		"movl $-1,%%eax\n\t"
		"xorl %%edx,%%edx\n\t"
		"repe; scasl\n\t"
		"je 1f\n\t"
		"xorl -4(%%edi),%%eax\n\t"
		"subl $4,%%edi\n\t"
		"bsfl %%eax,%%edx\n"
		"1:\tsubl %%ebx,%%edi\n\t"
		"shll $3,%%edi\n\t"
		"addl %%edi,%%edx"
		:"=d" (res)
		:"c" ((size + 31) >> 5), "D" (addr), "b" (addr)
		:"ax", "cx", "di");
	return res;
}

extern __inline__ int find_next_zero_bit (void * addr, int size, int offset)
{
	unsigned long * p = ((unsigned long *) addr) + (offset >> 5);
	int set = 0, bit = offset & 31, res;
	
	if (bit) {
		/*
		 * Look for zero in first byte
		 */
		__asm__("bsfl %1,%0\n\t"
			"jne 1f\n\t"
			"movl $32, %0\n"
			"1:"
			: "=r" (set)
			: "r" (~(*p >> bit)));
		if (set < (32 - bit))
			return set + offset;
		set = 32 - bit;
		p++;
	}
	/*
	 * No zero yet, search remaining full bytes for a zero
	 */
	res = find_first_zero_bit (p, size - 32 * (p - (unsigned long *) addr));
	return (offset + set + res);
}

/*
 * ffz = Find First Zero in word. Undefined if no zero exists,
 * so code should check against ~0UL first..
 */
extern __inline__ unsigned long ffz(unsigned long word)
{
	__asm__("bsfl %1,%0"
		:"=r" (word)
		:"r" (~word));
	return word;
}

#endif /* _I386_BITOPS_H */
