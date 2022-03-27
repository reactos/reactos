/*
 * PROJECT:   Registry manipulation library
 * LICENSE:   GPL - See COPYING in the top level directory
 * COPYRIGHT: Copyright 2005 Filip Navara <navaraf@reactos.org>
 *            Copyright 2001 - 2005 Eric Kohl
 */

#include "cmlib.h"
#define NDEBUG
#include <debug.h>

/**
 * @name HvpVerifyHiveHeader
 *
 * Internal function to verify that a hive header has valid format.
 */
BOOLEAN CMAPI
HvpVerifyHiveHeader(
    IN PHBASE_BLOCK BaseBlock)
{
    if (BaseBlock->Signature != HV_HBLOCK_SIGNATURE ||
        BaseBlock->Major != HSYS_MAJOR ||
        BaseBlock->Minor < HSYS_MINOR ||
        BaseBlock->Type != HFILE_TYPE_PRIMARY ||
        BaseBlock->Format != HBASE_FORMAT_MEMORY ||
        BaseBlock->Cluster != 1 ||
        BaseBlock->Sequence1 != BaseBlock->Sequence2 ||
        HvpHiveHeaderChecksum(BaseBlock) != BaseBlock->CheckSum)
    {
        DPRINT1("Verify Hive Header failed:\n");
        DPRINT1("    Signature: 0x%x, expected 0x%x; Major: 0x%x, expected 0x%x\n",
                BaseBlock->Signature, HV_HBLOCK_SIGNATURE, BaseBlock->Major, HSYS_MAJOR);
        DPRINT1("    Minor: 0x%x expected to be >= 0x%x; Type: 0x%x, expected 0x%x\n",
                BaseBlock->Minor, HSYS_MINOR, BaseBlock->Type, HFILE_TYPE_PRIMARY);
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
 * @name HvpFreeHiveBins
 *
 * Internal function to free all bin storage associated with a hive descriptor.
 */
VOID CMAPI
HvpFreeHiveBins(
    PHHIVE Hive)
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
 * @name HvpAllocBaseBlockAligned
 *
 * Internal helper function to allocate cluster-aligned hive base blocks.
 */
static __inline PHBASE_BLOCK
HvpAllocBaseBlockAligned(
    IN PHHIVE Hive,
    IN BOOLEAN Paged,
    IN ULONG Tag)
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
 * @name HvpInitFileName
 *
 * Internal function to initialize the UNICODE NULL-terminated hive file name
 * member of a hive header by copying the last 31 characters of the file name.
 * Mainly used for debugging purposes.
 */
static VOID
HvpInitFileName(
    IN OUT PHBASE_BLOCK BaseBlock,
    IN PCUNICODE_STRING FileName OPTIONAL)
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
 * @name HvpCreateHive
 *
 * Internal helper function to initialize a hive descriptor structure
 * for a newly created hive in memory.
 *
 * @see HvInitialize
 */
NTSTATUS CMAPI
HvpCreateHive(
    IN OUT PHHIVE RegistryHive,
    IN PCUNICODE_STRING FileName OPTIONAL)
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
    BaseBlock->BootType = 0;

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
 * @name HvpInitializeMemoryHive
 *
 * Internal helper function to initialize hive descriptor structure for
 * an existing hive stored in memory. The data of the hive is copied
 * and it is prepared for read/write access.
 *
 * @see HvInitialize
 */
NTSTATUS CMAPI
HvpInitializeMemoryHive(
    PHHIVE Hive,
    PHBASE_BLOCK ChunkBase,
    IN PCUNICODE_STRING FileName OPTIONAL)
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
        !HvpVerifyHiveHeader(ChunkBase))
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
           (Bin->Size % HBLOCK_SIZE) != 0)
        {
            DPRINT1("Invalid bin at BlockIndex %lu, Signature 0x%x, Size 0x%x\n",
                    (unsigned long)BlockIndex, (unsigned)Bin->Signature, (unsigned)Bin->Size);
            Hive->Free(Hive->Storage[Stable].BlockList, 0);
            Hive->Free(Hive->BaseBlock, Hive->BaseBlockAlloc);
            return STATUS_REGISTRY_CORRUPT;
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

    if (HvpCreateHiveFreeCellList(Hive))
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

    HvpInitFileName(Hive->BaseBlock, FileName);

    return STATUS_SUCCESS;
}

