/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: create.c,v 1.68 2004/06/23 20:23:59 hbirr Exp $
 *
 * PROJECT:          ReactOS kernel
 * FILE:             drivers/fs/vfat/create.c
 * PURPOSE:          VFAT Filesystem
 * PROGRAMMER:       Jason Filby (jasonfilby@yahoo.com)
 *                   Hartmut Birr
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <wchar.h>
#include <limits.h>

#define NDEBUG
#include <debug.h>

#include "vfat.h"

/* GLOBALS *******************************************************************/

#define ENTRIES_PER_PAGE   (PAGE_SIZE / sizeof (FATDirEntry))

/* FUNCTIONS *****************************************************************/

void  vfat8Dot3ToString (PFAT_DIR_ENTRY pEntry, PUNICODE_STRING NameU)
{
  OEM_STRING StringA;
  ULONG Length;
  CHAR  cString[12];
  
  memcpy(cString, pEntry->Filename, 11);
  cString[11] = 0;
  if (cString[0] == 0x05)
    {
      cString[0] = 0xe5;
    }      

  StringA.Buffer = cString;
  for (StringA.Length = 0; 
       StringA.Length < 8 && StringA.Buffer[StringA.Length] != ' ';
       StringA.Length++);
  StringA.MaximumLength = StringA.Length;
  
  RtlOemStringToUnicodeString(NameU, &StringA, FALSE);

  if (pEntry->lCase & VFAT_CASE_LOWER_BASE)
    {
      RtlDowncaseUnicodeString(NameU, NameU, FALSE);
    }
  if (cString[8] != ' ')
    {
      Length = NameU->Length;
      NameU->Buffer += Length / sizeof(WCHAR);
      if (!ENTRY_VOLUME(pEntry))
        {
	  Length += sizeof(WCHAR);
          NameU->Buffer[0] = L'.';
          NameU->Buffer++;
	}
      NameU->Length = 0;
      NameU->MaximumLength -= Length;
      
      StringA.Buffer = &cString[8];
      for (StringA.Length = 0; 
           StringA.Length < 3 && StringA.Buffer[StringA.Length] != ' ';
           StringA.Length++);
      StringA.MaximumLength = StringA.Length;
      RtlOemStringToUnicodeString(NameU, &StringA, FALSE);
      if (pEntry->lCase & VFAT_CASE_LOWER_EXT)
        {
          RtlDowncaseUnicodeString(NameU, NameU, FALSE);
        }
      NameU->Buffer -= Length / sizeof(WCHAR);
      NameU->Length += Length;
      NameU->MaximumLength += Length;
    }
  NameU->Buffer[NameU->Length / sizeof(WCHAR)] = 0;
  DPRINT("'%wZ'\n", NameU);
}

NTSTATUS
ReadVolumeLabel (PDEVICE_EXTENSION DeviceExt, PVPB Vpb)
/*
 * FUNCTION: Read the volume label
 */
{
  PVOID Context = NULL;
  ULONG DirIndex = 0;
  FATDirEntry* Entry;
  PVFATFCB pFcb;
  LARGE_INTEGER FileOffset;
  UNICODE_STRING NameU;

  NameU.Buffer = Vpb->VolumeLabel;
  NameU.Length = 0;
  NameU.MaximumLength = sizeof(Vpb->VolumeLabel);
  *(Vpb->VolumeLabel) = 0;
  Vpb->VolumeLabelLength = 0;

  ExAcquireResourceExclusiveLite (&DeviceExt->DirResource, TRUE);
  pFcb = vfatOpenRootFCB (DeviceExt);
  ExReleaseResourceLite (&DeviceExt->DirResource);

  FileOffset.QuadPart = 0;
  if (CcMapData(pFcb->FileObject, &FileOffset, PAGE_SIZE, TRUE, &Context, (PVOID*)&Entry))
  {
     while (TRUE)
     {
       if (ENTRY_VOLUME(Entry))
       {
          /* copy volume label */
          vfat8Dot3ToString (Entry, &NameU);
          Vpb->VolumeLabelLength = NameU.Length;
          break;
       }
       if (ENTRY_END(Entry))
       {
          break;
       }
       DirIndex++;       
       Entry++;
       if ((DirIndex % ENTRIES_PER_PAGE) == 0)
       {
	  CcUnpinData(Context);
	  FileOffset.u.LowPart += PAGE_SIZE;
	  if (!CcMapData(pFcb->FileObject, &FileOffset, PAGE_SIZE, TRUE, &Context, (PVOID*)&Entry))
	  {
	     Context = NULL;
	     break;
	  }
       }
     }
     if (Context)
     {
       CcUnpinData(Context);
     }
  }
  ExAcquireResourceExclusiveLite (&DeviceExt->DirResource, TRUE);
  vfatReleaseFCB (DeviceExt, pFcb);
  ExReleaseResourceLite (&DeviceExt->DirResource);

  return STATUS_SUCCESS;
}

