/**
 * inode.c - NTFS kernel inode handling. Part of the Linux-NTFS project.
 *
 * Copyright (c) 2001-2003 Anton Altaparmakov
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

#include <linux/pagemap.h>
#include <linux/buffer_head.h>
#include <linux/smp_lock.h>
#include <linux/quotaops.h>
#include <linux/mount.h>

#include "ntfs.h"
#include "dir.h"
#include "inode.h"
#include "attrib.h"

/**
 * ntfs_attr - ntfs in memory attribute structure
 * @mft_no:	mft record number of the base mft record of this attribute
 * @name:	Unicode name of the attribute (NULL if unnamed)
 * @name_len:	length of @name in Unicode characters (0 if unnamed)
 * @type:	attribute type (see layout.h)
 *
 * This structure exists only to provide a small structure for the
 * ntfs_{attr_}iget()/ntfs_test_inode()/ntfs_init_locked_inode() mechanism.
 *
 * NOTE: Elements are ordered by size to make the structure as compact as
 * possible on all architectures.
 */
typedef struct {
	unsigned long mft_no;
	uchar_t *name;
	u32 name_len;
	ATTR_TYPES type;
} ntfs_attr;

/**
 * ntfs_test_inode - compare two (possibly fake) inodes for equality
 * @vi:		vfs inode which to test
 * @na:		ntfs attribute which is being tested with
 *
 * Compare the ntfs attribute embedded in the ntfs specific part of the vfs
 * inode @vi for equality with the ntfs attribute @na.
 *
 * If searching for the normal file/directory inode, set @na->type to AT_UNUSED.
 * @na->name and @na->name_len are then ignored.
 *
 * Return 1 if the attributes match and 0 if not.
 *
 * NOTE: This function runs with the inode_lock spin lock held so it is not
 * allowed to sleep.
 */
static int ntfs_test_inode(struct inode *vi, ntfs_attr *na)
{
	ntfs_inode *ni;

	if (vi->i_ino != na->mft_no)
		return 0;
	ni = NTFS_I(vi);
	/* If !NInoAttr(ni), @vi is a normal file or directory inode. */
	if (likely(!NInoAttr(ni))) {
		/* If not looking for a normal inode this is a mismatch. */
		if (unlikely(na->type != AT_UNUSED))
			return 0;
	} else {
		/* A fake inode describing an attribute. */
		if (ni->type != na->type)
			return 0;
		if (ni->name_len != na->name_len)
			return 0;
		if (na->name_len && memcmp(ni->name, na->name,
				na->name_len * sizeof(uchar_t)))
			return 0;
	}
	/* Match! */
	return 1;
}

/**
 * ntfs_init_locked_inode - initialize an inode
 * @vi:		vfs inode to initialize
 * @na:		ntfs attribute which to initialize @vi to
 *
 * Initialize the vfs inode @vi with the values from the ntfs attribute @na in
 * order to enable ntfs_test_inode() to do its work.
 *
 * If initializing the normal file/directory inode, set @na->type to AT_UNUSED.
 * In that case, @na->name and @na->name_len should be set to NULL and 0,
 * respectively. Although that is not strictly necessary as
 * ntfs_read_inode_locked() will fill them in later.
 *
 * Return 0 on success and -errno on error.
 *
 * NOTE: This function runs with the inode_lock spin lock held so it is not
 * allowed to sleep. (Hence the GFP_ATOMIC allocation.)
 */
static int ntfs_init_locked_inode(struct inode *vi, ntfs_attr *na)
{
	ntfs_inode *ni = NTFS_I(vi);

	vi->i_ino = na->mft_no;

	ni->type = na->type;
	if (na->type == AT_INDEX_ALLOCATION)
		NInoSetMstProtected(ni);

	ni->name = na->name;
	ni->name_len = na->name_len;

	/* If initializing a normal inode, we are done. */
	if (likely(na->type == AT_UNUSED))
		return 0;

	/* It is a fake inode. */
	NInoSetAttr(ni);

	/*
	 * We have I30 global constant as an optimization as it is the name
	 * in >99.9% of named attributes! The other <0.1% incur a GFP_ATOMIC
	 * allocation but that is ok. And most attributes are unnamed anyway,
	 * thus the fraction of named attributes with name != I30 is actually
	 * absolutely tiny.
	 */
	if (na->name && na->name_len && na->name != I30) {
		unsigned int i;

		i = na->name_len * sizeof(uchar_t);
		ni->name = (uchar_t*)kmalloc(i + sizeof(uchar_t), GFP_ATOMIC);
		if (!ni->name)
			return -ENOMEM;
		memcpy(ni->name, na->name, i);
		ni->name[i] = cpu_to_le16('\0');
	}
	return 0;
}

typedef int (*test_t)(struct inode *, void *);
typedef int (*set_t)(struct inode *, void *);
static int ntfs_read_locked_inode(struct inode *vi);
static int ntfs_read_locked_attr_inode(struct inode *base_vi, struct inode *vi);

/**
 * ntfs_iget - obtain a struct inode corresponding to a specific normal inode
 * @sb:		super block of mounted volume
 * @mft_no:	mft record number / inode number to obtain
 *
 * Obtain the struct inode corresponding to a specific normal inode (i.e. a
 * file or directory).
 *
 * If the inode is in the cache, it is just returned with an increased
 * reference count. Otherwise, a new struct inode is allocated and initialized,
 * and finally ntfs_read_locked_inode() is called to read in the inode and
 * fill in the remainder of the inode structure.
 *
 * Return the struct inode on success. Check the return value with IS_ERR() and
 * if true, the function failed and the error code is obtained from PTR_ERR().
 */
struct inode *ntfs_iget(struct super_block *sb, unsigned long mft_no)
{
	struct inode *vi;
	ntfs_attr na;
	int err;

	na.mft_no = mft_no;
	na.type = AT_UNUSED;
	na.name = NULL;
	na.name_len = 0;

	vi = iget5_locked(sb, mft_no, (test_t)ntfs_test_inode,
			(set_t)ntfs_init_locked_inode, &na);
	if (!vi)
		return ERR_PTR(-ENOMEM);

	err = 0;

	/* If this is a freshly allocated inode, need to read it now. */
	if (vi->i_state & I_NEW) {
		err = ntfs_read_locked_inode(vi);
		unlock_new_inode(vi);
	}
	/*
	 * There is no point in keeping bad inodes around if the failure was
	 * due to ENOMEM. We want to be able to retry again layer.
	 */
	if (err == -ENOMEM) {
		iput(vi);
		vi = ERR_PTR(err);
	}
	return vi;
}

/**
 * ntfs_attr_iget - obtain a struct inode corresponding to an attribute
 * @base_vi:	vfs base inode containing the attribute
 * @type:	attribute type
 * @name:	Unicode name of the attribute (NULL if unnamed)
 * @name_len:	length of @name in Unicode characters (0 if unnamed)
 *
 * Obtain the (fake) struct inode corresponding to the attribute specified by
 * @type, @name, and @name_len, which is present in the base mft record
 * specified by the vfs inode @base_vi.
 *
 * If the attribute inode is in the cache, it is just returned with an
 * increased reference count. Otherwise, a new struct inode is allocated and
 * initialized, and finally ntfs_read_locked_attr_inode() is called to read the
 * attribute and fill in the inode structure.
 *
 * Return the struct inode of the attribute inode on success. Check the return
 * value with IS_ERR() and if true, the function failed and the error code is
 * obtained from PTR_ERR().
 */
struct inode *ntfs_attr_iget(struct inode *base_vi, ATTR_TYPES type,
		uchar_t *name, u32 name_len)
{
	struct inode *vi;
	ntfs_attr na;
	int err;

	na.mft_no = base_vi->i_ino;
	na.type = type;
	na.name = name;
	na.name_len = name_len;

	vi = iget5_locked(base_vi->i_sb, na.mft_no, (test_t)ntfs_test_inode,
			(set_t)ntfs_init_locked_inode, &na);
	if (!vi)
		return ERR_PTR(-ENOMEM);

	err = 0;

	/* If this is a freshly allocated inode, need to read it now. */
	if (vi->i_state & I_NEW) {
		err = ntfs_read_locked_attr_inode(base_vi, vi);
		unlock_new_inode(vi);
	}
	/*
	 * There is no point in keeping bad attribute inodes around. This also
	 * simplifies things in that we never need to check for bad attribute
	 * inodes elsewhere.
	 */
	if (err) {
		iput(vi);
		vi = ERR_PTR(err);
	}
	return vi;
}

struct inode *ntfs_alloc_big_inode(struct super_block *sb)
{
	ntfs_inode *ni;

	ntfs_debug("Entering.");
	ni = (ntfs_inode *)kmem_cache_alloc(ntfs_big_inode_cache,
			SLAB_NOFS);
	if (likely(ni != NULL)) {
		ni->state = 0;
		return VFS_I(ni);
	}
	ntfs_error(sb, "Allocation of NTFS big inode structure failed.");
	return NULL;
}

void ntfs_destroy_big_inode(struct inode *inode)
{
	ntfs_inode *ni = NTFS_I(inode);

	ntfs_debug("Entering.");
	BUG_ON(ni->page);
	if (!atomic_dec_and_test(&ni->count))
		BUG();
	kmem_cache_free(ntfs_big_inode_cache, NTFS_I(inode));
}

