/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
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
/* $Id: fcb.c,v 1.8 2002/09/07 15:12:00 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * FILE:             services/fs/cdfs/fcb.c
 * PURPOSE:          CDROM (ISO 9660) filesystem driver
 * PROGRAMMER:       Art Yerkes
 * UPDATE HISTORY: 
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>

#define NDEBUG
#include <debug.h>

#include "cdfs.h"


/* MACROS *******************************************************************/

#ifndef TAG
#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#endif

#define TAG_FCB TAG('I', 'F', 'C', 'B')

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))


/* FUNCTIONS ****************************************************************/

static PWCHAR
CdfsGetNextPathElement(PWCHAR FileName)
{
  if (*FileName == L'\0')
    {
      return(NULL);
    }

  while (*FileName != L'\0' && *FileName != L'\\')
    {
      FileName++;
    }

  return(FileName);
}


static VOID
CdfsWSubString(PWCHAR pTarget, const PWCHAR pSource, size_t pLength)
{
  wcsncpy (pTarget, pSource, pLength);
  pTarget [pLength] = L'\0';
}


PFCB
CdfsCreateFCB(PWSTR FileName)
{
  PFCB Fcb;

  Fcb = ExAllocatePoolWithTag(NonPagedPool, sizeof(FCB), TAG_FCB);
  RtlZeroMemory(Fcb, sizeof(FCB));

  if (FileName)
    {
      wcscpy(Fcb->PathName, FileName);
      if (wcsrchr(Fcb->PathName, '\\') != 0)
	{
	  Fcb->ObjectName = wcsrchr(Fcb->PathName, '\\');
	}
      else
	{
	  Fcb->ObjectName = Fcb->PathName;
	}
    }

  ExInitializeResourceLite(&Fcb->MainResource);

  return(Fcb);
}


VOID
CdfsDestroyFCB(PFCB Fcb)
{
  ExDeleteResourceLite(&Fcb->MainResource);

  ExFreePool(Fcb);
}


BOOLEAN
CdfsFCBIsDirectory(PFCB Fcb)
{
//  return(Fcb->entry.Attrib & FILE_ATTRIBUTE_DIRECTORY);
  return(Fcb->Entry.FileFlags & 0x02);
}


BOOLEAN
CdfsFCBIsRoot(PFCB Fcb)
{
  return(wcscmp(Fcb->PathName, L"\\") == 0);
}


VOID
CdfsGrabFCB(PDEVICE_EXTENSION Vcb,
	    PFCB Fcb)
{
  KIRQL  oldIrql;

  DPRINT("grabbing FCB at %x: %S, refCount:%d\n",
	 Fcb,
	 Fcb->PathName,
	 Fcb->RefCount);

  KeAcquireSpinLock(&Vcb->FcbListLock, &oldIrql);
  Fcb->RefCount++;
  KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);
}


VOID
CdfsReleaseFCB(PDEVICE_EXTENSION Vcb,
	       PFCB Fcb)
{
  KIRQL  oldIrql;

  DPRINT("releasing FCB at %x: %S, refCount:%d\n",
	 Fcb,
	 Fcb->PathName,
	 Fcb->RefCount);

  KeAcquireSpinLock(&Vcb->FcbListLock, &oldIrql);
  Fcb->RefCount--;
  if (Fcb->RefCount <= 0 && !CdfsFCBIsDirectory(Fcb))
    {
      RemoveEntryList(&Fcb->FcbListEntry);
      CcRosReleaseFileCache(NULL, Fcb->RFCB.Bcb);
      CdfsDestroyFCB(Fcb);
    }
  KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);
}


VOID
CdfsAddFCBToTable(PDEVICE_EXTENSION Vcb,
		  PFCB Fcb)
{
  KIRQL  oldIrql;

  KeAcquireSpinLock(&Vcb->FcbListLock, &oldIrql);
  Fcb->DevExt = Vcb;
  InsertTailList(&Vcb->FcbListHead, &Fcb->FcbListEntry);
  KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);
}


