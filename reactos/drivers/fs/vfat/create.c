/* $Id: create.c,v 1.20 2001/03/06 23:31:44 cnettel Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/vfat/create.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)

 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>
#include <limits.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* FUNCTIONS ****************************************************************/

BOOLEAN
IsLastEntry (PVOID Block, ULONG Offset)
/*
 * FUNCTION: Determine if the given directory entry is the last
 */
{
  return (((FATDirEntry *) Block)[Offset].Filename[0] == 0);
}

BOOLEAN
IsVolEntry (PVOID Block, ULONG Offset)
/*
 * FUNCTION: Determine if the given directory entry is a vol entry
 */
{
  if ((((FATDirEntry *) Block)[Offset].Attrib) == 0x28)
    return TRUE;
  else
    return FALSE;
}

BOOLEAN
IsDeletedEntry (PVOID Block, ULONG Offset)
/*
 * FUNCTION: Determines if the given entry is a deleted one
 */
{
  /* Checks special character */

  return ((((FATDirEntry *) Block)[Offset].Filename[0] == 0xe5) ||
	  (((FATDirEntry *) Block)[Offset].Filename[0] == 0));
}

BOOLEAN
GetEntryName (PVOID Block, PULONG _Offset, PWSTR Name, PULONG _jloop,
	      PDEVICE_EXTENSION DeviceExt, ULONG * _StartingSector)
/*
 * FUNCTION: Retrieves the file name, be it in short or long file name format
 */
{
  FATDirEntry *test;
  slot *test2;
  ULONG Offset = *_Offset;
  ULONG StartingSector = *_StartingSector;
  ULONG jloop = *_jloop;
  ULONG cpos;

  test = (FATDirEntry *) Block;
  test2 = (slot *) Block;

  *Name = 0;

  if (IsDeletedEntry (Block, Offset))
    {
      return (FALSE);
    }

  if (test2[Offset].attr == 0x0f)
    {
      vfat_initstr (Name, 256);
      vfat_wcsncpy (Name, test2[Offset].name0_4, 5);
      vfat_wcsncat (Name, test2[Offset].name5_10, 5, 6);
      vfat_wcsncat (Name, test2[Offset].name11_12, 11, 2);

      cpos = 0;
      while ((test2[Offset].id != 0x41) && (test2[Offset].id != 0x01) &&
	     (test2[Offset].attr > 0))
	{
	  Offset++;
	  if (Offset == ENTRIES_PER_SECTOR)
	    {
	      Offset = 0;
              /* FIXME: Check status */
              GetNextSector (DeviceExt, StartingSector, &StartingSector, FALSE);
	      jloop++;
	      /* FIXME: Check status */
	      VfatReadSectors (DeviceExt->StorageDevice,
			       StartingSector, 1, Block);
	      test2 = (slot *) Block;
	    }
	  cpos++;
	  vfat_movstr (Name, 13, 0, cpos * 13);
	  vfat_wcsncpy (Name, test2[Offset].name0_4, 5);
	  vfat_wcsncat (Name, test2[Offset].name5_10, 5, 6);
	  vfat_wcsncat (Name, test2[Offset].name11_12, 11, 2);

	}

      if (IsDeletedEntry (Block, Offset + 1))
	{
	  Offset++;
	  *_Offset = Offset;
	  *_jloop = jloop;
	  *_StartingSector = StartingSector;
	  return (FALSE);
	}

      *_Offset = Offset;
      *_jloop = jloop;
      *_StartingSector = StartingSector;

      return (TRUE);
    }

  RtlAnsiToUnicode (Name, test[Offset].Filename, 8);
  if (test[Offset].Ext[0] != ' ')
    {
      RtlCatAnsiToUnicode (Name, ".", 1);
    }
  RtlCatAnsiToUnicode (Name, test[Offset].Ext, 3);

  *_Offset = Offset;

  return (TRUE);
}

