/* @(#)iso9660.h	1.22 11/06/04 joerg */
/*
 * Header file iso9660.h - assorted structure definitions and typecasts.
 * specific to iso9660 filesystem.
 *
 * Written by Eric Youngdale (1993).
 *
 * Copyright 1993 Yggdrasil Computing, Incorporated
 * Copyright (c) 1999,2000-2007 J. Schilling
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
#ifndef	_ISOFS_FS_H
#define	_ISOFS_FS_H

/*
 * The isofs filesystem constants/structures
 */

/* This part borrowed from the bsd386 isofs */
#define	ISODCL(from, to) (to - from + 1)

struct iso_volume_descriptor {
	char type	[ISODCL(1, 1)]; /* 711 */
	char id		[ISODCL(2, 6)];
	char version	[ISODCL(7, 7)];
	char data	[ISODCL(8, 2048)];
};

/* volume descriptor types */
#define	ISO_VD_PRIMARY		1
#define	ISO_VD_SUPPLEMENTARY	2	/* Used by Joliet */
#define	ISO_VD_END		255

#define	ISO_STANDARD_ID		"CD001"

#define	EL_TORITO_ID		"EL TORITO SPECIFICATION"
#define	EL_TORITO_ARCH_x86	0
#define	EL_TORITO_ARCH_PPC	1
#define	EL_TORITO_ARCH_MAC	2
#define	EL_TORITO_ARCH_EFI	0xEF

#define	EL_TORITO_BOOTABLE	0x88
#define	EL_TORITO_NOT_BOOTABLE	0

#define	EL_TORITO_MEDIA_NOEMUL	0
#define	EL_TORITO_MEDIA_12FLOP	1
#define	EL_TORITO_MEDIA_144FLOP	2
#define	EL_TORITO_MEDIA_288FLOP	3
#define	EL_TORITO_MEDIA_HD	4

struct iso_primary_descriptor {
	char type			[ISODCL(1,    1)]; /* 711 */
	char id				[ISODCL(2,    6)];
	char version			[ISODCL(7,    7)]; /* 711 */
	char unused1			[ISODCL(8,    8)];
	char system_id			[ISODCL(9,   40)]; /* achars */
	char volume_id			[ISODCL(41,  72)]; /* dchars */
	char unused2			[ISODCL(73,  80)];
	char volume_space_size		[ISODCL(81,  88)]; /* 733 */
	char escape_sequences		[ISODCL(89, 120)];
	char volume_set_size		[ISODCL(121, 124)]; /* 723 */
	char volume_sequence_number	[ISODCL(125, 128)]; /* 723 */
	char logical_block_size		[ISODCL(129, 132)]; /* 723 */
	char path_table_size		[ISODCL(133, 140)]; /* 733 */
	char type_l_path_table		[ISODCL(141, 144)]; /* 731 */
	char opt_type_l_path_table	[ISODCL(145, 148)]; /* 731 */
	char type_m_path_table		[ISODCL(149, 152)]; /* 732 */
	char opt_type_m_path_table	[ISODCL(153, 156)]; /* 732 */
	char root_directory_record	[ISODCL(157, 190)]; /* 9.1 */
	char volume_set_id		[ISODCL(191, 318)]; /* dchars */
	char publisher_id		[ISODCL(319, 446)]; /* achars */
	char preparer_id		[ISODCL(447, 574)]; /* achars */
	char application_id		[ISODCL(575, 702)]; /* achars */
	char copyright_file_id		[ISODCL(703, 739)]; /* 7.5 dchars */
	char abstract_file_id		[ISODCL(740, 776)]; /* 7.5 dchars */
	char bibliographic_file_id	[ISODCL(777, 813)]; /* 7.5 dchars */
	char creation_date		[ISODCL(814, 830)]; /* 8.4.26.1 */
	char modification_date		[ISODCL(831, 847)]; /* 8.4.26.1 */
	char expiration_date		[ISODCL(848, 864)]; /* 8.4.26.1 */
	char effective_date		[ISODCL(865, 881)]; /* 8.4.26.1 */
	char file_structure_version	[ISODCL(882, 882)]; /* 711 */
	char unused4			[ISODCL(883, 883)];
	char application_data		[ISODCL(884, 1395)];
	char unused5			[ISODCL(1396, 2048)];
};

/*
 * Supplementary or enhanced volume descriptor
 */