static inline ntfs_inode *ntfs_alloc_extent_inode(void)
{
	ntfs_inode *ni;

	ntfs_debug("Entering.");
	ni = (ntfs_inode *)kmem_cache_alloc(ntfs_inode_cache, SLAB_NOFS);
	if (likely(ni != NULL)) {
		ni->state = 0;
		return ni;
	}
	ntfs_error(NULL, "Allocation of NTFS inode structure failed.");
	return NULL;
}

void ntfs_destroy_extent_inode(ntfs_inode *ni)
{
	ntfs_debug("Entering.");
	BUG_ON(ni->page);
	if (!atomic_dec_and_test(&ni->count))
		BUG();
	kmem_cache_free(ntfs_inode_cache, ni);
}

/**
 * __ntfs_init_inode - initialize ntfs specific part of an inode
 * @sb:		super block of mounted volume
 * @ni:		freshly allocated ntfs inode which to initialize
 *
 * Initialize an ntfs inode to defaults.
 *
 * NOTE: ni->mft_no, ni->state, ni->type, ni->name, and ni->name_len are left
 * untouched. Make sure to initialize them elsewhere.
 *
 * Return zero on success and -ENOMEM on error.
 */
static void __ntfs_init_inode(struct super_block *sb, ntfs_inode *ni)
{
	ntfs_debug("Entering.");
	ni->initialized_size = ni->allocated_size = 0;
	ni->seq_no = 0;
	atomic_set(&ni->count, 1);
	ni->vol = NTFS_SB(sb);
	init_run_list(&ni->run_list);
	init_MUTEX(&ni->mrec_lock);
	ni->page = NULL;
	ni->page_ofs = 0;
	ni->attr_list_size = 0;
	ni->attr_list = NULL;
	init_run_list(&ni->attr_list_rl);
	ni->itype.index.bmp_ino = NULL;
	ni->itype.index.block_size = 0;
	ni->itype.index.vcn_size = 0;
	ni->itype.index.block_size_bits = 0;
	ni->itype.index.vcn_size_bits = 0;
	init_MUTEX(&ni->extent_lock);
	ni->nr_extents = 0;
	ni->ext.base_ntfs_ino = NULL;
	return;
}

static inline void ntfs_init_big_inode(struct inode *vi)
{
	ntfs_inode *ni = NTFS_I(vi);

	ntfs_debug("Entering.");
	__ntfs_init_inode(vi->i_sb, ni);
	ni->mft_no = vi->i_ino;
	return;
}

inline ntfs_inode *ntfs_new_extent_inode(struct super_block *sb,
		unsigned long mft_no)
{
	ntfs_inode *ni = ntfs_alloc_extent_inode();

	ntfs_debug("Entering.");
	if (likely(ni != NULL)) {
		__ntfs_init_inode(sb, ni);
		ni->mft_no = mft_no;
		ni->type = AT_UNUSED;
		ni->name = NULL;
		ni->name_len = 0;
	}
	return ni;
}

/**
 * ntfs_is_extended_system_file - check if a file is in the $Extend directory
 * @ctx:	initialized attribute search context
 *
 * Search all file name attributes in the inode described by the attribute
 * search context @ctx and check if any of the names are in the $Extend system
 * directory.
 * 
 * Return values:
 *	   1: file is in $Extend directory
 *	   0: file is not in $Extend directory
 *	-EIO: file is corrupt
 */
static int ntfs_is_extended_system_file(attr_search_context *ctx)
{
	int nr_links;

	/* Restart search. */
	reinit_attr_search_ctx(ctx);

	/* Get number of hard links. */
	nr_links = le16_to_cpu(ctx->mrec->link_count);

	/* Loop through all hard links. */
	while (lookup_attr(AT_FILE_NAME, NULL, 0, 0, 0, NULL, 0, ctx)) {
		FILE_NAME_ATTR *file_name_attr;
		ATTR_RECORD *attr = ctx->attr;
		u8 *p, *p2;

		nr_links--;
		/*
		 * Maximum sanity checking as we are called on an inode that
		 * we suspect might be corrupt.
		 */
		p = (u8*)attr + le32_to_cpu(attr->length);
		if (p < (u8*)ctx->mrec || (u8*)p > (u8*)ctx->mrec +
				le32_to_cpu(ctx->mrec->bytes_in_use)) {
err_corrupt_attr:
			ntfs_error(ctx->ntfs_ino->vol->sb, "Corrupt file name "
					"attribute. You should run chkdsk.");
			return -EIO;
		}
		if (attr->non_resident) {
			ntfs_error(ctx->ntfs_ino->vol->sb, "Non-resident file "
					"name. You should run chkdsk.");
			return -EIO;
		}
		if (attr->flags) {
			ntfs_error(ctx->ntfs_ino->vol->sb, "File name with "
					"invalid flags. You should run "
					"chkdsk.");
			return -EIO;
		}
		if (!(attr->data.resident.flags & RESIDENT_ATTR_IS_INDEXED)) {
			ntfs_error(ctx->ntfs_ino->vol->sb, "Unindexed file "
					"name. You should run chkdsk.");
			return -EIO;
		}
		file_name_attr = (FILE_NAME_ATTR*)((u8*)attr +
				le16_to_cpu(attr->data.resident.value_offset));
		p2 = (u8*)attr + le32_to_cpu(attr->data.resident.value_length);
		if (p2 < (u8*)attr || p2 > p)
			goto err_corrupt_attr;
		/* This attribute is ok, but is it in the $Extend directory? */
		if (MREF_LE(file_name_attr->parent_directory) == FILE_Extend)
			return 1;	/* YES, it's an extended system file. */
	}
	if (nr_links) {
		ntfs_error(ctx->ntfs_ino->vol->sb, "Inode hard link count "
				"doesn't match number of name attributes. You "
				"should run chkdsk.");
		return -EIO;
	}
	return 0;	/* NO, it is not an extended system file. */
}

/**
 * ntfs_read_locked_inode - read an inode from its device
 * @vi:		inode to read
 *
 * ntfs_read_locked_inode() is called from ntfs_iget() to read the inode
 * described by @vi into memory from the device.
 *
 * The only fields in @vi that we need to/can look at when the function is
 * called are i_sb, pointing to the mounted device's super block, and i_ino,
 * the number of the inode to load. If this is a fake inode, i.e. NInoAttr(),
 * then the fields type, name, and name_len are also valid, and describe the
 * attribute which this fake inode represents.
 *
 * ntfs_read_locked_inode() maps, pins and locks the mft record number i_ino
 * for reading and sets up the necessary @vi fields as well as initializing
 * the ntfs inode.
 *
 * Q: What locks are held when the function is called?
 * A: i_state has I_LOCK set, hence the inode is locked, also
 *    i_count is set to 1, so it is not going to go away
 *    i_flags is set to 0 and we have no business touching it. Only an ioctl()
 *    is allowed to write to them. We should of course be honouring them but
 *    we need to do that using the IS_* macros defined in include/linux/fs.h.
 *    In any case ntfs_read_locked_inode() has nothing to do with i_flags.
 *
 * Return 0 on success and -errno on error. In the error case, the inode will
 * have had make_bad_inode() executed on it.
 */