PFCB
CdfsGrabFCBFromTable(PDEVICE_EXTENSION Vcb,
		     PWSTR FileName)
{
  KIRQL  oldIrql;
  PFCB Fcb;
  PLIST_ENTRY  current_entry;

  KeAcquireSpinLock(&Vcb->FcbListLock, &oldIrql);

  if (FileName == NULL || *FileName == 0)
    {
      DPRINT("Return FCB for stream file object\n");
      Fcb = ((PCCB)Vcb->StreamFileObject->FsContext2)->Fcb;
      Fcb->RefCount++;
      KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);
      return(Fcb);
    }

  current_entry = Vcb->FcbListHead.Flink;
  while (current_entry != &Vcb->FcbListHead)
    {
      Fcb = CONTAINING_RECORD(current_entry, FCB, FcbListEntry);

      DPRINT("Comparing '%S' and '%S'\n", FileName, Fcb->PathName);
      if (_wcsicmp(FileName, Fcb->PathName) == 0)
	{
	  Fcb->RefCount++;
	  KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);
	  return(Fcb);
	}

      //FIXME: need to compare against short name in FCB here

      current_entry = current_entry->Flink;
    }
  KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);

  return(NULL);
}


NTSTATUS
CdfsFCBInitializeCache(PVCB Vcb,
		       PFCB Fcb)
{
  PFILE_OBJECT FileObject;
  NTSTATUS Status;
  PCCB  newCCB;

  FileObject = IoCreateStreamFileObject(NULL, Vcb->StorageDevice);

  newCCB = ExAllocatePoolWithTag(NonPagedPool, sizeof(CCB), TAG_CCB);
  if (newCCB == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }
  RtlZeroMemory(newCCB,
		sizeof(CCB));

  FileObject->Flags = FileObject->Flags | FO_FCB_IS_VALID |
      FO_DIRECT_CACHE_PAGING_READ;
  FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
  FileObject->FsContext = (PVOID) &Fcb->RFCB;
  FileObject->FsContext2 = newCCB;
  newCCB->Fcb = Fcb;
  newCCB->PtrFileObject = FileObject;
  Fcb->FileObject = FileObject;
  Fcb->DevExt = Vcb;

  Status = CcRosInitializeFileCache(FileObject,
				    &Fcb->RFCB.Bcb,
				    PAGE_SIZE);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("CcRosInitializeFileCache failed\n");
      KeBugCheck(0);
    }

  ObDereferenceObject(FileObject);
  Fcb->Flags |= FCB_CACHE_INITIALIZED;

  return(Status);
}


PFCB
CdfsMakeRootFCB(PDEVICE_EXTENSION Vcb)
{
  PFCB Fcb;

  Fcb = CdfsCreateFCB(L"\\");

  Fcb->Entry.DataLengthL = Vcb->CdInfo.RootSize;
  Fcb->Entry.ExtentLocationL = Vcb->CdInfo.RootStart;
  Fcb->Entry.FileFlags = 0x02; // FILE_ATTRIBUTE_DIRECTORY;
  Fcb->RefCount = 1;
  Fcb->DirIndex = 0;
  Fcb->RFCB.FileSize.QuadPart = Vcb->CdInfo.RootSize;
  Fcb->RFCB.ValidDataLength.QuadPart = Vcb->CdInfo.RootSize;
  Fcb->RFCB.AllocationSize.QuadPart = Vcb->CdInfo.RootSize;

  CdfsFCBInitializeCache(Vcb, Fcb);
  CdfsAddFCBToTable(Vcb, Fcb);
  CdfsGrabFCB(Vcb, Fcb);

  return(Fcb);
}


PFCB
CdfsOpenRootFCB(PDEVICE_EXTENSION Vcb)
{
  PFCB Fcb;

  Fcb = CdfsGrabFCBFromTable(Vcb, L"\\");
  if (Fcb == NULL)
    {
      Fcb = CdfsMakeRootFCB(Vcb);
    }

  return(Fcb);
}


static VOID
CdfsGetDirEntryName(PDEVICE_EXTENSION DeviceExt,
		    PDIR_RECORD Record,
		    PWSTR Name)
