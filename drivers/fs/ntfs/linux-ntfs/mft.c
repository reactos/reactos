/**
 * mft.c - NTFS kernel mft record operations. Part of the Linux-NTFS project.
 *
 * Copyright (c) 2001-2003 Anton Altaparmakov
 * Copyright (c) 2002 Richard Russon
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

#include <linux/swap.h>

#include "ntfs.h"

/**
 * __format_mft_record - initialize an empty mft record
 * @m:		mapped, pinned and locked for writing mft record
 * @size:	size of the mft record
 * @rec_no:	mft record number / inode number
 *
 * Private function to initialize an empty mft record. Use one of the two
 * provided format_mft_record() functions instead.
 */
static void __format_mft_record(MFT_RECORD *m, const int size,
		const unsigned long rec_no)
{
	ATTR_RECORD *a;

	memset(m, 0, size);
	m->magic = magic_FILE;
	/* Aligned to 2-byte boundary. */
	m->usa_ofs = cpu_to_le16((sizeof(MFT_RECORD) + 1) & ~1);
	m->usa_count = cpu_to_le16(size / NTFS_BLOCK_SIZE + 1);
	/* Set the update sequence number to 1. */
	*(u16*)((char*)m + ((sizeof(MFT_RECORD) + 1) & ~1)) = cpu_to_le16(1);
	m->lsn = cpu_to_le64(0LL);
	m->sequence_number = cpu_to_le16(1);
	m->link_count = cpu_to_le16(0);
	/* Aligned to 8-byte boundary. */
	m->attrs_offset = cpu_to_le16((le16_to_cpu(m->usa_ofs) +
			(le16_to_cpu(m->usa_count) << 1) + 7) & ~7);
	m->flags = cpu_to_le16(0);
	/*
	 * Using attrs_offset plus eight bytes (for the termination attribute),
	 * aligned to 8-byte boundary.
	 */
	m->bytes_in_use = cpu_to_le32((le16_to_cpu(m->attrs_offset) + 8 + 7) &
			~7);
	m->bytes_allocated = cpu_to_le32(size);
	m->base_mft_record = cpu_to_le64((MFT_REF)0);
	m->next_attr_instance = cpu_to_le16(0);
	a = (ATTR_RECORD*)((char*)m + le16_to_cpu(m->attrs_offset));
	a->type = AT_END;
	a->length = cpu_to_le32(0);
}

/**
 * format_mft_record - initialize an empty mft record
 * @ni:		ntfs inode of mft record
 * @mft_rec:	mapped, pinned and locked mft record (optional)
 *
 * Initialize an empty mft record. This is used when extending the MFT.
 *
 * If @mft_rec is NULL, we call map_mft_record() to obtain the
 * record and we unmap it again when finished.
 *
 * We return 0 on success or -errno on error.
 */
int format_mft_record(ntfs_inode *ni, MFT_RECORD *mft_rec)
{
	MFT_RECORD *m;

	if (mft_rec)
		m = mft_rec;
	else {
		m = map_mft_record(ni);
		if (IS_ERR(m))
			return PTR_ERR(m);
	}
	__format_mft_record(m, ni->vol->mft_record_size, ni->mft_no);
	if (!mft_rec) {
		// FIXME: Need to set the mft record dirty!
		unmap_mft_record(ni);
	}
	return 0;
}

/**
 * ntfs_readpage - external declaration, function is in fs/ntfs/aops.c
 */
extern int ntfs_readpage(struct file *, struct page *);

/**
 * ntfs_mft_aops - address space operations for access to $MFT
 *
 * Address space operations for access to $MFT. This allows us to simply use
 * ntfs_map_page() in map_mft_record_page().
 */
struct address_space_operations ntfs_mft_aops = {
	.readpage	= ntfs_readpage,	/* Fill page with data. */
	.sync_page	= block_sync_page,	/* Currently, just unplugs the
						   disk request queue. */
};

/**
 * map_mft_record_page - map the page in which a specific mft record resides
 * @ni:		ntfs inode whose mft record page to map
 *
 * This maps the page in which the mft record of the ntfs inode @ni is situated
 * and returns a pointer to the mft record within the mapped page.
 *
 * Return value needs to be checked with IS_ERR() and if that is true PTR_ERR()
 * contains the negative error code returned.
 */
