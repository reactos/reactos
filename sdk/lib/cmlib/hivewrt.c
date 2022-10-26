/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Configuration Manager Library - Registry Syncing & Hive/Log Writing
 * COPYRIGHT:   Copyright 2001 - 2005 Eric Kohl
 *              Copyright 2005 Filip Navara <navaraf@reactos.org>
 *              Copyright 2021 Max Korostil
 *              Copyright 2022 George Bișoc <george.bisoc@reactos.org>
 */

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

/* DECLARATIONS *************************************************************/

#if !defined(CMLIB_HOST) && !defined(_BLDR_)
BOOLEAN
NTAPI
IoSetThreadHardErrorMode(
    _In_ BOOLEAN HardErrorEnabled);
#endif

/* GLOBALS *****************************************************************/

#if !defined(CMLIB_HOST) && !defined(_BLDR_)
extern BOOLEAN CmpMiniNTBoot;
#endif

/* PRIVATE FUNCTIONS ********************************************************/

/**
 * @brief
 * Validates the base block header of a primary
 * hive for consistency.
 *
 * @param[in] RegistryHive
 * A pointer to a hive descriptor to look
 * for the header block.
 */
static
VOID
HvpValidateBaseHeader(
    _In_ PHHIVE RegistryHive)
{
    PHBASE_BLOCK BaseBlock;

    /*
     * Cache the base block and validate it.
     * Especially...
     *
     * 1. It must must have a valid signature.
     * 2. It must have a valid format.
     * 3. It must be of an adequate major version,
     *    not anything else.
     */
    BaseBlock = RegistryHive->BaseBlock;
    ASSERT(BaseBlock->Signature == HV_HBLOCK_SIGNATURE);
    ASSERT(BaseBlock->Format == HBASE_FORMAT_MEMORY);
    ASSERT(BaseBlock->Major == HSYS_MAJOR);
}

/**
 * @unimplemented
 * @brief
 * Writes dirty data in a transacted way to a hive
 * log file during hive syncing operation. Log
 * files are used by the kernel/bootloader to
 * perform recovery operations against a
 * damaged primary hive.
 *
 * @param[in] RegistryHive
 * A pointer to a hive descriptor where the log
 * belongs to and of which we write data into the
 * said log.
 *
 * @return
 * Returns TRUE if log transaction writing has succeeded,
 * FALSE otherwise.
 *
 * @remarks
 * The function is not completely implemented, that is,
 * it lacks the implementation for growing the log file size.
 * See the FIXME comment below for further details.
 */