static int ntfs_read_locked_inode(struct inode *vi)
{
	ntfs_volume *vol = NTFS_SB(vi->i_sb);
	ntfs_inode *ni;
	MFT_RECORD *m;
	STANDARD_INFORMATION *si;
	attr_search_context *ctx;
	int err = 0;

	ntfs_debug("Entering for i_ino 0x%lx.", vi->i_ino);

	/* Setup the generic vfs inode parts now. */

	/* This is the optimal IO size (for stat), not the fs block size. */
	vi->i_blksize = PAGE_CACHE_SIZE;
	/*
	 * This is for checking whether an inode has changed w.r.t. a file so
	 * that the file can be updated if necessary (compare with f_version).
	 */
	vi->i_version = 1;

	vi->i_uid = vol->uid;
	vi->i_gid = vol->gid;
	vi->i_mode = 0;

	/*
	 * Initialize the ntfs specific part of @vi special casing
	 * FILE_MFT which we need to do at mount time.
	 */
	if (vi->i_ino != FILE_MFT)
		ntfs_init_big_inode(vi);
	ni = NTFS_I(vi);

	m = map_mft_record(ni);
	if (IS_ERR(m)) {
		err = PTR_ERR(m);
		goto err_out;
	}
	ctx = get_attr_search_ctx(ni, m);
	if (!ctx) {
		err = -ENOMEM;
		goto unm_err_out;
	}

	if (!(m->flags & MFT_RECORD_IN_USE)) {
		ntfs_error(vi->i_sb, "Inode is not in use! You should "
				"run chkdsk.");
		goto unm_err_out;
	}
	if (m->base_mft_record) {
		ntfs_error(vi->i_sb, "Inode is an extent inode! You should "
				"run chkdsk.");
		goto unm_err_out;
	}

	/* Transfer information from mft record into vfs and ntfs inodes. */
	ni->seq_no = le16_to_cpu(m->sequence_number);

	/*
	 * FIXME: Keep in mind that link_count is two for files which have both
	 * a long file name and a short file name as separate entries, so if
	 * we are hiding short file names this will be too high. Either we need
	 * to account for the short file names by subtracting them or we need
	 * to make sure we delete files even though i_nlink is not zero which
	 * might be tricky due to vfs interactions. Need to think about this
	 * some more when implementing the unlink command.
	 */
	vi->i_nlink = le16_to_cpu(m->link_count);
	/*
	 * FIXME: Reparse points can have the directory bit set even though
	 * they would be S_IFLNK. Need to deal with this further below when we
	 * implement reparse points / symbolic links but it will do for now.
	 * Also if not a directory, it could be something else, rather than
	 * a regular file. But again, will do for now.
	 */
	if (m->flags & MFT_RECORD_IS_DIRECTORY) {
		vi->i_mode |= S_IFDIR;
		/* Things break without this kludge! */
		if (vi->i_nlink > 1)
			vi->i_nlink = 1;
	} else
		vi->i_mode |= S_IFREG;

	/*
	 * Find the standard information attribute in the mft record. At this
	 * stage we haven't setup the attribute list stuff yet, so this could
	 * in fact fail if the standard information is in an extent record, but
	 * I don't think this actually ever happens.
	 */
	if (!lookup_attr(AT_STANDARD_INFORMATION, NULL, 0, 0, 0, NULL, 0,
			ctx)) {
		/*
		 * TODO: We should be performing a hot fix here (if the recover
		 * mount option is set) by creating a new attribute.
		 */
		ntfs_error(vi->i_sb, "$STANDARD_INFORMATION attribute is "
				"missing.");
		goto unm_err_out;
	}
	/* Get the standard information attribute value. */
	si = (STANDARD_INFORMATION*)((char*)ctx->attr +
			le16_to_cpu(ctx->attr->data.resident.value_offset));

	/* Transfer information from the standard information into vfs_ino. */
	/*
	 * Note: The i_?times do not quite map perfectly onto the NTFS times,
	 * but they are close enough, and in the end it doesn't really matter
	 * that much...
	 */
	/*
	 * mtime is the last change of the data within the file. Not changed
	 * when only metadata is changed, e.g. a rename doesn't affect mtime.
	 */
	vi->i_mtime.tv_sec = ntfs2utc(si->last_data_change_time);
	vi->i_mtime.tv_nsec = 0;
	/*
	 * ctime is the last change of the metadata of the file. This obviously
	 * always changes, when mtime is changed. ctime can be changed on its
	 * own, mtime is then not changed, e.g. when a file is renamed.
	 */
	vi->i_ctime.tv_sec = ntfs2utc(si->last_mft_change_time);
	vi->i_ctime.tv_nsec = 0;
	/*
	 * Last access to the data within the file. Not changed during a rename
	 * for example but changed whenever the file is written to.
	 */
	vi->i_atime.tv_sec = ntfs2utc(si->last_access_time);
	vi->i_atime.tv_nsec = 0;

	/* Find the attribute list attribute if present. */
	reinit_attr_search_ctx(ctx);
	if (lookup_attr(AT_ATTRIBUTE_LIST, NULL, 0, 0, 0, NULL, 0, ctx)) {
		if (vi->i_ino == FILE_MFT)
			goto skip_attr_list_load;
		ntfs_debug("Attribute list found in inode 0x%lx.", vi->i_ino);
		NInoSetAttrList(ni);
		if (ctx->attr->flags & ATTR_IS_ENCRYPTED ||
				ctx->attr->flags & ATTR_COMPRESSION_MASK ||
				ctx->attr->flags & ATTR_IS_SPARSE) {
			ntfs_error(vi->i_sb, "Attribute list attribute is "
					"compressed/encrypted/sparse. Not "
					"allowed. Corrupt inode. You should "
					"run chkdsk.");
			goto unm_err_out;
		}
		/* Now allocate memory for the attribute list. */
		ni->attr_list_size = (u32)attribute_value_length(ctx->attr);
		ni->attr_list = ntfs_malloc_nofs(ni->attr_list_size);
		if (!ni->attr_list) {
			ntfs_error(vi->i_sb, "Not enough memory to allocate "
					"buffer for attribute list.");
			err = -ENOMEM;
			goto unm_err_out;
		}
		if (ctx->attr->non_resident) {
			NInoSetAttrListNonResident(ni);
			if (ctx->attr->data.non_resident.lowest_vcn) {
				ntfs_error(vi->i_sb, "Attribute list has non "
						"zero lowest_vcn. Inode is "
						"corrupt. You should run "
						"chkdsk.");
				goto unm_err_out;
			}
			/*
			 * Setup the run list. No need for locking as we have
			 * exclusive access to the inode at this time.
			 */
			ni->attr_list_rl.rl = decompress_mapping_pairs(vol,
					ctx->attr, NULL);
			if (IS_ERR(ni->attr_list_rl.rl)) {
				err = PTR_ERR(ni->attr_list_rl.rl);
				ni->attr_list_rl.rl = NULL;
				ntfs_error(vi->i_sb, "Mapping pairs "
						"decompression failed with "
						"error code %i. Corrupt "
						"attribute list in inode.",
						-err);
				goto unm_err_out;
			}
			/* Now load the attribute list. */
			if ((err = load_attribute_list(vol, &ni->attr_list_rl,
					ni->attr_list, ni->attr_list_size,
					sle64_to_cpu(ctx->attr->data.
					non_resident.initialized_size)))) {
				ntfs_error(vi->i_sb, "Failed to load "
						"attribute list attribute.");
				goto unm_err_out;
			}
		} else /* if (!ctx.attr->non_resident) */ {
			if ((u8*)ctx->attr + le16_to_cpu(
					ctx->attr->data.resident.value_offset) +
					le32_to_cpu(
					ctx->attr->data.resident.value_length) >
					(u8*)ctx->mrec + vol->mft_record_size) {
				ntfs_error(vi->i_sb, "Corrupt attribute list "
						"in inode.");
				goto unm_err_out;
			}
			/* Now copy the attribute list. */
			memcpy(ni->attr_list, (u8*)ctx->attr + le16_to_cpu(
					ctx->attr->data.resident.value_offset),
					le32_to_cpu(
					ctx->attr->data.resident.value_length));
		}
	}
skip_attr_list_load:
	/*
	 * If an attribute list is present we now have the attribute list value
	 * in ntfs_ino->attr_list and it is ntfs_ino->attr_list_size bytes.
	 */
	if (S_ISDIR(vi->i_mode)) {
		struct inode *bvi;
		ntfs_inode *bni;
		INDEX_ROOT *ir;
		char *ir_end, *index_end;

		/* It is a directory, find index root attribute. */
		reinit_attr_search_ctx(ctx);
		if (!lookup_attr(AT_INDEX_ROOT, I30, 4, CASE_SENSITIVE, 0,
				NULL, 0, ctx)) {
			// FIXME: File is corrupt! Hot-fix with empty index
			// root attribute if recovery option is set.
			ntfs_error(vi->i_sb, "$INDEX_ROOT attribute is "
					"missing.");
			goto unm_err_out;
		}
		/* Set up the state. */
		if (ctx->attr->non_resident) {
			ntfs_error(vi->i_sb, "$INDEX_ROOT attribute is "
					"not resident. Not allowed.");
			goto unm_err_out;
		}
		/*
		 * Compressed/encrypted index root just means that the newly
		 * created files in that directory should be created compressed/
		 * encrypted. However index root cannot be both compressed and
		 * encrypted.
		 */
		if (ctx->attr->flags & ATTR_COMPRESSION_MASK)
			NInoSetCompressed(ni);
		if (ctx->attr->flags & ATTR_IS_ENCRYPTED) {
			if (ctx->attr->flags & ATTR_COMPRESSION_MASK) {
				ntfs_error(vi->i_sb, "Found encrypted and "
						"compressed attribute. Not "
						"allowed.");
				goto unm_err_out;
			}
			NInoSetEncrypted(ni);
		}
		if (ctx->attr->flags & ATTR_IS_SPARSE)
			NInoSetSparse(ni);
		ir = (INDEX_ROOT*)((char*)ctx->attr + le16_to_cpu(
				ctx->attr->data.resident.value_offset));
		ir_end = (char*)ir + le32_to_cpu(
				ctx->attr->data.resident.value_length);
		if (ir_end > (char*)ctx->mrec + vol->mft_record_size) {
			ntfs_error(vi->i_sb, "$INDEX_ROOT attribute is "
					"corrupt.");
			goto unm_err_out;
		}
		index_end = (char*)&ir->index +
				le32_to_cpu(ir->index.index_length);
		if (index_end > ir_end) {
			ntfs_error(vi->i_sb, "Directory index is corrupt.");
			goto unm_err_out;
		}
		if (ir->type != AT_FILE_NAME) {
			ntfs_error(vi->i_sb, "Indexed attribute is not "
					"$FILE_NAME. Not allowed.");
			goto unm_err_out;
		}
		if (ir->collation_rule != COLLATION_FILE_NAME) {
			ntfs_error(vi->i_sb, "Index collation rule is not "
					"COLLATION_FILE_NAME. Not allowed.");
			goto unm_err_out;
		}
		ni->itype.index.block_size = le32_to_cpu(ir->index_block_size);
		if (ni->itype.index.block_size &
				(ni->itype.index.block_size - 1)) {
			ntfs_error(vi->i_sb, "Index block size (%u) is not a "
					"power of two.",
					ni->itype.index.block_size);
			goto unm_err_out;
		}
		if (ni->itype.index.block_size > PAGE_CACHE_SIZE) {
			ntfs_error(vi->i_sb, "Index block size (%u) > "
					"PAGE_CACHE_SIZE (%ld) is not "
					"supported. Sorry.",
					ni->itype.index.block_size,
					PAGE_CACHE_SIZE);
			err = -EOPNOTSUPP;
			goto unm_err_out;
		}
		if (ni->itype.index.block_size < NTFS_BLOCK_SIZE) {
			ntfs_error(vi->i_sb, "Index block size (%u) < "
					"NTFS_BLOCK_SIZE (%i) is not "
					"supported. Sorry.",
					ni->itype.index.block_size,
					NTFS_BLOCK_SIZE);
			err = -EOPNOTSUPP;
			goto unm_err_out;
		}
		ni->itype.index.block_size_bits =
				ffs(ni->itype.index.block_size) - 1;
		/* Determine the size of a vcn in the directory index. */
		if (vol->cluster_size <= ni->itype.index.block_size) {
			ni->itype.index.vcn_size = vol->cluster_size;
			ni->itype.index.vcn_size_bits = vol->cluster_size_bits;
		} else {
			ni->itype.index.vcn_size = vol->sector_size;
			ni->itype.index.vcn_size_bits = vol->sector_size_bits;
		}

		/* Setup the index allocation attribute, even if not present. */
		NInoSetMstProtected(ni);
		ni->type = AT_INDEX_ALLOCATION;
		ni->name = I30;
		ni->name_len = 4;

		if (!(ir->index.flags & LARGE_INDEX)) {
			/* No index allocation. */
			vi->i_size = ni->initialized_size =
					ni->allocated_size = 0;
			/* We are done with the mft record, so we release it. */
			put_attr_search_ctx(ctx);
			unmap_mft_record(ni);
			m = NULL;
			ctx = NULL;
			goto skip_large_dir_stuff;
		} /* LARGE_INDEX: Index allocation present. Setup state. */
		NInoSetIndexAllocPresent(ni);
		/* Find index allocation attribute. */
		reinit_attr_search_ctx(ctx);
		if (!lookup_attr(AT_INDEX_ALLOCATION, I30, 4, CASE_SENSITIVE,
				0, NULL, 0, ctx)) {
			ntfs_error(vi->i_sb, "$INDEX_ALLOCATION attribute "
					"is not present but $INDEX_ROOT "
					"indicated it is.");
			goto unm_err_out;
		}
		if (!ctx->attr->non_resident) {
			ntfs_error(vi->i_sb, "$INDEX_ALLOCATION attribute "
					"is resident.");
			goto unm_err_out;
		}
		if (ctx->attr->flags & ATTR_IS_ENCRYPTED) {
			ntfs_error(vi->i_sb, "$INDEX_ALLOCATION attribute "
					"is encrypted.");
			goto unm_err_out;
		}
		if (ctx->attr->flags & ATTR_IS_SPARSE) {
			ntfs_error(vi->i_sb, "$INDEX_ALLOCATION attribute "
					"is sparse.");
			goto unm_err_out;
		}
		if (ctx->attr->flags & ATTR_COMPRESSION_MASK) {
			ntfs_error(vi->i_sb, "$INDEX_ALLOCATION attribute "
					"is compressed.");
			goto unm_err_out;
		}
		if (ctx->attr->data.non_resident.lowest_vcn) {
			ntfs_error(vi->i_sb, "First extent of "
					"$INDEX_ALLOCATION attribute has non "
					"zero lowest_vcn. Inode is corrupt. "
					"You should run chkdsk.");
			goto unm_err_out;
		}
		vi->i_size = sle64_to_cpu(
				ctx->attr->data.non_resident.data_size);
		ni->initialized_size = sle64_to_cpu(
				ctx->attr->data.non_resident.initialized_size);
		ni->allocated_size = sle64_to_cpu(
				ctx->attr->data.non_resident.allocated_size);
		/*
		 * We are done with the mft record, so we release it. Otherwise
		 * we would deadlock in ntfs_attr_iget().
		 */
		put_attr_search_ctx(ctx);
		unmap_mft_record(ni);
		m = NULL;
		ctx = NULL;
		/* Get the index bitmap attribute inode. */
		bvi = ntfs_attr_iget(vi, AT_BITMAP, I30, 4);
		if (unlikely(IS_ERR(bvi))) {
			ntfs_error(vi->i_sb, "Failed to get bitmap attribute.");
			err = PTR_ERR(bvi);
			goto unm_err_out;
		}
		ni->itype.index.bmp_ino = bvi;
		bni = NTFS_I(bvi);
		if (NInoCompressed(bni) || NInoEncrypted(bni) ||
				NInoSparse(bni)) {
			ntfs_error(vi->i_sb, "$BITMAP attribute is compressed "
					"and/or encrypted and/or sparse.");
			goto unm_err_out;
		}
		/* Consistency check bitmap size vs. index allocation size. */
		if ((bvi->i_size << 3) < (vi->i_size >>
				ni->itype.index.block_size_bits)) {
			ntfs_error(vi->i_sb, "Index bitmap too small (0x%Lx) "
					"for index allocation (0x%Lx).",
					bvi->i_size << 3, vi->i_size);
			goto unm_err_out;
		}
skip_large_dir_stuff:
		/* Everyone gets read and scan permissions. */
		vi->i_mode |= S_IRUGO | S_IXUGO;
		/* If not read-only, set write permissions. */
		if (!IS_RDONLY(vi))
			vi->i_mode |= S_IWUGO;
		/*
		 * Apply the directory permissions mask set in the mount
		 * options.
		 */
		vi->i_mode &= ~vol->dmask;
		/* Setup the operations for this inode. */
		vi->i_op = &ntfs_dir_inode_ops;
		vi->i_fop = &ntfs_dir_ops;
		vi->i_mapping->a_ops = &ntfs_aops;
	} else {
		/* It is a file. */
		reinit_attr_search_ctx(ctx);

		/* Setup the data attribute, even if not present. */
		ni->type = AT_DATA;
		ni->name = NULL;
		ni->name_len = 0;

		/* Find first extent of the unnamed data attribute. */
		if (!lookup_attr(AT_DATA, NULL, 0, 0, 0, NULL, 0, ctx)) {
			vi->i_size = ni->initialized_size =
					ni->allocated_size = 0LL;
			/*
			 * FILE_Secure does not have an unnamed $DATA
			 * attribute, so we special case it here.
			 */
			if (vi->i_ino == FILE_Secure)
				goto no_data_attr_special_case;
			/*
			 * Most if not all the system files in the $Extend
			 * system directory do not have unnamed data
			 * attributes so we need to check if the parent
			 * directory of the file is FILE_Extend and if it is
			 * ignore this error. To do this we need to get the
			 * name of this inode from the mft record as the name
			 * contains the back reference to the parent directory.
			 */
			if (ntfs_is_extended_system_file(ctx) > 0)
				goto no_data_attr_special_case;
			// FIXME: File is corrupt! Hot-fix with empty data
			// attribute if recovery option is set.
			ntfs_error(vi->i_sb, "$DATA attribute is "
					"missing.");
			goto unm_err_out;
		}
		/* Setup the state. */
		if (ctx->attr->non_resident) {
			NInoSetNonResident(ni);
			if (ctx->attr->flags & ATTR_COMPRESSION_MASK) {
				NInoSetCompressed(ni);
				if (vol->cluster_size > 4096) {
					ntfs_error(vi->i_sb, "Found "
						"compressed data but "
						"compression is disabled due "
						"to cluster size (%i) > 4kiB.",
						vol->cluster_size);
					goto unm_err_out;
				}
				if ((ctx->attr->flags & ATTR_COMPRESSION_MASK)
						!= ATTR_IS_COMPRESSED) {
					ntfs_error(vi->i_sb, "Found "
						"unknown compression method or "
						"corrupt file.");
					goto unm_err_out;
				}
				ni->itype.compressed.block_clusters = 1U <<
						ctx->attr->data.non_resident.
						compression_unit;
				if (ctx->attr->data.non_resident.
						compression_unit != 4) {
					ntfs_error(vi->i_sb, "Found "
						"nonstandard compression unit "
						"(%u instead of 4). Cannot "
						"handle this. This might "
						"indicate corruption so you "
						"should run chkdsk.",
						ctx->attr->data.non_resident.
						compression_unit);
					err = -EOPNOTSUPP;
					goto unm_err_out;
				}
				ni->itype.compressed.block_size = 1U << (
						ctx->attr->data.non_resident.
						compression_unit +
						vol->cluster_size_bits);
				ni->itype.compressed.block_size_bits = ffs(
					ni->itype.compressed.block_size) - 1;
			}
			if (ctx->attr->flags & ATTR_IS_ENCRYPTED) {
				if (ctx->attr->flags & ATTR_COMPRESSION_MASK) {
					ntfs_error(vi->i_sb, "Found encrypted "
							"and compressed data.");
					goto unm_err_out;
				}
				NInoSetEncrypted(ni);
			}
			if (ctx->attr->flags & ATTR_IS_SPARSE)
				NInoSetSparse(ni);
			if (ctx->attr->data.non_resident.lowest_vcn) {
				ntfs_error(vi->i_sb, "First extent of $DATA "
						"attribute has non zero "
						"lowest_vcn. Inode is corrupt. "
						"You should run chkdsk.");
				goto unm_err_out;
			}
			/* Setup all the sizes. */
			vi->i_size = sle64_to_cpu(
					ctx->attr->data.non_resident.data_size);
			ni->initialized_size = sle64_to_cpu(
					ctx->attr->data.non_resident.
					initialized_size);
			ni->allocated_size = sle64_to_cpu(
					ctx->attr->data.non_resident.
					allocated_size);
			if (NInoCompressed(ni)) {
				ni->itype.compressed.size = sle64_to_cpu(
						ctx->attr->data.non_resident.
						compressed_size);
			}
		} else { /* Resident attribute. */
			/*
			 * Make all sizes equal for simplicity in read code
			 * paths. FIXME: Need to keep this in mind when
			 * converting to non-resident attribute in write code
			 * path. (Probably only affects truncate().)
			 */
			vi->i_size = ni->initialized_size = ni->allocated_size =
					le32_to_cpu(
					ctx->attr->data.resident.value_length);
		}
no_data_attr_special_case:
		/* We are done with the mft record, so we release it. */
		put_attr_search_ctx(ctx);
		unmap_mft_record(ni);
		m = NULL;
		ctx = NULL;
		/* Everyone gets all permissions. */
		vi->i_mode |= S_IRWXUGO;
		/* If read-only, noone gets write permissions. */
		if (IS_RDONLY(vi))
			vi->i_mode &= ~S_IWUGO;
		/* Apply the file permissions mask set in the mount options. */
		vi->i_mode &= ~vol->fmask;
		/* Setup the operations for this inode. */
		vi->i_op = &ntfs_file_inode_ops;
		vi->i_fop = &ntfs_file_ops;
		vi->i_mapping->a_ops = &ntfs_aops;
	}
	/*
	 * The number of 512-byte blocks used on disk (for stat). This is in so
	 * far inaccurate as it doesn't account for any named streams or other
	 * special non-resident attributes, but that is how Windows works, too,
	 * so we are at least consistent with Windows, if not entirely
	 * consistent with the Linux Way. Doing it the Linux Way would cause a
	 * significant slowdown as it would involve iterating over all
	 * attributes in the mft record and adding the allocated/compressed
	 * sizes of all non-resident attributes present to give us the Linux
	 * correct size that should go into i_blocks (after division by 512).
	 */
	if (!NInoCompressed(ni))
		vi->i_blocks = ni->allocated_size >> 9;
	else
		vi->i_blocks = ni->itype.compressed.size >> 9;

	ntfs_debug("Done.");
	return 0;

unm_err_out:
	if (!err)
		err = -EIO;
	if (ctx)
		put_attr_search_ctx(ctx);
	if (m)
		unmap_mft_record(ni);
err_out:
	ntfs_error(vi->i_sb, "Failed with error code %i. Marking inode 0x%lx "
			"as bad.", -err, vi->i_ino);
	make_bad_inode(vi);
	return err;
}