/*
 * FUNCTION: Retrieves the file name from a directory record.
 */
{
  if (Record->FileIdLength == 1 && Record->FileId[0] == 0)
    {
      wcscpy(Name, L".");
    }
  else if (Record->FileIdLength == 1 && Record->FileId[0] == 1)
    {
      wcscpy(Name, L"..");
    }
  else
    {
      if (DeviceExt->CdInfo.JolietLevel == 0)
	{
	  ULONG i;

	  for (i = 0; i < Record->FileIdLength && Record->FileId[i] != ';'; i++)
	    Name[i] = (WCHAR)Record->FileId[i];
	  Name[i] = 0;
	}
      else
	{
	  CdfsSwapString(Name,
			 Record->FileId,
			 Record->FileIdLength);
	}
    }

  DPRINT("Name '%S'\n", Name);
}


NTSTATUS
CdfsMakeFCBFromDirEntry(PVCB Vcb,
			PFCB DirectoryFCB,
			PWSTR LongName,
			PWSTR ShortName,
			PDIR_RECORD Record,
			PFCB * fileFCB)
{
  WCHAR pathName[MAX_PATH];
  PFCB rcFCB;
  ULONG Size;

  if (LongName [0] != 0 && wcslen (DirectoryFCB->PathName) +
        sizeof(WCHAR) + wcslen (LongName) > MAX_PATH)
    {
      return(STATUS_OBJECT_NAME_INVALID);
    }

  wcscpy(pathName, DirectoryFCB->PathName);
  if (!CdfsFCBIsRoot(DirectoryFCB))
    {
      wcscat(pathName, L"\\");
    }

  if (LongName[0] != 0)
    {
      wcscat(pathName, LongName);
    }
  else
    {
      WCHAR entryName[MAX_PATH];

      CdfsGetDirEntryName(Vcb, Record, entryName);
      wcscat(pathName, entryName);
    }

  rcFCB = CdfsCreateFCB(pathName);
  memcpy(&rcFCB->Entry, Record, sizeof(DIR_RECORD));

  /* Copy short name into FCB */
  rcFCB->ShortNameLength = wcslen(ShortName) * sizeof(WCHAR);
  wcscpy(rcFCB->ShortName, ShortName);

  Size = rcFCB->Entry.DataLengthL;

  rcFCB->RFCB.FileSize.QuadPart = Size;
  rcFCB->RFCB.ValidDataLength.QuadPart = Size;
  rcFCB->RFCB.AllocationSize.QuadPart = ROUND_UP(Size, BLOCKSIZE);
  CdfsFCBInitializeCache(Vcb, rcFCB);
  rcFCB->RefCount++;
  CdfsAddFCBToTable(Vcb, rcFCB);
  *fileFCB = rcFCB;

  DPRINT("%S %d %I64d\n", LongName, Size, rcFCB->RFCB.AllocationSize.QuadPart);

  return(STATUS_SUCCESS);
}


NTSTATUS
CdfsAttachFCBToFileObject(PDEVICE_EXTENSION Vcb,
			  PFCB Fcb,
			  PFILE_OBJECT FileObject)
{
  NTSTATUS Status;
  PCCB  newCCB;

  newCCB = ExAllocatePoolWithTag(NonPagedPool, sizeof(CCB), TAG_CCB);
  if (newCCB == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }
  memset(newCCB, 0, sizeof(CCB));

  FileObject->Flags = FileObject->Flags | FO_FCB_IS_VALID |
      FO_DIRECT_CACHE_PAGING_READ;
  FileObject->SectionObjectPointer = &Fcb->SectionObjectPointers;
  FileObject->FsContext = (PVOID)&Fcb->RFCB;
  FileObject->FsContext2 = newCCB;
  newCCB->Fcb = Fcb;
  newCCB->PtrFileObject = FileObject;
  Fcb->DevExt = Vcb;

  if (!(Fcb->Flags & FCB_CACHE_INITIALIZED))
    {
      Status = CcRosInitializeFileCache(FileObject,
					&Fcb->RFCB.Bcb,
					PAGE_SIZE);
      if (!NT_SUCCESS(Status))
	{
	  DbgPrint("CcRosInitializeFileCache failed\n");
	  KeBugCheck(0);
	}
      Fcb->Flags |= FCB_CACHE_INITIALIZED;
    }

  DPRINT("file open: fcb:%x file size: %d\n", Fcb, Fcb->Entry.DataLengthL);

  return(STATUS_SUCCESS);
}