static
BOOLEAN
CMAPI
HvpWriteLog(
    _In_ PHHIVE RegistryHive)
{
    BOOLEAN Success;
    ULONG FileOffset;
    ULONG BlockIndex;
    ULONG LastIndex;
    PVOID Block;
    UINT32 BitmapSize, BufferSize;
    PUCHAR HeaderBuffer, Ptr;

    /*
     * The hive log we are going to write data into
     * has to be writable and with a sane storage.
     */
    ASSERT(RegistryHive->ReadOnly == FALSE);
    ASSERT(RegistryHive->BaseBlock->Length ==
           RegistryHive->Storage[Stable].Length * HBLOCK_SIZE);

    /* Validate the base header before we go further */
    HvpValidateBaseHeader(RegistryHive);

    /*
     * The sequences can diverge in an occurrence of forced
     * shutdown of the system such as during a power failure,
     * the hardware crapping itself or during a system crash
     * when one of the sequences have been modified during
     * writing into the log or hive. In such cases the hive
     * needs a repair.
     */
    if (RegistryHive->BaseBlock->Sequence1 !=
        RegistryHive->BaseBlock->Sequence2)
    {
        DPRINT1("The sequences DO NOT MATCH (Sequence1 == 0x%x, Sequence2 == 0x%x)\n",
                RegistryHive->BaseBlock->Sequence1, RegistryHive->BaseBlock->Sequence2);
        return FALSE;
    }

    /*
     * FIXME: We must set a new file size for this log
     * here but ReactOS lacks the necessary code implementation
     * that manages the growing and shrinking of a hive's log
     * size. So for now don't set any new size for the log.
     */

    /*
     * Now calculate the bitmap and buffer sizes to hold up our
     * contents in a buffer.
     */
    BitmapSize = ROUND_UP(sizeof(ULONG) + RegistryHive->DirtyVector.SizeOfBitMap / 8, HSECTOR_SIZE);
    BufferSize = HV_LOG_HEADER_SIZE + BitmapSize;

    /* Now allocate the base header block buffer */
    HeaderBuffer = RegistryHive->Allocate(BufferSize, TRUE, TAG_CM);
    if (!HeaderBuffer)
    {
        DPRINT1("Couldn't allocate buffer for base header block\n");
        return FALSE;
    }

    /* Great, now zero out the buffer */
    RtlZeroMemory(HeaderBuffer, BufferSize);

    /*
     * Update the base block of this hive and
     * increment the primary sequence number
     * as we are at the half of the work.
     */
    RegistryHive->BaseBlock->Type = HFILE_TYPE_LOG;
    RegistryHive->BaseBlock->Sequence1++;
    RegistryHive->BaseBlock->CheckSum = HvpHiveHeaderChecksum(RegistryHive->BaseBlock);

    /* Copy the base block header */
    RtlCopyMemory(HeaderBuffer, RegistryHive->BaseBlock, HV_LOG_HEADER_SIZE);
    Ptr = HeaderBuffer + HV_LOG_HEADER_SIZE;

    /* Copy the dirty vector */
    *((PULONG)Ptr) = HV_LOG_DIRTY_SIGNATURE;
    Ptr += sizeof(HV_LOG_DIRTY_SIGNATURE);

    /*
     * FIXME: In ReactOS a vector contains one bit per block
     * whereas in Windows each bit within a vector is per
     * sector. Furthermore, the dirty blocks within a respective
     * hive has to be marked as such in an appropriate function
     * for this purpose (probably HvMarkDirty or similar).
     *
     * For the moment being, mark the relevant dirty blocks
     * here.
     */
    BlockIndex = 0;
    while (BlockIndex < RegistryHive->Storage[Stable].Length)
    {
        /* Check if the block is clean or we're past the last block */
        LastIndex = BlockIndex;
        BlockIndex = RtlFindSetBits(&RegistryHive->DirtyVector, 1, BlockIndex);
        if (BlockIndex == ~HV_CLEAN_BLOCK || BlockIndex < LastIndex)
        {
            break;
        }

        /*
         * Mark this block as dirty and go to the next one.
         *
         * FIXME: We should rather use RtlSetBits but that crashes
         * the system with a bugckeck. So for now mark blocks manually
         * by hand.
         */
        Ptr[BlockIndex] = HV_LOG_DIRTY_BLOCK;
        BlockIndex++;
    }

    /* Now write the hive header and block bitmap into the log */
    FileOffset = 0;
    Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_LOG,
                                      &FileOffset, HeaderBuffer, BufferSize);
    RegistryHive->Free(HeaderBuffer, 0);
    if (!Success)
    {
        DPRINT1("Failed to write the hive header block to log (primary sequence)\n");
        return FALSE;
    }

    /* Now write the actual dirty data to log */
    FileOffset = BufferSize;
    BlockIndex = 0;
    while (BlockIndex < RegistryHive->Storage[Stable].Length)
    {
        /* Check if the block is clean or we're past the last block */
        LastIndex = BlockIndex;
        BlockIndex = RtlFindSetBits(&RegistryHive->DirtyVector, 1, BlockIndex);
        if (BlockIndex == ~HV_CLEAN_BLOCK || BlockIndex < LastIndex)
        {
            break;
        }

        /* Get the block */
        Block = (PVOID)RegistryHive->Storage[Stable].BlockList[BlockIndex].BlockAddress;

        /* Write it to log */
        Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_LOG,
                                          &FileOffset, Block, HBLOCK_SIZE);
        if (!Success)
        {
            DPRINT1("Failed to write dirty block to log (block 0x%p, block index 0x%x)\n", Block, BlockIndex);
            return FALSE;
        }

        /* Grow up the file offset as we go to the next block */
        BlockIndex++;
        FileOffset += HBLOCK_SIZE;
    }

    /*
     * We wrote the header and body of log with dirty,
     * data do a flush immediately.
     */
    Success = RegistryHive->FileFlush(RegistryHive, HFILE_TYPE_LOG, NULL, 0);
    if (!Success)
    {
        DPRINT1("Failed to flush the log\n");
        return FALSE;
    }

    /*
     * OK, we're now at 80% of the work done.
     * Increment the secondary sequence and flush
     * the log again. We can have a fully successful
     * transacted write of a log if the sequences
     * are synced up properly.
     */
    RegistryHive->BaseBlock->Sequence2++;
    RegistryHive->BaseBlock->CheckSum = HvpHiveHeaderChecksum(RegistryHive->BaseBlock);

    /* Write new stuff into log first */
    FileOffset = 0;
    Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_LOG,
                                      &FileOffset, RegistryHive->BaseBlock,
                                      HV_LOG_HEADER_SIZE);
    if (!Success)
    {
        DPRINT1("Failed to write the log file (secondary sequence)\n");
        return FALSE;
    }

    /* Flush it finally */
    Success = RegistryHive->FileFlush(RegistryHive, HFILE_TYPE_LOG, NULL, 0);
    if (!Success)
    {
        DPRINT1("Failed to flush the log\n");
        return FALSE;
    }

    return TRUE;
}

