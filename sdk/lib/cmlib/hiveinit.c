/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Configuration Manager Library - Registry Hive Loading & Initialization
 * COPYRIGHT:   Copyright 2001 - 2005 Eric Kohl
 *              Copyright 2005 Filip Navara <navaraf@reactos.org>
 *              Copyright 2021 Max Korostil
 *              Copyright 2022 George Bișoc <george.bisoc@reactos.org>
 */

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

/* ENUMERATIONS *************************************************************/

typedef enum _RESULT
{
    NotHive,
    Fail,
    NoMemory,
    HiveSuccess,
    RecoverHeader,
    RecoverData,
    SelfHeal
} RESULT;

/* PRIVATE FUNCTIONS ********************************************************/

/**
 * @brief
 * Validates the base block header of a registry
 * file (hive or log).
 *
 * @param[in] BaseBlock
 * A pointer to a base block header to
 * be validated.
 *
 * @param[in] FileType
 * The file type of a registry file to check
 * against the file type of the base block.
 *
 * @return
 * Returns TRUE if the base block header is valid,
 * FALSE otherwise.
 */
BOOLEAN
CMAPI
HvpVerifyHiveHeader(
    _In_ PHBASE_BLOCK BaseBlock,
    _In_ ULONG FileType)
{
    if (BaseBlock->Signature != HV_HBLOCK_SIGNATURE ||
        BaseBlock->Major != HSYS_MAJOR ||
        BaseBlock->Minor < HSYS_MINOR ||
        BaseBlock->Type != FileType ||
        BaseBlock->Format != HBASE_FORMAT_MEMORY ||
        BaseBlock->Cluster != 1 ||
        BaseBlock->Sequence1 != BaseBlock->Sequence2 ||
        HvpHiveHeaderChecksum(BaseBlock) != BaseBlock->CheckSum)
    {
        DPRINT1("Verify Hive Header failed:\n");
        DPRINT1("    Signature: 0x%x, expected 0x%x; Major: 0x%x, expected 0x%x\n",
                BaseBlock->Signature, HV_HBLOCK_SIGNATURE, BaseBlock->Major, HSYS_MAJOR);
        DPRINT1("    Minor: 0x%x expected to be >= 0x%x; Type: 0x%x, expected 0x%x\n",
                BaseBlock->Minor, HSYS_MINOR, BaseBlock->Type, FileType);
        DPRINT1("    Format: 0x%x, expected 0x%x; Cluster: 0x%x, expected 1\n",
                BaseBlock->Format, HBASE_FORMAT_MEMORY, BaseBlock->Cluster);
        DPRINT1("    Sequence: 0x%x, expected 0x%x; Checksum: 0x%x, expected 0x%x\n",
                BaseBlock->Sequence1, BaseBlock->Sequence2,
                HvpHiveHeaderChecksum(BaseBlock), BaseBlock->CheckSum);

        return FALSE;
    }

    return TRUE;
}

/**
 * @brief
 * Frees all the bins within storage space
 * associated with a hive descriptor.
 *
 * @param[in] Hive
 * A pointer to a hive descriptor where
 * all the bins are to be freed.
 */
VOID
CMAPI
HvpFreeHiveBins(
    _In_ PHHIVE Hive)
{
    ULONG i;
    PHBIN Bin;
    ULONG Storage;

    for (Storage = 0; Storage < Hive->StorageTypeCount; Storage++)
    {
        Bin = NULL;
        for (i = 0; i < Hive->Storage[Storage].Length; i++)
        {
            if (Hive->Storage[Storage].BlockList[i].BinAddress == (ULONG_PTR)NULL)
                continue;
            if (Hive->Storage[Storage].BlockList[i].BinAddress != (ULONG_PTR)Bin)
            {
                Bin = (PHBIN)Hive->Storage[Storage].BlockList[i].BinAddress;
                Hive->Free((PHBIN)Hive->Storage[Storage].BlockList[i].BinAddress, 0);
            }
            Hive->Storage[Storage].BlockList[i].BinAddress = (ULONG_PTR)NULL;
            Hive->Storage[Storage].BlockList[i].BlockAddress = (ULONG_PTR)NULL;
        }

        if (Hive->Storage[Storage].Length)
            Hive->Free(Hive->Storage[Storage].BlockList, 0);
    }
}

/**
 * @brief
 * Allocates a cluster-aligned hive base header block.
 *
 * @param[in] Hive
 * A pointer to a hive descriptor where
 * the header block allocator function is to
 * be gathered from.
 *
 * @param[in] Paged
 * If set to TRUE, the allocated base block will reside
 * in paged pool, otherwise it will reside in non paged
 * pool.
 *
 * @param[in] Tag
 * A tag name to supply for the allocated memory block
 * for identification. This is for debugging purposes.
 *
 * @return
 * Returns an allocated base block header if the function
 * succeeds, otherwise it returns NULL.
 */
static
__inline
PHBASE_BLOCK
HvpAllocBaseBlockAligned(
    _In_ PHHIVE Hive,
    _In_ BOOLEAN Paged,
    _In_ ULONG Tag)
{
    PHBASE_BLOCK BaseBlock;
    ULONG Alignment;

    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    /* Allocate the buffer */
    BaseBlock = Hive->Allocate(Hive->BaseBlockAlloc, Paged, Tag);
    if (!BaseBlock) return NULL;

    /* Check for, and enforce, alignment */
    Alignment = Hive->Cluster * HSECTOR_SIZE -1;
    if ((ULONG_PTR)BaseBlock & Alignment)
    {
        /* Free the old header and reallocate a new one, always paged */
        Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
        BaseBlock = Hive->Allocate(PAGE_SIZE, TRUE, Tag);
        if (!BaseBlock) return NULL;

        Hive->BaseBlockAlloc = PAGE_SIZE;
    }

    return BaseBlock;
}

/**
 * @brief
 * Initializes a NULL-terminated Unicode hive file name
 * of a hive header by copying the last 31 characters of
 * the hive file name. Mainly used for debugging purposes.
 *
 * @param[in,out] BaseBlock
 * A pointer to a base block header where the hive
 * file name is to be copied to.
 *
 * @param[in] FileName
 * A pointer to a Unicode string structure containing
 * the hive file name to be copied from. If this argument
 * is NULL, the base block will not have any hive file name.
 */
