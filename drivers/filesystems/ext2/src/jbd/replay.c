
#include <linux/module.h>
#include <linux/time.h>
#include <linux/fs.h>
#include <linux/jbd.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/mm.h>
#include <linux/freezer.h>
#include <linux/pagemap.h>
#include <linux/kthread.h>
#include <linux/poison.h>
#include <linux/proc_fs.h>
#include <linux/debugfs.h>


/*
 * Called under j_state_lock.  Returns true if a transaction was started.
 */
int __log_start_commit(journal_t *journal, tid_t target)
{
    /*
     * Are we already doing a recent enough commit?
     */
    if (!tid_geq(journal->j_commit_request, target)) {
        /*
         * We want a new commit: OK, mark the request and wakup the
         * commit thread.  We do _not_ do the commit ourselves.
         */

        journal->j_commit_request = target;
        jbd_debug(1, "JBD: requesting commit %d/%d\n",
                  journal->j_commit_request,
                  journal->j_commit_sequence);
        wake_up(&journal->j_wait_commit);
        return 1;
    }
    return 0;
}

int log_start_commit(journal_t *journal, tid_t tid)
{
    int ret;

    spin_lock(&journal->j_state_lock);
    ret = __log_start_commit(journal, tid);
    spin_unlock(&journal->j_state_lock);
    return ret;
}

/*
 * Journal abort has very specific semantics, which we describe
 * for journal abort.
 *
 * Two internal function, which provide abort to te jbd layer
 * itself are here.
 */

/*
 * Quick version for internal journal use (doesn't lock the journal).
 * Aborts hard --- we mark the abort as occurred, but do _nothing_ else,
 * and don't attempt to make any other journal updates.
 */
static void __journal_abort_hard(journal_t *journal)
{
    transaction_t *transaction;

    if (journal->j_flags & JFS_ABORT)
        return;

    spin_lock(&journal->j_state_lock);
    journal->j_flags |= JFS_ABORT;
    transaction = journal->j_running_transaction;
    if (transaction)
        __log_start_commit(journal, transaction->t_tid);
    spin_unlock(&journal->j_state_lock);
}

/* Soft abort: record the abort error status in the journal superblock,
 * but don't do any other IO. */
static void __journal_abort_soft (journal_t *journal, int err)
{
    if (journal->j_flags & JFS_ABORT)
        return;

    if (!journal->j_errno)
        journal->j_errno = err;

    __journal_abort_hard(journal);

    if (err)
        journal_update_superblock(journal, 1);
}


/**
 * void journal_abort () - Shutdown the journal immediately.
 * @journal: the journal to shutdown.
 * @errno:   an error number to record in the journal indicating
 *           the reason for the shutdown.
 *
 * Perform a complete, immediate shutdown of the ENTIRE
 * journal (not of a single transaction).  This operation cannot be
 * undone without closing and reopening the journal.
 *
 * The journal_abort function is intended to support higher level error
 * recovery mechanisms such as the ext2/ext3 remount-readonly error
 * mode.
 *
 * Journal abort has very specific semantics.  Any existing dirty,
 * unjournaled buffers in the main filesystem will still be written to
 * disk by bdflush, but the journaling mechanism will be suspended
 * immediately and no further transaction commits will be honoured.
 *
 * Any dirty, journaled buffers will be written back to disk without
 * hitting the journal.  Atomicity cannot be guaranteed on an aborted
 * filesystem, but we _do_ attempt to leave as much data as possible
 * behind for fsck to use for cleanup.
 *
 * Any attempt to get a new transaction handle on a journal which is in
 * ABORT state will just result in an -EROFS error return.  A
 * journal_stop on an existing handle will return -EIO if we have
 * entered abort state during the update.
 *
 * Recursive transactions are not disturbed by journal abort until the
 * final journal_stop, which will receive the -EIO error.
 *
 * Finally, the journal_abort call allows the caller to supply an errno
 * which will be recorded (if possible) in the journal superblock.  This
 * allows a client to record failure conditions in the middle of a
 * transaction without having to complete the transaction to record the
 * failure to disk.  ext3_error, for example, now uses this
 * functionality.
 *
 * Errors which originate from within the journaling layer will NOT
 * supply an errno; a null errno implies that absolutely no further
 * writes are done to the journal (unless there are any already in
 * progress).
 *
 */

void journal_abort(journal_t *journal, int err)
{
    __journal_abort_soft(journal, err);
}

/**
 * int journal_errno () - returns the journal's error state.
 * @journal: journal to examine.
 *
 * This is the errno numbet set with journal_abort(), the last
 * time the journal was mounted - if the journal was stopped
 * without calling abort this will be 0.
 *
 * If the journal has been aborted on this mount time -EROFS will
 * be returned.
 */
int journal_errno(journal_t *journal)
{
    int err;

    spin_lock(&journal->j_state_lock);
    if (journal->j_flags & JFS_ABORT)
        err = -EROFS;
    else
        err = journal->j_errno;
    spin_unlock(&journal->j_state_lock);
    return err;
}

/**
 * int journal_clear_err () - clears the journal's error state
 * @journal: journal to act on.
 *
 * An error must be cleared or Acked to take a FS out of readonly
 * mode.
 */
int journal_clear_err(journal_t *journal)
{
    int err = 0;

    spin_lock(&journal->j_state_lock);
    if (journal->j_flags & JFS_ABORT)
        err = -EROFS;
    else
        journal->j_errno = 0;
    spin_unlock(&journal->j_state_lock);
    return err;
}

/**
 * void journal_ack_err() - Ack journal err.
 * @journal: journal to act on.
 *
 * An error must be cleared or Acked to take a FS out of readonly
 * mode.
 */
void journal_ack_err(journal_t *journal)
{
    spin_lock(&journal->j_state_lock);
    if (journal->j_errno)
        journal->j_flags |= JFS_ACK_ERR;
    spin_unlock(&journal->j_state_lock);
}

int journal_blocks_per_page(struct inode *inode)
{
    return 1 << (PAGE_CACHE_SHIFT - inode->i_sb->s_blocksize_bits);
}


/*
 * Journal_head storage management
 */
static struct kmem_cache *journal_head_cache = NULL;
#ifdef CONFIG_JBD_DEBUG
static atomic_t nr_journal_heads = ATOMIC_INIT(0);
#endif