/**
 * @brief
 * Writes data (dirty or non) to a primary hive during
 * syncing operation. Hive writing is also performed
 * during a flush occurrence on request by the system.
 *
 * @param[in] RegistryHive
 * A pointer to a hive descriptor where the data is
 * to be written to that hive.
 *
 * @param[in] OnlyDirty
 * If set to TRUE, the function only looks for dirty
 * data to be written to the primary hive, otherwise if
 * it's set to FALSE then the function writes all the data.
 *
 * @return
 * Returns TRUE if writing to hive has succeeded,
 * FALSE otherwise.
 */
static
BOOLEAN
CMAPI
HvpWriteHive(
    _In_ PHHIVE RegistryHive,
    _In_ BOOLEAN OnlyDirty)
{
    BOOLEAN Success;
    ULONG FileOffset;
    ULONG BlockIndex;
    ULONG LastIndex;
    PVOID Block;

    ASSERT(RegistryHive->ReadOnly == FALSE);
    ASSERT(RegistryHive->BaseBlock->Length ==
           RegistryHive->Storage[Stable].Length * HBLOCK_SIZE);
    ASSERT(RegistryHive->BaseBlock->RootCell != HCELL_NIL);

    /* Validate the base header before we go further */
    HvpValidateBaseHeader(RegistryHive);

    /*
     * The sequences can diverge in an occurrence of forced
     * shutdown of the system such as during a power failure,
     * the hardware crapping itself or during a system crash
     * when one of the sequences have been modified during
     * writing into the log or hive. In such cases the hive
     * needs a repair.
     */
    if (RegistryHive->BaseBlock->Sequence1 !=
        RegistryHive->BaseBlock->Sequence2)
    {
        DPRINT1("The sequences DO NOT MATCH (Sequence1 == 0x%x, Sequence2 == 0x%x)\n",
                RegistryHive->BaseBlock->Sequence1, RegistryHive->BaseBlock->Sequence2);
        return FALSE;
    }

    /*
     * Update the primary sequence number and write
     * the base block to hive.
     */
    RegistryHive->BaseBlock->Type = HFILE_TYPE_PRIMARY;
    RegistryHive->BaseBlock->Sequence1++;
    RegistryHive->BaseBlock->CheckSum = HvpHiveHeaderChecksum(RegistryHive->BaseBlock);

    /* Write hive block */
    FileOffset = 0;
    Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_PRIMARY,
                                      &FileOffset, RegistryHive->BaseBlock,
                                      sizeof(HBASE_BLOCK));
    if (!Success)
    {
        DPRINT1("Failed to write the base block header to primary hive (primary sequence)\n");
        return FALSE;
    }

    /* Write the whole primary hive, block by block */
    BlockIndex = 0;
    while (BlockIndex < RegistryHive->Storage[Stable].Length)
    {
        /*
         * If we have to syncrhonize the registry hive we
         * want to look for dirty blocks to reflect the new
         * updates done to the hive. Otherwise just write
         * all the blocks as if we were doing a regular
         * writing of the hive.
         */
        if (OnlyDirty)
        {
            /* Check if the block is clean or we're past the last block */
            LastIndex = BlockIndex;
            BlockIndex = RtlFindSetBits(&RegistryHive->DirtyVector, 1, BlockIndex);
            if (BlockIndex == ~HV_CLEAN_BLOCK || BlockIndex < LastIndex)
            {
                break;
            }
        }

        /* Get the block and offset position */
        Block = (PVOID)RegistryHive->Storage[Stable].BlockList[BlockIndex].BlockAddress;
        FileOffset = (BlockIndex + 1) * HBLOCK_SIZE;

        /* Now write this block to primary hive file */
        Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_PRIMARY,
                                          &FileOffset, Block, HBLOCK_SIZE);
        if (!Success)
        {
            DPRINT1("Failed to write hive block to primary hive file (block 0x%p, block index 0x%x)\n",
                    Block, BlockIndex);
            return FALSE;
        }

        /* Go to the next block */
        BlockIndex++;
    }

    /*
     * We wrote all the hive contents to the file, we
     * must flush the changes to disk now.
     */
    Success = RegistryHive->FileFlush(RegistryHive, HFILE_TYPE_PRIMARY, NULL, 0);
    if (!Success)
    {
        DPRINT1("Failed to flush the primary hive\n");
        return FALSE;
    }

    /*
     * Increment the secondary sequence number and
     * update the checksum. A successful transaction
     * writing of hive is both of sequences are the
     * same indicating the writing operation didn't
     * fail.
     */
    RegistryHive->BaseBlock->Sequence2++;
    RegistryHive->BaseBlock->CheckSum = HvpHiveHeaderChecksum(RegistryHive->BaseBlock);

    /* Write hive block */
    FileOffset = 0;
    Success = RegistryHive->FileWrite(RegistryHive, HFILE_TYPE_PRIMARY,
                                      &FileOffset, RegistryHive->BaseBlock,
                                      sizeof(HBASE_BLOCK));
    if (!Success)
    {
        DPRINT1("Failed to write the base block header to primary hive (secondary sequence)\n");
        return FALSE;
    }

    /* Flush the hive immediately */
    Success = RegistryHive->FileFlush(RegistryHive, HFILE_TYPE_PRIMARY, NULL, 0);
    if (!Success)
    {
        DPRINT1("Failed to flush the primary hive\n");
        return FALSE;
    }

    return TRUE;
}

