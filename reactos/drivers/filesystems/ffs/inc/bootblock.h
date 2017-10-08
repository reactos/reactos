/*	$NetBSD: bootblock.h,v 1.24 2004/03/22 07:11:00 lukem Exp $	*/

/*-
 * Copyright (c) 2002-2004 The NetBSD Foundation, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the NetBSD
 *	Foundation, Inc. and its contributors.
 * 4. Neither the name of The NetBSD Foundation nor the names of its
 *    contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE NETBSD FOUNDATION, INC. AND CONTRIBUTORS
 * ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
/*-
 * Copyright (C) 1993	Allen K. Briggs, Chris P. Caputo,
 *			Michael L. Finch, Bradley A. Grantham, and
 *			Lawrence A. Kesteloot
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Alice Group.
 * 4. The names of the Alice Group or any of its members may not be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE ALICE GROUP ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE ALICE GROUP BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */
/*
 * Copyright (c) 1994, 1999 Christopher G. Demetriou
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Christopher G. Demetriou
 *      for the NetBSD Project.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
/*
 * Copyright (c) 1994 Rolf Grossmann
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by Rolf Grossmann.
 * 4. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _SYS_BOOTBLOCK_H
#define	_SYS_BOOTBLOCK_H

#include "type.h"

#if 0 /* XXX ffsdrv */
#if !defined(__ASSEMBLER__)
#if defined(_KERNEL) || defined(_STANDALONE)
#include <sys/stdint.h>
#else
#include <stdint.h>
#endif
#endif	/* !defined(__ASSEMBLER__) */
#endif

/* ------------------------------------------
 * MBR (Master Boot Record) --
 *	definitions for systems that use MBRs
 */

/*
 * Layout of boot records:
 *
 *	Byte range	Use	Description
 *	----------	---	-----------
 *
 *	0 - 2		FMP	JMP xxx, NOP
 *	3 - 10		FP	OEM Name
 *
 *	11 - 61		FMP	FAT12/16 BPB
 *				Whilst not strictly necessary for MBR,
 *				GRUB reserves this area
 *
 *	11 - 89		P	FAT32 BPB
 *				(are we ever going to boot off this?)
 *
 *
 *	62 - 217	FMP	Boot code
 *
 *	90 - 217	P	FAT32 boot code
 *
 *	218 - 223	M	Win95b/98/me "drive time"
 *		http://www.geocities.com/thestarman3/asm/mbr/95BMEMBR.htm#MYST
 *				only changed if all 6 bytes are 0
 *
 *	224 - 436	FMP	boot code (continued)
 *
 *	437 - 439	M	WinNT/2K/XP MBR "boot language"
 *		http://www.geocities.com/thestarman3/asm/mbr/Win2kmbr.htm
 *				not needed by us
 *
 *	400 - 439	MP	NetBSD: mbr_bootsel
 *
 *	440 - 443	M	WinNT/2K/XP Drive Serial Number (NT DSN)
 *		http://www.geocities.com/thestarman3/asm/mbr/Win2kmbr.htm
 *
 *	444 - 445	FMP	bootcode or unused
 *				NetBSD: mbr_bootsel_magic
 *
 *	446 - 509	M	partition table
 *
 *	510 - 511	FMP	magic number (0xAA55)
 *
 *	Use:
 *	----
 *	F	Floppy boot sector
 *	M	Master Boot Record
 *	P	Partition Boot record
 *
 */

/*
 * MBR (Master Boot Record)
 */
#define	MBR_BBSECTOR		0	/* MBR relative sector # */
#define	MBR_BPB_OFFSET		11	/* offsetof(mbr_sector, mbr_bpb) */
#define	MBR_BOOTCODE_OFFSET	90	/* offsetof(mbr_sector, mbr_bootcode) */
#define	MBR_BS_OFFSET		400	/* offsetof(mbr_sector, mbr_bootsel) */
#define	MBR_DSN_OFFSET		440	/* offsetof(mbr_sector, mbr_dsn) */
#define	MBR_BS_MAGIC_OFFSET	444	/* offsetof(mbr_sector, mbr_bootsel_magic) */
#define	MBR_PART_OFFSET		446	/* offsetof(mbr_sector, mbr_part[0]) */
#define	MBR_MAGIC_OFFSET	510	/* offsetof(mbr_sector, mbr_magic) */
#define	MBR_MAGIC		0xaa55	/* MBR magic number */
#define	MBR_BS_MAGIC		0xb5e1	/* mbr_bootsel magic number */
#define	MBR_PART_COUNT		4	/* Number of partitions in MBR */
#define	MBR_BS_PARTNAMESIZE	8	/* Size of name mbr_bootsel nametab */
					/* (excluding trailing NUL) */

		/* values for mbr_partition.mbrp_flag */
