/**
 * aops.c - NTFS kernel address space operations and page cache handling.
 * 	    Part of the Linux-NTFS project.
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

#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/swap.h>
#include <linux/buffer_head.h>

#include "ntfs.h"

/**
 * ntfs_end_buffer_async_read - async io completion for reading attributes
 * @bh:		buffer head on which io is completed
 * @uptodate:	whether @bh is now uptodate or not
 *
 * Asynchronous I/O completion handler for reading pages belonging to the
 * attribute address space of an inode. The inodes can either be files or
 * directories or they can be fake inodes describing some attribute.
 *
 * If NInoMstProtected(), perform the post read mst fixups when all IO on the
 * page has been completed and mark the page uptodate or set the error bit on
 * the page. To determine the size of the records that need fixing up, we cheat
 * a little bit by setting the index_block_size in ntfs_inode to the ntfs
 * record size, and index_block_size_bits, to the log(base 2) of the ntfs
 * record size.
 */
static void ntfs_end_buffer_async_read(struct buffer_head *bh, int uptodate)
{
	static spinlock_t page_uptodate_lock = SPIN_LOCK_UNLOCKED;
	unsigned long flags;
	struct buffer_head *tmp;
	struct page *page;
	ntfs_inode *ni;
	int page_uptodate = 1;

	page = bh->b_page;
	ni = NTFS_I(page->mapping->host);

	if (likely(uptodate)) {
		s64 file_ofs;

		set_buffer_uptodate(bh);

		file_ofs = (page->index << PAGE_CACHE_SHIFT) + bh_offset(bh);
		/* Check for the current buffer head overflowing. */
		if (file_ofs + bh->b_size > ni->initialized_size) {
			char *addr;
			int ofs = 0;

			if (file_ofs < ni->initialized_size)
				ofs = ni->initialized_size - file_ofs;
			addr = kmap_atomic(page, KM_BIO_SRC_IRQ);
			memset(addr + bh_offset(bh) + ofs, 0, bh->b_size - ofs);
			flush_dcache_page(page);
			kunmap_atomic(addr, KM_BIO_SRC_IRQ);
		}
	} else {
		clear_buffer_uptodate(bh);
		ntfs_error(ni->vol->sb, "Buffer I/O error, logical block %Lu.",
				(unsigned long long)bh->b_blocknr);
		SetPageError(page);
	}

	spin_lock_irqsave(&page_uptodate_lock, flags);
	clear_buffer_async_read(bh);
	unlock_buffer(bh);
	tmp = bh;
	do {
		if (!buffer_uptodate(tmp))
			page_uptodate = 0;
		if (buffer_async_read(tmp)) {
			if (likely(buffer_locked(tmp)))
				goto still_busy;
			/* Async buffers must be locked. */
			BUG();
		}
		tmp = tmp->b_this_page;
	} while (tmp != bh);
	spin_unlock_irqrestore(&page_uptodate_lock, flags);
	/*
	 * If none of the buffers had errors then we can set the page uptodate,
	 * but we first have to perform the post read mst fixups, if the
	 * attribute is mst protected, i.e. if NInoMstProteced(ni) is true.
	 */
	if (!NInoMstProtected(ni)) {
		if (likely(page_uptodate && !PageError(page)))
			SetPageUptodate(page);
	} else {
		char *addr;
		unsigned int i, recs, nr_err;
		u32 rec_size;

		rec_size = ni->itype.index.block_size;
		recs = PAGE_CACHE_SIZE / rec_size;
		addr = kmap_atomic(page, KM_BIO_SRC_IRQ);
		for (i = nr_err = 0; i < recs; i++) {
			if (likely(!post_read_mst_fixup((NTFS_RECORD*)(addr +
					i * rec_size), rec_size)))
				continue;
			nr_err++;
			ntfs_error(ni->vol->sb, "post_read_mst_fixup() failed, "
					"corrupt %s record 0x%Lx. Run chkdsk.",
					ni->mft_no ? "index" : "mft",
					(long long)(((s64)page->index <<
					PAGE_CACHE_SHIFT >>
					ni->itype.index.block_size_bits) + i));
		}
		flush_dcache_page(page);
		kunmap_atomic(addr, KM_BIO_SRC_IRQ);
		if (likely(!PageError(page))) {
			if (likely(!nr_err && recs)) {
				if (likely(page_uptodate))
					SetPageUptodate(page);
			} else {
				ntfs_error(ni->vol->sb, "Setting page error, "
						"index 0x%lx.", page->index);
				SetPageError(page);
			}
		}
	}
	unlock_page(page);
	return;
still_busy:
	spin_unlock_irqrestore(&page_uptodate_lock, flags);
	return;
}

/**
 * ntfs_read_block - fill a @page of an address space with data
 * @page:	page cache page to fill with data
 *
 * Fill the page @page of the address space belonging to the @page->host inode.
 * We read each buffer asynchronously and when all buffers are read in, our io
 * completion handler ntfs_end_buffer_read_async(), if required, automatically
 * applies the mst fixups to the page before finally marking it uptodate and
 * unlocking it.
 *
 * We only enforce allocated_size limit because i_size is checked for in
 * generic_file_read().
 *
 * Return 0 on success and -errno on error.
 *
 * Contains an adapted version of fs/buffer.c::block_read_full_page().
 */