/**
 * ntfs_read_locked_attr_inode - read an attribute inode from its base inode
 * @base_vi:	base inode
 * @vi:		attribute inode to read
 *
 * ntfs_read_locked_attr_inode() is called from the ntfs_attr_iget() to read
 * the attribute inode described by @vi into memory from the base mft record
 * described by @base_ni.
 *
 * ntfs_read_locked_attr_inode() maps, pins and locks the base inode for
 * reading and looks up the attribute described by @vi before setting up the
 * necessary fields in @vi as well as initializing the ntfs inode.
 *
 * Q: What locks are held when the function is called?
 * A: i_state has I_LOCK set, hence the inode is locked, also
 *    i_count is set to 1, so it is not going to go away
 */
static int ntfs_read_locked_attr_inode(struct inode *base_vi, struct inode *vi)
{
	ntfs_volume *vol = NTFS_SB(vi->i_sb);
	ntfs_inode *ni, *base_ni;
	MFT_RECORD *m;
	attr_search_context *ctx;
	int err = 0;

	ntfs_debug("Entering for i_ino 0x%lx.", vi->i_ino);

	ntfs_init_big_inode(vi);

	ni	= NTFS_I(vi);
	base_ni = NTFS_I(base_vi);

	/* Just mirror the values from the base inode. */
	vi->i_blksize	= base_vi->i_blksize;
	vi->i_version	= base_vi->i_version;
	vi->i_uid	= base_vi->i_uid;
	vi->i_gid	= base_vi->i_gid;
	vi->i_nlink	= base_vi->i_nlink;
	vi->i_mtime	= base_vi->i_mtime;
	vi->i_ctime	= base_vi->i_ctime;
	vi->i_atime	= base_vi->i_atime;
	ni->seq_no	= base_ni->seq_no;

	/* Set inode type to zero but preserve permissions. */
	vi->i_mode	= base_vi->i_mode & ~S_IFMT;

	m = map_mft_record(base_ni);
	if (IS_ERR(m)) {
		err = PTR_ERR(m);
		goto err_out;
	}
	ctx = get_attr_search_ctx(base_ni, m);
	if (!ctx) {
		err = -ENOMEM;
		goto unm_err_out;
	}

	/* Find the attribute. */
	if (!lookup_attr(ni->type, ni->name, ni->name_len, IGNORE_CASE, 0,
			NULL, 0, ctx))
		goto unm_err_out;

	if (!ctx->attr->non_resident) {
		if (NInoMstProtected(ni) || ctx->attr->flags) {
			ntfs_error(vi->i_sb, "Found mst protected attribute "
					"or attribute with non-zero flags but "
					"the attribute is resident (mft_no "
					"0x%lx, type 0x%x, name_len %i). "
					"Please report you saw this message "
					"to linux-ntfs-dev@lists.sf.net",
					vi->i_ino, ni->type, ni->name_len);
			goto unm_err_out;
		}
		/*
		 * Resident attribute. Make all sizes equal for simplicity in
		 * read code paths.
		 */
		vi->i_size = ni->initialized_size = ni->allocated_size =
			le32_to_cpu(ctx->attr->data.resident.value_length);
	} else {
		NInoSetNonResident(ni);
		if (ctx->attr->flags & ATTR_COMPRESSION_MASK) {
			if (NInoMstProtected(ni)) {
				ntfs_error(vi->i_sb, "Found mst protected "
						"attribute but the attribute "
						"is compressed (mft_no 0x%lx, "
						"type 0x%x, name_len %i). "
						"Please report you saw this "
						"message to linux-ntfs-dev@"
						"lists.sf.net", vi->i_ino,
						ni->type, ni->name_len);
				goto unm_err_out;
			}
			NInoSetCompressed(ni);
			if ((ni->type != AT_DATA) || (ni->type == AT_DATA &&
					ni->name_len)) {
				ntfs_error(vi->i_sb, "Found compressed non-"
						"data or named data attribute "
						"(mft_no 0x%lx, type 0x%x, "
						"name_len %i). Please report "
						"you saw this message to "
						"linux-ntfs-dev@lists.sf.net",
						vi->i_ino, ni->type,
						ni->name_len);
				goto unm_err_out;
			}
			if (vol->cluster_size > 4096) {
				ntfs_error(vi->i_sb, "Found "
					"compressed attribute but "
					"compression is disabled due "
					"to cluster size (%i) > 4kiB.",
					vol->cluster_size);
				goto unm_err_out;
			}
			if ((ctx->attr->flags & ATTR_COMPRESSION_MASK)
					!= ATTR_IS_COMPRESSED) {
				ntfs_error(vi->i_sb, "Found unknown "
						"compression method or "
						"corrupt file.");
				goto unm_err_out;
			}
			ni->itype.compressed.block_clusters = 1U <<
					ctx->attr->data.non_resident.
					compression_unit;
			if (ctx->attr->data.non_resident.compression_unit != 4) {
				ntfs_error(vi->i_sb, "Found "
					"nonstandard compression unit "
					"(%u instead of 4). Cannot "
					"handle this. This might "
					"indicate corruption so you "
					"should run chkdsk.",
					ctx->attr->data.non_resident.
					compression_unit);
				err = -EOPNOTSUPP;
				goto unm_err_out;
			}
			ni->itype.compressed.block_size = 1U << (
					ctx->attr->data.non_resident.
					compression_unit +
					vol->cluster_size_bits);
			ni->itype.compressed.block_size_bits = ffs(
				ni->itype.compressed.block_size) - 1;
		}
		if (ctx->attr->flags & ATTR_IS_ENCRYPTED) {
			if (ctx->attr->flags & ATTR_COMPRESSION_MASK) {
				ntfs_error(vi->i_sb, "Found encrypted "
						"and compressed data.");
				goto unm_err_out;
			}
			if (NInoMstProtected(ni)) {
				ntfs_error(vi->i_sb, "Found mst protected "
						"attribute but the attribute "
						"is encrypted (mft_no 0x%lx, "
						"type 0x%x, name_len %i). "
						"Please report you saw this "
						"message to linux-ntfs-dev@"
						"lists.sf.net", vi->i_ino,
						ni->type, ni->name_len);
				goto unm_err_out;
			}
			NInoSetEncrypted(ni);
		}
		if (ctx->attr->flags & ATTR_IS_SPARSE) {
			if (NInoMstProtected(ni)) {
				ntfs_error(vi->i_sb, "Found mst protected "
						"attribute but the attribute "
						"is sparse (mft_no 0x%lx, "
						"type 0x%x, name_len %i). "
						"Please report you saw this "
						"message to linux-ntfs-dev@"
						"lists.sf.net", vi->i_ino,
						ni->type, ni->name_len);
				goto unm_err_out;
			}
			NInoSetSparse(ni);
		}
		if (ctx->attr->data.non_resident.lowest_vcn) {
			ntfs_error(vi->i_sb, "First extent of attribute has "
					"non-zero lowest_vcn. Inode is "
					"corrupt. You should run chkdsk.");
			goto unm_err_out;
		}
		/* Setup all the sizes. */
		vi->i_size = sle64_to_cpu(
				ctx->attr->data.non_resident.data_size);
		ni->initialized_size = sle64_to_cpu(
				ctx->attr->data.non_resident.initialized_size);
		ni->allocated_size = sle64_to_cpu(
				ctx->attr->data.non_resident.allocated_size);
		if (NInoCompressed(ni)) {
			ni->itype.compressed.size = sle64_to_cpu(
					ctx->attr->data.non_resident.
					compressed_size);
		}
	}

	/* Setup the operations for this attribute inode. */
	vi->i_op = NULL;
	vi->i_fop = NULL;
	vi->i_mapping->a_ops = &ntfs_aops;

	if (!NInoCompressed(ni))
		vi->i_blocks = ni->allocated_size >> 9;
	else
		vi->i_blocks = ni->itype.compressed.size >> 9;

	/*
	 * Make sure the base inode doesn't go away and attach it to the
	 * attribute inode.
	 */
	igrab(base_vi);
	ni->ext.base_ntfs_ino = base_ni;
	ni->nr_extents = -1;

	put_attr_search_ctx(ctx);
	unmap_mft_record(base_ni);

	ntfs_debug("Done.");
	return 0;

unm_err_out:
	if (!err)
		err = -EIO;
	if (ctx)
		put_attr_search_ctx(ctx);
	unmap_mft_record(base_ni);
err_out:
	ntfs_error(vi->i_sb, "Failed with error code %i while reading "
			"attribute inode (mft_no 0x%lx, type 0x%x, name_len "
			"%i.", -err, vi->i_ino, ni->type, ni->name_len);
	make_bad_inode(vi);
	return err;
}