#define	MBR_PFLAG_ACTIVE	0x80	/* The active partition */

		/* values for mbr_partition.mbrp_type */
#define	MBR_PTYPE_FAT12		0x01	/* 12-bit FAT */
#define	MBR_PTYPE_FAT16S	0x04	/* 16-bit FAT, less than 32M */
#define	MBR_PTYPE_EXT		0x05	/* extended partition */
#define	MBR_PTYPE_FAT16B	0x06	/* 16-bit FAT, more than 32M */
#define	MBR_PTYPE_NTFS		0x07	/* OS/2 HPFS, NTFS, QNX2, Adv. UNIX */
#define	MBR_PTYPE_FAT32		0x0b	/* 32-bit FAT */
#define	MBR_PTYPE_FAT32L	0x0c	/* 32-bit FAT, LBA-mapped */
#define	MBR_PTYPE_FAT16L	0x0e	/* 16-bit FAT, LBA-mapped */
#define	MBR_PTYPE_EXT_LBA	0x0f	/* extended partition, LBA-mapped */
#define	MBR_PTYPE_ONTRACK	0x54
#define	MBR_PTYPE_LNXSWAP	0x82	/* Linux swap or Solaris */
#define	MBR_PTYPE_LNXEXT2	0x83	/* Linux native */
#define	MBR_PTYPE_EXT_LNX	0x85	/* Linux extended partition */
#define	MBR_PTYPE_NTFSVOL	0x87	/* NTFS volume set or HPFS mirrored */
#define	MBR_PTYPE_PREP		0x41	/* PReP */
#define	MBR_PTYPE_386BSD	0xa5	/* 386BSD partition type */
#define	MBR_PTYPE_APPLEUFS 	0xa8	/* Apple UFS */
#define	MBR_PTYPE_NETBSD	0xa9	/* NetBSD partition type */
#define	MBR_PTYPE_OPENBSD	0xa6	/* OpenBSD partition type */

#define	MBR_PSECT(s)		((s) & 0x3f)
#define	MBR_PCYL(c, s)		((c) + (((s) & 0xc0) << 2))

#define	MBR_IS_EXTENDED(x)	((x) == MBR_PTYPE_EXT || \
				 (x) == MBR_PTYPE_EXT_LBA || \
				 (x) == MBR_PTYPE_EXT_LNX)

		/* values for mbr_bootsel.mbrbs_flags */
#define	MBR_BS_ACTIVE	0x01	/* Bootselector active (or code present) */
#define	MBR_BS_EXTINT13	0x02	/* Set by fdisk if LBA needed (deprecated) */
#define	MBR_BS_READ_LBA	0x04	/* Force LBA reads - even for low numbers */
#define	MBR_BS_EXTLBA	0x08	/* Extended ptn capable (LBA reads) */
#define	MBR_BS_NEWMBR	0x80	/* New bootsel at offset 440 */

#if !defined(__ASSEMBLER__)					/* { */

#pragma pack(1)

/*
 * (x86) BIOS Parameter Block for FAT12
 */
struct mbr_bpbFAT12 {
	uint16_t	bpbBytesPerSec;	/* bytes per sector */
	uint8_t		bpbSecPerClust;	/* sectors per cluster */
	uint16_t	bpbResSectors;	/* number of reserved sectors */
	uint8_t		bpbFATs;	/* number of FATs */
	uint16_t	bpbRootDirEnts;	/* number of root directory entries */
	uint16_t	bpbSectors;	/* total number of sectors */
	uint8_t		bpbMedia;	/* media descriptor */
	uint16_t	bpbFATsecs;	/* number of sectors per FAT */
	uint16_t	bpbSecPerTrack;	/* sectors per track */
	uint16_t	bpbHeads;	/* number of heads */
	uint16_t	bpbHiddenSecs;	/* # of hidden sectors */
}; /* __attribute__((__packed__)); */

/*
 * (x86) BIOS Parameter Block for FAT16
 */
struct mbr_bpbFAT16 {
	uint16_t	bpbBytesPerSec;	/* bytes per sector */
	uint8_t		bpbSecPerClust;	/* sectors per cluster */
	uint16_t	bpbResSectors;	/* number of reserved sectors */
	uint8_t		bpbFATs;	/* number of FATs */
	uint16_t	bpbRootDirEnts;	/* number of root directory entries */
	uint16_t	bpbSectors;	/* total number of sectors */
	uint8_t		bpbMedia;	/* media descriptor */
	uint16_t	bpbFATsecs;	/* number of sectors per FAT */
	uint16_t	bpbSecPerTrack;	/* sectors per track */
	uint16_t	bpbHeads;	/* number of heads */
	uint32_t	bpbHiddenSecs;	/* # of hidden sectors */
	uint32_t	bpbHugeSectors;	/* # of sectors if bpbSectors == 0 */
	uint8_t		bsDrvNum;	/* Int 0x13 drive number (e.g. 0x80) */
	uint8_t		bsReserved1;	/* Reserved; set to 0 */
	uint8_t		bsBootSig;	/* 0x29 if next 3 fields are present */
	uint8_t		bsVolID[4];	/* Volume serial number */
	uint8_t		bsVolLab[11];	/* Volume label */
	uint8_t		bsFileSysType[8];
					/* "FAT12   ", "FAT16   ", "FAT     " */
}; /* __attribute__((__packed__)); */