static int ntfs_read_block(struct page *page)
{
	VCN vcn;
	LCN lcn;
	ntfs_inode *ni;
	ntfs_volume *vol;
	run_list_element *rl;
	struct buffer_head *bh, *head, *arr[MAX_BUF_PER_PAGE];
	sector_t iblock, lblock, zblock;
	unsigned int blocksize, vcn_ofs;
	int i, nr;
	unsigned char blocksize_bits;

	ni = NTFS_I(page->mapping->host);
	vol = ni->vol;

	blocksize_bits = VFS_I(ni)->i_blkbits;
	blocksize = 1 << blocksize_bits;

	if (!page_has_buffers(page))
		create_empty_buffers(page, blocksize, 0);
	bh = head = page_buffers(page);
	if (unlikely(!bh)) {
		unlock_page(page);
		return -ENOMEM;
	}

	iblock = page->index << (PAGE_CACHE_SHIFT - blocksize_bits);
	lblock = (ni->allocated_size + blocksize - 1) >> blocksize_bits;
	zblock = (ni->initialized_size + blocksize - 1) >> blocksize_bits;

#ifdef DEBUG
	if (unlikely(!ni->run_list.rl && !ni->mft_no && !NInoAttr(ni)))
		panic("NTFS: $MFT/$DATA run list has been unmapped! This is a "
				"very serious bug! Cannot continue...");
#endif

	/* Loop through all the buffers in the page. */
	rl = NULL;
	nr = i = 0;
	do {
		if (unlikely(buffer_uptodate(bh)))
			continue;
		if (unlikely(buffer_mapped(bh))) {
			arr[nr++] = bh;
			continue;
		}
		bh->b_bdev = vol->sb->s_bdev;
		/* Is the block within the allowed limits? */
		if (iblock < lblock) {
			BOOL is_retry = FALSE;

			/* Convert iblock into corresponding vcn and offset. */
			vcn = (VCN)iblock << blocksize_bits >>
					vol->cluster_size_bits;
			vcn_ofs = ((VCN)iblock << blocksize_bits) &
					vol->cluster_size_mask;
			if (!rl) {
lock_retry_remap:
				down_read(&ni->run_list.lock);
				rl = ni->run_list.rl;
			}
			if (likely(rl != NULL)) {
				/* Seek to element containing target vcn. */
				while (rl->length && rl[1].vcn <= vcn)
					rl++;
				lcn = vcn_to_lcn(rl, vcn);
			} else
				lcn = (LCN)LCN_RL_NOT_MAPPED;
			/* Successful remap. */
			if (lcn >= 0) {
				/* Setup buffer head to correct block. */
				bh->b_blocknr = ((lcn << vol->cluster_size_bits)
						+ vcn_ofs) >> blocksize_bits;
				set_buffer_mapped(bh);
				/* Only read initialized data blocks. */
				if (iblock < zblock) {
					arr[nr++] = bh;
					continue;
				}
				/* Fully non-initialized data block, zero it. */
				goto handle_zblock;
			}
			/* It is a hole, need to zero it. */
			if (lcn == LCN_HOLE)
				goto handle_hole;
			/* If first try and run list unmapped, map and retry. */
			if (!is_retry && lcn == LCN_RL_NOT_MAPPED) {
				is_retry = TRUE;
				/*
				 * Attempt to map run list, dropping lock for
				 * the duration.
				 */
				up_read(&ni->run_list.lock);
				if (!map_run_list(ni, vcn))
					goto lock_retry_remap;
				rl = NULL;
			}
			/* Hard error, zero out region. */
			SetPageError(page);
			ntfs_error(vol->sb, "vcn_to_lcn(vcn = 0x%Lx) failed "
					"with error code 0x%Lx%s.",
					(long long)vcn, (long long)-lcn,
					is_retry ? " even after retrying" : "");
			// FIXME: Depending on vol->on_errors, do something.
		}
		/*
		 * Either iblock was outside lblock limits or vcn_to_lcn()
		 * returned error. Just zero that portion of the page and set
		 * the buffer uptodate.
		 */
handle_hole:
		bh->b_blocknr = -1UL;
		clear_buffer_mapped(bh);
handle_zblock:
		memset(kmap(page) + i * blocksize, 0, blocksize);
		flush_dcache_page(page);
		kunmap(page);
		set_buffer_uptodate(bh);
	} while (i++, iblock++, (bh = bh->b_this_page) != head);

	/* Release the lock if we took it. */
	if (rl)
		up_read(&ni->run_list.lock);

	/* Check we have at least one buffer ready for i/o. */
	if (nr) {
		struct buffer_head *tbh;

		/* Lock the buffers. */
		for (i = 0; i < nr; i++) {
			tbh = arr[i];
			lock_buffer(tbh);
			tbh->b_end_io = ntfs_end_buffer_async_read;
			set_buffer_async_read(tbh);
		}
		/* Finally, start i/o on the buffers. */
		for (i = 0; i < nr; i++) {
			tbh = arr[i];
			if (likely(!buffer_uptodate(tbh)))
				submit_bh(READ, tbh);
			else
				ntfs_end_buffer_async_read(tbh, 1);
		}
		return 0;
	}
	/* No i/o was scheduled on any of the buffers. */
	if (likely(!PageError(page)))
		SetPageUptodate(page);
	else /* Signal synchronous i/o error. */
		nr = -EIO;
	unlock_page(page);
	return nr;
}

/**
 * ntfs_readpage - fill a @page of a @file with data from the device
 * @file:	open file to which the page @page belongs or NULL
 * @page:	page cache page to fill with data
 *
 * For non-resident attributes, ntfs_readpage() fills the @page of the open
 * file @file by calling the ntfs version of the generic block_read_full_page()
 * function, ntfs_read_block(), which in turn creates and reads in the buffers
 * associated with the page asynchronously.
 *
 * For resident attributes, OTOH, ntfs_readpage() fills @page by copying the
 * data from the mft record (which at this stage is most likely in memory) and
 * fills the remainder with zeroes. Thus, in this case, I/O is synchronous, as
 * even if the mft record is not cached at this point in time, we need to wait
 * for it to be read in before we can do the copy.
 *
 * Return 0 on success and -errno on error.
 *
 * WARNING: Do not make this function static! It is used by mft.c!
 */
int ntfs_readpage(struct file *file, struct page *page)
{
	s64 attr_pos;
	ntfs_inode *ni, *base_ni;
	char *addr;
	attr_search_context *ctx;
	MFT_RECORD *mrec;
	u32 attr_len;
	int err = 0;

	BUG_ON(!PageLocked(page));

	/*
	 * This can potentially happen because we clear PageUptodate() during
	 * ntfs_writepage() of MstProtected() attributes.
	 */
	if (PageUptodate(page)) {
		unlock_page(page);
		return 0;
	}

	ni = NTFS_I(page->mapping->host);

	if (NInoNonResident(ni)) {
		/*
		 * Only unnamed $DATA attributes can be compressed or
		 * encrypted.
		 */
		if (ni->type == AT_DATA && !ni->name_len) {
			/* If file is encrypted, deny access, just like NT4. */
			if (NInoEncrypted(ni)) {
				err = -EACCES;
				goto err_out;
			}
			/* Compressed data streams are handled in compress.c. */
			if (NInoCompressed(ni))
				return ntfs_read_compressed_block(page);
		}
		/* Normal data stream. */
		return ntfs_read_block(page);
	}
	/* Attribute is resident, implying it is not compressed or encrypted. */
	if (!NInoAttr(ni))
		base_ni = ni;
	else
		base_ni = ni->ext.base_ntfs_ino;

	/* Map, pin, and lock the mft record. */
	mrec = map_mft_record(base_ni);
	if (unlikely(IS_ERR(mrec))) {
		err = PTR_ERR(mrec);
		goto err_out;
	}
	ctx = get_attr_search_ctx(base_ni, mrec);
	if (unlikely(!ctx)) {
		err = -ENOMEM;
		goto unm_err_out;
	}
	if (unlikely(!lookup_attr(ni->type, ni->name, ni->name_len,
			IGNORE_CASE, 0, NULL, 0, ctx))) {
		err = -ENOENT;
		goto put_unm_err_out;
	}

	/* Starting position of the page within the attribute value. */
	attr_pos = page->index << PAGE_CACHE_SHIFT;

	/* The total length of the attribute value. */
	attr_len = le32_to_cpu(ctx->attr->data.resident.value_length);

	addr = kmap(page);
	/* Copy over in bounds data, zeroing the remainder of the page. */
	if (attr_pos < attr_len) {
		u32 bytes = attr_len - attr_pos;
		if (bytes > PAGE_CACHE_SIZE)
			bytes = PAGE_CACHE_SIZE;
		else if (bytes < PAGE_CACHE_SIZE)
			memset(addr + bytes, 0, PAGE_CACHE_SIZE - bytes);
		/* Copy the data to the page. */
		memcpy(addr, attr_pos + (char*)ctx->attr +
				le16_to_cpu(
				ctx->attr->data.resident.value_offset), bytes);
	} else
		memset(addr, 0, PAGE_CACHE_SIZE);
	flush_dcache_page(page);
	kunmap(page);

	SetPageUptodate(page);
put_unm_err_out:
	put_attr_search_ctx(ctx);
unm_err_out:
	unmap_mft_record(base_ni);
err_out:
	unlock_page(page);
	return err;
}

#ifdef NTFS_RW

/**
 * ntfs_write_block - write a @page to the backing store
 * @page:	page cache page to write out
 *
 * This function is for writing pages belonging to non-resident, non-mst
 * protected attributes to their backing store.
 *
 * For a page with buffers, map and write the dirty buffers asynchronously
 * under page writeback. For a page without buffers, create buffers for the
 * page, then proceed as above.
 *
 * If a page doesn't have buffers the page dirty state is definitive. If a page
 * does have buffers, the page dirty state is just a hint, and the buffer dirty
 * state is definitive. (A hint which has rules: dirty buffers against a clean
 * page is illegal. Other combinations are legal and need to be handled. In
 * particular a dirty page containing clean buffers for example.)
 *
 * Return 0 on success and -errno on error.
 *
 * Based on ntfs_read_block() and __block_write_full_page().
 */