/* PUBLIC FUNCTIONS ***********************************************************/

/**
 * @brief
 * Synchronizes a registry hive with latest updates
 * from dirty data present in volatile memory, aka RAM.
 * It writes both to hive log and corresponding primary
 * hive. Syncing is done on request by the system during
 * a flush occurrence.
 *
 * @param[in] RegistryHive
 * A pointer to a hive descriptor where syncing is
 * to be performed.
 *
 * @return
 * Returns TRUE if syncing has succeeded, FALSE otherwise.
 */
BOOLEAN
CMAPI
HvSyncHive(
    _In_ PHHIVE RegistryHive)
{
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
    BOOLEAN HardErrors;
#endif

    ASSERT(RegistryHive->ReadOnly == FALSE);
    ASSERT(RegistryHive->Signature == HV_HHIVE_SIGNATURE);

    /*
     * Check if there's any dirty data in the vector.
     * A space with clean blocks would be pointless for
     * a log because we want to write dirty data in and
     * sync up, not clean data. So just consider our
     * job as done as there's literally nothing to do.
     */
    if (RtlFindSetBits(&RegistryHive->DirtyVector, 1, 0) == ~HV_CLEAN_BLOCK)
    {
        DPRINT("The dirty vector has clean data, nothing to do\n");
        return TRUE;
    }

    /*
     * We are either in Live CD or we are sharing hives.
     * In either of the cases, hives can only be read
     * so don't do any writing operations on them.
     */
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
    if (CmpMiniNTBoot)
    {
        DPRINT("We are sharing hives or in Live CD mode, abort syncing\n");
        return TRUE;
    }
#endif

    /* Avoid any writing operations on volatile hives */
    if (RegistryHive->HiveFlags & HIVE_VOLATILE)
    {
        DPRINT("The hive is volatile (hive 0x%p)\n", RegistryHive);
        return TRUE;
    }

#if !defined(CMLIB_HOST) && !defined(_BLDR_)
    /* Disable hard errors before syncing the hive */
    HardErrors = IoSetThreadHardErrorMode(FALSE);
#endif

#if !defined(_BLDR_)
    /* Update hive header modification time */
    KeQuerySystemTime(&RegistryHive->BaseBlock->TimeStamp);
#endif

    /* Update the log file of hive if present */
    if (RegistryHive->Log == TRUE)
    {
        if (!HvpWriteLog(RegistryHive))
        {
            DPRINT1("Failed to write a log whilst syncing the hive\n");
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
            IoSetThreadHardErrorMode(HardErrors);
#endif
            return FALSE;
        }
    }

    /* Update the primary hive file */
    if (!HvpWriteHive(RegistryHive, TRUE))
    {
        DPRINT1("Failed to write the primary hive\n");
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
        IoSetThreadHardErrorMode(HardErrors);
#endif
        return FALSE;
    }

    /* Clear dirty bitmap. */
    RtlClearAllBits(&RegistryHive->DirtyVector);
    RegistryHive->DirtyCount = 0;

#if !defined(CMLIB_HOST) && !defined(_BLDR_)
    IoSetThreadHardErrorMode(HardErrors);
#endif
    return TRUE;
}