NTSTATUS
ReadVolumeLabel (PDEVICE_EXTENSION DeviceExt, PVPB Vpb)
/*
 * FUNCTION: Read the volume label
 */
{
  ULONG i = 0;
  ULONG j;
  ULONG Size;
  char *block;
  ULONG StartingSector;
  ULONG NextCluster;
  NTSTATUS Status;

  Size = DeviceExt->rootDirectorySectors;      /* FIXME : in fat32, no limit */
  StartingSector = DeviceExt->rootStart;
  NextCluster = 0;

  block = ExAllocatePool (NonPagedPool, BLOCKSIZE);
  DPRINT ("FindFile : start at sector %lx, entry %ld\n", StartingSector, i);
  for (j = 0; j < Size; j++)
    {
      /* FIXME: Check status */
      VfatReadSectors (DeviceExt->StorageDevice, StartingSector, 1, block);

      for (i = 0; i < ENTRIES_PER_SECTOR; i++)
	{
	  if (IsVolEntry ((PVOID) block, i))
	    {
	      FATDirEntry *test = (FATDirEntry *) block;

	      /* copy volume label */
	      RtlAnsiToUnicode (Vpb->VolumeLabel, test[i].Filename, 8);
	      RtlCatAnsiToUnicode (Vpb->VolumeLabel, test[i].Ext, 3);
	      Vpb->VolumeLabelLength = wcslen (Vpb->VolumeLabel);

	      ExFreePool (block);
	      return (STATUS_SUCCESS);
	    }
	  if (IsLastEntry ((PVOID) block, i))
	    {
	      *(Vpb->VolumeLabel) = 0;
	      Vpb->VolumeLabelLength = 0;
	      ExFreePool (block);
	      return (STATUS_UNSUCCESSFUL);
	    }
	}
      /* not found in this sector, try next : */

      /* directory can be fragmented although it is best to keep them
         unfragmented.*/
      StartingSector++;

      if (DeviceExt->FatType == FAT32)
	{
	  if (StartingSector == ClusterToSector (DeviceExt, NextCluster + 1))
	    {
	      Status = GetNextCluster (DeviceExt, NextCluster, &NextCluster,
				       FALSE);
	      if (NextCluster == 0 || NextCluster == 0xffffffff)
		{
		  *(Vpb->VolumeLabel) = 0;
		  Vpb->VolumeLabelLength = 0;
		  ExFreePool (block);
		  return (STATUS_UNSUCCESSFUL);
		}
	      StartingSector = ClusterToSector (DeviceExt, NextCluster);
	    }
	}
    }
  *(Vpb->VolumeLabel) = 0;
  Vpb->VolumeLabelLength = 0;
  ExFreePool (block);
  return (STATUS_UNSUCCESSFUL);
}


NTSTATUS
FindFile (PDEVICE_EXTENSION DeviceExt, PVFATFCB Fcb,
	  PVFATFCB Parent, PWSTR FileToFind, ULONG * StartSector,
	  ULONG * Entry)