static int ntfs_write_block(struct page *page)
{
	VCN vcn;
	LCN lcn;
	sector_t block, dblock, iblock;
	struct inode *vi;
	ntfs_inode *ni;
	ntfs_volume *vol;
	run_list_element *rl;
	struct buffer_head *bh, *head;
	unsigned int blocksize, vcn_ofs;
	int err;
	BOOL need_end_writeback;
	unsigned char blocksize_bits;

	vi = page->mapping->host;
	ni = NTFS_I(vi);
	vol = ni->vol;

	ntfs_debug("Entering for inode %li, attribute type 0x%x, page index "
			"0x%lx.\n", vi->i_ino, ni->type, page->index);

	BUG_ON(!NInoNonResident(ni));
	BUG_ON(NInoMstProtected(ni));

	blocksize_bits = vi->i_blkbits;
	blocksize = 1 << blocksize_bits;

	if (!page_has_buffers(page)) {
		BUG_ON(!PageUptodate(page));
		create_empty_buffers(page, blocksize,
				(1 << BH_Uptodate) | (1 << BH_Dirty));
	}
	bh = head = page_buffers(page);
	if (unlikely(!bh)) {
		ntfs_warning(vol->sb, "Error allocating page buffers. "
				"Redirtying page so we try again later.");
		/*
		 * Put the page back on mapping->dirty_pages, but leave its
		 * buffer's dirty state as-is.
		 */
		// FIXME: Once Andrew's -EAGAIN patch goes in, remove the
		// __set_page_dirty_nobuffers(page) and return -EAGAIN instead
		// of zero.
		__set_page_dirty_nobuffers(page);
		unlock_page(page);
		return 0;
	}

	/* NOTE: Different naming scheme to ntfs_read_block()! */

	/* The first block in the page. */
	block = page->index << (PAGE_CACHE_SHIFT - blocksize_bits);

	/* The first out of bounds block for the data size. */
	dblock = (vi->i_size + blocksize - 1) >> blocksize_bits;

	/* The last (fully or partially) initialized block. */
	iblock = ni->initialized_size >> blocksize_bits;

	/*
	 * Be very careful.  We have no exclusion from __set_page_dirty_buffers
	 * here, and the (potentially unmapped) buffers may become dirty at
	 * any time.  If a buffer becomes dirty here after we've inspected it
	 * then we just miss that fact, and the page stays dirty.
	 *
	 * Buffers outside i_size may be dirtied by __set_page_dirty_buffers;
	 * handle that here by just cleaning them.
	 */

	/*
	 * Loop through all the buffers in the page, mapping all the dirty
	 * buffers to disk addresses and handling any aliases from the
	 * underlying block device's mapping.
	 */
	rl = NULL;
	err = 0;
	do {
		BOOL is_retry = FALSE;

		if (unlikely(block >= dblock)) {
			/*
			 * Mapped buffers outside i_size will occur, because
			 * this page can be outside i_size when there is a
			 * truncate in progress. The contents of such buffers
			 * were zeroed by ntfs_writepage().
			 *
			 * FIXME: What about the small race window where
			 * ntfs_writepage() has not done any clearing because
			 * the page was within i_size but before we get here,
			 * vmtruncate() modifies i_size?
			 */
			clear_buffer_dirty(bh);
			set_buffer_uptodate(bh);
			continue;
		}

		/* Clean buffers are not written out, so no need to map them. */
		if (!buffer_dirty(bh))
			continue;

		/* Make sure we have enough initialized size. */
		if (unlikely((block >= iblock) &&
				(ni->initialized_size < vi->i_size))) {
			/*
			 * If this page is fully outside initialized size, zero
			 * out all pages between the current initialized size
			 * and the current page. Just use ntfs_readpage() to do
			 * the zeroing transparently.
			 */
			if (block > iblock) {
				// TODO:
				// For each page do:
				// - read_cache_page()
				// Again for each page do:
				// - wait_on_page_locked()
				// - Check (PageUptodate(page) &&
				// 			!PageError(page))
				// Update initialized size in the attribute and
				// in the inode.
				// Again, for each page do:
				// 	__set_page_dirty_buffers();
				// page_cache_release()
				// We don't need to wait on the writes.
				// Update iblock.
			}
			/*
			 * The current page straddles initialized size. Zero
			 * all non-uptodate buffers and set them uptodate (and
			 * dirty?). Note, there aren't any non-uptodate buffers
			 * if the page is uptodate.
			 * FIXME: For an uptodate page, the buffers may need to
			 * be written out because they were not initialized on
			 * disk before.
			 */
			if (!PageUptodate(page)) {
				// TODO:
				// Zero any non-uptodate buffers up to i_size.
				// Set them uptodate and dirty.
			}
			// TODO:
			// Update initialized size in the attribute and in the
			// inode (up to i_size).
			// Update iblock.
			// FIXME: This is inefficient. Try to batch the two
			// size changes to happen in one go.
			ntfs_error(vol->sb, "Writing beyond initialized size "
					"is not supported yet. Sorry.");
			err = -EOPNOTSUPP;
			break;
			// Do NOT set_buffer_new() BUT DO clear buffer range
			// outside write request range.
			// set_buffer_uptodate() on complete buffers as well as
			// set_buffer_dirty().
		}

		/* No need to map buffers that are already mapped. */
		if (buffer_mapped(bh))
			continue;

		/* Unmapped, dirty buffer. Need to map it. */
		bh->b_bdev = vol->sb->s_bdev;

		/* Convert block into corresponding vcn and offset. */
		vcn = (VCN)block << blocksize_bits >> vol->cluster_size_bits;
		vcn_ofs = ((VCN)block << blocksize_bits) &
				vol->cluster_size_mask;
		if (!rl) {
lock_retry_remap:
			down_read(&ni->run_list.lock);
			rl = ni->run_list.rl;
		}
		if (likely(rl != NULL)) {
			/* Seek to element containing target vcn. */
			while (rl->length && rl[1].vcn <= vcn)
				rl++;
			lcn = vcn_to_lcn(rl, vcn);
		} else
			lcn = (LCN)LCN_RL_NOT_MAPPED;
		/* Successful remap. */
		if (lcn >= 0) {
			/* Setup buffer head to point to correct block. */
			bh->b_blocknr = ((lcn << vol->cluster_size_bits) +
					vcn_ofs) >> blocksize_bits;
			set_buffer_mapped(bh);
			continue;
		}
		/* It is a hole, need to instantiate it. */
		if (lcn == LCN_HOLE) {
			// TODO: Instantiate the hole.
			// clear_buffer_new(bh);
			// unmap_underlying_metadata(bh->b_bdev, bh->b_blocknr);
			ntfs_error(vol->sb, "Writing into sparse regions is "
					"not supported yet. Sorry.");
			err = -EOPNOTSUPP;
			break;
		}
		/* If first try and run list unmapped, map and retry. */
		if (!is_retry && lcn == LCN_RL_NOT_MAPPED) {
			is_retry = TRUE;
			/*
			 * Attempt to map run list, dropping lock for
			 * the duration.
			 */
			up_read(&ni->run_list.lock);
			err = map_run_list(ni, vcn);
			if (likely(!err))
				goto lock_retry_remap;
			rl = NULL;
		}
		/* Failed to map the buffer, even after retrying. */
		bh->b_blocknr = -1UL;
		ntfs_error(vol->sb, "vcn_to_lcn(vcn = 0x%Lx) failed "
				"with error code 0x%Lx%s.",
				(long long)vcn, (long long)-lcn,
				is_retry ? " even after retrying" : "");
		// FIXME: Depending on vol->on_errors, do something.
		if (!err)
			err = -EIO;
		break;
	} while (block++, (bh = bh->b_this_page) != head);

	/* Release the lock if we took it. */
	if (rl)
		up_read(&ni->run_list.lock);

	/* For the error case, need to reset bh to the beginning. */
	bh = head;

	/* Just an optimization, so ->readpage() isn't called later. */
	if (unlikely(!PageUptodate(page))) {
		int uptodate = 1;
		do {
			if (!buffer_uptodate(bh)) {
				uptodate = 0;
				bh = head;
				break;
			}
		} while ((bh = bh->b_this_page) != head);
		if (uptodate)
			SetPageUptodate(page);
	}

	/* Setup all mapped, dirty buffers for async write i/o. */
	do {
		get_bh(bh);
		if (buffer_mapped(bh) && buffer_dirty(bh)) {
			lock_buffer(bh);
			if (test_clear_buffer_dirty(bh)) {
				BUG_ON(!buffer_uptodate(bh));
				mark_buffer_async_write(bh);
			} else
				unlock_buffer(bh);
		} else if (unlikely(err)) {
			/*
			 * For the error case. The buffer may have been set
			 * dirty during attachment to a dirty page.
			 */
			if (err != -ENOMEM)
				clear_buffer_dirty(bh);
		}
	} while ((bh = bh->b_this_page) != head);

	if (unlikely(err)) {
		// TODO: Remove the -EOPNOTSUPP check later on...
		if (unlikely(err == -EOPNOTSUPP))
			err = 0;
		else if (err == -ENOMEM) {
			ntfs_warning(vol->sb, "Error allocating memory. "
					"Redirtying page so we try again "
					"later.");
			/*
			 * Put the page back on mapping->dirty_pages, but
			 * leave its buffer's dirty state as-is.
			 */
			// FIXME: Once Andrew's -EAGAIN patch goes in, remove
			// the __set_page_dirty_nobuffers(page) and set err to
			// -EAGAIN instead of zero.
			__set_page_dirty_nobuffers(page);
			err = 0;
		} else
			SetPageError(page);
	}

	BUG_ON(PageWriteback(page));
	SetPageWriteback(page);		/* Keeps try_to_free_buffers() away. */
	unlock_page(page);

	/*
	 * Submit the prepared buffers for i/o. Note the page is unlocked,
	 * and the async write i/o completion handler can end_page_writeback()
	 * at any time after the *first* submit_bh(). So the buffers can then
	 * disappear...
	 */
	need_end_writeback = TRUE;
	do {
		struct buffer_head *next = bh->b_this_page;
		if (buffer_async_write(bh)) {
			submit_bh(WRITE, bh);
			need_end_writeback = FALSE;
		}
		put_bh(bh);
		bh = next;
	} while (bh != head);

	/* If no i/o was started, need to end_page_writeback(). */
	if (unlikely(need_end_writeback))
		end_page_writeback(page);

	ntfs_debug("Done.");
	return err;
}

