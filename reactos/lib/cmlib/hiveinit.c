/*
 * PROJECT:   registry manipulation library
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
   PHBASE_BLOCK BaseBlock)
{
   if (BaseBlock->Signature != HV_SIGNATURE ||
       BaseBlock->Major != HSYS_MAJOR ||
       BaseBlock->Minor < HSYS_MINOR ||
       BaseBlock->Type != HFILE_TYPE_PRIMARY ||
       BaseBlock->Format != HBASE_FORMAT_MEMORY ||
       BaseBlock->Cluster != 1 ||
       BaseBlock->Sequence1 != BaseBlock->Sequence2 ||
       HvpHiveHeaderChecksum(BaseBlock) != BaseBlock->CheckSum)
   {
      DPRINT1("Verify Hive Header failed: \n");
      DPRINT1("    Signature: 0x%x, expected 0x%x; Major: 0x%x, expected 0x%x\n",
          BaseBlock->Signature, HV_SIGNATURE, BaseBlock->Major, HSYS_MAJOR);
      DPRINT1("    Minor: 0x%x is not >= 0x%x; Type: 0x%x, expected 0x%x\n",
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
 * Internal function to free all bin storage associated with hive
 * descriptor.
 */

VOID CMAPI
HvpFreeHiveBins(
   PHHIVE Hive)
{
   ULONG i;
   PHBIN Bin;
   ULONG Storage;

   for (Storage = Stable; Storage < HTYPE_COUNT; Storage++)
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
 * @name HvpCreateHive
 *
 * Internal helper function to initialize hive descriptor structure for
 * newly created hive.
 *
 * @see HvInitialize
 */

NTSTATUS CMAPI
HvpCreateHive(
   PHHIVE RegistryHive,
   PCUNICODE_STRING FileName OPTIONAL)
{
   PHBASE_BLOCK BaseBlock;
   ULONG Index;

   BaseBlock = RegistryHive->Allocate(sizeof(HBASE_BLOCK), FALSE, TAG_CM);
   if (BaseBlock == NULL)
      return STATUS_NO_MEMORY;

   RtlZeroMemory(BaseBlock, sizeof(HBASE_BLOCK));

   BaseBlock->Signature = HV_SIGNATURE;
   BaseBlock->Major = HSYS_MAJOR;
   BaseBlock->Minor = HSYS_MINOR;
   BaseBlock->Type = HFILE_TYPE_PRIMARY;
   BaseBlock->Format = HBASE_FORMAT_MEMORY;
   BaseBlock->Cluster = 1;
   BaseBlock->RootCell = HCELL_NIL;
   BaseBlock->Length = 0;
   BaseBlock->Sequence1 = 1;
   BaseBlock->Sequence2 = 1;

   /* Copy the 31 last characters of the hive file name if any */
   if (FileName)
   {
      if (FileName->Length / sizeof(WCHAR) <= HIVE_FILENAME_MAXLEN)
      {
         RtlCopyMemory(BaseBlock->FileName,
                       FileName->Buffer,
                       FileName->Length);
      }
      else
      {
         RtlCopyMemory(BaseBlock->FileName,
                       FileName->Buffer +
                       FileName->Length / sizeof(WCHAR) - HIVE_FILENAME_MAXLEN,
                       HIVE_FILENAME_MAXLEN * sizeof(WCHAR));
      }

      /* NULL-terminate */
      BaseBlock->FileName[HIVE_FILENAME_MAXLEN] = L'\0';
   }

   BaseBlock->CheckSum = HvpHiveHeaderChecksum(BaseBlock);

   RegistryHive->BaseBlock = BaseBlock;
   for (Index = 0; Index < 24; Index++)
   {
      RegistryHive->Storage[Stable].FreeDisplay[Index] = HCELL_NIL;
      RegistryHive->Storage[Volatile].FreeDisplay[Index] = HCELL_NIL;
   }

   return STATUS_SUCCESS;
}

/**
 * @name HvpInitializeMemoryHive
 *
 * Internal helper function to initialize hive descriptor structure for
 * a hive stored in memory. The data of the hive are copied and it is
 * prepared for read/write access.
 *
 * @see HvInitialize
 */

NTSTATUS CMAPI
HvpInitializeMemoryHive(
   PHHIVE Hive,
   PVOID ChunkBase)
{
   SIZE_T BlockIndex;
   PHBIN Bin, NewBin;
   ULONG i;
   ULONG BitmapSize;
   PULONG BitmapBuffer;
   SIZE_T ChunkSize;

   ChunkSize = ((PHBASE_BLOCK)ChunkBase)->Length;
   DPRINT("ChunkSize: %lx\n", ChunkSize);

   if (ChunkSize < sizeof(HBASE_BLOCK) ||
       !HvpVerifyHiveHeader((PHBASE_BLOCK)ChunkBase))
   {
      DPRINT1("Registry is corrupt: ChunkSize %lu < sizeof(HBASE_BLOCK) %lu, "
          "or HvpVerifyHiveHeader() failed\n", ChunkSize, (SIZE_T)sizeof(HBASE_BLOCK));
      return STATUS_REGISTRY_CORRUPT;
   }

   Hive->BaseBlock = Hive->Allocate(sizeof(HBASE_BLOCK), FALSE, TAG_CM);
   if (Hive->BaseBlock == NULL)
   {
      return STATUS_NO_MEMORY;
   }
   RtlCopyMemory(Hive->BaseBlock, ChunkBase, sizeof(HBASE_BLOCK));

   /*
    * Build a block list from the in-memory chunk and copy the data as
    * we go.
    */

   Hive->Storage[Stable].Length = (ULONG)(ChunkSize / HV_BLOCK_SIZE);
   Hive->Storage[Stable].BlockList =
      Hive->Allocate(Hive->Storage[Stable].Length *
                     sizeof(HMAP_ENTRY), FALSE, TAG_CM);
   if (Hive->Storage[Stable].BlockList == NULL)
   {
      DPRINT1("Allocating block list failed\n");
      Hive->Free(Hive->BaseBlock, 0);
      return STATUS_NO_MEMORY;
   }

   for (BlockIndex = 0; BlockIndex < Hive->Storage[Stable].Length; )
   {
      Bin = (PHBIN)((ULONG_PTR)ChunkBase + (BlockIndex + 1) * HV_BLOCK_SIZE);
      if (Bin->Signature != HV_BIN_SIGNATURE ||
          (Bin->Size % HV_BLOCK_SIZE) != 0)
      {
         DPRINT1("Invalid bin at BlockIndex %lu, Signature 0x%x, Size 0x%x\n",
                 (unsigned long)BlockIndex, (unsigned)Bin->Signature, (unsigned)Bin->Size);
         Hive->Free(Hive->BaseBlock, 0);
         Hive->Free(Hive->Storage[Stable].BlockList, 0);
         return STATUS_REGISTRY_CORRUPT;
      }

      NewBin = Hive->Allocate(Bin->Size, TRUE, TAG_CM);
      if (NewBin == NULL)
      {
         Hive->Free(Hive->BaseBlock, 0);
         Hive->Free(Hive->Storage[Stable].BlockList, 0);
         return STATUS_NO_MEMORY;
      }

      Hive->Storage[Stable].BlockList[BlockIndex].BinAddress = (ULONG_PTR)NewBin;
      Hive->Storage[Stable].BlockList[BlockIndex].BlockAddress = (ULONG_PTR)NewBin;

      RtlCopyMemory(NewBin, Bin, Bin->Size);

      if (Bin->Size > HV_BLOCK_SIZE)
      {
         for (i = 1; i < Bin->Size / HV_BLOCK_SIZE; i++)
         {
            Hive->Storage[Stable].BlockList[BlockIndex + i].BinAddress = (ULONG_PTR)NewBin;
            Hive->Storage[Stable].BlockList[BlockIndex + i].BlockAddress =
               ((ULONG_PTR)NewBin + (i * HV_BLOCK_SIZE));
         }
      }

      BlockIndex += Bin->Size / HV_BLOCK_SIZE;
   }

   if (HvpCreateHiveFreeCellList(Hive))
   {
      HvpFreeHiveBins(Hive);
      Hive->Free(Hive->BaseBlock, 0);
      return STATUS_NO_MEMORY;
   }

   BitmapSize = ROUND_UP(Hive->Storage[Stable].Length,
                         sizeof(ULONG) * 8) / 8;
   BitmapBuffer = (PULONG)Hive->Allocate(BitmapSize, TRUE, TAG_CM);
   if (BitmapBuffer == NULL)
   {
      HvpFreeHiveBins(Hive);
      Hive->Free(Hive->BaseBlock, 0);
      return STATUS_NO_MEMORY;
   }

   RtlInitializeBitMap(&Hive->DirtyVector, BitmapBuffer, BitmapSize * 8);
   RtlClearAllBits(&Hive->DirtyVector);

   return STATUS_SUCCESS;
}

/**
 * @name HvpInitializeMemoryInplaceHive
 *
 * Internal helper function to initialize hive descriptor structure for
 * a hive stored in memory. The in-memory data of the hive are directly
 * used and it is read-only accessible.
 *
 * @see HvInitialize
 */

NTSTATUS CMAPI
HvpInitializeMemoryInplaceHive(
   PHHIVE Hive,
   PVOID ChunkBase)
{
   if (!HvpVerifyHiveHeader((PHBASE_BLOCK)ChunkBase))
   {
      return STATUS_REGISTRY_CORRUPT;
   }

   Hive->BaseBlock = (PHBASE_BLOCK)ChunkBase;
   Hive->ReadOnly = TRUE;
   Hive->Flat = TRUE;

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
    ULONG Alignment;
    ULONG Result;
    ULONG Offset = 0;
    ASSERT(sizeof(HBASE_BLOCK) >= (HV_BLOCK_SIZE * Hive->Cluster));

    /* Assume failure and allocate the buffer */
    *HiveBaseBlock = 0;
    BaseBlock = Hive->Allocate(sizeof(HBASE_BLOCK), TRUE, TAG_CM);
    if (!BaseBlock) return NoMemory;

    /* Check for, and enforce, alignment */
    Alignment = Hive->Cluster * HV_BLOCK_SIZE -1;
    if ((ULONG_PTR)BaseBlock & Alignment)
    {
        /* Free the old header */
        Hive->Free(BaseBlock, 0);
        BaseBlock = Hive->Allocate(PAGE_SIZE, TRUE, TAG_CM);
        if (!BaseBlock) return NoMemory;
    }

    /* Clear it */
    RtlZeroMemory(BaseBlock, sizeof(HBASE_BLOCK));

    /* Now read it from disk */
    Result = Hive->FileRead(Hive,
                            HFILE_TYPE_PRIMARY,
                            &Offset,
                            BaseBlock,
                            Hive->Cluster * HV_BLOCK_SIZE);

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
           IN ULONG FileSize)
{
    PHBASE_BLOCK BaseBlock = NULL;
    ULONG Result;
    LARGE_INTEGER TimeStamp;
    ULONG Offset = 0;
    PVOID HiveData;

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
    Hive->Version = Hive->BaseBlock->Minor;

    /* Allocate a buffer large enough to hold the hive */
    HiveData = Hive->Allocate(FileSize, TRUE, TAG_CM);
    if (!HiveData) return STATUS_INSUFFICIENT_RESOURCES;

    /* Now read the whole hive */
    Result = Hive->FileRead(Hive,
                            HFILE_TYPE_PRIMARY,
                            &Offset,
                            HiveData,
                            FileSize);
    if (!Result) return STATUS_NOT_REGISTRY_FILE;


    /* Free our base block... it's usless in this implementation */
    Hive->Free(BaseBlock, 0);

    /* Initialize the hive directly from memory */
    return HvpInitializeMemoryHive(Hive, HiveData);
}

/**
 * @name HvInitialize
 *
 * Allocate a new hive descriptor structure and intialize it.
 *
 * @param RegistryHive
 *        Output variable to store pointer to the hive descriptor.
 * @param Operation
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
   ULONG Operation,
   ULONG HiveType,
   ULONG HiveFlags,
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

   UNREFERENCED_PARAMETER(HiveType);

   /*
    * Create a new hive structure that will hold all the maintenance data.
    */

   RtlZeroMemory(Hive, sizeof(HHIVE));

   Hive->Allocate = Allocate;
   Hive->Free = Free;
   Hive->FileRead = FileRead;
   Hive->FileWrite = FileWrite;
   Hive->FileSetSize = FileSetSize;
   Hive->FileFlush = FileFlush;
   Hive->StorageTypeCount = HTYPE_COUNT;
   Hive->Cluster = 1;
   Hive->Version = HSYS_MINOR;
   Hive->HiveFlags = HiveFlags &~ HIVE_NOLAZYFLUSH;

   switch (Operation)
   {
      case HINIT_CREATE:
         Status = HvpCreateHive(Hive, FileName);
         break;

      case HINIT_MEMORY:
         Status = HvpInitializeMemoryHive(Hive, HiveData);
         break;

      case HINIT_FLAT:
         Status = HvpInitializeMemoryInplaceHive(Hive, HiveData);
         break;

      case HINIT_FILE:
      {
         /* HACK of doom: Cluster is actually the file size. */
         Status = HvLoadHive(Hive, Cluster);
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

      default:
         /* FIXME: A better return status value is needed */
         Status = STATUS_NOT_IMPLEMENTED;
         ASSERT(FALSE);
   }

   if (!NT_SUCCESS(Status)) return Status;

   if (Operation != HINIT_CREATE) CmPrepareHive(Hive);

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
         RegistryHive->Free(RegistryHive->BaseBlock, 0);
         RegistryHive->BaseBlock = NULL;
      }
   }
}

/* EOF */