static int journal_init_journal_head_cache(void)
{
    int retval;

    J_ASSERT(journal_head_cache == 0);
    journal_head_cache = kmem_cache_create("journal_head",
                                           sizeof(struct journal_head),
                                           0,		/* offset */
                                           SLAB_TEMPORARY,	/* flags */
                                           NULL);		/* ctor */
    retval = 0;
    if (journal_head_cache == 0) {
        retval = -ENOMEM;
        printk(KERN_EMERG "JBD: no memory for journal_head cache\n");
    }
    return retval;
}

static void journal_destroy_journal_head_cache(void)
{
    J_ASSERT(journal_head_cache != NULL);
    kmem_cache_destroy(journal_head_cache);
    journal_head_cache = NULL;
}

/*
 * journal_head splicing and dicing
 */
static struct journal_head *journal_alloc_journal_head(void)
{
    struct journal_head *ret;
    static unsigned long last_warning;

#ifdef CONFIG_JBD_DEBUG
    atomic_inc(&nr_journal_heads);
#endif
    ret = kmem_cache_alloc(journal_head_cache, GFP_NOFS);
    if (ret == NULL) {
        jbd_debug(1, "out of memory for journal_head\n");
        if (time_after(jiffies, last_warning + 5*HZ)) {
            printk(KERN_NOTICE "ENOMEM in %s, retrying.\n",
                   __FUNCTION__);
            last_warning = jiffies;
        }
        while (ret == NULL) {
            yield();
            ret = kmem_cache_alloc(journal_head_cache, GFP_NOFS);
        }
    }
    return ret;
}

static void journal_free_journal_head(struct journal_head *jh)
{
#ifdef CONFIG_JBD_DEBUG
    atomic_dec(&nr_journal_heads);
    memset(jh, JBD_POISON_FREE, sizeof(*jh));
#endif
    kmem_cache_free(journal_head_cache, jh);
}

/*
 * A journal_head is attached to a buffer_head whenever JBD has an
 * interest in the buffer.
 *
 * Whenever a buffer has an attached journal_head, its ->b_state:BH_JBD bit
 * is set.  This bit is tested in core kernel code where we need to take
 * JBD-specific actions.  Testing the zeroness of ->b_private is not reliable
 * there.
 *
 * When a buffer has its BH_JBD bit set, its ->b_count is elevated by one.
 *
 * When a buffer has its BH_JBD bit set it is immune from being released by
 * core kernel code, mainly via ->b_count.
 *
 * A journal_head may be detached from its buffer_head when the journal_head's
 * b_transaction, b_cp_transaction and b_next_transaction pointers are NULL.
 * Various places in JBD call journal_remove_journal_head() to indicate that the
 * journal_head can be dropped if needed.
 *
 * Various places in the kernel want to attach a journal_head to a buffer_head
 * _before_ attaching the journal_head to a transaction.  To protect the
 * journal_head in this situation, journal_add_journal_head elevates the
 * journal_head's b_jcount refcount by one.  The caller must call
 * journal_put_journal_head() to undo this.
 *
 * So the typical usage would be:
 *
 *	(Attach a journal_head if needed.  Increments b_jcount)
 *	struct journal_head *jh = journal_add_journal_head(bh);
 *	...
 *	jh->b_transaction = xxx;
 *	journal_put_journal_head(jh);
 *
 * Now, the journal_head's b_jcount is zero, but it is safe from being released
 * because it has a non-zero b_transaction.
 */

/*
 * Give a buffer_head a journal_head.
 *
 * Doesn't need the journal lock.
 * May sleep.
 */
struct journal_head *journal_add_journal_head(struct buffer_head *bh)
{
    struct journal_head *jh;
    struct journal_head *new_jh = NULL;

repeat:
    if (!buffer_jbd(bh)) {
        new_jh = journal_alloc_journal_head();
        memset(new_jh, 0, sizeof(*new_jh));
    }

    jbd_lock_bh_journal_head(bh);
    if (buffer_jbd(bh)) {
        jh = bh2jh(bh);
    } else {
        J_ASSERT_BH(bh,
                    (atomic_read(&bh->b_count) > 0) ||
                    (bh->b_page && bh->b_page->mapping));

        if (!new_jh) {
            jbd_unlock_bh_journal_head(bh);
            goto repeat;
        }

        jh = new_jh;
        new_jh = NULL;		/* We consumed it */
        set_buffer_jbd(bh);
        bh->b_private = jh;
        jh->b_bh = bh;
        get_bh(bh);
        BUFFER_TRACE(bh, "added journal_head");
    }
    jh->b_jcount++;
    jbd_unlock_bh_journal_head(bh);
    if (new_jh)
        journal_free_journal_head(new_jh);
    return bh->b_private;
}

/*
 * Grab a ref against this buffer_head's journal_head.  If it ended up not
 * having a journal_head, return NULL
 */
struct journal_head *journal_grab_journal_head(struct buffer_head *bh)
{
    struct journal_head *jh = NULL;

    jbd_lock_bh_journal_head(bh);
    if (buffer_jbd(bh)) {
        jh = bh2jh(bh);
        jh->b_jcount++;
    }
    jbd_unlock_bh_journal_head(bh);
    return jh;
}

static void __journal_remove_journal_head(struct buffer_head *bh)
{
    struct journal_head *jh = bh2jh(bh);

    J_ASSERT_JH(jh, jh->b_jcount >= 0);

    get_bh(bh);
    if (jh->b_jcount == 0) {
        if (jh->b_transaction == NULL &&
                jh->b_next_transaction == NULL &&
                jh->b_cp_transaction == NULL) {
            J_ASSERT_JH(jh, jh->b_jlist == BJ_None);
            J_ASSERT_BH(bh, buffer_jbd(bh));
            J_ASSERT_BH(bh, jh2bh(jh) == bh);
            BUFFER_TRACE(bh, "remove journal_head");
            if (jh->b_frozen_data) {
                printk(KERN_WARNING "%s: freeing "
                       "b_frozen_data\n",
                       __FUNCTION__);
                jbd_free(jh->b_frozen_data, bh->b_size);
            }
            if (jh->b_committed_data) {
                printk(KERN_WARNING "%s: freeing "
                       "b_committed_data\n",
                       __FUNCTION__);
                jbd_free(jh->b_committed_data, bh->b_size);
            }
            bh->b_private = NULL;
            jh->b_bh = NULL;	/* debug, really */
            clear_buffer_jbd(bh);
            __brelse(bh);
            journal_free_journal_head(jh);
        } else {
            BUFFER_TRACE(bh, "journal_head was locked");
        }
    }
}

