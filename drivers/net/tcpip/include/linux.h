#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#include <ddk/ntddk.h>

#ifndef NULL
#define NULL (void*)0
#endif

typedef struct page {
  int x;
} mem_map_t;













/* i386 */

typedef unsigned short umode_t;

/*
 * __xx is ok: it doesn't pollute the POSIX namespace. Use these in the
 * header files exported to user space
 */

typedef __signed__ char __s8;
typedef unsigned char __u8;

typedef __signed__ short __s16;
typedef unsigned short __u16;

typedef __signed__ int __s32;
typedef unsigned int __u32;

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
typedef __signed__ long long __s64;
typedef unsigned long long __u64;
#endif

/*
 * These aren't exported outside the kernel to avoid name space clashes
 */
typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

#define BITS_PER_LONG 32

/* DMA addresses come in generic and 64-bit flavours.  */

#ifdef CONFIG_HIGHMEM64G
typedef u64 dma_addr_t;
#else
typedef u32 dma_addr_t;
#endif
typedef u64 dma64_addr_t;



/*
 * This allows for 1024 file descriptors: if NR_OPEN is ever grown
 * beyond that you'll have to change this too. But 1024 fd's seem to be
 * enough even for such "real" unices like OSF/1, so hopefully this is
 * one limit that doesn't have to be changed [again].
 *
 * Note that POSIX wants the FD_CLEAR(fd,fdsetp) defines to be in
 * <sys/time.h> (and thus <linux/time.h>) - but this is a more logical
 * place for them. Solved by having dummy defines in <sys/time.h>.
 */

/*
 * Those macros may have been defined in <gnu/types.h>. But we always
 * use the ones here. 
 */
#undef __NFDBITS
#define __NFDBITS	(8 * sizeof(unsigned long))

#undef __FD_SETSIZE
#define __FD_SETSIZE	1024

#undef __FDSET_LONGS
#define __FDSET_LONGS	(__FD_SETSIZE/__NFDBITS)

#undef __FDELT
#define	__FDELT(d)	((d) / __NFDBITS)

#undef __FDMASK
#define	__FDMASK(d)	(1UL << ((d) % __NFDBITS))

typedef struct {
	unsigned long fds_bits [__FDSET_LONGS];
} __kernel_fd_set;

/* Type of a signal handler.  */
typedef void (*__kernel_sighandler_t)(int);

/* Type of a SYSV IPC key.  */
typedef int __kernel_key_t;


/*
 * This file is generally used by user-level software, so you need to
 * be a little careful about namespace pollution etc.  Also, we cannot
 * assume GCC is being used.
 */

typedef unsigned short	__kernel_dev_t;
typedef unsigned long	__kernel_ino_t;
typedef unsigned short	__kernel_mode_t;
typedef unsigned short	__kernel_nlink_t;
typedef long		__kernel_off_t;
typedef int		__kernel_pid_t;
typedef unsigned short	__kernel_ipc_pid_t;
typedef unsigned short	__kernel_uid_t;
typedef unsigned short	__kernel_gid_t;
typedef unsigned int	__kernel_size_t;
typedef int		__kernel_ssize_t;
typedef int		__kernel_ptrdiff_t;
typedef long		__kernel_time_t;
typedef long		__kernel_suseconds_t;
typedef long		__kernel_clock_t;
typedef int		__kernel_daddr_t;
typedef char *		__kernel_caddr_t;
typedef unsigned short	__kernel_uid16_t;
typedef unsigned short	__kernel_gid16_t;
typedef unsigned int	__kernel_uid32_t;
typedef unsigned int	__kernel_gid32_t;

typedef unsigned short	__kernel_old_uid_t;
typedef unsigned short	__kernel_old_gid_t;

#ifdef __GNUC__
typedef long long	__kernel_loff_t;
#endif

typedef struct {
#if defined(__KERNEL__) || defined(__USE_ALL)
	int	val[2];
#else /* !defined(__KERNEL__) && !defined(__USE_ALL) */
	int	__val[2];
#endif /* !defined(__KERNEL__) && !defined(__USE_ALL) */
} __kernel_fsid_t;

#if defined(__KERNEL__) || !defined(__GLIBC__) || (__GLIBC__ < 2)

#undef	__FD_SET
#define __FD_SET(fd,fdsetp) \
		__asm__ __volatile__("btsl %1,%0": \
			"=m" (*(__kernel_fd_set *) (fdsetp)):"r" ((int) (fd)))

#undef	__FD_CLR
#define __FD_CLR(fd,fdsetp) \
		__asm__ __volatile__("btrl %1,%0": \
			"=m" (*(__kernel_fd_set *) (fdsetp)):"r" ((int) (fd)))

#undef	__FD_ISSET
#define __FD_ISSET(fd,fdsetp) (__extension__ ({ \
		unsigned char __result; \
		__asm__ __volatile__("btl %1,%2 ; setb %0" \
			:"=q" (__result) :"r" ((int) (fd)), \
			"m" (*(__kernel_fd_set *) (fdsetp))); \
		__result; }))

#undef	__FD_ZERO
#define __FD_ZERO(fdsetp) \
do { \
	int __d0, __d1; \
	__asm__ __volatile__("cld ; rep ; stosl" \
			:"=m" (*(__kernel_fd_set *) (fdsetp)), \
			  "=&c" (__d0), "=&D" (__d1) \
			:"a" (0), "1" (__FDSET_LONGS), \
			"2" ((__kernel_fd_set *) (fdsetp)) : "memory"); \
} while (0)

#endif /* defined(__KERNEL__) || !defined(__GLIBC__) || (__GLIBC__ < 2) */


#ifndef __KERNEL_STRICT_NAMES

typedef __kernel_fd_set		fd_set;
typedef __kernel_dev_t		dev_t;
typedef __kernel_ino_t		ino_t;
typedef __kernel_mode_t		mode_t;
typedef __kernel_nlink_t	nlink_t;
typedef __kernel_off_t		off_t;
typedef __kernel_pid_t		pid_t;
typedef __kernel_daddr_t	daddr_t;
typedef __kernel_key_t		key_t;
typedef __kernel_suseconds_t	suseconds_t;

#ifdef __KERNEL__
typedef __kernel_uid32_t	uid_t;
typedef __kernel_gid32_t	gid_t;
typedef __kernel_uid16_t        uid16_t;
typedef __kernel_gid16_t        gid16_t;

#ifdef CONFIG_UID16
/* This is defined by include/asm-{arch}/posix_types.h */
typedef __kernel_old_uid_t	old_uid_t;
typedef __kernel_old_gid_t	old_gid_t;
#endif /* CONFIG_UID16 */

/* libc5 includes this file to define uid_t, thus uid_t can never change
 * when it is included by non-kernel code
 */
#else
typedef __kernel_uid_t		uid_t;
typedef __kernel_gid_t		gid_t;
#endif /* __KERNEL__ */

#if defined(__GNUC__)
typedef __kernel_loff_t		loff_t;
#endif

/*
 * The following typedefs are also protected by individual ifdefs for
 * historical reasons:
 */
#ifndef _SIZE_T
#define _SIZE_T
typedef __kernel_size_t		size_t;
#endif