/**
 * ntfs_writepage - write a @page to the backing store
 * @page:	page cache page to write out
 *
 * For non-resident attributes, ntfs_writepage() writes the @page by calling
 * the ntfs version of the generic block_write_full_page() function,
 * ntfs_write_block(), which in turn if necessary creates and writes the
 * buffers associated with the page asynchronously.
 *
 * For resident attributes, OTOH, ntfs_writepage() writes the @page by copying
 * the data to the mft record (which at this stage is most likely in memory).
 * Thus, in this case, I/O is synchronous, as even if the mft record is not
 * cached at this point in time, we need to wait for it to be read in before we
 * can do the copy.
 *
 * Note the caller clears the page dirty flag before calling ntfs_writepage().
 *
 * Based on ntfs_readpage() and fs/buffer.c::block_write_full_page().
 *
 * Return 0 on success and -errno on error.
 */
static int ntfs_writepage(struct page *page, struct writeback_control *wbc)
{
	s64 attr_pos;
	struct inode *vi;
	ntfs_inode *ni, *base_ni;
	char *kaddr;
	attr_search_context *ctx;
	MFT_RECORD *m;
	u32 attr_len, bytes;
	int err;

	BUG_ON(!PageLocked(page));

	vi = page->mapping->host;

	/* Is the page fully outside i_size? (truncate in progress) */
	if (unlikely(page->index >= (vi->i_size + PAGE_CACHE_SIZE - 1) >>
			PAGE_CACHE_SHIFT)) {
		unlock_page(page);
		ntfs_debug("Write outside i_size - truncated?");
		return 0;
	}

	ni = NTFS_I(vi);

	if (NInoNonResident(ni)) {
		/*
		 * Only unnamed $DATA attributes can be compressed, encrypted,
		 * and/or sparse.
		 */
		if (ni->type == AT_DATA && !ni->name_len) {
			/* If file is encrypted, deny access, just like NT4. */
			if (NInoEncrypted(ni)) {
				unlock_page(page);
				ntfs_debug("Denying write access to encrypted "
						"file.");
				return -EACCES;
			}
			/* Compressed data streams are handled in compress.c. */
			if (NInoCompressed(ni)) {
				// TODO: Implement and replace this check with
				// return ntfs_write_compressed_block(page);
				unlock_page(page);
				ntfs_error(vi->i_sb, "Writing to compressed "
						"files is not supported yet. "
						"Sorry.");
				return -EOPNOTSUPP;
			}
			// TODO: Implement and remove this check.
			if (NInoSparse(ni)) {
				unlock_page(page);
				ntfs_error(vi->i_sb, "Writing to sparse files "
						"is not supported yet. Sorry.");
				return -EOPNOTSUPP;
			}
		}

		/* We have to zero every time due to mmap-at-end-of-file. */
		if (page->index >= (vi->i_size >> PAGE_CACHE_SHIFT)) {
			/* The page straddles i_size. */
			unsigned int ofs = vi->i_size & ~PAGE_CACHE_MASK;
			kaddr = kmap_atomic(page, KM_USER0);
			memset(kaddr + ofs, 0, PAGE_CACHE_SIZE - ofs);
			flush_dcache_page(page);
			kunmap_atomic(kaddr, KM_USER0);
		}

		// TODO: Implement and remove this check.
		if (NInoMstProtected(ni)) {
			unlock_page(page);
			ntfs_error(vi->i_sb, "Writing to MST protected "
					"attributes is not supported yet. "
					"Sorry.");
			return -EOPNOTSUPP;
		}

		/* Normal data stream. */
		return ntfs_write_block(page);
	}

	/*
	 * Attribute is resident, implying it is not compressed, encrypted, or
	 * mst protected.
	 */
	BUG_ON(page_has_buffers(page));
	BUG_ON(!PageUptodate(page));

	// TODO: Consider using PageWriteback() + unlock_page() in 2.5 once the
	// "VM fiddling has ended". Note, don't forget to replace all the
	// unlock_page() calls further below with end_page_writeback() ones.
	// FIXME: Make sure it is ok to SetPageError() on unlocked page under
	// writeback before doing the change!
#if 0
	SetPageWriteback(page);
	unlock_page(page);
#endif

	if (!NInoAttr(ni))
		base_ni = ni;
	else
		base_ni = ni->ext.base_ntfs_ino;

	/* Map, pin, and lock the mft record. */
	m = map_mft_record(base_ni);
	if (unlikely(IS_ERR(m))) {
		err = PTR_ERR(m);
		m = NULL;
		ctx = NULL;
		goto err_out;
	}
	ctx = get_attr_search_ctx(base_ni, m);
	if (unlikely(!ctx)) {
		err = -ENOMEM;
		goto err_out;
	}
	if (unlikely(!lookup_attr(ni->type, ni->name, ni->name_len,
			IGNORE_CASE, 0, NULL, 0, ctx))) {
		err = -ENOENT;
		goto err_out;
	}

	/* Starting position of the page within the attribute value. */
	attr_pos = page->index << PAGE_CACHE_SHIFT;

	/* The total length of the attribute value. */
	attr_len = le32_to_cpu(ctx->attr->data.resident.value_length);

	if (unlikely(vi->i_size != attr_len)) {
		ntfs_error(vi->i_sb, "BUG()! i_size (0x%Lx) doesn't match "
				"attr_len (0x%x). Aborting write.", vi->i_size,
				attr_len);
		err = -EIO;
		goto err_out;
	}
	if (unlikely(attr_pos >= attr_len)) {
		ntfs_error(vi->i_sb, "BUG()! attr_pos (0x%Lx) > attr_len (0x%x)"
				". Aborting write.", attr_pos, attr_len);
		err = -EIO;
		goto err_out;
	}

	bytes = attr_len - attr_pos;
	if (unlikely(bytes > PAGE_CACHE_SIZE))
		bytes = PAGE_CACHE_SIZE;

	/*
	 * Here, we don't need to zero the out of bounds area everytime because
	 * the below memcpy() already takes care of the mmap-at-end-of-file
	 * requirements. If the file is converted to a non-resident one, then
	 * the code path use is switched to the non-resident one where the
	 * zeroing happens on each ntfs_writepage() invocation.
	 *
	 * The above also applies nicely when i_size is decreased.
	 *
	 * When i_size is increased, the memory between the old and new i_size
	 * _must_ be zeroed (or overwritten with new data). Otherwise we will
	 * expose data to userspace/disk which should never have been exposed.
	 *
	 * FIXME: Ensure that i_size increases do the zeroing/overwriting and
	 * if we cannot guarantee that, then enable the zeroing below.
	 */

	kaddr = kmap_atomic(page, KM_USER0);
	/* Copy the data from the page to the mft record. */
	memcpy((u8*)ctx->attr + le16_to_cpu(
			ctx->attr->data.resident.value_offset) + attr_pos,
			kaddr, bytes);
	flush_dcache_mft_record_page(ctx->ntfs_ino);
#if 0
	/* Zero out of bounds area. */
	if (likely(bytes < PAGE_CACHE_SIZE)) {
		memset(kaddr + bytes, 0, PAGE_CACHE_SIZE - bytes);
		flush_dcache_page(page);
	}
#endif
	kunmap_atomic(kaddr, KM_USER0);

	unlock_page(page);

	// TODO: Mark mft record dirty so it gets written back.
	ntfs_error(vi->i_sb, "Writing to resident files is not supported yet. "
			"Wrote to memory only...");

	put_attr_search_ctx(ctx);
	unmap_mft_record(base_ni);
	return 0;
err_out:
	if (err == -ENOMEM) {
		ntfs_warning(vi->i_sb, "Error allocating memory. Redirtying "
				"page so we try again later.");
		/*
		 * Put the page back on mapping->dirty_pages, but leave its
		 * buffer's dirty state as-is.
		 */
		// FIXME: Once Andrew's -EAGAIN patch goes in, remove the
		// __set_page_dirty_nobuffers(page) and set err to -EAGAIN
		// instead of zero.
		__set_page_dirty_nobuffers(page);
		err = 0;
	} else {
		ntfs_error(vi->i_sb, "Resident attribute write failed with "
				"error %i. Setting page error flag.", -err);
		SetPageError(page);
	}
	unlock_page(page);
	if (ctx)
		put_attr_search_ctx(ctx);
	if (m)
		unmap_mft_record(base_ni);
	return err;
}