/*
 * journal_remove_journal_head(): if the buffer isn't attached to a transaction
 * and has a zero b_jcount then remove and release its journal_head.   If we did
 * see that the buffer is not used by any transaction we also "logically"
 * decrement ->b_count.
 *
 * We in fact take an additional increment on ->b_count as a convenience,
 * because the caller usually wants to do additional things with the bh
 * after calling here.
 * The caller of journal_remove_journal_head() *must* run __brelse(bh) at some
 * time.  Once the caller has run __brelse(), the buffer is eligible for
 * reaping by try_to_free_buffers().
 */
void journal_remove_journal_head(struct buffer_head *bh)
{
    jbd_lock_bh_journal_head(bh);
    __journal_remove_journal_head(bh);
    jbd_unlock_bh_journal_head(bh);
}

/*
 * Drop a reference on the passed journal_head.  If it fell to zero then try to
 * release the journal_head from the buffer_head.
 */
void journal_put_journal_head(struct journal_head *jh)
{
    struct buffer_head *bh = jh2bh(jh);

    jbd_lock_bh_journal_head(bh);
    J_ASSERT_JH(jh, jh->b_jcount > 0);
    --jh->b_jcount;
    if (!jh->b_jcount && !jh->b_transaction) {
        __journal_remove_journal_head(bh);
        __brelse(bh);
    }
    jbd_unlock_bh_journal_head(bh);
}

/*
 * Log buffer allocation routines:
 */

int journal_next_log_block(journal_t *journal, unsigned long *retp)
{
    unsigned long blocknr;

    spin_lock(&journal->j_state_lock);
    J_ASSERT(journal->j_free > 1);

    blocknr = journal->j_head;
    journal->j_head++;
    journal->j_free--;
    if (journal->j_head == journal->j_last)
        journal->j_head = journal->j_first;
    spin_unlock(&journal->j_state_lock);
    return journal_bmap(journal, blocknr, retp);
}

/*
 * Conversion of logical to physical block numbers for the journal
 *
 * On external journals the journal blocks are identity-mapped, so
 * this is a no-op.  If needed, we can use j_blk_offset - everything is
 * ready.
 */
int journal_bmap(journal_t *journal, unsigned long blocknr,
                 unsigned long *retp)
{
    int err = 0;
    unsigned long ret;

    if (journal->j_inode) {
        ret = (unsigned long)bmap(journal->j_inode, (sector_t)blocknr);
        if (ret)
            *retp = ret;
        else {
            printk(KERN_ALERT "%s: journal block not found "
                   "at offset %lu ...\n",
                   __FUNCTION__,
                   blocknr);
            err = -EIO;
            __journal_abort_soft(journal, err);
        }
    } else {
        *retp = blocknr; /* +journal->j_blk_offset */
    }
    return err;
}

/*
 * We play buffer_head aliasing tricks to write data/metadata blocks to
 * the journal without copying their contents, but for journal
 * descriptor blocks we do need to generate bona fide buffers.
 *
 * After the caller of journal_get_descriptor_buffer() has finished modifying
 * the buffer's contents they really should run flush_dcache_page(bh->b_page).
 * But we don't bother doing that, so there will be coherency problems with
 * mmaps of blockdevs which hold live JBD-controlled filesystems.
 */
struct journal_head *journal_get_descriptor_buffer(journal_t *journal)
{
    struct buffer_head *bh;
    unsigned long blocknr;
    int err;

    err = journal_next_log_block(journal, &blocknr);

    if (err)
        return NULL;

    bh = __getblk(journal->j_dev, blocknr, journal->j_blocksize);
    lock_buffer(bh);
    memset(bh->b_data, 0, journal->j_blocksize);
    set_buffer_uptodate(bh);
    unlock_buffer(bh);
    BUFFER_TRACE(bh, "return this buffer");
    return journal_add_journal_head(bh);
}

/*
 * Management for journal control blocks: functions to create and
 * destroy journal_t structures, and to initialise and read existing
 * journal blocks from disk.  */

/* First: create and setup a journal_t object in memory.  We initialise
 * very few fields yet: that has to wait until we have created the
 * journal structures from from scratch, or loaded them from disk. */

static journal_t * journal_init_common (void)
{
    journal_t *journal;
    int err;

    journal = kzalloc(sizeof(*journal), GFP_KERNEL);
    if (!journal)
        goto fail;

    init_waitqueue_head(&journal->j_wait_transaction_locked);
    init_waitqueue_head(&journal->j_wait_logspace);
    init_waitqueue_head(&journal->j_wait_done_commit);
    init_waitqueue_head(&journal->j_wait_checkpoint);
    init_waitqueue_head(&journal->j_wait_commit);
    init_waitqueue_head(&journal->j_wait_updates);
    mutex_init(&journal->j_barrier);
    mutex_init(&journal->j_checkpoint_mutex);
    spin_lock_init(&journal->j_revoke_lock);
    spin_lock_init(&journal->j_list_lock);
    spin_lock_init(&journal->j_state_lock);

    journal->j_commit_interval = (HZ * JBD_DEFAULT_MAX_COMMIT_AGE);

    /* The journal is marked for error until we succeed with recovery! */
    journal->j_flags = JFS_ABORT;

    /* Set up a default-sized revoke table for the new mount. */
    err = journal_init_revoke(journal, JOURNAL_REVOKE_DEFAULT_HASH);
    if (err) {
        kfree(journal);
        goto fail;
    }
    return journal;
fail:
    return NULL;
}

/**
 *  journal_t * journal_init_inode () - creates a journal which maps to a inode.
 *  @inode: An inode to create the journal in
 *
 * journal_init_inode creates a journal which maps an on-disk inode as
 * the journal.  The inode must exist already, must support bmap() and
 * must have all data blocks preallocated.
 */
