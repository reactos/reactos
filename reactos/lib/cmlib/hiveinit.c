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
   PHIVE_HEADER HiveHeader)
{
   if (HiveHeader->Signature != HV_SIGNATURE ||
       HiveHeader->Major != HV_MAJOR_VER ||
       HiveHeader->Minor > HV_MINOR_VER ||
       HiveHeader->Type != HV_TYPE_PRIMARY ||
       HiveHeader->Format != HV_FORMAT_MEMORY ||
       HiveHeader->Cluster != 1 ||
       HiveHeader->Sequence1 != HiveHeader->Sequence2 ||
       HvpHiveHeaderChecksum(HiveHeader) != HiveHeader->Checksum)
   {
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
   PREGISTRY_HIVE Hive)
{        	
   ULONG i;
   PHBIN Bin;
   ULONG Storage;

   for (Storage = HvStable; Storage < HvMaxStorageType; Storage++)
   {
      Bin = NULL;
      for (i = 0; i < Hive->Storage[Storage].BlockListSize; i++)
      {
         if (Hive->Storage[Storage].BlockList[i].Bin == NULL)
            continue;
         if (Hive->Storage[Storage].BlockList[i].Bin != Bin)
         {
            Bin = Hive->Storage[Storage].BlockList[i].Bin;
            Hive->Free(Hive->Storage[Storage].BlockList[i].Bin);
         }
         Hive->Storage[Storage].BlockList[i].Bin = NULL;
         Hive->Storage[Storage].BlockList[i].Block = NULL;
      }

      if (Hive->Storage[Storage].BlockListSize)
         Hive->Free(Hive->Storage[Storage].BlockList);
   }
}

/**
 * @name HvpCreateHive
 *
 * Internal helper function to initalize hive descriptor structure for
 * newly created hive.
 *
 * @see HvInitialize
 */

NTSTATUS CMAPI
HvpCreateHive(
   PREGISTRY_HIVE RegistryHive)
{
   PHIVE_HEADER HiveHeader;
   ULONG Index;

   HiveHeader = RegistryHive->Allocate(sizeof(HIVE_HEADER), FALSE);
   if (HiveHeader == NULL)
      return STATUS_NO_MEMORY;
   RtlZeroMemory(HiveHeader, sizeof(HIVE_HEADER));
   HiveHeader->Signature = HV_SIGNATURE;
   HiveHeader->Major = HV_MAJOR_VER;
   HiveHeader->Minor = HV_MINOR_VER;
   HiveHeader->Type = HV_TYPE_PRIMARY;
   HiveHeader->Format = HV_FORMAT_MEMORY;
   HiveHeader->Cluster = 1;
   HiveHeader->RootCell = HCELL_NULL;
   HiveHeader->Length = HV_BLOCK_SIZE;
   HiveHeader->Sequence1 = 1;
   HiveHeader->Sequence2 = 1;
   /* FIXME: Fill in the file name */
   HiveHeader->Checksum = HvpHiveHeaderChecksum(HiveHeader);

   RegistryHive->HiveHeader = HiveHeader;
   for (Index = 0; Index < 24; Index++)
   {
      RegistryHive->Storage[HvStable].FreeListOffset[Index] = HCELL_NULL;
      RegistryHive->Storage[HvVolatile].FreeListOffset[Index] = HCELL_NULL;
   }
   RtlInitializeBitMap(&RegistryHive->DirtyBitmap, NULL, 0);

   return STATUS_SUCCESS;
}

/**
 * @name HvpInitializeMemoryHive
 *
 * Internal helper function to initalize hive descriptor structure for
 * a hive stored in memory. The data of the hive are copied and it is
 * prepared for read/write access.
 *
 * @see HvInitialize
 */

NTSTATUS CMAPI
HvpInitializeMemoryHive(
   PREGISTRY_HIVE Hive,
   ULONG_PTR ChunkBase,
   SIZE_T ChunkSize)
{
   SIZE_T BlockIndex;
   PHBIN Bin, NewBin;
   ULONG i;
   ULONG BitmapSize;
   PULONG BitmapBuffer;
   
   if (ChunkSize < sizeof(HIVE_HEADER) ||
       !HvpVerifyHiveHeader((PHIVE_HEADER)ChunkBase))
   {
      return STATUS_REGISTRY_CORRUPT;
   }

   Hive->HiveHeader = Hive->Allocate(sizeof(HIVE_HEADER), FALSE);
   if (Hive->HiveHeader == NULL)
   {
      return STATUS_NO_MEMORY;
   }
   RtlCopyMemory(Hive->HiveHeader, (PVOID)ChunkBase, sizeof(HIVE_HEADER));

   /*
    * Build a block list from the in-memory chunk and copy the data as
    * we go.
    */
   
   Hive->Storage[HvStable].BlockListSize = (ChunkSize / HV_BLOCK_SIZE) - 1;
   Hive->Storage[HvStable].BlockList =
      Hive->Allocate(Hive->Storage[HvStable].BlockListSize *
                     sizeof(BLOCK_LIST_ENTRY), FALSE);
   if (Hive->Storage[HvStable].BlockList == NULL)
   {
      DPRINT1("Allocating block list failed\n");
      Hive->Free(Hive->HiveHeader);
      return STATUS_NO_MEMORY;
   }

   for (BlockIndex = 0; BlockIndex < Hive->Storage[HvStable].BlockListSize; )
   {
      Bin = (PHBIN)((ULONG_PTR)ChunkBase + (BlockIndex + 1) * HV_BLOCK_SIZE);
      if (Bin->Signature != HV_BIN_SIGNATURE ||
          (Bin->BinSize % HV_BLOCK_SIZE) != 0)
      {
         Hive->Free(Hive->HiveHeader);
         Hive->Free(Hive->Storage[HvStable].BlockList);
         return STATUS_REGISTRY_CORRUPT;
      }

      NewBin = Hive->Allocate(Bin->BinSize, TRUE);
      if (NewBin == NULL)
      {
         Hive->Free(Hive->HiveHeader);
         Hive->Free(Hive->Storage[HvStable].BlockList);
         return STATUS_NO_MEMORY;
      }

      Hive->Storage[HvStable].BlockList[BlockIndex].Bin = NewBin;
      Hive->Storage[HvStable].BlockList[BlockIndex].Block = NewBin;

      RtlCopyMemory(NewBin, Bin, Bin->BinSize);

      if (Bin->BinSize > HV_BLOCK_SIZE)
      {
         for (i = 1; i < Bin->BinSize / HV_BLOCK_SIZE; i++)
         {
            Hive->Storage[HvStable].BlockList[BlockIndex + i].Bin = NewBin;
            Hive->Storage[HvStable].BlockList[BlockIndex + i].Block =
               (PVOID)((ULONG_PTR)NewBin + (i * HV_BLOCK_SIZE));
         }
      }

      BlockIndex += Bin->BinSize / HV_BLOCK_SIZE;
   }

   if (HvpCreateHiveFreeCellList(Hive))
   {
      HvpFreeHiveBins(Hive);
      Hive->Free(Hive->HiveHeader);
      return STATUS_NO_MEMORY;
   }

   BitmapSize = ROUND_UP(Hive->Storage[HvStable].BlockListSize,
                         sizeof(ULONG) * 8) / 8;
   BitmapBuffer = (PULONG)Hive->Allocate(BitmapSize, TRUE);
   if (BitmapBuffer == NULL)
   {
      HvpFreeHiveBins(Hive);
      Hive->Free(Hive->HiveHeader);
      return STATUS_NO_MEMORY;
   }

   RtlInitializeBitMap(&Hive->DirtyBitmap, BitmapBuffer, BitmapSize * 8);
   RtlClearAllBits(&Hive->DirtyBitmap);

   return STATUS_SUCCESS;
}

/**
 * @name HvpInitializeMemoryInplaceHive
 *
 * Internal helper function to initalize hive descriptor structure for
 * a hive stored in memory. The in-memory data of the hive are directly
 * used and it is read-only accessible.
 *
 * @see HvInitialize
 */

NTSTATUS CMAPI
HvpInitializeMemoryInplaceHive(
   PREGISTRY_HIVE Hive,
   ULONG_PTR ChunkBase,
   SIZE_T ChunkSize)
{
   if (ChunkSize < sizeof(HIVE_HEADER) ||
       !HvpVerifyHiveHeader((PHIVE_HEADER)ChunkBase))
   {
      return STATUS_REGISTRY_CORRUPT;
   }

   Hive->HiveHeader = (PHIVE_HEADER)ChunkBase;
   Hive->ReadOnly = TRUE;
   Hive->Flat = TRUE;

   return STATUS_SUCCESS;
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
   PREGISTRY_HIVE *RegistryHive,
   ULONG Operation,
   ULONG_PTR ChunkBase,
   SIZE_T ChunkSize,
   PHV_ALLOCATE Allocate,
   PHV_FREE Free,
   PHV_FILE_READ FileRead,
   PHV_FILE_WRITE FileWrite,
   PHV_FILE_SET_SIZE FileSetSize,
   PHV_FILE_FLUSH FileFlush,
   PVOID Opaque)
{
   NTSTATUS Status;
   PREGISTRY_HIVE Hive;

   /*
    * Create a new hive structure that will hold all the maintenance data.
    */

   Hive = Allocate(sizeof(REGISTRY_HIVE), TRUE);
   if (Hive == NULL)
      return STATUS_NO_MEMORY;
   RtlZeroMemory(Hive, sizeof(REGISTRY_HIVE));

   Hive->Allocate = Allocate;
   Hive->Free = Free;
   Hive->FileRead = FileRead;
   Hive->FileWrite = FileWrite;
   Hive->FileSetSize = FileSetSize;
   Hive->FileFlush = FileFlush;
   Hive->Opaque = Opaque;

   switch (Operation)
   {
      case HV_OPERATION_CREATE_HIVE:
         Status = HvpCreateHive(Hive);
         break;

      case HV_OPERATION_MEMORY:
         Status = HvpInitializeMemoryHive(Hive, ChunkBase, ChunkSize);
         break;

      case HV_OPERATION_MEMORY_INPLACE:
         Status = HvpInitializeMemoryInplaceHive(Hive, ChunkBase, ChunkSize);
         break;

      default:
         /* FIXME: A better return status value is needed */
         Status = STATUS_NOT_IMPLEMENTED;
         ASSERT(FALSE);
   }

   if (!NT_SUCCESS(Status))
   {
      Hive->Free(Hive);
      return Status;
   }

   *RegistryHive = Hive;
   
   return Status;
}

/**
 * @name HvFree
 *
 * Free all stroage and handles associated with hive descriptor.
 */

VOID CMAPI 
HvFree(
   PREGISTRY_HIVE RegistryHive)
{
   if (!RegistryHive->ReadOnly)
   {
      /* Release hive bitmap */
      if (RegistryHive->DirtyBitmap.Buffer)
      {
         RegistryHive->Free(RegistryHive->DirtyBitmap.Buffer);
      }

      HvpFreeHiveBins(RegistryHive);
   }

   RegistryHive->Free(RegistryHive);
}

/* EOF */
