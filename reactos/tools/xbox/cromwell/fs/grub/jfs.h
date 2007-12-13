/* jfs.h - an extractions from linux/include/linux/jfs/jfs* into one file */
/*   
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 2000  International Business Machines  Corp.
 *  Copyright (C) 2001  Free Software Foundation, Inc.
 *
 *  This program is free software;  you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or 
 *  (at your option) any later version.
 * 
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY;  without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See
 *  the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program;  if not, write to the Free Software 
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef _JFS_H_
#define _JFS_H_

/* those are from jfs_filsys.h */

/*
 *	 file system option (superblock flag)
 */
/* platform option (conditional compilation) */
#define JFS_AIX		0x80000000	/* AIX support */
/*	POSIX name/directory  support */

#define JFS_OS2		0x40000000	/* OS/2 support */
/*	case-insensitive name/directory support */

#define JFS_LINUX      	0x10000000	/* Linux support */
/*	case-sensitive name/directory support */

/* directory option */
#define JFS_UNICODE	0x00000001	/* unicode name */

/* bba */
#define	JFS_SWAP_BYTES		0x00100000	/* running on big endian computer */


/*
 *	buffer cache configuration
 */
/* page size */
#ifdef PSIZE
#undef PSIZE
#endif
#define	PSIZE		4096	/* page size (in byte) */

/*
 *	fs fundamental size
 *
 * PSIZE >= file system block size >= PBSIZE >= DISIZE
 */
#define	PBSIZE		512	/* physical block size (in byte) */
#define DISIZE		512	/* on-disk inode size (in byte) */
#define L2DISIZE	9
#define	INOSPERIAG	4096	/* number of disk inodes per iag */
#define	L2INOSPERIAG	12
#define INOSPEREXT	32	/* number of disk inode per extent */
#define L2INOSPEREXT	5

/* Minimum number of bytes supported for a JFS partition */
#define MINJFS			(0x1000000)

/*
 * fixed byte offset address
 */
#define SUPER1_OFF	0x8000	/* primary superblock */

#define AITBL_OFF	(SUPER1_OFF + PSIZE + (PSIZE << 1))

/*
 *	fixed reserved inode number
 */
/* aggregate inode */
#define	AGGREGATE_I	1	/* aggregate inode map inode */
#define	FILESYSTEM_I	16	/* 1st/only fileset inode in ait:
				 * fileset inode map inode
				 */

/* per fileset inode */
#define	ROOT_I		2	/* fileset root inode */

/*
 *	directory configuration
 */
#define JFS_NAME_MAX	255
#define JFS_PATH_MAX	PSIZE

typedef unsigned char u8;
typedef char s8;
typedef unsigned short u16;
typedef short s16;
typedef unsigned int u32;
typedef int s32;
typedef unsigned long long u64;
typedef long long s64;

typedef u16 UniChar;

/* these from jfs_btree.h */

/* btpaget_t flag */
#define BT_TYPE		0x07	/* B+-tree index */
#define	BT_ROOT		0x01	/* root page */
#define	BT_LEAF		0x02	/* leaf page */
#define	BT_INTERNAL	0x04	/* internal page */
#define	BT_RIGHTMOST	0x10	/* rightmost page */
#define	BT_LEFTMOST	0x20	/* leftmost page */

/* those are from jfs_types.h */

struct timestruc_t {
	u32 tv_sec;
	u32 tv_nsec;
};

/*
 *	physical xd (pxd)
 */
typedef struct {
	unsigned len:24;
	unsigned addr1:8;
	u32 addr2;
} pxd_t;

/* xd_t field extraction */
#define	lengthPXD(pxd)	((pxd)->len)
#define	addressPXD(pxd)	(((s64)((pxd)->addr1)) << 32 | ((pxd)->addr2))

/*
 *	data extent descriptor (dxd)
 */
typedef struct {
	unsigned flag:8;	/* 1: flags */
	unsigned rsrvd:24;	/* 3: */
	u32 size;		/* 4: size in byte */
	unsigned len:24;	/* 3: length in unit of fsblksize */
	unsigned addr1:8;	/* 1: address in unit of fsblksize */
	u32 addr2;		/* 4: address in unit of fsblksize */
} dxd_t;			/* - 16 - */

