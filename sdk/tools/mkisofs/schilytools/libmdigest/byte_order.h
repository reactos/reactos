/* @(#)byte_order.h	1.3 15/11/22 2015 J. Schilling */
/* byte_order.h */
/*
 * SHA3 hash code taken from
 * https://github.com/rhash/RHash/tree/master/librhash
 *
 * Portions Copyright (c) 2015 J. Schilling
 */
#ifndef BYTE_ORDER_H
#define	BYTE_ORDER_H
#include <schily/stdlib.h>
#include <schily/types.h>
#include <schily/stdint.h>

#ifdef IN_RHASH
#include "config.h"
#endif

#ifdef __GLIBC__
# include <endian.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#ifdef	HAVE_C_BIGENDIAN

/*
 * Use the Schily autoconf results.
 */
#ifdef	WORDS_BIGENDIAN
#define	CPU_BIG_ENDIAN
#else
#define	CPU_LITTLE_ENDIAN
#endif

#else	/* HAVE_C_BIGENDIAN */

/* if x86 compatible cpu */
#if defined(i386) || defined(__i386__) || defined(__i486__) || \
	defined(__i586__) || defined(__i686__) || defined(__pentium__) || \
	defined(__pentiumpro__) || defined(__pentium4__) || \
	defined(__nocona__) || defined(prescott) || defined(__core2__) || \
	defined(__k6__) || defined(__k8__) || defined(__athlon__) || \
	defined(__amd64) || defined(__amd64__) || \
	defined(__x86_64) || defined(__x86_64__) || defined(_M_IX86) || \
	defined(_M_AMD64) || defined(_M_IA64) || defined(_M_X64)
/* detect if x86-64 instruction set is supported */
# if defined(_LP64) || defined(__LP64__) || defined(__x86_64) || \
	defined(__x86_64__) || defined(_M_AMD64) || defined(_M_X64)
#define	CPU_X64
#else
#define	CPU_IA32
#endif
#endif


/* detect CPU endianness */
#if (defined(__BYTE_ORDER) && defined(__LITTLE_ENDIAN) && \
		__BYTE_ORDER == __LITTLE_ENDIAN) || \
	defined(CPU_IA32) || defined(CPU_X64) || \
	defined(__ia64) || defined(__ia64__) || defined(__alpha__) || defined(_M_ALPHA) || \
	defined(vax) || defined(MIPSEL) || defined(_ARM_)
#define	CPU_LITTLE_ENDIAN
#define	IS_BIG_ENDIAN 0
#define	IS_LITTLE_ENDIAN 1
#elif (defined(__BYTE_ORDER) && defined(__BIG_ENDIAN) && \
		__BYTE_ORDER == __BIG_ENDIAN) || \
	defined(__sparc) || defined(__sparc__) || defined(sparc) || \
	defined(_ARCH_PPC) || defined(_ARCH_PPC64) || defined(_POWER) || \
	defined(__POWERPC__) || defined(POWERPC) || defined(__powerpc) || \
	defined(__powerpc__) || defined(__powerpc64__) || defined(__ppc__) || \
	defined(__hpux) || defined(_MIPSEB) || defined(mc68000) || \
	defined(__s390__) || defined(__s390x__) || defined(sel)
#define	CPU_BIG_ENDIAN
#define	IS_BIG_ENDIAN 1
#define	IS_LITTLE_ENDIAN 0
#else
	error "Can't detect CPU architechture"
#endif

#endif	/* HAVE_C_BIGENDIAN */

#define	IS_ALIGNED_32(p) (0 == (3 & ((const char *)(p) - (const char *)0)))
#define	IS_ALIGNED_64(p) (0 == (7 & ((const char *)(p) - (const char *)0)))

#if defined(_MSC_VER)
#define	ALIGN_ATTR(n) __declspec(align(n))
#elif defined(__GNUC__)
#define	ALIGN_ATTR(n) __attribute__((aligned(n)))
#else
#define	ALIGN_ATTR(n) /* nothing */
#endif


#ifdef	PROTOTYPES
#if defined(_MSC_VER) || defined(__BORLANDC__)
#define	I64(x) x##ui64
#define	UI64(x) x##ui64
#else
#define	I64(x) x##LL
#define	UI64(x) x##ULL
#endif
#else	/* !PROTOTYPES */
#ifdef	__hpux
#define	I64(x) x/**/LL
#define	UI64(x) x/**/ULL
#else
#define	I64(x) ((long long)(x))
#define	UI64(x) ((unsigned long long)(x))
#endif
#endif	/* !PROTOTYPES */

/* convert a hash flag to index */
#if __GNUC__ >= 4 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4) /* GCC < 3.4 */
#define	rhash_ctz(x) __builtin_ctz(x)
#else
unsigned rhash_ctz __PR((unsigned)); /* define as function */
#endif

