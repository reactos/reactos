/*
 * mft.h - Defines for mft record handling in NTFS Linux kernel driver.
 *	   Part of the Linux-NTFS project.
 *
 * Copyright (c) 2001,2002 Anton Altaparmakov.
 *
 * This program/include file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program/include file is distributed in the hope that it will be 
 * useful, but WITHOUT ANY WARRANTY; without even the implied warranty 
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program (in the main directory of the Linux-NTFS 
 * distribution in the file COPYING); if not, write to the Free Software
 * Foundation,Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef _LINUX_NTFS_MFT_H
#define _LINUX_NTFS_MFT_H

#include <linux/fs.h>

#include "inode.h"

extern int format_mft_record(ntfs_inode *ni, MFT_RECORD *m);
//extern int format_mft_record2(struct super_block *vfs_sb,
//		const unsigned long inum, MFT_RECORD *m);

extern MFT_RECORD *map_mft_record(ntfs_inode *ni);
extern void unmap_mft_record(ntfs_inode *ni);

extern MFT_RECORD *map_extent_mft_record(ntfs_inode *base_ni, MFT_REF mref,
		ntfs_inode **ntfs_ino);

static inline void unmap_extent_mft_record(ntfs_inode *ni)
{
	unmap_mft_record(ni);
	return;
}

/*
 * flush_dcache_mft_record_page - flush_dcache_page() for mft records
 * @ni:		ntfs inode structure of mft record
 *
 * Call flush_dcache_page() for the page in which an mft record resides.
 *
 * This must be called every time an mft record is modified, just after the
 * modification.
 */
static inline void flush_dcache_mft_record_page(ntfs_inode *ni)
{
	flush_dcache_page(ni->page);
}

#endif /* _LINUX_NTFS_MFT_H */

