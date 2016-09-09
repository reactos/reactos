/*	$NetBSD: disklabel.h,v 1.88 2003/11/14 12:07:42 lukem Exp $	*/

/*
 * Copyright (c) 1987, 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)disklabel.h	8.2 (Berkeley) 7/10/94
 */

#ifndef _SYS_DISKLABEL_H_
#define	_SYS_DISKLABEL_H_

#include "type.h"

/*
 * We need <machine/types.h> for __HAVE_OLD_DISKLABEL
 */
#if 0 /* XXX ffsdrv */
#ifndef _LOCORE
#include <sys/types.h>
#endif
#endif

/* sys/arch/i386/include/type.h XXX ffsdrv */
#define	__HAVE_OLD_DISKLABEL

/*
 * Each disk has a label which includes information about the hardware
 * disk geometry, filesystem partitions, and drive specific information.
 * The location of the label, as well as the number of partitions the
 * label can describe and the number of the "whole disk" (raw)
 * paritition are machine dependent.
 */
#if 0 /* XXX ffsdrv */
#include <machine/disklabel.h>
#endif

/* arch/i386/include/disklabel.h */
#define	LABELSECTOR		1	/* sector containing label */
#define	LABELOFFSET		0	/* offset of label in sector */
#define	MAXPARTITIONS		16	/* number of partitions */
#define	OLDMAXPARTITIONS 	8	/* number of partitions before 1.6 */
#define	RAW_PART		3	/* raw partition: XX?d (XXX) */

/*
 * We use the highest bit of the minor number for the partition number.
 * This maintains backward compatibility with device nodes created before
 * MAXPARTITIONS was increased.
 */
#define __I386_MAXDISKS	((1 << 20) / MAXPARTITIONS)
#define DISKUNIT(dev)	((minor(dev) / OLDMAXPARTITIONS) % __I386_MAXDISKS)
#define DISKPART(dev)	((minor(dev) % OLDMAXPARTITIONS) + \
    ((minor(dev) / (__I386_MAXDISKS * OLDMAXPARTITIONS)) * OLDMAXPARTITIONS))
#define	DISKMINOR(unit, part) \
    (((unit) * OLDMAXPARTITIONS) + ((part) % OLDMAXPARTITIONS) + \
     ((part) / OLDMAXPARTITIONS) * (__I386_MAXDISKS * OLDMAXPARTITIONS))

/* Pull in MBR partition definitions. */
#include "bootblock.h"

/* end of arch/i386/include/disklabel.h */


/*
 * The absolute maximum number of disk partitions allowed.
 * This is the maximum value of MAXPARTITIONS for which 'struct disklabel'
 * is <= DEV_BSIZE bytes long.  If MAXPARTITIONS is greater than this, beware.
 */
#define	MAXMAXPARTITIONS	22
#if MAXPARTITIONS > MAXMAXPARTITIONS
#warning beware: MAXPARTITIONS bigger than MAXMAXPARTITIONS
#endif

/*
 * Ports can switch their MAXPARTITIONS once, as follows:
 *
 * - define OLDMAXPARTITIONS in <machine/disklabel.h> as the old number
 * - define MAXPARTITIONS as the new number
 * - define DISKUNIT, DISKPART and DISKMINOR macros in <machine/disklabel.h>
 *   as appropriate for the port (see the i386 one for an example).
 * - define __HAVE_OLD_DISKLABEL in <machine/types.h>
 */

#if defined(_KERNEL) && defined(__HAVE_OLD_DISKLABEL) && \
	   (MAXPARTITIONS < OLDMAXPARTITIONS)
#error "can only grow disklabel size"
#endif


/*
 * Translate between device numbers and major/disk unit/disk partition.
 */
#ifndef __HAVE_OLD_DISKLABEL
#define	DISKUNIT(dev)	(minor(dev) / MAXPARTITIONS)
#define	DISKPART(dev)	(minor(dev) % MAXPARTITIONS)
#define	DISKMINOR(unit, part) \
    (((unit) * MAXPARTITIONS) + (part))
#endif
#define	MAKEDISKDEV(maj, unit, part) \
    (makedev((maj), DISKMINOR((unit), (part))))

#define	DISKMAGIC	((u_int32_t)0x82564557)	/* The disk magic number */

#ifndef _LOCORE
struct disklabel {
	u_int32_t d_magic;		/* the magic number */
	u_int16_t d_type;		/* drive type */
	u_int16_t d_subtype;		/* controller/d_type specific */
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
#define	d_packname	d_un.un_d_packname
#define	d_boot0		d_un.un_b.un_d_boot0
#define	d_boot1		d_un.un_b.un_d_boot1