journal_t * journal_init_inode (struct inode *inode)
{
    struct buffer_head *bh;
    journal_t *journal = journal_init_common();
    int err;
    int n;
    unsigned long blocknr;

    if (!journal)
        return NULL;

    journal->j_dev = journal->j_fs_dev = inode->i_sb->s_bdev;
    journal->j_inode = inode;
    jbd_debug(1,
              "journal %p: inode %s/%ld, size %Ld, bits %d, blksize %ld\n",
              journal, inode->i_sb->s_id, inode->i_ino,
              (s64) inode->i_size,
              inode->i_sb->s_blocksize_bits, inode->i_sb->s_blocksize);

    journal->j_maxlen = (unsigned int)(inode->i_size >> inode->i_sb->s_blocksize_bits);
    journal->j_blocksize = inode->i_sb->s_blocksize;

    /* journal descriptor can store up to n blocks -bzzz */
    n = journal->j_blocksize / sizeof(journal_block_tag_t);
    journal->j_wbufsize = n;
    journal->j_wbuf = kmalloc(n * sizeof(struct buffer_head*), GFP_KERNEL);
    if (!journal->j_wbuf) {
        printk(KERN_ERR "%s: Cant allocate bhs for commit thread\n",
               __FUNCTION__);

        J_ASSERT(journal->j_revoke != NULL);
        if (journal->j_revoke)
            journal_destroy_revoke(journal);

        kfree(journal);
        return NULL;
    }

    err = journal_bmap(journal, 0, &blocknr);
    /* If that failed, give up */
    if (err) {
        printk(KERN_ERR "%s: Cannnot locate journal superblock\n",
               __FUNCTION__);

        J_ASSERT(journal->j_revoke != NULL);
        if (journal->j_revoke)
            journal_destroy_revoke(journal);
        J_ASSERT(journal->j_wbuf != NULL);
        kfree(journal->j_wbuf);
        kfree(journal);
        return NULL;
    }

    bh = __getblk(journal->j_dev, blocknr, journal->j_blocksize);
    J_ASSERT(bh != NULL);
    journal->j_sb_buffer = bh;
    journal->j_superblock = (journal_superblock_t *)bh->b_data;

    return journal;
}

/**
 *
 *   wipe all journal data ...
 *
 */

void journal_wipe_recovery(journal_t *journal)
{
    /* We can now mark the journal as empty. */

    journal->j_tail = 0;
    if (journal->j_sb_buffer) {
        journal_update_superblock(journal, 0);
        brelse(journal->j_sb_buffer);
        journal->j_sb_buffer = NULL;
    }
}

/**
 * void journal_destroy() - Release a journal_t structure.
 * @journal: Journal to act on.
 *
 * Release a journal_t structure once it is no longer in use by the
 * journaled object.
 */
void journal_destroy(journal_t *journal)
{
#if 0
    /* Wait for the commit thread to wake up and die. */
    journal_kill_thread(journal);

    /* Force a final log commit */
    if (journal->j_running_transaction)
        journal_commit_transaction(journal);

    /* Force any old transactions to disk */

    /* Totally anal locking here... */
    spin_lock(&journal->j_list_lock);
    while (journal->j_checkpoint_transactions != NULL) {
        spin_unlock(&journal->j_list_lock);
        log_do_checkpoint(journal);
        spin_lock(&journal->j_list_lock);
    }

    J_ASSERT(journal->j_running_transaction == NULL);
    J_ASSERT(journal->j_committing_transaction == NULL);
    J_ASSERT(journal->j_checkpoint_transactions == NULL);
    spin_unlock(&journal->j_list_lock);

    /* We can now mark the journal as empty. */
    journal->j_tail = 0;
    journal->j_tail_sequence = ++journal->j_transaction_sequence;
    if (journal->j_sb_buffer) {
        journal_update_superblock(journal, 1);
        brelse(journal->j_sb_buffer);
    }
#endif

    if (journal->j_sb_buffer) {
        brelse(journal->j_sb_buffer);
    }
    if (journal->j_inode)
        iput(journal->j_inode);
    if (journal->j_revoke)
        journal_destroy_revoke(journal);
    kfree(journal->j_wbuf);
    kfree(journal);
}



/**
 *int journal_check_used_features () - Check if features specified are used.
 * @journal: Journal to check.
 * @compat: bitmask of compatible features
 * @ro: bitmask of features that force read-only mount
 * @incompat: bitmask of incompatible features
 *
 * Check whether the journal uses all of a given set of
 * features.  Return true (non-zero) if it does.
 **/

int journal_check_used_features (journal_t *journal, unsigned long compat,
                                 unsigned long ro, unsigned long incompat)
{
    journal_superblock_t *sb;

    if (!compat && !ro && !incompat)
        return 1;
    if (journal->j_format_version == 1)
        return 0;

    sb = journal->j_superblock;

    if (((be32_to_cpu(sb->s_feature_compat) & compat) == compat) &&
            ((be32_to_cpu(sb->s_feature_ro_compat) & ro) == ro) &&
            ((be32_to_cpu(sb->s_feature_incompat) & incompat) == incompat))
        return 1;

    return 0;
}

/**
 * int journal_check_available_features() - Check feature set in journalling layer
 * @journal: Journal to check.
 * @compat: bitmask of compatible features
 * @ro: bitmask of features that force read-only mount
 * @incompat: bitmask of incompatible features
 *
 * Check whether the journaling code supports the use of
 * all of a given set of features on this journal.  Return true
 * (non-zero) if it can. */

int journal_check_available_features (journal_t *journal, unsigned long compat,
                                      unsigned long ro, unsigned long incompat)
{
    journal_superblock_t *sb;

    if (!compat && !ro && !incompat)
        return 1;

    sb = journal->j_superblock;

    /* We can support any known requested features iff the
     * superblock is in version 2.  Otherwise we fail to support any
     * extended sb features. */

    if (journal->j_format_version != 2)
        return 0;

    if ((compat   & JFS_KNOWN_COMPAT_FEATURES) == compat &&
            (ro       & JFS_KNOWN_ROCOMPAT_FEATURES) == ro &&
            (incompat & JFS_KNOWN_INCOMPAT_FEATURES) == incompat)
        return 1;

    return 0;
}

/**
 * int journal_set_features () - Mark a given journal feature in the superblock
 * @journal: Journal to act on.
 * @compat: bitmask of compatible features
 * @ro: bitmask of features that force read-only mount
 * @incompat: bitmask of incompatible features
 *
 * Mark a given journal feature as present on the
 * superblock.  Returns true if the requested features could be set.
 *
 */