/**
 * ntfs_read_inode_mount - special read_inode for mount time use only
 * @vi:		inode to read
 *
 * Read inode FILE_MFT at mount time, only called with super_block lock
 * held from within the read_super() code path.
 *
 * This function exists because when it is called the page cache for $MFT/$DATA
 * is not initialized and hence we cannot get at the contents of mft records
 * by calling map_mft_record*().
 *
 * Further it needs to cope with the circular references problem, i.e. can't
 * load any attributes other than $ATTRIBUTE_LIST until $DATA is loaded, because
 * we don't know where the other extent mft records are yet and again, because
 * we cannot call map_mft_record*() yet. Obviously this applies only when an
 * attribute list is actually present in $MFT inode.
 *
 * We solve these problems by starting with the $DATA attribute before anything
 * else and iterating using lookup_attr($DATA) over all extents. As each extent
 * is found, we decompress_mapping_pairs() including the implied
 * merge_run_lists(). Each step of the iteration necessarily provides
 * sufficient information for the next step to complete.
 *
 * This should work but there are two possible pit falls (see inline comments
 * below), but only time will tell if they are real pits or just smoke...
 */
void ntfs_read_inode_mount(struct inode *vi)
{
	VCN next_vcn, last_vcn, highest_vcn;
	s64 block;
	struct super_block *sb = vi->i_sb;
	ntfs_volume *vol = NTFS_SB(sb);
	struct buffer_head *bh;
	ntfs_inode *ni;
	MFT_RECORD *m = NULL;
	ATTR_RECORD *attr;
	attr_search_context *ctx;
	unsigned int i, nr_blocks;
	int err;

	ntfs_debug("Entering.");

	if (vi->i_ino != FILE_MFT) {
		ntfs_error(sb, "Called for inode 0x%lx but only inode %d "
				"allowed.", vi->i_ino, FILE_MFT);
		goto err_out;
	}

	/* Initialize the ntfs specific part of @vi. */
	ntfs_init_big_inode(vi);

	ni = NTFS_I(vi);

	/* Setup the data attribute. It is special as it is mst protected. */
	NInoSetNonResident(ni);
	NInoSetMstProtected(ni);
	ni->type = AT_DATA;
	ni->name = NULL;
	ni->name_len = 0;

	/*
	 * This sets up our little cheat allowing us to reuse the async io
	 * completion handler for directories.
	 */
	ni->itype.index.block_size = vol->mft_record_size;
	ni->itype.index.block_size_bits = vol->mft_record_size_bits;

	/* Very important! Needed to be able to call map_mft_record*(). */
	vol->mft_ino = vi;

	/* Allocate enough memory to read the first mft record. */
	if (vol->mft_record_size > 64 * 1024) {
		ntfs_error(sb, "Unsupported mft record size %i (max 64kiB).",
				vol->mft_record_size);
		goto err_out;
	}
	i = vol->mft_record_size;
	if (i < sb->s_blocksize)
		i = sb->s_blocksize;
	m = (MFT_RECORD*)ntfs_malloc_nofs(i);
	if (!m) {
		ntfs_error(sb, "Failed to allocate buffer for $MFT record 0.");
		goto err_out;
	}

	/* Determine the first block of the $MFT/$DATA attribute. */
	block = vol->mft_lcn << vol->cluster_size_bits >>
			sb->s_blocksize_bits;
	nr_blocks = vol->mft_record_size >> sb->s_blocksize_bits;
	if (!nr_blocks)
		nr_blocks = 1;

	/* Load $MFT/$DATA's first mft record. */
	for (i = 0; i < nr_blocks; i++) {
		bh = sb_bread(sb, block++);
		if (!bh) {
			ntfs_error(sb, "Device read failed.");
			goto err_out;
		}
		memcpy((char*)m + (i << sb->s_blocksize_bits), bh->b_data,
				sb->s_blocksize);
		brelse(bh);
	}

	/* Apply the mst fixups. */
	if (post_read_mst_fixup((NTFS_RECORD*)m, vol->mft_record_size)) {
		/* FIXME: Try to use the $MFTMirr now. */
		ntfs_error(sb, "MST fixup failed. $MFT is corrupt.");
		goto err_out;
	}

	/* Need this to sanity check attribute list references to $MFT. */
	ni->seq_no = le16_to_cpu(m->sequence_number);

	/* Provides readpage() and sync_page() for map_mft_record(). */
	vi->i_mapping->a_ops = &ntfs_mft_aops;

	ctx = get_attr_search_ctx(ni, m);
	if (!ctx) {
		err = -ENOMEM;
		goto err_out;
	}

	/* Find the attribute list attribute if present. */
	if (lookup_attr(AT_ATTRIBUTE_LIST, NULL, 0, 0, 0, NULL, 0, ctx)) {
		ATTR_LIST_ENTRY *al_entry, *next_al_entry;
		u8 *al_end;

		ntfs_debug("Attribute list attribute found in $MFT.");
		NInoSetAttrList(ni);
		if (ctx->attr->flags & ATTR_IS_ENCRYPTED ||
				ctx->attr->flags & ATTR_COMPRESSION_MASK ||
				ctx->attr->flags & ATTR_IS_SPARSE) {
			ntfs_error(sb, "Attribute list attribute is "
					"compressed/encrypted/sparse. Not "
					"allowed. $MFT is corrupt. You should "
					"run chkdsk.");
			goto put_err_out;
		}
		/* Now allocate memory for the attribute list. */
		ni->attr_list_size = (u32)attribute_value_length(ctx->attr);
		ni->attr_list = ntfs_malloc_nofs(ni->attr_list_size);
		if (!ni->attr_list) {
			ntfs_error(sb, "Not enough memory to allocate buffer "
					"for attribute list.");
			goto put_err_out;
		}
		if (ctx->attr->non_resident) {
			NInoSetAttrListNonResident(ni);
			if (ctx->attr->data.non_resident.lowest_vcn) {
				ntfs_error(sb, "Attribute list has non zero "
						"lowest_vcn. $MFT is corrupt. "
						"You should run chkdsk.");
				goto put_err_out;
			}
			/* Setup the run list. */
			ni->attr_list_rl.rl = decompress_mapping_pairs(vol,
					ctx->attr, NULL);
			if (IS_ERR(ni->attr_list_rl.rl)) {
				err = PTR_ERR(ni->attr_list_rl.rl);
				ni->attr_list_rl.rl = NULL;
				ntfs_error(sb, "Mapping pairs decompression "
						"failed with error code %i.",
						-err);
				goto put_err_out;
			}
			/* Now load the attribute list. */
			if ((err = load_attribute_list(vol, &ni->attr_list_rl,
					ni->attr_list, ni->attr_list_size,
					sle64_to_cpu(ctx->attr->data.
					non_resident.initialized_size)))) {
				ntfs_error(sb, "Failed to load attribute list "
						"attribute with error code %i.",
						-err);
				goto put_err_out;
			}
		} else /* if (!ctx.attr->non_resident) */ {
			if ((u8*)ctx->attr + le16_to_cpu(
					ctx->attr->data.resident.value_offset) +
					le32_to_cpu(
					ctx->attr->data.resident.value_length) >
					(u8*)ctx->mrec + vol->mft_record_size) {
				ntfs_error(sb, "Corrupt attribute list "
						"attribute.");
				goto put_err_out;
			}
			/* Now copy the attribute list. */
			memcpy(ni->attr_list, (u8*)ctx->attr + le16_to_cpu(
					ctx->attr->data.resident.value_offset),
					le32_to_cpu(
					ctx->attr->data.resident.value_length));
		}
		/* The attribute list is now setup in memory. */
		/*
		 * FIXME: I don't know if this case is actually possible.
		 * According to logic it is not possible but I have seen too
		 * many weird things in MS software to rely on logic... Thus we
		 * perform a manual search and make sure the first $MFT/$DATA
		 * extent is in the base inode. If it is not we abort with an
		 * error and if we ever see a report of this error we will need
		 * to do some magic in order to have the necessary mft record
		 * loaded and in the right place in the page cache. But
		 * hopefully logic will prevail and this never happens...
		 */
		al_entry = (ATTR_LIST_ENTRY*)ni->attr_list;
		al_end = (u8*)al_entry + ni->attr_list_size;
		for (;; al_entry = next_al_entry) {
			/* Out of bounds check. */
			if ((u8*)al_entry < ni->attr_list ||
					(u8*)al_entry > al_end)
				goto em_put_err_out;
			/* Catch the end of the attribute list. */
			if ((u8*)al_entry == al_end)
				goto em_put_err_out;
			if (!al_entry->length)
				goto em_put_err_out;
			if ((u8*)al_entry + 6 > al_end || (u8*)al_entry +
					le16_to_cpu(al_entry->length) > al_end)
				goto em_put_err_out;
			next_al_entry = (ATTR_LIST_ENTRY*)((u8*)al_entry +
					le16_to_cpu(al_entry->length));
			if (le32_to_cpu(al_entry->type) >
					const_le32_to_cpu(AT_DATA))
				goto em_put_err_out;
			if (AT_DATA != al_entry->type)
				continue;
			/* We want an unnamed attribute. */
			if (al_entry->name_length)
				goto em_put_err_out;
			/* Want the first entry, i.e. lowest_vcn == 0. */
			if (al_entry->lowest_vcn)
				goto em_put_err_out;
			/* First entry has to be in the base mft record. */
			if (MREF_LE(al_entry->mft_reference) != vi->i_ino) {
				/* MFT references do not match, logic fails. */
				ntfs_error(sb, "BUG: The first $DATA extent "
						"of $MFT is not in the base "
						"mft record. Please report "
						"you saw this message to "
						"linux-ntfs-dev@lists.sf.net");
				goto put_err_out;
			} else {
				/* Sequence numbers must match. */
				if (MSEQNO_LE(al_entry->mft_reference) !=
						ni->seq_no)
					goto em_put_err_out;
				/* Got it. All is ok. We can stop now. */
				break;
			}
		}
	}

	reinit_attr_search_ctx(ctx);

	/* Now load all attribute extents. */
	attr = NULL;
	next_vcn = last_vcn = highest_vcn = 0;
	while (lookup_attr(AT_DATA, NULL, 0, 0, next_vcn, NULL, 0, ctx)) {
		run_list_element *nrl;

		/* Cache the current attribute. */
		attr = ctx->attr;
		/* $MFT must be non-resident. */
		if (!attr->non_resident) {
			ntfs_error(sb, "$MFT must be non-resident but a "
					"resident extent was found. $MFT is "
					"corrupt. Run chkdsk.");
			goto put_err_out;
		}
		/* $MFT must be uncompressed and unencrypted. */
		if (attr->flags & ATTR_COMPRESSION_MASK ||
				attr->flags & ATTR_IS_ENCRYPTED ||
				attr->flags & ATTR_IS_SPARSE) {
			ntfs_error(sb, "$MFT must be uncompressed, "
					"non-sparse, and unencrypted but a "
					"compressed/sparse/encrypted extent "
					"was found. $MFT is corrupt. Run "
					"chkdsk.");
			goto put_err_out;
		}
		/*
		 * Decompress the mapping pairs array of this extent and merge
		 * the result into the existing run list. No need for locking
		 * as we have exclusive access to the inode at this time and we
		 * are a mount in progress task, too.
		 */
		nrl = decompress_mapping_pairs(vol, attr, ni->run_list.rl);
		if (IS_ERR(nrl)) {
			ntfs_error(sb, "decompress_mapping_pairs() failed with "
					"error code %ld. $MFT is corrupt.",
					PTR_ERR(nrl));
			goto put_err_out;
		}
		ni->run_list.rl = nrl;

		/* Are we in the first extent? */
		if (!next_vcn) {
			u64 ll;

			if (attr->data.non_resident.lowest_vcn) {
				ntfs_error(sb, "First extent of $DATA "
						"attribute has non zero "
						"lowest_vcn. $MFT is corrupt. "
						"You should run chkdsk.");
				goto put_err_out;
			}
			/* Get the last vcn in the $DATA attribute. */
			last_vcn = sle64_to_cpu(
					attr->data.non_resident.allocated_size)
					>> vol->cluster_size_bits;
			/* Fill in the inode size. */
			vi->i_size = sle64_to_cpu(
					attr->data.non_resident.data_size);
			ni->initialized_size = sle64_to_cpu(attr->data.
					non_resident.initialized_size);
			ni->allocated_size = sle64_to_cpu(
					attr->data.non_resident.allocated_size);
			/* Set the number of mft records. */
			ll = vi->i_size >> vol->mft_record_size_bits;
			/*
			 * Verify the number of mft records does not exceed
			 * 2^32 - 1.
			 */
			if (ll >= (1ULL << 32)) {
				ntfs_error(sb, "$MFT is too big! Aborting.");
				goto put_err_out;
			}
			vol->nr_mft_records = ll;
			/*
			 * We have got the first extent of the run_list for
			 * $MFT which means it is now relatively safe to call
			 * the normal ntfs_read_inode() function. Thus, take
			 * us out of the calling chain. Also we need to do this
			 * now because we need ntfs_read_inode() in place to
			 * get at subsequent extents.
			 */
			sb->s_op = &ntfs_sops;
			/*
			 * Complete reading the inode, this will actually
			 * re-read the mft record for $MFT, this time entering
			 * it into the page cache with which we complete the
			 * kick start of the volume. It should be safe to do
			 * this now as the first extent of $MFT/$DATA is
			 * already known and we would hope that we don't need
			 * further extents in order to find the other
			 * attributes belonging to $MFT. Only time will tell if
			 * this is really the case. If not we will have to play
			 * magic at this point, possibly duplicating a lot of
			 * ntfs_read_inode() at this point. We will need to
			 * ensure we do enough of its work to be able to call
			 * ntfs_read_inode() on extents of $MFT/$DATA. But lets
			 * hope this never happens...
			 */
			ntfs_read_locked_inode(vi);
			if (is_bad_inode(vi)) {
				ntfs_error(sb, "ntfs_read_inode() of $MFT "
						"failed. BUG or corrupt $MFT. "
						"Run chkdsk and if no errors "
						"are found, please report you "
						"saw this message to "
						"linux-ntfs-dev@lists.sf.net");
				put_attr_search_ctx(ctx);
				/* Revert to the safe super operations. */
				sb->s_op = &ntfs_mount_sops;
				goto out_now;
			}
			/*
			 * Re-initialize some specifics about $MFT's inode as
			 * ntfs_read_inode() will have set up the default ones.
			 */
			/* Set uid and gid to root. */
			vi->i_uid = vi->i_gid = 0;
			/* Regular file. No access for anyone. */
			vi->i_mode = S_IFREG;
			/* No VFS initiated operations allowed for $MFT. */
			vi->i_op = &ntfs_empty_inode_ops;
			vi->i_fop = &ntfs_empty_file_ops;
			/* Put back our special address space operations. */
			vi->i_mapping->a_ops = &ntfs_mft_aops;
		}

		/* Get the lowest vcn for the next extent. */
		highest_vcn = sle64_to_cpu(attr->data.non_resident.highest_vcn);
		next_vcn = highest_vcn + 1;

		/* Only one extent or error, which we catch below. */
		if (next_vcn <= 0)
			break;

		/* Avoid endless loops due to corruption. */
		if (next_vcn < sle64_to_cpu(
				attr->data.non_resident.lowest_vcn)) {
			ntfs_error(sb, "$MFT has corrupt attribute list "
					"attribute. Run chkdsk.");
			goto put_err_out;
		}
	}
	if (!attr) {
		ntfs_error(sb, "$MFT/$DATA attribute not found. $MFT is "
				"corrupt. Run chkdsk.");
		goto put_err_out;
	}
	if (highest_vcn && highest_vcn != last_vcn - 1) {
		ntfs_error(sb, "Failed to load the complete run list "
				"for $MFT/$DATA. Driver bug or "
				"corrupt $MFT. Run chkdsk.");
		ntfs_debug("highest_vcn = 0x%Lx, last_vcn - 1 = 0x%Lx",
				(long long)highest_vcn,
				(long long)last_vcn - 1);
		goto put_err_out;
	}
	put_attr_search_ctx(ctx);
	ntfs_debug("Done.");
out_now:
	ntfs_free(m);
	return;
em_put_err_out:
	ntfs_error(sb, "Couldn't find first extent of $DATA attribute in "
			"attribute list. $MFT is corrupt. Run chkdsk.");
put_err_out:
	put_attr_search_ctx(ctx);
err_out:
	/* Make sure we revert to the safe super operations. */
	sb->s_op = &ntfs_mount_sops;
	ntfs_error(sb, "Failed. Marking inode as bad.");
	make_bad_inode(vi);
	goto out_now;
}