/**
 * @name HvpInitializeFlatHive
 *
 * Internal helper function to initialize hive descriptor structure for
 * a hive stored in memory. The in-memory data of the hive are directly
 * used and it is read-only accessible.
 *
 * @see HvInitialize
 */
NTSTATUS CMAPI
HvpInitializeFlatHive(
    PHHIVE Hive,
    PHBASE_BLOCK ChunkBase)
{
    if (!HvpVerifyHiveHeader(ChunkBase))
        return STATUS_REGISTRY_CORRUPT;

    /* Setup hive data */
    Hive->BaseBlock = ChunkBase;
    Hive->Version = ChunkBase->Minor;
    Hive->Flat = TRUE;
    Hive->ReadOnly = TRUE;

    Hive->StorageTypeCount = 1;

    /* Set default boot type */
    ChunkBase->BootType = 0;

    return STATUS_SUCCESS;
}

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

RESULT CMAPI
HvpGetHiveHeader(IN PHHIVE Hive,
                 IN PHBASE_BLOCK *HiveBaseBlock,
                 IN PLARGE_INTEGER TimeStamp)
{
    PHBASE_BLOCK BaseBlock;
    ULONG Result;
    ULONG Offset = 0;

    ASSERT(sizeof(HBASE_BLOCK) >= (HSECTOR_SIZE * Hive->Cluster));

    /* Assume failure and allocate the base block */
    *HiveBaseBlock = NULL;
    BaseBlock = HvpAllocBaseBlockAligned(Hive, TRUE, TAG_CM);
    if (!BaseBlock) return NoMemory;

    /* Clear it */
    RtlZeroMemory(BaseBlock, sizeof(HBASE_BLOCK));

    /* Now read it from disk */
    Result = Hive->FileRead(Hive,
                            HFILE_TYPE_PRIMARY,
                            &Offset,
                            BaseBlock,
                            Hive->Cluster * HSECTOR_SIZE);

    /* Couldn't read: assume it's not a hive */
    if (!Result) return NotHive;

    /* Do validation */
    if (!HvpVerifyHiveHeader(BaseBlock)) return NotHive;

    /* Return information */
    *HiveBaseBlock = BaseBlock;
    *TimeStamp = BaseBlock->TimeStamp;
    return HiveSuccess;
}