int journal_set_features (journal_t *journal, unsigned long compat,
                          unsigned long ro, unsigned long incompat)
{
    journal_superblock_t *sb;

    if (journal_check_used_features(journal, compat, ro, incompat))
        return 1;

    if (!journal_check_available_features(journal, compat, ro, incompat))
        return 0;

    jbd_debug(1, "Setting new features 0x%lx/0x%lx/0x%lx\n",
              compat, ro, incompat);

    sb = journal->j_superblock;

    sb->s_feature_compat    |= cpu_to_be32(compat);
    sb->s_feature_ro_compat |= cpu_to_be32(ro);
    sb->s_feature_incompat  |= cpu_to_be32(incompat);

    return 1;
}

static int journal_convert_superblock_v1(journal_t *journal,
        journal_superblock_t *sb)
{
    int offset, blocksize;
    struct buffer_head *bh;

    printk(KERN_WARNING
           "JBD: Converting superblock from version 1 to 2.\n");

    /* Pre-initialise new fields to zero */
    offset = (INT)(((INT_PTR) &(sb->s_feature_compat)) - ((INT_PTR) sb));
    blocksize = be32_to_cpu(sb->s_blocksize);
    memset(&sb->s_feature_compat, 0, blocksize-offset);

    sb->s_nr_users = cpu_to_be32(1);
    sb->s_header.h_blocktype = cpu_to_be32(JFS_SUPERBLOCK_V2);
    journal->j_format_version = 2;

    bh = journal->j_sb_buffer;
    BUFFER_TRACE(bh, "marking dirty");
    mark_buffer_dirty(bh);
    sync_dirty_buffer(bh);
    return 0;
}


/*
 * If the journal init or create aborts, we need to mark the journal
 * superblock as being NULL to prevent the journal destroy from writing
 * back a bogus superblock.
 */
static void journal_fail_superblock (journal_t *journal)
{
    struct buffer_head *bh = journal->j_sb_buffer;
    brelse(bh);
    journal->j_sb_buffer = NULL;
}


/*
 * Read the superblock for a given journal, performing initial
 * validation of the format.
 */

static int journal_get_superblock(journal_t *journal)
{
    struct buffer_head *bh;
    journal_superblock_t *sb;
    int err = -EIO;

    bh = journal->j_sb_buffer;

    J_ASSERT(bh != NULL);
    if (!buffer_uptodate(bh)) {
        ll_rw_block(READ, 1, &bh);
        wait_on_buffer(bh);
        if (!buffer_uptodate(bh)) {
            printk (KERN_ERR
                    "JBD: IO error reading journal superblock\n");
            goto out;
        }
    }

    sb = journal->j_superblock;

    err = -EINVAL;

    if (sb->s_header.h_magic != cpu_to_be32(JFS_MAGIC_NUMBER) ||
            sb->s_blocksize != cpu_to_be32(journal->j_blocksize)) {
        printk(KERN_WARNING "JBD: no valid journal superblock found\n");
        goto out;
    }

    switch (be32_to_cpu(sb->s_header.h_blocktype)) {
    case JFS_SUPERBLOCK_V1:
        journal->j_format_version = 1;
        break;
    case JFS_SUPERBLOCK_V2:
        journal->j_format_version = 2;
        break;
    default:
        printk(KERN_WARNING "JBD: unrecognised superblock format ID\n");
        goto out;
    }

    if (be32_to_cpu(sb->s_maxlen) < journal->j_maxlen)
        journal->j_maxlen = be32_to_cpu(sb->s_maxlen);
    else if (be32_to_cpu(sb->s_maxlen) > journal->j_maxlen) {
        printk (KERN_WARNING "JBD: journal file too short\n");
        goto out;
    }

    return 0;

out:
    journal_fail_superblock(journal);
    return err;
}

/*
 * Load the on-disk journal superblock and read the key fields into the
 * journal_t.
 */

static int load_superblock(journal_t *journal)
{
    int err;
    journal_superblock_t *sb;

    err = journal_get_superblock(journal);
    if (err)
        return err;

    sb = journal->j_superblock;

    journal->j_tail_sequence = be32_to_cpu(sb->s_sequence);
    journal->j_tail = be32_to_cpu(sb->s_start);
    journal->j_first = be32_to_cpu(sb->s_first);
    journal->j_last = be32_to_cpu(sb->s_maxlen);
    journal->j_errno = be32_to_cpu(sb->s_errno);

    return 0;
}

/**
 * int journal_wipe() - Wipe journal contents
 * @journal: Journal to act on.
 * @write: flag (see below)
 *
 * Wipe out all of the contents of a journal, safely.  This will produce
 * a warning if the journal contains any valid recovery information.
 * Must be called between journal_init_*() and journal_load().
 *
 * If 'write' is non-zero, then we wipe out the journal on disk; otherwise
 * we merely suppress recovery.
 */

int journal_wipe(journal_t *journal, int write)
{
    journal_superblock_t *sb;
    int err = 0;

    J_ASSERT (!(journal->j_flags & JFS_LOADED));

    err = load_superblock(journal);
    if (err)
        return err;

    sb = journal->j_superblock;

    if (!journal->j_tail)
        goto no_recovery;

    printk (KERN_WARNING "JBD: %s recovery information on journal\n",
            write ? "Clearing" : "Ignoring");

    err = journal_skip_recovery(journal);
    if (write)
        journal_update_superblock(journal, 1);

no_recovery:
    return err;
}


/**
 * int journal_update_format () - Update on-disk journal structure.
 * @journal: Journal to act on.
 *
 * Given an initialised but unloaded journal struct, poke about in the
 * on-disk structure to update it to the most recent supported version.
 */
int journal_update_format (journal_t *journal)
{
    journal_superblock_t *sb;
    int err;

    err = journal_get_superblock(journal);
    if (err)
        return err;

    sb = journal->j_superblock;

    switch (be32_to_cpu(sb->s_header.h_blocktype)) {
    case JFS_SUPERBLOCK_V2:
        return 0;
    case JFS_SUPERBLOCK_V1:
        return journal_convert_superblock_v1(journal, sb);
    default:
        break;
    }
    return -EINVAL;
}


/**
 * void journal_update_superblock() - Update journal sb on disk.
 * @journal: The journal to update.
 * @wait: Set to '0' if you don't want to wait for IO completion.
 *
 * Update a journal's dynamic superblock fields and write it to disk,
 * optionally waiting for the IO to complete.
 */