/**
 * ntfs_prepare_nonresident_write -
 *
 */
static int ntfs_prepare_nonresident_write(struct page *page,
		unsigned from, unsigned to)
{
	VCN vcn;
	LCN lcn;
	sector_t block, ablock, iblock;
	struct inode *vi;
	ntfs_inode *ni;
	ntfs_volume *vol;
	run_list_element *rl;
	struct buffer_head *bh, *head, *wait[2], **wait_bh = wait;
	unsigned int vcn_ofs, block_start, block_end, blocksize;
	int err;
	BOOL is_retry;
	unsigned char blocksize_bits;

	vi = page->mapping->host;
	ni = NTFS_I(vi);
	vol = ni->vol;

	ntfs_debug("Entering for inode %li, attribute type 0x%x, page index "
			"0x%lx, from = %u, to = %u.", vi->i_ino, ni->type,
			page->index, from, to);

	BUG_ON(!NInoNonResident(ni));
	BUG_ON(NInoMstProtected(ni));

	blocksize_bits = vi->i_blkbits;
	blocksize = 1 << blocksize_bits;

	/*
	 * create_empty_buffers() will create uptodate/dirty buffers if the
	 * page is uptodate/dirty.
	 */
	if (!page_has_buffers(page))
		create_empty_buffers(page, blocksize, 0);
	bh = head = page_buffers(page);
	if (unlikely(!bh))
		return -ENOMEM;

	/* The first block in the page. */
	block = page->index << (PAGE_CACHE_SHIFT - blocksize_bits);

	/*
	 * The first out of bounds block for the allocated size. No need to
	 * round up as allocated_size is in multiples of cluster size and the
	 * minimum cluster size is 512 bytes, which is equal to the smallest
	 * blocksize.
	 */
	ablock = ni->allocated_size >> blocksize_bits;

	/* The last (fully or partially) initialized block. */
	iblock = ni->initialized_size >> blocksize_bits;

	/* Loop through all the buffers in the page. */
	block_start = 0;
	rl = NULL;
	err = 0;
	do {
		block_end = block_start + blocksize;
		/*
		 * If buffer @bh is outside the write, just mark it uptodate
		 * if the page is uptodate and continue with the next buffer.
		 */
		if (block_end <= from || block_start >= to) {
			if (PageUptodate(page)) {
				if (!buffer_uptodate(bh))
					set_buffer_uptodate(bh);
			}
			continue;
		}
		/*
		 * @bh is at least partially being written to.
		 * Make sure it is not marked as new.
		 */
		//if (buffer_new(bh))
		//	clear_buffer_new(bh);

		if (block >= ablock) {
			// TODO: block is above allocated_size, need to
			// allocate it. Best done in one go to accommodate not
			// only block but all above blocks up to and including:
			// ((page->index << PAGE_CACHE_SHIFT) + to + blocksize
			// - 1) >> blobksize_bits. Obviously will need to round
			// up to next cluster boundary, too. This should be
			// done with a helper function, so it can be reused.
			ntfs_error(vol->sb, "Writing beyond allocated size "
					"is not supported yet. Sorry.");
			err = -EOPNOTSUPP;
			goto err_out;
			// Need to update ablock.
			// Need to set_buffer_new() on all block bhs that are
			// newly allocated.
		}
		/*
		 * Now we have enough allocated size to fulfill the whole
		 * request, i.e. block < ablock is true.
		 */
		if (unlikely((block >= iblock) &&
				(ni->initialized_size < vi->i_size))) {
			/*
			 * If this page is fully outside initialized size, zero
			 * out all pages between the current initialized size
			 * and the current page. Just use ntfs_readpage() to do
			 * the zeroing transparently.
			 */
			if (block > iblock) {
				// TODO:
				// For each page do:
				// - read_cache_page()
				// Again for each page do:
				// - wait_on_page_locked()
				// - Check (PageUptodate(page) &&
				// 			!PageError(page))
				// Update initialized size in the attribute and
				// in the inode.
				// Again, for each page do:
				// 	__set_page_dirty_buffers();
				// page_cache_release()
				// We don't need to wait on the writes.
				// Update iblock.
			}
			/*
			 * The current page straddles initialized size. Zero
			 * all non-uptodate buffers and set them uptodate (and
			 * dirty?). Note, there aren't any non-uptodate buffers
			 * if the page is uptodate.
			 * FIXME: For an uptodate page, the buffers may need to
			 * be written out because they were not initialized on
			 * disk before.
			 */
			if (!PageUptodate(page)) {
				// TODO:
				// Zero any non-uptodate buffers up to i_size.
				// Set them uptodate and dirty.
			}
			// TODO:
			// Update initialized size in the attribute and in the
			// inode (up to i_size).
			// Update iblock.
			// FIXME: This is inefficient. Try to batch the two
			// size changes to happen in one go.
			ntfs_error(vol->sb, "Writing beyond initialized size "
					"is not supported yet. Sorry.");
			err = -EOPNOTSUPP;
			goto err_out;
			// Do NOT set_buffer_new() BUT DO clear buffer range
			// outside write request range.
			// set_buffer_uptodate() on complete buffers as well as
			// set_buffer_dirty().
		}

		/* Need to map unmapped buffers. */
		if (!buffer_mapped(bh)) {
			/* Unmapped buffer. Need to map it. */
			bh->b_bdev = vol->sb->s_bdev;

			/* Convert block into corresponding vcn and offset. */
			vcn = (VCN)block << blocksize_bits >>
					vol->cluster_size_bits;
			vcn_ofs = ((VCN)block << blocksize_bits) &
					vol->cluster_size_mask;

			is_retry = FALSE;
			if (!rl) {
lock_retry_remap:
				down_read(&ni->run_list.lock);
				rl = ni->run_list.rl;
			}
			if (likely(rl != NULL)) {
				/* Seek to element containing target vcn. */
				while (rl->length && rl[1].vcn <= vcn)
					rl++;
				lcn = vcn_to_lcn(rl, vcn);
			} else
				lcn = (LCN)LCN_RL_NOT_MAPPED;
			if (unlikely(lcn < 0)) {
				/*
				 * We extended the attribute allocation above.
				 * If we hit an ENOENT here it means that the
				 * allocation was insufficient which is a bug.
				 */
				BUG_ON(lcn == LCN_ENOENT);

				/* It is a hole, need to instantiate it. */
				if (lcn == LCN_HOLE) {
					// TODO: Instantiate the hole.
					// clear_buffer_new(bh);
					// unmap_underlying_metadata(bh->b_bdev,
					// 		bh->b_blocknr);
					// For non-uptodate buffers, need to
					// zero out the region outside the
					// request in this bh or all bhs,
					// depending on what we implemented
					// above.
					// Need to flush_dcache_page().
					// Or could use set_buffer_new()
					// instead?
					ntfs_error(vol->sb, "Writing into "
							"sparse regions is "
							"not supported yet. "
							"Sorry.");
					err = -EOPNOTSUPP;
					goto err_out;
				} else if (!is_retry &&
						lcn == LCN_RL_NOT_MAPPED) {
					is_retry = TRUE;
					/*
					 * Attempt to map run list, dropping
					 * lock for the duration.
					 */
					up_read(&ni->run_list.lock);
					err = map_run_list(ni, vcn);
					if (likely(!err))
						goto lock_retry_remap;
					rl = NULL;
				}
				/*
				 * Failed to map the buffer, even after
				 * retrying.
				 */
				bh->b_blocknr = -1UL;
				ntfs_error(vol->sb, "vcn_to_lcn(vcn = 0x%Lx) "
						"failed with error code "
						"0x%Lx%s.", (long long)vcn,
						(long long)-lcn, is_retry ?
						" even after retrying" : "");
				// FIXME: Depending on vol->on_errors, do
				// something.
				if (!err)
					err = -EIO;
				goto err_out;
			}
			/* We now have a successful remap, i.e. lcn >= 0. */

			/* Setup buffer head to correct block. */
			bh->b_blocknr = ((lcn << vol->cluster_size_bits)
					+ vcn_ofs) >> blocksize_bits;
			set_buffer_mapped(bh);

			// FIXME: Something analogous to this is needed for
			// each newly allocated block, i.e. BH_New.
			// FIXME: Might need to take this out of the
			// if (!buffer_mapped(bh)) {}, depending on how we
			// implement things during the allocated_size and
			// initialized_size extension code above.
			if (buffer_new(bh)) {
				clear_buffer_new(bh);
				unmap_underlying_metadata(bh->b_bdev,
						bh->b_blocknr);
				if (PageUptodate(page)) {
					set_buffer_uptodate(bh);
					continue;
				}
				/*
				 * Page is _not_ uptodate, zero surrounding
				 * region. NOTE: This is how we decide if to
				 * zero or not!
				 */
				if (block_end > to || block_start < from) {
					void *kaddr;

					kaddr = kmap_atomic(page, KM_USER0);
					if (block_end > to)
						memset(kaddr + to, 0,
								block_end - to);
					if (block_start < from)
						memset(kaddr + block_start, 0,
								from -
								block_start);
					flush_dcache_page(page);
					kunmap_atomic(kaddr, KM_USER0);
				}
				continue;
			}
		}
		/* @bh is mapped, set it uptodate if the page is uptodate. */
		if (PageUptodate(page)) {
			if (!buffer_uptodate(bh))
				set_buffer_uptodate(bh);
			continue; 
		}
		/*
		 * The page is not uptodate. The buffer is mapped. If it is not
		 * uptodate, and it is only partially being written to, we need
		 * to read the buffer in before the write, i.e. right now.
		 */
		if (!buffer_uptodate(bh) &&
				(block_start < from || block_end > to)) {
			ll_rw_block(READ, 1, &bh);
			*wait_bh++ = bh;
		}
	} while (block++, block_start = block_end,
			(bh = bh->b_this_page) != head);

	/* Release the lock if we took it. */
	if (rl) {
		up_read(&ni->run_list.lock);
		rl = NULL;
	}

	/* If we issued read requests, let them complete. */
	while (wait_bh > wait) {
		wait_on_buffer(*--wait_bh);
		if (!buffer_uptodate(*wait_bh))
			return -EIO;
	}

	ntfs_debug("Done.");
	return 0;
err_out:
	/*
	 * Zero out any newly allocated blocks to avoid exposing stale data.
	 * If BH_New is set, we know that the block was newly allocated in the
	 * above loop.
	 * FIXME: What about initialized_size increments? Have we done all the
	 * required zeroing above? If not this error handling is broken, and
	 * in particular the if (block_end <= from) check is completely bogus.
	 */
	bh = head;
	block_start = 0;
	is_retry = FALSE;
	do {
		block_end = block_start + blocksize;
		if (block_end <= from)
			continue;
		if (block_start >= to)
			break;
		if (buffer_new(bh)) {
			void *kaddr;

			clear_buffer_new(bh);
			if (buffer_uptodate(bh))
				buffer_error();
			kaddr = kmap_atomic(page, KM_USER0);
			memset(kaddr + block_start, 0, bh->b_size);
			kunmap_atomic(kaddr, KM_USER0);
			set_buffer_uptodate(bh);
			mark_buffer_dirty(bh);
			is_retry = TRUE;
		}
	} while (block_start = block_end, (bh = bh->b_this_page) != head);
	if (is_retry)
		flush_dcache_page(page);
	if (rl)
		up_read(&ni->run_list.lock);
	return err;
}