#ifndef _SSIZE_T
#define _SSIZE_T
typedef __kernel_ssize_t	ssize_t;
#endif

#ifndef _PTRDIFF_T
#define _PTRDIFF_T
typedef __kernel_ptrdiff_t	ptrdiff_t;
#endif

#ifndef _TIME_T
#define _TIME_T
typedef __kernel_time_t		time_t;
#endif

#ifndef _CLOCK_T
#define _CLOCK_T
typedef __kernel_clock_t	clock_t;
#endif

#ifndef _CADDR_T
#define _CADDR_T
typedef __kernel_caddr_t	caddr_t;
#endif

/* bsd */
typedef unsigned char		u_char;
typedef unsigned short		u_short;
typedef unsigned int		u_int;
typedef unsigned long		u_long;

/* sysv */
typedef unsigned char		unchar;
typedef unsigned short		ushort;
typedef unsigned int		uint;
typedef unsigned long		ulong;

#ifndef __BIT_TYPES_DEFINED__
#define __BIT_TYPES_DEFINED__

typedef		__u8		u_int8_t;
typedef		__s8		int8_t;
typedef		__u16		u_int16_t;
typedef		__s16		int16_t;
typedef		__u32		u_int32_t;
typedef		__s32		int32_t;

#endif /* !(__BIT_TYPES_DEFINED__) */

typedef		__u8		uint8_t;
typedef		__u16		uint16_t;
typedef		__u32		uint32_t;

#if defined(__GNUC__) && !defined(__STRICT_ANSI__)
typedef		__u64		uint64_t;
typedef		__u64		u_int64_t;
typedef		__s64		int64_t;
#endif

#endif /* __KERNEL_STRICT_NAMES */

/*
 * Below are truly Linux-specific types that should never collide with
 * any application/library that wants linux/types.h.
 */

struct ustat {
	__kernel_daddr_t	f_tfree;
	__kernel_ino_t		f_tinode;
	char			f_fname[6];
	char			f_fpack[6];
};














#ifndef __LITTLE_ENDIAN
#define __LITTLE_ENDIAN 1234
#endif
#ifndef __LITTLE_ENDIAN_BITFIELD
#define __LITTLE_ENDIAN_BITFIELD
#endif

#if 1 /* swab */

/*
 * linux/byteorder/swab.h
 * Byte-swapping, independently from CPU endianness
 *	swabXX[ps]?(foo)
 *
 * Francois-Rene Rideau <fare@tunes.org> 19971205
 *    separated swab functions from cpu_to_XX,
 *    to clean up support for bizarre-endian architectures.
 *
 * See asm-i386/byteorder.h and suches for examples of how to provide
 * architecture-dependent optimized versions
 *
 */

/* casts are necessary for constants, because we never know how for sure
 * how U/UL/ULL map to __u16, __u32, __u64. At least not in a portable way.
 */
#define ___swab16(x) \
({ \
	__u16 __x = (x); \
	((__u16)( \
		(((__u16)(__x) & (__u16)0x00ffU) << 8) | \
		(((__u16)(__x) & (__u16)0xff00U) >> 8) )); \
})

#define ___swab24(x) \
({ \
	__u32 __x = (x); \
	((__u32)( \
		((__x & (__u32)0x000000ffUL) << 16) | \
		 (__x & (__u32)0x0000ff00UL)        | \
		((__x & (__u32)0x00ff0000UL) >> 16) )); \
})

#define ___swab32(x) \
({ \
	__u32 __x = (x); \
	((__u32)( \
		(((__u32)(__x) & (__u32)0x000000ffUL) << 24) | \
		(((__u32)(__x) & (__u32)0x0000ff00UL) <<  8) | \
		(((__u32)(__x) & (__u32)0x00ff0000UL) >>  8) | \
		(((__u32)(__x) & (__u32)0xff000000UL) >> 24) )); \
})

#define ___swab64(x) \
({ \
	__u64 __x = (x); \
	((__u64)( \
		(__u64)(((__u64)(__x) & (__u64)0x00000000000000ffULL) << 56) | \
		(__u64)(((__u64)(__x) & (__u64)0x000000000000ff00ULL) << 40) | \
		(__u64)(((__u64)(__x) & (__u64)0x0000000000ff0000ULL) << 24) | \
		(__u64)(((__u64)(__x) & (__u64)0x00000000ff000000ULL) <<  8) | \
	        (__u64)(((__u64)(__x) & (__u64)0x000000ff00000000ULL) >>  8) | \
		(__u64)(((__u64)(__x) & (__u64)0x0000ff0000000000ULL) >> 24) | \
		(__u64)(((__u64)(__x) & (__u64)0x00ff000000000000ULL) >> 40) | \
		(__u64)(((__u64)(__x) & (__u64)0xff00000000000000ULL) >> 56) )); \
})

#define ___constant_swab16(x) \
	((__u16)( \
		(((__u16)(x) & (__u16)0x00ffU) << 8) | \
		(((__u16)(x) & (__u16)0xff00U) >> 8) ))