NTSTATUS
FindFile (PDEVICE_EXTENSION DeviceExt,
          PVFATFCB Parent,
          PUNICODE_STRING FileToFindU,
	  PVFAT_DIRENTRY_CONTEXT DirContext,
	  BOOLEAN First)
/*
 * FUNCTION: Find a file
 */
{
  WCHAR PathNameBuffer[MAX_PATH];
  NTSTATUS Status;
  PVOID Context = NULL;
  PVOID Page;
  PVFATFCB rcFcb;
  BOOLEAN FoundLong;
  BOOLEAN FoundShort;
  UNICODE_STRING PathNameU;
  BOOLEAN WildCard;
  PWCHAR curr, last;

  DPRINT ("FindFile(Parent %x, FileToFind '%wZ', DirIndex: %d)\n", 
          Parent, FileToFindU, DirContext->DirIndex);
  DPRINT ("FindFile: Path %wZ)\n",&Parent->PathNameU);

  PathNameU.Buffer = PathNameBuffer;
  PathNameU.Length = 0;
  PathNameU.MaximumLength = sizeof(PathNameBuffer);

  DirContext->LongNameU.Length = 0;
  DirContext->ShortNameU.Length = 0;

  /* FIXME: Use FsRtlDoesNameContainWildCards */
  WildCard = FALSE;
  curr = FileToFindU->Buffer;
  last = FileToFindU->Buffer + FileToFindU->Length / sizeof(WCHAR);
  while (curr < last)
    {
      if (*curr == L'?' || *curr == L'*')
        {
	  WildCard = TRUE;
	  break;
	}
      curr++;
    }

  if (WildCard == FALSE)
    {
      /* if there is no '*?' in the search name, than look first for an existing fcb */
      RtlCopyUnicodeString(&PathNameU, &Parent->PathNameU);
      if (!vfatFCBIsRoot(Parent))
        {
          PathNameU.Buffer[PathNameU.Length / sizeof(WCHAR)] = L'\\';
	  PathNameU.Length += sizeof(WCHAR);
        }
      RtlAppendUnicodeStringToString(&PathNameU, FileToFindU);
      PathNameU.Buffer[PathNameU.Length / sizeof(WCHAR)] = 0;
      rcFcb = vfatGrabFCBFromTable(DeviceExt, &PathNameU);
      if (rcFcb)
        {
	  if(rcFcb->startIndex >= DirContext->DirIndex)
	    {
	      RtlCopyUnicodeString(&DirContext->LongNameU, &rcFcb->LongNameU);
	      RtlCopyUnicodeString(&DirContext->ShortNameU, &rcFcb->ShortNameU);
	      memcpy(&DirContext->FatDirEntry, &rcFcb->entry, sizeof(FATDirEntry));
	      DirContext->StartIndex = rcFcb->startIndex;
	      DirContext->DirIndex = rcFcb->dirIndex;
              DPRINT("FindFile: new Name %wZ, DirIndex %d (%d)\n",
		     &DirContext->LongNameU, DirContext->DirIndex, DirContext->StartIndex);
	      Status = STATUS_SUCCESS;
	    }
          else
	    {
	      CHECKPOINT1;
	      Status = STATUS_UNSUCCESSFUL;
	    }
          vfatReleaseFCB(DeviceExt, rcFcb);
	  return Status;
	}
    }

  while(TRUE)
    {
      Status = vfatGetNextDirEntry(&Context, &Page, Parent, DirContext, First);
      First = FALSE;
      if (Status == STATUS_NO_MORE_ENTRIES)
        {
	  break;
        }
      if (ENTRY_VOLUME(&DirContext->FatDirEntry))
        {
          DirContext->DirIndex++;
          continue;
        }
      DirContext->LongNameU.Buffer[DirContext->LongNameU.Length / sizeof(WCHAR)] = 0;
      DirContext->ShortNameU.Buffer[DirContext->ShortNameU.Length / sizeof(WCHAR)] = 0;
      if (WildCard)
        {
	  /* FIXME: Use FsRtlIsNameInExpression */
          if (DirContext->LongNameU.Length > 0 && 
	      wstrcmpjoki (DirContext->LongNameU.Buffer, FileToFindU->Buffer))
            {
	      FoundLong = TRUE;
	    }
          else
            {
	      FoundLong = FALSE;
	    }
          if (FoundLong == FALSE)
            {
	      /* FIXME: Use FsRtlIsNameInExpression */
	      FoundShort = wstrcmpjoki (DirContext->ShortNameU.Buffer, FileToFindU->Buffer);
	    }
          else
            {
	      FoundShort = FALSE;
	    }
	}
      else
        {
	  FoundLong = RtlEqualUnicodeString(&DirContext->LongNameU, FileToFindU, TRUE);
	  if (FoundLong == FALSE)
	    {
	      FoundShort = RtlEqualUnicodeString(&DirContext->ShortNameU, FileToFindU, TRUE);
	    }
	}

      if (FoundLong || FoundShort)
        {
	  if (WildCard)
	    {
              RtlCopyUnicodeString(&PathNameU, &Parent->PathNameU);
              if (!vfatFCBIsRoot(Parent))
                {
                  PathNameU.Buffer[PathNameU.Length / sizeof(WCHAR)] = L'\\';
	          PathNameU.Length += sizeof(WCHAR);
                }
              RtlAppendUnicodeStringToString(&PathNameU, &DirContext->LongNameU);
              PathNameU.Buffer[PathNameU.Length / sizeof(WCHAR)] = 0;
              rcFcb = vfatGrabFCBFromTable(DeviceExt, &PathNameU);
	      if (rcFcb != NULL)
	        {
	          memcpy(&DirContext->FatDirEntry, &rcFcb->entry, sizeof(FATDirEntry));
                  vfatReleaseFCB(DeviceExt, rcFcb);
		}
	    }
          DPRINT("%d\n", DirContext->LongNameU.Length);
          DPRINT("FindFile: new Name %wZ, DirIndex %d\n",
	         &DirContext->LongNameU, DirContext->DirIndex);

          if (Context)
	    {
              CcUnpinData(Context);
	    }
          return STATUS_SUCCESS;
	}
      DirContext->DirIndex++;
    }

  if (Context)
    {
      CcUnpinData(Context);
    }

  return Status;
}

