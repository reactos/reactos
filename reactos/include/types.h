/*
 * Added wide character type wchar_t, june 1998 -- Boudewijn Dekker
 */
#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#ifndef NULL
# define NULL		((void *) 0)
#endif


#ifndef _I386_TYPES_H
#define _I386_TYPES_H

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
#ifdef __KERNEL__

typedef signed char s8;
typedef unsigned char u8;

typedef signed short s16;
typedef unsigned short u16;

typedef signed int s32;
typedef unsigned int u32;

typedef signed long long s64;
typedef unsigned long long u64;

#endif /* __KERNEL__ */

#endif


typedef unsigned int size_t;
typedef size_t __kernel_size_t;
typedef unsigned short wchar_t;


#endif /* _LINUX_TYPES_H */