/**
 * ntfs_dirty_inode - mark the inode's metadata dirty
 * @vi:		inode to mark dirty
 *
 * This is called from fs/inode.c::__mark_inode_dirty(), when the inode itself
 * is being marked dirty. An example is when update_atime() is invoked.
 *
 * We mark the inode dirty by setting both the page in which the mft record
 * resides and the buffer heads in that page which correspond to the mft record
 * dirty. This ensures that the changes will eventually be propagated to disk
 * when the inode is set dirty.
 *
 * FIXME: Can we do that with the buffer heads? I am not too sure. Because if we
 * do that we need to make sure that the kernel will not write out those buffer
 * heads or we are screwed as it will write corrupt data to disk. The only way
 * a mft record can be written correctly is by mst protecting it, writting it
 * synchronously and fast mst deprotecting it. During this period, obviously,
 * the mft record must be marked as not uptodate, be locked for writing or
 * whatever, so that nobody attempts anything stupid.
 *
 * FIXME: Do we need to check that the fs is not mounted read only? And what
 * about the inode? Anything else?
 *
 * FIXME: As we are only a read only driver it is safe to just return here for
 * the moment.
 */
void ntfs_dirty_inode(struct inode *vi)
{
	ntfs_debug("Entering for inode 0x%lx.", vi->i_ino);
	NInoSetDirty(NTFS_I(vi));
	return;
}