/**
 * ntfs_prepare_write - prepare a page for receiving data
 *
 * This is called from generic_file_write() with i_sem held on the inode
 * (@page->mapping->host). The @page is locked and kmap()ped so page_address()
 * can simply be used. The source data has not yet been copied into the @page.
 *
 * Need to extend the attribute/fill in holes if necessary, create blocks and
 * make partially overwritten blocks uptodate,
 *
 * i_size is not to be modified yet.
 *
 * Return 0 on success or -errno on error.
 *
 * Should be using block_prepare_write() [support for sparse files] or
 * cont_prepare_write() [no support for sparse files]. Can't do that due to
 * ntfs specifics but can look at them for implementation guidancea.
 *
 * Note: In the range, @from is inclusive and @to is exclusive, i.e. @from is
 * the first byte in the page that will be written to and @to is the first byte
 * after the last byte that will be written to.
 */
static int ntfs_prepare_write(struct file *file, struct page *page,
		unsigned from, unsigned to)
{
	struct inode *vi = page->mapping->host;
	ntfs_inode   *ni = NTFS_I(vi);

	ntfs_debug("Entering for inode %li, attribute type 0x%x, page index "
			"0x%lx, from = %u, to = %u.", vi->i_ino, ni->type,
			page->index, from, to);

	BUG_ON(!PageLocked(page));
	BUG_ON(from > PAGE_CACHE_SIZE);
	BUG_ON(to > PAGE_CACHE_SIZE);
	BUG_ON(from > to);

	if (NInoNonResident(ni)) {
		/*
		 * Only unnamed $DATA attributes can be compressed, encrypted,
		 * and/or sparse.
		 */
		if (ni->type == AT_DATA && !ni->name_len) {
			/* If file is encrypted, deny access, just like NT4. */
			if (NInoEncrypted(ni)) {
				ntfs_debug("Denying write access to encrypted "
						"file.");
				return -EACCES;
			}
			/* Compressed data streams are handled in compress.c. */
			if (NInoCompressed(ni)) {
				// TODO: Implement and replace this check with
				// return ntfs_write_compressed_block(page);
				ntfs_error(vi->i_sb, "Writing to compressed "
						"files is not supported yet. "
						"Sorry.");
				return -EOPNOTSUPP;
			}
			// TODO: Implement and remove this check.
			if (NInoSparse(ni)) {
				ntfs_error(vi->i_sb, "Writing to sparse files "
						"is not supported yet. Sorry.");
				return -EOPNOTSUPP;
			}
		}

		// TODO: Implement and remove this check.
		if (NInoMstProtected(ni)) {
			ntfs_error(vi->i_sb, "Writing to MST protected "
					"attributes is not supported yet. "
					"Sorry.");
			return -EOPNOTSUPP;
		}

		/* Normal data stream. */
		return ntfs_prepare_nonresident_write(page, from, to);
	}

	/*
	 * Attribute is resident, implying it is not compressed, encrypted, or
	 * mst protected.
	 */
	BUG_ON(page_has_buffers(page));

	/* Do we need to resize the attribute? */
	if (((s64)page->index << PAGE_CACHE_SHIFT) + to > vi->i_size) {
		// TODO: Implement resize...
		ntfs_error(vi->i_sb, "Writing beyond the existing file size is "
				"not supported yet. Sorry.");
		return -EOPNOTSUPP;
	}

	/*
	 * Because resident attributes are handled by memcpy() to/from the
	 * corresponding MFT record, and because this form of i/o is byte
	 * aligned rather than block aligned, there is no need to bring the
	 * page uptodate here as in the non-resident case where we need to
	 * bring the buffers straddled by the write uptodate before
	 * generic_file_write() does the copying from userspace.
	 *
	 * We thus defer the uptodate bringing of the page region outside the
	 * region written to to ntfs_commit_write(). The reason for doing this
	 * is that we save one round of:
	 *	map_mft_record(), get_attr_search_ctx(), lookup_attr(),
	 *	kmap_atomic(), kunmap_atomic(), put_attr_search_ctx(),
	 *	unmap_mft_record().
	 * Which is obviously a very worthwhile save.
	 *
	 * Thus we just return success now...
	 */
	ntfs_debug("Done.");
	return 0;
}