#define ___constant_swab24(x) \
	((__u32)( \
		(((__u32)(x) & (__u32)0x000000ffU) << 16) | \
		(((__u32)(x) & (__u32)0x0000ff00U)	  | \
		(((__u32)(x) & (__u32)0x00ff0000U) >> 16) ))
#define ___constant_swab32(x) \
	((__u32)( \
		(((__u32)(x) & (__u32)0x000000ffUL) << 24) | \
		(((__u32)(x) & (__u32)0x0000ff00UL) <<  8) | \
		(((__u32)(x) & (__u32)0x00ff0000UL) >>  8) | \
		(((__u32)(x) & (__u32)0xff000000UL) >> 24) ))
#define ___constant_swab64(x) \
	((__u64)( \
		(__u64)(((__u64)(x) & (__u64)0x00000000000000ffULL) << 56) | \
		(__u64)(((__u64)(x) & (__u64)0x000000000000ff00ULL) << 40) | \
		(__u64)(((__u64)(x) & (__u64)0x0000000000ff0000ULL) << 24) | \
		(__u64)(((__u64)(x) & (__u64)0x00000000ff000000ULL) <<  8) | \
	        (__u64)(((__u64)(x) & (__u64)0x000000ff00000000ULL) >>  8) | \
		(__u64)(((__u64)(x) & (__u64)0x0000ff0000000000ULL) >> 24) | \
		(__u64)(((__u64)(x) & (__u64)0x00ff000000000000ULL) >> 40) | \
		(__u64)(((__u64)(x) & (__u64)0xff00000000000000ULL) >> 56) ))

/*
 * provide defaults when no architecture-specific optimization is detected
 */
#ifndef __arch__swab16
#  define __arch__swab16(x) ({ __u16 __tmp = (x) ; ___swab16(__tmp); })
#endif
#ifndef __arch__swab24
#  define __arch__swab24(x) ({ __u32 __tmp = (x) ; ___swab24(__tmp); })
#endif
#ifndef __arch__swab32
#  define __arch__swab32(x) ({ __u32 __tmp = (x) ; ___swab32(__tmp); })
#endif
#ifndef __arch__swab64
#  define __arch__swab64(x) ({ __u64 __tmp = (x) ; ___swab64(__tmp); })
#endif

#ifndef __arch__swab16p
#  define __arch__swab16p(x) __arch__swab16(*(x))
#endif
#ifndef __arch__swab24p
#  define __arch__swab24p(x) __arch__swab24(*(x))
#endif
#ifndef __arch__swab32p
#  define __arch__swab32p(x) __arch__swab32(*(x))
#endif
#ifndef __arch__swab64p
#  define __arch__swab64p(x) __arch__swab64(*(x))
#endif

#ifndef __arch__swab16s
#  define __arch__swab16s(x) do { *(x) = __arch__swab16p((x)); } while (0)
#endif
#ifndef __arch__swab24s
#  define __arch__swab24s(x) do { *(x) = __arch__swab24p((x)); } while (0)
#endif
#ifndef __arch__swab32s
#  define __arch__swab32s(x) do { *(x) = __arch__swab32p((x)); } while (0)
#endif
#ifndef __arch__swab64s
#  define __arch__swab64s(x) do { *(x) = __arch__swab64p((x)); } while (0)
#endif


/*
 * Allow constant folding
 */
#if defined(__GNUC__) && (__GNUC__ >= 2) && defined(__OPTIMIZE__)
#  define __swab16(x) \
(__builtin_constant_p((__u16)(x)) ? \
 ___swab16((x)) : \
 __fswab16((x)))
#  define __swab24(x) \
(__builtin_constant_p((__u32)(x)) ? \
 ___swab24((x)) : \
 __fswab24((x)))
#  define __swab32(x) \
(__builtin_constant_p((__u32)(x)) ? \
 ___swab32((x)) : \
 __fswab32((x)))
#  define __swab64(x) \
(__builtin_constant_p((__u64)(x)) ? \
 ___swab64((x)) : \
 __fswab64((x)))
#else
#  define __swab16(x) __fswab16(x)
#  define __swab24(x) __fswab24(x)
#  define __swab32(x) __fswab32(x)
#  define __swab64(x) __fswab64(x)
#endif /* OPTIMIZE */


static __inline__ __const__ __u16 __fswab16(__u16 x)
{
	return __arch__swab16(x);
}
static __inline__ __u16 __swab16p(__u16 *x)
{
	return __arch__swab16p(x);
}
static __inline__ void __swab16s(__u16 *addr)
{
	__arch__swab16s(addr);
}

static __inline__ __const__ __u32 __fswab24(__u32 x)
{
	return __arch__swab24(x);
}
static __inline__ __u32 __swab24p(__u32 *x)
{
	return __arch__swab24p(x);
}
static __inline__ void __swab24s(__u32 *addr)
{
	__arch__swab24s(addr);
}

static __inline__ __const__ __u32 __fswab32(__u32 x)
{
	return __arch__swab32(x);
}
static __inline__ __u32 __swab32p(__u32 *x)
{
	return __arch__swab32p(x);
}
static __inline__ void __swab32s(__u32 *addr)
{
	__arch__swab32s(addr);
}

#ifdef __BYTEORDER_HAS_U64__
static __inline__ __const__ __u64 __fswab64(__u64 x)
{
#  ifdef __SWAB_64_THRU_32__
	__u32 h = x >> 32;
        __u32 l = x & ((1ULL<<32)-1);
        return (((__u64)__swab32(l)) << 32) | ((__u64)(__swab32(h)));
#  else
	return __arch__swab64(x);
#  endif
}
static __inline__ __u64 __swab64p(__u64 *x)
{
	return __arch__swab64p(x);
}
static __inline__ void __swab64s(__u64 *addr)
{
	__arch__swab64s(addr);
}
#endif /* __BYTEORDER_HAS_U64__ */

#if defined(__KERNEL__)
#define swab16 __swab16
#define swab24 __swab24
#define swab32 __swab32
#define swab64 __swab64
#define swab16p __swab16p
#define swab24p __swab24p
#define swab32p __swab32p
#define swab64p __swab64p
#define swab16s __swab16s
#define swab24s __swab24s
#define swab32s __swab32s
#define swab64s __swab64s
#endif

#endif /* swab */



#if 1 /* generic */

/*
 * linux/byteorder_generic.h
 * Generic Byte-reordering support
 *
 * Francois-Rene Rideau <fare@tunes.org> 19970707
 *    gathered all the good ideas from all asm-foo/byteorder.h into one file,
 *    cleaned them up.
 *    I hope it is compliant with non-GCC compilers.
 *    I decided to put __BYTEORDER_HAS_U64__ in byteorder.h,
 *    because I wasn't sure it would be ok to put it in types.h
 *    Upgraded it to 2.1.43
 * Francois-Rene Rideau <fare@tunes.org> 19971012
 *    Upgraded it to 2.1.57
 *    to please Linus T., replaced huge #ifdef's between little/big endian
 *    by nestedly #include'd files.
 * Francois-Rene Rideau <fare@tunes.org> 19971205
 *    Made it to 2.1.71; now a facelift:
 *    Put files under include/linux/byteorder/
 *    Split swab from generic support.
 *
 * TODO:
 *   = Regular kernel maintainers could also replace all these manual
 *    byteswap macros that remain, disseminated among drivers,
 *    after some grep or the sources...
 *   = Linus might want to rename all these macros and files to fit his taste,
 *    to fit his personal naming scheme.
 *   = it seems that a few drivers would also appreciate
 *    nybble swapping support...
 *   = every architecture could add their byteswap macro in asm/byteorder.h
 *    see how some architectures already do (i386, alpha, ppc, etc)
 *   = cpu_to_beXX and beXX_to_cpu might some day need to be well
 *    distinguished throughout the kernel. This is not the case currently,
 *    since little endian, big endian, and pdp endian machines needn't it.
 *    But this might be the case for, say, a port of Linux to 20/21 bit
 *    architectures (and F21 Linux addict around?).
 */

/*
 * The following macros are to be defined by <asm/byteorder.h>:
 *
 * Conversion of long and short int between network and host format
 *	ntohl(__u32 x)
 *	ntohs(__u16 x)
 *	htonl(__u32 x)
 *	htons(__u16 x)
 * It seems that some programs (which? where? or perhaps a standard? POSIX?)
 * might like the above to be functions, not macros (why?).
 * if that's true, then detect them, and take measures.
 * Anyway, the measure is: define only ___ntohl as a macro instead,
 * and in a separate file, have
 * unsigned long inline ntohl(x){return ___ntohl(x);}
 *
 * The same for constant arguments
 *	__constant_ntohl(__u32 x)
 *	__constant_ntohs(__u16 x)
 *	__constant_htonl(__u32 x)
 *	__constant_htons(__u16 x)
 *
 * Conversion of XX-bit integers (16- 32- or 64-)
 * between native CPU format and little/big endian format
 * 64-bit stuff only defined for proper architectures
 *	cpu_to_[bl]eXX(__uXX x)
 *	[bl]eXX_to_cpu(__uXX x)
 *
 * The same, but takes a pointer to the value to convert
 *	cpu_to_[bl]eXXp(__uXX x)
 *	[bl]eXX_to_cpup(__uXX x)
 *
 * The same, but change in situ
 *	cpu_to_[bl]eXXs(__uXX x)
 *	[bl]eXX_to_cpus(__uXX x)
 *
 * See asm-foo/byteorder.h for examples of how to provide
 * architecture-optimized versions
 *
 */


#if defined(__KERNEL__)
/*
 * inside the kernel, we can use nicknames;
 * outside of it, we must avoid POSIX namespace pollution...
 */
#define cpu_to_le64 __cpu_to_le64
#define le64_to_cpu __le64_to_cpu
#define cpu_to_le32 __cpu_to_le32
#define le32_to_cpu __le32_to_cpu
#define cpu_to_le16 __cpu_to_le16
#define le16_to_cpu __le16_to_cpu
#define cpu_to_be64 __cpu_to_be64
#define be64_to_cpu __be64_to_cpu
#define cpu_to_be32 __cpu_to_be32
#define be32_to_cpu __be32_to_cpu
#define cpu_to_be16 __cpu_to_be16
#define be16_to_cpu __be16_to_cpu
#define cpu_to_le64p __cpu_to_le64p
#define le64_to_cpup __le64_to_cpup
#define cpu_to_le32p __cpu_to_le32p
#define le32_to_cpup __le32_to_cpup
#define cpu_to_le16p __cpu_to_le16p
#define le16_to_cpup __le16_to_cpup
#define cpu_to_be64p __cpu_to_be64p
#define be64_to_cpup __be64_to_cpup
#define cpu_to_be32p __cpu_to_be32p
#define be32_to_cpup __be32_to_cpup
#define cpu_to_be16p __cpu_to_be16p
#define be16_to_cpup __be16_to_cpup
#define cpu_to_le64s __cpu_to_le64s
#define le64_to_cpus __le64_to_cpus
#define cpu_to_le32s __cpu_to_le32s
#define le32_to_cpus __le32_to_cpus
#define cpu_to_le16s __cpu_to_le16s
#define le16_to_cpus __le16_to_cpus
#define cpu_to_be64s __cpu_to_be64s
#define be64_to_cpus __be64_to_cpus
#define cpu_to_be32s __cpu_to_be32s
#define be32_to_cpus __be32_to_cpus
#define cpu_to_be16s __cpu_to_be16s
#define be16_to_cpus __be16_to_cpus
#endif


/*
 * Handle ntohl and suches. These have various compatibility
 * issues - like we want to give the prototype even though we
 * also have a macro for them in case some strange program
 * wants to take the address of the thing or something..
 *
 * Note that these used to return a "long" in libc5, even though
 * long is often 64-bit these days.. Thus the casts.
 *
 * They have to be macros in order to do the constant folding
 * correctly - if the argument passed into a inline function
 * it is no longer constant according to gcc..
 */

#undef ntohl
#undef ntohs
#undef htonl
#undef htons

/*
 * Do the prototypes. Somebody might want to take the
 * address or some such sick thing..
 */
#if defined(__KERNEL__) || (defined (__GLIBC__) && __GLIBC__ >= 2)
extern __u32			ntohl(__u32);
extern __u32			htonl(__u32);
#else
extern unsigned long int	ntohl(unsigned long int);
extern unsigned long int	htonl(unsigned long int);
#endif
extern unsigned short int	ntohs(unsigned short int);
extern unsigned short int	htons(unsigned short int);


#if defined(__GNUC__) && (__GNUC__ >= 2) && defined(__OPTIMIZE__) && !defined(__STRICT_ANSI__)

#define ___htonl(x) __cpu_to_be32(x)
#define ___htons(x) __cpu_to_be16(x)
#define ___ntohl(x) __be32_to_cpu(x)
#define ___ntohs(x) __be16_to_cpu(x)

#if defined(__KERNEL__) || (defined (__GLIBC__) && __GLIBC__ >= 2)
#define htonl(x) ___htonl(x)
#define ntohl(x) ___ntohl(x)
#else
#define htonl(x) ((unsigned long)___htonl(x))
#define ntohl(x) ((unsigned long)___ntohl(x))
#endif
#define htons(x) ___htons(x)
#define ntohs(x) ___ntohs(x)

#endif /* OPTIMIZE */

#endif /* generic */


#define __constant_htonl(x) ___constant_swab32((x))
#define __constant_ntohl(x) ___constant_swab32((x))
#define __constant_htons(x) ___constant_swab16((x))
#define __constant_ntohs(x) ___constant_swab16((x))
#define __constant_cpu_to_le64(x) ((__u64)(x))
#define __constant_le64_to_cpu(x) ((__u64)(x))
#define __constant_cpu_to_le32(x) ((__u32)(x))
#define __constant_le32_to_cpu(x) ((__u32)(x))
#define __constant_cpu_to_le24(x) ((__u32)(x))
#define __constant_le24_to_cpu(x) ((__u32)(x))
#define __constant_cpu_to_le16(x) ((__u16)(x))
#define __constant_le16_to_cpu(x) ((__u16)(x))
#define __constant_cpu_to_be64(x) ___constant_swab64((x))
#define __constant_be64_to_cpu(x) ___constant_swab64((x))
#define __constant_cpu_to_be32(x) ___constant_swab32((x))
#define __constant_be32_to_cpu(x) ___constant_swab32((x))
#define __constant_cpu_to_be24(x) ___constant_swab24((x))
#define __constant_be24_to_cpu(x) ___constant_swab24((x))
#define __constant_cpu_to_be16(x) ___constant_swab16((x))
#define __constant_be16_to_cpu(x) ___constant_swab16((x))
#define __cpu_to_le64(x) ((__u64)(x))
#define __le64_to_cpu(x) ((__u64)(x))
#define __cpu_to_le32(x) ((__u32)(x))
#define __le32_to_cpu(x) ((__u32)(x))
#define __cpu_to_le24(x) ((__u32)(x))
#define __le24_to_cpu(x) ((__u32)(x))
#define __cpu_to_le16(x) ((__u16)(x))
#define __le16_to_cpu(x) ((__u16)(x))
#define __cpu_to_be64(x) __swab64((x))
#define __be64_to_cpu(x) __swab64((x))
#define __cpu_to_be32(x) __swab32((x))
#define __be32_to_cpu(x) __swab32((x))
#define __cpu_to_be24(x) __swab24((x))
#define __be24_to_cpu(x) __swab24((x))
#define __cpu_to_be16(x) __swab16((x))
#define __be16_to_cpu(x) __swab16((x))
#define __cpu_to_le64p(x) (*(__u64*)(x))
#define __le64_to_cpup(x) (*(__u64*)(x))
#define __cpu_to_le32p(x) (*(__u32*)(x))
#define __le32_to_cpup(x) (*(__u32*)(x))
#define __cpu_to_le24p(x) (*(__u32*)(x))
#define __le24_to_cpup(x) (*(__u32*)(x))
#define __cpu_to_le16p(x) (*(__u16*)(x))
#define __le16_to_cpup(x) (*(__u16*)(x))
#define __cpu_to_be64p(x) __swab64p((x))
#define __be64_to_cpup(x) __swab64p((x))
#define __cpu_to_be32p(x) __swab32p((x))
#define __be32_to_cpup(x) __swab32p((x))
#define __cpu_to_be24p(x) __swab24p((x))
#define __be24_to_cpup(x) __swab24p((x))
#define __cpu_to_be16p(x) __swab16p((x))
#define __be16_to_cpup(x) __swab16p((x))
#define __cpu_to_le64s(x) do {} while (0)
#define __le64_to_cpus(x) do {} while (0)
#define __cpu_to_le32s(x) do {} while (0)
#define __le32_to_cpus(x) do {} while (0)
#define __cpu_to_le24s(x) do {} while (0)
#define __le24_to_cpus(x) do {} while (0)
#define __cpu_to_le16s(x) do {} while (0)
#define __le16_to_cpus(x) do {} while (0)
#define __cpu_to_be64s(x) __swab64s((x))
#define __be64_to_cpus(x) __swab64s((x))
#define __cpu_to_be32s(x) __swab32s((x))
#define __be32_to_cpus(x) __swab32s((x))
#define __cpu_to_be24s(x) __swab24s((x))
#define __be24_to_cpus(x) __swab24s((x))
#define __cpu_to_be16s(x) __swab16s((x))
#define __be16_to_cpus(x) __swab16s((x))








#if 1

/* Dummy types */

#define ____cacheline_aligned

typedef struct 
{
  volatile unsigned int lock;
} rwlock_t;

typedef struct {
	volatile unsigned int lock;
} spinlock_t;

struct task_struct;





#if 1 /* atomic */

/*
 * Atomic operations that C can't guarantee us.  Useful for
 * resource counting etc..
 */

#ifdef CONFIG_SMP
#define LOCK "lock ; "
#else
#define LOCK ""
#endif

/*
 * Make sure gcc doesn't try to be clever and move things around
 * on us. We need to use _exactly_ the address the user gave us,
 * not some alias that contains the same information.
 */
typedef struct { volatile int counter; } atomic_t;

#define ATOMIC_INIT(i)	{ (i) }

/**
 * atomic_read - read atomic variable
 * @v: pointer of type atomic_t
 * 
 * Atomically reads the value of @v.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */ 
#define atomic_read(v)		((v)->counter)

/**
 * atomic_set - set atomic variable
 * @v: pointer of type atomic_t
 * @i: required value
 * 
 * Atomically sets the value of @v to @i.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */ 
#define atomic_set(v,i)		(((v)->counter) = (i))

/**
 * atomic_add - add integer to atomic variable
 * @i: integer value to add
 * @v: pointer of type atomic_t
 * 
 * Atomically adds @i to @v.  Note that the guaranteed useful range
 * of an atomic_t is only 24 bits.
 */
static __inline__ void atomic_add(int i, atomic_t *v)
{
#if 0
	__asm__ __volatile__(
		LOCK "addl %1,%0"
		:"=m" (v->counter)
		:"ir" (i), "m" (v->counter));
#endif
}

/**
 * atomic_sub - subtract the atomic variable
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 * 
 * Atomically subtracts @i from @v.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static __inline__ void atomic_sub(int i, atomic_t *v)
{
#if 0
	__asm__ __volatile__(
		LOCK "subl %1,%0"
		:"=m" (v->counter)
		:"ir" (i), "m" (v->counter));
#endif
}

/**
 * atomic_sub_and_test - subtract value from variable and test result
 * @i: integer value to subtract
 * @v: pointer of type atomic_t
 * 
 * Atomically subtracts @i from @v and returns
 * true if the result is zero, or false for all
 * other cases.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */
static __inline__ int atomic_sub_and_test(int i, atomic_t *v)
{
#if 0
	unsigned char c;

	__asm__ __volatile__(
		LOCK "subl %2,%0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"ir" (i), "m" (v->counter) : "memory");
	return c;
#endif
}

/**
 * atomic_inc - increment atomic variable
 * @v: pointer of type atomic_t
 * 
 * Atomically increments @v by 1.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */ 
static __inline__ void atomic_inc(atomic_t *v)
{
#if 0
	__asm__ __volatile__(
		LOCK "incl %0"
		:"=m" (v->counter)
		:"m" (v->counter));
#endif
}

/**
 * atomic_dec - decrement atomic variable
 * @v: pointer of type atomic_t
 * 
 * Atomically decrements @v by 1.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */ 
static __inline__ void atomic_dec(atomic_t *v)
{
#if 0
	__asm__ __volatile__(
		LOCK "decl %0"
		:"=m" (v->counter)
		:"m" (v->counter));
#endif
}

/**
 * atomic_dec_and_test - decrement and test
 * @v: pointer of type atomic_t
 * 
 * Atomically decrements @v by 1 and
 * returns true if the result is 0, or false for all other
 * cases.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */ 
static __inline__ int atomic_dec_and_test(atomic_t *v)
{
#if 0
	unsigned char c;

	__asm__ __volatile__(
		LOCK "decl %0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
	return c != 0;
#else
  return 1;
#endif
}

/**
 * atomic_inc_and_test - increment and test 
 * @v: pointer of type atomic_t
 * 
 * Atomically increments @v by 1
 * and returns true if the result is zero, or false for all
 * other cases.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */ 
static __inline__ int atomic_inc_and_test(atomic_t *v)
{
#if 0
	unsigned char c;

	__asm__ __volatile__(
		LOCK "incl %0; sete %1"
		:"=m" (v->counter), "=qm" (c)
		:"m" (v->counter) : "memory");
	return c != 0;
#else
  return 1;
#endif
}

/**
 * atomic_add_negative - add and test if negative
 * @v: pointer of type atomic_t
 * @i: integer value to add
 * 
 * Atomically adds @i to @v and returns true
 * if the result is negative, or false when
 * result is greater than or equal to zero.  Note that the guaranteed
 * useful range of an atomic_t is only 24 bits.
 */ 
static __inline__ int atomic_add_negative(int i, atomic_t *v)
{
#if 0
	unsigned char c;

	__asm__ __volatile__(
		LOCK "addl %2,%0; sets %1"
		:"=m" (v->counter), "=qm" (c)
		:"ir" (i), "m" (v->counter) : "memory");
	return c;
#else
  return 0;
#endif
}

/* These are x86-specific, used by some header files */
#define atomic_clear_mask(mask, addr)
#if 0
__asm__ __volatile__(LOCK "andl %0,%1" \
: : "r" (~(mask)),"m" (*addr) : "memory")
#endif

#define atomic_set_mask(mask, addr)
#if 0
__asm__ __volatile__(LOCK "orl %0,%1" \
: : "r" (mask),"m" (*addr) : "memory")
#endif

/* Atomic operations are already serializing on x86 */
#define smp_mb__before_atomic_dec()
#define smp_mb__after_atomic_dec()
#define smp_mb__before_atomic_inc()
#define smp_mb__after_atomic_inc()



#endif /* atomic */





#if 1 /* list */

struct list_head {
	struct list_head *next, *prev;
};

#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

#define INIT_LIST_HEAD(ptr) do { \
	(ptr)->next = (ptr); (ptr)->prev = (ptr); \
} while (0)

/*
 * Insert a new entry between two known consecutive entries. 
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *new,
			      struct list_head *prev,
			      struct list_head *next)
{
#if 0
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
#endif
}

/**
 * list_add - add a new entry
 * @new: new entry to be added
 * @head: list head to add it after
 *
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new, struct list_head *head)
{
#if 0
	__list_add(new, head, head->next);
#endif
}

/**
 * list_add_tail - add a new entry
 * @new: new entry to be added
 * @head: list head to add it before
 *
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
#if 0
	__list_add(new, head->prev, head);
#endif
}

/*
 * Delete a list entry by making the prev/next entries
 * point to each other.
 *
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * list_del - deletes entry from list.
 * @entry: the element to delete from the list.
 * Note: list_empty on entry does not return true after this, the entry is in an undefined state.
 */
static inline void list_del(struct list_head *entry)
{
#if 0
	__list_del(entry->prev, entry->next);
	entry->next = (void *) 0;
	entry->prev = (void *) 0;
#endif
}

/**
 * list_del_init - deletes entry from list and reinitialize it.
 * @entry: the element to delete from the list.
 */
static inline void list_del_init(struct list_head *entry)
{
#if 0
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry); 
#endif
}

/**
 * list_move - delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void list_move(struct list_head *list, struct list_head *head)
{
#if 0
        __list_del(list->prev, list->next);
        list_add(list, head);
#endif
}

/**
 * list_move_tail - delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void list_move_tail(struct list_head *list,
				  struct list_head *head)
{
#if 0
        __list_del(list->prev, list->next);
        list_add_tail(list, head);
#endif
}

/**
 * list_empty - tests whether a list is empty
 * @head: the list to test.
 */
static inline int list_empty(struct list_head *head)
{
	return head->next == head;
}

static inline void __list_splice(struct list_head *list,
				 struct list_head *head)
{
#if 0
	struct list_head *first = list->next;
	struct list_head *last = list->prev;
	struct list_head *at = head->next;

	first->prev = head;
	head->next = first;

	last->next = at;
	at->prev = last;
#endif
}

/**
 * list_splice - join two lists
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 */
static inline void list_splice(struct list_head *list, struct list_head *head)
{
#if 0
	if (!list_empty(list))
		__list_splice(list, head);
#endif
}

/**
 * list_splice_init - join two lists and reinitialise the emptied list.
 * @list: the new list to add.
 * @head: the place to add it in the first list.
 *
 * The list at @list is reinitialised
 */
static inline void list_splice_init(struct list_head *list,
				    struct list_head *head)
{
#if 0
	if (!list_empty(list)) {
		__list_splice(list, head);
		INIT_LIST_HEAD(list);
	}
#endif
}

/**
 * list_entry - get the struct for this entry
 * @ptr:	the &struct list_head pointer.
 * @type:	the type of the struct this is embedded in.
 * @member:	the name of the list_struct within the struct.
 */
#define list_entry(ptr, type, member)
#if 0
	((type *)((char *)(ptr)-(unsigned long)(&((type *)0)->member)))
#endif

/**
 * list_for_each	-	iterate over a list
 * @pos:	the &struct list_head to use as a loop counter.
 * @head:	the head for your list.
 */
#define list_for_each(pos, head)
#if 0
	for (pos = (head)->next, prefetch(pos->next); pos != (head); \
        	pos = pos->next, prefetch(pos->next))
#endif

/**
 * list_for_each_prev	-	iterate over a list backwards
 * @pos:	the &struct list_head to use as a loop counter.
 * @head:	the head for your list.
 */
#define list_for_each_prev(pos, head)
#if 0
	for (pos = (head)->prev, prefetch(pos->prev); pos != (head); \
        	pos = pos->prev, prefetch(pos->prev))
#endif   	

/**
 * list_for_each_safe	-	iterate over a list safe against removal of list entry
 * @pos:	the &struct list_head to use as a loop counter.
 * @n:		another &struct list_head to use as temporary storage
 * @head:	the head for your list.
 */
#define list_for_each_safe(pos, n, head)
#if 0
	for (pos = (head)->next, n = pos->next; pos != (head); \
		pos = n, n = pos->next)
#endif

/**
 * list_for_each_entry	-	iterate over list of given type
 * @pos:	the type * to use as a loop counter.
 * @head:	the head for your list.
 * @member:	the name of the list_struct within the struct.
 */
#define list_for_each_entry(pos, head, member)
#if 0
	for (pos = list_entry((head)->next, typeof(*pos), member),	\
		     prefetch(pos->member.next);			\
	     &pos->member != (head); 					\
	     pos = list_entry(pos->member.next, typeof(*pos), member),	\
		     prefetch(pos->member.next))
#endif

#endif /* list */





#if 1 /* wait */

#define WNOHANG		0x00000001
#define WUNTRACED	0x00000002

#define __WNOTHREAD	0x20000000	/* Don't wait on children of other threads in this group */
#define __WALL		0x40000000	/* Wait on all children, regardless of type */
#define __WCLONE	0x80000000	/* Wait only on non-SIGCHLD children */

#if 0
#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/stddef.h>
#include <linux/spinlock.h>
#include <linux/config.h>

#include <asm/page.h>
#include <asm/processor.h>
#endif

/*
 * Debug control.  Slow but useful.
 */
#if defined(CONFIG_DEBUG_WAITQ)
#define WAITQUEUE_DEBUG 1
#else
#define WAITQUEUE_DEBUG 0
#endif

struct __wait_queue {
	unsigned int flags;
#define WQ_FLAG_EXCLUSIVE	0x01
	struct task_struct * task;
	struct list_head task_list;
#if WAITQUEUE_DEBUG
	long __magic;
	long __waker;
#endif
};
typedef struct __wait_queue wait_queue_t;

/*
 * 'dual' spinlock architecture. Can be switched between spinlock_t and
 * rwlock_t locks via changing this define. Since waitqueues are quite
 * decoupled in the new architecture, lightweight 'simple' spinlocks give
 * us slightly better latencies and smaller waitqueue structure size.
 */
#define USE_RW_WAIT_QUEUE_SPINLOCK 0

#if USE_RW_WAIT_QUEUE_SPINLOCK
# define wq_lock_t rwlock_t
# define WAITQUEUE_RW_LOCK_UNLOCKED RW_LOCK_UNLOCKED

# define wq_read_lock read_lock
# define wq_read_lock_irqsave read_lock_irqsave
# define wq_read_unlock_irqrestore read_unlock_irqrestore
# define wq_read_unlock read_unlock
# define wq_write_lock_irq write_lock_irq
# define wq_write_lock_irqsave write_lock_irqsave
# define wq_write_unlock_irqrestore write_unlock_irqrestore
# define wq_write_unlock write_unlock
#else
# define wq_lock_t spinlock_t
# define WAITQUEUE_RW_LOCK_UNLOCKED SPIN_LOCK_UNLOCKED

# define wq_read_lock spin_lock
# define wq_read_lock_irqsave spin_lock_irqsave
# define wq_read_unlock spin_unlock
# define wq_read_unlock_irqrestore spin_unlock_irqrestore
# define wq_write_lock_irq spin_lock_irq
# define wq_write_lock_irqsave spin_lock_irqsave
# define wq_write_unlock_irqrestore spin_unlock_irqrestore
# define wq_write_unlock spin_unlock
#endif

struct __wait_queue_head {
	wq_lock_t lock;
	struct list_head task_list;
#if WAITQUEUE_DEBUG
	long __magic;
	long __creator;
#endif
};
typedef struct __wait_queue_head wait_queue_head_t;


/*
 * Debugging macros.  We eschew `do { } while (0)' because gcc can generate
 * spurious .aligns.
 */
#if WAITQUEUE_DEBUG
#define WQ_BUG()	BUG()
#define CHECK_MAGIC(x)
#if 0
	do {									\
		if ((x) != (long)&(x)) {					\
			printk("bad magic %lx (should be %lx), ",		\
				(long)x, (long)&(x));				\
			WQ_BUG();						\
		}								\
	} while (0)
#endif

#define CHECK_MAGIC_WQHEAD(x)
#if 0
	do {									\
		if ((x)->__magic != (long)&((x)->__magic)) {			\
			printk("bad magic %lx (should be %lx, creator %lx), ",	\
			(x)->__magic, (long)&((x)->__magic), (x)->__creator);	\
			WQ_BUG();						\
		}								\
	} while (0)
#endif

#define WQ_CHECK_LIST_HEAD(list)
#if 0
	do {									\
		if (!(list)->next || !(list)->prev)				\
			WQ_BUG();						\
	} while(0)
#endif

#define WQ_NOTE_WAKER(tsk)
#if 0
	do {									\
		(tsk)->__waker = (long)__builtin_return_address(0);		\
	} while (0)
#endif
#else
#define WQ_BUG()
#define CHECK_MAGIC(x)
#define CHECK_MAGIC_WQHEAD(x)
#define WQ_CHECK_LIST_HEAD(list)
#define WQ_NOTE_WAKER(tsk)
#endif

/*
 * Macros for declaration and initialisaton of the datatypes
 */

#if WAITQUEUE_DEBUG
# define __WAITQUEUE_DEBUG_INIT(name) //(long)&(name).__magic, 0
# define __WAITQUEUE_HEAD_DEBUG_INIT(name) //(long)&(name).__magic, (long)&(name).__magic
#else
# define __WAITQUEUE_DEBUG_INIT(name)
# define __WAITQUEUE_HEAD_DEBUG_INIT(name)
#endif

#define __WAITQUEUE_INITIALIZER(name, tsk)
#if 0
{
	task:		tsk,						\
	task_list:	{ NULL, NULL },					\
			 __WAITQUEUE_DEBUG_INIT(name)}
#endif

#define DECLARE_WAITQUEUE(name, tsk)
#if 0
	wait_queue_t name = __WAITQUEUE_INITIALIZER(name, tsk)
#endif

#define __WAIT_QUEUE_HEAD_INITIALIZER(name)
#if 0
{
	lock:		WAITQUEUE_RW_LOCK_UNLOCKED,			\
	task_list:	{ &(name).task_list, &(name).task_list },	\
			__WAITQUEUE_HEAD_DEBUG_INIT(name)}
#endif

#define DECLARE_WAIT_QUEUE_HEAD(name)
#if 0
	wait_queue_head_t name = __WAIT_QUEUE_HEAD_INITIALIZER(name)
#endif

static inline void init_waitqueue_head(wait_queue_head_t *q)
{
#if 0
#if WAITQUEUE_DEBUG
	if (!q)
		WQ_BUG();
#endif
	q->lock = WAITQUEUE_RW_LOCK_UNLOCKED;
	INIT_LIST_HEAD(&q->task_list);
#if WAITQUEUE_DEBUG
	q->__magic = (long)&q->__magic;
	q->__creator = (long)current_text_addr();
#endif
#endif
}

static inline void init_waitqueue_entry(wait_queue_t *q, struct task_struct *p)
{
#if 0
#if WAITQUEUE_DEBUG
	if (!q || !p)
		WQ_BUG();
#endif
	q->flags = 0;
	q->task = p;
#if WAITQUEUE_DEBUG
	q->__magic = (long)&q->__magic;
#endif
#endif
}

static inline int waitqueue_active(wait_queue_head_t *q)
{
#if 0
#if WAITQUEUE_DEBUG
	if (!q)
		WQ_BUG();
	CHECK_MAGIC_WQHEAD(q);
#endif

	return !list_empty(&q->task_list);
#endif
}

static inline void __add_wait_queue(wait_queue_head_t *head, wait_queue_t *new)
{
#if 0
#if WAITQUEUE_DEBUG
	if (!head || !new)
		WQ_BUG();
	CHECK_MAGIC_WQHEAD(head);
	CHECK_MAGIC(new->__magic);
	if (!head->task_list.next || !head->task_list.prev)
		WQ_BUG();
#endif
	list_add(&new->task_list, &head->task_list);
#endif
}

/*
 * Used for wake-one threads:
 */
static inline void __add_wait_queue_tail(wait_queue_head_t *head,
						wait_queue_t *new)
{
#if 0
#if WAITQUEUE_DEBUG
	if (!head || !new)
		WQ_BUG();
	CHECK_MAGIC_WQHEAD(head);
	CHECK_MAGIC(new->__magic);
	if (!head->task_list.next || !head->task_list.prev)
		WQ_BUG();
#endif
	list_add_tail(&new->task_list, &head->task_list);
#endif
}

static inline void __remove_wait_queue(wait_queue_head_t *head,
							wait_queue_t *old)
{
#if 0
#if WAITQUEUE_DEBUG
	if (!old)
		WQ_BUG();
	CHECK_MAGIC(old->__magic);
#endif
	list_del(&old->task_list);
#endif
}




#endif /* wait */


#endif




#if 1 /* slab */

typedef struct
{
 int x;
} kmem_cache_s;

typedef struct kmem_cache_s kmem_cache_t;

#if 0
#include	<linux/mm.h>
#include	<linux/cache.h>
#endif

/* flags for kmem_cache_alloc() */
#define	SLAB_NOFS		GFP_NOFS
#define	SLAB_NOIO		GFP_NOIO
#define SLAB_NOHIGHIO		GFP_NOHIGHIO
#define	SLAB_ATOMIC		GFP_ATOMIC
#define	SLAB_USER		GFP_USER
#define	SLAB_KERNEL		GFP_KERNEL
#define	SLAB_NFS		GFP_NFS
#define	SLAB_DMA		GFP_DMA

#define SLAB_LEVEL_MASK		(__GFP_WAIT|__GFP_HIGH|__GFP_IO|__GFP_HIGHIO|__GFP_FS)
#define	SLAB_NO_GROW		0x00001000UL	/* don't grow a cache */

/* flags to pass to kmem_cache_create().
 * The first 3 are only valid when the allocator as been build
 * SLAB_DEBUG_SUPPORT.
 */
#define	SLAB_DEBUG_FREE		0x00000100UL	/* Peform (expensive) checks on free */
#define	SLAB_DEBUG_INITIAL	0x00000200UL	/* Call constructor (as verifier) */
#define	SLAB_RED_ZONE		0x00000400UL	/* Red zone objs in a cache */
#define	SLAB_POISON		0x00000800UL	/* Poison objects */
#define	SLAB_NO_REAP		0x00001000UL	/* never reap from the cache */
#define	SLAB_HWCACHE_ALIGN	0x00002000UL	/* align objs on a h/w cache lines */
#define SLAB_CACHE_DMA		0x00004000UL	/* use GFP_DMA memory */
#define SLAB_MUST_HWCACHE_ALIGN	0x00008000UL	/* force alignment */

/* flags passed to a constructor func */
#define	SLAB_CTOR_CONSTRUCTOR	0x001UL		/* if not set, then deconstructor */
#define SLAB_CTOR_ATOMIC	0x002UL		/* tell constructor it can't sleep */
#define	SLAB_CTOR_VERIFY	0x004UL		/* tell constructor it's a verify call */

/* prototypes */
extern void kmem_cache_init(void);
extern void kmem_cache_sizes_init(void);

extern kmem_cache_t *kmem_find_general_cachep(size_t, int gfpflags);
extern kmem_cache_t *kmem_cache_create(const char *, size_t, size_t, unsigned long,
				       void (*)(void *, kmem_cache_t *, unsigned long),
				       void (*)(void *, kmem_cache_t *, unsigned long));
extern int kmem_cache_destroy(kmem_cache_t *);
extern int kmem_cache_shrink(kmem_cache_t *);
extern void *kmem_cache_alloc(kmem_cache_t *, int);
extern void kmem_cache_free(kmem_cache_t *, void *);
extern unsigned int kmem_cache_size(kmem_cache_t *);

extern void *kmalloc(size_t, int);
extern void kfree(const void *);

//extern int FASTCALL(kmem_cache_reap(int));

/* System wide caches */
extern kmem_cache_t	*vm_area_cachep;
extern kmem_cache_t	*mm_cachep;
extern kmem_cache_t	*names_cachep;
extern kmem_cache_t	*files_cachep;
extern kmem_cache_t	*filp_cachep;
extern kmem_cache_t	*dquot_cachep;
extern kmem_cache_t	*bh_cachep;
extern kmem_cache_t	*fs_cachep;
extern kmem_cache_t	*sigact_cachep;

#endif /* slab */



/*
 *	Berkeley style UIO structures	-	Alan Cox 1994.
 *
 *		This program is free software; you can redistribute it and/or
 *		modify it under the terms of the GNU General Public License
 *		as published by the Free Software Foundation; either version
 *		2 of the License, or (at your option) any later version.
 */


/* A word of warning: Our uio structure will clash with the C library one (which is now obsolete). Remove the C
   library one from sys/uio.h if you have a very old library set */

struct iovec
{
	void *iov_base;		/* BSD uses caddr_t (1003.1g requires void *) */
	__kernel_size_t iov_len; /* Must be size_t (1003.1g) */
};

/*
 *	UIO_MAXIOV shall be at least 16 1003.1g (5.4.1.1)
 */
 
#define UIO_FASTIOV	8
#define UIO_MAXIOV	1024
#if 0
#define UIO_MAXIOV	16	/* Maximum iovec's in one operation 
				   16 matches BSD */
                                /* Beg pardon: BSD has 1024 --ANK */
#endif



/*
 * In Linux 2.4, static timers have been removed from the kernel.
 * Timers may be dynamically created and destroyed, and should be initialized
 * by a call to init_timer() upon creation.
 *
 * The "data" field enables use of a common timeout function for several
 * timeouts. You can use this field to distinguish between the different
 * invocations.
 */
struct timer_list {
	struct list_head list;
	unsigned long expires;
	unsigned long data;
	void (*function)(unsigned long);
};



struct timeval {
  unsigned long tv_sec;
  unsigned long tv_usec;
//	time_t		tv_sec;		/* seconds */
//	suseconds_t	tv_usec;	/* microseconds */
};







#if 1 /* poll */

struct file;

struct poll_table_page;

typedef struct poll_table_struct {
	int error;
	struct poll_table_page * table;
} poll_table;

extern void __pollwait(struct file * filp, wait_queue_head_t * wait_address, poll_table *p);

static inline void poll_wait(struct file * filp, wait_queue_head_t * wait_address, poll_table *p)
{
	if (p && wait_address)
		__pollwait(filp, wait_address, p);
}

static inline void poll_initwait(poll_table* pt)
{
	pt->error = 0;
	pt->table = NULL;
}
extern void poll_freewait(poll_table* pt);


/*
 * Scaleable version of the fd_set.
 */

typedef struct {
	unsigned long *in, *out, *ex;
	unsigned long *res_in, *res_out, *res_ex;
} fd_set_bits;

/*
 * How many longwords for "nr" bits?
 */
#define FDS_BITPERLONG	(8*sizeof(long))
#define FDS_LONGS(nr)	(((nr)+FDS_BITPERLONG-1)/FDS_BITPERLONG)
#define FDS_BYTES(nr)	(FDS_LONGS(nr)*sizeof(long))

/*
 * We do a VERIFY_WRITE here even though we are only reading this time:
 * we'll write to it eventually..
 *
 * Use "unsigned long" accesses to let user-mode fd_set's be long-aligned.
 */
static inline
int get_fd_set(unsigned long nr, void *ufdset, unsigned long *fdset)
{
#if 0
	nr = FDS_BYTES(nr);
	if (ufdset) {
		int error;
		error = verify_area(VERIFY_WRITE, ufdset, nr);
		if (!error && __copy_from_user(fdset, ufdset, nr))
			error = -EFAULT;
		return error;
	}
	memset(fdset, 0, nr);
	return 0;
#else
	return 0;
#endif
}

static inline
void set_fd_set(unsigned long nr, void *ufdset, unsigned long *fdset)
{
#if 0
	if (ufdset)
		__copy_to_user(ufdset, fdset, FDS_BYTES(nr));
#endif
}

static inline
void zero_fd_set(unsigned long nr, unsigned long *fdset)
{
#if 0
	memset(fdset, 0, FDS_BYTES(nr));
#endif
}

extern int do_select(int n, fd_set_bits *fds, long *timeout);

#endif /* poll */



typedef struct
{
  int x;
} read_descriptor_t;





#if 1 /* poll */

/* These are specified by iBCS2 */
#define POLLIN		0x0001
#define POLLPRI		0x0002
#define POLLOUT		0x0004
#define POLLERR		0x0008
#define POLLHUP		0x0010
#define POLLNVAL	0x0020

/* The rest seem to be more-or-less nonstandard. Check them! */
#define POLLRDNORM	0x0040
#define POLLRDBAND	0x0080
#define POLLWRNORM	0x0100
#define POLLWRBAND	0x0200
#define POLLMSG		0x0400

struct pollfd {
	int fd;
	short events;
	short revents;
};

#endif /* poll */

#endif /* _LINUX_TYPES_H */