static
VOID
HvpInitFileName(
    _Inout_ PHBASE_BLOCK BaseBlock,
    _In_opt_ PCUNICODE_STRING FileName)
{
    ULONG_PTR Offset;
    SIZE_T    Length;

    /* Always NULL-initialize */
    RtlZeroMemory(BaseBlock->FileName, (HIVE_FILENAME_MAXLEN + 1) * sizeof(WCHAR));

    /* Copy the 31 last characters of the hive file name if any */
    if (!FileName) return;

    if (FileName->Length / sizeof(WCHAR) <= HIVE_FILENAME_MAXLEN)
    {
        Offset = 0;
        Length = FileName->Length;
    }
    else
    {
        Offset = FileName->Length / sizeof(WCHAR) - HIVE_FILENAME_MAXLEN;
        Length = HIVE_FILENAME_MAXLEN * sizeof(WCHAR);
    }

    RtlCopyMemory(BaseBlock->FileName, FileName->Buffer + Offset, Length);
}

/**
 * @brief
 * Initializes a hive descriptor structure for a
 * newly created hive in memory.
 *
 * @param[in,out] RegistryHive
 * A pointer to a registry hive descriptor where
 * the internal structures field are to be initialized
 * for the said hive.
 *
 * @param[in] FileName
 * A pointer to a Unicode string structure containing
 * the hive file name to be copied from. If this argument
 * is NULL, the base block will not have any hive file name.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has created the
 * hive descriptor successfully. STATUS_NO_MEMORY is returned
 * if the base header block could not be allocated.
 */
