/*
 *  GRUB  --  GRand Unified Bootloader
 *  Copyright (C) 1999,2000,2001,2003   Free Software Foundation, Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef _PC_SLICE_H
#define _PC_SLICE_H

/*
 *  These define the basic PC MBR sector characteristics
 */

#define PC_MBR_SECTOR  0

#define PC_MBR_SIG_OFFSET  510
#define PC_MBR_SIGNATURE   0xaa55

#define PC_SLICE_OFFSET 446
#define PC_SLICE_MAX    4


/*
 *  Defines to guarantee structural alignment.
 */

#define PC_MBR_CHECK_SIG(mbr_ptr) \
  ( *( (unsigned short *) (((int) mbr_ptr) + PC_MBR_SIG_OFFSET) ) \
   == PC_MBR_SIGNATURE )

#define PC_MBR_SIG(mbr_ptr) \
  ( *( (unsigned short *) (((int) mbr_ptr) + PC_MBR_SIG_OFFSET) ) )

#define PC_SLICE_FLAG(mbr_ptr, part) \
  ( *( (unsigned char *) (((int) mbr_ptr) + PC_SLICE_OFFSET \
			  + (part << 4)) ) )

#define PC_SLICE_HEAD(mbr_ptr, part) \
  ( *( (unsigned char *) (((int) mbr_ptr) + PC_SLICE_OFFSET + 1 \
			  + (part << 4)) ) )

#define PC_SLICE_SEC(mbr_ptr, part) \
  ( *( (unsigned char *) (((int) mbr_ptr) + PC_SLICE_OFFSET + 2 \
			  + (part << 4)) ) )

#define PC_SLICE_CYL(mbr_ptr, part) \
  ( *( (unsigned char *) (((int) mbr_ptr) + PC_SLICE_OFFSET + 3 \
			  + (part << 4)) ) )

#define PC_SLICE_TYPE(mbr_ptr, part) \
  ( *( (unsigned char *) (((int) mbr_ptr) + PC_SLICE_OFFSET + 4 \
			  + (part << 4)) ) )

#define PC_SLICE_EHEAD(mbr_ptr, part) \
  ( *( (unsigned char *) (((int) mbr_ptr) + PC_SLICE_OFFSET + 5 \
			  + (part << 4)) ) )

#define PC_SLICE_ESEC(mbr_ptr, part) \
  ( *( (unsigned char *) (((int) mbr_ptr) + PC_SLICE_OFFSET + 6 \
			  + (part << 4)) ) )

#define PC_SLICE_ECYL(mbr_ptr, part) \
  ( *( (unsigned char *) (((int) mbr_ptr) + PC_SLICE_OFFSET + 7 \
			  + (part << 4)) ) )

#define PC_SLICE_START(mbr_ptr, part) \
  ( *( (unsigned long *) (((int) mbr_ptr) + PC_SLICE_OFFSET + 8 \
			  + (part << 4)) ) )

#define PC_SLICE_LENGTH(mbr_ptr, part) \
  ( *( (unsigned long *) (((int) mbr_ptr) + PC_SLICE_OFFSET + 12 \
			  + (part << 4)) ) )


/*
 *  PC flag types are defined here.
 */

#define PC_SLICE_FLAG_NONE      0
#define PC_SLICE_FLAG_BOOTABLE  0x80

/*
 *  Known PC partition types are defined here.
 */

/* This is not a flag actually, but used as if it were a flag.  */
#define PC_SLICE_TYPE_HIDDEN_FLAG	0x10

#define PC_SLICE_TYPE_NONE         	0
#define PC_SLICE_TYPE_FAT12        	1
#define PC_SLICE_TYPE_FAT16_LT32M  	4
#define PC_SLICE_TYPE_EXTENDED     	5
#define PC_SLICE_TYPE_FAT16_GT32M  	6
#define PC_SLICE_TYPE_FAT32		0xb
#define PC_SLICE_TYPE_FAT32_LBA		0xc
#define PC_SLICE_TYPE_FAT16_LBA		0xe
#define PC_SLICE_TYPE_WIN95_EXTENDED	0xf
#define PC_SLICE_TYPE_EZD        	0x55
#define PC_SLICE_TYPE_MINIX		0x80
#define PC_SLICE_TYPE_LINUX_MINIX	0x81
#define PC_SLICE_TYPE_EXT2FS       	0x83
#define PC_SLICE_TYPE_LINUX_EXTENDED	0x85
#define PC_SLICE_TYPE_VSTAFS		0x9e
#define PC_SLICE_TYPE_DELL_UTIL		0xde
#define PC_SLICE_TYPE_LINUX_RAID	0xfd


/* For convinience.  */
/* Check if TYPE is a FAT partition type. Clear the hidden flag before
   the check, to allow the user to mount a hidden partition in GRUB.  */
#define IS_PC_SLICE_TYPE_FAT(type)	\
  ({ int _type = (type) & ~PC_SLICE_TYPE_HIDDEN_FLAG; \
     _type == PC_SLICE_TYPE_FAT12 \
     || _type == PC_SLICE_TYPE_FAT16_LT32M \
     || _type == PC_SLICE_TYPE_FAT16_GT32M \
     || _type == PC_SLICE_TYPE_FAT16_LBA \
     || _type == PC_SLICE_TYPE_FAT32 \
     || _type == PC_SLICE_TYPE_FAT32_LBA \
     || _type == PC_SLICE_TYPE_DELL_UTIL; })

#define IS_PC_SLICE_TYPE_EXTENDED(type)	\
  (((type) == PC_SLICE_TYPE_EXTENDED)	\
   || ((type) == PC_SLICE_TYPE_WIN95_EXTENDED)	\
   || ((type) == PC_SLICE_TYPE_LINUX_EXTENDED))