			/* disk geometry: */
	u_int32_t d_secsize;		/* # of bytes per sector */
	u_int32_t d_nsectors;		/* # of data sectors per track */
	u_int32_t d_ntracks;		/* # of tracks per cylinder */
	u_int32_t d_ncylinders;		/* # of data cylinders per unit */
	u_int32_t d_secpercyl;		/* # of data sectors per cylinder */
	u_int32_t d_secperunit;		/* # of data sectors per unit */

	/*
	 * Spares (bad sector replacements) below are not counted in
	 * d_nsectors or d_secpercyl.  Spare sectors are assumed to
	 * be physical sectors which occupy space at the end of each
	 * track and/or cylinder.
	 */
	u_int16_t d_sparespertrack;	/* # of spare sectors per track */
	u_int16_t d_sparespercyl;	/* # of spare sectors per cylinder */
	/*
	 * Alternative cylinders include maintenance, replacement,
	 * configuration description areas, etc.
	 */
	u_int32_t d_acylinders;		/* # of alt. cylinders per unit */

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
	u_int16_t d_rpm;		/* rotational speed */
	u_int16_t d_interleave;		/* hardware sector interleave */
	u_int16_t d_trackskew;		/* sector 0 skew, per track */
	u_int16_t d_cylskew;		/* sector 0 skew, per cylinder */
	u_int32_t d_headswitch;		/* head switch time, usec */
	u_int32_t d_trkseek;		/* track-to-track seek, usec */
	u_int32_t d_flags;		/* generic flags */
#define	NDDATA 5
	u_int32_t d_drivedata[NDDATA];	/* drive-type specific information */
#define	NSPARE 5
	u_int32_t d_spare[NSPARE];	/* reserved for future use */
	u_int32_t d_magic2;		/* the magic number (again) */
	u_int16_t d_checksum;		/* xor of data incl. partitions */

			/* filesystem and partition information: */
	u_int16_t d_npartitions;	/* number of partitions in following */
	u_int32_t d_bbsize;		/* size of boot area at sn0, bytes */
	u_int32_t d_sbsize;		/* max size of fs superblock, bytes */
	struct	partition {		/* the partition table */
		u_int32_t p_size;	/* number of sectors in partition */
		u_int32_t p_offset;	/* starting sector */
		union {
			u_int32_t fsize; /* FFS, ADOS:
					    filesystem basic fragment size */
			u_int32_t cdsession; /* ISO9660: session offset */
		} __partition_u2;
#define	p_fsize		__partition_u2.fsize
#define	p_cdsession	__partition_u2.cdsession
		u_int8_t p_fstype;	/* filesystem type, see below */
		u_int8_t p_frag;	/* filesystem fragments per block */
		union {
			u_int16_t cpg;	/* UFS: FS cylinders per group */
			u_int16_t sgs;	/* LFS: FS segment shift */
		} __partition_u1;
#define	p_cpg	__partition_u1.cpg
#define	p_sgs	__partition_u1.sgs
	} d_partitions[MAXPARTITIONS];	/* actually may be more */
};

#ifdef __HAVE_OLD_DISKLABEL
/*
 * Same as above, but with OLDMAXPARTITIONS partitions. For use in
 * the old DIOC* ioctl calls.
 */
struct olddisklabel {
	u_int32_t d_magic;
	u_int16_t d_type;
	u_int16_t d_subtype;
	char	  d_typename[16];
	union {
		char	un_d_packname[16];
		struct {
			char *un_d_boot0;
			char *un_d_boot1;
		} un_b;
	} d_un;
	u_int32_t d_secsize;
	u_int32_t d_nsectors;
	u_int32_t d_ntracks;
	u_int32_t d_ncylinders;
	u_int32_t d_secpercyl;
	u_int32_t d_secperunit;
	u_int16_t d_sparespertrack;
	u_int16_t d_sparespercyl;
	u_int32_t d_acylinders;
	u_int16_t d_rpm;
	u_int16_t d_interleave;
	u_int16_t d_trackskew;
	u_int16_t d_cylskew;
	u_int32_t d_headswitch;
	u_int32_t d_trkseek;
	u_int32_t d_flags;
	u_int32_t d_drivedata[NDDATA];
	u_int32_t d_spare[NSPARE];
	u_int32_t d_magic2;
	u_int16_t d_checksum;
	u_int16_t d_npartitions;
	u_int32_t d_bbsize;
	u_int32_t d_sbsize;
	struct	opartition {
		u_int32_t p_size;
		u_int32_t p_offset;
		union {
			u_int32_t fsize;
			u_int32_t cdsession;
		} __partition_u2;
		u_int8_t p_fstype;
		u_int8_t p_frag;
		union {
			u_int16_t cpg;
			u_int16_t sgs;
		} __partition_u1;
	} d_partitions[OLDMAXPARTITIONS];
};
#endif /* __HAVE_OLD_DISKLABEL */
#else /* _LOCORE */
	/*
	 * offsets for asm boot files.
	 */
	.set	d_secsize,40
	.set	d_nsectors,44
	.set	d_ntracks,48
	.set	d_ncylinders,52
	.set	d_secpercyl,56
	.set	d_secperunit,60
	.set	d_end_,276		/* size of disk label */
