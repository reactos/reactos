/*
 * ntfs.h - Defines for NTFS Linux kernel driver. Part of the Linux-NTFS
 *	    project.
 *
 * Copyright (c) 2001,2002 Anton Altaparmakov.
 * Copyright (C) 2002 Richard Russon.
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

#ifndef _LINUX_NTFS_H
#define _LINUX_NTFS_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/compiler.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/nls.h>
#include <linux/pagemap.h>
#include <linux/smp.h>
#include <asm/atomic.h>

#include "types.h"
#include "debug.h"
#include "malloc.h"
#include "endian.h"
#include "volume.h"
#include "inode.h"
#include "layout.h"
#include "attrib.h"
#include "mft.h"

typedef enum {
	NTFS_BLOCK_SIZE		= 512,
	NTFS_BLOCK_SIZE_BITS	= 9,
	NTFS_SB_MAGIC		= 0x5346544e,	/* 'NTFS' */
	NTFS_MAX_NAME_LEN	= 255,
} NTFS_CONSTANTS;

/* Global variables. */

/* Slab caches (from super.c). */
extern kmem_cache_t *ntfs_name_cache;
extern kmem_cache_t *ntfs_inode_cache;
extern kmem_cache_t *ntfs_big_inode_cache;
extern kmem_cache_t *ntfs_attr_ctx_cache;

/* The various operations structs defined throughout the driver files. */
extern struct super_operations ntfs_sops;
extern struct super_operations ntfs_mount_sops;

extern struct address_space_operations ntfs_aops;
extern struct address_space_operations ntfs_mft_aops;

extern struct  file_operations ntfs_file_ops;
extern struct inode_operations ntfs_file_inode_ops;

extern struct  file_operations ntfs_dir_ops;
extern struct inode_operations ntfs_dir_inode_ops;

extern struct  file_operations ntfs_empty_file_ops;
extern struct inode_operations ntfs_empty_inode_ops;

/* Generic macro to convert pointers to values for comparison purposes. */
#ifndef p2n
#define p2n(p)          ((ptrdiff_t)((ptrdiff_t*)(p)))
#endif

/**
 * NTFS_SB - return the ntfs volume given a vfs super block
 * @sb:		VFS super block
 *
 * NTFS_SB() returns the ntfs volume associated with the VFS super block @sb.
 */
static inline ntfs_volume *NTFS_SB(struct super_block *sb)
{
	return sb->s_fs_info;
}

/**
 * ntfs_unmap_page - release a page that was mapped using ntfs_map_page()
 * @page:	the page to release
 *
 * Unpin, unmap and release a page that was obtained from ntfs_map_page().
 */
static inline void ntfs_unmap_page(struct page *page)
{
	kunmap(page);
	page_cache_release(page);
}

/**
 * ntfs_map_page - map a page into accessible memory, reading it if necessary
 * @mapping:	address space for which to obtain the page
 * @index:	index into the page cache for @mapping of the page to map
 *
 * Read a page from the page cache of the address space @mapping at position
 * @index, where @index is in units of PAGE_CACHE_SIZE, and not in bytes.
 *
 * If the page is not in memory it is loaded from disk first using the readpage
 * method defined in the address space operations of @mapping and the page is
 * added to the page cache of @mapping in the process.
 *
 * If the page is in high memory it is mapped into memory directly addressible
 * by the kernel.
 *
 * Finally the page count is incremented, thus pinning the page into place.
 *
 * The above means that page_address(page) can be used on all pages obtained
 * with ntfs_map_page() to get the kernel virtual address of the page.
 *
 * When finished with the page, the caller has to call ntfs_unmap_page() to
 * unpin, unmap and release the page.
 *
 * Note this does not grant exclusive access. If such is desired, the caller
 * must provide it independently of the ntfs_{un}map_page() calls by using
 * a {rw_}semaphore or other means of serialization. A spin lock cannot be
 * used as ntfs_map_page() can block.
 *
 * The unlocked and uptodate page is returned on success or an encoded error
 * on failure. Caller has to test for error using the IS_ERR() macro on the
 * return value. If that evaluates to TRUE, the negative error code can be
 * obtained using PTR_ERR() on the return value of ntfs_map_page().
 */
static inline struct page *ntfs_map_page(struct address_space *mapping,
		unsigned long index)
{
	struct page *page = read_cache_page(mapping, index,
			(filler_t*)mapping->a_ops->readpage, NULL);

	if (!IS_ERR(page)) {
		wait_on_page_locked(page);
		kmap(page);
		if (PageUptodate(page) && !PageError(page))
			return page;
		ntfs_unmap_page(page);
		return ERR_PTR(-EIO);
	}
	return page;
}

/* Declarations of functions and global variables. */

/* From fs/ntfs/compress.c */
extern int ntfs_read_compressed_block(struct page *page);

/* From fs/ntfs/super.c */
#define default_upcase_len 0x10000
extern wchar_t *default_upcase;
extern unsigned long ntfs_nr_upcase_users;
extern unsigned long ntfs_nr_mounts;
extern struct semaphore ntfs_lock;

typedef struct {
	int val;
	char *str;
} option_t;
extern const option_t on_errors_arr[];

/* From fs/ntfs/compress.c */
extern int allocate_compression_buffers(void);
extern void free_compression_buffers(void);

/* From fs/ntfs/mst.c */
extern int post_read_mst_fixup(NTFS_RECORD *b, const u32 size);
extern int pre_write_mst_fixup(NTFS_RECORD *b, const u32 size);
extern void post_write_mst_fixup(NTFS_RECORD *b);

/* From fs/ntfs/time.c */
extern inline s64 utc2ntfs(const time_t time);
extern inline s64 get_current_ntfs_time(void);
extern inline time_t ntfs2utc(const s64 time);

/* From fs/ntfs/unistr.c */
extern BOOL ntfs_are_names_equal(const uchar_t *s1, size_t s1_len,
		const uchar_t *s2, size_t s2_len,
		const IGNORE_CASE_BOOL ic,
		const uchar_t *upcase, const u32 upcase_size);
extern int ntfs_collate_names(const uchar_t *name1, const u32 name1_len,
		const uchar_t *name2, const u32 name2_len,
		const int err_val, const IGNORE_CASE_BOOL ic,
		const uchar_t *upcase, const u32 upcase_len);
extern int ntfs_ucsncmp(const uchar_t *s1, const uchar_t *s2, size_t n);
extern int ntfs_ucsncasecmp(const uchar_t *s1, const uchar_t *s2, size_t n,
		const uchar_t *upcase, const u32 upcase_size);
extern void ntfs_upcase_name(uchar_t *name, u32 name_len,
		const uchar_t *upcase, const u32 upcase_len);
extern void ntfs_file_upcase_value(FILE_NAME_ATTR *file_name_attr,
		const uchar_t *upcase, const u32 upcase_len);
extern int ntfs_file_compare_values(FILE_NAME_ATTR *file_name_attr1,
		FILE_NAME_ATTR *file_name_attr2,
		const int err_val, const IGNORE_CASE_BOOL ic,
		const uchar_t *upcase, const u32 upcase_len);
extern int ntfs_nlstoucs(const ntfs_volume *vol, const char *ins,
		const int ins_len, uchar_t **outs);
extern int ntfs_ucstonls(const ntfs_volume *vol, const uchar_t *ins,
		const int ins_len, unsigned char **outs, int outs_len);

/* From fs/ntfs/upcase.c */
extern uchar_t *generate_default_upcase(void);

#endif /* _LINUX_NTFS_H */

