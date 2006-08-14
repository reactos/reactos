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

static BOOLEAN CmiHardwareHiveImported = FALSE;

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
  Status = HvInitialize(&Hive->Hive, HV_OPERATION_MEMORY,
                        (ULONG_PTR)ChunkBase, ChunkSize,
                        CmpAllocate, CmpFree,
                        CmpFileRead, CmpFileWrite, CmpFileSetSize,
                        CmpFileFlush, Hive, NULL);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("Opening hive failed (%x)\n", Status);
      ExFreePool (Hive);
      return FALSE;
    }

  CmPrepareHive(Hive->Hive);

  /* Acquire hive list lock exclusively */
  KeEnterCriticalRegion();
  ExAcquireResourceExclusiveLite(&CmiRegistryLock, TRUE);

  DPRINT1("Adding new hive\n");

  /* Add the new hive to the hive list */
  InsertTailList(&CmiHiveListHead, &Hive->HiveList);

  /* Release hive list lock */
  ExReleaseResourceLite(&CmiRegistryLock);
  KeLeaveCriticalRegion();

  *RegistryHive = Hive;

  return TRUE;
}


BOOLEAN INIT_FUNCTION
CmImportSystemHive(PCHAR ChunkBase,
		   ULONG ChunkSize)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  PEREGISTRY_HIVE RegistryHive;
  UNICODE_STRING KeyName;
  NTSTATUS Status;

  DPRINT ("CmImportSystemHive() called\n");

  if (strncmp (ChunkBase, "regf", 4))
    {
      DPRINT1 ("Found invalid '%.*s' magic\n", 4, ChunkBase);
      return FALSE;
    }

  DPRINT ("Found '%.*s' magic\n", 4, ChunkBase);

  /* Import the binary system hive (non-volatile, offset-based, permanent) */
  if (!CmImportBinaryHive (ChunkBase, ChunkSize, 0, &RegistryHive))
    {
      DPRINT1 ("CmiImportBinaryHive() failed\n");
      return FALSE;
    }

  /* Attach it to the machine key */
  RtlInitUnicodeString (&KeyName,
			REG_SYSTEM_KEY_NAME);
  InitializeObjectAttributes (&ObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  Status = CmiConnectHive (&ObjectAttributes,
			   RegistryHive);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("CmiConnectHive(%wZ) failed (Status %lx)\n", &KeyName, Status);
      return FALSE;
    }

  /* Set the hive filename */
  RtlCreateUnicodeString (&RegistryHive->HiveFileName,
                          SYSTEM_REG_FILE);

  /* Set the log filename */
  RtlCreateUnicodeString (&RegistryHive->LogFileName,
                          SYSTEM_LOG_FILE);

  return TRUE;
}