void rhash_swap_copy_str_to_u32	__PR((void* to, int idx, const void* from, size_t length));
void rhash_swap_copy_str_to_u64 __PR((void* to, int idx, const void* from, size_t length));
void rhash_swap_copy_u64_to_str __PR((void* to, const void* from, size_t length));
void rhash_u32_mem_swap __PR((unsigned *p, int length_in_u32));

/* define bswap_32 */
#if defined(__GNUC__) && defined(CPU_IA32) && !defined(__i386__)
/* for intel x86 CPU */
static inline UInt32_t bswap_32(UInt32_t x) {
	__asm("bswap\t%0" : "=r" (x) : "0" (x));
	return (x);
}
#elif defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC__ > 4 || __GNUC_MINOR__ >= 3)
/* for GCC >= 4.3 */
# define bswap_32(x) __builtin_bswap32(x)
#elif (_MSC_VER > 1300) && (defined(CPU_IA32) || defined(CPU_X64)) /* MS VC */
# define bswap_32(x) _byteswap_ulong((unsigned long)x)
#elif !defined(__STRICT_ANSI__)
/* general bswap_32 definition */
static inline UInt32_t bswap_32 __PR((UInt32_t x));
static inline UInt32_t bswap_32(x)
	UInt32_t	x;
{
	x = ((x << 8) & 0xFF00FF00) | ((x >> 8) & 0x00FF00FF);
	return ((x >> 16) | (x << 16));
}
#else
#define	bswap_32(x) ((((x) & 0xff000000) >> 24) | (((x) & 0x00ff0000) >>  8) | \
	(((x) & 0x0000ff00) <<  8) | (((x) & 0x000000ff) << 24))
#endif /* bswap_32 */

#if defined(__GNUC__) && (__GNUC__ >= 4) && (__GNUC__ > 4 || __GNUC_MINOR__ >= 3)
# define bswap_64(x) __builtin_bswap64(x)
#elif (_MSC_VER > 1300) && (defined(CPU_IA32) || defined(CPU_X64)) /* MS VC */
# define bswap_64(x) _byteswap_uint64((__int64)x)
#elif !defined(__STRICT_ANSI__)
static inline UInt64_t bswap_64 __PR((UInt64_t x));
static inline UInt64_t bswap_64(x)
	UInt64_t	x;
{
	union {
		UInt64_t ll;
		UInt32_t l[2];
	} w, r;
	w.ll = x;
	r.l[0] = bswap_32(w.l[1]);
	r.l[1] = bswap_32(w.l[0]);
	return (r.ll);
}
#else
	error "bswap_64 unsupported"
#endif

#ifdef CPU_BIG_ENDIAN
# define be2me_32(x) (x)
# define be2me_64(x) (x)
# define le2me_32(x) bswap_32(x)
# define le2me_64(x) bswap_64(x)

# define be32_copy(to, index, from, length) memcpy((to) + (index), (from), (length))
# define le32_copy(to, index, from, length) rhash_swap_copy_str_to_u32((to), (index), (from), (length))
# define be64_copy(to, index, from, length) memcpy((to) + (index), (from), (length))
# define le64_copy(to, index, from, length) rhash_swap_copy_str_to_u64((to), (index), (from), (length))
# define me64_to_be_str(to, from, length) memcpy((to), (from), (length))
# define me64_to_le_str(to, from, length) rhash_swap_copy_u64_to_str((to), (from), (length))

#else /* CPU_BIG_ENDIAN */
# define be2me_32(x) bswap_32(x)
# define be2me_64(x) bswap_64(x)
# define le2me_32(x) (x)
# define le2me_64(x) (x)

# define be32_copy(to, index, from, length) rhash_swap_copy_str_to_u32((to), (index), (from), (length))
# define le32_copy(to, index, from, length) memcpy((to) + (index), (from), (length))
# define be64_copy(to, index, from, length) rhash_swap_copy_str_to_u64((to), (index), (from), (length))
# define le64_copy(to, index, from, length) memcpy((to) + (index), (from), (length))
# define me64_to_be_str(to, from, length) rhash_swap_copy_u64_to_str((to), (from), (length))
# define me64_to_le_str(to, from, length) memcpy((to), (from), (length))
#endif /* CPU_BIG_ENDIAN */

/* ROTL/ROTR macros rotate a 32/64-bit word left/right by n bits */
#define	ROTL32(dword, n) ((dword) << (n) ^ ((dword) >> (32 - (n))))
#define	ROTR32(dword, n) ((dword) >> (n) ^ ((dword) << (32 - (n))))
#define	ROTL64(qword, n) ((qword) << (n) ^ ((qword) >> (64 - (n))))
#define	ROTR64(qword, n) ((qword) >> (n) ^ ((qword) << (64 - (n))))

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* BYTE_ORDER_H */