NTSTATUS
CMAPI
HvpCreateHive(
    _Inout_ PHHIVE RegistryHive,
    _In_opt_ PCUNICODE_STRING FileName)
{
    PHBASE_BLOCK BaseBlock;
    ULONG Index;

    /* Allocate the base block */
    BaseBlock = HvpAllocBaseBlockAligned(RegistryHive, FALSE, TAG_CM);
    if (BaseBlock == NULL)
        return STATUS_NO_MEMORY;

    /* Clear it */
    RtlZeroMemory(BaseBlock, RegistryHive->BaseBlockAlloc);

    BaseBlock->Signature = HV_HBLOCK_SIGNATURE;
    BaseBlock->Major = HSYS_MAJOR;
    BaseBlock->Minor = HSYS_MINOR;
    BaseBlock->Type = HFILE_TYPE_PRIMARY;
    BaseBlock->Format = HBASE_FORMAT_MEMORY;
    BaseBlock->Cluster = 1;
    BaseBlock->RootCell = HCELL_NIL;
    BaseBlock->Length = 0;
    BaseBlock->Sequence1 = 1;
    BaseBlock->Sequence2 = 1;
    BaseBlock->TimeStamp.QuadPart = 0ULL;

    /*
     * No need to compute the checksum since
     * the hive resides only in memory so far.
     */
    BaseBlock->CheckSum = 0;

    /* Set default boot type */
    BaseBlock->BootType = HBOOT_TYPE_REGULAR;

    /* Setup hive data */
    RegistryHive->BaseBlock = BaseBlock;
    RegistryHive->Version = BaseBlock->Minor; // == HSYS_MINOR

    for (Index = 0; Index < 24; Index++)
    {
        RegistryHive->Storage[Stable].FreeDisplay[Index] = HCELL_NIL;
        RegistryHive->Storage[Volatile].FreeDisplay[Index] = HCELL_NIL;
    }

    HvpInitFileName(BaseBlock, FileName);

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Initializes a hive descriptor from an already loaded
 * registry hive stored in memory. The data of the hive is
 * copied and it is prepared for read/write access.
 *
 * @param[in] Hive
 * A pointer to a registry hive descriptor where
 * the internal structures field are to be initialized
 * from hive data that is already loaded in memory.
 *
 * @param[in] ChunkBase
 * A pointer to a valid base block header containing
 * registry header data for initialization.
 *
 * @param[in] FileName
 * A pointer to a Unicode string structure containing
 * the hive file name to be copied from. If this argument
 * is NULL, the base block will not have any hive file name.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has initialized the
 * hive descriptor successfully. STATUS_REGISTRY_CORRUPT is
 * returned if the base block header contains invalid header
 * data. STATUS_NO_MEMORY is returned if memory could not
 * be allocated for registry stuff.
 */
NTSTATUS
CMAPI
HvpInitializeMemoryHive(
    _In_ PHHIVE Hive,
    _In_ PHBASE_BLOCK ChunkBase,
    _In_opt_ PCUNICODE_STRING FileName)
{
    SIZE_T BlockIndex;
    PHBIN Bin, NewBin;
    ULONG i;
    ULONG BitmapSize;
    PULONG BitmapBuffer;
    SIZE_T ChunkSize;

    ChunkSize = ChunkBase->Length;
    DPRINT("ChunkSize: %zx\n", ChunkSize);

    if (ChunkSize < sizeof(HBASE_BLOCK) ||
        !HvpVerifyHiveHeader(ChunkBase, HFILE_TYPE_PRIMARY))
    {
        DPRINT1("Registry is corrupt: ChunkSize 0x%zx < sizeof(HBASE_BLOCK) 0x%zx, "
                "or HvpVerifyHiveHeader() failed\n", ChunkSize, sizeof(HBASE_BLOCK));
        return STATUS_REGISTRY_CORRUPT;
    }

    /* Allocate the base block */
    Hive->BaseBlock = HvpAllocBaseBlockAligned(Hive, FALSE, TAG_CM);
    if (Hive->BaseBlock == NULL)
        return STATUS_NO_MEMORY;

    RtlCopyMemory(Hive->BaseBlock, ChunkBase, sizeof(HBASE_BLOCK));

    /* Setup hive data */
    Hive->Version = ChunkBase->Minor;

    /*
     * Build a block list from the in-memory chunk and copy the data as
     * we go.
     */

    Hive->Storage[Stable].Length = (ULONG)(ChunkSize / HBLOCK_SIZE);
    Hive->Storage[Stable].BlockList =
        Hive->Allocate(Hive->Storage[Stable].Length *
                       sizeof(HMAP_ENTRY), FALSE, TAG_CM);
    if (Hive->Storage[Stable].BlockList == NULL)
    {
        DPRINT1("Allocating block list failed\n");
        Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_NO_MEMORY;
    }

    for (BlockIndex = 0; BlockIndex < Hive->Storage[Stable].Length; )
    {
        Bin = (PHBIN)((ULONG_PTR)ChunkBase + (BlockIndex + 1) * HBLOCK_SIZE);
        if (Bin->Signature != HV_HBIN_SIGNATURE ||
           (Bin->Size % HBLOCK_SIZE) != 0 ||
           (Bin->FileOffset / HBLOCK_SIZE) != BlockIndex)
        {
            /*
             * Bin is toast but luckily either the signature, size or offset
             * is out of order. For the signature it is obvious what we are going
             * to do, for the offset we are re-positioning the bin back to where it
             * was and for the size we will set it up to a block size, since technically
             * a hive bin is large as a block itself to accommodate cells.
             */
            if (!CmIsSelfHealEnabled(FALSE))
            {
                DPRINT1("Invalid bin at BlockIndex %lu, Signature 0x%x, Size 0x%x. Self-heal not possible!\n",
                    (unsigned long)BlockIndex, (unsigned)Bin->Signature, (unsigned)Bin->Size);
                Hive->Free(Hive->Storage[Stable].BlockList, 0);
                Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
                return STATUS_REGISTRY_CORRUPT;
            }

            /* Fix this bin */
            Bin->Signature = HV_HBIN_SIGNATURE;
            Bin->Size = HBLOCK_SIZE;
            Bin->FileOffset = BlockIndex * HBLOCK_SIZE;
            ChunkBase->BootType |= HBOOT_TYPE_SELF_HEAL;
            DPRINT1("Bin at index %lu is corrupt and it has been repaired!\n", (unsigned long)BlockIndex);
        }

        NewBin = Hive->Allocate(Bin->Size, TRUE, TAG_CM);
        if (NewBin == NULL)
        {
            Hive->Free(Hive->Storage[Stable].BlockList, 0);
            Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
            return STATUS_NO_MEMORY;
        }

        Hive->Storage[Stable].BlockList[BlockIndex].BinAddress = (ULONG_PTR)NewBin;
        Hive->Storage[Stable].BlockList[BlockIndex].BlockAddress = (ULONG_PTR)NewBin;

        RtlCopyMemory(NewBin, Bin, Bin->Size);

        if (Bin->Size > HBLOCK_SIZE)
        {
            for (i = 1; i < Bin->Size / HBLOCK_SIZE; i++)
            {
                Hive->Storage[Stable].BlockList[BlockIndex + i].BinAddress = (ULONG_PTR)NewBin;
                Hive->Storage[Stable].BlockList[BlockIndex + i].BlockAddress =
                    ((ULONG_PTR)NewBin + (i * HBLOCK_SIZE));
            }
        }

        BlockIndex += Bin->Size / HBLOCK_SIZE;
    }

    if (!NT_SUCCESS(HvpCreateHiveFreeCellList(Hive)))
    {
        HvpFreeHiveBins(Hive);
        Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_NO_MEMORY;
    }

    BitmapSize = ROUND_UP(Hive->Storage[Stable].Length,
                          sizeof(ULONG) * 8) / 8;
    BitmapBuffer = (PULONG)Hive->Allocate(BitmapSize, TRUE, TAG_CM);
    if (BitmapBuffer == NULL)
    {
        HvpFreeHiveBins(Hive);
        Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_NO_MEMORY;
    }

    RtlInitializeBitMap(&Hive->DirtyVector, BitmapBuffer, BitmapSize * 8);
    RtlClearAllBits(&Hive->DirtyVector);

    /*
     * Mark the entire hive as dirty. Indeed we understand if we charged up
     * the alternate variant of the primary hive (e.g. SYSTEM.ALT) because
     * FreeLdr could not load the main SYSTEM hive, due to corruptions, and
     * repairing it with a LOG did not help at all.
     */
    if (ChunkBase->BootRecover == HBOOT_BOOT_RECOVERED_BY_ALTERNATE_HIVE)
    {
        RtlSetAllBits(&Hive->DirtyVector);
        Hive->DirtyCount = Hive->DirtyVector.SizeOfBitMap;
    }

    HvpInitFileName(Hive->BaseBlock, FileName);

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Initializes a hive descriptor for an already loaded hive
 * that is stored in memory. This descriptor serves to denote
 * such hive as being "flat", that is, the data and properties
 * can be only read and not written into.
 *
 * @param[in] Hive
 * A pointer to a registry hive descriptor where
 * the internal structures fields are to be initialized
 * from hive data that is already loaded in memory. Such
 * hive descriptor will become read-only and flat.
 *
 * @param[in] ChunkBase
 * A pointer to a valid base block header containing
 * registry header data for initialization.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has initialized the
 * flat hive descriptor. STATUS_REGISTRY_CORRUPT is returned if
 * the base block header contains invalid header data.
 */
NTSTATUS
CMAPI
HvpInitializeFlatHive(
    _In_ PHHIVE Hive,
    _In_ PHBASE_BLOCK ChunkBase)
{
    if (!HvpVerifyHiveHeader(ChunkBase, HFILE_TYPE_PRIMARY))
        return STATUS_REGISTRY_CORRUPT;

    /* Setup hive data */
    Hive->BaseBlock = ChunkBase;
    Hive->Version = ChunkBase->Minor;
    Hive->Flat = TRUE;
    Hive->ReadOnly = TRUE;

    Hive->StorageTypeCount = 1;

    /* Set default boot type */
    ChunkBase->BootType = HBOOT_TYPE_REGULAR;

    return STATUS_SUCCESS;
}

/**
 * @brief
 * Retrieves the base block hive header from the
 * primary hive file stored in physical backing storage.
 * This function may invoke a self-healing warning if
 * hive header couldn't be obtained. See Return and Remarks
 * sections for further information.
 *
 * @param[in] Hive
 * A pointer to a registry hive descriptor that points
 * to the primary hive being loaded. This descriptor is
 * needed to obtain the hive header block from said hive.
 *
 * @param[in,out] HiveBaseBlock
 * A pointer returned by the function that contains
 * the hive header base block buffer obtained from
 * the primary hive file pointed by the Hive argument.
 * This parameter must not be NULL!
 *
 * @param[in,out] TimeStamp
 * A pointer returned by the function that contains
 * the time-stamp of the registry hive file at the
 * moment of creation or modification. This parameter
 * must not be NULL!
 *
 * @return
 * This function returns a result indicator. That is,
 * HiveSuccess is returned if the hive header was obtained
 * successfully. NoMemory is returned if the hive base block
 * could not be allocated. NotHive is returned if the hive file
 * that's been read isn't actually a hive. RecoverHeader is
 * returned if the header needs to be recovered. RecoverData
 * is returned if the hive data needs to be returned.
 *
 * @remarks
 * RecoverHeader and RecoverData are status indicators that
 * invoke a self-healing procedure if the hive header could not
 * be obtained in a normal way and as a matter of fact the whole
 * registry initialization procedure is orchestrated. RecoverHeader
 * implies that the base block header of a hive is corrupt and it
 * needs to be recovered, whereas RecoverData implies the registry
 * data is corrupt. The latter status indicator is less severe unlike
 * the former because the system can cope with data loss.
 */
RESULT
CMAPI
HvpGetHiveHeader(
    _In_ PHHIVE Hive,
    _Inout_ PHBASE_BLOCK *HiveBaseBlock,
    _Inout_ PLARGE_INTEGER TimeStamp)
{
    PHBASE_BLOCK BaseBlock;
    ULONG Result;
    ULONG FileOffset;
    PHBIN FirstBin;

    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    /* Assume failure and allocate the base block */
    *HiveBaseBlock = NULL;
    BaseBlock = HvpAllocBaseBlockAligned(Hive, TRUE, TAG_CM);
    if (!BaseBlock)
    {
        DPRINT1("Failed to allocate an aligned base block buffer\n");
        return NoMemory;
    }

    /* Clear it */
    RtlZeroMemory(BaseBlock, sizeof(HBASE_BLOCK));

    /* Now read it from disk */
    FileOffset = 0;
    Result = Hive->FileRead(Hive,
                            HFILE_TYPE_PRIMARY,
                            &FileOffset,
                            BaseBlock,
                            Hive->Cluster * HSECTOR_SIZE);
    if (!Result)
    {
        /*
         * Don't assume the hive is ultimately destroyed
         * but instead try to read the first block of
         * the first bin hive. So that we're sure of
         * ourselves we can somewhat recover this hive.
         */
        FileOffset = HBLOCK_SIZE;
        Result = Hive->FileRead(Hive,
                                HFILE_TYPE_PRIMARY,
                                &FileOffset,
                                (PVOID)BaseBlock,
                                Hive->Cluster * HSECTOR_SIZE);
        if (!Result)
        {
            DPRINT1("Failed to read the first block of the first bin hive (hive too corrupt)\n");
            Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
            return NotHive;
        }

        /*
         * Deconstruct the casted buffer we got
         * into a hive bin. Check if the offset
         * position is in the right place (namely
         * its offset must be 0 because it's the first
         * bin) and it should have a sane signature.
         */
        FirstBin = (PHBIN)BaseBlock;
        if (FirstBin->Signature != HV_HBIN_SIGNATURE ||
            FirstBin->FileOffset != 0)
        {
            DPRINT1("Failed to read the first block of the first bin hive (hive too corrupt)\n");
            Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
            return NotHive;
        }

        /*
         * There's still hope for this hive so acknowledge the
         * caller this hive needs a recoverable header.
         */
        *TimeStamp = BaseBlock->TimeStamp;
        DPRINT1("The hive is not fully corrupt, the base block needs to be RECOVERED\n");
        return RecoverHeader;
    }

    /*
     * This hive has a base block that's not maimed
     * but is the header data valid?
     *
     * FIXME: We must check if primary and secondary
     * sequences mismatch separately and fire up RecoverData
     * in that case  but due to a hack in HvLoadHive, let
     * HvpVerifyHiveHeader check the sequences for now.
     */
    if (!HvpVerifyHiveHeader(BaseBlock, HFILE_TYPE_PRIMARY))
    {
        DPRINT1("The hive base header block needs to be RECOVERED\n");
        *TimeStamp = BaseBlock->TimeStamp;
        Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
        return RecoverHeader;
    }

    /* Return information */
    *HiveBaseBlock = BaseBlock;
    *TimeStamp = BaseBlock->TimeStamp;
    return HiveSuccess;
}

/*
 * FIXME: Disable compilation for AMD64 for now since it makes
 * the FreeLdr binary size so large it makes booting impossible.
 */
#if !defined(_M_AMD64)
/**
 * @brief
 * Computes the hive space size by querying
 * the file size of the associated hive file.
 *
 * @param[in] Hive
 * A pointer to a hive descriptor where the
 * hive length size is to be calculated.
 *
 * @return
 * Returns the computed hive size.
 */
ULONG
CMAPI
HvpQueryHiveSize(
    _In_ PHHIVE Hive)
{
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
    NTSTATUS Status;
    FILE_STANDARD_INFORMATION FileStandard;
    IO_STATUS_BLOCK IoStatusBlock;
#endif
    ULONG HiveSize = 0;

    /*
     * Query the file size of the physical hive
     * file. We need that information in order
     * to ensure how big the hive actually is.
     */
#if !defined(CMLIB_HOST) && !defined(_BLDR_)
    Status = ZwQueryInformationFile(((PCMHIVE)Hive)->FileHandles[HFILE_TYPE_PRIMARY],
                                    &IoStatusBlock,
                                    &FileStandard,
                                    sizeof(FILE_STANDARD_INFORMATION),
                                    FileStandardInformation);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("ZwQueryInformationFile returned 0x%lx\n", Status);
        return HiveSize;
    }

    /* Now compute the hive size */
    HiveSize = FileStandard.EndOfFile.u.LowPart - HBLOCK_SIZE;
#endif
    return HiveSize;
}

/**
 * @brief
 * Recovers the base block header by obtaining
 * it from a log file associated with the hive.
 *
 * @param[in] Hive
 * A pointer to a hive descriptor associated
 * with the log file where the hive header is
 * to be read from.
 *
 * @param[in] TimeStamp
 * A pointer to a time-stamp used to check
 * if the provided time matches with that
 * of the hive.
 *
 * @param[in,out] BaseBlock
 * A pointer returned by the caller that contains
 * the base block header that was read from the log.
 * This base block could be also made manually by hand.
 * See Remarks for further information.
 *
 * @return
 * Returns HiveSuccess if the header was obtained
 * normally from the log. NoMemory is returned if
 * the base block header could not be allocated.
 * Fail is returned if self-healing mode is disabled
 * and the log couldn't be read or a write attempt
 * to the primary hive has failed. SelfHeal is returned
 * to indicate that self-heal mode goes further.
 *
 * @remarks
 * When SelfHeal is returned this indicates that
 * even the log we have gotten at hand is corrupt
 * but since we do have at least a log our only hope
 * is to reconstruct the pieces of the base header
 * by hand.
 */
RESULT
CMAPI
HvpRecoverHeaderFromLog(
    _In_ PHHIVE Hive,
    _In_ PLARGE_INTEGER TimeStamp,
    _Inout_ PHBASE_BLOCK *BaseBlock)
{
    BOOLEAN Success;
    PHBASE_BLOCK LogHeader;
    ULONG FileOffset;
    ULONG HiveSize;
    BOOLEAN HeaderResuscitated;

    /*
     * The cluster must not be greater than what the
     * base block can permit.
     */
    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    /* Assume we haven't resuscitated the header */
    HeaderResuscitated = FALSE;

    /* Allocate an aligned buffer for the log header */
    LogHeader = HvpAllocBaseBlockAligned(Hive, TRUE, TAG_CM);
    if (!LogHeader)
    {
        DPRINT1("Failed to allocate memory for the log header\n");
        return NoMemory;
    }

    /* Zero out our header buffer */
    RtlZeroMemory(LogHeader, HSECTOR_SIZE);

    /* Get the base header from the log */
    FileOffset = 0;
    Success = Hive->FileRead(Hive,
                             HFILE_TYPE_LOG,
                             &FileOffset,
                             LogHeader,
                             Hive->Cluster * HSECTOR_SIZE);
    if (!Success ||
        !HvpVerifyHiveHeader(LogHeader, HFILE_TYPE_LOG) ||
        TimeStamp->HighPart != LogHeader->TimeStamp.HighPart ||
        TimeStamp->LowPart != LogHeader->TimeStamp.LowPart)
    {
        /*
         * We failed to read the base block header from
         * the log, or the header itself or timestamp is
         * invalid. Check if self healing is enabled.
         */
        if (!CmIsSelfHealEnabled(FALSE))
        {
            DPRINT1("The log couldn't be read and self-healing mode is disabled\n");
            Hive->Free(LogHeader, Hive->BaseBlockAlloc);
            return Fail;
        }

        /*
         * Determine the size of this hive so that
         * we can estabilish the length of the base
         * block we are trying to resuscitate.
         */
        HiveSize = HvpQueryHiveSize(Hive);
        if (HiveSize == 0)
        {
            DPRINT1("Failed to query the hive size\n");
            Hive->Free(LogHeader, Hive->BaseBlockAlloc);
            return Fail;
        }

        /*
         * We can still resuscitate the base header if we
         * could not grab one from the log by reconstructing
         * the header internals by hand (this assumes the
         * root cell is not NIL nor damaged). CmCheckRegistry
         * does the ultimate judgement whether the root cell
         * is fatally kaput or not after the hive has been
         * initialized and loaded.
         *
         * For more information about base block header
         * resuscitation, see https://github.com/msuhanov/regf/blob/master/Windows%20registry%20file%20format%20specification.md#notes-4.
         */
        LogHeader->Signature = HV_HBLOCK_SIGNATURE;
        LogHeader->Sequence1 = 1;
        LogHeader->Sequence2 = 1;
        LogHeader->Cluster = 1;
        LogHeader->Length = HiveSize;
        LogHeader->CheckSum = HvpHiveHeaderChecksum(LogHeader);

        /*
         * Acknowledge that we have resuscitated
         * the header.
         */
        HeaderResuscitated = TRUE;
        DPRINT1("Header has been resuscitated, triggering self-heal mode\n");
    }

    /*
     * Tag this log header as a primary hive before
     * writing it to the hive.
     */
    LogHeader->Type = HFILE_TYPE_PRIMARY;

    /*
     * If we have not made attempts of recovering
     * the header due to log corruption then we
     * have to compute the checksum. This is
     * already done when the header has been resuscitated
     * so don't try to do it twice.
     */
    if (!HeaderResuscitated)
    {
        LogHeader->CheckSum = HvpHiveHeaderChecksum(LogHeader);
    }

    /* Write the header back to hive now */
    Success = Hive->FileWrite(Hive,
                              HFILE_TYPE_PRIMARY,
                              &FileOffset,
                              LogHeader,
                              Hive->Cluster * HSECTOR_SIZE);
    if (!Success)
    {
        DPRINT1("Couldn't write the base header to primary hive\n");
        Hive->Free(LogHeader, Hive->BaseBlockAlloc);
        return Fail;
    }

    *BaseBlock = LogHeader;
    return HeaderResuscitated ? SelfHeal : HiveSuccess;
}

/**
 * @brief
 * Recovers the registry data by obtaining it
 * from a log that is associated with the hive.
 *
 * @param[in] Hive
 * A pointer to a hive descriptor associated
 * with the log file where the hive data is to
 * be read from.
 *
 * @param[in] BaseBlock
 * A pointer to a base block header.
 *
 * @return
 * Returns HiveSuccess if the data was obtained
 * normally from the log. Fail is returned if
 * self-healing is disabled and we couldn't be
 * able to read the data from the log or the
 * dirty vector signature is garbage or we
 * failed to write the data block to the primary
 * hive. SelfHeal is returned to indicate that
 * the log is corrupt and the system will continue
 * to be recovered at the expense of data loss.
 */
RESULT
CMAPI
HvpRecoverDataFromLog(
    _In_ PHHIVE Hive,
    _In_ PHBASE_BLOCK BaseBlock)
{
    BOOLEAN Success;
    ULONG FileOffset;
    ULONG BlockIndex;
    ULONG LogIndex;
    ULONG StorageLength;
    UCHAR DirtyVector[HSECTOR_SIZE];
    UCHAR Buffer[HBLOCK_SIZE];

    /* Read the dirty data from the log */
    FileOffset = HV_LOG_HEADER_SIZE;
    Success = Hive->FileRead(Hive,
                             HFILE_TYPE_LOG,
                             &FileOffset,
                             DirtyVector,
                             HSECTOR_SIZE);
    if (!Success)
    {
        if (!CmIsSelfHealEnabled(FALSE))
        {
            DPRINT1("The log couldn't be read and self-healing mode is disabled\n");
            return Fail;
        }

        /*
         * There's nothing we can do on a situation
         * where dirty data could not be read from
         * the log. It does not make much sense to
         * behead the system on such scenario so
         * trigger a self-heal and go on. The worst
         * thing that can happen? Data loss, that's it.
         */
        DPRINT1("Triggering self-heal mode, DATA LOSS IS IMMINENT\n");
        return SelfHeal;
    }

    /* Check the dirty vector */
    if (*((PULONG)DirtyVector) != HV_LOG_DIRTY_SIGNATURE)
    {
        if (!CmIsSelfHealEnabled(FALSE))
        {
            DPRINT1("The log's dirty vector signature is not valid\n");
            return Fail;
        }

        /*
         * Trigger a self-heal like above. If the
         * vector signature is garbage then logically
         * whatever comes after the signature is also
         * garbage.
         */
        DPRINT1("Triggering self-heal mode, DATA LOSS IS IMMINENT\n");
        return SelfHeal;
    }

    /* Now read each data individually and write it back to hive */
    LogIndex = 0;
    StorageLength = BaseBlock->Length / HBLOCK_SIZE;
    for (BlockIndex = 0; BlockIndex < StorageLength; BlockIndex++)
    {
        /* Skip this block if it's not dirty and go to the next one */
        if (DirtyVector[BlockIndex + sizeof(HV_LOG_DIRTY_SIGNATURE)] != HV_LOG_DIRTY_BLOCK)
        {
            continue;
        }

        FileOffset = HSECTOR_SIZE + HSECTOR_SIZE + LogIndex * HBLOCK_SIZE;
        Success = Hive->FileRead(Hive,
                                 HFILE_TYPE_LOG,
                                 &FileOffset,
                                 Buffer,
                                 HBLOCK_SIZE);
        if (!Success)
        {
            DPRINT1("Failed to read the dirty block (index %u)\n", BlockIndex);
            return Fail;
        }

        FileOffset = HBLOCK_SIZE + BlockIndex * HBLOCK_SIZE;
        Success = Hive->FileWrite(Hive,
                                  HFILE_TYPE_PRIMARY,
                                  &FileOffset,
                                  Buffer,
                                  HBLOCK_SIZE);
        if (!Success)
        {
            DPRINT1("Failed to write dirty block to hive (index %u)\n", BlockIndex);
            return Fail;
        }

        /* Increment the index in log as we continue further */
        LogIndex++;
    }

    return HiveSuccess;
}
#endif

/**
 * @brief
 * Loads a registry hive from a physical hive file
 * within the physical backing storage. Base block
 * and registry data are read from the said physical
 * hive file. This function can perform registry recovery
 * if hive loading could not be done normally.
 *
 * @param[in] Hive
 * A pointer to a hive descriptor where the said hive
 * is to be loaded from the physical hive file.
 *
 * @param[in] FileName
 * A pointer to a NULL-terminated Unicode string structure
 * containing the hive file name to be copied from.
 *
 * @return
 * STATUS_SUCCESS is returned if the hive has been loaded
 * successfully. STATUS_INSUFFICIENT_RESOURCES is returned
 * if there's not enough memory resources to satisfy registry
 * operations and/or requests. STATUS_NOT_REGISTRY_FILE is returned
 * if the hive is not actually a hive file. STATUS_REGISTRY_CORRUPT
 * is returned if the hive has subdued previous damage and
 * the hive could not be recovered because there's no
 * log present or self healing is disabled. STATUS_REGISTRY_RECOVERED
 * is returned if the hive has been recovered. An eventual flush
 * of the registry is needed after the hive's been fully loaded.
 */
NTSTATUS
CMAPI
HvLoadHive(
    _In_ PHHIVE Hive,
    _In_opt_ PCUNICODE_STRING FileName)
{
    NTSTATUS Status;
    BOOLEAN Success;
    PHBASE_BLOCK BaseBlock = NULL;
/* FIXME: See the comment above (near HvpQueryHiveSize) */
#if defined(_M_AMD64)
    ULONG Result;
#else
    ULONG Result, Result2;
#endif
    LARGE_INTEGER TimeStamp;
    ULONG Offset = 0;
    PVOID HiveData;
    ULONG FileSize;
    BOOLEAN HiveSelfHeal = FALSE;

    /* Get the hive header */
    Result = HvpGetHiveHeader(Hive, &BaseBlock, &TimeStamp);
    switch (Result)
    {
        /* Out of memory */
        case NoMemory:
        {
            /* Fail */
            DPRINT1("There's no enough memory to get the header\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Not a hive */
        case NotHive:
        {
            /* Fail */
            DPRINT1("The hive is not an actual registry hive file\n");
            return STATUS_NOT_REGISTRY_FILE;
        }

        /* Hive data needs a repair */
        case RecoverData:
        {
            /*
             * FIXME: We must be handling this status
             * case if the header isn't corrupt but
             * the counter sequences do not match but
             * due to a hack in HvLoadHive we have
             * to do both a header + data recovery.
             * RecoverHeader also implies RecoverData
             * anyway. When HvLoadHive gets rid of
             * that hack, data recovery must be done
             * after we read the hive block by block.
             */
            break;
        }

        /* Hive header needs a repair */
        case RecoverHeader:
/* FIXME: See the comment above (near HvpQueryHiveSize) */
#if defined(_M_AMD64)
        {
            return STATUS_REGISTRY_CORRUPT;
        }
#else
        {
            /* Check if this hive has a log at hand to begin with */
            #if (NTDDI_VERSION < NTDDI_VISTA)
            if (!Hive->Log)
            {
                DPRINT1("The hive has no log for header recovery\n");
                return STATUS_REGISTRY_CORRUPT;
            }
            #endif

            /* The header needs to be recovered so do it */
            DPRINT1("Attempting to heal the header...\n");
            Result2 = HvpRecoverHeaderFromLog(Hive, &TimeStamp, &BaseBlock);
            if (Result2 == NoMemory)
            {
                DPRINT1("There's no enough memory to recover header from log\n");
                return STATUS_INSUFFICIENT_RESOURCES;
            }

            /* Did we fail? */
            if (Result2 == Fail)
            {
                DPRINT1("Failed to recover the hive header\n");
                return STATUS_REGISTRY_CORRUPT;
            }

            /* Did we trigger the self-heal mode? */
            if (Result2 == SelfHeal)
            {
                HiveSelfHeal = TRUE;
            }

            /* Now recover the data */
            Result2 = HvpRecoverDataFromLog(Hive, BaseBlock);
            if (Result2 == Fail)
            {
                DPRINT1("Failed to recover the hive data\n");
                return STATUS_REGISTRY_CORRUPT;
            }

            /* Tag the boot as self heal if we haven't done it before */
            if ((Result2 == SelfHeal) && (!HiveSelfHeal))
            {
                HiveSelfHeal = TRUE;
            }

            break;
        }
#endif
    }

    /* Set the boot type */
    BaseBlock->BootType = HiveSelfHeal ? HBOOT_TYPE_SELF_HEAL : HBOOT_TYPE_REGULAR;

    /* Setup hive data */
    Hive->BaseBlock = BaseBlock;
    Hive->Version = BaseBlock->Minor;

    /* Allocate a buffer large enough to hold the hive */
    FileSize = HBLOCK_SIZE + BaseBlock->Length; // == sizeof(HBASE_BLOCK) + BaseBlock->Length;
    HiveData = Hive->Allocate(FileSize, TRUE, TAG_CM);
    if (!HiveData)
    {
        Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
        DPRINT1("There's no enough memory to allocate hive data\n");
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* HACK (see explanation below): Now read the whole hive */
    Success = Hive->FileRead(Hive,
                             HFILE_TYPE_PRIMARY,
                             &Offset,
                             HiveData,
                             FileSize);
    if (!Success)
    {
        DPRINT1("Failed to read the whole hive\n");
        Hive->Free(HiveData, FileSize);
        Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_NOT_REGISTRY_FILE;
    }

    /*
     * HACK (FIXME): Free our base block... it's useless in
     * this implementation.
     *
     * And it's useless because while the idea of reading the
     * hive from physical file is correct, the implementation
     * is hacky and incorrect. Instead of reading the whole hive,
     * we should be instead reading the hive block by block,
     * deconstruct the block buffer and enlist the bins and
     * prepare the storage for the hive. What we currently do
     * is we try to initialize the hive storage and bins enlistment
     * by calling HvpInitializeMemoryHive below. This mixes
     * HINIT_FILE and HINIT_MEMORY together which is disgusting
     * because HINIT_FILE implementation shouldn't be calling
     * HvpInitializeMemoryHive.
     */
    Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
    Status = HvpInitializeMemoryHive(Hive, HiveData, FileName);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to initialize hive from memory\n");
        Hive->Free(HiveData, FileSize);
        return Status;
    }

    /*
     * If we have done some sort of recovery against
     * the hive we were going to load it from file,
     * tell the caller we did recover it. The caller
     * is responsible to flush the data later on.
     */
    return (Result == RecoverHeader) ? STATUS_REGISTRY_RECOVERED : STATUS_SUCCESS;
}

/**
 * @brief
 * Initializes a registry hive. It allocates a hive
 * descriptor and sets up the hive type depending
 * on the type chosen by the caller.
 *
 * @param[in,out] RegistryHive
 * A pointer to a hive descriptor to be initialized.
 *
 * @param[in] OperationType
 * The operation type to choose for hive initialization.
 * For further information about this, see Remarks.
 *
 * @param[in] HiveFlags
 * A hive flag. Such flag is used to determine what kind
 * of action must be taken into the hive or what aspects
 * must be taken into account for such hive. For further
 * information, see Remarks.
 *
 * @param[in] FileType
 * Hive file type. For the newly initialized hive, you can
 * choose from three different types for the hive:
 *
 * HFILE_TYPE_PRIMARY - Initializes a hive as primary hive
 * of the system.
 *
 * HFILE_TYPE_LOG - The newly created hive is a hive log.
 * Logs don't exist per se but they're accompanied with their
 * associated primary hives. The Log field member of the hive
 * descriptor is set to TRUE.
 *
 * HFILE_TYPE_EXTERNAL - The newly created hive is a portable
 * hive, that can be used and copied for different machines,
 * unlike primary hives.
 *
 * HFILE_TYPE_ALTERNATE - The newly created hive is an alternate hive.
 * Technically speaking it is the same as a primary hive (the representation
 * of on-disk image of the registry header is HFILE_TYPE_PRIMARY), with
 * the purpose is to serve as a backup hive. The Alternate field of the
 * hive descriptor is set to TRUE. Only the SYSTEM hive has a backup
 * alternate hive.
 *
 * @param[in] HiveData
 * An arbitrary pointer that points to the hive data. Usually this
 * data is in form of a hive base block given by the caller of this
 * function.
 *
 * @param[in] Allocate
 * A pointer to a ALLOCATE_ROUTINE function that describes
 * the main allocation routine for this hive. This parameter
 * can be NULL.
 *
 * @param[in] Free
 * A pointer to a FREE_ROUTINE function that describes the
 * the main memory freeing routine for this hive. This parameter
 * can be NULL.
 *
 * @param[in] FileSetSize
 * A pointer to a FILE_SET_SIZE_ROUTINE function that describes
 * the file set size routine for this hive. This parameter
 * can be NULL.
 *
 * @param[in] FileWrite
 * A pointer to a FILE_WRITE_ROUTINE function that describes
 * the file writing routine for this hive. This parameter
 * can be NULL.
 *
 * @param[in] FileRead
 * A pointer to a FILE_READ_ROUTINE function that describes
 * the file reading routine for this hive. This parameter
 * can be NULL.
 *
 * @param[in] FileFlush
 * A pointer to a FILE_FLUSH_ROUTINE function that describes
 * the file flushing routine for this hive. This parameter
 * can be NULL.
 *
 * @param[in] Cluster
 * The registry hive cluster to be set. Usually this value
 * is set to 1.
 *
 * @param[in] FileName
 * A to a NULL-terminated Unicode string structure containing
 * the hive file name. This parameter can be NULL.
 *
 * @return
 * Returns STATUS_SUCCESS if the function has successfully
 * initialized the hive. STATUS_REGISTRY_RECOVERED is returned
 * if the hive has subdued previous damage and it's been recovered.
 * This function will perform a hive writing and flushing with
 * healthy and recovered data in that case. STATUS_REGISTRY_IO_FAILED
 * is returned if registry hive writing/flushing of recovered data
 * has failed. STATUS_INVALID_PARAMETER is returned if an invalid
 * operation type pointed by OperationType parameter has been
 * submitted. A failure NTSTATUS code is returned otherwise.
 *
 * @remarks
 * OperationType parameter influences how should the hive be
 * initialized. These are the following supported operation
 * types:
 *
 * HINIT_CREATE -- Creates a new fresh hive.
 *
 * HINIT_MEMORY -- Initializes a registry hive that already exists
 *                 from memory. The hive data is copied from the
 *                 loaded hive in memory and used for read/write
 *                 access.
 *
 * HINIT_FLAT -- Initializes a flat registry hive, with data that can
 *               only be read and not written into. Cells are always
 *               allocated on a flat hive.
 *
 * HINIT_FILE -- Initializes a hive from a hive file from the physical
 *               backing storage of the system. In this situation the
 *               function will perform self-healing and resuscitation
 *               procedures if data read from the physical hive file
 *               is corrupt.
 *
 * HINIT_MEMORY_INPLACE -- This operation type is similar to HINIT_FLAT,
 *                         with the difference is that the hive is initialized
 *                         with hive data from memory. The hive can only be read
 *                         and not written into.
 *
 * HINIT_MAPFILE -- Initializes a hive from a hive file from the physical
 *                  backing storage of the system. Unlike HINIT_FILE, the
 *                  initialized hive is not backed to paged pool in memory
 *                  but rather through mapping views.
 *
 * Alongside the operation type, the hive flags also influence the aspect
 * of the newly initialized hive. These are the following supported hive
 * flags:
 *
 * HIVE_VOLATILE -- Tells the function that this hive will be volatile, that
 *                  is, the data stored inside the hive space resides only
 *                  in volatile memory of the system, aka the RAM, and the
 *                  data will be erased upon shutdown of the system.
 *
 * HIVE_NOLAZYFLUSH -- Tells the function that no lazy flushing must be
 *                     done to this hive.
 */
NTSTATUS
CMAPI
HvInitialize(
    _Inout_ PHHIVE RegistryHive,
    _In_  ULONG OperationType,
    _In_  ULONG HiveFlags,
    _In_  ULONG FileType,
    _In_opt_ PVOID HiveData,
    _In_opt_ PALLOCATE_ROUTINE Allocate,
    _In_opt_ PFREE_ROUTINE Free,
    _In_opt_ PFILE_SET_SIZE_ROUTINE FileSetSize,
    _In_opt_ PFILE_WRITE_ROUTINE FileWrite,
    _In_opt_ PFILE_READ_ROUTINE FileRead,
    _In_opt_ PFILE_FLUSH_ROUTINE FileFlush,
    _In_ ULONG Cluster,
    _In_opt_ PCUNICODE_STRING FileName)
{
    NTSTATUS Status;
    PHHIVE Hive = RegistryHive;

    /*
     * Create a new hive structure that will hold all the maintenance data.
     */

    RtlZeroMemory(Hive, sizeof(HHIVE));
    Hive->Signature = HV_HHIVE_SIGNATURE;

    Hive->Allocate = Allocate;
    Hive->Free = Free;
    Hive->FileSetSize = FileSetSize;
    Hive->FileWrite = FileWrite;
    Hive->FileRead = FileRead;
    Hive->FileFlush = FileFlush;

    Hive->RefreshCount = 0;
    Hive->StorageTypeCount = HTYPE_COUNT;
    Hive->Cluster = Cluster;
    Hive->BaseBlockAlloc = sizeof(HBASE_BLOCK); // == HBLOCK_SIZE

    Hive->Version = HSYS_MINOR;
#if (NTDDI_VERSION < NTDDI_VISTA)
    Hive->Log = (FileType == HFILE_TYPE_LOG);
    Hive->Alternate = (FileType == HFILE_TYPE_ALTERNATE);
#endif
    Hive->HiveFlags = HiveFlags & ~HIVE_NOLAZYFLUSH;

    // TODO: The CellRoutines point to different callbacks
    // depending on the OperationType.
    Hive->GetCellRoutine = HvpGetCellData;
    Hive->ReleaseCellRoutine = NULL;

    switch (OperationType)
    {
        case HINIT_CREATE:
        {
            /* Create a new fresh hive */
            Status = HvpCreateHive(Hive, FileName);
            break;
        }

        case HINIT_MEMORY:
        {
            /* Initialize a hive from memory */
            Status = HvpInitializeMemoryHive(Hive, HiveData, FileName);
            break;
        }

        case HINIT_FLAT:
        {
            /* Initialize a flat read-only hive */
            Status = HvpInitializeFlatHive(Hive, HiveData);
            break;
        }

        case HINIT_FILE:
        {
            /* Initialize a hive by loading it from physical file in backing storage */
            Status = HvLoadHive(Hive, FileName);
            if ((Status != STATUS_SUCCESS) &&
                (Status != STATUS_REGISTRY_RECOVERED))
            {
                /* Unrecoverable failure */
                DPRINT1("Registry hive couldn't be initialized, it's corrupt (hive 0x%p)\n", Hive);
                return Status;
            }

/* FIXME: See the comment above (near HvpQueryHiveSize) */
#if !defined(_M_AMD64)
            /*
             * Check if we have recovered this hive. We are responsible to
             * flush the primary hive back to backing storage afterwards.
             */
            if (Status == STATUS_REGISTRY_RECOVERED)
            {
                if (!HvSyncHiveFromRecover(Hive))
                {
                    DPRINT1("Fail to write healthy data back to hive\n");
                    return STATUS_REGISTRY_IO_FAILED;
                }

                /*
                 * We are saved from hell, now clear out the
                 * dirty bits and dirty count.
                 *
                 * FIXME: We must as well clear out the log
                 * and reset its size to 0 but we are lacking
                 * in code that deals with log growing/shrinking
                 * management. When the time comes to implement
                 * this stuff we must set the LogSize and file size
                 * to 0 here.
                 */
                RtlClearAllBits(&Hive->DirtyVector);
                Hive->DirtyCount = 0;

                /*
                 * Masquerade the status code as success.
                 * STATUS_REGISTRY_RECOVERED is not a failure
                 * code but not STATUS_SUCCESS either so the caller
                 * thinks we failed at our job.
                 */
                Status = STATUS_SUCCESS;
            }
#endif
            break;
        }

        case HINIT_MEMORY_INPLACE:
        {
            // Status = HvpInitializeMemoryInplaceHive(Hive, HiveData);
            // break;
            DPRINT1("HINIT_MEMORY_INPLACE is UNIMPLEMENTED\n");
            return STATUS_NOT_IMPLEMENTED;
        }

        case HINIT_MAPFILE:
        {
            DPRINT1("HINIT_MAPFILE is UNIMPLEMENTED\n");
            return STATUS_NOT_IMPLEMENTED;
        }

        default:
        {
            DPRINT1("Invalid operation type (OperationType = %u)\n", OperationType);
            return STATUS_INVALID_PARAMETER;
        }
    }

    return Status;
}

/**
 * @brief
 * Frees all the bins within the storage, the dirty vector
 * and the base block associated with the given registry
 * hive descriptor.
 *
 * @param[in] RegistryHive
 * A pointer to a hive descriptor where all of its data
 * is to be freed.
 */
VOID
CMAPI
HvFree(
    _In_ PHHIVE RegistryHive)
{
    if (!RegistryHive->ReadOnly)
    {
        /* Release hive bitmap */
        if (RegistryHive->DirtyVector.Buffer)
        {
            RegistryHive->Free(RegistryHive->DirtyVector.Buffer, 0);
        }

        HvpFreeHiveBins(RegistryHive);

        /* Free the BaseBlock */
        if (RegistryHive->BaseBlock)
        {
            RegistryHive->Free(RegistryHive->BaseBlock, RegistryHive->BaseBlockAlloc);
            RegistryHive->BaseBlock = NULL;
        }
    }
}

/* EOF */