NTSTATUS
VfatOpenFile (PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
	      PUNICODE_STRING FileNameU)
/*
 * FUNCTION: Opens a file
 */
{
  PVFATFCB ParentFcb;
  PVFATFCB Fcb;
  NTSTATUS Status;
  UNICODE_STRING NameU;
  WCHAR Name[MAX_PATH];
  ULONG Size;
  ULONG MediaChangeCount;

//  PDEVICE_OBJECT DeviceObject = DeviceExt->StorageDevice->Vpb->DeviceObject;
  
  DPRINT ("VfatOpenFile(%08lx, %08lx, '%wZ')\n", DeviceExt, FileObject, FileNameU);

  if (FileObject->RelatedFileObject)
    {
      DPRINT ("Converting relative filename to absolute filename\n");

      NameU.Buffer = Name;
      NameU.Length = 0;
      NameU.MaximumLength = sizeof(Name);

      Fcb = FileObject->RelatedFileObject->FsContext;
      RtlCopyUnicodeString(&NameU, &Fcb->PathNameU);
      if (!vfatFCBIsRoot(Fcb))
        {
	  NameU.Buffer[NameU.Length / sizeof(WCHAR)] = L'\\';
	  NameU.Length += sizeof(WCHAR);
	}
      RtlAppendUnicodeStringToString(&NameU, FileNameU);
      NameU.Buffer[NameU.Length / sizeof(WCHAR)] = 0;
      FileNameU = &NameU;
    }

  DPRINT ("PathName to open: '%wZ'\n", FileNameU);

  if (!DeviceExt->FatInfo.FixedMedia)
    {
      Size = sizeof(ULONG);
      Status = VfatBlockDeviceIoControl (DeviceExt->StorageDevice,
					 IOCTL_DISK_CHECK_VERIFY,
					 NULL,
					 0,
					 &MediaChangeCount,
					 &Size,
					 FALSE);

      if (Status == STATUS_VERIFY_REQUIRED || MediaChangeCount != DeviceExt->MediaChangeCount)
        {
          PDEVICE_OBJECT DeviceToVerify;

          DPRINT ("Media change detected!\n");
          DPRINT ("Device %p\n", DeviceExt->StorageDevice);

          DeviceToVerify = IoGetDeviceToVerify (PsGetCurrentThread ());
          IoSetDeviceToVerify (PsGetCurrentThread (),
			       NULL);

          Status = IoVerifyVolume (DeviceToVerify,
			          FALSE);
          if (!NT_SUCCESS(Status))
	    {
	      DPRINT ("Status %lx\n", Status);
	      return Status;
	    }
        }
      else if (!NT_SUCCESS(Status))
        {
          DPRINT ("Status %lx\n", Status);
          return Status;
        }
    }


  /*  try first to find an existing FCB in memory  */
  DPRINT ("Checking for existing FCB in memory\n");
  Fcb = vfatGrabFCBFromTable (DeviceExt, FileNameU);
  if (Fcb == NULL)
    {
      DPRINT ("No existing FCB found, making a new one if file exists.\n");
      Status = vfatGetFCBForFile (DeviceExt, &ParentFcb, &Fcb, FileNameU);
      if (ParentFcb != NULL)
        {
          vfatReleaseFCB (DeviceExt, ParentFcb);
        }
      if (!NT_SUCCESS (Status))
        {
          DPRINT ("Could not make a new FCB, status: %x\n", Status);
          return  Status;
	}
    }
  if (Fcb->Flags & FCB_DELETE_PENDING)
    {
      vfatReleaseFCB (DeviceExt, Fcb);
      return STATUS_DELETE_PENDING;
    }
  DPRINT ("Attaching FCB to fileObject\n");
  Status = vfatAttachFCBToFileObject (DeviceExt, Fcb, FileObject);

  return  Status;
}

