#ifndef _LINUX_FS_INCLUDE_
#define _LINUX_FS_INCLUDE_

#include <linux/types.h>
#include <linux/atomic.h>
#include <linux/rbtree.h>

//
// kdev
//

#define NODEV           0

typedef struct block_device * kdev_t;

#define MINORBITS   8
#define MINORMASK   ((1U << MINORBITS) - 1)

#define MAJOR(dev)   ((unsigned int)((int)(dev) >> MINORBITS))
#define MINOR(dev)   ((unsigned int)((int)(dev) & MINORMASK))

static inline unsigned int kdev_t_to_nr(kdev_t dev) {
    /*return (unsigned int)(MAJOR(dev)<<8) | MINOR(dev);*/
    return 0;
}

#define NODEV		0
#define MKDEV(ma,mi)	(((ma) << MINORBITS) | (mi))

static inline kdev_t to_kdev_t(int dev)
{
#if 0
    int major, minor;
#if 0
    major = (dev >> 16);
    if (!major) {
        major = (dev >> 8);
        minor = (dev & 0xff);
    } else
        minor = (dev & 0xffff);
#else
    major = (dev >> 8);
    minor = (dev & 0xff);
#endif
    return (kdev_t) MKDEV(major, minor);
#endif
    return 0;
}


//
// file system specific structures
//

/*
 * Kernel pointers have redundant information, so we can use a
 * scheme where we can return either an error code or a dentry
 * pointer with the same return value.
 *
 * This should be a per-architecture thing, to allow different
 * error and pointer decisions.
 */

struct super_block {
    unsigned long       s_magic;
    unsigned long       s_flags;
    unsigned long		s_blocksize;        /* blocksize */
    unsigned long long  s_maxbytes;
    unsigned char		s_blocksize_bits;   /* bits of blocksize */
    unsigned char		s_dirt;             /* any thing */
    char                s_id[30];           /* id string */
    kdev_t              s_bdev;             /* block_device */
    void *              s_priv;             /* EXT2_VCB */
    struct dentry      *s_root;
    void               *s_fs_info;
};

struct inode {
    __u32               i_ino;              /* inode number */
    loff_t			    i_size;             /* size */
    __u32               i_atime;	        /* Access time */
    __u32               i_ctime;	        /* Creation time */
    __u32               i_mtime;	        /* Modification time */
    __u32               i_dtime;	        /* Deletion Time */
    __u64               i_blocks;
    __u32               i_block[15];
    umode_t			    i_mode;             /* mode */
    uid_t               i_uid;
    gid_t               i_gid;
    atomic_t            i_count;            /* ref count */
    __u16               i_nlink;
    __u32               i_generation;
    __u32               i_version;
    __u32               i_flags;

    struct super_block *i_sb;               /* super_block */
    void               *i_priv;             /* EXT2_MCB */

    __u16               i_extra_isize;      /* extra fields' size */
    __u64               i_file_acl;
};

//
//  Inode state bits
//

#define I_DIRTY_SYNC        1 /* Not dirty enough for O_DATASYNC */
#define I_DIRTY_DATASYNC    2 /* Data-related inode changes pending */
#define I_DIRTY_PAGES       4 /* Data-related inode changes pending */
#define I_LOCK              8
#define I_FREEING          16
#define I_CLEAR            32

#define I_DIRTY (I_DIRTY_SYNC | I_DIRTY_DATASYNC | I_DIRTY_PAGES)


struct dentry {
    atomic_t                d_count;
    struct {
        int             len;
        char           *name;
    } d_name;
    struct inode           *d_inode;
    struct dentry          *d_parent;
    void                   *d_fsdata;
    struct super_block     *d_sb;
};

struct file {

    unsigned int    f_flags;
    umode_t         f_mode;
    __u32           f_version;
    __int64         f_size;
    loff_t          f_pos;
    struct dentry  *f_dentry;
    void           *private_data;
};

/*
 * File types
 *
 * NOTE! These match bits 12..15 of stat.st_mode
 * (ie "(i_mode >> 12) & 15").
 */
#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

void iget(struct inode *inode);
void iput(struct inode *inode);
ULONGLONG bmap(struct inode *i, ULONGLONG b);

#endif /*_LINUX_FS_INCLUDE_*/
