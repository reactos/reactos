/* $Id: create.c,v 1.40 2002/05/05 20:18:33 hbirr Exp $
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

#define ENTRIES_PER_PAGE   (PAGESIZE / sizeof (FATDirEntry))

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
    while (fromIndex < 3 && pExtension [fromIndex] != ' ')
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

NTSTATUS
GetEntryName(PVOID *pContext,
             PVOID *Block,
             PFILE_OBJECT FileObject,
             PWSTR Name,
             PULONG pIndex,
             PULONG pIndex2)
/*
 * FUNCTION: Retrieves the file name, be it in short or long file name format
 */
{
  NTSTATUS Status;
  FATDirEntry * test;
  slot * test2;
  ULONG cpos;
  ULONG Offset = *pIndex % ENTRIES_PER_PAGE;
  ULONG Read;
  LARGE_INTEGER FileOffset;

  *Name = 0;
  while (TRUE)
  {
    test = (FATDirEntry *) *Block;
    test2 = (slot *) *Block;
    if (vfatIsDirEntryEndMarker(&test[Offset]))
    {
      return STATUS_NO_MORE_ENTRIES;
    }
    if (test2[Offset].attr == 0x0f && !vfatIsDirEntryDeleted(&test[Offset]))
    {
      *Name = 0;
      if (pIndex2)
        *pIndex2 = *pIndex; // start of dir entry

      DPRINT ("  long name entry found at %d\n", *pIndex);

      DPRINT ("  name chunk1:[%.*S] chunk2:[%.*S] chunk3:[%.*S]\n",
              5, test2 [Offset].name0_4,
              6, test2 [Offset].name5_10,
              2, test2 [Offset].name11_12);

      vfat_initstr (Name, 255);
      vfat_wcsncpy (Name, test2[Offset].name0_4, 5);
      vfat_wcsncat (Name, test2[Offset].name5_10, 5, 6);
      vfat_wcsncat (Name, test2[Offset].name11_12, 11, 2);

      DPRINT ("  longName: [%S]\n", Name);
      cpos = 0;
      while ((test2[Offset].id != 0x41) && (test2[Offset].id != 0x01) &&
        (test2[Offset].attr > 0))
	  {
	    (*pIndex)++;
        Offset++;

	    if (Offset == ENTRIES_PER_PAGE)
        {
          Offset = 0;
          CcUnpinData(*pContext);
          FileOffset.QuadPart = *pIndex * sizeof(FATDirEntry);
          if(!CcMapData(FileObject, &FileOffset, PAGESIZE, TRUE, pContext, Block))
          {
            *pContext = NULL;
            return STATUS_NO_MORE_ENTRIES;
          }
	      test2 = (slot *) *Block;
        }
        DPRINT ("  long name entry found at %d\n", *pIndex);

        DPRINT ("  name chunk1:[%.*S] chunk2:[%.*S] chunk3:[%.*S]\n",
                 5, test2 [Offset].name0_4,
                 6, test2 [Offset].name5_10,
                 2, test2 [Offset].name11_12);

	    cpos++;
	    vfat_movstr (Name, 13, 0, cpos * 13);
	    vfat_wcsncpy (Name, test2[Offset].name0_4, 5);
	    vfat_wcsncat (Name, test2[Offset].name5_10, 5, 6);
	    vfat_wcsncat (Name, test2[Offset].name11_12, 11, 2);

        DPRINT ("  longName: [%S]\n", Name);
	  }
      (*pIndex)++;
      Offset++;
	  if (Offset == ENTRIES_PER_PAGE)
      {
        Offset = 0;
        CcUnpinData(*pContext);
        FileOffset.QuadPart = *pIndex * sizeof(FATDirEntry);
        if(!CcMapData(FileObject, &FileOffset, PAGESIZE, TRUE, pContext, Block))
         {
          *pContext = NULL;
          return STATUS_NO_MORE_ENTRIES;
        }
	    test2 = (slot *) *Block;
        test = (FATDirEntry*) *Block;
      }
    }
    else
    {
      if (vfatIsDirEntryEndMarker(&test[Offset]))
        return STATUS_NO_MORE_ENTRIES;
      if (vfatIsDirEntryDeleted(&test[Offset]))
        return STATUS_UNSUCCESSFUL;
      if (*Name == 0)
      {
        vfat8Dot3ToString (test[Offset].Filename, test[Offset].Ext, Name);
        if (pIndex2)
          *pIndex2 = *pIndex;
      }
      break;
    }
  }
  return STATUS_SUCCESS;
}

