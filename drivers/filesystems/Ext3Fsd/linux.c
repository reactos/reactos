/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             linux.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ext2fs.h>
#include <linux/jbd.h>
#include <linux/errno.h>

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, kzalloc)
#endif

struct task_struct current_task = {
    /* pid  */ 0,
    /* tid  */ 1,
    /* comm */ "current\0",
    /* journal_info */ NULL
};
struct task_struct *current = &current_task;

void *kzalloc(int size, int flags)
{
    void *buffer = kmalloc(size, flags);
    if (buffer) {
        memset(buffer, 0, size);
    }
    return buffer;
}

//
// slab routines
//

kmem_cache_t *
kmem_cache_create(
    const char *    name,
    size_t          size,
    size_t          offset,
    unsigned long   flags,
    kmem_cache_cb_t ctor
    )
{
    kmem_cache_t *kc = NULL;

    kc = kmalloc(sizeof(kmem_cache_t), GFP_KERNEL);
    if (kc == NULL) {
        goto errorout;
    }

    memset(kc, 0, sizeof(kmem_cache_t));
    ExInitializeNPagedLookasideList(
            &kc->la,
            NULL,
            NULL,
            0,
            size,
            'JBKC',
            0);

    kc->size = size;
    strncpy(kc->name, name, 31);
    kc->constructor = ctor;

errorout:

    return kc;
}

int kmem_cache_destroy(kmem_cache_t * kc)
{
    ASSERT(kc != NULL);

    ExDeleteNPagedLookasideList(&(kc->la));
    kfree(kc);

    return 0;
}

void* kmem_cache_alloc(kmem_cache_t *kc, int flags)
{
    PVOID  ptr = NULL;
    ptr = ExAllocateFromNPagedLookasideList(&(kc->la));
    if (ptr) {
        atomic_inc(&kc->count);
        atomic_inc(&kc->acount);
    }
    return ptr;
}

void kmem_cache_free(kmem_cache_t *kc, void *p)
{
    if (p) {
        atomic_dec(&kc->count);
        ExFreeToNPagedLookasideList(&(kc->la), p);
    }
}

//
// wait queue routines
//

void init_waitqueue_head(wait_queue_head_t *q)
{
        spin_lock_init(&q->lock);
        INIT_LIST_HEAD(&q->task_list);
}

struct __wait_queue *
wait_queue_create()
{
    struct __wait_queue * wait = NULL;
    wait = kmalloc(sizeof(struct __wait_queue), GFP_KERNEL);
    if (!wait) {
        return NULL;
    }

    memset(wait, 0, sizeof(struct __wait_queue));
    wait->flags = WQ_FLAG_AUTO_REMOVAL;
    wait->private = (void *)KeGetCurrentThread();
    INIT_LIST_HEAD(&wait->task_list);
    KeInitializeEvent(&(wait->event),
                      SynchronizationEvent,
                      FALSE);

    return wait;
}

void
wait_queue_destroy(struct __wait_queue * wait)
{
    kfree(wait);
}

static inline void __add_wait_queue(wait_queue_head_t *head, struct __wait_queue *new)
{
	list_add(&new->task_list, &head->task_list);
}

/*
 * Used for wake-one threads:
 */
static inline void __add_wait_queue_tail(wait_queue_head_t *head,
						    struct __wait_queue *new)
{
	list_add_tail(&new->task_list, &head->task_list);
}

static inline void __remove_wait_queue(wait_queue_head_t *head,
							struct __wait_queue *old)
{
	list_del(&old->task_list);
}

void add_wait_queue(wait_queue_head_t *q, wait_queue_t *waiti)
{
	unsigned long flags;
    struct __wait_queue *wait = *waiti;

	wait->flags &= ~WQ_FLAG_EXCLUSIVE;
	spin_lock_irqsave(&q->lock, flags);
	__add_wait_queue(q, wait);
	spin_unlock_irqrestore(&q->lock, flags);
}

void add_wait_queue_exclusive(wait_queue_head_t *q, wait_queue_t *waiti)
{
	unsigned long flags;
    struct __wait_queue *wait = *waiti;

	wait->flags |= WQ_FLAG_EXCLUSIVE;
	spin_lock_irqsave(&q->lock, flags);
	__add_wait_queue_tail(q, wait);
	spin_unlock_irqrestore(&q->lock, flags);
}