/*
 *	DASD limit information - stored in directory inode
 */
typedef struct dasd {
	u8 thresh;		/* Alert Threshold (in percent) */
	u8 delta;		/* Alert Threshold delta (in percent)   */
	u8 rsrvd1;
	u8 limit_hi;		/* DASD limit (in logical blocks)       */
	u32 limit_lo;		/* DASD limit (in logical blocks)       */
	u8 rsrvd2[3];
	u8 used_hi;		/* DASD usage (in logical blocks)       */
	u32 used_lo;		/* DASD usage (in logical blocks)       */
} dasd_t;


/* from jfs_superblock.h */

#define JFS_MAGIC 	0x3153464A	/* "JFS1" */

struct jfs_superblock
{
	u32 s_magic;		/* 4: magic number */
	u32 s_version;		/* 4: version number */

	s64 s_size;		/* 8: aggregate size in hardware/LVM blocks;
				 * VFS: number of blocks
				 */
	s32 s_bsize;		/* 4: aggregate block size in bytes; 
				 * VFS: fragment size
				 */
	s16 s_l2bsize;		/* 2: log2 of s_bsize */
	s16 s_l2bfactor;	/* 2: log2(s_bsize/hardware block size) */
	s32 s_pbsize;		/* 4: hardware/LVM block size in bytes */
	s16 s_l2pbsize;		/* 2: log2 of s_pbsize */
	s16 pad;		/* 2: padding necessary for alignment */

	u32 s_agsize;		/* 4: allocation group size in aggr. blocks */

	u32 s_flag;		/* 4: aggregate attributes:
				 *    see jfs_filsys.h
				 */
	u32 s_state;		/* 4: mount/unmount/recovery state: 
				 *    see jfs_filsys.h
				 */
	s32 s_compress;		/* 4: > 0 if data compression */

	pxd_t s_ait2;		/* 8: first extent of secondary
				 *    aggregate inode table
				 */

	pxd_t s_aim2;		/* 8: first extent of secondary
				 *    aggregate inode map
				 */
	u32 s_logdev;		/* 4: device address of log */
	s32 s_logserial;	/* 4: log serial number at aggregate mount */
	pxd_t s_logpxd;		/* 8: inline log extent */

	pxd_t s_fsckpxd;	/* 8: inline fsck work space extent */

	struct timestruc_t s_time;	/* 8: time last updated */

	s32 s_fsckloglen;	/* 4: Number of filesystem blocks reserved for
				 *    the fsck service log.  
				 *    N.B. These blocks are divided among the
				 *         versions kept.  This is not a per
				 *         version size.
				 *    N.B. These blocks are included in the 
				 *         length field of s_fsckpxd.
				 */
	s8 s_fscklog;		/* 1: which fsck service log is most recent
				 *    0 => no service log data yet
				 *    1 => the first one
				 *    2 => the 2nd one
				 */
	char s_fpack[11];	/* 11: file system volume name 
				 *     N.B. This must be 11 bytes to
				 *          conform with the OS/2 BootSector
				 *          requirements
				 */

	/* extendfs() parameter under s_state & FM_EXTENDFS */
	s64 s_xsize;		/* 8: extendfs s_size */
	pxd_t s_xfsckpxd;	/* 8: extendfs fsckpxd */
	pxd_t s_xlogpxd;	/* 8: extendfs logpxd */
	/* - 128 byte boundary - */

	/*
	 *      DFS VFS support (preliminary) 
	 */
	char s_attach;		/* 1: VFS: flag: set when aggregate is attached
				 */
	u8 rsrvd4[7];		/* 7: reserved - set to 0 */

	u64 totalUsable;	/* 8: VFS: total of 1K blocks which are
				 * available to "normal" (non-root) users.
				 */
	u64 minFree;		/* 8: VFS: # of 1K blocks held in reserve for 
				 * exclusive use of root.  This value can be 0,
				 * and if it is then totalUsable will be equal 
				 * to # of blocks in aggregate.  I believe this
				 * means that minFree + totalUsable = # blocks.
				 * In that case, we don't need to store both 
				 * totalUsable and minFree since we can compute
				 * one from the other.  I would guess minFree 
				 * would be the one we should store, and 
				 * totalUsable would be the one we should 
				 * compute.  (Just a guess...)
				 */

