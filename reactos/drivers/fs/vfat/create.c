/* $Id: create.c,v 1.28 2001/07/18 12:04:52 ekohl Exp $
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

/* GLOBALS *******************************************************************/

/* FUNCTIONS *****************************************************************/

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

void  vfat8Dot3ToString (PCHAR pBasename, PCHAR pExtension, PWSTR pName)
{
  int  fromIndex, toIndex;

  fromIndex = toIndex = 0; 
  while (fromIndex < 8 && pBasename [fromIndex] != ' ')
  {
    pName [toIndex++] = pBasename [fromIndex++];
  }
  if (pExtension [0] != ' ')
  {
    pName [toIndex++] = L'.';
    fromIndex = 0;
    while (fromIndex < 3 && pBasename [fromIndex] != ' ')
    {
      pName [toIndex++] = pExtension [fromIndex++];
    }
  }
  pName [toIndex] = L'\0';
}

static void  vfat8Dot3ToVolumeLabel (PCHAR pBasename, PCHAR pExtension, PWSTR pName)
{
  int  fromIndex, toIndex;

  fromIndex = toIndex = 0; 
  while (fromIndex < 8 && pBasename [fromIndex] != ' ')
  {
    pName [toIndex++] = pBasename [fromIndex++];
  }
  if (pExtension [0] != ' ')
  {
    fromIndex = 0;
    while (fromIndex < 3 && pBasename [fromIndex] != ' ')
    {
      pName [toIndex++] = pExtension [fromIndex++];
    }
  }
  pName [toIndex] = L'\0';
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
  PVOID OldBlock;

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

      // save values for returning
      *_Offset = Offset;
      *_jloop = jloop;
      *_StartingSector = StartingSector;

      if (Offset + 1 == ENTRIES_PER_SECTOR)
	{
	  // save the old sector
	  OldBlock = ExAllocatePool (NonPagedPool, BLOCKSIZE);
	  memcpy (OldBlock, Block, BLOCKSIZE);

	  // read the next sector
	  Offset = 0;
	  /* FIXME: Check status */
	  GetNextSector (DeviceExt, StartingSector, &StartingSector, FALSE);
	  jloop++;
	  /* FIXME: Check status */
	  VfatReadSectors (DeviceExt->StorageDevice, StartingSector, 1, Block);

	  test2 = (slot *) Block;

	  if (IsDeletedEntry (Block, Offset))
	    {
	      ExFreePool (OldBlock);
	      *_Offset = Offset;
	      *_jloop = jloop;
	      *_StartingSector = StartingSector;
	      return FALSE;
	    }

	  // restore the old sector
	  memcpy (Block, OldBlock, BLOCKSIZE);
	  ExFreePool (OldBlock);
	}
      else
	{
	  if (IsDeletedEntry (Block, Offset + 1))
	    {
	      Offset++;
	      *_Offset = Offset;
	      *_jloop = jloop;
	      *_StartingSector = StartingSector;
	      return (FALSE);
	    }
	}
      return (TRUE);
    }

  vfat8Dot3ToString (test[Offset].Filename, test[Offset].Ext, Name);

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
              vfat8Dot3ToVolumeLabel (test[i].Filename, test[i].Ext, Vpb->VolumeLabel);
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
  ULONG len;