/*
 * FUNCTION: Find a file
 */
{
  ULONG i, j;
  ULONG Size;
  char *block;
  WCHAR name[256];
  ULONG StartingSector;
  ULONG NextCluster;
  WCHAR TempStr[2];
  NTSTATUS Status;

  DPRINT ("FindFile(Parent %x, FileToFind '%S')\n", Parent, FileToFind);

  if (wcslen (FileToFind) == 0)
    {
      CHECKPOINT;
      TempStr[0] = (WCHAR) '.';
      TempStr[1] = 0;
      FileToFind = (PWSTR)&TempStr;
    }

  if (Parent == NULL || Parent->entry.FirstCluster == 1)
    {
      Size = DeviceExt->rootDirectorySectors;  /* FIXME : in fat32, no limit */
      StartingSector = DeviceExt->rootStart;
      NextCluster = 0;
      if (FileToFind[0] == 0 || (FileToFind[0] == '\\' && FileToFind[1] == 0)
	  || (FileToFind[0] == '.' && FileToFind[1] == 0))
	{
	  /* it's root : complete essentials fields then return ok */
	  memset (Fcb, 0, sizeof (VFATFCB));
	  memset (Fcb->entry.Filename, ' ', 11);
	  Fcb->entry.FileSize = DeviceExt->rootDirectorySectors * BLOCKSIZE;
	  Fcb->entry.Attrib = FILE_ATTRIBUTE_DIRECTORY;
	  if (DeviceExt->FatType == FAT32)
	    Fcb->entry.FirstCluster = 2;
	  else
	    Fcb->entry.FirstCluster = 1;    
	  if (StartSector)
	    *StartSector = StartingSector;
	  if (Entry)
	    *Entry = 0;
	  return (STATUS_SUCCESS);
	}
    }
  else
    {
      DPRINT ("Parent->entry.FileSize %x\n", Parent->entry.FileSize);

      Size = ULONG_MAX;
      if (DeviceExt->FatType == FAT32)
	NextCluster = Parent->entry.FirstCluster
	  + Parent->entry.FirstClusterHigh * 65536;
      else
	NextCluster = Parent->entry.FirstCluster;
      StartingSector = ClusterToSector (DeviceExt, NextCluster);
      if (Parent->entry.FirstCluster == 1 && DeviceExt->FatType != FAT32)
	{
	  /* read of root directory in FAT16 or FAT12 */
	  StartingSector = DeviceExt->rootStart;
	}
    }
  block = ExAllocatePool (NonPagedPool, BLOCKSIZE);
  if (StartSector && (*StartSector))
    StartingSector = *StartSector;
  i = (Entry) ? (*Entry) : 0;
  for (j = 0; j < Size; j++)
    {
      /* FIXME: Check status */
      VfatReadSectors (DeviceExt->StorageDevice, StartingSector, 1, block);

      for (i = (Entry) ? (*Entry) : 0; i < ENTRIES_PER_SECTOR; i++)
	{
	  if (IsVolEntry ((PVOID) block, i))
	    continue;
	  if (IsLastEntry ((PVOID) block, i))
	    {
	      if (StartSector)
		*StartSector = StartingSector;
	      if (Entry)
		*Entry = i;
	      ExFreePool (block);
	      return (STATUS_UNSUCCESSFUL);
	    }
	  if (GetEntryName
	      ((PVOID) block, &i, name, &j, DeviceExt, &StartingSector))
	    {
	      if (wstrcmpjoki (name, FileToFind))
		{
		  /* In the case of a long filename, the firstcluster is 
		     stored in the next record -- where it's short name is */
		  if (((FATDirEntry *) block)[i].Attrib == 0x0f)
		    i++;
		  if (i == (ENTRIES_PER_SECTOR))
		    {
                      /* FIXME: Check status */
		      GetNextSector (DeviceExt, StartingSector, &StartingSector, FALSE);

		      /* FIXME: Check status */
		      VfatReadSectors (DeviceExt->StorageDevice,
				       StartingSector, 1, block);
		      i = 0;
		    }
		  memcpy (&Fcb->entry, &((FATDirEntry *) block)[i],
			  sizeof (FATDirEntry));
		  vfat_wcsncpy (Fcb->ObjectName, name, MAX_PATH);
		  if (StartSector)
		    *StartSector = StartingSector;
		  if (Entry)
		    *Entry = i;
		  ExFreePool (block);
		  return (STATUS_SUCCESS);
		}
	    }
	}
      /* not found in this sector, try next : */

      /* directory can be fragmented although it is best to keep them
         unfragmented. Should we change this to also use GetNextSector?
         GetNextSector was originally implemented to handle the case above */
      if (Entry)
	*Entry = 0;
      StartingSector++;
      if ((Parent != NULL && Parent->entry.FirstCluster != 1)
	  || DeviceExt->FatType == FAT32)
	{
	  if (StartingSector == ClusterToSector (DeviceExt, NextCluster + 1))
	    {
	      Status = GetNextCluster (DeviceExt, NextCluster, &NextCluster,
				       FALSE);
	      if (NextCluster == 0 || NextCluster == 0xffffffff)
		{
		  if (StartSector)
		    *StartSector = StartingSector;
		  if (Entry)
		    *Entry = i;
		  ExFreePool (block);
		  return (STATUS_UNSUCCESSFUL);
		}
	      StartingSector = ClusterToSector (DeviceExt, NextCluster);
	    }
	}
    }
  if (StartSector)
    *StartSector = StartingSector;
  if (Entry)
    *Entry = i;
  ExFreePool (block);
  return (STATUS_UNSUCCESSFUL);
}



