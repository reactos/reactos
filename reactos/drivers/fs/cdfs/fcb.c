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
/* $Id: fcb.c,v 1.2 2002/04/15 20:39:49 ekohl Exp $
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

//#define NDEBUG
#include <debug.h>

#include "cdfs.h"


/* MACROS *******************************************************************/

#define TAG(A, B, C, D) (ULONG)(((A)<<0) + ((B)<<8) + ((C)<<16) + ((D)<<24))
#define TAG_FCB TAG('I', 'F', 'C', 'B')

#define ROUND_UP(N, S) ((((N) + (S) - 1) / (S)) * (S))


/* FUNCTIONS ****************************************************************/

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
      DPRINT1("Return FCB for strem file object\n");
      Fcb = ((PCCB)Vcb->StreamFileObject->FsContext2)->Fcb;
      Fcb->RefCount++;
      KeReleaseSpinLock(&Vcb->FcbListLock, oldIrql);
      return(Fcb);
    }

  current_entry = Vcb->FcbListHead.Flink;
  while (current_entry != &Vcb->FcbListHead)
    {
      Fcb = CONTAINING_RECORD(current_entry, FCB, FcbListEntry);

//      if (wstrcmpi(FileName, Fcb->PathName))
      if (_wcsicmp(FileName, Fcb->PathName))
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
  FileObject->SectionObjectPointers = &Fcb->SectionObjectPointers;
  FileObject->FsContext = (PVOID) &Fcb->RFCB;
  FileObject->FsContext2 = newCCB;
  newCCB->Fcb = Fcb;
  newCCB->PtrFileObject = FileObject;
  Fcb->FileObject = FileObject;
  Fcb->DevExt = Vcb;

  Status = CcRosInitializeFileCache(FileObject,
                                    &Fcb->RFCB.Bcb,
                                    PAGESIZE);
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

//  memset(Fcb->entry.Filename, ' ', 11);

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


#if 0
NTSTATUS
vfatMakeFCBFromDirEntry(PVCB  vcb,
			PVFATFCB  directoryFCB,
			PWSTR  longName,
			PFAT_DIR_ENTRY  dirEntry,
            ULONG dirIndex,
			PVFATFCB * fileFCB)
{
  PVFATFCB  rcFCB;
  WCHAR  pathName [MAX_PATH];
  ULONG Size;
  if (longName [0] != 0 && wcslen (directoryFCB->PathName) +
        sizeof(WCHAR) + wcslen (longName) > MAX_PATH)
  {
    return  STATUS_OBJECT_NAME_INVALID;
  }
  wcscpy (pathName, directoryFCB->PathName);
  if (!vfatFCBIsRoot (directoryFCB))
  {
    wcscat (pathName, L"\\");
  }
  if (longName [0] != 0)
  {
    wcscat (pathName, longName);
  }
  else
  {
    WCHAR  entryName [MAX_PATH];

    vfatGetDirEntryName (dirEntry, entryName);
    wcscat (pathName, entryName);
  }
  rcFCB = vfatNewFCB (pathName);
  memcpy (&rcFCB->entry, dirEntry, sizeof (FAT_DIR_ENTRY));

  if (vfatFCBIsDirectory(vcb, rcFCB))
  {
    ULONG FirstCluster, CurrentCluster;
    NTSTATUS Status;
    Size = 0;
    FirstCluster = vfatDirEntryGetFirstCluster (vcb, &rcFCB->entry);
    if (FirstCluster == 1)
    {
      Size = vcb->FatInfo.rootDirectorySectors * BLOCKSIZE;
    }
    else
    {
      CurrentCluster = FirstCluster;
      while (CurrentCluster != 0xffffffff)
      {
         Size += vcb->FatInfo.BytesPerCluster;
         Status = NextCluster (vcb, NULL, FirstCluster, &CurrentCluster, FALSE);
      }
    }
  }
  else
  {
    Size = rcFCB->entry.FileSize;
  }
  rcFCB->dirIndex = dirIndex;
  rcFCB->RFCB.FileSize.QuadPart = Size;
  rcFCB->RFCB.ValidDataLength.QuadPart = Size;
  rcFCB->RFCB.AllocationSize.QuadPart = ROUND_UP(Size, vcb->FatInfo.BytesPerCluster);
//  DPRINT1("%S %d %d\n", longName, Size, (ULONG)rcFCB->RFCB.AllocationSize.QuadPart);
  vfatFCBInitializeCache (vcb, rcFCB);
  rcFCB->RefCount++;
  vfatAddFCBToTable (vcb, rcFCB);
  *fileFCB = rcFCB;

  return  STATUS_SUCCESS;
}
#endif

NTSTATUS
CdfsAttachFCBToFileObject(PDEVICE_EXTENSION Vcb,
			  PFCB Fcb,
			  PFILE_OBJECT FileObject)
{
  NTSTATUS Status;
  PCCB  newCCB;

  newCCB = ExAllocatePoolWithTag (NonPagedPool, sizeof(CCB), TAG_CCB);
  if (newCCB == NULL)
    {
      return(STATUS_INSUFFICIENT_RESOURCES);
    }
  memset(newCCB, 0, sizeof(CCB));

  FileObject->Flags = FileObject->Flags | FO_FCB_IS_VALID |
      FO_DIRECT_CACHE_PAGING_READ;
  FileObject->SectionObjectPointers = &Fcb->SectionObjectPointers;
  FileObject->FsContext = (PVOID)&Fcb->RFCB;
  FileObject->FsContext2 = newCCB;
  newCCB->Fcb = Fcb;
  newCCB->PtrFileObject = FileObject;
  Fcb->DevExt = Vcb;

  if (!(Fcb->Flags & FCB_CACHE_INITIALIZED))
    {
      Status = CcRosInitializeFileCache(FileObject,
					&Fcb->RFCB.Bcb,
					PAGESIZE);
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

#if 0
NTSTATUS
vfatDirFindFile (PDEVICE_EXTENSION  pDeviceExt,
                 PVFATFCB  pDirectoryFCB,
                 PWSTR  pFileToFind,
                 PVFATFCB * pFoundFCB)
{
  BOOL  finishedScanningDirectory;
  ULONG  directoryIndex;
  NTSTATUS  status;
  WCHAR  defaultFileName [2];
  WCHAR  currentLongName [256];
  FAT_DIR_ENTRY  currentDirEntry;
  WCHAR  currentEntryName [256];

  assert (pDeviceExt);
  assert (pDirectoryFCB);
  assert (pFileToFind);

  DPRINT ("vfatDirFindFile(VCB:%08x, dirFCB:%08x, File:%S)\n",
          pDeviceExt,
          pDirectoryFCB,
          pFileToFind);
  DPRINT ("Dir Path:%S\n", pDirectoryFCB->PathName);

  //  default to '.' if no filename specified
  if (wcslen (pFileToFind) == 0)
  {
    defaultFileName [0] = L'.';
    defaultFileName [1] = 0;
    pFileToFind = defaultFileName;
  }

  directoryIndex = 0;
  finishedScanningDirectory = FALSE;
  while (!finishedScanningDirectory)
  {
    status = vfatGetNextDirEntry (pDeviceExt,
                                  pDirectoryFCB,
                                  &directoryIndex,
                                  currentLongName,
                                  &currentDirEntry);
    if (status == STATUS_NO_MORE_ENTRIES)
    {
      finishedScanningDirectory = TRUE;
      continue;
    }
    else if (!NT_SUCCESS(status))
    {
      return  status;
    }

    DPRINT ("  Index:%d  longName:%S\n",
            directoryIndex,
            currentLongName);

    if (!vfatIsDirEntryDeleted (&currentDirEntry)
      && !vfatIsDirEntryVolume(&currentDirEntry))
    {
      if (currentLongName [0] != L'\0' && wstrcmpjoki (currentLongName, pFileToFind))
      {
        DPRINT ("Match found, %S\n", currentLongName);
        status = vfatMakeFCBFromDirEntry (pDeviceExt,
                                          pDirectoryFCB,
                                          currentLongName,
                                          &currentDirEntry,
                                          directoryIndex - 1,
                                          pFoundFCB);
        return  status;
      }
      else
      {
        vfatGetDirEntryName (&currentDirEntry, currentEntryName);
        DPRINT ("  entryName:%S\n", currentEntryName);

        if (wstrcmpjoki (currentEntryName, pFileToFind))
        {
          DPRINT ("Match found, %S\n", currentEntryName);
          status = vfatMakeFCBFromDirEntry (pDeviceExt,
                                            pDirectoryFCB,
                                            currentLongName,
                                            &currentDirEntry,
                                            directoryIndex - 1,
                                            pFoundFCB);
          return  status;
        }
      }
    }
  }

  return  STATUS_OBJECT_NAME_NOT_FOUND;
}
#endif


NTSTATUS
CdfsGetFCBForFile(PDEVICE_EXTENSION Vcb,
		  PFCB *pParentFCB,
		  PFCB *pFCB,
		  const PWSTR pFileName)
{
  NTSTATUS  status;
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

  DPRINT1("CdfsGetFCBForFile() is incomplete!\n");
  return(STATUS_UNSUCCESSFUL);

#if 0
  else
    {
      currentElement = pFileName + 1;
      wcscpy (pathName, L"\\");
      FCB = CdfsOpenRootFCB (Vcb);
    }
  parentFCB = NULL;

  //  Parse filename and check each path element for existance and access
  while (vfatGetNextPathElement (currentElement) != 0)
  {
    //  Skip blank directory levels
    if ((vfatGetNextPathElement (currentElement) - currentElement) == 0)
    {
      currentElement++;
      continue;
    }

    DPRINT ("Parsing, currentElement:%S\n", currentElement);
    DPRINT ("  parentFCB:%x FCB:%x\n", parentFCB, FCB);

    //  descend to next directory level
    if (parentFCB)
    {
      vfatReleaseFCB (pVCB, parentFCB);
      parentFCB = 0;
    }
    //  fail if element in FCB is not a directory
    if (!vfatFCBIsDirectory (pVCB, FCB))
    {
      DPRINT ("Element in requested path is not a directory\n");

      vfatReleaseFCB (pVCB, FCB);
      FCB = 0;
      *pParentFCB = NULL;
      *pFCB = NULL;

      return  STATUS_OBJECT_PATH_NOT_FOUND;
    }
    parentFCB = FCB;

    //  Extract next directory level into dirName
    vfatWSubString (pathName,
                    pFileName,
                    vfatGetNextPathElement (currentElement) - pFileName);
    DPRINT ("  pathName:%S\n", pathName);

    FCB = vfatGrabFCBFromTable (pVCB, pathName);
    if (FCB == NULL)
    {
      vfatWSubString (elementName,
                      currentElement,
                      vfatGetNextPathElement (currentElement) - currentElement);
      DPRINT ("  elementName:%S\n", elementName);

      status = vfatDirFindFile (pVCB, parentFCB, elementName, &FCB);
      if (status == STATUS_OBJECT_NAME_NOT_FOUND)
      {
        *pParentFCB = parentFCB;
        *pFCB = NULL;
        currentElement = vfatGetNextPathElement(currentElement);
        if (*currentElement == L'\0' || vfatGetNextPathElement(currentElement + 1) == 0)
        {
          return  STATUS_OBJECT_NAME_NOT_FOUND;
        }
        else
        {
          return  STATUS_OBJECT_PATH_NOT_FOUND;
        }
      }
      else if (!NT_SUCCESS (status))
      {
        vfatReleaseFCB (pVCB, parentFCB);
        *pParentFCB = NULL;
        *pFCB = NULL;

        return  status;
      }
    }
    currentElement = vfatGetNextPathElement (currentElement);
  }

  *pParentFCB = parentFCB;
  *pFCB = FCB;

  return  STATUS_SUCCESS;
#endif
}

/* EOF */