/**
 * @unimplemented
 * @brief
 * Determines whether a registry hive needs
 * to be shrinked or not based on its overall
 * size of the hive space to avoid unnecessary
 * bloat.
 *
 * @param[in] RegistryHive
 * A pointer to a hive descriptor where hive
 * shrinking is to be determined.
 *
 * @return
 * Returns TRUE if hive shrinking needs to be
 * done, FALSE otherwise.
 */
BOOLEAN
CMAPI
HvHiveWillShrink(
    _In_ PHHIVE RegistryHive)
{
    /* No shrinking yet */
    UNIMPLEMENTED_ONCE;
    return FALSE;
}

/**
 * @brief
 * Writes data to a registry hive. Unlike
 * HvSyncHive, this function just writes
 * the wholy registry data to a primary hive,
 * ignoring if a certain data block is dirty
 * or not.
 *
 * @param[in] RegistryHive
 * A pointer to a hive descriptor where data
 * is be written into.
 *
 * @return
 * Returns TRUE if hive writing has succeeded,
 * FALSE otherwise.
 */
BOOLEAN
CMAPI
HvWriteHive(
    _In_ PHHIVE RegistryHive)
{
    ASSERT(RegistryHive->ReadOnly == FALSE);
    ASSERT(RegistryHive->Signature == HV_HHIVE_SIGNATURE);

#if !defined(_BLDR_)
    /* Update hive header modification time */
    KeQuerySystemTime(&RegistryHive->BaseBlock->TimeStamp);
#endif

    /* Update hive file */
    if (!HvpWriteHive(RegistryHive, FALSE))
    {
        DPRINT1("Failed to write the hive\n");
        return FALSE;
    }

    return TRUE;
}


/**
 * @brief
 * Synchronizes a hive with recovered
 * data during a healing/resuscitation
 * operation of the registry.
 *
 * @param[in] RegistryHive
 * A pointer to a hive descriptor where data
 * syncing is to be done.
 *
 * @return
 * Returns TRUE if hive syncing during recovery
 * succeeded, FALSE otherwise.
 */
BOOLEAN
CMAPI
HvSyncHiveFromRecover(
    _In_ PHHIVE RegistryHive)
{
    ASSERT(RegistryHive->ReadOnly == FALSE);
    ASSERT(RegistryHive->Signature == HV_HHIVE_SIGNATURE);

    /* Call the private API call to do the deed for us */
    return HvpWriteHive(RegistryHive, TRUE);
}

/* EOF */