void journal_update_superblock(journal_t *journal, int wait)
{
    journal_superblock_t *sb = journal->j_superblock;
    struct buffer_head *bh = journal->j_sb_buffer;

    /*
     * As a special case, if the on-disk copy is already marked as needing
     * no recovery (s_start == 0) and there are no outstanding transactions
     * in the filesystem, then we can safely defer the superblock update
     * until the next commit by setting JFS_FLUSHED.  This avoids
     * attempting a write to a potential-readonly device.
     */
    if (sb->s_start == 0 && journal->j_tail_sequence ==
            journal->j_transaction_sequence) {
        jbd_debug(1,"JBD: Skipping superblock update on recovered sb "
                  "(start %ld, seq %d, errno %d)\n",
                  journal->j_tail, journal->j_tail_sequence,
                  journal->j_errno);
        goto out;
    }

    spin_lock(&journal->j_state_lock);
    jbd_debug(1,"JBD: updating superblock (start %ld, seq %d, errno %d)\n",
              journal->j_tail, journal->j_tail_sequence, journal->j_errno);

    sb->s_sequence = cpu_to_be32(journal->j_tail_sequence);
    sb->s_start    = cpu_to_be32(journal->j_tail);
    sb->s_errno    = cpu_to_be32(journal->j_errno);
    spin_unlock(&journal->j_state_lock);

    BUFFER_TRACE(bh, "marking dirty");
    mark_buffer_dirty(bh);
    if (wait)
        sync_dirty_buffer(bh);
    else
        ll_rw_block(SWRITE, 1, &bh);

out:
    /* If we have just flushed the log (by marking s_start==0), then
     * any future commit will have to be careful to update the
     * superblock again to re-record the true start of the log. */

    spin_lock(&journal->j_state_lock);
    if (sb->s_start)
        journal->j_flags &= ~JFS_FLUSHED;
    else
        journal->j_flags |= JFS_FLUSHED;
    spin_unlock(&journal->j_state_lock);
}

/*
 * Given a journal_t structure, initialise the various fields for
 * startup of a new journaling session.  We use this both when creating
 * a journal, and after recovering an old journal to reset it for
 * subsequent use.
 */

static int journal_reset(journal_t *journal)
{
    journal_superblock_t *sb = journal->j_superblock;
    unsigned long first, last;

    first = be32_to_cpu(sb->s_first);
    last = be32_to_cpu(sb->s_maxlen);

    journal->j_first = first;
    journal->j_last = last;

    journal->j_head = first;
    journal->j_tail = first;
    journal->j_free = last - first;

    journal->j_tail_sequence = journal->j_transaction_sequence;
    journal->j_commit_sequence = journal->j_transaction_sequence - 1;
    journal->j_commit_request = journal->j_commit_sequence;

    journal->j_max_transaction_buffers = journal->j_maxlen / 4;

    /* Add the dynamic fields and write it to disk. */
    journal_update_superblock(journal, 1);
    return 0;
}

/**
 * int journal_load() - Read journal from disk.
 * @journal: Journal to act on.
 *
 * Given a journal_t structure which tells us which disk blocks contain
 * a journal, read the journal from disk to initialise the in-memory
 * structures.
 */
int journal_load(journal_t *journal)
{
    int err;
    journal_superblock_t *sb;

    err = load_superblock(journal);
    if (err)
        return err;

    sb = journal->j_superblock;
    /* If this is a V2 superblock, then we have to check the
     * features flags on it. */

    if (journal->j_format_version >= 2) {
        if ((sb->s_feature_ro_compat &
                ~cpu_to_be32(JFS_KNOWN_ROCOMPAT_FEATURES)) ||
                (sb->s_feature_incompat &
                 ~cpu_to_be32(JFS_KNOWN_INCOMPAT_FEATURES))) {
            printk (KERN_WARNING
                    "JBD: Unrecognised features on journal\n");
            return -EINVAL;
        }
    }

    /* Let the recovery code check whether it needs to recover any
     * data from the journal. */
    if (journal_recover(journal))
        goto recovery_error;

    /* OK, we've finished with the dynamic journal bits:
     * reinitialise the dynamic contents of the superblock in memory
     * and reset them on disk. */
    if (journal_reset(journal))
        goto recovery_error;

    journal->j_flags &= ~JFS_ABORT;
    journal->j_flags |= JFS_LOADED;
    return 0;

recovery_error:
    printk (KERN_WARNING "JBD: recovery failed\n");
    return -EIO;
}


//
// transactions routines
//


/*
 *
 * List management code snippets: various functions for manipulating the
 * transaction buffer lists.
 *
 */

/*
 * Append a buffer to a transaction list, given the transaction's list head
 * pointer.
 *
 * j_list_lock is held.
 *
 * jbd_lock_bh_state(jh2bh(jh)) is held.
 */

static inline void
__blist_add_buffer(struct journal_head **list, struct journal_head *jh)
{
    if (!*list) {
        jh->b_tnext = jh->b_tprev = jh;
        *list = jh;
    } else {
        /* Insert at the tail of the list to preserve order */
        struct journal_head *first = *list, *last = first->b_tprev;
        jh->b_tprev = last;
        jh->b_tnext = first;
        last->b_tnext = first->b_tprev = jh;
    }
}

/*
 * Remove a buffer from a transaction list, given the transaction's list
 * head pointer.
 *
 * Called with j_list_lock held, and the journal may not be locked.
 *
 * jbd_lock_bh_state(jh2bh(jh)) is held.
 */

static inline void
__blist_del_buffer(struct journal_head **list, struct journal_head *jh)
{
    if (*list == jh) {
        *list = jh->b_tnext;
        if (*list == jh)
            *list = NULL;
    }
    jh->b_tprev->b_tnext = jh->b_tnext;
    jh->b_tnext->b_tprev = jh->b_tprev;
}

/*
 * Remove a buffer from the appropriate transaction list.
 *
 * Note that this function can *change* the value of
 * bh->b_transaction->t_sync_datalist, t_buffers, t_forget,
 * t_iobuf_list, t_shadow_list, t_log_list or t_reserved_list.  If the caller
 * is holding onto a copy of one of thee pointers, it could go bad.
 * Generally the caller needs to re-read the pointer from the transaction_t.
 *
 * Called under j_list_lock.  The journal may not be locked.
 */
