#ifndef _LINUX_BITOPS_H
#define _LINUX_BITOPS_H

#include <ntifs.h>
#include <linux/types.h>

#ifdef	__KERNEL__
#define BIT(nr)			    (1 << (nr))
#define BIT_MASK(nr)		(1 << ((nr) % BITS_PER_LONG))
#define BIT_WORD(nr)		((nr) / BITS_PER_LONG)
#define BITS_TO_LONGS(nr)	DIV_ROUND_UP(nr, BITS_PER_LONG)
#define BITS_PER_BYTE		8
#endif

/*
 * Include this here because some architectures need generic_ffs/fls in
 * scope
 */

/**
 * find_first_zero_bit - find the first zero bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum size to search
 *
 * Returns the bit number of the first zero bit, not the number of the byte
 * containing a bit.
 */
#define find_first_zero_bit(addr, size) find_next_zero_bit((addr), (size), 0)

/**
 * find_next_zero_bit - find the first zero bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bit number to start searching at
 * @size: The maximum size to search
 */
int find_next_zero_bit(const unsigned long *addr, int size, int offset);

/**
 * __ffs - find first bit in word.
 * @word: The word to search
 *
 * Undefined if no bit exists, so code should check against 0 first.
 */
static inline unsigned long __ffs(unsigned long word)
{
    int num = 0;

#if BITS_PER_LONG == 64
    if ((word & 0xffffffff) == 0) {
        num += 32;
        word >>= 32;
    }
#endif
    if ((word & 0xffff) == 0) {
        num += 16;
        word >>= 16;
    }
    if ((word & 0xff) == 0) {
        num += 8;
        word >>= 8;
    }
    if ((word & 0xf) == 0) {
        num += 4;
        word >>= 4;
    }
    if ((word & 0x3) == 0) {
        num += 2;
        word >>= 2;
    }
    if ((word & 0x1) == 0)
        num += 1;
    return num;
}

/**
 * find_first_bit - find the first set bit in a memory region
 * @addr: The address to start the search at
 * @size: The maximum size to search
 *
 * Returns the bit number of the first set bit, not the number of the byte
 * containing a bit.
 */
static inline unsigned find_first_bit(const unsigned long *addr, unsigned size)
{
    unsigned x = 0;

    while (x < size) {
        unsigned long val = *addr++;
        if (val)
            return __ffs(val) + x;
        x += (sizeof(*addr)<<3);
    }
    return x;
}

/**
 * find_next_bit - find the next set bit in a memory region
 * @addr: The address to base the search on
 * @offset: The bitnumber to start searching at
 * @size: The maximum size to search
 */

/*
 * ffz - find first zero in word.
 * @word: The word to search
 *
 * Undefined if no zero exists, so code should check against ~0UL first.
 */
#define ffz(x)  __ffs(~(x))


/**
 * ffs - find first bit set
 * @x: the word to search
 *
 * This is defined the same way as
 * the libc and compiler builtin ffs routines, therefore
 * differs in spirit from the above ffz (man ffs).
 */
static inline int ffs(int x)
{
    int r = 1;

    if (!x)
        return 0;
    if (!(x & 0xffff)) {
        x >>= 16;
        r += 16;
    }
    if (!(x & 0xff)) {
        x >>= 8;
        r += 8;
    }
    if (!(x & 0xf)) {
        x >>= 4;
        r += 4;
    }
    if (!(x & 3)) {
        x >>= 2;
        r += 2;
    }
    if (!(x & 1)) {
        x >>= 1;
        r += 1;
    }
    return r;
}

/**
 * fls - find last (most-significant) bit set
 * @x: the word to search
 *
 * This is defined the same way as ffs.
 * Note fls(0) = 0, fls(1) = 1, fls(0x80000000) = 32.
 */

static inline int fls(int x)
{
    int r = 32;

    if (!x)
        return 0;
    if (!(x & 0xffff0000u)) {
        x <<= 16;
        r -= 16;
    }
    if (!(x & 0xff000000u)) {
        x <<= 8;
        r -= 8;
    }
    if (!(x & 0xf0000000u)) {
        x <<= 4;
        r -= 4;
    }
    if (!(x & 0xc0000000u)) {
        x <<= 2;
        r -= 2;
    }
    if (!(x & 0x80000000u)) {
        x <<= 1;
        r -= 1;
    }
    return r;
}

static inline int fls64(__u64 x)
{
    __u32 h = (__u32) (x >> 32);
    if (h)
        return fls(h) + 32;
    return fls((int)x);
}

#define for_each_bit(bit, addr, size) \
	for ((bit) = find_first_bit((addr), (size)); \
	     (bit) < (size); \
	     (bit) = find_next_bit((addr), (size), (bit) + 1))


static __inline int get_bitmask_order(unsigned int count)
{
    int order;

    order = fls(count);
    return order;	/* We could be slightly more clever with -1 here... */
}

static __inline int get_count_order(unsigned int count)
{
    int order;

    order = fls(count) - 1;
    if (count & (count - 1))
        order++;
    return order;
}


/**
 * rol32 - rotate a 32-bit value left
 * @word: value to rotate
 * @shift: bits to roll
 */
static inline __u32 rol32(__u32 word, unsigned int shift)
{
    return (word << shift) | (word >> (32 - shift));
}

/**
 * ror32 - rotate a 32-bit value right
 * @word: value to rotate
 * @shift: bits to roll
 */
static inline __u32 ror32(__u32 word, unsigned int shift)
{
    return (word >> shift) | (word << (32 - shift));
}

static inline unsigned fls_long(unsigned long l)
{
    if (sizeof(l) == 4)
        return fls(l);
    return fls64(l);
}

/*
 * hweightN: returns the hamming weight (i.e. the number
 * of bits set) of a N-bit word
 */

static inline unsigned long hweight32(unsigned long w)
{
    unsigned int res = (w & 0x55555555) + ((w >> 1) & 0x55555555);
    res = (res & 0x33333333) + ((res >> 2) & 0x33333333);
    res = (res & 0x0F0F0F0F) + ((res >> 4) & 0x0F0F0F0F);
    res = (res & 0x00FF00FF) + ((res >> 8) & 0x00FF00FF);
    return (res & 0x0000FFFF) + ((res >> 16) & 0x0000FFFF);
}

static inline unsigned long hweight64(__u64 w)
{
#if BITS_PER_LONG < 64
    return hweight32((unsigned int)(w >> 32)) + hweight32((unsigned int)w);
#else
    u64 res;
    res = (w & 0x5555555555555555U) + ((w >> 1) & 0x5555555555555555U);
    res = (res & 0x3333333333333333U) + ((res >> 2) & 0x3333333333333333U);
    res = (res & 0x0F0F0F0F0F0F0F0FU) + ((res >> 4) & 0x0F0F0F0F0F0F0F0FU);
    res = (res & 0x00FF00FF00FF00FFU) + ((res >> 8) & 0x00FF00FF00FF00FFU);
    res = (res & 0x0000FFFF0000FFFFU) + ((res >> 16) & 0x0000FFFF0000FFFFU);
    return (res & 0x00000000FFFFFFFFU) + ((res >> 32) & 0x00000000FFFFFFFFU);
#endif
}

static inline unsigned long hweight_long(unsigned long w)
{
    return sizeof(w) == 4 ? hweight32(w) : hweight64(w);
}

#endif