NTSTATUS CMAPI
HvLoadHive(IN PHHIVE Hive,
           IN PCUNICODE_STRING FileName OPTIONAL)
{
    NTSTATUS Status;
    PHBASE_BLOCK BaseBlock = NULL;
    ULONG Result;
    LARGE_INTEGER TimeStamp;
    ULONG Offset = 0;
    PVOID HiveData;
    ULONG FileSize;

    /* Get the hive header */
    Result = HvpGetHiveHeader(Hive, &BaseBlock, &TimeStamp);
    switch (Result)
    {
        /* Out of memory */
        case NoMemory:

            /* Fail */
            return STATUS_INSUFFICIENT_RESOURCES;

        /* Not a hive */
        case NotHive:

            /* Fail */
            return STATUS_NOT_REGISTRY_FILE;

        /* Has recovery data */
        case RecoverData:
        case RecoverHeader:

            /* Fail */
            return STATUS_REGISTRY_CORRUPT;
    }

    /* Set default boot type */
    BaseBlock->BootType = 0;

    /* Setup hive data */
    Hive->BaseBlock = BaseBlock;
    Hive->Version = BaseBlock->Minor;

    /* Allocate a buffer large enough to hold the hive */
    FileSize = HBLOCK_SIZE + BaseBlock->Length; // == sizeof(HBASE_BLOCK) + BaseBlock->Length;
    HiveData = Hive->Allocate(FileSize, TRUE, TAG_CM);
    if (!HiveData)
    {
        Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    /* Now read the whole hive */
    Result = Hive->FileRead(Hive,
                            HFILE_TYPE_PRIMARY,
                            &Offset,
                            HiveData,
                            FileSize);
    if (!Result)
    {
        Hive->Free(HiveData, FileSize);
        Hive->Free(BaseBlock, Hive->BaseBlockAlloc);
        return STATUS_NOT_REGISTRY_FILE;
    }

    // This is a HACK!
    /* Free our base block... it's usless in this implementation */
    Hive->Free(BaseBlock, Hive->BaseBlockAlloc);

    /* Initialize the hive directly from memory */
    Status = HvpInitializeMemoryHive(Hive, HiveData, FileName);
    if (!NT_SUCCESS(Status))
        Hive->Free(HiveData, FileSize);

    return Status;
}

/**
 * @name HvInitialize
 *
 * Allocate a new hive descriptor structure and intialize it.
 *
 * @param RegistryHive
 *        Output variable to store pointer to the hive descriptor.
 * @param OperationType
 *        - HV_OPERATION_CREATE_HIVE
 *          Create a new hive for read/write access.
 *        - HV_OPERATION_MEMORY
 *          Load and copy in-memory hive for read/write access. The
 *          pointer to data passed to this routine can be freed after
 *          the function is executed.
 *        - HV_OPERATION_MEMORY_INPLACE
 *          Load an in-memory hive for read-only access. The pointer
 *          to data passed to this routine MUSTN'T be freed until
 *          HvFree is called.
 * @param ChunkBase
 *        Pointer to hive data.
 * @param ChunkSize
 *        Size of passed hive data.
 *
 * @return
 *    STATUS_NO_MEMORY - A memory allocation failed.
 *    STATUS_REGISTRY_CORRUPT - Registry corruption was detected.
 *    STATUS_SUCCESS
 *
 * @see HvFree
 */
NTSTATUS CMAPI
HvInitialize(
    PHHIVE RegistryHive,
    ULONG OperationType,
    ULONG HiveFlags,
    ULONG FileType,
    PVOID HiveData OPTIONAL,
    PALLOCATE_ROUTINE Allocate,
    PFREE_ROUTINE Free,
    PFILE_SET_SIZE_ROUTINE FileSetSize,
    PFILE_WRITE_ROUTINE FileWrite,
    PFILE_READ_ROUTINE FileRead,
    PFILE_FLUSH_ROUTINE FileFlush,
    ULONG Cluster OPTIONAL,
    PCUNICODE_STRING FileName OPTIONAL)
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
#endif
    Hive->HiveFlags = HiveFlags & ~HIVE_NOLAZYFLUSH;

    // TODO: The CellRoutines point to different callbacks
    // depending on the OperationType.
    Hive->GetCellRoutine = HvpGetCellData;
    Hive->ReleaseCellRoutine = NULL;

    switch (OperationType)
    {
        case HINIT_CREATE:
            Status = HvpCreateHive(Hive, FileName);
            break;

        case HINIT_MEMORY:
            Status = HvpInitializeMemoryHive(Hive, HiveData, FileName);
            break;

        case HINIT_FLAT:
            Status = HvpInitializeFlatHive(Hive, HiveData);
            break;

        case HINIT_FILE:
        {
            Status = HvLoadHive(Hive, FileName);
            if ((Status != STATUS_SUCCESS) &&
                (Status != STATUS_REGISTRY_RECOVERED))
            {
                /* Unrecoverable failure */
                return Status;
            }

            /* Check for previous damage */
            ASSERT(Status != STATUS_REGISTRY_RECOVERED);
            break;
        }

        case HINIT_MEMORY_INPLACE:
            // Status = HvpInitializeMemoryInplaceHive(Hive, HiveData);
            // break;

        case HINIT_MAPFILE:

        default:
        /* FIXME: A better return status value is needed */
        Status = STATUS_NOT_IMPLEMENTED;
        ASSERT(FALSE);
    }

    if (!NT_SUCCESS(Status)) return Status;

    /* HACK: ROS: Init root key cell and prepare the hive */
    // r31253
    // if (OperationType == HINIT_CREATE) CmCreateRootNode(Hive, L"");
    if (OperationType != HINIT_CREATE) CmPrepareHive(Hive);

    return Status;
}

/**
 * @name HvFree
 *
 * Free all stroage and handles associated with hive descriptor.
 * But do not free the hive descriptor itself.
 */
VOID CMAPI
HvFree(
    PHHIVE RegistryHive)
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