/**
 * ntfs_commit_inode - write out a dirty inode
 * @ni:		inode to write out
 *
 */
int ntfs_commit_inode(ntfs_inode *ni)
{
	ntfs_debug("Entering for inode 0x%lx.", ni->mft_no);
	NInoClearDirty(ni);
	return 0;
}

/**
 * ntfs_put_inode - handler for when the inode reference count is decremented
 * @vi:		vfs inode
 *
 * The VFS calls ntfs_put_inode() every time the inode reference count (i_count)
 * is about to be decremented (but before the decrement itself.
 *
 * If the inode @vi is a directory with a single reference, we need to put the
 * attribute inode for the directory index bitmap, if it is present, otherwise
 * the directory inode would remain pinned for ever (or rather until umount()
 * time.
 */
void ntfs_put_inode(struct inode *vi)
{
	if (S_ISDIR(vi->i_mode) && (atomic_read(&vi->i_count) == 2)) {
		ntfs_inode *ni;

		ni = NTFS_I(vi);
		if (NInoIndexAllocPresent(ni) && ni->itype.index.bmp_ino) {
			iput(ni->itype.index.bmp_ino);
			ni->itype.index.bmp_ino = NULL;
		}
	}
	return;
}

void __ntfs_clear_inode(ntfs_inode *ni)
{
	int err;

	ntfs_debug("Entering for inode 0x%lx.", ni->mft_no);
	if (NInoDirty(ni)) {
		err = ntfs_commit_inode(ni);
		if (err) {
			ntfs_error(ni->vol->sb, "Failed to commit dirty "
					"inode synchronously.");
			// FIXME: Do something!!!
		}
	}
	/* Synchronize with ntfs_commit_inode(). */
	down(&ni->mrec_lock);
	up(&ni->mrec_lock);
	if (NInoDirty(ni)) {
		ntfs_error(ni->vol->sb, "Failed to commit dirty inode "
				"asynchronously.");
		// FIXME: Do something!!!
	}
	/* No need to lock at this stage as no one else has a reference. */
	if (ni->nr_extents > 0) {
		int i;

		// FIXME: Handle dirty case for each extent inode!
		for (i = 0; i < ni->nr_extents; i++)
			ntfs_clear_extent_inode(ni->ext.extent_ntfs_inos[i]);
		kfree(ni->ext.extent_ntfs_inos);
	}
	/* Free all alocated memory. */
	down_write(&ni->run_list.lock);
	if (ni->run_list.rl) {
		ntfs_free(ni->run_list.rl);
		ni->run_list.rl = NULL;
	}
	up_write(&ni->run_list.lock);

	if (ni->attr_list) {
		ntfs_free(ni->attr_list);
		ni->attr_list = NULL;
	}

	down_write(&ni->attr_list_rl.lock);
	if (ni->attr_list_rl.rl) {
		ntfs_free(ni->attr_list_rl.rl);
		ni->attr_list_rl.rl = NULL;
	}
	up_write(&ni->attr_list_rl.lock);

	if (ni->name_len && ni->name != I30) {
		/* Catch bugs... */
		BUG_ON(!ni->name);
		kfree(ni->name);
	}
}