	u64 realFree;		/* 8: VFS: # of free 1K blocks can be used by 
				 * "normal" users.  It may be this is something
				 * we should compute when asked for instead of 
				 * storing in the superblock.  I don't know how
				 * often this information is needed.
				 */
	/*
	 *      graffiti area
	 */
};

/* from jfs_dtree.h */

/*
 *      entry segment/slot
 *
 * an entry consists of type dependent head/only segment/slot and
 * additional segments/slots linked vi next field;
 * N.B. last/only segment of entry is terminated by next = -1;
 */
/*
 *	directory page slot
 */
typedef struct {
	s8 next;		/* 1: */
	s8 cnt;			/* 1: */
	UniChar name[15];	/* 30: */
} dtslot_t;			/* (32) */

#define DTSLOTDATALEN	15

/*
 *	 internal node entry head/only segment
 */
typedef struct {
	pxd_t xd;		/* 8: child extent descriptor */

	s8 next;		/* 1: */
	u8 namlen;		/* 1: */
	UniChar name[11];	/* 22: 2-byte aligned */
} idtentry_t;			/* (32) */

/*
 *	leaf node entry head/only segment
 *
 * 	For legacy filesystems, name contains 13 unichars -- no index field
 */
typedef struct {
	u32 inumber;		/* 4: 4-byte aligned */
	s8 next;		/* 1: */
	u8 namlen;		/* 1: */
	UniChar name[11];	/* 22: 2-byte aligned */
	u32 index;		/* 4: index into dir_table */
} ldtentry_t;			/* (32) */

#define DTLHDRDATALEN	11

/*
 * dir_table used for directory traversal during readdir
*/ 

/*
 * Maximum entry in inline directory table
 */

typedef struct dir_table_slot {
	u8 rsrvd;	/* 1: */
	u8 flag;	/* 1: 0 if free */
	u8 slot;	/* 1: slot within leaf page of entry */
	u8 addr1;	/* 1: upper 8 bits of leaf page address */
	u32 addr2;	/* 4: lower 32 bits of leaf page address -OR-
			      index of next entry when this entry was deleted */
} dir_table_slot_t;	/* (8) */

/*
 *	directory root page (in-line in on-disk inode):
 *
 * cf. dtpage_t below.
 */
typedef union {
	struct {
		dasd_t DASD;	/* 16: DASD limit/usage info  F226941 */

		u8 flag;	/* 1: */
		s8 nextindex;	/* 1: next free entry in stbl */
		s8 freecnt;	/* 1: free count */
		s8 freelist;	/* 1: freelist header */

		u32 idotdot;	/* 4: parent inode number */

		s8 stbl[8];	/* 8: sorted entry index table */
	} header;		/* (32) */

	dtslot_t slot[9];
} dtroot_t;

/*
 *	directory regular page:
 *
 *	entry slot array of 32 byte slot
 *
 * sorted entry slot index table (stbl):
 * contiguous slots at slot specified by stblindex,
 * 1-byte per entry
 *   512 byte block:  16 entry tbl (1 slot)
 *  1024 byte block:  32 entry tbl (1 slot)
 *  2048 byte block:  64 entry tbl (2 slot)
 *  4096 byte block: 128 entry tbl (4 slot)
 *
 * data area:
 *   512 byte block:  16 - 2 =  14 slot
 *  1024 byte block:  32 - 2 =  30 slot
 *  2048 byte block:  64 - 3 =  61 slot
 *  4096 byte block: 128 - 5 = 123 slot
 *
 * N.B. index is 0-based; index fields refer to slot index
 * except nextindex which refers to entry index in stbl;
 * end of entry stot list or freelist is marked with -1.
 */