NTSTATUS
VfatOpenFile (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
	     PWSTR FileName)
/*
 * FUNCTION: Opens a file
 */
{
  PWSTR current = NULL;
  PWSTR next;
  PWSTR string;
  PWSTR buffer; // used to store a pointer while checking MAX_PATH conformance
  PVFATFCB ParentFcb;
  PVFATFCB Fcb, pRelFcb;
  PVFATFCB Temp;
  PVFATCCB newCCB, pRelCcb;
  NTSTATUS Status;
  PFILE_OBJECT pRelFileObject;
  PWSTR AbsFileName = NULL;
  short i, j;
  PLIST_ENTRY current_entry;
  KIRQL oldIrql;
  ULONG BytesPerCluster;

  DPRINT ("VfatOpenFile(%08lx, %08lx, %S)\n", DeviceExt, FileObject, FileName);

  /* FIXME : treat relative name */
  if (FileObject->RelatedFileObject)
    {
      DbgPrint ("try related for %S\n", FileName);
      pRelFileObject = FileObject->RelatedFileObject;
      pRelCcb = pRelFileObject->FsContext2;
      assert (pRelCcb);
      pRelFcb = pRelCcb->pFcb;
      assert (pRelFcb);
      /*
       * verify related object is a directory and target name don't start with
       *  \.
       */
      if (!(pRelFcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY)
	  || (FileName[0] != '\\'))
	{
	  Status = STATUS_INVALID_PARAMETER;
	  return Status;
	}
      /* construct absolute path name */
      AbsFileName = ExAllocatePool (NonPagedPool, MAX_PATH);
      for (i = 0; pRelFcb->PathName[i]; i++)
	AbsFileName[i] = pRelFcb->PathName[i];
      AbsFileName[i++] = '\\';
      for (j = 0; FileName[j] && i < MAX_PATH; j++)
	AbsFileName[i++] = FileName[j];
      assert (i < MAX_PATH);
      AbsFileName[i] = 0;
      FileName = AbsFileName;
    }

  /*
   * try first to find an existing FCB in memory
   */
  CHECKPOINT;

  KeAcquireSpinLock (&DeviceExt->FcbListLock, &oldIrql);
  current_entry = DeviceExt->FcbListHead.Flink;
  while (current_entry != &DeviceExt->FcbListHead)
    {
      Fcb = CONTAINING_RECORD (current_entry, VFATFCB, FcbListEntry);

      DPRINT ("Scanning %x\n", Fcb);
      DPRINT ("Scanning %S\n", Fcb->PathName);

      if (DeviceExt == Fcb->pDevExt && wstrcmpi (FileName, Fcb->PathName))
	{
	  Fcb->RefCount++;
	  KeReleaseSpinLock (&DeviceExt->FcbListLock, oldIrql);
	  FileObject->FsContext = (PVOID)&Fcb->RFCB;
	  newCCB = ExAllocatePool (NonPagedPool, sizeof (VFATCCB));
	  memset (newCCB, 0, sizeof (VFATCCB));
	  FileObject->Flags = FileObject->Flags | 
	    FO_FCB_IS_VALID | FO_DIRECT_CACHE_PAGING_READ;
	  FileObject->SectionObjectPointers = 
	    &Fcb->SectionObjectPointers;
	  FileObject->FsContext2 = newCCB;
	  newCCB->pFcb = Fcb;
	  newCCB->PtrFileObject = FileObject;
	  if (AbsFileName)
	    ExFreePool (AbsFileName);
	  return (STATUS_SUCCESS);
	}

      current_entry = current_entry->Flink;
    }
  KeReleaseSpinLock (&DeviceExt->FcbListLock, oldIrql);

  CHECKPOINT;
  DPRINT ("FileName %S\n", FileName);

  string = FileName;
  ParentFcb = NULL;
  Fcb = ExAllocatePool (NonPagedPool, sizeof (VFATFCB));
  memset (Fcb, 0, sizeof (VFATFCB));
  Fcb->ObjectName = &Fcb->PathName[1];
  Fcb->PathName[0]='\\';
  next = &string[0];

  CHECKPOINT;
  if (*next == 0 || *(next+1) == 0)		// root
    {
      memset (Fcb->entry.Filename, ' ', 11);
      Fcb->entry.FileSize = DeviceExt->rootDirectorySectors * BLOCKSIZE;
      Fcb->entry.Attrib = FILE_ATTRIBUTE_DIRECTORY;
      if (DeviceExt->FatType == FAT32)
	Fcb->entry.FirstCluster = 2;
      else
	Fcb->entry.FirstCluster = 1;
      /* FIXME : is 1 the good value for mark root? */
      Fcb->ObjectName--;
      ParentFcb = Fcb;
      DPRINT("%S filename, PathName: %S \n",FileName, ParentFcb->PathName);
      Fcb = NULL;
    }
  else
    {
      while (TRUE)
	{
	  CHECKPOINT;
	  *next = '\\';
	  current = next + 1;
	  next = wcschr (next + 1, '\\');
	  if (next != NULL)
	    {
	      *next = 0;
	    }
	  else
	    {
	      /* reached the last path component */
	      DPRINT ("exiting: current '%S'\n", current);
	      break;
	    }

	  DPRINT ("current '%S'\n", current);
	  Status = FindFile (DeviceExt, Fcb, ParentFcb, current, NULL, NULL);
	  if (Status != STATUS_SUCCESS)
	    {
	      CHECKPOINT;
	      if (Fcb != NULL)
		ExFreePool (Fcb);
	      if (ParentFcb != NULL)
		ExFreePool (ParentFcb);
	      if (AbsFileName)
		ExFreePool (AbsFileName);

	      DPRINT ("error STATUS_OBJECT_PATH_NOT_FOUND\n");
	      return STATUS_OBJECT_PATH_NOT_FOUND;
	    }
	  Temp = Fcb;
	  CHECKPOINT;
	  if (ParentFcb == NULL)
	    {
	      CHECKPOINT;
	      Fcb = ExAllocatePool (NonPagedPool, sizeof (VFATFCB));
	      memset (Fcb, 0, sizeof (VFATFCB));
              Fcb->ObjectName = &Fcb->PathName[1];
	      Fcb->PathName[0] = '\\';
	    }
	  else
	      Fcb = ParentFcb;

	  buffer=Fcb->ObjectName;
	  Fcb->ObjectName = Fcb->PathName + (Temp->ObjectName-Temp->PathName) + wcslen(Temp->ObjectName)+1;
// The line above should be possible to optimize. I was tired when writing it and always did something wrong
	  if (Fcb->ObjectName - Fcb->PathName >= MAX_PATH)
	    {
	      if (Fcb != NULL)
		  ExFreePool (Fcb);
	      if (ParentFcb != NULL)
		 ExFreePool (ParentFcb);
	      if (AbsFileName)
		 ExFreePool (AbsFileName);

	      return STATUS_OBJECT_PATH_NOT_FOUND;
	// Which error code? It's no input buffer limit, it's the MAX_PATH limit.
	// However, the current one is still wrong. Fix it!
	    }
	  wcscat(buffer, Temp->ObjectName-1);
	  Fcb->ObjectName[-1]='\\';
	  Fcb->ObjectName[0]=0;
	  
	  CHECKPOINT;
	  ParentFcb = Temp;
	}

      /* searching for last path component */
      DPRINT ("current '%S'\n", current);
      Status = FindFile (DeviceExt, Fcb, ParentFcb, current, NULL, NULL);
      if (Status != STATUS_SUCCESS)
        {
	  /* file does not exist */
	  CHECKPOINT;
	  if (Fcb != NULL)
	    ExFreePool (Fcb);
	  if (ParentFcb != NULL)
	    ExFreePool (ParentFcb);
	  if (AbsFileName)
	    ExFreePool (AbsFileName);

          return STATUS_OBJECT_NAME_NOT_FOUND;
	}

      Temp = Fcb;

      Fcb = ParentFcb;
      ParentFcb = Temp;
      ParentFcb->ObjectName = wcschr (ParentFcb->ObjectName, '\\');
    }

  FileObject->Flags = FileObject->Flags | 
    FO_FCB_IS_VALID | FO_DIRECT_CACHE_PAGING_READ;
  FileObject->SectionObjectPointers = &ParentFcb->SectionObjectPointers;
  memset(FileObject->SectionObjectPointers, 0, 
	 sizeof(SECTION_OBJECT_POINTERS));
  FileObject->FsContext = (PVOID)&ParentFcb->RFCB;
  newCCB = ExAllocatePool (NonPagedPool, sizeof (VFATCCB));
  memset (newCCB, 0, sizeof (VFATCCB));
  FileObject->FsContext2 = newCCB;
  newCCB->pFcb = ParentFcb;
  newCCB->PtrFileObject = FileObject;
  ParentFcb->RefCount++;
  /* FIXME : initialize all fields in FCB and CCB */

  KeAcquireSpinLock (&DeviceExt->FcbListLock, &oldIrql);
  InsertTailList (&DeviceExt->FcbListHead, &ParentFcb->FcbListEntry);
  KeReleaseSpinLock (&DeviceExt->FcbListLock, oldIrql);

/*  vfat_wcsncpy (ParentFcb->PathName, FileName, MAX_PATH);
  ParentFcb->ObjectName = ParentFcb->PathName + (current - FileName); */
  ParentFcb->pDevExt = DeviceExt;
  BytesPerCluster = DeviceExt->Boot->SectorsPerCluster * BLOCKSIZE;
  if (BytesPerCluster >= PAGESIZE)
    {
      Status = CcInitializeFileCache(FileObject, &ParentFcb->RFCB.Bcb,
				     BytesPerCluster);
    }
  else
    {
      Status = CcInitializeFileCache(FileObject, &ParentFcb->RFCB.Bcb, 
				     PAGESIZE);
    }
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("CcInitializeFileCache failed\n");
      KeBugCheck(0);
    }
  DPRINT ("file open, fcb=%x\n", ParentFcb);
  DPRINT ("FileSize %d\n", ParentFcb->entry.FileSize);
  if (Fcb)
    ExFreePool (Fcb);
  if (AbsFileName)
    ExFreePool (AbsFileName);
  CHECKPOINT;

  return (STATUS_SUCCESS);
}