void ntfs_clear_extent_inode(ntfs_inode *ni)
{
	__ntfs_clear_inode(ni);

	/* Bye, bye... */
	ntfs_destroy_extent_inode(ni);
}

/**
 * ntfs_clear_big_inode - clean up the ntfs specific part of an inode
 * @vi:		vfs inode pending annihilation
 *
 * When the VFS is going to remove an inode from memory, ntfs_clear_big_inode()
 * is called, which deallocates all memory belonging to the NTFS specific part
 * of the inode and returns.
 *
 * If the MFT record is dirty, we commit it before doing anything else.
 */
void ntfs_clear_big_inode(struct inode *vi)
{
	ntfs_inode *ni = NTFS_I(vi);

	__ntfs_clear_inode(ni);

	if (NInoAttr(ni)) {
		/* Release the base inode if we are holding it. */
		if (ni->nr_extents == -1) {
			iput(VFS_I(ni->ext.base_ntfs_ino));
			ni->nr_extents = 0;
			ni->ext.base_ntfs_ino = NULL;
		}
	}
	return;
}

/**
 * ntfs_show_options - show mount options in /proc/mounts
 * @sf:		seq_file in which to write our mount options
 * @mnt:	vfs mount whose mount options to display
 *
 * Called by the VFS once for each mounted ntfs volume when someone reads
 * /proc/mounts in order to display the NTFS specific mount options of each
 * mount. The mount options of the vfs mount @mnt are written to the seq file
 * @sf and success is returned.
 */
int ntfs_show_options(struct seq_file *sf, struct vfsmount *mnt)
{
	ntfs_volume *vol = NTFS_SB(mnt->mnt_sb);
	int i;

	seq_printf(sf, ",uid=%i", vol->uid);
	seq_printf(sf, ",gid=%i", vol->gid);
	if (vol->fmask == vol->dmask)
		seq_printf(sf, ",umask=0%o", vol->fmask);
	else {
		seq_printf(sf, ",fmask=0%o", vol->fmask);
		seq_printf(sf, ",dmask=0%o", vol->dmask);
	}
	seq_printf(sf, ",nls=%s", vol->nls_map->charset);
	if (NVolCaseSensitive(vol))
		seq_printf(sf, ",case_sensitive");
	if (NVolShowSystemFiles(vol))
		seq_printf(sf, ",show_sys_files");
	for (i = 0; on_errors_arr[i].val; i++) {
		if (on_errors_arr[i].val & vol->on_errors)
			seq_printf(sf, ",errors=%s", on_errors_arr[i].str);
	}
	seq_printf(sf, ",mft_zone_multiplier=%i", vol->mft_zone_multiplier);
	return 0;
}

#ifdef NTFS_RW

/**
 * ntfs_truncate - called when the i_size of an ntfs inode is changed
 * @vi:		inode for which the i_size was changed
 *
 * We don't support i_size changes yet.
 *
 * Called with ->i_sem held.
 */
void ntfs_truncate(struct inode *vi)
{
	// TODO: Implement...
	ntfs_warning(vi->i_sb, "Eeek: i_size may have changed! If you see "
			"this right after a message from "
			"ntfs_{prepare,commit}_{,nonresident_}write() then "
			"just ignore it. Otherwise it is bad news.");
	// TODO: reset i_size now!
	return;
}

/**
 * ntfs_setattr - called from notify_change() when an attribute is being changed
 * @dentry:	dentry whose attributes to change
 * @attr:	structure describing the attributes and the changes
 *
 * We have to trap VFS attempts to truncate the file described by @dentry as
 * soon as possible, because we do not implement changes in i_size yet. So we
 * abort all i_size changes here.
 *
 * Called with ->i_sem held.
 *
 * Basically this is a copy of generic notify_change() and inode_setattr()
 * functionality, except we intercept and abort changes in i_size.
 */
int ntfs_setattr(struct dentry *dentry, struct iattr *attr)
{
	struct inode *vi;
	int err;
	unsigned int ia_valid = attr->ia_valid;

	vi = dentry->d_inode;

	err = inode_change_ok(vi, attr);
	if (err)
		return err;

	if ((ia_valid & ATTR_UID && attr->ia_uid != vi->i_uid) ||
			(ia_valid & ATTR_GID && attr->ia_gid != vi->i_gid)) {
		err = DQUOT_TRANSFER(vi, attr) ? -EDQUOT : 0;
		if (err)
			return err;
	}

	lock_kernel();

	if (ia_valid & ATTR_SIZE) {
		ntfs_error(vi->i_sb, "Changes in i_size are not supported "
				"yet. Sorry.");
		// TODO: Implement...
		// err = vmtruncate(vi, attr->ia_size);
		err = -EOPNOTSUPP;
		if (err)
			goto trunc_err;
	}

	if (ia_valid & ATTR_UID)
		vi->i_uid = attr->ia_uid;
	if (ia_valid & ATTR_GID)
		vi->i_gid = attr->ia_gid;
	if (ia_valid & ATTR_ATIME)
		vi->i_atime = attr->ia_atime;
	if (ia_valid & ATTR_MTIME)
		vi->i_mtime = attr->ia_mtime;
	if (ia_valid & ATTR_CTIME)
		vi->i_ctime = attr->ia_ctime;
	if (ia_valid & ATTR_MODE) {
		vi->i_mode = attr->ia_mode;
		if (!in_group_p(vi->i_gid) &&
				!capable(CAP_FSETID))
			vi->i_mode &= ~S_ISGID;
	}
	mark_inode_dirty(vi);

trunc_err:

	unlock_kernel();

	return err;
}

#endif