NTSTATUS
ReadVolumeLabel (PDEVICE_EXTENSION DeviceExt, PVPB Vpb)
/*
 * FUNCTION: Read the volume label
 */
{
  PVOID Context = NULL;
  ULONG Offset = 0;
  ULONG DirIndex = 0;
  FATDirEntry* Entry;
  PVFATFCB pFcb;
  LARGE_INTEGER FileOffset;

  *(Vpb->VolumeLabel) = 0;
  Vpb->VolumeLabelLength = 0;

  pFcb = vfatOpenRootFCB (DeviceExt);

  while (TRUE)
  {
    if (Context == NULL || Offset == ENTRIES_PER_PAGE)
    {
      if (Offset == ENTRIES_PER_PAGE)
      {
        Offset = 0;
      }
      if (Context)
      {
        CcUnpinData(Context);
      }
      FileOffset.u.HighPart = 0;
      FileOffset.u.LowPart = (DirIndex - Offset) * sizeof(FATDirEntry);
      if (!CcMapData(pFcb->FileObject, &FileOffset, PAGESIZE, TRUE, &Context, (PVOID*)&Entry))
      {
        Context = NULL;
        break;
      }
    }
    if (IsVolEntry(Entry, Offset))
    {
      /* copy volume label */
      vfat8Dot3ToVolumeLabel (Entry[Offset].Filename, Entry[Offset].Ext, Vpb->VolumeLabel);
      Vpb->VolumeLabelLength = wcslen (Vpb->VolumeLabel) * sizeof(WCHAR);
      break;
    }
    if (IsLastEntry(Entry, Offset))
    {
      break;
    }
    Offset++;
    DirIndex++;
  }

  if (Context)
  {
    CcUnpinData(Context);
  }
  vfatReleaseFCB (DeviceExt, pFcb);

  return STATUS_SUCCESS;
}

NTSTATUS
FindFile (PDEVICE_EXTENSION DeviceExt,
          PVFATFCB Fcb,
          PVFATFCB Parent,
          PWSTR FileToFind,
          ULONG *pDirIndex,
          ULONG *pDirIndex2)