void remove_wait_queue(wait_queue_head_t *q, wait_queue_t *waiti)
{
	unsigned long flags;
    struct __wait_queue *wait = *waiti;

	spin_lock_irqsave(&q->lock, flags);
	__remove_wait_queue(q, wait);
	spin_unlock_irqrestore(&q->lock, flags);
}

/*
 * Note: we use "set_current_state()" _after_ the wait-queue add,
 * because we need a memory barrier there on SMP, so that any
 * wake-function that tests for the wait-queue being active
 * will be guaranteed to see waitqueue addition _or_ subsequent
 * tests in this thread will see the wakeup having taken place.
 *
 * The spin_unlock() itself is semi-permeable and only protects
 * one way (it only protects stuff inside the critical region and
 * stops them from bleeding out - it would still allow subsequent
 * loads to move into the critical region).
 */
void
prepare_to_wait(wait_queue_head_t *q, wait_queue_t *waiti, int state)
{
	unsigned long flags;
    struct __wait_queue *wait = *waiti;

	wait->flags &= ~WQ_FLAG_EXCLUSIVE;
	spin_lock_irqsave(&q->lock, flags);
	if (list_empty(&wait->task_list))
		__add_wait_queue(q, wait);
	/*
	 * don't alter the task state if this is just going to
	 * queue an async wait queue callback
	 */
	if (is_sync_wait(wait))
		set_current_state(state);
	spin_unlock_irqrestore(&q->lock, flags);
}

void
prepare_to_wait_exclusive(wait_queue_head_t *q, wait_queue_t *waiti, int state)
{
	unsigned long flags;
    struct __wait_queue *wait = *waiti;

	wait->flags |= WQ_FLAG_EXCLUSIVE;
	spin_lock_irqsave(&q->lock, flags);
	if (list_empty(&wait->task_list))
		__add_wait_queue_tail(q, wait);
	/*
	 * don't alter the task state if this is just going to
 	 * queue an async wait queue callback
	 */
	if (is_sync_wait(wait))
		set_current_state(state);
	spin_unlock_irqrestore(&q->lock, flags);
}
EXPORT_SYMBOL(prepare_to_wait_exclusive);

void finish_wait(wait_queue_head_t *q, wait_queue_t *waiti)
{
	unsigned long flags;
    struct __wait_queue *wait = *waiti;

	__set_current_state(TASK_RUNNING);
	/*
	 * We can check for list emptiness outside the lock
	 * IFF:
	 *  - we use the "careful" check that verifies both
	 *    the next and prev pointers, so that there cannot
	 *    be any half-pending updates in progress on other
	 *    CPU's that we haven't seen yet (and that might
	 *    still change the stack area.
	 * and
	 *  - all other users take the lock (ie we can only
	 *    have _one_ other CPU that looks at or modifies
	 *    the list).
	 */
	if (!list_empty_careful(&wait->task_list)) {
		spin_lock_irqsave(&q->lock, flags);
		list_del_init(&wait->task_list);
		spin_unlock_irqrestore(&q->lock, flags);
	}

    /* free wait */
    wait_queue_destroy(wait);
}

int wake_up(wait_queue_head_t *queue)
{
    return 0; /* KeSetEvent(&wait->event, 0, FALSE); */
}


//
// kernel timer routines
//

//
// buffer head routines
//

struct _EXT2_BUFFER_HEAD {
    kmem_cache_t *  bh_cache;
    atomic_t        bh_count;
    atomic_t        bh_acount;
} g_jbh = {NULL, ATOMIC_INIT(0)};

int
ext2_init_bh()
{
    g_jbh.bh_count.counter = 0;
    g_jbh.bh_acount.counter = 0;
	g_jbh.bh_cache = kmem_cache_create(
                        "ext2_bh",   /* bh */
				        sizeof(struct buffer_head),
				        0,		        /* offset */
				        SLAB_TEMPORARY,	/* flags */
				        NULL);		    /* ctor */  
    if (g_jbh.bh_cache == NULL) {
        printk(KERN_EMERG "JBD: failed to create handle cache\n");
        return -ENOMEM;
    }
    return 0;
}

void
ext2_destroy_bh()
{
    if (g_jbh.bh_cache) {
        kmem_cache_destroy(g_jbh.bh_cache);
        g_jbh.bh_cache = NULL;
    }
}

struct buffer_head *
new_buffer_head()
{
    struct buffer_head * bh = NULL;
    bh = kmem_cache_alloc(g_jbh.bh_cache, GFP_NOFS);
    if (bh) {
        memset(bh, 0, sizeof(struct buffer_head));
    }
    return bh;
}