VOID STATIC
VfatSupersedeFile(PDEVICE_EXTENSION DeviceExt, PFILE_OBJECT FileObject,
		  PVFATFCB Fcb)
{
  ULONG Cluster, NextCluster;
  NTSTATUS Status;
  
  Fcb->entry.FileSize = 0;
  if (DeviceExt->FatInfo.FatType == FAT32)
    {
      Cluster = Fcb->entry.FirstCluster + Fcb->entry.FirstClusterHigh * 65536;
    }
  else
    {
      Cluster = Fcb->entry.FirstCluster;
    }
  Fcb->entry.FirstCluster = 0;
  Fcb->entry.FirstClusterHigh = 0;
  VfatUpdateEntry (Fcb);
  if (Fcb->RFCB.FileSize.QuadPart > 0)
    {
      Fcb->RFCB.AllocationSize.QuadPart = 0;
      Fcb->RFCB.FileSize.QuadPart = 0;
      Fcb->RFCB.ValidDataLength.QuadPart = 0;
      /* Notify cache manager about the change in file size if caching is
         initialized on the file stream */
      if (FileObject->SectionObjectPointer->SharedCacheMap != NULL)
        {
          CcSetFileSizes(FileObject, (PCC_FILE_SIZES)&Fcb->RFCB.AllocationSize);
        }
    }
  while (Cluster != 0xffffffff && Cluster > 1)
    {
      Status = GetNextCluster (DeviceExt, Cluster, &NextCluster, FALSE);
      WriteCluster (DeviceExt, Cluster, 0);
      Cluster = NextCluster;
    }
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
  PWCHAR c, last;
  BOOLEAN PagingFileCreate = FALSE;
  LARGE_INTEGER AllocationSize;
  BOOLEAN Dots;
  
  /* Unpack the various parameters. */
  Stack = IoGetCurrentIrpStackLocation (Irp);
  RequestedDisposition = ((Stack->Parameters.Create.Options >> 24) & 0xff);
  RequestedOptions =
    Stack->Parameters.Create.Options & FILE_VALID_OPTION_FLAGS;
  PagingFileCreate = (Stack->Flags & SL_OPEN_PAGING_FILE) ? TRUE : FALSE;
  FileObject = Stack->FileObject;
  DeviceExt = DeviceObject->DeviceExtension;

  /* Check their validity. */
  if (RequestedOptions & FILE_DIRECTORY_FILE &&
      RequestedDisposition == FILE_SUPERSEDE)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  /* This a open operation for the volume itself */
  if (FileObject->FileName.Length == 0 && 
      FileObject->RelatedFileObject == NULL)
    {      
      if (RequestedDisposition == FILE_CREATE ||
	  RequestedDisposition == FILE_OVERWRITE_IF ||
	  RequestedDisposition == FILE_SUPERSEDE)
	{
	  return(STATUS_ACCESS_DENIED);
	}
      if (RequestedOptions & FILE_DIRECTORY_FILE)
	{
	  return(STATUS_NOT_A_DIRECTORY);
	}
      pFcb = DeviceExt->VolumeFcb;
      pCcb = ExAllocateFromNPagedLookasideList(&VfatGlobalData->CcbLookasideList);
      if (pCcb == NULL)
	{
	  return (STATUS_INSUFFICIENT_RESOURCES);
	}
      memset(pCcb, 0, sizeof(VFATCCB));
      FileObject->Flags |= FO_FCB_IS_VALID;
      FileObject->SectionObjectPointer = &pFcb->SectionObjectPointers;
      FileObject->FsContext = pFcb;
      FileObject->FsContext2 = pCcb;
      pFcb->RefCount++;

      Irp->IoStatus.Information = FILE_OPENED;
      return(STATUS_SUCCESS);
    }

  /*
   * Check for illegal characters and illegale dot sequences in the file name
   */
  c = FileObject->FileName.Buffer + FileObject->FileName.Length / sizeof(WCHAR);
  last = c - 1;
  Dots = TRUE;
  while (c-- > FileObject->FileName.Buffer)
    {
      if (*c == L'\\' || c == FileObject->FileName.Buffer)
        {
	  if (Dots && last > c)
	    {
              return(STATUS_OBJECT_NAME_INVALID);
	    }
	  last = c - 1;
	  Dots = TRUE;
	}
      else if (*c != L'.')
        {
	  Dots = FALSE;
	}

      if (*c != '\\' && vfatIsLongIllegal(*c)) 
        {
          return(STATUS_OBJECT_NAME_INVALID);
	}
    }

  /* Try opening the file. */
  Status = VfatOpenFile (DeviceExt, FileObject, &FileObject->FileName);

  /*
   * If the directory containing the file to open doesn't exist then
   * fail immediately
   */
  if (Status == STATUS_OBJECT_PATH_NOT_FOUND ||
      Status == STATUS_INVALID_PARAMETER ||
      Status == STATUS_DELETE_PENDING)
    {
      return(Status);
    }

  /*
   * If the file open failed then create the required file
   */
  if (!NT_SUCCESS (Status))
    {
      if (RequestedDisposition == FILE_CREATE ||
	  RequestedDisposition == FILE_OPEN_IF ||
	  RequestedDisposition == FILE_OVERWRITE_IF ||
	  RequestedDisposition == FILE_SUPERSEDE)
	{
	  ULONG Attributes;
	  Attributes = Stack->Parameters.Create.FileAttributes;
	  Status = VfatAddEntry (DeviceExt, &FileObject->FileName, FileObject, RequestedOptions, 
				 (UCHAR)(Attributes & FILE_ATTRIBUTE_VALID_FLAGS));
	  if (NT_SUCCESS (Status))
	    {
	      pFcb = FileObject->FsContext;
        
	      Irp->IoStatus.Information = FILE_CREATED;
        
	      VfatSetAllocationSizeInformation(FileObject, 
					       pFcb,
					       DeviceExt,
					       &Irp->Overlay.AllocationSize);
	      VfatSetExtendedAttributes(FileObject, 
					Irp->AssociatedIrp.SystemBuffer,
					Stack->Parameters.Create.EaLength);
	      IoSetShareAccess(0 /*DesiredAccess*/,
			       Stack->Parameters.Create.ShareAccess,
			       FileObject,
			       &pFcb->FCBShareAccess);

	      if (PagingFileCreate)
                {
                  pFcb->Flags |= FCB_IS_PAGE_FILE;
                }
	    }
	  else
	    {
	      return(Status);
	    }
	}
      else
	{
	  return(Status);
	}
    }
  else
    {
      /* Otherwise fail if the caller wanted to create a new file  */
      if (RequestedDisposition == FILE_CREATE)
	{
	  Irp->IoStatus.Information = FILE_EXISTS;
	  VfatCloseFile (DeviceExt, FileObject);
	  return(STATUS_OBJECT_NAME_COLLISION);
	}

      pFcb = FileObject->FsContext;

      /*
       * Check the file has the requested attributes
       */
      if (RequestedOptions & FILE_NON_DIRECTORY_FILE && 
	  pFcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY)
	{
	  VfatCloseFile (DeviceExt, FileObject);
	  return(STATUS_FILE_IS_A_DIRECTORY);
	}
      if (RequestedOptions & FILE_DIRECTORY_FILE && 
	  !(pFcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY))
	{
	  VfatCloseFile (DeviceExt, FileObject);
	  return(STATUS_NOT_A_DIRECTORY);
	}

      if (PagingFileCreate)
        {
	  /* FIXME:
	   *   Do more checking for page files. It is possible, 
	   *   that the file was opened and closed previously 
	   *   as a normal cached file. In this case, the cache 
	   *   manager has referenced the fileobject and the fcb 
	   *   is held in memory. Try to remove the fileobject 
	   *   from cache manager and use the fcb.
	   */
          if (pFcb->RefCount > 1)
	    {
	      if(!(pFcb->Flags & FCB_IS_PAGE_FILE))
	        {
	          VfatCloseFile(DeviceExt, FileObject);
	          return(STATUS_INVALID_PARAMETER);
		}
	    }
	  else
	    {
	      pFcb->Flags |= FCB_IS_PAGE_FILE;
	    }
	}
      else
        {
	  if (pFcb->Flags & FCB_IS_PAGE_FILE)
	    {
	      VfatCloseFile(DeviceExt, FileObject);
	      return(STATUS_INVALID_PARAMETER);
	    }
	}
        

      if (RequestedDisposition == FILE_OVERWRITE ||
          RequestedDisposition == FILE_OVERWRITE_IF)
	{
	  AllocationSize.QuadPart = 0;
	  Status = VfatSetAllocationSizeInformation (FileObject,
	                                             pFcb,
	                                             DeviceExt,
	                                             &AllocationSize);
	  if (!NT_SUCCESS (Status))
	    {
	      VfatCloseFile (DeviceExt, FileObject);
	      return(Status);
	    }
	}
	
      
      /* Supersede the file */
      if (RequestedDisposition == FILE_SUPERSEDE)
	{
	  VfatSupersedeFile(DeviceExt, FileObject, pFcb);
	  Irp->IoStatus.Information = FILE_SUPERSEDED;
	}
      else if (RequestedDisposition == FILE_OVERWRITE || RequestedDisposition == FILE_OVERWRITE_IF)
  {
    Irp->IoStatus.Information = FILE_OVERWRITTEN;
  }
      else
  {
	  Irp->IoStatus.Information = FILE_OPENED;
	}
    }
  
  /* FIXME : test share access */
  /* FIXME : test write access if requested */

  return(Status);
}