NTSTATUS
CdfsDirFindFile(PDEVICE_EXTENSION DeviceExt,
		PFCB DirectoryFcb,
		PWSTR FileToFind,
		PFCB *FoundFCB)
{
  WCHAR TempName[2];
  WCHAR Name[256];
  PVOID Block;
  ULONG FirstSector;
  ULONG DirSize;
  PDIR_RECORD Record;
  ULONG Offset;
  ULONG BlockOffset;
  NTSTATUS Status;

  LARGE_INTEGER StreamOffset;
  PVOID Context;

  WCHAR ShortNameBuffer[13];
  UNICODE_STRING ShortName;
  UNICODE_STRING LongName;
  BOOLEAN HasSpaces;
  GENERATE_NAME_CONTEXT NameContext;


  assert(DeviceExt);
  assert(DirectoryFcb);
  assert(FileToFind);

  DPRINT("CdfsDirFindFile(VCB:%08x, dirFCB:%08x, File:%S)\n",
	 DeviceExt,
	 DirectoryFcb,
	 FileToFind);
  DPRINT("Dir Path:%S\n", DirectoryFcb->PathName);

  /*  default to '.' if no filename specified */
  if (wcslen(FileToFind) == 0)
    {
      TempName[0] = L'.';
      TempName[1] = 0;
      FileToFind = TempName;
    }

  DirSize = DirectoryFcb->Entry.DataLengthL;
  StreamOffset.QuadPart = (LONGLONG)DirectoryFcb->Entry.ExtentLocationL * (LONGLONG)BLOCKSIZE;

  if(!CcMapData(DeviceExt->StreamFileObject, &StreamOffset,
		BLOCKSIZE, TRUE, &Context, &Block))
  {
    DPRINT("CcMapData() failed\n");
    return(STATUS_UNSUCCESSFUL);
  }

  Offset = 0;
  BlockOffset = 0;
  Record = (PDIR_RECORD)Block;
  while(TRUE)
    {
      if (Record->RecordLength == 0)
	{
	  DPRINT("RecordLength == 0  Stopped!\n");
	  break;
	}
	
      DPRINT("RecordLength %u  ExtAttrRecordLength %u  NameLength %u\n",
	     Record->RecordLength, Record->ExtAttrRecordLength, Record->FileIdLength);

      CdfsGetDirEntryName(DeviceExt, Record, Name);
      DPRINT("Name '%S'\n", Name);

      RtlInitUnicodeString(&LongName, Name);
      ShortName.Length = 0;
      ShortName.MaximumLength = 26;
      ShortName.Buffer = ShortNameBuffer;
      memset(ShortNameBuffer, 0, 26);

      if ((RtlIsNameLegalDOS8Dot3(&LongName, NULL, &HasSpaces) == FALSE) ||
	  (HasSpaces == TRUE))
	{
	  /* Build short name */
	  RtlGenerate8dot3Name(&LongName,
			       FALSE,
			       &NameContext,
			       &ShortName);
	}
      else
	{
	  /* copy short name */
	  RtlUpcaseUnicodeString(&ShortName,
				 &LongName,
				 FALSE);
	}

      DPRINT("ShortName '%wZ'\n", &ShortName);

      if (wstrcmpjoki(Name, FileToFind) || wstrcmpjoki(ShortNameBuffer, FileToFind))
	{
	  DPRINT("Match found, %S\n", Name);
	  Status = CdfsMakeFCBFromDirEntry(DeviceExt,
					   DirectoryFcb,
					   Name,
					   ShortNameBuffer,
					   Record,
					   FoundFCB);

	  CcUnpinData(Context);

	  return(Status);
	}

      Offset += Record->RecordLength;
      BlockOffset += Record->RecordLength;
      Record = (PDIR_RECORD)(Block + BlockOffset);
      if (BlockOffset >= BLOCKSIZE || Record->RecordLength == 0)
	{
	  DPRINT("Map next sector\n");
	  CcUnpinData(Context);
	  StreamOffset.QuadPart += BLOCKSIZE;
	  Offset = ROUND_UP(Offset, BLOCKSIZE);
	  BlockOffset = 0;

	  if (!CcMapData(DeviceExt->StreamFileObject,
			 &StreamOffset,
			 BLOCKSIZE, TRUE,
			 &Context, &Block))
	    {
	      DPRINT("CcMapData() failed\n");
	      return(STATUS_UNSUCCESSFUL);
	    }
	  Record = (PDIR_RECORD)(Block + BlockOffset);
	}

      if (Offset >= DirSize)
	break;
    }

  CcUnpinData(Context);

  return(STATUS_OBJECT_NAME_NOT_FOUND);
}