static void __journal_temp_unlink_buffer(struct journal_head *jh)
{
    struct journal_head **list = NULL;
    transaction_t *transaction;
    struct buffer_head *bh = jh2bh(jh);

    J_ASSERT_JH(jh, jbd_is_locked_bh_state(bh));
    transaction = jh->b_transaction;
    if (transaction)
        assert_spin_locked(&transaction->t_journal->j_list_lock);

    J_ASSERT_JH(jh, jh->b_jlist < BJ_Types);
    if (jh->b_jlist != BJ_None)
        J_ASSERT_JH(jh, transaction != NULL);

    switch (jh->b_jlist) {
    case BJ_None:
        return;
    case BJ_SyncData:
        list = &transaction->t_sync_datalist;
        break;
    case BJ_Metadata:
        transaction->t_nr_buffers--;
        J_ASSERT_JH(jh, transaction->t_nr_buffers >= 0);
        list = &transaction->t_buffers;
        break;
    case BJ_Forget:
        list = &transaction->t_forget;
        break;
    case BJ_IO:
        list = &transaction->t_iobuf_list;
        break;
    case BJ_Shadow:
        list = &transaction->t_shadow_list;
        break;
    case BJ_LogCtl:
        list = &transaction->t_log_list;
        break;
    case BJ_Reserved:
        list = &transaction->t_reserved_list;
        break;
    case BJ_Locked:
        list = &transaction->t_locked_list;
        break;
    }

    __blist_del_buffer(list, jh);
    jh->b_jlist = BJ_None;
    if (test_clear_buffer_jbddirty(bh))
        mark_buffer_dirty(bh);	/* Expose it to the VM */
}

void __journal_unfile_buffer(struct journal_head *jh)
{
    __journal_temp_unlink_buffer(jh);
    jh->b_transaction = NULL;
}

void journal_unfile_buffer(journal_t *journal, struct journal_head *jh)
{
    jbd_lock_bh_state(jh2bh(jh));
    spin_lock(&journal->j_list_lock);
    __journal_unfile_buffer(jh);
    spin_unlock(&journal->j_list_lock);
    jbd_unlock_bh_state(jh2bh(jh));
}

/*
 * This buffer is no longer needed.  If it is on an older transaction's
 * checkpoint list we need to record it on this transaction's forget list
 * to pin this buffer (and hence its checkpointing transaction) down until
 * this transaction commits.  If the buffer isn't on a checkpoint list, we
 * release it.
 * Returns non-zero if JBD no longer has an interest in the buffer.
 *
 * Called under j_list_lock.
 *
 * Called under jbd_lock_bh_state(bh).
 */
static int __dispose_buffer(struct journal_head *jh, transaction_t *transaction)
{
    int may_free = 1;
    struct buffer_head *bh = jh2bh(jh);

    __journal_unfile_buffer(jh);

    if (jh->b_cp_transaction) {
        JBUFFER_TRACE(jh, "on running+cp transaction");
        __journal_file_buffer(jh, transaction, BJ_Forget);
        clear_buffer_jbddirty(bh);
        may_free = 0;
    } else {
        JBUFFER_TRACE(jh, "on running transaction");
        journal_remove_journal_head(bh);
        __brelse(bh);
    }
    return may_free;
}


/*
 * File a buffer on the given transaction list.
 */
void __journal_file_buffer(struct journal_head *jh,
                           transaction_t *transaction, int jlist)
{
    struct journal_head **list = NULL;
    int was_dirty = 0;
    struct buffer_head *bh = jh2bh(jh);

    J_ASSERT_JH(jh, jbd_is_locked_bh_state(bh));
    assert_spin_locked(&transaction->t_journal->j_list_lock);

    J_ASSERT_JH(jh, jh->b_jlist < BJ_Types);
    J_ASSERT_JH(jh, jh->b_transaction == transaction ||
                jh->b_transaction == NULL);

    if (jh->b_transaction && jh->b_jlist == (unsigned) jlist)
        return;

    /* The following list of buffer states needs to be consistent
     * with __jbd_unexpected_dirty_buffer()'s handling of dirty
     * state. */

    if (jlist == BJ_Metadata || jlist == BJ_Reserved ||
            jlist == BJ_Shadow || jlist == BJ_Forget) {
        if (test_clear_buffer_dirty(bh) ||
                test_clear_buffer_jbddirty(bh))
            was_dirty = 1;
    }

    if (jh->b_transaction)
        __journal_temp_unlink_buffer(jh);
    jh->b_transaction = transaction;

    switch (jlist) {
    case BJ_None:
        J_ASSERT_JH(jh, !jh->b_committed_data);
        J_ASSERT_JH(jh, !jh->b_frozen_data);
        return;
    case BJ_SyncData:
        list = &transaction->t_sync_datalist;
        break;
    case BJ_Metadata:
        transaction->t_nr_buffers++;
        list = &transaction->t_buffers;
        break;
    case BJ_Forget:
        list = &transaction->t_forget;
        break;
    case BJ_IO:
        list = &transaction->t_iobuf_list;
        break;
    case BJ_Shadow:
        list = &transaction->t_shadow_list;
        break;
    case BJ_LogCtl:
        list = &transaction->t_log_list;
        break;
    case BJ_Reserved:
        list = &transaction->t_reserved_list;
        break;
    case BJ_Locked:
        list =  &transaction->t_locked_list;
        break;
    }

    __blist_add_buffer(list, jh);
    jh->b_jlist = jlist;

    if (was_dirty)
        set_buffer_jbddirty(bh);
}

void journal_file_buffer(struct journal_head *jh,
                         transaction_t *transaction, int jlist)
{
    jbd_lock_bh_state(jh2bh(jh));
    spin_lock(&transaction->t_journal->j_list_lock);
    __journal_file_buffer(jh, transaction, jlist);
    spin_unlock(&transaction->t_journal->j_list_lock);
    jbd_unlock_bh_state(jh2bh(jh));
}


/*
 * journal_release_buffer: undo a get_write_access without any buffer
 * updates, if the update decided in the end that it didn't need access.
 *
 */
void
journal_release_buffer(handle_t *handle, struct buffer_head *bh)
{
    BUFFER_TRACE(bh, "entry");
}