struct iso_enhanced_descriptor {
	char type			[ISODCL(1,    1)]; /* 711 */
	char id				[ISODCL(2,    6)];
	char version			[ISODCL(7,    7)]; /* 711 */
	char flags			[ISODCL(8,    8)];
	char system_id			[ISODCL(9,   40)]; /* achars */
	char volume_id			[ISODCL(41,  72)]; /* dchars */
	char unused2			[ISODCL(73,  80)];
	char volume_space_size		[ISODCL(81,  88)]; /* 733 */
	char escape_sequences		[ISODCL(89, 120)];
	char volume_set_size		[ISODCL(121, 124)]; /* 723 */
	char volume_sequence_number	[ISODCL(125, 128)]; /* 723 */
	char logical_block_size		[ISODCL(129, 132)]; /* 723 */
	char path_table_size		[ISODCL(133, 140)]; /* 733 */
	char type_l_path_table		[ISODCL(141, 144)]; /* 731 */
	char opt_type_l_path_table	[ISODCL(145, 148)]; /* 731 */
	char type_m_path_table		[ISODCL(149, 152)]; /* 732 */
	char opt_type_m_path_table	[ISODCL(153, 156)]; /* 732 */
	char root_directory_record	[ISODCL(157, 190)]; /* 9.1 */
	char volume_set_id		[ISODCL(191, 318)]; /* dchars */
	char publisher_id		[ISODCL(319, 446)]; /* achars */
	char preparer_id		[ISODCL(447, 574)]; /* achars */
	char application_id		[ISODCL(575, 702)]; /* achars */
	char copyright_file_id		[ISODCL(703, 739)]; /* 7.5 dchars */
	char abstract_file_id		[ISODCL(740, 776)]; /* 7.5 dchars */
	char bibliographic_file_id	[ISODCL(777, 813)]; /* 7.5 dchars */
	char creation_date		[ISODCL(814, 830)]; /* 8.4.26.1 */
	char modification_date		[ISODCL(831, 847)]; /* 8.4.26.1 */
	char expiration_date		[ISODCL(848, 864)]; /* 8.4.26.1 */
	char effective_date		[ISODCL(865, 881)]; /* 8.4.26.1 */
	char file_structure_version	[ISODCL(882, 882)]; /* 711 */
	char unused4			[ISODCL(883, 883)];
	char application_data		[ISODCL(884, 1395)];
	char unused5			[ISODCL(1396, 2048)];
};

/* El Torito Boot Record Volume Descriptor */
struct eltorito_boot_descriptor {
	char type			[ISODCL(1,    1)]; /* 711 */
	char id				[ISODCL(2,    6)];
	char version			[ISODCL(7,    7)]; /* 711 */
	char system_id			[ISODCL(8,   39)];
	char unused2			[ISODCL(40,  71)];
	char bootcat_ptr		[ISODCL(72,  75)];
	char unused5			[ISODCL(76, 2048)];
};

/* Validation entry for El Torito */
/*
 * headerid must be 1
 * id is the manufacturer ID
 * cksum to make the sum of all shorts in this record 0
 */
struct eltorito_validation_entry {
	char headerid			[ISODCL(1,    1)]; /* 711 */
	char arch			[ISODCL(2,    2)];
	char pad1			[ISODCL(3,    4)]; /* 721 */
	char id				[ISODCL(5,   28)]; /* CD devel/man*/
	char cksum			[ISODCL(29,  30)];
	char key1			[ISODCL(31,  31)];
	char key2			[ISODCL(32,  32)];
};

/* El Torito initial/default entry in boot catalog */
struct eltorito_defaultboot_entry {
	char boot_id			[ISODCL(1,    1)]; /* 711 */
	char boot_media			[ISODCL(2,    2)];
	char loadseg			[ISODCL(3,    4)]; /* 721 */
	char sys_type			[ISODCL(5,    5)];
	char pad1			[ISODCL(6,    6)];
	char nsect			[ISODCL(7,    8)];
	char bootoff			[ISODCL(9,   12)];
	char pad2			[ISODCL(13,  32)];
};

/* El Torito section header entry in boot catalog */
struct eltorito_sectionheader_entry {
#define	EL_TORITO_SHDR_ID_SHDR		0x90
#define	EL_TORITO_SHDR_ID_LAST_SHDR	0x91
	char header_id			[ISODCL(1,    1)]; /* 711 */
	char platform_id		[ISODCL(2,    2)];
	char entry_count		[ISODCL(3,    4)]; /* 721 */
	char id				[ISODCL(5,   32)];
};

