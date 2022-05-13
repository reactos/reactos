/* byte_order.c - byte order related platform dependent routines,
 * from librhash0 Debian package sources
 *
 * Copyright (c) 2008, Aleksey Kravchenko <rhash.admin@gmail.com>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
 * REGARD TO THIS SOFTWARE  INCLUDING ALL IMPLIED WARRANTIES OF  MERCHANTABILITY
 * AND FITNESS.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
 * INDIRECT,  OR CONSEQUENTIAL DAMAGES  OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE,  DATA OR PROFITS,  WHETHER IN AN ACTION OF CONTRACT,  NEGLIGENCE
 * OR OTHER TORTIOUS ACTION,  ARISING OUT OF  OR IN CONNECTION  WITH THE USE  OR
 * PERFORMANCE OF THIS SOFTWARE.
 */
#include "byte_order.h"

#ifndef rhash_ctz

#  if _MSC_VER >= 1300 && (_M_IX86 || _M_AMD64 || _M_IA64) /* if MSVC++ >= 2002 on x86/x64 */
#  include <intrin.h>
#  pragma intrinsic(_BitScanForward)

/**
 * Returns index of the trailing bit of x.
 *
 * @param x the number to process
 * @return zero-based index of the trailing bit
 */
unsigned rhash_ctz(unsigned x)
{
	unsigned long index;
	unsigned char isNonzero = _BitScanForward(&index, x); /* MSVC intrinsic */
	return (isNonzero ? (unsigned)index : 0);
}
#  else /* _MSC_VER >= 1300... */

/**
 * Returns index of the trailing bit of a 32-bit number.
 * This is a plain C equivalent for GCC __builtin_ctz() bit scan.
 *
 * @param x the number to process
 * @return zero-based index of the trailing bit
 */
unsigned rhash_ctz(unsigned x)
{
	/* array for conversion to bit position */
	static unsigned char bit_pos[32] =  {
		0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
		31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
	};

	/* The De Bruijn bit-scan was devised in 1997, according to Donald Knuth
	 * by Martin Lauter. The constant 0x077CB531UL is a De Bruijn sequence,
	 * which produces a unique pattern of bits into the high 5 bits for each
	 * possible bit position that it is multiplied against.
	 * See http://graphics.stanford.edu/~seander/bithacks.html
	 * and http://chessprogramming.wikispaces.com/BitScan */
	return (unsigned)bit_pos[((uint32_t)((x & -x) * 0x077CB531U)) >> 27];
}
#  endif /* _MSC_VER >= 1300... */
#endif /* rhash_ctz */

/**
 * Copy a memory block with simultaneous exchanging byte order.
 * The byte order is changed from little-endian 32-bit integers
 * to big-endian (or vice-versa).
 *
 * @param to the pointer where to copy memory block
 * @param index the index to start writing from
 * @param from  the source block to copy
 * @param length length of the memory block
 */
void rhash_swap_copy_str_to_u32(void* to, int index, const void* from, size_t length)
{
	/* if all pointers and length are 32-bits aligned */
	if ( 0 == (( (int)((char*)to - (char*)0) | ((char*)from - (char*)0) | index | length ) & 3) ) {
		/* copy memory as 32-bit words */
		const uint32_t* src = (const uint32_t*)from;
		const uint32_t* end = (const uint32_t*)((const char*)src + length);
		uint32_t* dst = (uint32_t*)((char*)to + index);
		for (; src < end; dst++, src++)
			*dst = bswap_32(*src);
	} else {
		const char* src = (const char*)from;
		for (length += index; (size_t)index < length; index++)
			((char*)to)[index ^ 3] = *(src++);
	}
}

/**
 * Copy a memory block with changed byte order.
 * The byte order is changed from little-endian 64-bit integers
 * to big-endian (or vice-versa).
 *
 * @param to     the pointer where to copy memory block
 * @param index  the index to start writing from
 * @param from   the source block to copy
 * @param length length of the memory block
 */
void rhash_swap_copy_str_to_u64(void* to, int index, const void* from, size_t length)
{
	/* if all pointers and length are 64-bits aligned */
	if ( 0 == (( (int)((char*)to - (char*)0) | ((char*)from - (char*)0) | index | length ) & 7) ) {
		/* copy aligned memory block as 64-bit integers */
		const uint64_t* src = (const uint64_t*)from;
		const uint64_t* end = (const uint64_t*)((const char*)src + length);
		uint64_t* dst = (uint64_t*)((char*)to + index);
		while (src < end) *(dst++) = bswap_64( *(src++) );
	} else {
		const char* src = (const char*)from;
		for (length += index; (size_t)index < length; index++) ((char*)to)[index ^ 7] = *(src++);
	}
}

/**
 * Copy data from a sequence of 64-bit words to a binary string of given length,
 * while changing byte order.
 *
 * @param to     the binary string to receive data
 * @param from   the source sequence of 64-bit words
 * @param length the size in bytes of the data being copied
 */
void rhash_swap_copy_u64_to_str(void* to, const void* from, size_t length)
{
	/* if all pointers and length are 64-bits aligned */
	if ( 0 == (( (int)((char*)to - (char*)0) | ((char*)from - (char*)0) | length ) & 7) ) {
		/* copy aligned memory block as 64-bit integers */
		const uint64_t* src = (const uint64_t*)from;
		const uint64_t* end = (const uint64_t*)((const char*)src + length);
		uint64_t* dst = (uint64_t*)to;
		while (src < end) *(dst++) = bswap_64( *(src++) );
	} else {
		size_t index;
		char* dst = (char*)to;
		for (index = 0; index < length; index++) *(dst++) = ((char*)from)[index ^ 7];
	}
}

/**
 * Exchange byte order in the given array of 32-bit integers.
 *
 * @param arr    the array to process
 * @param length array length
 */
void rhash_u32_mem_swap(unsigned* arr, int length)
{
	unsigned* end = arr + length;
	for (; arr < end; arr++) {
		*arr = bswap_32(*arr);
	}
}

#ifdef HAS_INTEL_CPUID
#include <cpuid.h>

static uint64_t get_cpuid_features(void)
{
	uint32_t tmp, edx, ecx;
	if (__get_cpuid(1, &tmp, &tmp, &ecx, &edx))
		return ((((uint64_t)ecx) << 32) ^ edx);
	return 0;
}

int has_cpu_feature(unsigned feature_bit)
{
	static uint64_t features;
	const uint64_t feature = ((uint64_t)1) << feature_bit;
	if (!features)
		features = (get_cpuid_features() | 1);
	return !!(features & feature);
}
#endif