static inline MFT_RECORD *map_mft_record_page(ntfs_inode *ni)
{
	ntfs_volume *vol = ni->vol;
	struct inode *mft_vi = vol->mft_ino;
	struct page *page;
	unsigned long index, ofs, end_index;

	BUG_ON(ni->page);
	/*
	 * The index into the page cache and the offset within the page cache
	 * page of the wanted mft record. FIXME: We need to check for
	 * overflowing the unsigned long, but I don't think we would ever get
	 * here if the volume was that big...
	 */
	index = ni->mft_no << vol->mft_record_size_bits >> PAGE_CACHE_SHIFT;
	ofs = (ni->mft_no << vol->mft_record_size_bits) & ~PAGE_CACHE_MASK;

	/* The maximum valid index into the page cache for $MFT's data. */
	end_index = mft_vi->i_size >> PAGE_CACHE_SHIFT;

	/* If the wanted index is out of bounds the mft record doesn't exist. */
	if (unlikely(index >= end_index)) {
		if (index > end_index || (mft_vi->i_size & ~PAGE_CACHE_MASK) <
				ofs + vol->mft_record_size) {
			page = ERR_PTR(-ENOENT);
			goto err_out;
		}
	}
	/* Read, map, and pin the page. */
	page = ntfs_map_page(mft_vi->i_mapping, index);
	if (likely(!IS_ERR(page))) {
		ni->page = page;
		ni->page_ofs = ofs;
		return page_address(page) + ofs;
	}
err_out:
	ni->page = NULL;
	ni->page_ofs = 0;
	ntfs_error(vol->sb, "Failed with error code %lu.", -PTR_ERR(page));
	return (void*)page;
}

/**
 * map_mft_record - map, pin and lock an mft record
 * @ni:		ntfs inode whose MFT record to map
 *
 * First, take the mrec_lock semaphore. We might now be sleeping, while waiting
 * for the semaphore if it was already locked by someone else.
 *
 * The page of the record is mapped using map_mft_record_page() before being
 * returned to the caller.
 *
 * This in turn uses ntfs_map_page() to get the page containing the wanted mft
 * record (it in turn calls read_cache_page() which reads it in from disk if
 * necessary, increments the use count on the page so that it cannot disappear
 * under us and returns a reference to the page cache page).
 *
 * If read_cache_page() invokes ntfs_readpage() to load the page from disk, it
 * sets PG_locked and clears PG_uptodate on the page. Once I/O has completed
 * and the post-read mst fixups on each mft record in the page have been
 * performed, the page gets PG_uptodate set and PG_locked cleared (this is done
 * in our asynchronous I/O completion handler end_buffer_read_mft_async()).
 * ntfs_map_page() waits for PG_locked to become clear and checks if
 * PG_uptodate is set and returns an error code if not. This provides
 * sufficient protection against races when reading/using the page.
 *
 * However there is the write mapping to think about. Doing the above described
 * checking here will be fine, because when initiating the write we will set
 * PG_locked and clear PG_uptodate making sure nobody is touching the page
 * contents. Doing the locking this way means that the commit to disk code in
 * the page cache code paths is automatically sufficiently locked with us as
 * we will not touch a page that has been locked or is not uptodate. The only
 * locking problem then is them locking the page while we are accessing it.
 *
 * So that code will end up having to own the mrec_lock of all mft
 * records/inodes present in the page before I/O can proceed. In that case we
 * wouldn't need to bother with PG_locked and PG_uptodate as nobody will be
 * accessing anything without owning the mrec_lock semaphore. But we do need
 * to use them because of the read_cache_page() invocation and the code becomes
 * so much simpler this way that it is well worth it.
 *
 * The mft record is now ours and we return a pointer to it. You need to check
 * the returned pointer with IS_ERR() and if that is true, PTR_ERR() will return
 * the error code.
 *
 * NOTE: Caller is responsible for setting the mft record dirty before calling
 * unmap_mft_record(). This is obviously only necessary if the caller really
 * modified the mft record...
 * Q: Do we want to recycle one of the VFS inode state bits instead?
 * A: No, the inode ones mean we want to change the mft record, not we want to
 * write it out.
 */