/*
 * NOTES: There is a disparity between the apparent need to extend the
 * attribute in prepare write but to update i_size only in commit write.
 * Need to make sure i_sem protection is sufficient. And if not will need to
 * handle this in some way or another.
 */

/**
 * ntfs_commit_nonresident_write -
 *
 */
static int ntfs_commit_nonresident_write(struct page *page,
		unsigned from, unsigned to)
{
	s64 pos = ((s64)page->index << PAGE_CACHE_SHIFT) + to;
	struct inode *vi;
	struct buffer_head *bh, *head;
	unsigned int block_start, block_end, blocksize;
	BOOL partial;

	vi = page->mapping->host;

	ntfs_debug("Entering for inode %li, attribute type 0x%x, page index "
			"0x%lx, from = %u, to = %u.", vi->i_ino,
			NTFS_I(vi)->type, page->index, from, to);

	blocksize = 1 << vi->i_blkbits;

	// FIXME: We need a whole slew of special cases in here for MST
	// protected attributes for example. For compressed files, too...
	// For now, we know ntfs_prepare_write() would have failed so we can't
	// get here in any of the cases which we have to special case, so we
	// are just a ripped off unrolled generic_commit_write() at present.

	bh = head = page_buffers(page);
	block_start = 0;
	partial = FALSE;
	do {
		block_end = block_start + blocksize;
		if (block_end <= from || block_start >= to) {
			if (!buffer_uptodate(bh))
				partial = TRUE;
		} else {
			set_buffer_uptodate(bh);
			mark_buffer_dirty(bh);
		}
	} while (block_start = block_end, (bh = bh->b_this_page) != head);

	/*
	 * If this is a partial write which happened to make all buffers
	 * uptodate then we can optimize away a bogus ->readpage() for the next
	 * read(). Here we 'discover' whether the page went uptodate as a
	 * result of this (potentially partial) write.
	 */
	if (!partial)
		SetPageUptodate(page);

	/*
	 * Not convinced about this at all. See disparity comment above. For
	 * now we know ntfs_prepare_write() would have failed in the write
	 * exceeds i_size case, so this will never trigger which is fine.
	 */
	if (pos > vi->i_size) {
		ntfs_error(vi->i_sb, "Writing beyond the existing file size is "
				"not supported yet. Sorry.");
		// vi->i_size = pos;
		// mark_inode_dirty(vi);
	}
	ntfs_debug("Done.");
	return 0;
}

/**
 * ntfs_commit_write - commit the received data
 *
 * This is called from generic_file_write() with i_sem held on the inode
 * (@page->mapping->host). The @page is locked and kmap()ped so page_address()
 * can simply be used. The source data has already been copied into the @page.
 *
 * Need to mark modified blocks dirty so they get written out later when
 * ntfs_writepage() is invoked by the VM.
 *
 * Return 0 on success or -errno on error.
 *
 * Should be using generic_commit_write(). This marks buffers uptodate and
 * dirty, sets the page uptodate if all buffers in the page are uptodate, and
 * updates i_size if the end of io is beyond i_size. In that case, it also
 * marks the inode dirty. - We could still use this (obviously except for
 * NInoMstProtected() attributes, where we will need to duplicate the core code
 * because we need our own async_io completion handler) but we could just do
 * the i_size update in prepare write, when we resize the attribute. Then
 * we would avoid the i_size update and mark_inode_dirty() happening here.
 *
 * Can't use generic_commit_write() due to ntfs specialities but can look at
 * it for implementation guidance.
 *
 * If things have gone as outlined in ntfs_prepare_write(), then we do not
 * need to do any page content modifications here at all, except in the write
 * to resident attribute case, where we need to do the uptodate bringing here
 * which we combine with the copying into the mft record which means we only
 * need to map the mft record and find the attribute record in it only once.
 */