void
free_buffer_head(struct buffer_head * bh)
{
    if (bh) {
        kmem_cache_free(g_jbh.bh_cache, bh);
    }
}

struct buffer_head *
__getblk(
    struct block_device *   bdev,
    sector_t                block,
	unsigned long           size
    )
{
    PEXT2_VCB Vcb = bdev->bd_priv;
    LARGE_INTEGER offset;
    IO_STATUS_BLOCK iosb;

    KIRQL irql = 0;
    struct list_head *entry;

    /* allocate buffer_head and initialize it */
    struct buffer_head * bh = NULL;

    /* search the bdev bh list */
    spin_lock_irqsave(&bdev->bd_bh_lock, irql);
    list_for_each(entry, &bdev->bd_bh_list) {
        struct buffer_head * tbh = 
            list_entry(entry, struct buffer_head, b_list);
        if (block == tbh->b_blocknr) {
            bh = tbh;
            break;
        }

        if (bh) {
            get_bh(bh);
            spin_unlock_irqrestore(&bdev->bd_bh_lock, irql);
            return bh;
        }
    }
    spin_unlock_irqrestore(&bdev->bd_bh_lock, irql);

    bh = new_buffer_head();
    if (!bh) {
        goto errorout;
    }
    bh->b_bdev = bdev;
    bh->b_blocknr = block;
    bh->b_size = size;
    bh->b_data = kzalloc(size, GFP_KERNEL);
    if (!bh->b_data) {
        free_buffer_head(bh);
        bh = NULL;
        goto errorout;
    }
    atomic_inc(&g_jbh.bh_count);
    atomic_inc(&g_jbh.bh_acount);
        
again:

    offset.QuadPart = (s64) bh->b_blocknr;
    offset.QuadPart *= BLOCK_SIZE;
    if (!CcCopyRead( Vcb->Volume,
                     &offset,
                     bh->b_size,
                     TRUE,
                     bh->b_data,
                     &iosb
                     )) {
        goto again;
    }

    set_buffer_uptodate(bh);
    get_bh(bh);

    spin_lock_irqsave(&bdev->bd_bh_lock, irql);
    list_add(&bh->b_list, &bdev->bd_bh_list);
    spin_unlock_irqrestore(&bdev->bd_bh_lock, irql);

    DEBUG(DL_JNL, (" new_bh: cnt=%u size=%u blk=%10.10xh bh=%p ptr=%p\n",
                    atomic_read(&g_jbh.bh_count), bh->b_size,
                    bh->b_blocknr, bh, bh->b_data ));

    /* we get it */
errorout:

    return bh;
}

void __brelse(struct buffer_head *bh)
{
    struct block_device *bdev = bh->b_bdev;
    PEXT2_VCB Vcb = (PEXT2_VCB)bdev->bd_priv;
    KIRQL   irql = 0;

    ASSERT(Vcb->Identifier.Type == EXT2VCB);

    /* write data in case it's dirty */
    while (buffer_dirty(bh)) {
        ll_rw_block(WRITE, 1, &bh);
    }

    spin_lock_irqsave(&bdev->bd_bh_lock, irql);
    if (!atomic_dec_and_test(&bh->b_count)) {
       spin_unlock_irqrestore(&bdev->bd_bh_lock, irql);
        return;
    }
    list_del(&bh->b_list);
    spin_unlock_irqrestore(&bdev->bd_bh_lock, irql);

    DEBUG(DL_JNL, ("free_bh: cnt=%u size=%u blk=%10.10xh bh=%p ptr=%p\n",
                    atomic_read(&g_jbh.bh_count) - 1, bh->b_size,
                    bh->b_blocknr, bh, bh->b_data ));

    kfree(bh->b_data);
    free_buffer_head(bh);
    atomic_dec(&g_jbh.bh_count);
}

void __bforget(struct buffer_head *bh)
{
    clear_buffer_dirty(bh);
    __brelse(bh);
}

void __lock_buffer(struct buffer_head *bh)
{
}

void unlock_buffer(struct buffer_head *bh)
{
    clear_buffer_locked(bh);
}

void __wait_on_buffer(struct buffer_head *bh)
{
}