#define IS_PC_SLICE_TYPE_MINIX(type) \
  (((type) == PC_SLICE_TYPE_MINIX)	\
   || ((type) == PC_SLICE_TYPE_LINUX_MINIX))

/* these ones are special, as they use their own partitioning scheme
   to subdivide the PC partitions from there.  */
#define PC_SLICE_TYPE_FREEBSD		0xa5
#define PC_SLICE_TYPE_OPENBSD		0xa6
#define PC_SLICE_TYPE_NETBSD		0xa9

/* For convenience.  */
#define IS_PC_SLICE_TYPE_BSD_WITH_FS(type,fs)	\
  ((type) == (PC_SLICE_TYPE_FREEBSD | ((fs) << 8)) \
   || (type) == (PC_SLICE_TYPE_OPENBSD | ((fs) << 8)) \
   || (type) == (PC_SLICE_TYPE_NETBSD | (fs) << 8))

#define IS_PC_SLICE_TYPE_BSD(type)	IS_PC_SLICE_TYPE_BSD_WITH_FS(type,0)

/*
 *  *BSD-style disklabel & partition definitions.
 *
 *  This is a subdivided slice of type 'PC_SLICE_TYPE_BSD', so all of
 *  these, except where noted, are relative to the slice in question.
 */

#define BSD_LABEL_SECTOR 1
#define BSD_LABEL_MAGIC  0x82564557

#define BSD_LABEL_MAG_OFFSET 0
#define BSD_LABEL_MAG2_OFFSET 132
#define BSD_LABEL_NPARTS_OFFSET 138
#define BSD_LABEL_NPARTS_MAX 8

#define BSD_PART_OFFSET 148


/*
 *  Defines to guarantee structural alignment.
 */

#define BSD_LABEL_CHECK_MAG(l_ptr) \
  ( *( (unsigned long *) (((int) l_ptr) + BSD_LABEL_MAG_OFFSET) ) \
   == ( (unsigned long) BSD_LABEL_MAGIC ) )

#define BSD_LABEL_MAG(l_ptr) \
  ( *( (unsigned long *) (((int) l_ptr) + BSD_LABEL_MAG_OFFSET) ) )

#define BSD_LABEL_DTYPE(l_ptr) \
  ( *( (unsigned short *) (((int) l_ptr) + BSD_LABEL_MAG_OFFSET + 4) ) )

#define BSD_LABEL_NPARTS(l_ptr) \
  ( *( (unsigned short *) (((int) l_ptr) + BSD_LABEL_NPARTS_OFFSET) ) )

#define BSD_PART_LENGTH(l_ptr, part) \
  ( *( (unsigned long *) (((int) l_ptr) + BSD_PART_OFFSET \
			  + (part << 4)) ) )

#define BSD_PART_START(l_ptr, part) \
  ( *( (unsigned long *) (((int) l_ptr) + BSD_PART_OFFSET + 4 \
			  + (part << 4)) ) )

#define BSD_PART_FRAG_SIZE(l_ptr, part) \
  ( *( (unsigned long *) (((int) l_ptr) + BSD_PART_OFFSET + 8 \
			  + (part << 4)) ) )

#define BSD_PART_TYPE(l_ptr, part) \
  ( *( (unsigned char *) (((int) l_ptr) + BSD_PART_OFFSET + 12 \
			  + (part << 4)) ) )

#define BSD_PART_FRAGS_PER_BLOCK(l_ptr, part) \
  ( *( (unsigned char *) (((int) l_ptr) + BSD_PART_OFFSET + 13 \
			  + (part << 4)) ) )

#define BSD_PART_EXTRA(l_ptr, part) \
  ( *( (unsigned short *) (((int) l_ptr) + BSD_PART_OFFSET + 14 \
			  + (part << 4)) ) )


/* possible values for the "DISKTYPE"... all essentially irrelevant
   except for DTYPE_SCSI */
#define DTYPE_SMD               1	/* SMD, XSMD; VAX hp/up */
#define DTYPE_MSCP              2	/* MSCP */
#define DTYPE_DEC               3	/* other DEC (rk, rl) */
#define DTYPE_SCSI              4	/* SCSI */
#define DTYPE_ESDI              5	/* ESDI interface */
#define DTYPE_ST506             6	/* ST506 etc. */
#define DTYPE_HPIB              7	/* CS/80 on HP-IB */
#define DTYPE_HPFL              8	/* HP Fiber-link */
#define DTYPE_FLOPPY            10	/* floppy */


/* possible values for the *BSD-style partition type */
#define	FS_UNUSED	0	/* unused */
#define	FS_SWAP		1	/* swap */
#define	FS_V6		2	/* Sixth Edition */
#define	FS_V7		3	/* Seventh Edition */
#define	FS_SYSV		4	/* System V */
#define	FS_V71K		5	/* V7 with 1K blocks (4.1, 2.9) */
#define	FS_V8		6	/* Eighth Edition, 4K blocks */
#define	FS_BSDFFS	7	/* 4.2BSD fast file system */
#define	FS_MSDOS	8	/* MSDOS file system */
#define	FS_BSDLFS	9	/* 4.4BSD log-structured file system */
#define	FS_OTHER	10	/* in use, but unknown/unsupported */
#define	FS_HPFS		11	/* OS/2 high-performance file system */
#define	FS_ISO9660	12	/* ISO 9660, normally CD-ROM */
#define	FS_BOOT		13	/* partition contains bootstrap */
#define	FS_ADOS		14	/* AmigaDOS fast file system */
#define	FS_HFS		15	/* Macintosh HFS */
#define	FS_FILECORE	16	/* Acorn Filecore Filing System */
#define	FS_EXT2FS	17	/* Linux Extended 2 file system */


#endif /* _PC_SLICE_H */