//  DPRINT ("FindFile(Parent %x, FileToFind '%S')\n", Parent, FileToFind);
  DPRINT("FindFile: old Pathname %x, old Objectname %x)\n",Fcb->PathName, Fcb->ObjectName);

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
	  CHECKPOINT;
	  memset (Fcb, 0, sizeof (VFATFCB));
	  memset (Fcb->entry.Filename, ' ', 11);
	  CHECKPOINT;
	  Fcb->PathName[0]='\\';
	  Fcb->ObjectName = &Fcb->PathName[1];
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
	    DPRINT("FindFile: new Pathname %S, new Objectname %S)\n",Fcb->PathName, Fcb->ObjectName);
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
		  if (Parent && Parent->PathName)
		  {
		    len = wcslen(Parent->PathName);
		    CHECKPOINT;
		    memcpy(Fcb->PathName, Parent->PathName, len*sizeof(WCHAR));
		    Fcb->ObjectName=&Fcb->PathName[len];
		    if (len != 1 || Fcb->PathName[0] != '\\')
		      {
			Fcb->ObjectName[0] = '\\';
			Fcb->ObjectName = &Fcb->ObjectName[1];
		      }
		  }
		  else
		  {
		    Fcb->ObjectName=Fcb->PathName;
		    Fcb->ObjectName[0]='\\';
		    Fcb->ObjectName=&Fcb->ObjectName[1];
		  }

		  memcpy (&Fcb->entry, &((FATDirEntry *) block)[i],
			  sizeof (FATDirEntry));
		  vfat_wcsncpy (Fcb->ObjectName, name, MAX_PATH);
		  if (StartSector)
		    *StartSector = StartingSector;
		  if (Entry)
		    *Entry = i;
		  ExFreePool (block);
		  DPRINT("FindFile: new Pathname %S, new Objectname %S)\n",Fcb->PathName, Fcb->ObjectName);
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

      /* FIXME: Check status */
      GetNextSector (DeviceExt, StartingSector, &StartingSector, FALSE);

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
vfatMakeAbsoluteFilename (PFILE_OBJECT pFileObject, 
                          PWSTR pRelativeFileName,
                          PWSTR *pAbsoluteFilename)
{
  PWSTR  rcName;
  PVFATFCB  fcb;
  PVFATCCB  ccb;

  DbgPrint ("try related for %S\n", pRelativeFileName);
  ccb = pFileObject->FsContext2;
  assert (ccb);
  fcb = ccb->pFcb;
  assert (fcb);

  /* verify related object is a directory and target name 
     don't start with \. */
  if (!(fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY) 
      || (pRelativeFileName[0] != '\\'))
  {
    return  STATUS_INVALID_PARAMETER;
  }

  /* construct absolute path name */
  assert (wcslen (fcb->PathName) + 1 + wcslen (pRelativeFileName) + 1 
          <= MAX_PATH);
  rcName = ExAllocatePool (NonPagedPool, MAX_PATH);
  wcscpy (rcName, fcb->PathName);
  wcscat (rcName, L"\\");
  wcscat (rcName, pRelativeFileName);
  *pAbsoluteFilename = rcName;

  return  STATUS_SUCCESS;
}

NTSTATUS
VfatOpenFile (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
	     PWSTR FileName)
/*
 * FUNCTION: Opens a file
 */
{
  PVFATFCB ParentFcb;
  PVFATFCB Fcb;
  NTSTATUS Status;
  PWSTR AbsFileName = NULL;

  DPRINT ("VfatOpenFile(%08lx, %08lx, %S)\n", DeviceExt, FileObject, FileName);

  if (FileObject->RelatedFileObject)
    {
      DPRINT ("Converting relative filename to absolute filename\n");
      Status = vfatMakeAbsoluteFilename (FileObject->RelatedFileObject,
                                         FileName,
                                         &AbsFileName);
      FileName = AbsFileName;
    }

  //FIXME: Get cannonical path name (remove .'s, ..'s and extra separators)

  DPRINT ("PathName to open: %S\n", FileName);

  /*  try first to find an existing FCB in memory  */
  DPRINT ("Checking for existing FCB in memory\n");
  Fcb = vfatGrabFCBFromTable (DeviceExt, FileName);
  if (Fcb == NULL)
  {
    DPRINT ("No existing FCB found, making a new one if file exists.\n");
    Status = vfatGetFCBForFile (DeviceExt, &ParentFcb, &Fcb, FileName);
    if (ParentFcb != NULL)
    {
      vfatReleaseFCB (DeviceExt, ParentFcb);
    }
    if (!NT_SUCCESS (Status))
    {
      DPRINT ("Could not make a new FCB, status: %x\n", Status);

      if (AbsFileName)
        ExFreePool (AbsFileName);

      return  Status;
    }
  }

  DPRINT ("Attaching FCB to fileObject\n");
  Status = vfatAttachFCBToFileObject (DeviceExt, Fcb, FileObject);

  if (AbsFileName)
    ExFreePool (AbsFileName);

  return  Status;
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
	  return(STATUS_OBJECT_NAME_INVALID);
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
