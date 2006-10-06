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
   PHBASE_BLOCK HiveHeader)
{
   if (HiveHeader->Signature != HV_SIGNATURE ||
       HiveHeader->Major != HV_MAJOR_VER ||
       HiveHeader->Minor < HV_MINOR_VER ||
       HiveHeader->Type != HV_TYPE_PRIMARY ||
       HiveHeader->Format != HV_FORMAT_MEMORY ||
       HiveHeader->Cluster != 1 ||
       HiveHeader->Sequence1 != HiveHeader->Sequence2 ||
       HvpHiveHeaderChecksum(HiveHeader) != HiveHeader->Checksum)
   {
      DPRINT1("Verify Hive Header failed: \n");
      DPRINT1("    Signature: 0x%x and not 0x%x, Major: 0x%x and not 0x%x\n",
          HiveHeader->Signature, HV_SIGNATURE, HiveHeader->Major, HV_MAJOR_VER);
      DPRINT1("    Minor: 0x%x is not >= 0x%x, Type: 0x%x and not 0x%x\n",
          HiveHeader->Minor, HV_MINOR_VER, HiveHeader->Type, HV_TYPE_PRIMARY);
      DPRINT1("    Format: 0x%x and not 0x%x, Cluster: 0x%x and not 1\n",
          HiveHeader->Format, HV_FORMAT_MEMORY, HiveHeader->Cluster);
      DPRINT1("    Sequence: 0x%x and not 0x%x, Checksum: 0x%x and not 0x%x\n",
          HiveHeader->Sequence1, HiveHeader->Sequence2,
          HvpHiveHeaderChecksum(HiveHeader), HiveHeader->Checksum);
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

   for (Storage = HvStable; Storage < HvMaxStorageType; Storage++)
   {
      Bin = NULL;
      for (i = 0; i < Hive->Storage[Storage].Length; i++)
      {
         if (Hive->Storage[Storage].BlockList[i].Bin == (ULONG_PTR)NULL)
            continue;
         if (Hive->Storage[Storage].BlockList[i].Bin != (ULONG_PTR)Bin)
         {
            Bin = (PHBIN)Hive->Storage[Storage].BlockList[i].Bin;
            Hive->Free((PHBIN)Hive->Storage[Storage].BlockList[i].Bin);
         }
         Hive->Storage[Storage].BlockList[i].Bin = (ULONG_PTR)NULL;
         Hive->Storage[Storage].BlockList[i].Block = (ULONG_PTR)NULL;
      }

      if (Hive->Storage[Storage].Length)
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
   PHHIVE RegistryHive)
{
   PHBASE_BLOCK HiveHeader;
   ULONG Index;

   HiveHeader = RegistryHive->Allocate(sizeof(HBASE_BLOCK), FALSE);
   if (HiveHeader == NULL)
      return STATUS_NO_MEMORY;
   RtlZeroMemory(HiveHeader, sizeof(HBASE_BLOCK));
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
      RegistryHive->Storage[HvStable].FreeDisplay[Index] = HCELL_NULL;
      RegistryHive->Storage[HvVolatile].FreeDisplay[Index] = HCELL_NULL;
   }
   RtlInitializeBitMap(&RegistryHive->DirtyVector, NULL, 0);

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
   PHHIVE Hive,
   ULONG_PTR ChunkBase)
{
   SIZE_T BlockIndex;
   PHBIN Bin, NewBin;
   ULONG i;
   ULONG BitmapSize;
   PULONG BitmapBuffer;
   SIZE_T ChunkSize;

   //
   // This hack is similar in magnitude to the US's National Debt
   //
   ChunkSize = ((PHBASE_BLOCK)ChunkBase)->Length;
   ((PHBASE_BLOCK)ChunkBase)->Length = HV_BLOCK_SIZE;
   DPRINT("ChunkSize: %lx\n", ChunkSize);

   if (ChunkSize < sizeof(HBASE_BLOCK) ||
       !HvpVerifyHiveHeader((PHBASE_BLOCK)ChunkBase))
   {
      DPRINT1("Registry is corrupt: ChunkSize %d < sizeof(HBASE_BLOCK) %d, "
          "or HvpVerifyHiveHeader() failed\n", ChunkSize, sizeof(HBASE_BLOCK));
      return STATUS_REGISTRY_CORRUPT;
   }

   Hive->HiveHeader = Hive->Allocate(sizeof(HBASE_BLOCK), FALSE);
   if (Hive->HiveHeader == NULL)
   {
      return STATUS_NO_MEMORY;
   }
   RtlCopyMemory(Hive->HiveHeader, (PVOID)ChunkBase, sizeof(HBASE_BLOCK));

   /*
    * Build a block list from the in-memory chunk and copy the data as
    * we go.
    */
   
   Hive->Storage[HvStable].Length = (ULONG)(ChunkSize / HV_BLOCK_SIZE) - 1;
   Hive->Storage[HvStable].BlockList =
      Hive->Allocate(Hive->Storage[HvStable].Length *
                     sizeof(HMAP_ENTRY), FALSE);
   if (Hive->Storage[HvStable].BlockList == NULL)
   {
      DPRINT1("Allocating block list failed\n");
      Hive->Free(Hive->HiveHeader);
      return STATUS_NO_MEMORY;
   }

   for (BlockIndex = 0; BlockIndex < Hive->Storage[HvStable].Length; )
   {
      Bin = (PHBIN)((ULONG_PTR)ChunkBase + (BlockIndex + 1) * HV_BLOCK_SIZE);
      if (Bin->Signature != HV_BIN_SIGNATURE ||
          (Bin->Size % HV_BLOCK_SIZE) != 0)
      {
         Hive->Free(Hive->HiveHeader);
         Hive->Free(Hive->Storage[HvStable].BlockList);
         return STATUS_REGISTRY_CORRUPT;
      }

      NewBin = Hive->Allocate(Bin->Size, TRUE);
      if (NewBin == NULL)
      {
         Hive->Free(Hive->HiveHeader);
         Hive->Free(Hive->Storage[HvStable].BlockList);
         return STATUS_NO_MEMORY;
      }

      Hive->Storage[HvStable].BlockList[BlockIndex].Bin = (ULONG_PTR)NewBin;
      Hive->Storage[HvStable].BlockList[BlockIndex].Block = (ULONG_PTR)NewBin;

      RtlCopyMemory(NewBin, Bin, Bin->Size);

      if (Bin->Size > HV_BLOCK_SIZE)
      {
         for (i = 1; i < Bin->Size / HV_BLOCK_SIZE; i++)
         {
            Hive->Storage[HvStable].BlockList[BlockIndex + i].Bin = (ULONG_PTR)NewBin;
            Hive->Storage[HvStable].BlockList[BlockIndex + i].Block =
               ((ULONG_PTR)NewBin + (i * HV_BLOCK_SIZE));
         }
      }

      BlockIndex += Bin->Size / HV_BLOCK_SIZE;
   }

   if (HvpCreateHiveFreeCellList(Hive))
   {
      HvpFreeHiveBins(Hive);
      Hive->Free(Hive->HiveHeader);
      return STATUS_NO_MEMORY;
   }

   BitmapSize = ROUND_UP(Hive->Storage[HvStable].Length,
                         sizeof(ULONG) * 8) / 8;
   BitmapBuffer = (PULONG)Hive->Allocate(BitmapSize, TRUE);
   if (BitmapBuffer == NULL)
   {
      HvpFreeHiveBins(Hive);
      Hive->Free(Hive->HiveHeader);
      return STATUS_NO_MEMORY;
   }

   RtlInitializeBitMap(&Hive->DirtyVector, BitmapBuffer, BitmapSize * 8);
   RtlClearAllBits(&Hive->DirtyVector);

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
   PHHIVE Hive,
   ULONG_PTR ChunkBase)
{
   if (!HvpVerifyHiveHeader((PHBASE_BLOCK)ChunkBase))
   {
      return STATUS_REGISTRY_CORRUPT;
   }

   Hive->HiveHeader = (PHBASE_BLOCK)ChunkBase;
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
   PHHIVE RegistryHive,
   ULONG Operation,
   ULONG HiveType,
   ULONG HiveFlags,
   ULONG_PTR HiveData OPTIONAL,
   ULONG Cluster OPTIONAL,
   PALLOCATE_ROUTINE Allocate,
   PFREE_ROUTINE Free,
   PFILE_READ_ROUTINE FileRead,
   PFILE_WRITE_ROUTINE FileWrite,
   PFILE_SET_SIZE_ROUTINE FileSetSize,
   PFILE_FLUSH_ROUTINE FileFlush,
   IN PUNICODE_STRING FileName)
{
   NTSTATUS Status;
   PHHIVE Hive = RegistryHive;

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

   switch (Operation)
   {
      case HV_OPERATION_CREATE_HIVE:
         Status = HvpCreateHive(Hive);
         break;

      case HV_OPERATION_MEMORY:
         Status = HvpInitializeMemoryHive(Hive, HiveData);
         break;

      case HV_OPERATION_MEMORY_INPLACE:
         Status = HvpInitializeMemoryInplaceHive(Hive, HiveData);
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

   return Status;
}

/**
 * @name HvFree
 *
 * Free all stroage and handles associated with hive descriptor.
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
         RegistryHive->Free(RegistryHive->DirtyVector.Buffer);
      }

      HvpFreeHiveBins(RegistryHive);
   }

   RegistryHive->Free(RegistryHive);
}

/* EOF */