#endif /* _LOCORE */

/* d_type values: */
#define	DTYPE_SMD		1		/* SMD, XSMD; VAX hp/up */
#define	DTYPE_MSCP		2		/* MSCP */
#define	DTYPE_DEC		3		/* other DEC (rk, rl) */
#define	DTYPE_SCSI		4		/* SCSI */
#define	DTYPE_ESDI		5		/* ESDI interface */
#define	DTYPE_ST506		6		/* ST506 etc. */
#define	DTYPE_HPIB		7		/* CS/80 on HP-IB */
#define	DTYPE_HPFL		8		/* HP Fiber-link */
#define	DTYPE_FLOPPY		10		/* floppy */
#define	DTYPE_CCD		11		/* concatenated disk device */
#define	DTYPE_VND		12		/* vnode pseudo-disk */
#define	DTYPE_ATAPI		13		/* ATAPI */
#define	DTYPE_RAID		14		/* RAIDframe */
#define	DTYPE_LD		15		/* logical disk */
#define	DTYPE_JFS2		16		/* IBM JFS2 */
#define	DTYPE_CGD		17		/* cryptographic pseudo-disk */
#define	DTYPE_VINUM		18		/* vinum volume */

#ifdef DKTYPENAMES
static const char *const dktypenames[] = {
	"unknown",
	"SMD",
	"MSCP",
	"old DEC",
	"SCSI",
	"ESDI",
	"ST506",
	"HP-IB",
	"HP-FL",
	"type 9",
	"floppy",
	"ccd",
	"vnd",
	"ATAPI",
	"RAID",
	"ld",
	"jfs",
	"cgd",
	"vinum",
	NULL
};
#define	DKMAXTYPES	(sizeof(dktypenames) / sizeof(dktypenames[0]) - 1)
#endif

/*
 * Filesystem type and version.
 * Used to interpret other filesystem-specific
 * per-partition information.
 *
 * These are used only for COMPAT_09 support.
 */
#define	FS_UNUSED	0		/* unused */
#define	FS_SWAP		1		/* swap */
#define	FS_V6		2		/* Sixth Edition */
#define	FS_V7		3		/* Seventh Edition */
#define	FS_SYSV		4		/* System V */
#define	FS_V71K		5		/* V7 with 1K blocks (4.1, 2.9) */
#define	FS_V8		6		/* Eighth Edition, 4K blocks */
#define	FS_BSDFFS	7		/* 4.2BSD fast file system */
#define	FS_MSDOS	8		/* MSDOS file system */
#define	FS_BSDLFS	9		/* 4.4BSD log-structured file system */
#define	FS_OTHER	10		/* in use, but unknown/unsupported */
#define	FS_HPFS		11		/* OS/2 high-performance file system */
#define	FS_ISO9660	12		/* ISO 9660, normally CD-ROM */
#define	FS_BOOT		13		/* partition contains bootstrap */
#define	FS_ADOS		14		/* AmigaDOS fast file system */
#define	FS_HFS		15		/* Macintosh HFS */
#define	FS_FILECORE	16		/* Acorn Filecore Filing System */
#define	FS_EX2FS	17		/* Linux Extended 2 file system */
#define	FS_NTFS		18		/* Windows/NT file system */
#define	FS_RAID		19		/* RAIDframe component */
#define	FS_CCD		20		/* concatenated disk component */
#define	FS_JFS2		21		/* IBM JFS2 */
#define	FS_APPLEUFS	22		/* Apple UFS */
/* XXX this is not the same as FreeBSD.  How to solve? */
#define	FS_VINUM	23		/* Vinum */

/* Adjust the FSMAXTYPES def below if you add something after APPLEUFS */

#ifdef	FSTYPENAMES
static const char *const fstypenames[] = {
	"unused",
	"swap",
	"Version 6",
	"Version 7",
	"System V",
	"4.1BSD",
	"Eighth Edition",
	"4.2BSD",
	"MSDOS",
	"4.4LFS",
	"unknown",
	"HPFS",
	"ISO9660",
	"boot",
	"ADOS",
	"HFS",
	"FILECORE",
	"Linux Ext2",
	"NTFS",
	"RAID",
	"ccd",
	"jfs",
	"Apple UFS",
	"vinum",
	NULL
};
#define	FSMAXTYPES	(sizeof(fstypenames) / sizeof(fstypenames[0]) - 1)
#else
#define	FSMAXTYPES	(FS_VINUM + 1)
#endif