/* El Torito section entry in boot catalog */
struct eltorito_section_entry {
	char boot_id			[ISODCL(1,    1)]; /* 711 */
	char boot_media			[ISODCL(2,    2)];
	char loadseg			[ISODCL(3,    4)]; /* 721 */
	char sys_type			[ISODCL(5,    5)];
	char pad1			[ISODCL(6,    6)];
	char nsect			[ISODCL(7,    8)];
	char bootoff			[ISODCL(9,   12)];
	char sel_criteria		[ISODCL(13,  13)];
	char vendor_sel_criteria	[ISODCL(14,  32)];
};

/*
 * XXX JS: The next two structures have odd lengths!
 * Some compilers (e.g. on Sun3/mc68020) padd the structures to even length.
 * For this reason, we cannot use sizeof (struct iso_path_table) or
 * sizeof (struct iso_directory_record) to compute on disk sizes.
 * Instead, we use offsetof(..., name) and add the name size.
 * See mkisofs.h
 */

/* We use this to help us look up the parent inode numbers. */

struct iso_path_table {
	unsigned char  name_len[2];	/* 721 */
	char extent[4];			/* 731 */
	char  parent[2];		/* 721 */
	char name[1];
};

/*
 * A ISO filename is: "abcde.eee;1" -> <filename> '.' <ext> ';' <version #>
 *
 * The maximum needed string length is:
 *	30 chars (filename + ext)
 * +	 2 chars ('.' + ';')
 * +	   strlen("32767")
 * +	   null byte
 * ================================
 * =	38 chars
 *
 * We currently do not support CD-ROM-XA entension records, but we must honor
 * the needed space for ISO-9660:1999 (Version 2).
 *
 * XXX If we ever will start to support XA records, we will need to take care
 * XXX that the the maximum ISO-9660 name length will be reduced by another
 * XXX 14 bytes resulting in a new total of 179 Bytes.
 */
#define	LEN_ISONAME		31
#define	MAX_ISONAME_V1		37
#define	MAX_ISONAME_V2		207		/* 254 - 33 - 14 (XA Record) */
#define	MAX_ISONAME_V2_RR	193		/* 254 - 33 - 28 (CE Record) */
#define	MAX_ISONAME_V2_RR_XA	179		/* 254 - 33 - 14 - 28	    */
#define	MAX_ISONAME		MAX_ISONAME_V2	/* Used for array space defs */
#define	MAX_ISODIR		254		/* Must be even and <= 255   */

struct iso_directory_record {
	unsigned char length		[ISODCL(1,  1)];  /* 711 */
	char ext_attr_length		[ISODCL(2,  2)];  /* 711 */
	char extent			[ISODCL(3,  10)]; /* 733 */
	char size			[ISODCL(11, 18)]; /* 733 */
	char date			[ISODCL(19, 25)]; /* 7 by 711 */
	unsigned char flags		[ISODCL(26, 26)];
	char file_unit_size		[ISODCL(27, 27)]; /* 711 */
	char interleave			[ISODCL(28, 28)]; /* 711 */
	char volume_sequence_number	[ISODCL(29, 32)]; /* 723 */
	unsigned char name_len		[ISODCL(33, 33)]; /* 711 */
	char name			[MAX_ISONAME+1]; /* Not really, but we need something here */
};


/*
 * Iso directory flags.
 */
#define	ISO_FILE	0	/* Not really a flag...			*/
#define	ISO_EXISTENCE	1	/* Do not make existence known (hidden)	*/
#define	ISO_DIRECTORY	2	/* This file is a directory		*/
#define	ISO_ASSOCIATED	4	/* This file is an assiciated file	*/
#define	ISO_RECORD	8	/* Record format in extended attr. != 0	*/
#define	ISO_PROTECTION	16	/* No read/execute perm. in ext. attr.	*/
#define	ISO_DRESERVED1	32	/* Reserved bit 5			*/
#define	ISO_DRESERVED2	64	/* Reserved bit 6			*/
#define	ISO_MULTIEXTENT	128	/* Not final entry of a mult. ext. file	*/


struct iso_ext_attr_record {
	char owner			[ISODCL(1, 4)];	    /* 723 */
	char group			[ISODCL(5, 8)];	    /* 723 */
	char permissions		[ISODCL(9, 10)];    /* 16 bits */
	char creation_date		[ISODCL(11, 27)];   /* 8.4.26.1 */
	char modification_date		[ISODCL(28, 44)];   /* 8.4.26.1 */
	char expiration_date		[ISODCL(45, 61)];   /* 8.4.26.1 */
	char effective_date		[ISODCL(62, 78)];   /* 8.4.26.1 */
	char record_format		[ISODCL(79, 79)];   /* 711 */
	char record_attributes		[ISODCL(80, 80)];   /* 711 */
	char record_length		[ISODCL(81, 84)];   /* 723 */
	char system_id			[ISODCL(85, 116)];  /* achars */
	char system_use			[ISODCL(117, 180)];
	char ext_attr_version		[ISODCL(181, 181)]; /* 711 */
	char esc_seq_len		[ISODCL(182, 182)]; /* 711 */
	char reserved			[ISODCL(183, 246)]; /* for future use */
	char appl_use_len		[ISODCL(247, 250)]; /* 723 */
	char appl_use[1];		/* really more */
/*	char esc_seq[];			escape sequences recorded after appl_use */
};