/*
 * (x86) BIOS Parameter Block for FAT32
 */
struct mbr_bpbFAT32 {
	uint16_t	bpbBytesPerSec;	/* bytes per sector */
	uint8_t		bpbSecPerClust;	/* sectors per cluster */
	uint16_t	bpbResSectors;	/* number of reserved sectors */
	uint8_t		bpbFATs;	/* number of FATs */
	uint16_t	bpbRootDirEnts;	/* number of root directory entries */
	uint16_t	bpbSectors;	/* total number of sectors */
	uint8_t		bpbMedia;	/* media descriptor */
	uint16_t	bpbFATsecs;	/* number of sectors per FAT */
	uint16_t	bpbSecPerTrack;	/* sectors per track */
	uint16_t	bpbHeads;	/* number of heads */
	uint32_t	bpbHiddenSecs;	/* # of hidden sectors */
	uint32_t	bpbHugeSectors;	/* # of sectors if bpbSectors == 0 */
	uint32_t	bpbBigFATsecs;	/* like bpbFATsecs for FAT32 */
	uint16_t	bpbExtFlags;	/* extended flags: */
#define	MBR_FAT32_FATNUM	0x0F	/*   mask for numbering active FAT */
#define	MBR_FAT32_FATMIRROR	0x80	/*   FAT is mirrored (as previously) */
	uint16_t	bpbFSVers;	/* filesystem version */
#define	MBR_FAT32_FSVERS	0	/*   currently only 0 is understood */
	uint32_t	bpbRootClust;	/* start cluster for root directory */
	uint16_t	bpbFSInfo;	/* filesystem info structure sector */
	uint16_t	bpbBackup;	/* backup boot sector */
	uint8_t		bsReserved[12];	/* Reserved for future expansion */
	uint8_t		bsDrvNum;	/* Int 0x13 drive number (e.g. 0x80) */
	uint8_t		bsReserved1;	/* Reserved; set to 0 */
	uint8_t		bsBootSig;	/* 0x29 if next 3 fields are present */
	uint8_t		bsVolID[4];	/* Volume serial number */
	uint8_t		bsVolLab[11];	/* Volume label */
	uint8_t		bsFileSysType[8]; /* "FAT32   " */
}; /* __attribute__((__packed__)); */

/*
 * (x86) MBR boot selector
 */
struct mbr_bootsel {
	uint8_t		mbrbs_defkey;
	uint8_t		mbrbs_flags;
	uint16_t	mbrbs_timeo;
	uint8_t		mbrbs_nametab[MBR_PART_COUNT][MBR_BS_PARTNAMESIZE + 1];
}; /* __attribute__((__packed__)); */

/*
 * MBR partition
 */
struct mbr_partition {
	uint8_t		mbrp_flag;	/* MBR partition flags */
	uint8_t		mbrp_shd;	/* Starting head */
	uint8_t		mbrp_ssect;	/* Starting sector */
	uint8_t		mbrp_scyl;	/* Starting cylinder */
	uint8_t		mbrp_type;	/* Partition type (see below) */
	uint8_t		mbrp_ehd;	/* End head */
	uint8_t		mbrp_esect;	/* End sector */
	uint8_t		mbrp_ecyl;	/* End cylinder */
	uint32_t	mbrp_start;	/* Absolute starting sector number */
	uint32_t	mbrp_size;	/* Partition size in sectors */
}; /* __attribute__((__packed__)); */

int xlat_mbr_fstype(int);	/* in sys/lib/libkern/xlat_mbr_fstype.c */

/*
 * MBR boot sector.
 * This is used by both the MBR (Master Boot Record) in sector 0 of the disk
 * and the PBR (Partition Boot Record) in sector 0 of an MBR partition.
 */
