/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS File System Recognizer
 * FILE:             drivers/filesystems/fs_rec/ffs.h
 * PURPOSE:          FFS Header File
 * PROGRAMMER:       Peter Hater
 *                   Pierre Schweitzer (pierre@reactos.org)
 */

#include <pshpack1.h>

/*
 * The path name on which the file system is mounted is maintained
 * in fs_fsmnt. MAXMNTLEN defines the amount of space allocated in
 * the super block for this name.
 */
#define	MAXMNTLEN	468

/*
 * The volume name for this filesystem is maintained in fs_volname.
 * MAXVOLLEN defines the length of the buffer allocated.
 * This space used to be part of of fs_fsmnt.
 */
#define MAXVOLLEN	32

/*
 * Unused value currently, FreeBSD compat.
 */
#define FSMAXSNAP 20

/*
 * There is a 128-byte region in the superblock reserved for in-core
 * pointers to summary information. Originally this included an array
 * of pointers to blocks of struct csum; now there are just four
 * pointers and the remaining space is padded with fs_ocsp[].
 * NOCSPTRS determines the size of this padding. One pointer (fs_csp)
 * is taken away to point to a contiguous array of struct csum for
 * all cylinder groups; a second (fs_maxcluster) points to an array
 * of cluster sizes that is computed as cylinder groups are inspected;
 * the third (fs_contigdirs) points to an array that tracks the
 * creation of new directories; and the fourth (fs_active) is used
 * by snapshots.
 */
#define	NOCSPTRS	((128 / sizeof(void *)) - 4)

/*
 * Per cylinder group information; summarized in blocks allocated
 * from first cylinder group data blocks.  These blocks have to be
 * read in from fs_csaddr (size fs_cssize) in addition to the
 * super block.
 */
struct csum {
	INT32	cs_ndir;		/* number of directories */
	INT32	cs_nbfree;		/* number of free blocks */
	INT32	cs_nifree;		/* number of free inodes */
	INT32	cs_nffree;		/* number of free frags */
};

struct csum_total {
	INT64   cs_ndir;		/* number of directories */
	INT64   cs_nbfree;		/* number of free blocks */
	INT64   cs_nifree;		/* number of free inodes */
	INT64   cs_nffree;		/* number of free frags */
	INT64   cs_spare[4];	/* future expansion */
};

/*
 * Super block for an FFS file system in memory.
 */
