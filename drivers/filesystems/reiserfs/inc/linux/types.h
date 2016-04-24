#ifndef _LINUX_TYPES_H
#define _LINUX_TYPES_H

#if !defined(_MSC_VER) && !defined(__REACTOS__)
#error  _MSC_VER not defined
#endif


typedef unsigned __int8 __u8;
typedef signed __int8 __s8;

typedef signed   __int64 __s64;
typedef unsigned __int64 __u64;

typedef	signed   __int16	__s16;
typedef	unsigned __int16	__u16;

typedef	signed   __int32	__s32;
typedef	unsigned __int32	__u32;

typedef	signed   __int64	__s64;
typedef	unsigned __int64	__u64;


typedef __u16 u16;
typedef __u32 u32;
typedef __u32 ino_t;
typedef __u32 blk_t;

/**
 * container_of - cast a member of a structure out to the containing structure
 *
 * @ptr:        the pointer to the member.
 * @type:       the type of the container struct this is embedded in.
 * @member:     the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({                      \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})



/*
 * Inode flags		[from original ext2 sources]
 */
#define	EXT2_SECRM_FL			0x00000001 /* Secure deletion */
#define	EXT2_UNRM_FL			0x00000002 /* Undelete */
#define	EXT2_COMPR_FL			0x00000004 /* Compress file */
#define EXT2_SYNC_FL			0x00000008 /* Synchronous updates */
#define EXT2_IMMUTABLE_FL		0x00000010 /* Immutable file */
#define EXT2_APPEND_FL			0x00000020 /* writes to file may only append */
#define EXT2_NODUMP_FL			0x00000040 /* do not dump file */
#define EXT2_NOATIME_FL			0x00000080 /* do not update atime */
/* Reserved for compression usage... */
#define EXT2_DIRTY_FL			0x00000100
#define EXT2_COMPRBLK_FL		0x00000200 /* One or more compressed clusters */
#define EXT2_NOCOMP_FL			0x00000400 /* Don't compress */
#define EXT2_ECOMPR_FL			0x00000800 /* Compression error */
/* End compression flags --- maybe not all used */	
#define EXT2_BTREE_FL			0x00001000 /* btree format dir */
#define EXT2_RESERVED_FL		0x80000000 /* reserved for ext2 lib */

#define EXT2_FL_USER_VISIBLE		0x00001FFF /* User visible flags */
#define EXT2_FL_USER_MODIFIABLE		0x000000FF /* User modifiable flags */


#define __LITTLE_ENDIAN
#define le16_to_cpu(x)		(x)
#define cpu_to_le16(x)		(x)

#endif /* LINUX_TYPES_H */