/*
 * Iso extended attribute permissions.
 */
#define	ISO_GS_READ		0x0001	/* System Group Read */
#define	ISO_BIT_1		0x0002
#define	ISO_GS_EXEC		0x0004	/* System Group Execute */
#define	ISO_BIT_3		0x0008

#define	ISO_O_READ		0x0010	/* Owner Read */
#define	ISO_BIT_5		0x0020
#define	ISO_O_EXEC		0x0040	/* Owner Exexute */
#define	ISO_BIT_7		0x0080

#define	ISO_G_READ		0x0100	/* Group Read */
#define	ISO_BIT_9		0x0200
#define	ISO_G_EXEC		0x0400	/* Group Execute */
#define	ISO_BIT_11		0x0800

#define	ISO_W_READ		0x1000	/* World (other) Read */
#define	ISO_BIT_13		0x2000
#define	ISO_W_EXEC		0x4000	/* World (other) Execute */
#define	ISO_BIT_15		0x8000

#define	ISO_MB_ONE		(ISO_BIT_1|ISO_BIT_3|ISO_BIT_5|ISO_BIT_7| \
				ISO_BIT_9|ISO_BIT_11|ISO_BIT_13|ISO_BIT_15)

/*
 * Extended Attributes record according to Yellow Book.
 */
struct iso_xa_dir_record {
	char group_id			[ISODCL(1, 2)];
	char user_id			[ISODCL(3, 4)];
	char attributes			[ISODCL(5, 6)];
	char signature			[ISODCL(7, 8)];
	char file_number		[ISODCL(9, 9)];
	char reserved			[ISODCL(10, 14)];
};

/*
 * Definitions for XA attributes
 */
#define	XA_O_READ	0x0001	/* Owner Read				*/
#define	XA_O_RES	0x0002	/* Owner Reserved (write ?)		*/
#define	XA_O_EXEC	0x0004	/* Owner Execute			*/
#define	XA_O_RES2	0x0008	/* Owner Reserved			*/
#define	XA_G_READ	0x0010	/* Group Read				*/
#define	XA_G_RES	0x0020	/* Group Reserved (write ?)		*/
#define	XA_G_EXEC	0x0040	/* Group Execute			*/
#define	XA_G_RES2	0x0080	/* Group Reserved			*/
#define	XA_W_READ	0x0100	/* World Read				*/
#define	XA_W_RES	0x0200	/* World Reserved (write ?)		*/
#define	XA_W_EXEC	0x0400	/* World Execute			*/

#define	XA_FORM1	0x0800	/* File contains Form 1 sector		*/
#define	XA_FORM2	0x1000	/* File contains Form 2 sector		*/
#define	XA_INTERLEAVED	0x2000	/* File contains interleaved sectors	*/
#define	XA_CDDA		0x4000	/* File contains audio data		*/
#define	XA_DIR		0x8000	/* This is a directory			*/

/*
 * Definitions for CD-ROM XA-Mode-2-form-1/2 sector sub-headers
 */
struct xa_subhdr {
	Uchar	file_number;		/* Identifies file for block	*/
	Uchar	channel_number;		/* Playback channel selection	*/
	Uchar	sub_mode;		/* See bit definitions below	*/
	Uchar	coding;			/* Coding information		*/
};

/*
 * Sub mode bit definitions
 */
#define	XA_SUBH_EOR		0x01	/* End-Of-Record		*/
#define	XA_SUBH_VIDEO		0x02	/* Video Block			*/
#define	XA_SUBH_AUDIO		0x04	/* Audio Block (not CD-DA)	*/
#define	XA_SUBH_DATA		0x08	/* Data Block			*/
#define	XA_SUBH_TRIGGER		0x10	/* Trigger Block		*/
#define	XA_SUBH_FORM2		0x20	/* 0 == Form1, 1 == Form2	*/
#define	XA_SUBH_REALTIME	0x40	/* Real Time Block		*/
#define	XA_SUBH_EOF		0x80	/* End-Of-File			*/

#endif	/* _ISOFS_FS_H */