static int ntfs_commit_write(struct file *file, struct page *page,
		unsigned from, unsigned to)
{
	s64 attr_pos;
	struct inode *vi;
	ntfs_inode *ni, *base_ni;
	char *kaddr, *kattr;
	attr_search_context *ctx;
	MFT_RECORD *m;
	u32 attr_len, bytes;
	int err;

	vi = page->mapping->host;
	ni = NTFS_I(vi);

	ntfs_debug("Entering for inode %li, attribute type 0x%x, page index "
			"0x%lx, from = %u, to = %u.", vi->i_ino, ni->type,
			page->index, from, to);

	if (NInoNonResident(ni)) {
		/*
		 * Only unnamed $DATA attributes can be compressed, encrypted,
		 * and/or sparse.
		 */
		if (ni->type == AT_DATA && !ni->name_len) {
			/* If file is encrypted, deny access, just like NT4. */
			if (NInoEncrypted(ni)) {
				// Should never get here!
				ntfs_debug("Denying write access to encrypted "
						"file.");
				return -EACCES;
			}
			/* Compressed data streams are handled in compress.c. */
			if (NInoCompressed(ni)) {
				// TODO: Implement and replace this check with
				// return ntfs_write_compressed_block(page);
				// Should never get here!
				ntfs_error(vi->i_sb, "Writing to compressed "
						"files is not supported yet. "
						"Sorry.");
				return -EOPNOTSUPP;
			}
			// TODO: Implement and remove this check.
			if (NInoSparse(ni)) {
				// Should never get here!
				ntfs_error(vi->i_sb, "Writing to sparse files "
						"is not supported yet. Sorry.");
				return -EOPNOTSUPP;
			}
		}

		// TODO: Implement and remove this check.
		if (NInoMstProtected(ni)) {
			// Should never get here!
			ntfs_error(vi->i_sb, "Writing to MST protected "
					"attributes is not supported yet. "
					"Sorry.");
			return -EOPNOTSUPP;
		}

		/* Normal data stream. */
		return ntfs_commit_nonresident_write(page, from, to);
	}

	/*
	 * Attribute is resident, implying it is not compressed, encrypted, or
	 * mst protected.
	 */

	/* Do we need to resize the attribute? */
	if (((s64)page->index << PAGE_CACHE_SHIFT) + to > vi->i_size) {
		// TODO: Implement resize...
		// pos = ((s64)page->index << PAGE_CACHE_SHIFT) + to;
		// vi->i_size = pos;
		// mark_inode_dirty(vi);
		// Should never get here!
		ntfs_error(vi->i_sb, "Writing beyond the existing file size is "
				"not supported yet. Sorry.");
		return -EOPNOTSUPP;
	}

	if (!NInoAttr(ni))
		base_ni = ni;
	else
		base_ni = ni->ext.base_ntfs_ino;

	/* Map, pin, and lock the mft record. */
	m = map_mft_record(base_ni);
	if (unlikely(IS_ERR(m))) {
		err = PTR_ERR(m);
		m = NULL;
		ctx = NULL;
		goto err_out;
	}
	ctx = get_attr_search_ctx(base_ni, m);
	if (unlikely(!ctx)) {
		err = -ENOMEM;
		goto err_out;
	}
	if (unlikely(!lookup_attr(ni->type, ni->name, ni->name_len,
			IGNORE_CASE, 0, NULL, 0, ctx))) {
		err = -ENOENT;
		goto err_out;
	}

	/* Starting position of the page within the attribute value. */
	attr_pos = page->index << PAGE_CACHE_SHIFT;

	/* The total length of the attribute value. */
	attr_len = le32_to_cpu(ctx->attr->data.resident.value_length);

	if (unlikely(vi->i_size != attr_len)) {
		ntfs_error(vi->i_sb, "BUG()! i_size (0x%Lx) doesn't match "
				"attr_len (0x%x). Aborting write.", vi->i_size,
				attr_len);
		err = -EIO;
		goto err_out;
	}
	if (unlikely(attr_pos >= attr_len)) {
		ntfs_error(vi->i_sb, "BUG()! attr_pos (0x%Lx) > attr_len (0x%x)"
				". Aborting write.", attr_pos, attr_len);
		err = -EIO;
		goto err_out;
	}

	bytes = attr_len - attr_pos;
	if (unlikely(bytes > PAGE_CACHE_SIZE))
		bytes = PAGE_CACHE_SIZE;

	/*
	 * Calculate the address of the attribute value corresponding to the
	 * beginning of the current data @page.
	 */
	kattr = (u8*)ctx->attr + le16_to_cpu(
			ctx->attr->data.resident.value_offset) + attr_pos;

	kaddr = kmap_atomic(page, KM_USER0);

	/* Copy the received data from the page to the mft record. */
	memcpy(kattr + from, kaddr + from, to - from);
	flush_dcache_mft_record_page(ctx->ntfs_ino);

	if (!PageUptodate(page)) {
		/*
		 * Bring the out of bounds area(s) uptodate by copying data
		 * from the mft record to the page.
	 	 */
		if (from > 0)
			memcpy(kaddr, kattr, from);
		if (to < bytes)
			memcpy(kaddr + to, kattr + to, bytes - to);

		/* Zero the region outside the end of the attribute value. */
		if (likely(bytes < PAGE_CACHE_SIZE))
			memset(kaddr + bytes, 0, PAGE_CACHE_SIZE - bytes);

		/*
		 * The probability of not having done any of the above is
		 * extremely small, so we just flush unconditionally.
		 */
		flush_dcache_page(page);
		SetPageUptodate(page);
	}
	kunmap_atomic(kaddr, KM_USER0);

	// TODO: Mark mft record dirty so it gets written back.
	ntfs_error(vi->i_sb, "Writing to resident files is not supported yet. "
			"Wrote to memory only...");

	put_attr_search_ctx(ctx);
	unmap_mft_record(base_ni);
	ntfs_debug("Done.");
	return 0;
err_out:
	if (err == -ENOMEM) {
		ntfs_warning(vi->i_sb, "Error allocating memory required to "
				"commit the write.");
		if (PageUptodate(page)) {
			ntfs_warning(vi->i_sb, "Page is uptodate, setting "
					"dirty so the write will be retried "
					"later on by the VM.");
			/*
			 * Put the page on mapping->dirty_pages, but leave its
			 * buffer's dirty state as-is.
			 */
			__set_page_dirty_nobuffers(page);
			err = 0;
		} else
			ntfs_error(vi->i_sb, "Page is not uptodate. Written "
					"data has been lost. )-:");
	} else {
		ntfs_error(vi->i_sb, "Resident attribute write failed with "
				"error %i. Setting page error flag.", -err);
		SetPageError(page);
	}
	if (ctx)
		put_attr_search_ctx(ctx);
	if (m)
		unmap_mft_record(base_ni);
	return err;
}

#endif	/* NTFS_RW */

/**
 * ntfs_aops - general address space operations for inodes and attributes
 */
struct address_space_operations ntfs_aops = {
	.readpage	= ntfs_readpage,	/* Fill page with data. */
	.sync_page	= block_sync_page,	/* Currently, just unplugs the
						   disk request queue. */
#ifdef NTFS_RW
	.writepage	= ntfs_writepage,	/* Write dirty page to disk. */
	.prepare_write	= ntfs_prepare_write,	/* Prepare page and buffers
						   ready to receive data. */
	.commit_write	= ntfs_commit_write,	/* Commit received data. */
#endif
};