NTSTATUS
CdfsGetFCBForFile(PDEVICE_EXTENSION Vcb,
		  PFCB *pParentFCB,
		  PFCB *pFCB,
		  const PWSTR pFileName)
{
  NTSTATUS Status;
  WCHAR  pathName [MAX_PATH];
  WCHAR  elementName [MAX_PATH];
  PWCHAR  currentElement;
  PFCB  FCB;
  PFCB  parentFCB;

  DPRINT("CdfsGetFCBForFile(%x, %x, %x, '%S')\n",
	 Vcb,
	 pParentFCB,
	 pFCB,
	 pFileName);

  /* Trivial case, open of the root directory on volume */
  if (pFileName [0] == L'\0' || wcscmp(pFileName, L"\\") == 0)
    {
      DPRINT("returning root FCB\n");

      FCB = CdfsOpenRootFCB(Vcb);
      *pFCB = FCB;
      *pParentFCB = NULL;

      return((FCB != NULL) ? STATUS_SUCCESS : STATUS_OBJECT_PATH_NOT_FOUND);
    }
  else
    {
      currentElement = pFileName + 1;
      wcscpy (pathName, L"\\");
      FCB = CdfsOpenRootFCB (Vcb);
    }
  parentFCB = NULL;

  /* Parse filename and check each path element for existance and access */
  while (CdfsGetNextPathElement(currentElement) != 0)
    {
      /*  Skip blank directory levels */
      if ((CdfsGetNextPathElement(currentElement) - currentElement) == 0)
	{
	  currentElement++;
	  continue;
	}

      DPRINT("Parsing, currentElement:%S\n", currentElement);
      DPRINT("  parentFCB:%x FCB:%x\n", parentFCB, FCB);

      /* Descend to next directory level */
      if (parentFCB)
	{
	  CdfsReleaseFCB(Vcb, parentFCB);
	  parentFCB = NULL;
	}

      /* fail if element in FCB is not a directory */
      if (!CdfsFCBIsDirectory(FCB))
	{
	  DPRINT("Element in requested path is not a directory\n");

	  CdfsReleaseFCB(Vcb, FCB);
	  FCB = 0;
	  *pParentFCB = NULL;
	  *pFCB = NULL;

	  return(STATUS_OBJECT_PATH_NOT_FOUND);
	}
      parentFCB = FCB;

      /* Extract next directory level into dirName */
      CdfsWSubString(pathName,
		     pFileName,
		     CdfsGetNextPathElement(currentElement) - pFileName);
      DPRINT("  pathName:%S\n", pathName);

      FCB = CdfsGrabFCBFromTable(Vcb, pathName);
      if (FCB == NULL)
	{
	  CdfsWSubString(elementName,
			 currentElement,
			 CdfsGetNextPathElement(currentElement) - currentElement);
	  DPRINT("  elementName:%S\n", elementName);

	  Status = CdfsDirFindFile(Vcb, parentFCB, elementName, &FCB);
	  if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
	    {
	      *pParentFCB = parentFCB;
	      *pFCB = NULL;
	      currentElement = CdfsGetNextPathElement(currentElement);
	      if (*currentElement == L'\0' || CdfsGetNextPathElement(currentElement + 1) == 0)
		{
		  return(STATUS_OBJECT_NAME_NOT_FOUND);
		}
	      else
		{
		  return(STATUS_OBJECT_PATH_NOT_FOUND);
		}
	    }
	  else if (!NT_SUCCESS(Status))
	    {
	      CdfsReleaseFCB(Vcb, parentFCB);
	      *pParentFCB = NULL;
	      *pFCB = NULL;

	      return(Status);
	    }
	}
      currentElement = CdfsGetNextPathElement(currentElement);
    }

  *pParentFCB = parentFCB;
  *pFCB = FCB;

  return(STATUS_SUCCESS);
}

/* EOF */