/*
 * FUNCTION: Find a file
 */
{
  WCHAR name[256];
  WCHAR name2[14];
  char * block;
  WCHAR TempStr[2];
  NTSTATUS Status;
  ULONG len;
  ULONG DirIndex;
  ULONG Offset;
  ULONG FirstCluster;
  ULONG Read;
  BOOL isRoot;
  LARGE_INTEGER FileOffset;
  PVOID Context = NULL;

  DPRINT ("FindFile(Parent %x, FileToFind '%S', DirIndex: %d)\n", Parent, FileToFind, pDirIndex ? *pDirIndex : 0);
  DPRINT ("FindFile: old Pathname %x, old Objectname %x)\n",Fcb->PathName, Fcb->ObjectName);

  isRoot = FALSE;
  DirIndex = 0;
  if (wcslen (FileToFind) == 0)
  {
    CHECKPOINT;
    TempStr[0] = (WCHAR) '.';
    TempStr[1] = 0;
    FileToFind = (PWSTR)&TempStr;
  }
  if (Parent)
  {
    FirstCluster = vfatDirEntryGetFirstCluster(DeviceExt, &Parent->entry);
    if (DeviceExt->FatInfo.FatType == FAT32)
    {
      if (FirstCluster == DeviceExt->FatInfo.RootCluster)
        isRoot = TRUE;
    }
    else
    {
      if (FirstCluster == 1)
        isRoot = TRUE;
    }
  }
  else
    isRoot = TRUE;
  if (isRoot)
  {
    if (DeviceExt->FatInfo.FatType == FAT32)
      FirstCluster = DeviceExt->FatInfo.RootCluster;
    else
      FirstCluster = 1;

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
	  Fcb->entry.FileSize = DeviceExt->FatInfo.rootDirectorySectors * BLOCKSIZE;
	  Fcb->entry.Attrib = FILE_ATTRIBUTE_DIRECTORY;
	  if (DeviceExt->FatInfo.FatType == FAT32)
      {
	    Fcb->entry.FirstCluster = ((PUSHORT)FirstCluster)[0];
        Fcb->entry.FirstClusterHigh = ((PUSHORT)FirstCluster)[1];
      }
	  else
	    Fcb->entry.FirstCluster = 1;
	  if (pDirIndex)
	    *pDirIndex = 0;
      if (pDirIndex2)
        *pDirIndex2 = 0;
	  DPRINT("FindFile: new Pathname %S, new Objectname %S)\n",Fcb->PathName, Fcb->ObjectName);
	  return (STATUS_SUCCESS);
	}
  }
  else
  {
    DPRINT ("Parent->entry.FileSize %x\n", Parent->entry.FileSize);
    FirstCluster = vfatDirEntryGetFirstCluster (DeviceExt, &Parent->entry);
  }
  if (pDirIndex && (*pDirIndex))
    DirIndex = *pDirIndex;

  Offset = DirIndex % ENTRIES_PER_PAGE;
  while(TRUE)
  {
    if (Context == NULL || Offset == ENTRIES_PER_PAGE)
    {
      if (Offset == ENTRIES_PER_PAGE)
        Offset = 0;
      if (Context)
      {
        CcUnpinData(Context);
      }
      FileOffset.QuadPart = (DirIndex - Offset) * sizeof(FATDirEntry);
      if (!CcMapData(Parent->FileObject, &FileOffset, PAGESIZE, TRUE,
             &Context, (PVOID*)&block))
      {
         Context = NULL;
         break;
      }
    }
	if (vfatIsDirEntryVolume(&((FATDirEntry*)block)[Offset]))
    {
      Offset++;
      DirIndex++;
	  continue;
    }
    Status = GetEntryName (&Context, (PVOID*)&block, Parent->FileObject, name,
                           &DirIndex, pDirIndex2);
    if (Status == STATUS_NO_MORE_ENTRIES)
      break;
    Offset = DirIndex % ENTRIES_PER_PAGE;
    if (NT_SUCCESS(Status))
	{
      vfat8Dot3ToString(((FATDirEntry *) block)[Offset].Filename,((FATDirEntry *) block)[Offset].Ext, name2);
	  if (wstrcmpjoki (name, FileToFind) || wstrcmpjoki (name2, FileToFind))
	  {
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

		memcpy (&Fcb->entry, &((FATDirEntry *) block)[Offset],
			     sizeof (FATDirEntry));
		vfat_wcsncpy (Fcb->ObjectName, name, MAX_PATH);
		if (pDirIndex)
		  *pDirIndex = DirIndex;
         DPRINT("FindFile: new Pathname %S, new Objectname %S, DirIndex %d\n",Fcb->PathName, Fcb->ObjectName, DirIndex);
		if (Context)
		  CcUnpinData(Context);
		return STATUS_SUCCESS;
	  }
    }
    Offset++;
    DirIndex++;
  }
  if (pDirIndex)
	*pDirIndex = DirIndex;
  if (Context)
    CcUnpinData(Context);
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

  DPRINT ("try related for %S\n", pRelativeFileName);
  ccb = pFileObject->FsContext2;
  assert (ccb);
  fcb = ccb->pFcb;
  assert (fcb);

  /* verify related object is a directory and target name
     don't start with \. */
  if (!(fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY)
      || (pRelativeFileName[0] == L'\\'))
  {
    return  STATUS_INVALID_PARAMETER;
  }

  /* construct absolute path name */
  assert (wcslen (fcb->PathName) + 1 + wcslen (pRelativeFileName) + 1
          <= MAX_PATH);
  rcName = ExAllocatePool (NonPagedPool, MAX_PATH * sizeof(WCHAR));
  if (!rcName)
  {
    return STATUS_INSUFFICIENT_RESOURCES;
  }
  wcscpy (rcName, fcb->PathName);
  if (!vfatFCBIsRoot(fcb))
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
      if (!NT_SUCCESS(Status))
      {
        return Status;
      }
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
  if (Fcb->Flags & FCB_DELETE_PENDING)
  {
    vfatReleaseFCB (DeviceExt, Fcb);
    if (AbsFileName)
      ExFreePool (AbsFileName);
    return STATUS_DELETE_PENDING;
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
  BOOLEAN PagingFileCreate = FALSE;

  Stack = IoGetCurrentIrpStackLocation (Irp);
  assert (Stack);
  RequestedDisposition = ((Stack->Parameters.Create.Options >> 24) & 0xff);
  RequestedOptions =
    Stack->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
  PagingFileCreate = (Stack->Flags & SL_OPEN_PAGING_FILE) ? TRUE : FALSE;
  if ((RequestedOptions & FILE_DIRECTORY_FILE)
      && RequestedDisposition == FILE_SUPERSEDE)
    return STATUS_INVALID_PARAMETER;
  FileObject = Stack->FileObject;
  DeviceExt = DeviceObject->DeviceExtension;
  assert (DeviceExt);

  if (FileObject->FileName.Length == 0 && FileObject->RelatedFileObject == NULL)
  {
    /* This a open operation for the volume itself */
    if (RequestedDisposition == FILE_CREATE ||
        RequestedDisposition == FILE_OVERWRITE_IF ||
        RequestedDisposition == FILE_SUPERSEDE)
    {
       Irp->IoStatus.Status = STATUS_ACCESS_DENIED;
       return STATUS_ACCESS_DENIED;
    }
    if (RequestedOptions & FILE_DIRECTORY_FILE)
    {
       Irp->IoStatus.Status = STATUS_NOT_A_DIRECTORY;
       return STATUS_NOT_A_DIRECTORY;
    }
    pFcb = DeviceExt->VolumeFcb;
    pCcb = ExAllocatePoolWithTag (NonPagedPool, sizeof (VFATCCB), TAG_CCB);
    if (pCcb == NULL)
    {
       Irp->IoStatus.Status = STATUS_INSUFFICIENT_RESOURCES;
       return  STATUS_INSUFFICIENT_RESOURCES;
    }
    memset(pCcb, 0, sizeof(VFATCCB));
    FileObject->Flags |= FO_FCB_IS_VALID;
    FileObject->SectionObjectPointers = &pFcb->SectionObjectPointers;
    FileObject->FsContext = (PVOID) &pFcb->RFCB;
    FileObject->FsContext2 = pCcb;
    pCcb->pFcb = pFcb;
    pCcb->PtrFileObject = FileObject;
    pFcb->pDevExt = DeviceExt;
    pFcb->RefCount++;

    Irp->IoStatus.Information = FILE_OPENED;
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
  }

  /*
   * Check for illegal characters in the file name
   */
  c = FileObject->FileName.Buffer;
  while (*c != 0)
    {
      if (*c == L'*' || *c == L'?' || (*c == L'\\' && c[1] == L'\\'))
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
  if (Status == STATUS_INVALID_PARAMETER)
    {
      Irp->IoStatus.Status = Status;
      return Status;
    }
  if (Status == STATUS_DELETE_PENDING)
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
	  {
		if (PagingFileCreate)
		{
		  DPRINT("Creating a new paging file.\n");
		  pCcb = FileObject->FsContext2;
		  pFcb = pCcb->pFcb;
	      pFcb->Flags |= FCB_IS_PAGE_FILE;
	      pFcb->FatChainSize = 0;
	  	  pFcb->FatChain = NULL;
		}

	    Irp->IoStatus.Information = FILE_CREATED;
	  }
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
       if (DeviceExt->FatInfo.FatType == FAT32)
	    Cluster = pFcb->entry.FirstCluster
	      + pFcb->entry.FirstClusterHigh * 65536;
       else
         Cluster = pFcb->entry.FirstCluster;
       pFcb->entry.FirstCluster = 0;
       pFcb->entry.FirstClusterHigh = 0;
       updEntry (DeviceExt, FileObject);
       if ((ULONG)pFcb->RFCB.FileSize.QuadPart > 0)
       {
         pFcb->RFCB.AllocationSize.QuadPart = 0;
	 pFcb->RFCB.FileSize.QuadPart = 0;
	 pFcb->RFCB.ValidDataLength.QuadPart = 0;
	 CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&pFcb->RFCB.AllocationSize);
       }
       while (Cluster != 0xffffffff && Cluster > 1)
       {
	 Status = GetNextCluster (DeviceExt, Cluster, &NextCluster, FALSE);
	 WriteCluster (DeviceExt, Cluster, 0);
	 Cluster = NextCluster;
       }
    }

    /*
     * If this create was for a paging file then make sure all the
     * information needed to manipulate it is locked in memory.
     */
    if (PagingFileCreate)
      {
	ULONG CurrentCluster, NextCluster, i;
	DPRINT("Open an existing paging file\n");
	pFcb->Flags |= FCB_IS_PAGE_FILE;
	pFcb->FatChainSize =
	  ((pFcb->entry.FileSize + DeviceExt->FatInfo.BytesPerCluster - 1) / DeviceExt->FatInfo.BytesPerCluster);
	if (pFcb->FatChainSize)
	{
	  pFcb->FatChain = ExAllocatePool(NonPagedPool, 
					pFcb->FatChainSize * sizeof(ULONG));
	}

	if (DeviceExt->FatInfo.FatType == FAT32)
	  {
	    CurrentCluster = pFcb->entry.FirstCluster + 
	      pFcb->entry.FirstClusterHigh * 65536;
	  }
	else
	  {
	    CurrentCluster = pFcb->entry.FirstCluster;
	  }

	i = 0;
	if (pFcb->FatChainSize)
	{
		while (CurrentCluster != 0xffffffff)
		{
		  pFcb->FatChain[i] = CurrentCluster;	
	      Status = GetNextCluster (DeviceExt, CurrentCluster, &NextCluster, 
				     FALSE);
	      i++;
	      CurrentCluster = NextCluster;
		}
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


NTSTATUS VfatCreate (PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Create or open a file
 */
{
  NTSTATUS Status;

  assert (IrpContext);

  if (IrpContext->DeviceObject->Size == sizeof (DEVICE_OBJECT))
  {
     /* DeviceObject represents FileSystem instead of logical volume */
     DPRINT ("FsdCreate called with file system\n");
     IrpContext->Irp->IoStatus.Information = FILE_OPENED;
     Status = STATUS_SUCCESS;
     goto ByeBye;
  }

  if (!(IrpContext->Flags & IRPCONTEXT_CANWAIT))
  {
     return VfatQueueRequest (IrpContext);
  }

  ExAcquireResourceExclusiveLite (&IrpContext->DeviceExt->DirResource, TRUE);
  Status = VfatCreateFile (IrpContext->DeviceObject, IrpContext->Irp);
  ExReleaseResourceLite (&IrpContext->DeviceExt->DirResource);

ByeBye:
  IrpContext->Irp->IoStatus.Status = Status;
  IoCompleteRequest (IrpContext->Irp, NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT);
  VfatFreeIrpContext(IrpContext);
  return Status;
}

/* EOF */