typedef union {
	struct {
		s64 next;	/* 8: next sibling */
		s64 prev;	/* 8: previous sibling */

		u8 flag;	/* 1: */
		s8 nextindex;	/* 1: next entry index in stbl */
		s8 freecnt;	/* 1: */
		s8 freelist;	/* 1: slot index of head of freelist */

		u8 maxslot;	/* 1: number of slots in page slot[] */
		s8 stblindex;	/* 1: slot index of start of stbl */
		u8 rsrvd[2];	/* 2: */

		pxd_t self;	/* 8: self pxd */
	} header;		/* (32) */

	dtslot_t slot[128];
} dtpage_t;

/* from jfs_xtree.h */

/*
 *      extent allocation descriptor (xad)
 */
typedef struct xad {
	unsigned flag:8;	/* 1: flag */
	unsigned rsvrd:16;	/* 2: reserved */
	unsigned off1:8;	/* 1: offset in unit of fsblksize */
	u32 off2;		/* 4: offset in unit of fsblksize */
	unsigned len:24;	/* 3: length in unit of fsblksize */
	unsigned addr1:8;	/* 1: address in unit of fsblksize */
	u32 addr2;		/* 4: address in unit of fsblksize */
} xad_t;			/* (16) */

/* xad_t field extraction */
#define offsetXAD(xad)	(((s64)((xad)->off1)) << 32 | ((xad)->off2))
#define addressXAD(xad)	(((s64)((xad)->addr1)) << 32 | ((xad)->addr2))
#define lengthXAD(xad)	((xad)->len)

/* possible values for maxentry */
#define XTPAGEMAXSLOT   256
#define XTENTRYSTART    2

/*
 *      xtree page:
 */
typedef union {
	struct xtheader {
		s64 next;	/* 8: */
		s64 prev;	/* 8: */

		u8 flag;	/* 1: */
		u8 rsrvd1;	/* 1: */
		s16 nextindex;	/* 2: next index = number of entries */
		s16 maxentry;	/* 2: max number of entries */
		s16 rsrvd2;	/* 2: */

		pxd_t self;	/* 8: self */
	} header;		/* (32) */

	xad_t xad[XTPAGEMAXSLOT];	/* 16 * maxentry: xad array */
} xtpage_t;

/* from jfs_dinode.h */

struct dinode {
	/*
	 *      I. base area (128 bytes)
	 *      ------------------------
	 *
	 * define generic/POSIX attributes
	 */
	u32 di_inostamp;	/* 4: stamp to show inode belongs to fileset */
	s32 di_fileset;		/* 4: fileset number */
	u32 di_number;		/* 4: inode number, aka file serial number */
	u32 di_gen;		/* 4: inode generation number */

	pxd_t di_ixpxd;		/* 8: inode extent descriptor */

	s64 di_size;		/* 8: size */
	s64 di_nblocks;		/* 8: number of blocks allocated */

	u32 di_nlink;		/* 4: number of links to the object */

	u32 di_uid;		/* 4: user id of owner */
	u32 di_gid;		/* 4: group id of owner */

	u32 di_mode;		/* 4: attribute, format and permission */

	struct timestruc_t di_atime;	/* 8: time last data accessed */
	struct timestruc_t di_ctime;	/* 8: time last status changed */
	struct timestruc_t di_mtime;	/* 8: time last data modified */
	struct timestruc_t di_otime;	/* 8: time created */

	dxd_t di_acl;		/* 16: acl descriptor */

	dxd_t di_ea;		/* 16: ea descriptor */

	s32 di_next_index;  /* 4: Next available dir_table index */

	s32 di_acltype;		/* 4: Type of ACL */

	/*
	 * 	Extension Areas.
	 *
	 *	Historically, the inode was partitioned into 4 128-byte areas,
	 *	the last 3 being defined as unions which could have multiple
	 *	uses.  The first 96 bytes had been completely unused until
	 *	an index table was added to the directory.  It is now more
	 *	useful to describe the last 3/4 of the inode as a single
	 *	union.  We would probably be better off redesigning the
	 *	entire structure from scratch, but we don't want to break
	 *	commonality with OS/2's JFS at this time.
	 */
	union {
		struct {
			/*
			 * This table contains the information needed to
			 * find a directory entry from a 32-bit index.
			 * If the index is small enough, the table is inline,
			 * otherwise, an x-tree root overlays this table
			 */
			dir_table_slot_t _table[12];	/* 96: inline */