struct mbr_sector {
					/* Jump instruction to boot code.  */
					/* Usually 0xE9nnnn or 0xEBnn90 */
	uint8_t			mbr_jmpboot[3];	
					/* OEM name and version */
	uint8_t			mbr_oemname[8];	
	union {				/* BIOS Parameter Block */
		struct mbr_bpbFAT12	bpb12;
		struct mbr_bpbFAT16	bpb16;
		struct mbr_bpbFAT32	bpb32;
	} mbr_bpb;
					/* Boot code */
	uint8_t			mbr_bootcode[310];
					/* Config for /usr/mdec/mbr_bootsel */
	struct mbr_bootsel	mbr_bootsel;
					/* NT Drive Serial Number */
	uint32_t		mbr_dsn;
					/* mbr_bootsel magic */
	uint16_t		mbr_bootsel_magic;
					/* MBR partition table */
	struct mbr_partition	mbr_parts[MBR_PART_COUNT];
					/* MBR magic (0xaa55) */
	uint16_t		mbr_magic;
}; /* __attribute__((__packed__)); */

#endif	/* !defined(__ASSEMBLER__) */				/* } */

#pragma pack()


/* ------------------------------------------
 * shared --
 *	definitions shared by many platforms
 */

#if !defined(__ASSEMBLER__)					/* { */

	/* Maximum # of blocks in bbi_block_table, each bbi_block_size long */
#define	SHARED_BBINFO_MAXBLOCKS	118	/* so sizeof(shared_bbinfo) == 512 */

struct shared_bbinfo {
	uint8_t bbi_magic[32];
	int32_t bbi_block_size;
	int32_t bbi_block_count;
	int32_t bbi_block_table[SHARED_BBINFO_MAXBLOCKS];
};

#if 0 /* XXX ffsdrv */

/* ------------------------------------------
 * alpha --
 *	Alpha (disk, but also tape) Boot Block.
 *
 *	See Section (III) 3.6.1 of the Alpha Architecture Reference Manual.
 */

struct alpha_boot_block {
	uint64_t bb_data[63];		/* data (disklabel, also as below) */
	uint64_t bb_cksum;		/* checksum of the boot block,
					 * taken as uint64_t's
					 */
};
#define	bb_secsize	bb_data[60]	/* secondary size (blocks) */
#define	bb_secstart	bb_data[61]	/* secondary start (blocks) */
#define	bb_flags	bb_data[62]	/* unknown flags (set to zero) */

#define	ALPHA_BOOT_BLOCK_OFFSET		0	/* offset of boot block. */
#define	ALPHA_BOOT_BLOCK_BLOCKSIZE	512	/* block size for sector
						 * size/start, and for boot
						 * block itself.
						 */

#define	ALPHA_BOOT_BLOCK_CKSUM(bb,cksum)				\
	do {								\
		const struct alpha_boot_block *_bb = (bb);		\
		uint64_t _cksum;					\
		int _i;							\
									\
		_cksum = 0;						\
		for (_i = 0;						\
		    _i < (sizeof _bb->bb_data / sizeof _bb->bb_data[0]); \
		    _i++)						\
			_cksum += _bb->bb_data[_i];			\
		*(cksum) = _cksum;					\
	} while (/*CONSTCOND*/ 0)

/* ------------------------------------------
 * apple --
 *	Apple computers boot block related information
 */

/*
 *	Driver Descriptor Map, from Inside Macintosh: Devices, SCSI Manager
 *	pp 12-13.  The driver descriptor map always resides on physical block 0.
 */
struct apple_drvr_descriptor {
	uint32_t	descBlock;	/* first block of driver */
	uint16_t	descSize;	/* driver size in blocks */
	uint16_t	descType;	/* system type */
};

/*
 *	system types; Apple reserves 0-15
 */
#define	APPLE_DRVR_TYPE_MACINTOSH	1

#define	APPLE_DRVR_MAP_MAGIC		0x4552
#define	APPLE_DRVR_MAP_MAX_DESCRIPTORS	61

struct apple_drvr_map {
	uint16_t	sbSig;		/* map signature */
	uint16_t	sbBlockSize;	/* block size of device */
	uint32_t	sbBlkCount;	/* number of blocks on device */
	uint16_t	sbDevType;	/* (used internally by ROM) */
	uint16_t	sbDevID;	/* (used internally by ROM) */
	uint32_t	sbData;		/* (used internally by ROM) */
	uint16_t	sbDrvrCount;	/* number of driver descriptors */
	struct apple_drvr_descriptor sb_dd[APPLE_DRVR_MAP_MAX_DESCRIPTORS];
	uint16_t	pad[3];
} __attribute__((__packed__));

/*
 *	Partition map structure from Inside Macintosh: Devices, SCSI Manager
 *	pp. 13-14.  The partition map always begins on physical block 1.
 *
 *	With the exception of block 0, all blocks on the disk must belong to
 *	exactly one partition.  The partition map itself belongs to a partition
 *	of type `APPLE_PARTITION_MAP', and is not limited in size by anything
 *	other than available disk space.  The partition map is not necessarily
 *	the first partition listed.
 */