NTSTATUS VfatCreate (PVFAT_IRP_CONTEXT IrpContext)
/*
 * FUNCTION: Create or open a file
 */
{
  NTSTATUS Status;
  
  assert (IrpContext);
  
  if (IrpContext->DeviceObject == VfatGlobalData->DeviceObject)
    {
      /* DeviceObject represents FileSystem instead of logical volume */
      DPRINT ("FsdCreate called with file system\n");
      IrpContext->Irp->IoStatus.Information = FILE_OPENED;
      IrpContext->Irp->IoStatus.Status = STATUS_SUCCESS;
      IoCompleteRequest (IrpContext->Irp, IO_DISK_INCREMENT);
      VfatFreeIrpContext(IrpContext);
      return(STATUS_SUCCESS);
    }
  
  if (!(IrpContext->Flags & IRPCONTEXT_CANWAIT))
    {
      return(VfatQueueRequest (IrpContext));
    }
  
  IrpContext->Irp->IoStatus.Information = 0;
  ExAcquireResourceExclusiveLite (&IrpContext->DeviceExt->DirResource, TRUE);
  Status = VfatCreateFile (IrpContext->DeviceObject, IrpContext->Irp);
  ExReleaseResourceLite (&IrpContext->DeviceExt->DirResource);

  IrpContext->Irp->IoStatus.Status = Status;
  IoCompleteRequest (IrpContext->Irp, 
		     (CCHAR)(NT_SUCCESS(Status) ? IO_DISK_INCREMENT : IO_NO_INCREMENT));
  VfatFreeIrpContext(IrpContext);
  return(Status);
}

/* EOF */
