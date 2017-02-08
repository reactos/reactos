#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#if _NT_TARGET_VERSION <= 0x500
#define _WIN2K_COMPAT_SLIST_USAGE
#endif

#include <linux/config.h>
#include <ntifs.h>
#include <ntdddisk.h>
#include <windef.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <wchar.h>

typedef unsigned __int8     __u8;
typedef signed   __int8     __s8;

typedef	signed   __int16	__s16;
typedef	unsigned __int16	__u16;

typedef	signed   __int32	__s32;
typedef	unsigned __int32	__u32;

typedef	signed   __int64	__s64;
typedef	unsigned __int64	__u64;


typedef __s8         s8;
typedef __u8         u8;
typedef __s16       s16;
typedef __u16       u16;
typedef __s32       s32;
typedef __u32       u32;
typedef __s64       s64;
typedef __u64       u64;

#define __le16      u16
#define __le32      u32
#define __le64      u64

#define bool        BOOLEAN

//
// gcc special keyworks
//
#if defined(_MSC_VER) || !defined(__REACTOS__)
/* FIXME: Inspect why this is needed */
#define __attribute__(x)
#endif
#define __bitwise
#define __releases(x)

#ifdef _MSC_VER
#define inline __inline
#endif

#ifndef noinline
#define noinline
#endif

typedef __u32 __bitwise __be32;
typedef __u16 __bitwise __be16;

#define uid_t       u16
#define gid_t       u16
typedef int         pid_t;
typedef unsigned __bitwise gfp_t;

typedef unsigned short umode_t; /* inode mode */

/*
 * The type used for indexing onto a disc or disc partition.
 * If required, asm/types.h can override it and define
 * HAVE_SECTOR_T
 */
typedef unsigned __int64 sector_t;
typedef unsigned __int64 blkcnt_t;
typedef unsigned __int64 loff_t;

#define BITS_PER_LONG  (32)
#define ORDER_PER_LONG (05)

#if defined(_WIN64)
typedef __int64 long_ptr_t;
typedef unsigned __int64 ulong_ptr_t;
# define CFS_BITS_PER_LONG  (64)
# define CFS_ORDER_PER_LONG (06)
#else
typedef long long_ptr_t;
typedef unsigned long ulong_ptr_t;
# define CFS_BITS_PER_LONG  (32)
# define CFS_ORDER_PER_LONG (05)
#endif

//
// bit spin lock
//

#define __acquire(x)
#define __release(x)

#define preempt_enable()
#define preempt_disable()

//
// __FUNCTION__ issue
//

#if _MSC_VER <= 1300
#define __FUNCTION__ ("jbd")
#endif

#define BUG() do {DbgBreakPoint();} while(0)

#endif /* LINUX_TYPES_H */