#define	APPLE_PART_MAP_ENTRY_MAGIC	0x504d

struct apple_part_map_entry {
	uint16_t	pmSig;		/* partition signature */
	uint16_t	pmSigPad;	/* (reserved) */
	uint32_t	pmMapBlkCnt;	/* number of blocks in partition map */
	uint32_t	pmPyPartStart;	/* first physical block of partition */
	uint32_t	pmPartBlkCnt;	/* number of blocks in partition */
	uint8_t		pmPartName[32];	/* partition name */
	uint8_t		pmPartType[32];	/* partition type */
	uint32_t	pmLgDataStart;	/* first logical block of data area */
	uint32_t	pmDataCnt;	/* number of blocks in data area */
	uint32_t	pmPartStatus;	/* partition status information */
	uint32_t	pmLgBootStart;	/* first logical block of boot code */
	uint32_t	pmBootSize;	/* size of boot code, in bytes */
	uint32_t	pmBootLoad;	/* boot code load address */
	uint32_t	pmBootLoad2;	/* (reserved) */
	uint32_t	pmBootEntry;	/* boot code entry point */
	uint32_t	pmBootEntry2;	/* (reserved) */
	uint32_t	pmBootCksum;	/* boot code checksum */
	int8_t		pmProcessor[16]; /* processor type (e.g. "68020") */
	uint8_t		pmBootArgs[128]; /* A/UX boot arguments */
	uint8_t		pad[248];	/* pad to end of block */
};

#define	APPLE_PART_TYPE_DRIVER		"APPLE_DRIVER"
#define	APPLE_PART_TYPE_DRIVER43	"APPLE_DRIVER43"
#define	APPLE_PART_TYPE_DRIVERATA	"APPLE_DRIVER_ATA"
#define	APPLE_PART_TYPE_DRIVERIOKIT	"APPLE_DRIVER_IOKIT"
#define	APPLE_PART_TYPE_FWDRIVER	"APPLE_FWDRIVER"
#define	APPLE_PART_TYPE_FWB_COMPONENT	"FWB DRIVER COMPONENTS"
#define	APPLE_PART_TYPE_FREE		"APPLE_FREE"
#define	APPLE_PART_TYPE_MAC		"APPLE_HFS"
#define	APPLE_PART_TYPE_NETBSD		"NETBSD"
#define	APPLE_PART_TYPE_NBSD_PPCBOOT	"NETBSD/MACPPC"
#define	APPLE_PART_TYPE_NBSD_68KBOOT	"NETBSD/MAC68K"
#define	APPLE_PART_TYPE_PATCHES		"APPLE_PATCHES"
#define	APPLE_PART_TYPE_PARTMAP		"APPLE_PARTITION_MAP"
#define	APPLE_PART_TYPE_PATCHES		"APPLE_PATCHES"
#define	APPLE_PART_TYPE_SCRATCH		"APPLE_SCRATCH"
#define	APPLE_PART_TYPE_UNIX		"APPLE_UNIX_SVR2"

/*
 * "pmBootArgs" for APPLE_UNIX_SVR2 partition.
 * NetBSD/mac68k only uses Magic, Cluster, Type, and Flags.
 */
struct apple_blockzeroblock {
	uint32_t       bzbMagic;
	uint8_t        bzbCluster;
	uint8_t        bzbType;
	uint16_t       bzbBadBlockInode;
	uint16_t       bzbFlags;
	uint16_t       bzbReserved;
	uint32_t       bzbCreationTime;
	uint32_t       bzbMountTime;
	uint32_t       bzbUMountTime;
};

#define	APPLE_BZB_MAGIC		0xABADBABE
#define	APPLE_BZB_TYPEFS	1
#define	APPLE_BZB_TYPESWAP	3
#define	APPLE_BZB_ROOTFS	0x8000
#define	APPLE_BZB_USRFS		0x4000

/* ------------------------------------------
 * hp300
 *
 */

/* volume header for "LIF" format volumes */

struct	hp300_lifvol {
	int16_t	vol_id;
	char	vol_label[6];
	int32_t	vol_addr;
	int16_t	vol_oct;
	int16_t	vol_dummy;
	int32_t	vol_dirsize;
	int16_t	vol_version;
	int16_t	vol_zero;
	int32_t	vol_huh1;
	int32_t	vol_huh2;
	int32_t	vol_length;
};

/* LIF directory entry format */

struct	hp300_lifdir {
	char	dir_name[10];
	int16_t	dir_type;
	int32_t	dir_addr;
	int32_t	dir_length;
	char	dir_toc[6];
	int16_t	dir_flag;
	int32_t	dir_exec;
};