/**
 * void journal_forget() - bforget() for potentially-journaled buffers.
 * @handle: transaction handle
 * @bh:     bh to 'forget'
 *
 * We can only do the bforget if there are no commits pending against the
 * buffer.  If the buffer is dirty in the current running transaction we
 * can safely unlink it.
 *
 * bh may not be a journalled buffer at all - it may be a non-JBD
 * buffer which came off the hashtable.  Check for this.
 *
 * Decrements bh->b_count by one.
 *
 * Allow this call even if the handle has aborted --- it may be part of
 * the caller's cleanup after an abort.
 */
int journal_forget (handle_t *handle, struct buffer_head *bh)
{
    transaction_t *transaction = handle->h_transaction;
    journal_t *journal = transaction->t_journal;
    struct journal_head *jh;
    int drop_reserve = 0;
    int err = 0;

    BUFFER_TRACE(bh, "entry");

    jbd_lock_bh_state(bh);
    spin_lock(&journal->j_list_lock);

    if (!buffer_jbd(bh))
        goto not_jbd;
    jh = bh2jh(bh);

    /* Critical error: attempting to delete a bitmap buffer, maybe?
     * Don't do any jbd operations, and return an error. */
    if (!J_EXPECT_JH(jh, !jh->b_committed_data,
                     "inconsistent data on disk")) {
        err = -EIO;
        goto not_jbd;
    }

    /*
     * The buffer's going from the transaction, we must drop
     * all references -bzzz
     */
    jh->b_modified = 0;

    if (jh->b_transaction == handle->h_transaction) {
        J_ASSERT_JH(jh, !jh->b_frozen_data);

        /* If we are forgetting a buffer which is already part
         * of this transaction, then we can just drop it from
         * the transaction immediately. */
        clear_buffer_dirty(bh);
        clear_buffer_jbddirty(bh);

        JBUFFER_TRACE(jh, "belongs to current transaction: unfile");

        drop_reserve = 1;

        /*
         * We are no longer going to journal this buffer.
         * However, the commit of this transaction is still
         * important to the buffer: the delete that we are now
         * processing might obsolete an old log entry, so by
         * committing, we can satisfy the buffer's checkpoint.
         *
         * So, if we have a checkpoint on the buffer, we should
         * now refile the buffer on our BJ_Forget list so that
         * we know to remove the checkpoint after we commit.
         */

        if (jh->b_cp_transaction) {
            __journal_temp_unlink_buffer(jh);
            __journal_file_buffer(jh, transaction, BJ_Forget);
        } else {
            __journal_unfile_buffer(jh);
            journal_remove_journal_head(bh);
            __brelse(bh);
            if (!buffer_jbd(bh)) {
                spin_unlock(&journal->j_list_lock);
                jbd_unlock_bh_state(bh);
                __bforget(bh);
                goto drop;
            }
        }
    } else if (jh->b_transaction) {
        J_ASSERT_JH(jh, (jh->b_transaction ==
                         journal->j_committing_transaction));
        /* However, if the buffer is still owned by a prior
         * (committing) transaction, we can't drop it yet... */
        JBUFFER_TRACE(jh, "belongs to older transaction");
        /* ... but we CAN drop it from the new transaction if we
         * have also modified it since the original commit. */

        if (jh->b_next_transaction) {
            J_ASSERT(jh->b_next_transaction == transaction);
            jh->b_next_transaction = NULL;
            drop_reserve = 1;
        }
    }

not_jbd:
    spin_unlock(&journal->j_list_lock);
    jbd_unlock_bh_state(bh);
    __brelse(bh);
drop:
    if (drop_reserve) {
        /* no need to reserve log space for this block -bzzz */
        handle->h_buffer_credits++;
    }
    return err;
}

/*
 * debugfs tunables
 */
#ifdef CONFIG_JBD_DEBUG

u8 journal_enable_debug __read_mostly;
EXPORT_SYMBOL(journal_enable_debug);

static struct dentry *jbd_debugfs_dir;
static struct dentry *jbd_debug;

static void __init jbd_create_debugfs_entry(void)
{
    jbd_debugfs_dir = debugfs_create_dir("jbd", NULL);
    if (jbd_debugfs_dir)
        jbd_debug = debugfs_create_u8("jbd-debug", S_IRUGO,
                                      jbd_debugfs_dir,
                                      &journal_enable_debug);
}

static void __exit jbd_remove_debugfs_entry(void)
{
    debugfs_remove(jbd_debug);
    debugfs_remove(jbd_debugfs_dir);
}

#else

static inline void jbd_create_debugfs_entry(void)
{
}

static inline void jbd_remove_debugfs_entry(void)
{
}

#endif

struct kmem_cache *jbd_handle_cache = NULL;

static int __init journal_init_handle_cache(void)
{
    jbd_handle_cache = kmem_cache_create("journal_handle",
                                         sizeof(handle_t),
                                         0,		/* offset */
                                         SLAB_TEMPORARY,	/* flags */
                                         NULL);		/* ctor */
    if (jbd_handle_cache == NULL) {
        printk(KERN_EMERG "JBD: failed to create handle cache\n");
        return -ENOMEM;
    }
    return 0;
}

static void journal_destroy_handle_cache(void)
{
    if (jbd_handle_cache)
        kmem_cache_destroy(jbd_handle_cache);
}

/*
 * Module startup and shutdown
 */

static int __init journal_init_caches(void)
{
    int ret;

    ret = journal_init_revoke_caches();
    if (ret == 0)
        ret = journal_init_journal_head_cache();
    if (ret == 0)
        ret = journal_init_handle_cache();
    return ret;
}

static void journal_destroy_caches(void)
{
    journal_destroy_revoke_caches();
    journal_destroy_journal_head_cache();
    journal_destroy_handle_cache();
}

static int __init journal_init(void)
{
    int ret;

    J_ASSERT(sizeof(struct journal_superblock_s) == 1024);

    ret = journal_init_caches();
    if (ret != 0)
        journal_destroy_caches();
    jbd_create_debugfs_entry();
    return ret;
}

static void __exit journal_exit(void)
{
#ifdef CONFIG_JBD_DEBUG
    int n = atomic_read(&nr_journal_heads);
    if (n)
        printk(KERN_EMERG "JBD: leaked %d journal_heads!\n", n);
#endif
    jbd_remove_debugfs_entry();
    journal_destroy_caches();
}

MODULE_LICENSE("GPL");
module_init(journal_init);
module_exit(journal_exit);
