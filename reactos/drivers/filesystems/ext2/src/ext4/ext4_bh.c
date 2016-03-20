#include <ext2fs.h>
#include <linux/module.h>
#include <linux/errno.h>

/*
 * extents_bread: This function is a wrapper of CcPinRead routine.
 * 
 * @sb:    the device we need to undergo buffered IO on.
 * @block: the block we want to read from.
 *
 * If the call to this routine succeeds, the pages underlying the buffer header
 * will be locked into memory, so that the buffer header returned for use is safe.
 */
struct buffer_head *
extents_bread(struct super_block *sb, sector_t block)
{
    return sb_getblk(sb, block);
}

/*
 * extents_bwrite: This function is a wrapper of CcPreparePinWrite routine.
 * 
 * @sb:    the device we need to undergo buffered IO on.
 * @block: the block we want to write to.
 */
struct buffer_head *
extents_bwrite(struct super_block *sb, sector_t block)
{
    return sb_getblk_zero(sb, block);

}

/*
 * extents_mark_buffer_dirty: Mark the buffer dirtied and so
 *                            that changes will be written back.
 * 
 * @bh: The corresponding buffer header that is modified.
 */
void extents_mark_buffer_dirty(struct buffer_head *bh)
{
    set_buffer_dirty(bh);
}

/*
 * extents_brelse: Release the corresponding buffer header.
 *
 * @bh: The corresponding buffer header that is going to be freed.
 *
 * The pages underlying the buffer header will be unlocked.
 */
void extents_brelse(struct buffer_head *bh)
{
    brelse(bh);
}

/*
 * extents_bforget: Release the corresponding buffer header.
 * NOTE: The page owned by @bh will be marked invalidated.
 *
 * @bh: The corresponding buffer header that is going to be freed.
 *
 * The pages underlying the buffer header will be unlocked.
 */
void extents_bforget(struct buffer_head *bh)
{
    clear_buffer_uptodate(bh);
    bforget(bh);
}