/* load header for boot rom */
struct hp300_load {
	int32_t address;
	int32_t count;
};

#define	HP300_VOL_ID		-32768
#define	HP300_VOL_OCT		4096
#define	HP300_DIR_TYPE		-5822
#define	HP300_DIR_FLAG		0x8001	/* dont ask me! */
#define	HP300_SECTSIZE		256

#endif

/* ------------------------------------------
 * x86
 *
 */

/*
 * Parameters for NetBSD /boot written to start of pbr code by installboot
 */

struct x86_boot_params {
	uint32_t	bp_length;	/* length of patchable data */
	uint32_t	bp_flags;
	uint32_t	bp_timeout;	/* boot timeout in seconds */
	uint32_t	bp_consdev;
	uint32_t	bp_conspeed;
	uint8_t		bp_password[16];	/* md5 hash of password */
	char		bp_keymap[16];	/* keyboard traslation map */
};

#endif	/* !defined(__ASSEMBLER__) */				/* } */

#define	X86_BOOT_MAGIC(n)	('x' << 24 | 0x86b << 12 | 'm' << 4 | (n))
#define	X86_BOOT_MAGIC_1	X86_BOOT_MAGIC(1)	/* pbr.S */
#define	X86_BOOT_MAGIC_2	X86_BOOT_MAGIC(2)	/* bootxx.S */
#define	X86_BOOT_MAGIC_PXE	X86_BOOT_MAGIC(3)	/* start_pxe.S */

		/* values for bp_flags */
#define	X86_BP_FLAGS_RESET_VIDEO	1
#define	X86_BP_FLAGS_PASSWORD		2

		/* values for bp_consdev */
#define	X86_BP_CONSDEV_PC	0
#define	X86_BP_CONSDEV_COM0	1
#define	X86_BP_CONSDEV_COM1	2
#define	X86_BP_CONSDEV_COM2	3
#define	X86_BP_CONSDEV_COM3	4
#define	X86_BP_CONSDEV_COM0KBD	5
#define	X86_BP_CONSDEV_COM1KBD	6
#define	X86_BP_CONSDEV_COM2KBD	7
#define	X86_BP_CONSDEV_COM3KBD	8

#if !defined(__ASSEMBLER__)					/* { */

#if 0 /* XXX ffsdrv */
/* ------------------------------------------
 * macppc
 */

#define	MACPPC_BOOT_BLOCK_OFFSET	2048
#define	MACPPC_BOOT_BLOCK_BLOCKSIZE	512
#define	MACPPC_BOOT_BLOCK_MAX_SIZE	2048	/* XXX: could be up to 6144 */
	/* Magic string -- 32 bytes long (including the NUL) */
#define	MACPPC_BBINFO_MAGIC		"NetBSD/macppc bootxx   20020515"

/* ------------------------------------------
 * news68k, newsmips
 */

#define	NEWS_BOOT_BLOCK_LABELOFFSET	64 /* XXX from <machine/disklabel.h> */
#define	NEWS_BOOT_BLOCK_OFFSET		0
#define	NEWS_BOOT_BLOCK_BLOCKSIZE	512
#define	NEWS_BOOT_BLOCK_MAX_SIZE	(512 * 16)

	/* Magic string -- 32 bytes long (including the NUL) */
#define	NEWS68K_BBINFO_MAGIC		"NetBSD/news68k bootxx  20020518"
#define	NEWSMIPS_BBINFO_MAGIC		"NetBSD/newsmips bootxx 20020518"

/* ------------------------------------------
 * next68k
 */

#define	NEXT68K_LABEL_MAXPARTITIONS	8	/* number of partitions in next68k_disklabel */
#define	NEXT68K_LABEL_CPULBLLEN		24
#define	NEXT68K_LABEL_MAXDNMLEN		24
#define	NEXT68K_LABEL_MAXTYPLEN		24
#define	NEXT68K_LABEL_MAXBFLEN		24
#define	NEXT68K_LABEL_MAXHNLEN		32
#define	NEXT68K_LABEL_MAXMPTLEN		16
#define	NEXT68K_LABEL_MAXFSTLEN		8
#define	NEXT68K_LABEL_NBAD		1670	/* sized to make label ~= 8KB */