NTSTATUS
VfatCreateFile (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Create or open a file
 */
{
  PIO_STACK_LOCATION Stack;
  PFILE_OBJECT FileObject;
  NTSTATUS Status = STATUS_SUCCESS;
  PDEVICE_EXTENSION DeviceExt;
  ULONG RequestedDisposition, RequestedOptions;
  PVFATCCB pCcb;
  PVFATFCB pFcb;
  PWCHAR c;

  Stack = IoGetCurrentIrpStackLocation (Irp);
  assert (Stack);
  RequestedDisposition = ((Stack->Parameters.Create.Options >> 24) & 0xff);
  RequestedOptions =
    Stack->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
  if ((RequestedOptions & FILE_DIRECTORY_FILE)
      && RequestedDisposition == FILE_SUPERSEDE)
    return STATUS_INVALID_PARAMETER;
  FileObject = Stack->FileObject;
  DeviceExt = DeviceObject->DeviceExtension;
  assert (DeviceExt);
  
  /*
   * Check for illegal characters in the file name
   */
  c = FileObject->FileName.Buffer;
  while (*c != 0)
    {
      if (*c == L'*' || *c == L'?')
	{
	  Irp->IoStatus.Information = 0;
	  Irp->IoStatus.Status = STATUS_OBJECT_NAME_INVALID;
	  return(Status);
	}
      c++;
    }

  Status = VfatOpenFile (DeviceExt, FileObject, FileObject->FileName.Buffer);

  /*
   * If the directory containing the file to open doesn't exist then
   * fail immediately
   */
  Irp->IoStatus.Information = 0;
  if (Status == STATUS_OBJECT_PATH_NOT_FOUND)
    {
      Irp->IoStatus.Status = Status;
      return Status;
    }

  if (!NT_SUCCESS (Status))
    {
      /*
       * If the file open failed then create the required file
       */
      if (RequestedDisposition == FILE_CREATE || 
	  RequestedDisposition == FILE_OPEN_IF || 
	  RequestedDisposition == FILE_OVERWRITE_IF || 
	  RequestedDisposition == FILE_SUPERSEDE)
	{
	  CHECKPOINT;
	  Status = 
	    addEntry (DeviceExt, FileObject, RequestedOptions,
		      (Stack->Parameters.
		       Create.FileAttributes & FILE_ATTRIBUTE_VALID_FLAGS));
	  if (NT_SUCCESS (Status))
	    Irp->IoStatus.Information = FILE_CREATED;
	  /* FIXME set size if AllocationSize requested */
	  /* FIXME set extended attributes? */
	  /* FIXME set share access */
	  /* IoSetShareAccess(DesiredAccess,ShareAccess,FileObject,
	   * ((PVfatCCB)(FileObject->FsContext2))->pFcb->FCBShareAccess);
	   */
	}
    }
  else
    {
      /*
       * Otherwise fail if the caller wanted to create a new file
       */
      if (RequestedDisposition == FILE_CREATE)
	{
	  Irp->IoStatus.Information = FILE_EXISTS;
	  Status = STATUS_OBJECT_NAME_COLLISION;
	}      
      pCcb = FileObject->FsContext2;
      pFcb = pCcb->pFcb;
      /*
       * If requested then delete the file and create a new one with the
       * same name
       */
      if (RequestedDisposition == FILE_SUPERSEDE)
	{
	  ULONG Cluster, NextCluster;
	  /* FIXME set size to 0 and free clusters */
	  pFcb->entry.FileSize = 0;
	  if (DeviceExt->FatType == FAT32)
	    Cluster = pFcb->entry.FirstCluster
	      + pFcb->entry.FirstClusterHigh * 65536;
	  else
	    Cluster = pFcb->entry.FirstCluster;
	  pFcb->entry.FirstCluster = 0;
	  pFcb->entry.FirstClusterHigh = 0;
	  updEntry (DeviceExt, FileObject);
	  while (Cluster != 0xffffffff && Cluster > 1)
	    {
	      Status = GetNextCluster (DeviceExt, Cluster, &NextCluster, TRUE);
	      WriteCluster (DeviceExt, Cluster, 0);
	      Cluster = NextCluster;
	    }
	}

      /*
       * Check the file has the requested attributes
       */
      if ((RequestedOptions & FILE_NON_DIRECTORY_FILE)
	  && (pFcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
	{
	  Status = STATUS_FILE_IS_A_DIRECTORY;
	}
      if ((RequestedOptions & FILE_DIRECTORY_FILE)
	  && !(pFcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
	{
	  Status = STATUS_NOT_A_DIRECTORY;
	}
      /* FIXME : test share access */
      /* FIXME : test write access if requested */
      if (!NT_SUCCESS (Status))
	VfatCloseFile (DeviceExt, FileObject);
      else
	Irp->IoStatus.Information = FILE_OPENED;
      /* FIXME : make supersed or overwrite if requested */
    }

  Irp->IoStatus.Status = Status;

  return Status;
}


NTSTATUS STDCALL
VfatCreate (PDEVICE_OBJECT DeviceObject, PIRP Irp)
/*
 * FUNCTION: Create or open a file
 */
{
  NTSTATUS Status = STATUS_SUCCESS;
  PDEVICE_EXTENSION DeviceExt;

  assert (DeviceObject);
  assert (Irp);
  
  if (DeviceObject->Size == sizeof (DEVICE_OBJECT))
    {
      /* DeviceObject represents FileSystem instead of logical volume */
      DbgPrint ("FsdCreate called with file system\n");
      Irp->IoStatus.Status = Status;
      Irp->IoStatus.Information = FILE_OPENED;
      IoCompleteRequest (Irp, IO_NO_INCREMENT);
      return (Status);
    }

  DeviceExt = DeviceObject->DeviceExtension;
  assert (DeviceExt);
  ExAcquireResourceExclusiveLite (&DeviceExt->DirResource, TRUE);

  Status = VfatCreateFile (DeviceObject, Irp);
  
  ExReleaseResourceLite (&DeviceExt->DirResource);
  
  Irp->IoStatus.Status = Status;
  IoCompleteRequest (Irp, IO_NO_INCREMENT);

  return Status;
}

/* EOF */