typedef struct fs {
	INT32	 fs_firstfield;		/* historic file system linked list, */
	INT32	 fs_unused_1;		/*     used for incore super blocks */
	INT32    fs_sblkno;		/* addr of super-block in filesys */
	INT32    fs_cblkno;		/* offset of cyl-block in filesys */
	INT32    fs_iblkno;		/* offset of inode-blocks in filesys */
	INT32    fs_dblkno;		/* offset of first data after cg */
	INT32	 fs_old_cgoffset;	/* cylinder group offset in cylinder */
	INT32	 fs_old_cgmask;		/* used to calc mod fs_ntrak */
	INT32	 fs_old_time;		/* last time written */
	INT32	 fs_old_size;		/* number of blocks in fs */
	INT32	 fs_old_dsize;		/* number of data blocks in fs */
	INT32	 fs_ncg;		/* number of cylinder groups */
	INT32	 fs_bsize;		/* size of basic blocks in fs */
	INT32	 fs_fsize;		/* size of frag blocks in fs */
	INT32	 fs_frag;		/* number of frags in a block in fs */
/* these are configuration parameters */
	INT32	 fs_minfree;		/* minimum percentage of free blocks */
	INT32	 fs_old_rotdelay;	/* num of ms for optimal next block */
	INT32	 fs_old_rps;		/* disk revolutions per second */
/* these fields can be computed from the others */
	INT32	 fs_bmask;		/* ``blkoff'' calc of blk offsets */
	INT32	 fs_fmask;		/* ``fragoff'' calc of frag offsets */
	INT32	 fs_bshift;		/* ``lblkno'' calc of logical blkno */
	INT32	 fs_fshift;		/* ``numfrags'' calc number of frags */
/* these are configuration parameters */
	INT32	 fs_maxcontig;		/* max number of contiguous blks */
	INT32	 fs_maxbpg;		/* max number of blks per cyl group */
/* these fields can be computed from the others */
	INT32	 fs_fragshift;		/* block to frag shift */
	INT32	 fs_fsbtodb;		/* fsbtodb and dbtofsb shift constant */
	INT32	 fs_sbsize;		/* actual size of super block */
	INT32	 fs_spare1[2];		/* old fs_csmask */
					/* old fs_csshift */
	INT32	 fs_nindir;		/* value of NINDIR */
	INT32	 fs_inopb;		/* value of INOPB */
	INT32	 fs_old_nspf;		/* value of NSPF */
/* yet another configuration parameter */
	INT32	 fs_optim;		/* optimization preference, see below */
/* these fields are derived from the hardware */
	INT32	 fs_old_npsect;		/* # sectors/track including spares */
	INT32	 fs_old_interleave;	/* hardware sector interleave */
	INT32	 fs_old_trackskew;	/* sector 0 skew, per track */
/* fs_id takes the space of the unused fs_headswitch and fs_trkseek fields */
	INT32	 fs_id[2];		/* unique file system id */
/* sizes determined by number of cylinder groups and their sizes */
	INT32    fs_old_csaddr;		/* blk addr of cyl grp summary area */
	INT32	 fs_cssize;		/* size of cyl grp summary area */
	INT32	 fs_cgsize;		/* cylinder group size */
/* these fields are derived from the hardware */
	INT32	 fs_spare2;		/* old fs_ntrak */
	INT32	 fs_old_nsect;		/* sectors per track */
	INT32	 fs_old_spc;		/* sectors per cylinder */
	INT32	 fs_old_ncyl;		/* cylinders in file system */
	INT32	 fs_old_cpg;		/* cylinders per group */
	INT32	 fs_ipg;		/* inodes per group */
	INT32	 fs_fpg;		/* blocks per group * fs_frag */
/* this data must be re-computed after crashes */
	struct	csum fs_old_cstotal;	/* cylinder summary information */
/* these fields are cleared at mount time */
	INT8	 fs_fmod;		/* super block modified flag */
	INT8	 fs_clean;		/* file system is clean flag */
	INT8	 fs_ronly;		/* mounted read-only flag */
	UINT8	 fs_old_flags;		/* see FS_ flags below */
	UCHAR	 fs_fsmnt[MAXMNTLEN];	/* name mounted on */
	UCHAR    fs_volname[MAXVOLLEN];	/* volume name */
	UINT64   fs_swuid;		/* system-wide uid */
	INT32	 fs_pad;
/* these fields retain the current block allocation info */
	INT32	 fs_cgrotor;		/* last cg searched (UNUSED) */
	void 	*fs_ocsp[NOCSPTRS];	/* padding; was list of fs_cs buffers */
	UINT8   *fs_contigdirs;	/* # of contiguously allocated dirs */
	struct csum *fs_csp;		/* cg summary info buffer for fs_cs */
	INT32	*fs_maxcluster;		/* max cluster in each cyl group */
	INT32	*fs_active;		/* used by snapshots to track fs */
	INT32	 fs_old_cpc;		/* cyl per cycle in postbl */
/* this area is otherwise allocated unless fs_old_flags & FS_FLAGS_UPDATED */
	INT32	 fs_maxbsize;		/* maximum blocking factor permitted */
	INT64	 fs_sparecon64[17];	/* old rotation block list head */
	INT64	 fs_sblockloc;		/* byte offset of standard superblock */
	struct	csum_total fs_cstotal;	/* cylinder summary information */
	INT64    fs_time;		/* last time written */
	INT64	 fs_size;		/* number of blocks in fs */
	INT64	 fs_dsize;		/* number of data blocks in fs */
	INT64    fs_csaddr;		/* blk addr of cyl grp summary area */
	INT64	 fs_pendingblocks;	/* blocks in process of being freed */
	INT32	 fs_pendinginodes;	/* inodes in process of being freed */
	INT32	 fs_snapinum[FSMAXSNAP];/* list of snapshot inode numbers */
/* back to stuff that has been around a while */
	INT32	 fs_avgfilesize;	/* expected average file size */
	INT32	 fs_avgfpdir;		/* expected # of files per directory */
	INT32	 fs_save_cgsize;	/* save real cg size to use fs_bsize */
	INT32	 fs_sparecon32[26];	/* reserved for future constants */
	INT32    fs_flags;		/* see FS_ flags below */
/* back to stuff that has been around a while (again) */
	INT32	 fs_contigsumsize;	/* size of cluster summary array */
	INT32	 fs_maxsymlinklen;	/* max length of an internal symlink */
	INT32	 fs_old_inodefmt;	/* format of on-disk inodes */
	UINT64   fs_maxfilesize;	/* maximum representable file size */
	INT64	 fs_qbmask;		/* ~fs_bmask for use with 64-bit size */
	INT64	 fs_qfmask;		/* ~fs_fmask for use with 64-bit size */
	INT32	 fs_state;		/* validate fs_clean field (UNUSED) */
	INT32	 fs_old_postblformat;	/* format of positional layout tables */
	INT32	 fs_old_nrpos;		/* number of rotational positions */
	INT32    fs_spare5[2];		/* old fs_postbloff */
					/* old fs_rotbloff */
	INT32	 fs_magic;		/* magic number */
} FFSD_SUPER_BLOCK, *PFFSD_SUPER_BLOCK;

#define	MAXPARTITIONS		16	/* number of partitions */
#define	NDDATA 5
#define	NSPARE 5