struct next68k_partition {
	int32_t	cp_offset;		/* starting sector */
	int32_t	cp_size;		/* number of sectors in partition */
	int16_t	cp_bsize;		/* block size in bytes */
	int16_t	cp_fsize;		/* filesystem basic fragment size */
	char	cp_opt;			/* optimization type: 's'pace/'t'ime */
	char	cp_pad1;
	int16_t	cp_cpg;			/* filesystem cylinders per group */
	int16_t	cp_density;		/* bytes per inode density */
	int8_t	cp_minfree;		/* minfree (%) */
	int8_t	cp_newfs;		/* run newfs during init */
	char	cp_mountpt[NEXT68K_LABEL_MAXMPTLEN];
					/* default/standard mount point */
	int8_t	cp_automnt;		/* auto-mount when inserted */
	char	cp_type[NEXT68K_LABEL_MAXFSTLEN]; /* file system type name */
	char	cp_pad2;
} __attribute__ ((packed));

/* The disklabel the way it is on the disk */
struct next68k_disklabel {
	int32_t	cd_version;		/* label version */
	int32_t	cd_label_blkno;		/* block # of this label */
	int32_t	cd_size;		/* size of media area (sectors) */
	char	cd_label[NEXT68K_LABEL_CPULBLLEN]; /* disk name (label) */
	uint32_t cd_flags;		/* flags */
	uint32_t cd_tag;		/* volume tag */
	char	cd_name[NEXT68K_LABEL_MAXDNMLEN]; /* drive (hardware) name */
	char	cd_type[NEXT68K_LABEL_MAXTYPLEN]; /* drive type */
	int32_t	cd_secsize;		/* # of bytes per sector */
	int32_t	cd_ntracks;		/* # of tracks per cylinder */
	int32_t	cd_nsectors;		/* # of data sectors per track */
	int32_t	cd_ncylinders;		/* # of data cylinders per unit */
	int32_t	cd_rpm;			/* rotational speed */
	int16_t	cd_front;		/* # of sectors in "front porch" */
	int16_t	cd_back;		/* # of sectors in "back porch" */
	int16_t	cd_ngroups;		/* # of alt groups */
	int16_t	cd_ag_size;		/* alt group size (sectors) */
	int16_t	cd_ag_alts;		/* alternate sectors / alt group */
	int16_t	cd_ag_off;		/* sector offset to first alternate */
	int32_t	cd_boot_blkno[2];	/* boot program locations */
	char	cd_kernel[NEXT68K_LABEL_MAXBFLEN]; /* default kernel name */
	char	cd_hostname[NEXT68K_LABEL_MAXHNLEN];
				/* host name (usu. where disk was labeled) */
	char	cd_rootpartition;	/* root partition letter e.g. 'a' */
	char	cd_rwpartition;		/* r/w partition letter e.g. 'b' */
	struct next68k_partition cd_partitions[NEXT68K_LABEL_MAXPARTITIONS];

	union {
		uint16_t CD_v3_checksum; /* label version 3 checksum */
		int32_t	CD_bad[NEXT68K_LABEL_NBAD];
					/* block number that is bad */
	} cd_un;
	uint16_t cd_checksum;		/* label version 1 or 2 checksum */
} __attribute__ ((packed));

#define	NEXT68K_LABEL_cd_checksum	cd_checksum
#define	NEXT68K_LABEL_cd_v3_checksum	cd_un.CD_v3_checksum
#define	NEXT68K_LABEL_cd_bad		cd_un.CD_bad

#define	NEXT68K_LABEL_SECTOR		0	/* sector containing label */
#define	NEXT68K_LABEL_OFFSET		0	/* offset of label in sector */
#define	NEXT68K_LABEL_SIZE		8192	/* size of label */
#define	NEXT68K_LABEL_CD_V1		0x4e655854 /* version #1: "NeXT" */
#define	NEXT68K_LABEL_CD_V2		0x646c5632 /* version #2: "dlV2" */
#define	NEXT68K_LABEL_CD_V3		0x646c5633 /* version #3: "dlV3" */
#define	NEXT68K_LABEL_DEFAULTFRONTPORCH	(160 * 2)
#define	NEXT68K_LABEL_DEFAULTBOOT0_1	(32 * 2)
#define	NEXT68K_LABEL_DEFAULTBOOT0_2	(96 * 2)

/* ------------------------------------------
 * pmax --
 *	PMAX (DECstation / MIPS) boot block information
 */

/*
 * If mode is 0, there is just one sequence of blocks and one Dec_BootMap
 * is used.  If mode is 1, there are multiple sequences of blocks
 * and multiple Dec_BootMaps are used, the last with numBlocks = 0.
 */
struct pmax_boot_map {
	int32_t	num_blocks;		/* Number of blocks to read. */
	int32_t	start_block;		/* Starting block on disk. */
};

/*
 * This is the structure of a disk or tape boot block.  The boot_map
 * can either be a single boot count and start block (contiguous mode)
 * or a list of up to 61 (to fill a 512 byte sector) block count and
 * start block pairs.  Under NetBSD, contiguous mode is always used.
 */
