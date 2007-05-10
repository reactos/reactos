/* $Id$
 *
 * PROJECT:         ReactOS Kernel
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * FILE:            ntoskrnl/cm/import.c
 * PURPOSE:         Registry-Hive import functions
 *
 * PROGRAMMERS:     Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

#include "cm.h"

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, CmImportHardwareHive)
#endif

/* GLOBALS ******************************************************************/

/* FUNCTIONS ****************************************************************/

static BOOLEAN
CmImportBinaryHive (PCHAR ChunkBase,
		    ULONG ChunkSize,
		    ULONG Flags,
		    PEREGISTRY_HIVE *RegistryHive)
{
  PEREGISTRY_HIVE Hive;
  NTSTATUS Status;

  *RegistryHive = NULL;

  /* Create a new hive */
  Hive = ExAllocatePool (NonPagedPool,
			 sizeof(EREGISTRY_HIVE));
  if (Hive == NULL)
    {
      return FALSE;
    }
  RtlZeroMemory (Hive,
		 sizeof(EREGISTRY_HIVE));

  /* Set hive flags */
  Hive->Flags = Flags;

  /* Allocate hive header */
  ((PHBASE_BLOCK)ChunkBase)->Length = ChunkSize;
  Status = HvInitialize(&Hive->Hive, HV_OPERATION_MEMORY, 0, 0,
                        (ULONG_PTR)ChunkBase, 0,
                        CmpAllocate, CmpFree,
                        CmpFileRead, CmpFileWrite, CmpFileSetSize,
                        CmpFileFlush, NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("Opening hive failed (%x)\n", Status);
      ExFreePool (Hive);
      return FALSE;
    }

  CmPrepareHive(&Hive->Hive);

  /* Acquire hive list lock exclusively */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  DPRINT("Adding new hive\n");

  /* Add the new hive to the hive list */
  InsertTailList(&CmiHiveListHead, &Hive->HiveList);

  /* Release hive list lock */
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  *RegistryHive = Hive;

  return TRUE;
}


BOOLEAN
INIT_FUNCTION
CmImportSystemHive(PCHAR ChunkBase,
                   ULONG ChunkSize,
                   OUT PEREGISTRY_HIVE *RegistryHive)
{
  *RegistryHive = NULL;

  /* Import the binary system hive (non-volatile, offset-based, permanent) */
  if (!CmImportBinaryHive (ChunkBase, ChunkSize, 0, RegistryHive))
    {
      return FALSE;
    }

  /* Set the hive filename */
  RtlCreateUnicodeString (&(*RegistryHive)->HiveFileName,
                          SYSTEM_REG_FILE);

  /* Set the log filename */
  RtlCreateUnicodeString (&(*RegistryHive)->LogFileName,
                          SYSTEM_LOG_FILE);

  return TRUE;
}

BOOLEAN
INIT_FUNCTION
CmImportHardwareHive(PCHAR ChunkBase,
                     ULONG ChunkSize,
                     OUT PEREGISTRY_HIVE *RegistryHive)
{
  *RegistryHive = NULL;

  /* Import the binary system hive (volatile, offset-based, permanent) */
  if (!CmImportBinaryHive (ChunkBase, ChunkSize, HIVE_NO_FILE, RegistryHive))
    {
      return FALSE;
    }

  /* Set the hive filename */
  RtlInitUnicodeString (&(*RegistryHive)->HiveFileName,
			NULL);

  /* Set the log filename */
  RtlInitUnicodeString (&(*RegistryHive)->LogFileName,
			NULL);

  return TRUE;
}

/* EOF */
