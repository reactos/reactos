#ifndef _EXT4_JBD2_H
#define _EXT4_JBD2_H

/*
 * Wrapper functions with which ext4 calls into JBD.
 */
void ext4_journal_abort_handle(const char *caller, unsigned int line,
			       const char *err_fn, struct buffer_head *bh,
			       handle_t *handle, int err);

int __ext4_handle_dirty_super(const char *where, unsigned int line,
			      handle_t *handle, struct super_block *sb);

int __ext4_journal_get_write_access(const char *where, unsigned int line,
				    void *icb, handle_t *handle, struct buffer_head *bh);

int __ext4_forget(const char *where, unsigned int line, void *icb, handle_t *handle,
		  int is_metadata, struct inode *inode,
		  struct buffer_head *bh, ext4_fsblk_t blocknr);

int __ext4_journal_get_create_access(const char *where, unsigned int line,
				void *icb, handle_t *handle, struct buffer_head *bh);

int __ext4_handle_dirty_metadata(const char *where, unsigned int line,
				 void *icb, handle_t *handle, struct inode *inode,
				 struct buffer_head *bh);

#define ext4_journal_get_write_access(handle, icb, bh) \
	__ext4_journal_get_write_access("", __LINE__, (icb), (handle), (bh))
#define ext4_forget(handle, icb, is_metadata, inode, bh, block_nr) \
	__ext4_forget("", __LINE__, (icb), (handle), (is_metadata), (inode), \
		      (bh), (block_nr))
#define ext4_journal_get_create_access(handle, icb, bh) \
	__ext4_journal_get_create_access("", __LINE__, (icb), (handle), (bh))
#define ext4_handle_dirty_metadata(handle, icb, inode, bh) \
	__ext4_handle_dirty_metadata("", __LINE__, (icb), (handle), (inode), \
				     (bh))

handle_t *__ext4_journal_start_sb(void *icb, struct super_block *sb, unsigned int line,
				  int type, int blocks, int rsv_blocks);
int __ext4_journal_stop(const char *where, unsigned int line, void *icb, handle_t *handle);

#define ext4_journal_start_sb(icb, sb, type, nblocks)			\
	__ext4_journal_start_sb((icb), (sb), __LINE__, (type), (nblocks), 0)

#define ext4_journal_start(icb, inode, type, nblocks)			\
	__ext4_journal_start((icb), (inode), __LINE__, (type), (nblocks), 0)

static inline handle_t *__ext4_journal_start(void *icb, struct inode *inode,
					     unsigned int line, int type,
					     int blocks, int rsv_blocks)
{
	return __ext4_journal_start_sb(icb, inode->i_sb, line, type, blocks,
				       rsv_blocks);
}

#define ext4_journal_stop(icb, handle) \
	__ext4_journal_stop("", __LINE__, (icb), (handle))

static inline int ext4_journal_extend(void *icb, handle_t *handle, int nblocks)
{
	return 0;
}

#endif
