/*
 * COPYRIGHT:        See COPYRIGHT.TXT
 * PROJECT:          Ext2 File System Driver for WinNT/2K/XP
 * FILE:             recover.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ext2fs.h>
#include <linux/jbd.h>
#include <linux/ext3_fs.h>

/* GLOBALS ***************************************************************/

extern PEXT2_GLOBAL Ext2Global;

/* DEFINITIONS *************************************************************/

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, Ext2LoadInternalJournal)
#pragma alloc_text(PAGE, Ext2CheckJournal)
#pragma alloc_text(PAGE, Ext2RecoverJournal)
#endif

PEXT2_MCB
Ext2LoadInternalJournal(
    PEXT2_VCB         Vcb,
    ULONG             jNo
    )
{
    PEXT2_MCB   Jcb = NULL;

    Jcb = Ext2AllocateMcb(Vcb, NULL, NULL, 0);
    if (!Jcb) {
        goto errorout;
    }

    Jcb->iNo = jNo;

    if (!Ext2LoadInode(Vcb, Jcb->iNo, Jcb->Inode)) {
        DbgBreak();
        Ext2FreeMcb(Vcb, Jcb);
        goto errorout;
    }

    Jcb->FileSize.LowPart = Jcb->Inode->i_size;
    Jcb->FileSize.HighPart = Jcb->Inode->i_size_high;

errorout:

    return Jcb;
}

INT
Ext2CheckJournal(
    PEXT2_VCB          Vcb,
    PULONG             jNo
    )
{
    struct ext3_super_block* esb = NULL;

    /* check ext3 super block */
    esb = (struct ext3_super_block *)Vcb->SuperBlock;
    if (IsFlagOn(esb->s_feature_incompat,
                 EXT3_FEATURE_INCOMPAT_RECOVER)) {
        SetLongFlag(Vcb->Flags, VCB_JOURNAL_RECOVER);
    }

    /* must stop here if volume is read-only */
    if (IsFlagOn(Vcb->Flags, VCB_READ_ONLY)) {
        goto errorout;
    }

    /* journal is external ? */
    if (esb->s_journal_inum == 0) {
        goto errorout;
    }

    /* oops: volume is corrupted */
    if (esb->s_journal_dev) {
        goto errorout;
    }

    /* return the journal inode number */
    *jNo = esb->s_journal_inum;

    return TRUE;

errorout:

    return FALSE;
}

INT
Ext2RecoverJournal(
    PEXT2_IRP_CONTEXT  IrpContext,
    PEXT2_VCB          Vcb
    )
{
    INT rc = 0;
    ULONG                   jNo = 0;
    PEXT2_MCB               jcb = NULL;
    struct block_device *   bd = NULL;
    struct inode *          ji = NULL;
    struct super_block *    sb = NULL;
    journal_t *             journal = NULL;
    struct ext3_super_block *esb;

    /* check journal inode number */
    if (!Ext2CheckJournal(Vcb, &jNo)) {
        return -1;
    }

    /* allocate  block device */
    bd = kzalloc(sizeof(struct block_device), GFP_KERNEL);
    if (!bd) {
        rc = -4;
        goto errorout;
    }

    /* set block device */
    bd->bd_dev = Vcb->RealDevice;
    bd->bd_geo = Vcb->DiskGeometry;
    bd->bd_part = Vcb->PartitionInformation;
    bd->bd_volume = Vcb->Volume;
    bd->bd_priv = (void *) Vcb;
    INIT_LIST_HEAD(&bd->bd_bh_list);
    spin_lock_init(&bd->bd_bh_lock);
    bd->bd_bh_cache = kmem_cache_create("bd_bh_buffer",
                           Vcb->BlockSize, 0, 0, NULL); 
    if (!bd->bd_bh_cache) {
        goto errorout;
    }

    /* allocate fs super block */
    sb = kzalloc(sizeof(struct super_block), GFP_KERNEL);
    if (!sb) {
        rc = -5;
        goto errorout;
    }
    sb->s_bdev = bd;
    sb->s_blocksize = BLOCK_SIZE;
    sb->s_blocksize_bits = BLOCK_BITS;
    sb->s_priv = (void *) Vcb;

    /* allocate journal Mcb */
    jcb =  Ext2LoadInternalJournal(Vcb, jNo);
    if (!jcb) {
        rc = -6;
        goto errorout;
    }

    /* allocate journal inode */
    ji = kzalloc(sizeof(struct inode), GFP_KERNEL);
    if (!ji) {
        rc = -7;
        goto errorout;
    }

    /* initialize vfs inode */
    ji->i_count.counter = 2;
    ji->i_ino  = jcb->iNo;
    ji->i_mode = jcb->Inode->i_mode;
    ji->i_size = jcb->FileSize.QuadPart;
    ji->i_sb   = sb;
    ji->i_priv = (void *) jcb;

    /* initialize journal file from inode */
    journal = journal_init_inode(ji);

    /* initialzation succeeds ? */
    if (!journal) {
        iput(ji);
        rc = -8;
        goto errorout;
    }

    /* start journal recovery */
    rc = journal_load(journal);
    if (0 != rc) {
        rc = -9;
        DbgPrint("Ext2Fsd: recover_journal: failed "
                 "to recover journal data.\n");
    }

    /* reload super_block and group_description */
    Ext2RefreshSuper(IrpContext, Vcb);
    Ext2RefreshGroup(IrpContext, Vcb);

    /* wipe journal data and clear recover flag in sb */
    if (rc == 0) {
        journal_wipe_recovery(journal);
        ClearLongFlag(
            Vcb->SuperBlock->s_feature_incompat, 
            EXT3_FEATURE_INCOMPAT_RECOVER );
        Ext2SaveSuper(IrpContext, Vcb);
        sync_blockdev(bd);
        ClearLongFlag(Vcb->Flags, VCB_JOURNAL_RECOVER);
    }

errorout:

    /* destroy journal structure */
    if (journal) {
        journal_destroy(journal);
    }

    /* destory vfs inode */
    if (ji) {
        ASSERT(1 == atomic_read(&ji->i_count));
        iput(ji);
    }

    /* destory journal Mcb */
    if (jcb) {
        Ext2FreeMcb(Vcb, jcb);
    }

    /* destory vfs super_block */
    if (sb) {
        kfree(sb);
    }

    /* destory block device object */
    if (bd) {
        if (bd->bd_bh_cache) {
            kmem_cache_destroy(bd->bd_bh_cache);
        }
        kfree(bd);
    }

    return rc;
}