int submit_bh(int rw, struct buffer_head *bh)
{
    struct block_device *bdev = bh->b_bdev;
    PEXT2_VCB           Vcb = bdev->bd_priv;
    LARGE_INTEGER       offset;
    PVOID               data = NULL;
    PBCB                bcb = NULL;

    ASSERT(Vcb->Identifier.Type == EXT2VCB);
    ASSERT(bh->b_data);

    if (rw == WRITE) {

        offset.QuadPart = (s64) bh->b_blocknr;
        offset.QuadPart *= BLOCK_SIZE;
again:
        if (!CcPinRead( Vcb->Volume,
                        &offset,
                        bh->b_size,
                        PIN_WAIT,
                        &bcb,
                        &data )) {
            goto again;
        }

        RtlCopyMemory(data, bh->b_data, bh->b_size);
        CcSetDirtyPinnedData(bcb, NULL);
        Ext2AddBlockExtent( Vcb, NULL,
                            bh->b_blocknr,
                            bh->b_blocknr,
                            (bh->b_size >> BLOCK_BITS));

        CcUnpinData(bcb);

    } else{

        DbgBreak();
    }

    put_bh(bh);
    return 0;
}

void ll_rw_block(int rw, int nr, struct buffer_head * bhs[])
{
    int i;

    for (i = 0; i < nr; i++) {

        struct buffer_head *bh = bhs[i];

        if (rw == SWRITE)
            lock_buffer(bh);
        else if (test_set_buffer_locked(bh))
            continue;

        if (rw == WRITE || rw == SWRITE) {
            if (test_clear_buffer_dirty(bh)) {
                    get_bh(bh);
                    submit_bh(WRITE, bh);
                    continue;
                        }
        } else {
            if (!buffer_uptodate(bh)) {
                    get_bh(bh);
                    submit_bh(rw, bh);
                    continue;
            }
        }
        unlock_buffer(bh);
    }
}

int sync_dirty_buffer(struct buffer_head *bh)
{
    int ret = 0;

    ASSERT(atomic_read(&bh->b_count) <= 1);
    lock_buffer(bh);
    if (test_clear_buffer_dirty(bh)) {
        get_bh(bh);
        ret = submit_bh(WRITE, bh);
        wait_on_buffer(bh);
    } else {
        unlock_buffer(bh);
    }
    return ret;
}

void mark_buffer_dirty(struct buffer_head *bh)
{
    set_buffer_dirty(bh);
}

int sync_blockdev(struct block_device *bdev)
{
    PEXT2_VCB Vcb = (PEXT2_VCB) bdev->bd_priv;

    if (0 == atomic_read(&g_jbh.bh_count)) {
        Ext2FlushVolume(NULL, Vcb, FALSE);
    }
    return 0;
}

/*
 * Perform a pagecache lookup for the matching buffer.  If it's there, refre
 * it in the LRU and mark it as accessed.  If it is not present then return
 * NULL
 */
struct buffer_head *
__find_get_block(struct block_device *bdev, sector_t block, unsigned long size)
{
    return __getblk(bdev, block, size);
}

//
// inode block mapping
//

unsigned long bmap(struct inode *i, unsigned long b)
{
    unsigned long lcn = 0;
    struct super_block *s = i->i_sb;

    PEXT2_MCB  Mcb = (PEXT2_MCB)i->i_priv;
    PEXT2_VCB  Vcb = (PEXT2_VCB)s->s_priv;
    PEXT2_EXTENT extent = NULL;
    ULONGLONG  offset = (ULONGLONG)b;
    NTSTATUS   status;

    if (!Mcb || !Vcb) {
        goto errorout;
    }

    offset <<= BLOCK_BITS;
    status = Ext2BuildExtents(
                    NULL,
                    Vcb,
                    Mcb,
                    offset,
                    BLOCK_SIZE,
                    FALSE,
                    &extent
                );

    if (!NT_SUCCESS(status)) {
        goto errorout;
    }

    if (extent == NULL) {
        goto errorout;
    }

    lcn = (unsigned long)(extent->Lba >> BLOCK_BITS);

errorout:

    if (extent) {
        Ext2FreeExtent(extent);
    }

    return lcn;
}

void iget(struct inode *inode)
{
    atomic_inc(&inode->i_count);
}

void iput(struct inode *inode)
{
    if (atomic_dec_and_test(&inode->i_count)) {
        kfree(inode);
    }
}

//
// initialzer and destructor
//

int
ext2_init_linux()
{
    int rc = 0;

    rc = ext2_init_bh();
    if (rc != 0) {
        goto errorout;
    }

errorout:

    return rc;
}

void
ext2_destroy_linux()
{
    ext2_destroy_bh();
}