			dtroot_t _dtroot;		/* 288: dtree root */
		} _dir;					/* (384) */
#define di_dirtable	u._dir._table
#define di_dtroot	u._dir._dtroot
#define di_parent       di_dtroot.header.idotdot
#define di_DASD		di_dtroot.header.DASD

		struct {
			union {
				u8 _data[96];		/* 96: unused */
				struct {
					void *_imap;	/* 4: unused */
					u32 _gengen;	/* 4: generator */
				} _imap;
			} _u1;				/* 96: */
#define di_gengen	u._file._u1._imap._gengen

			union {
				xtpage_t _xtroot;
				struct {
					u8 unused[16];	/* 16: */
					dxd_t _dxd;	/* 16: */
					union {
						u32 _rdev;	/* 4: */
						u8 _fastsymlink[128];
					} _u;
					u8 _inlineea[128];
				} _special;
			} _u2;
		} _file;
#define di_xtroot	u._file._u2._xtroot
#define di_dxd		u._file._u2._special._dxd
#define di_btroot	di_xtroot
#define di_inlinedata	u._file._u2._special._u
#define di_rdev		u._file._u2._special._u._rdev
#define di_fastsymlink	u._file._u2._special._u._fastsymlink
#define di_inlineea     u._file._u2._special._inlineea
	} u;
};

typedef struct dinode dinode_t;

/* di_mode */
#define IFMT	0xF000		/* S_IFMT - mask of file type */
#define IFDIR	0x4000		/* S_IFDIR - directory */
#define IFREG	0x8000		/* S_IFREG - regular file */
#define IFLNK	0xA000		/* S_IFLNK - symbolic link */

/* extended mode bits (on-disk inode di_mode) */
#define INLINEEA        0x00040000	/* inline EA area free */

/* from jfs_imap.h */

#define	EXTSPERIAG	128	/* number of disk inode extent per iag  */
#define SMAPSZ		4	/* number of words per summary map      */
#define	MAXAG		128	/* maximum number of allocation groups  */

/*
 *	inode allocation map:
 * 
 * inode allocation map consists of 
 * . the inode map control page and
 * . inode allocation group pages (per 4096 inodes)
 * which are addressed by standard JFS xtree.
 */
/*
 *	inode allocation group page (per 4096 inodes of an AG)
 */
typedef struct {
	s64 agstart;		/* 8: starting block of ag              */
	s32 iagnum;		/* 4: inode allocation group number     */
	s32 inofreefwd;		/* 4: ag inode free list forward        */
	s32 inofreeback;	/* 4: ag inode free list back           */
	s32 extfreefwd;		/* 4: ag inode extent free list forward */
	s32 extfreeback;	/* 4: ag inode extent free list back    */
	s32 iagfree;		/* 4: iag free list                     */

	/* summary map: 1 bit per inode extent */
	s32 inosmap[SMAPSZ];	/* 16: sum map of mapwords w/ free inodes;
				 *      note: this indicates free and backed
				 *      inodes, if the extent is not backed the
				 *      value will be 1.  if the extent is
				 *      backed but all inodes are being used the
				 *      value will be 1.  if the extent is
				 *      backed but at least one of the inodes is
				 *      free the value will be 0.
				 */
	s32 extsmap[SMAPSZ];	/* 16: sum map of mapwords w/ free extents */
	s32 nfreeinos;		/* 4: number of free inodes             */
	s32 nfreeexts;		/* 4: number of free extents            */
	/* (72) */
	u8 pad[1976];		/* 1976: pad to 2048 bytes */
	/* allocation bit map: 1 bit per inode (0 - free, 1 - allocated) */
	u32 wmap[EXTSPERIAG];	/* 512: working allocation map  */
	u32 pmap[EXTSPERIAG];	/* 512: persistent allocation map */
	pxd_t inoext[EXTSPERIAG];	/* 1024: inode extent addresses */
} iag_t;			/* (4096) */

#endif /* _JFS_H_ */