struct pmax_boot_block {
	uint8_t		pad[8];
	int32_t		magic;			/* PMAX_BOOT_MAGIC */
	int32_t		mode;			/* Mode for boot info. */
	uint32_t	load_addr;		/* Address to start loading. */
	uint32_t	exec_addr;		/* Address to start execing. */
	struct		pmax_boot_map map[61];	/* boot program section(s). */
} __attribute__((__packed__));

#define	PMAX_BOOT_MAGIC			0x0002757a
#define	PMAX_BOOTMODE_CONTIGUOUS	0
#define	PMAX_BOOTMODE_SCATTERED		1

#define	PMAX_BOOT_BLOCK_OFFSET		0
#define	PMAX_BOOT_BLOCK_BLOCKSIZE	512


/* ------------------------------------------
 * sparc
 */

#define	SPARC_BOOT_BLOCK_OFFSET		512
#define	SPARC_BOOT_BLOCK_BLOCKSIZE	512
#define	SPARC_BOOT_BLOCK_MAX_SIZE	(512 * 15)
	/* Magic string -- 32 bytes long (including the NUL) */
#define	SPARC_BBINFO_MAGIC		"NetBSD/sparc bootxx    20020515"


/* ------------------------------------------
 * sparc64
 */

#define	SPARC64_BOOT_BLOCK_OFFSET	512
#define	SPARC64_BOOT_BLOCK_BLOCKSIZE	512
#define	SPARC64_BOOT_BLOCK_MAX_SIZE	(512 * 15)


/* ------------------------------------------
 * sun68k (sun2, sun3)
 */

#define	SUN68K_BOOT_BLOCK_OFFSET	512
#define	SUN68K_BOOT_BLOCK_BLOCKSIZE	512
#define	SUN68K_BOOT_BLOCK_MAX_SIZE	(512 * 15)
	/* Magic string -- 32 bytes long (including the NUL) */
#define	SUN68K_BBINFO_MAGIC		"NetBSD/sun68k bootxx   20020515"


/* ------------------------------------------
 * vax --
 *	VAX boot block information
 */

struct vax_boot_block {
/* Note that these don't overlap any of the pmax boot block */
	uint8_t		pad0[2];
	uint8_t		bb_id_offset;	/* offset in words to id (magic1)*/
	uint8_t		bb_mbone;	/* must be one */
	uint16_t	bb_lbn_hi;	/* lbn (hi word) of bootstrap */
	uint16_t	bb_lbn_low;	/* lbn (low word) of bootstrap */
	uint8_t		pad1[332];

	/* The rest of these fields are identification area and describe
	 * the secondary block for uVAX VMB.
	 */
	uint8_t		bb_magic1;	/* magic number */
	uint8_t		bb_mbz1;	/* must be zero */
	uint8_t		bb_pad1;	/* any value */
	uint8_t		bb_sum1;	/* ~(magic1 + mbz1 + pad1) */

	uint8_t		bb_mbz2;	/* must be zero */
	uint8_t		bb_volinfo;	/* volinfo */
	uint8_t		bb_pad2a;	/* any value */
	uint8_t		bb_pad2b;	/* any value */

	uint32_t	bb_size;	/* size in blocks of bootstrap */
	uint32_t	bb_load;	/* load offset to bootstrap */
	uint32_t	bb_entry;	/* byte offset in bootstrap */
	uint32_t	bb_sum3;	/* sum of previous 3 fields */

	/* The rest is unused.
	 */
	uint8_t		pad2[148];
} __attribute__((__packed__));

#define	VAX_BOOT_MAGIC1			0x18	/* size of BB info? */
#define	VAX_BOOT_VOLINFO_NONE		0x00	/* no special info */
#define	VAX_BOOT_VOLINFO_SS		0x01	/* single sided */
#define	VAX_BOOT_VOLINFO_DS		0x81	/* double sided */

#define	VAX_BOOT_SIZE			15	/* 15 blocks */
#define	VAX_BOOT_LOAD			0	/* no load offset */
#define	VAX_BOOT_ENTRY			0x200	/* one block in */

#define	VAX_BOOT_BLOCK_OFFSET		0
#define	VAX_BOOT_BLOCK_BLOCKSIZE	512


/* ------------------------------------------
 * x68k
 */

#define	X68K_BOOT_BLOCK_OFFSET		0
#define	X68K_BOOT_BLOCK_BLOCKSIZE	512
#define	X68K_BOOT_BLOCK_MAX_SIZE	(512 * 16)
	/* Magic string -- 32 bytes long (including the NUL) */
#define	X68K_BBINFO_MAGIC		"NetBSD/x68k bootxx     20020601"

#endif	/* !defined(__ASSEMBLER__) */				/* } */

#endif

#endif	/* !_SYS_BOOTBLOCK_H */