MFT_RECORD *map_mft_record(ntfs_inode *ni)
{
	MFT_RECORD *m;

	ntfs_debug("Entering for mft_no 0x%lx.", ni->mft_no);

	/* Make sure the ntfs inode doesn't go away. */
	atomic_inc(&ni->count);

	/* Serialize access to this mft record. */
	down(&ni->mrec_lock);

	m = map_mft_record_page(ni);
	if (likely(!IS_ERR(m)))
		return m;

	up(&ni->mrec_lock);
	atomic_dec(&ni->count);
	ntfs_error(ni->vol->sb, "Failed with error code %lu.", -PTR_ERR(m));
	return m;
}

/**
 * unmap_mft_record_page - unmap the page in which a specific mft record resides
 * @ni:		ntfs inode whose mft record page to unmap
 *
 * This unmaps the page in which the mft record of the ntfs inode @ni is
 * situated and returns. This is a NOOP if highmem is not configured.
 *
 * The unmap happens via ntfs_unmap_page() which in turn decrements the use
 * count on the page thus releasing it from the pinned state.
 *
 * We do not actually unmap the page from memory of course, as that will be
 * done by the page cache code itself when memory pressure increases or
 * whatever.
 */
static inline void unmap_mft_record_page(ntfs_inode *ni)
{
	BUG_ON(!ni->page);

	// TODO: If dirty, blah...
	ntfs_unmap_page(ni->page);
	ni->page = NULL;
	ni->page_ofs = 0;
	return;
}

/**
 * unmap_mft_record - release a mapped mft record
 * @ni:		ntfs inode whose MFT record to unmap
 *
 * We release the page mapping and the mrec_lock mutex which unmaps the mft
 * record and releases it for others to get hold of. We also release the ntfs
 * inode by decrementing the ntfs inode reference count.
 *
 * NOTE: If caller has modified the mft record, it is imperative to set the mft
 * record dirty BEFORE calling unmap_mft_record().
 */
void unmap_mft_record(ntfs_inode *ni)
{
	struct page *page = ni->page;

	BUG_ON(!page);

	ntfs_debug("Entering for mft_no 0x%lx.", ni->mft_no);

	unmap_mft_record_page(ni);
	up(&ni->mrec_lock);
	atomic_dec(&ni->count);
	/*
	 * If pure ntfs_inode, i.e. no vfs inode attached, we leave it to
	 * ntfs_clear_extent_inode() in the extent inode case, and to the
	 * caller in the non-extent, yet pure ntfs inode case, to do the actual
	 * tear down of all structures and freeing of all allocated memory.
	 */
	return;
}

/**
 * map_extent_mft_record - load an extent inode and attach it to its base
 * @base_ni:	base ntfs inode
 * @mref:	mft reference of the extent inode to load (in little endian)
 * @ntfs_ino:	on successful return, pointer to the ntfs_inode structure
 *
 * Load the extent mft record @mref and attach it to its base inode @base_ni.
 * Return the mapped extent mft record if IS_ERR(result) is false. Otherwise
 * PTR_ERR(result) gives the negative error code.
 *
 * On successful return, @ntfs_ino contains a pointer to the ntfs_inode
 * structure of the mapped extent inode.
 */