#ifdef FSCKNAMES
/* These are the names MOUNT_XXX from <sys/mount.h> */
static const char *const fscknames[] = {
	NULL,		/* unused */
	NULL,		/* swap */
	NULL,		/* Version 6 */
	NULL,		/* Version 7 */
	NULL,		/* System V */
	NULL,		/* 4.1BSD */
	NULL,		/* Eighth edition */
	"ffs",		/* 4.2BSD */
	"msdos",	/* MSDOS */
	"lfs",		/* 4.4LFS */
	NULL,		/* unknown */
	NULL,		/* HPFS */
	NULL,		/* ISO9660 */
	NULL,		/* boot */
	NULL,		/* ADOS */
	NULL,		/* HFS */
	NULL,		/* FILECORE */
	"ext2fs",	/* Linux Ext2 */
	NULL,		/* Windows/NT */
	NULL,		/* RAID Component */
	NULL,		/* concatenated disk component */
	NULL,		/* IBM JFS2 */
	"ffs",		/* Apple UFS */
	NULL		/* NULL */
};
#define	FSMAXNAMES	(sizeof(fscknames) / sizeof(fscknames[0]) - 1)

#endif

#ifdef MOUNTNAMES
/* These are the names MOUNT_XXX from <sys/mount.h> */
static const char *const mountnames[] = {
	NULL,		/* unused */
	NULL,		/* swap */
	NULL,		/* Version 6 */
	NULL,		/* Version 7 */
	NULL,		/* System V */
	NULL,		/* 4.1BSD */
	NULL,		/* Eighth edition */
	"ffs",		/* 4.2BSD */
	"msdos",	/* MSDOS */
	"lfs",		/* 4.4LFS */
	NULL,		/* unknown */
	NULL,		/* HPFS */
	"cd9660",	/* ISO9660 */
	NULL,		/* boot */
	"ados",		/* ADOS */
	NULL,		/* HFS */
	"filecore",	/* FILECORE */
	"ext2fs",	/* Linux Ext2 */
	"ntfs",		/* Windows/NT */
	NULL,		/* RAID Component */
	NULL,		/* concatenated disk component */
	NULL,		/* IBM JFS2 */
	"ffs",		/* Apple UFS */
	NULL		/* NULL */
};
#define	FSMAXMOUNTNAMES	(sizeof(mountnames) / sizeof(mountnames[0]) - 1)

#endif

/*
 * flags shared by various drives:
 */
#define		D_REMOVABLE	0x01		/* removable media */
#define		D_ECC		0x02		/* supports ECC */
#define		D_BADSECT	0x04		/* supports bad sector forw. */
#define		D_RAMDISK	0x08		/* disk emulator */
#define		D_CHAIN		0x10		/* can do back-back transfers */

/*
 * Drive data for SMD.
 */
#define	d_smdflags	d_drivedata[0]
#define		D_SSE		0x1		/* supports skip sectoring */
#define	d_mindist	d_drivedata[1]
#define	d_maxdist	d_drivedata[2]
#define	d_sdist		d_drivedata[3]

/*
 * Drive data for ST506.
 */
#define	d_precompcyl	d_drivedata[0]
#define	d_gap3		d_drivedata[1]		/* used only when formatting */

/*
 * Drive data for SCSI.
 */
#define	d_blind		d_drivedata[0]

#ifndef _LOCORE
/*
 * Structure used to perform a format or other raw operation,
 * returning data and/or register values.  Register identification
 * and format are device- and driver-dependent. Currently unused.
 */
struct format_op {
	char	*df_buf;
	int	 df_count;		/* value-result */
	daddr_t	 df_startblk;
	int	 df_reg[8];		/* result */
};

/*
 * Structure used internally to retrieve information about a partition
 * on a disk.
 */
struct partinfo {
	struct disklabel *disklab;
	struct partition *part;
};

#ifdef _KERNEL

struct disk;

void	 diskerr __P((const struct buf *, const char *, const char *, int,
	    int, const struct disklabel *));
u_int	 dkcksum __P((struct disklabel *));
int	 setdisklabel __P((struct disklabel *, struct disklabel *, u_long,
	    struct cpu_disklabel *));
const char *readdisklabel __P((dev_t, void (*)(struct buf *),
	    struct disklabel *, struct cpu_disklabel *));
int	 writedisklabel __P((dev_t, void (*)(struct buf *), struct disklabel *,
	    struct cpu_disklabel *));
int	 bounds_check_with_label __P((struct disk *, struct buf *, int));
int	 bounds_check_with_mediasize __P((struct buf *, int, u_int64_t));
#endif
#endif /* _LOCORE */

#if !defined(_KERNEL) && !defined(_LOCORE)

#if 0 /* XXX ffsdrv */
#include <sys/cdefs.h>
#endif

#endif

#endif /* !_SYS_DISKLABEL_H_ */
