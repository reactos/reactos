/* @(#)diskmbr.h	1.2 04/03/02 Copyright 1999-2004 J. Schilling */
/*
 *	Header file diskmbr.h - assorted structure definitions and macros
 *	describing standard PC partition table
 *
 *	Copyright (c) 1999-2004 J. Schilling
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; see the file COPYING.  If not, write to the Free Software
 * Foundation, 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifndef _DISKMBR_H
#define	_DISKMBR_H

#define	MBR_MAGIC		0xAA55

#define	PARTITION_UNUSED	0x00
#define	PARTITION_ACTIVE	0x80

#define	PARTITION_COUNT		4

#define	MBR_SECTOR(x)		((x)&0x3F)
#define	MBR_CYLINDER(x)		((x)>>8|((x)<<2&0x300))

struct disk_partition {
	unsigned char	status;
	unsigned char	s_head;
	unsigned char	s_cyl_sec[2];
	unsigned char	type;
	unsigned char	e_head;
	unsigned char	e_cyl_sec[2];
	unsigned char	boot_sec[4];
	unsigned char	size[4];
};

struct disk_master_boot_record {
	char			pad[0x1BE];
	struct disk_partition	partition[PARTITION_COUNT];
	unsigned char		magic[2];
};

#endif	/* _DISKMBR_H */