MFT_RECORD *map_extent_mft_record(ntfs_inode *base_ni, MFT_REF mref,
		ntfs_inode **ntfs_ino)
{
	MFT_RECORD *m;
	ntfs_inode *ni = NULL;
	ntfs_inode **extent_nis = NULL;
	int i;
	unsigned long mft_no = MREF_LE(mref);
	u16 seq_no = MSEQNO_LE(mref);
	BOOL destroy_ni = FALSE;

	ntfs_debug("Mapping extent mft record 0x%lx (base mft record 0x%lx).",
			mft_no, base_ni->mft_no);
	/* Make sure the base ntfs inode doesn't go away. */
	atomic_inc(&base_ni->count);
	/*
	 * Check if this extent inode has already been added to the base inode,
	 * in which case just return it. If not found, add it to the base
	 * inode before returning it.
	 */
	down(&base_ni->extent_lock);
	if (base_ni->nr_extents > 0) {
		extent_nis = base_ni->ext.extent_ntfs_inos;
		for (i = 0; i < base_ni->nr_extents; i++) {
			if (mft_no != extent_nis[i]->mft_no)
				continue;
			ni = extent_nis[i];
			/* Make sure the ntfs inode doesn't go away. */
			atomic_inc(&ni->count);
			break;
		}
	}
	if (likely(ni != NULL)) {
		up(&base_ni->extent_lock);
		atomic_dec(&base_ni->count);
		/* We found the record; just have to map and return it. */
		m = map_mft_record(ni);
		/* map_mft_record() has incremented this on success. */
		atomic_dec(&ni->count);
		if (likely(!IS_ERR(m))) {
			/* Verify the sequence number. */
			if (likely(le16_to_cpu(m->sequence_number) == seq_no)) {
				ntfs_debug("Done 1.");
				*ntfs_ino = ni;
				return m;
			}
			unmap_mft_record(ni);
			ntfs_error(base_ni->vol->sb, "Found stale extent mft "
					"reference! Corrupt file system. "
					"Run chkdsk.");
			return ERR_PTR(-EIO);
		}
map_err_out:
		ntfs_error(base_ni->vol->sb, "Failed to map extent "
				"mft record, error code %ld.", -PTR_ERR(m));
		return m;
	}
	/* Record wasn't there. Get a new ntfs inode and initialize it. */
	ni = ntfs_new_extent_inode(base_ni->vol->sb, mft_no);
	if (unlikely(!ni)) {
		up(&base_ni->extent_lock);
		atomic_dec(&base_ni->count);
		return ERR_PTR(-ENOMEM);
	}
	ni->vol = base_ni->vol;
	ni->seq_no = seq_no;
	ni->nr_extents = -1;
	ni->ext.base_ntfs_ino = base_ni;
	/* Now map the record. */
	m = map_mft_record(ni);
	if (unlikely(IS_ERR(m))) {
		up(&base_ni->extent_lock);
		atomic_dec(&base_ni->count);
		ntfs_clear_extent_inode(ni);
		goto map_err_out;
	}
	/* Verify the sequence number. */
	if (unlikely(le16_to_cpu(m->sequence_number) != seq_no)) {
		ntfs_error(base_ni->vol->sb, "Found stale extent mft "
				"reference! Corrupt file system. Run chkdsk.");
		destroy_ni = TRUE;
		m = ERR_PTR(-EIO);
		goto unm_err_out;
	}
	/* Attach extent inode to base inode, reallocating memory if needed. */
	if (!(base_ni->nr_extents & 3)) {
		ntfs_inode **tmp;
		int new_size = (base_ni->nr_extents + 4) * sizeof(ntfs_inode *);

		tmp = (ntfs_inode **)kmalloc(new_size, GFP_NOFS);
		if (unlikely(!tmp)) {
			ntfs_error(base_ni->vol->sb, "Failed to allocate "
					"internal buffer.");
			destroy_ni = TRUE;
			m = ERR_PTR(-ENOMEM);
			goto unm_err_out;
		}
		if (base_ni->ext.extent_ntfs_inos) {
			memcpy(tmp, base_ni->ext.extent_ntfs_inos, new_size -
					4 * sizeof(ntfs_inode *));
			kfree(base_ni->ext.extent_ntfs_inos);
		}
		base_ni->ext.extent_ntfs_inos = tmp;
	}
	base_ni->ext.extent_ntfs_inos[base_ni->nr_extents++] = ni;
	up(&base_ni->extent_lock);
	atomic_dec(&base_ni->count);
	ntfs_debug("Done 2.");
	*ntfs_ino = ni;
	return m;
unm_err_out:
	unmap_mft_record(ni);
	up(&base_ni->extent_lock);
	atomic_dec(&base_ni->count);
	/*
	 * If the extent inode was not attached to the base inode we need to
	 * release it or we will leak memory.
	 */
	if (destroy_ni)
		ntfs_clear_extent_inode(ni);
	return m;
}