typedef struct disklabel {
	UINT32 d_magic;		/* the magic number */
	UINT16 d_type;		/* drive type */
	UINT16 d_subtype;		/* controller/d_type specific */
	char	  d_typename[16];	/* type name, e.g. "eagle" */

	/*
	 * d_packname contains the pack identifier and is returned when
	 * the disklabel is read off the disk or in-core copy.
	 * d_boot0 and d_boot1 are the (optional) names of the
	 * primary (block 0) and secondary (block 1-15) bootstraps
	 * as found in /usr/mdec.  These are returned when using
	 * getdiskbyname(3) to retrieve the values from /etc/disktab.
	 */
	union {
		char	un_d_packname[16];	/* pack identifier */
		struct {
			char *un_d_boot0;	/* primary bootstrap name */
			char *un_d_boot1;	/* secondary bootstrap name */
		} un_b;
	} d_un;

			/* disk geometry: */
	UINT32 d_secsize;		/* # of bytes per sector */
	UINT32 d_nsectors;		/* # of data sectors per track */
	UINT32 d_ntracks;		/* # of tracks per cylinder */
	UINT32 d_ncylinders;		/* # of data cylinders per unit */
	UINT32 d_secpercyl;		/* # of data sectors per cylinder */
	UINT32 d_secperunit;		/* # of data sectors per unit */

	/*
	 * Spares (bad sector replacements) below are not counted in
	 * d_nsectors or d_secpercyl.  Spare sectors are assumed to
	 * be physical sectors which occupy space at the end of each
	 * track and/or cylinder.
	 */
	UINT16 d_sparespertrack;	/* # of spare sectors per track */
	UINT16 d_sparespercyl;	/* # of spare sectors per cylinder */
	/*
	 * Alternative cylinders include maintenance, replacement,
	 * configuration description areas, etc.
	 */
	UINT32 d_acylinders;		/* # of alt. cylinders per unit */

			/* hardware characteristics: */
	/*
	 * d_interleave, d_trackskew and d_cylskew describe perturbations
	 * in the media format used to compensate for a slow controller.
	 * Interleave is physical sector interleave, set up by the
	 * formatter or controller when formatting.  When interleaving is
	 * in use, logically adjacent sectors are not physically
	 * contiguous, but instead are separated by some number of
	 * sectors.  It is specified as the ratio of physical sectors
	 * traversed per logical sector.  Thus an interleave of 1:1
	 * implies contiguous layout, while 2:1 implies that logical
	 * sector 0 is separated by one sector from logical sector 1.
	 * d_trackskew is the offset of sector 0 on track N relative to
	 * sector 0 on track N-1 on the same cylinder.  Finally, d_cylskew
	 * is the offset of sector 0 on cylinder N relative to sector 0
	 * on cylinder N-1.
	 */
	UINT16 d_rpm;		/* rotational speed */
	UINT16 d_interleave;		/* hardware sector interleave */
	UINT16 d_trackskew;		/* sector 0 skew, per track */
	UINT16 d_cylskew;		/* sector 0 skew, per cylinder */
	UINT32 d_headswitch;		/* head switch time, usec */
	UINT32 d_trkseek;		/* track-to-track seek, usec */
	UINT32 d_flags;		/* generic flags */
	UINT32 d_drivedata[NDDATA];	/* drive-type specific information */
	UINT32 d_spare[NSPARE];	/* reserved for future use */
	UINT32 d_magic2;		/* the magic number (again) */
	UINT16 d_checksum;		/* xor of data incl. partitions */

			/* filesystem and partition information: */
	UINT16 d_npartitions;	/* number of partitions in following */
	UINT32 d_bbsize;		/* size of boot area at sn0, bytes */
	UINT32 d_sbsize;		/* max size of fs superblock, bytes */
	struct	partition {		/* the partition table */
		UINT32 p_size;	/* number of sectors in partition */
		UINT32 p_offset;	/* starting sector */
		union {
			UINT32 fsize; /* FFS, ADOS:
					    filesystem basic fragment size */
			UINT32 cdsession; /* ISO9660: session offset */
		} __partition_u2;
		UINT8 p_fstype;	/* filesystem type, see below */
		UINT8 p_frag;	/* filesystem fragments per block */
		union {
			UINT16 cpg;	/* UFS: FS cylinders per group */
			UINT16 sgs;	/* LFS: FS segment shift */
		} __partition_u1;
	} d_partitions[MAXPARTITIONS];	/* actually may be more */
} FFSD_DISKLABEL, *PFFSD_DISKLABEL;
#include <poppack.h>

C_ASSERT(FIELD_OFFSET(FFSD_SUPER_BLOCK, fs_cgsize) == 160);
C_ASSERT(FIELD_OFFSET(FFSD_SUPER_BLOCK, fs_fmod) == 208);
C_ASSERT(FIELD_OFFSET(FFSD_SUPER_BLOCK, fs_ocsp) == 728);

#define SBLOCK_UFS1     8192
#define SBLOCK_UFS2    65536

#define SBLOCKSIZE      8192

/*
 * File system identification
 */
#define	FS_UFS1_MAGIC	0x011954	/* UFS1 fast file system magic number */
#define	FS_UFS2_MAGIC	0x19540119	/* UFS2 fast file system magic number */

#define	DISKMAGIC	((UINT32)0x82564557)	/* The disk magic number */

#define	LABELSECTOR		1	/* sector containing label */

#define	FS_BSDFFS	7		/* 4.2BSD fast file system */