BOOLEAN INIT_FUNCTION
CmImportHardwareHive(PCHAR ChunkBase,
		     ULONG ChunkSize)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  PEREGISTRY_HIVE RegistryHive;
  UNICODE_STRING KeyName;
  HANDLE HardwareKey;
  ULONG Disposition;
  NTSTATUS Status;

  DPRINT ("CmImportHardwareHive() called\n");

  if (CmiHardwareHiveImported == TRUE)
    return TRUE;

  if (ChunkBase == NULL &&
      ChunkSize == 0)
    {
      /* Create '\Registry\Machine\HARDWARE' key. */
      RtlInitUnicodeString (&KeyName,
			    REG_HARDWARE_KEY_NAME);
      InitializeObjectAttributes (&ObjectAttributes,
				  &KeyName,
				  OBJ_CASE_INSENSITIVE,
				  NULL,
				  NULL);
      Status = ZwCreateKey (&HardwareKey,
			    KEY_ALL_ACCESS,
			    &ObjectAttributes,
			    0,
			    NULL,
			    REG_OPTION_VOLATILE,
			    &Disposition);
      if (!NT_SUCCESS(Status))
	{
          DPRINT1("NtCreateKey() failed, status: 0x%x\n", Status);
          return FALSE;
	}
      ZwClose (HardwareKey);

      /* Create '\Registry\Machine\HARDWARE\DESCRIPTION' key. */
      RtlInitUnicodeString(&KeyName,
			   REG_DESCRIPTION_KEY_NAME);
      InitializeObjectAttributes (&ObjectAttributes,
				  &KeyName,
				  OBJ_CASE_INSENSITIVE,
				  NULL,
				  NULL);
      Status = ZwCreateKey (&HardwareKey,
			    KEY_ALL_ACCESS,
			    &ObjectAttributes,
			    0,
			    NULL,
			    REG_OPTION_VOLATILE,
			    &Disposition);
      if (!NT_SUCCESS(Status))
	{
          DPRINT1("NtCreateKey() failed, status: 0x%x\n", Status);
          return FALSE;
	}
      ZwClose (HardwareKey);

      /* Create '\Registry\Machine\HARDWARE\DEVICEMAP' key. */
      RtlInitUnicodeString (&KeyName,
			    REG_DEVICEMAP_KEY_NAME);
      InitializeObjectAttributes (&ObjectAttributes,
				  &KeyName,
				  OBJ_CASE_INSENSITIVE,
				  NULL,
				  NULL);
      Status = ZwCreateKey (&HardwareKey,
			    KEY_ALL_ACCESS,
			    &ObjectAttributes,
			    0,
			    NULL,
			    REG_OPTION_VOLATILE,
			    &Disposition);
      if (!NT_SUCCESS(Status))
	{
          DPRINT1("NtCreateKey() failed, status: 0x%x\n", Status);
          return FALSE;
	}
      ZwClose (HardwareKey);

      /* Create '\Registry\Machine\HARDWARE\RESOURCEMAP' key. */
      RtlInitUnicodeString(&KeyName,
			   REG_RESOURCEMAP_KEY_NAME);
      InitializeObjectAttributes (&ObjectAttributes,
				  &KeyName,
				  OBJ_CASE_INSENSITIVE,
				  NULL,
				  NULL);
      Status = ZwCreateKey (&HardwareKey,
			    KEY_ALL_ACCESS,
			    &ObjectAttributes,
			    0,
			    NULL,
			    REG_OPTION_VOLATILE,
			    &Disposition);
      if (!NT_SUCCESS(Status))
	{
          DPRINT1("NtCreateKey() failed, status: 0x%x\n", Status);
          return FALSE;
	}
      ZwClose (HardwareKey);

      return TRUE;
    }

  /* Check the hive magic */
  if (strncmp (ChunkBase, "regf", 4))
    {
      DPRINT1 ("Found invalid '%.*s' magic\n", 4, ChunkBase);
      return FALSE;
    }

  DPRINT ("Found '%.*s' magic\n", 4, ChunkBase);
  DPRINT ("ChunkBase %lx  ChunkSize %lu\n", ChunkBase, ChunkSize);

  /* Import the binary system hive (volatile, offset-based, permanent) */
  if (!CmImportBinaryHive (ChunkBase, ChunkSize, HIVE_NO_FILE, &RegistryHive))
    {
      DPRINT1 ("CmiImportBinaryHive() failed\n");
      return FALSE;
    }

  /* Attach it to the machine key */
  RtlInitUnicodeString (&KeyName,
			REG_HARDWARE_KEY_NAME);
  InitializeObjectAttributes (&ObjectAttributes,
			      &KeyName,
			      OBJ_CASE_INSENSITIVE,
			      NULL,
			      NULL);
  Status = CmiConnectHive (&ObjectAttributes,
			   RegistryHive);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1 ("CmiConnectHive(%wZ) failed (Status %lx)\n", &KeyName, Status);
//      CmiRemoveRegistryHive(RegistryHive);
      return FALSE;
    }

  /* Set the hive filename */
  RtlInitUnicodeString (&RegistryHive->HiveFileName,
			NULL);

  /* Set the log filename */
  RtlInitUnicodeString (&RegistryHive->LogFileName,
			NULL);

  CmiHardwareHiveImported = TRUE;

  return TRUE;
}

/* EOF */
